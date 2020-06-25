/* Types of Servers using different methods */

/*

*** This is also in reference to Unix Networking Book by Stevens as well as Beej's Netwoking ***


<---------------- CONNECTION ESTABLISHMENT - SERVER ----------------->

This file is to establish a connection between the user and the lender's machine
This program will create a TCP connection for Data transfer
This script will act as a server in the Server-Client Architecture


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

void peerinfo(int sockfd){
    /* Returning the peer info */
    struct sockaddr_storage addr;
    socklen_t len;
    int port;
    char str[INET6_ADDRSTRLEN];

    getpeername(sockfd,(struct sockaddr*)&addr,&len);
    if(addr.ss_family == AF_INET){
        struct sockaddr_in *s = (struct sockaddr_in*)&addr;
        port = ntohs(s->sin_port);
        inet_ntop(AF_INET,&s->sin_addr,str,sizeof(str));
    } else if(addr.ss_family ==AF_INET6){
        struct sockaddr_in6 *s = (struct sockaddr_in6*)&addr;
        port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6,&s->sin6_addr,str,sizeof(str));
    }

    cout << "Printing Peer info" << endl;
    cout << "IP Address: " << str << endl << "Port: " << port << endl;

}

int create_tcp_server(){  //using select function

    int max_clients = 25;
    int sockfd, newfd, i, rv, yes = 1, clients_list[max_clients];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    int f = 0;


    fd_set master;
    fd_set temp;
    fd_set xset;
    int fdmax; // to store the value of highest descriptor.
    FD_ZERO(&master);
    FD_ZERO(&temp);
    FD_ZERO(&xset);

    int ret;
    char buffer[100];

    if ((ret = gethostname(buffer, sizeof(buffer))) == -1)
    {
        perror("gethostname");
        exit(1);
    }

    struct hostent *host_entry;
    host_entry = gethostbyname(buffer);
    char *IP;
    IP = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0])); 

    cout << buffer << ": " << IP << endl;
 
    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    for(i = 0;i<max_clients;i++){ //initialising all the clients with 0 i.e. empty.
        clients_list[i] = 0;
    }

    rv = getaddrinfo(NULL,PORT,&hints,&servinfo);
    if(rv == -1){
        cout << "Error" << endl;
    }

    //process of binding to the port.
    for(p = servinfo;p!=NULL;p = p->ai_next){

        sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if(sockfd == -1){
            perror("socket");
            continue;
        }

        if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) == -1){
            perror("setsockopt");
            exit(1);
        }
        
        if(setsockopt(sockfd,SOL_SOCKET,SO_OOBINLINE,&f,sizeof(int)) == -1){
            perror("setsockopt");
            exit(1);
        }

        int a  = bind(sockfd,p->ai_addr,p->ai_addrlen);
        if(a == -1){
            close(sockfd);
            continue;
        }
        break;

    }

    if(p == NULL){
        cout << "Failed to Bind " << endl;
    }

    freeaddrinfo(servinfo);

    int BACKLOG = 10;
    if(listen(sockfd,BACKLOG) == -1){
        perror("listen");
        exit(1);
    }

    int activity, sd;
    sin_size = sizeof(their_addr);

    while(1){

        FD_ZERO(&temp);

        FD_SET(sockfd,&temp);
        FD_SET(sockfd,&xset);
        fdmax = sockfd;

        for(i=0;i<max_clients;i++){
            sd = clients_list[i];

            if(sd > 0){
                FD_SET(sd,&temp);
                FD_SET(sd,&xset);
            }

            if(sd > fdmax){
                fdmax = sd;
            }
        }

        activity = select(fdmax+1, &temp, NULL, &xset, NULL);
        if(activity < 0){
            perror("select");
        }

        if(FD_ISSET(sockfd,&temp)){

            newfd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
            if(newfd == -1){
                perror("accept");
            }
            cout << "Got new connection" << endl;
            sleep(5);
            for( i =0 ;i<max_clients;i++){
                if(clients_list[i] == 0){
                    clients_list[i] = newfd;

                    break;
                }
            }

        } 

        for(i = 0;i < max_clients; i++){
            sd = clients_list[i];

            if(FD_ISSET(sd,&xset)){
                char buf[5];
                int n = recv(sd,buf,sizeof(buf),MSG_OOB);
                if(n == -1){
                    perror("recv:");
                    exit(1);
                }
                buf[n] = 0;
                cout << "MSG_OOG: " << buf << endl;
            }

            if(FD_ISSET(sd,&temp)){
                sin_size = sizeof(their_addr);
                char buf[10];
                int arg = 0,n;
                do{
                    n = recv(sd,buf,sizeof(buf),0);
                    if(n > 0){
                        buf[n] = '\0';
                        cout << "from the node: " << buf << endl;
                        ioctl(sd,FIONREAD,&arg); 
                    } else{
                        arg = 0;
                    }
                } while (arg > 0);
                if(n == -1){
                    perror("recv");
                }

            }
        }


    }
    
    return 0;
}

int multiclient_tcpserver_fork(){

    int sockfd, newfd, yes = 1, rv,i;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    rv = getaddrinfo(NULL,PORT,&hints,&servinfo);
    if(rv == -1){
        perror("getaddrinfo:");
    }

    for(p = servinfo;p!=NULL; p = p->ai_next){

        sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if(sockfd == -1){
            perror("socket:");
            continue;
        }

        if(setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR,&yes, sizeof(yes)) == -1){
            perror("socktopt:");
            exit(1);
        }

        int n;
        n = bind(sockfd,p->ai_addr,p->ai_addrlen);
        if(n == -1){
            perror("bind");
            continue;
        }

        

        break;
    }

    if(p == NULL){
        cout << "Couldn't bind to the port" << endl;
        exit(1);
    }

    freeaddrinfo(servinfo);

    socklen_t z;
    size_t len;
    int q = getsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,&len,&z);
    if(q == -1){
        perror("getsockopt:");
        exit(1);
    } else{
        cout << q << endl;
    }

    int BACKLOG = 10;
    if(listen(sockfd,BACKLOG) == -1){
        perror("listen");
        exit(1);
    }

    pid_t pid;

    while(1){

        sin_size = sizeof their_addr;
        newfd = accept(sockfd, (struct sockaddr*)&their_addr,&sin_size); // blocks until it find a client and connects to it.
        if((pid = fork()) == -1){
            perror("fork");
        } else if(pid > 0){
            cout << "successfull creation of socket" << endl;
            cout << "child process created: " << pid << endl;
            char buf[100];
            int n = recv(newfd,buf,sizeof buf,0);
            if(n == -1){
                //error message
                cout << "didnt get the message" << endl; 
                continue;
            }

            buf[n] = '\0';
            cout << "From Node: " << buf << endl;
            sleep(7);
            close(newfd);
            kill(pid,SIGKILL);

        }
    }


    return 0;
}

int multiclient_tcpserver_poll(){ //using poll function to handle multiple clients

    int sockfd, newfd;
    int yes = 1, i, max_clients = 25, rv, maxi;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct pollfd client_list[max_clients];

    // initialising all the client sockets as -1
    for(i = 0; i< max_clients; i++){
        client_list[i].fd = -1;
        client_list[i].events = POLLIN | POLLOUT | POLLRDNORM;
    }

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    rv = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if(rv == -1){
        perror("getaddrinfo");
        exit(1);
    }

    for(p = servinfo; p!=NULL; p=p->ai_next){

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sockfd == -1){
            perror("socket");
            continue;
        }

        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
            perror("setsockopt:");
            exit(1);
        }

        int n = bind(sockfd, p->ai_addr, p->ai_addrlen);
        if(n == -1){
            close(sockfd);
            perror("bind");
            continue;
        }

        break;
    }

    if(p == NULL){
        cout << "server: failed to bind" << endl;
    }

    freeaddrinfo(servinfo);

    if(listen(sockfd,10) == -1){
        perror("listen");
        exit(1);
    }

    maxi = sockfd;
    int activity;
    client_list[0].fd = sockfd; // first index of this structure will be saved to accept a new client


    while(1){

        activity = poll(client_list,maxi+1,5000);
        if(client_list[0].revents & POLLRDNORM){
            // got a new_client
            sin_size = sizeof(their_addr);
            newfd = accept(sockfd,(struct sockaddr*)&their_addr,&sin_size);
            for(i = 1; i<max_clients;i++){
                if(client_list[i].fd < 0){
                    client_list[i].fd = newfd;
                    break;
                }
            }

            if(newfd > maxi){
                maxi = newfd;
            }
            
        }

        /* Checking for data from exisiting clients */


        for(i = 1; i<max_clients; i++){
            if(client_list[i].revents & POLLIN){
                char buf[100];
                int n = recv(client_list[i].fd, buf, sizeof(buf), 0);
                //displaying the peer information
                // peerinfo(client_list[i].fd);
                if(n == -1){
                    perror("recv");
                } else if(n == 0){
                    cout << "Client has closed the connection" << endl;
                    client_list[i].fd = -1;
                } else{
                    buf[n] = '\0';
                    cout << "From node: " << buf << endl;
                }
                
            }
        }

    }




    return 0;
}

int tcp_server_using_kqueue(){  //use kqueue with non-blocking sockets using fcntl and a infinte loop to read data until EWOULDBLOCK error

    int sockfd, newfd;
    int i,rv, yes = 1;
    int kq, nev;
    struct sockaddr_storage their_addr;
    struct addrinfo hints, *servinfo, *p;
    socklen_t sin_size;
    char buf[100];
    int max_clients = 10;
    struct kevent kev;
    struct kevent kev_list[max_clients];
    struct timespec ts;

    cout << "Running a TCP Server using Kqueue" << endl;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    rv = getaddrinfo(NULL,PORT,&hints,&servinfo);
    if(rv != 0){
        perror("getaddrinfo: ");
        return 1;
    }

    for(p = servinfo;p!=NULL;p = p->ai_next){
        sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if(sockfd == -1){
            perror("socket:");
            continue;
        }

        if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1){
            perror("setsockopt:");
            exit(1);
        }

        if(bind(sockfd,p->ai_addr,p->ai_addrlen) == -1){
            perror("bind");
            exit(1);
        }

        break;
    }

    if(p == NULL){
        cout << "Could not bind to the port" << endl;
        exit(2);
    }

    free(servinfo);

    if(listen(sockfd,10) == -1){
        perror("listen:");
        exit(3);
    }

    //using kevents
   
    kq = kqueue();
    EV_SET(&kev,sockfd,EVFILT_READ,EV_ADD,0,0,NULL);
    if(kevent(kq,&kev,1,NULL,0,0) == -1){
        perror("kevent:");
    }

    while(1){

        nev = kevent(kq,NULL,0,kev_list,max_clients,0);
        if(nev < 1){
            perror("kevents");
        }
        for(i = 0;i<nev;i++){
            if(kev_list[i].flags & EV_EOF){
                cout << "Disconnected" << endl;
                newfd = kev_list[i].ident;
                EV_SET(&kev,newfd,EVFILT_READ,EV_DELETE,0,0,NULL);
                if(kevent(kq,&kev,1,NULL,0,0) == -1){
                    perror("kevent:");
                }
            } else if(kev_list[i].ident == sockfd){
                sin_size = sizeof(their_addr);
                newfd = accept(sockfd,(struct sockaddr*)&their_addr,&sin_size);
                EV_SET(&kev,newfd,EVFILT_READ,EV_ADD,0,0,NULL);
                if(kevent(kq,&kev,1,NULL,0,0) == -1){
                    perror("kevents 2:");
                }

            } else if(kev_list[i].filter == EVFILT_READ){

                int n = recv(kev_list[i].ident,buf,sizeof(buf),0);
                if(n == -1){
                    perror("recv");
                } else{
                    buf[n] = '\0';
                    cout << "Message : " << buf << endl;
                }
                
            }
            
        }

    }


    

    return 0;
}

int main(){

    int x = create_tcp_server(); //use any of the above function to create server
    return 0;

}