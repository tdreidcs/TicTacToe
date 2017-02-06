/************************************************************************
			 ttt.c
Simple ttt client. No queries, no timeouts.

Phil Kearns
(for Systems Programming)

************************************************************************/

#include "common.h"
#include <assert.h>
#include "child.h"

void dump_board (FILE *, char *);
char *concat(char *str_1, char *str_2);

int
main (argc, argv)
     int argc;
     char **argv;
{
  char hostid[128], handle[32], opphandle[32]; //junk;
  char my_symbol;		/* X or O ... specified by server in MATCH message */
  char board[9], oldboard[9];
  unsigned short xrport;
  int sock, sfile;
  struct sockaddr_in remote;
  struct hostent *h;
  int num, i, valid = 0; //move, valid, finished;
  struct tttmsg inmsg, outmsg;

  if (argc != 1)
    {
      fprintf (stderr, "ttt:usage is ttt\n");
      exit (1);
    }

  /* Get host,port of server from file. */

  if ((sfile = open (SFILE, O_RDONLY)) < 0)
    {
      perror ("TTT:sfile");
      exit (1);
    }
  i = 0;
  while (1)
    {
      num = read (sfile, &hostid[i], 1);
      if (num == 1)
	{
	  if (hostid[i] == '\0')
	    break;
	  else
	    i++;
	}
      else
	{
	  fprintf (stderr, "ttt:error reading hostname\n");
	  exit (1);
	}
    }
  if (read (sfile, &xrport, sizeof (int)) != sizeof (unsigned short))
    {
      fprintf (stderr, "ttt:error reading port\n");
      exit (1);
    }
  close (sfile);

  /* Got the info. Connect. */

  if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("ttt:socket");
      exit (1);
    }

  bzero ((char *) &remote, sizeof (remote));
  remote.sin_family = AF_INET;
  if ((h = gethostbyname (hostid)) == NULL)
    {
      perror ("ttt:gethostbyname");
      exit (1);
    }
  bcopy ((char *) h->h_addr, (char *) &remote.sin_addr, h->h_length);
  remote.sin_port = xrport;
  if (connect (sock, (struct sockaddr *) &remote, sizeof (remote)) < 0)
    {
      perror ("ttt:connect");
      exit (1);
    }

  /* We're connected to the server. Engage in the prescribed dialog */

  /* Await WHO */

  bzero ((char *) &inmsg, sizeof (inmsg));
  getmsg (sock, &inmsg);
  if (ntohl (inmsg.type) != WHO)
    protocol_error (WHO, &inmsg);

  /* Send HANDLE */

  printf ("Enter handle (31 char max):");
  fgets (handle, 31, stdin);
  bzero ((char *) &outmsg, sizeof (outmsg));
  outmsg.type = htonl (HANDLE);
  strncpy (outmsg.data, handle, 31);
  outmsg.data[31] = '\0';
  putmsg (sock, &outmsg);

  FILE *read_from, *write_to;
  char result[80];
  int childpid, turn = 0;
  char *myhandle = handle;
  myhandle = strtok(myhandle, "\n");

  childpid = start_child("wish", &read_from, &write_to);
  fprintf (write_to, "source ttt.tcl\n");
  fprintf(write_to, ".g create text 190 20 -anchor nw -text \"Awaiting Match\" -tag wait\n");

  /* Await MATCH */

  bzero ((char *) &inmsg, sizeof (inmsg));
  getmsg (sock, &inmsg);
  if (ntohl (inmsg.type) != MATCH)
    protocol_error (MATCH, &inmsg);
  my_symbol = inmsg.board[0];
  //fprintf(stderr, "this is my symbol %c\n", my_symbol);
  strncpy (opphandle, inmsg.data, 31);
  opphandle[31] = '\0';

  void checkWin(FILE *write_to, char *board) {
    if (board[0] != ' ' && board[0] == board[1] && board[0] == board[2])
      fprintf(write_to, "WinCondition top\n");
    if (board[3] != ' ' && board[3] == board[4] && board[3] == board[5])
      fprintf(write_to, "WinCondition mid\n");
    if (board[6] != ' ' && board[6] == board[7] && board[6] == board[8])
      fprintf(write_to, "WinCondition bot\n");
    if (board[0] != ' ' && board[0] == board[4] && board[0] == board[8])
      fprintf(write_to, "WinCondition dia\n");

    if (board[0] != ' ' && board[0] == board[3] && board[0] == board[6])
      fprintf(write_to, "WinCondition lef\n");
    if (board[1] != ' ' && board[1] == board[4] && board[1] == board[7])
      fprintf(write_to, "WinCondition cen\n");
    if (board[2] != ' ' && board[2] == board[5] && board[2] == board[8])
      fprintf(write_to, "WinCondition rig\n");
    if (board[2] != ' ' && board[2] == board[4] && board[2] == board[6])
      fprintf(write_to, "WinCondition rev\n");
  }

  void runResignation () {
    int turn = 99;
    bzero ((char *) &outmsg, sizeof (outmsg));
    outmsg.type = htonl (MOVE);
    outmsg.number = htonl (turn);
    putmsg (sock, &outmsg);
  }

  void map_board(FILE *read_from, FILE *write_to, char*
    board, int turn) {
    int i; //char str_mark[14];

    //fprintf(stderr, "check itclown |%s|\n", board);
    for (i = 0; i < 9; i++) {
      if (board[i] == 'U')
        board[i] = ' ';
    }

    //fprintf(stderr, "check itclown |%s|\n", board);
    for (i = 0; i < 9; i++) {

      if (board[i] == 'O') {
        fprintf(write_to, "DrawMark %d %c\n", i, board[i]);
        //fprintf(stdout, "Drew Mark %d %c\n", i, board[i]);
      }
      if (board[i] == 'X') {
        fprintf(write_to, "DrawMark %d %c\n", i, board[i]);
      }
      //sprintf(str_mark, "DrawMark %d %c\n", i, board[i]);
      //fprintf(stderr, "%s\n", str_mark);
    }
    checkWin(write_to, board);

    bzero ((char *) &outmsg, sizeof (outmsg));
    outmsg.type = htonl (MOVE);
    outmsg.number = htonl (turn);
    putmsg (sock, &outmsg);
    //fprintf(stderr, "%d\n", turn);
    fprintf(write_to, "YourMove off\n");
  }

  void receive_board(FILE *write_to, char* board, char opp_symbol) {

    //fprintf(write_to, "SetSymbol %c\n", opp_symbol);
    for (i = 0; i < 9; i++) {
      if (board[i] == 'O') {
        fprintf(write_to, "DrawMark %d %c\n", i, board[i]);
        //fprintf(stdout, "Drew Mark %d %c\n", i, board[i]);
      }
      if (board[i] == 'X') {
        fprintf(write_to, "DrawMark %d %c\n", i, board[i]);
      }
    }
    checkWin(write_to, board);
  }


  char opp_symbol;
  if (my_symbol == 'X')
    opp_symbol = 'O';
  else {
    opp_symbol = 'X';
  }

  if (my_symbol == 'X')
    fprintf(write_to, "YourMove on\n");
  else
    fprintf(write_to, "YourMove off\n");
  char *opp_handle = opphandle;
  opp_handle = strtok(opp_handle, "\n");
  fprintf(write_to, ".g create text 0 20 -anchor nw -text \"You: %s (%c)\" \n", handle, my_symbol);
  fprintf(write_to, ".g create text 0 50 -anchor w -text \"Opponent: %s (%c)\" \n",  opphandle, opp_symbol);
  fprintf(write_to, ".g delete wait\n");

  while(1) {

    for (i = 0; i < 9; i++)
      oldboard[i] = ' ';
    //fprintf(stderr, "test\n");
    bzero ((char *) &inmsg, sizeof (inmsg));
    getmsg (sock, &inmsg);
    fprintf(write_to, "SetSymbol %c\n", my_symbol);
    //system("echo -e \"\007\" >/dev/tty10");
    switch (ntohl (inmsg.type))
  {
    case WHATMOVE:
      fprintf(write_to, "YourMove on\n");
      for (i = 0; i < 9; i++)
        oldboard[i] = inmsg.board[i];
      //fprintf(stderr, "old board: |%s|\n", oldboard);
      receive_board(write_to, oldboard, opp_symbol);
    do {
        valid = 0;
      /* Blocks on read from wish */
      if (fgets (result, 80, read_from) <= 0) exit(0); /* Exit if wish dies */

      /* Scan the string from wish */
      if ((sscanf (result, "%s %d", board, &turn)) == 2) {
        map_board(read_from,write_to, board, turn);
        //fprintf(stderr, "GOOD COMMAND\n");
        valid = 1;
      }
      else {
        runResignation();
        //fprintf(stderr,"Bad command: %s\n",result);
      }
    }
      while (!valid);
    break;

    case RESULT:
      fprintf(write_to, "YourMove clear\n");
      fprintf(write_to, "setGameState\n");
      for (i = 0; i < 9; i++)
        oldboard[i] = inmsg.board[i];
      //fprintf(stderr, "old board: |%s|\n", oldboard);
      receive_board(write_to, oldboard, opp_symbol);
      switch (ntohl (inmsg.number))
        {
        case 0:
          fprintf(write_to, ".g create text 230 20 -anchor nw -text \"You Win\" \n");
          break;
        case 1:
         fprintf(write_to, ".g create text 230 20 -anchor nw -text \"You Lose\" \n");
          break;
        case 2:
          fprintf(write_to, ".g create text 230 20 -anchor nw -text \"Draw\" \n");
          break;
        case 3:
          fprintf(write_to, ".g create text 165 20 -anchor nw -text \"Opponent Resigned\" \n");
          break;
        }
    }
  }
  return 0;


// /***********************************************/

//   /* In the match */

//   for (i = 0; i < 9; i++)
//     board[i] = ' ';
//   finished = 0;

//   while (!finished)
//   {

//       /* Await WHATMOVE/RESULT from server */

//       bzero ((char *) &inmsg, sizeof (inmsg));
//       getmsg (sock, &inmsg);
//       switch (ntohl (inmsg.type))
// 	{

// 	case WHATMOVE:
// 	  for (i = 0; i < 9; i++)
// 	    board[i] = inmsg.board[i];
// 	  dump_board (stdout, board);
// 	  do
// 	    {
// 	      valid = 0;
// 	      printf ("Enter your move: ");
// 	      num = scanf ("%d", &move);
// 	      if (num == EOF)
// 		{
// 		  fprintf (stderr, "ttt:unexpected EOF on standard input\n");
// 		  exit (1);
// 		}
// 	      if (num == 0)
// 		{
// 		  if (fread (&junk, 1, 1, stdin) == EOF)
// 		    {
// 		      fprintf (stderr,
// 			       "ttt:unexpected EOF on standard input\n");
// 		      exit (1);
// 		    }
// 		  continue;
// 		}
// 	      if ((num == 1) && (move >= 1) && (move <= 9))
// 		valid = 1;
// 	      if ((valid) && (board[move - 1] != ' '))
// 		valid = 0;
// 	    }
// 	  while (!valid);

// 	  /* Send MOVE to server */

// 	  bzero ((char *) &outmsg, sizeof (outmsg));
// 	  outmsg.type = htonl (MOVE);
// 	  outmsg.number = htonl (move - 1);
// 	  putmsg (sock, &outmsg);
// 	  break;

// 	case RESULT:
// 	  for (i = 0; i < 9; i++)
// 	    board[i] = inmsg.board[i];
// 	  dump_board (stdout, board);
// 	  switch (ntohl (inmsg.number))
// 	    {
// 	    case 0:
// 	      printf ("You win\n");
// 	      break;
// 	    case 1:
// 	      printf ("You lose\n");
// 	      break;
// 	    case 2:
// 	      printf ("Draw\n");
// 	      break;
// 	    default:
// 	      fprintf (stderr, "Invalid result code\n");
// 	      exit (1);
// 	    }
// 	  finished = 1;
// 	  break;

// 	default:
// 	  protocol_error (MOVE, &inmsg);
// 	}
//     }
//   return 0;
}


void
dump_board (s, board)
     FILE *s;
     char board[];
{
  fprintf (s, "%c | %c | %c\n", board[0], board[1], board[2]);
  fprintf (s, "----------\n");
  fprintf (s, "%c | %c | %c\n", board[3], board[4], board[5]);
  fprintf (s, "----------\n");
  fprintf (s, "%c | %c | %c\n", board[6], board[7], board[8]);
}


char *concat(char *str_1, char *str_2) {
    char *cat_str = malloc(strlen(str_1)+strlen(str_2)+1);
    strcpy(cat_str, str_1);
    strcat(cat_str, str_2);
    return cat_str;
}