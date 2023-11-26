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

			struct message msg = {LOGIN, sizeof(array[2]), "", ""};
			memcpy(msg.source, array[1], sizeof(array[1]));
			memcpy(cliid, array[1], sizeof(array[1]));
			memcpy(msg.data, array[2], sizeof(array[2]));
			char* packet = malloc(2*sizeof(msg));
			free(array);
			// send


			//

			free(packet);

		} else if (strcmp(buffer, "/logout") == 0) {

			struct message msg = {EXIT, 0, "", ""};
			memcpy(msg.source, cliid, sizeof(cliid));

		} else if (strstr(buffer, "/joinsession ") == buffer) {

			char** array = parse(buffer, " ");

			struct message msg = {JOIN, sizeof(array[1]), "", ""};
			memcpy(msg.source, cliid, sizeof(cliid));
			memcpy(msg.data, array[1], sizeof(array[1]));
			free(array);


		} else if (strcmp(buffer, "/leavesession") == 0) {

			struct message msg = {LEAVE_SESS, 0, "", ""};
			memcpy(msg.source, cliid, sizeof(cliid));

		} else if (strstr(buffer, "/createsession ") == buffer) {

			char** array = parse(buffer, " ");
			struct message msg = {NEW_SESS, sizeof(array[1]), "", ""};
			memcpy(msg.source, cliid, sizeof(cliid));
			memcpy(msg.data, array[1], sizeof(array[1]));
			free(array);

		} else if (strcmp(buffer, "/list") == 0) {

			struct message msg = {QUERY, 0, "", ""};
			memcpy(msg.source, cliid, sizeof(cliid));

		} else if (strcmp(buffer, "/quit") == 0) {

			return 0;

		} else {

			struct message msg = {MESSAGE, sizeof(buffer), "", ""};
			memcpy(msg.source, cliid, sizeof(cliid));
			memcpy(msg.data, buffer, sizeof(buffer));

		}
	}
}
