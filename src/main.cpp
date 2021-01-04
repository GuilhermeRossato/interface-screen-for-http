#include <windows.h>
#include <wingdi.h>
#include <stdio.h>
#include <stdbool.h>
#include <cstdint>
#include <string>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "./miniz-2.1.0/miniz.h"
#include "./capture.cpp"

#define DEFAULT_PORT "8081"
#define OUTPUT_BUFFER_SIZE 10314290

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "d3d9.lib")

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

int are_same_string(const char * a, const char * b) {
	int i;
	for (i = 0; a[i] != '\0' && b[i] != '\0'; i++) {
		if (a[i] != b[i]) {
			return 0;
		}
	}
	if (a[i] == '\0' && b[i] == '\0') {
		return 1;
	}
	return 0;
}

int does_first_start_with_second(const char * a, const char * b) {
	int i;
	for (i = 0; a[i] != '\0' && b[i] != '\0'; i++) {
		if (a[i] != b[i]) {
			return 0;
		}
	}
	if (a[i] != '\0' && b[i] != '\0') {
		return 0;
	}
	if (b[i] == '\0') {
		return 1;
	}
	return 0;
}

int interpret_http_query(
	char * query,
	int64_t buffer_length,
	int * left, int * top,
	int * width, int * height,
	int * is_png_format,
	int * is_binary_format,
	int * is_json_format,
	int * is_region_request
) {
	if (query == NULL || buffer_length <= 0) {
		return -1;
	} else if (query[0] != '/') {
		// printf("str is \"%s\"\n", &query[0]);
		return -7; // string must start at a backslash
	} else if (query[1] == ' ') {
		*is_region_request = 1;
		*is_png_format = 1;
		*is_binary_format = 0;
		*is_json_format = 0;
		return 1; // Success as request is with default parameters
	}

	int i = 0;

	if (does_first_start_with_second(&query[i], "/pixel/")) {
		i += 7;
		*is_region_request = 0;
	} else if (query[i] == '/' && (query[i+1] == ' ' || query[i+1] == '?' || query[i+1] == '#')) {
		i += 1;
		*is_region_request = 1;
	} else {
		return -2; // Unknown route
	}

	if (query[i] == ' ') {
		*is_png_format = 1;
		*is_binary_format = 0;
		*is_json_format = 0;
		return 1; // Success as request is with default parameters
	}

	if (query[i] != '?') {
		// printf("query at 0 is %c%c, at %d is %c%c%c\n", query[0], query[1], i, query[i], query[i+1], query[i+2]);
		return -3; // Something other than ? and # must be invalid query
	}

	i++;

	long value;
	int veredict;

	for (; query[i] != ' ' && query[i] != '\0' && query[i] != '\r' && query[i] != '\n'; i++) {
		if (query[i] == '&') {
			continue; // next parameter
		}

		int parameter_code =
			does_first_start_with_second(&query[i], "width=") ? 1 :
			does_first_start_with_second(&query[i], "height=") ? 2 :
			does_first_start_with_second(&query[i], "left=") ? 3 :
			does_first_start_with_second(&query[i], "x=") ? 4 :
			does_first_start_with_second(&query[i], "top=") ? 5 :
			does_first_start_with_second(&query[i], "y=") ? 6 :
			does_first_start_with_second(&query[i], "format=") ? 7 : 0;

		// check for unknown parameter
		if (parameter_code == 0) {
			// skip to next parameter or end
			for (i; query[i] != ' ' && query[i] != '&' && query[i] != '\r' && query[i] != '\n'; i++);
			i--;
			continue;
		}

		// printf("parameter code is %d\n", parameter_code);
		// skip to after equal sign
		while (query[i] != '=' && query[i] != ' ' && query[i] != '\0') {
			i++;
		}
		if (query[i] != '=') {
			return -8; // unexpected parameter with no content
		}
		i++;
		// printf("Parameter value is \"%s\"\n", &query[i]);

		if (parameter_code == 7) {
			if (query[i] == 't' || query[i] == 'T') {
				*is_png_format = 0;
				*is_binary_format = 0;
				*is_json_format = 0;
			} else if (((query[i] == 'p' || query[i] == 'P') && (query[i+1] == 'n' || query[i+1] == 'N')) || ((query[i] == 'i' || query[i] == 'I') && (query[i+1] == 'm' || query[i+1] == 'M'))) {
				*is_png_format = 1;
				*is_binary_format = 0;
				*is_json_format = 0;
			} else if ((query[i] == 'b' || query[i] == 'B') && (query[i+1] == 'i' || query[i+1] == 'I')) {
				*is_png_format = 0;
				*is_binary_format = 1;
				*is_json_format = 0;
			} else if ((query[i] == 'j' || query[i] == 'J') && (query[i+1] == 's' || query[i+1] == 'S')) {
				*is_png_format = 0;
				*is_binary_format = 0;
				*is_json_format = 1;
			} else {
				return -4; // invalid 'format' parameter
			}
			// skip to next parameter or end of line
			for (i; query[i] != ' ' && query[i] != '&' && query[i] != '\r' && query[i] != '\n'; i++);
			i--;
			continue;
		}

		// printf("About to interpret \"%s\"\n", &query[i]);

		convert_string_to_long(&query[i], &value, &veredict);

		if (!veredict) {
			return -5; // expected numeric parameter, got non-number
		} else if (value < 0 || value > 10000) {
			return -6; // parameter outside acceptable range
		}

		switch (parameter_code) {
			case 1: *width = value; break;
			case 2: *height = value; break;
			case 3:
			case 4: *left = value; break;
			case 5:
			case 6: *top = value; break;
		}

		// skip to next parameter or end
		for (i; query[i] != ' ' && query[i] != '&' && query[i] != '\r' && query[i] != '\n'; i++);
		i--;
		continue;
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

	char * buffer = (char *) malloc(OUTPUT_BUFFER_SIZE);
	if (!buffer) {
		printf("Error: Could not allocate buffer for output of size %d\n", OUTPUT_BUFFER_SIZE);
		return 1;
	}

	char recv_buffer[3000];
    int64_t count = 0;
	struct sockaddr_in client_addr;
	int i, buffer_size;

	uint8_t * image = CaptureScreenBuffer(0, 0, 0, 0);
	if (!image) {
		printf("Error: Could not generate test image\n");
		WSACleanup();
		return 1;
	} else if (globalLastCaptureWidth <= 0 || globalLastCaptureHeight <= 0 || globalLastCaptureWidth >= 10000 || globalLastCaptureHeight >= 10000) {
		printf("Error: Unexpected screen size: %I64d, %I64d\n", (int64_t) globalLastCaptureWidth, (int64_t) globalLastCaptureHeight);
		WSACleanup();
		return 1;
	}
	uint64_t screen_width = globalLastCaptureWidth;
	uint64_t screen_height = globalLastCaptureHeight;
	uint8_t * png_image;

	print_timestamp(1, 1);
    printf("Info: Waiting for connection at port %I64d\n", (int64_t) portnumber);

    while (1) {
		if (listen(sock, 10) == SOCKET_ERROR) {
			printf("Error: Listen function failed\n");
			WSACleanup();
			return 1;
		}

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

		if (recv_length < 4 || recv_buffer[0] != 'G' || recv_buffer[1] != 'E' || recv_buffer[2] != 'T' || recv_buffer[3] != ' ' || recv_buffer[4] != '/') {
			print_timestamp(1, 1);
			printf("Info: Server sent unexpected request method\n");
			send(
				msg_sock,
				"HTTP/1.1 405 Method Not Allowed\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: 33\r\n"
				"Connection: close\r\n"
				"\r\n"
				"Only GET requests are allowed\r\n",
				148+1,
				0
			);
			closesocket(msg_sock);
			continue;
		}

		if (does_first_start_with_second(&recv_buffer[4], "/size/")) {
			print_timestamp(1, 1);
			printf("Info: Client requested utility method '/size/'\n");
			char content_buffer[64];
			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 400 OK\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n"
				"\r\n"
				"%s",
				snprintf(content_buffer, 64, "%d,%d", screen_width, screen_height),
				content_buffer
			);
			send(
				msg_sock,
				buffer,
				buffer_size,
				0
			);
			closesocket(msg_sock);
			continue;
		}

		int left = 0;
		int top = 0;
		int width = screen_width;
		int height = screen_height;
		int is_png_format = 1;
		int is_binary_format = 0;
		int is_json_format = 0;
		int is_region_request = 0;

		int interpretation_code = interpret_http_query(
			&recv_buffer[4],
			recv_length - 4,
			&left, &top,
			&width, &height,
			&is_png_format,
			&is_binary_format,
			&is_json_format,
			&is_region_request
		);

		if (interpretation_code != 1) {
			print_timestamp(1, 1);
			printf("Info: Query interpretation failed with error %d\n", interpretation_code);
			if (interpretation_code == -2) {
				buffer_size = snprintf(
					buffer,
					OUTPUT_BUFFER_SIZE,
					"HTTP/1.1 404 Not Found\r\n"
					"Content-Type: text/html; charset=UTF-8\r\n"
					"Content-Length: 125\r\n"
					"Connection: close\r\n"
					"\r\n"
					"Unknown url path<br/>\r\n"
					"<a href='/'>use /</a> for region requests</br>\r\n"
					"<a href='/pixel/'>use /pixel/</a> for pixel requests\r\n\r\n"
				);
			} else {
				buffer_size = snprintf(
					buffer,
					OUTPUT_BUFFER_SIZE,
					"HTTP/1.1 403 Forbidden\r\n"
					"Content-Type: text/html; charset=UTF-8\r\n"
					"Content-Length: 75\r\n"
					"Connection: close\r\n"
					"\r\n"
					"Invalid or unexpected query parameter. Query interpretation returned %2d\r\n\r\n",
					interpretation_code
				);
			}
			send(
				msg_sock,
				buffer,
				buffer_size,
				0
			);
			closesocket(msg_sock);
			continue;
		}

		if (top < 0 || top > 10000) {
			top = 0;
		}
		if (left < 0 || left > 10000) {
			left = 0;
		}
		if (!is_region_request || is_json_format) {
			width = 1;
			height = 1;
		} else {
			if (width == 0) {
				width = screen_width;
			}
			if (height == 0) {
				height = screen_height;
			}
			if (left + width >= screen_width) {
				width = screen_width - left - 1;
			}
			if (top + height >= screen_height) {
				height = screen_height - top - 1;
			}
		}

		image = CaptureScreenBuffer(left, top, width, height);

		if (image == NULL) {
			print_timestamp(1, 1);
			printf("Info: Screen capture failed for parameters %d %d %d %d\n", left, top, width, height);

			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 403 Forbidden\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: 25\r\n"
				"Connection: close\r\n"
				"\r\n"
				"Screen capture failed\r\n\r\n"
			);
		} else if (globalLastCaptureHeight <= 0 || globalLastCaptureHeight <= 0 || globalLastCaptureHeight >= 10000 || globalLastCaptureHeight >= 10000) {
			print_timestamp(1, 1);
			printf("Info: Last capture size was out of bounds: %I64d and %I64d\n", (int64_t) globalLastCaptureWidth, (int64_t) globalLastCaptureHeight);

			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 403 Forbidden\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: 44\r\n"
				"Connection: close\r\n"
				"\r\n"
				"Invalid or unexpected query capture size\r\n\r\n"
			);
		} else if (is_png_format) {
			//printf("PNG FORMAT\n");
			size_t image_size = 0;
			png_image = (uint8_t *) tdefl_write_image_to_png_file_in_memory_ex(image, globalLastCaptureWidth, globalLastCaptureHeight, 3, &image_size, MZ_NO_COMPRESSION, 0);
			if (png_image == NULL) {
				print_timestamp(1, 1);
				printf("Error: Miniz png creation failed\n");
				WSACleanup();
				return 1;
			}
			print_timestamp(1, 1);
			printf("Info: Server created image of %zd bytes\n", image_size);

			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: image/png; charset=UTF-8\r\n"
				"Content-Length: %zd\r\n"
				"Connection: close\r\n"
				"\r\n",
				image_size
			);

			if (buffer_size + image_size < OUTPUT_BUFFER_SIZE) {
				if (buffer[buffer_size] != '\0') {
					printf("Error: My assumption of the buffer structure is incorrect\n");
					WSACleanup();
					return 1;
				}

				for (i = buffer_size; i < buffer_size + image_size && i < (OUTPUT_BUFFER_SIZE); i++) {
					buffer[i] = png_image[i - buffer_size];
				}
				if (i < (OUTPUT_BUFFER_SIZE)) {
					buffer[i] = '\0';
				}

				mz_free(png_image);

				buffer_size = buffer_size + image_size + 1;
			} else {
				buffer_size = snprintf(
					buffer,
					OUTPUT_BUFFER_SIZE,
					"HTTP/1.1 200 OK\r\n"
					"Content-Type: text/html; charset=UTF-8\r\n"
					"Content-Length: 41\r\n"
					"Connection: close\r\n"
					"\r\n"
					"Buffer to write image output is too small\r\n"
				);
			}
		} else if (is_binary_format) {
			//printf("BINARY FORMAT\n");
			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: application/octet-stream; charset=UTF-8\r\n"
				"Content-Length: %I64d\r\n"
				"Connection: close\r\n"
				"\r\n",
				(int64_t) width * height * 3
			);
			if (buffer_size + width * height * 3 >= OUTPUT_BUFFER_SIZE) {
				buffer_size = snprintf(
					buffer,
					OUTPUT_BUFFER_SIZE,
					"HTTP/1.1 200 OK\r\n"
					"Content-Type: text/html; charset=UTF-8\r\n"
					"Content-Length: 41\r\n"
					"Connection: close\r\n"
					"\r\n"
					"Buffer to write image output is too small\r\n"
				);
			} else {
				memcpy_s(&buffer[buffer_size], OUTPUT_BUFFER_SIZE - 1 - buffer_size, image, width * height * 3);
				buffer_size += width * height * 3;
			}
		} else if (is_json_format) {
			//printf("JSON FORMAT\n");
			buffer_size = snprintf(buffer, OUTPUT_BUFFER_SIZE, "{\"r\": %d, \"g\": %d, \"b\": %d, \"x\": %d, \"y\": %d}", image[0], image[1], image[2], left, top);
			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: application/json; charset=UTF-8\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n"
				"\r\n"
				"{\"r\": %d, \"g\": %d, \"b\": %d, \"x\": %d, \"y\": %d}",
				buffer_size,
				image[0], image[1], image[2], left, top
			);
		} else {
			//printf("TEXT FORMAT\n");
			// Reply with bytes separated by commas and pixels by lines
			int byte_counter = 0;
			for (i = 0; i < width * height * 3; i++) {
				if (image[i] < 10) {
					byte_counter += 1;
				} else if (image[i] < 100) {
					byte_counter += 2;
				} else {
					byte_counter += 3;
				}
				if (i + 1 != width * height * 3) {
					byte_counter += 1;
				}
			}
			buffer_size = snprintf(
				buffer,
				OUTPUT_BUFFER_SIZE,
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n"
				"\r\n",
				byte_counter
			);
			if (buffer[buffer_size] != '\0') {
				print_timestamp(1, 1);
				printf("Error: Unexpected snprintf output\n");
				WSACleanup();
				return 1;
			}
			for (i = 0; i < width * height * 3; i++) {
				if (buffer_size + 3 >= OUTPUT_BUFFER_SIZE) {
					break;
				} else if (image[i] < 10) {
					buffer[buffer_size++] = '0' + image[i];
				} else if (image[i] < 100) {
					buffer[buffer_size++] = '0' + ((image[i] / 10) % 10);
					buffer[buffer_size++] = '0' + ((image[i] % 10) % 10);
				} else {
					buffer[buffer_size++] = '0' + ((image[i] / 100) % 10);
					buffer[buffer_size++] = '0' + ((image[i] / 10) % 10);
					buffer[buffer_size++] = '0' + (image[i] % 10);
				}

				if (i + 1 != width * height * 3) {
					buffer[buffer_size++] = i % 3 == 2 ? '\n' : ',';
				}
			}
			buffer[buffer_size++] = '\r';
			buffer[buffer_size++] = '\n';
			buffer[buffer_size++] = '\0';
		}

		send(
			msg_sock,
			buffer,
			buffer_size,
			0
		);

        closesocket(msg_sock);
    }

    WSACleanup();
	return 0;
}
