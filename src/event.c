#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "note.h"
#include "script.h"
#include "track.h"
#include "event.h"
#include "rumor.h"
#ifdef USE_WILDERNESS
#include "wildalife.h"
#endif

static EVENT *current_event = NULL;   /* The event being executed. This is 
				  here so that if the thing doing
				  the executing is freed, the event
				  will also be freed. */
static EVENT *next_current_event = NULL; 

/* This creates a new event and returns it. */

EVENT *
new_event (void)
{
  EVENT *newevent;
  
  if (!event_free)
    ADD_TO_MEMORY_POOL(EVENT,event_free,event_count);
  newevent = event_free;
  event_free = event_free->next;
  bzero (newevent, sizeof (EVENT));
  newevent->th_prev = NULL;
  newevent->th_next = NULL;
  newevent->prev = NULL;
  newevent->next = NULL;
  newevent->callback = NULL;
  newevent->arg = nonstr;
  newevent->th = NULL;
  newevent->attacker = NULL;
  RBIT (newevent->flags, EVENT_DEAD);
  return newevent;
}


/* This frees an event and puts it on the free list for future use. */
void
free_event (EVENT *event)
{
  if (!event || IS_SET (event->flags, EVENT_DEAD))
    return;
  
  /* These are used to update the event list if the event we're using
     currently gets freed. These updates MUST come before you remove
     the event from the list or from the thing or bzero it or
     else you end up with memory leaks and infinite loops and
     other bad stuff. */

  if (event == current_event)
    current_event = NULL;
  if (event == next_current_event)
    next_current_event = next_current_event->next;
  
  
  
  event_from_thing (event);
  event_from_list (event);
  free_str (event->arg);
  event->arg = nonstr;
  event->th = NULL;
  event->flags = EVENT_DEAD;
  event->times_left = 0;
  event->next = event_free;
  event_free = event;
  return;
}

/* This creates a new event based on the following arguments that
   directly set the event. Times left and arg are not set here
   because they're "special" and not normally used. */

EVENT *
create_repeating_event (THING *th, int delay, EVENT_FUNCTION *callback)
{
  EVENT *event = new_event();
  
  if (!event)
    return NULL;
  
  if (delay < 1)
    delay = 1;
  event->th = th;
  event->time = times_through_loop + nr (1, delay);
  event->delay = delay;
  event->flags = EVENT_REPEAT;
  event->callback = callback;
  if (event->th)
    event_to_thing (event, th);
  event_to_list (event);
  return event;
}
  
  

/* This adds an event to the list of events on a thing. Note that
   these are NOT ordered. The only reason they're even on the thing
   is to make it easier to remove them when and if the thing gets
   freed. */


void
event_to_thing (EVENT *event, THING *th)
{
  if (!event || !th)
    return;
  
  event->th = th;
  event->th_next = th->events;
  if (th->events)
    th->events->th_prev = event;
  th->events = event;
  return;
}
    

/* This removes an event from a thing. */

void
event_from_thing (EVENT *event)
{
  if (!event)
    return;
  
  if (event->th_prev)
    event->th_prev->th_next = event->th_next;
  if (event->th_next)
    event->th_next->th_prev = event->th_prev;
  if (event->th && event == event->th->events)
    event->th->events = event->th_next;
  
  event->th_prev = NULL;
  event->th_next = NULL;
  return;
}


/* This removes an event from the table of events. */

void
event_from_list (EVENT *event)
{
  int num;
  if (!event)
    return;

  num = event->time % EVENT_HASH;
  if (event->prev)
    event->prev->next = event->next;
  if (event->next)
    event->next->prev = event->prev;
  if (event == event_hash[num])
    event_hash[num] = event->next;
  event->next = NULL;
  event->prev = NULL;  
  return;
}

/* This adds an event to the appropriate spot in the event list. */

void
event_to_list (EVENT *event)
{
  int num, slot = 0;
  EVENT *prev, *prevn;
  
  if (!event)
    return;
  
  num = event->time % EVENT_HASH;
  
  
  /* If the head of the list doesn't exist for this number or if
     the time on the head is larger than the time for this event, then
     add it to the list as the head. */
  
  if (!event_hash[num] || event_hash[num]->time >= event->time)
    {
      event->next = event_hash[num];
      if (event_hash[num])
	event_hash[num]->prev = event;
      event_hash[num] = event;
      return;
    }

  /* Otherwise search down until we reach a spot just before the end
     of the list or just before the times on the list items get
     larger than that of the current event being added. */
  
  
  for (prev = event_hash[num]; prev; prev = prevn)
    {
      slot++;
      prevn = prev->next;
      if (!prev->next || prev->next->time >= event->time)
	{
	  event->next = prev->next;
	  if (prev->next)
	    prev->next->prev = event;
	  event->prev = prev;
	  prev->next = event;
	  {
	    char errbuf[STD_LEN];
	    sprintf (errbuf, "EVENT PLACED AFTER %d\n", slot);
	    log_it (errbuf);
	  }
	  return;
	}
    }

  log_it ("ERROR EVENT NOT ADDED TO LIST!\n");
  return;
}
  

/* This removes all events from a single thing. */

void
remove_events_from_thing (THING *th)
{
  EVENT *event, *eventn;
  
  if (!th)
    return;
  
  for (event = th->events; event; event = eventn)
    {
      eventn = event->th_next;
      free_event (event);
    }
  th->events = NULL;
  return;
}


/* This sets up all of the "standard" events a thing will need to 
   have. */

void
add_events_to_thing (THING *th)
{
  VALUE *cast;
  if (!th)
    return;
  
  /* Most things only get the general thing update. */
  
  /* Mobiles get the thing update. */
  
  if (!IS_ROOM (th) && !IS_AREA (th) &&
      (CAN_FIGHT (th) || CAN_MOVE (th)))
    create_repeating_event (th, PULSE_THING, update_thing);
    
  /* Casters get the cast update. */
  if ((cast = FNV (th, VAL_CAST)) != NULL)
    create_repeating_event (th, PULSE_THING, find_spell_to_cast);
    
  /* Everyone gets an hourly update. */
  create_repeating_event (th, PULSE_HOUR, update_thing_hour);
  
  /* Players get a fast update. */
  if (IS_PC (th))
    create_repeating_event (th, PULSE_FAST, update_fast);
  
  return;
}

/* This sets up the "global" events that will be used by the game
   to do various things. */

void
set_up_global_events (void)
{
  /* Only set up these events during bootup. */
  
  if (!BOOTING_UP)
    return;
  
  /* Add the hourly updates. */
 create_repeating_event (NULL, PULSE_HOUR, update_hour);
 create_repeating_event (NULL, PULSE_HOUR, update_png);
 create_repeating_event (NULL, PULSE_HOUR, update_rumors);
 create_repeating_event (NULL, PULSE_HOUR, update_raids);
 create_repeating_event (NULL, PULSE_HOUR, clean_up_tracks);
 create_repeating_event (NULL, PULSE_HOUR, update_alignments);
 create_repeating_event (NULL, PULSE_HOUR, update_alignment_pkill_bonuses);
 
  
 create_repeating_event (NULL, PULSE_AUCTION, update_auctions);
 
#ifdef USE_WILDERNESS
  /* Add the wilderness sector update */
 
 create_repeating_event (NULL, PULSE_SECTOR, update_wilderness_sectors_event);
#endif
 
 /* Add societies update. */
 create_repeating_event (NULL, UPD_PER_SECOND, update_societies_event);
 
  return;
}

/* This goes down the list of current events and executes them. */
/* Current_event and next_current_event are globals since I don't
   have a nice list template to use here to make sure I don't screw
   up the list when I remove arbitrary elements. It gets set to NULL 
   in free_event if it is the event getting deleted. */

void
update_events (void)
{
  int delay;
  for (current_event = event_hash[times_through_loop % EVENT_HASH];
       current_event; current_event = next_current_event)
    {
      next_current_event = current_event->next;
      if (current_event->time <= times_through_loop)
	{
	  event_from_list (current_event);
	  exec_event(current_event);
	  
	  /* It's possible that the event will be deleted if the
	     thing calling it gets nuked. If that doesn't happen,
	     then add it back into the list at the new spot. */
	  if (current_event)
	    {
	      if ((IS_SET (current_event->flags, EVENT_REPEAT) ||
		   --current_event->times_left > 0))
	      {
		  
		  /* Haste and slow affect the speed at which events
		     are reset. This may be changed to haste/slow as
		     percents that affect things with finer granularity. */
		  
		  delay = current_event->delay;
		  /* Maybe let this haste/slow thing go in. Dunno. */
		  /* if (current_event->th)
		    {
		      if (IS_AFF (current_event->th, AFF_HASTE))
			delay = 5*delay/6;
		      else if (IS_HURT (current_event->th, AFF_SLOW))
			delay = 7*delay/6;
			} */
		  current_event->time = times_through_loop + MAX (1, delay);
		  event_to_list (current_event);
		}
	      else
		free_event (current_event);
	    }
	}
    }
  current_event = NULL;
  next_current_event = NULL;
  return;
}

/* This executes an event. Many of the events are executed by "the world"
   and don't need a thing. The thing update and fast update and
   combat update all require a thing to work. */

void
exec_event (EVENT *event)
{
  THING *th;
  if (!event)
    return;
  
  th = event->th;
  if (IS_SET (event->flags, EVENT_COMMAND))
    do_command_event (event);
  else if (IS_SET (event->flags, EVENT_DAMAGE))
    do_damage_event (event);
  else if (event->callback)
    {
      if (event->th)
	  (*event->callback) (event->th);
      else
	(*event->callback) ();
    }
  /* If this was a thing event and the thing gets nuked...
     Make sure this is for the thing BEFORE the event. If it gets
     killed, it may not be in there anymore (As the events will all
     be cleared). */
  if (th && !th->in)
    {
      RBIT (event->flags, EVENT_REPEAT);
      event->times_left = 0;
    }
  return;
}





/* This actually carries out a command event. */

void
do_command_event (EVENT *event)
  {
    if (!event || !IS_SET (event->flags, EVENT_COMMAND) || !event->th)
    {
      free_event (event);
      return;
    }
  
  
  SBIT (server_flags, SERVER_DOING_NOW);
  interpret (event->th, event->arg);
  RBIT (server_flags, SERVER_DOING_NOW);
  free_event (event);
  return;
}

/* This adds a command event to a thing. You specify the thing doing
   the command, the argument (including the commandname) to be
   sent to the interpreter, and the number of ticks before the 
   command gets executed. */

void
add_command_event (THING *th, char *arg, int ticks)
{
  EVENT *event;
  int hurt_bits;
  if (!th || !arg || !*arg)
    return;
  
  hurt_bits = flagbits (th->flags, FLAG_HURT);

  /* Forget will keep you from finishing a command once in a while. */
  
  if (IS_PC (th) && IS_SET (hurt_bits, AFF_FORGET) && nr (1,7) == 3)
    return;
  event = new_event();
  event->flags = EVENT_COMMAND;
  
  /* Don't allow delayed commands longer than 400 ticks? Too cheater
     otherwise. */
  
  event->time = times_through_loop;
  
  /* Haste and slow affect the amount of time it takes to do things. */
  
  if (IS_PC (th))
    {
      if (IS_AFF (th, AFF_HASTE))
	event->time -= nr (0, ticks/3);
      else if (IS_SET (hurt_bits, AFF_SLOW))
	event->time += nr (ticks/2, ticks);
      if (LEVEL(th) < BLD_LEVEL || !IS_PC1_SET (th, PC_HOLYSPEED))
	event->time += MID (1, ticks, 60*UPD_PER_SECOND);
    }
  else
    event->time += MID (1, ticks, 60*UPD_PER_SECOND);
  
  event->th = th;
  event->arg = new_str (arg);
  event_to_thing (event, th);
  event_to_list (event);
  return;
}

		 
  
  
/* This adds an event to make a pc track something. */

void
add_track_event (THING *th)
{
  EVENT *event;
  if (!th )
    return;

  /* Check for a current track event. If the thing is a pc, add the
     tracking time to the event again and resend it to the list again.
     If it's a mob tracking, just leave it alone and return. */
  
  for (event = th->events; event; event = event->th_next)
    {
      if (event->callback == track_victim)
	{
	  if (IS_PC (th))
	    {
	      event_from_list (event);
	      event->time = times_through_loop + TRACK_TICKS;
	      event_to_list (event);
	    }
	  return;
	}
    }
  
  
  event = new_event();
  event->th = th;
  event->flags = EVENT_TRACK;
  event->callback = track_victim;
  event->time = times_through_loop + TRACK_TICKS;
  event_to_thing (event, th);
  event_to_list (event);
  return;
}
  
  
/* This removes events of a certain type from a thing. Command events get
   a little jazzing up so that the player who interrupts a command
   knows why it happened. :) */

void
remove_typed_events (THING *th, int flags)
{
  EVENT *event, *eventn;
  char comname[STD_LEN];
  char buf[STD_LEN];
  
  for (event = th->events; event; event = eventn)
    {
      eventn = event->th_next;
      if (!IS_SET (event->flags, flags))
	continue;
      
      if (IS_SET (event->flags, EVENT_COMMAND))
	{
	  comname[0] = '\0';
	  if (event->arg && *event->arg)
	    f_word (event->arg, comname);
	  if (*comname)
	    {
	      sprintf (buf, "You stop %sing.\n\r", comname);
	      stt (buf, th);
	    }
	  if (th->position > POSITION_MEDITATING)
	    th->position = POSITION_STANDING;
	}
      free_event (event);
    }
  return;
}

/* This adds a damage event from a thing to another thing. */

void
add_damage_event (THING *attacker, THING *th, SPELL *spl)
{
  EVENT *event;
  if (!attacker || !th || !spl)
    return;

  /* Only repeated spells with delays get events. */
  
  if (spl->repeat < 1 || spl->delay < 1)
    return;
  
  /* Check for the same spell cast by the same person. If it's
     there, don't allow them to recast it. This is to keep things
     from getting TOO cheater either with helpful or harmful spells. */

  for (event = th->events; event; event = event->th_next)
    {
      if (IS_SET (event->flags, EVENT_DAMAGE) &&
	  !str_cmp (event->arg, spl->name) &&
	  event->attacker == attacker)
	return;
    }

  
  
  event = new_event();
  event->th = th;
  event->attacker = attacker;
  event->times_left = spl->repeat;
  event->time = times_through_loop + spl->delay*UPD_PER_SECOND;
  event->delay = spl->delay*UPD_PER_SECOND;
  event->flags = EVENT_DAMAGE;
  event->arg = new_str (spl->name);
  event_to_list (event);
  event_to_thing (event, th);
  return;
}


/* This carries out a damage event. */

void
do_damage_event (EVENT *event)
{
  SPELL *spl;
  
  if (!event || !event->th || !event->attacker ||
      !event->attacker->in ||
      !event->arg || !*event->arg ||
      (spl = find_spell (event->arg,  0)) == NULL)
    {
      free_event (event);
      return;
    }
  
  cast_spell (event->attacker, event->th, spl, FALSE, FALSE, event);
  return;
}



/* This makes an event to delete a thing at a future time. */

void
add_free_thing_event (THING *th)
{

  EVENT *event;
  
  if (!th || IS_PC (th))
    return;
  
  if ((event = create_repeating_event (th, 60*UPD_PER_SECOND, free_thing_final_event)) != NULL)
    {
      event->flags = 0;
      event_from_thing (event);
    }
  return;
}










