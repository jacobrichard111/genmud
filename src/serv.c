

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h> 
#include <unistd.h>
#include "serv.h"
#include "society.h"
#include "script.h"
#include "track.h"
#include "event.h"

static struct timeval curr_time;
static struct sockaddr_in listen_address;
static fd_set input_set;
int times_through_loop = 0;
static int number_of_pulses = 0;

/* All this function does is 1.  Set up the socket on the correct port.
   2. Load a database and 3. Run the server loop. Then it will clean up
   by rebooting if the server ends. */


int 
main (int argc, char **argv)
{
  gettimeofday (&curr_time, NULL);
  my_srand (curr_time.tv_usec + (curr_time.tv_sec >> 2));
  current_time = curr_time.tv_sec;
  signal (SIGSEGV, seg_handler); 
  signal (SIGPIPE, SIG_IGN);
  read_server ();
  init_socket ();
  run_server ();
  exit (0);
}

/* This function opens the server socket (if possible). */

void
init_socket (void)
{
  static int sockflags = 1;
  
  /* You NEED to zero this out or else it will fail to rebind. Bleh. */
  bzero (&listen_address, sizeof(listen_address));
  

  listen_address.sin_family = AF_INET;
  listen_address.sin_port = htons (PORT_NUMBER);
  if (((listen_socket = socket (AF_INET, SOCK_STREAM, 0)) < 0) ||
      
      (setsockopt (listen_socket, SOL_SOCKET, SO_REUSEADDR , (char *) &sockflags, sizeof (sockflags)) < 0) ||
      ((bind (listen_socket, (struct sockaddr *) &listen_address, sizeof (listen_address))) < 0) ||
      (fcntl (listen_socket, F_SETFL, (fcntl (listen_socket, F_GETFL, 0) | O_NONBLOCK)) < 0) ||
      (listen (listen_socket, 5) < 0))
    {
      log_it ("Socket Failed!");
       exit (1);
    }
  log_it ("Socket Ok.");

}

/* Main loop to run the world. */

void
run_server (void)
{
  struct timeval now_time;
  int count;
  int current_time_update_ticks = 0;

  
  max_fd = listen_socket;
  boot_time = current_time;
  
  /* Start the thread that reads data in from the sockets. */
  init_read_thread ();
  init_pulse_thread ();
  while (!IS_SET (server_flags, SERVER_REBOOTING))
    {    
      
      /* Work with the data from the sockets. */
      
      process_input ();  
      
      /* Update the regular event updates. */
      
      update_events ();
      
      
      
      /* Update special script events. */
      check_script_event_list ();
      
      /* Send output to the sockets. */
      process_output ();
      
      /* Update current time once a second. */
      if (++current_time_update_ticks >= PULSE_PER_SECOND)
	{
	  current_time_update_ticks = 0;
	  gettimeofday (&now_time, NULL);
	  current_time = now_time.tv_sec;
	}
      
      /* Wait until the pulse catches up to real game loop times. */
      
      count = 0;
      while (times_through_loop > number_of_pulses &&
	     !IS_SET (server_flags, SERVER_SPEEDUP | SERVER_REBOOTING) &&
	     ++count < 10)
	usleep (5000);  
      
      /* Do not allow speedup with players. */
      
      if (max_players > 2)
	RBIT (server_flags, SERVER_SPEEDUP);
      
      /* times_through_loop is VERY important. It is used to set up
	 the timing for the events which drive the game. Make sure
	 this variable is only changed here or the whole timing
	 of the game will be off. */
      
      times_through_loop++;
    }
  shutdown_server ();
  return;
}
/* This starts the thread that keeps the game pulse. */
  
  void
init_pulse_thread (void)
{
  pthread_t pulse_thread;
  pthread_create (&pulse_thread, NULL, update_pulse, NULL);
  pthread_detach (pulse_thread);
}
 
 
void * 
update_pulse (void *foo)
{
  while (!IS_SET (server_flags, SERVER_REBOOTING))
    {
      if (!IS_SET (server_flags, SERVER_SPEEDUP | SERVER_REBOOTING))
	{
	  number_of_pulses++;
	  usleep (900000/PULSE_PER_SECOND);
	}
      else
	{
	  number_of_pulses = times_through_loop + 1;
	  usleep (1000);
	}
    }
  return NULL;
} 
     

/* This starts the thread that reads data in from the sockets. */

void
init_read_thread (void)
{
  pthread_t read_thread;
  pthread_create (&read_thread, NULL, read_in_from_network, NULL);
  pthread_detach (read_thread);
}

void*
read_in_from_network (void*pointer)
{
  struct timeval wait_time;
  FILE_DESC *fd, *fd_next;
  
  
  while (!IS_SET (server_flags, SERVER_REBOOTING))
    {
      FD_ZERO (&input_set);
      FD_SET (listen_socket, &input_set); 
      for (fd = fd_list; fd != NULL; fd = fd_next)
	{
	  fd_next = fd->next;
	  if ((fd->timer++ > 5000 && fd->connected > CON_ONLINE) ||
	      IS_SET (fd->flags, FD_HANGUP))
	    { /* This special flag gets set so ONLY this function can
		 close off a FD. May be a problem with write/close on
		 diff threads. Will have to work it out later. */
	      SBIT (fd->flags, FD_HANGUP2);
	      close_fd (fd);
	    }
	  else
	    FD_SET (fd->r_socket, &input_set);
	}
      
      /* Need to do always initialize it since linux might ange it -shrug */
      wait_time.tv_sec = 0; 
      wait_time.tv_usec = 1000;
      
      if (select (max_fd + 1, &input_set, NULL, NULL, &wait_time) < 0)
	{
	  log_it ("Bad Select!\n");
	} 
      
      if (FD_ISSET (listen_socket, &input_set))
	new_fd (); 
      
      /* Now we get inputs from players. If any input was sent, this
       returns true and we set the flag for the game to read this stuff,
       and we wait for the game to update this. */
      if (get_input ())
	{
	  int count = 0;
	  SBIT (server_flags, SERVER_READ_INPUT);
	  /* Do not continue the loop until all data has been handled by
	     the server. */
	  /* Only give a second lag time of the reading gets
	     funked somehow. This ++count < 50 can be dangerous but
	     once in a while it seems to hang so i wanted to put this
	     in. Be careful with it. threads suck. Move along. */
	  while (IS_SET (server_flags, SERVER_READ_INPUT) && ++count < 20)
	    usleep (20000);
	}
      else
	usleep (20000);
    }
  return NULL;
}

/* This gets input from the sockets. */

bool
get_input (void)
{
  FILE_DESC *fd, *fd_next;
  bool got_input = FALSE;
  for (fd = fd_list; fd; fd = fd_next)
{
      fd_next = fd->next;
      if (FD_ISSET (fd->r_socket, &input_set) && 
	  !read_into_buffer (fd))
	{
	  close_fd (fd);
	  continue;
	} /* You have input if you got new input or if you had old input
	     still queued up. */
      if (fd->read_buffer[0])
	got_input = TRUE;
    }
  return got_input;
}

/* This function takes the pending input from a descriptor and brings
   the string into the buffer for use within the server. recv maybe? */

bool
read_into_buffer (FILE_DESC *fd)
{
  int len, num_read;
  if (!fd || (len = strlen (fd->read_buffer)) >= STD_LEN/2)
    return FALSE;
  if ((num_read = read (fd->r_socket, fd->read_buffer + len, STD_LEN/2)) >= STD_LEN *3/2 
      || num_read < 0 || len > STD_LEN * 3/2)
    {
      fd->read_buffer[0] = '\0';
      return FALSE;
    }
  fd->read_buffer[len + num_read] = '\0';
  if (num_read > 0)
    fd->timer = 0;
  png_bytes += num_read;
  return TRUE;
}


/* This strips off commands from the players and executes them. */

void
process_input (void)
{
  THING *th;
  FILE_DESC *fd, *fd_next;
  /* If we do read in a command, then we execute it. */
  
  
  for (fd = fd_list; fd; fd = fd_next)
    {
      fd_next = fd->next;
      th = fd->th;
      if (th && IS_PC (th))
	{
	  if (IS_PC1_SET (th, PC_HOLYSPEED))
	    {
	      th->pc->wait = 0;
	      if (th->fgt)
		th->fgt->delay = 0;		  
	    } /* Slow makes this change somehow? */
	  else if (--th->pc->wait < 0)
		th->pc->wait = 0;
	}
    }  
  
  for (fd = fd_list; fd; fd = fd_next)
    {
      fd_next = fd->next;
      th = fd->th;
      if (th && IS_PC (th) &&
	  th->pc->wait > 0)
	continue;
      RBIT (fd->flags, FD_TYPED_COMMAND);
      read_in_command (fd);
      
      /* Entering a command while hunting resets the timer... 
	 and stops other activities like casting etc. */
      
      if ((fd->command_buffer[0] || 
	   IS_SET (fd->flags, FD_TYPED_COMMAND)) 
	  && th && IS_PC (th))
	{
	  remove_typed_events (th, EVENT_COMMAND);
	  remove_typed_events (th, EVENT_TRACK);
	}

      if (IS_SET (fd->flags, FD_TYPED_COMMAND))
	{
	  if (fd->th)
            {
	      fd->th->timer = 10;
              sprintf (prev_command, "%s: %s\n", NAME (fd->th),
		       fd->command_buffer);
            }
	  if (fd->command_buffer[0])
	    {
	      if (!str_cmp (fd->command_buffer, fd->last_command))
		fd->repeat++;
	      else
		{
		  fd->repeat = 0;
		  fd->channel_repeat = 0;
		}
	      strcpy (fd->last_command, fd->command_buffer);
	    }
	  if (fd->snooper)

	    write_to_buffer (fd->command_buffer, fd->snooper);
	  do_something (fd, fd->command_buffer);
	  fd->command_buffer[0] = '\0';
	  if (fd->repeat > 25)
	    {
	      if (fd->th && IS_PC (fd->th))
		{
		  write_to_buffer ("STOP SPAMMING!\n\r", fd);
		  fd->th->pc->wait += 70;
		  fd->repeat = 20;
		}
	    }
	}
    }
  prev_command[0] = '\0';
  RBIT (server_flags, SERVER_READ_INPUT);
  return;
}

/* This sends output to everyone connected */

void 
process_output (void)
{
  FILE_DESC *fd, *fd_next;
  for (fd = fd_list; fd != NULL; fd = fd_next)
    {
      fd_next = fd->next;
      if ((fd->write_buffer[0] ||
	   IS_SET (fd->flags, FD_TYPED_COMMAND)) && 
	  !send_output (fd))
	close_fd (fd);
    }
  return;
}



/* This function parses the input buffer for a single command string
   which is then stripped off and put into the command_buffer and
   the rest of the read_buffer is then moved up to the front. 
   It also handles aliases and replacement. You may need to add a space
   before the alias replacements if players become a pain. */

void
read_in_command (FILE_DESC *fd)
{
  char *s, *t, *r;
  int i, alias_chosen;
  char tempbuf[STD_LEN * 4];
  char aliasname[STD_LEN];
  char alias_rep[MAX_ALIAS_REP][STD_LEN];
  PCDATA *rpc;
  if (fd->command_buffer[0] || !fd->read_buffer[0])
    return;
  
  if (strlen (fd->read_buffer) >= STD_LEN)
    {
      fd->read_buffer[0] = '\0';
      fd->command_buffer[0] = '\0';
      return;
    }
  
  SBIT (fd->flags, FD_TYPED_COMMAND);
  /* Now read stuff into the command buffer until we get to a return or
     the END_STRING_CHAR character which is special and denotes a stop. */
  
  s = fd->command_buffer;
  t = fd->read_buffer;
  
  if (*t == -1) /* Go past the echo on/off string. */ 
    t += 3;
  
  for (; (*t && *t != '\n' && *t != '\r' && *t != END_STRING_CHAR);)
    {
      /* Deal with backspaces. Ignore the space, and move s back one if
	 it's more than the command buffer. */
      if (*t == 8) /* Backspace */
	{
	  if (s > fd->command_buffer)
	    s--;
          t++;
	}
      else
	*s++ = *t++;
    }
  *s = '\0';
  
  if ((fd->connected == CON_CONNECT || fd->connected == CON_GET_NAME) &&
      !str_cmp (fd->command_buffer, "USING_KDE"))
    {
      SBIT (fd->flags, FD_USING_KDE);
      fd->command_buffer[0] = '\0';
      fd->read_buffer[0] = '\0';
      return;
    }
	
  if (*t == '\0')
    {
      fd->command_buffer[0] = '\0';
      RBIT (fd->flags, FD_TYPED_COMMAND);
      return;
    }
      

  /* Now since, we stopped inputting, *t must be a newline/cr/`. If the
     next character is a newline/cr move the pointer up. Then, if the
     orig char was a END_STRING_CHAR, we check one more space to see if 
     the NEXT char is a newline/cr and if so, we delete it to, to avoid 
     too many enters after someone enters a END_STRING_CHAR. */
  
  


  if (*(t + 1) == '\n' || *(t + 1) == '\r')
    {
      t++;
      if (*(t - 1) == END_STRING_CHAR && (* (t + 1) == '\n' || *(t + 1) == '\r'))
	t++;
    }
  t++;
  
  /* Now shift the read buffer up by the length of the command
     string removed. Note that if the last string entered was a 
     return, then the read buffer is emptied. */
  
  s = fd->read_buffer;
  
  while (*t)
    {
      *s++ = *t;
      *t++ = '\0';
    }
  *s = '\0';
 

  if (fd->command_buffer[0] == '!')
    {
      strcpy (fd->command_buffer, fd->last_command);
      return;
    }
  /* Now we check aliases... Make sure the thing and pc exist. */
  
  if (!fd->th || !fd->th->pc || !fd->command_buffer[0] ||
      fd->th->pc->curr_string || fd->th->pc->big_string)
    return;
  rpc = fd->th->pc;
  t = fd->command_buffer;
  t = f_word (t, aliasname);
  
  /* Are we using an alias? */

  for (alias_chosen = 0; alias_chosen < MAX_ALIAS; alias_chosen++)
    {
      if (!str_cmp (aliasname, rpc->aliasname[alias_chosen]))
	break;
    }
  
  /* If we don't find the alias, then just return. */
  
  if (alias_chosen == MAX_ALIAS)
    return;
  
  /* Now strip off the arguments into the various buffers. */
  
  if (*rpc->aliasexp[alias_chosen])
    for (i = 0; i < MAX_ALIAS_REP; i++)
      t = f_word (t, alias_rep[i]);
  
  
  s = tempbuf;
  
  /* Copy the expanded alias into a new buffer. We replace @1 @2 @3... with
     strings of the user's choice. We replace '*' with a newline to allow
     for big numbers of commands to be done in one alias. */
  
  for (t = rpc->aliasexp[alias_chosen]; *t; t++)
    {
      if (*t == '@')
	{
	  t++;

	  /* We have something of the form @1 to @5 or so, and we 
	     need to replace that string with the alias_rep string. */
	  if (*t >= '1' && *t <= (MAX_ALIAS_REP + '0'))
	    {
	      r = alias_rep[*t - '1'];
	      while (*r)
		*s++ = *r++;
	    }
	  else 
	    *s++ = '@';
	}
      else if (*t == '*')
	{
	  *s++ = '\n';
	  *s++ = '\r';
	}
      else
	*s++ = *t;
    }
  *s = '\0';
  
  /* Now we have a new command entered into a temporary buffer. We now check
     if only one command or not, and if not, we copy all of this back
     into the read_buffer...except for the first command. */
  
  strcat (tempbuf, fd->read_buffer);
  if (strlen (tempbuf) > STD_LEN*7/4)
    {
      fd->read_buffer[0] = '\0';
      fd->command_buffer[0] = '\0';
      fd->last_command[0] = '\0';
      return;
    }
  s = fd->command_buffer;
  
  for (t = tempbuf; (*t && *t != '\n' && *t != '\r'); t++)
    {
      *s = *t;
      s++;
    }
  *s = '\0';
  if (*t == '\n' || *t == '\r')
    t++;
  if (*t == '\n' || *t == '\r')
    t++;
  if (*t)
    strcat (t, "\n\r");
  strcpy (fd->read_buffer, t);
  return;
}


/* This creates a new file descriptor that lets the server communicate
   with the various things connected to the server. */

void 
new_fd (void)
{
  int new_socket;
  struct sockaddr_in sock_address;
  FILE_DESC *fd, *fdn;
  int size, num_connected = 0; 
  char buf[STD_LEN];

  size = sizeof (sock_address);
  getsockname (listen_socket, (struct sockaddr *) &sock_address, (socklen_t *) &size);
  
  if ((new_socket = accept (listen_socket, (struct sockaddr *) &sock_address, (socklen_t *) &size)) < 0)
    {
      perror ("failed on socket accept!");
      return;
    } 
  if (fcntl (new_socket, F_SETFL, (fcntl (new_socket, F_GETFL, 0) | O_NONBLOCK)) < 0)
    {
      perror ("new_socket blocked!");
      close (new_socket);
    }
  max_fd = MAX (max_fd, new_socket);
  
  if (fd_free)
    {
      fd = fd_free;
      fd_free = fd_free->next;
      bzero (fd, sizeof (*fd));
    }
  else
    {
      fd = (FILE_DESC *) mallok (sizeof (FILE_DESC));
      file_desc_count++;
      bzero (fd, sizeof (*fd));
    }
  
  /* This initializes the data. */
  
  fd->r_socket = new_socket;
  fd->connected = CON_CONNECT;
  strcpy (fd->num_address, (char *) inet_ntoa(sock_address.sin_addr));
  fd->hostnum = (int) innet_address (fd->num_address);
  fd->name_address[0] = '\0';
  fd->write_buffer[0] = '\0';
  fd->write_pos = fd->write_buffer;
  fd->repeat = 0;
  fd->next = NULL;
  fd->channel_repeat = 0;
  for (fdn = fd_list; fdn != NULL; fdn = fdn->next)
    num_connected++;
  max_players = MAX (max_players, num_connected + 1);
  if (num_connected > 59) /* MAX_CONNECTIONS */
    {
      write_fd_direct(fd, "\n\rToo many connections.\n\r");
      close_fd (fd);
      return;
    }
  
  if (is_sitebanned (fd, 0))
    {
      char errbuf[STD_LEN];
      write_fd_direct (fd, "Your site has been banned.\n\r");
      sprintf (errbuf, "Banned site attempted to log in: %s\n", fd->num_address);
      log_it (errbuf);
      close_fd (fd);
      return;
    }
  

  fd->next = fd_list;
  fd_list = fd;
  
  sprintf (buf, "New Con! %s\n", fd->num_address);
  log_it (buf);
  write_fd_direct (fd, "Welcome to the game. Press enter to continue.\n\r ");
  return;
}



/* This function attempts to write out all pending output for the
   descriptor. */

bool
send_output (FILE_DESC *fd)
{
  bool written;
  if (fd->write_buffer[0] == '\0')
    {
      if (!IS_SET (fd->flags, FD_TYPED_COMMAND))
	return TRUE;
    }
  if (fd->connected <= CON_ONLINE)
    {
      
      if (fd->th && IS_PC (fd->th))
	{
	  if (fd->th->pc->curr_string)
	    {
	      int lines = find_num_lines (*(fd->th->pc->curr_string));
	      char buf[30];
	      sprintf (buf, "%2d> ", lines + 1);
	      write_to_buffer (buf, fd);
	    }
	  else if (fd->th && IS_PC (fd->th) &&  !fd->th->pc->big_string &&
		   (!fd->snooping || IS_SET (fd->flags, FD_TYPED_COMMAND)))
	    {
	      if (!USING_KDE (fd->th))
		write_to_buffer ("\n\r", fd);
	      print_update (fd->th);
	    }
	}
    }
  
  written = write_fd_direct (fd, fd->write_buffer);
  RBIT (fd->flags, FD_TYPED_COMMAND);
  fd->write_buffer[0] = '\0';
  fd->write_pos = fd->write_buffer;
  return written;
}

/* This function appends characters to the output buffer. It cuts
   things off if the new string being added is too long. Just added
   ansi colors on/off. */

void
write_to_buffer (char *txt, FILE_DESC *fd)
{
  char *t;
  if (!fd || !txt || !*txt || fd->r_socket < 1 ||
      (fd->write_pos - fd->write_buffer + strlen(txt) > 
       WRITEBUF_SIZE - 100))
    return;
  
  if (fd->snooper)
    {
      write_to_buffer ("# ", fd->snooper);
      write_to_buffer (txt, fd->snooper);
    }
  if (fd->write_pos == fd->write_buffer && !IS_SET (fd->flags, FD_TYPED_COMMAND))
    {
      strcpy (fd->write_buffer, "\n\r");
      fd->write_pos += 2;
    }
  if (!IS_SET (fd->flags, FD_NO_ANSI))
    {
      strcpy (fd->write_pos, txt);
      fd->write_pos += strlen (txt);
      return;
    }
  t = txt;
  while (*t)
    {
      if (*t != '\x1b')
	*fd->write_pos++ = *t++;
      else
	{
	  while (!isalpha (LC(*t)) && *t)
	    t++;
	  if (*t)
	    t++;
	}
    }
  *fd->write_pos = '\0';
  return;
}
  
/* This is the actual function that writes to the output buffer...it can
   be accessed directly, but usually is accessed through send_output 
   This is sort of like the writen function */
  
bool 
write_fd_direct (FILE_DESC *fd, char *txt)
{
  int len;
  if (!fd || fd->r_socket < 1)
    return FALSE;
  if (txt == fd->write_buffer)
    len = fd->write_pos-fd->write_buffer;
  else
    len = strlen (txt);
  if (write (fd->r_socket, txt, len) >= 0)
    return TRUE;
  return FALSE;
}


void 
close_fd (FILE_DESC *fd)
{
  FILE_DESC *fd2;
  char buf[STD_LEN];
  
  if (!fd || fd->r_socket < 1)
    return;
  
  /* Clean up any residual output */
  
  FD_CLR (fd->r_socket, &input_set);
  
  /* Clean up snoop/switch. */

  if (fd->snooping)
    fd->snooping->snooper = NULL;
  if (fd->snooper)
    fd->snooper->snooping = NULL;
  if (fd->oldthing)
    fd->oldthing->fd = NULL;
  fd->oldthing = NULL;
  
 
  
  if (fd->th && IS_PC (fd->th))
    {
      write_fd_direct (fd, "<HANGUP> </HANGUP>");
      sprintf (buf, "%s leaves the server.\n", NAME (fd->th));
      log_it (buf);
      fd->th->fd = fd;
      fd->th->pc->curr_string = NULL;
      fd->th->pc->editing = NULL;
      act ("@1n has lost @1s link!", fd->th, NULL, NULL, NULL, TO_ROOM);
      fd->th->fd = NULL;
    } 
  
  if (!IS_SET (fd->flags, FD_HANGUP) ||
      !IS_SET (fd->flags, FD_HANGUP2))
    {
      SBIT (fd->flags, FD_HANGUP);
      fd->write_buffer[0] = '\0';
      fd->write_pos = fd->write_buffer;
    }
  
  
  if (fd->connected < CON_ONLINE)
     fd->connected = CON_ONLINE;
  
  
  /* Now remove this from the linked list */

  close (fd->r_socket);
  max_fd = listen_socket;
  if (fd == fd_list)
    {
      fd_list = fd->next;
      fd->next = NULL;
    }
  else
    {
      for (fd2 = fd_list; fd2; fd2 = fd2->next)
	{
	  if (fd2->next == fd)
	    {
	      fd2->next = fd->next;
	      fd->next = NULL;
              break;
	    }
	}
    }
  fd->next = NULL;
  for (fd2 = fd_list; fd2; fd2 = fd2->next)
    max_fd = MAX (max_fd, fd2->r_socket);
       
  
  
  /* Clean up pointers. */
  
  if (fd->th)
    {
      fd->th->fd = NULL;
      fd->th = NULL;
    }
  
  bzero (fd, sizeof (*fd));
  
  /* And add it to the free list. */
  
  fd->next = fd_free;
  fd_free = fd;
  return;
}
  
  
void
do_something (FILE_DESC *fd, char *arg)
{
  THING *th = NULL;
  if ((fd->connected < CON_CONNECT ||
	      fd->connected > CON_GET_NEW_PWD) &&
      ((th = fd->th) == NULL || (!IS_PC (th) && !fd->oldthing)))
    {
      close_fd (fd);
      return;
    }
  if (fd->connected <= CON_ONLINE)
    {
      if (IS_PC (th) && th->pc->big_string && th->pc->big_string_loc)
	{
	  send_page (th);
	}
      else if (IS_PC (th) && th->pc->curr_string)
	{ 
	  if (!fd->command_buffer[0])
	    sprintf (fd->command_buffer, " ");
	  append_string (th);
	  fd->command_buffer[0] = '\0';
	  return;
	}
      else
	{	
	  switch (fd->connected)
	    {
		case CON_ONLINE:
		default:
		  interpret (th, fd->command_buffer);
		  break;
		case CON_EDITING:
		  if (th && IS_PC (th) && th->pc->editing)
		    edit (th, arg);
		  break;
		case CON_SEDITING:
		  if (th && IS_PC (th) && th->pc->editing)
		    sedit (th, arg);
		  break;
		case CON_RACEDITING:
		  if (th && IS_PC (th) && th->pc->editing)
		    racedit (th, arg);
		  break;
		case CON_SOCIEDITING:
		  if (th && IS_PC (th) && th->pc->editing)
		    society_edit (th, arg);
		  break;
		case CON_TRIGEDITING:
		  if (th && IS_PC (th) && th->pc->editing)
		    trigedit (th, arg);
		  break;		  
	    }
	}
    }
  else
    connect_to_game (fd, arg);
  return;
}



/* End switching into something. */

void
do_return (THING *th, char *arg)
{
 
  if (IS_PC (th) && LEVEL (th) < MAX_LEVEL)
    {
      stt("What?\n\r", th);
      return;
    }

  if (!th->fd || !th->fd->oldthing)
    {
      stt ("You haven't switched into anything!\n\r", th);
      return;
    }
  
  stt ("You return to your old body.\n\r", th);
  th->fd->th = th->fd->oldthing;
  th->fd->oldthing = NULL;
  th->fd->th->fd = th->fd;
  th->fd = NULL;
  
  return;
}

/* Switch into something..you become that thing. */

void
do_switch (THING *th, char *arg)
{
  THING *vict;
  

  if (!th->fd)
    return;

  if ((vict = find_thing_in (th, arg)) == NULL)
    {
      stt ("They aren't here for you to switch into.\n\r", th);
      return;
    }
   
  if (IS_PC (vict) || !CAN_MOVE (vict))
    {
      stt ("You can't switch into that.\n\r", th);
      return;
    }
  
  th->fd->oldthing = th;
  th->fd->th = vict;
  vict->fd = th->fd;
  th->fd = NULL;
  return;
}
  
/* Lets you see the output that another character sees. */


void
do_snoop (THING *th, char *arg)
{
  THING *vict;
  FILE_DESC *fd;
  

  if (!str_cmp (arg, "me"))
    {
      if (!th->fd->snooping)
	{
	  stt ("You aren't snooping anyone.\n\r", th);
	  return;
	}
      th->fd->snooping->snooper = NULL;
      th->fd->snooping = NULL;
      stt ("You stop snooping your victim.\n\r", th);
      return;
    }

  
  if ((vict = find_pc (th, arg)) == NULL ||
      vict->fd == NULL)
    {
      stt ("They aren't here.\n\r", th);
      return;
    }
  
  if (vict->fd->snooper)
    {
      stt ("They are already being snooped.\n\r", th);
      return;
    }
  
  if (vict == th)
    {
      stt ("You can't snoop yourself.\n\r", th);
      return;
    }
  
  /* Check for loops */

  fd = vict->fd->snooping;
  while (fd)
    {
      if (fd == th->fd)
	{
	  stt ("You can't snoop in loops!\n\r", th);
	  return;
	}
      fd = fd->snooping;
    }
  
  th->fd->snooping = vict->fd;
  vict->fd->snooper = th->fd;
  stt ("Ok, you snoop your victim.\n\r", th);
  return;
}


/* Bah, inet_addr wouldn't compile under g++ so I wrote this crap :P */

int
innet_address (char *txt)
{
  int ret = 0, block = 0, temp = 0;
  char *t;

  if (!txt || !*txt)
    return 0;
  
  t = txt;
  
  for (block = 0; block < 4 && *t; block++)
    {
      temp = 0;
      while (*t && *t != '.')
	{
	  if (*t >= '0' && *t <= '9')
	    {
	      temp *= 10;
	      temp += *t - '0';
	    }
	  t++;
	}
      if (*t == '.')
	t++;
      ret *= 256;
      ret += temp;
    }
  return ret;
}

/* In case crypt doesn't work or has to be replaced by another function. */


char *
crypt2 (const char *key, const char *salt)
{
  return (char *) crypt ( key, salt);
  
}
/* This lets you wizlock/unwizlock the game. */

void
do_wizlock (THING *th, char *arg)
{
  char buf[STD_LEN];
  if (!th || !IS_PC (th) || LEVEL(th) != MAX_LEVEL)
    return;
  
  if (IS_SET (server_flags, SERVER_WIZLOCK))
    {
      RBIT (server_flags, SERVER_WIZLOCK);
      sprintf (buf, "\\rm %swizlock.dat", WLD_DIR);
      system(buf);
      stt ("Wizlock removed.\n\r", th);
      return;
    }
  else
    {
      SBIT (server_flags, SERVER_WIZLOCK);
      sprintf (buf, "touch %swizlock.dat", WLD_DIR);
      system(buf);
      stt ("Wizlock set.\n\r",th);
      return;
    }
  return;
}
/* This lets you toggle between the server going as fast as possible
   and pulsing 10x a second. */

void
do_speedup (THING *th, char *arg)
{
  if (!th || LEVEL (th) != MAX_LEVEL || !IS_PC (th) || !th->fd)
    return;
  
  if (str_cmp (arg, "foozball"))
    {
      stt ("speedup foozball will toggle whether or not the game goes at maximum speed or not.\n\r", th);
      return;
    }
  
  if (IS_SET (server_flags, SERVER_SPEEDUP))
    {
      RBIT (server_flags, SERVER_SPEEDUP);
      stt ("Server now back to normal speed.\n\r", th);
    }
  else
    {
      SBIT (server_flags, SERVER_SPEEDUP);
      stt ("Server is now going as quickly as possible.\n\r", th);
    }
  return;
}
      

     
