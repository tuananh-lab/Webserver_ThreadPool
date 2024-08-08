all:
	g++ -std=c++17 -pthread -o server server.cpp
clean:
	rm server