#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "serv.h"
#include "script.h"
#include "track.h"
#include "event.h"
#include "society.h"
/* Makes a new channel struct. */

CHAN *
new_channel (void)
{
  CHAN *newchan;
  int i;

  newchan = (CHAN *) mallok (sizeof (*newchan));
  bzero (newchan, sizeof (*newchan));
  for (i = 0; i < CHAN_NAME_MAX; i++)
    newchan->name[i] = nonstr;
  
  newchan->number = channel_count++;
  if (channel_count >= MAX_CHANNEL)
    {
      log_it( "TOO MANY CHANNELS!!! INCREASE MAX_CHANNEL!!!!\n");
      exit (1);
    }
  return newchan;
}

/* These two functions make and free scrollbacks... */

SCROLLBACK *
new_scrollback (void)
{
  SCROLLBACK *sback;
  
  if (scrollback_free)
    {
      sback = scrollback_free;
      scrollback_free = scrollback_free->next;
    }
  else
    {
      sback = (SCROLLBACK *) mallok (sizeof (SCROLLBACK));
      scrollback_count++;
    }
  sback->text[0] = '\0';
  sback->next = NULL;
  return sback;
}

void
free_scrollback (SCROLLBACK *sback)
{
  if (!sback)
    return;

  sback->next = scrollback_free;
  scrollback_free = sback;
  sback->text[0] = '\0';
  return;
}

/* Loads channels in from a datafile. */

void
read_channels (void)
{
  CHAN *chan;
  FILE_READ_SETUP("channels.dat");
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("CHAN")
	{
	  chan = read_channel(f);
	  chan->next = channel_list;
	  channel_list = chan;
	}
      FKEY("END_OF_CHANNELS")
	break;
      FILE_READ_ERRCHECK("channels.dat");
    }
  fclose (f);
  return;
}

/* Reads one channel in from a datafile. */

CHAN *
read_channel (FILE *f)
{
  CHAN *chan;
  int i;
  FILE_READ_SINGLE_SETUP;
  chan = new_channel ();
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY ("Name")
	{
	  for (i = 0; i < CHAN_NAME_MAX; i++)
	    {
	      if (chan->name[i] == nonstr)
		{
		  chan->name[i] = new_str (read_string (f));
		  chan->num_names = i;
		  break;
		}
	    }
	  if (i == CHAN_NAME_MAX)
	    read_string (f);
	}
      FKEY("Message")
	chan->msg = new_str (read_string (f));
      FKEY("Flags")
	chan->flags = read_number (f);
      FKEY("LevelUse")
	chan->use_level = read_number (f);
      FKEY("LevelTo")
	chan->to_level = read_number (f);
      FKEY("ClanType")      
	chan->clantype = read_number (f);
      FKEY("END_CHAN")
	break;
      FILE_READ_ERRCHECK("channels.dat");
    }
  return chan;
}

/* This command lets you see what channels you are listening to atm. */

void
do_channels (THING *th, char *arg)
{
  CHAN *chan;
  char buf[STD_LEN];

  if (!IS_PC (th)) 
    return;
  stt ("Your current channel status:\n\n\r", th);

  for (chan = channel_list; chan; chan = chan->next)
    {
      if (chan->number >= 0 && chan->number < MAX_CHANNEL &&
          LEVEL (th) >= chan->use_level)
        {
          sprintf (buf, "\x1b[1;37m%-15s\x1b[0;37m -- \x1b[1;3%dm%s\x1b[0;37m\n\r", 
          chan->name[0], (th->pc->channel_off[chan->number] ? 4 : 3),
          (th->pc->channel_off[chan->number] ? "Off" : "On"));
          stt (buf, th);
        }
    }
   return;
}

/* This is where all channel communications get done. I don't know
   if this is really any more efficient (probably not) or any easier
   to understand than having all these separate channels, but I do
   like the idea of trying to combine similar activities into one 
   function if at all possible. */

void
do_channel (THING *th, CHAN *chan, char *arg)
{
  char buf[STD_LEN];
  char arg1[STD_LEN];
  char buf2[STD_LEN];
  char alignbuf[50];
  THING *victim = NULL, *victimn = NULL, *loc = NULL;
  char *t, *oldarg;
  bool quote;
  SCROLLBACK *sback;
  int flags, count, pos, align = -1;
  
  
  if (!th || !chan || !arg || (currchan = chan->number) < 0 
      || currchan >= MAX_CHANNEL || !CAN_TALK (th) ||
      (!IS_PC (th) && th->position == POSITION_SLEEPING) ||
      (!CAN_TALK(th) && !SERVER_BEING_ATTACKED_YELL))
    { 
      return;
    }
  


  if (IS_PC1_SET (th, PC_SILENCE) && LEVEL (th) < MAX_LEVEL)
    {
      stt ("You are silenced!\n\r", th);
      return;
    }



  if (!*arg && IS_PC (th)) /* Do scrollbacks bleh */
    {
      pos = th->pc->sback_pos[currchan];
      for (count = 0; count < MAX_SCROLLBACK; count++)
	{
	  if ((sback = th->pc->sback[currchan][(count + pos + 1) % MAX_SCROLLBACK]) != NULL && *sback->text)
	    {
	      stt (sback->text, th);
	    }
	}
      return;
    }
	
  /* Do channel on/off. */
   
  if (IS_PC (th) && !str_cmp (arg, "off"))
    {
      if (th->pc->channel_off[currchan])
        {
          stt ("This channel is already off.\n\r", th);
          return;
        }
      stt ("Ok, channel turned off.\n\r", th);
      th->pc->channel_off[currchan] = TRUE;
      return;
    }  


  if (th->fd)
    {
      if (!str_cmp (th->fd->command_buffer, th->fd->last_command))
	{
	  th->fd->channel_repeat++;
	}
      else
	th->fd->channel_repeat = 0;
      
      if (th->fd->channel_repeat > 3)
	{
	  stt ("STOP SPAMMING THE CHANNELS YOU MAROON!!!!\n\r", th);
	  if (IS_PC (th))
	    th->pc->wait += 120;
	  th->fd->channel_repeat = 3;
	  return;
	}
    }
  flags = chan->flags;
  quote = !IS_SET (flags, CHAN_NO_QUOTE);

  oldarg = arg;
  alignbuf[0] = '\0';
  if (IS_SET (flags, CHAN_TO_ALL))
    {
      if (LEVEL (th) == MAX_LEVEL)
	{
	  arg = f_word (arg, arg1);
	  if (isdigit (arg1[0]) && (align = atoi(arg1)) < ALIGN_MAX)
	    {
	      sprintf (alignbuf, " \x1b[1;32m[Align %d]\x1b[0;37m ", align);
	    }
	  else 
	    arg = oldarg;
	}
      else
	{ 
	  sprintf (alignbuf, " \x1b[1;32m[Align %d]\x1b[0;37m ", th->align);
	}
    }
  
  if (IS_SET (flags, CHAN_TO_TARGET))
    {
      arg = f_word (arg, arg1);
      if ((victim = find_thing_world (th, arg1)) == NULL ||
         !CAN_TALK (victim))
	{
	  sprintf (buf, "Who was that %s supposed to go to?\n\r", chan->name[0]);
	  stt (buf, th);
	  return;
	}
      if (victim == th)
	{
	  stt ("Talking to yourself again?\n\r", th);
	  return;
	}
      if (ignore (victim, th))
        {
          stt ("They are ignoring you.\n\r", th);
          return;
        }		
      if (IS_PC (victim) && victim->pc->channel_off[currchan])
        {
           stt ("Your intended victim is not listening to this channel.\n\r", th);
           return;
        }
 	
    }


  if (IS_SET (flags, CHAN_TO_CLAN))
    {
      if (chan->clantype < 0 || chan->clantype >= CLAN_MAX)
	{ 
	  stt ("There are no clans of that type.\n\r", th);
	  return;
	}
      if (!IS_PC (th))
	return;
      if (th->pc->clan_num[chan->clantype] == 0)
	{
	  stt ("Huh you aren't in a clan of that type!\n\r", th);
	  return;
	}
    }
  for (t = arg; *t; t++)
    {
      if (*t == '@')
	*t = 'a';
    }

  if (IS_PC (th) && th->pc->channel_off[currchan])
    {
      stt ("You start listening to this channel again.\n\r", th);
      th->pc->channel_off[currchan] = FALSE;
    }
    


  if (!IS_SET (flags, CHAN_USE_COLOR) && LEVEL (th) < MAX_LEVEL &&
      IS_PC (th))
    {
      for (t = arg; *t; t++)
	{
	  if (*t == '$')
	    {
	      stt ("You cannot use colors in this channel!\n\r", th);
	      return;
	    }
	}
    }
  
  /* Add premessages as needed for certain things. */
  
  if (IS_SET (flags, CHAN_TO_TARGET))
    sprintf (buf2, " @3n");
  else buf2[0] = '\0';
  
  /* This command makes messages of the following sort. Assuming that
     the "message" in channels.dat is like "@1n <chat@s>". this will make
     a message like @1n <chat@s> 'Hello there!' and send it to act (); */
  
  sprintf (buf, "%s%s %s%s%s$7", chan->msg,  buf2, 
	   (quote ? "'" : ""), 
	   arg, (quote ? "'" : ""));
  
  if (IS_SET(flags, CHAN_TO_TARGET))
    {
      if (IS_PC (victim))
        victim->pc->reply = th;
      act (buf, th, NULL, victim, NULL, TO_CHAR + TO_CHAN);
      act (buf, th, NULL, victim, NULL, TO_VICT + TO_CHAN);
      if (victim)
	check_trigtype (victim, th, NULL, arg, TRIG_TELL);
      return;
    }
  else if (IS_SET (flags, CHAN_TO_NEAR))
    { 
      if (th->position <= POSITION_SLEEPING)
	{
	  stt ("You can't do this while you're sleeping!\n\r", th);
	  return;
	}
      
      process_yell (th, buf);
      return;
    }
  else if (IS_SET (flags, CHAN_TO_ROOM))
    { 
      if (th->position <= POSITION_SLEEPING)
	{
	  stt ("You can't do this while you're sleeping!\n\r", th);
	  return;
	}
      
      if (th->in && th->in->in && 
	  !IS_ROOM(th->in) &&
	  (IS_WORN(th) || CAN_FIGHT(th->in) ||
				   CAN_MOVE (th->in)))
	loc = th->in->in;
      else if (IS_AREA (th))
	loc = find_random_room (th, FALSE,0, 0);
      else if (IS_ROOM (th))
	loc = th;
      else 
	loc = th->in;
      for (victim = loc->cont; victim; victim = victimn)
        {
          victimn = victim->next_cont;
          if (ignore (victim, th) || BUILDER_QUIET (victim) ||
              (IS_PC (victim) && victim->pc->channel_off[currchan]))	      
	    continue;
	  act (buf, th, NULL, victim, NULL, TO_VICT + TO_CHAN);
	  if (IS_SET (victim->thing_flags, TH_SCRIPT))
            check_trigtype (victim, th, NULL, arg, TRIG_SAY);
	}
      return;
    }
  
  /* The big "if" down there gets rid of those fd's which are a) not 
     attached to a thing, or b) of the wrong alignment when an
     aligned channel is on or c) not in the same clan or d) not 
     in the same group. */
  
  
  else if (IS_SET (flags, CHAN_TO_ALL | CHAN_TO_GROUP | CHAN_TO_CLAN))
    {      
      FILE_DESC *fd;

      if (BUILDER_QUIET (th))
	{
	  stt ("You are currently building quietly. You cannot use channels.\n\r", th);
	  return;
	}
      
      for (fd = fd_list; fd; fd = fd->next)
	{
	  if (!fd->th || 
	      (!IS_SET (flags, CHAN_OPP_ALIGN) && 
	       LEVEL (fd->th) < BLD_LEVEL &&
               LEVEL (th) < BLD_LEVEL &&
	       DIFF_ALIGN (th->align, fd->th->align)) ||
	      (align != -1 && DIFF_ALIGN (fd->th->align, align)
	       && LEVEL (fd->th) < MAX_LEVEL) ||
	      (IS_SET (flags, CHAN_TO_CLAN) && 
	       chan->clantype >= 0 && 
	       chan->clantype < CLAN_MAX &&
	       (!IS_PC (fd->th) ||
		fd->th->pc->clan_num[chan->clantype] !=
		th->pc->clan_num[chan->clantype])) ||
	      (LEVEL (fd->th) < chan->to_level) ||
	      ignore (fd->th, th) ||
	      BUILDER_QUIET (fd->th) ||
              (IS_PC (fd->th) && fd->th->pc->channel_off[currchan]) ||
	      (IS_SET(flags, CHAN_TO_GROUP) &&
	       !in_same_group (th, fd->th))) 		 
	    continue;
	  if ((align != -1 || LEVEL (th) < BLD_LEVEL)
	      && LEVEL (fd->th) == MAX_LEVEL)
	    stt (alignbuf, fd->th);
	  act (buf, th, NULL, fd->th, NULL, TO_VICT + TO_CHAN);
	}
    }
  else
    stt ("This channel didn't go to anything..odd.\n\r", th);
  return;
}

/* Lets a PC reply to whatever told to it last. */	    
	      
void
do_reply (THING *th, char *arg)
{
  char buf[STD_LEN];
  if (!IS_PC (th))
    return;
  
  if (!th->pc->reply)
    {
      stt ("Reply to whom?\n\r", th);
      return;
    }

  sprintf (buf, "tell %s %s", NAME (th->pc->reply), arg);
  interpret (th, buf);
  return;
}  

/* Yells move outward in all directions from the location of the yeller,
   and sometimes creatures respond to these yells and come attacking. */

void
process_yell (THING *yeller, char *buf)
{
  THING *to, *room, *nroom, *ton, *attacker = NULL, *flee_room;
  int i, depth = 0, j, num_sylls = 0;
  VALUE *exit, *soc, *soc2;
  SOCIETY *society = NULL;
  
  if (!yeller || !buf || !*buf || ((room = yeller->in) == NULL))
    return;
  
  if (!CAN_TALK (yeller))
    {
      /* We assume that buf is at least STD_LEN long. */
      
      sprintf (buf, "$9@1n %ss '", (nr (1,3) == 2 ? "bellow" :
				    (nr (1,2) == 1 ? "roar" : "yell")));
      num_sylls = nr (3,6);
      for (i = 0; i < num_sylls; i++)
	{
	  switch (nr (1,5))
	    {
	      case 1:
		strcat (buf, "Roar");
		break;
	      case 2:
		strcat (buf, "Raar");
		break;
	      case 3:
		strcat (buf, "Grrr");
		break;
	      case 4:
		strcat (buf, "Eeagh");
		break;
	      case 5:
		strcat (buf, "Growll");
		break;
	      case 6:
	      default:
		strcat (buf, "Aargh");
		break;
	    }
	}
      strcat (buf, "'$7");
    }
  
  if (BUILDER_QUIET (yeller))
    {
      stt ("You are currently building quietly. You cannot use channels.\n\r", yeller);
      return;
    }
  
  /* Find society yeller is in. */
  
  if ((soc = FNV (yeller, VAL_SOCIETY)) != NULL &&      
      (society = find_society_in (yeller)) == NULL)
    soc = NULL;
  
  if (BEING_ATTACKED && 
      (!yeller->fgt || (attacker = yeller->fgt->fighting) == NULL ||
       yeller->fgt->hunting[0] == '\0'))
    return;
  
  undo_marked (room);
  clear_bfs_list ();
  
  add_bfs (NULL, room, DIR_MAX);
  
  while (bfs_curr)
    {
      
      /* Check if there is a room here to work with. */
      
      if ((room = bfs_curr->room) != NULL)
	{
	  
	  /* Send the message to everyone in the room, and check for 
	     mobs that track down someone attacking someone else. */
	  
	  for (to = room->cont; to != NULL; to = ton)
	    {
	      ton = to->next_cont;
	      
	      if (IS_PC (to))
		{		  
		  if (!ignore (to, yeller) && !BUILDER_QUIET (to))
		    act (buf, yeller, NULL, to, NULL, TO_VICT + TO_CHAN);
		}
	      /* Check for yell helpers. The yeller must be fighting
		 something and the to must be able to move and fight
		 and be of diff align from attacker and kill_opp or soci.  */
	      else if (BEING_ATTACKED && attacker &&
		       CAN_FIGHT (to) && to != yeller &&
		       CAN_MOVE (to) && !is_hunting (to))
		{
		  /* If the thing is sleeping, it probably won't wake up.
		     unless it's close to the yeller. */
		  
		  if (to->position == POSITION_SLEEPING)
		    {
		      if (nr (1, bfs_curr->depth) >= 2)	      
			continue;
		      else
			add_command_event (to, "wake", nr (2*UPD_PER_SECOND, 6*UPD_PER_SECOND));
		    }
		  else if (nr (1,6) != 2)
		    continue;
		  if (soc && (soc2 = FNV (to, VAL_SOCIETY)) != NULL &&
		      soc->val[0] == soc2->val[0])
		    {		
		      if (!IS_SET (soc2->val[2], BATTLE_CASTES))
			{
			  /* Nonwarrior society members flee. */
			  if (nr (1,5) == 3)
			    {
			      for (j = 0; j < 5; j++)
				{
				  if ((flee_room = find_thing_num (nr (society->room_start, society->room_end))) != NULL &&
				      IS_ROOM (flee_room) &&
				      !IS_MARKED (flee_room))
				    {
				      start_hunting_room (to, flee_room->vnum, HUNT_HEALING);
				      break;
				    }
				}
			    }
			  continue;
			}
		      else if (!IS_SET (soc->val[3], WARRIOR_SENTRY))
			start_hunting (to, yeller->fgt->hunting, HUNT_KILL); 
		    }
		  else if (is_enemy (to, attacker))
		    start_hunting (to, yeller->fgt->hunting, HUNT_KILL); 
		}	      
	    }
	  
	  /* If this is a room, we add new things to the end of the list. */
	  
	  if (IS_ROOM (room))
	    {
	      for (i = 0; i < REALDIR_MAX; i++)
		{
		  depth = bfs_curr->depth;
		  if ((exit = FNV (room, i+1))  != NULL &&
		      !IS_SET (exit->val[1], EX_WALL) &&
		      (nroom = find_thing_num (exit->val[0])) != NULL &&
		      IS_ROOM (nroom) && !IS_MARKED (nroom))
		    {
		      depth++;
		      if (IS_SET (exit->val[1], EX_DOOR) &&
			  IS_SET (exit->val[1], EX_CLOSED))
			depth += 2;
		      if (IS_ROOM_SET (room, ROOM_NOISY))
			depth += 3;
		      bfs_curr->depth = depth;
		      if (depth <= MAX_YELL_DEPTH)
			add_bfs (bfs_curr, nroom, i);
		    }		  
		}
	    }
	}
      bfs_curr = bfs_curr->next;
    }
  clear_bfs_list ();
  return;
}


void
do_echo (THING *th, char *arg)
{
  if (th && IS_PC (th) && LEVEL (th) < MAX_LEVEL)
    {
      stt ("Huh?\n\r", th);
      return;
    }
  
  if (arg && *arg)
    echo (arg);
  return;
}
 
/* This just sends a message directly to all people connected. */ 


void
echo (char *buf)
{
  FILE_DESC *fd;
  for (fd = fd_list; fd; fd = fd->next)
    {
      if (fd->connected <= CON_ONLINE)
	write_to_buffer (buf, fd);
    }
  return;
}



/* Make sure there is a channel called "say" in channels.dat or this
   will overload the stack and crash it. */

void
do_say (THING *th, char *arg)
{
  char buf[STD_LEN];
  sprintf (buf, "say %s", arg);
  interpret (th, buf);
  return;
}

/* send_to_thing :P just a way for the database to talk to the network. */

void
stt (char *buf, THING *th)
{
  if (buf && th && *buf && th->fd)
    write_to_buffer (buf, th->fd);  
  return;
}

/* This does a send_to_thing to a thing and everything it contains
   all the way down the tree. */

void
sttr (char *buf, THING *th)
{
  THING *thg, *thgn;
  
  if (!th || !buf || !*buf)
    return;
  stt (buf, th);  
  for (thg = th->cont; thg; thg = thgn)
    {
      thgn = thg->next_cont;
      sttr (buf, thg);
    }
  return;
}
  
  
/* This should roughly do what the diku act () function does...
   the idea of being able to replace THINGS with their names
   and him/her/it is a really powerful idea. Check out the TO_ALL
   which is used all over and saves a lot of typing..even if it
   requires the coders to be less of idiots. */
   
void 
act (char *buf, THING *th, THING *use, THING *vict, char *txt, int type)
{
  THING *to, *curr, *ton;
  char *r, *s, *t, *u;
  bool spam = FALSE, chan = FALSE;
  int i, num;
  char actret[10000];
  actret[0] = '\0';

  
  if (!buf || !buf[0] || !th || !th->in || CONSID)
    return;
  
  if (type >= 20)
    {
      type -= TO_SPAM;
      spam = TRUE;
    }
  else if (type >= 10)
    {
      type -= TO_CHAN;
      chan = TRUE;
    }

  if (type == TO_VICT && !vict)
    return;
  
  /* The code makes 2 passes. First, it goes down the list of things
     in th->in ... (th->in->cont) and then it attempts to show them all
     the message. Then, if the type is not TO_CHAR, it then checks
     if the vict is in another thing. If it is, it shows the message
     to the vict's "room" and whoever can see it in there. */
  
  for (i = 0; i < 2; i++)
    {
      if (i == 0)
        {
          if (type == TO_CHAR)
            to = th;
          else if (type == TO_VICT)
            to = vict;
          else
      	   to = th->in->cont;
        }
      else if (type != TO_CHAR && type != TO_VICT &&
              vict && vict->in && vict->in != th->in)
  	to = vict->in->cont;
      else        
	break;
      for (; to; to = ton)
	{
          ton = to->next_cont;
	  if (!to->fd ||
              (to->position <= POSITION_SLEEPING && to != th) ||
	      to->fd->connected > CON_ONLINE ||
	      (type == TO_CHAR && to != th) ||
	      (type == TO_VICT && to != vict) ||
	      (type == TO_ROOM && to == th) ||
	      (type == TO_NOTVICT && (to == vict || to == th)) ||
	      (spam && IS_PC (to) && IS_PC2_SET (to, PC2_NOSPAM)) ||
	      (chan && ignore (to, th)))
	    continue;
	  s = actret;
	  for (t = buf; *t; t++)
	    {
	    
	      /* First we add ansi if needed. */
	      if (*t == '$')
		{
		  t++;
		  if (*t == '\0')
		    break;
		  else if (*t == '$')
		    {
		      *s = '$'; 
		      s++;
		    }
		  else
		    {
		      num = ((*t >= '0' && *t <= '9') ? (*t - '0') :
			     ((LC(*t) >= 'a' && LC(*t) <= 'f') ? (LC(*t) - 'a' + 10) : -1));
		      if (type == TO_COMBAT && LC (*t) == 'x')
			{
			  if (vict == to)
			    num = 14;
			  else if (th == to)
			    num = 13;
			  else
			    num = 15;
			}
		      if (num != -1)
			{
			  *s = '\x1b';
			  *(s + 1) = '[';
			  *(s + 2) = ((num / 8) + '0');
			  *(s + 3) = ';';
			  *(s + 4) = '3';
			  *(s + 5) = ((num % 8) + '0');
			  *(s + 6) = 'm';
			  s += 7;
			}
		    }
		}
	      /* Then we add info based on the things that are doing
		 the acting. The basic idea is this. You take a string
		 and if it is of the form '@t' or '@T' it adds text or else
		 '@<digit><letter>' where digit is from 1-3 and represents
		 (1 == th), (2 == use), (3 == vict), and then attempts
		 to give the name or sex or her his or him her or whatever. 
		 if it is NULL...then we just add a small notice
		 @1n gives a name @1s gives his/her/its @1m gives
		 her/him/it 
		 Another piece of functionality is '@s'. This is 
		 designed to deal with things like 
		 "You slash the dog. Bob slashes you. Bob slashes
		 the dog...based on the perspective between the
		 th and the victim. */

	      else if (*t == '@')
		{
		  t++;
		  if (*t == '\0')
		    break;
		  if (*t == '@')
		    {
		      *s = '@';
		      s++;
		    }
		  else if (UC(*t) == 'T')
		    {
		      if (txt && *txt)
			for (r = txt; *r; r++, s++)
			  *s = *r; 
		    }
		  else if (UC(*t) == 'S')
		    {
		      /* This deals with problems like "slash", "slashes"
			 etc... Perspective in verbs. So, if the
			 to == th, we don't need to add anything. 
			 If the @s is too close to the start, we
			 also don't need to add anything. */

                      /* This is to deal with shot and shoot. */

                      if ((s - actret) < 3)
                        continue;


                      if (UC (*(s - 1)) == 'T' &&
                          UC (*(s - 2)) == 'O' &&
                          UC (*(s - 3)) == 'H')
                        {
                          if (UC (*(s - 1)) == *(s - 1))
                            {
                              *(s - 1) = 'O';
                              *s = 'T';
                              s++;
                            }
                          else
                            {
                              *(s - 1) = 'o';
                              *s = 't';
                              s++;
                            }
                         }


		      if (to == th)
			continue;
		      
		      /* So if the word ends in 'slash' we make it 
			 'slashes' and if not, we just make it 's',
			 and 'boss' goes to 'bosses' */
		      /* Change watch to watches */

		      if (((UC(*(s - 1)) == 'H' &&
			  (UC (*(s - 2)) == 'S' ||
			   UC (*(s - 2)) == 'C' ||
			   UC (*(s - 2)) == 'T')) ||
			  (UC (*(s - 1)) == 'S')) ||
			  (UC (*(s - 1)) == 'O' &&
			  UC (*(s - 2)) == 'G'))
			{
			  if (UC (*(s - 1)) == *(s - 1))
			    *s = 'E';
			  else
			    *s = 'e';
			  s++;
			}
              
                      /* This does 'You are' vs 'Bob is' */
                      /* Need to do are@s */
     
                      if (s - buf > 2 &&
                          UC (*(s - 1)) == 'E' &&
                          UC (*(s - 2)) == 'R' &&
                          UC (*(s - 3)) == 'A' &&
                          *(s - 4) == ' ')
                        {
                          if (*(s - 3) == UC (*(s - 3)) )
                            {
                              *(s - 3) = 'I';
                              *(s - 2) = 'S';
                            }
                          else
                            {
                              *(s - 3) = 'i';
                              *(s - 2) = 's';
                            }
                          s--;
                        }
		      /* These change fly fry try spy into
			 flies fries tries spies...add more if
			 needed. I am not an English major :P 
			 I think the rule is "consonant Y" becomes
			 "cons ies" and "vowel y" becomes "vow y s" */
		      
		      if (UC(*(s - 1)) == 'Y' &&
			  (UC (*(s - 2)) != 'A' &&
			   UC (*(s - 2)) != 'E' &&
			   UC (*(s - 2)) != 'I' &&
			   UC (*(s - 2)) != 'O' &&
			   UC (*(s - 2)) != 'U'))
			{
			  if (UC (*(s - 1)) == *(s - 1))
			    {
			      *(s - 1) = 'I';
			      *s = 'E';
			    }
			  else
			    {
			      *(s - 1) = 'i';
			      *s = 'e';
			    }
			  s++;
			}
		      if (UC (*(s - 2)) == *(s - 2))
			{
			  *s = 'S';
			}
		      else
			*s = 's';
		      s++;
		    }
		  else 
		    {
		      
		      /* this is where we add the him/her/it/name/his
			 etc for the th, use, and vict. Note that if
			 these things are missing, we just put
			 error messages instead of crashing (hopefully) 
			 The digit tells which THING it is, and the
			 next letter tells what sort of word we need.
			 'N' returns the name of the thing as seen
			 by the viewer. 'M' returns him/her/it/your
			 'S' returns his/her/its/your 'H' returns
			 he/she/it/you, and 'P' returns
			 the name's your, its, something's...possessive
			 names. As in: Your slash hits the fido vs.
			 Bob's slash hits the fido. Or Fred's slash
			 hits you.  The himself_herself returns
			 himself/herself/itself IF th == curr or else
			 it returns NAME (curr) */

		      if (*t == '1')
			{
			  r = "<Th>";
			  curr = th;
			}
		      else if (*t == '2')
			{
			  curr = use;
			  r = "<Use>";
			}
		      else if (*t == '3')
			{
			  curr = vict;
			  r = "<Vict>";
			}
		      else
			{
			  r = "<BadNum>";
			  curr = NULL;
			}
		      t++;
		      if (curr)
			{
			  if (UC (*t) == 'N')
			    r = name_seen_by (to, curr);
			  else if (UC (*t) == 'H')
			    r = he_she (to, curr);
			  else if (UC (*t) == 'M')
			    r = him_her (to, curr); 
			  else if (UC (*t) == 'S')
			    r = his_her (to, curr); 
			  else if (UC (*t) == 'P')
			    r = pname (to, curr);
			  else if (UC (*t) == 'F')
			    r = himself_herself (th, to, curr);
			  else
			    r = "<BadLetter>";
			}
		      for (u = r; *u; u++, s++)
			*s = *u;
		    }
		} /* If you're using the KDE client and you get a message
		     sent to you, clean up the <'s. */
	      else if (*t == '<' && USING_KDE (to))
		{
		  *s++ = '&';
		  *s++ = 'l';
		  *s++ = 't';
		  *s++ = ';';
		}	      
	      else
		{
		  *s = *t;
		  s++;
		}
	    }
	  *s = '\0';
	  for (s = actret; *s; s++)
	    {
	      if (*s == '\x1b')
		{
		  while (*s != 'm' && *s)
		    s++;
		  if (!*s)
		    s--;
		}
	      else if (isalpha (*s))
		{
		  *s = UC(*s);
		  break;
		}
	    } 
	  if (type == TO_COMBAT && current_damage_amount > 0 && 
	      IS_PC (to) &&
	      IS_PC2_SET (to, PC2_NUMBER_DAM))
	    sprintf (actret + strlen (actret), " \x1b[1;31m[\x1b[1;37m%d\x1b[1;31m]", current_damage_amount);
	  strcat (actret, "\x1b[0;37m\n\r");
	  if (chan && IS_PC (to) && currchan >= 0 &&
	      currchan < MAX_CHANNEL) /* Do scrollbacks */
	    {	 
	      SCROLLBACK *new_sback;
	      to->pc->sback_pos[currchan] = 
		(to->pc->sback_pos[currchan] + 1) % MAX_SCROLLBACK;
	      free_scrollback (to->pc->sback[currchan][to->pc->sback_pos[currchan]]);
	      new_sback = new_scrollback();
	      strncpy (new_sback->text, actret, MAX_SCROLLBACK_LENGTH);
              new_sback->text[MAX_SCROLLBACK_LENGTH-1] = '\n';
              new_sback->text[MAX_SCROLLBACK_LENGTH] = '\r';
	      to->pc->sback[currchan][to->pc->sback_pos[currchan]] = new_sback;
	    }	  
	  
	 
	  stt (actret, to);
          if (type == TO_CHAR || type == TO_VICT)
            break;
	}
      if (type == TO_CHAR || type == TO_VICT)
	break;
    }
  return;
}


/* This command lets you add a person to your ignore list, remove them
   from it, or list the people you have ignored out. */

void
do_ignore (THING *th, char *arg)
{
  int i, num = 0;
  bool found = FALSE;
  char buf[STD_LEN];
  char *t;

  
  if (!IS_PC (th))
    return;
  
  if (arg[0] == '\0')
    {
      stt ("\n\rIgnored people:\n\n\r", th);
      buf[0] = '\0';
      for (i = 0; i < MAX_IGNORE; i++)
	{
	  if (th->pc->ignore[i] && *th->pc->ignore[i])
	    {
	      found = TRUE;
	      sprintf (buf + strlen(buf), "%-15s", th->pc->ignore[i]);
	      if (++num % 4 == 0)
		{
		  strcat (buf, "\n\r");
		  stt (buf, th);
		  buf[0] = '\0';
		}
	    }
	}
      if (buf[0])
	{
	  strcat (buf, "\n\r");
	  stt (buf, th);
	}
      if (!found)
	{
	  stt ("None.\n\r", th);
	}
      return;
    }
      
  for (t = arg; *t; t++)
    {
      if (LC (*t) < 'a' || LC (*t) > 'z')
	{
	  stt ("You can only ignore NAMES made up of all letters.\n\r", th);
	  return;
	}
    }
  
  for (i = 0; i < MAX_IGNORE; i++)
    {
      if (!str_cmp (th->pc->ignore[i], arg))
	{
	  free_str (th->pc->ignore[i]);
	  th->pc->ignore[i] = nonstr;
	  stt ("Ignore removed.\n\r", th);
	  return;
	}
    }
  
  if (!str_cmp (arg, NAME (th)))
    {
      stt ("Duh...you try to ignore yourself. We try to ignore you too, but for some reason you're still here. Go figure.\n\r", th);
      return;
    }
  
  
  for (i = 0; i < MAX_IGNORE; i++)
    {
      if (!th->pc->ignore[i] || !*th->pc->ignore[i])
	{
	  th->pc->ignore[i] = new_str (arg);
	  stt ("Ignore added.\n\r", th);
	  return;
	}
    }
  
  
  stt ("You have no free ignore slots!!! Try removing some of the old ignores you don't need.\n\r", th);
  return;
}
  

/* This tells if the listener is ignoring the talker or not. */

bool
ignore (THING *listener, THING *talker)
{
  int i;
  if (!listener || !IS_PC (listener) || !talker ||
      !IS_PC (talker) || LEVEL (talker) == MAX_LEVEL)
    return FALSE;
  for (i = 0; i < MAX_IGNORE; i++)
    if (LC (*listener->pc->ignore[i]) == *talker->name &&
	!str_cmp (listener->pc->ignore[i], talker->name))
      return TRUE;
  return FALSE;
}

/* Someone on some newsgroup posted this idea.. real simple, real cute
   but they say it's interesting...so here goes. */
void
do_think (THING *th, char *arg)
{
  char buf[STD_LEN];
  if (!arg || !*arg)
    {
      stt ("You think deeply.\n\r", th);
      return;
    }
  sprintf (buf, "You think '%s'\n\r", arg);
  stt (buf, th);
  return;
}

/* These are all used mostly in act(). They give the he/she/him/it stuff
   so you don't just have to use real names. */

char *
he_she (THING *viewer, THING *victim)
{
  int sex;
  if (!viewer || !victim || !can_see (viewer, victim))
    return "something";
  if (viewer == victim)
    return "you";
  sex = SEX (victim);
  if (sex == 1)
    return "she";
  else if (sex == 2)
    return "he";
  return "it";
}


char *
him_her (THING *viewer, THING *victim)
{
  int sex;
  if (!viewer || !victim || !can_see (viewer, victim))
    return "it";
  if (viewer == victim)
    return "you";
  sex = SEX (victim);
  if (sex == 1)
    return "her";
  else if (sex == 2)
    return "him";
  return "it";
}


char *
his_her (THING *viewer, THING *victim)
{
  int sex;
  if (!viewer || !victim || !can_see (viewer, victim))
    return "its";
  if (viewer == victim)
    return "your";
  sex = SEX (victim);
  if (sex == 1)
    return "her";
  else if (sex == 2)
    return "his";
  return "its";
}



char *
name_seen_by (THING *viewer, THING *victim)
{
  static char ret[200];
  RACE *race, *align;
  ret[0] = '\0';
  if (!viewer || !victim || !can_see (viewer, victim))
    return "something";
  if (viewer == victim)
    return "you";
  if (IS_PC (viewer) && IS_PC (victim) && DIFF_ALIGN (viewer->align, victim->align) && LEVEL (viewer) < BLD_LEVEL && LEVEL (victim) < BLD_LEVEL &&
      (race = FRACE(victim)) != NULL && (align = FALIGN(victim)) != NULL)
    {
      sprintf (ret, "+* %s %s %s *+", 
	       a_an (align->name), 
	       align->name,
	       race->name);
    }
  else
    strcpy (ret, NAME (victim));
  return ret;
}

char *
pname (THING *viewer, THING *victim)
{
  static char ret[200];
  char *t;
  RACE *race, *align;
  ret[0] = '\0';
  if (!viewer || !victim || !can_see (viewer, victim))
    return "something's";
  if (viewer == victim)
    return "your";
  if (IS_PC (viewer) && IS_PC (victim) && DIFF_ALIGN (viewer->align, victim->align) && LEVEL (viewer) < BLD_LEVEL && LEVEL (victim) < BLD_LEVEL &&
      (race = FRACE(victim)) != NULL && (align = FALIGN(victim)) != NULL)
    {
      sprintf (ret, "+* %s %s %s *+", 
	       a_an (align->name), 
	       align->name,
	       race->name);
    }
  else
    strcpy (ret, NAME (victim));
  t = ret + strlen (ret);
  if (t == ret)
    return ret;
  if (*t == 's' || *t == 'S')
    strcat (ret, "'");
  else if (UC (*t) != *t)
    strcat (ret, "'S");
  else 
    strcat (ret, "'s");
  return ret;
}

/* This is used for words following prepositions which are 
   in cases where victim == th in act (). If victim != th,
   then we use the victim's name, else we use himself/herself/
   itself. */

char *
himself_herself (THING *th, THING *viewer, THING *victim)
{
  if (!th || !viewer || !victim)
    return "itself";
  if (!can_see (viewer, victim))
    return "something";
  if (viewer == victim && th == victim)
    return "yourself";
  else if (viewer == victim)
    return "you";
  if (th != victim)
    return name_seen_by (viewer, victim);
  if (SEX (victim) == 1)
    return "herself";
  else if (SEX (victim) == 2)
    return "himself";
  return "itself";
}


/* This command lets you order your pets if they're in the same
   room as you. */

void
do_order (THING *th, char *arg)
{
  char arg1[STD_LEN];
  bool all = FALSE, found = FALSE;
  THING *vict = NULL, *victn;
  VALUE *pet;
  
  if (!th->in)
    return;
  
  arg = f_word (arg, arg1);
  
  if (arg1[0] == '\0' || !*arg)
    {
      stt ("Syntax: order <follower(s)> <some command>\n\r", th);
      return;
    }
  
  /* Get the pet(s) we are ordering. */
  
  if (!str_cmp (arg1, "pet") || !str_cmp (arg1, "follower") ||
      !str_cmp (arg1, "pets") || !str_cmp (arg1, "followers") ||
      !str_cmp (arg1, "all"))
    {
      all = TRUE;
      vict = th->in->cont;
    }
  else if ((vict = find_thing_in (th, arg1)) == NULL)
    {
      stt ("Syntax: order <follower(s)> <some command>\n\r", th);
      return;
    }
  
  
  for (; vict != NULL; vict = victn)
    {
      victn = vict->next_cont;
      
      /* Check if the victim has a pet value and belongs to th... */
      
      if ((pet = FNV (vict, VAL_PET)) == NULL ||
	  !pet->word || str_cmp (pet->word, NAME (th)))
	{
	  if (!all)
	    {
	      stt ("You can't order that creature around!\n\r", th);
	      return;
	    }
	  continue;
	}
      
      /* This creature does belong to th, so it does what it's told. */
      
      found = TRUE;
      act ("@1n order@s @3n to '@t'", th, NULL, vict, arg, TO_ALL);
      interpret (vict, arg);
      
      
      /* Return if we just ordered one thing around. */
      
      if (!all)
	return;
      
    }
  
  if (!found)
    stt ("You don't see anyone around here willing to listen to your orders!\n\r", th);
  
  return;
}
