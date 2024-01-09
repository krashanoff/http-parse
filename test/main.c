#include "../../../Unity/src/unity.h"
#include "string.h"
#include <http.h>

void test_parse_smoke(void);
void test_parse_basic(void);
void test_parse_post(void);

void setUp(void) {}

void tearDown(void) {}

/* Ensures that the library at least parses something. */
void test_parse_smoke(void) {
  ll *parser = new_parser();
  const char *r = "GET / HTTP/1.1\r\n\r\n";
  int rc = parse(parser, r, strlen(r));
  TEST_ASSERT_EQUAL_INT(LL_PARSING_COMPLETE, rc);
}

void test_parse_basic(void) {
  ll *parser = new_parser();
  const char *r = "GET / HTTP/1.1\r\nHost: google.com\r\nUser-Agent: "
                  "curl/8.1.2\r\nAccept: */*\r\n\r\n";
  int rc = parse(parser, r, strlen(r));
  TEST_ASSERT_EQUAL_INT(LL_PARSING_COMPLETE, rc);

  llrequest *request = get_request(parser);
  TEST_ASSERT_NOT_NULL(request);
  TEST_ASSERT_EQUAL(GET, request->method);
  const char *host_value = get_header_value(request, "Host");
  TEST_ASSERT_NOT_NULL(host_value);
}

void test_parse_post(void) {
  ll *parser = new_parser();

  const char *r = "POST / HTTP/1.1\r\n"
                  "Host: google.com\r\n"
                  "User-Agent: curl/8.1.2\r\n"
                  "Accept: */*\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: 4\r\n"
                  "\r\n"
                  "test"
                  "\r\n"
                  "\r\n";
  int rc = parse(parser, r, strlen(r));
  TEST_ASSERT_EQUAL_INT(LL_PARSING_COMPLETE, rc);

  llrequest *request = get_request(parser);
  TEST_ASSERT_NOT_NULL(request);
  TEST_ASSERT_EQUAL(POST, request->method);
  TEST_ASSERT_EQUAL(HTTP1_1, request->version);
  TEST_ASSERT_EQUAL_STRING("google.com", get_header_value(request, "Host"));
  TEST_ASSERT_EQUAL_STRING("curl/8.1.2",
                           get_header_value(request, "User-Agent"));
  TEST_ASSERT_EQUAL_STRING("*/*", get_header_value(request, "Accept"));
  TEST_ASSERT_EQUAL_STRING("application/json",
                           get_header_value(request, "Content-Type"));
  TEST_ASSERT_EQUAL_STRING("4", get_header_value(request, "Content-Length"));
  TEST_ASSERT_EQUAL_STRING("test", request->body);
}

void test_empty_search(void) {
  ll *parser = new_parser();

  const char *r = "GET /somepath? HTTP/1.1"
                  "\r\n"
                  "\r\n";
  int rc = parse(parser, r, strlen(r));
  TEST_ASSERT_EQUAL_INT(LL_PARSING_COMPLETE, rc);

  llrequest *request = get_request(parser);
  TEST_ASSERT_NOT_NULL(request);
  TEST_ASSERT_EQUAL(GET, request->method);
  TEST_ASSERT_EQUAL(HTTP1_1, request->version);
  TEST_ASSERT_EQUAL_STRING("/somepath", request->path);
  TEST_ASSERT_EQUAL_STRING("", request->search);
}

void test_search(void) {
  ll *parser = new_parser();

  const char *r = "POST /somepath?key1=val1&key2=val2 HTTP/1.1\r\n"
                  "Host: google.com\r\n"
                  "User-Agent: curl/8.1.2\r\n"
                  "Accept: */*\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: 4\r\n"
                  "\r\n"
                  "test"
                  "\r\n"
                  "\r\n";
  int rc = parse(parser, r, strlen(r));
  TEST_ASSERT_EQUAL_INT(LL_PARSING_COMPLETE, rc);

  llrequest *request = get_request(parser);
  TEST_ASSERT_NOT_NULL(request);
  TEST_ASSERT_EQUAL(POST, request->method);
  TEST_ASSERT_EQUAL(HTTP1_1, request->version);
  TEST_ASSERT_EQUAL_STRING("/somepath", request->path);
  TEST_ASSERT_EQUAL_STRING("google.com", get_header_value(request, "Host"));
  TEST_ASSERT_EQUAL_STRING("curl/8.1.2",
                           get_header_value(request, "User-Agent"));
  TEST_ASSERT_EQUAL_STRING("*/*", get_header_value(request, "Accept"));
  TEST_ASSERT_EQUAL_STRING("application/json",
                           get_header_value(request, "Content-Type"));
  TEST_ASSERT_EQUAL_STRING("4", get_header_value(request, "Content-Length"));
  TEST_ASSERT_EQUAL_STRING("test", request->body);
  TEST_ASSERT_EQUAL_STRING("key1=val1&key2=val2", request->search);
}

void test_multiple_headers(void) {
  ll *parser = new_parser();

  const char *r = "GET /h HTTP/1.1\r\nHost: google.com\r\nHost: "
                  "example.com\r\nAccept: */*\r\n\r\n";
  int rc = parse(parser, r, strlen(r));
  TEST_ASSERT_EQUAL_INT_MESSAGE(LL_PARSING_COMPLETE, rc, "parsing failed");

  llrequest *request = get_request(parser);
  TEST_ASSERT_NOT_NULL(request);
  TEST_ASSERT_EQUAL(GET, request->method);
  TEST_ASSERT_EQUAL(HTTP1_1, request->version);
  TEST_ASSERT_EQUAL_STRING("/h", request->path);

  int header_values_len = 0;
  char *const *values = get_header_values(request, "Host", &header_values_len);
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, header_values_len,
                                "header count mismatched");
  TEST_ASSERT_EQUAL_STRING("google.com", values[0]);
  TEST_ASSERT_EQUAL_STRING("example.com", values[1]);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_smoke);
  RUN_TEST(test_parse_basic);
  RUN_TEST(test_parse_post);
  RUN_TEST(test_search);
  RUN_TEST(test_empty_search);
  RUN_TEST(test_multiple_headers);
  return UNITY_END();
}
