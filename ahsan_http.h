// Define the HTTPRequest structure
typedef struct HTTPRequest {
    char method[10];
    char path[300];
    char http_version[10];
    char headers[200][3000];
    char body[30000];
} HTTPRequest;

// Define the HTTPResponse structure
typedef struct HTTPResponse {
    int status_code;
    char headers[200][3000];
    char body[30000];
} HTTPResponse;

// Define the HTTPRoute structure
typedef struct HTTPRoute {
    char path[300];
    char method[10];
    HTTPResponse (*handlerPtr)(HTTPRequest);
} HTTPRoute;

// Define the HTTPServer structure with a dynamically allocated routes array
typedef struct HTTPServer {
	int port;            //Port to run on
    int size;            // Current number of routes
    int capacity;        // Total capacity of the routes array
    HTTPRoute *routes;   // Pointer to dynamically allocated array of routes
} HTTPServer;

// Function prototypes
void init_http_server(HTTPServer *server);
void add_route(HTTPServer *server, char method[10], char path[300], HTTPResponse (*handlerPtr)(HTTPRequest));
void start_http_server(HTTPServer *server);
void free_http_server(HTTPServer *server);
