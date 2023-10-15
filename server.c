#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_LINE 1024

struct packet {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
};

int main(int argc, char* argv[]) {
	if (argc > 2) {
		printf("Too many arguments - format: server <UDP listen port>\n");
		exit(EXIT_FAILURE);
	}
	if (!(atoi(argv[1]))) {
		printf("Invalid port number\n");
		exit(EXIT_FAILURE);
	}

	int sockfd;
	char buffer[MAX_LINE];
	char* msg;
	struct sockaddr_in servaddr, cliaddr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(atoi(argv[1]));

	if (bind(sockfd, (const struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	int n, len;

	len = sizeof(cliaddr);
	//printf("ready\n");
	n = recvfrom(sockfd, (char*) buffer, MAX_LINE, MSG_WAITALL, (struct sockaddr*) &cliaddr, &len);
	//printf("received\n");
	buffer[n] = '\0';
	if(!(strcmp(buffer, "ftp"))) {
		msg = "yes";
	} else {
		msg = "no";
	}

	sendto(sockfd, (const char*) msg, strlen(msg), MSG_CONFIRM, (const struct sockaddr*) &cliaddr, len);

	struct packet* recvpacket = (struct packet*) malloc(sizeof(struct packet));
	recvpacket->filename = (char*) malloc(sizeof(char) * MAX_LINE);
	char* filedata;
	FILE *fp;
	bool received = false;
	char ACK[] = "ACK";
	int cur_packets = 0;
	while (1) {
		recvfrom(sockfd, recvpacket, sizeof(struct packet), MSG_WAITALL, (struct sockaddr*) &cliaddr, &len);
		if (!received) {
			printf("%d \n", recvpacket->total_frag);
			filedata = (char *) malloc(sizeof(char) * recvpacket->total_frag * 1000);

			received = true;
		}

		if (recvpacket->frag_no == 1) {
			fp = fopen(recvpacket->filename, "w+");
			printf("received head\n");
			printf("%s\n", recvpacket->filename);
		}

		for (int c = 0; c < recvpacket->size; ++c) {
			filedata[(recvpacket->frag_no - 1) * 1000 + c] = recvpacket->filedata[c];
		}

		sendto(sockfd, (const char*) ACK, strlen(ACK), MSG_CONFIRM, (const struct sockaddr*) &cliaddr, len);

		cur_packets = cur_packets + 1;
		if(cur_packets == recvpacket->total_frag){
			printf("1");
			printf("its fwrite");
			//fwrite(filedata, sizeof(char), sizeof(filedata), fp);
			printf("not really");
			if(fp==NULL) printf("NULL");
			//fclose(fp);
			// free(filedata);
			// free(recvpacket->filename);
			// free(recvpacket);
			break;
		}
		
	}
}
