#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_LINE 1024

int main(int argc, char* argv[]) {
	if (argc > 2) {
		printf("Too many arguments - format: server <UDP listen port>\n");
		exit(EXIT_FAILURE);
	}
	if (!(atoi(argv[1]))) {
		printf("Invalid port number\n");
		exit(EXIT_FAILURE);
	}

	int sockfd;
	char buffer[MAX_LINE];
	char* msg;
	struct sockaddr_in servaddr, cliaddr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(atoi(argv[1]));

	if (bind(sockfd, (const struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	while(1) {
		int n, len;

		len = sizeof(cliaddr);

		n = recvfrom(sockfd, (char*) buffer, MAX_LINE, MSG_WAITALL, (struct sockaddr*) &cliaddr, &len);

		buffer[n] = '\0';
		if(!(strcmp(buffer, "ftp"))) {
			msg = "yes";
		} else {
			msg = "no";
		}

		sendto(sockfd, (const char*) msg, strlen(msg), MSG_CONFIRM, (const struct sockaddr*) &cliaddr, len);
	}
}
