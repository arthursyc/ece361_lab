#include <stdio.h>
#include <stdbool.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "../helpers.h"

pthread_mutex_t client_mtx;
struct session* sess_head;
pthread_mutex_t sess_mtx;
client_list * list;

void* handleMessages(void* connfdptr) {
	int connfd = *((int*) connfdptr);
	while (1) {
		struct message incoming = getMessage(connfd, false);
		printf("Received: %d`%d`%s`%s\n", incoming.type, incoming.size, incoming.source, incoming.data);

		pthread_mutex_lock(&client_mtx);
		client_node* cli_node = find_client(list, incoming.source);
		pthread_mutex_unlock(&client_mtx);

		struct session* sess;

		char outgoing[MAX_DATA];
		switch (incoming.type) {
			case LOGIN:
				pthread_mutex_lock(&client_mtx);
				if (cli_node == NULL) {
					sprintf(outgoing, "%d`%d`%s`%s", LO_NAK, 14, "Server", "User not found");

				} else if (cli_node->client->online) {
					sprintf(outgoing, "%d`%d`%s`%s", LO_NAK, 19, "Server", "User already online");

				} else if (strcmp(cli_node->client->pwd, incoming.data)) {
					sprintf(outgoing, "%d`%d`%s`%s", LO_NAK, 14, "Server", "Wrong password");

				} else {
					cli_node->client->online = true;
					cli_node->client->fd = connfd;
					sprintf(outgoing, "%d`%d`%s`%s", LO_ACK, 1, "Server", " ");
					printf("%s logged in\n", cli_node->client->id);

				}
				write(connfd, outgoing, sizeof(outgoing));
				if (cli_node->client->fd != connfd) {
					close(connfd);
					cli_node->client->fd = -1;
					pthread_mutex_unlock(&client_mtx);
					return NULL;
				} else {
					pthread_mutex_unlock(&client_mtx);
				}
				break;

			case EXIT:
				pthread_mutex_lock(&client_mtx);
				if (cli_node != NULL && cli_node->client->online) {
					pthread_mutex_lock(&sess_mtx);
					sess = cli_node->client->sess;
					if (sess != NULL) {
						char outgoing2[MAX_DATA];
						char leftmsg[MAX_DATA] = "";
						strcat(leftmsg, incoming.source);
						strcat(leftmsg, " left\0");
						for (client_node* cur = list->head; cur != NULL; cur = cur->next) {
							if (cur != cli_node &&
								cur->client->online &&
								cur->client->sess == sess) {

								sprintf(outgoing2, "%d`%d`%s`%s", MESSAGE, sizeof(leftmsg), "Server", leftmsg);
								write(cur->client->fd, outgoing2, sizeof(outgoing2));

							}
						}
						printf("%s left session %s\n", cli_node->client->id, sess->id);
						if (--sess->usercount == 0) {
							if (sess_head->next == NULL) {
								printf("%s closed\n", sess_head->id);
								free(sess_head);
								sess_head = NULL;
							} else {
								struct session* cur = sess_head;
								for (; cur->next != sess; cur = cur->next);
								cur->next = sess->next;
								printf("%s closed\n", sess->id);
								free(sess);
							}
						}
					}
					pthread_mutex_unlock(&sess_mtx);
					cli_node->client->online = false;
					cli_node->client->fd = -1;
					cli_node->client->sess = NULL;
					printf("%s logged out\n", cli_node->client->id);
				}
				pthread_mutex_unlock(&client_mtx);
				close(connfd);
				return NULL;
				break;

			case JOIN:
				pthread_mutex_lock(&sess_mtx);
				sess = findSess(incoming.data, sess_head);
				if (cli_node->client->sess != NULL) {

					sprintf(outgoing, "%d`%d`%s`%s", NS_NAK, 18, "Server", "Already in session");

				} else if (sess == NULL) {

					sprintf(outgoing, "%d`%d`%s`%s", JN_NAK, 17, "Server", "Session not found");

				} else {

					pthread_mutex_lock(&client_mtx);
					cli_node->client->sess = sess;
					++sess->usercount;
					sprintf(outgoing, "%d`%d`%s`%s", JN_ACK, 1, "Server", " ");
					printf("%s joined session %s\n", cli_node->client->id, sess->id);
					char outgoing2[MAX_DATA];
					char joinedmsg[MAX_DATA] = "";
					strcat(joinedmsg, incoming.source);
					strcat(joinedmsg, " joined\0");
					for (client_node* cur = list->head; cur != NULL; cur = cur->next) {
						if (cur != cli_node &&
							cur->client->online &&
							cur->client->sess == cli_node->client->sess) {

							sprintf(outgoing2, "%d`%d`%s`%s", MESSAGE, sizeof(joinedmsg), "Server", joinedmsg);
							write(cur->client->fd, outgoing2, sizeof(outgoing2));

						}
					}
					pthread_mutex_unlock(&client_mtx);

				}
				pthread_mutex_unlock(&sess_mtx);
				write(connfd, outgoing, sizeof(outgoing));
				break;

			case LEAVE_SESS:
				pthread_mutex_lock(&sess_mtx);
				sess = cli_node->client->sess;
				if (sess != NULL) {
					pthread_mutex_lock(&client_mtx);
					char outgoing2[MAX_DATA];
					char leftmsg[MAX_DATA] = "";
					strcat(leftmsg, incoming.source);
					strcat(leftmsg, " left\0");
					for (client_node* cur = list->head; cur != NULL; cur = cur->next) {
						if (cur != cli_node &&
							cur->client->online &&
							cur->client->sess == cli_node->client->sess) {

							sprintf(outgoing2, "%d`%d`%s`%s", MESSAGE, sizeof(leftmsg), "Server", leftmsg);
							write(cur->client->fd, outgoing2, sizeof(outgoing2));

						}
					}
					cli_node->client->sess = NULL;
					printf("%s left session %s\n", cli_node->client->id, sess->id);
					pthread_mutex_unlock(&client_mtx);
					if (--sess->usercount == 0) {
						if (sess_head->next == NULL) {
							printf("%s closed\n", sess_head->id);
							free(sess_head);
							sess_head = NULL;
						} else {
							struct session* cur = sess_head;
							for (; cur->next != sess; cur = cur->next);
							cur->next = sess->next;
							printf("%s closed\n", sess->id);
							free(sess);
						}
					}
				}
				pthread_mutex_unlock(&sess_mtx);
				break;

			case NEW_SESS:
				pthread_mutex_lock(&sess_mtx);
				sess = findSess(incoming.data, sess_head);
				if (cli_node->client->sess != NULL) {

					sprintf(outgoing, "%d`%d`%s`%s", NS_NAK, 18, "Server", "Already in session");

				} else if (sess == NULL) {

					struct session* new_sess = malloc(sizeof(struct session));
					memcpy(new_sess->id, incoming.data, incoming.size);
					new_sess->usercount = 1;
					new_sess->next = NULL;
					if (sess_head != NULL) {
						struct session* cur = sess_head;
						for (; cur->next != NULL; cur = cur->next);
						cur->next = new_sess;
					} else {
						sess_head = new_sess;
					}
					pthread_mutex_lock(&client_mtx);
					cli_node->client->sess = new_sess;
					printf("%s created session %s\n", cli_node->client->id, new_sess->id);
					pthread_mutex_unlock(&client_mtx);
					sprintf(outgoing, "%d`%d`%s`%s", NS_ACK, 1, "Server", " ");

				} else {

					sprintf(outgoing, "%d`%d`%s`%s", NS_NAK, 22, "Server", "Session already exists");

				}
				pthread_mutex_unlock(&sess_mtx);
				write(connfd, outgoing, sizeof(outgoing));
				break;

			case QUERY: ;
				char query[MAX_DATA] = "|-------------------|\n___Online Users___\n";
				pthread_mutex_lock(&client_mtx);
				for (client_node* cur = list->head; cur != NULL; cur = cur->next) {
					if (cur->client->online) {
						strcat(query, cur->client->id);
						strcat(query, "\n");
					}
				}
				pthread_mutex_unlock(&client_mtx);

				strcat(query, "\n___Available Sessions and Number of Users in Session___\n");
				pthread_mutex_lock(&sess_mtx);
				for (struct session* cur = sess_head; cur != NULL; cur = cur->next) {
					char session[MAX_DATA];
					sprintf(session, "%s - %d\n", cur->id, cur->usercount);
					strcat(query, session);
				}
				strcat(query, "\n|-------------------|\n");
				pthread_mutex_unlock(&sess_mtx);

				sprintf(outgoing, "%d`%d`%s`%s", QU_ACK, sizeof(query), "Server", query);
				write(connfd, outgoing, sizeof(outgoing));
				break;

			case MESSAGE:
				if (cli_node->client->sess == NULL) break;
				pthread_mutex_lock(&client_mtx);
				pthread_mutex_lock(&sess_mtx);
				for (client_node* cur = list->head; cur != NULL; cur = cur->next) {
					if (cur != cli_node &&
						cur->client->online &&
						cur->client->sess == cli_node->client->sess) {

						sprintf(outgoing, "%d`%d`%s`%s", MESSAGE, incoming.size, incoming.source, incoming.data);
						write(cur->client->fd, outgoing, sizeof(outgoing));

					}
				}
				printf("%s to %s: %s\n", cli_node->client->id, cli_node->client->sess->id, incoming.data);
				pthread_mutex_unlock(&client_mtx);
				pthread_mutex_unlock(&sess_mtx);
				break;
			/*
			case REGIS:
				pthread_mutex_lock(&client_mtx);
				if(find_client(list, incoming.source) != NULL){
					sprintf(outgoing, "%d`%d`%s`%s", REG_NAK, 1, "Server", " ");;//already registered, throw an message back?
					printf("%s registered\n", incoming.source);
					write(connfd, outgoing, sizeof(outgoing));
					pthread_mutex_unlock(&client_mtx);
					break;
				}
				struct client * new_client = malloc(sizeof(struct client));
				client_node * new_node = malloc(sizeof(client_node));
				new_node->client = new_client;
				printf("1\n");
				memcpy(new_node->client->id, incoming.source, sizeof(incoming.source));
				memcpy(new_node->client->pwd, incoming.data, sizeof(incoming.data));
				printf("2\n");
				new_node->client->fd = connfd;
				new_node->client->online = true;
				new_node->client->sess = NULL;
				new_node->next = NULL;
				printf("3\n");
				list->tail->next = new_node;
				list->tail = list->tail->next;
				list->length++;
				//write new client onto the file
				FILE* fptr;
				fptr = fopen("clientlist.txt", "a");
				printf("4\n");
				if(fptr == NULL){
					;// message?
				}else{
					fprintf(fptr, "%s`%s", incoming.source, incoming.data);
				}
				printf("5\n");
				fclose(fptr);
				sprintf(outgoing, "%d`%d`%s`%s", REG_ACK, 1, "Server", " ");
				printf("%s registered\n", incoming.source);
				write(connfd, outgoing, sizeof(outgoing));
				pthread_mutex_unlock(&client_mtx);
				break;
			*/
			default: ;
		}
	}
}

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

	pthread_mutex_init(&client_mtx, NULL);
	pthread_mutex_init(&sess_mtx, NULL);

	list = load_client_list();

	while (1) {

		connfd = accept(sockfd, (struct sockaddr*) &cli_addr, &len);

		if (connfd < 0) {
			perror("Server accept failed");
			return 1;
		}

		pthread_t incoming_thread;
		pthread_create(&incoming_thread, NULL, &handleMessages, &connfd);
		pthread_detach(incoming_thread);

	}
}
