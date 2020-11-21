#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "esp/hwrand.h"

#include <unistd.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/api.h"

#include "bearssl.h"
#include "./BearSSLTrustAnchors.h"

#include "../config.h"

#define CLOCK_SECONDS_PER_MINUTE (60UL)
#define CLOCK_MINUTES_PER_HOUR (60UL)
#define CLOCK_HOURS_PER_DAY (24UL)
#define CLOCK_SECONDS_PER_HOUR (CLOCK_MINUTES_PER_HOUR*CLOCK_SECONDS_PER_MINUTE)
#define CLOCK_SECONDS_PER_DAY (CLOCK_HOURS_PER_DAY*CLOCK_SECONDS_PER_HOUR)

// TODO: if dev
#include "gdbstub.h"

#define WEB_SERVER "openapi.etsy.com"
#define WEB_PORT "443"
#define WEB_URL "/v2/users/"ETSY_USER"/profile?api_key="ETSY_API_KEY

#define GET_REQUEST "GET "WEB_URL" HTTP/2\r\n\
Host: "WEB_SERVER"\r\n\
User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:82.0) Gecko/20100101 Firefox/82.0\r\n\
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n\
Accept-Language: en-US,en;q=0.5\r\n\
Accept-Encoding: gzip, deflate, br\r\n\
Connection: keep-alive\r\n\
Upgrade-Insecure-Requests: 1\r\n\
Pragma: no-cache\r\n\
Cache-Control: no-cache\r\n\
\r\n"

// Low-level data read callback for the simplified SSL I/O API.
static int sock_read(void *ctx, unsigned char *buf, size_t len) {
  for (;;) {
    ssize_t rlen;

    rlen = read(*(int *)ctx, buf, len);
    if (rlen <= 0) {
      if (rlen < 0 && errno == EINTR) {
        continue;
      }
      return -1;
    }
    return (int)rlen;
  }
}


// Low-level data write callback for the simplified SSL I/O API.
static int sock_write(void *ctx, const unsigned char *buf, size_t len) {
  for (;;) {
    ssize_t wlen;

    wlen = write(*(int *)ctx, buf, len);
    if (wlen <= 0) {
      if (wlen < 0 && errno == EINTR) {
        continue;
      }
      return -1;
    }
    return (int)wlen;
  }
}

/*
 * The hardcoded trust anchors. These are the two DN + public key that
 * correspond to the self-signed certificates cert-root-rsa.pem and
 * cert-root-ec.pem.
 *
 * C code for hardcoded trust anchors can be generated with the "brssl"
 * command-line tool (with the "ta" command). To build that tool run:
 *
 * $ cd /path/to/esp-open-rtos/extras/bearssl/BearSSL
 * $ make build/brssl
 *
 * Below is the imported "Let's Encrypt" root certificate, as howsmyssl
 * is depending on it:
 *
 * https://letsencrypt.org/certs/letsencryptauthorityx3.pem
 *
 * The generate the trust anchor code below, run:
 *
 * $ /path/to/esp-open-rtos/extras/bearssl/BearSSL/build/brssl \
 *   ta letsencryptauthorityx3.pem
 *
 * To get the server certificate for a given https host:
 *
 * $ openssl s_client -showcerts -servername www.howsmyssl.com \
 *   -connect www.howsmyssl.com:443 < /dev/null | \
 *   openssl x509 -outform pem > server.pem
 *
 * Or just use this website: https://openslab-osu.github.io/bearssl-certificate-utility/
 * with the "domains to include" set to openapi.etsy.com
 */

/* Buffer to store a record + BearSSL state
 * We use MONO mode to save 16k of RAM.
 * This could be even smaller by using max_fragment_len, but
 * the etsy server doesn't seem to support it.
 */
static unsigned char bearssl_buffer[BR_SSL_BUFSIZE_MONO];

static br_ssl_client_context sc;
static br_x509_minimal_context xc;
static br_sslio_context ioc;

void http_get_task(void *pvParameters) {
  int successes = 0, failures = 0;
  int provisional_time = 0;

  printf(GET_REQUEST);

  while (1) {
     // Wait until we can resolve the DNS for the server, as an indication
     // our network is probably working...
    const struct addrinfo hints = {
      .ai_family = AF_INET,
      .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res = NULL;
    int dns_err = 0;
    do {
      if (res) {
        freeaddrinfo(res);
      }
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      dns_err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);
    } while(dns_err != 0 || res == NULL);

    int fd = socket(res->ai_family, res->ai_socktype, 0);
    if (fd < 0) {
      freeaddrinfo(res);
      printf("socket failed\n");
      failures++;
      continue;
    }

    printf("Initializing BearSSL... ");
    br_ssl_client_init_full(&sc, &xc, TAs, TAs_NUM);

    /*
     * Set the I/O buffer to the provided array. We allocated a
     * buffer large enough for full-duplex behaviour with all
     * allowed sizes of SSL records, hence we set the last argument
     * to 1 (which means "split the buffer into separate input and
     * output areas").
     */
    br_ssl_engine_set_buffer(&sc.eng, bearssl_buffer, sizeof bearssl_buffer, 0);

    // Inject some entropy from the ESP hardware RNG
    // This is necessary because we don't support any of the BearSSL methods
    for (int i = 0; i < 10; i++) {
      int rand = hwrand();
      br_ssl_engine_inject_entropy(&sc.eng, &rand, 4);
    }

    /*
     * Reset the client context, for a new handshake. We provide the
     * target host name: it will be used for the SNI extension. The
     * last parameter is 0: we are not trying to resume a session.
     */
    br_ssl_client_reset(&sc, WEB_SERVER, 0);

    /*
     * Initialise the simplified I/O wrapper context, to use our
     * SSL client context, and the two callbacks for socket I/O.
     */
    br_sslio_init(&ioc, &sc.eng, sock_read, &fd, sock_write, &fd);
    printf("done.\r\n");

    // FIXME: set date & time using epoch time precompiler flag for now */
    provisional_time = CONFIG_EPOCH_TIME + (xTaskGetTickCount()/configTICK_RATE_HZ);
    xc.days = (provisional_time / CLOCK_SECONDS_PER_DAY) + 719528;
    xc.seconds = provisional_time % CLOCK_SECONDS_PER_DAY;
    printf("Time: %02i:%02i\r\n",
        (int)(xc.seconds / CLOCK_SECONDS_PER_HOUR),
        (int)((xc.seconds % CLOCK_SECONDS_PER_HOUR)/CLOCK_SECONDS_PER_MINUTE)
        );

    if (connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
      close(fd);
      freeaddrinfo(res);
      printf("connect failed\n");
      failures++;
      continue;
    }
    printf("Connected\r\n");

    /*
     * Note that while the context has, at that point, already
     * assembled the ClientHello to send, nothing happened on the
     * network yet. Real I/O will occur only with the next call.
     *
     * We write our simple HTTP request. We test the call
     * for an error (-1), but this is not strictly necessary, since
     * the error state "sticks": if the context fails for any reason
     * (e.g. bad server certificate), then it will remain in failed
     * state and all subsequent calls will return -1 as well.
     */
    if (br_sslio_write_all(&ioc, GET_REQUEST, strlen(GET_REQUEST)) != BR_ERR_OK) {
      close(fd);
      freeaddrinfo(res);
      printf("br_sslio_write_all failed: %d\r\n", br_ssl_engine_last_error(&sc.eng));
      failures++;
      continue;
    }

    // SSL is a buffered protocol: we make sure that all our request
    // bytes are sent onto the wire.
    br_sslio_flush(&ioc);

    //Read and print the server response
    for (;;) {
      int rlen;
      unsigned char buf[128];

      // TODO: add gdbstub_do_break(); here?

      bzero(buf, 128);
      // Leave the final byte for zero termination
      rlen = br_sslio_read(&ioc, buf, sizeof(buf) - 1);

      if (rlen < 0) {
        break;
      }
      if (rlen > 0) {
        printf("%s", buf);
      }
    }

    // If reading the response failed for any reason, we detect it here
    if (br_ssl_engine_last_error(&sc.eng) != BR_ERR_OK) {
      close(fd);
      freeaddrinfo(res);
      printf("failure, error = %d\r\n", br_ssl_engine_last_error(&sc.eng));
      failures++;
      continue;
    }

    printf("\r\n\r\nfree heap pre  = %u\r\n", xPortGetFreeHeapSize());

    // Close the connection and start over after a delay
    close(fd);
    freeaddrinfo(res);

    printf("free heap post = %u\r\n", xPortGetFreeHeapSize());

    successes++;
    printf("successes = %d failures = %d\r\n", successes, failures);
    for(int countdown = 10; countdown >= 0; countdown--) {
      printf("%d...\n", countdown);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Starting again!\r\n\r\n");
  }
}

void user_init(void) {
  uart_set_baud(0, 115200);

  // TODO: if dev
  gdbstub_init();

  printf("SDK version:%s\n", sdk_system_get_sdk_version());

  struct sdk_station_config config = {
    .ssid = WIFI_SSID,
    .password = WIFI_PASS,
  };

  sdk_wifi_set_opmode(STATION_MODE);
  sdk_wifi_station_set_config(&config);

  xTaskCreate(&http_get_task, "get_task", 4096, NULL, 2, NULL);
}
