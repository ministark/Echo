#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <map>
#include "utils.h"
using namespace std;
#define ENTER_KEY 13
#define LOWER_KEY 31
#define UPPER_KEY 127
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
struct User
{
	string name,status;
	vector <Chat_message> message_list;
	User()
	{
		name = "default";
		status = "offline";
	}
	User(string name,string status)
	{
		this->name = name;
		this->status = status;
	}
};

struct UI
{
	int type,cursor_x,cursor_y,scroll;
	bool update,exit_program;
	vector<User> user_list;
	map <string,int> user_map;
	int number_online,unread_messages;
	User receipent; //The person whom we are talking with
	char display[24][80];
	UI()
	{
		type = 0;
		update = true;
		exit_program = false;
		number_online = 1;
		unread_messages = 0;
		scroll = 0;
		load_ui();
	}
	void load_ui()
	{
		int i = 0;
		string template_name,template_line;
		ifstream template_file;
		if (type == 0)
		{
			template_name = "ui/ui_login.txt";
			cursor_x = 13;
			cursor_y = 39;
		}
		else if (type == 1)
		{
			template_name = "ui/ui_home.txt";
			cursor_x = 23;
			cursor_y = 3;
		}
		template_file.open(template_name);
		while (getline(template_file,template_line))
		{
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
void RefreshDisplay(char display[][80])
{
	for (int i=0; i<24; i++)
	{
		for (int j=0; j<80; j++)
		{
			mvprintw(i,j,"%c",display[i][j]);
			refresh();
		}
	}
}
void *Display(void *thread_arg)
{
	UI *ui = (UI *)thread_arg;
	while (!ui->exit_program)
	{
		if (ui->update)
		{
			if (ui->type == 0) // Login
			{
                
			}
			else if (ui->type == 1) // Home 
			{
				//ui->edit_display(2,40,"Active Echo : " + string(itoa(ui->number_online)));
				//ui->edit_display(2,60,"Unread Echo : " + string(itoa(ui->unread_messages)));
				int line_x = 8;
				for (int j=8; j<20; j++)
					ui->edit_display(j,25,"                                            ");
				for (int j=ui->scroll; j<min(int(ui->user_list.size()),ui->scroll+12); j++)
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
	UI *ui = (UI *)thread_arg;
	bool command_start = false,username_read,password_read;
	int ch;
	string username,password;
	while (!ui->exit_program)
	{
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
		this_thread::sleep_for(chrono::milliseconds(50));
	}
	pthread_exit(NULL);
}
void *CommunicationHandler(void *thread_arg)
{
	UI *ui = (UI *)thread_arg;
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
	while (!testhome_file.eof())
	{
		string name,status;
		testhome_file >> name >> status;
		ui.user_list.push_back(User(name,status));
		ui.user_map[name] = k++;
	}
	testhome_file.close();
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