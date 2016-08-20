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
#include "mobgen.h"
#include "mapgen.h"
#include "citygen.h"


/* This adds fields around a city by adding rooms to all outer boundary
   rooms in each direction once, then twice. It tells when a room is
   a regular city room and when it's a field because fields will have a
   mv of -1; CITYGEN_FIELD_DEPTH */

void
citygen_add_fields (THING *area)
{

  THING *room, *nroom, *start_room, *last_room;
  int x, y, z = CITYGEN_STREET_LEVEL;
  /* Used for looking at adjacent coordinates. */
  int nx, ny, nz;
  int num_rooms_left = 0, vnum, num_rooms_needed = 0, highest_room_vnum;
  int num_passes = 1, pass;
  int curr_vnum; /* The current vnum we assign to new rooms. */
  int max_room_vnum; /* Max vnum we're allowed to use for rooms. */
  int dir;
  
  int count, num_choices, num_chose;
  /* Used to make sure that the city map has edges that other areas
     can link to. */
  int min_x = CITY_SIZE, max_x = 0, min_y = CITY_SIZE, max_y = 0;
  char name[STD_LEN];
  char buf[STD_LEN];
  VALUE *exit;
  bool edge_exists[FLATDIR_MAX];
  
  
  
  if (!area)
    return;

  highest_room_vnum = area->vnum + 1; 
  max_room_vnum = area->vnum + area->mv;
  for (vnum = area->vnum + 1; vnum <= max_room_vnum; vnum++)
    {
      if ((room = find_thing_num (vnum)) != NULL)
	highest_room_vnum = vnum;
    }


  num_rooms_left = max_room_vnum - highest_room_vnum - 1;
  
  /* Now check the number of boundary rooms. */

  for (x = 0; x < CITY_SIZE; x++)
    {
      for (y = 0; y < CITY_SIZE; y++)
	{
	  if ((room = city_grid[x][y][z]) != NULL)
	    {
	      for (dir = 0; dir < FLATDIR_MAX; dir++)
		{
		  nx = x;
		  ny = y;
		  nz = z;
		  if (dir == DIR_NORTH)
		    ny++;
		  else if (dir == DIR_SOUTH)
		    ny--;
		  else if (dir == DIR_EAST)
		    nx++;
		  else if (dir == DIR_WEST)
		    nx--;
		  
		  if (city_coord_is_ok (nx, ny, nz) &&
		      (nroom = city_grid[nx][ny][nz]) == NULL)
		    num_rooms_needed++;
		}
	    }
	}
    }

  /* These checks are not *quite* correct since there will be slightly
     more rooms in the second pass, but it's close enough and I don't
     feel like figuring out where all of the corners etc... are. */
  
  if (num_rooms_needed > num_rooms_left)
    return;

  /* The + 4 I think is correct for the extra rooms needed for the outer
     layer since the rooms that get surrounded and those that need extra
     rooms I think cancel out with the +4 remaining. */
  
  if (num_rooms_left >= 2*num_rooms_needed + 4 )
    num_passes = 2;
  else
    num_passes = 1;
  
  curr_vnum = highest_room_vnum + 1;
  for (pass = 0; pass < num_passes && curr_vnum < max_room_vnum; pass++)
    {
      for (x = 0; x < CITY_SIZE; x++)
	{
	  for (y = 0; y < CITY_SIZE; y++)
	    {
	      if ((room = city_grid[x][y][z]) != NULL &&
		  room->mv != CITYGEN_FIELD_DEPTH)
		{
		  for (dir = 0; dir < FLATDIR_MAX; dir++)
		    {
		      nx = x;
		      ny = y;
		      nz = z;
		      if (dir == DIR_NORTH)
			ny++;
		      else if (dir == DIR_SOUTH)
			ny--;
		      else if (dir == DIR_EAST)
			nx++;
		      else if (dir == DIR_WEST)
			nx--;
		      
		      if (city_coord_is_ok (nx, ny, nz) &&
			  (nroom = city_grid[nx][ny][nz]) == NULL)
			{
			  /* Set up a simple field room in the appropriate
			     place. */
			  nroom = new_room (curr_vnum);
			  city_grid[nx][ny][nz] = nroom;
			  curr_vnum++;
			  add_flagval (nroom, FLAG_ROOM1, ROOM_FIELD);
			  nroom->mv = CITYGEN_FIELD_DEPTH;		       
			  append_name (nroom, "outer_field");
			  free_str (nroom->short_desc);
			  nroom->short_desc = new_str ("Some Fields");
			  
		       
			  
			}
		    }
		  if (curr_vnum > max_room_vnum)
		    break;
		}
	      if (curr_vnum > max_room_vnum)
		break;
	    }
	  if (curr_vnum > max_room_vnum)
	    break;
	}
      
      /* Now connect the field rooms to each other and to the gates.
	 Note that if we only do one pass, this may lead to disconnected
	 field regions in places away from the gates, but it's better
	 than having random entrances to the city. */
      
      z = CITYGEN_STREET_LEVEL;
      for (x = 0; x < CITY_SIZE; x++)
	{
	  for (y = 0; y < CITY_SIZE; y++)
	    {
	      if (!city_coord_is_ok (x, y, z) ||
		  (room = city_grid[x][y][z]) == NULL ||
		  !is_named (room, "outer_field"))
		continue;
	      
	      for (dir = 0; dir < FLATDIR_MAX; dir++)
		{
		  nx = x;
		  ny = y;
		  nz = z;
		  if (dir == DIR_NORTH)
		    ny++;
		  else if (dir == DIR_SOUTH)
		    ny--;
		  else if (dir == DIR_EAST)
		    nx++;
		  else if (dir == DIR_WEST)
		    nx--;
		  
		  /* Add exits if the adjacent room is a field or gate. */
		  if (city_coord_is_ok (nx, ny, nz) &&
		      (nroom = city_grid[nx][ny][nz]) != NULL &&
		      (is_named (nroom, "outer_field") ||
		       is_named (nroom, "gate_house")))
		    {
		      room_add_exit (room, dir, nroom->vnum, 0);
		      room_add_exit (nroom, RDIR(dir), room->vnum, 0);
		    }
		}
	    }
	}
      
      /* Now clear the "field depths to 1 so that we can add another 
	 layer maybe. */
      
      z = CITYGEN_STREET_LEVEL;
      for (x = 0; x < CITY_SIZE; x++)
	{
	  for (y = 0; y < CITY_SIZE; y++)
	    {
	      if (city_coord_is_ok (x, y, z) &&
		  (room = city_grid[x][y][z]) != NULL &&
		  room->mv == CITYGEN_FIELD_DEPTH)
		room->mv = 1;
	    }
	}
    }

  /* Now extend roads out from "gate_house" rooms in the
     direction of "outer_field" rooms to set the rooms that are
     roads leading to the cities --- and set the rooms to be "dir_edge"
     rooms, as well so that the worldgen code can link cities in correctly. */
  
  z = CITYGEN_STREET_LEVEL;
  for (x = 0; x < CITY_SIZE; x++)
    {
      for (y = 0; y < CITY_SIZE; y++)
	{
	  if ((start_room = city_grid[x][y][z]) != NULL &&
	      is_named (start_room, "gate_house"))
	    {	      
	      name[0] = '\0';
	      if (nr (1,2) == 1)
		sprintf (name, "The Road ");
	      sprintf (name + strlen(name), "%s %s",
		       (nr (1,3) == 2 ? "Near" :
			(nr (1,2) == 2 ? "Outside" :
			 "Just Outside")),
		       (nr (1,2) == 1 ?
			city_name : city_full_name));
	      
	      for (dir = 0; dir < FLATDIR_MAX; dir++)
		{
		  room = start_room;
		  last_room = NULL;
		  do
		    {
		      if ((exit = FNV (room, dir + 1)) != NULL &&
			  (nroom = find_thing_num (exit->val[0])) != NULL &&
			  IS_ROOM (nroom) &&
			  is_named (nroom, "outer_field"))
			{
			  free_str (nroom->short_desc);
			  nroom->short_desc = new_str (name);
			  remove_flagval (nroom, FLAG_ROOM1, ~0);
			  add_flagval (nroom, FLAG_ROOM1, ROOM_EASYMOVE);
			  last_room = nroom;
			  room = nroom;
			}
		      else
			  nroom = NULL;
		    }
		  while (nroom);
		  
		  if (last_room)
		    {
		      sprintf (name, "%s_edge",
			       dir_name [dir]);
		      append_name (last_room, name);
		    }
		}
	    }
	}
    }

  /* Check if all 4 edges exist or not. */
  
  for (dir = 0; dir < FLATDIR_MAX; dir++)
    edge_exists[dir] = FALSE;
  
  for (room = area->cont; room; room = room->next_cont)
    {
      if (!IS_ROOM (room))
	continue;
      
      for (dir = 0; dir < FLATDIR_MAX; dir++)
	{
	  sprintf (buf, "%s_edge", dir_name[dir]);
	  if (is_named (room, buf))
	    edge_exists[dir] = TRUE;
	}
      
    }
  
  /* Now go deal with edges that don't exist. */

  z = CITYGEN_STREET_LEVEL;
  
  for (x = 0; x < CITY_SIZE; x++)
    {
      for (y = 0; y < CITY_SIZE; y++)
	{
	  if (city_coord_is_ok (x,y,z) &&
	      (room = city_grid[x][y][z]) != NULL)
	    {
	      if (x < min_x)
		min_x = x;
	      if (x > max_x)
		max_x = x;
	      if (y < min_y)
		min_y = y;
	      if (y > max_y)
		max_y = y;

	    }
	}
    }

  /* Now add edge rooms for each direction that lacks them. */

  z = CITYGEN_STREET_LEVEL;
  for (dir = 0; dir < FLATDIR_MAX; dir++)
    {
      if (edge_exists[dir])
	continue;
      
      room = NULL;
      if (dir == DIR_NORTH || dir == DIR_SOUTH)
	{
	  if (dir == DIR_NORTH)
	    y = max_y;
	  else if (dir == DIR_SOUTH)
	    y = min_y;
	  else
	    continue;
	  
	  num_choices = 0;
	  num_chose = 0;
	  for (count = 0; count < 2; count++)
	    {
	      for (x = 0; x < CITY_SIZE; x++)
		{
		  if (city_coord_is_ok (x,y,z) &&
		      (room = city_grid[x][y][z]) != NULL)
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
		    break;
		  num_chose = nr (1, num_choices);
		}
	    }
	}
      else if (dir == DIR_EAST || dir == DIR_WEST)
	{
	   if (dir == DIR_EAST)
	    x = max_x;
	  else if (dir == DIR_WEST)
	    x = min_x;
	  else
	    continue;
	  
	  num_choices = 0;
	  num_chose = 0;
	  for (count = 0; count < 2; count++)
	    {
	      for (y = 0; y < CITY_SIZE; y++)
		{
		  if (city_coord_is_ok (x,y,z) &&
		      (room = city_grid[x][y][z]) != NULL)
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
		    break;
		  num_chose = nr (1, num_choices);
		}
	    }
	}
      if (!room)
	continue;
      
      sprintf (buf, "%s_edge", dir_name[dir]);
      append_name (room, buf);
    }
  
  return;
}
