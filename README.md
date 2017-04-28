# Echo

## Server-Client Implementation
Server Port **9034**

###### Make executable for server-client
- `g++ -std=c++14 -I ./jsoncpp/dist/ client.cpp  utils.cpp ./jsoncpp/dist/jsoncpp.cpp -lcurses -pthread -o client.out `
- `g++ -std=c++11 -I ./jsoncpp/dist/ server.cpp  utils.cpp ./jsoncpp/dist/jsoncpp.cpp -o server.out`

###### Run server
- `./server.out`
- `./client.out`
