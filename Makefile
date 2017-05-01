echo: server.o client.o 
	
server.o: server.cpp utils.cpp utils.h 
	g++ -std=c++11 -I ./jsoncpp/dist/ server.cpp utils.cpp ./jsoncpp/dist/jsoncpp.cpp -o server.o

client.o: client.cpp utils.cpp utils.h 
	g++ -std=c++11 -I ./jsoncpp/dist/ client.cpp  utils.cpp ./jsoncpp/dist/jsoncpp.cpp -lcurses -pthread -o client.o

.PHONY: clean
clean:
	rm -f *.txt *.o