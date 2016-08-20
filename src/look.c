#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "event.h"
#include "roomdescgen.h"


void
show_thing_to_thing (THING *th, THING *target, int flags)
{
  char buf[STD_LEN];
  char buf2[STD_LEN];
  char posbuf[STD_LEN];
  int color, bright, roomflags;
  bool show_built_room = FALSE;
  VALUE *build;
  buf[0] = '\0';
  buf2[0] = '\0';
  posbuf[0] = '\0';
  if (!target)
    return;
  if (IS_PC (target))
    {
      bright = 1;
      color = 5;
    }
  else
    {
      color = 6;
      bright = CAN_MOVE (target);
    }
  
  if ((build = FNV (target, VAL_BUILD)) != NULL &&
      build->val[0] >= 1 && build->val[1] >= 1)
    show_built_room = TRUE;
  else
    show_built_room = FALSE;
  
  if (IS_SET (flags, LOOK_SHOW_SHORT))
    {
      sprintf (buf2, "\x1b[%d;37m%s%s %s\x1b[0;37m", IS_ROOM(target) ? 1 : 0,
	       (IS_WORN (target) ?
		wear_data[target->wear_loc].view_name : ""),
	       show_build_name (target),
	       (!IS_SET (flags, LOOK_SHOW_TYPE) && th ==
		target->in) ?  show_condition (target, FALSE) : "");
      strcat (buf, buf2);
      
      if (IS_ROOM (target) || (target->in && IS_AREA (target->in)))
	{
	  if (show_built_room)
	    strcat (buf, show_build_sector (target));
	  else
	    {
	      roomflags = flagbits (target->flags, FLAG_ROOM1);
	      if(!IS_SET (roomflags, ROOM_UNDERWATER | ROOM_INSIDE | ROOM_UNDERGROUND | ROOM_EARTHY))
		{
		  sprintf (buf2, "\x1b[1;34m[\x1b[0;37m%s\x1b[1;34m%s]\x1b[0;37m",
			   ((find_room_temperature (target) > 30 ||
			    wt_info->val[WVAL_WEATHER] < WEATHER_RAINY) ?
			    weather_names[wt_info->val[WVAL_WEATHER]] : "Snow"),
			   (wt_info->val[WVAL_HOUR] > NUM_HOURS - 6 ||
			    wt_info->val[WVAL_HOUR] < 6) ? "/\x1b[1;30mNight\x1b[1;34m" : "");
		}
	      else if (IS_SET (roomflags, ROOM_EARTHY))
		sprintf (buf2, "\x1b[1;34m[\x1b[1;30mSolid Rock\x1b[1;34m]\x1b[0;37m");  
	      else if (IS_SET (roomflags, ROOM_UNDERWATER))
		sprintf (buf2, "\x1b[1;34m[\x1b[0;34mUnderwater\x1b[1;34m]\x1b[0;37m");
	      else if (IS_SET (roomflags, ROOM_UNDERGROUND))
		sprintf (buf2, "\x1b[1;34m[\x1b[1;30mUnderground\x1b[1;34m]\x1b[0;37m");  	     
	      else 
		sprintf (buf2, "\x1b[1;34m[\x1b[0;37mInside\x1b[1;34m]\x1b[0;37m");
	      strcat (buf, buf2);
	      if (IS_SET (roomflags, ROOM_FIERY))
		strcat (buf, " \x1b[0;31m(\x1b[1;31mBurning\x1b[0;31m)\x1b[0;37m");
	    }
	}
      else if (IS_SET (flags, LOOK_SHOW_TYPE))
	{
	  RACE *race;
	  if (IS_PC (target) && 
	      (race = FRACE (target)) != NULL)
	    {
	      sprintf (buf2, "is a proud member of the %s race.",
		       race->name);
	    }
	  else
	    sprintf (buf2, "is some sort of %s.",
		     (target->proto  && target->proto->type &&
		      *target->proto->type ?
		      target->proto->type : "thing"));
	  strcat (buf, buf2);
	  strcat (buf, show_condition (target, TRUE));
	}
      strcat (buf, "\n\r");
      stt (buf, th);      
    }
  
  if (IS_SET (flags, LOOK_SHOW_DESC))
    {
      THING *curr_tar, *curr_tar2;
      if (IS_ROOM (target) && 
	  (show_built_room || 
	   (!target->desc || !*target->desc)))
	stt (generate_room_description (target), th);
      else
	{
	  if ((curr_tar = target->proto) == NULL)
	    curr_tar = target;
	  if ((curr_tar2 = find_thing_num (atoi (curr_tar->desc))) != NULL)
	    curr_tar = curr_tar2; 
	  if (curr_tar && curr_tar->desc && *curr_tar->desc)
	    stt (curr_tar->desc, th);
	}
    }
  if (IS_SET (flags, LOOK_SHOW_EXITS))
     show_exits (th, target);
  if (IS_ROOM (target) && target->hp && target->in != th->in)
    show_blood (th, target);
  if (IS_SET (flags, LOOK_SHOW_CONTENTS) && !IS_AREA (target))
    {
      VALUE *lid;
      if ((lid = FNV (target, VAL_EXIT_O)) == NULL ||
	  !IS_SET (lid->val[1], EX_DOOR) ||
	  !IS_SET (lid->val[1], EX_CLOSED))	
	show_contents_list (th, target, flags);      
    }
  return;
}

void
show_exits (THING *th, THING *in)
{
  char buf[STD_LEN];
  char buf2[STD_LEN];
  char buf3[STD_LEN];
  VALUE *exit;
  int door, roomflags;
  THING *room;
  int exit_color = 7; /* Last digit of the exit color...to make it
			 easier to read. 4 = regular blue. */
  buf[0] = '\0';
  buf2[0] = '\0';  
  if (!th || !in)
    return;
  
  if (IS_ROOM (in))
    {
      for (door = 1; door <= DIR_MAX; door++)
	{
	  if ((exit = FNV (in, door)) == NULL ||
	      (room = find_thing_num (exit->val[0])) == NULL ||
	      !IS_ROOM (room))
	    continue;
	  roomflags = flagbits (room->flags, FLAG_ROOM1);
	  buf3[0] = '\0';
	  if (IS_SET (exit->val[1], EX_WALL))
	    {
	      if (!IS_PC1_SET (th, PC_HOLYLIGHT))
		continue;
	      sprintf (buf3, "\x1b[1;32m |%s|", dir_name[door - 1]);
	    }
	  else if (IS_SET (exit->val[1], EX_DOOR) && 
		   IS_SET (exit->val[1], EX_CLOSED))
	    { 
	      if (IS_SET (exit->val[1], EX_HIDDEN))
		{
		  if (!IS_PC1_SET (th, PC_HOLYLIGHT))
		    continue;
		  sprintf (buf3, "\x1b[1;3%dm (%s)", exit_color, dir_name[door -1]);
		}
	      else
		sprintf (buf3, "\x1b[1;3%dm [%s]", exit_color, dir_name[door -1]);
	    }
	  else if (IS_SET (roomflags, ROOM_AIRY))
	    {
	      sprintf (buf3, "\x1b[1;36m %s ",  dir_name[door -1]);
	    }
	  else if (IS_SET (roomflags, ROOM_FIERY))
	    {
	      sprintf (buf3, "\x1b[1;31m *%s*",  dir_name[door -1]); 
	    }
	  else if (IS_SET (roomflags, ROOM_WATERY))
	    {
	      sprintf (buf3, "\x1b[1;34m ~%s~", dir_name[door -1]); 
	    }
	  else if (IS_SET (roomflags, ROOM_EARTHY))
	    {
	      sprintf (buf3, "\x1b[0;33m #%s#", dir_name[door -1]); 
	    }
	  else
	    sprintf (buf3, "\x1b[1;3%dm %s", exit_color, dir_name[door -1]);
	  
	  strcat (buf, buf3);
	}
    }
  if (buf[0])
    {
      sprintf (buf2, "\x1b[1;34m[Exits:%s\x1b[1;34m]\x1b[0;37m\n\r", buf);
    }
  else
    {
      sprintf (buf2, "\x1b[1;34m[Exits: None.]\x1b[0;37m\n\r");
    }
  stt (buf2, th);
  return;
}


void 
do_exits (THING *th, char *arg)
{
  THING *room;
  VALUE *exit;
  int door;
  char buf[STD_LEN];
  bool found = FALSE;

  if (!IS_ROOM (th->in))
    {
       stt ("Exits: None.\n\r", th);
       return;
    }  
  for (door = 1; door <= DIR_MAX; door++)
    {
      if ((exit = FNV (th->in, door)) == NULL ||
          (room = find_thing_num (exit->val[0])) == NULL ||
	  !IS_ROOM (room) ||
	  /* No holylight means you can't see past closed, hidden
	     doors. */
	  (!IS_PC1_SET (th, PC_HOLYLIGHT) &&
	   !IS_SET (~(exit->val[1]),
		    EX_DOOR | EX_CLOSED | EX_HIDDEN)))
	continue;
      if (!found)
        {
          stt ("Exits:\n\r", th);
          found = TRUE;
        }
      sprintf (buf, "%-6s-- %s\n\r", dir_name[door - 1], 
	       show_build_name (room));
      buf[0] = UC(buf[0]);
      stt (buf, th);
    }
  if (!found)
    {
      stt ("Exits: None.\n\r", th);
      return;
    }
  return;
}


void
do_look (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  THING *look_at = NULL, *map_room, *portal_room, *obj;
  int flags = 0, dir = DIR_MAX;
  VALUE *map, *exit, *portal;
  EDESC *eds;
  if (!th || !th->in || !th->fd)
    return;  
  arg = f_word (arg, arg1);
  if (th->position == POSITION_SLEEPING)
    {
      stt ("You cannot look while you are sleeping.\n\r", th);
      return;
    }
  if (!IS_PC1_SET (th, PC_HOLYLIGHT) && IS_HURT (th, AFF_BLIND))
    {
      stt ("\x1b[1;31mYou're blind!\x1b[0;37m\n\r", th);
      return;
    }

  if ((dir = find_dir (arg1)) < REALDIR_MAX && IS_ROOM (th->in) &&
      (exit = FNV (th->in, dir + 1)) != NULL &&
      (look_at = find_thing_num (exit->val[0])) != NULL &&
      IS_ROOM (look_at) && !IS_SET (exit->val[1], EX_WALL) &&
      (!IS_SET (exit->val[1], EX_DOOR) || !IS_SET (exit->val[1], EX_CLOSED)))
    {
      if (!IS_DET (th, AFF_DARK) && dark_inside (look_at) &&
	  !IS_PC1_SET (th, PC_HOLYLIGHT))
	{
	  stt ("\x1b[1;30mIt's too dark over there to see anything.\x1b[0;37m\n\r", th);
	  return;
	}
      sprintf (buf, "You look %s and see....\n\r", dir_name[dir]);
      stt (buf, th);
      show_thing_to_thing (th, look_at, LOOK_SHOW_SHORT | LOOK_SHOW_EXITS | LOOK_SHOW_CONTENTS | LOOK_SHOW_DESC);
      return;
    }
  /* Look here...autolook when moving. */
  if (arg1[0] == '\0' || !str_cmp (arg1, "zzduhql"))
    {
      look_at = th->in;
      flags = LOOK_SHOW_SHORT |
	LOOK_SHOW_EXITS | LOOK_SHOW_CONTENTS;
      /* If we "look zzduhql" and have brief on, we don't see desc. */
      if ((str_cmp (arg1, "zzduhql") || !IS_PC2_SET (th, PC2_BRIEF)))
	flags |= LOOK_SHOW_DESC;
    }  
  /* Look outside if we're in something. */
  else if (!str_cmp (arg1, "out"))
    {
      if (th->in && th->in->in &&
	  !IS_SET (th->in->in->thing_flags, TH_IS_AREA) && 
	  th->in->in != the_world)
	{
	  look_at = th->in->in;
	  flags = LOOK_SHOW_SHORT | LOOK_SHOW_DESC | 
	    LOOK_SHOW_EXITS | LOOK_SHOW_CONTENTS;
	}
      else
	{
	  stt ("You can't seem to see out of here.\n\r", th);
	  return;
	}
    }
  /* Look inside of an object. */
  else if (!str_cmp (arg1, "i") || 
	   !str_cmp (arg1, "in") ||
	   !str_cmp (arg1, "inside") )
    {
      if ((look_at = find_thing_here (th, arg, TRUE)) == NULL)
	{
	  stt ("You don't see that here to look into!\n\r", th);
	  return;
	} /* Look through portals...gotta love 8 lines to do this :P */
      if ((portal = FNV (look_at, VAL_EXIT_I)) != NULL &&
	  (portal_room = find_thing_num (portal->val[0])) != NULL &&
	  IS_ROOM (portal_room))
	{
	  act ("You look through @2n and see...", th, look_at, NULL, NULL, TO_CHAR);
	  flags = LOOK_SHOW_SHORT |
	    LOOK_SHOW_EXITS | LOOK_SHOW_CONTENTS | LOOK_SHOW_DESC;
	  look_at = portal_room;
	}
      else
	flags = LOOK_SHOW_SHORT | LOOK_SHOW_TYPE | LOOK_SHOW_CONTENTS;
    }
  else if ((look_at = find_thing_here (th, arg1, FALSE)) == NULL)
    {
      /* Check extra descriptions on all things here. */
      if ((eds = find_edesc_thing (th->in, arg1, FALSE)) != NULL)
	{
	  sprintf (buf, "You examine %s...\n\r", NAME(th->in));
	      stt (buf, th);
	  stt (eds->desc, th);
	  return;
	}
      for (obj = th->in->cont; obj; obj = obj->next_cont)
	{
	  if (can_see (th, obj) && 
	      (eds = find_edesc_thing (obj, arg1, FALSE)) != NULL)
	    {  
	      sprintf (buf, "You examine %s...\n\r", NAME(obj));
	      stt (buf, th);
	      stt (eds->desc, th);
	      return;
	    }
	}
      for (obj = th->cont; obj; obj = obj->next_cont)
	{
	  if (can_see (th, obj) && 
	      (eds = find_edesc_thing (obj, arg1, FALSE)) != NULL)
	    {  
	      sprintf (buf, "You examine %s...\n\r", NAME(obj));
	      stt (buf, th);
	      stt (eds->desc, th);
	      return;
	    }
	}
      stt ("You don't see that here to look at!\n\r", th);
      return;
    }
  else if ((map = FNV (look_at, VAL_MAP)) != NULL &&
	   (map_room = find_thing_num (map->val[0])) != NULL &&
	   IS_ROOM (map_room))
    {
      
      create_map (th, map_room, MAP_MAXX, MAP_MAXY);
      return;
    }
  else
    flags = LOOK_SHOW_SHORT | LOOK_SHOW_TYPE |
      /* peek? */  LOOK_SHOW_CONTENTS | LOOK_SHOW_DESC;
  
  if (!IS_PC1_SET (th, PC_HOLYLIGHT) && 
      dark_inside (look_at) && !IS_DET (th, AFF_DARK))
    {
      if (IS_ROOM_SET (look_at, ROOM_INSIDE | ROOM_UNDERGROUND | ROOM_UNDERWATER))
	stt ("\x1b[1;30mIt's too dark in here to see anything.\x1b[0;37m\n\r", th);
      else
	stt ("\x1b[1;30mIt's too dark out here to see anything.\x1b[0;37m\n\r", th);	
    return;
    }
  
  show_thing_to_thing (th, look_at, flags);
  return;
}

void
do_inventory (THING *th, char *arg)
{
  show_contents_list (th, th, LOOK_SHOW_TYPE);
  return;
}

void 
do_equipment (THING *th, char *arg)
{
  THING *obj;
  bool found = FALSE;
  stt ("You are using:\n\n\r", th);
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if (IS_WORN(obj))
	{
	  found = TRUE;
	  show_thing_to_thing (th, obj, LOOK_SHOW_SHORT);
	}
    }
  if (!found)
    stt ("Nothing.\n\r", th);
  
  return;
}



char *
show_condition (THING *th, bool long_descr)
{
  static char buf[STD_LEN];
  VALUE *light, *gem, *power;
  int num, objflags;
  buf[0] = '\0';
  objflags = flagbits (th->flags, FLAG_OBJ1);
  if (!th || th->max_hp < 1)
    return "";
  if (th->hp >= 200000000 && th->max_hp >= 200000000)
    {
      num = MID (0, (th->hp/30) * (CONDITIONS-1)/(th->max_hp/30), CONDITIONS-1);
    }
  else
   num = MID (0, th->hp * (CONDITIONS-1)/th->max_hp, CONDITIONS-1);
  
  if (long_descr)
    {
      if (CAN_FIGHT (th) || CAN_MOVE (th))
	sprintf (buf, "\n\r%s %s", 
		 (th->sex == SEX_MALE ? "He" :
		  th->sex == SEX_FEMALE ? "She" : "It"),
		 mob_condition[num]);
      else
	sprintf (buf, "\n\rIt %s.", obj_condition[num]);
      return buf;
    }
  
  sprintf (buf, " %s ", obj_short_condition[num]);
  
  if ((light = FNV (th, VAL_LIGHT)) != NULL &&
      IS_SET (light->val[2], LIGHT_LIT))
    {
      if (light->val[1] == 0 || light->val[0] > 9)
	strcat (buf, " \x1b[1;33m(Brightly Lit)\x1b[0;37m");
      else if (light->val[0] > 6)
	strcat (buf, " \x1b[1;33m(Lit)\x1b[0;37m");
      else if (light->val[0] > 3)
	strcat (buf, " \x1b[0;37m(Dimly Lit)");
      else strcat (buf, " \x1b[1;30m(Almost Out)");
    }
  if ((gem = FNV (th, VAL_GEM)) != NULL && gem->val[1] >= 0)
    {
      switch (gem->val[1]/10)
	{
	case 0:
	case 1:
	  strcat (buf, " \x1b[1;30m(very dim)\x1b[0;37m");
	  break;
	case 2:
	case 3:
	  strcat (buf, " \x1b[1;30m(dim)\x1b[0;37m");
	  break;
	case 4:
	case 5:
	  strcat (buf, " (soft glow)");
	  break;
	case 6:
	case 7: 
	  strcat (buf, " (glowing)");
	  break;
	case 8:
	case 9:
	  strcat (buf, " \x1b[1;37m(bright glow)\x1b[0;37m");
	  break;
	case 10:
	default:
	  strcat (buf, " \x1b[1;37m(extremely bright)\x1b[0;37m");
	  break;
	}
    }
  
  if ((power = FNV (th, VAL_POWERSHIELD)) != NULL && power->val[1] >= 0)
    {
      switch (power->val[1]/10)
	{
	  case 0:
	case 1:
	  strcat (buf, " \x1b[1;30m(very dim)\x1b[0;37m");
	  break;
	case 2:
	case 3:
	  strcat (buf, " \x1b[1;30m(dim)\x1b[0;37m");
	  break;
	case 4:
	case 5:
	  strcat (buf, " (soft glow)");
	  break;
	case 6:
	case 7: 
	  strcat (buf, " (glowing)");
	  break;
	case 8:
	case 9:
	  strcat (buf, " \x1b[1;34m(bright glow)\x1b[0;37m");
	  break;
	case 10:
	default:
	  strcat (buf, " \x1b[1;34m(extremely bright)\x1b[0;37m");
	  break;
	}
    }
  if (IS_SET (objflags, OBJ_MAGICAL))
    strcat (buf, "\x1b[0;35m(\x1b[0;36mMagical\x1b[0;35m)\x1b[0;37m");  
  if (IS_SET (objflags, OBJ_GLOW))
    {
      if (IS_SET (objflags, OBJ_HUM))
	strcat (buf, "\x1b[1;37m..it Glows and Hums with power!\x1b[0;37m");
      else
	strcat (buf, "\x1b[1;37m..it Glows with energy!\x1b[0;37m");
    }
  else if (IS_SET (objflags, OBJ_HUM))
    strcat (buf, "\x1b[1;37m..it Hums with power!\x1b[0;37m");
  
  return buf;
}



void
show_blood (THING *th, THING *room)
{
  char buf[STD_LEN];
  int i;
  if (!IS_ROOM (room))
    return;
  for (i = 0; i < REALDIR_MAX; i++)
    {
      if (IS_SET (room->hp, (1 << i)))
	{
	  sprintf (buf, "\x1b[0;31mThere is a trail of blood leading \x1b[1;31m%s\x1b[0;31m.\x1b[0;37m\n\r", dir_name[i]);
	  stt (buf, th);
	}
    }
  if (IS_SET (room->hp, BLOOD_POOL))
    stt ("\x1b[0;31mThere is a pool of \x1b[1;31mblood\x1b[0;31m here.\x1b[0;37m\n\r", th);
  return;
}
  
/* Simple scan function. */  

void
do_scan (THING *th, char *arg)
{
  char buf[STD_LEN*3];
  int dir, dist;
  VALUE *exit;
  THING *croom, *nroom, *vict;
  bool seen = FALSE;
  
  if (!th || !th->in || !IS_ROOM (th->in))
    {
      stt ("You can't scan here.\n\r", th);
      return;
    }

  if (FIGHTING(th)) 
    {  
      stt ("You can't scan while you're fighting.\n\r", th);
      return;
    }
  
  if (th->position == POSITION_SLEEPING)
    {
      stt ("You cannot scan while you are sleeping.\n\r", th);
      return;
    }

  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      croom = th->in;
      for (dist = 0; dist < 3; dist++)
	{
	  if ((exit = FNV (croom, dir + 1)) != NULL &&
	      (nroom = find_thing_num (exit->val[0])) != NULL &&
	      IS_ROOM (nroom) && !IS_SET (exit->val[1], EX_WALL) &&
	      (!IS_SET (exit->val[1], EX_DOOR) || 
	       !IS_SET (exit->val[1], EX_CLOSED)))
	    {
	      buf[0] = '\0';
	      for (vict = nroom->cont; vict != NULL; vict = vict->next_cont)
		{
		  if (can_see (th, vict) &&
		      (CAN_MOVE (vict) || CAN_FIGHT (vict)))
		    {
		      seen = TRUE;
		      if (!buf[0])
			sprintf (buf, "%s %s from here you see %s",dir_dist[dist], dir_name[dir], name_seen_by (th, vict));
		      else if (strlen (buf) < STD_LEN * 2)
			sprintf (buf + strlen(buf), ", %s", name_seen_by (th, vict));
		    }
		}
	      if (buf[0])
		{
		  
		  strcat (buf, ".\n\r");
		  buf[0] = UC (buf[0]);
		  stt (buf, th);
		  break;
		}
	      croom = nroom;
	    }
	}
    }
  if (!seen)
    {
      stt ("Nothing.\n\r", th);
    }
  return;
}

/* Looks for secret doors/hidden things */

void
do_search (THING *th, char *arg)
{
  char buf[STD_LEN];
  int dir;
  VALUE *exit = NULL;
  bool hidden_door = FALSE;
  THING *room;
  
  if (!IS_PC (th) || !th->in)
    return;

  
  if (IS_ROOM (th->in))
    {
      for (exit = th->in->values; exit != NULL; exit = exit->next)
	{
	  if (exit->type > 0 && exit->type <= REALDIR_MAX && IS_SET (exit->val[1], EX_HIDDEN))
	    {
	      hidden_door = TRUE;
	      break;
	    }
	}
    }
	  
  if (!str_cmp (arg, "donttypethisinnodplzthanks"))
    {
      if (hidden_door)
	stt ("There's a hidden door here someplace...\n\r", th);
      return;
    }

  dir = find_dir (arg);
  
  if (dir != DIR_MAX && IS_ROOM (th->in))
    {
      exit = FNV (th->in, dir + 1);
      if (exit && IS_SET (exit->val[1], EX_WALL))
	exit = NULL;
    }
  
  
  if (!DOING_NOW)
    {
      stt ("You begin to search the room...\n\r", th);
      sprintf (buf, "search %s", arg);
      add_command_event (th, buf, 5*UPD_PER_SECOND/2);
      th->position = POSITION_SEARCHING;
      return;
    }

  if (exit == NULL)
    {
      if ((nr (1,9) == 2 || check_spell (th, "search", 0)) && nr (1,3) != 2)
	{
	  if (IS_ROOM (th->in))
	    {
	      for (exit = th->in->values; exit != NULL; exit = exit->next)
		{
		  if (exit->type > 0 && exit->type <= REALDIR_MAX &&
		      IS_SET (exit->val[1], EX_HIDDEN) &&
		      IS_SET (exit->val[1], EX_CLOSED) &&
		      IS_SET (exit->val[1], EX_DOOR) &&
		      (room = find_thing_num (exit->val[0])) != NULL &&
		      IS_ROOM (room))
		    {
		      hidden_door = TRUE;
		      break;
		    }
		}
	    }
	  if (hidden_door)
	    {
	      stt ("There's a hidden door here someplace...\n\r", th);
	      return;
	    }
	  else
	    {
	      stt ("You don't see anything unusual here...\n\r", th);
	      return;
	    }
	}
      stt ("You don't see anything unusual here...\n\r", th);
      return;
    }
  if (!IS_SET (exit->val[1], EX_HIDDEN) ||
      !IS_SET (exit->val[1], EX_DOOR) ||
      !IS_SET (exit->val[1], EX_CLOSED))
    {
      stt ("That exit isn't hidden!\n\r", th);
      return;
    }
  if ((nr (1,10) == 2 || check_spell (th, "search", 0)) &&
      nr (1,2) == 2)
    {
      sprintf (buf, "You have found a %s %s from here!\n\r", *exit->word ?
	       exit->word : "Error NO NAME!!!", dir_name[dir]);
      stt (buf, th);
      return;
    }
  stt ("You don't see anything unusual here...\n\r", th);
  return;
  
}

/* Finds out what killed a corpse */

void
do_investigate (THING *th, char *arg)
{
  THING *corpse;
  char buf[STD_LEN];
  if (!IS_PC (th))
    return;

  if (th->pc->prac[301 /* Investigate */] < 20)
    {
      stt ("You don't know how to investigate a corpse!\n\r", th);
      return;
    }
  
  if (!*arg)
    corpse = find_thing_in (th, "corpse");
  else
    corpse = find_thing_in (th, arg);
  
  if (!corpse || corpse->vnum != CORPSE_VNUM)
    {
      stt ("You don't see that here to investigate!\n\r", th);
      return;
    }


  if (!DOING_NOW)
    {
      
      stt ("You begin to investigate the corpse...\n\r", th);
      sprintf (buf, "investigate %s", arg);
      add_command_event (th, buf, 9*UPD_PER_SECOND/2);
      th->position = POSITION_INVESTIGATING;
      return;
    }
  
  if (check_spell (th, "investigate", 0))
    {
      sprintf (buf, "The murderer is %s.\n\r", 
	       (corpse->type && *corpse->type ? corpse->type : "unknown"));
      stt (buf, th);
      return;
    }
  
  stt ("The murderer is unknown.\n\r", th);
  return;
  

}


/* Makes a steak out of a corpse. */


void
do_butcher (THING *th, char *arg)
{
  THING *steak, *corpse, *knife;
  char buf[STD_LEN];
  char arg1[STD_LEN];
  bool has_knife = FALSE;
  VALUE *wpn;
  int num_steaks, i;
  
  if (!IS_PC (th) || !th->in)
    return;


  if (!*arg)
    corpse = find_thing_in (th, "corpse");
  else
    corpse = find_thing_in (th, arg);
  
  if (!corpse || corpse->vnum != CORPSE_VNUM)
    {
      stt ("I don't see that corpse here!\n\r", th);
      return;
    }

  if (IS_SET (corpse->thing_flags, TH_NO_TAKE_BY_OTHER))
    {
      stt ("You cannot butcher player corpses.\n\r", th);
      return;
    }


  for (knife = th->cont; knife != NULL; knife = knife->next_cont)
    {
      if (!IS_WORN(knife))
	break;
      
      if (knife->wear_loc == ITEM_WEAR_WIELD)
	{
	  if ((wpn = FNV (knife, VAL_WEAPON)) != NULL &&
	      wpn->val[2] == WPN_DAM_PIERCE)
	    {
	      has_knife = TRUE;
	      break;
	    }
	}
    }
  
  if (!has_knife)
    {
      stt ("You must be wielding a piercing weapon to butcher a corpse!\n\r", th);
      return;
    }
  
  num_steaks = nr (1, LEVEL (corpse)/20 + 1);
  
  if (!check_spell (th, "butcher", 0) || nr (1,9) == 3)
    {
      act ("@1n tries to butcher a steak, but ends up cutting @1f!", th, NULL, NULL, NULL, TO_ROOM);
      stt ("You mess up and cut yourself...\n\r", th);
      damage (knife, th, LEVEL (knife) + LEVEL (th), "cut");
      return;
    }
  act ("$8@1n butchers @3n, making some yummy steaks...MMMMM", th, NULL, corpse, NULL, TO_ALL);
  
  f_word (corpse->name, arg1);

  for (i = 0; i < num_steaks; i++)
    {
      if ((steak = create_thing (STEAK_VNUM)) != NULL)
	{
	  free_str (steak->name);
	  steak->name = new_str ("steak yummy");
	  sprintf (buf, "\x1b[0;31mA yummy-looking steak\x1b[0;37m");
	  free_str (steak->short_desc);
	  steak->short_desc = new_str (buf);
	  free_str (steak->long_desc);
	  steak->long_desc = new_str ("A delicious-looking steak is sitting on the ground slowly rotting.");
	  steak->timer = nr (10,25);
	  thing_to (steak, th->in);
	}
    }
  
  /* We destroy all stuff inside the corpse when it is butchered! */
  
  free_thing (corpse);
  
  
  return;

}

/* This lists the contents of a thing. It also does shop inventory. */

void
show_contents_list (THING *th, THING *target, int flags)
{  
  char listbuf[LBUF_SIZE][STD_LEN]; 
  int listnum[LBUF_SIZE], cost[LBUF_SIZE];
  char buf[STD_LEN*2];
  char buf2[STD_LEN*2];
  char posbuf[STD_LEN*2];
  char coinbuf[200]; /* Show coins info if needed */
  char numbuf[200]; /* Show number info if needed. */
  int i, newslot, pass;
  THING *cont;
  VALUE *shop = NULL;
  int color, bright;
  bool combine = FALSE;
  bool can_peek = FALSE;
  bool showed_money = FALSE;
  if (!target || !th)
    return;
  /* Clear data */

  for (i = 0; i < LBUF_SIZE; i++)
    {
      listbuf[i][0] = '\0';
      listnum[i] = 0;
      cost[i] = 0;
    }
  
  if ((LEVEL (th) >= BLD_LEVEL && IS_PC1_SET (th, PC_HOLYLIGHT)) ||
      check_spell (th, "peek", 0) || th == target ||
      (!CAN_MOVE (target) && !CAN_FIGHT (target)))
    can_peek = TRUE;
  if (IS_SET (flags, LOOK_SHOW_SHOP))
    {
      if ((shop = FNV (target, VAL_SHOP)) == NULL)
	{
	  stt ("Error. No shop here.\n\r", th);
	  return;
	}
      sprintf (buf, "A list of things for sale at %s's shop:\n\n\rNum   Cost:       Name:\n\r", name_seen_by (th, target));
      stt (buf, th);
      can_peek = TRUE;
    }
  else
    {      
      
      if (total_money (target) > 0 && 
	  (target == th->in || can_peek))
	{
	  showed_money = TRUE;
	  show_money (th, target, IS_SET (flags, LOOK_SHOW_TYPE));
	}
      
      if ((target == th->in || can_peek) && IS_SET (flags, LOOK_SHOW_TYPE))
	{
	  if (!IS_SET (target->thing_flags, TH_NO_CONTAIN) ||
	      target->cont)
	    {
	      if (target == th)
		sprintf (buf, "You are carrying:\n\r");
	      else if (CAN_MOVE (target) || CAN_FIGHT (target))
		sprintf (buf, "%s is carrying:\n\r", NAME (target));
	      else
		sprintf (buf, "%s contains:\n\r", NAME (target));
	      stt (buf, th);
	    }
	}
    }
  for (pass = 0; pass < 2; pass++)
    {
      for (cont = target->cont; cont != NULL; cont = cont->next_cont)
	{
	  if (!can_see (th, cont) || cont == th ||
              (target == th && flags == LOOK_SHOW_TYPE &&
	       IS_WORN (cont)) ||
	      (target->in == th->in && 
	       (CAN_MOVE (target) || CAN_FIGHT (target)) &&
	       !can_peek && !IS_WORN (cont)) ||
	      (pass != (CAN_MOVE (cont) || CAN_FIGHT (cont))))
	    continue;
	  
	  combine = FALSE;
          buf[0] = '\0';
	  if (IS_SET (flags, LOOK_SHOW_SHOP))
	    {
	      bright = 0;
	      color = 7;
	    }
	  else if (IS_PC (cont))
	    {
	      bright = 1;
	      color = 5;
	    }
	  else
	    {
	      color = 6;
	      bright = (CAN_MOVE (cont) ? 1 : 0);
	    }
	  /* Find the "look" of the item in question. Generally, it
	     will be a shop item, a short desc (inven/eq) or a long desc
	     (looking in a room). */
	  
	  if (IS_SET (flags, LOOK_SHOW_SHOP))
	    {
	      combine = TRUE;
	      if (is_valid_shop_item (cont, shop))
		strncpy (buf, NAME (cont), STD_LEN-1);
	      else
		continue;
	    }
	  else if (!IS_SET (flags, LOOK_SHOW_TYPE))
	    {
	      if (!CAN_MOVE (cont) && !CAN_FIGHT (cont))
		combine = TRUE;  
	      if (cont->position == POSITION_STANDING &&
		  (cont->long_desc[0] || 
		   (cont->proto && cont->proto->long_desc[0])))
		{
		  sprintf (buf, "\x1b[%d;3%dm%s %s\x1b[0;37m", bright, color, LONG (cont), show_affects (cont));
		}
	      else
		{
		  buf[0] = '\0';
		  buf2[0] = '\0';
		  posbuf[0] = '\0';
		  if (cont->position == POSITION_FIGHTING || cont->position == POSITION_TACKLED)
		    sprintf (posbuf, position_looks[cont->position], (FIGHTING (cont) ? name_seen_by (th, FIGHTING(cont)) : "noone"));
		  else
		    strcpy (posbuf, position_looks[(cont->position > 0 && cont->position < POSITION_MAX ? cont->position : POSITION_STANDING)]);
		  
		  if (IS_PC (cont) && !cont->fd)
		    sprintf (buf, "\x1b[1;34m> LINKDEAD: \x1b[1;31mPKILL ME\x1b[1;34m!!! < ");
		  if (IS_PC (th) && IS_PC (cont) && DIFF_ALIGN (th->align, cont->align))
		    {
		      sprintf (buf + strlen (buf), "\x1b[%d;3%dm%s is %s %s\x1b[0;37m", bright, color, name_seen_by (th, cont), posbuf, show_affects(cont));
		    }
		  else
		    sprintf (buf + strlen(buf), "\x1b[%d;3%dm%s is %s %s\x1b[0;37m", bright, color, NAME (cont), posbuf, show_affects (cont));
		}
	    }
	  else 
	    {
	      sprintf (buf, "\x1b[0;37m%s%s %s\x1b[0;37m", 
		       (IS_WORN(cont) ?
			wear_data[cont->wear_loc].view_name : ""), 
		       NAME(cont), (th == cont->in ?  show_condition (cont, FALSE) : ""));
	      if (!IS_WORN(cont))
		combine = TRUE;
	    }
	  if (buf[0] == '\0')
	    continue;
	  
	  /* Add to the list. */
	  newslot = LBUF_SIZE;
	  if (combine)
	    {
	      for (i = 0; i < LBUF_SIZE && listbuf[i][0] != '\0'; i++)
		{
		  if (*buf == *listbuf[i] && !str_cmp (buf, listbuf[i]))
		    {
		      newslot = i;
		      break;
		    }
		}
	    }
	  
	  if (newslot == LBUF_SIZE)
	    {
	      for (i = 0; i < LBUF_SIZE; i++)
		{
		  if (listbuf[i][0] == '\0')
		    {
		      newslot = i;
		      break;
		    }
		}
	    }
	  
	  if (newslot == LBUF_SIZE)
	    {
	      log_it ("LIST TOO LONG!!! MAKE IT BIGGER!!! in show_contents_list!!!!!\n");
	      continue;
	    }
	  
	  if (listbuf[i][0] == '\0')
	    {
	      strncpy (listbuf[i], buf, STD_LEN -1);
              listbuf[i][LBUF_SIZE-1] = '\0';
	      if (IS_SET (flags, LOOK_SHOW_SHOP) && shop)
		cost[i] = find_item_cost (th, cont, target, SHOP_COST_BUY);
	    }	 
	  if (combine)
	    listnum[i]++;
	}
    }
  /* So, at this point we should have the list of things and their
     descs all setup nice in the listbuf array. We will now send the
     output. */
  
  if (IS_SET (flags, LOOK_SHOW_SHOP) && listbuf[0][0] == '\0')
    {
      stt ("Nothing.\n\r", th);
      return;
    }
  
  for (i = 0; i < LBUF_SIZE && listbuf[i][0] != '\0'; i++)
    { 
	 
      if (IS_SET (flags, LOOK_SHOW_SHOP))
	{
	  sprintf (numbuf, "(x%2d) ", listnum[i]);
	  sprintf (coinbuf, "%-5d coins: ", cost[i]);
	}
      else
	{
	  coinbuf[0] = '\0';
	  if (listnum[i] > 1)
	    sprintf (numbuf, "%s[x%d] ", IS_SET(flags, LOOK_SHOW_TYPE) ? 
                     "" : "\x1b[0;36m", listnum[i]);
	  else
	    numbuf[0] = '\0';
	}
      sprintf (buf, "%s%s%s\n\r",
	       numbuf, coinbuf, listbuf[i]);
      stt (buf, th);
    }
  return;
		   
		   
  

}





char *
show_affects (THING *th)
{
  static char ret[STD_LEN * 2];
  int affbits;
  
  ret[0] = '\0';
  if (!th)
    return ret;
  affbits = flagbits (th->flags, FLAG_AFF);
  
  if (IS_SET (affbits, AFF_FLYING))
    strcat (ret, "\x1b[1;35m(Flying) ");
  if (IS_SET (affbits, AFF_PROTECT))
    strcat (ret, "\x1b[1;36m(\x1b[1;34mBlue Aura\x1b[1;36m) "); 
  if (IS_SET (affbits, AFF_SANCT))
    strcat (ret, "\x1b[0;37m(\x1b[1;37mSanctuary\x1b[0;37m) ");
  if (IS_SET (affbits, AFF_INVIS))
    strcat (ret, "\x1b[0;37m(\x1b[1;30mInvis\x1b[0;37m) ");
  if (IS_SET (affbits, AFF_CHAMELEON))
    strcat (ret, "\x1b[1;32m(\x1b[0;32mChameleon\x1b[1;32m) ");
  if (IS_SET (affbits, AFF_HIDDEN))
    strcat (ret, "\x1b[1;30m(\x1b[0;34mHidden\x1b[1;30m) ");
  if (IS_SET (affbits, AFF_AIR))
    strcat (ret, "\x1b[1;33m(Electricity) ");
  if (IS_SET (affbits, AFF_EARTH))
    strcat (ret, "\x1b[0;33m(Spikes) ");
  if (IS_SET (affbits, AFF_FIRE))
    strcat (ret, "\x1b[0;31m(\x1b[1;31mFiery\x1b[0;31m) ");
  if (IS_SET (affbits, AFF_WATER))
    strcat (ret, "\x1b[1;37m(\x1b[1;36mIcy\x1b[1;37m) ");
  if (IS_ACT1_SET (th, ACT_PRISONER))
    strcat (ret, "\x1b[1;36m(\x1b[1;31mPrisoner\x1b[1;36m)\x1b[0;37m");
  if (ret[0])
    strcat (ret, "\x1b[0;37m");
  return ret;
}

