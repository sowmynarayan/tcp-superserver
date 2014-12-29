/*
 * client.c
 * Author: Sowmiya Narayan Srinath
 * Main file for the client side. Takes ip addr/hostname as command line arg,
 * takes user choice of client (time/echo) and launches the corresponding
 * client.
 */

#include  "unp.h"
#include  <stdbool.h>
#define SIZE 1024

void sig_chld(int);
int forkanddostuff(char,char*);

int main(int argc, char **argv)
{
  char choice,ipaddr[16];
  bool quit = false;
  struct in_addr addr;
  struct hostent *host; //This structure is needed for the gethost* functions

  if(argc != 2)
  {
    fprintf(stderr,"Usage: ./client <IP address>|<hostname> \n");    
    return(-1);
  }

  if(inet_addr(argv[1]) != -1)  //Entered valid IP address
  {
    if( inet_pton(AF_INET,argv[1],&addr) == NULL )
    {  
      fprintf(stderr,"inet_pton error!Invalid IP. Usage: ./client <IP address>|<hostname> \n");
      return(-2);
    }

    if( (host = gethostbyaddr(&addr,sizeof(addr),AF_INET) )== NULL)
    {
      fprintf(stderr,"Could not connect to ip address!\nUsage: ./client <ip address>|<hostname>\n");
      return(-1);
    }

    fprintf(stderr,"You are connecting to the server %s\n",host->h_name);
    strcpy(ipaddr,argv[1]);
  }
  else
  {
    /* user entered domain name (or invalid name) */

    if( (host=gethostbyname(argv[1])) == NULL )
    {
      fprintf(stderr,"Host name was not resolved!\nUsage: ./client <ip address>|<hostname>\n");
      return(-1);
    }

    if( inet_ntop(AF_INET,*(host->h_addr_list),ipaddr,sizeof(ipaddr)) == NULL )
    {  
      fprintf(stderr,"inet_ntop error!");
      return(-2);
    }
    fprintf(stderr,"Connecting to %s\n",ipaddr);
  }

  do
  {
    fprintf(stderr,"\n"
        "[Tt]ime service\n[Ee]cho service\n"
        "[Qq]uit\nEnter your choice:");
    fscanf(stdin," %c",&choice);

    switch(choice)
    {
      case 'T':
      case 't':
      case 'E':
      case 'e':
        fprintf(stderr,"Starting service...\n");
        Signal(SIGCHLD, sig_chld);
    	if(forkanddostuff(choice,ipaddr) < 0)
        {
            fprintf(stderr,"Client side error!");
            return(-3);
        }
        break;

      case 'Q':
      case 'q':
        fprintf(stderr,"Quitting,bye!\n");
        quit = true;
        break;

      default:
        fprintf(stderr,"Invalid choice! Enter again\n");
    }

  }while(!quit);

  return 0;
}

/* This function handles the actual fork and exec.
 * Launch the client of user's choice from child proc
 * and run parent in a loop to monitor child status.
 */

int forkanddostuff(char choice,char *ipaddr)
{
  int pfd[2];
  int pid,nread,ret,maxfd,s;
  char buf[SIZE],pipearg[SIZE];
  fd_set rset;
  FD_ZERO(&rset);

  if (pipe(pfd) == -1)
  {
    fprintf(stderr,"Pipe creation failed\n");
    return(-1);
  }
  if ((pid = fork()) < 0)
  {
    fprintf(stderr,"Fork failed\n");
    return(-1);
  }

  if (pid == 0)
  {
    /* child */
    close(pfd[0]);
    if(choice == 'e' || choice == 'E')
    {
        sprintf(pipearg,"%d",pfd[1]);
        if( (ret=execlp("xterm","xterm","-e","./echocli",ipaddr,pipearg,(char *)0)) < 0)
            fprintf(stderr,"Exec failed %d with error %s",ret,strerror(errno));
        strcpy(buf,"Echo client ended!");
        write(pfd[1],buf,strlen(buf)+1);
    }
    else if(choice == 't' || choice =='T')
    {
        sprintf(pipearg,"%d",pfd[1]);
        if( (ret=execlp("xterm","xterm","-e","./timecli",ipaddr,pipearg,(char *)0)) < 0)
            fprintf(stderr,"Exec failed %d with error %s",ret,strerror(errno));
        strcpy(buf,"Time client ended!");
        write(pfd[1],buf,strlen(buf)+1);
    }
    else
    {
        fprintf(stderr,"You should never be here!\n");
        return(-3);
    }

    close(pfd[1]);
    exit(0);  //Terminate child process

  } 

  else          //Parent process
  {
    close(pfd[1]);

    for(;;)
    {
        FD_SET(pfd[0],&rset);
        FD_SET(fileno(stdin),&rset);
        maxfd = max(pfd[0],fileno(stdin)) + 1;
        if( (s = select(maxfd,&rset,NULL,NULL,NULL)) <0)
        {
            printf("select is <0\n");
            if(errno == EINTR)
                continue;
            else
            {
                fprintf(stderr,"Error in select!\n");
                return(-2);
            }
        }
        
        if(FD_ISSET(pfd[0],&rset)) // pipe read end is open
        {
           if( (nread = read(pfd[0], buf, SIZE)) < 0)
           {
                if(errno == EINTR)
                    break;
                fprintf(stderr,"Read error\n");
           }
           else if(nread == 0)
              break;
           else
           {
                fprintf(stderr,"Status from child:%s\n",buf);
           }
        }

       if(FD_ISSET(fileno(stdin),&rset)) //user input at stdin
       {
           if((nread = read(fileno(stdin),buf,SIZE)) <= 0)
                break;
                //fprintf(stderr,"Read error!\n");
           else
                fprintf(stderr,"Client is running!Please type in the xterm window!\n");
       }
    }

    close(pfd[0]);
  }

  return(1);
}

/*
 * Simple signal handler pasted from Stevens to handle zombie process.
 * Not good to have i/o operation in signal handler,so removed printf.
 */

void sig_chld(int signal)
{
  pid_t pid;
  int stat;
  while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
    ;
 //   printf("child %d terminated\n", pid);
  return;
}
