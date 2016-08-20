#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "serv.h"
#include "mapgen.h"


/* This code is designed to let you generate pieces of maps by specifying
   certain characteristics you would like them to have, such as
   the direction of travel, the width of the generated map, the
   fuzziness of the line generated, and the amount of holes or 
   openings you find within the map. Hopefully, this will become
   a general purpose map-building code that will let you make a
   lot of different kinds of maps by pasting pieces together. */


int mapgen_count = 0;  /* Total number of mapgens made. */
MAPGEN *mapgen_free = NULL; /* The free list of mapgens. */


/* This is a "global" in this module because I want to do a dfs search
   to find the largest connected component attached to the central
   pathway after the map is generated. */

static short mapgen_used[MAPGEN_MAXX *2][MAPGEN_MAXY *2];


/* This makes a new mapgen, or finds on on the free list. */

MAPGEN *
new_mapgen (void)
{
  MAPGEN *new_mapgen;
  
  if (mapgen_free)
    {
      new_mapgen = mapgen_free;
      mapgen_free = mapgen_free->next;
    }
  else
    {
      mapgen_count++;
      new_mapgen = (MAPGEN *) mallok (sizeof (MAPGEN));
    }
  
  /* Clear out the data. */
  bzero (new_mapgen, sizeof (MAPGEN));
  new_mapgen->next = NULL;
  return new_mapgen;
}

/* This takes a mapgen and adds it to the freed list.
   Note that this mapgen never gets saved when the player logs out,
   so there is no need to read or write it from/to a file. */
   
void
free_mapgen (MAPGEN *mapgen)
{
  if (!mapgen)
    return;
  
  bzero (mapgen, sizeof (MAPGEN));
  
  mapgen->next = mapgen_free;
  mapgen_free = mapgen;
  return;
}

/* This shows a mapgen to a player. It first checks the actual height
   used so that the player doesn't get spammed off if there's no
   reason to be spammed off. */

void
show_mapgen (THING *th, MAPGEN *mapgen)
{
  int x, y, maxy = 0, miny = MAPGEN_MAXY, i;
  int lastx[MAPGEN_MAXY]; /* Where the last x shows up in a row. */
  char buf[STD_LEN];

  char *t;

  if (!th || LEVEL (th) < MAX_LEVEL || !IS_PC (th) || !mapgen)
    return;

  for (i = 0; i < MAPGEN_MAXY; i++)
    lastx[i] = -1;
  
  /* Find the min and max heights that we used to avoid spamming off
     the player as much as possible. */
  
  stt ("\n\r", th);
  for (y = 0; y < MAPGEN_MAXY; y++)
    {
      for (x = 0; x < MAPGEN_MAXX; x++)
	{
	  if (mapgen->used[x][y])
	    {
	      if (maxy < y) /* Set the max y used. */
		maxy = y;
	      if (miny > y) /* Set the min y used. */
		miny = y;
	      lastx[y] = x; /* Set the last x position used in this row. */
	    }
	}
    }

  /* Now, we only show the map from miny to maxy. */

  
  for (y = miny; y <= maxy; y++)
    {
      if (lastx[y] == -1)
	{
	  stt ("\n\r", th);
	  continue;
	}
      t = buf;      
      for (x = 0; x < MAPGEN_MAXX; x++)
	{
	  /* Add either a ' ' or a '#' depending on if that block is used. */
	  *t++ = (mapgen->used[x][y] ? '#' : ' ');
	  
	  if (x > lastx[y]) /* Don't keep adding ' 's if no more rooms. */
	    break; 
	}
      *t++ = '\n'; /* Add the newline and terminate the string. */
      *t++ = '\r';
      *t++ = '\0';
      stt (buf, th);
    }
  
  sprintf (buf, "\n\n\rThis map used %d rooms.\n\n\r", mapgen->num_rooms);
  stt (buf, th);
  return;
}

/* This is the do mapgen function. Making a map is a two-step process to
   avoid mistakes. First, you generate the map, then you should look
   at it, then you will need to type mapgen create. You can also use
   mapgen show or mapgen show <playername> to see their map before
   they create it. There are also several arguments you can turn on and
   off if you want. */

void
do_mapgen (THING *th, char *arg)
{
  char *oldarg;
  THING *vict;
  char arg1[STD_LEN];
  MAPGEN *oldmap = NULL;
  
  if (!th || !IS_PC (th) || LEVEL (th) != MAX_LEVEL)
    return;
  
  oldarg = arg;
  
  arg = f_word (arg, arg1);

  /* Clear your current map. */

  if (!*arg1)
    {
      stt ("mapgen dx dy length width fuzziness holeyness curviness extra_lines\n\r", th);
      return;
    }

  if (!str_cmp (arg1, "clear") || !str_cmp (arg1, "delete") ||
      !str_cmp (arg1, "nuke"))
    {
      if (!th->pc->mapgen)
	{
	  stt ("You don't have a mapgen to delete!\n\r", th);
	  return;
	}
      free_mapgen (th->pc->mapgen);
      th->pc->mapgen = NULL;
      stt ("Ok, mapgen cleared.\n\r", th);
      return;
    }

  /* Show you a map. Either yours or someone else's. */

  if (!str_cmp (arg1, "show")) 
    {
      MAPGEN *mapgen_show = NULL;
      if (*arg)
	{
	  if ((vict = find_pc (th, arg)) == NULL)
	    {
	      stt ("That person doesn't exist!\n\r", th);
	      return;
	    }
	  if (LEVEL (vict) != MAX_LEVEL)
	    {
	      stt ("That person isn't an admin, so they can't use mapgen.\n\r", th);
	      return;
	    }
	  if ((mapgen_show = vict->pc->mapgen) == NULL)
	    {
	      stt ("That person doesn't have a mapgen at the moment!\n\r", th);
	      return;
	    }
	}
      else if ((mapgen_show = th->pc->mapgen) == NULL)
	{
	  stt ("You don't have a mapgen to look at right now!\n\r", th);
	  return;
	}
      
      /* At this point, show_mapgen should exist either way. */
      
      show_mapgen (th, mapgen_show);
      return;
    }
  
  if (!str_cmp (arg1, "make") || !str_cmp (arg1, "generate") ||
      !str_cmp (arg1, "create"))
    {
      /* Get the start vnum where you will make the new map. */
      
      int start_vnum = atoi (arg);
      
      /* Attempt to make the map starting at that number. */
      
      stt (mapgen_create (th->pc->mapgen, start_vnum), th);
      return;
    }
  
  /* Move the arg back to the start. If we get to this point, it's because
     we want to generate a new map. */
  
  arg = oldarg;
  oldmap = th->pc->mapgen;
  th->pc->mapgen = mapgen_generate (arg);
 
  if (th->pc->mapgen)
    {
      show_mapgen (th, th->pc->mapgen);
      free_mapgen (oldmap);
    }
  else
    stt ("mapgen dx dy length width fuzziness holeyness curviness extra_lines\n\r", th);
  return;
}

/* This generates a random chunk of map. It takes a TON of arguments.
   To generate a map you must enter the following:

   mapgen dx dy len width fuzziness holeyness curviness
   
   They are each explained below, but you MUST specify all of the 
   arguments, or else it will bail out. 

   For those of you keeping score at home, this essentially carries
   out a modification of the rasterization technique. It just
   takes the basic digitized line and squiggles it some and maybe adds some
   "fatness" to the line. */

MAPGEN *
mapgen_generate (char *arg)
{
  char arg1[STD_LEN];
  int dx = 0; /* How much the map moves in the x direction. */
  int dy = 0; /* How much the map moves in the y direction. */
  int len = 0; /* How many spaces (pythagorean) the map will move.*/
  int x0, y0, x1, y1; /* Where the map will start and end.*/
  int width = 0; /* The width of the map If dx and dx = 0, this = radius.*/
  int fuzziness = 0; /* How much the width varies.*/
  int curviness = 0; /* How much the map deviates from a straight line.*/
  int holeyness = 0;    /* How "holey" the map is...missing pieces.*/
  int extra_lines = 0; /* How many "extra lines" the map will have. */
  int x, y, i; /* Counters; Also current x and y during rasterizing.*/
  int x_len, y_len; /* How far the map will go in the x and y dirs. */
  int x_step, y_step;       /* Steps used for rastering. */
  int x_count, y_count;       /* Counters used for rasterizing.*/
  int decrement;    /* Decrement counter for rastering.*/
  int jump_dist;    /* How far the path jumps with curviness.*/
  int total_len;    /* Total length in the x and y directions.*/
  int curr_width; /* The current width of the map here.*/
  int last_width; /* To keep it from being to "spiky."*/
  bool x_big = FALSE; /* Is the dx > the dy? */ 
  MAPGEN *map = NULL;     /* The new mapgen data.*/
  bool curve_jump_last_room = FALSE; /* Did curviness jump us last room? */
  bool curve_jump_now;  /* Did we curve jump this room? */
  /* A matrix of the squares that have been used. */
  
  
  /* These are for use at the end when we are "centering" the map created
   so that it will fit nicely into the mapgen struct that will be generated
   for the player to see. */
  
  int minx = 2 * MAPGEN_MAXX, maxx = 0;
  int miny = 2 * MAPGEN_MAXY, maxy = 0;
  
  /* These are the midpoints of the generated maps. */
  
  int midx, midy;
  
  /* These are the differences between the midpoint of the generated map,
     and where they will have to be mapped on the player's map. */
  
  int diffx, diffy;

  /* This is used to tell if you pass bad parameters to the mapgen script. */
  
  bool bad_param = FALSE;
  
  
  /* Strip off the arguments one by one, and make sure we have all of them. */
  
  arg = f_word (arg, arg1);
  if (!isdigit (*arg1) && *arg1 != '-')
    return NULL;
  dx = atoi (arg1);

  arg = f_word (arg, arg1);
  if (!isdigit (*arg1) && *arg1 != '-')
    return NULL;
  dy = atoi (arg1);
  
  /* This is needed because the matrix is actually flipped upside down.
     Heh. */
  
  dy = -dy;
  
  if (dx == 0 && dy == 0)
    return NULL;

  arg = f_word (arg, arg1);
  if (!isdigit (*arg1))
    return NULL;
  len = atoi (arg1);
  if (len < 2) 
    len = 2;

  
  arg = f_word (arg, arg1);
  
  if (!*arg1)
    width = 1;
  else
    {
      if (!isdigit (*arg1))
	return NULL;
      width = atoi (arg1);
      if (width < 1) /* The width must be at least 1. */
	width = 1; 
    }
  


  /* Fuzziness varies the width of the block. */
  
  arg = f_word (arg, arg1);
  
  if (!*arg1)
    fuzziness = 0;
  else
    {   
      if (!isdigit (*arg1))
	return NULL;
      fuzziness = atoi (arg1);
      if (fuzziness < 0) /* The fuzziness must be at least 0. */
	fuzziness = 0;
      
      if (fuzziness > 0)
	{
	  if (nr(0,1) == 0)
	    width -= nr (0, 2*fuzziness);
	  else
	    width += nr (0, 2*fuzziness);
	}
      if (width < 1)
	width = 1;
    }
  
  
  /* The holeyness represents how likely it is to have a hole in the graph. */
  /* Holeyness around 8-9 (out of 20) makes interesting graphs... if
     holeyness is < 8 the graph is full, if it's > 9  or 10, then it's
     not usually much more than a line. */
  
  arg = f_word (arg, arg1);
  
  if (!*arg1)
    holeyness = 0;
  else
    {
      if (!isdigit (*arg1))
	return NULL;
      holeyness = atoi (arg1);
      if (holeyness < 0) /* Holeyness must be at least 0. */
	holeyness = 0;
    }
  

  /* This makes the straight line bend a bit. */

  arg = f_word (arg, arg1);
  
  if (!*arg1)
    curviness = 0;
  else
    {
      if (!isdigit (*arg1))
	return NULL;
      curviness = atoi (arg1);
      if (curviness < 0) /* The curviness must be at least 0. */
	curviness = 0; 
    }
  
  arg = f_word (arg, arg1);
  
  if (!*arg1)
    extra_lines = 0;
  else
    {
      if (!isdigit (*arg1))
	return NULL;
      extra_lines = atoi (arg1);
      if (extra_lines < 0) /* The extra_lines must be at least 0. */
	extra_lines = 0; 
    }
  
  
  
  /* Ok, so now we have all of the arguments, start the map generation. */
  
  
  
  for (x = 0; x < MAPGEN_MAXX * 2; x++)
    {
      for (y = 0; y < MAPGEN_MAXY *2; y++)
	{
	  mapgen_used[x][y] = 0;
	}
    }
  
  /* Start in the middle of the temp map. */
  
  mapgen_used[MAPGEN_MAXX][MAPGEN_MAXY] = MAPGEN_USED;
  x = MAPGEN_MAXX;
  y = MAPGEN_MAXY;
  
  
  
  total_len = ABS(dx)+ABS(dy); /* This should not be 0. */
 
  if (total_len < 1)
    total_len = 1;
  
  x_len = dx*len/total_len;
  y_len = dy*len/total_len;
 
  if (y_len == 0)
    {
      if (dy < 0)
	y_len = -1;
      else if (dy > 0)
	y_len = 1;
    }
  if (x_len == 0)
    {
      if (dx < 0)
	x_len = -1;
      else if (dx > 0)
	x_len = 1;
    }
 
  
  /* Beginning point is the middle. */
  
  x0 = x;
  y0 = y;
  
  /* End point is the middle plus the x and y distances you go. */
  
  x1 = x0 + x_len;
  y1 = y0 + y_len;
  
  /* Now begin rasterization. */
   
  
  /* Set up the x increment and y increment. */
  
  if (x_len > 0) 
    x_step = 1;
  else if (x_len < 0)
    {
      x_step = -1;
      x_len = -x_len;    
    }
  else
    x_step = 1;
  
  if (y_len > 0)
    y_step = 1;
  else if (y_len < 0)
    {
      y_step = -1;
      y_len = -y_len;
    }
  else
    y_step = 1;
  
  /* Note that since dx and y_len cannot both be zero, at least one of x_step
     and y_step have to be nonzero. */
  
  x_count = 2*x_len;
  y_count = 2*y_len;
  
  /* At this point x = MAPGEN_MAXX and y = MAPGEN_MAXY */
  
  last_width = width;
  
  /* Find which direction is longer/"bigger" and set the base decrement. */
  
  if (y_len <= x_len)
    {
      x_big = TRUE;
      decrement = y_count-x_count;
      x -= x_step;
    }
  else
    {
      x_big = FALSE; 
      decrement = x_count-y_count;
      y -= y_step;
    }
  
  /* This loop is complicated because I want to take care of both
     of the cases when dx > dy and when dy > dx since the internal code
     is more complicated than simply drawing a line. */
  
  while(TRUE)
    {
      if (x_big)
	{
	  x += x_step;
	  decrement += y_count;
	}
      else
	{
	  y += y_step;
	  decrement += x_count;
	}
      mapgen_used[x][y] = MAPGEN_USED;
      
      jump_dist = 0;
      curve_jump_now = FALSE;
      if (decrement >= 0 || nr (1,50) <= curviness)
	{
	  if (x_big)
	    decrement -= x_count;
	  else
	    decrement -= y_count;
	  if (nr (1,20) > curviness ||
	      curve_jump_last_room)
	    {
	      if (x_big)
		y += y_step;
	      else
		x += x_step;
	      mapgen_used[x][y] = MAPGEN_USED;
	    }
	  else
	    {
	      curve_jump_now = TRUE;
	      jump_dist = MID (1, curviness/3, 4);
	      jump_dist = nr (-jump_dist, jump_dist);
	      if (jump_dist > 0)
		{
		  for (i = 0; i <= jump_dist; i++)
		    {
		      if (x_big)
			mapgen_used[x][y + i] = MAPGEN_USED;
		      else
			mapgen_used[x + i][y] = MAPGEN_USED;
		    }
		}
	      else
		{
		  for (i = 0; i >= jump_dist; i--)
		    {
		      if (x_big)
			mapgen_used[x][y + i] = MAPGEN_USED;
		      else
			mapgen_used[x + i][y] = MAPGEN_USED;
		    }
		}
	      if (x_big)
		y += jump_dist;
	      else
		x += jump_dist;	      
	    }	  
	} 
      
      /* Stay within the map bounds. */
      if (x < 4 || x >= MAPGEN_MAXX * 2 - 4 || y < 4 
	  || y >= MAPGEN_MAXY * 2 - 4)
	break;
      

      if (curve_jump_now)
	curve_jump_last_room = TRUE;
      else
	curve_jump_last_room = FALSE;
      
      /* Now "fatten the line" */
       
      curr_width = MAX (1, last_width); 
      if (fuzziness > 0)
	curr_width += nr (0,fuzziness) - nr (0,fuzziness);
      
      if (curr_width > width && nr (1,3) == 2)
	curr_width--;
      if (curr_width < width && nr (1,3) == 2)
	curr_width++;
      if (curr_width < width/2 && last_width < width/2)
	    curr_width++;
      if (curr_width - last_width > 2)
	curr_width = last_width + 2;
      else if (last_width - curr_width > 2)
	curr_width = last_width - 2;
      last_width = curr_width;
      
      if (curr_width < 2)
	{
	  if (x_big && x == x1)
	    break;
	  else if (!x_big && y == y1)
	    break;
	  continue;
	}
      
      /* We fatten in the x or y direction. */
      
      if (x_big)
	{
	  for (i = MAX (0, y - curr_width/2 - curr_width % 2); 
	       i < MIN(MAPGEN_MAXY * 2-1, y + curr_width/2); i++)
	    {
	      if (nr (1,20) > holeyness)
		mapgen_used[x][i] = MAPGEN_USED;
	    }
	  if (x == x1)
	    break;
	}
      else
	{
	  for (i = MAX (0, x - curr_width/2 - curr_width % 2); 
	       i < MIN(MAPGEN_MAXX * 2- 1, x + curr_width/2); i++)
	    {
	      if (nr (1,20) > holeyness)
		mapgen_used[i][y] = MAPGEN_USED;
	    }
	  if (y == y1)
	    break;
	}
    }
  
  /* At this point, add some random rooms to the map in certain rows and
     columns to make it a little more connected. */
  
  if (width > 2 && holeyness > 0)
    {
      for (x = 0; x < MAPGEN_MAXX *2; x++)
	{
	  if (nr (1,8) == 4)
	    {
	      for (y = 0; y < MAPGEN_MAXY *2; y++)
		{
		  if (nr (1,8) == 7)
		    mapgen_used[x][y] = MAPGEN_USED;
		}
	    }
	}
      
      for (y = 0; y < MAPGEN_MAXY; y++)
	{
	  if (nr (1,8) == 5)
	    {
	      for (x = 0; x < MAPGEN_MAXX; x++)
		{
		  if (nr (1,8) == 2)
		    mapgen_used[x][y] = MAPGEN_USED;
		}
	    }
	}
    } 

  /* Now add some extra lines if it's requested. These are vertical or
     Horizintal lines that hopefully will fill in some of the gaps in
     the holes in the map.  */

  if (extra_lines > 0)
    {
      int el_count, temp;
      
      /* If x0 and x1 and y0 and y1 aren't in the correct order, swap them. */
      if (x0 > x1)
	{
	  temp = x1;
	  x1 = x0;
	  x0 = temp;
	}
      if (y0 > y1)
	{
	  temp = y1;
	  y1 = y0;
	  y0 = temp;
	}
      
      for (el_count = 0; el_count < extra_lines; el_count++)
	{
	  if (nr (0,1) == 1)
	    {
	      x = nr (x0, x1);	  
	      for (y = y0; y < y1; y++)
		{
		  if (nr (1,2) != 2)
		    mapgen_used[x][y] = MAPGEN_USED;
		  if (nr (1,3) == 2)
		    {	
		      if (nr (0,1) == 1)
			x += nr (1,6);
		      else
			x -= nr (1,6);
		      x = MID (x0, x, x1);
		    }
		}
	    }
	  else
	    {
	      y = nr (y0, y1);	  
	      for (x = x0; x < x1; x++)
		{
		  if (nr (1,2) != 2)
		    mapgen_used[x][y] = MAPGEN_USED;
		  if (nr (1,3) == 2)
		    {		  
		      if (nr (0,1) == 1)
			y += nr (1,6);
		      else
			y -= nr (1,6);
		      y = MID (y0, y, y1);
		    }
		}
	    }
	}
    }
  /* Now we find the largest connected component attached to the middle path.
     This is just a simple dfs. */ 
  
  mapgen_connection (MAPGEN_MAXX, MAPGEN_MAXY);
  
  
  /* See what the min and max bounds for the generated map are. */
  
  for (x = 0; x < MAPGEN_MAXX * 2; x++)
    {
      for (y = 0; y < MAPGEN_MAXY * 2; y++)
	{
	  if (mapgen_used[x][y] == MAPGEN_SEARCHED)
	    {
	      /* Adjust the min and max values. */
	      if (minx > x)
		minx = x;
	      if (maxx < x)
		maxx = x;
	      if (miny > y)
		miny = y;
	      if (maxy < y)
		maxy = y;
	    }
	}
    }
  
  
  
  /* Now get the x and y lengths. If they are too big, then shrink them. */
  
  x_len = maxx - minx;
  y_len = maxy - miny;
  
  /* Crop the map if it's too big. */
  
  if (x_len > MAPGEN_MAXX - 1)
    {
      minx += ((x_len - MAPGEN_MAXX)/2 + 1);
      maxx -= ((x_len - MAPGEN_MAXX)/2 + 1);
    }
  
  if (y_len > MAPGEN_MAXY - 1)
    {
      miny += ((y_len - MAPGEN_MAXY)/2 + 1);
      maxy -= ((y_len - MAPGEN_MAXY)/2 + 1);
    }
  
  
  
  /* So by now the size of the map should be ok.
     Now, recalculate the x_len and y_len. This will give us the 
     location of the center that we want. */
  
  x_len = maxx - minx;
  y_len = maxy - miny;
  
  
  
  
  /* Create a new mapgen to put the map into. */
  
  map = new_mapgen();
  

  map->min_x = minx;
  map->max_x = maxx;
  map->min_y = miny;
  map->max_y = maxy;

  /* Set the number of rooms used. Not quite correct if we crop the map,
     but it is an overestimate, so it's ok. You could do a more precise
     cropping and recalc the rooms. Also, note that the mapgen struct will
     have all rooms listed as unused to start with. */
  
  
  
  /* Find the midpoints of the generated map. */
  
  midx = (minx + maxx)/2;
  midy = (miny + maxy)/2;
  
  /* This sets up the difference between the generated map and the player's
     own mapgen struct, since the midpoint of the generated map at 
     (midx, midy) has to map to (MAPGEN_MAXX/2, MAPGEN_MAXY/2). */

  
  
  diffx = MAPGEN_MAXX/2 - midx;
  diffy = MAPGEN_MAXY/2 - midy;
  
  map->num_rooms = 0;
  for (x = minx; x <= maxx && !bad_param; x++)
    {
      for (y = miny; y <= maxy; y++)
	{
	  if ((x + diffx) < 0 || (x + diffx) >= MAPGEN_MAXX ||
	      (y + diffy) < 0 || (y + diffy) >= MAPGEN_MAXY)
	    {
	      bad_param = TRUE;
	      break;
	    }
	  if (mapgen_used[x][y] == MAPGEN_SEARCHED)
	    {
	      map->used[x + diffx][y + diffy] = TRUE;
	      map->num_rooms++;
	    }
	}
    }
  return map;
}


/* This translates the mapgen map that was created into a real block
   of rooms. The thing that calls this must be a pc and it must have
   a mapgen already created. It must then make sure that start_vnum is
   inside an area, and that every thing from start_vnum up to
   start_vnum + num_rooms is a room in the same area which hasn't been 
   made yet. Then, the game will assign numbers to the blocks in the map
   that was generated and make the exits and link it all up. */

char *
mapgen_create (MAPGEN *map, int start_vnum)
{
  THING *ar;     /* The area where the map gets made. */
  THING *room;   /* The rooms being made. */
  VALUE *exit;   /* The exits as they get made. */
  int x, y;      /* Counters for the maps. */
  int i, count;  /* General counters. */
  static char buf[STD_LEN];
  int new_map[MAPGEN_MAXX][MAPGEN_MAXY]; /* The map generated. */

  /* These variables are used to determine if the map is more tall
     or wide so that the vnums can be assigned more appropriately. */
  int x_max = -1, y_max = -1, x_min = MAPGEN_MAXX,  y_min = MAPGEN_MAXY;
  bool x_wider = FALSE;
  
  if (!map)
    return "You haven't generated a map yet!";
  
  
  if ((ar = find_area_in (start_vnum)) == NULL)
    {
      return "That starting vnum isn't in an area!";
    }
  
  if (start_vnum <= ar->vnum || start_vnum > ar->vnum + ar->mv ||
      start_vnum + map->num_rooms <= ar->vnum ||
      start_vnum + map->num_rooms > ar->vnum + ar->mv)
    {
      return "Your beginning and ending rooms must lie within the allocated rooms for the area.\n\r";
    }
  
  for (i = start_vnum; i < start_vnum + map->num_rooms; i++)
    {
      if ((room = find_thing_num (i)) != NULL)
	{
	  sprintf (buf, "Room %d already exists, cannot make the map.\n\r", i);
	  return buf;
	}
    }

  /* If the map is more tall than wide, then the vnums get put in
     rows up and down the map. If the map is more wide than tall,
     then the vnums get put into columns across the map. */
  
  for (x = 0; x < MAPGEN_MAXX; x++)
    {
      for (y = 0; y < MAPGEN_MAXY; y++)
	{
	  if (map->used[x][y])
	    {
	      if (x < x_min)
		x_min = x;
	      if (x > x_max)
		x_max = x;
	      if (y < y_min)
		y_min = y;
	      if (y > y_max)
		y_max = y;
	    }
	}
    }
  
  
  /* Now see which direction is bigger. */
  
  if ((x_max-x_min) >= (y_max - y_min))
    x_wider = TRUE;
  else
    x_wider = FALSE;
  
  count = start_vnum;

  /* Place the vnums on the new map and create the new rooms. */
  
  if (x_wider)
    {
      for (x = 0; x < MAPGEN_MAXX; x++)
	{
	  for (y = 0; y < MAPGEN_MAXY; y++)
	    {
	      if (map->used[x][y])
		{
		  new_map[x][y] = count;
		  room = new_room(count);
		  count++;
		}
	      else
		new_map[x][y] = 0;
	    }
	}
    }
  else
    {
      for (y = 0; y < MAPGEN_MAXY; y++)
	{
	  for (x = 0; x < MAPGEN_MAXX; x++)
	    {
	      if (map->used[x][y])
		{
		  new_map[x][y] = count;
		  room = new_room(count);
		  count++;
		}
	      else
		new_map[x][y] = 0;
	    }
	}
    }
  /* Now link up the exits. */
  
  for (x = 0; x < MAPGEN_MAXX; x++)
    {
      for (y = 0; y < MAPGEN_MAXY; y++)
	{
	  if (new_map[x][y])
	    {
	      if ((room = find_thing_num (new_map[x][y])) == NULL)
		continue;
	      
	      if (x > 0 && new_map[x - 1][y]) /* West exit. */
		{
		  exit = new_value();
		  exit->type = DIR_WEST + 1;
		  exit->val[0] = new_map[x - 1][y];
		  add_value (room, exit);
		}
	      if (x < MAPGEN_MAXX - 1 && new_map[x + 1][y]) /* East exit. */
		{
		  exit = new_value();
		  exit->type = DIR_EAST + 1;
		  exit->val[0] = new_map[x + 1][y];
		  add_value (room, exit);
		}
	      
	      if (y > 0 && new_map[x][y - 1]) /* North exit. */
		{
		  exit = new_value();
		  exit->type = DIR_NORTH + 1;
		  exit->val[0] = new_map[x][y - 1];
		  add_value (room, exit);
		}
	      if (y < MAPGEN_MAXY - 1 && new_map[x][y + 1]) /* South exit. */
		{
		  exit = new_value();
		  exit->type = DIR_SOUTH + 1;
		  exit->val[0] = new_map[x][y + 1];
		  add_value (room, exit);
		}
	    }
	}
    }
  return "Ok, new map generated.\n\r";
}

  
/* This is used to find the largest connected component attached to the
   middle pathway running through the map. */


void
mapgen_connection (short x, short y)
{
  /* Check for out of bounds values, and make sure this space has
     been used, but not searched. */
  
  if (x < 0 || y < 0 || x >= MAPGEN_MAXX * 2 || y >= MAPGEN_MAXY * 2 ||
      mapgen_used[x][y] != MAPGEN_USED)
    return;

  mapgen_used[x][y] = MAPGEN_SEARCHED;

  /* Search the adjacent nodes. */

  mapgen_connection (x - 1, y);
  mapgen_connection (x + 1, y);
  mapgen_connection (x, y - 1);
  mapgen_connection (x, y + 1);
  return;
}


/* This generates the caverns of modnar on reboot. This has been
   disabled. Add this to the list in boot.c after the areas are 
   loaded in and it will be put back in. */



void
generate_modnar (void)
{
  THING *area;
  THING *start_room, *room;
  MAPGEN *map = NULL;
  char descstr[STD_LEN];
  int i, times = 0, start_vnum = 0; /* Some counters and the start room vnum. */
  int type; /* The type of room set up here. */
  VALUE *exit;
  char *shortdesc = NULL; /* The short desc that will be given to the genned rooms. */
  int flagval;
  /* No area set up. */
  if ((area = find_thing_num (MODNAR_AREA_VNUM)) == NULL ||
      area->mv < MODNAR_MAX_SIZE ||
      (start_room = find_thing_num (MODNAR_AREA_VNUM+1)) == NULL)
    {
      log_it ("Start area 120000 or start room not available for the caverns of modnar.\n");
      return;
    }
  
  /* Description string put into all of the rooms. */
  sprintf (descstr, "%d", MODNAR_AREA_VNUM+1);
  
  /* Make sure no rooms exist in this range. */
  
  for (i = MODNAR_AREA_VNUM+2; i < MODNAR_AREA_VNUM+1+MODNAR_MAX_SIZE; i++)
    {
      if ((room = find_thing_num (i)) != NULL)
	{
	  log_it ("Could not create modnar because a room exists.\n");
	  return;
	}
    }

  /* Try to make a map up to 20 times. */
  
  while (times++ < 20 &&
	 ((map = mapgen_generate ("1 0 30 30 0 8 13 0")) == NULL ||
	  map->num_rooms < MODNAR_MAX_SIZE/2 || map->num_rooms >=  MODNAR_MAX_SIZE))
    {
      free_mapgen(map);
      map = NULL;
    }
  
  
  
  /* Hope the map will exist. */
  
  if (!map)
    {
      log_it ("Failed to create the caverns of modnar after repeated tries.\n");
      return;
    }
  
  mapgen_create (map, 120002);
  
  
  room = NULL; /* Find entrance room. */
  while (room == NULL && ++times < 100)
    {
      start_vnum = nr (MODNAR_AREA_VNUM+2, MODNAR_AREA_VNUM + map->num_rooms);
      if ((room = find_thing_num (start_vnum)) != NULL)
	break;
    }
  
  if (!room)
    {
      free_mapgen (map);
      log_it ("Failed to create Modnar!\n");
      return;
    }
  if ((exit = FNV (start_room, VAL_EXIT_D)) == NULL)
    {
      exit = new_value();
      exit->type = VAL_EXIT_D;
      add_value (start_room, exit);
    }  
  exit->val[0] = start_vnum;
  
  if ((exit = FNV (room, VAL_EXIT_U)) == NULL)
    {
      exit = new_value();
      exit->type = VAL_EXIT_U;
      add_value (room, exit);
    }  
  exit->val[0] = start_room->vnum;
  
  /* Now make all of the new rooms unsaveable, and make them underground.
     Later on, I will add fun stuff like underwater rooms and fire rooms
     and earth rooms and all that to make things worse for the players. */
  
  for (i = MODNAR_AREA_VNUM + 2; i < MODNAR_AREA_VNUM+MODNAR_MAX_SIZE; i++)
    {
      if ((room = find_thing_num (i)) != NULL && room->in == area
	  && (flagval = flagbits (room->flags, FLAG_ROOM1)) == 0)
	{	
	  /* Pick what type of room this will be then spread the
	     type out from this room in all directions. */
	  type = nr (0,5);
	  switch (type)
	    {
	      case 0:
		shortdesc = "The Forests of Modnar";
		flagval = ROOM_FOREST;
		break;
	      case 1:
		shortdesc = "The Fields of Modnar";
		flagval = ROOM_FIELD;
		break;
	      case 2:
		shortdesc = "The Deserts of Modnar";
		flagval = ROOM_DESERT;
		break;
	      case 3:
		shortdesc = "The Swamps of Modnar";
		flagval = ROOM_SWAMP;
		break;
	      case 4:
		shortdesc = "The Hills of Modnar";
		flagval = ROOM_ROUGH;
		break;
	      case 5:
		shortdesc = "The Caves of Modnar";
		flagval = ROOM_UNDERGROUND;
		break;
	    }
	  set_up_modnar_room (room, flagval, shortdesc, 0);
	  room->desc = new_str (descstr);
	  room->thing_flags |= TH_NUKE;
	}
    }
  write_area (area);
  free_mapgen (map);  
  return;
}


/* This function sets up a modnar room to a certain type with a certain
   short description and then checks adjacent rooms that haven't been
   fixed up yet and fixes them up, also. It only goes until the depth
   or distance moved from the first location is at least 10 rooms. */

void
set_up_modnar_room (THING *room, int flagval, char *shortdesc, int depth)
{
  int dir, curr_flagval;
  THING *nroom;
  VALUE *exit;
  
  if (!room || !room->in || room->in->vnum != MODNAR_AREA_VNUM)
    return;
  
  /* Only go out 6-12 spaces. */
  if (depth > nr (3,7))
    return;
  
  if ((curr_flagval = flagbits (room->flags, FLAG_ROOM1)) == 0)
    {
      add_flagval (room, FLAG_ROOM1, flagval);
      room->short_desc = new_str (shortdesc);
      room->thing_flags |= TH_NUKE;
    } 
  /* If we must pass over a different kind of terrain, don't check adjacent */
  /* rooms (to avoid little tiny blocks of terrain). */
  else 
    return;

    /*  Check and set up all adjacent rooms. */
  
  for (dir = 0; dir < FLATDIR_MAX; dir++)
    {
      if ((exit = FNV (room, ((dir + depth) % 4) + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL)
	{
	  set_up_modnar_room (nroom, flagval, shortdesc, depth + 1);
	}
    }
  return;
}

/* This OR's two mapgens into a combined mapgen. It allows you to
   either have them perfectly centered, or offset, depending
   on what you feel like. */

MAPGEN *
mapgen_combine (MAPGEN *map1, MAPGEN *map2, int offset_x, int offset_y)
{
  MAPGEN *map;
  int x, y, times = 0;

  /* Starting x and y shifts based only on the maps, not the
     offsets. */
  int start_shift_x, start_shift_y;
  
  /* Shifts between the two maps to line up their centers. */
  int shift_x, shift_y, i;
  /* Extremes */
  int ext1[REALDIR_MAX];
  int ext2[REALDIR_MAX];

  /* Tell if we used the first room, the second room or both since
     we must be sure that the rooms overlap by making the offsets
     small enough. */
  
  bool used_first_room, used_second_room, rooms_overlap;

  if (!map1)
    {
      if (map2)
	return map2;
      else
	return NULL;
    }
  if (!map2)
    {
      if (map1)
	return map1;
      else
	return NULL;
    }
  
  ext1[DIR_NORTH] = 0;
  ext2[DIR_NORTH] = 0;
  ext1[DIR_SOUTH] = MAPGEN_MAXY;
  ext2[DIR_SOUTH] = MAPGEN_MAXY;
  ext1[DIR_EAST] = 0;
  ext2[DIR_EAST] = 0;
  ext1[DIR_WEST] = MAPGEN_MAXX;
  ext2[DIR_WEST] = MAPGEN_MAXX;
  
  for (x = 0; x < MAPGEN_MAXX; x++)
    {
      for (y = 0; y < MAPGEN_MAXY; y++)
	{
	  if (map1->used[x][y])
	    {	      
	      if (x < ext1[DIR_WEST])
		ext1[DIR_WEST] = x;
	      if (x > ext1[DIR_EAST])
		ext1[DIR_EAST] = x;
	      if (y < ext1[DIR_SOUTH])
		ext1[DIR_SOUTH] = y;
	      if (y > ext1[DIR_NORTH])
		ext1[DIR_NORTH] = y;
	    }
	}
    }
  
  for (x = 0; x < MAPGEN_MAXX; x++)
    {
      for (y = 0; y < MAPGEN_MAXY; y++)
	{
	  if (map1->used[x][y])
	    {	      
	      if (x < ext2[DIR_WEST])
		ext2[DIR_WEST] = x;
	      if (x > ext2[DIR_EAST])
		ext2[DIR_EAST] = x;
	      if (y < ext2[DIR_SOUTH])
		ext2[DIR_SOUTH] = y;
	      if (y > ext2[DIR_NORTH])
		ext2[DIR_NORTH] = y;
	    }
	}
    }
  
  /* Now find the shifts. */

  start_shift_x = ((ext1[DIR_WEST]+ext1[DIR_EAST])/2-
	     (ext2[DIR_WEST]+ext2[DIR_EAST])/2)/2;
  start_shift_y = ((ext1[DIR_SOUTH]+ext1[DIR_NORTH])/2-
	     (ext2[DIR_SOUTH]+ext2[DIR_NORTH])/2)/2;
  start_shift_x = -start_shift_x;
  start_shift_y = -start_shift_y; 
  
  do
    {
      /* The reason the mapgen creation and shifts are within the
	 loop is if the two maps are shifted too much and miss
	 each other, then the map is tried again with less of a
	 shift until it works. */
      rooms_overlap = FALSE;
      map = new_mapgen();
      map->num_rooms = 0;
      shift_x = start_shift_x + offset_x;
      shift_y = start_shift_y + offset_y;
      
      for (x = 0; x < MAPGEN_MAXX; x++)
	{
	  for (y = 0; y < MAPGEN_MAXY; y++)
	    {
	      used_first_room = FALSE;
	      used_second_room = FALSE;
	      
	      if (x + shift_x < MAPGEN_MAXX &&
		  x + shift_x >= 0 &&
		  y + shift_y < MAPGEN_MAXY &&
		  y + shift_y >= 0 &&
		  map1->used[x+shift_x][y+shift_y])
		{
		  used_first_room = TRUE;
		  map->used[x][y] |= map1->used[x+shift_x][y+shift_y];
		}
	      
	      if (x - shift_x < MAPGEN_MAXX &&
		  x - shift_x >= 0 &&
		  y - shift_y < MAPGEN_MAXY &&
		  y - shift_y >= 0 &&
		  map2->used[x-shift_x][y-shift_y])
		{	      
		  map->used[x][y] |= map2->used[x-shift_x][y-shift_y];
		  used_second_room = TRUE;
		}
	      
	      if (used_first_room && used_second_room)
		rooms_overlap = TRUE;
	      
	      if (map->used[x][y])
		map->num_rooms++;
	    }
	}
      if (!rooms_overlap)
	{
	  free_mapgen(map);
	  offset_x = offset_x*9/10;
	  offset_y = offset_y*9/10;
	}
    }
  while (!rooms_overlap && ++times < 100);
  
  if (!rooms_overlap)
    {
      free_mapgen (map);
      return NULL;
    }

  /* Now poke holes in the map in certain places. */
  
  for (x = 1; x < MAPGEN_MAXX-1; x++)
    {
      for (y = 1; y < MAPGEN_MAXY-1; y++)
	{
	  for (i = 1; i < 10; i++)
	    {
	      if (!map->used[x-1+(i/3)][y-1+(i%3)])
		break;
	    }
	  if (i == 10 && nr (1,5) == 3)
	    map->used[x][y] = 0;
	}
    }
  
  /* Now clean up these other maps. */
  free_mapgen (map1);
  free_mapgen (map2);
  
  
  return map;
}

