all: server worker user 

server: 
	@echo "Compiling server.cpp..."
	@g++ server.cpp -o server
	@echo "Done"

worker: 
	@echo "Compiling worker.cpp..."
	@g++ worker.cpp -lcrypt -o worker
	@echo "Done"

user:	
	@echo "Compiling user.cpp..."
	@g++ -std=c++11 user.cpp -o user
	@echo "Done"

clean:	
	@echo "Cleaning up..."
	@rm -f server
	@rm -f worker
	@rm -f user
	@echo "Done"
