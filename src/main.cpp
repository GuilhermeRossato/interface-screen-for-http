#include <windows.h>
#include <wingdi.h>
#include <stdio.h>
#include <stdbool.h>
#include <cstdint>
#include <string>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "./miniz-2.1.0/miniz.h"
#include "./miniz-2.1.0/miniz.c"

#define DEFAULT_PORT "8081"

#pragma comment(lib, "ws2_32.lib")

void print_timestamp(int with_brackets, int is_local_time) {
	if (with_brackets) {
		printf("[ ");
	}
    SYSTEMTIME tm;

	if (is_local_time) {
    	GetLocalTime(&tm);
	} else {
    	GetSystemTime(&tm);
	}

	printf("%04d/%02d/%02d", (int) tm.wYear, (int) tm.wMonth, (int) tm.wDay);
	printf(" %02d:%02d:%02d", (int) tm.wHour, (int) tm.wMinute, (int) tm.wSecond);

	if (with_brackets) {
		printf(" ] ");
	}
}

void show_help(char * executable_name) {
	printf(
		"This utility answers HTTP requests to retrieve the screen data\n"
		"\n"
		"\n%s "
		"[portnumber] The port number of the HTTP server to listen to, default " DEFAULT_PORT "\n"
		"\n",
		executable_name
	);
}

int convert_string_to_long(char * string, long * number, int * veredict) {
	if (string == NULL || number == NULL) {
		return 0;
	}
	char *endptr;
	*number = std::strtol(string, &endptr, 10);

	if (endptr == string) {
		// Not a valid number at all
		if (veredict != NULL) {
			*veredict = 1;
		}
		return 0;
	}

	if (*endptr != '\0') {
		// String begins with a valid number, but also contains something else after the number
		if (veredict != NULL) {
			*veredict = 2;
		}
		return 0;
	}

	// String is a number
	if (veredict != NULL) {
		*veredict = 3;
	}
	return 1;
}

int main(int argn, char ** argc) {
	char * portnumber_str = argn >= 2 ? argc[2] : DEFAULT_PORT;

	if (portnumber_str != NULL && strcmp(portnumber_str, "--help") == 0) {
		show_help(argc[0]);
		printf("\nFor more information access the project's repository:\n\nhttps://github.com/GuilhermeRossato/interface-screen-for-http\n");
		return 1;
	}

	long portnumber;
	if (!convert_string_to_long(portnumber_str, &portnumber, NULL)) {
		printf("The portnumber parameter must contain a number, got invalid input: (%s)\n", portnumber_str);
		return 1;
	}

	int err_code;

    SOCKET sock, msg_sock;
    WSADATA wsaData;

	// Initiate use of the Winsock DLL by this process

	err_code = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err_code == SOCKET_ERROR) {
        printf("WSAStartup returned SOCKET_ERROR. Program will exit.\n");
		return 1;
	} else if (err_code != 0) {
		printf("WSAStartup returned %d. Program will exit.\n", err_code);
		return 1;
	}

    int addr_len;
    struct sockaddr_in local;

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		printf("Warning: WinSock DLL doest not seem to support version 2.2 in which this code has been tested with\n");
	}

    // Fill in the address structure
    local.sin_family        = AF_INET;
    local.sin_addr.s_addr   = INADDR_ANY;
    local.sin_port          = htons(portnumber);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET) {
        printf("Error: Socket function returned a invalid socket\n");
		WSACleanup();
		return 1;
	}

    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR) {
		printf("Error: Bind function failed\n");
		WSACleanup();
		return 1;
	}

	char recv_buffer[3000];
    int64_t count = 0;
	struct sockaddr_in client_addr;
	char buffer[1024*1024];
	int i;

	print_timestamp(1, 1);
    printf("Info: Waiting for connection at port %I64d\n", (int64_t) portnumber);

listen_goto:

    if (listen(sock, 10) == SOCKET_ERROR) {
		printf("Error: Listen function failed\n");
		WSACleanup();
        return 1;
	}

    while (1) {
        addr_len = sizeof(client_addr);
        msg_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);

        if (msg_sock == INVALID_SOCKET || msg_sock == -1) {
			print_timestamp(1, 1);
            printf("Error: Accept function returned a invalid receiving socket: %I64d\n", (int64_t)msg_sock);
    		WSACleanup();
			return 1;
		}

		int64_t recv_length = recv(msg_sock, recv_buffer, sizeof(recv_buffer), 0);

		print_timestamp(1, 1);
		printf("Info: Connection %I64d received %I64d bytes from \"%s\" at port %d\n", ++count, (int64_t) recv_length, inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));

		if (recv_length < 4 || recv_buffer[0] != 'G' || recv_buffer[1] != 'E' || recv_buffer[2] != 'T' || recv_buffer[3] != ' ') {
			print_timestamp(1, 1);
			printf("Info: Server sent unexpected request method\n");
			send(
				msg_sock,
				"HTTP/1.1 405 Method Not Allowed\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: 33\r\n"
				"Connection: close\r\n"
				"\r\n\r\n"
				"Only GET requests are allowed\r\n",
				148+1,
				0
			);
		}

		uint8_t image[3*3*3] = {0};
		memset(image, 127, sizeof(image));
		size_t image_size = 0;
		uint8_t * binary_image = (uint8_t *) tdefl_write_image_to_png_file_in_memory_ex(image, 3, 3, 3, &image_size, MZ_NO_COMPRESSION, 0);
		if (binary_image == NULL) {
			printf("Error: miniz png creation failed\n");
    		WSACleanup();
			return 1;
		}
		print_timestamp(1, 1);
		printf("Info: Server created image of %zd bytes\n", image_size);

		int buffer_size = snprintf(
			buffer,
			sizeof(buffer) - 1,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: image/png; charset=UTF-8\r\n"
			"Content-Length: %zd\r\n"
			"Connection: close\r\n"
			"\r\n",
			image_size
		);

		if (buffer_size + image_size >= sizeof(buffer)) {
			printf("Error: We run out of space to write the output image into the reply buffer\n");
    		WSACleanup();
			return 1;
		}

		if (buffer[buffer_size] != '\0') {
			printf("Error: My assumption of the buffer structure is incorrect\n");
			WSACleanup();
			return 1;
		}

		for (i = buffer_size; i < buffer_size + image_size && i < sizeof(buffer); i++) {
			buffer[i] = binary_image[i - buffer_size];
		}
		if (i < sizeof(buffer)) {
			buffer[i] = '\0';
		}

		mz_free(binary_image);

		send(
			msg_sock,
			buffer,
			buffer_size + image_size + 1,
			0
		);

        closesocket(msg_sock);

        goto listen_goto;
    }

    WSACleanup();
	return 0;
}
