#include <sys/types.h>
#include<ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "serv.h"
#include "society.h"
#include "event.h"
#include "track.h"
#include "rumor.h"



/* This gives a society under drastic assault the chance to run
   away and start afresh in a new area. It looks for an emply area
   and then has all society members attempt to go there as quickly
   as possible. */

void
society_run_away (SOCIETY *soc)
{
  THING *area, *room, *mob, *oldarea;
  int count, num_choices = 0, num_chose = 0;
  int min_room_vnum = 2000000000, max_room_vnum = 0;
  int caste, tier, vnum, i;
  int curr_start_room, curr_end_room;
  SOCIETY *soc2;
  VALUE *socval, *build;
  /* Don't go unless we're under a big alert and the society has been
     really beat up lately. Only allow one abandon every 10 hours rl or
     so. */
  
  if (soc->abandon_hours > 0)
    {
      soc->abandon_hours--;
      return;
    }
  if (soc->alert < 10 ||
      soc->recent_maxpop > 40 ||
      soc->population > soc->recent_maxpop/3 ||
      nr (1,60) != 50)
    return;
  
  

  /* Find all areas with societies atm...*/

  unmark_areas();
  oldarea = find_area_in (soc->room_start);
  for (soc2 = society_list; soc2; soc2 = soc2->next)
    {
      if ((area = find_area_in (soc2->room_start)) != NULL)
	SBIT (area->thing_flags, TH_MARKED);
    }
  
  /* Now pick a new area to go to. */
  
  for (count = 0; count < 2; count++)
    {
      for (area = the_world->cont; area; area = area->next_cont)
	{
	  /* This has to be an empty (unmarked) area and it has to be
	     a regular area and it has to be a "normal" area, not one
	     like water or astral. */
	  if (!IS_AREA (area) || IS_MARKED (area) ||
	      IS_AREA_SET (area, AREA_NOLIST | AREA_NOREPOP | AREA_NOSETTLE) ||
	      IS_ROOM_SET (area, BADROOM_BITS))
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    break;
	  num_chose = nr (1, num_choices);
	}
    }
  
  unmark_areas();
  if (!area)
    return;

  
  /* Have all society members move to the new area. */

  for (room = area->cont; room; room = room->next_cont)
    {
      if (!IS_ROOM (room))
	continue;
      
      if (room->vnum < min_room_vnum)
	min_room_vnum = room->vnum;
      if (room->vnum > max_room_vnum)
	max_room_vnum = room->vnum;
    }
  
  /* Make the min room vnum go up and max room vnum go down. */
  min_room_vnum += nr (10, (max_room_vnum-min_room_vnum)/4);
  max_room_vnum -= nr (10, (max_room_vnum-min_room_vnum)/3);
  
  curr_start_room = soc->room_start;
  curr_end_room = soc->room_end;
  
  /* Now move all mobs. */
  
  /* Now find the room to go to. */
  
  for (vnum = min_room_vnum; vnum <= max_room_vnum; vnum++)
    {
      if ((room = find_thing_num (vnum)) != NULL &&
	  !IS_ROOM_SET (room, BADROOM_BITS))
	break;
      room = NULL;
    }
 
  if (!room)
    return;
  
  soc->room_start = min_room_vnum;
  soc->room_end = max_room_vnum;
  for (caste = 0; caste < CASTE_MAX; caste++)
    {
      for (tier = 0; tier < soc->max_tier[caste]; tier++)
	{
	  vnum = soc->start[caste]+tier;
	  for (mob = thing_hash[vnum % HASH_SIZE]; mob; mob = mob->next)
	    {
	      if ((socval = FNV (mob, VAL_SOCIETY)) == NULL ||
		  socval->val[0] != soc->vnum)
		continue;
	      
	      /* Only society mobs now. Unsentinel them and make them
		 stop hunting and make them move to their new homeland. */

	      remove_flagval (mob, FLAG_ACT1, ACT_SENTINEL);
	      
	      /* Make them say "run away" a few times. */

	      for (i = 0; i < 4; i++)
		{
		  add_command_event (mob, "say Run Away! Run Away!",
				     nr (i*30+10, i*30+30));
		}
	      
	      
	      /* Exception to stop hunting is people on a raid who will
		 come home eventually. */
	      if (IS_SET (socval->val[2], BATTLE_CASTES) &&
		  socval->val[3] == WARRIOR_HUNTER &&
		  socval->val[4] > 0)
		continue;
	      /* Prisoner */
	      if (socval->val[5] > 0)
		continue;
	      stop_hunting (mob, TRUE);
	      start_hunting_room (mob, room->vnum, HUNT_HEALING);
	      
	      add_command_event (mob, "wake", nr (5,15));
	      
	    }
	}
    }
  
  /* Now clear the old city. */

  for (vnum = curr_start_room; vnum <= curr_end_room; vnum++)
    {
      if ((room = find_thing_num (vnum)) == NULL ||
	  (build = FNV (room, VAL_BUILD)) == NULL ||
	  build->val[0] != soc->vnum)
	continue;
      
      build->val[1] = SOCIETY_BUILD_ABANDONED;
    }
  
  /* Delete all raws. */

  for (i = 0; i < RAW_MAX; i++)
    soc->raw_curr[i] = 0;
  
  add_rumor (RUMOR_ABANDON, soc->vnum, (oldarea ? oldarea->vnum : 0), 0, 0);
  soc->abandon_hours = 500;
  return;
}
