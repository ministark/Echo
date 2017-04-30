#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include "utils.h"
using namespace std;

#define ENTER_KEY 13
#define LOWER_KEY 31
#define UPPER_KEY 127
#define SERVER_PORT "9034"  // port we're listening on
#define MAXDATASIZE 256 	// max number of bytes we can get at once 

#define EOM "```"

mutex mtx_user_list,mtx_display,mtx_comm,mtx_file;
ofstream debug("debug.txt");
	
struct Auth_message{ // all message types sent by server
	int time_stamp;
	bool status;
	string sender, password;
	Auth_message(string username, string passwd = "NULL",int time=1234){
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

struct User{
	string name,status;
	User(){
		name = "default";
		status = "offline";
	}
	User(string name,string status){
		this->name = name;
		this->status = status;
	}
};

struct UI{
	int type,cursor_x,cursor_y,scroll,maxscroll ;
	bool update,exit_program;
	bool auth_ack_received;
	string LISTEN_PORT;
	bool logged_in;
	vector <User> user_list;
	map<string,int> user_map;
	int number_online,unread_messages;
	int my_sock_fd,listener_fd;
	User recipient; //The person whom we are talking with
	string uname;
	char display[24][80];
	UI(){
		type = 0;
		auth_ack_received = false;
		update = true;
		logged_in = false;
		exit_program = false;
		number_online = 1;
		unread_messages = 0;
		scroll = 0;
		maxscroll = 0;
		load_ui();
	}
	void load_ui(){
		int i = 0;
		string template_name,template_line;
		ifstream template_file;
		scroll = 0;
		if (type == 0){
			template_name = "ui/ui_login.txt";
			cursor_x = 13;
			cursor_y = 39;
		}
		else if (type == 1){
			template_name = "ui/ui_home.txt";
			cursor_x = 23;
			cursor_y = 3;
		}
		else if (type == 2){
			template_name = "ui/ui_chat.txt";
			cursor_x = 23;
			cursor_y = 3;
		}
		template_file.open(template_name);
		while (getline(template_file,template_line)){
			for (int j=0; j<80; j++)
				display[i][j] = template_line[j];
			i++;
		}
		template_file.close();
	}
	void edit_display(int x,int y,string s)
	{
		for (int i=0; i<s.length(); i++)
			display[x][i+y] = s[i];
	}
};

void RefreshDisplay(char display[][80]){
	for (int i=0; i<24; i++){
		for (int j=0; j<80; j++){
			mvprintw(i,j,"%c",display[i][j]);
			refresh();
		}
	}
}

void *Display(void *thread_arg)
{
	this_thread::sleep_for(chrono::milliseconds(1000));
	UI *ui = (UI *)thread_arg;
	while (!ui->exit_program)
	{
		//mtx_display.lock();
		if (ui->update)
		{
			if (ui->type == 0) // Login
			{
                
			}
			else if (ui->type == 1) // Home 
			{
				ui->edit_display(2,3,"                ");
				ui->edit_display(2,3,ui->uname);
				ifstream online_file("./data/online_list.txt"),offline_file("./data/offline_list.txt");
				string name,status;vector <string> name_list,status_list;
				int ls = ui->user_list.size();
				while (!online_file.eof())
				{
					online_file >> name;
					name_list.push_back(name);
					status_list.push_back("online");
					if (ui->user_map.find(name) == ui->user_map.end())
					{
						ui->user_map[name] = ls++;
						ui->user_list.push_back(User(name,status));
					}
					else
					{
						int j = ui->user_map[name];
						ui->user_list[j].status = status;
					}
				}
				while (!offline_file.eof())
				{
					offline_file >> name;
					name_list.push_back(name);
					status_list.push_back("offline");
					if (ui->user_map.find(name) == ui->user_map.end())
					{
						ui->user_map[name] = ls++;
						ui->user_list.push_back(User(name,status));
					}
					else
					{
						int j = ui->user_map[name];
						ui->user_list[j].status = status;
					}
				}
				online_file.close();offline_file.close();
				int line_x = 8,list_size = name_list.size();
				ui->maxscroll = max(0,list_size - 11);
				for (int j=8; j<20; j++)
					ui->edit_display(j,25,"                                            ");
				for (int j=ui->scroll; j<min(int(list_size),ui->scroll+12); j++)
				{
					name = name_list[j];status = status_list[j];
					ui->edit_display(line_x,25,name);
					ui->edit_display(line_x,50,status);
					line_x++;
				}
			}
			else if (ui->type == 2) // Chat Window
			{
				ui->edit_display(2,3,"                ");
				ui->edit_display(2,3,ui->recipient.name);
				ofstream debug_chat("debug_chat.txt");
				for (int j=7; j<22; j++)
					ui->edit_display(j,1,"                                                                              ");
				string line;
				vector <string> line_list;
				ifstream myfile("./chat/"+ui->recipient.name+".echo");
				int start;
				while (getline(myfile,line))
					line_list.push_back(line);
				int list_len = line_list.size(),lk = 21, str_line = 0;
				ui->maxscroll = max(0,list_len-11);
				for (int j = list_len - 1 - ui->scroll; j >= 0; j--)
				{
					if(lk-((line_list[j].length()-1)/50)+1  < 7) 
					{
						str_line = j+1;
						break;
					}
					lk -= ((line_list[j].length()-1)/50)+1;
				}
				int ck = 7;
				for(int j = str_line; j < list_len; j++)
				{
					if(line_list[j][0] == '>')
						start = 28;
					else
						start = 3;
					int line_len = line_list[j].length();
					line = line_list[j].substr(1);
					debug_chat << line << endl;
					if (line_len <= 50 and start == 28)
					{
						ui->edit_display(ck,79-line_len,line);
						ck++;
					}
					else
					{
						while (line_len > 0 and ck < 22)
						{
							ui->edit_display(ck,start,line.substr(0,min(50,line_len)));
							debug_chat << line.substr(0,min(50,line_len)) << endl;
							line_len -= min(50,line_len);
							if (line_len > 0)
								line = line.substr(50);
							ck++;
						}
					}
					if (ck >= 22)
						break;
				}
				myfile.close();
			}
			RefreshDisplay(ui->display);
			ui->update = false;
		}
	}
	pthread_exit(NULL);
}

bool authenticate(UI * ui,string username,string password)
{
	Auth_message msg(username,password);
	string data = msg.to_str(2) + EOM;
	char *to_send = new char[data.length() + 1];
	strcpy(to_send, data.c_str());                				
	if (send(ui->my_sock_fd, to_send, data.length() + 1, 0) == -1) 
	{
        perror("send");
        exit(1);
    }
    delete [] to_send;
    while(!ui->auth_ack_received);
    return ui->logged_in;
}	

void ProcessInput(UI *ui,int self_client_sock_fd)
{
	string command;
	for (int i=3; i<80; i++)
		command = command + ui->display[23][i];
	int l = 0,r = 76;
	while (l < 77 and command[l] == ' ')
		l++;
	while (r >= 0 and command[r] == ' ')
		r--;
	if (l > r)
		return;
	command = command.substr(l,r-l+1);
	ofstream debug_input("debug_input.txt");
	if (command[0] == ':')
	{		
		command = command.substr(1);
		debug_input << "command" << endl;
		debug_input << command << endl;
		if(command.substr(0,5) == "chat ") {
			command = command.substr(5);
			debug_input << command << endl;
			auto it = ui->user_map.find(command);
			if (it == ui->user_map.end()) 
				return;
			ui->recipient = ui->user_list[it->second];
			ui->type = 2;
			ui->load_ui();
			ui->update = true;
			debug_input << "ui to be updated" << endl;
		}
		else if (command == "quit") {
			debug_input <<"program should quit" << endl;
			ui->exit_program = true;
		}
		else if (command == "home") {
			debug_input << "program should go to home" << endl;
			ui->type = 1;
			ui->load_ui();
			ui->update = true;
		}
	}
	else if (ui -> type == 2) // Chat Window
	{	debug<<"uname : "<<ui->uname<<endl;
		Chat_message msg(ui->recipient.name,ui->uname,command);
		string data = msg.to_str() + EOM;
		char *to_send = new char[data.length() + 1];
		strcpy(to_send, data.c_str());                				
		if (send(ui->my_sock_fd, to_send, data.length() + 1, 0) == -1) 
		{
            perror("send");
            exit(1);
        }
        
    	ofstream myfile("./chat/"+ui->recipient.name+".echo", fstream::out | fstream::app);
        myfile << ">" << msg.data<<"\n";
        myfile.close();

        debug<<"MSG writtend"<<endl;
        delete []to_send;

	}
	return;
}

void *InputHandler(void *thread_arg)
{

	UI *ui = (UI *)thread_arg;
	this_thread::sleep_for(chrono::milliseconds(50));
	/*******Connecting to actual server*********/
			//ofstream output_file("client_comm_log.txt");
			struct addrinfo hints1, *servinfo1, *p1;
			memset(&hints1, 0, sizeof hints1);
			hints1.ai_family = AF_INET;
			hints1.ai_socktype = SOCK_STREAM;
			
			int tmp = getaddrinfo("localhost", ui->LISTEN_PORT.c_str(), &hints1, &servinfo1);
			if (tmp != 0){
				//output_file << "get addrinfo: " << gai_strerror(tmp) << endl;
				return NULL;
			}
			
			int self_client_sock_fd;  
			for(p1 = servinfo1; p1 != NULL; p1 = p1->ai_next){ // loop through all the results and connect to the first we can
				if ((self_client_sock_fd = socket(p1->ai_family, p1->ai_socktype,p1->ai_protocol)) == -1) {
					//output_file << "client : socket " << LISTEN_PORT << endl;
					// perror("client: socket");
					continue;
				}

				if (connect(self_client_sock_fd, p1->ai_addr, p1->ai_addrlen) == -1) {
					//output_file << "client: connect" << LISTEN_PORT << endl;
					// perror("client: connect");
					close(self_client_sock_fd);
					continue;
				}
				break;
			}
			if (p1 == NULL) {
				//output_file << "client: failed to connect " << endl;
				return NULL;
			}	
			freeaddrinfo(servinfo1);
			// char serverIP[INET6_ADDRSTRLEN];
			// inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), serverIP, sizeof serverIP);
			// printf("client: connecting to %s\n", serverIP);
	/*******Connected*********/
	
	bool command_start = false,username_read,password_read;
	int ch;
	string username,password;
	while (!ui->exit_program)
	{
		//mtx_display.lock();
		ch = getch();
		
		//Login Page
		if (ui->type == 0) 
		{
			username_read = password_read = false;
			if (ui->cursor_x == 13)
				username_read = true;
			else if (ui->cursor_x == 15)
				password_read = true;
			if (ch != ENTER_KEY)
			{
				if (ch == KEY_BACKSPACE and ui->cursor_y > 39)
				{
					ui->cursor_y--;
					ui->display[ui->cursor_x][ui->cursor_y] = ' ';
					if (username_read)
						username.pop_back();
					else if (password_read)
						password.pop_back();
				}
				else if (ch > LOWER_KEY and ch < UPPER_KEY and ui->cursor_y < 55)
				{
					if (username_read)
					{
						ui->display[ui->cursor_x][ui->cursor_y++] = ch;
						username += ch;
					}
					else if (password_read)
					{
						ui->display[ui->cursor_x][ui->cursor_y++] = '*';
						password += ch;
					}
				}
				ui->update = true;
			}
			else
			{
				if (username_read)
				{
					ui->cursor_x = 15;
					ui->cursor_y = 39;
				}
				else if (password_read)
				{
					if (authenticate(ui,username,password))
					{
						ui->type = 1;
						ui->uname = username;
						ui->load_ui();
						ui->update = true;
					}
					else
					{
						username = "";
						password = "";
						ui->cursor_x = 13;
						ui->cursor_y = 39;
						ui->edit_display(13,39,"                ");
						ui->edit_display(15,39,"                ");
						ui->update = true;
 					}
				}
			}

		}
		//Home Screen & Chat Window
		else
		{
			if (ch != ENTER_KEY)
			{	
				if (ch == KEY_BACKSPACE and ui->cursor_y > 3)
				{
					ui->display[ui->cursor_x][--ui->cursor_y] = ' ';
					ui->update = true;
				}
				else if (ch > LOWER_KEY and ch < UPPER_KEY and ui->cursor_y < 78)
				{
					ui->display[ui->cursor_x][ui->cursor_y++] = ch;
					ui->update = true;
				}	
				else if (ch == KEY_DOWN)
				{
					if (ui->type == 1 and ui->scroll < ui->maxscroll)
					{
						ui->scroll++;
						ui->update = true;
					}
					else if (ui->type == 2 and ui-> scroll > 0)
					{
						ui->scroll--;
						ui->update = true;
					}
				}
				else if (ch == KEY_UP)
				{
					if (ui->type == 2 and ui->scroll<ui->maxscroll)
					{	
						ui->scroll++;
						ui->update = true;
					}
					else if (ui->type == 1 and ui -> scroll > 0)
					{
						ui->scroll--;
						ui->update = true;
					}
				}
			}
			else
			{
				ProcessInput(ui,self_client_sock_fd);
				for (int i=0; i<80; i++)
					ui->display[ui->cursor_x][i] = ' ';
				ui->edit_display(ui->cursor_x,0,"->");
				ui->update = true;
				ui->cursor_y = 3;
			}
		}
		//mtx_display.unlock();
		this_thread::sleep_for(chrono::milliseconds(50));
	}
	pthread_exit(NULL);
}
void *CommunicationHandler(void *thread_arg)
{
	UI *ui = (UI *)thread_arg;
	int self_sock_id, my_sock_fd = ui->my_sock_fd,listener_fd = ui->listener_fd;  // newly accepted socket descriptor
	
	fd_set master_fds;    	// master file descriptor list
    fd_set read_fds;  		// read fds
    int fd_max = my_sock_fd;

    FD_ZERO(&master_fds);   // clear the master and read sets
    FD_ZERO(&read_fds);

    FD_SET(my_sock_fd, &master_fds);

	struct sockaddr self_address;		// client address
	socklen_t addr_len = sizeof self_address;	// client address length
	char selfIP[INET_ADDRSTRLEN];
	
	self_sock_id = accept(listener_fd, &self_address, &addr_len);

	if (self_sock_id == -1){
	    perror("accept"); 
	}
	else{
	    FD_SET(self_sock_id, &master_fds); // add to master set
	    fd_max = max(self_sock_id,fd_max); // Keep track of maxfd
	    // inet_ntop(self_address.sa_family, &((struct sockaddr_in*)&self_address)->sin_addr,selfIP, INET_ADDRSTRLEN);
		//close(listener_fd);
	}

    while(!ui->exit_program){// main loop 
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
		    	// printf("selectserver: socket %d hung up\n", self_sock_id);
           	}
           	else if(nbytes < 0)
       		{
           		// perror("recv");
       		}
            else{
            	debug<<"Sending data to server "<<data<<endl;
         		char *to_send = new char[data.length() + 1];
				strcpy(to_send, data.c_str());                				
 				if (send(my_sock_fd, to_send, data.length() + 1, 0) == -1) {
                    // perror("send");
                }
    			delete [] to_send;

         	}
        }
        else if(FD_ISSET(my_sock_fd, &read_fds)){ // Data from server
        	    	debug<<"receiving data from server "<<endl;
        	int nbytes = 0;
           	string data = read_full(my_sock_fd,nbytes);\
           	debug << "YOYO : "<< data << endl;
           	if(nbytes == 0){
           		close(my_sock_fd);	// close the curr socket 
		    	FD_CLR(my_sock_fd, &master_fds); // remove from master set
		    	// printf("selectserver: socket %d hung up\n", my_sock_fd);
           	}
           	else if(nbytes < 0)
           	{
           		debug << "recv problem";
           		exit(1);
           	}
            else{ //Process received data	
         		Json::Value rec_msg = s2json(data);
         		if(rec_msg["type"].asInt() == 1){ // Chat_message
         			Chat_message msg(rec_msg);
					ofstream myfile("./chat/"+msg.sender+".echo",fstream::out|fstream::app);
         			myfile<< "<"<<msg.data<<"\n";
         			debug<<msg.data<<endl;
         			myfile.close();
         			ui->update = true;
         		}
         		else if (rec_msg["type"].asInt() == 2 ){
         			debug << data << endl;
         			Auth_message msg(rec_msg);
         			ui->logged_in  = rec_msg["status"].asBool();
         			ui->auth_ack_received = true;

					for (int i = 0; i < rec_msg["unread_list"].size(); ++i){
						debug<<rec_msg["unread_list"][i].get("sender","XXX").asString()<<endl;
						ofstream myfile("./chat/"+rec_msg["unread_list"][i].get("sender","XXX").asString()+".echo",fstream::out|fstream::app);
						myfile<<"<"<<rec_msg["unread_list"][i].get("data","------------------------").asString()<<endl;
						myfile.close();
					}
					ui->update = true;
         		}
         		else if(rec_msg["type"].asInt() == 4 ){
         			debug << data << endl;
         			Auth_message msg(rec_msg);
					
					ofstream myfile("./data/online_list.txt", fstream::out);
					for (int i = 0; i < rec_msg["online_users"].size(); ++i){
						myfile<<rec_msg["online_users"][i].asString()<<endl;
					}
					myfile.close();
					
					ofstream myfile2("./data/offline_list.txt", fstream::out);
					for (int i = 0; i < rec_msg["offline_users"].size(); ++i){
						myfile2<<rec_msg["offline_users"][i].asString()<<endl;
					}
					myfile2.close();
			        
			        debug << "online and offline users updated";
			        ui -> update = true;
         		}
	         	else if(rec_msg["type"].asInt() == 3){
	         		Auth_message msg(rec_msg);
	         	} 	
         	}
        }
    }
	pthread_exit(NULL);
}
int main(int  argc, char  *argv[])
{
	string LISTEN_PORT;
	string server_name;
	if ( argc != 3 ){
    	printf("usage :- ./client server_hostname port \n");
		return 0;
	}
  	else{
  		LISTEN_PORT = argv[2];
  		server_name = argv[1];
  	}
	initscr();
    cbreak();
    noecho();
    nonl();
    curs_set(0);
    keypad(stdscr, TRUE);
	int d_thread,i_thread,c_thread;
	UI ui;
	ui.LISTEN_PORT = LISTEN_PORT;
	ifstream testhome_file,testchat_file;
	testhome_file.open("data/user_list.txt");

	/*******Connecting to actual server*********/
	ofstream output_file("client_comm_log.txt");
		struct addrinfo hints, *servinfo, *p;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		
		int tmp = getaddrinfo(server_name.c_str(), SERVER_PORT, &hints, &servinfo);
		debug << "TOTO :" << tmp; 
		if (tmp != 0){
			output_file << "get addrinfo: " << gai_strerror(tmp) << endl;
			return 0;
		}
		
		int my_sock_fd;  
		for(p = servinfo; p != NULL; p = p->ai_next){ // loop through all the results and connect to the first we can
			if ((my_sock_fd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
				output_file << "client : socket " << SERVER_PORT << endl;
				// perror("client: socket");
				continue;
			}

			if (connect(my_sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
				output_file << "client: connect" << SERVER_PORT << endl;
				// perror("client: connect");
				output_file.close();
				close(my_sock_fd);
				continue;
			}

			break;
		}
		if (p == NULL) {
			output_file << "client: failed to connect " << endl;
			output_file.close();
			return 0;
		}	

		freeaddrinfo(servinfo);		 // all done with this structure
		// char serverIP[INET6_ADDRSTRLEN];
		// inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), serverIP, sizeof serverIP);
		// printf("client: connecting to %s\n", serverIP);
	/*******Connected********my_sock_fd*/

	/*******Complete the connection for other thread*********/
		int error_code = 0, k = 0;
		int listener_fd = get_listner(LISTEN_PORT.c_str() ,error_code);
		if(listener_fd == -1)
			exit(error_code);
	/*******Completed********listener_fd*/
	
	testhome_file.close();
	ui.my_sock_fd = my_sock_fd;ui.listener_fd = listener_fd;
	pthread_t display_thread,input_thread,communication_thread;
	d_thread = pthread_create(&display_thread,NULL,Display,(void *)&ui);
	i_thread = pthread_create(&input_thread,NULL,InputHandler,(void *)&ui);
	c_thread = pthread_create(&communication_thread,NULL,CommunicationHandler,(void *)&ui);
	if (d_thread || i_thread || c_thread)
	{
		endwin();
		return 0;
	}
	while (!ui.exit_program)
	{
		this_thread::sleep_for(chrono::milliseconds(50));
	}
	clear();
	endwin();
	return 0;
}