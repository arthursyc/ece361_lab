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

struct client clients[8] = {
	{"Alex",		"alexawsm",			false,			-1},
	{"Billy",		"billy123",			false,			-1},
	{"Carol",		"carcarca",			false,			-1},
	{"Dean",		"amongsus",			false,			-1},
	{"Elaine",		"12345678",			false,			-1},
	{"Farquaad",	"shrexy",			false,			-1},
	{"Gwyn",		"gwyngwyn", 		false,			-1},
	{"Hector",		"dingding",			false,			-1}
};

char sessions[MAX_SESS][MAX_NAME];
int sess_num = 0;

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

	int pid;
	while (1) {
		connfd = accept(sockfd, (struct sockaddr*) &cli_addr, &len);
		if (connfd < 0) {
			perror("Server accept failed");
			continue;
		}

		while (1) {
			pid = fork();
			if (pid < 0) {
				perror("Fork failed");
			} else break;
		}
		if (pid > 0) {
			close(connfd);
		} else {
			close(sockfd);

			struct message incoming = getMessage(connfd);
			switch (incoming.type) {
				case LOGIN:
					break;

				case EXIT:
					break;

				case JOIN:
					break;

				case LEAVE_SESS:
					break;

				case NEW_SESS:
					break;

				case QUERY:
					break;

				case MESSAGE:
					break;

				default: ;
			}
		}

	}
}
