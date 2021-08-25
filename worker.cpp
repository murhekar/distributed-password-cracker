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
#include <crypt.h>

#define MAX_DATA_SIZE 100 // Maximum Number of Bytes we can get at once
#define NUM_SECS 0
#define NUM_USECS 100000

using namespace std;

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

// Function to incrementally generate a possible password 
// passwd[] is a candidate password
// arr[] is the array of all possible characters
// idx[i] is the index in arr[] that is the character in passwd[i] 
// passwd[i] = arr[idx[i]]  
void increment(char passwd[], char arr[], int idx[]) 
{
	int i, pwd_size = strlen(passwd), arr_size = strlen(arr);
	for (i = pwd_size-1; i >= 0; --i) {
		if (passwd[i] != arr[arr_size-1]) { // If this is not the last character in arr[]
			passwd[i] = arr[idx[i]+1];      // Update this charcter of the passwd
			idx[i] += 1;
			return;
		}
		else { // If this is the last character in arr[], wrap around to 0
			passwd[i] = arr[0];
			idx[i] = 0;
		}
	}
	return;
}

int main (int argc, char *argv[])
{
	srand(time(0)); // Seed the random number generator with current time

	int sockfd; // Socket File Descriptor
	char buf[MAX_DATA_SIZE]; // Buffer
	struct hostent *hserver; // Server Information
	struct sockaddr_in my_addr; // My address informaion
	struct sockaddr_in server_addr; // Server address information
	int numbytes; // Number of bytes
	int i; 
	int fdmax; // Max file descriptor
	int counter;
	int yes = 1; // Used in the setsockopt condition check

	fd_set read_fds; // Set of read fds
	fd_set master; // Master set

	// Structure for timeout
	struct timeval tv;
	tv.tv_sec = NUM_SECS;
	tv.tv_usec = NUM_USECS;

	// Checking right format of input
	if (argc != 3) {
		printf("Usage: ./exec_name <server_ip/hostname> <server-port> \n");
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

	// bind() to a specific port in range (10000, 20000)
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	// Some random port in the range (5000,8000) for the worker
	my_addr.sin_port = htons((rand())%10000 + 10000); 
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

	FD_SET(sockfd, &master);
	fdmax = sockfd;

	while (true) {
		// recv()
		printf("Waiting for next task .. \n");
		if ((numbytes = recv(sockfd, buf, MAX_DATA_SIZE-1, 0)) == -1) {
			perror("recv");
			exit(1);
		}

		// If we receive a message of length 15, then it is the interrupt message
		if (numbytes == 15) {
			continue;
		}

		// If we receive a message of length > 17, then we have received 
		// the interrupt message and then another hash together (i.e. 15+17 = 32 bytes) 
		else if (numbytes > 17) {
			for (i = 15; i < numbytes; ++i) {
				buf[i-15] = buf[i];
			}
			buf[numbytes-15] = '\0';
		} 
		buf[numbytes] = '\0';
		printf("Received:%sof length %d\n, numbytes = %d", buf, (int)strlen(buf), numbytes);

		// Decomposing recvd packet into hash, length and set
		char hash[MAX_DATA_SIZE];
		int set, len; 
		for (i = 0; i < 13; ++i) {
			hash[i] = buf[i];
		}
		hash[i] = '\0';
		len = (int)buf[13]; // Length of password
		set = (int)buf[14]; // Type of password (binary string)

		printf("%s %d %d \n", hash, len, set);

		char arr[64];  // arr[] stores all characters that can be part of the password 
		int setstr[4]; // setstr[] stores  type of characters in the password
		int cnt = 2;
		// Finding setstr[] using set
		while (set > 0 || cnt >= 0) {
			setstr[cnt--] = set&1;
			set >>= 1;
		}
		printf("%d %d %d \n", setstr[0], setstr[1], setstr[2]);

		cnt = 0; // Index to fill arr[] 
		// If numeric characters are present
		if (setstr[2] == 1) {
			for (i = 0; i < 10; ++i) {
				arr[cnt++] = i+'0';
			}
		}
		// If uppercase characters are present
		if (setstr[1] == 1) {
			for (i = 0; i < 26; ++i) {
				arr[cnt++] = i+'A';
			}
		}
		// If lowercase characters are present
		if (setstr[0] == 1) {
			for (i = 0; i < 26; ++i) {
				arr[cnt++] = i+'a';
			}
		}
		arr[cnt] = '\0';

		char passwd[MAX_DATA_SIZE], salt[3];
		int idx[MAX_DATA_SIZE];
		passwd[len] = '\0';
		salt[0] = hash[0];
		salt[1] = hash[1];
		salt[2] = '\0';

		// The position of candidate password in the solution space from where the worker starts checking
		double start = (double)((int)buf[15]-1)/((int)buf[16]); // Ratio of idle workers to total workers
		printf("start = %lf \n", start);
		int str_idx = (int)(10*(start*strlen(arr))); 
		printf("idx = %d \n", str_idx);

		// Building the initial password (first guess is initialized here)
		if (len > 1) {
			idx[1] = str_idx%10;
			passwd[1] = arr[str_idx%10];
		}
		str_idx /= 10;
		idx[0] = str_idx%10;
		passwd[0] = arr[str_idx%10];
		str_idx /= 10;
		for (i = 2; i < len; ++i) {
			idx[i] = 0;
			passwd[i] = arr[0];
		}

		bool done = false;

		printf("Starting to crack the password from %s ..\n", passwd);
		while (1) {
			read_fds = master;
			// Use select() to fill read_fds set with file descriptors ready for reading
			if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
				perror("select");
				exit(1);
			}
			// If the server sends any message, it must be an Interrupt packet 
			for (i = 0; i <= fdmax; ++i) {
				if (FD_ISSET(i, &read_fds)) {
					done = true;
					break;
				}
			}
			counter = 0;
			if (done)
				break;

			// The password-cracking process, checks 25000 at a time to look for interrupt messages
			while (counter < 25000) {
				if (!strcmp(crypt(passwd, salt), hash)) { // If decrypted, break
					done = true;
					break;
				}
				increment(passwd, arr, idx); 
				++counter;
			}
			if (done)
				break;
		}

		// If password not yet cracked by me(in case the server interrupted because 
		// some other worker finished, continue
		if (strcmp(crypt(passwd, salt), hash)) {
			printf("Task interrupted by server \n");
			continue;
		}

		// Else, send the cracked password
		for (i = len; i < 15; ++i) {
			passwd[i] = '0';
		}
		passwd[i] = '\0';
		if ((numbytes = sendeverything(sockfd, passwd)) == -1) {
			perror("recv");
			exit(1);
		}
		printf("Password cracked successfully \n");
		printf("Number of bytes sent = %d\n", numbytes);
	}

	close(sockfd);

	return 0;
}
