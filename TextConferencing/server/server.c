#include <stdio.h>
#include <stdbool.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../helpers.h"

int* clifd;
int clifd_num;

struct client clients[8] = {
	{"Alex", "alexawsm", "", 0, false},
	{"Billy", "billy123", "", 0, false},
	{"Carol", "carcarca", "", 0, false},
	{"Dean", "amongsus", "", 0, false},
	{"Elaine", "12345678", "", 0, false},
	{"Farquaad", "shrexy", "", 0, false},
	{"Gwyn", "gwyngwyn", "", 0, false},
	{"Hector", "dingding", "", 0, false}
};

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("Wrong arguments - format: server <TCP listen port>\n");
		exit(EXIT_FAILURE);
	}
	if (!(atoi(argv[1]))) {
		printf("Invalid port number\n");
		exit(EXIT_FAILURE);
	}

	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli_addr;

	clifd_num = 0;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	if ((bind(sockfd, (const struct sockaddr*) &servaddr, sizeof(servaddr))) != 0) {
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	if ((listen(sockfd, 5)) != 0) {
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}

	len = sizeof(cli_addr);

	while (1) {
		int newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &len);
		if (newsockfd < 0) {
			perror("Server accept failed");
			continue;
		}

		char buff[64];
		read(newsockfd, buff, sizeof(buff));

		printf("%s\n", buff);

	}
}
