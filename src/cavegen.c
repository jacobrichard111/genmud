#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "serv.h"
#include "areagen.h"
#include "cavegen.h"


/* This is the cavegen grid...it gets used for bits at first, then
   it's set with the room numbers we will use. */

static int cave_grid[CAVEGEN_MAX*2+1][CAVEGEN_MAX*2+1][CAVEGEN_MAX];

/* These are the min/max xyz variables so that the code will
   be a bit faster here. */
static int min_x, max_x, min_y, max_y, min_z, max_z;

#define USED_CAVE(x,y,z) (IS_SET(cave_grid[(x)][(y)][(z)],CAVEGEN_USED))

void 
do_cavegen (THING *th, char *arg)
{
  cavegen (th, arg);
  return;
}


bool
cavegen (THING *th, char *arg)
{
  /* Dimensions of the cave. */
  int dx = 0, dy = 0, dz = 0;
  int total_rooms = 0;
  int x, y, z;
  int count = 0;
  char arg1[STD_LEN];
  int total_dist, val;
  int tries = 0, i;
  int max_room_add_tries;
  if (!arg || !*arg)
    {
      stt ("Cavegen <rooms|size|num> <size> or cavegen <dx> [<dy>] [<dz>] or cavegen show or cavegen generate vnum\n\r", th);
      return FALSE;
    }


  arg = f_word (arg, arg1);

  if (!str_cmp (arg1, "show"))
    {
      cavegen_show (th);
      return FALSE;
    }
  
  if (!str_cmp (arg1, "generate"))
    {
      cavegen_generate (th, atoi(arg));
      return FALSE;
    }

  
  if (!str_cmp (arg1, "size") ||
      !str_cmp (arg1, "num") ||
      !str_cmp (arg1, "room") ||
      !str_cmp (arg1, "rooms"))
    {
      total_rooms = atoi (arg);
      if (total_rooms < CAVEGEN_MIN_ROOMS)
	{ 
	  stt ("Cavegen <rooms|size|num> <size >= 100> or cavegen <dx> [<dy>] [<dz>]\n\r", th);
	  return FALSE;
	}
      for (dx = 1; dx*dx*dx < total_rooms*total_rooms/20; dx++);
      
      dx = nr (dx*4/5, dx*6/5);
      if (dx < 5)
	dx = 5;
      dy = nr (dx*4/5,dx*6/5);
      dz = 2;
      
    }
  else
    {
      dx = atoi (arg1);
      if (dx < 5)
	dx = 5;
      arg = f_word (arg, arg1);
      
      if (!*arg1)
	{ 
	  stt ("Cavegen <rooms|size|num> <size >= 100> or cavegen <dx> [<dy>] [<dz>]\n\r", th);
	  return FALSE;
	}
      dy = atoi (arg1);
      if (dy < 1)
	dy = dx;
      else if (dy < 5)
	dy = 5;
      if (!*arg)
	{ 
	  stt ("Cavegen <rooms|size|num> <size >= 100> or cavegen <dx> [<dy>] [<dz>]\n\r", th);
	  return FALSE;
	}
      dz = atoi (arg);
      if (dz < 1)
	dz = dx*2/5;
    }
  
  /* Make sure that the dimensions will fit in the space allotted. */
  if (dx >CAVEGEN_MAX*2-2)
    dx = CAVEGEN_MAX*2-2;
  if (dy > CAVEGEN_MAX*2-2)
      dy = CAVEGEN_MAX*2-2;
  if (dz >= CAVEGEN_MAX -2)
    dz = CAVEGEN_MAX-2;
  
  /* Now set the min/max x/y/z. */
  
  /* NOTE: This is the only place these variables should EVER
     be set or else bad things may happen since other loops in
     other functions depend on these being correct. The smallest 
     the variables can be is 1 and the largest is max size - 1*/
  
  min_x = MID (CAVEGEN_NEAR_DIST, CAVEGEN_MAX-(dx-1)/2, CAVEGEN_MAX*2 - CAVEGEN_NEAR_DIST);
  max_x = MID (CAVEGEN_NEAR_DIST,  CAVEGEN_MAX+dx/2, CAVEGEN_MAX*2-CAVEGEN_NEAR_DIST);  
  min_y = MID (CAVEGEN_NEAR_DIST, CAVEGEN_MAX-(dy-1)/2, CAVEGEN_MAX*2-CAVEGEN_NEAR_DIST);
  max_y = MID (CAVEGEN_NEAR_DIST,  CAVEGEN_MAX+dy/2, CAVEGEN_MAX*2-CAVEGEN_NEAR_DIST); 
  min_z = MID (1, CAVEGEN_MAX/2-(dz-1)/2, CAVEGEN_MAX-2);
  max_z = MID (1,  CAVEGEN_MAX/2+dz/2, CAVEGEN_MAX-2);
  
  /* Basically don't attempt to add rooms too much to really small maps
     since most of the maps will fail, so let them fail quickly. */
  
  max_room_add_tries = MIN (dx, dy)*3/2;
  

 
  /* Now clear the grid. and seed some rooms. */
  cavegen_clear_bit (~0);
  
  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {	
	      total_dist = 0;
	      val = ABS(x-(max_x+min_x)/2);
	      total_dist += val*val;
	      val = ABS(y-(max_y+min_y)/2);
	      total_dist += val * val; 
	      val = 2*ABS(z-(max_z+min_z)/2);
	      total_dist += val*val;
	      if (total_dist <= (dx*dx + dy*dy + dz*dz)/7)
		SBIT (cave_grid[x][y][z], CAVEGEN_ALLOWED);
	    }
	}
    }
  
  
  /* Now keep checking if the cave is connected or not and adding
     more rooms until it's done. */
  
  for (tries = 0; tries < 200; tries++)
    {
      /* Clear used flags and seed the rooms. The reason that I 
	 keep reseeding if the numbers get big is that I've noticed
	 that it either generates a cave really quickly, or it thrashes
	 for a long time, so if it gets a bad seed, I will just start over
	 with a new seed. */
      cavegen_clear_bit (CAVEGEN_USED | CAVEGEN_ADDED_ADJACENT);
      cavegen_seed_rooms ();      
      count = 0;
      while (!cavegen_is_connected () && ++count < max_room_add_tries)
	{
	  /* Do N add rooms before checking connected status. */
	  for (i = 0; i < CAVEGEN_ADD_BEFORE_CHECK_CONNECT; i++)
	    cavegen_add_rooms(count*CAVEGEN_FORCE_UD/CAVEGEN_ADD_TRIES);
	}
      if (cavegen_is_connected())
	break;
    }
  if (!cavegen_is_connected ())
    {
      stt ("\x1b[1;36mFailed to generate a cave!\x1b[0;37m\n\r", th);
      
      return FALSE;
    }
  stt ("Cave generated. Use cavegen generate <number> to make the map.\n\r", th);
  return TRUE;
}


/* This clears some bit from the entire cavegen grid -- not too efficient
   but who cares. */

void
cavegen_clear_bit (int bit)
{
  int x, y, z;
  
  
  /* When clearing cavegen allowed, clear EVERYTHING. */
  if (IS_SET (bit, CAVEGEN_ALLOWED))
    {
      for (x = 0; x < CAVEGEN_MAX*2+1; x++)
	{
	  for (y = 0; y < CAVEGEN_MAX*2+1; y++)
	    {
	      for (z = 0; z < CAVEGEN_MAX; z++)
		{
		  RBIT(cave_grid[x][y][z], bit);
		}
	    }
	}
    }
  else
    {
      for (x = min_x; x < max_x; x++)
	{
	  for (y = min_y; y <= max_y;  y++)
	    {
	      for (z = min_z; z <= max_z; z++)
		{
		  RBIT(cave_grid[x][y][z], bit);
		}
	    }
	}
    }
  return;
}

/* This seeds the map with some rooms on the edges of the map so that they
   have to grow toward the middle of the map. */
  
void
cavegen_seed_rooms (void)
{  
    int i;
    int z;
    int x, y;
  /* Now go to the edges and seed new rooms. */
  
  for (z = min_z; z <= max_z; z++)
    {
      int times = 2;
      if (z > min_z && z < max_z && nr (1,3) != 2)
	times++;
      for (i = 0; i < times; i++)
	cavegen_seed_room (0, 0, z);      
    }

  /* Now do interlevel connections. */

  for (z = min_z; z < max_z; z++)
    {
      x = nr (min_x, max_x);
      y = nr (min_y, max_y);
      cavegen_seed_room (x, y, z);
      cavegen_seed_room (x, y, z+1);
    }

  return;
}

/* This seeds specific cave rooms. It assumes that there are already
   certain rooms set as valid rooms and that we just need to 
   set them as used. */


void
cavegen_seed_room (int start_x, int start_y, int start_z)
{
  int x, y, z;
  int times, i;
  int tries;
  /* Get min/max x, y, z values. */
 

  times = 1;
  
  for (i = 0; i < times; i++)
    {  
      for (tries = 0; tries < 20; tries++)
	{
	  if (start_x == 0)
	    x = nr (min_x, max_x);
	  else 
	    x = MID (min_x, start_x, max_x);
	  if (start_y == 0)
	    y = nr (min_y, max_y);
	  else 
	    y = MID (min_y, start_y, max_y);
	  if (start_z == 0)
	    z = nr (min_z, max_z);
	  else 
	    z = MID (min_z, start_z, max_z);
	  
	  
	  if (IS_SET (cave_grid[x][y][z], CAVEGEN_ALLOWED))
	    {
	      cave_grid[x][y][z] |= CAVEGEN_USED;
	      break;
	    }
	}
    }
  return;
}


/* This adds new rooms to the cave by looking at rooms adjacent to
   1 or more current cave rooms (You should do this only after
   seeding some rooms.) and adding the room into the allowed set. */


void
cavegen_add_rooms (int force_ud)
{ 
  int x, y, z;
  /* Number of adjacent rooms. */
  int num_adj_rooms;   /* Number of rooms directly adjacent to this NEWS. */
  int num_above_below_rooms; /* How many rooms are above and below this. */
  int num_corner_rooms; /* How many rooms are diagonal from this in the 
			   NEWS plane. */
  int chance_to_add_room;

  /* Number of rooms on the same level within +/-2 in x and y dirs. */
  int num_near_rooms; 
  int cx, cy, cz;

  cavegen_clear_bit(CAVEGEN_ADDED_ADJACENT);
  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {
	      if (USED_CAVE (x,y,z) ||
		  !IS_SET (cave_grid[x][y][z], CAVEGEN_ALLOWED) ||
		  IS_SET (cave_grid[x][y][z], CAVEGEN_ADDED_ADJACENT))
		continue;

	      /* Don't usually do rooms on edges. */
	      
	      num_adj_rooms = 0;
	      num_above_below_rooms = 0;
	      num_corner_rooms = 0;
	      num_near_rooms = 0; 
	      cz = z;
	      for (cx = x-CAVEGEN_NEAR_DIST; cx <= x+CAVEGEN_NEAR_DIST; cx++)
		{
		  for (cy = y-CAVEGEN_NEAR_DIST; cy <= y+CAVEGEN_NEAR_DIST; cy++)
		    {
		      if (USED_CAVE(cx,cy,cz))
			num_near_rooms++;
		    }
		}
	      
	      /* Most of the time don't connect when we're near
		 other rooms. Be careful that as CAVEGEN_NEAR_DIST
		 increases, you may have to work out a new formula here
		 or else your caves won't be created correctly. */
	      
	      if (num_near_rooms > CAVEGEN_MAX_ROOMS_NEAR)
		{
		  num_near_rooms -= CAVEGEN_MAX_ROOMS_NEAR;
		  if (nr (1,num_near_rooms) > 2) 
		    continue;
		}

	      if (USED_CAVE (x+1,y,z))
		num_adj_rooms++;
	      if (USED_CAVE (x-1,y,z))
		num_adj_rooms++;
	      if (USED_CAVE (x,y+1,z))
		num_adj_rooms++;
	      if (USED_CAVE (x,y-1,z))
		num_adj_rooms++;
	      if (USED_CAVE (x+1, y+1, z))
		num_corner_rooms++;
	      if (USED_CAVE (x-1, y+1, z))
		num_corner_rooms++;
	      if (USED_CAVE (x-1, y-1, z))
		num_corner_rooms++;
	      if (USED_CAVE (x+1, y-1, z))
		num_corner_rooms++;
	      
	      if (USED_CAVE (x,y,z+1))
		num_above_below_rooms++;
	      if (USED_CAVE (x,y,z-1))
		num_above_below_rooms++;
	      
	      /* Don't connect if there's too many rooms on the
		 same level. */
	      if (num_adj_rooms < 1 || num_adj_rooms > 3)
		continue;

	      if (num_adj_rooms == 3 && nr (1,50) != 34)
		continue;
	      
	      /* Don't connect 3 levels at once. */
	      if (num_above_below_rooms >= 2 && nr (1,20) != 3)
		continue;
	      
	      /* Dont' connect if there are lots of adjacent rooms and
		 an above/below room. */
	      if (num_adj_rooms > 1 && num_above_below_rooms > 0 &&
		  (nr (1, CAVEGEN_FORCE_UD) > force_ud*2  || nr (1,15) == 2))
		continue;
	      
	      /* Don't allow too many corner rooms near this. */
	      if (num_adj_rooms > 1 && num_corner_rooms > 1 &&
		  nr (1,3) == 2)
		continue;
	      
	      /* Don't allow too many corner rooms near this. */
	      if (num_corner_rooms > 2 && nr (1,5) != 3)
		continue;
	      
	      if (num_corner_rooms > 1 && nr (1, 4) != 2)
		continue;
	      
	      /* Don't connect most of the time if there are adj and
		 above/below rooms. */
	      if (num_adj_rooms > 0 && num_above_below_rooms > 0 &&
		  nr (1,100) != 2)
		continue;
	      
	      /* The chance to add a room drops off like the fourth
		 power of the number of rooms already attached!
		 This should make the map have a lot of tunnels
		 but not many caverns. */
	      
	      if (num_adj_rooms == 1)
		chance_to_add_room = 1;
	      else 
		chance_to_add_room = num_adj_rooms*3;
	      
	      if (nr (0,chance_to_add_room) != chance_to_add_room/2)
		continue;
	      
	      cave_grid[x][y][z] |= CAVEGEN_USED;
	      
	      /* Now mark all nearby as added adjacent... */
	      cave_grid[x+1][y][z] |= CAVEGEN_ADDED_ADJACENT;
	      cave_grid[x-1][y][z] |= CAVEGEN_ADDED_ADJACENT;
	      cave_grid[x][y+1][z] |= CAVEGEN_ADDED_ADJACENT;
	      cave_grid[x][y-1][z] |= CAVEGEN_ADDED_ADJACENT;
	      
	    }
	}
    }
  return;
}

/* This checks if the cave is connected or not. */

bool
cavegen_is_connected (void)
{
  int x = 0, y = 0, z = 0;
  bool found = FALSE;
  /* Find a room in the cave. */
  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {
	      if (USED_CAVE(x,y,z))
		{
		  found = TRUE;
		  break;
		}
	    }
	  if (found)
	    break;
	}
      if (found)
	break;
    }
  
  /* Clear all connected bits. */
  cavegen_clear_bit (CAVEGEN_CONNECTED);
  
  /* Do a dfs to see what this room is connected to. */
  cavegen_check_connected (x, y, z);
  
  /* Loop through all rooms and if any are used and not connected, then
     the cave is not connected. */

  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {
	      if (USED_CAVE(x,y,z))
		{ 
		  if (!IS_SET (cave_grid[x][y][z], CAVEGEN_CONNECTED))
		    {
		      return FALSE;
		    }
		}
	    }
	}
    }
  return TRUE;
}


/* This marks a certain room as connected and also as checked so we
   don't go back to it again. */

void
cavegen_check_connected (int x, int y, int z)
{
  if (x < min_x || x > max_x ||
      y < min_y || y > max_y ||
      z < min_z || z > max_z)
    return;
  
  if (!USED_CAVE (x, y, z))
    return;
  
  if (IS_SET (cave_grid[x][y][z], CAVEGEN_CONNECTED))
    return;
  SBIT (cave_grid[x][y][z], CAVEGEN_CONNECTED);
  
  cavegen_check_connected (x+1, y, z);
  cavegen_check_connected (x-1, y, z);
  cavegen_check_connected (x, y+1, z);
  cavegen_check_connected (x, y-1, z);
  cavegen_check_connected (x, y, z+1);
  cavegen_check_connected (x, y, z-1);
  return;
}

/* Now I want to actually generate the rooms to go somewhere. */

bool
cavegen_generate (THING *th, int start_vnum)
{
  int x, y, z;
  int end_vnum, num_rooms = 0, vnum;
  THING *area, *area2, *room;
  /* These next 6 variables are used for setting the dir_edge names
     on rooms for use with the worldgen linking code. */
  int min_z_room = CAVEGEN_MAX, max_z_room = 0;
  int min_x_room = CAVEGEN_MAX*2, max_x_room = 0;
  int min_y_room = CAVEGEN_MAX*2, max_y_room = 0;
  char buf[STD_LEN];
 
  /* Used to start doing the DFS to mark rooms so we don't have
     many U/D connections in our map. */
  
  if ((area = find_area_in (start_vnum)) == NULL)
    {
      stt ("This start vnum isn't in an area!\n\r", th);
      return FALSE;
    }
  
  
  
  for (x = 0; x < CAVEGEN_MAX*2; x++)
    {
      for (y = 0; y < CAVEGEN_MAX*2; y++)
	{
	  for (z = 0; z < CAVEGEN_MAX-1; z++)
	    {
	      if (USED_CAVE(x,y,z))
		{		  
		  num_rooms++;
		  if (z > max_z_room)
		    max_z_room = z;
		  if (z < min_z_room)
		    min_z_room = z;
		  if (x > max_x_room)
		    max_x_room = x;
		  if (x < min_x_room)
		    min_x_room = x; 
		  if (y > max_y_room)
		    max_y_room = y;
		  if (y < min_y_room)
		    min_y_room = y;
		}
	      else
		cave_grid[x][y][z] = 0;
	    }
	}
    }
  
  if (num_rooms < 1)
    {
      stt ("You have to make a cave before you generate the rooms!\n\r", th);
      cavegen_clear_bit (~0);
      return FALSE;
    }

  if (!cavegen_is_connected ())
    {
      stt ("This cave isn't connected! Clearing map!\n\r", th);
      cavegen_clear_bit (~0);
      return FALSE;
    }
  
  /* Ok this is off by 1 but I don't want to stretch it. */
  end_vnum = start_vnum+num_rooms;
  area2 = find_area_in (start_vnum+num_rooms);
  
  if (area2 != area)
    {
      sprintf (buf, "Your cave has %d rooms and it's too big to fit in that area!\n\r", num_rooms);
      stt (buf, th);
      return FALSE;
    }
  
  if (end_vnum > area->vnum + area->mv)
    {
      sprintf (buf, "The cave is too big to fit in the room space you've allotted for your area. Increase the number of rooms to at least %d.\n\r", num_rooms+(start_vnum-area->vnum));
      stt (buf, th);
      return FALSE;
    }
  
  for (vnum = start_vnum; vnum <= start_vnum+num_rooms;vnum++)
    {
      if ((room = find_thing_num (vnum)) != NULL)
	{
	  sprintf (buf, "You need vnum range %d-%d to be open, and object %d exists.\n\r", start_vnum, end_vnum, vnum);
	  stt (buf, th);
	  return FALSE;
	}
    }
  
  /* At this point we know that the target area exists and that it has
     enough space for the rooms and nothing is in the target vnum range,
     so start to add the rooms in. The loop goes from minmax_xyz -1 to
     minmax_xyz + 1 because there are "adjacent" rooms marked at the 
     edges of the map that need to be unmarked. */
  
  vnum = start_vnum;
  for (x = min_x - 1; x <= max_x + 1; x++)
    {
      for (y = min_y - 1; y <= max_y + 1; y++)
	{
	  for (z = min_z - 1; z <= max_z + 1; z++)
	    {
	      if (USED_CAVE(x,y,z))
		cave_grid[x][y][z] = vnum++;
	      else /* Set unused rooms to 0. */
		cave_grid[x][y][z] = 0;
	    }
	}
    }
  
  /* Now all rooms have been marked with increasing vnums. So, 
     generate the actual rooms and link them up. */
  
  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {	      
	      if (cave_grid[x][y][z])
		{
		  if ((room = new_thing()) == NULL)
		    return FALSE;
		  
		  room->vnum = cave_grid[x][y][z];
		  room->thing_flags = ROOM_SETUP_FLAGS; 
		  if (z == max_z_room)
		    append_name (room, "up_edge");
		  if (z == min_z_room)
		    append_name (room, "down_edge");
		  if (x == max_x_room)
		    append_name (room, "east_edge");
		  if (x == min_x_room)
		    append_name (room, "west_edge");
		  if (y == max_y_room)
		    append_name (room, "north_edge");
		  if (y == min_y_room)
		    append_name (room, "south_edge");
		  
		  thing_to (room, area);
		  add_thing_to_list (room);
		}
	    }
	}
    }
  
		  
  /* Now add exits. Do this in 2 loops because adding exits doesn't
     work unless the target room exists. ONLY ADD NEWS EXITS NOW. 
     THERE SHOULD BE VERY FEW UD EXITS AND THOSE GET ADDED NEXT. */
  
  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {
	      if (cave_grid[x][y][z] &&
		  (room = find_thing_num (cave_grid[x][y][z])) != NULL)
		{
		  room_add_exit (room, DIR_EAST, cave_grid[x+1][y][z],0);
		  room_add_exit (room, DIR_WEST, cave_grid[x-1][y][z],0);
		  room_add_exit (room, DIR_NORTH, cave_grid[x][y+1][z],0);
		  room_add_exit (room, DIR_SOUTH, cave_grid[x][y-1][z],0);
		  room_add_exit (room, DIR_UP, cave_grid[x][y][z-1],0);
		  room_add_exit (room, DIR_DOWN, cave_grid[x][y][z+1],0);
		}
	    }
	}
    }
  
  undo_marked (area);
  
  
  /* Now add up/down connections only as needed! */

  


  /* Now mark the rooms on the area edges. */

  for (x = min_x; x <= max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {
	      if (USED_CAVE (x,y,z) &&
		  (room = find_thing_num (cave_grid[x][y][z])) != NULL &&
		  IS_ROOM (room))
		{
		  if (x == min_x)
		    append_name (room, "west_edge");
		  if (x == max_x)
		    append_name (room, "east_edge");
		  if (y == max_y)
		    append_name (room, "north_edge");
		  if (y == min_y) 
		    append_name (room, "south_edge");
		  if (y == min_z)
		    append_name (room, "down_edge");
		  if (y == max_z)
		    append_name (room, "up_edge");
		}
	    }
	}
    }
  SBIT (area->thing_flags, TH_CHANGED);
  stt ("Caves generated.\n\r", th);
  cavegen_clear_bit (~0);
  return TRUE;
}
  
/* This shows the cavegen map as layers. */

void
cavegen_show (THING *th)
{
  int x, y, z;
  char buf[STD_LEN];
  char *t;
  
  if (!th)
    return;
  
  
  /* Loop through the levels from highest down to lowest and show each
     level of the map. Loop through y dir first, then across
     show all levels of the cave in each row. */
  
  stt ("\n\r", th);
  for (y = max_y; y >= min_y; y--)
    {
      t = buf;
      for (z = max_z; z >= min_z; z--)
	{
	  for (x = min_x; x <= max_x; x++)
	    {
	      
	      if (USED_CAVE(x,y,z))
		{
		  if (USED_CAVE (x,y,z-1))
		    {
		      if (!USED_CAVE (x,y,z+1))
			{
			  sprintf (t, "\x1b[1;31m#\x1b[0;37m");
			  while (*t)
			    t++;
			  t--;
			}
		      else
			{
			  sprintf (t, "\x1b[1;36m#\x1b[0;37m");
			  while (*t)
			    t++;
			  t--;
			}
		    }		  
		  else if (USED_CAVE (x,y,z+1))
		    { 
		      sprintf (t, "\x1b[1;32m#\x1b[0;37m");
		      while (*t)
			t++;
		      t--;
		    }
		  else
		    *t = '#';
		}
	      else
		*t = ' ';
	      t++;
	    }
	  if (z > min_z)
	    {
	      sprintf (t, " | ");
	      t += 3;
	    }
	}
      *t++ = '\n';
      *t++ = '\r';
      *t = '\0';
      stt (buf, th);
    }
  
  sprintf (buf, "The total number of rooms is %d\n\r", find_num_cave_rooms());
  stt (buf, th);
  return;
}


/* This returns the number of cave rooms currently in use. */

int
find_num_cave_rooms (void)
{
  int x, y, z;
  int total = 0;
  for (x = min_x; x < max_x; x++)
    {
      for (y = min_y; y <= max_y; y++)
	{
	  for (z = min_z; z <= max_z; z++)
	    {
	      if (USED_CAVE(x,y,z))
		total++;
	    }
	}
    }
  return total;
}
