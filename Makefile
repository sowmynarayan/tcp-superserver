# This is a sample Makefile which compiles source files named:
# - server.c
# - client.c
# - time_cli.c
# - echo_cli.c
# and creating executables: "server", "client", "time_cli"
# and "echo_cli", respectively.
#
# It uses various standard libraries, and the copy of Stevens'
# library "libunp.a" in ~cse533/Stevens/unpv13e_solaris2.10 .
#
# It also picks up the thread-safe version of "readline.c"
# from Stevens' directory "threads" and uses it when building
# the executable "server".
#
# It is set up, for illustrative purposes, to enable you to use
# the Stevens code in the ~cse533/Stevens/unpv13e_solaris2.10/lib
# subdirectory (where, for example, the file "unp.h" is located)
# without your needing to maintain your own, local copies of that
# code, and without your needing to include such code in the
# submissions of your assignments.
#
# Modify it as needed, and include it with your submission.

CC = gcc

LIBS = -lresolv -lsocket -lnsl -lpthread\
	/home/courses/cse533/Stevens/unpv13e_solaris2.10/libunp.a\
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib

all: client server timecli echocli


# server uses the thread-safe version of readline.c

server: server.o readline.o
	${CC} ${FLAGS} -o server server.o readline.o ${LIBS}
server.o: server.c
	${CC} ${CFLAGS} -c server.c


client: client.o
	${CC} ${FLAGS} -o client client.o ${LIBS}
client.o: client.c
	${CC} ${CFLAGS} -c client.c


timecli: timecli.o
	${CC} ${FLAGS} -o timecli timecli.o ${LIBS}
timecli.o: timecli.c
	${CC} ${CFLAGS} -c timecli.c


echocli: echocli.o
	${CC} ${FLAGS} -o echocli echocli.o ${LIBS}
echocli.o: echocli.c
	${CC} ${CFLAGS} -c echocli.c

# pick up the thread-safe version of readline.c from directory "threads"

readline.o: /home/courses/cse533/Stevens/unpv13e_solaris2.10/threads/readline.c
	${CC} ${CFLAGS} -c /home/courses/cse533/Stevens/unpv13e_solaris2.10/threads/readline.c


clean:
	rm  server server.o client client.o timecli timecli.o echocli echocli.o readline.o
