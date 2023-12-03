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
#include <pthread.h>
#include "../helpers.h"

int msg_type = NONE;
bool logged = false;
pthread_mutex_t mtx;

void* handleMessages(void* sockfdptr) {
	int sockfd = *((int*) sockfdptr);
	struct message incoming = getMessage(sockfd, false);

	pthread_mutex_lock(&mtx);
	switch (incoming.type) {
		case LO_ACK:
			printf("- Login successful\n");
			msg_type = LO_ACK;
			logged = true;
			break;

		case LO_NAK:
			printf("- Login failed: %s\n", incoming.data);
			msg_type = LO_NAK;
			logged = false;
			pthread_mutex_unlock(&mtx);
			return NULL;

		default:
			;
	}
	pthread_mutex_unlock(&mtx);
	while (1) {
		pthread_mutex_lock(&mtx);
		if (msg_type == NONE) {
			pthread_mutex_unlock(&mtx);
			break;
		}
		pthread_mutex_unlock(&mtx);
	}

	while (1) {
		incoming = getMessage(sockfd, true);

		pthread_mutex_lock(&mtx);
		if (!logged) {
			pthread_mutex_unlock(&mtx);
			return NULL;
		}

		switch (incoming.type) {
			case JN_ACK:
				printf("- Join successful\n");
				msg_type = JN_ACK;
				break;

			case JN_NAK:
				printf("- Join failed: %s\n", incoming.data);
				msg_type = JN_NAK;
				break;

			case NS_ACK:
				printf("- New session created\n");
				msg_type = NS_ACK;
				break;

			case NS_NAK:
				printf("- New session creation failed: %s\n", incoming.data);
				msg_type = NS_NAK;
				break;

			case QU_ACK:
				printf("%s\n", incoming.data);
				msg_type = QU_ACK;
				break;

			case MESSAGE:
				printf("%s: %s\n", incoming.source, incoming.data);
				msg_type = NONE;
				break;

			default:
				;
		}
		pthread_mutex_unlock(&mtx);
		while (1) {
			pthread_mutex_lock(&mtx);
			if (msg_type == NONE) {
				pthread_mutex_unlock(&mtx);
				break;
			}
			pthread_mutex_unlock(&mtx);
		}
	}
}

int main() {
	int sockfd;
	struct sockaddr_in servaddr, cli;
	char cliid[MAX_NAME];
	pthread_t incoming_thread;

	while (1) {
		printf(">> ");
		char buffer[1024];
		fgets(buffer, sizeof(buffer), stdin);
		buffer[strcspn(buffer, "\n")] = '\0';

		if (strstr(buffer, "/login ") == buffer) {

			char** array = parse(buffer, " ");

			if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
				perror("Socket creation failed");
				exit(EXIT_FAILURE);
			}

			memset(&servaddr, 0, sizeof(servaddr));


			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = inet_addr(array[3]);
			servaddr.sin_port = htons(atoi(array[4]));
			if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0) {
				perror("Connect failed");
				continue;
			} else {
				printf("- Connected\n");
			}

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", LOGIN, sizeof(array[2]), array[1], array[2]);
			memcpy(cliid, array[1], sizeof(array[1]));
			free(array);

			pthread_mutex_init(&mtx, NULL);
			pthread_create(&incoming_thread, NULL, &handleMessages, &sockfd);

			write(sockfd, outgoing, sizeof(outgoing));

			while (1) {
				pthread_mutex_lock(&mtx);
				if (msg_type == LO_ACK || msg_type == LO_NAK) {
					if (msg_type == LO_NAK) pthread_join(incoming_thread, NULL);
					pthread_mutex_unlock(&mtx);
					msg_type = NONE;
					break;
				}
				pthread_mutex_unlock(&mtx);
			}

		} else if (strcmp(buffer, "/logout") == 0) {

			if (!logged) {
				printf("- Not logged in\n");
				continue;
			}

			char outgoing[MAX_DATA];
			pthread_mutex_lock(&mtx);
			logged = false;
			pthread_mutex_unlock(&mtx);
			pthread_join(incoming_thread, NULL);
			sprintf(outgoing, "%d:%d:%s:%s", EXIT, 1, cliid, " ");
			write(sockfd, outgoing, sizeof(outgoing));
			close(sockfd);

		} else if (strstr(buffer, "/joinsession ") == buffer) {

			pthread_mutex_lock(&mtx);
			if (!logged) {
				printf("- Not logged in\n");
				continue;
			}
			pthread_mutex_unlock(&mtx);

			char** array = parse(buffer, " ");

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", JOIN, sizeof(array[1]), cliid, array[1]);
			free(array);

			write(sockfd, outgoing, sizeof(outgoing));

			while (1) {
				pthread_mutex_lock(&mtx);
				if (msg_type == JN_ACK || msg_type == JN_NAK) {
					pthread_mutex_unlock(&mtx);
					msg_type = NONE;
					break;
				}
				pthread_mutex_unlock(&mtx);
			}

		} else if (strcmp(buffer, "/leavesession") == 0) {

			pthread_mutex_lock(&mtx);
			if (!logged) {
				printf("- Not logged in\n");
				continue;
			}
			pthread_mutex_unlock(&mtx);

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", LEAVE_SESS, 1, cliid, " ");
			write(sockfd, outgoing, sizeof(outgoing));

		} else if (strstr(buffer, "/createsession ") == buffer) {

			pthread_mutex_lock(&mtx);
			if (!logged) {
				printf("- Not logged in\n");
				continue;
			}
			pthread_mutex_unlock(&mtx);

			char** array = parse(buffer, " ");

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", NEW_SESS, sizeof(array[1]), cliid, array[1]);
			free(array);

			write(sockfd, outgoing, sizeof(outgoing));

			while (1) {
				pthread_mutex_lock(&mtx);
				if (msg_type == NS_ACK || msg_type == NS_NAK) {
					pthread_mutex_unlock(&mtx);
					msg_type = NONE;
					break;
				}
				pthread_mutex_unlock(&mtx);
			}

		} else if (strcmp(buffer, "/list") == 0) {

			pthread_mutex_lock(&mtx);
			if (!logged) {
				printf("- Not logged in\n");
				continue;
			}
			pthread_mutex_unlock(&mtx);

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", QUERY, 1, cliid, " ");

			write(sockfd, outgoing, sizeof(outgoing));

			while (1) {
				pthread_mutex_lock(&mtx);
				if (msg_type == QU_ACK) {
					pthread_mutex_unlock(&mtx);
					msg_type = NONE;
					break;
				}
				pthread_mutex_unlock(&mtx);
			}

		} else if (strcmp(buffer, "/quit") == 0) {

			pthread_mutex_lock(&mtx);
			if (!logged) {
				printf("- Not logged in\n");
				continue;
			}
			pthread_mutex_unlock(&mtx);

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", EXIT, 1, cliid, " ");
			write(sockfd, outgoing, sizeof(outgoing));
			close(sockfd);
			return 0;

		} else if (buffer[0] == '/') {

			pthread_mutex_lock(&mtx);
			if (!logged) {
				printf("- Not logged in\n");
				continue;
			}
			pthread_mutex_unlock(&mtx);
			printf("- Invalid command\n");

		} else {

			pthread_mutex_lock(&mtx);
			if (!logged) {
				printf("- Not logged in\n");
				continue;
			}
			pthread_mutex_unlock(&mtx);

			char outgoing[MAX_DATA];
			sprintf(outgoing, "%d:%d:%s:%s", MESSAGE, sizeof(buffer), cliid, buffer);
			write(sockfd, outgoing, sizeof(outgoing));

		}
	}
}
