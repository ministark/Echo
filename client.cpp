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
#define LISTEN_PORT "9033"	//local port for thread intercommunication

mutex mtx_user_list,mtx_display,mtx_comm;
struct Chat_message{ // peer to peer messages
	int time_stamp;
	int type;
	string receiver, sender;
	string data;
	Chat_message(){}
	Chat_message(string username, int time, string rec, int t){
		time_stamp = time;
		sender = username;
		receiver = rec;
		type = t;
	}
	Chat_message(Json::Value root){
		time_stamp = root["time_stamp"].asInt();
		receiver = root["receiver"].asString();
		sender = root["sender"].asString();
		data = root["data"].asString();
		type = root["type"].asInt();
	}
	string to_str(){
		string s = "{\n";
		s+= assign("type",type) + ",\n" +
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

struct User{
	string name,status;
	vector <Chat_message> message_list;
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
	int type,cursor_x,cursor_y,scroll;
	bool update,exit_program;
	vector<User> user_list;
	map <string,int> user_map;
	int number_online,unread_messages;
	int my_sock_fd,listener_fd;
	User receipent; //The person whom we are talking with
	char display[24][80];
	UI(){
		type = 0;
		update = true;
		exit_program = false;
		number_online = 1;
		unread_messages = 0;
		scroll = 0;
		load_ui();
	}
	void load_ui(){
		int i = 0;
		string template_name,template_line;
		ifstream template_file;
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
		template_file.open(template_name);
		while (getline(template_file,template_line)){
			for (int j=0; j<80; j++)
				display[i][j] = template_line[j];
			i++;
		}
		template_file.close();
	}
	void edit_display(int x,int y,string s){
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
	this_thread::sleep_for(chrono::milliseconds(5000));
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
				//ui->edit_display(2,40,"Active Echo : " + string(itoa(ui->number_online)));
				//ui->edit_display(2,60,"Unread Echo : " + string(itoa(ui->unread_messages)));
				int line_x = 8,list_size = ui->user_list.size();
				for (int j=8; j<20; j++)
					ui->edit_display(j,25,"                                            ");
				for (int j=ui->scroll; j<min(int(list_size),ui->scroll+12); j++)
				{
					User user = ui->user_list[j];
					ui->edit_display(line_x,25,user.name);
					ui->edit_display(line_x,50,user.status);
					line_x++;
				}

				// Display number of online-user by iterating over map
			}
			else if (ui->type == 2) // Chat Window
			{
				// Check Notification Bar
				// Process messages from user->messages and paste it in display

			}
			RefreshDisplay(ui->display);
			ui->update = false;
		}
		//mtx_display.unlock();
	}
	pthread_exit(NULL);
}

bool authenticate(string username,string password)
{
	return true;
}	

void ProcessInput(UI *ui)
{
	return;
}

void *InputHandler(void *thread_arg)
{
	this_thread::sleep_for(chrono::milliseconds(5000));
	/*******Connecting to actual server*********/
			ofstream output_file("client_comm_log.txt");
			struct addrinfo hints1, *servinfo1, *p1;
			memset(&hints1, 0, sizeof hints1);
			hints1.ai_family = AF_INET;
			hints1.ai_socktype = SOCK_STREAM;
			
			int tmp = getaddrinfo("localhost", LISTEN_PORT, &hints1, &servinfo1);
			if (tmp != 0){
				output_file << "get addrinfo: " << gai_strerror(tmp) << endl;
				return NULL;
			}
			
			int self_client_sock_fd;  
			for(p1 = servinfo1; p1 != NULL; p1 = p1->ai_next){ // loop through all the results and connect to the first we can
				if ((self_client_sock_fd = socket(p1->ai_family, p1->ai_socktype,p1->ai_protocol)) == -1) {
					output_file << "client : socket " << LISTEN_PORT << endl;
					// perror("client: socket");
					continue;
				}

				if (connect(self_client_sock_fd, p1->ai_addr, p1->ai_addrlen) == -1) {
					output_file << "client: connect" << LISTEN_PORT << endl;
					// perror("client: connect");
					close(self_client_sock_fd);
					continue;
				}
				break;
			}
			if (p1 == NULL) {
				output_file << "client: failed to connect " << endl;
				return NULL;
			}	
			freeaddrinfo(servinfo1);
			// char serverIP[INET6_ADDRSTRLEN];
			// inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), serverIP, sizeof serverIP);
			// printf("client: connecting to %s\n", serverIP);
	/*******Connected*********/
	UI *ui = (UI *)thread_arg;
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
					if (authenticate(username,password))
					{
						ui->type = 1;
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
					if (ui->type == 1 and ui->scroll < ui->user_list.size())
					{
						ui->scroll++;
						ui->update = true;
					}
				}
				else if (ch == KEY_UP and ui->scroll > 0)
				{
					ui->scroll--;
					ui->update = true;
				}
			}
			else
			{
				ProcessInput(ui);
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
		cout << "XXX" << endl;
	    perror("accept"); 
	}
	else{
	    FD_SET(self_sock_id, &master_fds); // add to master set
	    fd_max = max(self_sock_id,fd_max); // Keep track of maxfd
	    // inet_ntop(self_address.sa_family, &((struct sockaddr_in*)&self_address)->sin_addr,selfIP, INET_ADDRSTRLEN);
		close(listener_fd);
	}

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
		    	// printf("selectserver: socket %d hung up\n", self_sock_id);
           	}
           	else if(nbytes < 0)
       		{
           		// perror("recv");
       		}
            else{
         		char *to_send = new char[data.length() + 1];
				strcpy(to_send, data.c_str());                				
 				if (send(my_sock_fd, to_send, data.length() + 1, 0) == -1) {
                    // perror("send");
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
		    	// printf("selectserver: socket %d hung up\n", my_sock_fd);
           	}
           	else if(nbytes < 0)
           	{
           		// perror("recv");
           	}
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
	pthread_exit(NULL);
}
int main()
{
	initscr();
    cbreak();
    noecho();
    nonl();
    curs_set(0);
    keypad(stdscr, TRUE);
	int d_thread,i_thread,c_thread;
	UI ui;
	ifstream testhome_file,testchat_file;
	testhome_file.open("data/online_list.txt");
	int k = 0;
	/*******Connecting to actual server*********/
		ofstream output_file("client_comm_log.txt");
		struct addrinfo hints, *servinfo, *p;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		
		int tmp = getaddrinfo("localhost", SERVER_PORT, &hints, &servinfo);
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
				close(my_sock_fd);
				continue;
			}

			break;
		}
		if (p == NULL) {
			output_file << "client: failed to connect " << endl;
			return 0;
		}	
		freeaddrinfo(servinfo);		 // all done with this structure
		// char serverIP[INET6_ADDRSTRLEN];
		// inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), serverIP, sizeof serverIP);
		// printf("client: connecting to %s\n", serverIP);
	/*******Connected********my_sock_fd*/

	/*******Complete the connection for other thread*********/
		int error_code = 0;
		int listener_fd = get_listner(LISTEN_PORT,error_code);
		if(listener_fd == -1)
			exit(error_code);	
	/*******Completed********listener_fd*/
	
	while (!testhome_file.eof())
	{
		string name,status;
		testhome_file >> name >> status;
		ui.user_list.push_back(User(name,status));
		ui.user_map[name] = k++;
	}
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
	pthread_exit(NULL);
	endwin();
}