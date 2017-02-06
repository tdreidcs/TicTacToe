/************************************************************************
 			     TTT.c       			         
 Castrated TTT server. No datagram queries, no timeouts. Assumes
 completely well-behaved clients.

 Phil Kearns							         
 (for Systems Programming)
 								         
*************************************************************************/

#include "common.h"

void init_board();
void dump_board();
int check_board(int);

char board[9]; /* The tic-tac-toe board */

int main(argc,argv)
int argc; char **argv;
{
  int listener;  /* fd for socket on which we get connection requests */
  int Xfd, Ofd;  /* fds for sockets onto VCs to clients */
  struct sockaddr_in s1, Xaddr, Oaddr;
  int sfile, i, moveto, result, Xresult, Oresult;
  socklen_t length;
  unsigned short lport;
  int currentmove;  /* 0->X, 1->O */
  char hostid[128], Xhandle[32], Ohandle[32];
  struct tttmsg outmsg, inmsg;

  if (argc != 1) {
    fprintf(stderr,"TTT:usage is TTT\n");
    exit(1);
  }

  if ( (sfile = open(SFILE, O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0) {
    perror("TTT:sfile");
    exit(1);
  }

  if (gethostname(hostid,128) < 0){
    perror("TTT:gethostname");
        exit(1);
  }

  i=0;
  while (hostid[i] != '\0')
    write(sfile,&hostid[i++], 1);
  write(sfile,"\0", 1);

  if ( (listener = socket( AF_INET, SOCK_STREAM, 0 )) < 0 ) {
    perror("TTT:socket");
    exit(1);
  }

  bzero((char *) &s1, sizeof(s1));
  s1.sin_family = AF_INET;
  s1.sin_addr.s_addr = INADDR_ANY;  /* Any of this host's interfaces is OK. */
  s1.sin_port = 0;                  /* bind() will gimme unique port. */
  if (bind(listener, (struct sockaddr *)&s1, sizeof(s1)) < 0) {
    perror("TTT:bind");
    exit(1);
  }

  length = sizeof(s1);
  if (getsockname(listener, (struct sockaddr *)&s1, &length) < 0) {
    perror("TTT:getsockname");
    exit(1);
  }
  lport = s1.sin_port;
  write(sfile, &lport, sizeof(unsigned short));
  close(sfile);

  listen(listener,5);

  length = sizeof(Xaddr);
  if ((Xfd = accept(listener, (struct sockaddr *)&Xaddr, &length)) < 0) {
    perror("TTT:accept X");
    exit(1);
  }

  /* Send WHO to X */

  bzero((char *)&outmsg, sizeof(outmsg));
  outmsg.type = htonl(WHO);
  putmsg(Xfd, &outmsg);

  /* Await HANDLE from X */

  bzero((char *)&inmsg, sizeof(inmsg));
  getmsg(Xfd, &inmsg);
  if (ntohl(inmsg.type) != HANDLE) protocol_error(HANDLE, &inmsg);
  strncpy(Xhandle, inmsg.data, 31);
  Xhandle[31] = '\0';

  length = sizeof(Oaddr);
  if ((Ofd = accept(listener, (struct sockaddr *)&Oaddr, &length)) < 0) {
    perror("TTT:accept O");
    exit(1);
  }

  /* Send WHO to O */

  bzero((char *)&outmsg, sizeof(outmsg));
  outmsg.type = htonl(WHO);
  putmsg(Ofd, &outmsg);

  /* Await HANDLE from O */

  bzero((char *)&inmsg, sizeof(inmsg));
  getmsg(Ofd, &inmsg);
  if (ntohl(inmsg.type) != HANDLE) protocol_error(HANDLE, &inmsg);
  strncpy(Ohandle, inmsg.data, 31);
  Ohandle[31] = '\0';

  /* WE HAVE A MATCH */
  //fprintf(stderr, "after WE HAVA MATCH\n" );

  for(i=0; i<9; i++) board[i]=' ';

  /* Send MATCH to X */

  bzero((char *)&outmsg, sizeof(outmsg));
  outmsg.type=htonl(MATCH);
  strncpy(outmsg.data, Ohandle, 31);
  outmsg.data[31] = '\0';
  outmsg.board[0] = 'X';
  putmsg(Xfd, &outmsg);

    //fprintf(stderr, "after SEND MATCH TO X\n" );

  /* Send MATCH to O */    

  bzero((char *)&outmsg, sizeof(outmsg));
  outmsg.type=htonl(MATCH);
  strncpy(outmsg.data, Xhandle, 31);
  outmsg.data[31] = '\0';
  outmsg.board[0] = 'O';
  putmsg(Ofd, &outmsg);

    //fprintf(stderr, "AFTER SEND MATCH TO 0\n" );

  while(1) {
    
    /* Send  WHATMOVE to X */

    bzero((char *)&outmsg, sizeof(outmsg));
    outmsg.type=htonl(WHATMOVE);
    for(i=0; i<9; i++) {outmsg.board[i]=board[i];}
    putmsg(Xfd, &outmsg);

  //fprintf(stderr, "AFTER SEND WHATMOVE XXX\n" );

    /* Await MOVE from X */

    bzero((char *)&inmsg, sizeof(inmsg));  
    getmsg(Xfd, &inmsg);
    if (ntohl(inmsg.type) != MOVE) protocol_error(MOVE, &inmsg);
    currentmove = 0; /* X's move being reported*/
    moveto = ntohl(inmsg.number);
    if (moveto == 99) {currentmove = 1; result = 3; break;}
    if (board[moveto] != ' ') {
      fprintf(stderr, "TTT: INVALID MOVE\n");
      exit(1);
    }
    board[moveto] = 'X';
    //fprintf(stderr, "board point %c\n",board[moveto] );
    if ((result = check_board(currentmove)) != 0) break;

    //fprintf(stderr, "AFTER AWAIT MOVE FROM X\n" );

    /* Send  WHATMOVE to O */

    bzero((char *)&outmsg, sizeof(outmsg));
    outmsg.type=htonl(WHATMOVE);
    for(i=0; i<9; i++) outmsg.board[i]=board[i];
    putmsg(Ofd, &outmsg);

   // fprintf(stderr, "after SENDWHATMOVE TO 00000\n" );

    /* Await MOVE from O */

    bzero((char *)&inmsg, sizeof(inmsg));  
    getmsg(Ofd, &inmsg);
    if (ntohl(inmsg.type) != MOVE) protocol_error(MOVE, &inmsg);
    currentmove = 1; /* O's move being reported*/
    moveto = ntohl(inmsg.number);
    if (moveto == 99) {currentmove = 0; result = 3; break;}
    if (board[moveto] != ' ') {
      fprintf(stderr, "TTT: invalid move\n");
      exit(1);
    }
    board[moveto] = 'O';
    if ((result = check_board(currentmove)) != 0) break;

    //fprintf(stderr, "after AWAIT MOVE from 0000\n" );
  } /* while */

  /* Match is over, report the result to both clients */

  if (result == 2) {
    Xresult = Oresult = 2;
  }
  else if (result == 3) {
    Xresult = (currentmove?0:3);
    Oresult = (currentmove?3:0);
  }
  else { /* result must be 1, we have a winner */
    Xresult = (currentmove?1:0);
    Oresult = (currentmove?0:1);
  }

  /* Send RESULT to X */

  bzero((char *)&outmsg, sizeof(outmsg));
  outmsg.type = htonl(RESULT);
  outmsg.number = htonl(Xresult);
  for(i=0; i<9; i++) outmsg.board[i]=board[i];
  putmsg(Xfd, &outmsg);

  /* Send RESULT to O */

  bzero((char *)&outmsg, sizeof(outmsg));
  outmsg.type = htonl(RESULT);
  outmsg.number = htonl(Oresult);
  for(i=0; i<9; i++) outmsg.board[i]=board[i];
  putmsg(Ofd, &outmsg);

  /* The game is finished */
  return 0; 
}


/************************************************************************
   check_board() returns 0 if no winner
                         1 if winner
                         2 if draw
   Brute force.
************************************************************************/
int
check_board(player)
int player; /* X is 0, O is 1 */
{
  char match;
  int i,spaces;

  match = (player?'O':'X');

  if (((board[0] == match) && (board[1] == match) && (board[2] == match)) ||
     ((board[3] == match) && (board[4] == match) && (board[5] == match)) ||
     ((board[6] == match) && (board[7] == match) && (board[8] == match)) ||
     ((board[0] == match) && (board[3] == match) && (board[6] == match)) ||
     ((board[1] == match) && (board[4] == match) && (board[7] == match)) ||
     ((board[2] == match) && (board[5] == match) && (board[8] == match)) ||
     ((board[0] == match) && (board[4] == match) && (board[8] == match)) ||
     ((board[2] == match) && (board[4] == match) && (board[6] == match))) return 1;
  spaces = 0;
  for(i=0; i<9; i++)
    if (board[i] == ' ') {
      spaces++;
      break;
    }
  if (!spaces) return 2;
  return 0;
}

void
dump_board(s,board)
FILE *s;
char board[];
{
  fprintf(s,"%c | %c | %c\n", board[0], board[1], board[2]);
  fprintf(s,"----------\n");
  fprintf(s,"%c | %c | %c\n", board[3], board[4], board[5]);
  fprintf(s,"----------\n");
  fprintf(s,"%c | %c | %c\n", board[6], board[7], board[8]);
}
