/* Echocli.c
 * Author: Sowmiya Narayan Srinath
 * This file implements the Echo client which is execed from client.c
 */

#include "unp.h"

#define SIZE 1024
#define ECHOPORT 5108 //Ephemeral port of my choice
#define CONN_TIMEOUT 10 // Timeout for connect()

void doEcho(FILE*, int, int);
int connect_timeout(struct sockaddr_in, int ,int);

int main(int argc, char **argv)
{
  int sockfd, n,err,pfd;
  char recvline[MAXLINE + 1], buf[SIZE], ipaddr[16];
  struct sockaddr_in servaddr;

  if (argc != 3)
  {
        fprintf(stderr,"Dont run echocli directly! Invoke from ./client\n");
        exit(0);
  }

  pfd = atoi(argv[2]);
  strcpy(ipaddr,argv[1]);

  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    /* Since printf msgs wont be seen on the xterm,writing error status on the pipe fd  */
    strcpy(buf,"Socket creation error in Echo client");
    write(pfd,buf,strlen(buf)+1);
    return(-1);
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port   = htons(ECHOPORT);  
  if ((err = inet_pton(AF_INET, ipaddr , &servaddr.sin_addr)) <= 0)
  {
    sprintf(buf,"inet_pton error %d",err);
    write(pfd,buf,strlen(buf)+1);
    return(-1);
  }

  //if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
  if(connect_timeout(servaddr, sockfd, CONN_TIMEOUT) < 0)  //Connection timeout of 10 sec
  {
     sprintf(buf,"Connect error in client,server probably not running.Errno: %s",strerror(errno));
     write(pfd,buf,strlen(buf)+1);
     return(-1);
  }

  fprintf(stderr,"Echo client started.\nEnter some text: ");
  doEcho(stdin,sockfd,pfd);

  strcpy(buf,"Server down! Echo client quitting");
  write(pfd,buf,strlen(buf)+1);
  return(1); 
}

/*
 * This function implements the actual i/o of the echo client.
 * Based on Stevens strcliselect01
 */
void doEcho(FILE *fp, int sockfd, int pfd)
{
    int         maxfdp1;
    fd_set      rset;
    char        sendline[MAXLINE], recvline[MAXLINE], buf[SIZE];

    FD_ZERO(&rset);
    for ( ; ; ) {
        FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdp1 = max(fileno(fp), sockfd) + 1;
        Select(maxfdp1, &rset, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &rset)) {  /* socket is readable */
            if (Readline(sockfd, recvline, MAXLINE) == 0)
            {
              strcpy(buf,"Server down! Echo client quitting");
              write(pfd,buf,strlen(buf)+1);
              exit(-1);
            }
            fprintf(stderr,"Server says: ");
            Fputs(recvline, stderr);
            fprintf(stderr,"\n");
            fprintf(stderr,"Enter some text: ");
        }

        if (FD_ISSET(fileno(fp), &rset)) 
        {  /* input is readable */
          if (Fgets(sendline, MAXLINE, fp) == NULL)
            return;     /* all done */
          Writen(sockfd, sendline, strlen(sendline));
        }
    }
}

/*
 * This function is a wrapper for the connect() function.The 
 * default timeout for connect is too high and hence using this
 * function.This is a non blocking version.
 */
int connect_timeout(struct sockaddr_in sa, int sock, int timeout)
{   
  int flags = 0, error = 0, ret = 0;
  fd_set  rset, wset;
  socklen_t   len = sizeof(error);
  struct timeval  ts;

  ts.tv_sec = timeout;
  ts.tv_usec = 0;

  FD_ZERO(&rset);
  FD_SET(sock, &rset);
  wset = rset;    

  if( (flags = fcntl(sock, F_GETFL, 0)) < 0)
    return -1;

  if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
    return -1;

  if( (ret = connect(sock, (struct sockaddr *)&sa, 16)) < 0 )
    if (errno != EINPROGRESS)
      return -1;

  if(ret == 0)    
    goto done;

  if( (ret = select(sock + 1, &rset, &wset, NULL, (timeout) ? &ts : NULL)) < 0)
    return -1;
  if(ret == 0){   
    errno = ETIMEDOUT;
    return -1;
  }

  //we had a positive return so a descriptor is ready
  if (FD_ISSET(sock, &rset) || FD_ISSET(sock, &wset)){
    if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
      return -1;
  }else
    return -1;

  if(error){  //check if we had a socket error
    errno = error;
    return -1;
  }

done:
  //put socket back in blocking mode
  if(fcntl(sock, F_SETFL, flags) < 0)
    return -1;

  return 0;
}
