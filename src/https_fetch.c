#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/api.h"

#include "mbedtls/config.h"

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsmn.h"

#define WEB_SERVER_COMMON_NAME "etsy.com"
#define WEB_SERVER "openapi.etsy.com"
#define WEB_PORT "443"
#define WEB_URL "/v2/users/your_user_id_or_name/profile?api_key=your_api_key"

#define GET_REQUEST "GET "WEB_URL" HTTP/1.1\n\
Host: "WEB_SERVER"\n\
\n"

extern int orderCount;

// Root cert for openapi.etsy.com, stored in cert.c. See instructions there for setup.
extern const char *server_root_cert;

/* MBEDTLS_DEBUG_C disabled by default to save substantial bloating of
 * firmware, define it in
 * examples/http_get_mbedtls/include/mbedtls/config.h if you'd like
 * debugging output.
 */
#ifdef MBEDTLS_DEBUG_C

/* Increase this value to see more TLS debug details,
 * 0 prints nothing, 1 will print any errors, 4 will print _everything_
 */
#define DEBUG_LEVEL 1

static void print_debug(void *ctx, int level, const char *file, int line, const char *str) {
  ((void) level);

  /* Shorten 'file' from the whole file path to just the filename
   * This is a bit wasteful because the macros are compiled in with
   * the full _FILE_ path in each case, so the firmware is bloated out
   * by a few kb. But there's not a lot we can do about it...
   */
  char *file_sep = rindex(file, '/');
  if(file_sep)
    file = file_sep+1;

  printf("%s:%04d: %s", file, line, str);
}
#endif

static int json_eq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

// Iterate tokens in root Object, which is expected to match schema:
// {
//   ...,
//   results: [{
//     ...,
//     transaction_sold_count: 1234
//   }]
//  }
//
// If an error occurs while parsing, -1 will be returned.
int parse_order_count(const char *json) {
  int numTokens;
  jsmn_parser parser;
  jsmntok_t tokens[128]; // Actual response is about 73 tokens

  jsmn_init(&parser);
  numTokens = jsmn_parse(&parser, json, strlen(json), tokens, sizeof(tokens)/sizeof(tokens[0]));
  if (numTokens < 0) {
    printf("Failed to parse JSON. See JSMN error code %d\n", numTokens);
  }

  if (numTokens < 1 || tokens[0].type != JSMN_OBJECT) {
    printf("Invalid JSON: Root of JSON expected to be Object\n");
    return -1;
  }

  int i;
  for (i = 1; i < numTokens; i++) {
    if (json_eq(json, &tokens[i], "results") == 0) {
      if (tokens[i + 1].type != JSMN_ARRAY || tokens[i + 1].size != 1) {
        printf("Invalid JSON: Results property is not valid\n");
        break;
      }

      if (tokens[i + 2].type != JSMN_OBJECT || tokens[i + 2].size == 0) {
        printf("Invalid JSON: Results[0] is not valid\n");
        break;
      }
      i+=2; // skip ahead to results[0] identifier
      continue;
    }

    if (json_eq(json, &tokens[i], "transaction_sold_count") == 0) {
      if (tokens[i + 1].type != JSMN_PRIMITIVE) {
        printf("Invalid JSON: Transaction sold count not valid\n");
        break;
      }
      int count = atoi(json + tokens[i + 1].start);
      printf("Parsed order count: %d\n", count);
      return count;
    }
  }
  return -1;
}

void fetch_order_count_task(void *pvParameters) {
  int successes = 0, failures = 0, ret;
  printf("HTTP get task starting...\n");

  uint32_t flags;
  unsigned char buf[1024];
  const char *pers = "ssl_client1";

  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context ssl;
  mbedtls_x509_crt cacert;
  mbedtls_ssl_config conf;
  mbedtls_net_context server_fd;

  // Initialize the RNG and the session data
  mbedtls_ssl_init(&ssl);
  mbedtls_x509_crt_init(&cacert);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  printf("\n  . Seeding the random number generator...");

  mbedtls_ssl_config_init(&conf);

  mbedtls_entropy_init(&entropy);
  if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
    (const unsigned char *) pers,
    strlen(pers))) != 0)
  {
    printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
    abort();
  }

  printf(" ok\n");

  // Initialize certificates
  printf("  . Loading the CA root certificate ...");

  ret = mbedtls_x509_crt_parse(&cacert, (uint8_t*)server_root_cert, strlen(server_root_cert)+1);
  if(ret < 0) {
    printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
    abort();
  }

  printf(" ok (%d skipped)\n", ret);

  // Hostname set here should match CN in server certificate
  if((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER_COMMON_NAME)) != 0) {
    printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
    abort();
  }

  // Configure SSL/TLS
  printf("  . Setting up the SSL/TLS structure...");

  if((ret = mbedtls_ssl_config_defaults(&conf,
          MBEDTLS_SSL_IS_CLIENT,
          MBEDTLS_SSL_TRANSPORT_STREAM,
          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
  {
    printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
    goto exit;
  }

  printf(" ok\n");

  /* OPTIONAL is not optimal for security, in this example it will print
   * a warning if CA verification fails but it will continue to connect.
   */
  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
  mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef MBEDTLS_DEBUG_C
  mbedtls_debug_set_threshold(DEBUG_LEVEL);
  mbedtls_ssl_conf_dbg(&conf, print_debug, stdout);
#endif

  if((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
    printf(" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
    goto exit;
  }

  /* Wait until we can resolve the DNS for the server, as an indication
   * our network is probably working...
   */
  printf("Waiting for server DNS to resolve... ");
  err_t dns_err;
  ip_addr_t host_ip;
  do {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    dns_err = netconn_gethostbyname(WEB_SERVER, &host_ip);
  } while(dns_err != ERR_OK);
  printf("done.\n");

  while(true) {
    mbedtls_net_init(&server_fd);
    printf("top of loop, free heap = %u\n", xPortGetFreeHeapSize());
    // Start the connection
    printf("  . Connecting to %s:%s...", WEB_SERVER, WEB_PORT);

    if((ret = mbedtls_net_connect(&server_fd, WEB_SERVER_COMMON_NAME,
            WEB_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
    {
      printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
      goto exit;
    }

    printf(" ok\n");

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    // Perform handshake
    printf("  . Performing the SSL/TLS handshake...");

    while((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
      if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
        goto exit;
      }
    }

    printf(" ok\n");

    // Verify the server certificate
    printf("  . Verifying peer X.509 certificate...");

    /* TODO: We probably want to bail out when ret != 0 */
    if((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
    {
      char vrfy_buf[512];

      printf(" failed\n");

      mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

      printf("%s\n", vrfy_buf);
    }
    else {
      printf(" ok\n");
    }

    // Write the GET request
    printf("  > Write to server:");

    int len = sprintf((char *) buf, GET_REQUEST);

    while((ret = mbedtls_ssl_write(&ssl, buf, len)) <= 0) {
      if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        printf(" failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);
        goto exit;
      }
    }

    len = ret;
    printf(" %d bytes written\n\n%s", len, (char *) buf);

    // Read the HTTP response
    printf("  < Read from server:");
    fflush(stdout);

    do {
      len = sizeof(buf) - 1;
      memset(buf, 0, sizeof(buf));
      ret = mbedtls_ssl_read(&ssl, buf, len);

      if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        continue;

      if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        ret = 0;
        break;
      }

      if(ret < 0) {
        printf("failed\n  ! mbedtls_ssl_read returned %d\n\n", ret);
        break;
      }

      if(ret == 0) {
        printf("\n\nEOF\n\n");
        break;
      }

      len = ret;
      printf(" %d bytes read %s\n\n", len, (char *) buf);
      orderCount = parse_order_count((char *) buf);
      printf("Parsed order count: %d", orderCount);
    } while(1);

    mbedtls_ssl_close_notify(&ssl);

exit:
    mbedtls_ssl_session_reset(&ssl);
    mbedtls_net_free(&server_fd);

    if(ret != 0) {
      char error_buf[100];
      mbedtls_strerror(ret, error_buf, 100);
      printf("\n\nLast error was: %d - %s\n\n", ret, error_buf);
      failures++;
    } else {
      successes++;
    }

    printf("\n\nsuccesses = %d failures = %d\n", successes, failures);
    for(int countdown = successes ? 10 : 5; countdown >= 0; countdown--) {
      printf("%d... ", countdown);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("\nStarting again!\n");
  }
}
