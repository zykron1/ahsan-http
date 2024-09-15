#include "ahsan_http.h"
HTTPResponse hello(HTTPRequest req) {
	HTTPResponse response = {200, {"Content-Type: text/html"}, "<h1>Hello World</h1>"};
	return response;
}

int main() {
	HTTPServer http_server = {8000};
	add_route(&http_server, "GET", "/", hello);
	start_http_server(&http_server);
	return 0;
}
