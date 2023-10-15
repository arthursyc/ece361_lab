#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <math.h>
#include <time.h>

#define MAX_LINE 1024

struct packet {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
};

int main(int argc, char* argv[]) {
	if (argc > 3) {
		printf("Too many arguments - format: deliver <server address> <server port number>\n");
		exit(EXIT_FAILURE);
	}
	if (!(atoi(argv[2]))) {
		printf("Invalid port number\n");
		exit(EXIT_FAILURE);
	}

	int sockfd;
	char buffer[MAX_LINE];
	char* ftp = "ftp";
	char cmd[4], filename[MAX_LINE];
	struct sockaddr_in servaddr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);

	printf("Input: ftp <file name>\n");
	scanf("%s %s", &cmd, &filename);
	if(!(strcmp(ftp, cmd))) {
		if (access(filename, F_OK) != 0) {
			exit(EXIT_FAILURE);
		}
	} else {
		printf("Invalid input\n");
		exit(EXIT_FAILURE);
	}

	FILE *fp = fopen(filename, "r");
	fseek(fp, 0, SEEK_END);
	int total_filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	int total_packets = ceil(total_filesize / 1000);
	struct packet* packets = (struct packet*) malloc(sizeof(struct packet) * total_packets);
	int cur_packet = 0;
	for (int c = 0; c < total_filesize; c += 1000) {
		packets[cur_packet].total_frag = total_packets;
		packets[cur_packet].frag_no = cur_packet + 1;
		packets[cur_packet].size = ((total_filesize - 1000) > c) ? 1000 : (total_filesize - c);
		strcpy(packets[cur_packet].filename, filename);
		fread(packets[cur_packet].filedata, sizeof(char), packets[cur_packet].size, fp);
	}

	int n, len;

	clock_t start = clock();
	sendto(sockfd, (const char *)ftp, strlen(ftp), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

	n = recvfrom(sockfd, (char *)buffer, MAX_LINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
	clock_t end = clock();

	buffer[n] = '\0';
	if(!(strcmp(buffer, "yes"))) {
		printf("A file transfer can start.\n");
	}

	printf("Round trip time: %f\n", (double)(end - start)/CLOCKS_PER_SEC);

	for (int packet = 0; packet < total_packets; ++packet) {
		sendto(sockfd, (const struct packet *)&packets[packet], sizeof(packets[packet]), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
	}

	close(sockfd);
	exit(EXIT_SUCCESS);
}
