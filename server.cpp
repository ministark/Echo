#include "utils.h"

#include <unordered_map>

#define LISTEN_PORT "9034"   // port we're listening on

struct User_data{
	string password;
	int socket_id;
	User_data(){
		password = "admin";
		socket_id = -1;
	}
	vector<Chat_message> unread_list;
	vector<Group_formation_message> g_unread_list;
};
	
typedef unordered_map<string,User_data> t_user_map;
t_user_map user_map;
typedef unordered_map<string,set<string> > t_group_map;
t_group_map group_map;

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

int main(){
	User_data u1,u2,u3,u4;
	user_map["Rishabh"] = u1;
	user_map["Sourabh"] = u2;
	user_map["Harsh"] = u3;
	user_map["Bharat"] = u4;

	int error_code = 0;
	int listener_fd = get_listner(LISTEN_PORT,error_code);
	if(listener_fd == -1)
		exit(error_code);
	
	fd_set master_fds;	// master file descriptor list
	fd_set read_fds;		// read fds
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
		for(int i = 0; i <= fd_max; i++){// run through the existing connections looking for data to read 
			if (FD_ISSET(i, &read_fds)){ // we got one!!
				if (i == listener_fd){ // new connection					
					
					struct sockaddr client_address;		// client address
					socklen_t addr_len = sizeof client_address;	// client address length
					char clientIP[INET_ADDRSTRLEN];
					int new_fd = accept(listener_fd, &client_address, &addr_len); // New socket descriptor
					if (new_fd == -1)
						perror("accept"); 
					else{
						FD_SET(new_fd, &master_fds); // add to master set
						fd_max = max(new_fd,fd_max); // Keep track of maxfd
						inet_ntop(client_address.sa_family, &((struct sockaddr_in*)&client_address)->sin_addr,clientIP, INET_ADDRSTRLEN);
						printf("selectserver: new connection from %s on socket %d\n", clientIP, new_fd);
					}
				} 
				else{ // read data from a previously connected client
					int curr_fd = i;
					int nbytes = 0;
				   	string data = read_full(curr_fd,nbytes);
				   	if(nbytes == 0){
				   		close(curr_fd);	// close the curr socket 
						FD_CLR(i, &master_fds); // remove from master set
						printf("selectserver: socket %d hung up\n", curr_fd);
				   	}
				   	else if(nbytes < 0)
				   		perror("recv");
					else{
				 		Json::Value  rec_msg = s2json(data);
				 		if(rec_msg["type"].asInt() == 1){ // Chat_message
				 			Chat_message msg(rec_msg);
				 			if(online_users.count(msg.receiver) == 0)// Receiver Offline
				 				user_map[msg.receiver].unread_list.push_back(msg);
				 			else{// Receiver Online
				 				char *to_send = new char[data.length() + 1];
								strcpy(to_send, data.c_str());								
				 				if (send(user_map[msg.receiver].socket_id, to_send, data.length() + 1, 0) == -1) {
									perror("send");
								}
								delete [] to_send;
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
				 					msg_str+= "```";
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
				 			msg_str+= "```";
				 			char *to_send = new char[msg_str.length() + 1];
							strcpy(to_send, msg_str.c_str());
							printf("%s\n",to_send);
				 			if (send(curr_fd, to_send, msg_str.length() + 1, 0) == -1) {
								perror("send");
							}
							delete [] to_send;
						}
						else if(rec_msg["type"].asInt() == 5){
							Group_formation_message msg(rec_msg);
				 			for (auto it = group_map[msg.group_name].begin(); it!=group_map[msg.group_name].end(); ++it){
				 				if(online_users.count(*it) == 0)// Receiver Offline
				 					user_map[*it].g_unread_list.push_back(msg);
					 			else{// Receiver Online
					 				char *to_send = new char[data.length() + 1];
									strcpy(to_send, data.c_str());								
					 				if (send(user_map[*it].socket_id, to_send, data.length() + 1, 0) == -1) {
										perror("send");
									}
									delete [] to_send;
								}
							}
						}
					}   
				}
			}
		}
	} 
}
