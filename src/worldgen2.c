#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<sys/time.h>
#include<sys/types.h>
#include<dirent.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "areagen.h"
#include "worldgen.h"
#include "mobgen.h"
#include "questgen.h"
#include "objectgen.h"
#include "script.h"
#include "historygen.h"
#include "rumor.h"

/* These worldgen functions are used for more "Details" being added
   into the world like demons and guildmasters etc... */


/* This generates areas for demons to live in down below the underworld
   and it generates the demons, too. */

void
worldgen_generate_demons (int curr_vnum, int area_size)
{
  int times, max_underworld_level = 0, max_times;
  int tier; /* Caste tier for generating names. */
  THING *above_area = NULL, *below_area = NULL, *area;
  THING *proto, *thg;
  char buf[STD_LEN];
  char demon_names[STD_LEN];
  char demon_name[STD_LEN];
  char proto_desc[STD_LEN*10];
  char *t;
  int caste;
  int curr_size, curr_level;
  int extra_room_flags= 0;
  bool name_is_ok = FALSE;
  VALUE *val;
  SOCIETY *soc;
  /* First find the max level for an underworld area. This is where
     we will link the demonic realms below it. */

  for (area = the_world->cont; area; area = area->next_cont)
    {
      if (area->vnum >= WORLDGEN_START_VNUM &&
	  area->vnum <= WORLDGEN_END_VNUM &&
	  IS_AREA (area) &&
	  IS_ROOM_SET (area, ROOM_UNDERGROUND) &&
	  LEVEL(area) >= max_underworld_level)
	{
	  above_area = area;
	  max_underworld_level = area->level;
	}
    }
  area_size = (area_size/50 + 1)*50;
  if (!above_area)
    return;
  
  max_times = nr (4,6);
  curr_level = max_underworld_level;
  /* Now create each area. */
  for (times = 0; times < max_times; times++)
    {
      curr_level += max_underworld_level/5;
      curr_size = nr (area_size*2/3, area_size*3/2);
      sprintf (buf, "%d %d underground %d",
	       curr_vnum,
	       curr_size,
	       curr_level);
      areagen (NULL, buf);
      /* If the area was made, fix it up some. */
      if ((below_area = find_thing_num (curr_vnum)) != NULL &&
	  IS_AREA (below_area))
	{
	  curr_vnum += curr_size;
	  
	  extra_room_flags = ROOM_ASTRAL|ROOM_FIERY;
	  add_flagval (below_area, FLAG_ROOM1, extra_room_flags);
	  if (extra_room_flags)
	    {
	      for (thg = below_area->cont; thg; thg = thg->next_cont)
		{
		  /* Replace all rooms w/same name with hellish rooms. */
		  if (IS_ROOM (thg))
		    {
		      if (!str_cmp (NAME(thg), NAME(below_area)))
			{
			  
			  add_flagval (thg, FLAG_ROOM1, extra_room_flags);
			  free_str (thg->short_desc);
			  thg->short_desc = new_str ("The Pits of Hell");			  
			}
		    }
		  /* Mobs get extra protections. */
		  else if (!IS_SET (thg->thing_flags, TH_NO_FIGHT))
		    {
		      add_flagval (thg, FLAG_AFF, AFF_PROTECT);
		      add_flagval (thg, FLAG_PROT, AFF_FIRE);
		    }
		}
	    }
	  
	  free_str (below_area->short_desc);
	  below_area->short_desc = new_str ("The Pits of Hell");
	  /* Now link the areas. */
	  worldgen_link_above_to_below (above_area, below_area);
	  above_area = below_area;
	}
    }
  /* NOTE THAT THE BELOW AREA WILL BE USED BELOW TO CREATE THE PLACE
     WHERE THE DEMON SOCIETY STARTS! */

  /* Now generate the demon society. */

  proto = new_thing();
  proto->level = 350;
  proto->name = new_str ("Demon Demons Demonic");
  proto->sex = SEX_NEUTER;
  add_flagval (proto, FLAG_PROT, ~0);
  add_flagval (proto, FLAG_AFF, AFF_FLYING | AFF_WATER_BREATH | AFF_FOGGY | AFF_PROTECT);
  add_flagval (proto, FLAG_DET, ~0);
  add_flagval (proto, FLAG_ACT1, ACT_AGGRESSIVE | ACT_FASTHUNT);
  add_flagval (proto, FLAG_MOB, MOB_DEMON);
  add_flagval (proto, FLAG_SOCIETY, SOCIETY_NORESOURCES | SOCIETY_NOSLEEP | SOCIETY_NONAMES);
  add_flagval (proto, FLAG_ROOM1, BADROOM_BITS);
  proto->align = 0;
  proto->vnum = DEMON_SOCIGEN_VNUM;
  proto->height = nr (300,600);
  proto->weight = nr (10000, 20000);
  
  /* Give special attack names. */
  val = new_value();
  val->type = VAL_WEAPON;
  set_value_word (val, "claw claw bite strike");
  add_value (proto, val);
  
  *proto_desc = '\0';
  for (caste = 0; caste < CASTE_MAX &&
	 caste1_flags[caste].flagval != 0; caste++)
    {
      sprintf (demon_names, "Caste %s", caste1_flags[caste].app);
      
      /* Make all of the demonic tier names. */
      for (tier = 0; tier < 10; tier++)
	{
	  strcat (demon_names, " ");
	  do
	    {
	      name_is_ok = FALSE;
	      strcpy (demon_name, create_society_name (NULL));
	      
	      /* Name must be long enough. */
	      if (strlen (demon_name) < 6)
		continue;
	      
	      /* Name must have a strange letter or a symbol in it. */
	      for (t = demon_name; *t; t++)
		{
		  if (!isalpha (*t))
		    name_is_ok = TRUE;
		  if (must_be_near_vowel (*t))
		    name_is_ok = TRUE;
		}
	    }
	  while (!name_is_ok);
	  *demon_name = UC (*demon_name);
	  strcat (demon_names, demon_name);
	  
	}
      strcat (demon_names, "\n\r");
      strcat (proto_desc, demon_names);
    }
  proto->desc = new_str (proto_desc);
  soc = generate_society (proto);
  free_thing (proto);
  
  if (soc)
    {
      soc->room_start = below_area->vnum + 1;
      soc->room_end = below_area->vnum + below_area->mv;
      /* Make this area pop a LOT of randpop mobs. Help those
	 demons grow by giving them something to kill. :) */
      add_reset (below_area, MOB_RANDPOP_VNUM, 50, 200, 1);
    }
  return;
}


/* This removes aggro monsters from lowlevel areas. It also adds extra
   standard eq pops to the monsters and lowers the hps on the monsters
   so players stand a better chance at the beginning. */

void
setup_newbie_areas (void)
{
  THING *area, *mob;
  RESET *rst;
  
  for (area = the_world->cont; area; area = area->next_cont)
    {
      if (area->vnum < WORLDGEN_START_VNUM ||
	  area->vnum > WORLDGEN_END_VNUM ||
	  LEVEL (area) >= WORLDGEN_AGGRO_AREA_MIN_LEVEL)
	continue;
      
      
      /* Remove aggros and remove society settling here so that
	 players never have to face "invasions" that close to home. */
      add_flagval (area, FLAG_AREA, AREA_NOSETTLE);
      
      for (mob = area->cont; mob; mob = mob->next_cont)
	{
	  if (IS_ROOM (mob) || !CAN_FIGHT (mob) || !CAN_MOVE (mob))
	    continue;
	  if (mob->max_hp > LEVEL(area)/5)
	    mob->max_hp = LEVEL(area)/5;
	  
	  remove_flagval (mob, FLAG_ACT1, ACT_AGGRESSIVE | ACT_ANGRY);
	  /* Add more armor/wpn pops to the talker mobs. */
	  if (CAN_TALK (mob))
	    {
	      add_reset (mob, ARMOR_RANDPOP_VNUM, 20, 3, 1);
	      add_reset (mob, WEAPON_RANDPOP_VNUM, 20, 1, 1);
	    }
	}
      
      
      
      /* Now add more "animal" resets to these areas so players have
	 more stuff to kill. */

      for (rst = area->resets; rst; rst = rst->next)
	{
	  if (rst->vnum == MOB_RANDPOP_VNUM)
	    break;
	}
      if (!rst)
	add_reset (area, MOB_RANDPOP_VNUM, 50, area->mv/2, 1);
      else
	rst->times *= 2;
    }
  return;
}



/* This generates guildmasters and places them in different places 
   around the world. */

void 
worldgen_place_guildmasters (void)
{
  THING *area, *outpost_area;
  int al, guildnum;
  RACE *align;
  int rank;
  int count, num_choices = 0, num_chose = 0;
  /* Loop through the three ranks we will set up. */
  
  int max_area_level = 70;
  
  /* For 1st rank, put a guildmaster of each type near the
     align homelands. */
  
  for (al = 1; al < ALIGN_MAX; al++)
    {
      unmark_areas();
      
      /* Make sure the alignment exists and the outpost gate
	 exists. */
      
      if ((align = align_info[al]) == NULL)
	continue;
      
      
      if ((outpost_area = find_area_in (align->outpost_gate)) == NULL)
	continue;
      
      /* Mark the outpost and the adjacent area(s). */
      SBIT (outpost_area->thing_flags, TH_MARKED);
      mark_adjacent_areas (outpost_area);
      for (area = the_world->cont; area; area = area->next_cont)
	{
	  if (IS_MARKED (area) && area != outpost_area)
	    break;
	}
      
      if (!area)
	{
	  echo ("Failed to find app area.\n\r");
	  continue;
	}
      
      /* Now mark the areas adjacent to this new area. */
       
      /* Not anymore--- just have the guildmasters right outside the 
	 homeland. */
      /* mark_adjacent_areas (area); */
       
	      
      /* Now pick an area at random -- not the outpost area. */
      
      RBIT (outpost_area->thing_flags, TH_MARKED);
      /* Now for each guild pick an area and create a guildmaster
	 in that area. */
      for (guildnum = 0; guildnum < GUILD_MAX; guildnum++)
	{
	  num_choices = 0;
	  num_chose = 0;
	  for (count = 0; count < 2; count++)
	    {
	      for (area = the_world->cont; area; area = area->next_cont)
		{
		  if (!IS_MARKED (area))
		    continue;
		  
		  if (find_free_mobject_vnum (area) == 0)
		    continue;
			  
		  if (count == 0)
		    num_choices++;
		  else if (--num_chose < 1)
		    break;
		}
	      
	      if (count == 0)
		{
		  if (num_choices < 1)
		    break;
		  else 
		    num_chose = nr (1, num_choices);
		}
	    }
	  
	  if (area && IS_MARKED (area))
	    add_guildmaster_to_area (area, guildnum, 0);
	}
    }
  unmark_areas();
  
  /* Find the max level for an area in the world. */
  for (area = the_world->cont; area; area = area->next_cont)
    {
      if (area->vnum >= WORLDGEN_START_VNUM && 
	  area->vnum <= WORLDGEN_END_VNUM &&
	  area->level > max_area_level &&
	  str_cmp (area->short_desc, "the pits of hell"))
	max_area_level = area->level;
    }
  
      
      
  for (rank = 1; rank < 3; rank++)
    {
      /* Mark the appropriate areas based on levels and being in 
	 the worldgen areas. */
      
      unmark_areas();
      for (area = the_world->cont; area; area = area->next_cont)
	{
	  if (area->vnum < WORLDGEN_START_VNUM ||
	      area->vnum > WORLDGEN_END_VNUM ||
	      (rank == 1 && 
	       (area->level > max_area_level/2 ||
		area->level < max_area_level/4)) ||
	      (rank == 2 && 
	       area->level <= max_area_level/2) ||
	      find_free_mobject_vnum (area) == 0 ||
	      !str_cmp (NAME(area), "the pits of hell"))
	    continue;
	  
	  /* Only marked areas should be those that are worldgen areas 
	     and in the proper level region. */
	  SBIT (area->thing_flags, TH_MARKED);
	}
	    
	       
      for (guildnum = 0; guildnum < GUILD_MAX; guildnum++)
	{
	  
	  num_choices = 0;
	  num_chose = 0;
	  for (count = 0; count < 2; count++)
	    { 	      
	      for (area = the_world->cont; area; area = area->next_cont)
		{
		  if (!IS_MARKED (area))
		    continue;
		  
		  if (find_free_mobject_vnum (area) == 0)
		    {
		      RBIT (area->thing_flags, TH_MARKED);
		      continue;
		    }
		  
		  if (count == 0)
		    num_choices++;
		  else if (--num_chose < 1)
		    break;
		}

	      if (count == 0)
		{
		  if (num_choices < 1)
		    {
		      echo ("Failed to find GM area\n\r");
		      break;
		    }
		  else
		    num_chose = nr (1, num_choices);
		}
	    }
	  
	  if (area && find_free_mobject_vnum (area) != 0)
	    add_guildmaster_to_area (area, guildnum, rank);
	}
    }
  unmark_areas();
  return;
}

/* This creates and adds a guildmaster to an area. */

void
add_guildmaster_to_area (THING *area, int guild, int rank)
{
  THING *proto, *room = NULL;
  int vnum;
  int room_tries;
  char guildname[STD_LEN];
  char rankname[STD_LEN];
  char name[STD_LEN];
  char sdesc[STD_LEN];
  char proper_name[STD_LEN];
  VALUE *gld;

  
  if (!area || !IS_AREA (area) || 
      guild < 0 || guild >= GUILD_MAX ||
      (vnum = find_free_mobject_vnum (area)) == 0)
    return;
  
  if (rank < 0)
    rank = 0;
  if (rank > 2)
    rank = 2;

  for (room_tries = 0; room_tries < 20; room_tries++)
    {
      if ((room = find_random_room (area, FALSE, 0, BADROOM_BITS)) == NULL ||
	  (room_tries < 10 && room->resets))
	continue;
      break;
    }
  
  if (!room)
    {
      sprintf (sdesc, "Area: %d\n", area->vnum);
      echo (sdesc);
      return;
    }

  SBIT (area->thing_flags, TH_CHANGED);
  proto = new_thing();
  proto->vnum = vnum;
  proto->level = 50 + 50*rank;
  proto->max_hp = 100;
  proto->max_mv = 1;
  proto->align = 0;
  proto->height = nr (60,80);
  proto->weight = nr (2000, 4000);
  proto->thing_flags = MOB_SETUP_FLAGS;
  add_flagval (proto, FLAG_ACT1, ACT_SENTINEL);
  add_flagval (proto, FLAG_DET, AFF_DARK | AFF_INVIS | AFF_HIDDEN);
  thing_to (proto, area);
  add_thing_to_list (proto);

  /* Set the rank, proper, and guild names. */
  if (rank == 0)
    strcpy (rankname, " apprentice");
  else if (rank == 1)
    strcpy (rankname, " journeyman");
  else
    strcpy (rankname, "master");

  strcpy (proper_name, create_society_name(NULL));
  sprintf (guildname, "%s", guild_info[guild].name);
  
  sprintf (name, "%s %s %s guildmaster", rankname, proper_name, guildname);
  proto->name = new_str (name);
  sprintf (sdesc, "%s the %s's guild%s", proper_name, guildname, rankname);
  proto->short_desc = new_str (sdesc);
  
  strcat (sdesc, " is ");
  if (nr (1,3) != 2)
    {
      if (nr (1,3) == 2)
	strcat (sdesc, "calmly ");
      else if (nr (1,2) == 1)
	strcat (sdesc, "quietly ");
      else
	strcat (sdesc, "confidently ");
    }
  strcat (sdesc, "standing here.");
  proto->long_desc = new_str (sdesc);
  
  /* Add the guildmaster value so this actually does something useful...*/
  if ((gld = FNV (proto, VAL_GUILD)) == NULL)
    {
      gld = new_value();
      gld->type = VAL_GUILD;
      add_value (proto, gld);
    }
  gld->val[0] = guild; /* What kind of guild. */
  gld->val[1] = 1;  /* Min tier */
  gld->val[2] = MIN((rank+1)*2, GUILD_TIER_MAX); /* Max tier. */
  
  add_reset (room, vnum, 100, 1, 1);
  return;
}
  

/* This places the implant and remort rooms someplace in the world
   and puts guards in front of them with little scripts so people know
   when players go there. */
  
void
worldgen_link_special_room (int special_room_vnum)
{

  THING *area, *room, *proto, *special_room, *start_area;
  int count, num_choices = 0, num_chose = 0;
  VALUE *exit, *guard;
  int guard_vnum = 0;
  char name[STD_LEN];
  char buf[STD_LEN];
  int tries = 0;
  TRIGGER *trig;
  CODE *code;
  
  if ((special_room = find_thing_num (special_room_vnum)) == NULL)
    return;
  if ((start_area = find_thing_num (1)) == NULL)
    return;
  /* Clear exits and all else. */
  while (special_room->values)
    remove_value (special_room, special_room->values);
  
  for (count = 0; count < 2; count++)
    {
      for (area = the_world->cont; area; area = area->next_cont)
	{
	  if (!IS_AREA (area) ||
	      area->vnum < WORLDGEN_START_VNUM ||
	      area->vnum > WORLDGEN_END_VNUM ||
	      IS_ROOM_SET (area, BADROOM_BITS) ||
	      (guard_vnum = find_free_mobject_vnum (area)) == 0 ||
	      IS_AREA_SET (area, AREA_NOLIST | AREA_NOSETTLE | AREA_NOREPOP) ||
	      LEVEL (area) < WORLDGEN_AGGRO_AREA_MIN_LEVEL*3/2 ||
	      LEVEL (area) > WORLDGEN_AGGRO_AREA_MIN_LEVEL*3)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}

      if (count == 0)
	{
	  if (num_choices < 1)
	    return;
	  num_chose = nr (1,num_choices);
	}
    }
  if (!area)
    return;

  /* Try to find a decent room. */
  for (tries = 0; tries < 100; tries++)
    {
      int dir;
      count = 0;
      num_choices = 0;
      num_chose = 0;
      if ((room = find_random_room (area, FALSE, 0, BADROOM_BITS | ROOM_EASYMOVE)) != NULL)
	{
	  
	  for (count = 0; count < 2; count++)
	    {
	      /* Check for open dirs to this room. */
	      for (dir = 0; dir < REALDIR_MAX; dir++)
		{
		  if ((exit = FNV (room, dir + 1)) == NULL)
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
	  
	  if (dir >= REALDIR_MAX ||
	      (exit = FNV (room, dir + 1)) != NULL)
	    continue;
	  
	  /* Add the exits and exit the loop. */
	  exit = new_value ();
	  exit->type = dir + 1;
	  add_value (room, exit);
	  exit->val[0] = special_room_vnum;
	  
	  exit = new_value ();
	  exit->type = RDIR(dir) + 1;
	  add_value (special_room, exit);
	  exit->val[0] = room->vnum;

	  /* Now make the guard mob and put it there guarding. */

	  proto = new_thing();
	  proto->vnum = guard_vnum;
	  proto->thing_flags = MOB_SETUP_FLAGS | TH_SCRIPT;
	  proto->height = nr (100,120);
	  proto->weight = nr (5000, 10000);
	  proto->level = MAX_LEVEL;
	  proto->max_hp = 50;
	  proto->max_mv = 1;
	  thing_to (proto, area);
	  add_thing_to_list (proto);
	  sprintf (name, "%s Guardian", NAME(special_room));
	  proto->name = new_str (name);
	  proto->short_desc = new_str (name);
	  strcat (name, " is here.");
	  proto->long_desc = new_str (name);
	  add_reset (room, guard_vnum, 100, 1, 1);	  
	  guard = new_value();
	  guard->type = VAL_GUARD;
	  set_value_word (guard, dir_letter[dir]);
	  guard->val[0] = 1;
	  add_value (proto, guard);
	  add_flagval (proto, FLAG_DET, AFF_DARK | AFF_INVIS | AFF_HIDDEN | AFF_CHAMELEON);
	  add_flagval (proto, FLAG_ACT1, ACT_SENTINEL);

	  sprintf (name, "room%dguard", special_room_vnum);
	  
	  trig = new_trigger();
	  trig->vnum = proto->vnum;
	  trig->type = TRIG_ENTER;
	  trig->next = trigger_hash[trig->vnum % HASH_SIZE];
	  trigger_hash[trig->vnum % HASH_SIZE] = trig;
	  trig->name = new_str (name);
	  trig->code = new_str (name);
	  trig->flags = TRIG_PC_ONLY;
	  
	  /* Now set up the code. */
	  code = new_code();
	  code->name = new_str (name);
	  add_code_to_table (code);
	  sprintf (buf, "wait @r(5,10)\n\rdo echo $9@ssd is raiding %s!$7\n\r", NAME(special_room));
	  code->code = new_str (add_color(buf));
	  break;
	}
    }
  return;
}
  
/* This finds the total number of words used for generating 
   the game by looking at each item in each of the generator areas. */

int
find_num_gen_words (THING *obj)
{
  THING *area, *th; 
  int total = 0;   
  EDESC *edesc;
  if (!obj)
    return 0;
  /* Do search all areas. */
  if (obj == the_world)
    {  
      for (area = the_world->cont; area; area = area->next_cont)
	{
	  if (area->vnum < GENERATOR_NOCREATE_VNUM_MIN ||
	      area->vnum > GENERATOR_NOCREATE_VNUM_MAX)
	    continue;
	  
	  if (!area->cont)
	    continue;
	  th = area->cont->next_cont;
	  
	  for (; th; th = th->next_cont)
	    {
	      total += find_num_gen_words (th);
	    }
	}
      return total;
    }

  /* Add up the total number of words in each part of each 
     thing. This isn't perfect due to phrases and &'s in name/colorname
     strings, but it's close. */
  
  total += find_num_words (obj->name);
  total += find_num_words (obj->short_desc);
  total += find_num_words (obj->long_desc);
  total += find_num_words (obj->desc);
  total += find_num_words (obj->type);
  
  for (edesc = obj->edescs; edesc; edesc = edesc->next)
    {
      total += find_num_words (edesc->name);
      total += find_num_words (edesc->desc);
    }
  return total;
}

