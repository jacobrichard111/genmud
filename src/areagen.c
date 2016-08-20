

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
#include "cavegen.h"
#include "citygen.h"
 
/* These are the kinds of patches you can use to make areas. */

const int sector_patchtypes[SECTOR_PATCHTYPE_MAX] =
  {
    ROOM_ROUGH,
    ROOM_FOREST,
    ROOM_FIELD,
    ROOM_SWAMP,
    ROOM_DESERT,
    ROOM_SNOW,
    ROOM_WATERY,
    ROOM_UNDERGROUND,
  };

/* This file deals with autogenerating areas. */

void
do_areagen (THING *th, char *arg)
{
  if (!th || LEVEL (th) < MAX_LEVEL)
    return;

  areagen (th, arg);
  
  return;
}


void
areagen (THING *th, char *arg)
{
  char arg1[STD_LEN];
  int size = 0;   /* Size of area. */
  int start = 0;  /* Start vnum of area. */
  int type = 0;   /* Type...the base room sector flag type of the area. */
  int map_times = 0, name_times = 0; /* Counters to only try making
					the map and name a limited
					number of times. */
  int dx, dy, count;
  THING *area, *room;  /* The area and the rooms that will be put into
			  the area. */
  THING *ar2; /* Used to find if the area name is valid. */
  int vnum;
  int dir;
  int i;
  MAPGEN *map, *map_part[AREAGEN_MAP_PART_MAX];
  char buf[STD_LEN];
  char namebuf[STD_LEN], *t;   /* The name of the area. */
  char filenamebuf[STD_LEN]; /* Name of the file. */
  char realnamebuf[STD_LEN]; /* The real full name the area used in the
				game. */
  char filetypebuf[STD_LEN];
  bool name_ok = FALSE;  /* Is the filename for the area ok? */
  int go_dirs = 0;   /* Directions the area goes in...not just big blob. */
  int arealevel;
  int length; /* sqrt of size -- length of the area generated. */
  int num_map_parts = 1;
  /* Curr length and width of the map. */
  int curr_length = 0, curr_width;
  /* These are the max and min number of rooms in the area allowed. 
     They aren't close to area->mv so that there are lots of rooms
     left over for trees and catwalks and special locations. */
  int min_num_rooms, max_num_rooms;
  
  /* First get the start the size and the type of area. */
  
  arg = f_word (arg, arg1);
  start = atoi (arg1);
  
  if (start <= 1)
    {
      stt ("areagen <start> <size> <type> [<level>]\n\r", th);
      stt ("Start must be a number where the new area will start.\n\r", th);
      return;
    }

  arg = f_word (arg, arg1);
  size = atoi (arg1);
  
  /* Type can be a word or a flag. */
  arg = f_word (arg, arg1);
  if ((type = atoi (arg1)) == 0)
    type = find_bit_from_name (FLAG_ROOM1, arg1);
  
  if (type == ROOM_EASYMOVE || 
      !str_cmp (arg1, "city"))
    {
      sprintf (buf, "%d %d %d %d",
	       start, size, atoi (arg), 101001);
      citygen (th, buf);
      return;
    }
  

  if (!IS_SET (type, ROOM_SECTOR_FLAGS))
    {
      stt ("You can only setup areas with room sector flags as a base!\n\r", th);
      return;
    }
  /* Add the type of area this is. */
  
  sprintf (filetypebuf, arg1);
  filetypebuf[3] = '\0';
 
   /* We can also specify a level. Othewise the level is size/10. */
  
  
  if (atoi(arg) > 0)
    arealevel = atoi (arg);  
  else      
    arealevel = size/10;
  
  
  for (t = arg; *t; t++)
    {
      for (dir = 0; dir < FLATDIR_MAX; dir++)
	{
	  if (LC(*t) == dir_name[dir][0])
	    {
	      go_dirs |= (1 << dir);
	    }
	}
    }
  
  
  /* Sanity checks. */
  
  if (size < 100 || size > 2000)
    {
      stt ("areagen <start> <size> <type> [<level>]\n\r", th);
      stt ("The area must be from 100-2000 vnums big.\n\r", th);
      return;
    }
  
  /* Calc length of area for later on. */

  for (length = 10; length < 50; length++)
    if (length*length >= size)
      break;
  
  length = length*1;
  
  if ((area = find_thing_num (start)) != NULL &&
      area->cont)
    area = NULL;
  
  /* If this area already exists, we don't check it really except
     to make sure the rooms/mobjects won't overlap other areas. */
  if (!check_area_vnums (area, start, start + size - 1))
    {
      stt ("areagen <start> <size> <type> [<level>]\n\r", th);
      stt ("These vnums are already part of another area!\n\r", th);
      return;
    }
  
  /* Only allow certain kinds of sector flags to be used. */
  
  if (type == 0 || !IS_SET (type, ROOM_SECTOR_FLAGS) ||
      !IS_SET (type, AREAGEN_SECTOR_FLAGS))
    {
      stt ("areagen <start> <size> <type> [level]\n\r", th);
      stt ("The type must be a room sector flag!\n\r", th);
      return;
    }
  
  /* Create the area and set it up. */
  
  if ((area = new_thing ()) == NULL)
    return;
  
  area->vnum = start;
  top_vnum = MAX (top_vnum, start);
  add_thing_to_list (area);
  set_up_new_area(area, size);
  area->mv = size/2;
  min_num_rooms = area->mv/3;
  max_num_rooms = area->mv*5/6;    
  area->max_mv = size;
  area->level = arealevel;
  add_flagval (area, FLAG_ROOM1, type);
  if (IS_SET (type, BADROOM_BITS))
    add_flagval (area, FLAG_AREA, AREA_NOSETTLE);
  /* Generate a name. The name is 4-8 chars no ' no - z q x */
  
  while (!name_ok && name_times < 100)
    {
      name_ok = TRUE;
      strcpy (namebuf, capitalize(create_society_name (NULL)));
      if (strlen (namebuf) < 4)
	name_ok = FALSE;
      else
	{
	  for (t = namebuf; *t; t++)
	    {
	      if (*t == '-' ||
		  *t == '\'')
		{
		  name_ok = FALSE;
		  break;
		}
	    }
	}
    }
  
  namebuf[6] = '\0';
  if (!name_ok)
    return;
  
  sprintf (filenamebuf, "%s%sqz.are",
	   namebuf, filetypebuf);
  
  
  
  /* Check if it matches another area (unlikely but possible). */
  
  for (ar2 = the_world->cont; ar2 != NULL; ar2 =ar2->next_cont)
    {
      if (ar2 != area &&
	  !str_cmp (ar2->name, arg))
	{	  
	  break;
	}
    }
  
  
  /* If the name matches, then try to randomize the name a bit. */
  if (ar2)
    { 
      
      int tries;
      
      for (tries = 0; tries < 10; tries++)
	{
	  sprintf (filenamebuf, "%s%s%cqz.are",
		   namebuf, filetypebuf, nr ('a','z')); 
	  
	  /* Not likely to see two matching names in a row. */
	  
	  for (ar2 = the_world->cont; ar2 != NULL; ar2 =ar2->next_cont)
	    {
	      if (ar2 != area &&
		  !str_cmp (ar2->name, arg))
		{	  
		  break;
		}
	    }
	  
	  if (!ar2)
	    break;
	}
      
      /* Failed to find a different name after several tries. :( */
      if (ar2)
	{
	  stt ("Again a matching name found. Bailing out.\n\r", th);
	  return;
	}
    }
  
  
  free_str (area->name);
  filenamebuf[0] = LC (filenamebuf[0]);
  area->name = new_str (filenamebuf);
  
  /* This creates an area name based on the kind of area. It's used
     to name the rooms. */
  
  strcpy (realnamebuf, generate_area_name (namebuf, type));
  free_str (area->short_desc);
  area->short_desc = new_str (realnamebuf);
  
  free_str (area->long_desc);
  area->long_desc = new_str ("Something moves close by.");
  
  free_str (area->type);
  area->type = new_str ("Lonath");
  
  
  /* Now generate the map. The map consists of 1-4 map parts that
     get overlaid to try to make more random maps. If the map is an
     underground area, then you use different cavegen code to generate
     the area. */
  
 /* If this area goes in certain directions, you must attempt to
	 make parts for each of those directions. */
  
  if (!IS_SET (type, ROOM_UNDERGROUND))
    {
  
      
      if (go_dirs != 0)
	num_map_parts = FLATDIR_MAX;
      else
	num_map_parts = nr (2,AREAGEN_MAP_PART_MAX);
      
      
      
      
      for (map_times = 0; map_times < 500; map_times++)
	{      
	  map = NULL;
	  
	  for (count = 0; count < AREAGEN_MAP_PART_MAX; count++)
	    map_part[count] = NULL;
	  
	  
	  for (count = 0; count < num_map_parts; count++)
	    {
	      
	      curr_length = nr (length*5/4, length*3/2)/(num_map_parts);
	      if (curr_length < 4)
		curr_length = 4;
	      if (num_map_parts < 1)
		num_map_parts = 1;
	      curr_width = size/(nr (curr_length/2, curr_length)*(num_map_parts));
	      
	  /* For regular areas, go to the old code. */
	      if (go_dirs == 0)
		{
		  dx = count % 2;
		  dy = (count+1) %2;
		  
		}
	      else  /* Go dirs is > 0 */
		{
		  if (!IS_SET (go_dirs, (1 << count)))
		    continue;
		  
		  dx = 0;
		  dy = 0;
		  
		  
		  
		  /* THIS MAKES HEAVY USE OF THE ORDERING OF THE DIR_XXXX 
		     CONSTANTS IN SERV.H ! */
		  
		  if (count <= DIR_SOUTH)
		    {
		      dy = (1-2*count)*50;
		      if (IS_SET (go_dirs, (1 << DIR_EAST)))
			dx += nr (5,10);
		      if (IS_SET (go_dirs, (1 << DIR_WEST)))
			dx -= nr (5,10);
		    }
		  else /* EW parts */
		    {
		      dx = (5- 2*count)*50;
		      if (IS_SET (go_dirs, (1 << DIR_NORTH)))
			dy += nr (5,10);
		      if (IS_SET (go_dirs, (1 << DIR_SOUTH)))
			dy -= nr (5,10);
		    } 
		  
		  curr_width = curr_width*2/3;
		  if (curr_width < 7)
		    curr_width = 7;
		  curr_length = curr_length*2;
		}
	      
	      sprintf (buf, "%d %d %d %d %d %d %d %d",
		       dx, 
		       dy, 
		       curr_length,
		       curr_width,
		       nr (1,2),
		       /* Making the number below this higher (like 9+) makes
			  the areas "thinner" and therefore it's cheaper
			  to do tracking and it will cut down on CPU quite
			  a bit. */
		       nr (8,9) ,
		   nr (10,20),
		       nr (0, size/100));
	      map_part[count] = mapgen_generate (buf);
	    }
	  map = NULL;
	  for (i = 0; i < num_map_parts; i++)
	    {
	      if (map_part[i] != NULL)
		{
		  map = map_part[i];
		  map_part[i] = NULL;
		  break;
		}
	    }
	  for (count = 0; count < num_map_parts; count++)
	    {
	      if (go_dirs != 0)
		{
		  dx = 0;
		  dy = 0;
		  if (!IS_SET (go_dirs, (1 << count)))
		    continue;
		  /* Combine the edges together only. */
		  if (count == DIR_NORTH)
		    dy = curr_length*5/4;
		  else if (count == DIR_SOUTH)
		    dy = -curr_length*5/4;
		  else if (count == DIR_WEST || count == DIR_EAST)
		    { /* E/W Get added later and if the map only has
			 an N or S component, then these get added up
			 or down respectively to make the map look correct. */
		      if (IS_SET (go_dirs, (1 << DIR_NORTH)))
			{
			  if (IS_SET (go_dirs, (1 << DIR_SOUTH)))
			    dy = curr_length*3/5;
			  else
			    dy = curr_length*5/4;	
			}	  
		      else if (IS_SET (go_dirs, (1 << DIR_SOUTH)))
			dy = -curr_length*5/4;
		      
		      if (count == DIR_WEST)
			dx = -curr_length*5/4;
		      else if (count == DIR_EAST) 
			dx = curr_length*5/4;
		      
		    }
		}
	      else
		{
		  dx = nr (-length/2, length/2);
		  dy = nr (-length/2, length/2);
		}
	      if ((map = mapgen_combine 
		   (map, map_part[count], 
		    dx, dy)) == NULL)
		{
		  int count2;
		  free_mapgen (map);
		  map = NULL;
		  
		  for (count2 = count+1; count2 < num_map_parts; count2++)
		    {
		      free_mapgen (map_part[count2]);
		      map_part[count2] = NULL;
		    }
		  break;
		}  
	      map_part[count] = NULL;
	    }
	  if (map)
	    {
	      if (map->num_rooms < min_num_rooms ||
		  map->num_rooms > max_num_rooms)
		{
		  if (map->num_rooms < min_num_rooms)
		    length++;
		  else
		    length--;
		  free_mapgen (map);
		  map = NULL;
		}
	      if (map)
		break;      
	    }
	}
      
      
      if (!map)
	{
	  
	  sprintf (buf, "Failed to make area of size %d at %d\n", size, start);
	  log_it (buf);
	  return;
	}
      
      /* Otherwise make the map. */
      
      stt (mapgen_create (map, start + 1), th);
      free_mapgen (map);
    }
  else /* Do underground area...CAVEGEN! */
    {
      int dx, dy, dz;
      for (dx = 1; dx*dx < area->mv*2/3; dx++);
      	  
      for (map_times = 0; map_times < 100; map_times++)
	{
	  dy = nr (dx*5/6,dx*6/5);
	  dz = 2 + nr (0,8)/8;
	  if (dx < 10)
	    dx = 10;
	  if (dy < 10)
	    dy = 10;
	  sprintf (buf, "%d %d %d", dx, dy, dz);
	  cavegen (th, buf);
	  if (!cavegen_is_connected())
	    continue;
	  else if (find_num_cave_rooms () < min_num_rooms)
	    dx++;
	  else if (find_num_cave_rooms () > max_num_rooms)
	    dx--;
	  else  if (cavegen_generate (th, start + 1))
	    break;	  	  
	}
    }
	    
	    
	  
      /* Now set all of the rooms to have the name of the area and
	 the correct room flags. */
      
      for (vnum = start + 1; vnum < start + size; vnum++)
	{
	  if ((room = find_thing_num (vnum)) != NULL &&
	      IS_ROOM (room))
	    {
	      free_str (room->short_desc);
	      room->short_desc = new_str (realnamebuf);
	      add_flagval (room, FLAG_ROOM1, type);
	    }
	}
      
      /* Now mark the edge rooms with dir_edge as their name. This
	 must be done before you try to do any road linking or anything 
	 or else you won't be able to find rooms on area edges. */
      
  mark_area_edges (area);

  
  
  /* Now add the "roads" into the area. */
  
  if (type != ROOM_WATERY)
    {
      /* Now add the "roads" into the area. */
 
      add_roads (area, go_dirs);
      /* Add water to the area. */
      
      add_water (area);

    }
  
  /* This adds details like fields/caverns etc...to the area. 
     The type is sent so we don't add details of that type. */
  add_sector_details (area);

  add_room_descriptions (area);
  /* This tries to reconnect any sections that were disconnected by
     water. First pass -- no bad bridges, 2nd pass -- yes bad bridges. */
  
  
  if (type != ROOM_WATERY)
    fix_disconnected_sections (area);
 


  /* Now add trees to the area. This is after details so that
     you don't add details to the trees.*/
  add_trees (area);
  /* Add catwalks between trees in the area. This must be done before
     you add the elevations or the game won't be able to figure out
     where the tree bottoms are relative to each other on the map. 
     This also has to be done before you add underpasses or that
     might mess things up, too. */
  add_catwalks (area);
  
  
  /* AFTER THIS POINT YOU CANNOT GENERATE AN AREA GRID AND EXPECT IT TO 
     WORK BECAUSE THE AREA IS NO LONGER FLAT! ALSO, IF YOU DO GENERATE
     AN AREA GRID, START WITH area->vnum + 1 BECAUSE THERE ARE TREES
     AND THOSE TREES WILL MESS THINGS UP IF YOU ATTEMPT TO GENERATE A GRID
     STARTING AT ANY OLD POINT! */
 
  detailgen (area);
  
  
  
  
  /*  These can be added whenever now that the area extremes are 
      premarked. */
  
  if (!IS_SET (type, ROOM_UNDERGROUND))
    {
      add_underpasses (area);
      
      add_elevations (area);
    }
  
  add_shorelines (area);

  /* This adds some secret doors to the area. */
  
  add_secret_doors (area);



  /* Now start adding some people mobs that will pop eq. */
  
  areagen_generate_persons (area);
  
  /* Now unmark all the rooms in the area. */

  undo_marked (area);
  
  set_up_map(area);

  reset_thing (area, 0);
  return;
}

/* This creates a road name. */


char *
generate_road_name (int sector_types)
{
  static char buf[STD_LEN];
  char prefixbuf[STD_LEN];
  char namebuf[STD_LEN];
  char undergroundbuf[STD_LEN];
  char tempbuf[STD_LEN];
  
  strcpy (namebuf, find_gen_word (AREAGEN_AREA_VNUM, "road_names", NULL));
  strcpy (prefixbuf, find_gen_word (AREAGEN_AREA_VNUM, "road_prefixes", NULL));
  if (IS_SET (sector_types, ROOM_UNDERGROUND))
    strcpy (undergroundbuf, find_gen_word (AREAGEN_AREA_VNUM, "underground_places", NULL));
  else
    undergroundbuf[0] = '\0';
  if (*prefixbuf)
    strcat (prefixbuf, " ");
  
  if (nr (1,10) == 3)
    *prefixbuf = '\0';
  
  
  if (*undergroundbuf)
    strcat (undergroundbuf, " ");
  
  sprintf (tempbuf, "%s%s%s", prefixbuf, undergroundbuf, namebuf);
  
  sprintf (buf, "%s %s", a_an(tempbuf), tempbuf);
  *buf = UC(*buf);
  return buf;
}

/* This generates an area name based on the type of area and the
   name that the area has been given. The type is the sector flag 
   for the majority of the rooms in the area. */

char *
generate_area_name (char *name, int type)
{
  static char retbuf[STD_LEN];
  int room_flag_num; /* What the room flag number is for this type. */
  char typename[STD_LEN]; /* Typename woods forest.. */
  char prefixname[STD_LEN]; /* Dark light etc... */
  char keyword[STD_LEN];
  char suffixname[STD_LEN];
  static char buf[STD_LEN];
  char *t;
  SOCIETY *soc = NULL;
  
 
  /* Ok, give a small chance of a single word name that's a random
     syllable or two followed by a specal suffix. */
  
  if (nr (1,6) == 2 || type == ROOM_EASYMOVE)
    {
      int size = nr (3,5);
      do
	{
	  strcpy (buf, create_society_name (NULL));
	  buf[size] = '\0';
	  for (t = buf; *t; t++);
	  t--;
	  
	  if (strlen (buf) < 4 || strlen (buf) > 7 || isvowel (*t))
	    continue;
	  for (t = buf; *t; t++)
	    {
	      if (!isalpha(*t))
		break;
	    }
	  if (*t)
	    continue;
	}
      while (0);
      
      strcat (buf, find_gen_word (AREAGEN_AREA_VNUM, "area_name_suffix", NULL));
      buf[0] = UC (buf[0]);
      for (t = buf + 1; *t; t++)
	*t = LC(*t);
  
      return buf;
    }
  
 retbuf[0] = '\0'; 

 
 /* Now make sure the name is ok and check for a proper sector
    type. */
 
 if (!name || !*name || strlen(name) > 100 ||
     type == 0 || 
     (!IS_SET (type, ROOM_SECTOR_FLAGS)))
   return retbuf;
 
 
  /* Find which row in the room_flag array we want to use. */
  
  for (room_flag_num = 0; room1_flags[room_flag_num].flagval != 0; 
       room_flag_num++)
    {
      if ((int) room1_flags[room_flag_num].flagval == type)
	break;
    }
  
  if (room1_flags[room_flag_num].flagval == 0)
    return retbuf;
  
  /* So the name and type are ok, so generate the area typename. */
  
  /* The keyword is sector_names */
  
  sprintf (keyword, "%s_names", room1_flags[room_flag_num].name);
  strcpy (typename, find_gen_word (AREAGEN_AREA_VNUM, keyword , NULL));
  
  if (!*typename)
    strcpy (typename, "Lands");
  /* Now try for a prefix. May not exist. */

  sprintf (keyword, "%s_prefixes", room1_flags[room_flag_num].name);
  strcpy (prefixname, find_gen_word (AREAGEN_AREA_VNUM, keyword , NULL));
  if (!*prefixname || nr (1,5) == 3)
    strcpy (prefixname, find_gen_word (AREAGEN_AREA_VNUM, "sector_prefixes", NULL));
  
  if (*prefixname)
    strcat (prefixname, " ");
  
  
  

  strcpy (suffixname, find_gen_word (AREAGEN_AREA_VNUM, "sector_suffixes", NULL));
  suffixname[0] = UC (suffixname[0]);
  /* Use suffixes. Don't use proper name. */
  
  /* Have society names put in there with small probability. */

  if ((soc = find_random_society (TRUE)) != NULL &&
      IS_ROOM_SET (soc, type) && soc->name && *soc->name)
    {
      if (nr (1,3) == 3) /* Prefix name isn't set...name is set. */
	{
	  name[0] = '\0';
	  if (soc->adj)
	    sprintf (name, "%s", soc->adj);	  
	  if (*name)
	    strcat (name, " ");	  
	  if (soc->aname && *soc->aname && nr (1,2) == 2)
	    strcat (name, soc->aname);
	  else
	    strcat (name, soc->name);
	}
      if (nr (1,3) == 3)
	{
	  sprintf (suffixname, "the ");
	  if (soc->adj)
	    sprintf (suffixname + strlen(suffixname), "%s ", soc->adj);
	  if (soc->aname && *soc->aname && nr (1,2) == 2)
	    strcat (suffixname, soc->aname);
	  else
	    strcat (suffixname, soc->name);
	}
    }
  
  if (nr (1,4) == 2 && *suffixname && *typename)
    {
      sprintf (retbuf, "%s%s of %s", 
	       (nr (1,7) != 3 ? "The " : ""), typename, suffixname);
    }  
  else
    {
      if (nr (1,2) == 2)
	{
	  sprintf (retbuf, "%s%s%s",    
		   (nr (1,5) != 3 ? "The " : ""),
		   prefixname, typename);
	  if (nr (1,3) == 2)
	    {
	      strcat (retbuf, " of ");
	      strcat (retbuf, name);
	    }
	}
      else if (nr (1,3) == 1)
	sprintf (retbuf, "The %s%s %s", prefixname, name, typename);
      else if (type == ROOM_UNDERGROUND && nr (1,3) == 2)
	sprintf (retbuf, "Some %s%s", prefixname, typename);
      else
	sprintf (retbuf, "%s %s", name, typename);
    }
  
  retbuf[0] = UC(retbuf[0]);
  if (!*retbuf)
    return retbuf;
  for (t = retbuf + 1; *t; t++)
    {
      if (*(t-1) == ' ')
	*t = UC (*t);
    }
  return retbuf;
}


/* This adds roads to an area. It only adds roads between the 
   edges defined in the go_dirs bits. */

void
add_roads (THING *area, int go_dirs)
{
  int num_roads = 2;
  int sector_type;
  bool half_road = FALSE;
  int start_dir, end_dir;
  /* This is the number of full roads that fully cross the area. */
  
  int ns_roads = 0, ew_roads = 0;
  int num_full_roads = 0;
  int road_loop;
  int num_dirs = 0, i; /* Number of dirs this area goes. */
  bool ew_road, ns_road;
  
  if (!area || !IS_AREA (area))
    return;
  
  if ((sector_type = flagbits (area->flags, FLAG_ROOM1)) == 0)
    return;
  
  for (i = 0; i < FLATDIR_MAX; i++)
    if (IS_SET (go_dirs, (1 << i)))
      num_dirs++;
  
  if (num_dirs == 1) /* 1 direction area...no roads...it's a dead end. */
    return;

  /* 3 dirs...find the missing one -- make a full road on the long axis
     and a short one on the opp axis. */
  	
  /* All or none set, we do a "Regular" area. */
  if (num_dirs == 0 || num_dirs == FLATDIR_MAX)
    {
      /* Number of roads is proportional to the size of the area. */
      
      for (i = 0; i < area->mv/DETAIL_DENSITY; i++)
	num_roads += nr (0,1);
      
      /* Some of the roads are full roads that go edge to edge...some are
	 half roads that stop as soon as they're adjacent to another road. */
      num_full_roads = nr (num_roads/5, num_roads*2/3);
      
      if (num_roads < 1)
	return;
    }
  else if (num_dirs == 3) /* Num dirs 3. */ 
    {
      /* Ew is main. */
      if (!IS_SET (go_dirs, (1 << DIR_NORTH)) || 
	  !IS_SET (go_dirs, (1 << DIR_SOUTH)))
	{
	  /* Add the long EW road. */
	  add_road (area, DIR_EAST, DIR_WEST);
	  if (IS_SET (go_dirs, (1 << DIR_NORTH)))
	    add_road (area, DIR_NORTH, REALDIR_MAX);
	  else if (IS_SET (go_dirs, (1 << DIR_SOUTH)))
	    add_road (area, DIR_SOUTH, REALDIR_MAX);
	}
      else
	{
	  add_road (area, DIR_NORTH, DIR_SOUTH);
	  if (IS_SET (go_dirs, (1 << DIR_EAST)))
	    add_road (area, DIR_EAST, REALDIR_MAX);
	  else if (IS_SET (go_dirs, (1 << DIR_WEST)))
	    add_road (area, DIR_WEST, REALDIR_MAX);
	}
      return;
    }
  else if (num_dirs == 2)
    {
      start_dir = REALDIR_MAX;
      end_dir = REALDIR_MAX;
      if (IS_SET (go_dirs, (1 << DIR_NORTH)))
	start_dir = DIR_NORTH;
      if (IS_SET (go_dirs, (1 << DIR_SOUTH)))
	{
	  if (start_dir != REALDIR_MAX)
	    end_dir = DIR_SOUTH;
	  else
	    start_dir = DIR_SOUTH;
	}
      if (IS_SET (go_dirs, (1 << DIR_EAST)))
	{
	  if (start_dir != REALDIR_MAX)
	    end_dir = DIR_EAST;
	  else
	    start_dir = DIR_EAST;
	}
      if (IS_SET (go_dirs, (1 << DIR_WEST)))
	{
	  if (start_dir != REALDIR_MAX)
	    end_dir = DIR_WEST;
	  else
	    /* Shouldn't be possible...*/
	    start_dir = DIR_WEST;
	}
      add_road (area, start_dir, end_dir);
      return;
    }  
  for (road_loop = 0; road_loop < num_roads; road_loop++)
    {
      if (road_loop > num_full_roads)
	half_road = TRUE;
      else
	half_road = FALSE;
      
      ew_road = FALSE;
      ns_road = FALSE;
      
      /* Pick ns or ew roads. If you have an ns_road and no
	 ew_road, then the next one has to be ew. */
      
      if (ew_roads == 0 && ns_roads > 0) 
	ew_road = TRUE;
      else if (ns_roads == 0 && ew_roads > 0)
	ns_road = TRUE;
      else if (nr (1,2) == 1)
	ew_road = TRUE;
      else
	ns_road = TRUE;
      
      if (ew_road)
	{
	  start_dir = nr (DIR_EAST, DIR_WEST);
	  if (half_road)
	    end_dir = REALDIR_MAX;
	  else
	    end_dir = RDIR (start_dir);	  
	  add_road (area, start_dir, end_dir);
	  ew_roads++;	
	}
      else
	{
	  start_dir = nr (DIR_NORTH, DIR_SOUTH);
	  if (half_road)
	    end_dir = REALDIR_MAX;
	  else
	    end_dir = RDIR (start_dir);	  
	  add_road (area, start_dir, end_dir);
	  ns_roads++;
	}
    }
  add_road_turns (area);
  return;
}

/* This adds a road to an area. If the end_dir is REALDIR_MAX, it assumes
   that the road stops at the first road it meets. */


void
add_road (THING *area, int start_dir, int end_dir)
{
  int dir;
  THING *room, *nroom;
  VALUE *exit;
  int sector_type;
  char roadname[STD_LEN];
  BFS *bfs;
  bool half_road = FALSE;
  /* The road starting and ending points. */
  THING *road_start_room, *road_end_room;
  /* This is the number of full roads that fully cross the area. */
  
  
  if (start_dir < 0 || start_dir >= FLATDIR_MAX || !area || !IS_AREA (area))
    return;

  if ((road_start_room = find_room_on_area_edge (area, start_dir, 0)) == NULL)
    return;
  
  if (end_dir < 0 || end_dir >= FLATDIR_MAX || end_dir == start_dir)
    {
      end_dir = RDIR(start_dir);
      half_road = TRUE;
    }
      
  sector_type = flagbits (area->flags, FLAG_ROOM1);
  /* Find an end room on the opposite side anyway...it's just that with
     half roads, we stop them once they hit another road. */
  
  if ((road_end_room = find_room_on_area_edge (area, end_dir, 0)) == NULL)
    return;
  

  /* Find the shortest path from the start dir to the end dir. */
  
  clear_bfs_list();  
  add_bfs (NULL, road_start_room, REALDIR_MAX);
  
  while (bfs_curr)
    {
      if ((room = bfs_curr->room) != NULL && IS_ROOM(room))
	{ /* Go until the end room is found or a road is found
	     with a half room. */
	  if (room == road_end_room ||		  
	      (half_road && IS_ROOM_SET (room, ROOM_EASYMOVE)))
	    break;
	  
	  
	  /* Otherwise add more rooms. */
	  
	  for (dir = 0; dir < REALDIR_MAX; dir++)
	    {
	      if ((exit = FNV (room, dir + 1)) != NULL &&
		  (nroom = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM (nroom) &&
		  !IS_MARKED(nroom))
		{
		  add_bfs (bfs_curr, nroom, dir);
		}
	    }
	}
      bfs_curr = bfs_curr->next;
    }
  
  /* If the list looks ok from start to end, alter the rooms 
     along this road. */
  
  strcpy (roadname, generate_road_name(sector_type));
  
  
  /* Now if the endpoints exist, overwrite the rooms. */
  
  if (bfs_list &&
      bfs_list->room &&
      bfs_list->room == road_start_room &&
      bfs_curr &&
	  bfs_curr->room &&
      (bfs_curr->room == road_end_room ||
	   (half_road && 
	    IS_ROOM_SET (bfs_curr->room, ROOM_EASYMOVE))))
    {
      
      for (bfs = bfs_curr; bfs; bfs = bfs->from)
	{
	  /* Don't overwrite current roads. */
	  if ((room = bfs->room) != NULL &&
	      !IS_ROOM_SET (room, ROOM_EASYMOVE))
	    {
	      free_str (room->short_desc);
	      room->short_desc = new_str (roadname);
	      remove_flagval (room, FLAG_ROOM1, ROOM_SECTOR_FLAGS);
	      add_flagval (room, FLAG_ROOM1, ROOM_EASYMOVE);
	      if (sector_type == ROOM_UNDERGROUND)
		add_flagval (room, FLAG_ROOM1, ROOM_UNDERGROUND);
	      
	    }
	}
    }
  clear_bfs_list();
  return;
}

/* This adds turns into the roads in the area. */

void
add_road_turns (THING *area)
{
  THING *room, *nroom;
  int adjacent_with_same_name = 0;
  int dir;
  int dirs_with_same_name;
  char buf[STD_LEN];
  char oldname[STD_LEN];
  char prefixname[STD_LEN];
  char roadname[STD_LEN];
  for (room = area->cont; room; room = room->next_cont)
    {
      if (!IS_ROOM (room) || !IS_ROOM_SET (room, ROOM_EASYMOVE))
	continue;
      adjacent_with_same_name = 0;
      dirs_with_same_name = 0;
      for (dir = 0; dir < FLATDIR_MAX; dir++)
	{
	  if ((nroom = find_track_room (room, dir, 0)) != NULL &&
	      IS_ROOM_SET (nroom, ROOM_EASYMOVE) &&
	      !str_cmp (NAME(room), NAME(nroom)) &&
	      nroom != room)
	    {
	      adjacent_with_same_name++;
	      dirs_with_same_name |= (1 << dir);
	    }
	}
      /* Need exactly 2 adjacent with same name and NOT opp dirs. */

      dir = 0;
      if (adjacent_with_same_name == 2)
	{
	  for (dir = 0; dir < REALDIR_MAX; dir++)
	    {
	      if (IS_SET (dirs_with_same_name, (1 << dir)) &&
		  IS_SET (dirs_with_same_name, (1 << RDIR(dir))))
		  break;
	    }
	}
      if (dir != REALDIR_MAX || nr (1,7) != 3)
	continue;
      /* We now have 2 rooms adjacent with same name and not opp
	 dirs from each other. */
      strcpy (roadname, find_gen_word (AREAGEN_AREA_VNUM, "road_turns", NULL));
      strcpy (prefixname, find_gen_word (AREAGEN_AREA_VNUM, "road_turn_prefixes", NULL));
      if (*prefixname)
	{
	  strcat (prefixname, " ");
	}
      strcpy (oldname, NAME(room));
      oldname[0] = LC(oldname[0]);
      sprintf (buf, "%s %s%s in %s",
	       (*prefixname ? a_an (prefixname) : a_an (roadname)),
	       prefixname, roadname, oldname);
      free_str (room->short_desc);
      buf[0] = UC(buf[0]);
      room->short_desc = new_str (buf);
    }
  return;
}

/* This adds water to an area in the form of pools and little streams. */

/* We assume that the area has been built and the base type of room
   sector is sector_type. We also assume that the roads have been built. */

void
add_water (THING *area)
{
  int sector_type;
  
  if (!area || !IS_AREA (area))
    return;

  sector_type = flagbits (area->flags, FLAG_ROOM1);
  if (!sector_type)
    return;

  /* First figure out where the lakes will go. */
     
  /* For a desert only place simple oases. They are single water rooms
     with field/forest rooms adjacent to them. */
  
  add_lakes (area);
  add_streams (area);
  
}


/* This creates a grid of room vnums inside of grid[][] that tell 
   how those vnums are related to each other so that it's
   possible to find the extreme points in an area. */

void
generate_room_grid (THING *room, int grid[AREAGEN_MAX][AREAGEN_MAX], int extremes[REALDIR_MAX], int x, int y)
{
  THING *nroom;
  VALUE *exit;
  int dir;

  if (!room || !IS_ROOM (room) || x < 0 || x >= AREAGEN_MAX ||
      y < 0 || y >= AREAGEN_MAX || IS_MARKED(room) ||
      grid[x][y] != 0)
    return;
  
  SBIT (room->thing_flags, TH_MARKED);
  grid[x][y] = room->vnum;
  
  
  /* Update the extreme rooms. */
  if (x > extremes[DIR_EAST])
    extremes[DIR_EAST] = x;
  if (x < extremes[DIR_WEST])
    extremes[DIR_WEST] = x;
  if (y > extremes[DIR_NORTH])
    extremes[DIR_NORTH] = y;
  if (y < extremes[DIR_SOUTH])
    extremes[DIR_SOUTH] = y;
  
  /* Now check the other exits. */
  
  for (dir = 0; dir < FLATDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  nroom->in == room->in && nroom != room)
	{	  
	  generate_room_grid (nroom, grid, extremes,
			      (dir < 2 ? x : 
			       dir == 2 ? x + 1 : x - 1),
			      (dir == 0 ? y + 1 :
			       dir == 1 ? y - 1 : y));
	}
    }
  return;
}

/* This finds a room on the (geographic) edge of an area. To use this
   you need to have called mark_area_edges() already so that the
   appropriate edge rooms are marked. */

THING *
find_room_on_area_edge (THING *area, int dir, int room_flags_wanted)
{
  int count = 0, num_choices = 0, num_chose = 0, num_with_room_flags = 0;
  THING *room;
  char name[STD_LEN];
  bool got_room_flags = FALSE;
  
  /* The area must exist and the dir must be from 0 to 3 (no u-d edges). */
  if (!area || !IS_AREA (area) || dir < 0 || dir >= 4)
    return NULL;
  
  /* First get the name we want to find. */

  sprintf (name, "%s_edge", dir_name[dir]);

  
  for (count = 0; count < 2; count++)
    {
      for (room = area->cont; room; room = room->next_cont)
	{
	  if (!IS_ROOM (room) || !is_named (room, name))
	    continue;
	  
	  if (count == 0)
	    {
	      num_choices++;
	      if (!room_flags_wanted || 
		  IS_ROOM_SET (room, room_flags_wanted))
		num_with_room_flags++;	      
	    }
	  else
	    {
	      if (got_room_flags)
		{
		  if (IS_ROOM_SET (room, room_flags_wanted) &&
		      --num_chose < 1)
		    break;
		}
	      else if (--num_chose < 1)
		break;
	    }	
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    break;
	  
	  if (room_flags_wanted &&
	      num_with_room_flags > 0)
	    {
	      got_room_flags = TRUE;
	      num_chose = nr (1, num_with_room_flags);
	    }
	  else
	    {
	      num_chose = nr (1, num_choices);
	      got_room_flags = FALSE;
	    }
	}	          
    }      
  return room;
}   

/* This adds some lakes to an area. */

void
add_lakes (THING *area)
{
  int i, count;
  int room_choices, room_chose = 0;
  int room_flags;
  int lake_size;
  int num_lakes = 0;
  int sector_type;
  THING *room;
  char prefixname[STD_LEN];
  char name[STD_LEN];  
  char typename[STD_LEN];
  char undergroundname[STD_LEN];
  char fullname[STD_LEN];
  if (!area || !IS_AREA (area))
    return;
  
  sector_type = flagbits (area->flags, FLAG_ROOM1);
  
  if (!sector_type)
    return;
  
  num_lakes = nr (1, 1+area->mv/DETAIL_DENSITY);
  
  if (IS_SET (sector_type, ROOM_DESERT))
    num_lakes = (num_lakes+1)/2;
  else if (IS_SET (sector_type, ROOM_SWAMP))
    num_lakes = num_lakes*3/2;
  
  for (i = 0; i < num_lakes; i++)
    {
      room_choices = 0;
      for (count = 0; count < 2; count++)
	{
	  for (room = area->cont; room; room = room->next_cont)
	    {
	      if (IS_ROOM (room) &&
		  (room_flags = flagbits (room->flags, FLAG_ROOM1)) == 
		  sector_type)
		{
		  if (count == 0)
		    room_choices++;
		  else if (--room_chose < 1)
		    break;
		}
	    }
	  if (count == 0)
	    {		  
	      if (room_choices < 1)
		break;
	      room_chose = nr (1, room_choices);
	    }	      
	}
      
      /* If no more open rooms, bail out. */
      if (!room)
	break;

      if (nr (1,5) == 2)
	{
	  if (nr (1,2) == 1)
	    sprintf (fullname, "%s Lake", capitalize(create_society_name(NULL)));
	  else
	    sprintf (fullname, "Lake %s", capitalize(create_society_name(NULL)));
	}
      else
	{
	  strcpy (prefixname, find_gen_word (AREAGEN_AREA_VNUM, "watery_prefixes", NULL));
	  if (*prefixname)
	    strcat (prefixname, " ");
	  
	  if (IS_SET (sector_type, ROOM_UNDERGROUND))	    
	    strcpy (undergroundname, find_gen_word (AREAGEN_AREA_VNUM, "underground_placenames", NULL));
	  else
	    undergroundname[0] = '\0';
	  if (*undergroundname)
	    strcat (undergroundname, " ");
	  
	  if (sector_type == ROOM_DESERT)
	    strcpy (typename, find_gen_word (AREAGEN_AREA_VNUM, "oasis_names", NULL));
	  else
	    strcpy (typename, find_gen_word (AREAGEN_AREA_VNUM, "lake_names", NULL));
	  
	  sprintf (name, "%s%s%s", prefixname, undergroundname, typename);
	  sprintf (fullname, "%s %s", a_an (name), name);
	  capitalize_all_words (fullname);
	  
	}
      if (sector_type == ROOM_DESERT)
	lake_size = 1;
      else if (sector_type == ROOM_SWAMP)
	lake_size = nr (2,5);
      else 
	lake_size = nr (2, 3);
      
      /* This recursively adds a lake to the area. */
      
      undo_marked(room);
      add_lake (room, fullname, sector_type, 1, lake_size);
      undo_marked(room);

      /* This adds forest and field rooms next to the oasis. */
      if (sector_type == ROOM_DESERT)
	add_adjacent_oasis_rooms (room, sector_type);
    }
  return;
}
/* This adds a lake to an area. It gets passed a current room, a 
   name for the room, an original sector type, and a current depth
   and a max depth to know when to stop. */

void
add_lake (THING *room, char *name, int sector_type, int curr_depth,
	  int max_depth)
{
  int room_flags, dir;
  THING *nroom;
  VALUE *exit;
  
  if (!room || !name || !*name || !IS_ROOM (room) ||
      (room_flags = flagbits (room->flags, FLAG_ROOM1)) != sector_type ||
      curr_depth > max_depth || IS_MARKED(room))
    return;
  
  free_str (room->short_desc);
  room->short_desc = new_str (name);  
  add_flagval (room, FLAG_ROOM1, ROOM_WATERY);
  if (sector_type != ROOM_UNDERGROUND)
    remove_flagval (room, FLAG_ROOM1, sector_type);
  SBIT (room->thing_flags, TH_MARKED);
  
  if (curr_depth == max_depth)
    return;
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  nroom->in == room->in)
	add_lake (nroom, name, sector_type, curr_depth + 1, max_depth);
    }
  return;
}
/* These add field and forest rooms adjacent to an oasis. */

void
add_adjacent_oasis_rooms (THING *room, int sector_type)
{
  THING *nroom;
  VALUE *exit;
  int dir;
  int room_flags;
  char name[STD_LEN];
  char fieldprefix[STD_LEN];
  if (!room || !IS_ROOM (room))
    return;
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (nroom) &&
	  (room_flags = flagbits(nroom->flags, FLAG_ROOM1)) == 
	  sector_type)
	{
	  /* Decide on field or forest. */
	  
	  if (nr (1,2) == 1)
	    {
	      sprintf (name, "A %s %sTrees",
		       (nr (1,3) == 2 ? "Copse of" :
			(nr (1,2) == 1 ? "Stand of" : "Few")),
		       (nr (1,4) == 3 ? "Palm " : ""));
	      free_str (nroom->short_desc);
	      nroom->short_desc = new_str (name);
	      add_flagval (nroom, FLAG_ROOM1, ROOM_FIELD);
	      remove_flagval (nroom, FLAG_ROOM1, ROOM_DESERT);
	    }
	  else
	    {
	      strcpy (fieldprefix, find_gen_word (AREAGEN_AREA_VNUM, "field_prefixes", NULL));
	      sprintf (name, "A %s%s",
		       fieldprefix,
		       (nr (1,2) == 2 ? "Patch of Grass" : "Field"));
	      free_str (nroom->short_desc);
	      nroom->short_desc = new_str (name); 
	      add_flagval (nroom, FLAG_ROOM1, ROOM_FOREST);
	      remove_flagval (nroom, FLAG_ROOM1, ROOM_DESERT);
	    }
	}
    }
  return;
}

/* This generates a stream name. It's a separate function so that
   splits in streams can be handled easier. */
char *
generate_stream_name (int sector_flags)
{
  static char buf[STD_LEN];
  char name[STD_LEN], *t;
  char prefixname[STD_LEN];
  char typename[STD_LEN];
  char undergroundname[STD_LEN];
  
  strcpy (prefixname, find_gen_word (AREAGEN_AREA_VNUM, "watery_prefixes", NULL));
  if (*prefixname)
    strcat (prefixname, " ");
  
  strcpy (typename, find_gen_word (AREAGEN_AREA_VNUM, "stream_names", NULL));
  
  if (IS_SET (sector_flags, ROOM_UNDERGROUND))    
    strcpy (undergroundname, find_gen_word (AREAGEN_AREA_VNUM, "underground_placenames", NULL));
  else
    undergroundname[0] = '\0';
  if (*undergroundname)
    strcat (undergroundname, " ");
  

  if (nr (1,3) != 2)
    {
      sprintf (name, "%s%s%s", prefixname, undergroundname, typename);
      sprintf (buf, "%s %s", a_an (name), name);
      
      buf[0] = UC(buf[0]);
      return buf;
    }
  /* Otherwise make a "named" stream. */

  strcpy (name, capitalize(create_society_name(NULL)));
  for (t = name; *t; t++);
  t--;
  
  /* Possessive name Bob's Stream vs The Bob Stream. */
  if (nr (1,2) == 2)
    {
      possessive_form (name);
      sprintf (buf, "%s %s", name, typename);
    }
  else
    { 
      sprintf (buf, "The %s %s", name, typename);
    }
  buf[0] = UC(buf[0]);
  return buf;
}


/* This adds streams to an area. They start in random places and 
   go until they can't go anymore, or until they hit a patch of water. */

void
add_streams (THING *area)
{
  int i, count;
  int room_choices, room_chose = 0;
  int room_flags;
  int num_streams;
  int sector_type;
  THING *room;
  char name[STD_LEN];  
  
  if (!area || !IS_AREA (area))
    return;  
  sector_type = flagbits (area->flags, FLAG_ROOM1);
  
  if (IS_SET (sector_type, ROOM_DESERT) ||
      !sector_type)
    return;
  
  
  num_streams = nr (1, 1+area->mv/DETAIL_DENSITY);
  if (IS_SET (sector_type, ROOM_SWAMP))
    num_streams += nr (0, num_streams/2);
  else if (IS_SET (sector_type, ROOM_DESERT))
    num_streams /= 2;
  
  for (i = 0; i < num_streams; i++)
    {
      room_choices = 0;
      for (count = 0; count < 2; count++)
	{
	  for (room = area->cont; room; room = room->next_cont)
	    {
	      if (IS_ROOM (room) &&
		  (room_flags = flagbits (room->flags, FLAG_ROOM1)) == 
		  sector_type)
		{
		  if (count == 0)
		    room_choices++;
		  else if (--room_chose < 1)
		    break;
		}
	    }
	  if (count == 0)
	    {		  
	      if (room_choices < 1)
		break;
	      room_chose = nr (1, room_choices);
	    }	      
	}
      
      /* If no more open rooms, bail out. */
      if (!room)
	break;
      
      /* This recursively adds a lake to the area. */
      
      strcpy (name, generate_stream_name (sector_type));
      add_stream (room, name, sector_type, 0, REALDIR_MAX);
    }
  return;
}

/* This adds a stream to an area. It is a random walk that tends to
   stay in the direction it's going, and it goes under roads and
   it stops when it hits more water. It also has a really small
   chance of spawning another stream. */

void
add_stream (THING *room, char *name, int sector_type, int depth, int dir)
{
  THING *nroom, *nroom2;
  VALUE *exit, *exit2;
  int room_flags, nroom_flags;
  THING *area;
  bool other_water_nearby = FALSE;
  int num_area_rooms;
  char name2[STD_LEN];
  int i, j;
  int rand_turn;
  bool done_once = FALSE; /*IF this stream has no dir from, we try
			    to make it go in 2 dirs. */
  bool looping_back; /* Is this stream looping back on itself? */
  /* This is used to try to make the stream turn once in a while but
     generally go straight. */
  if (nr (1,7) == 3)
    rand_turn = nr (0,3);
  else
    rand_turn = 0;
  
  /* First do some sanity checking. */
  
  /* Don't make this a stream room if the room or name don't exist
     or if the sector is bad or if the room is not in an area or if
     the stream is too long relative to the area size or the 
     room is already a road or water room. */
  if (!room  || !name || !*name || sector_type == 0 ||
      sector_type == ROOM_DESERT || (area = room->in) == NULL ||
      (num_area_rooms = area->mv) < 20 ||
      depth > nr (5, 5 + num_area_rooms/30) ||
      IS_SET ((room_flags = flagbits (room->flags, FLAG_ROOM1)),
	      ROOM_WATERY) ||
      !IS_SET (room_flags, sector_type))
    return;
  
  /* Check if there are other named streams nearby. */

  for (i = 0; i < REALDIR_MAX; i++)
    {
      if ((exit = FNV (room, i + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM_SET (nroom, ROOM_WATERY) &&
	  str_cmp (NAME(nroom), name))
	other_water_nearby = TRUE;
    }
  
  
  
  /* If this is a road room, then try to move "under" it. */
  
  if (IS_ROOM_SET (room, ROOM_EASYMOVE))
    {
      if (dir < 0 || dir >= REALDIR_MAX)
	return;
      
      /* Try to move past the road. Search for an exit from the
	 road that isn't a stream room. */
      
      for (i = 0; i < REALDIR_MAX; i++)
	{
	  if ((exit = FNV (room, ((i + dir) % REALDIR_MAX) + 1)) != NULL &&
	      (nroom = find_thing_num (exit->val[0])) != NULL &&
	      IS_ROOM (nroom) && 
	      (nroom_flags = flagbits (nroom->flags, FLAG_ROOM1)) == sector_type)
	    {
	      add_stream (nroom, name, sector_type, depth + 2, (i + dir)%REALDIR_MAX);
	      break;
	    }
	}
      
      
      return;
    }
       
  free_str (room->short_desc);
  room->short_desc = new_str (name); 
  add_flagval (room, FLAG_ROOM1, ROOM_WATERY);
  if (sector_type != ROOM_UNDERGROUND)
    remove_flagval (room, FLAG_ROOM1, sector_type);
  
  /* Do not continue if there's other water nearby. */

  if (other_water_nearby)
    return;


  /* Now add more rooms to the stream. */
      
      
  for (i = 0; i < REALDIR_MAX; i++)
    {
      if ((exit = FNV (room, (i + dir + rand_turn) % REALDIR_MAX)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (nroom) &&
	  IS_ROOM_SET (nroom, sector_type) &&
	  !IS_ROOM_SET (nroom, ROOM_WATERY) &&
	  (!IS_ROOM_SET(nroom, ROOM_EASYMOVE) ||
	   nr (1,2) == 2))
	{
	  
	  /* Do not add a stream to this dir if the room is adjacent
	     to another room with the same stream name (loopback) */
	  looping_back = FALSE;
	  for (j = 0; j < REALDIR_MAX; j++)
	    {
	      if (j == RDIR((i + dir + rand_turn) % REALDIR_MAX))
		continue;
	      if ((exit2 = FNV (nroom, j + 1)) != NULL &&
		  (nroom2 = find_thing_num (exit2->val[0])) != NULL &&
		  !str_cmp (NAME(nroom2), name) &&
		  nroom2 != room)
		looping_back = TRUE;
	    }
	  if (!looping_back)
	    {
	      add_stream (nroom, name, sector_type, depth + 1, 
			  (i + dir + rand_turn)%REALDIR_MAX);
	    }
	  if (dir !=  REALDIR_MAX || done_once)
	    break;
	  done_once = TRUE;
	} 
    }

  /* Add a small chance for a new stream. */

  if (nr (1,45) == 17 && depth > 5)
    {
      for (i = 0; i < REALDIR_MAX; i++)
	{
	  if ((exit = FNV (room, (i % REALDIR_MAX))) != NULL &&
	      (nroom = find_thing_num (exit->val[0])) != NULL &&
	      IS_ROOM (nroom) &&
	      IS_ROOM_SET (nroom, sector_type) &&
	      !IS_ROOM_SET (nroom, ROOM_WATERY))
	    {
	      strcpy (name2, generate_stream_name(sector_type));
	      add_stream (nroom, name2, sector_type, 0, i);
	      break;
	    }
	}
    }
  return;
}

/* Will add some details to an area. It only messes with rooms 
   that are of the same sector type as the base sector type. */

void 
add_sector_details (THING *area)
{
  /* Number of rooms of these types that we want to put into
     this area. */
  
  int i, j;
  int patch_count[SECTOR_PATCHTYPE_MAX];
  int sector_type;
  if (!area || !IS_AREA (area))
    return;
  sector_type = flagbits (area->flags, FLAG_ROOM1);
  
  if (!sector_type)
    return;


  /* The number of patches to add of each type is given by 
     a random number then modified somewhat */
  
  for (i = 0; i < SECTOR_PATCHTYPE_MAX; i++)
    {	
      patch_count[i] = nr (0, area->mv/(2*DETAIL_DENSITY));
      
      if (sector_type == ROOM_WATERY)
	patch_count[i] /= 2;

      if (IS_SET (sector_patchtypes[i], ROOM_WATERY))
	patch_count[i] = 0;
      /* This can be made more efficient, since the three here
	 desert swamp snow don't touch, but it's more clear to make
	 it longer. */
      
      if (sector_type == ROOM_DESERT)
	{
	  if (IS_SET (sector_patchtypes[i],
		      (ROOM_SNOW | ROOM_SWAMP)))
	    patch_count[i] = 0;
	}
      if (sector_type == ROOM_SNOW)
	{
	  if (IS_SET (sector_patchtypes[i],
		      (ROOM_DESERT | ROOM_SWAMP)))
	    patch_count[i] = 0;
	}
      if (sector_type == ROOM_SWAMP)
	{
	  if (IS_SET (sector_patchtypes[i],
		      (ROOM_DESERT | ROOM_SNOW)))
	    patch_count[i] = 0;
	}
      if (sector_type == ROOM_UNDERGROUND &&
	  !IS_SET (sector_patchtypes[i],
		   (ROOM_FIELD | ROOM_FOREST)))
	{
	  patch_count[i] = 0;
	}
      
	
      if (sector_type != ROOM_ROUGH && sector_patchtypes[i] == ROOM_SNOW)
	patch_count[i] = 0;
      
      
    }
  
  for (i = 0; i < SECTOR_PATCHTYPE_MAX; i++)
    {
      for (j = 0; j < patch_count[i]; j++)
	{
	    start_sector_patch (area, sector_patchtypes[i]);
	}
    }
  return;
}



/* This generates a name for a patch of ground based on the 
   patch type. */

char *
generate_patch_name (int sector_type, int patch_type)
{
  static char retbuf[STD_LEN];
  
  char patchname[STD_LEN]; /* Typename woods forest.. */
  char prefixname[STD_LEN]; /* Dark light open etc... */
  char undergroundname[STD_LEN];
  char ugfieldprefix[STD_LEN];
  char keyword[STD_LEN];
  char buf[STD_LEN];
  int i;

  patchname[0] = '\0';
  prefixname[0] = '\0';
  undergroundname[0] = '\0';
  ugfieldprefix[0] = '\0';
  retbuf[0] = '\0'; 
  if (!patch_type)
    return retbuf;
  
  for (i = 0; room1_flags[i].flagval != 0; i++)
    {
      if (patch_type == (int) room1_flags[i].flagval &&
	  *room1_flags[i].name)
	break;
    }
  
  if (!*room1_flags[i].name)
    return retbuf;
  
  /* Get the patchname and the prefix. */
  sprintf (keyword, "%s_patchnames", room1_flags[i].name);
  strcpy (patchname, find_gen_word (AREAGEN_AREA_VNUM, keyword, NULL));
  sprintf (keyword, "%s_prefixes", room1_flags[i].name);
  strcpy (prefixname, find_gen_word (AREAGEN_AREA_VNUM, keyword, NULL));
  
  if (!* patchname)
    strcpy (patchname, "Wilderness");
  
  /* Add the prefix. */
  
  
  if (!*prefixname || nr (1,10) == 3)
    strcpy (prefixname, find_gen_word (AREAGEN_AREA_VNUM, "patch_prefixes", NULL));

  if (sector_type == ROOM_UNDERGROUND)
    {
      strcpy (undergroundname, find_gen_word (AREAGEN_AREA_VNUM, "underground_places", NULL));
      
      if (IS_SET (patch_type, (ROOM_FOREST | ROOM_FIELD)))
	strcpy (prefixname, find_gen_word (AREAGEN_AREA_VNUM, "underground_forest_field_prefixes", NULL));
      
    }
  
  if (*prefixname)
    strcat (prefixname, " ");
  if (*undergroundname)
    strcat (undergroundname, " ");
  
  if (sector_type == ROOM_WATERY && patch_type != ROOM_WATERY &&
      nr (1,4) == 2)
    {
      sprintf (retbuf, "%s Island", capitalize(create_society_name (NULL)));
    }
  else
    {
      char *t;
      sprintf (buf, "%s%s%s", undergroundname, prefixname, patchname);
      for (t = buf; *t; t++);
      t--;
      if (LC(*t) == 's')
	sprintf (retbuf, "some %s", buf);
      else
	sprintf (retbuf, "%s %s", a_an (buf), buf);
    }
  capitalize_all_words (retbuf);
  return retbuf;
}

/* This adds a patch of a certain kind to an area. Note that the 
   sector_type and the patch_type can be the same to add some
   extra randomness in there. */

void 
start_sector_patch (THING *area, int patch_type)
{
  THING *room;
  char name[STD_LEN];
  int max_depth;
  int sector_type;
  int num_choices = 0, num_chose = 0, count;
  
  if (!area || !IS_AREA (area) ||
      patch_type == 0)
    return;

  sector_type = flagbits(area->flags, FLAG_ROOM1);
  
  if (!sector_type)
    return;
  /* Find a room of the starting sector type to use to start the patch. */
 
  for (count = 0; count < 2; count++)
    {
      for (room = area->cont; room; room = room->next_cont)
	{
	  if (IS_ROOM (room) && 
	      IS_ROOM_SET (room, sector_type))
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
  
  if (!room)
    return;
  
  strcpy (name, generate_patch_name (sector_type, patch_type));
  max_depth = nr (2,5);
  if (sector_type == ROOM_WATERY && max_depth > 4)
    max_depth = 4;
  undo_marked(room);
  add_sector_patch (room, name, sector_type, patch_type , 1, max_depth);
  undo_marked(room);
  return;
}


/* This actually adds a sector patch into the game. It expands out from
   its central location and jumps over roads and water. */

void
add_sector_patch (THING *room, char *name, int sector_type, int patch_type,
		  int curr_depth, int max_depth)
{
  THING *nroom;
  int dir;
  VALUE *exit;
  int room_flags;
  
  if (!room || !name || !*name || sector_type == 0 || patch_type == 0 ||
      curr_depth > max_depth || nr (1,12) == 3 ||
      (curr_depth == max_depth && nr (1,3) == 1) || 
      !IS_ROOM (room) || IS_MARKED(room))
    return;
  
  SBIT (room->thing_flags, TH_MARKED);
  room_flags = flagbits (room->flags, FLAG_ROOM1);
  if (room_flags != sector_type)
    {
      /* Try to jump over water/roads. */
      if (!IS_SET (room_flags, ROOM_WATERY | ROOM_EASYMOVE))
	return;
    }
  append_name (room, "sector_patch");
  /* If we want to change this room, do it. */
  
  if (!IS_SET (room_flags, ROOM_EASYMOVE) &&
      (!IS_SET (room_flags, ROOM_WATERY) || sector_type == ROOM_WATERY))
    {
      free_str (room->short_desc);
      room->short_desc = new_str(name); 
      if (sector_type != patch_type)
	{
	  add_flagval (room, FLAG_ROOM1, patch_type);
	  if (sector_type != ROOM_UNDERGROUND)
	    remove_flagval (room, FLAG_ROOM1, sector_type);
	}
    }
  /* Now expand the patch outside this room. */
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL)
	add_sector_patch (nroom, name, sector_type, patch_type,
			  curr_depth+1, max_depth);
    }
  return;
}

/* This generates room descriptions for the rooms in the area. */

void
add_room_descriptions (THING *area)
{
  THING *room;

  if (!area || !IS_AREA (area) || 
      flagbits(area->flags, FLAG_ROOM1) == 0)
    return;

  /* I think that I want to dynamically generate these for now. */
  return;

  
  for (room = area->cont; room; room = room->next_cont)
    {
      if (IS_ROOM (room))
	{
	  free_str (room->desc);
	  room->desc = generate_room_description (room);
	}
    }
  return;
}


/* This makes sure that the areas we generate are connected. Start with
   a road room (if none exist then don't bother...something must
   have gone really wrong anyway...) then search to find all rooms
   that are connected to that room (without badroom bits)
   and then check all rooms to see if they are marked or have badroom
   bits. If some rooms are blocked, then fix them.  Bad bridges are
   bridges that go from one disconnected area to another. They are
   only allowed after all regular good bridges have been made. */

void
fix_disconnected_sections (THING *area)
{
  THING *room, *nroom, *start_room;
  VALUE *exit;
  int main_section_size = 0;
  int dir;
  int depth;
  bool found_path = FALSE;
  bool added_bridge = FALSE; /* Did we add a bridge at this depth? */
  bool added_bridge_this_time = TRUE;
  
  if (!area)
    return;
  
 
  /* For each length search through all rooms. Keep going until we make
     a pass where we add no more bridges. */

  while (added_bridge_this_time)
    {
      added_bridge_this_time = FALSE;
      for (depth = 1; depth < 10; depth++)
	{
	  /* Do this in several passes to try to get all areas connected. */
	  for (start_room = area->cont; start_room; start_room = start_room->next_cont)
	    {
	      if (IS_ROOM (start_room) &&
		  IS_ROOM_SET (start_room, ROOM_EASYMOVE))
		break;
	    }
	  
	  /* Boggle, no roads? Shouldn't happen. */
      
	  if (!start_room)
	    {
	      if (area->cont && IS_ROOM (area->cont))
		start_room = area->cont;
	      else
		return;
	    }
	  
	  
	  
	  /* Find the number of rooms marked in the main section of the map. */
	  
	  undo_marked (area);
	  
	  main_section_size = find_connected_rooms_size (start_room, 0);
	  
	  added_bridge = FALSE;
	  for (room = area->cont; room; room = room->next_cont)
	    {
	      /* Any room that's not connected and isn't a badroom bit
		 gets checked. */
	      if (IS_ROOM (room) &&
		  !IS_MARKED (room) &&
		  !IS_ROOM_SET (room, BADROOM_BITS))
		{ 	      
		  
		  found_path = FALSE;
		  /* See if there are any badrooms connected to this room. */
		  for (dir = 0; dir < FLATDIR_MAX; dir++)
		    {
		      /* If we can make a path... */
		      if ((exit = FNV (room, dir + 1)) != NULL &&
			  (nroom = find_thing_num (exit->val[0])) != NULL &&
			  found_path_through_badrooms (nroom, depth))
			{
			  
			  found_path = TRUE;
			  /* Mark everything we just connected. */
			  find_connected_rooms_size (room, 0);
			  added_bridge = TRUE;
			  added_bridge_this_time = TRUE;
			  break;
			}
		    }
		}
	    }
	  if (added_bridge)
	    depth--;
	}
      
    }
  undo_marked (area);
  return;
}

  
/* This returns the number of rooms that are connected to this room in the
   area that aren't blocked by badroom rooms. */

int
find_connected_rooms_size (THING *room, int badroom_bits_allowed)
{
  int size = 1;
  int dir;
  THING *nroom;
  VALUE *exit;
  int room_flags;
  
  if (!IS_ROOM (room) || IS_MARKED (room))
    return 0;
  
  room_flags = flagbits (room->flags, FLAG_ROOM1);
  
  /* If the room has disallowed badroom bits set, then return. */
  
  if (IS_SET (room_flags, (BADROOM_BITS & ~badroom_bits_allowed)))
    return  0;

  SBIT (room->thing_flags, TH_MARKED);
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (nroom) && !IS_ROOM_SET (room, BADROOM_BITS))
	size += find_connected_rooms_size (nroom, 0);
    }
  return size;
}


/* Now we try to find a path through the badrooms. It must be a
   straight line. */

bool
found_path_through_badrooms (THING *start_room, int max_depth)
{
  THING *room, *nroom;
  int dir, curr_dir;
  int name_type = 0;
  char buf[STD_LEN];
  char bridgename[STD_LEN];
  char prefixname[STD_LEN];
  char locname[STD_LEN];
  VALUE *exit;
  bool found_end_room = FALSE;
  BFS *bfs;

  
  clear_bfs_list();
  
  if (!start_room || !IS_ROOM (start_room) || !IS_ROOM_SET (start_room,
							    BADROOM_BITS))
    return FALSE;
  
  add_bfs (NULL, start_room, nr (0, FLATDIR_MAX-1));
  
  while (bfs_curr && !found_end_room)
    {
      /* If the room exists */
      
      if ((room = bfs_curr->room) != NULL && IS_ROOM (room))
	{
	  for (dir = 0; dir < FLATDIR_MAX && !found_end_room; dir++)
	    {
	      /* If it's next to a good marked room, then break out
		 and make the bridge. Otherwise, add more rooms. */
	      curr_dir = (bfs_curr->dir + dir) % FLATDIR_MAX;
	      if ((exit = FNV (room, curr_dir + 1)) != NULL &&
		  (nroom = find_thing_num (exit->val[0])) != NULL)
		{
		  if (!IS_MARKED (nroom) && IS_BADROOM(nroom) &&
		      bfs_curr->depth < max_depth)
		    add_bfs (bfs_curr, nroom, curr_dir);
		  else if (IS_MARKED (nroom) && !IS_BADROOM (nroom))
		    {
		      found_end_room = TRUE;
		      break;
		    }
		}
	    }
	}
      if (!found_end_room)
	bfs_curr = bfs_curr->next;
    }

  if (!bfs_curr)
    {
      clear_bfs_list();
      return FALSE;
    }

  /* Get the overall name. */



  strcpy (prefixname, find_gen_word (AREAGEN_AREA_VNUM, "bridge_prefixes", NULL));
  strcpy (bridgename, find_gen_word (AREAGEN_AREA_VNUM, "bridge_names", NULL));
  strcpy (locname, find_gen_word (AREAGEN_AREA_VNUM, "bridge_locations", NULL));
  if (*prefixname)
    strcat (prefixname, " ");
  
  if (*locname)
    strcat (locname, " ");

  /* See if we add the "over roomname" to the name...on name_type = 1
     we do add this part. */
  if (nr (0,4) == 1)
    name_type = 1;
  else 
    name_type = 0;
  
  for (bfs = bfs_curr; bfs; bfs = bfs->from)
    {
      if ((room = bfs->room) != NULL)
	{
	  sprintf (buf, "%s %s%s %s%s",		   
		   (*prefixname ?
		    a_an (prefixname) :
		    a_an (bridgename)),
		   prefixname, 
		   bridgename,
		   locname,
		   (*locname ?
		    NAME (room) : ""));
	  
	  *buf = UC(*buf);
	  add_flagval (room, FLAG_ROOM1, ROOM_EASYMOVE);
	  remove_flagval (room, FLAG_ROOM1, BADROOM_BITS);
	  free_str (room->short_desc);
	  room->short_desc = new_str (buf);
	}
    }
  clear_bfs_list();
  return TRUE;
}

/* This adds trees to an area randomizing the height and frequency
   of branches and the size of the branches. */

void 
add_trees (THING *area)
{
  THING *room;
  int num_times, i, j;
  
  if (!area || !IS_AREA (area) || 
      IS_ROOM_SET (area, ROOM_UNDERGROUND | ROOM_DESERT))
    return;
  
  /* Clear marked flags. */
  for (room = area->cont; room; room = room->next_cont)
    {
      RBIT (room->thing_flags, TH_MARKED);
    }
  
  num_times = nr (1, 1+area->mv/DETAIL_DENSITY);

  
  
  /* Find a room and make sure that it's not marked. */
 for (i = 0; i < num_times; i++)
    { 
      for (j = 0; j < 20; j++)
	{
	  if ((room = find_random_room (area, FALSE, ROOM_FOREST, 0)) != NULL 
	      && (!room->name || !*room->name))
	    {
	      /* Mark all rooms within N spaces as marked so we don't 
		 have trees too close together. */
	      mark_rooms_nearby (room, 9);
	      add_tree (room);
	      break;
	    }
	}
    }
  
  for (room = area->cont; room; room = room->next_cont)
    undo_marked (room);
  return;
}


/* This adds a tree to an area above a starting point. */

void
add_tree (THING *base_room)
{
  THING *room, *trunk_room, *new_room, *area;
  int max_height, height, branch_length, dir, dir2;
  int branches = 0, dist;
  int i;
  char color;
  /* How far out we were when we added side branches. */
  int side_branch_depth;
  char treename[STD_LEN];
  char prefixname[STD_LEN];
  char fullname[STD_LEN];
  char belowname[STD_LEN];
  char currname[STD_LEN];
  char branchprefix[STD_LEN];
  char branchadj[STD_LEN];
  /* First make sure were in an area and get the number of free rooms. */
  
  if (!base_room || !IS_ROOM (base_room) || 
      (area = base_room->in) == NULL || !IS_AREA (area))
    return;
  
  /* Now get the height of the tree. */

  max_height = 3 + nr (0,2)+nr (0,2);
  
  /* Don't allow the trees to get too high. */
  if (max_height > area->mv/60)
    max_height = area->mv/60;
  
  /* Once in a while get a supertall tree. */
  if (nr (1,12) == 2)
    max_height += 3;
  
  /* Find out which heights of the tree will have branches. */
  
  for (i = 2; i <= max_height; i++)
    {
      if (IS_SET (branches, (1 << (i-1))) && nr (1,4) == 2)
	SBIT (branches, (1 << i));
      else if (nr (1,3) != 2)
	SBIT (branches, (1 << i));      
    }
  
  /* Make sure at least one branch. */
  if (branches == 0)
    {
      SBIT (branches, 1 << (nr (1,max_height)));
    }
  
  /* Find the tree name. */
  
  strcpy (prefixname, (find_gen_word (AREAGEN_AREA_VNUM,
				      "tree_prefixes", &color)));
  strcpy (treename, (find_gen_word (AREAGEN_AREA_VNUM, 
				    "tree_names", &color)));
  
  if (!*prefixname || !*treename)
    return;
  
  
  sprintf (fullname, "%s %s %s",
	   a_an (prefixname), prefixname, treename);
  /* Make the name of the room below the tree. */
  

  strcpy (belowname, find_gen_word (AREAGEN_AREA_VNUM,
				    "below_tree_name", &color));
  if (*belowname)
    strcat (belowname, " ");
  strcat (belowname, fullname);
  
  free_str (base_room->short_desc);
  base_room->short_desc = new_str (belowname);
  SBIT (base_room->thing_flags, TH_MARKED);
  /* Used for making catwalks. */
  
  append_name (base_room, "tree_bottom");
  
  
  
  trunk_room = base_room;
  for (height = 0; height < max_height; height++)
    {
      /* First move the tree up one level and add the trunk in. */
      /* Describe the trunk */
      
      if (nr (1,2) == 2)
	{
	  switch (nr (1,4))
	    {
	      case 1:
		sprintf (currname, "The trunk of %s", fullname);
		break;
	      case 2:
		sprintf (currname, "%s", fullname);
		break;
	      case 3:
		sprintf (currname, "%s Tree's Trunk", fullname);
		break;
	      case 4:
		sprintf (currname, "%s's Trunk", fullname);
		break;
	    }
	}
      else /* Tell how high up we are in the tree. */
	{
	  if (height < max_height/3)
	    sprintf (currname, "Up in %s%s",
		     fullname, (nr(1,3) == 2 ? " Tree" : ""));
	  else if (height < max_height*2/3)
	    sprintf (currname, "High Up in %s%s",
		     fullname, (nr(1,3) == 2 ? " Tree" : ""));
	  else if (height < max_height - 1)
	    sprintf (currname, "Very High Up in %s%s",
		     fullname, (nr(1,3) == 2 ? " Tree" : ""));
	  else
	    sprintf (currname, "The top of %s%s",
		     fullname, (nr(1,3) == 2 ? " Tree" : ""));
	}
      capitalize_all_words(currname);
      /* Added new trunk room. */
      if ((trunk_room = make_room_dir (trunk_room, DIR_UP, currname, ROOM_FOREST)) == NULL)
	break;
      append_name (trunk_room, "in_tree");
      SBIT (trunk_room->thing_flags, TH_MARKED);
      
      /* Only make branches if we already decided to. */
      if (!IS_SET (branches, (1 << height)))
	continue;
      
      
      if (max_height < 7)
	branch_length = 2-(2*height/max_height);
      else
	branch_length = 3-(3*height/max_height);
      branch_length = MID(1,branch_length,3);
      
      for (dir = 0; dir < FLATDIR_MAX; dir++)
	{	
	  room = trunk_room;
	  /* Set up the branch name. */
	  /* The branch name is kind of complex...sorry. :P */
	  if (nr (1,3) == 2)
	    {
	      switch (nr(1,4))
		{
		  case 1:
		    sprintf (branchprefix, "Out on");
		    break;
		  case 2:
		    sprintf (branchprefix, "On");
		    break;
		  case 3:
		  case 4:
		  default:
		    branchprefix[0] = '\0';
		    break;
		}
	    }
	  else
	    branchprefix[0] = '\0';
	  /* Get the branch adjective. */
	  
	  if (nr (1,3) != 2)
	    strcpy (branchadj, find_gen_word (AREAGEN_AREA_VNUM, "tree_prefixes", &color));
	  else
	    branchadj[0] = '\0';
	  
	  sprintf (currname, "%s%s", branchprefix, (*branchprefix ? " " : ""));
	  if (*branchadj)
	    {
	      strcat (currname, a_an (branchadj));
	      strcat (currname, " ");
	      strcat (currname, branchadj);
	      strcat (currname, " ");
	    }
	  else
	    strcat (currname, "A ");
	  if (nr (1,2) == 2)
	    strcat (currname, "Branch");
	  else
	    strcat (currname, "Limb");
	  
	  if (nr (1,3) == 1)
	    {
	      if (nr (1,3) == 2)
		{
		  if (height < max_height/3)
		    strcat (currname, " Up");
		  else if (height < max_height*2/3)
		    strcat (currname, " High Up");
		  else
		    strcat (currname, " Very High Up");
		}
	      strcat (currname, " in ");
	      if (nr (1,3) == 2)
		strcat (currname, "a Tree");
	      else
		{
		  strcat (currname, fullname);
		  if (nr (1,3) == 1)
		    strcat (currname, " Tree");
		}
	    }
	  
	  side_branch_depth = 0;
	  capitalize_all_words (currname);
	  for (dist = 0; dist < branch_length; dist++)
	    {
	      if ((room = make_room_dir (room, dir, currname, ROOM_FOREST)) == NULL)
		break;
	      append_name (room, "in_tree");
	      SBIT (room->thing_flags, TH_MARKED);
	      if (branch_length == 3 && dist > 0 &&
		  ((side_branch_depth == 0 && nr (1,5) == 2) ||
		  side_branch_depth == dist))
		{
		  side_branch_depth = dist;
		  for (dir2 = 0; dir2 < FLATDIR_MAX; dir2++)
		    {
		      if ((new_room = make_room_dir (room, dir2, currname, ROOM_FOREST)) != NULL)
			SBIT (new_room->thing_flags, TH_MARKED);
		    }
		}
	    }
	}
    }
  return;
}


/* This marks the rooms on the edges of an area with names like
   dir_edge and so forth to make looking for them easier later on. */

void
mark_area_edges (THING *area)
{
  THING *room;
  int grid[AREAGEN_MAX][AREAGEN_MAX]; 
  int extremes[REALDIR_MAX];  
  int x, y;
  char name[STD_LEN];
  
  if (!area || !IS_AREA (area))
    return;
  
  /* Clear grid. */

  for (x = 0; x < AREAGEN_MAX; x++)
    {
      for (y = 0; y < AREAGEN_MAX; y++)
	{
	  grid[x][y] = 0;
	}
    }

  /* Clear extremes. */

  extremes[DIR_NORTH] = -1;
  extremes[DIR_SOUTH] = AREAGEN_MAX;
  extremes[DIR_WEST] = AREAGEN_MAX;
  extremes[DIR_EAST] = -1;


  if ((room = find_random_room (area, FALSE, 0, 0)) == NULL)
    return;
  
  generate_room_grid (room, grid, extremes, AREAGEN_MAX/2, AREAGEN_MAX/2);
  
  for (x = 0; x < AREAGEN_MAX; x++)
    {
      for (y = 0; y < AREAGEN_MAX; y++)
	{
	  if (!grid[x][y] || 
	      (room = find_thing_num (grid[x][y])) == NULL ||
	      !IS_ROOM (room) || room->in != area)
	    continue;
	  
	  sprintf (name, room->name);
	  free_str (room->name);
	  room->name = nonstr;
	  
	  if (x == extremes[DIR_WEST])
	    append_name (room, "west_edge");
	  if (x == extremes[DIR_EAST])
	    append_name (room, "east_edge");
	  if (y == extremes[DIR_NORTH]) 
	    append_name (room, "north_edge");
	  if (y == extremes[DIR_SOUTH]) 
	    append_name (room, "south_edge");
	  
	}      
    }
  undo_marked (room);
  return;
}
  
  
