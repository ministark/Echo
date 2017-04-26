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
using namespace std;
#define ENTER_KEY 13
struct Message
{
	string from,to,timestamp,data;
};
struct User
{
	string name,status;
	vector <Message> message_list;
};

struct UI
{
	int type,cursor_x,cursor_y;
	bool update,exit_program;
	map<string,User> user_list;
	int number_online,unread_messages;
	User *curr_chat_window; //The person whom we are talking with
	char display[24][80];
	UI()
	{
		curr_chat_window = NULL;
		type = 0;
		update = true;
		exit_program = false;
		number_online = 1;
		unread_messages = 0;
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
				else if (ch != KEY_BACKSPACE and ui->cursor_y < 55)
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
						ui->edit_display(19,25,"invalid username or password");
						ui->update = true;
 					}
				}
			}
		}
		//Home Screen & Chat Windo
		else
		{
			if (ch != ENTER_KEY)
			{	
				if (ch == KEY_BACKSPACE and ui->cursor_y > 3)
				{
					ui->display[ui->cursor_x][--ui->cursor_y] = ' ';
					ui->update = true;
				}
				else if (ch != KEY_BACKSPACE and ui->cursor_y < 78)
				{
					ui->display[ui->cursor_x][ui->cursor_y++] = ch;
					ui->update = true;
				}	
			}
			else
			{
				ProcessInput(ui);
				for (int i=0; i<80; i++)
					ui->display[ui->cursor_x][i] = ' ';
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