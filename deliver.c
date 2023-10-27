#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>

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
	int total_packets = (total_filesize % 1000) ? total_filesize / 1000 + 1: total_filesize / 1000;
	struct packet* packets = (struct packet*) malloc(sizeof(struct packet) * total_packets);
	int cur_packet = 0;
	for (int c = 0; c < total_filesize; c += 1000) {
		struct packet new_packet;

		new_packet.total_frag = total_packets;

		new_packet.frag_no = cur_packet + 1;

		new_packet.size = ((total_filesize - c) > 1000) ? 1000 : (total_filesize - c);

		new_packet.filename = (char *) malloc(sizeof(char) * MAX_LINE);
		strcpy(new_packet.filename, filename);

		fread(new_packet.filedata, sizeof(char), new_packet.size, fp);

		packets[cur_packet] = new_packet;
		++cur_packet;
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

	printf("Section 2: round trip time: %f\n", (double)(end - start)/CLOCKS_PER_SEC);

	fd_set socks;
	FD_ZERO(&socks);
	FD_SET(sockfd, &socks);
	struct timeval t;
	struct timespec timeout_wait;
	t.tv_sec = 0;
	t.tv_usec = 500000;
	timeout_wait.tv_sec = 0;
	char testpacket[] = "1000:-1:1000:this_is_just_test_packet:Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
	while (1) {
		start = clock();
		sendto(sockfd, (const char *)testpacket, strlen(testpacket), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
		if (select((sockfd + 1), &socks, NULL, NULL, &t)) {
			n = recvfrom(sockfd, (char *)buffer, MAX_LINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
			end = clock();
			break;
		}
	}
	int timeout = ((double)(end - start)/CLOCKS_PER_SEC) * 1000000 * 2;
	timeout_wait.tv_nsec = timeout % 1000 * 1000000 * 5;
	printf("Section 4: ACK wait time: %dus\n", timeout);
	sleep(5);

	for (int packet = 0; packet < total_packets; ++packet) {
		char packetstr[MAX_LINE * 2];
		char buffer[MAX_LINE];
		sprintf(packetstr, "%d:%d:%d:%s:", packets[packet].total_frag, packets[packet].frag_no, packets[packet].size, packets[packet].filename);
		memcpy(&packetstr[strlen(packetstr)], packets[packet].filedata, packets[packet].size);
		while (1) {
			start = clock();
			sendto(sockfd, (const char *)packetstr, sizeof(packetstr), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
			printf("Packet sent: %d/%d\n", packets[packet].frag_no, packets[packet].total_frag);
			t.tv_usec = timeout;
			if (select((sockfd + 1), &socks, NULL, NULL, &t)) {
				n = recvfrom(sockfd, (char *)buffer, MAX_LINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
				end = clock();
				buffer[n] = '\0';
				if(!(strcmp(buffer, "ACK"))) {
					printf("Packet ACK received: %d/%d - Round trip time = %f\n", packets[packet].frag_no, packets[packet].total_frag, (double)(end - start)/CLOCKS_PER_SEC);
					break;
				} else if (buffer[0] == 'N') {
					int expecting;
					sscanf(buffer, "NACK:%d", &expecting);
					printf("Packet NACK received: %d/%d - expecting %d/%d\n", packets[packet].frag_no, packets[packet].total_frag, expecting, packets[packet].total_frag);
					packet = expecting;
				}
			} else {
				end = clock();
				printf("Timeout for packet %d/%d - %fs passed - resending\n", packets[packet].frag_no, packets[packet].total_frag, (double)(end - start)/CLOCKS_PER_SEC);
				nanosleep(&timeout_wait, &timeout_wait);
				printf("%d\n", timeout_wait.tv_nsec);
			}
		}
	}
	n = recvfrom(sockfd, (char *)buffer, MAX_LINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
	printf("\n");
	buffer[n] = '\0';
	printf("%s received\n", buffer);
	free(packets);
	close(sockfd);
	exit(EXIT_SUCCESS);
}
