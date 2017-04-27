#include "utils.h"

#define SERVER_PORT "9034"  // port we're listening on
#define MAXDATASIZE 256 	// max number of bytes we can get at once 
#define LISTEN_PORT "9033"	//local port for thread intercommunication

struct Chat_message{ // peer to peer messages
	int time_stamp;
	string receiver, sender;
	string data;
	Chat_message(){}
	Chat_message(string username, int time, string rec){
		time_stamp = time;
		sender = username;
		receiver = rec;
	}
	Chat_message(Json::Value root){
		time_stamp = root["time_stamp"].asInt();
		receiver = root["receiver"].asString();
		sender = root["sender"].asString();
		data = root["data"].asString();
	}
	string to_str(){
		string s = "{\n";
		s+= assign("type",1) + ",\n" +
			assign("time_stamp",time_stamp) + ",\n" +
			assign("receiver",receiver) + ",\n" +
			assign("sender",sender) + ",\n" +
			assign("data",data) + "\n}";
		return s;
	}
};
	
struct Auth_message{ // all message types sent by server
	int time_stamp;
	bool status;
	string sender, password;
	Auth_message(string username, int time, string passwd = "NULL"){
		time_stamp = time;
		sender = username;
		password = passwd;
	}
	Auth_message(Json::Value root){
		time_stamp = root["time_stamp"].asInt();
		status = root["status"].asBool();
	}
	string to_str(int type){
		string s = "{\n";
		s += assign("type",type) + ",\n" +
			assign("time_stamp",time_stamp) + ",\n" +
			assign("sender",sender);
			
		if(type == 2) // LOGIN
			s+=  ",\n" + assign("password",password);		

		s+= "\n}";
		return s;
	}
};
	
int main(int argc, char *argv[]){
	/*******Input check*********/
		if (argc != 2) {
		    fprintf(stderr,"usage: hostname \n");
		    exit(1);
		}
	/*******Input valid*********/

	/*******Connect to server*********/
		struct addrinfo hints, *servinfo, *p;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		
		int tmp = getaddrinfo(argv[1], SERVER_PORT, &hints, &servinfo);
		if (tmp != 0){
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(tmp));
			return 1;
		}
		
		int my_sock_fd;  
		for(p = servinfo; p != NULL; p = p->ai_next){ // loop through all the results and connect to the first we can
			if ((my_sock_fd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
				perror("client: socket");
				continue;
			}

			if (connect(my_sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
				perror("client: connect");
				close(my_sock_fd);
				continue;
			}

			break;
		}
		if (p == NULL) {
			fprintf(stderr, "client: failed to connect\n");
			return 2;
		}	
		freeaddrinfo(servinfo); // all done with this structure
	/*******Connected*********/

	/*******Connect to other thread*********/
		char serverIP[INET6_ADDRSTRLEN];
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), serverIP, sizeof serverIP);
		printf("client: connecting to %s\n", serverIP);

		int error_code = 0;
		int listener_fd = get_listner(LISTEN_PORT,error_code);
		if(listener_fd == -1)
			exit(error_code);	

		fd_set master_fds;    	// master file descriptor list
	    fd_set read_fds;  		// read fds
	    int fd_max = my_sock_fd;

	    FD_ZERO(&master_fds);   // clear the master and read sets
	    FD_ZERO(&read_fds);

	    FD_SET(my_sock_fd, &master_fds);

		struct sockaddr self_address;		// client address
		socklen_t addr_len = sizeof self_address;	// client address length
		int self_sock_id;        							// newly accepted socket descriptor
		char selfIP[INET_ADDRSTRLEN];

		self_sock_id = accept(listener_fd, &self_address, &addr_len);
		if (self_sock_id == -1){
			cout << "XXX" << endl;
		    perror("accept"); 
		}
		else{
		    FD_SET(self_sock_id, &master_fds); // add to master set
		    fd_max = max(self_sock_id,fd_max); // Keep track of maxfd
		    inet_ntop(self_address.sa_family, &((struct sockaddr_in*)&self_address)->sin_addr,selfIP, INET_ADDRSTRLEN);
		    printf("selectserver: new connection from %s on socket %d\n", selfIP, self_sock_id);
			close(listener_fd);
		}
	/*******Connected*********/
	
    while(true){// main loop 
        read_fds = master_fds; // copy it
        if (select(fd_max+1, &read_fds, NULL, NULL, NULL) == -1) {//Timeout not set so no check for 0	
            perror("select");
            exit(4);
        }
        if (FD_ISSET(self_sock_id, &read_fds)){ // Data from other thread
        	int nbytes = 0;
           	string data = read_full(self_sock_id,nbytes);
           	if(nbytes == 0){
           		close(self_sock_id);	// close the curr socket 
		    	FD_CLR(self_sock_id, &master_fds); // remove from master set
		    	printf("selectserver: socket %d hung up\n", self_sock_id);
           	}
           	else if(nbytes < 0)
           		perror("recv");
            else{
         		char *to_send = new char[data.length() + 1];
				strcpy(to_send, data.c_str());                				
 				if (send(my_sock_fd, to_send, data.length() + 1, 0) == -1) {
                    perror("send");
                }
                delete [] to_send;
         	}
        }
        else if(FD_ISSET(my_sock_fd, &read_fds)){ // Data from server
        	int nbytes = 0;
           	string data = read_full(listener_fd,nbytes);
           	if(nbytes == 0){
           		close(my_sock_fd);	// close the curr socket 
		    	FD_CLR(my_sock_fd, &master_fds); // remove from master set
		    	printf("selectserver: socket %d hung up\n", my_sock_fd);
           	}
           	else if(nbytes < 0)
           		perror("recv");
            else{ //Process received data	
         		Json::Value rec_msg = s2json(data);
         		if(rec_msg["type"].asInt() == 1){ // Chat_message
         			Chat_message msg(rec_msg);
         		}
	         	else if(rec_msg["type"].asInt() == 2 or rec_msg["type"].asInt() == 3 or rec_msg["type"].asInt() == 4){
	         		Auth_message msg(rec_msg);
	         	} 	
         	}
        }
    }
}