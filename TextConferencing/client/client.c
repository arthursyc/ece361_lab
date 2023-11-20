#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../helpers.h"

int main() {
	int sockfd;
	struct sockaddr_in servaddr, cli;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	while (1) {
		char buffer[1024];
		fgets(buffer, 1024, stdin);


		if (strstr(buffer, "/login") == buffer) {

		} else if (strstr(buffer, "/logout") == buffer) {

		} else if (strstr(buffer, "/login") == buffer) {

		} else if (strstr(buffer, "/joinsession") == buffer) {

		} else if (strstr(buffer, "/createsession") == buffer) {

		} else if (strstr(buffer, "/list") == buffer) {

		} else if (strstr(buffer, "/quit") == buffer) {

		} else {

		}
	}
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("128.100.13.171");
	servaddr.sin_port = htons(2000);

	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0) {
		perror("Connect failed");
		exit(EXIT_FAILURE);
	} else {
		printf("connected\n");
	}

	write(sockfd, "hi", sizeof(char) * 2);
}
