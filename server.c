/* server.c
 * Author: Sowmiya Narayan Srinath
 * This file implements a multithreaded server. Two separate sockets
 * are created for the time and echo server and on accept, the
 * corresponding thread is spawned.
 */

#include	"unp.h"
#include    <thread.h>
#include	<time.h>
#include	<stdio.h>

#define ECHOPORT 5108
#define TIMEPORT 6108

static void* timesrv_thread(void *);
static void* echosrv_thread(void *);

int main(int argc, char **argv)
{
  int					listenfd, *connfdptr, listenfd_e, *connfdptr_e;
  int                   maxfd, s, flags, flags_e;
  thread_t              tid;
  struct sockaddr_in	servaddr, servaddr_e;
  const int             on = 1; //For some reason setsockopt doesnt accept NULL and needs this.
  fd_set                rset;

  FD_ZERO(&rset);

  listenfd = Socket(AF_INET, SOCK_STREAM, 0);
  listenfd_e = Socket(AF_INET, SOCK_STREAM, 0);

  fprintf(stderr,"Server started\n");

  bzero(&servaddr_e, sizeof(servaddr_e));
  servaddr_e.sin_family      = AF_INET;
  servaddr_e.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr_e.sin_port        = htons(ECHOPORT);	/* echo server */

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port        = htons(TIMEPORT);	/* daytime server */

  /* Enable socket to reuse address and avoid bind errors */
  if ( (setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) < 0 ))
    fprintf(stderr,"setsockopt() failed ,errno :%s",strerror(errno));
  if ( (setsockopt(listenfd_e,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) < 0 ))
    fprintf(stderr,"setsockopt() failed ,errno :%s",strerror(errno));

  /* Make sockets non blocking */
  if( (flags = fcntl(listenfd,F_GETFL,0)) < 0 )
    fprintf(stderr,"F_GETFL error");
  if( (fcntl(listenfd,F_SETFL,flags | O_NONBLOCK)) < 0 )
    fprintf(stderr,"F_SETFL error");

  if( (flags_e = fcntl(listenfd_e,F_GETFL,0)) < 0 )
    fprintf(stderr,"F_GETFL error");
  if( (fcntl(listenfd_e,F_SETFL,flags | O_NONBLOCK)) < 0 )
    fprintf(stderr,"F_SETFL error");

  Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
  Bind(listenfd_e, (SA *) &servaddr_e, sizeof(servaddr_e));

  Listen(listenfd, LISTENQ);
  Listen(listenfd_e, LISTENQ);

  while(1)
  {
    connfdptr = malloc(sizeof(int));
    connfdptr_e = malloc(sizeof(int));

    FD_SET(listenfd,&rset);
    FD_SET(listenfd_e,&rset);
    maxfd = max(listenfd,listenfd_e) +1;

    if((s=select(maxfd,&rset,NULL,NULL,NULL)) < 0)
    {
      if(errno == EINTR)
        continue;
      else
      {
        fprintf(stderr,"Select error in server,errno:%s.\n",strerror(errno));
        exit(-1);
      }
    }

    if(FD_ISSET(listenfd,&rset))
    {
      if( (*connfdptr = accept(listenfd, (SA *) NULL, NULL)) < 0)
      {
        if(errno==EINTR || errno==ECONNABORTED )
          continue;
        else
        {
          fprintf(stderr,"Accept failed,errno:%s",strerror(errno));
          return(-1);
        }
      }
      //set the connection socket to blocking mode
      if( (fcntl(*connfdptr,F_SETFL,flags)) < 0 )
        fprintf(stderr,"F_SETFL error");

      pthread_create(&tid,NULL,&timesrv_thread,connfdptr);
    }

    if(FD_ISSET(listenfd_e,&rset))
    {
      if( (*connfdptr_e = accept(listenfd_e, (SA *) NULL, NULL)) < 0)
      {
        if(errno==EINTR || errno==ECONNABORTED )
          continue;
        else
        {
          fprintf(stderr,"Accept failed,errno:%s",strerror(errno));
          return(-1);
        }
      }
      //set the connection socket to blocking mode
      if( (fcntl(*connfdptr_e,F_SETFL,flags_e)) < 0 )
        fprintf(stderr,"F_SETFL error");

      pthread_create(&tid,NULL,&echosrv_thread,connfdptr_e);
    }
  }
}

/*
 * This function implements the time server thread.
 * Using the timeout value in select() to send the time
 * once in 5 sec.Otherwise based on Steven's daytime server.
 */

static void* timesrv_thread (void *arg)
{
  int                 connfd,s,nread;
  fd_set              rset;
  struct timeval      timeout;
  char				  buff[MAXLINE],readbuf[MAXLINE];
  time_t			  ticks;

  connfd = *( (int *) arg);
  free(arg);
  pthread_detach(pthread_self());

  fprintf(stderr,"Time client connected\n");
  FD_ZERO(&rset);

  ticks = time(NULL);
  snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
  Write(connfd, buff, strlen(buff));
  for(;;)
  {
    FD_SET(connfd,&rset);
    timeout.tv_sec=5;
    timeout.tv_usec=0;
    if( (s = select((connfd+1),&rset,NULL,NULL,&timeout)) < 0 )
    {
      if(errno==EINTR)
        continue;
      else
      {
        fprintf(stderr,"Error in select,errno: %s.\n",strerror(errno));
        return; 
      }
    }
    if(FD_ISSET(connfd,&rset)) //socket read end is open
    {
      if( (nread = read(connfd,readbuf,MAXLINE)) <0 )
      {
        if(errno == EINTR)
          continue;
        fprintf(stderr,"Time client disconnected, socket read returned -1.Errno:%s.\n",strerror(errno));
        return; //Back to accept and allow new clients to connect
      }
      else if(nread == 0)
      {
        fprintf(stderr,"Time client disconnected, socket read returned with 0.\n");
        return;
      }
    }
    else  //Timeout happened on select,so write the current time to socket
    {
      ticks = time(NULL);
      snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
      Write(connfd, buff, strlen(buff));
    }
  }
  Close(connfd);
}


/* 
 * This thread implements the simple echo server .
 * Based on Stevens str_echo function.
 */

static void* echosrv_thread (void *arg)
{
  int connfd;
  ssize_t     n;
  char        buf[MAXLINE];

  connfd = *( (int *) arg);
  free(arg);
  pthread_detach(pthread_self());
  fprintf(stderr,"Echo client connected\n");

again:
  while ( (n = read(connfd, buf, MAXLINE)) > 0)
    Writen(connfd, buf, n);

  if (n < 0 && errno == EINTR)
    goto again;
  else if (n < 0)
    fprintf(stderr,"Echo client disconnected, socket read returned -1.Errno:%s.\n",strerror(errno));
  else if(n == 0)
    fprintf(stderr,"Echo client disconnected, socket read returned with 0.\n");

  Close(connfd);
}
