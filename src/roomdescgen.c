#include<stdio.h>
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#include "serv.h"
#include "mapgen.h"
#include "society.h"
#include "areagen.h"
#include "track.h"
#include "roomdescgen.h"

/* I was inspired to write a lot of this after seeing something that
   Dodinas trice@bigpond.net.au wrote on TMC. */

/* These are things that litter the ground in a room. */

static const char *roomdesc_litter[ROOMDESC_LITTER_MAX] =
  {
    "pebbles",
    "stones",
    "rocks",
    "smooth pebbles",
    "sharp pebbles", /* 5 */

    "sharp rocks",
    "sharp stones",
    "jagged pebbles",
    "jagged rocks",
    "leaves",  /* 10 */

    "rotten leaves",
    "brown leaves",
    "fallen leaves",
    "pine needles",
    "pine cones",   /* 15 */

    "branches",
    "twigs",
    "shoots",
    "tufts of grass",
    "dead grass",   /* 20 */

    "decaying plants",
    "fragrant plants",
    "feathers",
    "patches of fur",
    "clumps of fur",  /* 25 */

    "pieces of fur",
    "sand piles",
    "gravel pieces",
    "snail shells",
    "shells",   /* 30 */

    "seedlings",
    "insect shells",
    "carapices",
    "animal droppings",
    "animal leavings", /* 35 */

    "dusty patches", 
    "berries",
    "rotten berries", 
    "wood chips",
    "chips of wood",/* 40 */
    
    "pieces of wood",
    "bark chips",
    "strips of bark",
    "pieces of bark",
    "wood chunks",   /* 45 */

    "bark chunks",
    "chunks of wood", 
    "acorns",
    "rotting acorns",
    "seeds",   /* 50 */
    
    "sprouting plants",  /* 51 */
  };


static const char *roomdesc_litter_amount[ROOMDESC_LITTER_AMOUNTS_MAX] =
  {
    "some",
    "a few",
    "a scattering of",
    "a smattering of", 
    "several",    /* 5 */

    "a small amount of",
    "a small number of",
    "a handful of",
    "a fair amount of",
    "a number of",   /* 10 */

    "small amounts of",
    "small numbers of", /* 12 */
    
    
  };

static const char *roomdesc_litter_suffix[ROOMDESC_LITTER_SUFFIX_MAX] =
  {
    "are here. ",
    "are on the ground. ",
    "are all over the ground. ",
    "are scattered about. ",
    "are lying here. ",  /* 5 */

    "are around. ",
    "litter the ground. ", 
    "lie on the ground. ", /* 8 */
  };


/* Temperatures the ground can have. */

static const char *roomdesc_ground_temp[ROOMDESC_GROUND_TEMP_MAX] =
  {
    "burning hot",
    "very hot",
    "hot",
    "very warm",
    "warm",  /* 5 */

    "comfortable",
    "",
    "cool",
    "very cool",
    "cold",  /* 10 */

    "very cold",
    "freezing cold",
    "frigid", /* 13 */
  };

/* The hardness the ground can have. */

static const char *roomdesc_ground_hardness[ROOMDESC_GROUND_HARDNESS_MAX] =
  {
    "squishy",
    "spongy",
    "soft",
    "packed",
    "sandy",
    
    "loose",
    "hard",
    "solid",
    "rock-solid",  /* 9 */
  };

static const char *roomdesc_ground_dryness[ROOMDESC_GROUND_DRYNESS_MAX] =
  {
    "liquid",
    "soaked",
    "muddy",
    "wet",
    "damp", /* 5 */

    "slightly damp",
    "dry",
    "dried out",
    "dessicated",
    "cracked",  /* 10 */
  };

/* This creates a description for a room. It's not very good but 
   I don't really look at room descs anyway. :) */

char *
generate_room_description (THING *room)
{
  static char retbuf[STD_LEN*10];
  THING *area;
  THING *croom, *nroom;
  
  THING *nearby_rooms[REALDIR_MAX][SCAN_RANGE];
  /* Randpop is used to show bones or signs of some animal here. */
  VALUE *exit, *randpop;
  THING *randpop_item, *mob;

  int dir, range, i;
  int area_sectors;
  int room_sectors;
  int hours_since_last_rain; /* Hours since last rain. Used on ground temp/
		    hardness/dryness. */
  int ground_temp, ground_hardness, ground_dryness, groundstart;
  char areaname[STD_LEN];
  char roomname[STD_LEN];
  char nroomname[STD_LEN];
  char groundprefix[STD_LEN];
  char buf[STD_LEN];
  char *t;
  int temp;
  int dir_order[REALDIR_MAX]; /* What order the info about nearby
				 rooms is given. */
  int choice;
  /* Used to generate this on the fly but exactly the same for each
     room each time. */
  int rand_num, old_rand, litter_choice, litter_choice2;
  retbuf[0] = '\0';
  if (!room || (area = room->in) == NULL || 
      !IS_ROOM (room) || !IS_AREA (area))
    return nonstr;
  
  area_sectors = flagbits (area->flags, FLAG_ROOM1);
  room_sectors = flagbits (room->flags, FLAG_ROOM1);
  
  if (!area_sectors || !room_sectors )
    return nonstr;

  rand_num = ((area->vnum << 3) + (room->vnum << 1));
  
  old_rand = random();
  my_srand(rand_num);
  
  /* Figure out what order the nearby room descs will be given in. */
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    dir_order[dir] = 0;
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      choice = nr (1, REALDIR_MAX - dir);
      for (i = 0; i < REALDIR_MAX; i++)
	{
	  if (dir_order[i] == 0 &&
	      --choice == 0)
	    {
	      dir_order[i] = dir;
	      break;
	    }
	}
    }
  

  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      for (range = 0; range < SCAN_RANGE; range++)
	{
	  nearby_rooms[dir][range] = NULL;
	}
    }
  
  /* Find the nearby rooms. They can't have a hidden door or wall along the
     way or it blocks all viewing past that location. */
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      croom = room;
      for (range = 0; range < SCAN_RANGE; range++)
	{
	  if ((exit = FNV (croom, dir + 1)) != NULL &&
	      !IS_SET (exit->val[2] | exit->val[1], EX_HIDDEN | EX_WALL) &&
	      (nroom = find_thing_num (exit->val[0])) &&
	      nroom->in == area)
	    {	
	      nearby_rooms[dir][range] = nroom;
	      croom = nroom;
	    }
	  else
	    break;
	}
    }
  
  strcpy (areaname, NAME (area));
  strcpy (roomname, show_build_name(room));
  
  for (t = areaname; *t; t++)
    *t = LC (*t);
  for (t = roomname; *t; t++)
    *t = LC (*t);
  t--;
  if (!str_cmp (roomname, areaname))
    {
      if (nr (1,2) == 2)
	sprintf (buf, "You are %s %s. ", 
		 (nr (1,2) == 2 ? "in" : "at"),
		 roomname);
      else
	{
	  sprintf (buf, "%s %s. ",
		   (*t == 's' ? "These are" : "This is"),
		   roomname);
	}
    }
  else
    {
      if (nr (1,3) == 1)
	sprintf (buf, "You are %s %s in %s. ",
		 (nr (1,2) == 2 ? "in" : "at"),
		 roomname, areaname);
      else 
	{
	  sprintf (buf, "%s %s in %s. ",
		   (*t == 's' ? "These are" : "This is"),
		   roomname, areaname);
	}
    }
  strcat (retbuf, buf);

  if (!IS_ROOM_SET (room, BADROOM_BITS))
    {
      /* Add info about the type of ground. Adjust it based on the kind
	 of room. */
      
      /* Get the last rain component. */

      if (wt_info && wt_info->val[WVAL_LAST_RAIN] < 10)
	{
	  hours_since_last_rain = MID (-2, (12 - wt_info->val[WVAL_LAST_RAIN])/3, 2);
	}
      else
	hours_since_last_rain = 0;
      
      groundstart = ROOMDESC_GROUND_TEMP_MAX/3;
      
      if (IS_SET (room_sectors, ROOM_DESERT | ROOM_SWAMP))
	groundstart += 2;
      if (IS_SET (area_sectors, ROOM_DESERT | ROOM_SWAMP))
	groundstart++;
      if (IS_SET (room_sectors, ROOM_ROUGH | ROOM_MOUNTAIN | ROOM_UNDERGROUND))
	groundstart -= 2;
      if (IS_SET (area_sectors, ROOM_ROUGH | ROOM_MOUNTAIN | ROOM_UNDERGROUND))
	groundstart--;
      
      if (IS_SET (room_sectors, ROOM_SNOW))
	groundstart -= 4;
      if (IS_SET (area_sectors, ROOM_SNOW))
	groundstart -= 2;
   
      /* Temp goes down when it rains. */

      
      ground_temp = nr (groundstart, groundstart + ROOMDESC_GROUND_TEMP_MAX/3);
      ground_temp -= hours_since_last_rain;
      
      /* Temp also affected by the overall outside temp. */

      if (wt_info)
	{
	  ground_temp -= (60 - wt_info->val[WVAL_TEMP])/20;
	}
      ground_temp = MID (0, ground_temp, ROOMDESC_GROUND_TEMP_MAX-1);
      
      /* Get the ground hardness. */
     
      groundstart = ROOMDESC_GROUND_HARDNESS_MAX/3;
      
      if (IS_SET (room_sectors, ROOM_SWAMP))
	groundstart -= 3;
      if (IS_SET (room_sectors, ROOM_SNOW))
	groundstart--;
      if (IS_SET (area_sectors, ROOM_SWAMP | ROOM_SNOW))
	groundstart--;
      
      if (IS_SET (room_sectors, ROOM_DESERT))
	groundstart += 3;
      if (IS_SET (area_sectors, ROOM_DESERT))
	groundstart++;

      
      
      ground_hardness = nr (groundstart, groundstart + ROOMDESC_GROUND_HARDNESS_MAX/3); 
      /* Hardness goes down when it rains. */
      
      ground_hardness -= hours_since_last_rain;
     
      ground_hardness = MID (0, ground_hardness, ROOMDESC_GROUND_HARDNESS_MAX-1);
      
      /* Get the ground dryness. */

      groundstart = ROOMDESC_GROUND_DRYNESS_MAX/3;
      
      if (IS_SET (room_sectors, ROOM_DESERT))
	groundstart += 3;
      if (IS_SET (area_sectors, ROOM_DESERT))
	groundstart += 1;
      
      if (IS_SET (room_sectors, ROOM_SNOW))
	groundstart -= 2;
      if (IS_SET (area_sectors, ROOM_SNOW))
	groundstart--;

      
      if (IS_SET (room_sectors, ROOM_SWAMP))
	groundstart -= 2;
      if (IS_SET (area_sectors, ROOM_SWAMP))
	groundstart--;
      

      
      ground_dryness = nr (groundstart, groundstart + ROOMDESC_GROUND_DRYNESS_MAX/3);
      /* Dryness goes down when it rains. */
      
      ground_dryness -= hours_since_last_rain;
      ground_dryness = MID (0, ground_dryness, ROOMDESC_GROUND_DRYNESS_MAX-1);
      
      if (nr (1,5) == 3)
	strcpy (groundprefix, "It appears that ");
      else if (nr (1,4) == 2)
	strcpy (groundprefix, "You notice that ");
      else if (nr (1,3) == 3)
	strcpy (groundprefix, "You notice ");
      else if (nr (1,2) == 1)
	strcpy (groundprefix, "");
      else 
	strcpy (groundprefix, "It looks like ");
   
	

      /* Format: The (temp) ground is (dryness) and (hardness). */
      
      sprintf (buf, "%sthe %s %s %sis %s and %s and ",
	       groundprefix,
	       roomdesc_ground_temp[ground_temp],
	       (nr (1,3) == 3 ? "ground" : 
		(nr(1,2) == 1 ? "soil" : "earth")),
	       (nr (1,6) == 2 ? "beneath your feet " : 
		(nr (1,5) == 3 ? "here " : " ")),
	       roomdesc_ground_hardness[ground_hardness],
	       roomdesc_ground_dryness[ground_dryness]);
      
      /* Now add the litter to the ground. */
      
      strcat (buf, roomdesc_litter_amount[nr(0, ROOMDESC_LITTER_AMOUNTS_MAX-1)]);
      litter_choice = nr (0, ROOMDESC_LITTER_MAX-1);
      strcat (buf,  " ");
      strcat (buf, roomdesc_litter[litter_choice]);
      if (nr (1,3) == 2)
	strcat (buf, " ");
      else
	{
	  strcat (buf, " and ");
	  strcat (buf, roomdesc_litter_amount[nr(0,ROOMDESC_LITTER_AMOUNTS_MAX-1)]);
	  strcat (buf, " ");
	  
	  while ((litter_choice2 = nr (0, ROOMDESC_LITTER_MAX-1)) == 
		 litter_choice);
	  
	  strcat (buf, roomdesc_litter[litter_choice2]);
	  strcat (buf, " ");
	}
      
      strcat (buf, roomdesc_litter_suffix[nr(0, ROOMDESC_LITTER_SUFFIX_MAX-1)]);
      buf[0] = UC(buf[0]);      
      strcat (retbuf, buf);
    }
  /* Show the signs or corpse of a mob here. */
  if (nr (1,4) != 2)
    {
      if ((randpop_item = find_thing_num (MOB_RANDPOP_VNUM)) != NULL &&
	  (randpop = FNV (randpop_item, VAL_RANDPOP)) != NULL &&
	  (mob = find_thing_num (calculate_randpop_vnum (randpop, LEVEL(area)))) != NULL &&
	  LEVEL (mob) >= 15 &&
	  (flagbits(mob->flags, FLAG_ROOM1) == 0 ||
	   IS_ROOM_SET (mob, (room_sectors | area_sectors))))
	{
	  sprintf (buf, "%s %s of %s here. ",
		   (nr (1,3) == 2 ? "There are some" : 
		    (nr (1,2) == 2 ? "There appear to be some" : "Some")),
		   (nr (1,5) == 3 ? "signs" :
		    (nr (1,4) == 2 ? "tracks" :
		     (nr (1,3) == 3 ? "bones" :
		      (nr (1,2) == 1 ? "leavings" : "droppings")))),
		   NAME (mob));
	  buf[0] = UC(buf[0]);
	  strcat (retbuf, buf);
	}
    }
  
  /* Now give the nighttime/seasonal desc. TAKE THIS OUT IF YOU STOP
     DYNAMICALLY GENERATING THESE DESCRIPTIONS! */

  /* First get the temperature. */

  temp = find_room_temperature (room);
  
  
  if (temp > 120)
    strcat (retbuf, "It is blistering hot here");
  else if (temp < 0)
    strcat (retbuf, "It is freezing cold here");
  else switch (temp/10)
    {
      case 11:
	strcat (retbuf, "It is burning hot here");
	break;
      case 10:
	strcat (retbuf, "It is extremely hot here");
	break;
      case 9:
	strcat (retbuf, "It is very hot here");
	break;
      case 8:
	strcat (retbuf, "It is very warm here");
	break;
      case 7:
	strcat (retbuf, "It is warm here");
	break;
      case 6:
      case 5:
      default:
	strcat (retbuf, "It is comfortable here");
	break;
      case 4:
	strcat (retbuf, "It is cool here");
	break;
      case 3:
	strcat (retbuf, "It is cool here");
	break;
      case 2:
	strcat (retbuf, "It is cold here");
	break;
      case 1:
      case 0:
	strcat (retbuf, "It is very cold here");
	break;
    }
  
  if (wt_info && !IS_ROOM_SET (room, ROOM_WATERY | ROOM_FIERY | ROOM_EARTHY | ROOM_INSIDE | ROOM_UNDERGROUND | ROOM_UNDERWATER))
    {
      int hour = wt_info->val[WVAL_HOUR];
      bool is_night = FALSE;
      
      if (hour >= HOUR_NIGHT || 
	  hour < HOUR_DAYBREAK)
	is_night = TRUE;
      
      /* Add the space from the previous temp sentence. */

      strcat (retbuf, " and");

      switch (wt_info->val[WVAL_WEATHER])
	{
	  case WEATHER_SUNNY:
	  default:
	    if (is_night)
	      strcat (retbuf, " the night sky is filled with stars. ");
	    else
	      {
		if (hour - HOUR_DAYBREAK < 4)
		  strcat (retbuf, " the sun is rising off to the west. ");
		else if (HOUR_NIGHT - hour < 4)
		  strcat (retbuf, " the sun is setting in the east. ");
		else
		  strcat (retbuf, " the sun is high overhead. ");
	      }
	    break;
	  case WEATHER_CLOUDY:
	    if (is_night)
	      strcat (retbuf, " the night sky is overcast and dreary. ");
	    else
	      strcat (retbuf, " some cloudy skies loom overhead. " );
	    break;
	  case  WEATHER_RAINY:
	    if (IS_ROOM_SET (room, ROOM_DESERT))
	      break;
	    if (!IS_ROOM_SET (room, ROOM_SNOW))
	      strcat (retbuf, " rainy skies overhead are drenching the ground. ");
	    else
	      strcat (retbuf, " the snow is coming down peacefully. ");
	    break;
	  case WEATHER_STORMY:
	    if (IS_ROOM_SET (room, ROOM_DESERT))
	      strcat (retbuf, " a squall overhead soaks the desert. ");
	    else if (IS_ROOM_SET (room, ROOM_SNOW) ||
		     wt_info->val[WVAL_TEMP] < 30)
	      strcat (retbuf, " a blizzard blankets the earth with a large amount of snow. ");
	    else
	      strcat (retbuf, " stormy skies above turn the ground into a muddy morass. ");
	    break;
	  case WEATHER_FOGGY:
	    if (IS_ROOM_SET (room, ROOM_DESERT))
	      break;
	    if (is_night)
	      strcat (retbuf, " fog obscures the starry skies overhead. ");
	    else
	      strcat (retbuf, " The sun tries to peek through the foggy sky. ");
	    break;
	}
    }
  else
    strcat (retbuf, ". ");
  /* Now give the desc for nearby rooms. */
  
	  
  for (i = 0; i < REALDIR_MAX; i++)
    {
      dir = dir_order[(i + range) % REALDIR_MAX];
      
      
      /* Display nearby rooms and _some_ rooms farther away. */
      
      for (range = 0; range < SCAN_RANGE; range++)
	{
	  if (nr (0, range*2) > 0 ||
	      (nroom = nearby_rooms[dir][range]) == NULL)
	    continue;
	  
	  
	  strcpy (nroomname, show_build_name(nroom));
	  if (!*nroomname)
	    continue;
	  for (t = nroomname; *t; t++)
	    *t = LC (*t);
	  t--;
	  if (!str_cmp (roomname, nroomname))
	    {
	      /* t should be in the last char of the name so if
		 it's an s it's plural so no continues otherwise
		 it's singular so we do continues. */
	      if (nr (1,4) == 1)
		{
		  sprintf (buf, "%s%s%s%s, %s continue%s. ",
			   dir_dist[range], (*dir_dist[range] ? " " : ""),
			   (dir < FLATDIR_MAX ? "to " : ""),
			   dir_track[dir],
			   nroomname,
			   (*t == 's' ? "" : "s"));
		}
	      else if (nr (1,3) == 2)
		{
		  sprintf (buf, "%s %s %s %s%s. ",			   
			   nroomname,			   
			   (nr (1,3) == 2 ? "is" :
			    (*t == 's' ? "continue" : "continues")),
			 
			   dir_dist[range], 
			   (dir < FLATDIR_MAX ? "to " : ""),
			   dir_track[dir]);
		  
		}
	      else
		{
		  sprintf (buf, "%s %s %s %s %s%s. ",
			   (nr (1,3) == 3 ? "more of" :
			    nr (1,2) == 2 ? "some more of" : 
			    "still more of"),
			   nroomname, 
			   (*t == 's' ?
			    (nr (1,4) == 2 ? "are visible" :
			     (nr (1,3) == 1 ? "can be seen" : 
			      (nr (1,2) == 2 ? "are" : "appear to be"))) :
			    (nr (1,4) == 2 ? "is visible" :
			     (nr (1,3) == 1 ? "can be seen" : 
			      (nr (1,2) == 2 ? "is" : "appears to be")))),
			   dir_dist[range],
			   (dir < FLATDIR_MAX ? "to " : ""),
			   dir_track[dir]);
		}
	    }
	  else
	    {
	      if (nr (1,3) == 2)
		{
		  sprintf (buf, "%s%s%s%s, %s %s. ",
			   dir_dist[range], (*dir_dist[range] ? " " : ""),
			   dir < FLATDIR_MAX ? "to " : "",
			   dir_track[dir],
			   (nr (1,5) == 2 ? 
			    (*t == 's' ? "there are" : "there is") :
			    nr (1,4) == 1 ? "it looks like" : 
			    nr (1,3) == 2 ? "there appears to be" :
			    nr (1,2) == 1 ? "you see" : "you notice"),		
			   nroomname);
		}
	      else
		{
		  sprintf (buf, "%s %s %s %s%s. ",
			   (nr (1,5) == 2 ? 
			    (*t == 's' ? "there are" : "there is") :
			    nr (1,4) == 1 ? "it looks like" : 
			    nr (1,3) == 2 ? "there appears to be" :
			    nr (1,2) == 1 ? "you see" : "you notice"),
			   nroomname,
			   dir_dist[range],
			   (dir < FLATDIR_MAX ? "to " : ""),
			   dir_track[dir]);
		}
	    }
	  buf[0] = UC(buf[0]);
	  strcat (retbuf, buf);
	  if (dark_inside (room))
	    break;
	}   
    }
  if (dark_inside(room))
    strcat (retbuf, " It's too dark to see much farther. ");
  format_string (NULL, retbuf);
  my_srand(old_rand);
  return retbuf;
}
  
