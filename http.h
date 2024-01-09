#ifndef HTTP_H_
#define HTTP_H_

#include <llhttp.h>

#include <sds.h>

// A nonzero value enables logging to stderr in this library.
static int DEBUG_ENABLED = 0;

/* HTTP methods. */
typedef enum llmethod_t {
  GET = 1,
  POST = 1 << 1,
  PUT = 1 << 2,
  DELETE = 1 << 3,
  HEAD = 1 << 4,
  OPTIONS = 1 << 5,
} llmethod;

typedef enum llversion_t {
  UNKNOWN = 0,
  HTTP1_1 = 1,
  HTTP2 = 1 << 1,
  HTTP3 = 1 << 2,
} llversion;

typedef struct llheader_t {
  const char *key;
  const char *value;
} llheader;

struct llheaders_t {
  llheader *headers;
  uint32_t headers_len;
};
typedef struct llheaders_t llheaders;

enum llstatus_t {
  /**
   * @brief Indicates that the request has succeeded. A 200 response
   * is cacheable by default.
   */
  OK = 200,
  
  /**
   * @brief The request succeeded, and a new resource was created as a
   * result. This is typically the response sent after POST requests,
   * or some PUT requests.
   */
  CREATED = 201,

  /**
   * @brief The request has been received but not yet acted upon.
   * It is noncommittal, since there is no way in HTTP to later send
   * an asynchronous response indicating the outcome of the request.
   * It is intended for cases where another process or server handles
   * the request, or for batch processing. 
   */
  ACCEPTED = 202,

  /**
   * @brief The HTTP 204 No Content success status response code indicates
   * that a request has succeeded, but that the client doesn't need to
   * navigate away from its current page. 
   */
  NO_CONTENT = 204,

  /**
   * @brief Indicates that the requested resource has been definitively moved
   * to the URL given by the Location headers. A browser redirects to the new
   * URL and search engines update their links to the resource.
   */
  MOVED_PERMANENTLY = 301,
  NOT_MODIFIED = 304,
  TEMPORARY_REDIRECT = 307,
  PERMANENT_REDIRECT = 308,
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  INTERNAL_SERVER_ERROR = 500,
  HTTP_VERSION_NOT_SUPPORTED = 505,
};
typedef enum llstatus_t llstatus;

/** @brief A request from a client. */
struct llrequest_t {
  llmethod method;
  llversion version;

  // The part before and excluding the `?`
  char *path;

  // The part after and excluding the `?`
  char *search;

  llheaders headers;
  char *body;
};
typedef struct llrequest_t llrequest;

enum llstate_t {
  /** @brief The parser requires more data. */
  NEEDS_MORE,
  /** @brief The parser is done parsing. */
  COMPLETE,
  /** @brief The parser has reached an error state. */
  ERR,
};
typedef enum llstate_t llstate;

struct ll_t {
  llhttp_t *parser;
  llstate state;

  llrequest *request;

  // Working variables.
  sds method;
  sds version;
  sds url;
  sds search;
  sds body;
  
  sds current_header_key;
  sds current_header_val;
};
typedef struct ll_t ll;

/**
 * @brief Create a new request parser.
 */
ll *new_parser();

/** Free a parser. */
void ll_free(ll *);

#define LL_CONTINUE 1
#define LL_PARSING_COMPLETE 2
#define LL_STOPPED 3

/**
 * @brief Process incoming data.
 */
int parse(ll *l, const char *input, const int input_len);

/*
Eject the first complete request, if there is one. After ejection,
the request must be manually freed with free_request().
*/
llrequest *get_request(ll *l);

void free_request(llrequest *);

/** Returns first matching header. NULL if not present. */
const char *get_header_value(llrequest *r, const char *header_name);

/**
 * Returns a NULL-terminated list of strings corresponding to the matching
 * header entries for the given header key. Returns NULL if none are found.
 */
char * const *get_header_values(llrequest *r, const char *header_name, int32_t *vals_len);

#endif // HTTP_H_
