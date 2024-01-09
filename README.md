# I don't have a name for this

> [!WARNING]  
> This code is incomplete! ...but it is available for you to use should you so
> desire. Bear in mind that I have not audited it for:
> * API completeness
> * Correctness
> * Memory leaks

This folder supplies a wrapper around [llhttp](https://github.com/nodejs/llhttp)
to make it easier to parse HTTP requests from C without copy-pasting a bunch of
callbacks. The idea is that you could write a very simple HTTP server without
too much arbitrary parsing, reinventing the wheel with these callbacks around
llhttp, or using something larger like [libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/).

It uses [sds](https://github.com/antirez/sds) for string manipulation while processing
incoming requests. The project doesn't really need this requirement - it could just
be standard library functions. I just think it's easier to reason about.

## Ways it could be improved

* Use [uthash](https://troydhanson.github.io/uthash/) for header storage and retrieval.
* Integrate JSON parsing for bodies.
* CORS, maybe?

## Example

```c
int main(void) {
  ll *parser = new_parser();
  assert(parser != NULL);
  const char *request_text =
    "GET / HTTP/1.1\r\nHost: google.com\r\nHost: example.com\r\nAccept: */*\r\n\r\n";
  assert(parse(parser, request_text, strlen(request_text)) >= 0);

  llrequest *request = get_request(parser);
  assert(request != NULL);

  // Case-sensitive O(n) lookup with respect to the number of headers
  const char *host_header = get_header_value(request, "Host");
  assert(strcmp(host_header, "google.com") == 0);
  return 0;
}
```
