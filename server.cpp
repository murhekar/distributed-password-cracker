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

#define MAX_DATA_SIZE 100
#define MAX_USERS 3
#define MAX_WORKERS 5
#define MAX_CONNECTIONS 8

using namespace std;

// Structure to store job information assigned to a worker
struct task
{
	// If worker is idle, userfd = -1, else userfd of user whose task it is working on
	int userfd; 
	bool working; // Idle - false, otherwise - true
	int start; // Point where he starts/position among the workers who are assigned the task 
	char msg[MAX_DATA_SIZE]; // Carries the message sent
	// Function to update the struct
	void update(int ufd, bool wrking, int strt) {
		userfd = ufd;
		working = wrking;
		start = strt;
	}
};

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
	return bytes_sent; // Should equal the buflen 
}

// Function to recv() until exp_length (expected length) bytes have been received
int recvuntil(int sockfd, char buf[], int len, unsigned int flags, int exp_len)
{
	int nbytes = 0, total_recv = 0, i;
	char temp[MAX_DATA_SIZE];
	// While not all bytes have been received
	while (total_recv < exp_len) {
		if ((nbytes = recv(sockfd, temp, len, flags)) <= 0) { // Error or connection closed
			return nbytes;
		}
		else { // Store the bytes received
			for (i = total_recv; i < total_recv+nbytes; ++i) {  
				buf[i] = temp[i];
			}
			total_recv += nbytes;
		}
		printf("Total Bytes Recvd = %d \n", total_recv);
	}
	buf[total_recv] = '\0';
	return total_recv; // Should equal exp_len
}

// Function that grants to tasks to idle workers
// Decides whether to assign a task to idle workers, or to wait for 
// other workers to finish existing task and then join on this task
bool cond_check(char msg[], int idle_workers, int no_of_workers)
{
	if (no_of_workers == idle_workers) // All workers are idle, assign the task
		return true;
	int len = (int)msg[13];
	int set = (int)msg[14];
	int setstr[4]; // setstr[] stores the type of characters in the password
	int cnt = 2;
	while (set > 0 || cnt >= 0) {
		setstr[cnt--] = set&1;
		set >>= 1;
	}
	cnt = 0; // Number of possible characters that can appear in the password
	// If numeric characters are present
	if (setstr[2] == 1) {
		cnt += 10;
	}
	// If uppercase characters are presen
	if (setstr[1] == 1) {
		cnt += 26;
	}
	// If lowercase characters are present
	if (setstr[0] == 1) {
		cnt += 26;
	}

	printf("pow = %lf idle = %d\n", pow(cnt, len), idle_workers);
	// Condition for granting the task
	if (pow(cnt, len) < (1e7 * idle_workers)) {
		return true;
	}
	else { 
		return false;
	}

}

int main(int argc, char* argv[])
{
	fd_set master_read; // Master File descriptor list(s)
	fd_set master_write; 
	fd_set read_fds; // Temp FD List for read
	fd_set write_fds; // Temp FD List for write

	struct sockaddr_in my_addr; // My socket information
	struct sockaddr_in remote_addr; // Other's socket information

	int fdmax; // MAX FD
	int newfd; // Obtained on new accept()ance
	char buf[MAX_DATA_SIZE];
	int listener; // Socket File Descriptor for listening
	int yes = 1; // Used in the setsockopt condition check
	socklen_t addrlen;
	int i, j;
	int nbytes;
	int no_of_workers, no_of_users;
	string client_type;
	int remote_port;

	// Stores the FDs of users and workers
	set<int> users_set; // Set of users registered with server
	set<int> workers_set; // Set of workers registered with server 
	map<int, task> tasks; // Map of workerfd with the task assigned to the worker
	map<string, int> user_hashes; // Map of hash with userfd

	list<task> pend_users; // List (Used only as a queue) of users whose tasks are pending
	set<int> :: iterator it;

	// Clearing file-descriptors
	FD_ZERO(&master_read);
	FD_ZERO(&master_write);
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	// Checking right format of input
	if (argc != 2) {
		printf("Usage: ./exec_name <server-port> \n");
		exit(1);
	}

	// get the initial listening socket
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	// lose the "Address already in Use" Error Message
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	// bind()
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(atoi(argv[1]));
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
		perror("bind");
		exit(1);
	}

	// listen() on listener port
	if (listen(listener, MAX_CONNECTIONS) == -1) {
		perror("listen");
		exit(1);
	}
	printf("Server listening on port %s ...\n", argv[1]);

	// Add the listener to master set
	FD_SET(listener, &master_read);
	fdmax = listener; // Max file descriptor till now

	while (true) {
		
		read_fds = master_read;
		write_fds = master_write;
		printf("Waiting before select .. \n");
		// Use select() to fill read_fds and write_fds with file descriptors ready for reading & writing
		if (select(fdmax+1, &read_fds, &write_fds, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}
		printf("Select done .. \n");
		// Brute-forcing through all FDs to check which is the socket which can be read from
		for (i = 0; i <= fdmax; ++i) { 
			if (FD_ISSET(i, &read_fds)) {
				if (i == listener) {
					// We are now handling a new connection
					addrlen = sizeof(remote_addr);
					if ((newfd = accept(listener, (struct sockaddr*)&remote_addr, &addrlen)) == -1) {
						perror("accept");
					}
					else {
						FD_SET(newfd, &master_read); // add the newfd to the master set
						if (newfd > fdmax) { // Update the fdmax
							fdmax = newfd;
						}
						getpeername(newfd, (struct sockaddr*)&remote_addr, &addrlen); // Peer name
						struct sockaddr_in *s = (struct sockaddr_in *)&remote_addr;
						remote_port = ntohs(s->sin_port); // Getting the remote port
						if (remote_port >= 5000 && remote_port <= 8000)
						{
							// Connection from a User identified based on port
							client_type = "User";
							users_set.insert(newfd); // Insert into users_set
						}
						else
						{
							// Connection from a Worker identified based on port
							client_type = "Worker";
							workers_set.insert(newfd); // Insert into workers_set
							task t;
							t.update(-1, false, -1);
							tasks[newfd] = t; // Add into the map, with userfd = -1 (idle)
						}
						printf("New %s connected, IP : %s \n", client_type.c_str(), inet_ntoa(remote_addr.sin_addr));
						printf("Socket : %d \n", newfd);
					}
				}
				else { // if i is not listener
					// recvuntil() 15 bytes
					if ((nbytes = recvuntil(i, buf, sizeof(buf), 0, 15)) <= 0) { 
						if (nbytes == 0) {
							printf("Connection closed. \n");
						}
						else if (nbytes == -1) {
							perror("recv");
						}
						close(i);
						FD_CLR(i, &master_read); // Clear the master_read set
						if (i == fdmax) {
							fdmax -= 1;
						}
						// Remove the user/worker from the relevant map
						list<task> :: iterator it1;
						for (it1 = pend_users.begin(); it1 != pend_users.end(); ++it1) {
							if (it1->userfd == i) {
								pend_users.erase(it1);
								break;
							}
						}
						// Remove the user/worker from the relevant set
						if (users_set.find(i) != users_set.end()) {
							users_set.erase(i);
						}
						else {
							workers_set.erase(i);
						}
					}
					else {	
						// Received message, process based on User or Worker
						printf("Message recvd is %s \n", buf);
						if (users_set.find(i) != users_set.end()) { // if i is user
							task t; // Struct stores info about this user's task
							t.update(i, false, -1);
							strcpy(t.msg, buf);		 
							pend_users.push_back(t); // Push into the pending users queue
						}
						else if (workers_set.find(i) != workers_set.end()) { // if i is worker
							// Worker contacted server with the cracker password, send to corresponding user
							int sendfd = tasks[i].userfd;
							if ((nbytes = sendeverything(sendfd, buf)) == -1) {
								perror("recv");
							}
							// Update the workers
							tasks[i].update(-1, false, -1); // Make this worker idle
							for (it = workers_set.begin(); it != workers_set.end(); ++it) {
								// Send interrupt message to all workers working on the same task 
								if (tasks[*it].userfd == sendfd && (*it != i)) {
									tasks[*it].update(-1, false, -1);
									char packet[MAX_DATA_SIZE]; // Interrupt message, 15 zeroes
									for (j = 0; j < 15; ++j) {
										packet[j] = '0';
									}
									packet[j] = '\0';
									// Sending interrupt message                      
									if ((nbytes = sendeverything(*it, packet)) == -1) {
										perror("recv");
									}
									printf("Packet sent through socket %d \n", *it);
									// Wait until starting on the next task
									for (i = 0; i < 1000000; ++i);
								}
							}
						}
					}
				}
			}

			// Check the Pending Users Queue here
			if (pend_users.empty())
				continue;

			no_of_workers = 0; // Number of idle workers
			for (it = workers_set.begin(); it != workers_set.end(); ++it) {
				if (tasks[*it].userfd == -1) {
					++no_of_workers;
				}
			}

			// Invoke cond_check to grant task to idle users or not
			if (cond_check(pend_users.front().msg, no_of_workers, workers_set.size())) {
				int us_fd = pend_users.front().userfd;
				string str(pend_users.front().msg);
				user_hashes[str] = us_fd; // Map of hash with userfd
				// Assigning different jobs to idle users
				int job_assign = 0; 
				for (it = workers_set.begin(); it != workers_set.end(); ++it) {
					if (tasks[*it].userfd == -1) {
						// Construct a 17-byte packet and send with job_assign and no_of_workers
						// 15-byte is the recvd msg from user
						// 16th byte and 17th byte determine position where the worker
						// must start computation
						char packet[MAX_DATA_SIZE];
						for (j = 0; j < str.length(); ++j) {
							packet[j] = str[j];
						}
						// job_assign is the position of this worker among all the workers assigned this task
						// 1-indexed clearly
						packet[j] = (char)job_assign+1;
						packet[j+1] = (char)no_of_workers;
						packet[j+2] = '\0';
						if ((nbytes = sendeverything(*it, packet)) == -1) {
							perror("recv");
						}
						tasks[*it].update(us_fd, true, job_assign);
						++job_assign;
					}
				}
				pend_users.pop_front(); // Since task has been assigned, pop from pending users list
			}
		}
	}
	return 0;
}