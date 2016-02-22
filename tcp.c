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
#include <fcntl.h>
#include <errno.h>
#include "tcp.h" 

void error(char *msg) {
    perror(msg);
    exit(0);
}

int initServer(int parentfd, int portno)
{
    int optval;
    parentfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serveraddr; /* server's addr */
  if (parentfd < 0) 
    error("ERROR opening socket");

  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
       (const void *)&optval , sizeof(int));

  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  if (bind(parentfd, (struct sockaddr *) &serveraddr, 
     sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  if (listen(parentfd, 2) < 0)
    error("ERROR on listen");
    
  return parentfd;
}

int acceptClient(int childfd, int parentfd, char* buf)
{ 
    unsigned int clientlen;
    struct sockaddr_in clientaddr;
    
    clientlen = sizeof(clientaddr);
    childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (childfd < 0) 
      error("ERROR on accept");
    
    printf("Connected to client...\n");
    return childfd;
}

int sendMessageToServer(char* hostname, int port, char* buf)
{
    int sockfd, n, success;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    int resp = 0;// 1 for time reqest, 7 for cache request, 10 for lastupdate request
    if ((buf[0] == '1') && (buf[1] == '0'))
    {
        printf("Sending LastUpdate request...\n");
        resp = 10;
    }
    else if (buf[0] == '1')
    {
        printf("Sending time request...\n");
        resp = 1;
    }
    else if (buf[0] == '7')
    {
        printf("Sending cache request...\n");
        resp = 7;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
      (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port);

    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    { 
        success = 0;
        if (errno == EINPROGRESS)
        {
            struct timeval tv;
            tv.tv_usec = 0;
            tv.tv_sec = TIMEOUTSEC;

            fd_set rfds;

            FD_ZERO(&rfds);
            FD_SET(sockfd, &rfds);
            if (select(sockfd + 1, NULL, &rfds, NULL, &tv) > 0)
            {
                int so_error;
                socklen_t len = sizeof(int);

                getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);

                if (so_error == 0) {
                    success = 1;
                }
            }
            else { 
               printf("Connection timeout\n");
            } 
        }
    }
    else
    {
        success = 1;
    }
    if (success)
    {
      int arg;
      arg = fcntl(sockfd, F_GETFL, NULL); 
      arg &= (~O_NONBLOCK); 
      fcntl(sockfd, F_SETFL, arg); 
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

      if (resp == 1)
      {
          //save time response in buf
          bzero(buf, BUFSIZE);
          sleep(1);
          char foo[2];
          foo[1] = '\0';
          int i = 0;
          printf("Awaiting database time...\n");
          while (foo[0] != (char)-128)
          {
              foo[0] = '\0';
              while (foo[0] != '\n' && foo[0] != (char)-128)
              {
                 n = read(sockfd, foo, 1);
                 if (n < 0) 
                 error("ERROR reading from socket");
                 printf("%c", foo[0]);
                 buf[i] = foo[0];
                 i++;
                 fflush(stdout);

              }

          }
      }
      else if (resp == 10)
      {
          //save timestamp response in buf
          bzero(buf, BUFSIZE);
          sleep(1);
          char foo[2];
          foo[1] = '\0';
          int i = 0;
          int start = 0;
          printf("Awaiting database timestamp...\n");
          while (foo[0] != (char)-128)
          {
              foo[0] = '\0';
              while (foo[0] != '\n' && foo[0] != (char)-128)
              {
                 n = read(sockfd, foo, 1);
                 if (n < 0) 
                 error("ERROR reading from socket");
                 printf("%c", foo[0]);
                 if(foo[0] != '~')
                  start = 1;
                 if(foo[0] != (char)-128 && start){
                    buf[i] = foo[0];
                    i++;
                    fflush(stdout);
                  }

              }

          }
      }
      else if (resp == 7)
      {
        //write cache response to file
         printf("Awaiting database cache...\n");
         FILE *fw;
         fw = fopen(CACHEFILE, "w");
         char foo[2];
         foo[1] = '\0';
         int start = 0;
         while (foo[0] != (char)-128)
         {
             foo[0] = '\0';
             while (foo[0] != '\n' && foo[0] != (char)-128)
             {
                n = read(sockfd, foo, 1);
                if (n < 0) 
                 error("ERROR reading from socket");
                if(foo[0] == '<')
                  start = 1;
                if(foo[0] != (char)-128 && start)
                    fputc(foo[0], fw);
                    fflush(stdout);

             }

         }
         fclose(fw);
      }
      
    }
    close(sockfd);
    return success;
}

char* createBalanceUpdate(int cid, float diff, char* msgParam)
{
    printf("Creating balance update...");
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
    printf("Message to database: %s\n", msgParam);

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

char* createGetEntryAll(char* msgParam)
{
    //cmd = 7
    msgParam[0] = '7';
    msgParam[1] = '\0';
    printf("Message: %s\n", msgParam);
    
    return msgParam;
}

char* createGetTime(char* msgParam)
{
    //cmd = 7
    msgParam[0] = '1';
    msgParam[1] = '\0';
    printf("Message: %s\n", msgParam);
    
    return msgParam;
}

char* createGetLastUpdate(int cid, char* msgParam)
{
    char cidBuf[BUFSIZE];
    
    snprintf(cidBuf, BUFSIZE, "%d", cid);
    
    //cmd + ' ' + cid
    int i = 1 + 1 + 1 + strlen(cidBuf);
    msgParam[0] = '1';
    msgParam[1] = '0';
    msgParam[2] = ' ';
    strncpy(msgParam + 3, cidBuf, strlen(cidBuf));
    msgParam[i] = '\0';
    printf("Message: %s\n", msgParam);
    
    return msgParam;
}