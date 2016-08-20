#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "areagen.h"
#include "objectgen.h"
#include "detailgen.h"



/* Global society name used because I don't feel like passing it
   around to all of the functions. */

static char detail_society_name[STD_LEN];

void
detailgen (THING *area)
{
  THING *start_room = NULL, *area_in, *room;
  int sector_type;
  int num_details;
  int i, times,  j;
  int used[DETAIL_MAX];
  
  if (!area || !IS_AREA (area) || 
      (sector_type = flagbits (area->flags, FLAG_ROOM1)) == 0 ||
      !IS_SET (sector_type, ROOM_SECTOR_FLAGS))
    return;
  
  
  
  /* Get the number of details: */
  
  for (i = 0; i < DETAIL_MAX; i++)
    used[i] = 0;
  
  num_details = nr (1, 1 + area->mv/DETAIL_DENSITY);
  
  /* Try to add a detail for each detail listed. */

  for (times = 0; times < num_details; times++)
    {
      if ((start_room = find_detail_type (area, used)) != NULL &&
	  IS_ROOM (start_room) &&
	  (area_in = find_area_in (start_room->vnum)) != NULL &&
	  area_in->vnum == DETAILGEN_AREA_VNUM)
	{
	  /* The weight in the detail room lets you determine
	     how many times the detail can be added to an area. */
	  for (j = 0; j < MAX(1, (int) start_room->weight); j++)
	    {
	      if (j == 0 || nr(1,2) == 1)
		{
		  detail_society_name[0] = '\0'; 
		  for (room = area->cont; room; room = room->next_cont)
		    {
		      RBIT (room->thing_flags, TH_MARKED);
		    }
		  add_detail (area, area, start_room, 0); 
		  for (room = area->cont; room; room = room->next_cont)
		    {
		      RBIT (room->thing_flags, TH_MARKED);
		    }
		  detail_society_name[0] = '\0';
		}
	    }
	}
    }

 
  return;
  
}

/* This finds a detail from the details area and tries to get it
   set up within the area. The used array is used so that we don't
   put two details of the same type in an area. */

THING *
find_detail_type (THING *area, int used[DETAIL_MAX])
{
  THING *room, *detail_area, *start_room = NULL;
  int num_choices = 0, num_chose = 0, count;
  int num = 0;
  bool used_already = FALSE;
  if ((detail_area = find_thing_num (DETAILGEN_AREA_VNUM)) == NULL ||
      !IS_AREA (detail_area) || !area || !IS_AREA (area))
    return NULL;
  
  
  for (count = 0; count < 2; count++)
    {
      for (room = detail_area->cont; room; room = room->next_cont)
	{
	  /* We only search rooms in the first half of the detail area
	     room allocation, and then we only pick rooms with
	     names. */
	  if (!IS_ROOM (room) || 
	      (room->vnum - detail_area->vnum) < 2 ||
	      ((room->vnum - detail_area->vnum) >= detail_area->mv/2))
	    continue;
			
	  /* You can set levels on detail rooms so that they
	     don't show up in lower level areas. */
	  if (room->level > area->level)
	    continue;
	  
	  used_already = FALSE;
	  for (num = 0; num < DETAIL_MAX; num++)
	    {
	      if (used[num] == room->vnum)
		{
		  used_already = TRUE;
		  break;
		}
	    }
	  if (used_already)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;

	  num_chose = nr (1, num_choices);
	}
    }
  
  if (!room)
    return NULL;
  
  for (num = 0; num < DETAIL_MAX; num++)
    {
      if (used[num] == 0)
	{
	  used[num] = room->vnum;
	  start_room = room;
	  break;
	}
    }

  return start_room;
}

/* This adds a detail to a thing. It does it in the following
   ways .

   1. If to is an area, then if detail_thing is an area, quit. If
   detail_thing is not in the detail area, quit. If detail_thing is 
   a room within detail_area->mv/2 then setup the main block of rooms
   using add_main_detail_rooms. If the detail_thing is another room,
   add the block of rooms using start_other_detail_rooms, and then
   return one of the rooms that the detail was added to as the newth.
   If detail_thing is a mob or object, create it and add it as a reset
   to one of the marked rooms in the area.
   
   2. If to is a room, then if detail_thing is a room or area, quit. If
   detail_thing is a mob or object, create the mobject from the
   info given and add it as a reset to the room and call the thing
   made newth;
   
   If to is a mobject and detail_thing isn't an object, quit. Else,
   add a reset of the object onto to, and make the new object newth
   
   If newth doesn't exist, then bail out.
   
   Then, go down the list of resets in detail_thing. If they are resets
   for real items outside of the detail area, then copy the reset to newth
   directly. Otherwise, find the thing in the detail area that the
   reset represents and call it reset_detail_thing. Then call
   add_detail (newth, reset_detail_thing, depth+1); */

/* The reason for the had_society_name_first variable is that if I 
   didn't have such a name and I created a mobject and that mobject
   happened to add such a name, I want to dump the name when I get
   out of this mobject. */

void
add_detail (THING *area, THING *to, THING *detail_thing, int depth)
{
  THING *newth = NULL, *detail_area;
  bool had_society_name_first = FALSE;

  if (!to || !detail_thing || IS_AREA (detail_thing) || 
      depth >= DETAIL_MAX_DEPTH)
    return;
  
  if ((detail_area = detail_thing->in) == NULL ||
      !IS_AREA (detail_area) || 
      detail_area->vnum != DETAILGEN_AREA_VNUM)
    return;
  
  if (*detail_society_name)
    had_society_name_first = TRUE;
  
  if (IS_AREA (to))
    {      
      /* Little check here to make sure that the area we're adding the
	 detail to is the area we're adding the detail to.... */
      if (to != area)
	return;

      if (IS_ROOM (detail_thing))
	{
	  /* Don't add secondary detail rooms that are in the primary 
	     detail room area. */
	  if (detail_thing->vnum - detail_area->vnum <
	      detail_area->mv/2 && depth == 0)
	    {
	      start_main_detail_rooms (area, detail_thing);
	      newth = to;
	    }
	  else
	    {	      
	      newth = start_other_detail_rooms (area, detail_thing);
	    }
	}
      else
	newth = generate_detail_mobject (area, to, detail_thing);
    }
  else if (IS_ROOM (to))
    {
      if (IS_AREA (detail_thing) || IS_ROOM (detail_thing))
	return;
      
      newth = generate_detail_mobject (area, to, detail_thing);
    }
  else 
    {
      /* Don't allow areas or rooms to go to nonroom/nonareas. */
      /* Don't allow mobs into nonrooms UNLESS...that object 
	 can contain, and it allows you to leave and enter. */
      if (IS_AREA (detail_thing) || IS_ROOM (detail_thing) )
	return;
      	  
      newth = generate_detail_mobject (area, to, detail_thing);
    }
  
  if (newth)
    add_detail_resets (area, newth, detail_thing, depth);
  
  /* Now if we didn't have a society name to start with and we do 
     now and this is a mobject, then delete the society name. */
  
  if (!had_society_name_first && 
      (!newth || 
       (!IS_ROOM (newth) && !IS_AREA(newth))) &&
      *detail_society_name)
    *detail_society_name = '\0';
  
  return;
}
	  
/* This starts a detail by generating its name and then creating the
   detail. Then it looks for the resets on the detail to add more 
   details to the detail. */

void
start_main_detail_rooms (THING *area, THING *detail_room)
{
  THING *detail_area, *room, *start_room, *nroom;
  VALUE *exit;
  int area_sector_flags;
  /* Used for picking the room where the detail will start. */
  bool adjacent_rooms_ok;
  int num_choices = 0, num_chose = 0, dir, count;
  /* What kinds of rooms this detail can be placed into. */
  int detail_room_bits, extra_detail_room_bits = 0;
  VALUE *oval, *nval;
  if ((detail_area = find_thing_num (DETAILGEN_AREA_VNUM)) == NULL ||
      !IS_AREA (detail_area))
    return;
  
  if (!area || !IS_AREA (area))
    return;
  
  if (!IS_ROOM (detail_room) || 
      /* Only allow primary detail rooms to start this process. */
      ((detail_room->vnum - detail_area->vnum) >= detail_area->mv/2))
    return;
  
  detail_room_bits = flagbits (detail_room->flags, FLAG_ROOM1) & ROOM_SECTOR_FLAGS ;
  /* Inside can go anywhere. */
  extra_detail_room_bits = detail_room_bits & ROOM_INSIDE;
  RBIT (detail_room_bits, ROOM_INSIDE);
  area_sector_flags = flagbits (area->flags, FLAG_ROOM1);
  
  /* If the detail room has bits specified, then the area must have
     some of those bits or else the detail won't get made. The room
     where the detail gets started must also have some of those
     bits and it must be adjacent only to room with those bits. */
  if (IS_SET (detail_room_bits, AREAGEN_SECTOR_FLAGS) &&
      !(area_sector_flags & detail_room_bits & AREAGEN_SECTOR_FLAGS))
    return;
  
  /* Get an appropriate room to use for starting the detail. */
  
  for (count = 0; count < 2; count++)
    {
      for (room = area->cont; room; room = room->next_cont)
	{
	  /* The room must be a room and it must have the correct
	     bits set and it must not have resets. */
	  if (!IS_ROOM (room) || room->resets ||
	      IS_ROOM_SET (room, BADROOM_BITS | ROOM_EASYMOVE) ||
	      is_named (room, "detail") ||
	      is_named (room, "in_tree") ||
	      is_named (room, "catwalk") ||
	      (detail_room_bits &&
	       !IS_ROOM_SET (room, detail_room_bits)))
	    continue;
	  
	  adjacent_rooms_ok = TRUE;
	  
	  for (dir = 0; dir < REALDIR_MAX; dir++)
	    {
	      if ((exit = FNV (room, dir + 1)) != NULL &&
		  (nroom = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM (nroom) &&
		  (nroom->resets ||
		   IS_ROOM_SET (nroom, BADROOM_BITS | ROOM_EASYMOVE) ||
		   (detail_room_bits && 
		    (!IS_ROOM_SET (nroom, detail_room_bits)))))
		{
		  adjacent_rooms_ok = FALSE;
		  break;
		}
	    }
	  if (!adjacent_rooms_ok)
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
	  else
	    num_chose = nr (1, num_choices);
	}
      
    }
  
  if (room && IS_ROOM (room) && !room->resets &&
      !IS_ROOM_SET (room, BADROOM_BITS | ROOM_EASYMOVE) &&
      (!detail_room_bits || IS_ROOM_SET (room, detail_room_bits)))
    start_room = room;
  else
    return;
  
  /* Now get the name of this detail and make sure that it exists. */
 
  if (!start_room->short_desc || !*start_room->short_desc)
    return;

  
  detail_room_bits |= extra_detail_room_bits;
  
  
  copy_flags (detail_room, start_room);
  remove_flagval (start_room, FLAG_ROOM1, 
		  flagbits(detail_room->flags, FLAG_ROOM1) & ROOM_SECTOR_FLAGS);
  add_flagval (start_room, FLAG_ROOM1, detail_room_bits);

  /* Copy the flags and values over. */
  for (oval = detail_room->values; oval; oval = oval->next)
    {
      
      if (oval->type > REALDIR_MAX)
	{
	  nval = new_value();
	  copy_value (oval, nval);
	  add_value (start_room, nval);
	}
    }
  undo_marked (start_room);
  append_detail_to_room_name (start_room);
  add_main_detail_room (start_room, MAX(1,detail_room->height), detail_room_bits);
  return;
}

/* This adds the word "detail" to a room name. */

void
append_detail_to_room_name (THING *room)
{
  char buf[STD_LEN];
  if (!room || !IS_ROOM (room))
    return;
  
  if (room->name && *room->name)
    {
      strcpy (buf, room->name);
      strcat (buf, " ");
    }
  else
    buf[0] = '\0';
  strcat (buf, "detail");
  free_str (room->name);
  room->name = new_str (buf);
}

/* This adds a detail room to the map. It adds the room's name. */

void
add_main_detail_room (THING *room, int depth_left, int room_bits)
{
  THING *nroom;
  VALUE *exit;
  int dir;
  VALUE *oval, *nval;
  if (!room || !IS_ROOM (room) || 
      !room->name || !*room->name ||
      IS_ROOM_SET (room, BADROOM_BITS | ROOM_EASYMOVE) ||
      IS_MARKED (room) || depth_left < 1)
    return;
  
  SBIT (room->thing_flags, TH_MARKED);
  
  if (depth_left < 1)
    return;
  
  /* Now remove old bits except underground and add new bits. */
  
  

  if (room_bits)
    {
      remove_flagval (room, FLAG_ROOM1, AREAGEN_SECTOR_FLAGS &~ ROOM_UNDERGROUND);
      add_flagval (room, FLAG_ROOM1, room_bits);
    }
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (nroom) &&
	  !is_named (nroom, "detail") &&
	  !is_named (nroom, "in_tree") &&
	  !is_named (nroom, "catwalk") &&
	  !IS_SET (nroom->thing_flags, TH_MARKED))
	{
	  /* Now copy the flags and values over. */
	  copy_flags (room, nroom); 
	  for (oval = room->values; oval; oval = oval->next)
	    {	      
	      if (oval->type > REALDIR_MAX)
		{
		  nval = new_value();
		  copy_value (oval, nval);
		  add_value (nroom, nval);
		}
	    }
	  free_str (nroom->short_desc);
	  nroom->short_desc = new_str (room->short_desc);
	  append_detail_to_room_name (nroom);
	  add_main_detail_room (nroom, depth_left - 1, room_bits);
	}
    }
  return;
}

/* This starts and adds other detail rooms besides the original ones. 
   It returns the room where the details were started. This only goes
   to a depth of 1 room in any dir from the central room... since
   these are supposed to be smaller. Could change this tho. */

THING *
start_other_detail_rooms (THING *area, THING *detail_thing)
{
  THING *room, *nroom, *detail_area;
  int num_choices = 0, num_chose = 0, count;
  int dir;
  int room_flags = 0;
  VALUE *exit;
  
  /* Sanity checking... */
  
  if (!area || !IS_AREA (area) || !detail_thing ||
      (detail_area = detail_thing->in) == NULL || 
      detail_area->vnum != DETAILGEN_AREA_VNUM ||
      !IS_ROOM (detail_thing) ||
      ((detail_thing->vnum-detail_area->vnum) < detail_area->mv/2))
    return NULL;
  
  
  
  /* Get the room flags we will set. */

  room_flags = flagbits (detail_thing->flags, FLAG_ROOM1) & ~ROOM_MINERALS;

  /* We know we're in an area and the detail thing is a room in 
     the proper part of the detail area. */

  /* Find the room where the smaller detail chunk will start. */

  for (count = 0; count < 2; count++)
    {
      for (room = area->cont; room; room = room->next_cont)
	{
	  if (IS_ROOM (room) && IS_MARKED (room))
	    {
	      if (count == 0)
		num_choices++;
	      else if (--num_chose < 1)
		break;
	    }
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;
	  num_chose = nr (1, num_choices);
	}
    }
  
  
  if (!room)
    return NULL;
  
  /* Now replace the room and things nearby (if needed) */
  
  generate_detail_name (detail_thing, room);
  capitalize_all_words (room->short_desc);
  RBIT (room->thing_flags, TH_MARKED);
  if (room_flags)
    {
      remove_flagval (room, FLAG_ROOM1, 
		      flagbits (room->flags, FLAG_ROOM1));
      add_flagval (room, FLAG_ROOM1, room_flags);
      if (IS_ROOM_SET (area, ROOM_UNDERGROUND))
	add_flagval (room, FLAG_ROOM1, ROOM_UNDERGROUND);
    }
  
  /* Now check each dir to see if the exits need to be added. The way 
     to do it is to see if nr (1,4) > detail_room->height-1 or not. */

  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir+1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (nroom) && IS_MARKED(nroom) &&
	  nr (1,4) >= ((int) detail_thing->height-1))
	{
	  if (!is_named (detail_thing, "same_name"))
	    {
	      free_str (nroom->short_desc);
	      nroom->short_desc = new_str (room->short_desc); 
	    }
	  RBIT (nroom->thing_flags, TH_MARKED);
	  if (room_flags)
	    { 
	      remove_flagval (nroom, FLAG_ROOM1, 
			      flagbits (nroom->flags, FLAG_ROOM1));
	      add_flagval (nroom, FLAG_ROOM1, room_flags);
	    }
	}
    }
  return room;
}



/* This lets you create a mobject within an area and then add it to
   something as a reset. */

THING *
generate_detail_mobject (THING *area, THING *to, THING *detail_thing)
{
  THING *detail_area, *mobject, *thg, *new_to;
  int vnum;
  RESET *reset;
  
  if (!area || !IS_AREA (area) || !to || !detail_thing ||
      (IS_AREA (area) && IS_AREA(to) && to != area) ||
      (detail_area = detail_thing->in) == NULL ||
      detail_area->vnum != DETAILGEN_AREA_VNUM ||
      IS_ROOM (detail_thing) || IS_AREA (detail_thing))
    return NULL;
  /* Mobs can only go to areas/rooms, unless the other object allows
     people to enter and leave. That requres noleave, nocontain, 
     no_move_into to all be turned off. */
  
  if (!IS_ROOM (to) && !IS_AREA(to) &&
      IS_SET (to->thing_flags, TH_NO_CONTAIN | TH_NO_MOVE_INTO | TH_NO_LEAVE) &&
      (CAN_MOVE (detail_thing) || CAN_FIGHT (detail_thing)))
    return NULL;
  
  
  /* Find an open vnum. */
  
  for (vnum = area->vnum +area->mv+1; vnum < area->vnum + area->max_mv; vnum++)
    {
      if ((thg = find_thing_num (vnum)) == NULL)
	break;
    }
  
  if (vnum <= area->vnum + area->mv ||
      vnum >= area->vnum + area->max_mv)
    return NULL;
  
 
  
 

  mobject = new_thing ();
  copy_thing (detail_thing, mobject); 
  mobject->type = new_str (detail_thing->type);
  generate_detail_name (detail_thing, mobject);
  mobject->vnum = vnum;
  if (CAN_TALK (mobject))
    mobject->level += nr (area->level/2, area->level);
  thing_to (mobject, area);
  add_thing_to_list (mobject);
  free_str (mobject->desc);
  
  /* Mobs get set to max 1 reset. */
  if (CAN_MOVE (mobject) || CAN_FIGHT (mobject))
    mobject->max_mv = 1;
  else
    mobject->max_mv = 0;
  mobject->mv = 0;

  /* Now add detection. */

  if (CAN_MOVE (mobject) || CAN_FIGHT (mobject))
    {
      if (LEVEL (mobject) >= 80)
	add_flagval (mobject, FLAG_DET, AFF_DARK);
      if (LEVEL (mobject) >= 120)
	add_flagval (mobject, FLAG_DET, AFF_INVIS);
      if (LEVEL (mobject) >= 200)
	add_flagval (mobject, FLAG_DET, AFF_CHAMELEON);
    }
  
  /* Now add the reset. */
  
  if (IS_AREA (to))
    {
      new_to = find_random_room (area, TRUE, 0, 0);
    }
  else
    new_to = to;
  
  if (!new_to)
    return NULL;
  
  reset = new_reset();
  reset->vnum = vnum;
  reset->times = 1;
  reset->nest = 1;
  if (CAN_MOVE(mobject) || CAN_FIGHT (mobject) ||
      IS_SET (mobject->thing_flags, TH_NO_TAKE_BY_OTHER | TH_NO_MOVE_BY_OTHER))
    reset->pct = 100;
  else
    reset->pct = objectgen_reset_percent (mobject->level);
  
   
  reset->next = new_to->resets;
  new_to->resets = reset;
  return mobject;
  
}

  
  
/* This adds the details contained in detail_loc to the
   to_loc object. */

void
add_detail_resets (THING *area, THING *to, THING *detail_thing, int depth)
{
  RESET *reset, *newreset, *prev;
  int times, num_objects;
  THING *detail_area, *reset_in_area, *new_detail_thing;
  THING *randpop_item, *room_to;
  VALUE *randpop;
  int randmob_sector_flags;
  if (!area || !IS_AREA (area) || !to || !detail_thing ||
      (IS_AREA (area) && IS_AREA(to) && to != area) ||
      (detail_area = detail_thing->in) == NULL ||
      detail_area->vnum != DETAILGEN_AREA_VNUM)
    return;
  
  for (reset = detail_thing->resets; reset; reset = reset->next)
    {
      /* If the reset is the objectgen area number, make random
	 objects for this thing. */
      
      if (reset->vnum == OBJECTGEN_AREA_VNUM)
	{
	  num_objects = MAX (1, reset->times);
	  /* If this is a "pk world" -- autogenerated -- then the mobs get
	     more objects. */
	  
	  if (IS_SET (server_flags, SERVER_AUTO_WORLDGEN) &&
	      IS_SET (server_flags, SERVER_WORLDGEN))
	    {
	      num_objects += nr (num_objects/2, num_objects);
	    }
	  for (times = 0; times < MAX(1, (int) reset->times); times++)
	    {
	      if (nr(1,100) <= reset->pct &&
		  (new_detail_thing = 
		   objectgen (area, ITEM_WEAR_NONE, 
			      nr (to->level*2/3, to->level*5/4), 
			      0, NULL)) != NULL)
		{
		  newreset = new_reset();
		  newreset->vnum = new_detail_thing->vnum;
		  newreset->times = 0;
		  newreset->pct = objectgen_reset_percent (new_detail_thing->level);
		  newreset->next = to->resets;
		  to->resets = newreset;
		}
	    }
	}
      else if ((reset_in_area = find_area_in (reset->vnum)) != detail_area)
	{
	  /* Deal with randpop mobs differently. In this case, pick
	     a randpop mob and have several resets of it in one
	     place. */
	  
	  if (reset->vnum == MOB_RANDPOP_VNUM &&
	      (randpop_item = find_thing_num (MOB_RANDPOP_VNUM)) != NULL &&
	      (randpop = FNV (randpop_item, VAL_RANDPOP)) != NULL &&
	      (IS_AREA (to) || IS_ROOM (to)))
	    {
	      /* If to is an area, find one of the marked rooms. */
	      
	      
	      for (times = 0; times < 30; times++)
		{
		  if ((new_detail_thing = 
		       find_thing_num 
		       (calculate_randpop_vnum 
			(randpop, area->level))) != NULL && 
		      !IS_ROOM (new_detail_thing) && 
		      !IS_AREA (new_detail_thing) &&		      
		      CAN_MOVE (new_detail_thing) &&
		      CAN_FIGHT (new_detail_thing) &&
		      ((randmob_sector_flags = 
			flagbits (new_detail_thing->flags, FLAG_ROOM1)) == 0 ||
		       IS_ROOM_SET (area, randmob_sector_flags)) &&
		      ((IS_ROOM (to) && (room_to = to)) ||
		       (IS_AREA (to) &&
			(room_to = find_random_room (to, TRUE, 0, 0)) != NULL)))
		    {
		      if (!room_to)
			continue;
		     
		      newreset = new_reset ();
		      newreset->vnum = new_detail_thing->vnum;
		      newreset->pct = 50;
		      newreset->times = 50/(MAX(10, new_detail_thing->level));
		      newreset->times = MID (1, newreset->times, 3);
		      newreset->next = room_to->resets;
		      newreset->nest = 1;
		      room_to->resets = newreset;
		      break;
		    }
		}

	    }
	  else
	    {
	      newreset = new_reset();
	      copy_reset (reset, newreset);	      	      
	      if (!to->resets)
		{
		  newreset->next = to->resets;
		  to->resets = newreset;
		}
	      else
		{
		  for (prev = to->resets; prev; prev = prev->next)
		    {
		      if (!prev->next)
			{
			  prev->next = newreset;
			  newreset->next = NULL;
			  break;
			}
		    }
		}
	    }
	}
      else if ((new_detail_thing = find_thing_num (reset->vnum)) != NULL)
	{
	  for (times = 0; times < MAX(1, (int) reset->times); times++)
	    {
	      if (nr(1,100) <= reset->pct)		
		add_detail (area, to, new_detail_thing, depth+1);
	    }
	}
    }
  
  return;
}


/* This generates a detail name based on the extra descs in the object.
   It requires an extra description named "format" that has a list
   of phrases enclosed in quotes that it uses to format a string
  based on other words in other edescs. */

void
generate_detail_name (THING *proto, THING *target)
{
  EDESC *fdesc, *wdesc;
  char keywords[STD_LEN]; /* Actual names of this thing. */
  char format[STD_LEN]; /* The format string we use. */
  char lookup_word[STD_LEN]; /* What word we're looking up in edesc list. */
  char word[STD_LEN];
  char sdesc[STD_LEN];
  char ldesc[STD_LEN*2];
  char lformat[STD_LEN];
  char *format_pos; /* Where we are in the format. */
  bool need_a_an_here;
  bool use_plural = FALSE;  /* Is this a plural name? */
  bool use_notice = FALSE;  /* Do we have the "You notice" prefix on the ldesc? */
  bool no_space_here = FALSE; /* Set this if you don't want a space here. */
  char *t;
  char word2[STD_LEN];
  THING *wlist_obj;
  /* Proto and target needed. */
  if (!proto || !target)
    return;

  /* Same named things don't set a name. */

  if (is_named (proto, "same_name") || is_named (proto, "previous_name"))
    return;
  
  keywords[0] = '\0';
  format[0] = '\0';
  lookup_word[0] = '\0';
  word[0] = '\0';
  sdesc[0] = '\0';
  lformat[0] = '\0';
  /* Make sure that the "format" string(s) exist. If not, bail out. */
  if ((fdesc = find_edesc_thing (proto, "format", TRUE)) != NULL ||
      (fdesc = find_edesc_thing (proto, "sformat", TRUE)) != NULL)
    {
      strcpy (format, find_random_word (fdesc->desc, NULL));
    }
  else /* No explicit format-- fall back on klong kshort kname */
    {
      strcpy (format, "a_an klong kshort kname");
    }
  if (!*format)
    return;

  if (is_named (proto, "plural"))
    use_plural = TRUE;
  
  /* Now make the short desc by going down the format string word by
     word stripping off words to check against the list of other edescs
     this detail has. If we have a match, we lookup a word to add into
     the sdesc. If no match, we plop the word into sdesc. The other
     thing is that if the lookup_word starts with k, and we find a
     matching word, then that word we eventually add into the sdesc
     goes into the keywords list. */

  /* Now go word by word down the format stripping off words and
     seeing if they exist in the list of edescs we have. */

  format_pos = format;  
  sdesc[0] = '\0';
  
  need_a_an_here = FALSE;
  do
    {
      no_space_here = FALSE;
      format_pos = f_word (format_pos, lookup_word);
      /* If the lookup word is for an a/an we have to wait for the
	 next word to actually add this word in... */
      if (!str_cmp (lookup_word, "a_an"))
	need_a_an_here = TRUE;      
      else if (!str_cmp (lookup_word, "##"))
	no_space_here = TRUE;
      else
	{
	  
	  word[0] = '\0';
	  /* If the word belongs to another edesc add a random word... */
	  if ((wdesc = find_edesc_thing (proto, lookup_word, TRUE)) != NULL)
	    strcpy (word, find_random_word (wdesc->desc, detail_society_name));
	  
	  
	  
	  /* Now see if this is a word from the word list area or not. */
	  *word2 = '\0';
	  
	  if (!*word)
	    strcpy (word2, find_gen_word (WORDLIST_AREA_VNUM, lookup_word, NULL));
	  
	  /* If the lookup_word isn't one of the WORDLIST_AREA things,
	     then check if the word we got from the lookup_word is. */
	  if (*word && !*word2)
	    strcpy (word2, find_gen_word (WORDLIST_AREA_VNUM, word, NULL));
	  
	  if (*word2 && str_cmp (word, word2))
	    {
	      strcpy (word, word2);
	      /* Word gives us the actual name to look up. */
	      if (LC(*word) == 'k')
		{
		  if (*keywords)
		    strcat (keywords, " ");
		  strcat (keywords, word2);
		}
	    }
	  /* lookup_word gives us the name to look up. */
	  else if (LC(*lookup_word) == 'k')
	    {
	      if (*keywords)
		strcat (keywords, " ");
	      strcat (keywords, word);
	    }
	  
	  
	  if (!*word)
	    {
	      if (!named_in_full_word ("kshort klong kname", lookup_word))
		/* Otherwise plop this word directly in there. */
		strcpy (word, lookup_word);
	      else
		*word = '\0';
	    }
      
      
      /* If we need an a_an here first, plop it down then put the
	 word. */
	  
	  if (need_a_an_here && *word)
	    {
	      if (*sdesc && *word)
		strcat (sdesc, " ");
	      if (use_plural)
		strcat (sdesc, find_gen_word (WORDLIST_AREA_VNUM, "a_an_plural", NULL));
	      else
		strcat (sdesc, a_an (word));
	      need_a_an_here = FALSE;
	    }
	  if (*sdesc && !no_space_here)
	    strcat (sdesc, " ");
	  strcat (sdesc, word);
	  
	}
    }
  while (*format_pos);
  
  /* So the sdesc is now set up. If it's a room, bail out. Otherwise
     continue to get the long desc. */

  if (IS_ROOM (target) && *sdesc)
    {
      free_str (target->short_desc);
      target->short_desc = new_str (sdesc); 
      
      return;
    }

  /* Set the name and short_desc. */
  
  free_str (target->name);
  target->name = new_str (keywords);
  free_str (target->short_desc);
  target->short_desc = new_str (sdesc); 

  /* Check to see if there's a "long format" string. If not, make a simple
     long desc. These complex long descs are used for mobs only. */
  
  if ((CAN_FIGHT (proto) || CAN_MOVE (proto)))
    {
      lformat[0] = '\0';
      /* Try to get a proper format. */
      if ((fdesc = find_edesc_thing (proto, "lformat", TRUE)) != NULL &&
	  fdesc->desc && *fdesc->desc)
	strcpy (lformat, find_random_word (fdesc->desc, NULL));
      if (!*lformat) /* If none, then set it up by hand. */
	{ 
	  *lformat = '\0';
	  
	  if (is_named (proto, "plural"))
	    use_plural = TRUE;
	  /* You notice.. */
	  if (nr (1,3) == 2)
	    {
	      if (use_plural)
		strcat (lformat, "mob_notice_plural ");
	      else
		strcat (lformat, "mob_notice ");	  
	    }
	  
	  if (*lformat)
	    use_notice = TRUE;
	  
	  /* The mob name */
	  strcat (lformat, " ");
	  strcat (lformat, sdesc);
	  strcat (lformat, " ");
	  
	  /* Is here..*/
	  if (!use_notice)
	    {
	      if (use_plural)
		strcat (lformat, "are ");
	      else
		strcat (lformat, "is ");
	    }
	  
	  /* If the thing can talk, it's doing something reasonable
	     and intelligent so we don't say it's lookig for food
	     and whatnot. */
	  
	  if (CAN_TALK (proto))
	    strcat (lformat, "near_here");
	  else /* Otherwise it has to look for food and so forth. */
	    {	  
	      if (nr (1,3) != 2)
		{
		  if (IS_AFF (proto, AFF_WATER_BREATH))
		    strcat (lformat, "swimming ");
		  else if (!IS_AFF (proto, AFF_FLYING))
		    strcat (lformat, "mob_action ");
		  else
		    strcat (lformat, "mob_action_flying ");
		  
		  /* Here */
		  strcat (lformat, "near_here ");
		  
		  /* Searching */
		  if (nr (1,3) != 2)
		    {
		      strcat (lformat, "mob_searching ");
		      
		      /* Searching for */
		      if (!IS_AFF (proto, AFF_FLYING) || nr (1,10) != 3)
			strcat (lformat, "mob_searchfor ");
		      else
			strcat (lformat, "mob_searchrest ");
		    }
		}
	      else
		strcat (lformat, "here ");
	    }
	}
    }
  else
    { 
      lformat[0] = '\0';
      if ((fdesc = find_edesc_thing (proto, "lformat", TRUE)) != NULL &&
	  fdesc->desc && *fdesc->desc)
	strcpy (lformat, find_random_word (fdesc->desc, NULL));
      if (!*lformat)
	{
	  char tempformat[STD_LEN];
	  strcpy (tempformat, find_gen_word (WORDLIST_AREA_VNUM, "obj_long_format", NULL));
	  if (!*tempformat)
	    sprintf (lformat, "%s is here", sdesc);
	  else
	    {
	      char *wd = tempformat;
	      lformat[0] = '\0';
	      while (wd && *wd)
		{
		  wd = f_word (wd, word);
		  if (*lformat)
		    strcat (lformat, " ");
		  if (!str_cmp (word, "sdesc"))		    
		    strcat (lformat, sdesc);
		  else
		    strcat (lformat, word);
		}
	    }
	}
    }
  /* Then loop through it setting it up. This is a little different than
   the previous setup. In there we had some lookup_words that we used to
  find random words from the list of words in that edesc. We can't do
  that here as easily since we need to use the same words as before.
  So here's what happens. For each lookup word in the format, we check
  if there's an edesc with that name. If not, we add the word into
  the list. If so, we then check to see if any words in that list
  for that edesc were used in the keywords or the short desc. If not,
  we just pick a random word from the new list. Otherwise we use the
  old word. */
  
  format_pos = lformat;
  need_a_an_here = FALSE;
  ldesc[0] = '\0';
  do
    {
      format_pos = f_word (format_pos, lookup_word);
      /* If the lookup word is for an a/an we have to wait for the
	 next word to actually add this word in... */
      if (!str_cmp (lookup_word, "a_an"))
	need_a_an_here = TRUE;      
      else
	{
	  *word = '\0';
        /* If the word belongs to another edesc add a random word... */
	  if ((wdesc = find_edesc_thing (proto, lookup_word, TRUE)) != NULL)
	    {
	      strcpy (word, string_found_in (wdesc->desc, target->short_desc));
	      
	      if (!*word)
		{
		  *word2 = '\0';
		  /* Look for the word in the wordlist area. */
		  if ((wlist_obj = find_thing_thing (the_world, find_thing_num (WORDLIST_AREA_VNUM), word, FALSE)) != NULL)
		    {
		      strcpy (word2, string_found_in (wlist_obj->desc, target->short_desc));
		      
		      if (!*word2)
			strcpy (word2, find_random_word (wlist_obj->desc, NULL));		      
		    }
		  /* If we get a match, use it. */
		  if (*word2)
		    {
		      strcpy (word, word2);
		    }
		}
	    }
	  else  /* Look for something in the word list. */
	    {
	      strcpy (word, find_gen_word (WORDLIST_AREA_VNUM, lookup_word, NULL));
	    }
	  
	  if (!*word)
	    strcpy (word, lookup_word);
	  
	  /* If we need an a_an here first, plop it down then put the
	     word. */
	  
	  if (need_a_an_here && *word)
	    {
	      strcat (ldesc, " ");
	      if (use_plural)
		strcat (ldesc, find_gen_word (WORDLIST_AREA_VNUM, "a_an_plural", NULL));
	      else
		strcat (ldesc, a_an (word));
	      need_a_an_here = FALSE;
	    }
	  if (*ldesc && *word)
	    strcat (ldesc, " ");
	  strcat (ldesc, word);
	  
	}
    }
  while (*format_pos);
  ldesc[0] = UC (ldesc[0]);
  for (t = ldesc; *t; t++);
  t--;
  if (*t != '.')
    {
      t++;
      *t = '.';
      *(t+1) = '\0';
    }
  free_str (target->long_desc);
  target->long_desc = new_str (ldesc);
  return;
}
  
