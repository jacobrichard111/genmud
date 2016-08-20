#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "event.h"

/* This lets you play with society children to get rewards. */

void
do_play (THING *th, char *arg)
{
  THING *child;
  char arg1[STD_LEN];
  VALUE *socval;
  SOCIETY *soc;
  if (!th || !IS_PC (th))
    return;

  if (!arg || !*arg)
    {
      stt ("Play with <childname>\n\r", th);
      return;
    }

  arg = f_word (arg, arg1);
  
  if ((child = find_thing_in (th, arg)) == NULL ||
      /* This short circuit is here since there's no nice way to say
	 "you can't play with yourself". :P */
      child == th)
    {
      stt ("Play with <childname>\n\r", th);
      return;
    }
  

  if (!CAN_TALK (child))
    {
      stt ("They don't want to play.\n\r", th);
      return;
    }

  
  if ((socval = FNV (child, VAL_SOCIETY)) == NULL ||
      (soc = find_society_num (socval->val[0])) == NULL)
    {
      if (is_named (child, "child") || is_named (child, "kid"))
	{
	  act ("@1n play@s with @3f.", th, NULL, child, NULL, TO_ALL);
	  return;
	}
      stt ("They don't want to play.\n\r", th);
      return;
    }

  /* Have a society now. Make sure this is a child. */

  if (!IS_SET (socval->val[2], CASTE_CHILDREN) || 
      child->position <= POSITION_SLEEPING ||
      is_hunting (child) ||
      /* Children don't play when they're being led by an overlord. */
      IS_SET (soc->society_flags, SOCIETY_OVERLORD))
    {
      stt ("They don't want to play.\n\r", th);
      return;
    }

  /* Check for the proper alignment. */
  
  if (DIFF_ALIGN (child->align, th->align) ||
      child->align == 0)
    {
      THING *adult;

      if ((adult = find_society_member_nearby (child, BATTLE_CASTES, 5)) != NULL)
	{
	  start_hunting (child, NAME(adult), HUNT_HEALING);
	  hunt_thing (child, 20);
	}
      else
	stt ("They don't want to play.\n\r", th);
      return;
    }
  
  act ("@1n play@s with @3n.", th, NULL, child, NULL, TO_ALL);
  add_society_reward (th, child, REWARD_PLAY, nr (5,10));
  
  /* Now the child will do some stuff. */
  
  if (nr (1,3) == 2)
    {
      switch (nr (1,4))
	{
	  case 1:
	    add_command_event (child, "smile", nr (10,30));
	    break;
	  case 2:
	    add_command_event (child, "giggle", nr (10,30));
	    break;
	  case 3:
	    add_command_event (child, "laugh", nr (10,30));
	    break;
	  case 4:
	  default:
	    add_command_event (child, "grin", nr (10,30));
	    break;
	}
      
      /* Now see if the child wanders away or something. */
      
      if (nr (1,2) == 2)
	{
	  add_command_event (child, dir_name[nr(0, REALDIR_MAX-1)], nr (35, 50));   
	  if (nr (1,2) == 1)
	    {
	      add_command_event (child, "yawn", nr (60, 80));
	      add_command_event (child, "sleep", nr (100,120));
	    }
	}
    }
  th->pc->wait += 70;
  return;
}
