#include<stdio.h>
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#include "serv.h"
#include "mapgen.h"
#include "society.h"
#include "areagen.h"
#include "track.h"
#include "objectgen.h"
#include "roomdescgen.h"
#include "detailgen.h"
#include "mobgen.h"

/* Areagen.c was getting too long, so I started this. */


/* This adds underpasses for roads. Basically a room that is a road
   room and ((EW are road and NS are water) or 
   (NS are road and EW are water)) gets a new room put underneath it
   and gets connected to the other two water rooms. */

void
add_underpasses (THING *area)
{
  THING *rooms[REALDIR_MAX], *room, *newroom, *nroom;
  int roomflags[REALDIR_MAX];
  VALUE *exit;
  int dir;
  int main_dir = 0; /* Main dir we start from to avoid repeated code. */
  char roomname[STD_LEN];
  THING *belowroom, *nbelowroom;


  int rooms_missing; /* Number of missing rooms. Can be 0 or 1. */
  int ns_flags, ew_flags;
  if (!area || !IS_AREA (area))
    return;

  /* Loop through all rooms. */

  for (room = area->cont; room; room = room->next_cont)
    {
      RBIT (room->thing_flags, TH_MARKED);
      if (!IS_ROOM (room) ||
	  !IS_ROOM_SET (room, ROOM_EASYMOVE))
	continue;
      rooms_missing = 0;
      for (dir = 0; dir < FLATDIR_MAX; dir++)
	{
	  rooms[dir] = NULL;
	  roomflags[dir] = 0;
	  if ((exit = FNV (room, dir + 1)) != NULL &&
	      (rooms[dir] = find_thing_num (exit->val[0])) != NULL &&
	      !IS_ROOM (rooms[dir]))
	    rooms[dir] = NULL;
	  
	  if (rooms[dir])
	    roomflags[dir] = flagbits (rooms[dir]->flags, FLAG_ROOM1);
	  else
	    rooms_missing++;
	  
	}
      
      /* Now make sure the rooms are ok. */
      
      if (rooms_missing > 1)
	continue;

      /* Get the flags that are both NS and both EW to make checking
	 easier. */
      
      ns_flags = roomflags[DIR_NORTH] | roomflags[DIR_SOUTH];
      ew_flags = roomflags[DIR_EAST] | roomflags[DIR_WEST];
      
      /* First we need a pair of rooms that are not watery. If both
	 dirs have water, then don't do anything. */

      if (IS_SET (ns_flags & ew_flags, ROOM_WATERY))
	continue;
      
      /* Find the direction in which the underpass will go. */
      
      if (!IS_SET (ns_flags, ROOM_WATERY) &&
	  IS_SET (ew_flags, ROOM_WATERY))
	main_dir = DIR_EAST;
      else if (IS_SET (ns_flags, ROOM_WATERY) &&
	       !IS_SET (ew_flags, ROOM_WATERY))
	main_dir = DIR_NORTH;
      else 
	continue;
      
      if (main_dir != DIR_NORTH && main_dir != DIR_EAST)
	continue;

       /* Now create the name of this thing. */
      
      switch (nr(1,4))
	{
	  case 1:
	  default:
	    sprintf (roomname, "Under ");
	    break;
	  case 2:
	    sprintf (roomname, "Below ");
	    break;
	  case 3:
	    sprintf (roomname, "Beneath ");
	    break;
	  case 4:
	    sprintf (roomname, "Underneath ");
	    break;
	}      
      strcat (roomname, room->short_desc);
      
      /* The main dir needs at least one room and if that particular
	 room isn't there, pick the one in the opp dir. Make sure that
	 the room you pick is a watery room! */
      
      if (rooms[main_dir] == NULL || 
	  !IS_SET (roomflags[main_dir], ROOM_WATERY))
	{
	  main_dir = RDIR(main_dir);
	  if (rooms[main_dir] == NULL || !IS_SET (roomflags[main_dir], ROOM_WATERY))
	    continue;
	}
      
      /* Clear this exit for a second, then see if the room exists,
	 then if it does, clear this exit out. Has to be this way
	 since there's a chance that the newroom won't be created. 
	 if that happens, then this room will still be sitting here
	 not linked to anything. Bad situation. */
      
      if ((exit = FNV (rooms[main_dir], RDIR(main_dir) + 1)) != NULL)
	exit->type = 0;
      
      if ((newroom = make_room_dir 
	   (rooms[main_dir], RDIR(main_dir), 
	    roomname, ROOM_WATERY)) == NULL)
	{
	  if (exit)
	    exit->type = RDIR(main_dir) + 1;
	  continue;
	}
      remove_value (rooms[main_dir], exit);
      
      
      add_flagval (newroom, FLAG_ROOM1, ROOM_NOMAP);
      /* Newroom exists. Link up the new room to the other room
	 (if it exists!) . Also make sure that this room is a 
	 watery room. */
      
      if (rooms[RDIR(main_dir)] && 
	  IS_SET (roomflags[RDIR(main_dir)], ROOM_WATERY))
	{
	  if ((exit = FNV (rooms[RDIR(main_dir)], main_dir + 1)) == NULL)
	    {
	      exit = new_value();
	      exit->type = main_dir + 1;
	      add_value (rooms[RDIR(main_dir)], exit);
	    }
	  exit->val[0] = newroom->vnum;
	  
	  
	  if ((exit = FNV (newroom, RDIR (main_dir) + 1)) == NULL)
	    {
	      exit = new_value();
	      exit->type = RDIR(main_dir) + 1;
	      add_value (newroom, exit);
	    }
	  exit->val[0] = rooms[RDIR(main_dir)]->vnum;
	}
      
      /* Add exit "up" from the new room to the bridge itself...
	 but no exit DOWN to below the bridge. */
      
      if ((exit = FNV (newroom, DIR_UP + 1)) == NULL)
	{
	  exit = new_value();
	  exit->type = DIR_UP + 1;
	  add_value (newroom, exit);
	}      
      exit->val[0] = room->vnum;
      SBIT (room->thing_flags, TH_MARKED);
    }

  /* Now all overpasses are marked. Now link the below rooms to each
     other. */
  
  for (room = area->cont; room; room = room->next_cont)
    {
      if (!IS_ROOM (room) || !IS_MARKED (room) || 
	  !IS_ROOM_SET (room, ROOM_EASYMOVE))
	continue;
      
      /* Now see if any adjacent rooms are marked, and if so then
	 mark attach the underlying rooms also. */
      
      RBIT (room->thing_flags, TH_MARKED);
      if ((belowroom = find_below_room (room)) == NULL)
	continue;
      
      for (dir = 0; dir < FLATDIR_MAX; dir++)
	{
	  if ((exit = FNV (room, dir + 1)) != NULL &&
	      (nroom = find_thing_num (exit->val[0])) != NULL &&
	      IS_ROOM (nroom) && IS_MARKED (nroom) &&
	      (nbelowroom = find_below_room (nroom)) != NULL &&
	      belowroom != nbelowroom)
	    {
	      /* Link the two below rooms now. */

	      if ((exit = FNV (belowroom, dir + 1)) == NULL)
		{
		  exit = new_value();
		  exit->type = dir + 1;
		  add_value (belowroom, exit);
		}
	      exit->val[0] = nbelowroom->vnum;

	      if ((exit = FNV (nbelowroom, RDIR(dir) + 1)) == NULL)
		{
		  exit = new_value();
		  exit->type = RDIR(dir) + 1;
		  add_value (nbelowroom, exit);
		}
	      exit->val[0] = belowroom->vnum;
	    }
	}
    }
	      
  return;
}
					
/* This finds a room below an overpass. The below room is the room such
   that the RDIR of an exit heading out in direction dir from a room
   points back to a room that has an up exit to the original room. */


THING *
find_below_room (THING *room)
{
  THING *nroom, *belowroom;
  VALUE *exit, *rexit, *upexit;
  int dir;

  if (!room || !IS_ROOM (room))
    return NULL;


  for (dir = 0; dir < FLATDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (nroom) &&
	  (rexit = FNV (nroom, RDIR(dir) + 1)) != NULL &&
	  (belowroom = find_thing_num (rexit->val[0])) != NULL &&
	  belowroom != room &&
	  (upexit = FNV (belowroom, DIR_UP + 1)) != NULL &&
	  upexit->val[0] == room->vnum)
	return belowroom;
    }
  return NULL;
}

/* This adds some ridges to an area by picking some random rooms, then
   seeing if there are rooms in a row that have the following properties:
   
   1. They don't have down exits in them and all of the things in their
   "row" or "column" in the room grid don't have down exits. 
   2. Everything in the adjacent row or column doesn't have up exits.
 
*/

void
add_elevations (THING *area)
{
  THING *start_room, *room, *nroom;
  VALUE *exit;
  int ridge_dir;   /* Which direction the elevation follows. */
  int opp_dir; /* Which direction is side up. */
  int times, num_times, room_tries;
  int grid[AREAGEN_MAX][AREAGEN_MAX];
  int extremes[REALDIR_MAX];
  int start_x, start_y;
  int x, y, new_x, new_y;
  
  /* Are the rooms ok so far? */
  bool rooms_ok;
  bool found_room;  /* Did we find the room in the grid? */

  if (!area || !IS_AREA (area))
    return;
  
  if (area->cont == NULL || !IS_ROOM(area->cont))
    return;
  
  for (x = 0; x < AREAGEN_MAX; x++)
    {
      for (y = 0; y < AREAGEN_MAX; y++)
	{
	  grid[x][y] = 0;
	}
    }

  generate_room_grid (area->cont, grid, extremes, AREAGEN_MAX/2, AREAGEN_MAX/2);
  
  num_times = nr (0, area->mv/DETAIL_DENSITY);
  
  for (times = 0; times < num_times; times++)
    {
      start_room = NULL;
      rooms_ok = TRUE;
      for (room_tries = 0; room_tries < 50 && !start_room; room_tries++)
	{
	  if ((start_room = find_thing_num (nr(area->vnum + 1, area->vnum+area->mv))) == NULL)
	    continue;
	  found_room = FALSE;
	  for (start_x = 0; start_x < AREAGEN_MAX && !found_room; start_x++)
	    {
	      for (start_y = 0; start_y < AREAGEN_MAX && !found_room ; start_y++)
		{
		  if (grid[start_x][start_y] == start_room->vnum)
		    {
		      found_room = TRUE;
		      break;
		    }
		}
	    }
	  
	  if (!found_room || start_x < 1 || start_y < 1 ||
	      start_x >= AREAGEN_MAX-1 || start_y >= AREAGEN_MAX-1)
	    continue;
	  
	  /* Now pick an elevation dir. */

	  ridge_dir = nr (0, FLATDIR_MAX);
	  
	  /* Now find the opp dir where the elevation will go.
	     This is fairly simple: Use the 
	     elevation dir + 2 mod FLATDIR_MAX So it's in an opp dir
	     and the dirs are random. */
	  
	   opp_dir = (ridge_dir + 2) % FLATDIR_MAX;
	  
	 
	  /* Check N/S ridges. We always go UP from the room where
	     we're starting and down from the */
	  if (ridge_dir == DIR_NORTH  || ridge_dir == DIR_SOUTH)
	    {
	      if (opp_dir == DIR_EAST)
		new_x = start_x + 1;
	      else
		new_x = start_x - 1;
	      
	      for (y = 0; y < AREAGEN_MAX && rooms_ok; y++)
		{
		  /* If both rooms exist, they can't have U/D Exits. */
		  if ((room = find_thing_num (grid[start_x][y])) != NULL &&
		      (nroom = find_thing_num (grid[new_x][y])) != NULL &&
		      ((exit = FNV (room, DIR_UP + 1)) != NULL ||
		       (exit = FNV (room, DIR_DOWN + 1)) != NULL ||
		       (exit = FNV (nroom, DIR_DOWN + 1)) != NULL ||
		       (exit = FNV (nroom, DIR_UP + 1)) != NULL))
		    rooms_ok = FALSE;
		}
	      
	      if (!rooms_ok)
		continue;

	      /* Add the ridge by going down the same list of rooms
		 and adding up and down exits. */

	      for (y = 0; y < AREAGEN_MAX; y++)
		{
		  if ((room = find_thing_num (grid[start_x][y])) != NULL &&
		      (nroom = find_thing_num (grid[new_x][y])) != NULL)
		    {
		      /* Remove the old exits. */

		      if ((exit = FNV (room, opp_dir + 1)) != NULL)
			remove_value (room, exit);
		      if ((exit = FNV (nroom, RDIR(opp_dir) + 1)) != NULL)
			remove_value (nroom, exit);

		      /* Add in the new exits. */

		      exit = new_value();
		      exit->val[0] = nroom->vnum;
		      exit->type = DIR_UP + 1;
		      add_value (room, exit);
		      exit = new_value();
		      exit->val[0] = room->vnum;
		      exit->type = DIR_DOWN + 1;
		      add_value (nroom, exit);		      
		    }
		}
	    }
	  else
	    {
	      
	      if (opp_dir == DIR_NORTH)
		new_y = start_y + 1;
	      else
		new_y = start_y - 1;
	      
	      for (x = 0; x < AREAGEN_MAX && rooms_ok; x++)
		{
		  /* If both rooms exist, they can't have U/D Exits. */
		  if ((room = find_thing_num (grid[x][start_y])) != NULL &&
		      (nroom = find_thing_num (grid[x][new_y])) != NULL &&
		      ((exit = FNV (room, DIR_UP + 1)) != NULL ||
		       (exit = FNV (nroom, DIR_DOWN + 1)) != NULL))
		    rooms_ok = FALSE;
		}
	      /* If the rooms are ok, so add the ridge in. */
	      
	      if (!rooms_ok)
		continue;
	      
	       for (x = 0; x < AREAGEN_MAX; x++)
		 {
		   if ((room = find_thing_num (grid[x][start_y])) != NULL &&
		       (nroom = find_thing_num (grid[x][new_y])) != NULL)
		     { 
		       /* Remove the old exits. */

		       if ((exit = FNV (room, opp_dir + 1)) != NULL)
			remove_value (room, exit);
		      if ((exit = FNV (nroom, RDIR(opp_dir) + 1)) != NULL)
			remove_value (nroom, exit);

		      /* Add in the new exits. */
		      
		       exit = new_value();
		       exit->val[0] = nroom->vnum;
		       exit->type = DIR_UP + 1;
		       add_value (room, exit);
		       exit = new_value();
		       exit->val[0] = room->vnum;
		       exit->type = DIR_DOWN + 1;
		       add_value (nroom, exit);
		     }
		 }
	    }
	  if (rooms_ok)
	    break;
	}
    }
  return;
}
 
/* This tells if a room is ok to use in a elevation. */

bool
ok_elevation_room (THING *room, bool link_dir)
{
  VALUE *exit;
  
  if (!room || !IS_ROOM (room) || 
      (link_dir != DIR_DOWN && link_dir != DIR_UP))
    return FALSE;

  /* The room is bad if it's got a link in the direction we want to go
     or if it's above an underpass and we want to link down. */

  if ((exit = FNV (room, link_dir + 1)) != NULL ||
      (link_dir == DIR_DOWN && find_below_room (room)))
    return FALSE;
  
  return TRUE;
}



/* This adds catwalks to an area. It first generates a grid in the
   area and then it finds all of the trees and records their 
   x-y positions. Then it figures out how to connect those trees 
   by making catwalks that span from one tree to another. */


void
add_catwalks (THING *area)
{

  THING *room;
  
  /* Record the numbers of the trees and where they are in the
     x-y grid. */
  int tree[AREAGEN_TREE_MAX];
  THING *treeroom[AREAGEN_TREE_MAX];
  int tree_x[AREAGEN_TREE_MAX];
  int tree_y[AREAGEN_TREE_MAX];
  bool linked[AREAGEN_TREE_MAX]; /* Are these trees linked? */
  int num_trees = 0;
  
  int grid[AREAGEN_MAX][AREAGEN_MAX];
  int extremes[REALDIR_MAX];
  int extremetree[REALDIR_MAX];
  int dist[AREAGEN_TREE_MAX][AREAGEN_TREE_MAX];
  
  /* Used to find the shortest distance between two unattached
     trees to make the spanning tree. */
  
  int shortest_dist, tree_from, tree_to;
  int x, y, num, i, j, dir;
  int branch_dir;
  char name[STD_LEN];
  int dx, dy; /* The change in x and y required to make the catwalk. */
  
  /* Branches where the catwalks start and end. */
  
  THING *branch_from, *branch_to;
  
  if (!area || !IS_AREA (area) || 
      IS_ROOM_SET (area, ROOM_WATERY | ROOM_UNDERGROUND | ROOM_DESERT))
    return;

  /* Need a starting room. */
  
  /* Start with the first room in the area. */
  if ((room = find_thing_num (area->vnum + 1)) == NULL)
    return;
  
  /* Init data. */
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    extremes[dir] = AREAGEN_MAX/2;
  
  for (x = 0; x < AREAGEN_MAX; x++)
    {
      for (y = 0; y < AREAGEN_MAX; y++)
	{
	  grid[x][y] = 0;
	}
    }
  
  for (i = 0; i < AREAGEN_TREE_MAX; i++)
    {
      for (j = 0; j < AREAGEN_TREE_MAX; j++)
	{
	  dist[i][j] = 0;
	}
    }
  
 /* Clear the tree data. */

  for (num = 0; num < AREAGEN_TREE_MAX; num++)
    {
      tree[num] = 0;
      treeroom[num] = NULL;
      tree_x[num] = 0;
      tree_y[num] = 0;
      linked[num] = FALSE;
    }

  /* Now make the grid. */

  generate_room_grid (room, grid, extremes, AREAGEN_MAX/2, AREAGEN_MAX/2);
  
 
  
  /* Now loop through the grid and find all trees. */
  num_trees = 0;
  for (x = 0; x < AREAGEN_MAX; x++)
    {
      for (y = 0; y < AREAGEN_MAX; y++)
	{
	  if (grid[x][y] == 0)
	    continue;
	  if ((room = find_thing_num (grid[x][y])) == NULL)
	    continue;
	  if (!IS_ROOM (room) || !room->name || !*room->name)
	    continue;
	  if (!is_named (room, "tree_bottom"))
	    continue;
	  /* Room exists and it's a tree bottom. Add it to the list. */
	  
	  if (num_trees >= AREAGEN_TREE_MAX)
	    {
	      char errbuf[STD_LEN];
	      sprintf (errbuf, "Too many trees in area %d. Increase AREAGEN_TREE_MAX!\n", area->vnum);
	      log_it (errbuf);
	      continue;
	    }
	  
	  tree[num_trees] = grid[x][y];
	  tree_x[num_trees] = x;
	  tree_y[num_trees] = y;
	  treeroom[num_trees] = room;
	  num_trees++;
	  
	}
    }


  /* Now find the farthest NEWS trees so we can attach trees in 
     other areas to these trees later on. */
  
  extremes[DIR_NORTH] = -1;
  extremes[DIR_EAST] = -1;
  extremes[DIR_WEST] = AREAGEN_MAX;
  extremes[DIR_SOUTH] = AREAGEN_MAX;

  for (i = 0; i < REALDIR_MAX; i++)
    extremetree[i] = 0;
     
  for (i = 0; i <= num_trees; i++)
    {
      if (tree[i] == 0)
	continue;


      if (tree_x[i] >= extremes[DIR_EAST])
	{
	  extremes[DIR_EAST] = tree_x[i];
	  extremetree[DIR_EAST] = tree[i];
	}


      if (tree_x[i] <= extremes[DIR_WEST])
	{
	  extremes[DIR_WEST] = tree_x[i];
	  extremetree[DIR_WEST] = tree[i];
	}
      
      if (tree_y[i] >= extremes[DIR_NORTH])
	{
	  extremes[DIR_NORTH] = tree_y[i];
	  extremetree[DIR_NORTH] = tree[i];
	}


      if (tree_y[i] <= extremes[DIR_SOUTH])
	{
	  extremes[DIR_SOUTH] = tree_y[i];
	  extremetree[DIR_SOUTH] = tree[i];
	}
    }
  
  /* Now set the tree names for the extreme trees. This will be used
     to link catwalks between areas in the worldgen code. */

  for (i = 0; i < FLATDIR_MAX; i++)
    {
      if ((room = find_thing_num (extremetree[i])) != NULL &&
	  IS_ROOM (room) &&
	  is_named (room, "tree_bottom"))
	{
	  sprintf (name, "%s_tree", dir_name[i]);
	  append_name (room, name);
	}
    }
		   
      
  

  /* Now we have a list of all of the trees. I now want to find the
     distance matrix between all pairs of trees. The distance is the
     |dx|+|dy| distance since it's really all about the rooms anyway. */

  /* Can replace AREAGEN_TREE_MAX with num_trees */
  
  for (i = 0; i < AREAGEN_TREE_MAX; i++)
    {
      for (j = 0; j < AREAGEN_TREE_MAX; j++)
	{
	  if (i == j || tree[i] == 0 || tree[j] == 0)
	    continue;
	  	  
	  
	  dist[i][j] = ABS(tree_x[i]-tree_x[j]) + ABS (tree_y[i]-tree_y[j]);
	 
	}
    }
  
  
  /* Now start to link trees. Link 1 to shortest, 1 and 2 to shortest
     1-3 to shortest and so forth. */
  
  /* First tree is linked to itself. */
  
  linked[0] = TRUE;
  for (num = 0; num < num_trees; num++)
    {
      shortest_dist = 100000;
      tree_to = -1;
      tree_from = -1;
      /* Loop through all linked trees. */
      for (i = 0; i < num_trees; i++)
	{
	  if (!linked[i])
	    continue;
	  
	  /* For each linked tree, loop through all unlinked trees
	     and find the shortest distance to one of those
	     trees. */
	  
	  for (j = 0; j < num_trees; j++)
	    {
	      if (linked[j])
		continue;
	      
	      if (dist[i][j] < shortest_dist)
		{
		  shortest_dist = dist[i][j];
		  tree_to = j;
		  tree_from = i;
		}
	    }
	}
      
      if (tree_to >= 0 && tree_to < AREAGEN_TREE_MAX)
	linked[tree_to] = TRUE;
      else
	continue;
      
      /* Now see if a shortest path exists or not. */

      if (tree_to < 0 || tree_to >= num_trees ||
	  tree_from < 0 || tree_from >= num_trees ||
	  !treeroom[tree_from] || !treeroom[tree_to])
	continue;
      
      dx = tree_x[tree_to] - tree_x[tree_from];
      dy = tree_y[tree_to] - tree_y[tree_from];

      if (dx == 0 && dy == 0)
	continue;
      
      if (ABS(dx) > ABS(dy))
	{
	  if (dx > 0)
	    branch_dir = DIR_EAST;
	  else
	    branch_dir = DIR_WEST;
	}
      else
	{
	  if (dy > 0)
	    branch_dir = DIR_NORTH;
	  else
	    branch_dir = DIR_SOUTH;
	}
      
      
      if ((branch_from = find_tree_branch (treeroom[tree_from], branch_dir)) == NULL ||
	  (branch_to = find_tree_branch (treeroom[tree_to], branch_dir)) == NULL)
	continue;
      
      /* We now have the branches, so attach them. Assume that 
	 if ABS(dx) or ABS(dy) are > 0, they get reduced by
	 1-2 each to represent the branch distances. */
      
      if (dx > 0)
	dx = MAX (1, dx - 1);
      else if (dx < 0)
	dx = MIN (-1, dx + 1);
      
      if (dy > 0)
	dy = MAX (1, dy - 1);
      else if (dy < 0)
	dy = MIN (-1, dy + 1);
      
      /* This shouldn't happen. */
      
      if (dx == 0 && dy == 0)
	continue;
      
      make_catwalk (branch_from, branch_to, dx, dy);
      
    }
  
  
  return;
}

/* This finds a tree in the area either at a certain side, or
   a random tree in the area. */

THING *
find_tree_area (THING *area, int dir)
{
  int num_choices = 0, num_chose = 0, count = 0;
  THING *room;
  char name[STD_LEN];
  if (!area || !IS_AREA (area))
    return NULL;

  /* Find a tree with a certain direction named in it. */
  if (dir >= DIR_NORTH && dir < FLATDIR_MAX)
    {
      sprintf (name, "%s_tree", dir_name[dir]);
      for (room = area->cont; room; room = room->next_cont)
	{
	  if (IS_ROOM (room) && is_named (room, "tree_bottom") &&
	      is_named (room, name))
	    return room;
	}
      return NULL;
    }

  for (count = 0; count < 2; count++)
    {
      
      for (room = area->cont; room; room = room->next_cont)
	{
	  if (!IS_ROOM (room) || !is_named (room, "tree_bottom"))
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
  
  if (room && IS_ROOM (room) && is_named(room, "tree_bottom"))
    return room;
  return NULL;
}

/* This finds a branch in the tree heading in the "right" direction
   so that a catwalk can be made between the trees. */

THING *
find_tree_branch (THING *tree_base, int dir)
{
  THING *room;
  VALUE *exit;
  /* Number of sets of branches choices we have. */
  int num_choices = 0, num_chose = 0, count;
  /* What direction the branch will take from the base of the tree. */
  
  int times;

  /* Make sure that the tree base is ok and the dir is ok. */
  
  if (!tree_base || !IS_ROOM (tree_base) || dir < 0 || dir >= 4)
    return NULL;

  
  
  /* Now search up the tree counting the number of branches that go off
     in that specified direction. */

  for (count = 0; count < 2; count++)
    {
      room = tree_base;
      times = 0;
      do
	{
	  /* Check if the next room up exists. If not, bail out. */
	  if ((exit = FNV (room, DIR_UP + 1)) == NULL)
	    break;
	  
	  if ((room = find_thing_num (exit->val[0])) == NULL)
	    break;
	  
	  /* See if this room has a branch in the proper direction. */
	  if ((exit = FNV (room, dir + 1)) == NULL)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      while (++times < 20);
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;
	  num_chose = nr (1, num_choices);
	}
    }
  
  /* If we have a room and a certain branch in the correct
     direction then head off in that direction as long as you
     can (up to 20 times) and return the end of the branch. */
  
  times = 0;
  do
    {
      if ((exit = FNV (room, dir + 1)) == NULL)
	break;
      if ((find_thing_num (exit->val[0])) == NULL)
	break;
      
      if ((room = find_thing_num (exit->val[0])) == NULL)
	break;
    }
  while (++times < 20); /* Stop if it goes too far. */
  
  return room;
}


/* This actually draws a catwalk from one branch to another. */

void
make_catwalk (THING *branch_from, THING *branch_to, int dx, int dy)
{
  THING *room, *curr_room;
  VALUE *exit;
  int count = 0;
  int abs_dx, abs_dy;
  int num_rooms_made = 0;
  /* The main direction and the opposite directions in which the
     catwalk goes. Based on relative magnitudes of dx and dy. */
  
  int main_dir, opp_dir;

  /* The x dir and the y dir in which the cat walk goes. Based on the
     signs of dx and dy. */
  int x_dir, y_dir;
  
  
  int num, max_rooms;
  char fullroomname[STD_LEN];
  char roomname[STD_LEN];
  /* Branches must exist. */

  if (!branch_from || !IS_ROOM (branch_from) || !branch_to ||
      !IS_ROOM (branch_to))
    return;

  if (dx == 0 && dy == 0)
    return;

  /* Small chance of adding secret doors to both ends. */


  abs_dx = ABS(dx);
  abs_dy = ABS(dy);
  
  /* Set the overall directions dx and dy = 0 won't matter.  */
  if (dx > 0)
    x_dir = DIR_EAST;
  else 
    x_dir = DIR_WEST;

  if (dy > 0)
    y_dir = DIR_NORTH;
  else
    y_dir = DIR_SOUTH;

  if (abs_dx > abs_dy)
    {
      main_dir = x_dir;
      opp_dir = y_dir;
    }
  else
    {
      main_dir = y_dir;
      opp_dir = x_dir;
    }

  max_rooms = MAX (abs_dx, abs_dy);
  
  /* First make sure there aren't any excess rooms at the end of either
     branch. There shouldn't be but if there are, try to go past them. */
  
  curr_room = branch_from;
  
  do
    {
      if ((exit = FNV (curr_room, main_dir + 1)) != NULL &&
	  (room = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (room))
	curr_room = room;
      else
	break;
      max_rooms--;
    }
  while (curr_room && ++count < 20);
  
  /* If there are too many rooms in this direction, then just bail out. */
  if (!curr_room ||
      (exit = FNV (curr_room, main_dir + 1)) != NULL)
    return;
  
  branch_from = curr_room;

  /* Do the same for the end branch except this time look in the
     opposite direction. */

  curr_room = branch_to;

  do
    {
      if ((exit = FNV (curr_room, RDIR(main_dir) + 1)) != NULL &&
	  (room = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (room))
	curr_room = room;
      else
	break;
      max_rooms--;
    }
  while (curr_room && ++count < 20);
  
  /* If there are too many rooms in this direction, then just bail out. */
  if (!curr_room ||
      (exit = FNV (curr_room, RDIR(main_dir) + 1)) != NULL)
    return;
  
  branch_to = curr_room;
  
  /* So the branch from and branch to _should_ be ok. */

  /* Now generate the rooms. Make sure there are at least 3 rooms. */

  if (max_rooms < 3)
    max_rooms = 3;
  
  /* Now create a name. */
  
  strcpy (roomname, find_gen_word (AREAGEN_AREA_VNUM, "bridge_prefixes", NULL));
	 
  if (strstr (roomname, "stone") ||
      strstr (roomname, "rock"))
    roomname[0] = '\0';
  if (*roomname)
    strcat (roomname, " ");
  strcat (roomname, find_gen_word (AREAGEN_AREA_VNUM, "catwalk_names", NULL));
 
  

  if (nr (1,8) == 3)
    {
      if (nr (1,5) == 2)
	strcat (roomname, " Between Trees");
      else
	{
	  if (nr (1,17) == 2)
	    strcat (roomname, " Very High");
	  else if (nr (1,15) == 3)
	    strcat (roomname, " High");
	  
	  if (nr (1,9) == 6)
	    strcat (roomname, " Up");
	  
	  strcat (roomname, " in the Trees");
	}
    }
  
  sprintf (fullroomname, "%s %s",
	   a_an(roomname), roomname);
  curr_room = branch_from;
  
  
  
  for (num = 0; num < max_rooms; num++)
    {
      if ((room = make_room_dir (curr_room, main_dir, fullroomname, ROOM_EASYMOVE)) == NULL)
	break;
      append_name (room, "catwalk");
      /* Check if the catwalk shifts in the opposite direction. */
      
      if ((abs_dx > abs_dy && nr (1,abs_dx) <= abs_dy) ||
	  (abs_dy > abs_dx && nr (1,abs_dy) <= abs_dx))
	{
	  curr_room = room;
	  if ((room = make_room_dir (curr_room, opp_dir, fullroomname, ROOM_EASYMOVE)) == NULL)
	    break;  
	  append_name (room, "catwalk");
	} 
      
      num_rooms_made++;
      curr_room = room;
    }
 
  /* If no rooms were made (out of rooms) then don't make the
     catwalk after all. This is overridden if this is a catwalk
     between areas. Then we make it happen anyway. Small chance of
     "el cheapo" teleport from one area to another this way, but 
     not much I can do about it cheaply and easily. Maybe will 
     fix later. */

  if (num_rooms_made < 1 && branch_from->in == branch_to->in)
    return;
 
  /* Attach the exits at the end but only if it makes sense...
     this can lead to bad catwalks, but I don't know what to do if
     this stuff fails. */
  
  if (curr_room)
    {
      if ((exit = FNV (curr_room, main_dir + 1)) == NULL)
	{
	  exit = new_value();
	  add_value (curr_room, exit);
	}
      exit->type = main_dir + 1;
      exit->val[0] = branch_to->vnum;
      
      if ((exit = FNV (branch_to, RDIR(main_dir) + 1)) == NULL)
	{
	  exit = new_value();
	  add_value (branch_to, exit);
	}
      exit->type = RDIR(main_dir) + 1;
      exit->val[0] = curr_room->vnum;
    }
 

  /* Now add secret doors if necessary. */

  if (nr (1,15) == 10)
    {
      place_secret_door (branch_from, main_dir, generate_secret_door_name());
      place_secret_door (branch_to, RDIR(main_dir), generate_secret_door_name());
    }
 
  if (branch_to->in)
    SBIT (branch_to->in->thing_flags, TH_CHANGED);
  if (branch_from->in)
    SBIT (branch_from->in->thing_flags, TH_CHANGED);

 
  
  return;
}
	

/* This finds if there are any loop exits in an area and if so
   where they are and what stage we're at. */

void
check_loop_exits (THING *area, int num)
{
  THING *room;
  VALUE *exit;
  char buf[STD_LEN];
  
  if (!area || !IS_AREA (area))
    return;

  for (room = area->cont; room; room = room->next_cont)
    {
      if (!IS_ROOM (room))
	continue;
      
      for (exit = room->values; exit; exit = exit->next)
	{
	  if (exit->type <= 0 || exit->type > REALDIR_MAX)
	    continue;
	  
	  if (exit->val[0] == room->vnum)
	    {
	      sprintf (buf, "Loop Room: %d Stage: %d\n", 
		       room->vnum, num);
	      echo (buf);
	    }
	}
    }
  return;
}

/* This places a secret door with a certain name in a certain
   direction from a room. */


void
place_secret_door (THING *room, int dir, char *name)
{
  THING *nroom;
  VALUE *exit, *nexit;

  if (!room || !IS_ROOM (room) || dir < 0 || dir >= REALDIR_MAX ||
      !name || !*name)
    return;

  if ((exit = FNV (room, dir + 1)) == NULL ||
      (nroom = find_thing_num (exit->val[0])) == NULL ||
      (nexit = FNV (nroom, RDIR(dir) + 1)) == NULL ||
      nexit->val[0] != room->vnum || 
      IS_SET (exit->val[2], EX_HIDDEN) ||
      IS_SET (nexit->val[2], EX_HIDDEN))
    return;

  exit->val[2] |= EX_DOOR | EX_CLOSED | EX_HIDDEN;
  exit->val[1] |= exit->val[2];
  set_value_word (exit, name);
  nexit->val[2] |= EX_DOOR | EX_CLOSED | EX_HIDDEN;
  nexit->val[1] |= nexit->val[2];
  set_value_word (nexit, name);
  
  return;
}

/* This generates a secret door name. */

char *
generate_secret_door_name (void)
{
  static char name[STD_LEN];
  
  strcpy (name, find_gen_word (AREAGEN_AREA_VNUM, "secret_door_prefixes", NULL));
  strcat (name, find_gen_word (AREAGEN_AREA_VNUM, "secret_door_names", NULL));
  return name;
}

/* This adds secret doors to the area. Secret doors are added inside
   of cave areas either touching the outside with only one exit to
   the outside and no adjacent rooms touching the outside, and 
   also to rooms which have only 1-2 exits so that the caves become 
   more difficult to navigate. Also, the doors added to UNDERGROUND
   areas are much less frequent to keep things from getting 
   really annoying. */


void
add_secret_doors (THING *area)
{
  THING *room;
  THING *nroom, *nroom2;
  VALUE *exit, *exit2;
  int dir, dir2, room_flags, outside_exit_dir;
  /* Number of exits from this room to the outside world. */
  int num_outside_exits = 0;

  /* Are the adjacent rooms all ok -- either 1 outdoor, or they 
     don't connect to the outside. */
  bool adjacent_rooms_ok;
  
  if (!area || !IS_AREA (area))
    return;

  /* First add secret doors to underground/outside connections
     where there's only one room touching the outside and the adjacent
     rooms don't touch the outside. */

  for (room = area->cont; room; room = room->next_cont)
    {
      if (!IS_ROOM (room) || !IS_ROOM_SET (room, ROOM_UNDERGROUND))
	continue;

      adjacent_rooms_ok = TRUE;
      num_outside_exits = 0;
      outside_exit_dir = REALDIR_MAX;
      for (dir = 0; dir < REALDIR_MAX; dir++)
	{
	  if ((exit = FNV (room, dir + 1)) == NULL ||
	      (nroom = find_thing_num (exit->val[0])) == NULL)
	    continue;
	  
	  /* Ok so the adjacent room exists. If it's not an underground
	     room, we increment the number of rooms that are outside
	     exits. Otherwise, we make sure the room has no adjacent 
	     outside exits. */

	  if (!IS_ROOM_SET (nroom, ROOM_UNDERGROUND))
	    {
	      outside_exit_dir = dir;
	      num_outside_exits++;
	    }
	  else
	    {
	      for (dir2 = 0; dir2 < REALDIR_MAX; dir2++)
		{
		  /* If the adjacent room exists, see if its room
		     sector flags are exactly underground or not. */
		  if ((exit2 = FNV (nroom, dir2 + 1)) != NULL &&
		      (nroom2 = find_thing_num (exit2->val[0])) != NULL &&
		      IS_ROOM (nroom2))
		    {
		      
		      if (((room_flags = flagbits (nroom2->flags, FLAG_ROOM1)) 
			   & ROOM_SECTOR_FLAGS) != ROOM_UNDERGROUND)
			adjacent_rooms_ok = FALSE;
		    }
		}
	    }
	}
      
      if (num_outside_exits != 1 || !adjacent_rooms_ok)
	continue;
    
      place_secret_door (room, outside_exit_dir, generate_secret_door_name());


      /* Now mark the rooms nearby and see if there's an area on one
	 side or another of the secret door that's really small. If there
	 is, then we want to put a powerful mob there with some guards
	 to pop some eq. */

      add_guarded_mob (room);
	 
	 

    }

 

  return;
}


/* This tries to add a powerful mob to a room in such a way that it
   sees if this room cuts the area into a small and large set, and
   if so, it tries to put the mob into the "small" area. */

void
add_guarded_mob (THING *room)
{
  THING *nroom = NULL, *person_proto = NULL;
  THING *realroom = NULL;
  VALUE *exit;
  int dir, num_choices = 0, num_chose = 0, count;
  int person_tries;
  char name[STD_LEN];
  
  char fullname[STD_LEN];
  /* Depths leading toward underground areas and away from
     underground areas. */

  
  int inner_rooms = 0, outer_rooms = 0;
  bool inner_mob = FALSE, outer_mob = FALSE;
  
  undo_marked (room);
  inner_rooms = find_num_adjacent_rooms (room, ROOM_UNDERGROUND);
  undo_marked (room);
  outer_rooms = find_num_adjacent_rooms (room, ~ROOM_UNDERGROUND);
  undo_marked (room);
  
  /* Only one of these is possible. (if any) */
  if (outer_rooms >= 100 && outer_rooms/8 > inner_rooms)
    {
      inner_mob = TRUE;
      realroom = room;
    }
  else if (inner_rooms >= 100 && inner_rooms/8 > outer_rooms)
    {
      /* Find a random room connected that isn't an underground room. */

      for (count = 0; count < 2; count++)
	{
	  for (dir = 0; dir < REALDIR_MAX; dir++)
	    {
	      if ((exit = FNV (room, dir + 1)) != NULL &&
		  (nroom = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM (room) &&
		  !IS_ROOM_SET (nroom, ROOM_UNDERGROUND))
		{
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
	}
      if (!nroom)
	return;
      realroom = nroom;
      outer_mob = TRUE;
    }
  else /* Neither side is really small compared to the other...*/
    return;
  
  if (!realroom || !realroom->in)
    return;

  /* We now have a room, now try to generate a person. */
  
  for (person_tries = 0; person_tries < 20; person_tries++)
    {
      if ((person_proto = areagen_generate_person (realroom->in, LEVEL(realroom->in)/3)) != NULL)
	break;

    }

  if (!person_proto)
    return;

  /* Remove all resets for this person and add a new one in the proper
     room. Also make the person sentinel. */
  remove_resets (realroom->in, person_proto->vnum, person_proto->vnum);
  add_reset (realroom, person_proto->vnum, 100, 1, 1);
  add_flagval (person_proto, FLAG_ACT1, ACT_SENTINEL);
  
  /* Now try to make guards. */

  for (person_tries = 0; person_tries < 20; person_tries++)
    {
      if ((person_proto = areagen_generate_person (realroom->in, 0)) != NULL)
	break;
    }
  
  if (!person_proto)
    return;

  if (nr (1,3) == 2)
    strcpy (name, "buffed ");
  else if (nr (1,2) == 1)
    strcpy (name, "elite ");
  else
    name[0] = '\0';
  
  if (nr (1,4) == 4)
    strcat (name, "guard");
  else if (nr (1,3) == 2)
    strcat (name, "guardian");
  else if (nr (1,2) == 1)
    strcat (name, "bodyguard");
  else 
    strcat (name, "warrior");
  
  free_str (person_proto->name);
  person_proto->name = new_str (name);
  
  sprintf (fullname, "%s %s", a_an (name), name);
  free_str (person_proto->short_desc);
  person_proto->short_desc = new_str (fullname);
  
  strcat (fullname, " is here.");
  free_str (person_proto->long_desc);
  person_proto->long_desc = new_str (fullname);
  person_proto->max_mv = nr (3,5);
  add_flagval (person_proto, FLAG_ACT1, ACT_SENTINEL);
  add_reset (realroom, person_proto->vnum, 100, person_proto->max_mv, 1);
  return;
}
  
/* This adds shoreline rooms to an area. The idea is to look for rooms
   next to watery rooms that aren't designated as "shore" rooms and give
   them watery names 1/N of the time. */

void 
add_shorelines (THING *area)
{
  THING *room, *nroom = NULL;
  /* Used to decide which watery room to name. */
  int count, num_choices, num_chose, dir;
  VALUE *exit;
  char name[STD_LEN], *t;
  
  if (!area || !IS_AREA (area))
    return;

  for (room = area->cont; room; room = room->next_cont)
    {
      /* Only do nonwater nonroad rooms that aren't shores. */
      if (!IS_ROOM (room) || is_named (room, "sector_patch") ||
	  is_named (room, "detail") || is_named (room, "shore") ||
	  IS_ROOM_SET (room, ROOM_WATERY | ROOM_EASYMOVE) ||
	  nr (1,4) != 2)
	continue;
	  
      num_choices = 0;
      num_chose = 0;
      
      for (count = 0; count < 2; count++)
	{
	  for (dir = 0; dir < FLATDIR_MAX; dir++)
	    {
	      if ((exit = FNV (room, dir + 1)) != NULL &&
		  (nroom = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM_SET (nroom, ROOM_WATERY) &&
		  !is_named (nroom, "shore") &&
		  !is_named (nroom, "detail") &&
		  !is_named (nroom, "sector_patch") &&
		  nroom->short_desc && *nroom->short_desc)
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
      if (dir < FLATDIR_MAX &&
	  nroom && IS_ROOM_SET (nroom, ROOM_WATERY))
	{
	  sprintf (name, string_gen ("close_by shoreline", AREAGEN_AREA_VNUM));
	  capitalize_all_words (name);
	  for (t = name; *t; t++)
	    {
	      if (*t == '\n' || *t == '\r')
		{
		  *t = '\0';
		  break;
		}
	    }
	  strcat (name, " ");
	  strcat (name, nroom->short_desc);
	  free_str (room->short_desc);
	  room->short_desc = new_str (name);
	  append_name (room, "shore");
	}
    }
  return;
}
	  
