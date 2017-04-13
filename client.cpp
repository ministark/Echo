#include <iostream>
#include <vector>
#include <string>
#include <string.h>
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
struct TermData
{
	int type;
	bool update,exit_program;
	Notification notification;
	char display[24][80];
	TermData()
	{
		// Initizaling the terminal for starting page
		update = true;
		exit_program = false;
		for (int i=0; i<24; i++)
		{
			for (int j=0; j<80; j++)
				display[i][j] = ' ';
			if (i != 23)
				display[i][0] = '~';
		}
		strncpy(display[0], "                                ___ ___ _  _  ___   ", 80);
		strncpy(display[1], "                               | __/ __| || |/ _ \\ ", 80);
		strncpy(display[2], "                       --------| _| (__| __ | (_) --------", 80);
		strncpy(display[3], "                               |___\\___|_||_|\\___/ ", 80);
		memset(display[22],'_',sizeof(display[23]));

	}
};

//Display thread
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
	move(23,0);refresh();
}
void *Display(void *thread_arg)
{
	TermData *ui = (TermData *)thread_arg;
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
	if(command == "echo" ){

	}
	return;
}

//Handles the input and then manages the state transition
void *InputHandler(void *thread_arg)
{
	TermData *ui = (TermData *)thread_arg;
	bool command_start = false;
	int position = 3;
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
			position = 3;
		}
		if (ch == ':'){
			ui -> display[23][0] = '-';	ui -> display[23][1] = '>';ui -> display[23][2] = ' ';
			command_start = true; ui -> update = true;
		}
		usleep(10);
	}
	pthread_exit(NULL);
}

//Thread for handling communication and updating the term
void *CommunicationHandler(void *thread_arg)
{
	TermData *ui = (TermData *)thread_arg;
	pthread_exit(NULL);
}
int main()
{
	//Setting up the Terminal and Ncurser
	initscr();cbreak();
    noecho();nonl();
    curs_set(0);keypad(stdscr, TRUE);

    //Creating Thread for Communication, Display and Input
	int d_thread,i_thread,c_thread;
	TermData ui;
	pthread_t display_thread,input_thread,communication_thread;
	d_thread = pthread_create(&display_thread,NULL,Display,(void *)&ui);
	i_thread = pthread_create(&input_thread,NULL,InputHandler,(void *)&ui);
	c_thread = pthread_create(&communication_thread,NULL,CommunicationHandler,(void *)&ui);
	if (d_thread || i_thread || c_thread)
	{
		endwin();
		return 0;
	}

	//Exiting
	pthread_exit(NULL);
	endwin();
}