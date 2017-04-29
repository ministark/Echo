#include <json/json.h>
#include <json/json-forwards.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include <iostream>
#include <string>
#include <set>

using namespace std;

string assign(string key , int value);
string assign(string key , string value);
string assign(string key , bool value);
Json::Value s2json(string s);
void *get_in_addr(struct sockaddr *sa);
string read_full(int sock_fd,int &nbytes);
int get_listner(const char * port,int &error_code);

struct Chat_message{ // peer to peer messages
	int time_stamp;
	string receiver, sender;
	string data;
	Chat_message();
	Chat_message(Json::Value root);
	Chat_message(string rcvr, string sndr,string dat);
	string to_str();
};

struct Group_formation_message{ // peer to peer messages
	int time_stamp;
	string sender, group_name;
	set<string> members;
	Group_formation_message(Json::Value root);
	string to_str();
};