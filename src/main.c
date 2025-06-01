#include <errno.h>
#include <iso646.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define REQUEST_RESPONSE_ARENA_SIZE 16384
#define BUFFER_SIZE 4096
#define START_LINE_BUFFER_SIZE 1024
#define HEADERS_BUFFER_SIZE 1024
#define BODY_BUFFER_SIZE 1024

typedef struct Arena {
	char* memory;
	int32_t capacity;
	int32_t offset;
} Arena;

void* Arena_Alloc(Arena* arena, int32_t size)
{
	if (!arena->memory) {
		fprintf(stderr, "Failed to allocate arena memory\n");
		exit(1);
	}
	if (arena->offset + size > arena->capacity) {
		fprintf(stderr, "Arena overflow\n");
		exit(1);
	}
	void* ptr = arena->memory + arena->offset;
	arena->offset += size;
	return ptr;
}

void Arena_Destroy(Arena* arena)
{
	free(arena->memory);
	arena->memory = NULL;
	arena->capacity = 0;
	arena->offset = 0;
}
typedef struct String {
	char* value;
	int32_t length;
} String;

char String_GetChar(String string, int32_t index)
{
	if (index < 0 || index >= string.length) {
		raise(SIGTRAP);
	}
	return string.value[index];
}

void String_Slice(String* destination, String src, int32_t starting_index,
	int32_t ending_index)
{
	if (starting_index < 0) {
		raise(SIGTRAP);
	}
	if (ending_index < 0 || ending_index <= starting_index) {
		raise(SIGTRAP);
	}
	destination->length = ending_index - starting_index;
	strncpy(destination->value, &src.value[starting_index],
		destination->length);
}

int32_t String_IndexOf(String string, const char* substring,
	int32_t starting_index)
{
	size_t substring_length = strlen(substring);
	if (substring_length == 0 || substring_length > string.length)
		raise(SIGTRAP);

	for (int i = starting_index; i <= string.length - substring_length; i++) {
		if (strncmp(&string.value[i], substring, substring_length) == 0) {
			return i;
		}
	}
	return -1;
}

void String_Create(String* string, const char* const_string)
{
	if (string->length > 0) {
		raise(SIGTRAP);
	}
	size_t length = strlen(const_string);
	if (length <= 0) {
		raise(SIGTRAP);
	}
	strncpy(string->value, const_string, length);
	string->length = length;
}

void String_NCopy(String* destination, String src, size_t index)
{
	strncpy(destination->value, src.value, index);
	destination->length = src.length;
}

typedef struct Response {
	String send_data;
} Response;

typedef struct Request {
	String request_data;
	String start_line;
	String headers;
	String body;
} Request;

void parse_request(Arena* request_response_data, Request* request)
{
	int32_t index_end_of_start_line = String_IndexOf(request->request_data, "\r\n", 0);
	request->start_line.value = Arena_Alloc(request_response_data, START_LINE_BUFFER_SIZE);
	String_Slice(&request->start_line, request->request_data, 0, index_end_of_start_line);

	int32_t index_end_of_headers = String_IndexOf(request->request_data, "\r\n\r\n", index_end_of_start_line + 2);
	if (index_end_of_headers == -1) {
		printf("index not found: %i\n", index_end_of_headers);
		raise(SIGTRAP);
	}
	request->headers.value = Arena_Alloc(request_response_data, HEADERS_BUFFER_SIZE);
	String_Slice(&request->headers, request->request_data, index_end_of_start_line + 2,
		index_end_of_headers);

	request->body.value = Arena_Alloc(request_response_data, BODY_BUFFER_SIZE);
	String_Slice(&request->body, request->request_data, index_end_of_headers + 4, request->request_data.length);
	/* printf("start_line: %s\n", request->start_line.value); */
	/* printf("headers: %s\n", request->headers.value); */
	/* printf("body: %s\n", request->body.value); */
}

int main()
{
	/* // Disable output buffering */
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	/**/
	// You can use print statements as follows for debugging, they'll be visible
	// when running tests.
	printf("Logs from your program will appear here!\n");

	Arena request_response_data = (Arena) {
		.memory = calloc(1, REQUEST_RESPONSE_ARENA_SIZE),
		.capacity = REQUEST_RESPONSE_ARENA_SIZE,
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
		.sin_addr = { htonl(INADDR_ANY) },
	};

	if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0) {
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

	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);

	Request request = { 0 };
	request.request_data.value = Arena_Alloc(&request_response_data, BUFFER_SIZE);

	int request_length = read(client_fd, request.request_data.value, BUFFER_SIZE);
	if (request_length < 0) {
		printf("\nSocket read error (%d): %s\n", errno, strerror(errno));
		exit(1);
	}
	request.request_data.length = request_length;

	parse_request(&request_response_data, &request);

	Response response = { 0 };
	response.send_data.value = Arena_Alloc(&request_response_data, BUFFER_SIZE);

	if (strcmp(request.headers.value, "/") == 0) {
		strncpy(response.send_data.value, "HTTP/1.1 200 OK\r\n\r\n",
			(sizeof(char) * response.send_data.length));
	} else {
		strncpy(response.send_data.value, "HTTP/1.1 404 Not Found\r\n\r\n",
			(sizeof(char) * response.send_data.length));
	}

	int error = send(client_fd, response.send_data.value,
		strlen(response.send_data.value), 0);
	if (error < 0) {
		printf("\nSocket error.");
		exit(1);
	}
	printf("Client connected\n");

	close(server_fd);
	Arena_Destroy(&request_response_data);
	return 0;
}
