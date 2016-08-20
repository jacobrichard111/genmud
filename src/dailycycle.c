#include <stdlib.h>
#include <ctype.h>
#include "serv.h"
#include "track.h"
#include "dailycycle.h"

/* This is the list of hourly cycles in an array so we can check things
   easily. */

static const int cycle_hours[CYCLE_HOUR_MAX] = 
  {
    CYCLE_WAKE_UP,
    CYCLE_AM_WORK,
    CYCLE_LUNCH,
    CYCLE_PM_WORK,
    CYCLE_DINNER,
    CYCLE_HOME,
    CYCLE_SLEEP,
  };



/* This updates the daily cycle for a thing and makes it do stuff
   depending on what time it is. */

bool
update_daily_cycle (THING *th)
{
  VALUE *cycle;
  /* What hour we're effectively in during the daily cycle for this
     thing. The way this works is we take the real hour and subtract 
     the thing's wake up hour, then take it mod NUM_HOURS. */
  int effective_hour;
  int should_be_in_stage = 0, current_stage = 0;
  int i;
  THING *room;
  
  if (!th || IS_PC (th) || !th->in || is_hunting(th) ||
      IS_SET (th->thing_flags, TH_NO_MOVE_SELF))
    return FALSE;

  /* Make sure that it has a daily cycle variable. */
  
  if ((cycle = FNV (th, VAL_DAILY_CYCLE)) == NULL)
    return FALSE;

  if (!wt_info)
    return FALSE;
  
  /* This is where the mob is effectively during its day. */
  effective_hour = (wt_info->val[WVAL_HOUR] + NUM_HOURS - cycle->val[0]) % NUM_HOURS;
  
  /* See where we are in the cycle. Only loop to CYCLE_HOUR_MAX - 1 since
     if it ever gets there, then we MUST be in the sleep cycle. */
  for (i = 0; i < CYCLE_HOUR_MAX - 1; i++)
    {
      if (effective_hour >= cycle_hours[i] &&
	  effective_hour < cycle_hours[i+1])
	break;
    }
  
  /* i must be from 0 to CYCLE_HOUR_MAX - 1. */

  current_stage = cycle->val[1];
  should_be_in_stage = i;

  /* If these are the same, then don't do anything. These also send some 
     little messages to let the players know that something's happening. */
  
  if (current_stage == should_be_in_stage)
    {
      switch (cycle->val[1])
	{
	  case CYCLE_HOUR_AM_WORK:
	  case CYCLE_HOUR_PM_WORK:
	    if (nr (1,5) == 3)
	      act ("@1n work@s steadily away.", th, NULL, NULL, NULL, TO_ALL);
	    break;	
	  case CYCLE_HOUR_LUNCH:
	  case CYCLE_HOUR_DINNER:
	    if (nr (1,6) == 2)
	      act ("@1n eat@s @1s @t.", th, NULL, NULL,
		   (cycle->val[1] == CYCLE_HOUR_LUNCH ? "lunch" : "dinner"));
	  default:
	    break;
	}
      return FALSE;
    }
  
  /* Now start to deal with the different cases. */
  
  cycle->val[1] = should_be_in_stage;
  
  switch (cycle->val[1])
    {
      case CYCLE_WAKE_UP:
	do_wake (th, "");
	if (nr (1,3) == 2)
	  add_command_event (th, "yawn", nr (UPD_PER_SECOND, 4*UPD_PER_SECOND));
	if (nr (1,3) == 1) 
	  add_command_event (th, "stretch", nr (UPD_PER_SECOND, 4*UPD_PER_SECOND));
	break;
	/* Go to work if not there. */
      case CYCLE_HOUR_AM_WORK:
      case CYCLE_HOUR_PM_WORK:       
	if (th->in->vnum != cycle->val[3])
	  start_hunting_room (th, cycle->val[3], HUNT_HEALING);
      case CYCLE_HOUR_HOME:
      case CYCLE_HOUR_SLEEP:
	if (th->in->vnum != cycle->val[2])
	  start_hunting_room (th, cycle->val[2], HUNT_HEALING);
	else if (cycle->val[1] == CYCLE_HOUR_SLEEP)
	  do_sleep (th, "");
	break;
      case CYCLE_HOUR_LUNCH:
      case CYCLE_HOUR_DINNER:
	if (!is_an_inn (th->in))
	  {
	    if ((room = find_in_near (th)) != NULL)
	      start_hunting_room (th, room->vnum, HUNT_HEALING);
	  }
      default:
	break;
    }
  




}
