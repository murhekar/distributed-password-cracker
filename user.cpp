#include <bits/stdc++.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <chrono>

#define MAX_DATA_SIZE 100 // Maximum Number of Bytes we can get at once

using namespace std;

// Function to convert binary string to decimal
int decimal(char n[]) 
{
	int res = 0, prod = 1, i;
	for (i = strlen(n)-1; i >= 0; --i) {
		res += (prod*(n[i]-'0'));
		prod *= 2;
	}
	return res;
}

// Ensures that all bytes of the message are sent
int sendeverything(int sockfd, char buf[]) 
{
	int buflen = strlen(buf);
	int bytes_sent = 0;
	int flag = 0;
	// While not all bytes have been sent
	while (bytes_sent < buflen) {
		// Send the unsent bytes again
		flag = send(sockfd, buf+bytes_sent, (buflen - bytes_sent), 0); 
		if (flag == -1) {
			return -1;
		}
		bytes_sent += flag;
		flag = 0;
		printf("Bytes Sent = %d \n", bytes_sent);
	}
	return bytes_sent;
}

int main (int argc, char *argv[])
{
	srand(time(0)); // Seed the random number generator with current time

	int sockfd; // Socket File Descriptor
	char buf[MAX_DATA_SIZE]; // Buffer
	struct hostent *hserver; // Server Infon
	struct sockaddr_in my_addr; // My address information
	struct sockaddr_in server_addr; // Server address information
	int buflen; // Length of buffer 
	int i;
	int yes = 1; // Used in the setsockopt condition check

	// chrono used to calc time taken, beginning point of time noted
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	// Checking right format of input
	if (argc != 6) {
		printf("Usage: ./user <server ip/host-name> <server-port> <hash> <passwd-length> <binary-string> \n");
		exit(1);
	}

	// Resolving Server IP 
	if ((hserver = gethostbyname(argv[1])) == NULL) {
		herror("gethostbyname");
		exit(1);
	}

	// Printing Server Details
	printf("Server Name       : %s \n", hserver->h_name);
	printf("Server IP Address : %s \n", inet_ntoa(*((struct in_addr *)hserver->h_addr)));
	
	// Getting socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	
	// lose the "Address already in Use" Error Message
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	// bind() to a specific port in range (5000, 8000)
	my_addr.sin_family = AF_INET; 
	my_addr.sin_addr.s_addr = INADDR_ANY;
	// Some random port in the range (5000,8000) for the user
	my_addr.sin_port = htons((rand())%3000 + 5000); 
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
		perror("bind");
		exit(1);
	}

	// Initializing server details for connect()
	server_addr.sin_family = AF_INET; 
	server_addr.sin_port = htons(atoi(argv[2]));
	server_addr.sin_addr = *((struct in_addr*)hserver->h_addr);
	
	// connect()ing to the server
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}
	
	// Storing the information appropriately in a buffer
	// 13 Bytes Hash + 1 Byte Password Length +  1 Byte Type of Binary String (in decimal)
	char *p = argv[3];
	for (i = 0; i < strlen(argv[3]); ++i, ++p) {
		buf[i] = *p;
	}
	buf[i] = (char)(atoi(argv[4]));
	printf("Password Length = %d\n", atoi(argv[4]));
	++i;
	buf[i] = (char)(decimal(argv[5]));
	printf("Password Type = %d\n", (int)buf[i]);
	++i;
	buf[i] = '\0';
	printf("Hash = %s\n", buf);

	// send()ing the data to server
	if (sendeverything(sockfd, buf) == -1) {
		perror("send");
		exit(1);
	}

	// recv()ing the cracked password from server
	if ((buflen = recv(sockfd, buf, MAX_DATA_SIZE-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}
	buf[atoi(argv[4])] = '\0';

	// Printing the password on the console
	printf("\nPassword is : %s \n", buf);

	// Printing time taken on the console using end time
	std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
	printf("Time taken = %lf s\n", (double)std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()/1000000);
	close(sockfd);
	
	return 0;
}