
/* General Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Unix Specific */
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>

/* Network Specific */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/* Bluetooth */
#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH    31
#define PF_BLUETOOTH    AF_BLUETOOTH
#endif
#define BTPROTO_RFCOMM  3

typedef struct {
        uint8_t b[6];
} __attribute__((packed)) bdaddr_t;

#define BDADDR_ANY   (&(bdaddr_t) {{0, 0, 0, 0, 0, 0}})

static inline void bacpy(bdaddr_t *dst, const bdaddr_t *src) {
  memcpy(dst, src, sizeof(bdaddr_t));
}

/* RFCOMM socket address */
struct sockaddr_rc {
  sa_family_t     rc_family;
  bdaddr_t        rc_bdaddr;
  uint8_t         rc_channel;
};

#define VERSION         "01.09.1"
/* Programa Constants & Macros -----------------------------*/
#define T_UDP           SOCK_DGRAM
#define T_TCP           SOCK_STREAM
#define T_BT            1000
#define T_SE            1001
#define INOUT_CLIENT    0
#define BUFSIZE         2048
/* FIXME: Static Array for UDP connections */
#define NPORTS          100  

#ifdef DEBUG                                                              
#define PDEBUG(fmt, arg...)     \
        fprintf(stderr, "%ld [NetKitty]: " fmt, time(NULL), ##arg)            
#else                                                                           
#define PDEBUG(fmt, arg...)     do { } while(0)                                 
#endif 

/* Global Vars --------------------------------------------*/
/* Server can listen to several ports */
static int   *sport_type = NULL, n_port_types = 0;
static int   *s_accept = NULL, n_ports = 0;
/* Real communication sockets */
static int   *s_comm = NULL, n_comm = 0;
static char  *prg = NULL;
static int   ex = 0, _daemon = 0;

/* UDP lazy management */
static int                 udp_sender = -1; /* Use any UDP server for sending */
static struct sockaddr_in  uclient[NPORTS];  /* Store any UDP access */
static int                 n_uclient = 0;

/* Serial Port Functions ***************************/
int 
serialport_init (char* port, int speed) {
  struct termios tio;
  speed_t        _speed = B9600;
  int            fd = -1;
  
  memset (&tio, 0, sizeof(tio));
  tio.c_iflag     = 0;
  tio.c_oflag     = 0;
  tio.c_cflag     = CS8|CREAD|CLOCAL; /* 8n1 */
  tio.c_lflag     = 0;
  tio.c_cc[VMIN]  = 1;
  tio.c_cc[VTIME] = 5;
  
  fd = open (port, O_RDWR | O_NONBLOCK);      
  
  switch (speed)
    {
    case 4800:   _speed = B4800;   break;
    case 9600:   _speed = B9600;   break;
    case 19200:  _speed = B19200;  break;
    case 38400:  _speed = B38400;  break;
    case 57600:  _speed = B57600;  break;
    case 115200: _speed = B115200; break;
    }

  cfsetospeed (&tio,_speed);
  cfsetispeed (&tio,_speed);
 
  tcflush (fd, TCIFLUSH);
  tcsetattr (fd,TCSANOW,&tio);

  return fd;
}

/* Bluetooth Functions *****************************/
void baswap(bdaddr_t *dst, const bdaddr_t *src) {
  register unsigned char *d = (unsigned char *) dst;
  register const unsigned char *s = (const unsigned char *) src;
  register int i;

  for (i = 0; i < 6; i++) d[i] = s[5-i];
}

int str2ba(const char *str, bdaddr_t *ba) {
  uint8_t b[6];
  const char *ptr = str;
  int i;

  for (i = 0; i < 6; i++) {
    b[i] = (uint8_t) strtol(ptr, NULL, 16);
    if (i != 5 && !(ptr = strchr(ptr, ':')))
      ptr = ":00:00:00:00:00";
    ptr++;
  }
  baswap(ba, (bdaddr_t *) b);
  return 0;
}

/* Helper Functions ***************************/
/* Simple print function using write */
int my_print (char *str) {return write (1, str, strlen (str));}

/* Execute an external command its stdin/stdout/stderr the actual socket */
int process ( int s )
{
  pid_t pid ;
  char *name[3] ;

  if ((pid = fork ()) < 0)
    write (2, "Cannot create process\n", 22);
  else
    {
      if (pid) close (s);
      else
	{
	  dup2 (s, 0);
	  dup2 (s, 1);
	  dup2 (s, 2);
	  name[0] = strdup (prg);

	  name[1] = "-i";
	  name[2] = NULL;
	  execv (name[0], name );
	  exit (1);
	}
    }
  return 0;
}

/* Find empty connection entry*/
static int find_empty_entry (int *slist, int n)
{
  int i = 0;

  while (i < n && slist[i] != -1) i++;
  return i;
}

static int add_handler (int **slist, int *n)
{
  int  i;

  if ((i = find_empty_entry (*slist, *n)) == *n)
    {
      (*n)++;
      *slist = realloc (*slist, (*n) * sizeof(int));
    }
  return i;
}

int create_initial_socket (char *ip, char *port, int type1, int inout)
{
  int                  j, i = 0, ops = 1;
  int                  **list, *n;
  struct sockaddr_in   server;
  struct sockaddr_rc   addr;
  void                 *g_addr;
  int                   g_len, family, proto, type;

  list = (inout) ? &s_accept : &s_comm;
  n    = (inout) ? &n_ports  : &n_comm;
  g_len = sizeof(struct sockaddr_in);

  if (type1 == T_BT)
    {
      family = PF_BLUETOOTH;
      proto = BTPROTO_RFCOMM;
      type = SOCK_STREAM;

      addr.rc_family = AF_BLUETOOTH;
      addr.rc_channel = atoi(port);
      g_addr = &addr;
      g_len = sizeof(struct sockaddr_rc);

      if (inout) bacpy(&addr.rc_bdaddr, BDADDR_ANY);
      else str2ba (ip, &addr.rc_bdaddr);
    }
  else
    {
      server.sin_addr.s_addr = (inout) ? INADDR_ANY : inet_addr(ip);
      server.sin_family = AF_INET;
      server.sin_port = htons(atoi(port));
      g_addr = &server;
      family = PF_INET;
      proto = 0;
      type = type1;
    }
  /* Initialise socket. */
  j = add_handler (list, n);
  (*list)[j] = socket (family, type, proto);
  if ((*list)[j] < 0) perror ("socket(NET):");
  
  if (inout)
    {
      i = add_handler (&sport_type, &n_port_types);
      sport_type[i] = type1 == T_BT ? T_TCP : type;
      
      /* Set reuse address/port socket option */
      setsockopt ((*list)[j], SOL_SOCKET, SO_REUSEADDR, &ops, sizeof(ops));
      i = bind ((*list)[j], (struct sockaddr *) g_addr, g_len);
      if (i < 0) perror ("bind:");
    }

  if (type == T_TCP || type1 == T_BT) 
    i = (inout) ? listen ((*list)[j], 10) : 
      connect ((*list)[j], (struct sockaddr*)g_addr, g_len);

  if (ex)
    {
      procesa ((*list)[j]);
      (*list)[j] = -1;
    }

  /* use any UDP socket as default sender*/
  if ((udp_sender == -1) && (type == T_UDP) && inout) 
    udp_sender = j; 
  if (type == T_UDP && !inout) 
    i = connect ((*list)[j], (struct sockaddr*)&server, g_len);

  if (i < 0) {(*list)[j] = -1; (*n)--;} /* On error discard handler */
  return 0;
} 

int hub_send (int ex_tcp, int ex_udp, char *buffer, int len)
{
  int    sa_len = sizeof(struct sockaddr_in);
  int    k;

  PDEBUG ("Broadcasting data... %d TCP (ex. %d)  %d UDP (ex. %d)\n", 
	  n_comm, ex_tcp, n_uclient, ex_udp);

  /* Send data to TCP clients*/
  for (k = 0; k < n_comm; k++)
    if (ex_tcp != k) 
      /*if (send (s_comm[k], buffer, len, 0) < 0) */
      if (write (s_comm[k], buffer, len) < 0) 
	perror ("TCP send:");
    /* Send data to UDP clients */
  if (udp_sender)
    for (k = 0; k < n_uclient; k++)
      if (ex_udp != k)
	if ((sendto (s_accept[udp_sender], buffer, len, 0, 
		     (struct sockaddr*) &uclient[k], sa_len)) < 0)
	  perror ("UDP send:");
  
  return 0;
}

void abrupt_exit (int s)
{
  int   i;

  PDEBUG ("Abrupt Closing. Trying to close connections\n");
  for (i = 0; i < n_comm; i++) if (s_comm[i] != -1) close (s_comm[i]);

  for (i = 0; i < n_ports; i++) if (s_accept[i] != -1) close (s_accept[i]);
  exit (1);
}

/* Main application */
int main (int argc, char *argv[])
{
  fd_set             rfds;
  struct timeval     tv;
  int                i, max, n_res, len, ilen, j, k, l, r;
  int                use_sin = 1, loop4ever = 1;
  char               buffer[BUFSIZE], ibuffer[BUFSIZE];
  struct sockaddr_in client;
  socklen_t          sa_len = sizeof(struct sockaddr_in);
  int                arg_flag, type = 0;
  char               *aux, *aux1;
  int                shell = 0, hub = 0, one_shot = 0;

  if (argc == 1)
    {
      my_print ("NetKitty Version " VERSION "\n");
      my_print ("(c) 2006-2011,2013,2016. David Martinez Oliveira\n\n");
      my_print ("Usage: nk [-daemon] [-shell [path_to_shell]] [-hub] [-os] "
		"[-client ((T|U|B|S),(ip|bt|serial),(port|baud))+] "
		"[-server ((T|U|B),port)+]\n\n");
      exit (1);
    }
  signal (SIGQUIT, abrupt_exit);
  signal (SIGINT, abrupt_exit);

  arg_flag = 0;
  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "-hub", 4) == 0)
	{
	  hub = 1;
	  continue;
	}
      if (strncmp (argv[i], "-os", 3) == 0)
	{
	  one_shot = 1;
	  continue;
	}
      if ((strncmp (argv[i], "-shell", 6) == 0) || (strncmp (argv[i], "-sh", 3) == 0))
	{
	  write (1, "WARNNING: Running in shell\n", 27);
	  shell = 1;
	  if (argv[i+1] && argv[i+1][0] == '-') 
	    {
	      prg = strdup ("/bin/sh");
	      continue;
	    }
	  
	  i++;
	  prg = strdup (argv[i]);

	  continue;
	}
      if ((strncmp (argv[i], "-daemon", 7) == 0) || (strncmp (argv[i], "-d", 2) == 0))
	{
	  _daemon = 1;
	  use_sin = 0;
	  continue;
	}
      /* XXX: To be removed??? */
      if ((strncmp (argv[i], "-exec", 5) == 0) || (strncmp (argv[i], "-e", 2) == 0))
	{
	  ex = 1;
	  i++;
	  prg = strdup (argv[i]);
	  continue;
	}
      if ((strncmp (argv[i], "-client", 7) == 0) || (strncmp (argv[i], "-c", 2) == 0))
	{
	  PDEBUG ("** Reading Client information\n");
	  arg_flag = 1; /* Process client data */
	  continue;
	}
      if ((strncmp (argv[i], "-server", 7) == 0) || (strncmp (argv[i], "-s", 2) == 0))
	{
	  PDEBUG ("** Reading Server information\n");
	  arg_flag = 2; /* Process server data */
	  use_sin = 0;
	  continue;
	}
      aux = argv[i];
      switch (*aux)
	{
	case 'T':
	  type = T_TCP;
	  break;
	case 'U':
	  type = T_UDP;
	  break;
	case 'B':
	  type = T_BT;
	  break;
	case 'S':
	  type = T_SE;
	  arg_flag = 3;
	  break;
	}
      aux += 2;
      switch (arg_flag)
	{
	case 1:
	  *(aux1 = strchr (aux, ',')) = 0;
	  PDEBUG ("Connecting to '%s':'%s' (%d)\n", aux, aux1 + 1, type);
	  create_initial_socket (aux, aux1 + 1, type, INOUT_CLIENT);
	  break;
	case 2:
	  PDEBUG ("Accepting '%s' (%d)\n", aux, type);
	  create_initial_socket (NULL, aux, type, INOUT_CLIENT + 1);
	  break;
	case 3:
	  *(aux1 = strchr (aux, ',')) = 0;
	  PDEBUG ("Opening '%s':'%s' (%d)\n", aux, aux1 + 1, type);
	  int index = add_handler (&s_comm, &n_comm);
	  s_comm[j] = serialport_init (aux, atoi(aux1+1));
	  break;
	default:
	  break;
	}      
    }  
  /* Main loop */
  while (loop4ever)
    {
      if (n_ports == 0 && n_comm == 0) break;
      /* Set File Descriptor Set */
      FD_ZERO(&rfds);

      ilen = max = 0;
      PDEBUG ("Building select data %d, %d\n", n_ports, n_comm);
      for (i = 0; i < n_ports; i++)
	{
	  FD_SET(s_accept[i], &rfds);
	  if (s_accept[i] >= max) max = s_accept[i] + 1;
	}

      for (i = 0; i < n_comm; i++)
	  {
	    if (s_comm[i] != -1) FD_SET(s_comm[i], &rfds);
	    if (s_comm[i] >= max) max = s_comm[i] + 1;
	  }
      /* Add stdin to rdset and set timeout*/
      if (use_sin) FD_SET(0, &rfds);

      /* 4 sec Timeout*/
      tv.tv_sec = 4;
      tv.tv_usec = 0;

      if ((n_res = select (max, &rfds, NULL, NULL, &tv)) < 0)
	perror ("select:");
      else
	{
	  /* Check stdin data */
	  if (!_daemon && FD_ISSET(0, &rfds))
	    {
	      PDEBUG ("stdin data available... read %d bytes\n", ilen);
	      /* Get stdin data to send to all clients */
	      ilen = read (0, ibuffer, BUFSIZE);
	      if (ilen == 0) 
		{
		  /* XXX: Piped mode we got file descriptor active 
		     but no data is available*/
		  loop4ever = 0;
		  continue;
		}
	      PDEBUG ("stdin data available... read %d bytes\n", ilen);
	      ibuffer[ilen] = 0;
	    }
	  if (n_res)
	    {
	      PDEBUG ("---> Got Something (%d connections active): %d\n", 
		      n_comm, n_res);
	      /* Check client sockets */
	      /* ---------------------------------------------*/
	      for (i = 0; i < n_comm; i++)
		{
		  if (s_comm[i] == -1) continue;
		  if (FD_ISSET(s_comm[i], &rfds))
		    {
		      /*if ((len = recv (s_comm[i], buffer, BUFSIZE, 0)) <= 0)*/
		      if ((len = read (s_comm[i], buffer, BUFSIZE)) <= 0)
			{
			  PDEBUG ("0 bytes read.... removing socket\n");
			  s_comm[i] = -1;
			  if (one_shot) return 0; else continue;
			}
		      PDEBUG ("Read %d bytes from TCP stream\n", len);
		      buffer[len] = 0; 
		      if (!_daemon) write (1, buffer, len);
		      /* Send to each connected socket*/
		      if (hub) hub_send (i, -1, buffer, len);
		    }		  
		  if (ilen) 
		    /*if (send (s_comm[i], ibuffer, ilen, 0) < 0)*/
		    if (write (s_comm[i], ibuffer, ilen) < 0)
		      {
			close (s_comm[i]);
			s_comm[i] = -1;
			if (one_shot && (n_comm <= 0)) return 0;
		      }
		}
	      /* Check accept sockets */
	      /* --------------------------------------------------*/
	      for (i = 0; i < n_ports;i++)
		{
		  if (FD_ISSET(s_accept[i], &rfds))
		    {
		      /* Process TCP Connections */
		      if (sport_type[i] == T_TCP)
			{
			  j = add_handler (&s_comm, &n_comm);
			  s_comm[j] = 
			    accept (s_accept[i], (struct sockaddr*) &client, 
				    &sa_len);
			  if (!_daemon) use_sin = s_comm[j];
			  PDEBUG ("Connection accepted for channel %d\n", j);
			  if(shell) 
			    {
			      procesa (s_comm[j]);
			      s_comm[j] = -1;
			    }
			}
		      else /* Process UDP data */
			{
			  PDEBUG ("Processing %d UDP port\n", s_accept[i]);
			  len = recvfrom (s_accept[i], buffer, BUFSIZE, 0, 
					  (struct sockaddr*) &client, &sa_len);
			  buffer[len] = 0;
			  if (!_daemon) write (1, buffer, len);

			  /* Check if client already exists */
			  for (l = 0; l < n_uclient; l++)
			    if (memcmp (&uclient[l], &client, sa_len) == 0)
			      break;
			  if (l == n_uclient)
			    {
			      memcpy (&uclient[n_uclient], &client, sa_len);
			      n_uclient++;
			    }
			  /* Send to each connected socket*/
			  if (hub) hub_send (-1, l, buffer, len);
			  /* fflush(NULL); */
			}
		    }
		  /* Send standard input data to all UDP clients */
		  if (ilen && sport_type[i] == T_UDP)
		    {
		      for (k = 0; k < n_uclient; k++)
			sendto (s_accept[i], ibuffer, ilen, 0, 
				(struct sockaddr*) &uclient[k], sa_len);
		    }
		}
	    }
	  else
	    {
	      /* Add here your idle operation */
	      PDEBUG ("TIMEOUT!!!!\n");
	    }
	}
    }
  return 0;
}
