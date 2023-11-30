#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include "../helpers.h"

int main() {
	int sockfd;
	struct sockaddr_in servaddr, cli;
	char cliid[MAX_NAME];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	int pid = getpid();
	int p[2];
	while (1) {
		if (pid != 0) {

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

				pipe(p);
				pid = fork();
				if (pid == 0) {	// i pray that fork is successful
					close(p[0]);
					continue;
				}

				close(p[1]);
				char outgoing[MAX_DATA];
				sprintf(outgoing, "%d:%d:%s:%s", LOGIN, sizeof(array[2]), array[1], array[2]);
				memcpy(cliid, array[1], sizeof(array[1]));
				free(array);

				write(sockfd, outgoing, sizeof(outgoing));

				while (1) {
					int* type;
					read(p[0], type, sizeof(int));
					if (*type == LO_ACK || *type == LO_NAK) {
						break;
					}
				}

			} else if (strcmp(buffer, "/logout") == 0) {

				char outgoing[MAX_DATA];
				sprintf(outgoing, "%d:%d:%s:%s", EXIT, 0, cliid, "");
				write(sockfd, outgoing, sizeof(outgoing));
				if (pid != getpid()) {
					kill(pid, SIGKILL);
					close(p[0]);
					pid = getpid();
				}

			} else if (strstr(buffer, "/joinsession ") == buffer) {

				char** array = parse(buffer, " ");

				char outgoing[MAX_DATA];
				sprintf(outgoing, "%d:%d:%s:%s", JOIN, sizeof(array[2]), cliid, array[2]);
				free(array);

				write(sockfd, outgoing, sizeof(outgoing));

				while (1) {
					int* type;
					read(p[0], type, sizeof(int));
					if (*type == JN_ACK || *type == JN_NAK) {
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
					int* type;
					read(p[0], type, sizeof(int));
					if (*type == NS_ACK || *type == NS_NAK) {
						break;
					}
				}

			} else if (strcmp(buffer, "/list") == 0) {

				char outgoing[MAX_DATA];
				sprintf(outgoing, "%d:%d:%s:%s", QUERY, 0, cliid, "");

				write(sockfd, outgoing, sizeof(outgoing));

				while (1) {
					int* type;
					read(p[0], type, sizeof(int));
					if (*type == QU_ACK) {
						break;
					}
				}

			} else if (strcmp(buffer, "/quit") == 0) {

				char outgoing[MAX_DATA];
				sprintf(outgoing, "%d:%d:%s:%s", EXIT, 0, cliid, "");
				write(sockfd, outgoing, sizeof(outgoing));
				close(sockfd);
				if (pid != getpid()) {
					kill(pid, SIGKILL);
					close(p[0]);
				}
				return 0;

			} else {

				char outgoing[MAX_DATA];
				sprintf(outgoing, "%d:%d:%s:%s", MESSAGE, sizeof(buffer), cliid, buffer);
				write(sockfd, outgoing, sizeof(outgoing));

			}
		} else {

			struct message incoming = getMessage(sockfd);
			int* type;

			switch (incoming.type) {
				case LO_ACK:
					printf(">> Login successful\n");
					*type = LO_ACK;
					write(p[1], type, sizeof(int));
					break;

				case LO_NAK:
					printf(">> Login failed: %s\n", incoming.data);
					*type = LO_NAK;
					write(p[1], type, sizeof(int));
					break;

				case JN_ACK:
					printf(">> Join successful\n");
					*type = JN_ACK;
					write(p[1], type, sizeof(int));
					break;

				case JN_NAK:
					printf(">> Join failed: %s\n", incoming.data);
					*type = JN_NAK;
					write(p[1], type, sizeof(int));
					break;

				case NS_ACK:
					printf(">> New session created\n");
					*type = NS_ACK;
					write(p[1], type, sizeof(int));
					break;

				case NS_NAK:
					printf(">> New session creation failed: %s\n", incoming.data);
					*type = NS_NAK;
					write(p[1], type, sizeof(int));
					break;

				case QU_ACK:
					printf("%s\n", incoming.data);
					*type = QU_ACK;
					write(p[1], type, sizeof(int));
					break;

				case MESSAGE:
					printf("%s: %s\n", incoming.source, incoming.data);
					break;

				default:
					;
			}

		}
	}
}
