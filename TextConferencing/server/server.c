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
struct client clients[8] = {
	{"Alex",		"alexawsm",			-1,			false,			NULL},
	{"Billy",		"billy123",			-1,			false,			NULL},
	{"Carol",		"carcarca",			-1,			false,			NULL},
	{"Dean",		"amongsus",			-1,			false,			NULL},
	{"Elaine",		"12345678",			-1,			false,			NULL},
	{"Farquaad",	"shrexy",			-1,			false,			NULL},
	{"Gwyn",		"gwyngwyn", 		-1,			false,			NULL},
	{"Hector",		"dingding",			-1,			false,			NULL}
};
pthread_mutex_t client_mtx;

struct session* sess_head;
pthread_mutex_t sess_mtx;
client_list * list;



void* handleMessages(void* connfdptr) {
	int connfd = *((int*) connfdptr);
	while (1) {
		struct message incoming = getMessage(connfd, false);

		int cli_index = 0;
		pthread_mutex_lock(&client_mtx);
		for (; cli_index < 8; ++cli_index) {
			if (strcmp(clients[cli_index].id, incoming.source) == 0) {
				break;
			}
		}
		pthread_mutex_unlock(&client_mtx);

		struct session* sess;

		char outgoing[MAX_DATA];
		switch (incoming.type) {
			case LOGIN:
				pthread_mutex_lock(&client_mtx);
				if (cli_index == 8) {
					sprintf(outgoing, "%d:%d:%s:%s", LO_NAK, 14, "server", "User not found");

				} else if (clients[cli_index].online) {
					sprintf(outgoing, "%d:%d:%s:%s", LO_NAK, 19, "server", "User already online");

				} else if (strcmp(clients[cli_index].pwd, incoming.data) == 0) {
					sprintf(outgoing, "%d:%d:%s:%s", LO_NAK, 14, "server", "Wrong password");

				} else {
					clients[cli_index].online = true;
					clients[cli_index].fd = connfd;
					sprintf(outgoing, "%d:%d:%s:%s", LO_ACK, 1, "server", " ");
					printf("%s logged in\n", clients[cli_index].id);

				}
				write(connfd, outgoing, sizeof(outgoing));
				if (!clients[cli_index].online) {
					close(connfd);
					clients[cli_index].fd = -1;
					return NULL;
				}
				pthread_mutex_unlock(&client_mtx);
				break;

			case EXIT:
				pthread_mutex_lock(&client_mtx);
				if (cli_index < 8 && clients[cli_index].online) {
					clients[cli_index].online = false;
					clients[cli_index].fd = -1;
					printf("%s logged out\n", clients[cli_index].id);
				}
				pthread_mutex_unlock(&client_mtx);
				close(connfd);
				return NULL;
				break;

			case JOIN:
				pthread_mutex_lock(&sess_mtx);
				sess = findSess(incoming.data, sess_head);
				if (sess == NULL) {

					sprintf(outgoing, "%d:%d:%s:%s", JN_NAK, 17, "server", "Session not found");

				} else {

					pthread_mutex_lock(&client_mtx);
					clients[cli_index].sess = sess;
					++sess->usercount;
					sprintf(outgoing, "%d:%d:%s:%s", JN_ACK, 1, "server", " ");
					printf("%s joined session %s\n", clients[cli_index].id, sess->id);
					pthread_mutex_unlock(&client_mtx);

				}
				pthread_mutex_unlock(&sess_mtx);
				write(connfd, outgoing, sizeof(outgoing));
				break;

			case LEAVE_SESS:
				pthread_mutex_lock(&sess_mtx);
				sess = clients[cli_index].sess;
				if (sess != NULL) {
					pthread_mutex_lock(&client_mtx);
					clients[cli_index].sess = NULL;
					printf("%s left session %s\n", clients[cli_index].id, sess->id);
					pthread_mutex_unlock(&client_mtx);
					if (--sess->usercount == 0) {
						struct session* cur = sess_head;
						for (; cur->next != sess; cur = cur->next);
						cur->next = sess->next;
						printf("%s closed\n", sess->id);
						free(sess);
					}
				}
				pthread_mutex_unlock(&sess_mtx);
				break;

			case NEW_SESS:
				pthread_mutex_lock(&sess_mtx);
				sess = findSess(incoming.data, sess_head);
				if (sess == NULL) {

					struct session* new_sess = malloc(sizeof(struct session));
					memcpy(new_sess->id, incoming.data, incoming.size);
					new_sess->usercount = 0;
					new_sess->next = NULL;
					struct session* cur = sess_head;
					for (; cur->next != NULL; cur = cur->next);
					cur->next = new_sess;
					pthread_mutex_lock(&client_mtx);
					clients[cli_index].sess = new_sess;
					printf("%s created session %s\n", clients[cli_index].id, new_sess->id);
					pthread_mutex_unlock(&client_mtx);
					sprintf(outgoing, "%d:%d:%s:%s", NS_ACK, 1, "server", " ");

				} else {

					sprintf(outgoing, "%d:%d:%s:%s", NS_NAK, 22, "server", "Session already exists");

				}
				pthread_mutex_unlock(&sess_mtx);
				write(connfd, outgoing, sizeof(outgoing));
				break;

			case QUERY: ;
				char query[MAX_DATA] = "";
				strcat(query, "Online Users:\n");
				pthread_mutex_lock(&client_mtx);
				for (int i = 0; i < 8; ++i) {
					if (clients[i].online) {
						strcat(query, clients[i].id);
						strcat(query, "\n");
					}
				}
				pthread_mutex_unlock(&client_mtx);

				strcat(query, "\nAvailable Sessions and Number of Users in Session:\n");
				pthread_mutex_lock(&sess_mtx);
				for (struct session* cur = sess_head; cur != NULL; cur = cur->next) {
					char session[MAX_DATA];
					sprintf(session, "%s - %d\n", cur->id, cur->usercount);
					strcat(query, session);
				}
				strcat(query, "\n");
				pthread_mutex_unlock(&sess_mtx);

				sprintf(outgoing, "%d:%d:%s:%s", QU_ACK, sizeof(query), "server", query);
				write(connfd, outgoing, sizeof(outgoing));
				break;

			case MESSAGE:
				pthread_mutex_lock(&client_mtx);
				pthread_mutex_lock(&sess_mtx);
				for (int i = 0; i < 8; ++i) {
					if (i != cli_index &&
						clients[i].online &&
						clients[i].sess == clients[cli_index].sess) {

						sprintf(outgoing, "%d:%d:%s:%s", MESSAGE, incoming.size, incoming.source, incoming.data);
						write(clients[i].fd, outgoing, sizeof(outgoing));

					}
				}
				printf("%s to %s: %s\n", clients[cli_index].id, clients[cli_index].sess->id, incoming.data);
				pthread_mutex_unlock(&client_mtx);
				pthread_mutex_unlock(&sess_mtx);
				break;

			case REGIS: 
				pthread_mutex_lock(&client_mtx);
				if(find_client(list, incoming.source) != NULL){
					;//already registered, throw an message back?
				}
				struct client * new_client = malloc(sizeof(struct client));
				memcpy(new_client->id, incoming.source, sizeof(incoming.source));
				memcpy(new_client->pwd, incoming.data, sizeof(incoming.data));
				new_client->fd = -1;
				new_client->online = true;
				new_client->sess = NULL;
				client_node * new_node = malloc(sizeof(client_node));
				new_node->client = new_client;
				new_node->next = NULL;
				list->tail->next = new_node;
				list->tail = list->tail->next;
				list->length++;
				//write new client onto the file
				FILE* fptr;
				fptr = fopen("clientlist.txt", "a");
				if(fptr == NULL){
					;// message?
				}else{
					fprintf(fptr, "%s:%s", incoming.source, incoming.data);
				}
				fclose(fptr);
				pthread_mutex_unlock(&client_mtx);
				break;
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
