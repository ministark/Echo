#include "utils.h"
#include <fstream> 			// ouput to files for debug
#include <unordered_map> 	// storing all 
#include <time.h>
#define LISTEN_PORT "9034"  // Port on which server will listen for incomming connections
#define EOM "```"

//Stores password, socket ID, unread messages of user
struct User_data{
	string password;
	string last_seen;
	int socket_id;
	User_data(string password = "admin"){
		password = password;
		socket_id = -1;
	}
	void update_last_seen(){
		time_t rawtime;
		struct tm * timeinfo;
		time ( &rawtime );
	 	timeinfo = localtime ( &rawtime );
	  	last_seen = asctime (timeinfo);
	  	last_seen = last_seen.substr(11,8);
	}
	vector<Chat_message> unread_list;
	vector<Group_formation_message> g_unread_list;
};

//Store all users in a hash map	
typedef unordered_map<string,User_data> t_user_map;
t_user_map user_map;

//Store all groups in a hash map
typedef unordered_map<string,set<string> > t_group_map;
t_group_map group_map;

//Stores set of all online users
set<string> online_users;

//Message data of all useful messages sent and received by server
struct Auth_message{ 
	string time_stamp;
	string password;
	int status;
	string receiver, sender;
	Auth_message(Json::Value root, int type){
		if(type == 2){ // LOGIN
			time_stamp = root["time_stamp"].asString();
			password = root["password"].asString();
			sender = root["sender"].asString();
			receiver = sender;
		}
		else if (type == 3){ // LOGOUT
			time_stamp = root["time_stamp"].asString();
			sender = root["sender"].asString();
			receiver = sender;
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
					s+= "\t" + user_map[receiver].unread_list[i].to_str(); 
					if(i != user_map[receiver].unread_list.size() - 1)
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
				assign("time_stamp",time_stamp) + ",\n";
			s+= "\"online_users\":[\n";
			int count_online = 0;
			for (auto it = online_users.begin(); it!=online_users.end(); ++it){
				count_online++;
				s+= "\t\""+ *it + "\"";
				if(online_users.size()!=count_online)
					s+= ",";
				s+="\n";		
			}
			s+= "\t],\n\"offline_users\":[\n";
			int count_offline = 0;
				for (auto it = user_map.begin(); it!=user_map.end(); ++it){
					if(online_users.count(it->first)==0){
						count_offline++;
						s+= "\t{" + assign("user",it->first) + ",\n" +
							assign("last_seen",it->second.last_seen) + "\n}";
						if(count_offline != user_map.size() - online_users.size())
							s+= ",\n";
						s+= "\n";
					}		
				}
				s+="\t]\n}";
				return s;	
			}
		}
	};

//Authenticate the user
bool authenticate(string username,string password){
	for ( auto local_it = user_map.begin(); local_it!= user_map.end(); ++local_it ){
		cout << local_it->first << " ::: " << local_it->second.password << endl;
		if(local_it->first == username and local_it->second.password == password){
			cout << local_it->first << " ::: " << local_it->second.password << endl;
			return true;
		}
	}
	return false;
}

int main(){
/*********Load set of valid users in the files***********/
	User_data u[100];
	int k =0;
	ifstream users_file;
	users_file.open("data/user_list.txt");
	while (!users_file.eof()){
		string name;
		users_file >> name;
		if(name!= ""){
			cout<<name<<endl;
			user_map[name] = u[k++];
		}
	}
	users_file.close();
/*********Data loaded************/
	
/*********Get Listener for SERVER PORT***********/
	int error_code = 0;
	int listener_fd = get_listner(LISTEN_PORT,error_code);
	if(listener_fd == -1)
		exit(error_code);
/*********listener socket created***********/

/*********Create fdset for selece server functionality***********/
	fd_set master_fds;			// master  file descriptor list
	fd_set read_fds;			// to read file descriptor list
	int fd_max = listener_fd;  	// maximum file descriptor number
	
	FD_ZERO(&master_fds);   // clear the master and read sets
	FD_ZERO(&read_fds);

	FD_SET(listener_fd, &master_fds); //add listener to master set

	while(true){// main loop 
		read_fds = master_fds; // copy master in read to check for read data in these connections
		
		if (select(fd_max+1, &read_fds, NULL, NULL, NULL) == -1) {//Timeout not set BLOCKING CALL 
			perror("select");
			exit(4);
		}
		for(int i = 0; i <= fd_max; i++){	// run through the existing connections looking for data to read 
			if (FD_ISSET(i, &read_fds)){ 	// we got one!!
				if (i == listener_fd){ 		// new connection					
					
					struct sockaddr client_address;				// client address
					socklen_t addr_len = sizeof client_address;	// client address length
					char clientIP[INET_ADDRSTRLEN];				// char* to staore IPstring of client
					//Accept new connection and spawn new socket for this connection
					int new_fd = accept(listener_fd, &client_address, &addr_len); 
					if (new_fd == -1) 		// Error in accept
						perror("accept"); 
					else{
						FD_SET(new_fd, &master_fds); 			// add newfd to master set
						fd_max = max(new_fd,fd_max); 			// Keep track of maxfd
						inet_ntop(client_address.sa_family, &((struct sockaddr_in*)&client_address)->sin_addr,clientIP, INET_ADDRSTRLEN);
						printf("selectserver: new connection from %s on socket %d\n", clientIP, new_fd);
					}
				} 
				else{ 						// read data from a previously connected client
					int curr_fd = i;
					int nbytes = 0;
					string data = read_full(curr_fd,nbytes); //read till end of data
				   	cout << data << endl; 	// INPORTANT DEBUG STATEMENT
 				   	
				   	if(nbytes == 0){ 		// Client wants to close the connection
				   		for(auto it = user_map.begin(); it!= user_map.end(); ++it){ //Make the use offline
				   			if(it->second.socket_id == curr_fd)
				   				online_users.erase(it->first);
				   		}
				   		close(curr_fd);			// close the curr socket 
						FD_CLR(i, &master_fds); // remove from master set
						printf("selectserver: socket %d hung up\n", curr_fd);
				   	}
				   	else if(nbytes < 0) 		// Error in receiveing
				   		perror("recv");
					else{
				 		Json::Value  rec_msg = s2json(data);  			//Parse json received
				 		
				 		if(rec_msg["type"].asInt() == 1){ 				// Chat_message
				 			Chat_message msg(rec_msg);
				 			cout<<"Received message \n"<<msg.data<<endl;
				 			
				 			if(online_users.count(msg.receiver) == 0)	// Receiver Offline
				 				user_map[msg.receiver].unread_list.push_back(msg);
				 			else{										// Receiver Online //SEND msg to receiver 
				 				data = data + EOM;
				 				char *to_send = new char[data.length() + 1];
								strcpy(to_send, data.c_str());								
				 				if (send(user_map[msg.receiver].socket_id, to_send, data.length() + 1, 0) == -1) {
									perror("send");
								}
								delete [] to_send;
							}
				 		}
				 		else if(rec_msg["type"].asInt() == 2 or rec_msg["type"].asInt() == 3){ // Server messages
				 			Auth_message msg(rec_msg,2);
				 			msg.status = 0;

				 			if(rec_msg["type"].asInt() == 2){	//Login Request Message
				 				cout << "SS :" << msg.sender << " PP :" << msg.password<<endl;
					 			if(authenticate(msg.sender,msg.password)){ // authenticate USER
					 				msg.status = 1;
					 				online_users.insert(msg.sender);
					 				user_map[msg.sender].socket_id = curr_fd;
					 			}
					 			else{
					 				auto it = user_map.begin();
					 				for (it = user_map.begin(); it != user_map.end(); ++it){
					 					if(it->first == msg.sender)
					 						break;
					 				}
					 				if(it == user_map.end()){ // Never break => never a match so register
					 					msg.status = 2;
					 					user_map[msg.sender] = User_data();
					 					user_map[msg.sender].password = msg.password;
					 					online_users.insert(msg.sender);
					 					user_map[msg.sender].socket_id = curr_fd;
					 				}
					 				else //Password wrong
					 					msg.status = 0;
					 			}
							}
							else{								//Logout Request Message
								if(online_users.count(msg.sender) != 0){ // If not already online
				 					msg.status = 1;
				 					user_map[msg.sender].update_last_seen();
				 					online_users.erase(msg.sender);	
				 				}
							}

							//Send response to sender and his/her unread list
				 			string msg_str = msg.to_str(rec_msg["type"].asInt()) + EOM;
				 			char *to_send1 = new char[msg_str.length() + 1];
							strcpy(to_send1, msg_str.c_str());
							printf("%s\n",to_send1);
				 			if (send(curr_fd, to_send1, msg_str.length() + 1, 0) == -1) {
								perror("send");
							}
							if(rec_msg["type"].asInt() == 2)
								user_map[msg.sender].unread_list.clear();
							delete [] to_send1;

							//Send online user list to all users for updating
							msg_str = msg.to_str(4) + EOM;
		 					cout<<msg_str<<endl;
				 			char *to_send = new char[msg_str.length() + 1];
							strcpy(to_send, msg_str.c_str());
		 			
		 					for (auto it = online_users.begin(); it != online_users.end(); ++it){
		 						if (send(user_map[*it].socket_id, to_send, msg_str.length() + 1, 0) == -1) {
									perror("send");
								}
		 					}
		 					delete [] to_send;	
						}
						else if(rec_msg["type"].asInt() == 5){ 			// Group chat message
							Group_formation_message msg(rec_msg);				 			
				 			for (auto it = group_map[msg.group_name].begin(); it!=group_map[msg.group_name].end(); ++it){
				 				if(online_users.count(*it) == 0)	// Receiver of group offline ADD message to his/her queue
				 					user_map[*it].g_unread_list.push_back(msg);
					 			else{								// Receiver of group online SEND immediately
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
