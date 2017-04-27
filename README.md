# Echo

## Server-Client Implementation
default PORT **9034**

###### Make executable for server and client
- `g++ -std=c++11 -I ./jsoncpp/dist/ server.cpp  ./jsoncpp/dist/jsoncpp.cpp -o server.out`
- `g++ -std=c++11 -I ./jsoncpp/dist/ remote.cpp  ./jsoncpp/dist/jsoncpp.cpp -o remote.out`
- `g++ -std=c++14 client.cpp -lcurses -pthread`

###### Run server
- `./server.out`
- `./client.out serverIP`
