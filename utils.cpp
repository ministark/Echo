#include "utils.h"

Json::Value s2json(string s){
	Json::Reader reader;
	Json::Value root;
	bool parsingSuccessful = reader.parse( s.c_str(), root );     //parse process
    if (!parsingSuccessful){
        std::cout  << "Failed to parse\n" << reader.getFormattedErrorMessages();
    }
    return root;
}
string assign(string key , int value){
	return ("\"" + key + "\": " + to_string(value) ); 
}
string assign(string key , string value){
	return ("\"" + key + "\":\"" + value + "\""); 
}
string assign(string key , bool value){
	return ("\"" + key + "\": " + to_string(value) );  
}

void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

string read_full(int sock_fd,int &nbytes){
	string data = "";
	while(true){
		char buf[256] = {};    	// buffer for client data
		nbytes = recv(sock_fd, buf, 256, 0); 

		if (nbytes <= 0) // got error
	        return "";
	    else{
	    	data += string(buf);
	    	if(data.substr(data.length()-3,3) == "```"){
	    		data = data.substr(0,data.length()-3);
	    		return data;
	    	}
	    	else{
	    		cout << data << endl;
	    	}
	    }
	}
}
int get_listner(const char *port,int &error_code){

	struct addrinfo  hints, *ai_list, *p;
	int listener_fd;     	// listening socket descriptor

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	int tmp1 = getaddrinfo(NULL, port, &hints, &ai_list);
	if (tmp1 != 0) {//ERROR
		fprintf(stderr, "selectlistner: %s\n", gai_strerror(tmp1));
		exit(1);
		error_code = 1;
		return -1;
	}
	
	int yes=1;	// for setsockopt() SO_REUSEADDR, below  	
	for(p = ai_list; p != NULL; p = p->ai_next) { //iterate over obtined list of addresses
    	listener_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener_fd < 0) 
			continue;
		
		setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); // If address already in use try to reuse it

		if (bind(listener_fd, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener_fd); // close prev listener and try again
			continue;
		}
		break;
	}
	if (p == NULL) {// if we got here, it means we didn't get bound
		fprintf(stderr, "selectserver: failed to bind\n");
		error_code = 2;
		return -1;
	}

	freeaddrinfo(ai_list); // all done with this

    // listen
    if (listen(listener_fd, 10) == -1) {
        perror("listen");
        error_code = 3;
        return -1;
    }	
}

