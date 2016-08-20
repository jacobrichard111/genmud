#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "track.h"
#ifdef USE_WILDERNESS
#include "wilderness.h"
#include "wildalife.h"
#endif



/* These are a list of pixmap symbols that are used to show the wilderness
   map and the pixmap map. The first character is the color (0-15) 
   and the second character is the symbol. */


/* The first number is whether or not this is displayed as a bright
   ANSI color code (0 or 1) and the second is what the code is (0-7). 
   The third character is the actual symbol shown. 

   Make sure that these match the PIXMAP_XXXX constants in serv.h */
const char pixmap_symbols[PIXMAP_MAX][4] =
  {
    "07+",
    "07+",
    "14~",
    "02#",
    "12,",
    "02*",
    "05^",
    "17&",
    "13:",
    "06+",
    "16@",
    "11^",
    "07=",
    "10\"",
  };



/* This sets up the symbols for mapping...very rudimentary so far. */
/* The only reason it is all smooshed together is because I wanted
   to see what it looked like all on one page :) So, if you really want
   to find out how it works...just add a bunch of  { } :) */


void
set_up_map (THING *area)
{
  THING *ar, *arn, *room;
  if (area)
    ar = area;
  else
    ar = the_world->cont;
  for (; ar; ar = arn)
    {
      if (area)
	arn = NULL;
      else
	arn = ar->next_cont;
#ifdef USE_WILDERNESS
      if (ar == wilderness_area)
	continue;
#endif
      for (room = ar->cont; room; room = room->next_cont)
	{
	  set_up_map_room (room);
	}
    }
  return;
}

void
set_up_map_room (THING *room)
{
  THING *otherroom;
  int exits, i, flags;
  if (!room || !IS_ROOM (room))
    return;
  
  exits = 0;
  flags = flagbits (room->flags, FLAG_ROOM1);
  set_up_move_flags (room);
  room->exits = 0;
  room->goodroom_exits = 0;
  room->move_flags = (flagbits (room->flags, FLAG_ROOM1) & BADROOM_BITS);
  for (i = 0; i < REALDIR_MAX; i++)
    {
      /* Check if the exit in that dir exists, and if it
	 is not hidden/wall, and if it leads to something
	 and if that something is another room. */
      
      if ((otherroom = map_dir_room (room, i)) != NULL)
	SBIT (exits, (1 << i));
      /* This is bad. But I need tracking to override mapping. */
      if ((otherroom = find_track_room (room, i, BADROOM_BITS)) != NULL)
	{
	  room->exits |= (1 << i);
	  room->adj_room[i] = otherroom;
	  if ((otherroom = find_track_room (room, i, 0)) != NULL)
	    {
	      room->goodroom_exits |= (1 << i);
	      room->adj_goodroom[i] = otherroom;
	    }
	  else
	    room->adj_goodroom[i] = NULL;
	}      
      else
	room->adj_room[i] = NULL;
      
    }

  /* Maybe add some stuff in here about other symbols for mountains
     and forests etc. */
  /* First do rooms where color AND symbol are set */
  
  room->symbol = '\0';
  if (IS_SET (flags, ROOM_EARTHY))
    {
      room->color = 11;
      room->symbol = '&'; 
      room->kde_pixmap = PIXMAP_DEFAULT;
    }
  else if (IS_SET (flags, ROOM_WATERY))
    {
      room->color = 12;
      room->symbol = '#';   
      room->kde_pixmap = PIXMAP_WATER;
    }
  else if (IS_SET (flags, ROOM_UNDERWATER))
    {
      room->color = 4;
      room->symbol = '#';     
      room->kde_pixmap = PIXMAP_WATER;
    }
  else if (IS_SET (flags, ROOM_FIERY))
    {
      room->color = 9;
      room->symbol = '^';  
      room->kde_pixmap = PIXMAP_FIRE; 
    }
  else if (IS_SET (flags, ROOM_FOREST))
    {
      room->color = 2;
      room->symbol = '*';   
      room->kde_pixmap = PIXMAP_FOREST;
    }
  else if (IS_SET (flags, ROOM_DESERT))
    {
      room->color = 11;
      room->symbol = ',';
      room->kde_pixmap = PIXMAP_DESERT;
    } /* Now set colors */
  else if (IS_SET (flags, ROOM_SWAMP))
    {
      room->color = 2;
      room->symbol = '#';
      room->kde_pixmap = PIXMAP_SWAMP;
    } /* Now set colors */
  else if (IS_SET (flags, ROOM_FIELD))
    {
      room->color = 10;
      room->symbol = ',';
      room->kde_pixmap = PIXMAP_FIELD;
    } /* Now set colors */
  else if (IS_SET (flags, ROOM_SNOW))
    {
      room->color = 15;
      room->symbol = '*';
      room->kde_pixmap = PIXMAP_SNOW;
    }
  else if (IS_SET (flags, ROOM_ASTRAL))
    {
      room->color = 9;
      room->symbol = '@';
      room->kde_pixmap = PIXMAP_DEFAULT;
    }
  else if (IS_SET (flags, ROOM_AIRY))
    {
      room->color = 14;
      room->kde_pixmap = PIXMAP_AIR; 
    }		 
  else if (IS_SET (flags, ROOM_ROUGH | ROOM_MOUNTAIN))
    {
      room->color = 5;
      room->symbol = '\0';
      room->kde_pixmap = PIXMAP_MOUNTAIN;
    }
  else if (IS_SET (flags, ROOM_EASYMOVE))
    {
      room->color = 6;
      room->kde_pixmap = PIXMAP_ROAD;
    }
  else if (IS_SET (flags, ROOM_INSIDE | ROOM_UNDERGROUND))
    {
      room->color = 8;
      if (IS_SET (flags, ROOM_UNDERGROUND))
	room->kde_pixmap = PIXMAP_UNDERGROUND;
      else
	room->kde_pixmap = PIXMAP_INSIDE;
    }
  else
    {
      room->color = 7;
      room->kde_pixmap = PIXMAP_DEFAULT;
    }

  /* Set up the "road" symbols and the ascii symbols
     for those with upper ascii displays. */

  if (IS_SET (exits, (1 << DIR_NORTH)))
    {
      if (IS_SET (exits, (1 << DIR_SOUTH)))
	{
	  if (IS_SET (exits, (1 << DIR_EAST)))
	    {
	      if (IS_SET (exits, (1 << DIR_WEST)))
		{
		  if (room->symbol == '\0')
		    room->symbol = '+'; /* nsew */
		  room->ascii_symbol = 197;
		}
	      else
		{
		  if (room->symbol == '\0')
		    room->symbol = '}'; /* nse */
		  room->ascii_symbol = 195;
		}
	    }
	  else if (IS_SET (exits, (1 << DIR_WEST)))
	    { 
	      if (room->symbol == '\0')
		room->symbol = '{'; /* nsw */
	      room->ascii_symbol = 180;
	    }		      
	  else
	    { 
	      if (room->symbol == '\0')
		room->symbol = '|'; /* ns */
	      room->ascii_symbol = 179;
	    }
	}
      else if (IS_SET (exits, (1 << DIR_EAST)))
	{
	  if (IS_SET (exits, (1 << DIR_WEST)))
	    { 
	      if (room->symbol == '\0')
		room->symbol = '+'; /* new */
	      room->ascii_symbol = 193;
	    }
	  else
	    { 
	      if (room->symbol == '\0')
		room->symbol = '\\'; /* ne */
	      room->ascii_symbol = 192;
	    }
	}
      else if (IS_SET (exits, (1 << DIR_WEST)))
	{ 
	  if (room->symbol == '\0')
	    room->symbol = '/'; /* nw */
	  room->ascii_symbol = 217;
	}
      else
	{
	  if (room->symbol == '\0')
	    room->symbol = '|'; /* n */
	  room->ascii_symbol = 179;
	}
    }
  else if (IS_SET (exits, (1 << DIR_SOUTH)))
    {
      if (IS_SET (exits, (1 << DIR_EAST)))
	{
	  if (IS_SET (exits, (1 << DIR_WEST)))
	    {
	      if (room->symbol == '\0')
		room->symbol = '+'; /* sew */
	      room->ascii_symbol = 194;
	    }
	  else
	    { 
	      if (room->symbol == '\0')
		room->symbol = '/'; /* se */
	      room->ascii_symbol = 218;
	    }
	}
      else if (IS_SET (exits, (1 << DIR_WEST)))
	{ 
	  if (room->symbol == '\0')
	    room->symbol = '\\'; /* sw */
	  room->ascii_symbol = 191;
	}
      else
	{ 
	  if (room->symbol == '\0')
	    room->symbol = '|'; /* s */
	  room->ascii_symbol = 179;
	}
    }
  else
    { 
      if (room->symbol == '\0')
	room->symbol = '-'; /* ew e w or none */
      room->ascii_symbol = 196;
    }
  
return;
}

/* Sets color to gray, clears screen, makes whole screen scroll again. */

void
do_cls (THING *th, char *arg)
{
  char buf[50];
  if (!IS_PC (th) || !th->fd || USING_KDE (th))
    return;
  sprintf (buf, "\x1b[0;37m\x1b[2j\x1b[1;%dr\x1b[%d;1f",
   th->pc->pagelen, th->pc->pagelen);
  stt (buf, th);
  return;
}


/* This checks if a map can be drawn in a certain direction,
   i.e. if there is an exit, the exit is not hidden or a wall, there
   is something that the exit leads to, and this something is a room. */

THING *
map_dir_room (THING *start_in, int dir)
{
  THING *room;
  VALUE *exit;
  if (dir >= 0 && dir < REALDIR_MAX &&
      (exit = FNV (start_in, dir + 1)) != NULL &&
      !IS_SET (exit->val[2], EX_HIDDEN | EX_WALL) &&
      (room = find_thing_num (exit->val[0])) != NULL &&
      IS_ROOM (room) && room != start_in &&
      !IS_SET (room->thing_flags, TH_MARKED))
    return room;
  return NULL;
}


void
do_map (THING *th, char *arg)
{
  if (!th || !th->in || !IS_PC (th))
    return;
  
  undo_marked (th->in);
  if (str_cmp (arg, "large"))
    create_map (th, th->in, 75, th->pc->pagelen);
  else
    create_map (th, th->in, MAP_MAXX, MAP_MAXY);
  /* sprintf (buf, "\x1b[1;%dr\x1b[%d;1f", IS_PC (th) ? th->pc->pagelen : 24,
     IS_PC (th) ? th->pc->pagelen : 24);
     stt (buf, th); */
  return;
}

void
create_map (THING *th, THING *room, int maxx, int maxy)
{
  int i, j;
  VALUE *exit;
  THING *uproom;
  
  if (!th || !IS_PC (th) || !room || !IS_ROOM (room))
    return;

  /* If this is a nomap room (under a bridge) and there's a 
     mappable room above it, start the map there. */

  if (IS_ROOM_SET (room, ROOM_NOMAP) &&
      (exit = FNV (room, DIR_UP + 1)) != NULL &&
      (uproom = find_thing_num (exit->val[0])) != NULL)
    room = uproom;

#ifdef USE_WILDERNESS
  if (IS_WILDERNESS_ROOM (room))
    {
      create_wilderness_map (th, room, maxx, maxy);
      return;
    }
#endif
  if (maxx == SMAP_MAXX && maxy == SMAP_MAXY &&
      !IS_PC2_SET (th, PC2_MAPPING))
    return;
  
  maxx = MIN (maxx, MAP_MAXX);
  maxy = MIN (maxy, MAP_MAXY);
  
  /* Clear map buffer. */
  
  for (i = 0; i < maxx; i++)
    {
      for (j = 0; j < maxy; j++)
	{	  
	  map[i][j] = '\0';
	  col[i][j] = 0;
	  exi[i][j] = 0;
	  pix[i][j] = 0;
	}
    }
  
  /* Start the drawing process. This is recursive and should fill up
     the map to the edges. */
  
  draw_room (room, maxx/2, (maxy - 1)/2, maxx, maxy,
	     (IS_PC(th) && IS_PC2_SET (th, PC2_ASCIIMAP) ? TRUE : FALSE));
  map[maxx/2][(maxy - 1)/2] = '@';
  col[maxx/2][(maxy - 1)/2] = 7;
  undo_marked (room);
  
  show_map (th, maxx, maxy);
  
  return;
}

/* Draws the map recursively. */

  
void 
draw_room (THING *room, int x, int y, int maxx, int maxy, bool use_ascii)
{
  int dir, i, sdir;
  THING *newroom;
  VALUE *build;
  bool found_caste = FALSE;
  int randjump = nr (0,3);
  if (!room || !IS_ROOM (room) || x < 0 || x >= maxx ||
      y < 0 || y >= maxy || IS_SET (room->thing_flags, TH_MARKED) ||
      IS_ROOM_SET (room, ROOM_NOMAP))
    return;
  
  SBIT (room->thing_flags, TH_MARKED);
  
  
  if ((build = FNV (room, VAL_BUILD)) != NULL &&
      build->val[3] != 0 && build->val[1] > 0)
    {
      for (i = 0; caste1_flags[i].flagval != 0; i++)
	{
	  if (IS_SET (build->val[3], caste1_flags[i].flagval))
	    {
	      map[x][y] = caste1_flags[i].help[0];
	      found_caste = TRUE;
	      break;
	    }
	}
    }
  if (!found_caste)
    {
      if (use_ascii)
	map[x][y] = room->ascii_symbol;
      else
	map[x][y] = (room->symbol ? room->symbol : '+');
    }
  col[x][y] = (room->color >= 0 && room->color < 16 ? room->color : 0);
  exi[x][y] = room->exits;
  pix[x][y] = room->kde_pixmap;
  
  for (sdir = 0; sdir < FLATDIR_MAX; sdir++)
    {
      dir = (sdir + randjump) % 4;
      if ((newroom = map_dir_room (room, dir % 4)) != NULL &&
	  newroom != room)
	{ /* This is complicated, but it works. */
	  draw_room (newroom, x + (5 - 2*dir)*(dir > 1), 
		     y + (2*dir - 1)*(dir < 2), maxx, maxy, use_ascii);
	}
    }
  return;
}

/* Recursively unset the marked flags on all of the rooms adjacent to
   the starting room. If it's an area, it unmarks all of its contents.  */

void
undo_marked (THING *th)
{
  VALUE *exit;
  int dir;
  THING *room;
  THING *newroom;
  
  if (!th)
    return;
  
  if (IS_AREA (th))
    {
      for (room = th->cont; room; room = room->next_cont)
	undo_marked (room);
      return;
    }
  if (!IS_SET (th->thing_flags, TH_MARKED))
    return;
  RBIT (th->thing_flags, TH_MARKED);
  for (dir = 0; dir < REALDIR_MAX; dir++)
    if ((exit = FNV (th, dir + 1)) != NULL &&
	(newroom = find_thing_num (exit->val[0])) != NULL)
      undo_marked (newroom);
  return;
}




void
show_map (THING *th, int maxx, int maxy)
{
  int i, j, curr_color = 7;
  char buf[3*STD_LEN];
  char buf2[STD_LEN];
  char *t;
  /* If this is a kde map update, we send them the special map...*/
  
  /* We send the map color + 'a'. We then send the exitdata + 'a'. */
  
  if (USING_KDE (th) && maxx == SMAP_MAXX && maxy == SMAP_MAXY)
    {
      sprintf (buf, "<MAP>");
      t = buf + strlen(buf);      
      for (j = 0; j < maxy; j++)
	{
	  for (i = 0; i < maxx; i++)
	    {
	      *t++ = pix[i][j] + 'a';
	      /* Now maybe add exits info. Only worry about S and E exits. */
	      if (i < maxx - 1 && pix[i+1][j] > 0 &&
		       !IS_SET (exi[i][j], (1 << DIR_EAST)))
		{
		  *t++ = 'E';
		  *t++ = exi[i][j] + 'a';
		}	     
	      else if (j < maxy - 1 && pix[i][j + 1] > 0 &&
		       !IS_SET (exi[i][j], (1 << DIR_SOUTH)))
		{
		  *t++ = 'E';
		  *t++ = exi[i][j] + 'a';
		}	
	    }
	}
      sprintf (t, " </MAP>");
      stt (buf, th);
      return;
    }
  sprintf (buf, "\x1b[%d;%dr", 
	   MIN(maxy  + 1, th->pc->pagelen), th->pc->pagelen);
  stt (buf, th);
  for (i = 0; i < maxy; i++)
    {
      sprintf (buf, "\x1b[%d;1f\x1b[K", i + 1);
      t = buf + strlen (buf);
      for (j = 0; j < maxx; j++)
	{
	  if (col[j][i] != curr_color && col[j][i] > 0)
	    {
	      curr_color = col[j][i];
	      sprintf (buf2, "\x1b[%d;3%dm%c", curr_color/8, curr_color % 8, 
		       map[j][i] ? map[j][i] : ' ');
	      sprintf (t, buf2);
	      t += strlen (buf2);
	    }
	  else
	    *t++ = map[j][i] ? map[j][i] : ' ';
	}
      *t++ = '\n';
      *t++ = '\r';
      *t = '\0';
      stt (buf, th);
    }
  sprintf (buf, "\x1b[0;37m\x1b[%d;1f", th->pc->pagelen);
  stt (buf, th);
  return;
}











