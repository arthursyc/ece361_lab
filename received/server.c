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
	n = recvfrom(sockfd, (char*) buffer, MAX_LINE, MSG_WAITALL, (struct sockaddr*) &cliaddr, &len);
	buffer[n] = '\0';
	if(!(strcmp(buffer, "ftp"))) {
		msg = "yes";
	} else {
		msg = "no";
	}

	sendto(sockfd, (const char*) msg, strlen(msg), MSG_CONFIRM, (const struct sockaddr*) &cliaddr, len);

	char recvpacketstr[MAX_LINE * 2];
	char* filedata;
	FILE *fp;
	bool received = false;
	char ACK[] = "ACK";
	int cur_packets = 0;
	int expecting_packet = 1;
	int total_filesize = 0;
	while (1) {
		if (rand() % 10) {
			printf("Receiving packet...\n");
			recvfrom(sockfd, recvpacketstr, sizeof(recvpacketstr), MSG_WAITALL, (struct sockaddr*) &cliaddr, &len);
		} else {
			printf("Ignoring packet...\n");
			recvfrom(sockfd, recvpacketstr, sizeof(recvpacketstr), MSG_WAITALL, (struct sockaddr*) &cliaddr, &len);
			continue;
		}

		struct packet recvpacket;
		int C = 0;

		int c = 0;
		char temp_str_holder[MAX_LINE];
		while (recvpacketstr[C] != ':') {
			temp_str_holder[c++] = recvpacketstr[C++];
		}
		temp_str_holder[c] = '\0';
		recvpacket.total_frag = atoi(temp_str_holder);
		++C;

		c = 0;
		while (recvpacketstr[C] != ':') {
			temp_str_holder[c++] = recvpacketstr[C++];
		}
		temp_str_holder[c] = '\0';
		recvpacket.frag_no = atoi(temp_str_holder);
		++C;

		c = 0;
		while (recvpacketstr[C] != ':') {
			temp_str_holder[c++] = recvpacketstr[C++];
		}
		temp_str_holder[c] = '\0';
		recvpacket.size = atoi(temp_str_holder);
		++C;

		c = 0;
		recvpacket.filename = (char *) malloc(sizeof(char) * MAX_LINE);
		while (recvpacketstr[C] != ':') {
			recvpacket.filename[c++] = recvpacketstr[C++];
		}
		++C;

		memcpy(recvpacket.filedata, &recvpacketstr[C], recvpacket.size);
		int pid = 1;
		if (recvpacket.frag_no == -1) {
			pid = fork();
			if (pid != 0 && pid != -1) {
				continue;
			}
		}

		if (recvpacket.frag_no != expecting_packet && recvpacket.frag_no != -1) {
			char NACKMSG[MAX_LINE];
			sprintf(NACKMSG, "NACK:%d", expecting_packet);
			sendto(sockfd, (const char*) NACKMSG, strlen(NACKMSG), MSG_CONFIRM, (const struct sockaddr*) &cliaddr, len);
			printf("Packet already received, NACK sent: %d/%d - expecting %d/%d\n", recvpacket.frag_no, recvpacket.total_frag, expecting_packet, recvpacket.total_frag);
			free(recvpacket.filename);
			continue;
		}

		if (!received) {
			filedata = (char *) malloc(sizeof(char) * recvpacket.total_frag * 1000);
			if (pid != 0) {
				received = true;
			}
		}

		if (recvpacket.frag_no == 1) {
			fp = fopen(recvpacket.filename, "w+");
		}

		total_filesize += recvpacket.size;

		if (pid == 0) {
			recvpacket.frag_no = 1;
		}

		for (int c = 0; c < recvpacket.size; ++c) {
			filedata[(recvpacket.frag_no - 1) * 1000 + c] = recvpacket.filedata[c];
		}

		sendto(sockfd, (const char*) ACK, strlen(ACK), MSG_CONFIRM, (const struct sockaddr*) &cliaddr, len);

		free(recvpacket.filename);

		if (pid == 0) {
			printf("Test packet ACKed\n");
			return 0;
		} else {
			printf("Packet %s sent: %d/%d\n", ACK, recvpacket.frag_no, recvpacket.total_frag);
		}
		++expecting_packet;

		if(++cur_packets == recvpacket.total_frag){
			fwrite(filedata, sizeof(char), sizeof(char) * total_filesize, fp);
			sendto(sockfd, (const char*) ACK, strlen(ACK), MSG_CONFIRM, (const struct sockaddr*) &cliaddr, len);
			printf("File %s sent\n", ACK);
			fclose(fp);
			free(filedata);
			break;
		}
	}
}
