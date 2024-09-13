# ahsan-http
A simple HTTP server written entirely in C.

Usage
-----
You can build this today just by using the makefile:
```
make; ./example
```
Example code(example.c)
```c
#include "ahsan_http.h"
HTTPResponse hello(HTTPRequest req) {
	HTTPResponse response = {200, {}, "Cool"};
	return response;
}

int main() {
	HTTPServer http_server = {8000};
	add_route(&http_server, "GET", "/", hello);
	start_http_server(&http_server);
	return 0;
}
```
