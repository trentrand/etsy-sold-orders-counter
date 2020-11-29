/* This is the CA certificate for the CA trust chain of
   openapi.etsy.com in PEM format, as dumped via:

   openssl s_client -showcerts -connect openapi.etsy.com:443 </dev/null

   The CA cert is the last cert in the chain output by the server.
*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/*
 * 1 s:C = BE, O = GlobalSign nv-sa, CN = GlobalSign CloudSSL CA - SHA256 - G3
 * i:C = BE, O = GlobalSign nv-sa, OU = Root CA, CN = GlobalSign Root CA
 */
const char *server_root_cert = "-----BEGIN CERTIFICATE-----\r\n"
"MIIEizCCA3OgAwIBAgIORvCM288sVGbvMwHdXzQwDQYJKoZIhvcNAQELBQAwVzEL\r\n"
"MAkGA1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsT\r\n"
"B1Jvb3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw0xNTA4MTkw\r\n"
"MDAwMDBaFw0yNTA4MTkwMDAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBH\r\n"
"bG9iYWxTaWduIG52LXNhMS0wKwYDVQQDEyRHbG9iYWxTaWduIENsb3VkU1NMIENB\r\n"
"IC0gU0hBMjU2IC0gRzMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCj\r\n"
"wHXhMpjl2a6EfI3oI19GlVtMoiVw15AEhYDJtfSKZU2Sy6XEQqC2eSUx7fGFIM0T\r\n"
"UT1nrJdNaJszhlyzey2q33egYdH1PPua/NPVlMrJHoAbkJDIrI32YBecMbjFYaLi\r\n"
"blclCG8kmZnPlL/Hi2uwH8oU+hibbBB8mSvaSmPlsk7C/T4QC0j0dwsv8JZLOu69\r\n"
"Nd6FjdoTDs4BxHHT03fFCKZgOSWnJ2lcg9FvdnjuxURbRb0pO+LGCQ+ivivc41za\r\n"
"Wm+O58kHa36hwFOVgongeFxyqGy+Z2ur5zPZh/L4XCf09io7h+/awkfav6zrJ2R7\r\n"
"TFPrNOEvmyBNVBJrfSi9AgMBAAGjggFTMIIBTzAOBgNVHQ8BAf8EBAMCAQYwHQYD\r\n"
"VR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMBIGA1UdEwEB/wQIMAYBAf8CAQAw\r\n"
"HQYDVR0OBBYEFKkrh+HOJEc7G7/PhTcCVZ0NlFjmMB8GA1UdIwQYMBaAFGB7ZhpF\r\n"
"DZfKiVAvfQTNNKj//P1LMD0GCCsGAQUFBwEBBDEwLzAtBggrBgEFBQcwAYYhaHR0\r\n"
"cDovL29jc3AuZ2xvYmFsc2lnbi5jb20vcm9vdHIxMDMGA1UdHwQsMCowKKAmoCSG\r\n"
"Imh0dHA6Ly9jcmwuZ2xvYmFsc2lnbi5jb20vcm9vdC5jcmwwVgYDVR0gBE8wTTAL\r\n"
"BgkrBgEEAaAyARQwPgYGZ4EMAQICMDQwMgYIKwYBBQUHAgEWJmh0dHBzOi8vd3d3\r\n"
"Lmdsb2JhbHNpZ24uY29tL3JlcG9zaXRvcnkvMA0GCSqGSIb3DQEBCwUAA4IBAQCi\r\n"
"HWmKCo7EFIMqKhJNOSeQTvCNrNKWYkc2XpLR+sWTtTcHZSnS9FNQa8n0/jT13bgd\r\n"
"+vzcFKxWlCecQqoETbftWNmZ0knmIC/Tp3e4Koka76fPhi3WU+kLk5xOq9lF7qSE\r\n"
"hf805A7Au6XOX5WJhXCqwV3szyvT2YPfA8qBpwIyt3dhECVO2XTz2XmCtSZwtFK8\r\n"
"jzPXiq4Z0PySrS+6PKBIWEde/SBWlSDBch2rZpmk1Xg3SBufskw3Z3r9QtLTVp7T\r\n"
"HY7EDGiWtkdREPd76xUJZPX58GMWLT3fI0I6k2PMq69PVwbH/hRVYs4nERnh9ELt\r\n"
"IjBrNRpKBYCkZd/My2/Q\r\n"
"-----END CERTIFICATE-----\r\n";


