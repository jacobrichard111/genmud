#include<stdio.h>
#include<ctype.h>
#include<stdlib.h>
#include<string.h>
#include "serv.h"
#include "track.h"
#include "society.h"

/* This lets a thing work on building a city.

   The thing must be in a society and it must be a builder. It must 
   also not be doing anything else, and the society must have built
   a city already (checked by seeing if there are any VAL_CITY
   values in the society rooms.  

   Generally builders will work on a room if the rooms no more than a
   few spaces deep are at least as built up as this room is. If not,
   we move to the least built up room. If all of the rooms are built
   up near where we are, we usually do nothing, but once in a while, 
   we scan out farther to look for undone rooms -- At some point
   I may make this more efficient by checking for undone rooms only
   once every several minutes...but for now this is ok. */

static char *burn_message[BURN_MESSAGE_MAX] =
  {
    "",
    "Singed ",
    "Blackened ",
    "Burned ",
    "Charred ",
    "Melted ",
  };
    



void
do_citybuild (THING *th, char* arg)
{
  VALUE *soc, *build, *worst_build_val = NULL, *exit, *rev_exit;
  SOCIETY *society;
  THING *room = NULL, *nroom = NULL;
  THING *worst_room = NULL;
  int worst_rank = SOCIETY_BUILD_TIERS, dir, i;
  int worst_fire = 0;
  
  if (!th || !th->in || !IS_ROOM (th->in))
    return;
  
  /* Nonpcs have to go through this whole mess of searching for a 
     correct room to build. */
  if (!IS_PC (th))
    {
      if ((soc = FNV (th, VAL_SOCIETY)) == NULL ||
	  !IS_SET (soc->val[2], CASTE_BUILDER) ||
	  (society = find_society_num (soc->val[0])) == NULL)
	return;
  
      clear_bfs_list();
      undo_marked(th->in);
      add_bfs (NULL, th->in, REALDIR_MAX);
      
  
      /* Search all rooms nearby. */
      while (bfs_curr && bfs_curr->depth < 3)
	{
	  /* In each room */
	  if ((room = bfs_curr->room) != NULL &&
	      IS_ROOM (room))
	    {
	      /* If the room is a city room */
	      if ((build = FNV (room, VAL_BUILD)) != NULL)
		{
		  /* See if it belongs to the society and if it's the worst
		 room of the bunch so far. */
		  if (build->val[0] == soc->val[0] &&
		      (build->val[1] < worst_rank ||
		       build->val[4] > worst_fire))
		    {
		      worst_room = room;
		      worst_rank = build->val[1];
		      worst_build_val = build;
		      if (build->val[4] > worst_fire)
			worst_fire = build->val[4];
		    }
		}
	    }
	  
	  /* Add more rooms if needed. */
	  
	  for (dir = 0; dir < REALDIR_MAX; dir++)
	    {
	      if ((exit = FNV (room, dir + 1)) != NULL &&
		  (nroom = find_thing_num (exit->val[0])) != NULL &&
		  is_track_room (nroom, th->move_flags))
		add_bfs (bfs_curr, nroom, dir);
	    }
	  bfs_curr = bfs_curr->next;
	}
      clear_bfs_list();
      
      /* If there's nothing to build. Once in a while go to a random location
	 within the society area. */
      
      if (!worst_room || !worst_build_val ||
	  worst_build_val->val[1] > SOCIETY_BUILD_TIERS)
	{
	  if (nr (1,40) == 3 && !is_hunting (th) &&
	      (room = find_thing_num (nr(society->room_start, society->room_end)))  != NULL &&
	      IS_ROOM (room))
	    {
	      start_hunting_room (th, room->vnum, HUNT_HEALING);
	      if (!hunt_thing (th, 0))
		stop_hunting (th, FALSE);
	    }
	  return;
	}
      /* Otherwise the worst room is not this room, so go to it. */
      if (worst_room && worst_room != th->in)
	{
	  start_hunting_room (th, worst_room->vnum, HUNT_HEALING);
	  if (!hunt_thing (th, 0))
	    stop_hunting (th, FALSE);
	  return;
	}
    }
  else
    {
      if (guild_rank (th, GUILD_TINKER) < 4)
	{
	  stt ("Only experienced tinkers can help to build cities!\n\r", th);
	  return;
	}
      worst_room = th->in;

      /* Make sure this is a valid room to build in. */
      
      if ((worst_build_val = FNV (th->in, VAL_BUILD)) == NULL ||
	  (worst_build_val->val[4] == 0 &&
	   worst_build_val->val[1] == SOCIETY_BUILD_TIERS))
	{
	  stt ("You can't build anything here.\n\r", th);
	  return;
	}
      
      /* Make sure a friendly society owns this room. */
      if ((society = find_society_num (worst_build_val->val[0])) == NULL ||
	  DIFF_ALIGN (society->align, th->align))
	{
	  stt ("This city doesn't belong to an ally!\n\r", th);
	  return;
	}

      /* So for pc's the room is here and its only partially built 
	 and the society it's being built for is an ally of the player. */
      
    }
  /* Make sure worst room is this room. */

  if (worst_room != th->in)
    return;
  
  
  /* If there's a fire here...*/
  
  if (worst_build_val->val[4] > 0)
    {
      worst_build_val->val[4] -= MIN (5, worst_build_val->val[4]);
      
      if (worst_build_val->val[4] == 0)
	{
	  if (IS_ROOM_SET (worst_room, ROOM_FIERY))
	    {
	      act ("$E@1n put@s out the $9fire$e!$7", th, NULL, NULL, NULL, TO_ALL);
	      remove_flagval (worst_room, FLAG_ROOM1, ROOM_FIERY);
	    }
	}
      else
	{
	  act ("@1n work@s on putting out the fire!", th, NULL, NULL, NULL, TO_ALL);
	}
      add_society_reward (th, th->in, REWARD_BUILD, nr (10,20));
      if (IS_PC (th))
	th->pc->wait += 30;
      return;
    }
  
  

  act ("@1n work@s on constructing something.", th, NULL, NULL, NULL, TO_ALL);
  if (worst_build_val->val[1] < SOCIETY_BUILD_TIERS)
    {
      worst_build_val->val[2]++;
      if (worst_build_val->val[2] >= SOCIETY_BUILD_REPEAT)
	{
	  worst_build_val->val[1]++;
	  worst_build_val->val[2] = 0;
	}
      add_society_reward (th, th->in, REWARD_BUILD, nr (10,20));
    }
  
  if (worst_room->in && IS_AREA (worst_room->in))
    worst_room->in->thing_flags |= TH_CHANGED;
  
  
  /* Reward the player. */
  

  if (IS_PC (th))
    th->pc->wait += 30;
  /* Now deal with gates/walls. If you get to the top tier, you place 
     walls around the city if the room has a boundary with another
     area and isn't a guard post. */
      
  /* See if the room is a guard post. If it is, don't put walls
     up. */
  
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    if (society->guard_post[i] == worst_room->vnum)	
      return;
  
  /* Otherwise DO put walls up. */
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      /* A wall gets put up if the exit leads outside of the
	 society area and the room isn't a guard post. */
      
      if ((exit = FNV (worst_room, dir + 1)) != NULL &&
	  (room = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (room) &&
	  (room->vnum < society->room_start ||
	   room->vnum > (society->room_end)))
	{
	  /* exit->val[1] |= EX_WALL; */
	  
	  /* Put the wall going the other way now, too. */
	  
	  if ((rev_exit = FNV (room, RDIR(dir) + 1)) != NULL &&
	      rev_exit->val[0] >= society->room_start &&
	      rev_exit->val[0] <= society->room_end)
	    /*rev_exit->val[1] |= EX_WALL*/;
	}
    }  
  return;
}

/* Shows the name of a room being built to someone looking at it. */
  
char *
show_build_name (THING *target)
{
  VALUE *build;
  int i;
  SOCIETY *soc;
  int burn_rank;
  static char buf[STD_LEN*2]; /* Return buffer. */
  char typebuf[STD_LEN]; /* Size like village/town or barracks/farm etc.... */
  char burnbuf[STD_LEN]; /* Burn amount message. */
  char a_anbuf[10]; /* Where the a_an goes. */
  char socinamebuf[STD_LEN]; /* Society name. */
  char ugroundbuf[STD_LEN]; /* Buffer telling if this is underground or not. */

  if (!target)
    return "Nothing.";
  
  if (!IS_ROOM (target))
    return NAME(target);
  
  if ((build = FNV (target, VAL_BUILD)) == NULL ||
      build->val[1] < 1)
    return NAME (target);
  
  burn_rank = MID (0, build->val[4]/10, BURN_MESSAGE_MAX-1);
  strcpy (burnbuf, burn_message[burn_rank]);
  
  if (IS_ROOM_SET (target, ROOM_UNDERGROUND))
    strcpy (ugroundbuf,  "Underground ");
  else
    ugroundbuf[0] = '\0';
  
  
  if ((soc = find_society_num (build->val[0])) == NULL ||
      build->val[1] > SOCIETY_BUILD_TIERS)
    {
      if (!IS_ROOM_SET (target, ROOM_UNDERGROUND))
	{
	  if (build->val[1] == SOCIETY_BUILD_RUINS)
	    sprintf (buf, "The %sRuins of a City", burnbuf);
	  else if (build->val[1] == SOCIETY_BUILD_RUINS)
	    sprintf (buf, "Some Overgrown %sRuins", burnbuf);
	  else
	    sprintf (buf, "%s %sAbandoned City", burnbuf[0] ? 
		     a_an(burnbuf) : "A", burnbuf);
	}
      else
	{
	  if (build->val[1] == SOCIETY_BUILD_ABANDONED)
	    strcpy (socinamebuf, "Old, Dusty Tunnel");
	  else if (build->val[1] == SOCIETY_BUILD_RUINS)
	    strcpy (socinamebuf, "Abandoned Tunnel");
	  else
	    strcpy (socinamebuf, "Ancient Tunnel");
	  
	  sprintf (buf, "%s %s%s",
		   (*burnbuf ? a_an (burnbuf) : a_an (socinamebuf)),
		   burnbuf, socinamebuf);
	}
      buf[0] = UC(buf[0]);
      return buf;
    }
  
  /* At this point the society exists and the build value represents a
     regular city. */
  
  if (build->val[1] < SOCIETY_BUILD_TIERS)
    {
      if (!IS_ROOM_SET (target, ROOM_UNDERGROUND))
	{
	  if (build->val[1] == 1)
	    sprintf (buf,  "A %sRough Patch of Soil", burnbuf);
	  else if (build->val[1] == 2)	
	    sprintf (buf, "A %sWorked Piece of Ground", burnbuf);
	  else if (build->val[1] == 3)
	    sprintf (buf, "Some %sPrepared Ground", burnbuf); 
	  else
	    sprintf (buf, "A %sBuilding Site", burnbuf);
	}
      else
	{ 
	  if (build->val[1] == 1)
	    sprintf (buf, "A %sRough Hewn Passage", burnbuf);
	  else if (build->val[1] == 2)	
	    sprintf (buf, "A %sWorked Tunnel", burnbuf);
	  else if (build->val[1] == 3)
	    sprintf (buf, "A %sSmooth Tunnel", burnbuf);
	  else
	    sprintf (buf, "A %sReinforced Tunnel", burnbuf);
	}
      return buf;
    }

  /* Now set up the name and info.  */
  
  if (soc->adj && *soc->adj && 
      soc->generated_from != ORGANIZATION_SOCIGEN_VNUM &&
      soc->generated_from != ANCIENT_RACE_SOCIGEN_VNUM)
    sprintf (socinamebuf, "%s ", soc->adj);
  else
    socinamebuf[0] = '\0';
  if (soc->aname && *soc->aname)
    strcat (socinamebuf, soc->aname);
  else
    sprintf (socinamebuf, "Society");
  strcat (socinamebuf, " ");
  
  /* Guard posts. */
  
  typebuf[0] = '\0';
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    {
      if (soc->guard_post[i] == target->vnum)
	{
	  sprintf (typebuf, "Guard Post");
	  break;
	}
    }

  if (!*typebuf)
    strcpy (typebuf, find_city_room_name (soc, build->val[3]));
  typebuf[0] = UC(typebuf[0]);
  
  /* Now set up the a_an for the whole name. The format is

  a/an [Burnbuf] [Underground] [Adjective] [Society name] [Room Type name] */

  if (*burnbuf)
    strcpy (a_anbuf, a_an(burnbuf));
  else if (*ugroundbuf)
    strcpy (a_anbuf, a_an(ugroundbuf));
  else if (*socinamebuf)
    strcpy (a_anbuf, a_an (socinamebuf));
  else 
    strcpy (a_anbuf, a_an (typebuf));
  
  sprintf (buf, "%s %s%s%s%s", a_anbuf, burnbuf, ugroundbuf, socinamebuf, typebuf);
  capitalize_all_words (buf);
  return buf;
}
  

char *show_build_sector (THING *target)
{
  VALUE *build;
  static char buf[STD_LEN];
  char buf2[STD_LEN];
  bool underg = FALSE;
  SOCIETY *soc;
  if (!target || !IS_ROOM (target) ||
      (build = FNV (target, VAL_BUILD)) == NULL ||
      build->val[1] < 1)
    return "";
  
  if (IS_ROOM_SET (target, ROOM_UNDERGROUND))
    {
      strcpy (buf2, "Underground");
      underg = TRUE;
    }
  else if ((soc = find_society_num (build->val[0])) == NULL ||
      build->val[1] > SOCIETY_BUILD_TIERS)
    {
      if (build->val[1] == SOCIETY_BUILD_ABANDONED)
	strcpy (buf2, "City");
      else if (build->val[1] == SOCIETY_BUILD_RUINS)
	strcpy (buf2, "Ruins");
      else if (build->val[1] == SOCIETY_BUILD_OVERGROWN)
	strcpy (buf2, "Overgrown");
    }
  else if (build->val[1] <= SOCIETY_BUILD_TIERS - 2)
    strcpy (buf2, "Dirt");
  else if (build->val[1] == SOCIETY_BUILD_TIERS - 1)
    strcpy (buf2, "Construction");
  else
    strcpy (buf2, find_city_room_name (soc, build->val[3]));
  
  sprintf (buf, "\x1b[1;34m[\x1b[%d;3%dm%s\x1b[1;34m]\x1b[0;37m",
	   (underg ? 1 : 0),
	   (underg ? 0 : build->val[1] <= 3 ? 3 : 7),
	   buf2);  
  return buf;
}

/* This shows the build description for the player. */

char *show_build_desc (THING *target)
{
  static char buf[STD_LEN];
  VALUE *build;
  SOCIETY *soc;
  if (!target || !IS_ROOM (target) ||
      (build = FNV (target, VAL_BUILD)) == NULL ||
      build->val[1] < 1)
    return "";
  
  if ((soc = find_society_num (build->val[0])) == NULL ||
      build->val[1] > SOCIETY_BUILD_TIERS)
    {
      if (IS_ROOM_SET (target, ROOM_UNDERGROUND))
	return "These are some abandoned tunnels.\n\r";      
      return "These are the ruins of an abandoned city.\n\r";
    }
  
  if (IS_ROOM_SET (target, ROOM_UNDERGROUND))
    return ("The tunnel continues off into the darkness.\n\r");
  else if (build->val[1] <= SOCIETY_BUILD_TIERS-2)
    return ("This patch of ground is being worked.\n\r");
  else if (build->val[1] == SOCIETY_BUILD_TIERS-1)
    return ("This is a construction site.\n\r");
  
  if (soc->aname && *soc->aname)
    {
      sprintf (buf, "This is part of %s %s %s.\n\r", a_an(soc->aname), 
	       soc->aname,
	       find_city_room_name (soc, build->val[3]));
    }
  else
    sprintf (buf, "This is part of %s %s.\n\r", a_an(find_city_room_name(soc, build->val[3])), find_city_room_name (soc, build->val[3]));
  return buf;
}





/* This sets up the city that the society will live in.
   Right now its REAL boring. */


void
setup_city (SOCIETY *soc)
{
  THING *room; 
  VALUE *build;
  SOCIETY *oldsoc;
  int num, i, real_caste_max, castes_in_society = 0;
  
  /* Different kinds of caste houses out there. */
  bool has_caste_house[CASTE_MAX];
  
  if (!soc)
    return;
  
  for (i = 0; caste1_flags[i].flagval != 0; i++);
    
  
  real_caste_max = MIN (i, CASTE_MAX);
    
  
  /* Find all castes in this society. */
  
  for (i = 0; i < CASTE_MAX; i++)
    castes_in_society |= soc->cflags[i];


  for (i = 0; i < real_caste_max; i++)
    {      
      /* If members of a certain caste aren't in a society, we don't
	 need to build a house, so set having that house to true. */
      if (!IS_SET (castes_in_society, caste1_flags[i].flagval))	
	has_caste_house[i] = TRUE;
      else
	has_caste_house[i] = FALSE;
    }
  
  for (num = soc->room_start; num <= soc->room_end; num++)
    {
      if ((room = find_thing_num (num)) == NULL ||
	  !IS_ROOM (room) ||
	  IS_ROOM_SET (room, BADROOM_BITS))
	continue;
      
      if ((build = FNV (room, VAL_BUILD)) == NULL)
	{
	  build = new_value();
	  add_value (room, build);
	  build->type = VAL_BUILD;
	  build->val[6] = 0;
	  build->val[0] = soc->vnum;
	}

      /* If the society that made this is gone, don't keep it
	 around. */
      
      if ((oldsoc = find_society_num (build->val[0])) == NULL ||
	  build->val[1] >  SOCIETY_BUILD_TIERS)
	{
	  build->val[0] = soc->vnum;
	  build->val[1] = 0;
	  build->val[2] = 0;
	  build->val[3] = 0;
	  build->val[6] = 0;
	}
      
      
      if (room->in && IS_AREA (room->in))
	room->in->thing_flags |= TH_CHANGED;
      
      if (build->val[3])
	{
	  for (i = 0; i < real_caste_max; i++)
	    {
	      if (IS_SET (build->val[3], caste1_flags[i].flagval))
		has_caste_house[i] = TRUE;
	    }
	}      
    }
  
  /* Now setting up caste parts of the city consists of making the
     various kinds of castes have different homes. */
  
  for (i = 0; i < real_caste_max; i++)
    {
      if (has_caste_house[i] == FALSE)
	setup_caste_house (soc, caste1_flags[i].flagval);
    }

  return;
}


/* This sets up a block of rooms to be the caste house
   for a caste within the society rooms. */

void
setup_caste_house (SOCIETY *soc, int caste_flags)
{
  THING *room, *nroom;
  VALUE *exit, *build, *nbuild;
  int vnum, i, vnum2;
  /* Used to find the number of room choices we have to work with. */
  int count, num_choices = 0, num_chose = 0, choice = 0;
  /* Used to find the number of free rooms adjacent to this room
     we can work with. */
  
  int free_exits, dir;
  if (!soc || caste_flags == 0)
    return;
  
  
  /* Unmark all rooms. */
  
  for (vnum = soc->room_start; 
       vnum <= soc->room_end;
       vnum++)
    {
      if ((room = find_thing_num (vnum)) != NULL && IS_ROOM (room))
	RBIT (room->thing_flags, TH_MARKED);
    }
  
  /* First, if this is a "Farm" setup, we really set up all farm
     rooms in fields that aren't used and are in the society. 
     Also some underground rooms get set to be farms, also. */
  
  if (caste_flags == CASTE_FARMER)
    {
      for (vnum = soc->room_start; vnum <= soc->room_end; vnum++)
	{
	  if ((room = find_thing_num (vnum)) != NULL && IS_ROOM(room) &&
	      (build = FNV (room, VAL_BUILD)) != NULL &&
	      build->val[0] == soc->vnum && build->val[3] == 0 &&
	      (IS_ROOM_SET (room, ROOM_FIELD | ROOM_UNDERGROUND) 
	       && nr (1,5) == 3))
	    build->val[3] = CASTE_FARMER;
	}
      return;
    }
	      
  
  /* AFTER THIS WE MUST UNMARK ALL SOCIETY ROOMS BEFORE BAILING OUT! */

  /* Mark all guard posts as unusable. */
  
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    {
      if (soc->guard_post[i] &&
	  (room = find_thing_num (soc->guard_post[i])) != NULL &&
	  IS_ROOM (room))
	SBIT (room->thing_flags, TH_MARKED);
    }
  
  /* Now mark all rooms that are either not in the city's control, or
     are used already. */
  
  
  for (vnum =  soc->room_start; vnum <= soc->room_end; vnum++)
    {
      if ((room = find_thing_num (vnum)) == NULL ||
	  !IS_ROOM (room))
	continue;
      
      if ((build = FNV (room, VAL_BUILD)) == NULL ||
	  build->val[0] != soc->vnum ||
	  build->val[3] ||
	  IS_ROOM_SET (room, (BADROOM_BITS & ~ROOM_MOUNTAIN)))
	SBIT (room->thing_flags, TH_MARKED);
    }
  
  /* Now find all rooms with at least 3 unmarked rooms next to them
     (unless they only have 2 exits, then 2 is ok, and use them as
     starting points. */
  
  for (count = 0; count < 2; count++)
    {
      for (vnum = soc->room_start; vnum <= soc->room_end; vnum++)
	{
	  if ((room = find_thing_num (vnum)) == NULL ||
	      !IS_ROOM (room) || 
	      IS_SET (room->thing_flags, TH_MARKED) ||
	      (build = FNV (room, VAL_BUILD)) == NULL ||
	      build->val[0] != soc->vnum ||
	      build->val[3])
	    continue;
	  
	  free_exits = 0;
	  
	  /* Check and see if each room has >= 2 exits and if
	     both free if it has 2, or 3 or more free if it has
	     three exits. Free being an exit to a room in the society
	     area that isn't used yet. */

	  for (dir = 0; dir < REALDIR_MAX; dir++)
	    {
	      if ((exit = FNV (room, dir + 1)) == NULL ||
		  exit->val[0] == room->vnum ||
		  exit->val[0] < soc->room_start ||
		  exit->val[0] > soc->room_end ||
		  (nroom = find_thing_num (exit->val[0])) == NULL ||
		  !IS_ROOM (nroom) || IS_MARKED (nroom) ||
		  (nbuild = FNV (nroom, VAL_BUILD)) == NULL ||
		  nbuild->val[0] != soc->vnum ||
		  nbuild->val[3] != 0)
		continue;
	      
	      free_exits++;
	    }
	  
	  /* If not enough exits or free exits, don't add this room
	     to the list. */
	  
	  if (free_exits < 2)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (++choice >= num_chose)
	    break;
	}
      if (count == 0)
	{
	  if (num_choices == 0)
	    break;
	  num_chose = nr (1, num_choices);
	}
    }
  

  /* If there are no choices, clear all marked rooms. */
  
  for (vnum2 = soc->room_start; vnum2 <= soc->room_end; vnum2++)
    {
      if ((room = find_thing_num (vnum2)) != NULL)
	RBIT (room->thing_flags, TH_MARKED);
    }
  
  
  if (num_choices == 0)
    return;
  
  /* A screwup if it returns here. */

  if ((room = find_thing_num (vnum)) == NULL ||
      (build = FNV (room, VAL_BUILD)) == NULL ||
      build->val[0] != soc->vnum ||
      build->val[3])
    return;
  
  /* Small dfs here. */
  
  if (IS_SET (caste_flags, CASTE_WIZARD | CASTE_HEALER))
    add_flagval (room, FLAG_ROOM1, ROOM_EXTRAMANA | ROOM_EXTRAHEAL);
  setup_caste_house_rooms (room, soc, caste_flags, REALDIR_MAX, 4);
  
  return;

}

/* This marks rooms near a certain room to be caste rooms. It counts
   down from the max depth used to minimal depth. */

void
setup_caste_house_rooms (THING *room, SOCIETY *soc, int caste_flags, int dir_from, int depth_left)
{
  THING *nroom;
  VALUE *exit, *build, *nbuild;
  int dir;
  /* If this room is bad, don't set it up. Just return. */
  
  if (depth_left < 0 || !soc || !caste_flags || 
      !room || !IS_ROOM (room) || IS_MARKED (room) ||
      room->vnum < soc->room_start ||
      room->vnum > soc->room_end ||
      (build = FNV (room, VAL_BUILD)) == NULL ||
      build->val[0] != soc->vnum ||
      build->val[3] != 0)
    return;
  
  /* If there's an adjacent caste house of another type, 
     we want to stop making this chouse. Only do this AFTER the first
     room has been set down. */
  
  if (dir_from != REALDIR_MAX)
    {
      for (dir = 0; dir < REALDIR_MAX; dir++)
	{
	  if ((exit = FNV (room, dir + 1)) != NULL &&
	      (nroom = find_thing_num (exit->val[0])) != NULL &&
	      IS_ROOM (nroom) &&
	      (nbuild = FNV (nroom, VAL_BUILD)) != NULL &&
	      nbuild->val[0] == soc->vnum &&
	      nbuild->val[3] != 0 &&
	      nbuild->val[3] != caste_flags)
	    return;
	}
    }
  
  build->val[3] = caste_flags;




  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      /* Don't let this go 2 rooms in the same dir...keeps the shape more
	 like a 3x3 square (cube) than a diamond. */
      
      if (dir == dir_from)
	continue;
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL)
	setup_caste_house_rooms (nroom, soc, caste_flags, dir, depth_left-1);      
    }		
}	

/* This returns the relative size of a city based on the society's
   recent max population. */

char *
find_city_room_name (SOCIETY *soc, int flags)
{
  int i;
  if (!soc)
    return "City";
  
  if (flags)
    {
      for (i = 0; caste1_flags[i].flagval != 0; i++)
	{
	  if (IS_SET (flags, caste1_flags[i].flagval))
	    return caste1_flags[i].help;
	}
    }

  if (soc->recent_maxpop < soc->population_cap/10)
    return "Outpost";
  else if (soc->recent_maxpop < soc->population_cap/5)
    return "Village";
  else if (soc->recent_maxpop < soc->population_cap/3)
    return "Town";
  else if (soc->recent_maxpop < soc->population_cap*2/3)
    return "City";
  else if (soc->recent_maxpop < soc->population_cap*9/10)
    return "Citadel";
  return "Fortress";
}


/* If a society that built up a certain area becomes defeated, its rooms
   slowly decay back into wilderness. */

void
update_built_room (THING *room)
{
  SOCIETY *soc, *soc2;
  VALUE *build, *exit, *build2, *exit2;
  bool decay = FALSE;
  THING *room2;

  if (!room || !IS_ROOM (room) ||
      (build = FNV (room, VAL_BUILD)) == NULL)
    return;

  

  /* Val4 is how burnt the room is. It can be undone with 
     repairs from builders. */

  if (IS_ROOM_SET (room, ROOM_FIERY))
    {
      build->val[4]++;
    }
  
  /* The room decays if the society goes away. */
  
  if ((soc = find_society_num (build->val[0])) == NULL ||
      (soc->population == 0 && soc->base_chance[0] == 0 &&
       soc->chance[0] == 0) ||
      room->vnum < soc->room_start || 
      room->vnum > soc->room_end)
    decay = TRUE;
  /* Room is regular room, so it's ok. */

  if (!decay)
    {
      build->val[6] = 0;
      return;
    }
  
  
  /* Remove the build sector from a society. (if it's still with one.) */
  build->val[0] = 0;
  
  /* Change the room type to ruins if it's still a regular room rank. */
  
  if (build->val[1] <= SOCIETY_BUILD_TIERS)
    build->val[1] = SOCIETY_BUILD_ABANDONED;
  
  /* Let the room decay more...*/

  if (++build->val[6] > SOCIETY_BUILD_DECAY_HOURS)
    {
      build->val[1]++;
      build->val[6] = 0;
    }
  
  /* After a long time, remove the building anyway. */
  
  if (build->val[1] > SOCIETY_BUILD_OVERGROWN)
    {
      remove_flagval (room, FLAG_ROOM1, ROOM_EXTRAMANA | ROOM_EXTRAHEAL);
      remove_value (room, build);
    }
  
  
  /* In a decaying city, the walls go down when the city becomes
     ruins or overgrown. */
  
  else if (build->val[6] >= SOCIETY_BUILD_RUINS &&
	   build->val[6] == 0)
    {
      /* Go down list of exits. */
      
      for (exit = room->values; exit; exit = exit->next)
	{
	  if (exit->type >0 && exit->type <= REALDIR_MAX)
	    {
	     
	      if ((room2 = find_thing_num (exit->val[0])) == NULL ||
		  (exit2 = FNV (room2, RDIR(exit->type))) != NULL ||
		  exit2->val[0] != room->vnum)
		continue;
	      
	       /* Now the exit leads to another room, and this other
		 room has an exit leading back to this room. If it
		 doesn't have a good build val, nuke the walls. */
	      
	      if ((build2 = FNV (room2, VAL_BUILD)) == NULL ||
		  (soc2 = find_society_num (build2->val[0])) == NULL ||
		  (soc2->population == 0 && soc2->base_chance[0] == 0 &&
		   soc2->chance[0] == 0) ||
		  room2->vnum < soc2->room_start || 
		  room2->vnum > soc2->room_end)
		{
		  
		  /* If the room has a temp wall, remove it. */
		  
		  if (!IS_SET (exit->val[2], EX_WALL) &&
		      IS_SET (exit->val[1], EX_WALL))
		    RBIT (exit->val[1], EX_WALL);
		  
		  if (!IS_SET (exit2->val[2], EX_WALL) &&
		      IS_SET (exit2->val[1], EX_WALL))
		    RBIT (exit2->val[1], EX_WALL);
		}
	    }
	}
    }
					       
  return;
}
  
  
/* This is used to reduce damage to a society member who is inside of
   their home city that's already built. */

int
society_defensive_damage_reduction (THING *vict, int dam)
{
  VALUE *socval, *build;
  SOCIETY *soc;
  
  if (!vict || !vict->in ||
      (socval = FNV (vict, VAL_SOCIETY)) == NULL ||
      (build = FNV (vict->in, VAL_BUILD)) == NULL ||
      build->val[1] != SOCIETY_BUILD_TIERS ||
      socval->val[0] != build->val[0] ||
      (soc = find_society_num (socval->val[0])) == NULL ||
      soc->recent_maxpop < 1)
    return 0;
  
  /* The formula is to take the damage and multiply it by
     the recent_maxpop/population_cap, to give a rough percentage
     of how "full" the society is, and then multiply that by
     1/4 so you get a 25 percent reduction in the damage for a fully
     developed society. The whole point behind the odd order of operations
     is just to keep integer divides from losing fractions if possible. */

  return (dam*soc->recent_maxpop*25/(MAX(100,soc->population_cap)))/100;
}



/* The raze command lets you destroy a city of an enemy if you happen
   to be in the room. It also gives a small chance of making the
   room catch fire, and the fires can spread from room to room. :) */

void
do_raze (THING *th, char *arg)
{
  THING *room;
  VALUE *build;
  FLAG *flg;
  SOCIETY *soc;
  VALUE *socval;
  bool ok_to_raze = FALSE;

  if (!th || (room = th->in) == NULL || !IS_ROOM(room))
    {
      stt ("You can't raze anything here\n\r", th);
      return;
    }

  if ((build = FNV (room, VAL_BUILD)) == NULL ||
      (soc = find_society_num (build->val[0])) == NULL)
    {
      stt ("This isn't a city you can raze.\n\r", th);
      return;
    }
      
  socval = FNV (room, VAL_SOCIETY);
  
  /* Now see if this built up room is owned by an enemy society or not. */

  /* If the society is align 0 and you aren't in the society, you can
     raze here. Otherwise if the alignment is > 0, you must be of a 
     different alignment to raze here. */
  
  if (soc->align == 0 &&
      (!socval || socval->val[0] != soc->vnum))
    ok_to_raze = TRUE;
  else if (soc->align > 0 &&
	   DIFF_ALIGN (th->align, soc->align))
    ok_to_raze = TRUE;

  if (!ok_to_raze)
    {
      stt ("You can't raze the city of an ally!\n\r", th);
      return;
    }

  /* Reduce the build_tier of the room by 1 if a society still owns
     the room, otherwise destroy the built room. */

  if (build->val[1] <= SOCIETY_BUILD_TIERS)
    build->val[1]--;
  else
    build->val[1] = 0;
  
  if (build->val[1] <= 0)
    {
      build->val[0] = 0;
      update_built_room (room);
    }
  
  /* Now give a small chance for a fire to start. */
  
  if (nr (1,23) == 11)
    {
      flg = new_flag();
      flg->type = FLAG_ROOM1;
      flg->from = 2000;
      flg->val = ROOM_FIERY;	
      flg->timer = nr (15, 40);
      aff_update (flg, room);
      set_up_map_room (room);
    }
  
  act ("@1n raze@s the city!", th, NULL, NULL, NULL, TO_ALL);
  if (IS_PC (th))
    th->pc->wait += 50;

  add_society_reward (th, th->in, REWARD_RAZE, 20);
  
  return;
}
  
