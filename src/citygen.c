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
#include "plasmgen.h"




/* The actual city rooms are stored in this grid. */

THING *city_grid[CITY_SIZE][CITY_SIZE][CITY_HEIGHT];
char city_name[STD_LEN];
char city_full_name[STD_LEN];


/* This is a specialized type of "areagen". */

void
do_citygen (THING *th, char *arg)
{
  if (!th || LEVEL(th) < MAX_LEVEL)
    return;
  citygen (th, arg);
  
  return;
}

void 
citygen (THING *th, char *arg)
{
  int start, size, level;
  char arg1[STD_LEN];
  char areafile[STD_LEN];
  THING *area, *thg;
  THING *start_proto = NULL;
  VALUE *start_dims; /* The starting dimensions of this 
			prototype. */
  RESET *rst;
  int x, y, dx, dy, dir;
  THING *room1, *room2;

  if (!arg || !*arg)
    return;

  if (!str_cmp (arg, "show"))
    {
      show_city_map (th);
      return;
    }

  arg = f_word (arg, arg1);
  if ((start = atoi (arg1)) < 1)
    {
      stt ("citygen <start> <size> [<level>] [<start proto>]\n\r", th);
      return;
    }
  
  arg = f_word (arg, arg1);
  if ((size = atoi (arg1)) < 100 || size > 2000)
    {
      stt ("The size of the city must be from 100-2000 things.\n\r", th);
      return;
    }
  
  if (!check_area_vnums (NULL, start, start+size))
    {
      stt ("This city site overlaps another area.\n\r", th);
      return;
    }

  

  arg = f_word (arg, arg1);  
  if ((level = atoi (arg1)) == 0)
    level = 100;
  
  arg = f_word (arg, arg1);
  
  if ((start_proto = find_thing_num (atoi (arg1))) == NULL)
    start_proto = find_thing_num (CITYGEN_AREA_VNUM + 1);
  
  if (!start_proto)
    {
      stt ("Couldn't find the starting object needed for the city.\n\r", th);
      return;
    }
  
  clear_city_grid (FALSE);


  /* Create the area and set it up. */
  
  if ((area = find_thing_num (start)) == NULL)
    {
      if ((area = new_thing ()) == NULL)
	return;
      
      area->vnum = start;
      top_vnum = MAX (top_vnum, start);
      area->level = level;
      add_thing_to_list (area);
      set_up_new_area(area, size);
      
      /* If we worldgen, then make this area name end in qz.are */
      if (IS_SET (server_flags, SERVER_WORLDGEN))
	{
	  if (area->name)
	    free_str (area->name);
	  sprintf (areafile, "area%dqz.are", area->vnum);
	  area->name = new_str (areafile);
	}
      /* Now make an area name like "The city of Foo" or something. */
      
      strcpy (city_name, generate_area_name (NULL, ROOM_EASYMOVE));
      
      if (nr (1,3) == 1)
	strcpy (city_full_name, city_name);
      else if (nr (1,2) == 2)
	sprintf (city_full_name, "The City of %s", city_name);
      else 
	sprintf (city_full_name, "%s City", city_name);
      
      free_str (area->short_desc);
      area->short_desc = new_str (city_full_name);
    }

  
  area->mv = size*2/3;
  area->thing_flags |= TH_CHANGED;
  /* This generates a base city grid that uses up about 3/4 of the
     available rooms. It makes a grid of partially overlapping 
     squares. */
  
  generate_base_city_grid (start_proto, area->vnum+1);
  

  /* Now add details based on all of the resets that this object
     has. */
  
  /* If this thing is a detail itself, (in that it has dimensions)
     we reset it, otherwise we do the resets of it. */

  if ((start_dims = FNV (start_proto, VAL_DIMENSION)) != NULL)
    {
      citygen_add_detail (NULL, area, start_proto, NULL, 2);
    }
  else
    {
      for (rst = start_proto->resets; rst; rst = rst->next)
	{
	  citygen_add_detail (rst, area, NULL, NULL, 2);
	}
    }
  
  /* Now connect all rooms at depth 1 on the road level. */

  for (x = 0; x < CITY_SIZE; x++)
    {
      for (y = 0; y < CITY_SIZE; y++)
	{
	  if (city_coord_is_ok (x, y, CITYGEN_STREET_LEVEL) &&
	      (room1 = city_grid[x][y][CITYGEN_STREET_LEVEL]) != NULL &&
	      room1->mv == 0)
	    {
	      for (dir = 0; dir < FLATDIR_MAX; dir++)
		{
		  dx = 0;
		  dy = 0;
		  if (dir == DIR_NORTH)
		    dy++;
		  else if (dir == DIR_SOUTH)
		    dy--;
		  else if (dir == DIR_EAST)
		    dx++;
		  else if (dir == DIR_WEST)
		    dx--;
		  
		  if (city_coord_is_ok (x+dx, y+dy, CITYGEN_STREET_LEVEL) &&
		      (room2 = city_grid[x+dx][y+dy][CITYGEN_STREET_LEVEL]) != NULL &&
		      (room2->mv == 0 ||
		       is_named (room2, "road")))
		    {		      
		      room_add_exit (room1, dir, room2->vnum, 0);
		      room_add_exit (room2, RDIR (dir), room1->vnum, 0);
		    }
		}
	    }
	}
    }

  
  citygen_link_base_grid ();
  
  /* This connects disconnected regions in the city. */

  citygen_connect_regions (area, 1);
  
  if (!IS_SET (server_flags, SERVER_WORLDGEN))
    plasmgen_export (area);
  /* Now set all rooms/mobs in the area to mv 1 and all objects to
     mv 0. So we get 1 of each room and unlimited objects of
     each other type. */

  for (thg = area->cont; thg; thg = thg->next_cont)
    {
      /* Rooms/mobs */
      if (IS_ROOM (thg) || CAN_MOVE(thg) || CAN_FIGHT (thg))
	thg->max_mv = 1;
      else  /* Objects. */
	thg->max_mv = 0;
      thg->mv = 0;
    }
  

  /* This adds guards and stock eq and gatehouses to the city.
     THIS MUST COME AFTER THE PREVIOUS CODE WHERE YOU SET ALL OF THE
     MAX_MV'S TO LOW VALUES OR ELSE THE GUARDS WON'T WORK RIGHT! */

  /* Only do this for full cities...not partial cities! */
  
  if (!start_dims)
    citygen_add_guards (area);
  
  /* This adds fields ringing the city that will attach to gatehouses. */
 
  citygen_add_fields (area);


  
  clear_city_grid(FALSE);

  reset_thing (area, 0);
  return;
}


  
/* This clears the city grid of all objects...and may delete some of
   them if you specify that it should do so. */
  
void
clear_city_grid (bool nuke_things)
{
  int x, y, z;
  
  for (x = 0; x < CITY_SIZE; x++)
    {
      for (y = 0; y < CITY_SIZE; y++)
	{
	  for (z = 0; z < CITY_SIZE; z++)
	    {
	      if (nuke_things && city_grid[x][y][z])
		{
		  /* Get rid of room thing_flag so it will nuke the thing. */
		  city_grid[x][y][z]->thing_flags = 0;
		  free_thing (city_grid[x][y][z]);
		}
	      city_grid[x][y][z] = NULL;
	    }
	}
    }
  city_name[0] = '\0';
  city_full_name[0] = '\0';
  return;
}

/* This tells if a certain x, y, z triple is within the city grid
   or not. */

bool
city_coord_is_ok (int x, int y, int z)
{
  if (x < 0 || x >= CITY_SIZE ||
      y < 0 || y >= CITY_SIZE ||
      z < 0 || z >= CITY_HEIGHT)
    return FALSE;
  return TRUE;
}


/* This generates the base city shape as a grid of partially overlapping
   squares. */

void
generate_base_city_grid (THING *obj, int start_vnum)
{
  /* These are the curr_vnum we're on and the max vnum available for
     generating this grid of rooms. */
  int curr_vnum, max_vnum = 0;
  int dim_x = 0, dim_y = 0, dim_z;
  int start_x, start_y, start_z;
  int x, y, z;
  int num_rooms, max_num_rooms, max_num_rooms_sqrt;
  int num_blocks; /* Number of blocks of rooms we will use to make the 
		     city or building. */
  int block_times = 0; /* Number of times we've added blocks. */
  VALUE *dim;
  THING *area, *room;
  int vnum;
  bool rooms_overlap;
  
  if (!obj || !start_vnum)
    return;

  if (IS_AREA (obj))
    area = obj;
  else if ((area = find_area_in (start_vnum)) == NULL)
    return;
  
  max_vnum = area->vnum + area->mv-1;
  
  /* If this is a city, we make a bunch of blocks until we finally get
     to our target size. If it's a single building or room, just make a 
     single block based on the VAL_DIMENSION data given. */
  
  if ((dim = FNV (obj, VAL_DIMENSION)) != NULL)
    {
      num_blocks = 1;
      dim_x = nr (dim->val[0], dim->val[1]);
      dim_y = nr (dim->val[2], dim->val[3]);
      dim_z = nr (dim->val[4], dim->val[5]);
      if (dim_x < 1)
	dim_x = 1;
      if (dim_y < 1)
	dim_y = 1;
      if (dim_z < 1)
	dim_z = 1;
      num_rooms = dim_x*dim_y*dim_z;
      curr_vnum = start_vnum;
    }
  else /* For areas, add unlimited numbers of blocks. */
    {
      num_blocks = 0; /* Unlimited blocks to be added. */
      num_rooms = (area->vnum + area->mv-start_vnum)*2/3;
      dim_z = 1;
      dim_x = 0;
      dim_y = 0;
      curr_vnum = start_vnum;
    }
  
  if (num_rooms < 1 || num_rooms > 2000)
    return;

  /* Check to make sure that all of these rooms are open. */
  for (vnum = start_vnum; vnum <= start_vnum + num_rooms; vnum++)
    {
      if ((room = find_thing_num (vnum)) != NULL)
	return;
    }
  
  if (num_blocks == 1)
    {
      start_x = (CITY_SIZE-dim_x)/2;
      start_y = (CITY_SIZE-dim_y)/2;
      start_z = CITYGEN_STREET_LEVEL;
      
      for (z = start_z; z < start_z + dim_z; z++)
	{
	  for (x = start_x; x < start_x + dim_x; x++)
	    {
	      for (y = start_y; y < start_y + dim_y; y++)
		{
		  if (!city_coord_is_ok (x, y, z))
		    continue;
		  if (curr_vnum >= start_vnum && curr_vnum <= max_vnum &&
		      !city_grid[x][y][z])
		    {
		      city_grid[x][y][z] = new_room(curr_vnum);
		      free_str (city_grid[x][y][z]->short_desc);
		      if (obj)
			append_name (city_grid[x][y][z], obj->name);
		      city_grid[x][y][z]->short_desc = new_str (city_full_name);
		      city_grid[x][y][z]->mv = 1;
		      curr_vnum++;
		      num_rooms++;
		    }
		}
	    }
	}
      return;
    }
  
     
  /* Add the blocks of rooms to the area. */
  
  num_rooms = 0;
  max_num_rooms = (max_vnum-start_vnum)/2;

  max_num_rooms_sqrt = 0;
  while (max_num_rooms_sqrt*max_num_rooms_sqrt < max_num_rooms)
    max_num_rooms_sqrt++;
  if (max_num_rooms_sqrt < 9)
    max_num_rooms_sqrt = 9;
  /* Do this while we haven't used at least 2/3 of the alotted rooms. */
  while (num_rooms < max_num_rooms && ++block_times < 100)
    {
      dim_x = nr (max_num_rooms_sqrt/3,max_num_rooms_sqrt*2/3);
      dim_y = nr (max_num_rooms_sqrt/3,max_num_rooms_sqrt*2/3);
      dim_z = 1;

      
      if (num_rooms == 0)
	{
	  start_x = CITY_SIZE/2 - dim_x/2;
	  start_y = CITY_SIZE/2 - dim_y/2;
	  start_z = CITYGEN_STREET_LEVEL;
	}
      else
	{
	  start_x = CITY_SIZE/2 + nr (0, max_num_rooms_sqrt/2) - nr (0, max_num_rooms_sqrt/2);
	  start_y = CITY_SIZE/2 + nr (0, max_num_rooms_sqrt/2) - nr (0, max_num_rooms_sqrt/2);
	  start_z = CITYGEN_STREET_LEVEL;
	  
	  /* Now check if the rooms overlap. */
	  rooms_overlap = FALSE;
	  
	  for (x = start_x; x < start_x + dim_x && !rooms_overlap; x++)
	    {
	      for (y = start_y; y < start_y + dim_y && !rooms_overlap; y++)
		{
		  for (z = start_z; z < start_z + dim_z; z++)
		    {
		      if (!city_coord_is_ok (x, y, z))
			continue;
		      
		      if (city_grid[x][y][z])
			{
			  rooms_overlap = TRUE;
			  break;
			}
		    }
		}
	    }
	  if (!rooms_overlap)
	    continue;
	}
      
      
      for (x = start_x; x < start_x + dim_x; x++)
	{
	  for (y = start_y; y < start_y + dim_y; y++)
	    {
	      for (z = start_z; z < start_z + dim_z; z++)
		{
		  if (!city_coord_is_ok (x, y, z))
		    continue;
		  
		  if (curr_vnum >= max_vnum)
		    break;
		  
		  if (!city_grid[x][y][z])
		    {
		      city_grid[x][y][z] = new_room(curr_vnum);
		      add_flagval (city_grid[x][y][z], FLAG_ROOM1, ROOM_EASYMOVE);
		      city_grid[x][y][z]->mv = 1;
		      append_name (city_grid[x][y][z], obj->name);
		      city_grid[x][y][z]->short_desc = new_str (city_full_name);
		      curr_vnum++;		      
		      num_rooms++;
		    }
		}
	    }
	}
    }  
  
  
  
  return;
}
      
      
      
void
show_city_map (THING *th)
{
  int min_x = CITY_SIZE, max_x = 0;
  int min_y = CITY_SIZE, max_y = 0;
  int x, y;
  
  char buf[STD_LEN], *t;
  
  if (!th)
    return;
  
  for (x = 0; x < CITY_SIZE; x++)
    {
      for (y = 0; y < CITY_SIZE; y++)
	{
	  if (city_grid[x][y][CITYGEN_STREET_LEVEL])
	    {
	      if (x > max_x)
		max_x = x;
	      if (x < min_x)
		min_x = x;
	      if (y > max_y)
		max_y = y;
	      if (y < min_y)
		min_y = y;
	    }
	}
    }
  for (y = max_y; y >= min_y; y--)
    {
      buf[0] = '\0';
      t = buf;
      for (x = min_x; x <= max_x; x++)
	{
	  if (city_grid[x][y][CITYGEN_STREET_LEVEL])
	    *t = '*';
	  else
	    *t = ' ';
	  t++;
	}
      *t++ = '\n';
      *t++ = '\r';
      *t = '\0';
      stt (buf, th);
    }
  return;
}



/* This adds a detail to an existing city. The obj argument is the
   thing describing the detail, the area is where the obj goes, 
  the dims are the xyz coords where this goes (NULL = randomly
  select from all rooms that exist) and the depth is how deeply nested
  this room is atm. */

void
citygen_add_detail (RESET *start_reset, THING *area, THING *to, VALUE *start_dims, int depth)
{
  
  /* Actual xyz sizes for the object we will place here. */
  
  int o_dim_x, o_dim_y, o_dim_z;
  
  /* The starting dims where we're allowed to search. Gotten from start
     dims. If start_dims is null, search the whole city. (at ground level). */
  
  int s_min_x, s_min_y, s_min_z, s_max_x, s_max_y, s_max_z;
  

  /* Starting x, y, z when we actually find a start location. */

  int sx, sy, sz;

  int num_choices = 0, num_chose = 0, num_perfect_choices = 0, count;
  
  
  int x = 0, y = 0, z = 0;  /* Outer loop to find starting area. */
  int x1, y1, z1;
  char word[STD_LEN];
  VALUE this_obj_dims;  /* Allowed dimensions of this object. */
  bool bad_room = FALSE; 
  bool location_is_bad = FALSE;
  bool use_perfect_choices = FALSE; /* Go with perfect choices for room
				       placement if any are
				       available. */
  
  int vnum;
  VALUE *obj_dims;
  THING *obj = NULL, *start_obj = NULL;
  /* Number of good rooms in this particular region. */
  int good_rooms, good_rooms_base_level;
  int rooms_needed, base_rooms_needed;
  bool found_start_location = FALSE;
  
  bool need_edge_room = FALSE;
  bool need_center_room = FALSE;
  
  bool make_full_size = FALSE;
  bool room_is_on_edge = FALSE;

  bool room_is_below_room = FALSE; /* Is this a room below the other rooms...
			      like a basement. */
  
  bool room_is_above_room = FALSE; /* Steeple/tower. */
  /* Start and end vnum where we can put things. */
  int start_room_vnum = 0, end_room_vnum = 0;
  THING *room, *proto = NULL, *sroom = NULL;
  /* Used for linking up/down rooms. */
  int stair_x, stair_y;
  THING *below_room, *above_room;
  RESET *rst;
  
  /* This finds the last name of the parent object (to object) so that
     we only overwrite things that have this name in it...to avoid 
     stomping on different pieces of the city. */
  char last_parent_name[STD_LEN];
  char *wd;
  int reset_times = 1;
  int reset_percent = 100;
  VALUE *randpop;
  
  if((!start_reset && !to) || !area || depth < 1 || depth > 30)
    return;
  
  
  /* Argh, not having ctors sucks. Must init struct on the stack
     by hand. :P */

  this_obj_dims.word = nonstr;

  /* First order of business. If this obj is NOT within the
     GENERATOR_NOCREATE_VNUM_MIN/MAX range, then this is a REGULAR
     RESET PUT ONTO to!!!! if it's not a ROOM! 
     Also, we set the number of times and the percent here if this
     came from a reset up above. */
  
  if (start_reset)
    {
      start_obj = find_thing_num (start_reset->vnum);
      reset_times = MAX(1, start_reset->times);
      reset_percent = MAX(1, start_reset->pct);
    }
  else if (to)
    start_obj = to;
  else
    return;

  if (!start_obj)
    return;
  
  last_parent_name[0] = '\0';
  if (to)
    {
      wd = to->name;
      while (wd && *wd)
	wd = f_word (wd, last_parent_name);
    }


  /* Now if the obj is outside the range, we just add it as a 
     reset to to. */

  if (to && start_obj != to && start_reset &&
      (start_reset->vnum < GENERATOR_NOCREATE_VNUM_MIN ||
       start_reset->vnum > GENERATOR_NOCREATE_VNUM_MAX))
    {
      add_reset (to, start_reset->vnum, start_reset->pct, start_reset->times, start_reset->nest);
      return;
    }
  
  /* Now loop through the number of times the reset says to, and
     each time you add some other detail. */
  
  randpop = FNV (start_obj, VAL_RANDPOP);
  
  while (--reset_times >= 0)
    {/*
      {
	char buf[STD_LEN];
      sprintf (buf, "Reset obj %-6d  %-20s  (x%d)\n\r",
	       start_obj->vnum, start_obj->name, reset_times);
      echo (buf);
      }*/

      if (nr (1,100) > reset_percent)
	continue;
      
      bad_room = FALSE; 
      location_is_bad = FALSE;
      use_perfect_choices = FALSE;       
      found_start_location = FALSE;      
      need_edge_room = FALSE;
      need_center_room = FALSE;      
      room_is_on_edge = FALSE;
      make_full_size = FALSE;
      proto = NULL;
      num_choices = 0;
      num_chose = 0;
      num_perfect_choices = 0;
      room_is_below_room = FALSE;
      room_is_above_room = FALSE;
      /* Second, see if we change to another object due to a randpop
	 appearing. */
      
      if (randpop)
	{
	  obj = find_thing_num (calculate_randpop_vnum 
				(randpop, (to ? LEVEL (to) : LEVEL(area))));
	  if (!obj)
	    obj = start_obj;
	}
      else
	obj = start_obj;
      
      
      /* Get the vnum(s) where these objects will be stored in the area. */
      start_room_vnum = 0;
      end_room_vnum = 0;
      if (IS_ROOM (obj))
	{
	  int max_room_vnum = area->vnum+1;
	  for (room = area->cont; room; room = room->next_cont)
	    {
	      if (IS_ROOM (room) &&
		  room->vnum > max_room_vnum)
		max_room_vnum = room->vnum;
	    }
	  start_room_vnum = max_room_vnum+1;
	  end_room_vnum = area->vnum+area->mv;
	  if (start_room_vnum > end_room_vnum)
	    {
	      start_room_vnum = end_room_vnum = 0;
	    }
	}
      else
	{
	  start_room_vnum = find_free_mobject_vnum (area);
	  end_room_vnum = start_room_vnum;
	}
      
      
      if (start_room_vnum == 0)
	continue;      
      /* Get the size of the new object. */
      
      
      
      /* Get the starting and ending places to look. */
      
      if (start_dims)
	{
	  s_min_x = start_dims->val[0];
	  s_max_x = start_dims->val[1];
	  s_min_y = start_dims->val[2];
	  s_max_y = start_dims->val[3];
	  s_min_z = start_dims->val[4];
	  s_max_z = start_dims->val[5];
	}
      else
	{
	  s_min_x = s_min_y = 0;
	  s_max_x = s_max_y = CITY_SIZE-1;
	  s_min_z = s_max_z = CITYGEN_STREET_LEVEL;
	}
      
      /* If this has no dimensions set, then make it 1x1x1. Same for
	 nonrooms. */
      if ((obj_dims = FNV (obj, VAL_DIMENSION)) == NULL || !IS_ROOM (obj))
	{
	  o_dim_x = o_dim_y = o_dim_z = 1;
	}
      /* If this is the start of a single building, then we make
	 sure that the start dims are as large as possible to fill
	 in the entire grid. */
      else if (to == obj)
	{
	  o_dim_x = MAX(1, obj_dims->val[1]);
	  o_dim_y = MAX(1, obj_dims->val[3]);
	  o_dim_z = MAX(1, obj_dims->val[5]);
	}
      else
	{
	  /* Otherwise the size is a random, but capped by the
	     size of the x y z dims we have to work with. */
	  o_dim_x = MID (1, nr (obj_dims->val[0],obj_dims->val[1]),
			 s_max_x-s_min_x+1);
	  o_dim_y = MID (1, nr (obj_dims->val[2],obj_dims->val[3]),
			 s_max_y-s_min_y+1);
	  o_dim_z = MID (1, nr (obj_dims->val[4],obj_dims->val[5]),
			 s_max_z-s_min_z+1);
	  
	  /* Except...the z dim can eb as big as needed if the
	     initial object is on street level. */
	  
	  if (s_min_z == CITYGEN_STREET_LEVEL)
	    {
	      o_dim_z = MAX (1, nr (obj_dims->val[4],obj_dims->val[5]));
	    }

	  /* This is for making "stringy" objects like roads. */
	  
	  if (obj_dims->word && *obj_dims->word)
	    {
	      
	      if (named_in (obj_dims->word, "stringy"))
		{ 
		  citygen_add_stringy_detail (start_reset, area, to, start_dims, depth);
		  continue;
		}      
	      else if (named_in (obj_dims->word, "edge"))
		need_edge_room = TRUE;
	      else if (named_in (obj_dims->word, "center"))
		need_center_room = TRUE;
	      

	      /* This makes the inner object full size (i.e. taking
		 up the full area alotted. This might sound stupid, but
		 if you combine this with some of the other things
		 below like north or top or east or whatever, you
		 can do things like make entire floors of locations
		 the same name. */
	      if (named_in (obj_dims->word, "full"))
		make_full_size = TRUE;
	      
	      /* These are for making the object sit in different
		 parts of the larger object. The idea is that you can
		 specify NEWSUD or top/bottom (sets to edge) or 
		 left/right directions so that the objects appear in
		 certain places inside the larger set. 

		 The reason for all of the + 1's is that for example
		 if you have a range of 2-3 in some direction and you want
		 the object to appear in the 3 place, so if you did
		 (2+3)/2 as the new lower endpoint, you haven't gotten 
		 anywhere. The reason for not doing it in the opp
		 direction is that again if we have a range from 2-3 and
		 we only want to look in the 2 coord, (2+3)/2 = 2, but
		 (2+3+1)/2 = 3 so we don't need it there.
		 Also, for ranges like 2-4, we get (2+4+1)/2 = 3 for upper,
		 (2+4)/2 = 3 for lower, so there's an overlap.  */
	      if (named_in (obj_dims->word, "east"))
		s_min_x = (s_min_x + s_max_x+ 1)/2;
	      if (named_in (obj_dims->word, "west"))
		s_max_x = (s_min_x + s_max_x )/2;
	      if (named_in (obj_dims->word, "north"))
		s_min_y = (s_min_y + s_max_y+ 1)/2;
	      if (named_in (obj_dims->word, "south"))
		s_min_y = (s_min_y + s_max_y  )/2;
	      if (named_in (obj_dims->word, "upper"))
		s_min_z = (s_min_z + s_max_z + 1)/2;
	      if (named_in (obj_dims->word, "lower"))
		s_max_z = (s_min_z + s_max_z )/2;

	      /* These last two set the max/min z's directly to the ends. */
	      if (named_in (obj_dims->word, "top"))
		s_min_z = s_max_z;
	      if (named_in (obj_dims->word, "bottom"))
		s_max_z = s_min_z;
	      
	      if (named_in (obj_dims->word, "south"))
		s_min_y = (s_min_y + s_max_y + 1)/2;
	      



	      /* These make things like basements and such. */
	      if (named_in (obj_dims->word, "underneath") ||
		  named_in (obj_dims->word, "beneath") ||
		  named_in (obj_dims->word, "below"))
		{
		  s_min_z--;
		  s_max_z = s_min_z;
		  room_is_below_room = TRUE;
		  
		}

	      /* This makes things like towers and steeples and such. */

	      if (named_in (obj_dims->word, "above") ||
		  named_in (obj_dims->word, "atop") ||
		  named_in (obj_dims->word, "over"))
		{
		  s_max_z++;
		  s_min_z = s_max_z;
		  room_is_above_room = TRUE;
		}

	    }
	}
      
      
      /* Now search for the proper place to go. Search between search_min
	 and search_max in the x, y, z coords..with the rule that the
	 "base" level of the search area must contain all rooms, and you
	 can build up into blank rooms, but you can't add the rooms to the 
	 base level of rooms. */
      

      /* If this is full size, just set it to the s_min_xyz locations
	 and be done with it. */
      
      if (make_full_size)
	{
	  x = s_min_x;
	  y = s_min_y;
	  z = s_min_z;

	  o_dim_x = s_max_x - s_min_x + 1;
	  o_dim_y = s_max_y - s_min_y + 1;
	  
	  if (o_dim_z < s_max_z - s_min_z + 1)
	    o_dim_z = s_max_z - s_min_z + 1;
	  found_start_location = TRUE;
	}
      
      /* If there's no starting reset, but there is a to, then
	 we have a single building, and not a city. */
      
      
      
      else if (!start_reset && to)
	{
	  z = CITYGEN_STREET_LEVEL;
	  for (x = 0; x < CITY_SIZE; x++)
	    {
	      for (y = 0; y < CITY_SIZE; y++)
		{
		  if (city_grid[x][y][z])
		    {
		      found_start_location = TRUE;
		      break;
		    }
		}
	      if (found_start_location)
		break;
	    }
	}
      else
	{

	  for (count = 0; count < 2; count++)
	    {
	      for (x = s_min_x; x <= s_max_x-o_dim_x + 1; x++)
		{
		  for (y = s_min_y; y <= s_max_y-o_dim_y +1; y++)
		    {
		      for (z = s_min_z; z < s_min_z + o_dim_z; z++)
			{
			  if (IS_ROOM (obj) &&
			      (sroom = city_grid[x][y][z]) != NULL &&
			      (is_named (sroom, "road") ||
			       (*last_parent_name && sroom->name &&
				*sroom->name &&
				!named_in (sroom->name, last_parent_name))))
			    continue;
			  
			  if (x <= s_min_x || x + o_dim_x - 1 >= s_max_x ||
			      y <= s_min_y || y + o_dim_y - 1 >= s_max_y ||
			      z <= s_min_z || z + o_dim_z - 1 >= s_max_z)
			    room_is_on_edge = TRUE;
			  else
			    room_is_on_edge = FALSE;
			  /* At each possible starting point see if the rooms are
			     available for something of the size we need. */
			  
			  good_rooms = 0;
			  good_rooms_base_level = 0;
			  for (z1 = z; z1 < z + o_dim_z; z1++)
			    {
			      for (x1 = x; x1 < x + o_dim_x; x1++)
				{
				  for (y1 = y; y1 < y + o_dim_y; y1++)
				    { 
				      if (!city_coord_is_ok (x1, y1, z1))
					continue;
				      /* Set it so we haven't found a bad room. 
					 A bad room is not a killer. It just means
					 that we have to find enough good
					 rooms to justify putting this here. */
				      bad_room = FALSE;
				      /* The bottom level NEEDS all of the
					 rooms to be there. */
				      if (!city_grid[x1][y1][z1])
					{
					  if ((z1 == z &&
					       z1 >= CITYGEN_STREET_LEVEL) 
					      || !IS_ROOM(obj))
					    bad_room = TRUE;
					}
				      
				      
				      /* Now the city grid exists. If it's depth
					 is too high, ignore it. It's already
					 been detailed. */
				      
				      else if (city_grid[x1][y1][z1]->mv >= depth)
					bad_room = TRUE;
				      
				      if (!bad_room)
					{
					  good_rooms++;
					  if (z == z1)
					    good_rooms_base_level++;
					}
				    }
				}
			    }
			  /* Should be >= 1. */		  
			  rooms_needed = (o_dim_x*o_dim_y*o_dim_z);
			  base_rooms_needed = (o_dim_x*o_dim_y);
			  
			  /* Now allow some small errors to appear where
			     we don't require ALL rooms to be good. Just most
			     of them. */
			  location_is_bad = FALSE;
			  
			  /* If all rooms are available, we can go with a perfect
			     fit area.*/
			  if (good_rooms >= rooms_needed)
			    {
			      /* Only use the rooms if we don't care about
				 center/edge stuff, or if we do care and 
				 this is the right kind of room. */
			      if ((!need_edge_room && !need_center_room) ||
				  (need_edge_room && room_is_on_edge) ||
				  (need_center_room && !room_is_on_edge))
				num_perfect_choices++;
			    }
			  else 
			    {
			      /* Small areas require all rooms. */
			      if (rooms_needed < 2 &&
				  good_rooms < rooms_needed)
				location_is_bad = TRUE;
			      
			      if (good_rooms < rooms_needed/2)
				location_is_bad = TRUE;
			      
			      /* You need the base level rooms to be there. */
			      if (good_rooms_base_level < base_rooms_needed/2)
				location_is_bad = TRUE;
			      
			    }
			  
			  if (!location_is_bad)
			    {
			      if (count == 0)		
				num_choices++;
			      
			      /* When we're choosing places to put the rooms,
				 if you have perfect choices to pick from, then
				 use them. Otherwise do this if it's any old ok
				 location. We can also specify if we need to
				 use center rooms or edge rooms. */
			      
			      else if ((!use_perfect_choices ||
					(good_rooms >= rooms_needed &&
					 ((!need_edge_room && !need_center_room) ||
					  (need_edge_room && room_is_on_edge) ||
				      (need_center_room && !room_is_on_edge)))) &&
				       --num_chose < 1)
				{
				  found_start_location = TRUE;
				  break;
				}
			    }
			  if (found_start_location)
			    break;
			}
		      if (found_start_location)
			break;
		    }
		  if (found_start_location)
		    break;
		}
	      if (count == 0)
		{
		  use_perfect_choices = FALSE;
		  if (num_perfect_choices > 0)
		    {
		      num_chose = nr (1, num_perfect_choices);
		      use_perfect_choices = TRUE;
		    }
		  else if (num_choices > 0)
		    num_chose = nr (1, num_choices);
		  else
		    {
		      found_start_location = FALSE;
		      break;	      
		    }
		}
	      if (found_start_location)
		break;
	    }
	}
      
    
      if (!found_start_location)
	continue;
      
      /* Get the starting coords  */
      sx = x;
      sy = y;
      sz = z;
      /* Now fill in the rooms as needed. */
      
      if (IS_ROOM (obj))
	{
	  THING *copying_start_room = NULL;
	  vnum = start_room_vnum;
	  for (x = sx; x < sx + o_dim_x; x++)
	    {
	      for (y = sy; y < sy + o_dim_y; y++)
		{
		  for (z = sz; z < sz + o_dim_z; z++)
		    {
		      if (!city_coord_is_ok (x, y, z))
			continue;
		      
		      if (!city_grid[x][y][z])
			{
			  city_grid[x][y][z] = new_room(vnum);
			  
			  vnum++;
			}  
		      else if ((room = city_grid[x][y][z]) != NULL &&
			       room->mv != depth - 1 &&
			       (is_named (room, "road") ||
				(*last_parent_name && room->name &&
				 *room->name &&
				 !named_in (room->name, last_parent_name))))
			continue;
		      
		      if ((room = city_grid[x][y][z]) == NULL)
			continue;
		      
		      if (!copying_start_room)
			{
			  copying_start_room = city_grid[x][y][z];
			  free_str (copying_start_room->short_desc);
			  copying_start_room->short_desc = nonstr;
			  generate_detail_name (obj, copying_start_room);
			  capitalize_all_words (copying_start_room->short_desc);
			  strcpy (word, copying_start_room->short_desc);
			}
		      else
			{
			  free_str (room->short_desc);
			  room->short_desc = new_str (word);
			}
		      append_name (room, obj->name);
		      remove_flagval (room, FLAG_ROOM1, ~0);
		      add_flagval (room, FLAG_ROOM1, flagbits (obj->flags, FLAG_ROOM1));
		      room->mv = depth;
		      
		      
		      if (vnum >= end_room_vnum)
			break;
		    }
		  if (vnum >= end_room_vnum)
		    break;
		}
	      if (vnum >= end_room_vnum)
		break;
	    }
	  
	  
	  
	  /* ADD U/D links between rooms. */
	  
	  for (z = sz; z < sz + o_dim_z + 
		 (room_is_above_room ? 1 : 0) +
		 (room_is_below_room ? 0 : -1 ); z++)
	    {
	      bool added_stair = FALSE;
	      int count = 0;
	      
	      while (!added_stair && ++count < 100)
		{
		  stair_x = nr (sx, sx+o_dim_x-1);
		  stair_y = nr (sy, sy+o_dim_y-1);
		  
		  if (city_coord_is_ok(stair_x, stair_y, sz) &&
		      city_coord_is_ok(stair_x, stair_y, sz+1) &&
		      city_grid[stair_x][stair_y][sz] &&
		      (below_room = city_grid[stair_x][stair_y][sz]) != NULL &&
		      city_grid[stair_x][stair_y][sz+1] &&
		      (above_room = city_grid[stair_x][stair_y][sz+1]) != NULL &&
		      !is_named (above_room, "road"))
		    {
		      room_add_exit (below_room, DIR_UP, above_room->vnum, 0);
		      room_add_exit (above_room, DIR_DOWN, below_room->vnum, 0);
		      if (room_is_below_room && z < CITYGEN_STREET_LEVEL)
			place_secret_door (below_room, DIR_UP, generate_secret_door_name());
		      added_stair = TRUE;
		    }
		}
	    }
	  
	} 
      else if ((city_grid[sx][sy][sz] || to) &&
	       (vnum = find_free_mobject_vnum (area)) != 0)
	{
	  int reset_pct;
	  int obj_level;
	  proto = NULL;
	  if (to)
	    obj_level = LEVEL(to);
	  else
	    obj_level = LEVEL(area);
	  
	  if (obj->vnum == OBJECTGEN_AREA_VNUM)
	    proto = objectgen (area, ITEM_WEAR_NONE, obj_level, 0, NULL);
	  else if (obj->vnum == PERSONGEN_AREA_VNUM)
	    proto = areagen_generate_person (area, nr (0, LEVEL(area)/2));
	  
	  if (proto == NULL)
	    {
	      
	      proto = new_thing();
	      copy_thing (obj, proto);
	      proto->type = new_str (obj->type);
	      proto->thing_flags = obj->thing_flags;      
	      proto->vnum = vnum;
	      thing_to (proto, area);
	      add_thing_to_list (proto);
	      generate_detail_name (obj, proto);
	      
	      
	    }
	  
	  if (CAN_FIGHT (proto) || CAN_MOVE (proto))
	    reset_pct = 100;
	  else
	    reset_pct = objectgen_reset_percent (LEVEL(proto));
	  
	  if (!to && city_coord_is_ok (sx, sy, sz))
	    {
	      to = city_grid[sx][sy][sz];
	    }
	  
	  add_reset (to, proto->vnum, reset_pct, MAX(1, proto->max_mv), 1); 
	}
      
      
      
      /* At this point the rooms (or proto) are created, and the names
	 have been set. */
      
      /* Now set up the dims where the subobjects will be reset. 
       The reasons for the -1's are that for example if you have
       a room at 3,4,5 and you want to plop something into it, the
       thing being put into it has dims 1, 1, 1 but you wnat it to
       only work in 3, 4, 5 hence the -1's. */
      
      this_obj_dims.type = VAL_DIMENSION;
      
      this_obj_dims.val[0] = sx;
      this_obj_dims.val[1] = sx + o_dim_x -1;
      this_obj_dims.val[2] = sy;
      this_obj_dims.val[3] = sy + o_dim_y-1;
      this_obj_dims.val[4] = sz;
      this_obj_dims.val[5] = sz + o_dim_z-1;
      
      
      
      for (rst = obj->resets; rst; rst = rst->next)
	{
	  citygen_add_detail (rst, area, proto, &this_obj_dims, depth + 1);
	}
      
      
      
      
      
      
      if (IS_ROOM (obj))
	{
	  
	  /* ADD NEWS links within the rooms here. We do this after the other
	     resets since it will tend to make things */
	  
	  
	  
	  /* For each level....*/
      
	  
	  
	  citygen_connect_same_level (depth, &this_obj_dims);
	  

	  citygen_connect_next_level_up (depth, &this_obj_dims);
	}
    }
  return;
}

  
void
citygen_connect_same_level (int depth, VALUE *dims)
{
  int x, y, z;
  THING *room1 = NULL, *room2 = NULL;
  THING *start_room = NULL;
  THING *area = NULL;
  int count = 0, num_choices, num_chose;
  bool found_room = FALSE;
  bool all_attached = FALSE;
  int attach_tries = 0;
  int dir =  REALDIR_MAX;
  int nx, ny, nz;
  VALUE *exit;

  if (!dims)
    return;
  
  
  for (z = dims->val[4]; z <= dims->val[5]; z++)
    {
	  
      /* Add n/e exits to all approprate rooms. */
      for (x = dims->val[0]; x <= dims->val[1]; x++)
	{
	  for (y = dims->val[2]; y <= dims->val[3]; y++)
	    
	    {
	      if (city_coord_is_ok(x,y,z) && 
		  (room1 = city_grid[x][y][z]) != NULL &&
		  room1->mv == depth)
		{
		  if (!start_room)
		    {
		      start_room = room1;
		      if (room1->in && IS_AREA (room1->in))
			area = room1->in;
		    }
		  if (city_coord_is_ok (x+1, y, z) &&
		      (room2 = city_grid[x+1][y][z]) != NULL &&
		      room2->mv == depth &&
		      x < dims->val[1])
		    {
		      room_add_exit (room1, DIR_EAST, room2->vnum, 0);
		      room_add_exit (room2, DIR_WEST, room1->vnum, 0);
		    }
		  if (city_coord_is_ok (x, y+1, z) &&
		      (room2 = city_grid[x][y+1][z]) != NULL &&
		      room2->mv == depth &&
		      y < dims->val[3])
		    {
		      room_add_exit (room1, DIR_NORTH, room2->vnum, 0);
		      room_add_exit (room2, DIR_SOUTH, room1->vnum, 0);
		    }
		}
	    }
	}
    }

  /* The algorithm above isn't quite correct. If the details in a block
     split the rooms into 2 pieces, then you don't get a complete
     connection. For that reason, this code tries to do a DFS connected
     component check to try to see if all of the rooms are connected
     or not. */

  if (!start_room || !area)
    return;
  
  do
    {
      undo_marked (area);

      num_choices = 0;
      num_chose = 0;
      found_room = FALSE;
      find_connected_rooms_size (start_room, BADROOM_BITS);
      for (count = 0; count < 2; count++)
	{
	  for (z = dims->val[4]; z <= dims->val[5]; z++)
	    {
	      for (x = dims->val[0]; x <= dims->val[1]; x++)
		{
		  for (y = dims->val[2]; y <= dims->val[2]; y++)
		    {

		      /* Room must be a detail room in the block we're
			 looking at, and it cannot be marked. */
		      if (!city_coord_is_ok(x,y,z) ||
			  (room1 = city_grid[x][y][z]) == NULL ||
			  room1->mv < depth ||
			  IS_MARKED (room1))
			continue;
		      
		      /* Now check adjacent rooms to see if they're of
			 depth > room1->mv so we can attach them. */
		      
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
			  else 
			    break;
			  
			  /* Make sure that this room exists and
			     that it's in the grid we're looking at and
			     that it's marked and that it's of depth >= 
			     the current depth. */
			  if (city_coord_is_ok (nx, ny, nz) &&
			      (room2 = city_grid[nx][ny][nz]) != NULL &&
			      nx >= dims->val[0] &&
			      nx <= dims->val[1] &&
			      ny >= dims->val[2] &&
			      ny <= dims->val[3] &&			     
			      room2->mv >= depth &&
			      IS_MARKED (room2) &&
			      (exit = FNV (room1, dir + 1)) == NULL)
			    {
			      if (count == 0)
				num_choices++;
			      else if (--num_chose < 1)
				{
				  found_room = TRUE;
				  break;
				}
			    }
			  if (found_room)
			    break;
			}
		    }
		  if (found_room)
		    break;
		}
	      if (found_room)
		break;
	    }
	  
	  if (count == 0)
	    {
	      if (num_choices < 1)
		all_attached = TRUE;
	      else
		num_chose = nr (1, num_choices);
	    }
	  if (all_attached)
	    break;
	}
      if (room1 && !IS_MARKED (room1) && room1->mv >= depth &&
	  room2 && room2->mv >= depth && IS_MARKED (room2) &&
	  dir >= 0 && dir < FLATDIR_MAX)
	{
	  int door_flags = 0;

	  if (IS_ROOM_SET (room1, ROOM_INSIDE) ||
	      IS_ROOM_SET (room2, ROOM_INSIDE))
	    door_flags = EX_DOOR | EX_CLOSED;
	  room_add_exit (room1, dir, room2->vnum, door_flags);
	  room_add_exit (room2, RDIR(dir), room1->vnum, door_flags);
	}
      undo_marked (area);
    }
  while (!all_attached && ++attach_tries < 10);
			 
  

return;
}

  
  
  


/* This links rooms at a certain depth to rooms a depth above. */

void
citygen_connect_next_level_up (int depth, VALUE *dims)
{
  /* Starting and ending x, y, z, coords */
     
  int sx, sy, sz, ex, ey, ez;
  
  int cx, cy, cz;
  int x, y, z;
  
  int i, dir;
  int door_flags;
  
  /* How many links we have to make for this block of names, and
     how many choices of roomws we have left to pick from. */
  int num_links_left, num_choices_left;
  
  /* These are used for linking "up a level" to something of lower
     depth. ul_num is the number of links to each of these other
     named rooms, and ul_name is the name of each of the things
     being linked up to. */
  
  int ul_num[CITYGEN_UL_NAMES];
  char *ul_name[CITYGEN_UL_NAMES];
  THING *area = NULL; /* The area these rooms are in...used for marking. */
  THING *room, *nroom;

  if (!dims)
    return;

  sx = dims->val[0];
  ex = dims->val[1];
  sy = dims->val[2];
  ey = dims->val[3];
  sz = dims->val[4];
  ez = dims->val[5];
  
  for (i = 0; i < CITYGEN_UL_NAMES; i++)
    {
      ul_num[i] = 0;
      ul_name[i] = nonstr;
    }

  for (z = sz; z <= ez; z++)
    {
      for (x = sx; x <= ex; x++)
	{
	  for (y = sy; y <= ey; y++)
	    {
	      if (!city_coord_is_ok (x, y, z) ||
		  (room = city_grid[x][y][z]) == NULL ||
		  room->mv != depth)
		continue;
	      
	      /* Set this up so we know what area we clear marked bits
		 from down below. */
	      
	      if (!area)
		area = room->in;
	      
	      /* Only link these things to rooms on SAME Z LEVEL! */
	      for (dir = 0; dir < FLATDIR_MAX; dir++)
		{
		  /* Set current coords we will try to link to. */
		  cx = x;
		  cy = y;
		  cz = z;
		  
		  if (dir == DIR_NORTH)
		    cy++;
		  if (dir == DIR_SOUTH)
		    cy--;
		  if (dir == DIR_WEST)
		    cx--;
		  if (dir == DIR_EAST)
		    cx++;
		  
		  if (!city_coord_is_ok (cx, cy, cz) ||
		      (nroom = city_grid[cx][cy][cz]) == NULL ||
		      nroom->mv != depth-1 ||
		      !nroom->short_desc || 
		      !*nroom->short_desc)
		    continue;
		  
		  /* Ok so we found a room in the proper direction with
		     the proper depth. Try to find its name in our 
		     list of adjacent room names. */
		  
		  for (i = 0; i < CITYGEN_UL_NAMES; i++)
		    {
		      /* If we reach the end of the names we've found
			 so far... add this name to this new place. */
		      if (!ul_name[i] || !*ul_name[i])
			{
			  ul_num[i]++;
			  ul_name[i] = nroom->short_desc;
			  break;
			}
		      if (!str_cmp (NAME (nroom), ul_name[i]))
			{
			  ul_num[i]++;
			  break;
			}
		    }
		}
	    }
	}
    }
  
  /* Now we have a list of room names of things of the correct depth that
     are adjacent to this room and we have the number of times that they're
     adjacent so we can add the links we need. */

  for (i = 0; i < CITYGEN_UL_NAMES; i++)
    {
      /* Make sure we have a valid name we're looking for. */
      if (ul_num[i] < 1 || !ul_name[i] || !*ul_name[i])
	continue;
      
      undo_marked (area);
      num_links_left = nr (0, ul_num[i]/7)+1;
      if (ul_num[i] > 9)
	num_links_left++;
      num_choices_left = ul_num[i];
      
      for (z = sz; z <= ez; z++)
	{
	  if (num_links_left < 1)
	    break;
	  for (x = sx; x <= ex; x++)
	    {
	      if (num_links_left < 1)
		break;
	      for (y = sy; y <= ey; y++)
		{
		  if (num_links_left < 1)
		    break;
		  if (!city_coord_is_ok (x, y, z) ||
		      (room = city_grid[x][y][z]) == NULL ||
		      room->mv != depth)
		    continue;
		  
		 
		  /* Only link these things to rooms on SAME Z LEVEL! */
		  
		  for (dir = 0; dir < FLATDIR_MAX; dir++)
		    {
		      /* Set current coords we will try to link to. */
		      cx = x;
		      cy = y;
		      cz = z;
		      
		      if (dir == DIR_NORTH)
			cy++;
		      if (dir == DIR_SOUTH)
			cy--;
		      if (dir == DIR_WEST)
			cx--;
		      if (dir == DIR_EAST)
			cx++;
		      
		      if (!city_coord_is_ok (cx, cy, cz) ||
			  (nroom = city_grid[cx][cy][cz]) == NULL ||
			  nroom->mv != depth-1 ||
			  !nroom->short_desc || 
			  !*nroom->short_desc ||
			  str_cmp (ul_name[i], NAME(nroom)))
			continue;

		      /* Now check if we choose this room at random to
			 link up. */
		      
		      if (nr (1, num_choices_left) > num_links_left)
			{
			  num_choices_left--;
			  continue;
			}
		      
		      /* Add the exits and reduce the number of links
			 that we have to add. */
		      
		      if (IS_ROOM_SET (room, ROOM_INSIDE) ||
			  IS_ROOM_SET (nroom, ROOM_INSIDE))
			door_flags = EX_DOOR | EX_CLOSED;
		      else
			door_flags = 0;
		      
		      room_add_exit (room, dir, nroom->vnum, door_flags);
		      room_add_exit (nroom, RDIR(dir), room->vnum, door_flags);
		      num_links_left--;
		    }
		}
	    }
	}
    }
  return;
}
  
/* This links anything of depth 0 together to make the whole city
   into one thing. */

void
citygen_link_base_grid (void)
{
  int x, y, z;
  THING *nroom, *room;

  for (x = 0; x < CITY_SIZE; x++)
    {
      for (y = 0; y < CITY_SIZE; y++)
	{
	  for (z = 0; z < CITY_HEIGHT; z++)
	    {
	      if (!city_coord_is_ok (x,y,z) ||
		  (room = city_grid[x][y][z]) == NULL ||
		  room->mv != 1)
		continue;

	      if (x < CITY_SIZE-1 &&
		  city_coord_is_ok (x+1, y, z) &&
		  (nroom = city_grid[x+1][y][z]) != NULL && 
		  nroom->mv == 1)
		{
		  room_add_exit (room, DIR_EAST, nroom->vnum, 0);
		  room_add_exit (nroom, DIR_WEST, room->vnum, 0);
		}
	      if (y < CITY_SIZE-1 &&
		  city_coord_is_ok (x, y+1, z) &&
		  (nroom = city_grid[x][y+1][z]) != NULL &&
		  nroom->mv == 1)
		{
		  room_add_exit (room, DIR_NORTH, nroom->vnum, 0);
		  room_add_exit (nroom, DIR_SOUTH, room->vnum, 0);
		}
	    }
	}
    }
  return;
}
    
	      
	      

/* This adds a "stringy" detail to an area -- like a road or trail
   or something like that. It's because lines and blocks (which is
   what the other objects are like) seem to be different. 
   It is assumed that all rooms exist so we don't need to make any
   new rooms here. Just attach and rename certain rooms. */

void
citygen_add_stringy_detail (RESET *start_reset, THING *area, THING *to, VALUE *start_dims, int depth)
{
  int count = 0, num_choices = 0, num_chose = 0;
  VALUE *o_dims;
  
  /* The starting dims where we're allowed to search. Gotten from start
     dims. If start_dims is null, search the whole city. (at ground level). */
  
  int s_min_x, s_min_y, s_min_z, s_max_x, s_max_y, s_max_z;
  

  /* Starting x, y, z when we actually find a start location. */

  int sx = 0, sy = 0, sz = 0;

  THING *rst_obj;
  
  bool found_start_room = FALSE;
  
  int bad_dir_this_room = REALDIR_MAX;
  int dir;
  int x, y, z;
  
  THING *room;
  THING *start_obj, *obj;
  char word[STD_LEN];
  RESET *rst;
  MAPGEN *map = NULL;
  char buf[STD_LEN];
  int dx = 0, dy = 0;
  THING *nroom;
  int map_x = 0, map_y = 0, city_x, city_y, city_z;
  int old_room_num = 0, last_dir = REALDIR_MAX;
  bool found = FALSE;
  int num_open_dirs;
  int nx, ny;
  int reset_times = 1;
  int reset_percent = 100;
  int map_tries = 0;
  VALUE *randpop = NULL;
  
  THING *last_room = NULL, *first_room = NULL;
  
  /* The new object dims used to tell where to reset something along the
     road. */
  
  VALUE n_obj_dims;
  
  if (!area || depth < 1 || depth > 30 ||
      (!start_reset && !to))
    return;
  
  /* First order of business. If this obj is NOT within the
     GENERATOR_NOCREATE_VNUM_MIN/MAX range, then this is a REGULAR
     RESET PUT ONTO to!!!! if it's not a ROOM! 
     Also, we set the number of times and the percent here if this
     came from a reset up above. */
  
  if (start_reset)
    {
      start_obj = find_thing_num (start_reset->vnum);
      reset_times = MAX(1, start_reset->times);
      reset_percent = MAX(1, start_reset->pct);
    }
  else if (to)
    start_obj = to;
  else
    return;
  
  while (--reset_times >= 0)
    {
      if (nr (1,100) > reset_percent)
	continue;
      
      last_dir = REALDIR_MAX;
      old_room_num = 0;
      bad_dir_this_room = REALDIR_MAX;
      found_start_room = FALSE;
      
      if (randpop)
	{
	  obj = find_thing_num (calculate_randpop_vnum 
				(randpop, (to ? LEVEL (to) : LEVEL(area))));
	  if (!obj)
	    obj = start_obj;
	}
      else
	obj = start_obj;
      
      /* Need a dimension value on this object that says that
	 it's stringy. */
      
      if ((o_dims = FNV (obj, VAL_DIMENSION)) == NULL ||
	  !o_dims->word || !*o_dims->word || 
	  !named_in (o_dims->word, "stringy"))
	continue;
      
      n_obj_dims.word = nonstr;
      
      /* Get the starting and ending places to look. */
      
      if (start_dims)
	{
	  s_min_x = start_dims->val[0];
	  s_max_x = start_dims->val[1];
	  s_min_y = start_dims->val[2];
	  s_max_y = start_dims->val[3];
	  s_min_z = start_dims->val[4];
	  s_max_z = start_dims->val[5];
	}
      else
	{
	  s_min_x = s_min_y = 0;
	  s_max_x = s_max_y = CITY_SIZE-1;
	  s_min_z = s_max_z = CITYGEN_STREET_LEVEL;
	}
      
      
      
      
      /* Now search for the proper place to go. Search between search_min
	 and search_max in the x, y, z coords..with the rule that the
	 "base" level of the search area must contain all rooms, and you
	 can build up into blank rooms, but you can't add the rooms to the 
	 base level of rooms. */
      
      
      for (count = 0; count < 2; count++)
	{
	  for (x = s_min_x; x <= s_max_x; x++)
	    {
	      for (y = s_min_y; y <= s_max_y; y++)
		{
		  for (z = s_min_z; z <= s_max_z; z++)
		    {
		      bad_dir_this_room = REALDIR_MAX;
		      num_open_dirs = 0;
		      /* This is a bit different. We need rooms that
			 exist and are on an edge. of the area. */
		      
		      if (!city_coord_is_ok (x, y, z) ||
			  (room = city_grid[x][y][z]) == NULL ||
			  is_named (room, "road"))
			continue;
		      
		      /* The room is on an edge if it's at the
			 xyz edges, or if a room doesn't exist
			 adjacent to it. For each acceptable position,
			 add another direction to teh acceptable
			 directions list. */
		      
		      if (x == s_min_x ||
			  !city_coord_is_ok(x-1,y,z) ||
			  !city_grid[x-1][y][z])
			{
			  bad_dir_this_room = DIR_WEST;
			  num_open_dirs++;
			}
		      
		      if (x == s_max_x ||
			  !city_coord_is_ok(x+1,y,z) ||
			  !city_grid[x+1][y][z])
			{
			  bad_dir_this_room = DIR_EAST;
			  num_open_dirs++;
			}
		      
		      if (y == s_min_y ||
			  !city_coord_is_ok(x,y-1,z) ||
			  !city_grid[x][y-1][z])
			{
			  bad_dir_this_room = DIR_SOUTH;
			  num_open_dirs++;
			}
		      
		      if (y == s_max_y ||
			  !city_coord_is_ok(x,y+1,z) ||
			  !city_grid[x][y+1][z])
			{
			  bad_dir_this_room = DIR_NORTH;
			  num_open_dirs++;
			}
		      
		      /* Need exactly 1 edge room because corners suck. */
		      if (num_open_dirs == 1)
			{
			  if (count == 0)
			    num_choices++;
			  else if (--num_chose < 1)
			    {
			      found_start_room = TRUE;
			      sx = x;
			      sy = y;
			      sz = z;
			    }
			}
		    }
		  if (found_start_room)
		    break;
		}
	      if (found_start_room)
		break;
	    }
	  if (found_start_room)
	    break;
	  
	  if (count == 0)
	    {
	      if (num_choices < 1)
		break;
	      num_chose = nr (1, num_choices);
	    }	  
	}
      
      if (!city_coord_is_ok (sx, sy, sz) ||
	  (room = city_grid[sx][sy][sz]) == NULL ||
	  is_named (room, "road") ||
	  bad_dir_this_room == REALDIR_MAX)
	continue;
      
      
      /* Get the correct dx and dy based on where this
	 room edge is. We go in the opp dir to the "bad" dir. */
      
      
      if (bad_dir_this_room == DIR_EAST)
	dx--;
      else if (bad_dir_this_room == DIR_WEST)
	dx++;
      else if (bad_dir_this_room == DIR_NORTH)
	dy--;
      else if (bad_dir_this_room == DIR_SOUTH)
	dy++;
      else
	continue;
      
      
      sprintf (buf, "%d %d %d %d %d %d %d %d",
	       dx, /* Dx */
	       dy, /* Dy */
	       50, /* Length */
	       1, /* Width */ 
	       0,  /* Fuzziness */
	       0,   /* Holeyness */
	       7, /* Curviness */
	       0); /* Xtra Lines */
      
      do
	{
	  map = mapgen_generate (buf);
	  
	  if (map && map->num_rooms > 20)
	    break;
	  else
	    {
	      free_mapgen (map);
	      map = NULL;
	    }
	}
      while (!map && ++map_tries < 1000);
      
      if (!map)
	continue;

      /* Now we have a map. Overlay it onto the city map. */
      
      /* Get the name we will use here. */
      free_str (city_grid[sx][sy][sz]->short_desc);
      city_grid[sx][sy][sz]->short_desc = nonstr;
      generate_detail_name (obj, city_grid[sx][sy][sz]);
      capitalize_all_words (city_grid[sx][sy][sz]->short_desc);
      strcpy (word, city_grid[sx][sy][sz]->short_desc);
      
      /* Now loop through the sequence of rooms given by the 
	 mapgen map and set the rooms in the city to the same value.
	 The way this is done is that we start with the mapgen
	 center room and stick it into the start room, then we
	 copy the object values into that room and nuke the
	 room in the mapgen. then we go adjacent to the room and
	 find whatever room is next to and in the mapgen map. */
      
      city_x = sx;
      city_y = sy;
      city_z = sz;
      if (dx > 0)
	{
	  for (x = 0; x < MAPGEN_MAXX && !found; x++)
	    {
	      for (y = 0; y < MAPGEN_MAXY; y++)
		{
		  if (map->used[x][y])
		    {
		      map_x = x;
		      map_y = y;
		      found = TRUE;
		      break;
		    }
		}
	    }
	}
      else if (dx < 0)
	{
	  for (x = MAPGEN_MAXX-1; x >= 0 && !found; x--)
	    {
	      for (y = 0; y < MAPGEN_MAXY; y++)
		{
		  if (map->used[x][y])
		    {
		      map_x = x;
		      map_y = y;
		      found = TRUE;
		      break;
		    }
		}
	    }
	}
      
      else if (dy > 0)
	{
	  for (y = 0; y < MAPGEN_MAXY && !found; y++)
	    {
	      for (x = 0; x < MAPGEN_MAXX; x++)
		{
		  if (map->used[x][y])
		    {
		      map_x = x;
		      map_y = y;
		      found = TRUE;
		      break;
		    }
		}
	    }
	}
      else if (dy < 0)
	{
	  for (y = MAPGEN_MAXY-1; y >= 0 && !found; y--)
	    {
	      for (x = 0; x < MAPGEN_MAXY; x++)
		{
		  if (map->used[x][y])
		    {
		      map_x = x;
		      map_y = y;
		      found = TRUE;
		      break;
		    }
		}
	    }
	}
      
      
      
      /* Every time we are at a map room */
      old_room_num = 0;
      last_dir = REALDIR_MAX;
      undo_marked (area);
      while (map->used[map_x][map_y])
	{
	  /* Check if the city room is ok. It's ok if it's
	     the right depth or if it's named as a road. */
	  if (!city_coord_is_ok (city_x, city_y, city_z) ||
	      (room = city_grid[city_x][city_y][city_z]) == NULL ||
	      (room->mv >= depth &&
	       !is_named (room, "road")))
	    break;
	  
	  if (!first_room)
	    first_room = room;
	  last_room = room;
	  room->mv = depth;
	  free_str (room->short_desc);
	  append_name (room, obj->name);
	  room->short_desc = new_str (word);
	  append_name (room, "road");
	  SBIT (room->thing_flags, TH_MARKED);
	  if (flagbits (obj->flags, FLAG_ROOM1))
	    {
	      remove_flagval (room, FLAG_ROOM1, ~0);
	      add_flagval (room, FLAG_ROOM1, 
			   /*   flagbits (obj->flags, FLAG_ROOM1) | */ ROOM_ROUGH);
	      
	    }
	  
	  map->used[map_x][map_y] = 0;
	  
	  /* Now link to adjacent rooms. */
	  
	  for (dir = 0; dir < FLATDIR_MAX; dir++)
	    {
	      nx = city_x;
	      ny = city_y;
	      
	      if (dir == DIR_NORTH)
		ny++;
	      else if (dir == DIR_SOUTH)
		ny--;
	      else if (dir == DIR_EAST)
		nx++;
	      else if (dir == DIR_WEST)
		nx--;
	      
	      /* Now make sure this room won't be in the road
		 eventually. */
	      
	      
	      
	      if (city_coord_is_ok (nx, ny, city_z) &&
		  (nroom = city_grid[nx][ny][city_z]) != NULL &&
		  !is_named (nroom, "road") &&
		  nroom->mv <= depth &&
		  !FNV (room, dir + 1))
		{
		  if (map->used[map_x+(nx-city_x)][map_y+(ny-city_y)] ||
		      nr (1,5) == 2)
		    {
		      room_add_exit (room, dir, nroom->vnum, 0);
		      room_add_exit (nroom, RDIR (dir), room->vnum, 0);
		    }
		}
	    }
	  
	  
	  
	  
	  if (map->used[map_x+1][map_y])
	    {
	      map_x++;
	      city_x++;
	      last_dir = DIR_EAST;
	    }
	  else if (map->used[map_x-1][map_y])
	    {
	      map_x--;
	      city_x--;
	      last_dir = DIR_WEST;
	    }
	  else if (map->used[map_x][map_y+1])
	    {
	      map_y++;
	      city_y++;
	      last_dir = DIR_NORTH;
	    }
	  else if (map->used[map_x][map_y-1])
	    {
	      map_y--;
	      city_y--;
	      last_dir = DIR_SOUTH;
	    }
	  
	  old_room_num = room->vnum;
	}	      
      free_mapgen (map);
      map = NULL;
      
      /* Now make the ends of the roads "road ends" */
      
      append_name (first_room, "road_end");
      append_name (last_room, "road_end");
      
      /* At this point the rooms (or proto) are created, and the names
	 have been set. */
      
      /*  Now perform resets on this road. */
      
      /* These things should ALWAYS be a road reset,so we don't
	 need to add the "to" here.  These resets are screwy
	 and the times and pcts must be done here since
	 we can't search for their proper locations within the
	 stringy object we've made at the next level down. */
      
      
      for (rst = obj->resets; rst; rst = rst->next)
	{
	  RESET temp_reset;
	  int times;
	  if ((rst_obj = find_thing_num (rst->vnum)) == NULL)
	    continue;
	  
	  copy_reset (rst, &temp_reset);
	  temp_reset.times = 1;
	  
	 
	  for (times = 0; times < (int) rst->times; times++)
	    {
	      if (nr(1,100) <= MAX(2, rst->pct))
		{
		  /* Now set up the dims where the subobjects will be reset. 
		     Pick a random marked room at the correct depth
		     and it should be on the road. */
		  
		  num_choices = 0;
		  num_chose = 0;
		  found_start_room = FALSE;
		  
		  for (count = 0; count < 2; count++)
		    {
		      for (x = s_min_x; x <= s_max_x; x++)
			{
			  for (y = s_min_y; y <= s_max_y; y++)
			    {
			      for (z = s_min_z; z <= s_max_z; z++)
				{
				  if ((room = city_grid[x][y][z]) == NULL ||
				      !IS_MARKED (room) ||
				      room->mv != depth)
				    continue;
				  
				  if (count == 0)
				    num_choices++;
				  else if (--num_chose < 1)
				    {
				      found_start_room = TRUE;
				      sx = x;
				      sy = y;
				      sz = z;
				      break;
				    }
				}
			      if (found_start_room)
				break;
			    }
			  if (found_start_room)
			    break;		      
			}
		      if (found_start_room)
			break;
		      
		      if (count == 0)
			{
			  if (num_choices < 1)
			    break;
			  else
			    num_chose = nr (1, num_choices);
			}
		    }
		  
		  /* Make the dimensions for the next reset location,
		     "this room." */
		  
		  n_obj_dims.type = VAL_DIMENSION;
		  n_obj_dims.val[0] = n_obj_dims.val[1] = sx;
		  n_obj_dims.val[2] = n_obj_dims.val[3] = sy;
		  n_obj_dims.val[4] = n_obj_dims.val[5] = sz;
		  
		  
		  
		  citygen_add_detail (&temp_reset, area, NULL, &n_obj_dims, depth+ 1);
		}
	    }
	}
    } 
  if (depth < 2)    
    undo_marked (area);
  return;
}


/* This adds guards and gatehouses and stock eq to the area. */

void
citygen_add_guards (THING *area)
{
  THING *mob[CITYGEN_GUARD_MAX], *proto;
  THING *obj;
  int vnum;
  int times;
  
  char buf[STD_LEN];
  char nametype[STD_LEN];
  
  THING *jewelry_randpop_item = NULL;
  VALUE *randpop;
  int i;
  

  /* These variables are used to find and place guard posts. You
     find road rooms that are on the edge of the city and you place
     the gatehouses there and fill them with guards. */
  
  THING *room = NULL, *nroom = NULL;
  int x, y, z;
  int nx, ny, nz;
  
  int dir = REALDIR_MAX;
  int num_guard_posts = 0, guards_per_post = 0;
  
  /* Used to set up the min and max vnums for the armor and
     jewelry and such. */
  
  int armor_start_vnum = 0, armor_end_vnum = 0;
  int jewelry_start_vnum = 0, jewelry_end_vnum = 0;
  int overall_color;
  int num_worn = 0;
  
  if (!area)
    return;
  guards_per_post = MIN(10, area->mv/40);
  

  for (times = 0; times < CITYGEN_GUARD_MAX; times++)
    {
      if ((vnum = find_free_mobject_vnum (area)) == 0)
	return;
      
      
      proto = new_thing();
      proto->vnum = vnum;
      proto->thing_flags = MOB_SETUP_FLAGS;
      
      /* Set up level and max hps. */
      proto->max_hp = 30;
      proto->level = LEVEL(area)*5/4 + 10;
      

      thing_to (proto, area);
      add_thing_to_list (proto);
      proto->level = LEVEL(area);
      add_flagval (proto, FLAG_DET, AFF_DARK | AFF_INVIS | AFF_HIDDEN);
      /* Guards assist all. Nasty. */
      add_flagval (proto, FLAG_ACT1, ACT_ASSISTALL);
      if (times == 0)
	{
	  proto->name = new_str ("guard gateguard");
	  proto->short_desc = new_str ("a gateguard");
	  proto->long_desc = new_str ("A gateguard is here watching for trouble.");	  
	  add_flagval (proto, FLAG_ACT1, ACT_SENTINEL);
	}
      else if (times == 1)
	{
	  proto->name = new_str ("guard cityguard");
	  proto->short_desc = new_str ("a cityguard");
	  proto->long_desc = new_str ("An alert cityguard stands here.");	  
	  proto->max_mv = area->mv/10;
	  /* Add a reset to the area so the guards just 
	     wander all over. */
	  add_reset (area, proto->vnum, 50, proto->max_mv, 1);
	}
      mob[times] = proto;
    }

/* Now make the objects. */

  /* First generate a name that all objects will share. Either a
     metal name or a prefix or a suffix. */
  
  switch (nr (1,2))
    {
      case 1:
	sprintf (nametype, "prefix");
	break;
      case 2:
      default:
	sprintf (nametype, "suffix");
	break;
    }
  
  sprintf (buf, "%s '%s'", nametype, find_gen_word (OBJECTGEN_AREA_VNUM, nametype, NULL));
  
  /* Overall color for objects in the city. */
  overall_color = nr (1,15);
  sprintf (buf + strlen (buf), " overall_color %d", overall_color);
  
  /* Do this in 2 passes. First, we have one pass that makes
     common armor and weapons, and another that makes rare
     jewelry. Then we give resets of these items to the
     guards we created above. */
  
  /* Make armor. */
  
  for (i = ITEM_WEAR_NONE + 1; i < ITEM_WEAR_MAX; i++)
    {
      if (wear_data[i].how_many_worn > 1 &&
	  i != ITEM_WEAR_WIELD)
	continue;
      
      if (i == ITEM_WEAR_BELT)
	continue;

      num_worn = wear_data[i].how_many_worn;
      
      while (--num_worn >= 0)
	{
	  
	  if ((vnum = find_free_mobject_vnum (area)) == 0)
	    break;
	  
	  if ((obj = objectgen (area, i, LEVEL(area), vnum, buf)) == NULL)
	    break;
	  
	  /* Only set once. */
	  if (armor_start_vnum == 0)
	    armor_start_vnum = obj->vnum;
	  
	  /* Keeps going up. */
	  armor_end_vnum = obj->vnum;      
	}
    }
  
  
  /* Now do jewelry. */
  for (i = ITEM_WEAR_NONE + 1; i < ITEM_WEAR_MAX; i++)
    {
      if (wear_data[i].how_many_worn <= 1 ||
	  i == ITEM_WEAR_WIELD || i == ITEM_WEAR_BELT)
	continue;

      
      num_worn = wear_data[i].how_many_worn;
      
      while (--num_worn >= 0)
	{
	  
	  if ((vnum = find_free_mobject_vnum (area)) == 0)
	    break;
	  
	  if ((obj = objectgen (area, i, LEVEL(area), vnum, buf)) == NULL)
	    break;
	  
	  /* Only set once. */
	  if (jewelry_start_vnum == 0)
	    jewelry_start_vnum = obj->vnum;
	  
	  /* Keeps going up. */
	  jewelry_end_vnum = obj->vnum;      
	}
    }

  /* Set up the randpop item for the jewelry. */
  
  if ((vnum = find_free_mobject_vnum (area)) != 0 &&
      jewelry_start_vnum > 0 && 
      jewelry_end_vnum >= jewelry_start_vnum)
    {
      jewelry_randpop_item = new_thing();
      jewelry_randpop_item->vnum = vnum;
      jewelry_randpop_item->thing_flags = OBJ_SETUP_FLAGS;
      thing_to (jewelry_randpop_item, area);
      add_thing_to_list (jewelry_randpop_item);
      
      jewelry_randpop_item->name = new_str ("randpop");
      jewelry_randpop_item->short_desc = new_str ("randpop");
      randpop = new_value();
      randpop->type = VAL_RANDPOP;
      add_value (jewelry_randpop_item, randpop);
      randpop->val[0] = jewelry_start_vnum;
      randpop->val[1] = jewelry_end_vnum - jewelry_start_vnum+1;
      randpop->val[2] = 1;
    }
  
  /* Now add the item resets to the guards. */

  for (times = 0; times < CITYGEN_GUARD_MAX; times++)
    {
      if (!mob[times])
	continue;
      
      if (armor_start_vnum > 0 &&
	  armor_end_vnum >= armor_start_vnum)
	{
	  for (vnum = armor_start_vnum; vnum <= armor_end_vnum; vnum++)
	    {
	      if ((obj = find_thing_num (vnum)) != NULL)
		{
		  add_reset (mob[times], vnum, 50, 1, 1);
		}
	    }
	}
      
      if (jewelry_randpop_item)
	add_reset (mob[times], jewelry_randpop_item->vnum, 30, 3, 1);
    }

  
  /* Now we have to set up the guards and the guard posts. */
  
  if (mob[CITYGEN_GUARD_GATE])
    {
      char name[STD_LEN];
      
      /* YOU MUST FREE THIS THING BEFORE YOU RETURN IF YOU DON'T DO
	 THIS IS WILL CAUSE A MEMORY LEAK! IT WON'T BE TOO BAD SINCE
	 YOU ONLY GET ONE LEAK PER CITY GENERATED, BUT STILL. */
   
      /* Only do this for things at depth 2 (main roads) and things
	 on the z == CITYGEN_STREET_LEVEL */
      
      z = CITYGEN_STREET_LEVEL;
      for (x = 0; x < CITY_SIZE; x++)
	{
	  for (y = 0; y < CITY_SIZE; y++)
	    {
	      if (!city_coord_is_ok (x, y, z) ||
		  (room = city_grid[x][y][z]) == NULL ||
		  !is_named (room, "road_end") ) 
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
		  
		  
		  /* If an adjacent room doesn't exist, and it's a 
		     road end, then we call this a gate. */
		  if (!city_coord_is_ok (nx, ny, nz) ||
		      (nroom = city_grid[nx][ny][nz]) == NULL)
		    {
		      *name = '\0';
		      
		      if (nr (1,3) == 2)
			strcpy (name, string_gen ("a_an room_size gate_house", WORDLIST_AREA_VNUM));
		      else
			strcpy (name, string_gen ("a_an building_condition gate_house", WORDLIST_AREA_VNUM));
		      
		      capitalize_all_words (name);
		      if (*name)
			{
			  free_str (room->short_desc);
			  room->short_desc = new_str (name);
			  append_name (room, "gate_house");
			  add_reset (room, mob[CITYGEN_GUARD_GATE]->vnum, 50, guards_per_post, 1);
			  num_guard_posts++;
			  break;
			}
		    }
		}
	    }
	}
    }
  mob[CITYGEN_GUARD_GATE]->max_mv = num_guard_posts*guards_per_post;      
  return;
}


/* This fixes disconnected regions within the city by connecting
   things to adjacent open regions. */


/* It works by starting at the lowest room vnum in the area and
   doing a DFS to check connectivity. If it's connected, we stop
   else we continue to link new regions as needed. */

void 
citygen_connect_regions (THING *area, int max_jump_depth)
{
  THING *start_room, *room, *nroom = NULL;
  int count, num_choices = 0, num_chose = 0, 
    num_road_choices = 0, num_road_chose = 0;

  int total_num_rooms = 0;
  int current_rooms_connected;
  bool added_a_link; /* Did we add a room link. */
  
  bool connect_to_road = FALSE; /* Can we connect to a road? */
  
  bool found_room = FALSE;
  
  int dir = REALDIR_MAX;
  
  /* Need to loop through all rooms with these variables. */
  int x, y, z;

  /* These are the "adjacent" room variables we will use
     to find which adjacent rooms don't have a connection. */
  int nx, ny, nz;

  if (!area || (start_room = area->cont) == NULL ||
      !IS_ROOM (start_room) || max_jump_depth >= 10)
    return;

  if (max_jump_depth < 1)
    max_jump_depth = 1;
  
  undo_marked (area);
  
  for (room = area->cont; room; room = room->next_cont)
    {
      if (IS_ROOM (room))
	total_num_rooms++;
    }
  
  do
    {
      added_a_link = FALSE;
      undo_marked (area);
      
      current_rooms_connected = find_connected_rooms_size (start_room, ~0);
     
      if (current_rooms_connected >= total_num_rooms)
	break;
      
      num_choices = 0;
      num_chose = 0;
      num_road_choices = 0;
      num_road_chose = 0;
      connect_to_road = FALSE;
      for (count = 0; count < 2; count++)
	{
	  for (x = 0; x < CITY_SIZE; x++)
	    {
	      for (y = 0; y < CITY_SIZE; y++)
		{
		  for (z = 0; z < CITY_HEIGHT; z++)
		    {
		      if (!city_coord_is_ok (x, y, z) ||
			  (room = city_grid[x][y][z]) == NULL ||
			  !IS_MARKED (room))
			continue;
		      for (dir = 0; dir < REALDIR_MAX; dir++)
			{
			  
			  /* Find the coordinates of the adjacent room
			     we care about. */
			  
			  nx = x;
			  ny = y;
			  nz = z;
			  
			  if (dir == DIR_NORTH)
			    ny++;
			  if (dir == DIR_SOUTH)
			    ny--;
			  if (dir == DIR_EAST)
			    nx++;
			  if (dir == DIR_WEST)
			    nx--;
			  if (dir == DIR_UP)
			    nz++;
			  if (dir == DIR_DOWN)
			    nz--;
			  
			  if (city_coord_is_ok(nx, ny, nz) &&
			      (nroom = city_grid[nx][ny][nz]) != NULL &&
			      IS_ROOM (nroom) &&
			      !IS_MARKED (nroom) &&
			      nroom->mv - room->mv <= max_jump_depth &&
			      room->mv - nroom->mv <= max_jump_depth)
			    {
			     
			      if (is_named (nroom, "road"))
				{
				  connect_to_road = TRUE;
				  if (count == 0)
				    num_road_choices++;
				  else if (--num_road_chose < 1)
				    {
				      found_room = TRUE;
				      break;
				    }
				}
			      if (!connect_to_road)
				{
				  if (count == 0)
				    num_choices++;
				  else if (--num_chose < 1)
				    {
				      found_room = TRUE;
				      break;
				    }
				}
			      
			    }
			  if (found_room)
			    break;
			}
		      if (found_room)
			break;
		    }
		  if (found_room)
		    break;
		}
	      if (found_room)
		break;
	    }
	  if (found_room)
	    break;
	  
	  if (count == 0)
	    {
	      if (connect_to_road)
		{
		  if (num_road_choices < 1)
		    break;
		  num_road_chose = nr (1, num_road_choices);
		}
	      else
		{
		  if (num_choices < 1)
		    break;
		  num_chose = nr (1, num_choices);
		}
	    }
	}
  
  /* We should have a room. */



      if (room && nroom && found_room && dir >= 0 &&
	  dir < REALDIR_MAX)
	{
	  room_add_exit (room, dir, nroom->vnum, 0);
	  room_add_exit (nroom, RDIR (dir), room->vnum, 0);
	  added_a_link = TRUE;
	}     
    }
  while (added_a_link);
 
  undo_marked (area);
  
  if (find_connected_rooms_size (start_room, ~0) < total_num_rooms)
    citygen_connect_regions (area, max_jump_depth + 1);
  return;
  
}

