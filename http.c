#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Frees the original string.
char *sds_to_c(sds s) {
  size_t s_len = sdslen(s);
  char *result = malloc((s_len + 1) * sizeof(char));
  for (size_t k = 0; k < sdslen(s); k++) {
    result[k] = s[k];
  }
  result[s_len] = '\0';
  return result;
}

inline static int
on_url(llhttp_t *parser, const char *at, size_t length)
{
  ll *l = (ll *)parser->data;
  int question_mark_position = -1;
  for (size_t k = 0; k < length; k++) {
    if (at[k] == '?') {
      question_mark_position = k + 1;
      break;
    }
  }

  if (question_mark_position != -1 || sdslen(l->search) != 0) {
    question_mark_position = question_mark_position == -1 ? 0 : question_mark_position;
    if (question_mark_position != 0) {
      l->url = sdscatlen(l->url, at, question_mark_position - 1);
    }
    l->search = sdscatlen(l->search, at + question_mark_position, length - question_mark_position);
  } else {
    l->url = sdscatlen(l->url, at, length);
  }
  return 0;
}

inline static int
on_url_complete(llhttp_t *parser)
{
  ll *l = (ll *)parser->data;
  l->request->path = sds_to_c(l->url);
  sdsclear(l->url);
  l->request->search = sds_to_c(l->search);
  sdsclear(l->search);
  return HPE_OK;
}

inline static int
on_header_field(llhttp_t* parser, const char *at, size_t length)
{
  ll *l = (ll *)parser->data;
  l->current_header_key = sdscatlen(l->current_header_key, at, length);
  return HPE_OK;
}

inline static int
on_header_field_complete(llhttp_t* parser)
{
  ll *l = (ll *)parser->data;
  l->request->headers.headers_len++;
  l->request->headers.headers = realloc(l->request->headers.headers, l->request->headers.headers_len * sizeof(llheader));
  l->request->headers.headers[l->request->headers.headers_len - 1].key = sds_to_c(l->current_header_key);
  sdsclear(l->current_header_key);
  return HPE_OK;
}

inline static int
on_header_value(llhttp_t *parser, const char *at, size_t length)
{
  ll *l = (ll *)parser->data;
  l->current_header_val = sdscatlen(l->current_header_val, at, length);
  return HPE_OK;
}

inline static int
on_header_value_complete(llhttp_t *parser) {
  ll *l = (ll *)parser->data;
  l->request->headers.headers[l->request->headers.headers_len - 1].value = sds_to_c(l->current_header_val);
  sdsclear(l->current_header_val);
  return HPE_OK;
}

inline static int
on_headers_complete(llhttp_t *_)
{
  return HPE_OK;
}

inline static int
on_method(llhttp_t *parser, const char *at, size_t length) {
  ll *l = parser->data;
  l->method = sdscatlen(l->method, at, length);
  return HPE_OK;
}

inline static int
on_method_complete(llhttp_t *parser)
{
  ll *l = parser->data;
  if (strcmp(l->method, "GET") == 0) {
    l->request->method = GET;
  } else if (strcmp(l->method, "POST") == 0) {
    l->request->method = POST;
  } else if (strcmp(l->method, "PUT") == 0) {
    l->request->method = PUT;
  } else if (strcmp(l->method, "DELETE") == 0) {
    l->request->method = DELETE;
  } else if (strcmp(l->method, "HEAD") == 0) {
    l->request->method = HEAD;
  } else if (strcmp(l->method, "OPTIONS") == 0) {
    l->request->method = OPTIONS;
  }
  return HPE_OK;
}

inline static int
on_body(llhttp_t *parser, const char *at, size_t length) {
  ll *l = parser->data;
  l->body = sdscatlen(l->body, at, length);
  return HPE_OK;
}

inline static int
on_message_complete(llhttp_t *parser)
{
  ll *l = parser->data;
  l->request->body = sds_to_c(l->body);
  return 0;
}

inline static int
on_version(llhttp_t *parser, const char *at, size_t length) {
  ll *l = parser->data;
  l->version = sdscatlen(l->version, at, length);
  return HPE_OK;
}

inline static int
on_version_complete(llhttp_t *parser)
{
  ll *l = parser->data;
  if (strcmp(l->version, "1.1") == 0) {
    l->request->version = HTTP1_1;
  } else if (strcmp(l->version, "2") == 0) {
    l->request->version = HTTP2;
  } else if (strcmp(l->version, "3") == 0) {
    l->request->version = HTTP3;
  } else {
    l->request->version = UNKNOWN;
  }
  sdsclear(l->version);
  return HPE_OK;
}

// New request parser.
ll *new_parser() {
  ll *l = (ll *)malloc(sizeof(ll));
  memset(l, 0, sizeof(ll));

  // Init working space.
  l->method = sdsempty();
  l->version = sdsempty();
  l->url = sdsempty();
  l->search = sdsempty();
  l->current_header_key = sdsempty();
  l->current_header_val = sdsempty();
  l->body = sdsempty();

  // Init LLHTTP stuff.
  l->parser = (llhttp_t *) malloc(sizeof(llhttp_t));
  memset(l->parser, 0, sizeof(llhttp_t));
  
  llhttp_settings_t *settings = (llhttp_settings_t *)malloc(sizeof(llhttp_settings_t));
  memset(settings, 0, sizeof(llhttp_settings_t));
  llhttp_settings_init(settings);

  settings->on_body = &on_body;
  settings->on_url = &on_url;
  settings->on_url_complete = &on_url_complete;
  settings->on_header_field = &on_header_field;
  settings->on_header_field_complete = &on_header_field_complete;
  settings->on_header_value = &on_header_value;
  settings->on_header_value_complete = &on_header_value_complete;
  settings->on_headers_complete = &on_headers_complete;
  settings->on_version = &on_version;
  settings->on_version_complete = &on_version_complete;
  settings->on_method_complete = &on_method_complete;
  settings->on_version_complete = &on_version_complete;
  settings->on_method = &on_method;
  settings->on_message_complete = &on_message_complete;

  llhttp_init(l->parser, HTTP_REQUEST, settings);
  l->request = (llrequest *) malloc(sizeof(llrequest));
  memset(l->request, 0, sizeof(llrequest));
  l->parser->data = l;
  return l;
}

void ll_free(ll *parser) {
  // TODO
}

void free_request(llrequest *request) {
  // TODO
}

int parse(ll *l, const char *input, const int input_len) {
  llhttp_errno_t r = llhttp_execute(l->parser, input, input_len);
  switch (r) {
  case HPE_OK:
    return LL_PARSING_COMPLETE;
  case HPE_PAUSED:
    return LL_STOPPED;
  case HPE_PAUSED_UPGRADE:
    return LL_STOPPED;
  default:
    printf("Parse error: %s %s\n", llhttp_errno_name(r), l->parser->reason);
    return -1;
  }
}

llrequest *get_request(ll *l) {
  return l->request;
}

const char *get_header_value(llrequest *r, const char *header_name) {
  int len = 0;
  char * const *header_vals = get_header_values(r, header_name, &len);
  if (len > 0) {
    return header_vals[0];
  }
  return NULL;
}

char * const *get_header_values(llrequest *r, const char *header_name, int32_t *vals_len) {
  char **vals = NULL;
  *vals_len = 0;

  for (uint32_t k = 0; k < r->headers.headers_len; k++) {
    const llheader *header = &r->headers.headers[k];
    if (strncmp(header->key, header_name, strlen(header->key)) == 0) {
      (*vals_len)++;
      vals = realloc(vals, sizeof(char *) * (*vals_len));
      vals[*vals_len - 1] = malloc((strlen(header->value) + 1) * sizeof(char));
      for (unsigned long k = 0; k < strlen(header->value); k++) {
        vals[*vals_len - 1][k] = header->value[k];
      }
      vals[*vals_len - 1][strlen(header->value)] = '\0';
    }
  }
  return vals;
}
