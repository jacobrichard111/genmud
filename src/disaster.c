#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serv.h"
#include "society.h"


/* This creates various kinds of disasters that can crop up in
   the world. */

void
create_disaster (void)
{
 
  if (nr (1,20) != 17)
    return;

  /* Fire. */
  
  switch (nr (1,3))
    {
      case 1:
	fire_disaster ();
	break;
      case 2:
	society_get_disease ();
	break;
      case 3:
      default:
	flood_disaster();
	break;
    }
  return;
}

void
fire_disaster (void)
{ 
  THING *area, *room;
  int count, num_choices = 0, num_chose = 0;
  FLAG *flg;
  for (count = 0; count < 2; count++)
    {
      for (area = the_world->cont; area; area = area->next_cont)
	{
	  /* Use regular areas that are forest/field/mtn. */
	  if (IS_AREA_SET (area, AREA_NOLIST | AREA_NOREPOP | AREA_NOSETTLE))
	    continue;
	  
	  if (!IS_ROOM_SET (area, ROOM_FOREST | ROOM_FIELD | ROOM_ROUGH))
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    return;
	  num_chose = nr (1, num_choices);
	}
    }
  
  if (area)
    {
      for (room = area->cont; room; room = room->next_cont)
	{
	  if (!IS_ROOM (room) || nr (1,2) == 2 ||
	      !IS_ROOM_SET (room, ROOM_FOREST | ROOM_FIELD | ROOM_ROUGH | ROOM_EASYMOVE))
	    continue;
	  
	  flg = new_flag();
	  flg->type = FLAG_ROOM1;
	  flg->timer = nr (10, 20);
	  flg->val = ROOM_FIERY;
	  flg->from = MAX_SPELL+2;
	  if (room->cont)
	    act ("A spark ignites a $9fire$7!", room->cont, NULL, NULL, NULL, TO_ALL);
	  aff_update (flg, room);
	  set_up_map_room (room);
	}
    }
  return;
}



void
flood_disaster (void)
{ 
  THING *area, *room, *nroom = NULL;
  int count, num_choices = 0, num_chose = 0, i;
  FLAG *flg;
  VALUE *exit; 
  bool water_adjacent;
  
  /* Floods only happen within a few hours of rains. */
  if (wt_info && wt_info->val[WVAL_LAST_RAIN] > 10)
    return;
  for (count = 0; count < 2; count++)
    {
      for (area = the_world->cont; area; area = area->next_cont)
	{
	  /* Use regular areas that are forest/field/mtn. */
	  if (IS_AREA_SET (area, AREA_NOLIST | AREA_NOREPOP | AREA_NOSETTLE))
	    continue;
	  
	  if (!IS_ROOM_SET (area, ROOM_FOREST | ROOM_FIELD | ROOM_ROUGH))
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    return;
	  num_chose = nr (1, num_choices);
	}
    }
  
  if (area)
    {
		 
      for (room = area->cont; room; room = room->next_cont)
	{
	  water_adjacent = FALSE;
	  if (!IS_ROOM (room) || IS_ROOM_SET (room, ROOM_WATERY))
	    continue;
	  
	  for (i = 0; i < REALDIR_MAX; i++)
	    {
	      if ((exit = FNV (room, i + 1)) != NULL &&
		  (nroom = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM_SET (nroom, ROOM_WATERY))
		{
		  /* Make sure it's a perma water flag. */
		  for (flg = nroom->flags; flg; flg = flg->next)
		    {
		      if (flg->timer == 0 &&
			  flg->type == FLAG_ROOM1 &&
			  IS_SET (flg->val, ROOM_WATERY))
			{
			  water_adjacent = TRUE;
			  break;
			}
		    }
		  /* If we have adjacent water and this floods its bank,
		     then flood the room. */
		}
	      if (water_adjacent && nroom)
		break;
	    }
	  if (water_adjacent && nroom)
	    {	      
	      flg = new_flag();
	      flg->type = FLAG_ROOM1;
	      flg->timer = nr (10,25);
	      flg->val = ROOM_WATERY;
	      flg->from = MAX_SPELL+2;
	      if (room->cont)
		act ("@3n overflow@s its banks!", room->cont, NULL, nroom, NULL, TO_ALL);
	      aff_update (flg, room);
	      set_up_map_room (room);
	    }
	}
    }
  return;
}


/* A Snow disaster makes all above ground field/forest/rough/road rooms
   get temp snow flags in areas that are forest/field/rough/water /snow
   areas. */


void
snow_disaster (void)
{
  FLAG *flg;
  
  THING *area, *room;
  
  
  for (area = the_world->cont; area; area = area->next_cont)
    {
      if (!IS_ROOM_SET (area, ROOM_ROUGH | ROOM_FIELD | ROOM_FOREST | ROOM_SNOW | ROOM_WATERY))
	continue;
      
      if (IS_ROOM_SET (area, ROOM_DESERT | ROOM_SWAMP))
	continue;

      for (room = area->cont; room; room = room->next_cont)
	{
	  if (!IS_ROOM (room))
	    continue;
	  
	  if (!IS_ROOM_SET (room, ROOM_FOREST | ROOM_FIELD | ROOM_EASYMOVE | ROOM_ROUGH))
	    continue;

	  if (IS_ROOM_SET (room, ROOM_DESERT| ROOM_SWAMP))
	    continue;
	  
	  flg = new_flag();
	  flg->from = MAX_SPELL+2;
	  flg->timer = nr (5,15);
	  flg->val = ROOM_SNOW;
	  flg->type = FLAG_ROOM1;
	  if (room->cont)
	    act ("$FThe snow accumulates.$7", room->cont, NULL, NULL, NULL, TO_ALL);
	  aff_update (flg, room);
	  set_up_map_room (room);
	}
    }
  return;
}
