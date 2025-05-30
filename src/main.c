#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define METHOD_BUFFER_SIZE 8
#define PATH_BUFFER_SIZE 1024
#define VERSION_BUFFER_SIZE 16

typedef struct Response {
        uint32_t length;
        char send_data[BUFFER_SIZE];
} Response;

typedef struct Request {
        uint32_t length;
        char receive_data[BUFFER_SIZE];
		char method[METHOD_BUFFER_SIZE];
		char path[PATH_BUFFER_SIZE];
		char version[VERSION_BUFFER_SIZE];
} Request;

void parse_request(Request *request) {
	printf("%s\n", request->receive_data);
	printf("%d\n", request->length);

	sscanf(request->receive_data, "%s %s %s\r\n", request->method, request->path, request->version);
}

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible
	// when running tests.
	printf("Logs from your program will appear here!\n");

	int server_fd;
	struct sockaddr_in client_addr;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}

	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}

	struct sockaddr_in serv_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(4221),
		.sin_addr = {htonl(INADDR_ANY)},
	};

	if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	printf("Waiting for a client to connect...\n");
	socklen_t client_addr_len = sizeof(client_addr);


	int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

	Request request;
	int request_length = read(client_fd, request.receive_data, 128);
	if (request_length < 0) {
		printf("\nSocket read error.");
		exit(1);
	}
	request.length = request_length;

	parse_request(&request);

	Response response;

	if (strcmp(request.path, "/") == 0) {
		strncpy(response.send_data, "HTTP/1.1 200 OK\r\n\r\n", sizeof(response.send_data));
	} else {
		strncpy(response.send_data, "HTTP/1.1 404 Not Found\r\n\r\n", sizeof(response.send_data));
	}

	int error = send(client_fd, response.send_data, strlen(response.send_data), 0);
	if (error < 0) {
		printf("\nSocket error.");
		exit(1);
	}
	printf("Client connected\n");

	close(server_fd);

	return 0;
}
