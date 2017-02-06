#ifndef COMMON_H

#define COMMON_H

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/************************************************************
 All "messages" sent between server and clients in the
 distributed tic-tac-toe system are in the format defined
 by struct tttmsg. The structure of the message is determined
 by the type field:

 type=0   WHO              no other fields used
 type=1   HANDLE           data is the string handle for ttt
                            sending this message
 type=2   MATCH            data is string handle of opponent;
                            board[0] is 'X' or 'O' to denote
                            character this ttt is using
 type=3   WHATMOVE         board[] contains X/O/space chars
                            to denote current state
 type=4   MOVE             number indicates square into which
                            client is moving
 type=5   RESULT           board[] contains X/O/space chars
                            to denote current state;
                            number= 0 -> you win
                                    1 -> you lose
                                    2 -> draw
**************my**********************************************/

#define WHO 0
#define HANDLE 1
#define MATCH 2
#define WHATMOVE 3
#define MOVE 4
#define RESULT 5

struct tttmsg{
  int type;      /* Message type */
  char board[9]; /* X/O */
  char data[32]; /* null-terminated string */
  int number;    /* integer data */
};

#define SFILE "./serverloc"

void putmsg(int, struct tttmsg *);
void getmsg(int, struct tttmsg *);
void protocol_error(int, struct tttmsg *);
void dumpmsg(struct tttmsg *);

#endif