/* timecli.c
 * Author: Sowmiya Narayan Srinath
 * This file implements the time client which is execed from client.c
 */

#include "unp.h"

#define SIZE 1024
#define TIMEPORT 6108 //Ephemeral port of my choice
#define CONN_TIMEOUT 10 //Timeout value for connect()

int main(int argc, char **argv)
{
  int sockfd, n,err,pfd;
  char recvline[MAXLINE + 1], buf[SIZE], ipaddr[16];
  struct sockaddr_in servaddr;

  if (argc != 3)
  {
        fprintf(stderr,"Dont run timecli directly! Invoke from ./client\n");
        exit(0);
  }

  pfd = atoi(argv[2]);
  strcpy(ipaddr,argv[1]);

  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    /* Since printf msgs wont be seen on the xterm,writing error status on the pipe fd  */
    strcpy(buf,"Socket creation error in time client");
    write(pfd,buf,strlen(buf)+1);
    return(-1);
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port   = htons(TIMEPORT);  /* daytime server */
  if ((err = inet_pton(AF_INET, ipaddr , &servaddr.sin_addr)) <= 0)
  {
    sprintf(buf,"inet_pton error %d",err);
    write(pfd,buf,strlen(buf)+1);
    return(-1);
  }

  //if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
  if (connect_timeout(servaddr, sockfd, CONN_TIMEOUT) < 0)
  {
     sprintf(buf,"Connect error in client,server probably not running.Errno:%s",strerror(errno));
     write(pfd,buf,strlen(buf)+1);
     return(-1);
  }

  fprintf(stderr,"Time client started.\nFetching time from server...\n");
  while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
    recvline[n] = 0;  /* null terminate */
    if (fputs(recvline, stderr) == EOF)
    {
        sprintf(buf,"Error in fputs");
        write(pfd,buf,strlen(buf)+1);
        return(-1);
    }
  }
  /* if (n < 0)
  {
    sprintf(buf,"Read error");
    write(pfd,buf,strlen(buf)+1);
    return(-1);
  } */

  strcpy(buf,"Server down! Time client quitting");
  write(pfd,buf,strlen(buf)+1);
  return(1); 
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

/*
 * Unused dummy function.Make this main to test the basic
 * xterm and pipe functionality.
 */

int dummy(int argc, char **argv)
{
    int pfd = atoi(argv[2]);
    char buf[100];
    char ipaddr[16];
    strcpy(ipaddr,argv[1]);
    fprintf(stderr,"ipaddr=%s\n",ipaddr);
    sleep(15);
    strcpy(buf,"hello from timecli");
    write(pfd,buf,strlen(buf)+1);
    return(1);
}
