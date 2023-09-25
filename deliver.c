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
	if (argc > 3) {
		printf("Too many arguments - format: deliver <server address> <server port number>\n");
		exit(EXIT_FAILURE);
	}
	if (!(atoi(argv[1]))) {
		printf("Invalid server address\n");
		exit(EXIT_FAILURE);
	}
	if (!(atoi(argv[2]))) {
		printf("Invalid port number\n");
		exit(EXIT_FAILURE);
	}

	int sockfd;
	char buffer[MAX_LINE];
	char* ftp = "ftp";
	char* cmd[4], filename[MAX_LINE];
	struct sockaddr_in servaddr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr = atoi(argv[1]);

	printf("Input: ftp <file name>\n");
	scanf("%s %s", &cmd, &filename);
	if(!(strcmp(cmd, "ftp"))) {

	}

	int n, len;

	sendto(sockfd, (const char *)hello, strlen(hello),
		MSG_CONFIRM, (const struct sockaddr *) &servaddr,
			sizeof(servaddr));
	printf("Hello message sent.\n");

	n = recvfrom(sockfd, (char *)buffer, MAXLINE,
				MSG_WAITALL, (struct sockaddr *) &servaddr,
				&len);
	buffer[n] = '\0';
	printf("Server : %s\n", buffer);

	close(sockfd);
	return 0;
}
