#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define REQUEST_RESPONSE_ARENA_SIZE 16384
#define BUFFER_SIZE 4096
#define METHOD_BUFFER_SIZE 8
#define PATH_BUFFER_SIZE 1024
#define VERSION_BUFFER_SIZE 16

typedef struct String {
	char* characters;
	int32_t length;
} String;

char String_GetChar(String string, int32_t index) {
	if (index < 0 || index >= string.length) {
		raise(SIGTRAP);
	}
	return string.characters[index];
}

typedef struct Arena {
	char* memory;
	int32_t capacity;
	int32_t offset;
} Arena;


void* Arena_Alloc(Arena* arena, int32_t size) {
    if (arena->offset + size > arena->capacity) {
        fprintf(stderr, "Arena overflow\n");
        exit(1);
    }
    void* ptr = arena->memory + arena->offset;
    arena->offset += size;
    return ptr;
}

void Arena_Destroy(Arena* arena) {
    free(arena->memory);
    arena->memory = NULL;
    arena->capacity = 0;
    arena->offset = 0;
}

typedef struct Response {
        String send_data;
} Response;

typedef struct Request {
		int32_t length;
        String receive_data;
		String method;
		String path;
		String version;
} Request;

void parse_request(Request *request) {
	printf("%s\n", request->receive_data.characters);
	printf("%d\n", request->length);

	sscanf(
		request->receive_data.characters,
		"%s %s %s\r\n",
		request->method.characters,
		request->path.characters,
		request->version.characters
	);
}

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible
	// when running tests.
	printf("Logs from your program will appear here!\n");


	Arena request_response_data = (Arena) {
		.memory = malloc(REQUEST_RESPONSE_ARENA_SIZE),
		.capacity = REQUEST_RESPONSE_ARENA_SIZE,
	};

	Request request = (Request){
		.receive_data = (String){
			.characters = Arena_Alloc(&request_response_data, BUFFER_SIZE),
		},
		.method = (String){
			.characters = Arena_Alloc(&request_response_data, METHOD_BUFFER_SIZE),
		},
		.path = (String){
			.characters = Arena_Alloc(&request_response_data, PATH_BUFFER_SIZE),
		},
		.version = (String){
			.characters = Arena_Alloc(&request_response_data, VERSION_BUFFER_SIZE),
		},
	};

	Response response = (Response){
		.send_data = (String){
			.characters = Arena_Alloc(&request_response_data, BUFFER_SIZE),
		},
	};

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

	int request_length = read(client_fd, request.receive_data.characters, 128);
	if (request_length < 0) {
		printf("\nSocket read error (%d): %s\n", errno, strerror(errno));
		exit(1);
	}
	request.length = request_length;

	parse_request(&request);


	if (strcmp(request.path.characters, "/") == 0) {
		strncpy(response.send_data.characters, "HTTP/1.1 200 OK\r\n\r\n", (sizeof(char) * response.send_data.length));
	} else {
		strncpy(response.send_data.characters, "HTTP/1.1 404 Not Found\r\n\r\n", (sizeof(char) * response.send_data.length));
	}

	int error = send(client_fd, response.send_data.characters, strlen(response.send_data.characters), 0);
	if (error < 0) {
		printf("\nSocket error.");
		exit(1);
	}
	printf("Client connected\n");

	close(server_fd);

	return 0;
}
