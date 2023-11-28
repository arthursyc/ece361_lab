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
	{"Alex",		"alexawsm",			-1,			false,			""},
	{"Billy",		"billy123",			-1,			false,			""},
	{"Carol",		"carcarca",			-1,			false,			""},
	{"Dean",		"amongsus",			-1,			false,			""},
	{"Elaine",		"12345678",			-1,			false,			""},
	{"Farquaad",	"shrexy",			-1,			false,			""},
	{"Gwyn",		"gwyngwyn", 		-1,			false,			""},
	{"Hector",		"dingding",			-1,			false,			""}
};

char sessions[MAX_SESS][MAX_NAME];
int sess_usercount[MAX_SESS];
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

			int cli_index = 0;
			for (; cli_index < 8; ++cli_index){
				if (strcmp(clients[cli_index].id, incoming.source) == 0) {
					break;
				}
			}

			int sess_index;

			char outgoing[MAX_DATA];
			switch (incoming.type) {
				case LOGIN:
					if (cli_index == 8) {

						sprintf(outgoing, "%d:%d:%s:%s", LO_NAK, 14, "server", "User not found");

					} else if (clients[cli_index].online) {

						sprintf(outgoing, "%d:%d:%s:%s", LO_NAK, 19, "server", "User already online");

					} else if (strcmp(clients[cli_index].pwd, incoming.data) == 0) {

						sprintf(outgoing, "%d:%d:%s:%s", LO_NAK, 14, "server", "Wrong password");

					} else {

						clients[cli_index].online = true;
						clients[cli_index].fd = connfd;
						sprintf(outgoing, "%d:%d:%s:%s", LO_ACK, 0, "server", "");

					}
					write(connfd, outgoing, sizeof(outgoing));
					break;

				case EXIT:
					if (cli_index < 8 && clients[cli_index].online) {
						clients[cli_index].online = false;
						clients[cli_index].fd = -1;
					}
					break;

				case JOIN:
					sess_index = findSess(incoming.data, sessions, sess_num);
					if (sess_index == -1) {

						sprintf(outgoing, "%d:%d:%s:%s", JN_NAK, 17, "server", "Session not found");

					} else {

						memcpy(clients[cli_index].sess, incoming.data, incoming.size);
						++sess_usercount[sess_index];
						sprintf(outgoing, "%d:%d:%s:%s", JN_ACK, 0, "server", "");

					}
					write(connfd, outgoing, sizeof(outgoing));
					break;

				case LEAVE_SESS:
					sess_index = findSess(clients[cli_index].sess, sessions, sess_num);
					memset(clients[cli_index].sess, 0, MAX_NAME);
					if (--sess_usercount[sess_index] == 0) {
						for (int i = sess_index; i < sess_num - 1; ++i) {
							memcpy(sessions[i], sessions[i+1], MAX_NAME);
							sess_usercount[i] = sess_usercount[i+1];
						}
						memset(sessions[--sess_num], 0, MAX_NAME);
					}

				case NEW_SESS:
					if (sess_num == MAX_SESS) {

						sprintf(outgoing, "%d:%d:%s:%s", NS_NAK, 18, "server", "Max sessions reached");

					} else {

						sess_index = findSess(incoming.data, sessions, sess_num);
						if (sess_index == -1) {

							memcpy(sessions[sess_num], incoming.data, incoming.size);
							memcpy(clients[cli_index].sess, incoming.data, incoming.size);
							sprintf(outgoing, "%d:%d:%s:%s", NS_ACK, 0, "server", "");

						} else {

							sprintf(outgoing, "%d:%d:%s:%s", NS_NAK, 22, "server", "Session already exists");

						}

					}
					write(connfd, outgoing, sizeof(outgoing));
					break;

				case QUERY: ;

					char query[MAX_DATA] = "";
					strcat(query, "Online Users:\n");
					for (int i = 0; i < 8; ++i) {
						if (clients[i].online) {
							strcat(query, clients[i].id);
							strcat(query, "\n");
						}
					}
					strcat(query, "\nAvailable Sessions and Number of Users in Session:\n");
					for (int i = 0; i < sess_num; ++i) {
						char session[MAX_DATA];
						sprintf(session, "%s - %d\n", sessions[i], sess_usercount[i]);
						strcat(query, session);
					}
					strcat(query, "\n");

					sprintf(outgoing, "%d:%d:%s:%s", QU_ACK, sizeof(query), "server", query);
					write(connfd, outgoing, sizeof(outgoing));
					break;

				case MESSAGE:
					for (int i = 0; i < 8; ++i) {
						if (i != cli_index &&
						  clients[i].online &&
						  strcmp(clients[i].sess, clients[cli_index].sess) == 0) {

							sprintf(outgoing, "%d:%d:%s:%s", MESSAGE, incoming.size, incoming.source, incoming.data);
							write(clients[i].fd, outgoing, sizeof(outgoing));

						}
					}
					break;

				case REGIS: ;
					//assumme source is username and data is password, expand list of clients
					struct client new_clients[cli_index+1];
					for(int i = 0; i < cli_index+1; cli_index ++){
						if(i < cli_index){
							new_clients[i] = clients[i];
						}else{
							struct client new_client = {incoming.source, incoming.data, FALSE, -1};
							new_clients[i] = new_client;
							clients = new_clients;
						}
					}
					break;
				default: ;
			}
		}

	}
}
