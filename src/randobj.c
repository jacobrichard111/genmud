#include <stdio.h>
#include "serv.h"
#include "objectgen.h"
#include "randobj.h"
#include "worldgen.h"




/* This generates the randobj objects and then 
   seeds the randobj resets onto mobs all around the world. */


void 
worldgen_randobj_generate (void)

{
  THING *area, *obj;
  int tier = 0, count = 0, level = 0;
  
  int start_vnum = 0, vnum = 0;
  VALUE *randpop;
    
  
  /* Need to have the correct area there. */
  if ((area = find_thing_num (RANDOBJ_AREA_VNUM)) == NULL)
    return;
  
  /* Cannot have anything in it except for the starting room or else 
     it fails. */
  
  if (area->cont && area->cont->next_cont)
    return;
  
  /* Make sure the area is big enough. */

  if (area->max_mv - area->mv < (RANDOBJ_COUNT_PER_TIER*RANDOBJ_TIERS))
    {
      char buf[STD_LEN];
      sprintf (buf, "The randobj area %d needs at least %d nonroom thing slots in it for the randobj code to work!\n\r", RANDOBJ_AREA_VNUM, RANDOBJ_AREA_VNUM*RANDOBJ_TIERS);
      echo (buf);
      return;
    }

  start_vnum = area->vnum+area->mv+1;
  vnum = start_vnum;
  
  /* Now set up the randobj items. */

  level = RANDOBJ_TIER_LEVEL_JUMP;
  for (tier = 0; tier < RANDOBJ_TIERS; tier++)
    {
      for (count = 0; count < RANDOBJ_COUNT_PER_TIER; count++)
	{
	  objectgen (area, ITEM_WEAR_NONE, level, vnum, NULL);
	  vnum++;
	}
      level += RANDOBJ_TIER_LEVEL_JUMP;
    }

  /* Set up the randobj item. */
  if ((obj = find_thing_num (RANDOBJ_VNUM)) == NULL)
    {
      obj = new_thing();
      obj->vnum = RANDOBJ_VNUM;
      thing_to (obj, find_thing_num (START_AREA_VNUM));
      area = find_thing_num (1);
      area->thing_flags |= TH_CHANGED;
    }
  if (!obj)
    return;
  
  if ((randpop = FNV (obj, VAL_RANDPOP)) == NULL)
    {
      randpop = new_value();
      randpop->type = VAL_RANDPOP;
      add_value (obj, randpop);
    }
  
  randpop->val[0] = start_vnum;
  randpop->val[1] = RANDOBJ_COUNT_PER_TIER;
  randpop->val[2] = RANDOBJ_TIERS;
  randpop->val[3] = RANDOBJ_TIERS*RANDOBJ_TIER_LEVEL_JUMP;
  
  /* Now seed the resets on mobs within the game. */

  for (area = the_world->cont; area; area = area->next_cont)
    {
      if (area->vnum < WORLDGEN_START_VNUM)
	continue;
      
      /* Only talky fighty movey nonrooms get resets. */
      for (obj = area->cont; obj; obj = obj->next_cont)
	{
	  if (IS_ROOM (obj) ||
	      !CAN_MOVE (obj) ||
	      !CAN_FIGHT (obj) ||
	      !CAN_TALK (obj))
	    continue;

	  /* Add the reset. */

	  add_reset (obj, RANDOBJ_VNUM, 10, 1, 1);

	}
    }
      
  return;
	  
}


void 
clear_randobj_items (THING *th)
{
  THING *area, *obj;

  if (th && LEVEL(th) < MAX_LEVEL)
    return;

  if ((area = find_thing_num (RANDOBJ_AREA_VNUM)) == NULL)
    return;

  if (!area->cont)
    return;
  
  obj = area->cont->next_cont;

  while (obj)
    {
      obj->thing_flags |= TH_NUKE;
      obj = obj->next_cont;
    }
  area->thing_flags |= TH_CHANGED;
  if (th)
    stt ("Randobj items cleared.\n\r", th);
  return;
}
