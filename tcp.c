/* 
 * tcpclient.c - based on
 * http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpclient.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "tcp.h" 

#define BUFSIZE 64

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

void sendMessageToServer(char* hostname, int port, char* buf)
{
    int sockfd, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
      (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port);

    /* connect: create a connection with the server */
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");

    printf("%s\n", buf);

    write(sockfd, LOGIN, strlen(LOGIN));
    char end2[BUFSIZE];
    end2[0] = (char)-128;
    end2[1] = '\0';
    write(sockfd, end2, strlen(end2));
    /* send the message line to the server */
    n = write(sockfd, buf, strlen(buf));
    if (n < 0) 
      error("ERROR writing to socket");
    
    char end[BUFSIZE];
    end[0] = (char)-128;
    end[1] = '\0';
    write(sockfd, end, strlen(end));
    
    close(sockfd);
}

char* readMessageFromServer(char* hostname, int port, char* buf)
{
    int sockfd, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
      (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port);

    /* connect: create a connection with the server */
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");

    /* print the server's reply */
    bzero(buf, BUFSIZE);
    n = read(sockfd, buf, BUFSIZE);
    if (n < 0) 
      error("ERROR reading from socket");
    printf("Message received: %s", buf);
    close(sockfd);
    
    return buf;
}

char* receiveGUIMessage(int port, char* buf)
{
  int parentfd; /* parent socket */
  int childfd; /* child socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  
  portno = port;

  /* 
   * socket: create the parent socket 
   */
  parentfd = socket(AF_INET, SOCK_STREAM, 0);
  if (parentfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
         (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));

  /* this is an Internet address */
  serveraddr.sin_family = AF_INET;

  /* let the system figure out our IP address */
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* this is the port we will listen on */
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(parentfd, (struct sockaddr *) &serveraddr, 
       sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * listen: make this socket ready to accept connection requests 
   */
  if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
    error("ERROR on listen");

  /* 
   * main loop: wait for a connection request, echo input line, 
   * then close connection.
   */
  clientlen = sizeof(clientaddr);
    /* 
     * accept: wait for a connection request 
     */
    childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (childfd < 0) 
      error("ERROR on accept");
    
    /* 
     * gethostbyaddr: determine who sent the message 
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
              sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server established connection with %s (%s)\n", 
       hostp->h_name, hostaddrp);
    
    /* 
     * read: read input string from the client
     */
    bzero(buf, BUFSIZE);
    n = read(childfd, buf, BUFSIZE);
    if (n < 0) 
      error("ERROR reading from socket");
    printf("server received %d bytes: %s", n, buf);
    
//    /* 
//     * write: echo the input string back to the client 
//     */
//    n = write(childfd, buf, strlen(buf));
//    if (n < 0) 
//      error("ERROR writing to socket");

    close(childfd); 
    close(parentfd);
    return buf;
    
}

char* createBalanceUpdate(int cid, float diff, char* msgParam)
{
    int i;
    
    char cidBuf[BUFSIZE];
    char diffBuf[BUFSIZE];
   
    snprintf(cidBuf, BUFSIZE, "%d", cid);
    snprintf(diffBuf, BUFSIZE, "%f", diff);
    
    printf("%s %s\n", cidBuf, diffBuf);
    
    //cmd + ' ' + cid + ' ' + '-' + balance
    i = 1 + 1 + strlen(cidBuf) + 1 + strlen(diffBuf);
    msgParam[0] = '5';
    msgParam[1] = ' ';
    msgParam[2 + strlen(cidBuf)] = ' ';
    strncpy(msgParam + 2, cidBuf, strlen(cidBuf));
    strncpy(msgParam + 2 + strlen(cidBuf) + 1, diffBuf, strlen(diffBuf));
    msgParam[i] = '\0';
    printf("Message: %s\n", msgParam);

    return msgParam;
}

char* createEntry(int cid, float balance, char* msgParam)
{
    int i;
    
    char cidBuf[BUFSIZE];
    char balanceBuf[BUFSIZE];
   
    snprintf(cidBuf, BUFSIZE, "%d", cid);
    snprintf(balanceBuf, BUFSIZE, "%f", balance);
    printf("%s %s\n", cidBuf, balanceBuf);
    
    //cmd + ' ' + cid + ' ' + '-' + balance
    i = 1 + 1 + strlen(cidBuf) + 1 + strlen(balanceBuf);
    msgParam[0] = '4';
    msgParam[1] = ' ';
    msgParam[2 + strlen(cidBuf)] = ' ';
    strncpy(msgParam + 2, cidBuf, strlen(cidBuf));
    strncpy(msgParam + 2 + strlen(cidBuf) + 1, balanceBuf, strlen(balanceBuf));
    msgParam[i] = '\0';
    printf("Message: %s\n", msgParam);

    return msgParam;
}

char* createGetEntry(int cid, char* msgParam)
{
    //TODO: email (from GUI?), not stored on card)
    char cidBuf[BUFSIZE];
    
    snprintf(cidBuf, BUFSIZE, "%d", cid);
    
    //cmd + ' ' + cid
    int i = 1 + 1 + strlen(cidBuf);
    msgParam[0] = '7';
    msgParam[1] = ' ';
    strncpy(msgParam + 2, cidBuf, strlen(cidBuf));
    msgParam[i] = '\0';
    printf("Message: %s\n", msgParam);
    
    return msgParam;
}

// int main(int argc, char **argv) {
    
//     char msgBuf[BUFSIZE];
//     int port = 5555;
//     char hostname[] = "localhost";
//     //char hostname[] = "192.168.100.117";
    
//     //update balance "4 cid balance"
//     //p->cid, p->balance
//     int cid = 1234567890;
//     //float balance = 2.5;
//     float balance = -2.5;
    
// //    char* msgParam = malloc(BUFSIZE);
// //    strcpy(msgBuf, createBalanceUpdate(cid, balance, msgParam));
// //    sendMessageToServer(hostname, port, msgBuf);
// //    sleep(1);
// //    free(msgParam);
// //    
// //    msgParam = malloc(BUFSIZE);
// //    strcpy(msgBuf, createEntry(cid, 200, msgParam));
// //    sendMessageToServer(hostname, port, msgBuf);
// //    sleep(1);
// //    free(msgParam);
// //    
// //    msgParam = malloc(BUFSIZE);
// //    strcpy(msgBuf, createGetEntry(cid, msgParam));
// //    sendMessageToServer(hostname, port, msgBuf);
// //    sleep(1);
// //    free(msgParam);
    
//     readMessageFromServer(hostname, port, msgBuf);
//     sleep(1);
    
//     return 0;
// }
