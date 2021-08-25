CS 252 Lab 6 : SOCKET PROGRAMMING
---------------------------------

Group Members	
-------------	
	
	Ravi Teja A V, 	 140050082	
	Aniket Murhekar, 140050084

Contents
--------

1.	server.cpp
2.	worker.cpp
3.	user.cpp
4. 	Makefile
5.	results.jpg
6.	data (directory)
	- script.py
	- plot.gnu
	- data6.dat
	- data7.dat
	- data8.dat
7.	data.txt
8.	README

Running Instructions
--------------------

1.	make
	Generates the executables server,worker,user.
2.	make clean
	Removes executables.
3.	To start server, run: ./server <server-port>
	To start worker, run: /worker server ip/host-name> <server-port>
	To start user, run: ./user <server ip/host-name> <server-port> <hash> <passwd-length> <binary-string>