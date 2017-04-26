#include <iostream>
#include <string>
#include <unordered_map>
#include <set>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <json/json.h>
#include <json/json-forwards.h>

#define LISTEN_PORT "9034"   // port we're listening on

using namespace std;

string assign(string key , int value);
string assign(string key , string value);
string assign(string key , bool value);
Json::Value s2json(string s);

struct Chat_message{ // peer to peer messages
	int time_stamp;
	string receiver, sender;
	string data;
	Chat_message(){}
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

struct User_data{
	string password;
	int socket_id;
	vector<Chat_message> unread_list;
};
	
typedef unordered_map<string,User_data> t_user_map;
t_user_map user_map;
set<string>online_users;

struct Auth_message{ // all message types sent by server
	int time_stamp;
	string password;
	bool status;
	string receiver, sender;
	Auth_message(Json::Value root, int type){
		if(type == 2){ // LOGIN
			time_stamp = root["time_stamp"].asInt();
			password = root["password"].asString();
			sender = root["sender"].asString();
		}
		else if (type == 3){ // LOGOUT
			time_stamp = root["time_stamp"].asInt();
			sender = root["sender"].asString();
		}
	}
	string to_str(int type){
		if(type == 2){ // LOGIN
			string s = "{\n";
			s+= assign("type",type) + ",\n" +
				assign("time_stamp",time_stamp) + ",\n" +
				assign("status",status) + ",\n" +
				"\"unread_list\":[\n";
			if(status == 1){
				for (int i = 0; i < user_map[receiver].unread_list.size(); ++i){
					s+= "\t\"" + to_string(i) + "\":\"" + user_map[receiver].unread_list[i].to_str() + "\""; 
					if(i != user_map[receiver].unread_list.size() - 1)
						s+= ",";
					s+="\n";
				}
				
				s+= "\t],\n\"online_users\":[\n";
				int count_online = 0;
				for (auto it = online_users.begin(); it!=online_users.end(); ++it){
					count_online++;
					s+= "\t\"" + to_string(count_online) + "\":\"" + *it + "\"";
					if(online_users.size()!=count_online)
						s+= ",";
					s+="\n";		
				}
			}
			s+= "\t]\n}";		
			return s;
		}
		else if(type == 3){ //LOGOUT
			string s = "{\n";
			s+= assign("type",type) + ",\n" +
				assign("time_stamp",time_stamp) + ",\n" +
				assign("status",status) + "\n}";
			return s;	
		}
		else if(type == 4){ //UPDATE
			string s = "{\n";
			s+= assign("type",type) + ",\n" +
				assign("time_stamp",time_stamp) + ",\n" +
			s+= "\"online_users\":[\n";
			int count_online = 0;
			for (auto it = online_users.begin(); it!=online_users.end(); ++it){
				count_online++;
				s+= "\t\"" + to_string(count_online) + "\":\"" + *it + "\"";
				if(online_users.size()!=count_online)
					s+= ",";
				s+="\n";		
			}
			s+= "\t]\n}";
			return s;	
		}
	}
};

bool authenticate(string username,string password){
	for ( auto local_it = user_map.begin(); local_it!= user_map.end(); ++local_it ){
    	if(local_it->first == username and local_it->second.password == password)
    		return true;
	}
}	
Json::Value s2json(string s){
	Json::Reader reader;
	Json::Value root;
	bool parsingSuccessful = reader.parse( s.c_str(), root );     //parse process
    if (!parsingSuccessful){
        std::cout  << "Failed to parse\n" << reader.getFormattedErrorMessages();
    }
    return root;
}
string assign(string key , int value){
	return ("\"" + key + "\": " + to_string(value) ); 
}
string assign(string key , string value){
	return ("\"" + key + "\":\"" + value + "\""); 
}
string assign(string key , bool value){
	return ("\"" + key + "\": " + to_string(value) );  
}

int main(){
	
	/*******GET a listner socket and bind it*********/
		struct addrinfo  hints, *ai_list, *p;
		int listener_fd;     	// listening socket descriptor
    
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		int tmp1 = getaddrinfo(NULL, LISTEN_PORT, &hints, &ai_list);
		if (tmp1 != 0) {//ERROR
			fprintf(stderr, "selectserver: %s\n", gai_strerror(tmp1));
			exit(1);
		}
		
		int yes=1;        		// for setsockopt() SO_REUSEADDR, below  	
		for(p = ai_list; p != NULL; p = p->ai_next) { //iterate over obtined list of addresses
	    	listener_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (listener_fd < 0) 
				continue;
			
			setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); // If address already in use try to reuse it

			if (bind(listener_fd, p->ai_addr, p->ai_addrlen) < 0) {
				close(listener_fd); // close prev listener and try again
				continue;
			}
			break;
		}
		if (p == NULL) {// if we got here, it means we didn't get bound
			fprintf(stderr, "selectserver: failed to bind\n");
			exit(2);
		}

		freeaddrinfo(ai_list); // all done with this

	    // listen
	    if (listen(listener_fd, 10) == -1) {
	        perror("listen");
	        exit(3);
	    }	
	/*******GOT listener and listning NOW*********/   

	fd_set master_fds;    	// master file descriptor list
    fd_set read_fds;  		// read fds
    int fd_max = listener_fd;  // maximum file descriptor number

    FD_ZERO(&master_fds);   // clear the master and read sets
    FD_ZERO(&read_fds);

    FD_SET(listener_fd, &master_fds);
	
    while(true){// main loop 
        read_fds = master_fds; // copy it
        if (select(fd_max+1, &read_fds, NULL, NULL, NULL) == -1) {//Timeout not set so no check for 0	
            perror("select");
            exit(4);
        }

    	struct sockaddr client_address;		// client address
    	socklen_t addr_len = sizeof client_address;	// client address length
    	int new_fd;        							// newly accepted socket descriptor
    	char clientIP[INET_ADDRSTRLEN];

        for(int i = 0; i <= fd_max; i++){// run through the existing connections looking for data to read           
            if (FD_ISSET(i, &read_fds)){ // we got one!!
                
                if (i == listener_fd){ // new connection					
					new_fd = accept(listener_fd, &client_address, &addr_len);
					if (new_fd == -1)
                        perror("accept"); 
                    else{
                        FD_SET(new_fd, &master_fds); // add to master set
                        fd_max = max(new_fd,fd_max); // Keep track of maxfd
                        printf("selectserver: new connection from %s on socket %d\n",
							inet_ntop(client_address.sa_family, &((struct sockaddr_in*)&client_address)->sin_addr,clientIP, INET_ADDRSTRLEN),
							new_fd);
                    }
                } 
                else{ // read data from a previously connected client
                	int curr_fd = i;
                	char *buf = new char[256];    	// buffer for client data
					int nbytes = recv(curr_fd, buf, sizeof buf, 0); 
                    
                    if (nbytes < 0) // got error
                        perror("recv");
                    else if (nbytes == 0){//connection closed by client
                    	close(curr_fd);	// close the curr socket 
                    	FD_CLR(i, &master_fds); // remove from master set
                    	printf("selectserver: socket %d hung up\n", curr_fd);   
                    }
                    else{// we got some data from a client
                 		string data = buf;
                 		Json::Value  rec_msg = s2json(data);
                 		if(rec_msg["type"].asInt() == 1){ // Chat_message
                 			Chat_message msg(rec_msg);
                 			if(online_users.count(msg.receiver) == 0)// Receiver Offline
                 				user_map[msg.receiver].unread_list.push_back(msg);
                 			else{// Receiver Online                				
                 				if (send(user_map[msg.receiver].socket_id, buf, nbytes , 0) == -1) {
			                        perror("send");
			                    }
							}
                 		}
                 		else if(rec_msg["type"].asInt() == 2 or rec_msg["type"].asInt() == 3){ //LOGIN
                 			Auth_message msg(rec_msg,2);
                 			msg.status = 0;

                 			if(rec_msg["type"].asInt() == 2){
	                 			if(authenticate(msg.sender,msg.password)){
	                 				msg.status = 1;
	                 				online_users.insert(msg.sender);
	                 				user_map[msg.sender].socket_id = curr_fd;
	                 			}
							}
							else{
								if(online_users.count(msg.sender) == 0){
                 					msg.status = 1;
                 					online_users.erase(msg.sender);

                 					string msg_str = msg.to_str(4);
		                 			char *to_send = new char[msg_str.length() + 1];
									strcpy(to_send, msg_str.c_str());
                 			
                 					for (auto it = online_users.begin(); it != online_users.end(); ++it){
                 						if (send(user_map[*it].socket_id, to_send, msg_str.length() + 1, 0) == -1) {
		                        			perror("send");
		                    			}
                 					}
                 					delete [] to_send;		
                 				}
							}
                 			string msg_str = msg.to_str(rec_msg["type"].asInt());
                 			char *to_send = new char[msg_str.length() + 1];
							strcpy(to_send, msg_str.c_str());

                 			if (send(curr_fd, to_send, msg_str.length() + 1, 0) == -1) {
		                        perror("send");
		                    }
							delete [] to_send;
		                }
		                delete [] buf;
                    }   
                }
            }
        }
    } 
}

/*
	struct addrinfo {
	    int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
	    int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
	    int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
	    int              ai_protocol;  // use 0 for "any"
	    size_t           ai_addrlen;   // size of ai_addr in bytes
	    struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
	    char            *ai_canonname; // full canonical hostname

	    struct addrinfo *ai_next;      // linked list, next node
	};

	struct sockaddr_storage {
	    sa_family_t  ss_family;     // address family

	    // all this is padding, implementation specific, ignore it:
	    char      __ss_pad1[_SS_PAD1SIZE];
	    int64_t   __ss_align;
	    char      __ss_pad2[_SS_PAD2SIZE];
	};

	struct sockaddr {
	    unsigned short    sa_family;    // address family, AF_xxx
	    char              sa_data[14];  // 14 bytes of protocol address
	};

	struct sockaddr_in {
	    short int          sin_family;  // Address family, AF_INET
	    unsigned short int sin_port;    // Port number
	    struct in_addr     sin_addr;    // Internet address
	    unsigned char      sin_zero[8]; // Same size as struct sockaddr
	};

	struct in_addr {
	    uint32_t s_addr; // that's a 32-bit int (4 bytes)
	};
	 
	struct timeval {
	    int tv_sec;     // seconds
	    int tv_usec;    // microseconds
	}; 
*/