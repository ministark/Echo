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
	char display[24][80];
	UI()
	{
		update = true;
		exit_program = false;
		for (int i=0; i<24; i++)
		{
			for (int j=0; j<80; j++)
				display[i][j] = ' ';
			if (i != 23)
				display[i][0] = '~';
		}
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
void ProcessInput(string command)
{

	return;
}
void *InputHandler(void *thread_arg)
{
	UI *ui = (UI *)thread_arg;
	bool command_start = false;
	int position = 0;
	while (!ui -> exit_program)
	{
		char ch = getch();
		if (command_start and ch != 13)
		{	
			ui->display[23][position++] = ch;	
			ui->update = true;
		}
		else if (command_start)
		{
			command_start = false;
			ProcessInput(ui -> display[23]);
			for (int i=0; i<80; i++)
				ui->display[23][i] = ' ';
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
    // curs_set(0);
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