#include "stdio.h"
#include "serv.h"
#include "track.h"
#include "plasmgen.h"

/* This lets you export an area map to a plasm file. */


void
plasmgen_export (THING *area)
{
  FILE *f;
  int i;
  THING *room;
  char buf[STD_LEN];
  int num_cubes = 0;
  int dir;
  VALUE *exit, *coord;
  THING *nroom;
  int dx = 0, dy = 0, dz = 0;
  int color;
  
  const char *colors[PLASMGEN_COLOR_MAX] = 
    {
      "gray",
      "red",
      "green",
      "yellow",
      "blue",
      "purple",
      "cyan",
      "white",
      "brown",
    };

  return;
  if (!area)
    return;
  
  if (area == the_world->cont)
    {
      if (the_world->cont->next_cont)
	room = the_world->cont->next_cont->cont;
      else
	room = NULL;
      undo_marked (the_world);
    }
  else
    {
      room = area->cont;
      undo_marked (area);
    }
	

  if (!room || !IS_ROOM (room))
    return;
  
  if ((f = fopen ("../../plasm/world.psm", "w")) == NULL)
    {
      return;
    }

  
  /* Set up the 8 different colored cubes we will use. */
  
  for (i = 0; i < PLASMGEN_COLOR_MAX; i++)
    {
      sprintf (buf, "DEF cube%s = CUBOID:<1,1,1> COLOR %s;\n",
	       colors[i], colors[i]);
      fprintf (f, buf);
    }
  
  
  /* Set up the city structure. */
  
  fprintf (f, "DEF world = STRUCT:< ");

  clear_bfs_list ();

  add_plasm_bfs (room, 0, 0, 0);
  bfs_curr = bfs_list;
  while (bfs_curr)
    {
      room = bfs_curr->room;
      
      if (room && IS_ROOM (room) && (coord = FNV (room, VAL_COORD)) != NULL)
	{
	  set_up_map_room (room);
	  color = room->color % 8;
	  
	  if (IS_ROOM_SET (room, ROOM_UNDERGROUND))
	    color = PLASMGEN_UNDERGROUND_COLOR;
	  
	  if (num_cubes > 0)
	    fprintf (f, ",\n");
	  num_cubes++;
	  
	  sprintf (buf, "T:<1,2,3>:<%d,%d,%d>:cube%s",
		   coord->val[0], coord->val[1], coord->val[2], colors[color]);
	  fprintf (f, buf);
	  
	  for (dir = 0; dir < REALDIR_MAX; dir++)
	    {
	      if ((exit = FNV (room, dir + 1)) == NULL ||
		  (nroom = find_thing_num (exit->val[0])) == NULL ||
		  !IS_ROOM (nroom) || IS_MARKED (nroom))
		continue;
	      
	      dx = dy = dz = 0;
	      if (dir == DIR_NORTH)
		dy = 1;
	      else if (dir == DIR_SOUTH)
		dy = -1;
	      else if (dir == DIR_EAST)
	    dx = 1;
	      else if (dir == DIR_WEST)
		dx = -1;
	      else if (dir == DIR_UP)
		dz = 1;
	      else if (dir == DIR_DOWN)
		dz = -1;
	      
	      add_plasm_bfs (nroom, coord->val[0] + dx,
			     coord->val[1] + dy, 
			     coord->val[2] + dz);
	    }
	}
      bfs_curr = bfs_curr->next;
    }
      
  /* Now close the city struct. */
  
  fprintf (f, ">;\nworld;\n");
  
  fclose (f);

  
  clear_bfs_list ();
  
  for (area = the_world->cont; area; area = area->next_cont)
    {
      for (room = area->cont; room; room = room->next_cont)
	{
	  if ((coord = FNV (room, VAL_COORD)) != NULL)
	    remove_value (room, coord);
	}
    }
  
  undo_marked (the_world);
  

  return;
}



      /* This adds a bfs to the bfs list but does some other
	 stuff that plasm needs done. */

void
add_plasm_bfs (THING *room, int x, int y, int z)
{
  VALUE *coord;
  
  if (!room || !IS_ROOM (room) || IS_MARKED(room))
    return;
  add_bfs (NULL, room, DIR_MAX);
  
  
  if ((coord = FNV (room, VAL_COORD)) == NULL)
    {
      coord = new_value();
      coord->type = VAL_COORD;
      add_value (room, coord);
    }
  coord->val[0] = x;
  coord->val[1] = y;
  coord->val[2] = z;
  return;
}
 
