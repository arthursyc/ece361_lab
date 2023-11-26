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
	char cliid[MAX_NAME];
	char sess[MAX_NAME];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	while (1) {
		char buffer[1024];
		fgets(buffer, sizeof(buffer), stdin);
		buffer[strcspn(buffer, "\n")] = '\0';

		if (strstr(buffer, "/login ") == buffer) {

			char** array = parse(buffer, " ");

			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = inet_addr(array[3]);
			servaddr.sin_port = htons(atoi(array[4]));
			if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0) {
				perror("Connect failed");
				continue;
			} else {
				printf("Connected\n");
			}

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", LOGIN, sizeof(array[2]), array[1], array[2]);
			memcpy(cliid, array[1], sizeof(array[1]));
			free(array);

			write(sockfd, outgoing, sizeof(outgoing));

			while (1) {
				struct message incoming = getMessage(sockfd);
				if (incoming.type == LO_ACK) {
					printf(">> Login successful\n");
					break;
				} else if (incoming.type == LO_NAK) {
					printf(">> Login failed: %s\n", incoming.data);
					break;
				}
			}

		} else if (strcmp(buffer, "/logout") == 0) {

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", EXIT, 0, cliid, "");
			write(sockfd, outgoing, sizeof(outgoing));

		} else if (strstr(buffer, "/joinsession ") == buffer) {

			char** array = parse(buffer, " ");

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", JOIN, sizeof(array[2]), cliid, array[2]);
			free(array);

			write(sockfd, outgoing, sizeof(outgoing));

			while (1) {
				struct message incoming = getMessage(sockfd);
				if (incoming.type == JN_ACK) {
					printf(">> Join successful\n");
					break;
				} else if (incoming.type == JN_NAK) {
					printf(">> Join failed: %s\n", incoming.data);
					break;
				}
			}

		} else if (strcmp(buffer, "/leavesession") == 0) {

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", LEAVE_SESS, 0, cliid, "");
			write(sockfd, outgoing, sizeof(outgoing));

		} else if (strstr(buffer, "/createsession ") == buffer) {

			char** array = parse(buffer, " ");

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", NEW_SESS, sizeof(array[2]), cliid, array[2]);
			free(array);

			write(sockfd, outgoing, sizeof(outgoing));

			while (1) {
				struct message incoming = getMessage(sockfd);
				if (incoming.type == NS_ACK) {
					printf(">> New session created\n");
					break;
				} else if (incoming.type == JN_NAK) {
					printf(">> New session creation failed: %s\n", incoming.data);
					break;
				}
			}

		} else if (strcmp(buffer, "/list") == 0) {

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", QUERY, 0, cliid, "");

			write(sockfd, outgoing, sizeof(outgoing));

			while (1) {
				struct message incoming = getMessage(sockfd);
				if (incoming.type == QU_ACK) {
					printf(">> List:\n%s\n", incoming.data);
					break;
				}
			}

		} else if (strcmp(buffer, "/quit") == 0) {

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", EXIT, 0, cliid, "");
			write(sockfd, outgoing, sizeof(outgoing));
			close(sockfd);
			return 0;

		} else {

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", MESSAGE, sizeof(buffer), cliid, buffer);
			write(sockfd, outgoing, sizeof(outgoing));

		}
	}
}
