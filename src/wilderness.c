#include<stdlib.h>
#include<stdio.h>
#include<ctype.h>
#include "serv.h"
#include "wilderness.h"
#include "event.h"
#include "wildalife.h"

THING *wilderness_area;
char wilderness[WILDERNESS_SIZE][WILDERNESS_SIZE];

/* This sets up the wilderness area if need be. */

void
setup_wilderness_area (void)
{
  int i, j;
  THING *base_wild_room = NULL;
  FILE *f;
  char c; 
  
  if ((f = wfopen ("wilderness.dat", "r")) == NULL)
    {
      for (i = 0; i < WILDERNESS_SIZE; i++)
	{
	  for (j = 0; j < WILDERNESS_SIZE; j++)
	    {
	      wilderness[i][j] = nr (PIXMAP_DEFAULT, PIXMAP_UNDERGROUND);
	    }
	}
    }
  else
    {
      for (i = WILDERNESS_SIZE -1; i >= 0; i--)
	{
	  for (j = 0; j < WILDERNESS_SIZE; j++)
	    {
	      c = getc (f);
	      wilderness[j][i] = c - 'a';
	    }
	}
      fclose (f);
    }
  
  if ((wilderness_area = find_thing_num (WILDERNESS_AREA_VNUM)) != NULL &&
      (base_wild_room = find_thing_num (WILDERNESS_FIELD_PROTO)) != NULL)
    return;
  if (wilderness_area == NULL)
    {
      wilderness_area = new_thing();
      wilderness_area->vnum = WILDERNESS_AREA_VNUM;
      thing_to (wilderness_area, the_world);
      add_thing_to_list (wilderness_area);
      wilderness_area->thing_flags = TH_IS_AREA | TH_CHANGED;
      wilderness_area->name = new_str ("wildarea.are");
      wilderness_area->short_desc = new_str ("The great wilderness.");
      wilderness_area->long_desc = new_str ("Mists overtake the land.");
      /* These are not correct. But I don't want to use 32 bit ints for these. */
      wilderness_area->max_mv = 10000;
      wilderness_area->mv = 9000;
      
    }
  if (base_wild_room == NULL)
    {
      base_wild_room = new_thing();
      base_wild_room->vnum = WILDERNESS_FIELD_PROTO;
      base_wild_room->short_desc = new_str ("A Giant Field");
      base_wild_room->thing_flags = ROOM_SETUP_FLAGS;	
      thing_to (base_wild_room, find_thing_num (1));
      add_thing_to_list (base_wild_room);
    }
}


void
write_wilderness (void)
{
  int i, j;
  FILE *f;
  if ((f = wfopen ("wilderness.dat", "w")) == NULL)
    {
      perror ("wilderness.dat");
      return;
    }

  for (i = 0; i < WILDERNESS_SIZE; i++)
    {
      for (j = 0; j < WILDERNESS_SIZE; j++)
	{
	  putc ((wilderness[i][j] + 'a'), f);
	}
    }
  fclose (f);
}

/* This updates a wilderness room to see if it needs a timer or needs
   to be destroyed. */

bool
update_wilderness_room (THING *room)
{
  THING *mob, *proto;
  bool player_here = FALSE;
  
  if (!room || !IS_WILDERNESS_ROOM(room))
    return FALSE;
  
  /* See if any players are here. */
  
  for (mob = room->cont; mob; mob = mob->next_cont)
    if (IS_PC (mob))
      player_here = TRUE;
  

  /* If a player is here, reset all of the timers of the things in the
     room and don't delete things. */
  
  if (player_here)
    {
      for (mob = room->cont; mob; mob = mob->next_cont)
	{
	  if (mob->timer > 0 &&
	      (proto = find_thing_num (mob->vnum)) != NULL &&
	      proto->timer == 0)
	    mob->timer = 0;
	}
      return FALSE;
    }
  
  /* No players here. Set all objects to decay in 2 hours. */
  
  for (mob = room->cont; mob; mob = mob->next_cont)
    {      
      /* Everything except corpses rot quickly. */
      if (mob->timer != 1 && mob->vnum != CORPSE_VNUM &&
	  mob->hp == mob->max_hp)
	mob->timer = 2;
    }
  
  /* If the room has contents or tracks don't get rid of it. */
  
  if (room->cont || room->tracks || !IS_SET (room->thing_flags, TH_NUKE))
    return FALSE;

  
  
  /* If it has no timer, and is empty, nuke it. */
  
  free_thing (room);
  return TRUE;
}

/* This will make a wilderness map eventually once there's something
   to show. */

void
create_wilderness_map (THING *th, THING *in, int maxx, int maxy)
{
  int min_x, max_x, min_y, max_y;
  int start_x, start_y;
  int new_vnum;
  char mapbuf[STD_LEN*5];
  char *t;
  int ycount, xcount;
  int curr_color = 7;
  int sector, color;
  if (!in || !IS_WILDERNESS_ROOM(in) || !th || !th->fd)
    return;

  new_vnum = in->vnum - WILDERNESS_MIN_VNUM;
  
  start_x = new_vnum % WILDERNESS_SIZE;
  start_y = new_vnum / WILDERNESS_SIZE;

  min_x = MAX(0, start_x - maxx/2);
  max_x = MIN(WILDERNESS_SIZE-1, start_x + maxx/2);

  min_y = MAX(0, start_y - maxy/2);
  max_y = MIN(WILDERNESS_SIZE-1, start_y+maxy/2);
  
  for (ycount = max_y; ycount >= min_y; ycount--)
    {
      t = mapbuf;
      for (xcount = min_x; xcount <= max_x; xcount++)
	{	  
	  if (xcount == start_x && ycount == start_y)
	    {
	      if (curr_color != 7)
		{
		  *t++ = '\x1b'; 
		  *t++ = '[';
		  *t++ = '0';
		  *t++ = ';';
		  *t++ = '3';
		  *t++ = '7';
		  *t++ = 'm';
		  curr_color = 7;
		}
	      *t++ = '@';
	    }
	  else
	    {
	      /* Find the sector type and set to default if error. */
	      sector = wilderness[xcount][ycount];
	      if (sector < 0 || sector >= PIXMAP_MAX)
		sector = 0;
	      /* Get the color of this sector. If it different than the
		 current color, change the color. */
	      color = pixmap_symbols[sector][0]*8 + 
		pixmap_symbols[sector][1];
	      if (curr_color != color)
		{
		  *t++ = '\x1b'; 
		  *t++ = '[';
		  *t++ = pixmap_symbols[sector][0];
		  *t++ = ';';
		  *t++ = '3';
		  *t++ = pixmap_symbols[sector][1];;
		  *t++ = 'm';
		  curr_color = color;
		}
	      *t++ = pixmap_symbols[sector][2];		  
	    }
	}
      *t++ = '\n';
      *t++ = '\r';
      *t = '\0';      
      stt (mapbuf, th);
    }
  stt ("\x1b[0;37m", th);
  return;
}


/* This makes a wilderness room if none exists. */


THING *
make_wilderness_room (int vnum)
{
  THING *room;
  int new_vnum;
  VALUE *exit;
  
  int x, y;
  /* Check if the vnum is in the appropriate range. */
  
  if (vnum < WILDERNESS_MIN_VNUM || vnum >= WILDERNESS_MAX_VNUM
      || !wilderness_area)
    return NULL;
 new_vnum = vnum - WILDERNESS_MIN_VNUM;
  /* Should be ok based on previous checks...*/
  
  x = new_vnum % WILDERNESS_SIZE;
  y = new_vnum / WILDERNESS_SIZE;
  
  room = new_thing();
  room->thing_flags = ROOM_SETUP_FLAGS | TH_NUKE;
  room->proto = find_thing_num (WILDERNESS_FIELD_PROTO +
				wilderness[x][y]);
  room->vnum = vnum;
  thing_to (room, wilderness_area);
  add_thing_to_list (room);
  if (room->proto)
    add_flagval (room, FLAG_ROOM1, flagbits (room->proto->flags, FLAG_ROOM1));
  

  /* Set up exits. */
  if (y > 0)
    {
      exit = new_value();
      exit->type = DIR_SOUTH + 1;
      exit->val[0] = vnum - WILDERNESS_SIZE;
      add_value (room, exit);
    }
  if (y < WILDERNESS_SIZE - 1)
    {
      exit = new_value();
      exit->type = DIR_NORTH + 1;
      exit->val[0] = vnum + WILDERNESS_SIZE;
      add_value (room, exit);
    }
  
  if (x > 0)
    {
      exit = new_value();
      exit->type = DIR_WEST + 1;
      exit->val[0] = vnum - 1;
      add_value (room, exit);
    }
  
  if (x < WILDERNESS_SIZE - 1)
    {
      exit = new_value();
      exit->type = DIR_EAST + 1;
      exit->val[0] = vnum + 1;
      add_value (room, exit);
    }
  add_events_to_thing (room);
  return room;
}
  
  
  





  










