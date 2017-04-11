#include <iostream>
#include <vector>
#include <string>
#include <curses.h>
#include <pthread.h>
#include <unistd.h>
using namespace std;
struct Message
{
	string from,to,timestamp,data;
};
struct Notification
{
	int number_online,unread_messages;
};
struct User
{
	string name,status;
};
struct Chat
{
	User user;
	vector <Message> message_list;
};
struct Home
{
	vector <User> user_list;
};
struct UI
{
	int type;
	bool update,exit_program;
	Notification notification;
	vector <string> display;
	UI()
	{
		update = true;
		exit_program = false;
		display = vector <string> (24,"~\n");
		display[23] = "";
	}
};


void RefreshDisplay(vector <string> &line)
{
	clear();
    for (int i=0; i<24; i++) {
		addstr(&line[i][0]);
		refresh();
	}
}
void *Display(void *thread_arg)
{
	UI *ui = (UI *)thread_arg;
	while (!ui -> exit_program)
	{
		if (ui -> update)
		{
			RefreshDisplay(ui -> display);
			ui -> update = false;
		}
	}
	pthread_exit(NULL);
}	
void *InputHandler(void *thread_arg)
{
	UI *ui = (UI *)thread_arg;
	bool command_start = false;
	while (!ui -> exit_program)
	{
		char ch = getch();
		if (command_start)
		{	
			ui->display[23] += ch;
			ui -> update = true;
		}
		if (ch == ':')
			command_start = true;
		usleep(10);
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