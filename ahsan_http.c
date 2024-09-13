#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "ahsan_http.h"
#include <time.h>

void print_current_time() {
	// Get current time
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime); // Get the current time
	timeinfo = localtime(&rawtime); // Convert to local time format

	// Print time in the desired format
	printf("[%04d-%02d-%02d:%02d:%02d:%02d] ",
		   timeinfo->tm_year + 1900,  // Year since 1900
		   timeinfo->tm_mon + 1,	  // Months since January (0-11)
		   timeinfo->tm_mday,		  // Day of the month (1-31)
		   timeinfo->tm_hour,		  // Hours since midnight (0-23)
		   timeinfo->tm_min,		  // Minutes after the hour (0-59)
		   timeinfo->tm_sec);		  // Seconds after the minute (0-59)
}

char** split(const char* input, char delimiter, int* num_tokens) {
	// Allocate memory for a temporary buffer to hold the split tokens
	char* temp_str = strdup(input);
	if (temp_str == NULL) {
		perror("strdup failed");
		return NULL;
	}

	// First pass: Count the number of tokens
	int count = 0;
	char delimiter_str[2] = {delimiter, '\0'}; // Create a string with the delimiter
	char* token = strtok(temp_str, delimiter_str);
	while (token != NULL) {
		count++;
		token = strtok(NULL, delimiter_str);
	}

	// Allocate memory for the array of string pointers
	char** result = (char**)malloc(count * sizeof(char*));
	if (result == NULL) {
		perror("malloc failed");
		free(temp_str);
		return NULL;
	}

	// Restore the original string and copy the tokens into the result array
	strcpy(temp_str, input);
	token = strtok(temp_str, delimiter_str);
	for (int i = 0; i < count; i++) {
		result[i] = strdup(token);
		if (result[i] == NULL) {
			perror("strdup failed");
			// Free previously allocated memory before returning
			for (int j = 0; j < i; j++) {
				free(result[j]);
			}
			free(result);
			free(temp_str);
			return NULL;
		}
		token = strtok(NULL, delimiter_str);
	}

	free(temp_str);
	*num_tokens = count;
	return result;
}

HTTPRequest raw_request_to_http_request(char** tokens, int num_tokens) {
	HTTPRequest request;
	// Initialize the request with zeroes
	memset(&request, 0, sizeof(HTTPRequest));

	if (num_tokens < 1) {
		return request;  // Return empty request if not enough tokens
	}

	// Parse the request line (first token)
	char* request_line = tokens[0];
	sscanf(request_line, "%s %s %s", request.method, request.path, request.http_version);
	

	// Parse headers and body (remaining tokens)
	int i = 1;
	while (i < num_tokens) {
		char* line = tokens[i];
		if (strlen(line) == 0) {
			// Empty line signifies the start of the body (if following HTTP standard)
			i++;
			break;
		}
		
		// Parse headers (this assumes headers are in format "Key: Value")
		char* colon_pos = strchr(line, ':');
		if (colon_pos) {
			size_t key_len = colon_pos - line;
			size_t value_len = strlen(line) - key_len - 1;

			// Find an empty spot in headers array
			int header_index = 0;
			while (header_index < 200 && strlen(request.headers[header_index]) > 0) {
				header_index++;
			}

			if (header_index < 200) {
				strncpy(request.headers[header_index], line, sizeof(request.headers[header_index]) - 1);
				request.headers[header_index][sizeof(request.headers[header_index]) - 1] = '\0';  // Null-terminate
			}
		}
		i++;
	}

	// Copy remaining tokens into body
	size_t body_length = 0;
	while (i < num_tokens && body_length < sizeof(request.body) - 1) {
		size_t line_length = strlen(tokens[i]);
		if (body_length + line_length < sizeof(request.body) - 1) {
			strcpy(request.body + body_length, tokens[i]);
			body_length += line_length;
			request.body[body_length] = '\0';  // Null-terminate
		}
		i++;
	}

	return request;
}

char* struct_response_to_http_response(HTTPResponse* response) {
	// Allocate memory for the response string with a reasonable initial size
	// Adjust size as needed based on expected response size
	size_t buffer_size = 4096;
	char* response_str = malloc(buffer_size);
	if (response_str == NULL) {
		return NULL; // Allocation failed
	}

	// Start with the status line
	int offset = snprintf(response_str, buffer_size, "HTTP/1.1 %d \r\n", response->status_code);

	if (offset < 0 || offset >= buffer_size) {
		free(response_str);
		return NULL; // Formatting error or buffer too small
	}

	// Append headers
	for (int i = 0; i < 200; i++) {
		if (strlen(response->headers[i]) == 0) {
			break; // Stop if an empty header is encountered
		}

		int header_len = strlen(response->headers[i]);
		if (offset + header_len + 2 >= buffer_size) {
			// Reallocate memory if needed
			size_t new_size = buffer_size * 2;
			char* new_response_str = realloc(response_str, new_size);
			if (new_response_str == NULL) {
				free(response_str);
				return NULL; // Reallocation failed
			}
			response_str = new_response_str;
			buffer_size = new_size;
		}

		strcpy(response_str + offset, response->headers[i]);
		offset += header_len;
		strcpy(response_str + offset, "\r\n");
		offset += 2;
	}

	// End headers section
	strcpy(response_str + offset, "\r\n");
	offset += 2;

	// Append body
	size_t body_len = strlen(response->body);
	if (offset + body_len >= buffer_size) {
		// Reallocate memory if needed
		size_t new_size = buffer_size + body_len;
		char* new_response_str = realloc(response_str, new_size);
		if (new_response_str == NULL) {
			free(response_str);
			return NULL; // Reallocation failed
		}
		response_str = new_response_str;
		buffer_size = new_size;
	}

	strcpy(response_str + offset, response->body);

	return response_str;
}

void start_http_server(HTTPServer *http_server) {
	int server_fd, new_socket;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	char buffer[30000] = {0};

	// Create socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Bind to port
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(http_server->port);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	// Start listening
	if (listen(server_fd, 3) < 0) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	printf("Listening on port %d\n", http_server->port);
	
	while (1) {
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept failed");
			exit(EXIT_FAILURE);
		}
		// Read the request
		read(new_socket, buffer, 30000);

		//Parse the request
		char delimiter = '\n';
		int num_tokens;
		char** tokens = split(buffer, delimiter, &num_tokens);
		HTTPRequest request = raw_request_to_http_request(tokens, num_tokens);

		//Print out the request for debugging
		//printf("%s", request.method);
		//printf("\n");
		//printf("%s", request.path);
		//printf("\n");
		//printf("%s", request.http_version);
		//printf("\n");
		//printf("Headers:\n");
		//for (int i = 0; i < 200; i++){
			//printf("%s", request.headers[i]);
			//printf("\n");
		//}
		//printf("%s", request.body);
		//
		
		// Find the route and get its handler if found
		int raise_404 = 1;
		if (request.path == NULL || request.method == NULL || request.http_version == NULL){
			const char *response = "HTTP/1.1 400 Bad Request\nContent-Type: text/html\nContent-Length: 12\n\n<h1>Bad Request!</h1>";
			print_current_time();
			printf("400 - ");
			printf("%s\n", request.path);
			write(new_socket, response, strlen(response));
			close(new_socket);
			raise_404 = 0;
		}
		for (int i = 0; i < http_server->size; i++) {
			HTTPRoute route = http_server->routes[i];
			if (strcmp(route.method, request.method) == 0 && strcmp(route.path, request.path) == 0) {
				HTTPResponse r = route.handlerPtr(request);
				print_current_time();
				printf("%d", r.status_code);
				printf(" - ");
				printf("%s\n", request.path);
				char *response = struct_response_to_http_response(&r);
				write(new_socket, response, strlen(response));
				close(new_socket);
				raise_404 = 0;
			}
		}
		if (raise_404 == 1) {
			print_current_time();
			printf("404 - ");
			printf("%s\n", request.path);
			const char *response = "HTTP/1.1 404 Not Found\nContent-Type: text/html\nContent-Length: 22\n\n<h1>404 Not Found</h1>";
			write(new_socket, response, strlen(response));
			close(new_socket);
		}	
	}

}

void init_http_server(HTTPServer *server) {
	server->routes = NULL;
	server->size = 0;
	server->capacity = 0;
}

// Add a route to the HTTPServer
void add_route(HTTPServer *server, char method[10], char path[300], HTTPResponse (*handlerPtr)(HTTPRequest)) {
	// If the server is not initialized, initialize it
	if (server->routes == NULL) {
		init_http_server(server);
	}

	// If the array is full, double the capacity
	if (server->size >= server->capacity) {
		server->capacity = (server->capacity == 0) ? 2 : server->capacity * 2;
		server->routes = realloc(server->routes, server->capacity * sizeof(HTTPRoute));
		if (server->routes == NULL) {
			perror("Failed to allocate memory for routes");
			exit(EXIT_FAILURE);
		}
	}

	// Create the route
	HTTPRoute route;
	strncpy(route.path, path, sizeof(route.path) - 1);
	route.path[sizeof(route.path) - 1] = '\0';	// Ensure null termination
	strncpy(route.method, method, sizeof(route.method) - 1);
	route.method[sizeof(route.method) - 1] = '\0';	// Ensure null termination
	route.handlerPtr = handlerPtr;

	// Add the route
	server->routes[server->size] = route;
	server->size++;
}

// Free the allocated memory for the server
void free_http_server(HTTPServer *server) {
	free(server->routes);
	server->routes = NULL;
	server->size = 0;
	server->capacity = 0;
}
