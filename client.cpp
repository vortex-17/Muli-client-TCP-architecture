/* Client */

/*

*** This is also in reference to Unix Networking Book by Stevens as well as Beej's Netwoking ***


<---------------- CONNECTION ESTABLISHMENT - CLIENT ----------------->

This file is to establish a connection between the user and the lender's machine
This program will create a TCP connection for Data transfer
This script will act as a client in the Server-Client Architecture


<-------------- FOR PERSONAL PURPOSE ----------------------->

        SERVER                  CLIENT 
          |                        |
      SETSOCKOPT                   |
          |                        |
        BIND                       |
          |                        |
        LISTEN  <--------------- CONNECT
          |                        |
        ACCEPT                     |
          |                        |
        COMMUNICATE <----------> COMMUNICATE

<----------------------------------------------------------->

*/

/* Importing necessary libraries */

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <cpuid.h>
#include <syslog.h>
#include <sys/uio.h>
#include <sys/event.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <pthread.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/route.h>

using namespace std;

#define PORT "3876"

void send_message(int sockfd, const string msg){

    // To send the resource message to the Server.
    cout << "Sending the message " << endl;
    cout << msg << endl;

    if(send(sockfd, msg.c_str(), msg.size(),0) == -1){
        cout << "Error sending the message" << endl;
    } else{
        cout << "Message sent" << endl;
        shutdown(sockfd,SHUT_RDWR);
    }

}


int connect(){

    char hostname[] = "<your_hostname>";
    char ip_address[] = "<your_ip_address>";


    int sockfd, numbytes;
    string message = "This is a test message. Thank you!"; 
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN]; //To store the IP_ADDRESS
    int f = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // string hostname;
    cout << "Connecting to the Host...\n" << endl;

    rv = getaddrinfo(ip_address,PORT,&hints,&servinfo);
    if(rv != 0){
        exit(1);
    }

    for(p = servinfo; p!=NULL; p = p->ai_next){

        sockfd = socket(p->ai_family,p->ai_socktype, p->ai_protocol);
        if(sockfd == -1){
            //error message
            continue;
        } 

        if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            cout << "Couldnt Connect\n" << endl;
        } else {
            cout << "Connected " << endl;
            return sockfd;
        }

        break;
    }

    if(p==NULL){
        cout << "Couldnt connect to the host" << endl;
        return 0;
    }

    send_message(sockfd,message);

    return sockfd;
    

}

int main(){

    int conn = connect();
    return 0;
}
