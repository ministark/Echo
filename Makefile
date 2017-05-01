echo: server client 
	
server: server.cpp utils.cpp utils.h 
	g++ -std=c++11 -I ./jsoncpp/dist/ server.cpp utils.cpp ./jsoncpp/dist/jsoncpp.cpp -o server

client: client.cpp utils.cpp utils.h 
	g++ -std=c++11 -I ./jsoncpp/dist/ client.cpp  utils.cpp ./jsoncpp/dist/jsoncpp.cpp -lcurses -pthread -o echo

.PHONY: clean
clean:
	rm -f *.txt echo server
