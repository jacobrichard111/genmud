#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "serv.h"
#include "society.h"
#include "areagen.h"
#include "worldgen.h"
#include "objectgen.h"

/* This macro is used to remove certain words from the list of names
   for an object. It's only useful inside of 
   objectgen_generate_names. */

#define del_obj_name(a) {*name[OBJECTGEN_NAME_ ## a ] = '\0'; \
*colorname[OBJECTGEN_NAME_ ## a ] = '\0'; \
RBIT (name_parts_used, (1 << OBJECTGEN_NAME_ ## a ));} 

/* The first numbers are the ranks given to different afecst.
   The second numbers are the chances that certain affects appear 
   in genned objects. They are weighted so certain things are more 
   likely, and certain things are less likely. Basically, within a 
   single rank of affects, you add all the possible numbers up then 
   pick a random affect from the weighted sum. Larger numbers mean 
   it's more likely to show up in an object. */

const int aff_power_ranks[AFF_MAX-AFF_START][AFF_POWER_TYPES] =
  {
    { AFF_RANK_EXCELLENT, 5 }, /* Start stats -- str */
    { AFF_RANK_EXCELLENT, 2 }, /* int */
    { AFF_RANK_EXCELLENT, 2 }, /* wis */
    { AFF_RANK_EXCELLENT, 2 }, /* dex */
    { AFF_RANK_EXCELLENT, 1 }, /* con */
    { AFF_RANK_EXCELLENT, 1 }, /* luc */
    { AFF_RANK_EXCELLENT, 3 }, /* End stats. -- cha */
    { AFF_RANK_EXCELLENT, 10 }, /* Damroll */
    { AFF_RANK_GOOD,  10 }, /* Hitroll */
    { AFF_RANK_POOR,  10 }, /* hp */
    { AFF_RANK_POOR,  5}, /* mv */
    { AFF_RANK_GOOD,  2 }, /* Sp att */
    { AFF_RANK_GOOD,  2 }, /* Heal pct */
    { AFF_RANK_FAIR,  4 }, /* Sp resist */
    { AFF_RANK_GOOD,  2 }, /* Th att */
    { AFF_RANK_FAIR,  2 }, /* Defense -- block/dodge etc... */
    { AFF_RANK_GOOD,  2 }, /* Kickdam */
    { AFF_RANK_POOR,  5 }, /* Armor */
    { AFF_RANK_EXCELLENT,  4}, /* Dam resist...-pcts off damage taken. Really good. */
    { AFF_RANK_FAIR,  2 }, /* Groundfight attack. */
    { AFF_RANK_FAIR,  2 }, /* Groundfight defense. */
    { AFF_RANK_GOOD,  1 }, /* Elem power start air */
    { AFF_RANK_GOOD,  1 }, /* Earth */
    { AFF_RANK_GOOD,  1 }, /* Fire */
    { AFF_RANK_GOOD,  1 }, /* Water */
    { AFF_RANK_GOOD,  1 },  /* Elem power end. spirit */
    { AFF_RANK_EXCELLENT,  2}, /* Elem level start */
    { AFF_RANK_EXCELLENT,  2}, /* earth */
    { AFF_RANK_EXCELLENT,  2}, /* fire */
    { AFF_RANK_EXCELLENT,  2}, /* water */
    { AFF_RANK_EXCELLENT,  2}, /* Elem level end. spirit */
  };





/* These are the levels you need to attain to get certain kinds of
   affects of certain ranks. The ranks are shown in const.c
   just below where the affects are defined. */

static int aff_rank_levels[AFF_RANK_MAX] = 
  {
    30, 
    60,
    90,
    120,
  };


/* This creates an object of a certain type and of a certain level of
   power. If the wear_loc is not a valid location, then it picks
   a random value. */
static const char *objectgen_part_names[OBJECTGEN_NAME_MAX] =
  {
    "owner", /*0 */
    "a_an",   /* Only set if owner isn't. */
    "prefix",
    "color",
    "gem",
    
    "metal", /* 5 */
    "nonmetal", /* Other things besides metals that an object can
		   be made of. */
    "animal", /* Things like skin, bones, feathers etc... */
    "society", 
    "type",   /* 8th slot, array[7]. Important to keep distinct. */
    "suffix",
    "called",
  };


THING *
objectgen (THING *area, int wear_loc, int level, int curr_vnum, char *preset_names)
{
  THING *obj, *in_area;
  int weapon_type = -1;
  char name[OBJECTGEN_NAME_MAX][STD_LEN];
  char color[OBJECTGEN_NAME_MAX][STD_LEN];
  
  if (wear_loc <= ITEM_WEAR_NONE || wear_loc >= ITEM_WEAR_MAX ||
      wear_loc == ITEM_WEAR_BELT)
    {
      wear_loc = objectgen_find_wear_location();
      if (wear_loc <= ITEM_WEAR_NONE || wear_loc >= ITEM_WEAR_MAX ||
	  wear_loc == ITEM_WEAR_BELT)
	{
	  log_it ("objectgen_find_wear_location FAILED!\n");
	  return NULL;
	}
    }
  if (wear_loc == ITEM_WEAR_WIELD)
    weapon_type = generate_weapon_type ();
  
  
  /* See if we have an open vnum slot. */
  if (curr_vnum > 0)
    {
      if ((in_area = find_area_in (curr_vnum)) == NULL ||
	  (area && in_area != area) ||
	  curr_vnum <= in_area->vnum + in_area->mv ||
	  curr_vnum >= in_area->vnum + in_area->max_mv ||
	  (obj = find_thing_num (curr_vnum)) != NULL)
	return NULL;
    }
  else if (area != NULL)
    {
      in_area = area;
      for (curr_vnum = area->vnum + area->mv + 1; 
	   curr_vnum < area->vnum + area->max_mv; curr_vnum++)
	{
	  if ((obj = find_thing_num (curr_vnum)) == NULL)
	    break;
	}
      if (curr_vnum < area->vnum + area->mv + 1 ||
	  curr_vnum >= area->vnum + area->max_mv)
	return NULL;
    }
  else /* No curr_vnum and no area. */
    return NULL;
  /* Get the descriptions of the object. */
  
  objectgen_generate_names (name, color, wear_loc, weapon_type, level, preset_names);
  
  
  if ((obj = new_thing()) == NULL)
    return NULL;
  obj->vnum = curr_vnum;
  if (curr_vnum > top_vnum)
    top_vnum = curr_vnum;
  obj->thing_flags = OBJ_SETUP_FLAGS;
  obj->wear_pos = wear_loc;
  obj->level = level;
  thing_to (obj, in_area);
  add_thing_to_list (obj);
  /* Set up the name, short_desc, long_desc for the object. Maybe
     desc at some way way future date. */

  objectgen_setup_names (obj, name, color, preset_names);
  
  /* This puts the stats and bonuses onto the object. */
  
  objectgen_setup_stats (obj, weapon_type);
  
  
  return obj;
}

/* This gives the type of weapon you will make...it's weighted toward
   making swords. */

int
generate_weapon_type (void)
{
  int weapon_type;
  if (nr (1,12) == 3)
    weapon_type = WPN_DAM_WHIP;
  else if (nr (1,9) == 2)
    weapon_type = WPN_DAM_CONCUSSION;
  else if (nr (1,4) == 1)
    weapon_type = WPN_DAM_PIERCE;
  else
    weapon_type = WPN_DAM_SLASH;
  return weapon_type;
}


/* This sets up the names for the object by looking at the list of names. */

void
objectgen_setup_names (THING *obj, char name[OBJECTGEN_NAME_MAX][STD_LEN],
		       char colorname[OBJECTGEN_NAME_MAX][STD_LEN],
		       char *preset_names)
{
  int i;
  char fullname[STD_LEN*2];
  char realfullname[STD_LEN * 2];
  char longname[STD_LEN*3];
  char namebuf[STD_LEN];
  char *t;

  /* Used to get an "overall color" out of this item. */
  char *wd;
  char word[STD_LEN];
  int overall_color = 0;

  bool put_prefix_at_end = FALSE; /* Do we put a prefix at the end? */
  if (!obj || !name || !colorname || obj->wear_pos <= ITEM_WEAR_NONE ||
      obj->wear_pos >= ITEM_WEAR_MAX || obj->wear_pos == ITEM_WEAR_BELT)
    return;
  
  wd = preset_names;
  while (wd && *wd)
    {
      wd = f_word (wd, word);
      if (!str_cmp (word, "overall_color"))
	{
	  wd = f_word (wd, word);
	  if (atoi (word) > 0)	    
	    overall_color = atoi(word);
	  else if (LC(*word) >= 'a' && LC(*word) <= 'f')
	    overall_color = LC(*word)-'a'+ 10;
	}
    }
  
  /* Set up the name name. */
  fullname[0] = '\0';
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    {
      if (i == OBJECTGEN_NAME_A_AN ||
	  !*name[i])
	continue;
      if (*fullname)    
	strcat (fullname, " ");
      if (!*colorname[i] && *name[i])
	strcpy (colorname[i], name[i]);
      /* sprintf (fullname + strlen (fullname), "%s:  '%s'", objectgen_part_names[i], name[i]); */
      
      t = name[i];
      while (t && *t)
	{
	  t = f_word (t, namebuf);
	  if (str_cmp (namebuf, "the") &&
	      str_cmp (namebuf, "an") &&
	      str_cmp (namebuf, "a"))
	    {	      
	      strcat (fullname, namebuf);
	      if (t && *t)
		strcat (fullname, " ");
	    }
	}
    }
  
  free_str (obj->name);
  obj->name = new_str (fullname);
  
  /* Let the thing move the prefix to the end part of the time. */
  if (*name[OBJECTGEN_NAME_PREFIX] &&
      *name[OBJECTGEN_NAME_SUFFIX] &&
      nr (1,5) == 2)
    {
      put_prefix_at_end = TRUE;
      if (str_cmp (name[OBJECTGEN_NAME_A_AN], "the"))
	{
	  for (i = OBJECTGEN_NAME_PREFIX+1; i <= OBJECTGEN_NAME_TYPE; i++)
	    {
	      if (*name[i])
		{
		  strcpy (name[OBJECTGEN_NAME_A_AN], a_an (name[i]));
		  break;
		}
	    }
	}
    }

  /* Set up the short and long descs. */
  fullname[0] = '\0';
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    { 
      
      if (!*name[i] || isspace(*name[i]) ||
	  (i == OBJECTGEN_NAME_PREFIX && 
	   put_prefix_at_end))
	continue;
      if (i == OBJECTGEN_NAME_METAL && *name[OBJECTGEN_NAME_GEM])
	{
	  if (obj->wear_pos == ITEM_WEAR_WIELD)
	    strcat (fullname, "$7-encrusted ");
	  else if (wear_data[obj->wear_pos].how_many_worn == 1)
	    strcat (fullname, "$7-studded ");
	  else
	    { /* Jewelry is gem and metal jewelry...*/
	      if (*name[OBJECTGEN_NAME_METAL])
		{
		  if (nr (1,2) == 2)
		    strcat (fullname, "$7-encrusted ");
		  else
		    strcat (fullname, " $7and ");
		}
	      else
		strcat (fullname, " ");	      
	    }
	}  
     
      else if (*fullname)
	strcat (fullname, " ");
      
      
      if (i == OBJECTGEN_NAME_SUFFIX) /* Suffix -- of life etc... */
	strcat (fullname, "of ");
      
      if (i == OBJECTGEN_NAME_CALLED) /* called 'soulstealer' */
	{
	  sprintf (fullname + strlen (fullname), "%s '%s'",
		   (nr (1,2) == 1 ? "called" : "named"),
		   (*colorname[i] ? colorname[i] : name[i])); 
	}
      else 
	{
	  /* Put a prefix and suffix at the end. If the suffix has the
	     word "the" in it, we must remove it first and put it before
	     the prefix, then do the suffix, otherwise we just do the 
	     prefix/suffix normally. */
	  
	  if (i == OBJECTGEN_NAME_SUFFIX && 
	      put_prefix_at_end)
	    { 
	      char word[STD_LEN], *secondword;
	      secondword = f_word (name[OBJECTGEN_NAME_SUFFIX], word);
	      if (!str_cmp (word, "the"))
		{
		  strcat (fullname, "the ");
		  if (*colorname[OBJECTGEN_NAME_PREFIX])
		    strcat (fullname, colorname[OBJECTGEN_NAME_PREFIX]);
		  else
		    strcat (fullname, name[OBJECTGEN_NAME_PREFIX]);
		  strcat (fullname, " ");
		  
		  strcat (fullname, secondword);
		}
	      else
		{ 
		  if (*colorname[OBJECTGEN_NAME_PREFIX])
		    strcat (fullname, colorname[OBJECTGEN_NAME_PREFIX]);
		  else
		    strcat (fullname, name[OBJECTGEN_NAME_PREFIX]);
		  strcat (fullname, " ");
		  if (*colorname[OBJECTGEN_NAME_SUFFIX])
		    strcat (fullname, colorname[OBJECTGEN_NAME_SUFFIX]);
		  else
		    strcat (fullname, name[OBJECTGEN_NAME_SUFFIX]);
		  
		}
	    }
	  else
	    {
	      if (*colorname[i])
		strcat (fullname, colorname[i]);
	      else
		strcat (fullname, name[i]);
	    }
	}
      
      /* Cloaks/leather/cloth/hide armor are metal-woven armor,
	 not metal armor. Also add the woven look to a small percentage
	 of nonweapons. */
      if (i == OBJECTGEN_NAME_METAL &&
	  *name[OBJECTGEN_NAME_METAL] &&
	  obj->wear_pos != ITEM_WEAR_WIELD &&
	  ((obj->wear_pos == ITEM_WEAR_ABOUT ||
	    strstr (name[OBJECTGEN_NAME_TYPE], "leather") ||
	    strstr (name[OBJECTGEN_NAME_TYPE], "cloth") ||
	    strstr (name[OBJECTGEN_NAME_TYPE], "woven") ||
	    strstr (name[OBJECTGEN_NAME_TYPE], "hide") ||
	    nr (1,52) == 3)))
	{
	  strcat (fullname, find_gen_word (OBJECTGEN_AREA_VNUM, "metal_weaves", NULL));
	}
    }
  
  /* Now possibly set the overall name of the object to something
     different than just gray. You can set this outside objectgen
     by putting the world "overall_color" into the list of names
     passed to objectgen, and if the number is a hex digit or an int
     from 1-15, it gets sent here and colors the object one main color. */
  
  strcpy (realfullname, add_color (fullname));
  if (nr (1,3) == 3 || (overall_color > 0 && overall_color <= 15))
    {
      char *t, c;
      char tempname[STD_LEN];
      int num = nr (1,15);
      
      /* If the color has been preset, then we use it, otherwise we
	 use the random number we choose. */
      if (overall_color > 0)
	num = overall_color;
      c = int_to_hex_digit (num);

      
      sprintf (tempname, "\x1b[%d;3%dm", num/8, num%8);
      strcat (tempname, realfullname);
      strcpy (realfullname, tempname);
      for (t = realfullname; *t; t++)
	{
	  if (*t == '\x1b')
	    {
	      t += 2;
	      if (*t == '0' &&
		  *(t + 3) == '7' && 
		  *(t + 5) != '\0')
		{
		  *t = num/8 + '0';
		  *(t+3) = num % 8 + '0';
		}
	      t += 4;
	    }
	} 
      strcat (realfullname, "\x1b[0;37m");
      
    }
  
		


  
  free_str (obj->short_desc);
  obj->short_desc = new_str (realfullname);
  
  /* Put a prefix here. */
  
  longname[0] = '\0';
  if (!*name[OBJECTGEN_NAME_OWNER])
    {
      realfullname[0] = LC(realfullname[0]);
      longname[0] = '\0';
      switch (nr (1,8))
	{
	  case 1:
	  case 2:
	  default:
	    break;
	  case 5:
	    sprintf (longname, "You notice ");
	    break;
	  case 6:
	    sprintf (longname, "You see ");
	    break;
	  case 7:
	    sprintf (longname, "You see that ");
	    break;
	  case 8:
	    sprintf (longname, "You notice that ");
	    break;
	}
    }
  strcat (longname, realfullname);
  strcat (longname, "\x1b[0;36m ");
  
  switch (nr (1,8))
    {
      case 1:
      case 2:
	strcat (longname, "is here.");
	break;
      case 3:
	strcat (longname, "has been left here.");
	break;
      case 4:
	strcat (longname, "is lying here.");
	break;
      case 5:
	strcat (longname, "was left here by somebody.");
	break;
      case 6:
	strcat (longname, "is on the ground.");
	break;
      case 7:
	strcat (longname, "was left here.");
	break;
      case 8:
	strcat (longname, "has been abandoned here.");
	break;
    }
  strcat (longname, "\x1b[0;37m");
  free_str (obj->long_desc);
  strcpy (longname, capitalize (longname));
  obj->long_desc = new_str (longname);
  
  
  
  return;
}

/* This returns what kind of part name this word is...used for presetting
   names on objects. */

int 
find_objectgen_part_name (char *nametype)
{
  int part;
  if (!nametype || !*nametype)
    return OBJECTGEN_NAME_MAX;
  
  for (part = 0; part < OBJECTGEN_NAME_MAX; part++)
    {
      if (!str_cmp (objectgen_part_names[part], nametype))
	break;
    }
  return part;
}


/* This generates the objectgen names. It puts the names into the
 name[][] array and the colornames (if any) into the colorname
 array. The colornames are set up by putting a line like
 name & colorname into whatever string is being used to generate
 the names.  The names variable is a list of names used at the
 end of generation to override/set names to certain preset things. */

void
objectgen_generate_names (char name[OBJECTGEN_NAME_MAX][STD_LEN], 
			  char colorname[OBJECTGEN_NAME_MAX][STD_LEN],
			  int wear_loc, int weapon_type, int level, 
			  char *names)
{
  int i;
  char *t;
  int names_left = 0, num_names_used;

  /* Used for setting up user preset nametypes using the names variable. */
  char arg1[STD_LEN], *arg;
  int name_part = OBJECTGEN_NAME_MAX, 
    name_parts_used = 0;
  
  
  if (!name || !colorname)
    return;
  
  if (level < 0)
    level = 0;
  
  /* Generate the names. */
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    {
      name[i][0] = '\0';
      colorname[i][0] = '\0';
      
      if (i == OBJECTGEN_NAME_METAL && nr (1,8) == 2)
	strcpy (name[i], find_gen_word 
		(OBJECTGEN_AREA_VNUM,
		 "metal_gen_names", colorname[i]));
      else if (i == OBJECTGEN_NAME_ANIMAL)
	strcpy (name[i], objectgen_animal_part_name (wear_loc, weapon_type)); 
      else if (i == OBJECTGEN_NAME_SUFFIX &&
	       nr (1,10) == 3)
	{
	  /* Give a chance to find an animal name. */
	  if (nr (1,3) == 2)
	    strcpy (name[i], objectgen_animal_suffix_name());
	  else if (wear_loc > ITEM_WEAR_NONE && wear_loc < ITEM_WEAR_MAX &&
		   wear_loc != ITEM_WEAR_BELT)
	    {
	      /* wpns -> bad godly sphereres, jewelry, either one,
		 armor -> good spheres. */
	      strcpy (name[i], find_gen_word 
		      (HISTORYGEN_AREA_VNUM, 
		       (wear_data[wear_loc].how_many_worn > 1 &&
			(wear_loc == ITEM_WEAR_WIELD || nr (1,2) == 2) ? 
			"godly_spheres_bad" :
			"godly_spheres_good"),
		       NULL));
	    }
	}
      /* Add "profession's" prefix. */
      else if (i == OBJECTGEN_NAME_PREFIX && nr (1,12) == 3)
	
	{
	  strcpy (name[i], find_random_caste_name_and_flags (BATTLE_CASTES, NULL));
	  possessive_form (name[i]);
	}
      else if (i == OBJECTGEN_NAME_TYPE)
	strcpy (name[i], objectgen_find_typename (wear_loc, weapon_type)); 

      /* Used for "a sword called 'foodancer'" or stuff like that. */
      else if (i == OBJECTGEN_NAME_CALLED)
	{
	  char suffix_name[STD_LEN];
	  /* used called_prefix then called_suffix here. */
	  sprintf (name[i], "%s", find_gen_word (OBJECTGEN_AREA_VNUM, "called_prefix", NULL));
	  *name[i] = UC(*name[i]);
	  strcpy (suffix_name, find_gen_word (OBJECTGEN_AREA_VNUM, "called_suffix", NULL));
	  
	  
	  /* Since words like fire and storm can be prefixes and suffixes,
	     make sure you don't use the same word twice. :P */
	  if (str_cmp (name[i], suffix_name))
	    strcat (name[i], suffix_name);
	  else
	    name[i][0] = '\0';
	}
      else /* Generic name. */
	strcpy (name[i], find_gen_word 
		(OBJECTGEN_AREA_VNUM,
		 (char *) objectgen_part_names[i], colorname[i]));
     
    }
  
 
  /* Now get the preset part names (if any) */

  if (names && *names)
    {
      arg = names;
      
      while (arg && *arg)
	{
	  arg = f_word (arg, arg1);
	 
	  /* Find the part name here if possible... */
	  if ((name_part = find_objectgen_part_name (arg1)) != OBJECTGEN_NAME_MAX)
	    { 
	      
	      arg = f_word (arg, arg1);
	      
	      /* If it exists, find the name associated with it. */
	      if (*arg1)
		{
		  if (name_part == OBJECTGEN_NAME_OWNER)
		    *arg1 = UC (*arg1);			 
		  strcpy (name[name_part], arg1);
		  /* Check if this name has colors associated with it. */
		  if (*arg == '&' && isspace (*(arg+1)))
		    {
		      arg = f_word (arg, arg1);
		      arg = f_word (arg, arg1);
		      if (*arg1)
			strcpy (colorname[name_part], arg1);
		    }
		  else
		    strcpy (colorname[name_part], name[name_part]);
		  /* Set this as one of the name parts we 
		     MUST use. */
		  name_parts_used |= (1 << name_part);
		}
	    }
	}
    }

  /* Choose metal or nonmetal or animal genned. 

  Metal is for all. nonmetal is for armor, animal is for all. 
  
  Breakdown: 70pct metal, 20pct animal, 10pct nonmetal. 
  Overridden if you require certain name parts to be used. */
  
  if (IS_SET (name_parts_used, (1 << OBJECTGEN_NAME_METAL)) ||
      wear_loc == ITEM_WEAR_WIELD)
    {
      del_obj_name (NONMETAL);
      del_obj_name (ANIMAL);
      /* Check for "mail" items...since mail wpns is stupid. :P */
      if (strstr (name[OBJECTGEN_NAME_METAL], "mail"))
	{
	  del_obj_name(METAL);
	}
    }
  else if (IS_SET (name_parts_used, (1 << OBJECTGEN_NAME_NONMETAL)))
    {
      del_obj_name(METAL);
      del_obj_name(ANIMAL);
    }
  else if (IS_SET (name_parts_used, (1 << OBJECTGEN_NAME_ANIMAL)))
    {
      del_obj_name(NONMETAL);
      del_obj_name(METAL);
    }
  else if ((nr (1,10) <= 7 ||
	    (!*name[OBJECTGEN_NAME_NONMETAL] &&
	     !*name[OBJECTGEN_NAME_ANIMAL])) && 
	   *name[OBJECTGEN_NAME_METAL])
    {
      del_obj_name (NONMETAL);
      del_obj_name (ANIMAL);
    }      
  /* Allow animal names */ 
  else if (*name[OBJECTGEN_NAME_ANIMAL] &&
	   (nr (1,3) != 2 || 
	    !*name[OBJECTGEN_NAME_NONMETAL] ||
	    wear_loc <= ITEM_WEAR_NONE || wear_loc >= ITEM_WEAR_MAX ||
	    wear_loc == ITEM_WEAR_BELT))
    {
      del_obj_name (NONMETAL);
      del_obj_name (METAL);
    }
  else
    {
      del_obj_name (METAL);
      del_obj_name (ANIMAL);
    }
  
  if (IS_SET (name_parts_used, (1 << OBJECTGEN_NAME_CALLED)))
    {
      del_obj_name (SUFFIX);
    }
  else if (IS_SET (name_parts_used, (1 << OBJECTGEN_NAME_SUFFIX)))
    {
      del_obj_name (CALLED);
    }
  
  
  /* Now deal with suffix and "called names". Can only have one. */

  else if (nr (1, 25) == 3 && *name[OBJECTGEN_NAME_CALLED])
    {
      del_obj_name (SUFFIX);
    }
  else
    {
      del_obj_name (CALLED);
    }
  
  
  /* The typename really has to exist... */
  
  
  if (!*name[OBJECTGEN_NAME_TYPE])
    {
      if (wear_loc == ITEM_WEAR_WIELD)
	strcpy (name[OBJECTGEN_NAME_TYPE], "weapon");
      else
	strcpy (name[OBJECTGEN_NAME_TYPE], "armor");
    }
  
  
  /* Do some sanity checking. */
  
  if (IS_SET (name_parts_used, (1 << OBJECTGEN_NAME_METAL)) &&
      (!str_cmp (name[OBJECTGEN_NAME_PREFIX], "soft") ||
       wear_loc == ITEM_WEAR_WIELD))
    {
      del_obj_name (PREFIX);
    }
  else if (!IS_SET (name_parts_used, (1 << OBJECTGEN_NAME_METAL)) &&
	   !str_cmp (name[OBJECTGEN_NAME_PREFIX], "rusty"))
    {
      del_obj_name(PREFIX);
    }
  
  /* Set names_used to have bits set for all nonnull names that won't
     be fixed. */
  
  name_parts_used |= ((1 << OBJECTGEN_NAME_TYPE) | (1 << OBJECTGEN_NAME_A_AN));
  
  if (*name[OBJECTGEN_NAME_OWNER])
    {
      /* Most of the time keep the name on the front, otherwise move
	 it to the back where the suffix is. */
      
       *name[OBJECTGEN_NAME_A_AN] = '\0';
       
       if (nr (1,6) == 2)
	 {
	   del_obj_name (OWNER);
	  SBIT (name_parts_used, (1 << OBJECTGEN_NAME_SUFFIX));
	  strcpy (name[OBJECTGEN_NAME_SUFFIX],
		  name[OBJECTGEN_NAME_OWNER]);
	  strcpy (colorname[OBJECTGEN_NAME_SUFFIX],
		  colorname[OBJECTGEN_NAME_OWNER]);
	  strcpy (name[OBJECTGEN_NAME_A_AN], "The");
	  strcpy (colorname[OBJECTGEN_NAME_A_AN], "The");
	 }
    } 
  
  

  /* If there's a suffix, sometimes change the "A/An" into a "The" */
  else if (*name[OBJECTGEN_NAME_SUFFIX] && nr (1,12) == 3)
    {
      strcpy (name[OBJECTGEN_NAME_A_AN], "The");
      strcpy (colorname[OBJECTGEN_NAME_A_AN], "The");
    }
  num_names_used = 0;
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    {
      if (IS_SET (name_parts_used, (1 << i)))
	num_names_used++;
    }
  
  names_left = nr (1, level/50+2);

  if (num_names_used + names_left > 3 && IS_SET (name_parts_used, (1 << OBJECTGEN_NAME_SOCIETY)))
    names_left--;
  
  /* Cut down on the number of names. */
  
  if (num_names_used + names_left > 4)
    {
	names_left = MAX(1,4-num_names_used);
    }
  
  /* Choose from the remaining descriptive names. You only pick
     from things that are not mutexed and which have names and which
     haven't been picked yet. */
  while (names_left > 0)
    {
      int count, num_choices = 0, num_chose = 0, nm;
      for (count = 0; count < 2; count++)
	{
	  for (nm = 0; nm < OBJECTGEN_NAME_MAX; nm++)
	    {
	      if (*name[nm] && !IS_SET (name_parts_used, (1 << nm)))
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
	      num_chose = nr (1,num_choices);
	    }
	}
      if (*name[nm] && !IS_SET (name_parts_used, (1 << nm)))
	SBIT (name_parts_used, (1 << nm));
      names_left--;
    }
	

  
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    {
      if (!IS_SET (name_parts_used, (1 << i)))
	*name[i] = '\0';
    }
  
  /* Find the a_an at the start of the name. */
  
  for (t = name[OBJECTGEN_NAME_TYPE]; *t; t++);
  t--;
  /* Go back past any quotes or whitespace in the name. */
  while ((*t == '\'' || *t == '\"' || isspace (*t)) &&
	 t > name[OBJECTGEN_NAME_TYPE])
    t--;
  
  /* If the owner_name exists, the a_an gets set to empty. */
  
  
  *name[OBJECTGEN_NAME_A_AN] = '\0';
  *colorname[OBJECTGEN_NAME_A_AN] = '\0';
  if (!*name[OBJECTGEN_NAME_OWNER])
    {
      if (LC (*t) == 's'  &&
	  str_cmp (name[OBJECTGEN_NAME_TYPE], "cutlass") &&
	  str_cmp (name[OBJECTGEN_NAME_TYPE], "cuirass"))
	{
	  if ((wear_loc == ITEM_WEAR_BODY &&
	       strstr (name[OBJECTGEN_NAME_TYPE], "mail")) || nr (1,3) != 2)  
	    strcpy (name[OBJECTGEN_NAME_A_AN], "some");
	  else
	    strcpy (name[OBJECTGEN_NAME_A_AN], "a pair of");
	}
      else
	{
	  for (i = 0; i < OBJECTGEN_NAME_MAX-1; i++)
	    {
	      if (*name[i])
		{
		  strcpy (name[OBJECTGEN_NAME_A_AN], a_an(name[i]));
		  break;
		}
	    }	 
	}
      
    }
  else
    {
      possessive_form (name[OBJECTGEN_NAME_OWNER]);
      possessive_form (colorname[OBJECTGEN_NAME_OWNER]);
    }
  return;
}
  
 




/* This finds a wear location at random. It has a 25% chance of picking
   a wielded item (Weapon) a 60% chance of picking a large armor
   (worn only one location) and a 15% chance of picking small armor
   (jewelry and such). */

int
objectgen_find_wear_location (void)
{
  int num_single_choices = 0, num_multi_choices = 0;
  int num_chose;
  int wearloc;
  int obj_type; /* Wpn = 0 big armor = 1 jewelry = 2 */

  switch (nr(1,10))
    {
      case 3:
      case 6:
      case 2:
	obj_type = 0; /* 30pct */
	break;
      case 9:
      case 4:
	obj_type = 2; /* 20pct */
	break;
      default:
	obj_type = 1; /* 50pct */
	break;
    }

  for (wearloc = 0; wear_data[wearloc].flagval < ITEM_WEAR_MAX; wearloc++)
    {
      if (wearloc == ITEM_WEAR_WIELD)
	continue;
      if (wear_data[wearloc].how_many_worn == 1)
	num_single_choices++;
      else if (wear_data[wearloc].how_many_worn > 1)
	num_multi_choices++;
    }

  if (obj_type == 0) /* Weapon */
    wearloc = ITEM_WEAR_WIELD;
  else if (obj_type == 1) /* Armor */
    {
      if (num_single_choices < 1)
	return ITEM_WEAR_NONE;
      num_chose = nr (1, num_single_choices);
      
      for (wearloc = 0; wear_data[wearloc].flagval < ITEM_WEAR_MAX; wearloc++)
	{
	  if (wearloc == ITEM_WEAR_WIELD)
	    continue;
	  if (wear_data[wearloc].how_many_worn == 1 &&
	      --num_chose < 1)
	    break;
	}
    }
  else if (obj_type == 2)
    {
      if (num_multi_choices < 1)
	return ITEM_WEAR_NONE;
      num_chose = nr (1, num_multi_choices);
      
      for (wearloc = 0; wear_data[wearloc].flagval < ITEM_WEAR_MAX; wearloc++)
	{
	  if (wearloc == ITEM_WEAR_WIELD)
	    continue;
	  if (wear_data[wearloc].how_many_worn > 1 &&
	      --num_chose < 1)
	    break;
	}
    }
  
  if (wearloc <= ITEM_WEAR_NONE || wearloc >= ITEM_WEAR_MAX ||
      wearloc == ITEM_WEAR_BELT)
    return ITEM_WEAR_NONE;
  return wearloc;
}


/* This finds the typename for objects with certain wear locations. 
   There's some special coding to handle weapons. */

char *
objectgen_find_typename (int wear_loc, int weapon_type)
{
  static char retbuf[STD_LEN];
  char typebuf[STD_LEN];
  char color = '7';
  
  retbuf[0] = '\0';
  typebuf[0] = '\0';
  if (wear_loc <= ITEM_WEAR_NONE || wear_loc >= ITEM_WEAR_MAX ||
      wear_loc == ITEM_WEAR_BELT || 
      (wear_loc == ITEM_WEAR_WIELD &&
       (weapon_type < 0 || weapon_type >= WPN_DAM_MAX)))
    return retbuf;
  
  if (wear_loc == ITEM_WEAR_WIELD)
    strcpy (typebuf, weapon_damage_types[weapon_type]);
  else
    strcpy (typebuf, wear_data[wear_loc].name);
  
  strcpy (retbuf, find_gen_word (OBJECTGEN_AREA_VNUM, typebuf, &color));
  return retbuf;
}


/* This sets up the stats for the object. It is assumed that the 
   wear_pos of the object is already set. */

void
objectgen_setup_stats (THING *obj, int weapon_type)
{
  FLAG *flg;
  VALUE *val;
  int total_flags, curr_total_flags, rank_choice;
  int lev_sqrt;  /* Needed since item power is proportional to the
		    sqrt of its level. */
  int main_part; /* Main part that the armor protects. */
  int i, rank, times, gen_tries;
  int obj_flags = 0;
  
  /* Number of flags of each affect rank type. */
  
  int num_flags[AFF_RANK_MAX];
  /* Actual flags used in each rank. */
  int aff_used[MAX_AFF_PER_RANK];
  if (!obj ||  IS_AREA (obj) || IS_ROOM (obj) ||
      !obj->in || !IS_AREA (obj->in))
    return;
  
  
  if (obj->wear_pos <= ITEM_WEAR_NONE || obj->wear_pos >= ITEM_WEAR_MAX ||
      obj->wear_pos == ITEM_WEAR_BELT)
    return;

  if (obj->level < 10)
    obj->level = 10;
  
  /* if (obj->level > 300)
     obj->level = 300; */
  
  obj->level += nr (0, obj->level/3);
  obj->level -= nr (0, obj->level/4);
  
  /* CHEATER, make objs better. */
  obj->level += obj->level/3;
  
  
  for (lev_sqrt = 1; lev_sqrt*lev_sqrt <= obj->level; lev_sqrt++);
  
  obj->cost = obj->level*obj->level;
  
  if (obj->wear_pos == ITEM_WEAR_WIELD)
    {
      if ((val = FNV (obj, VAL_WEAPON)) == NULL)
	{
	  val = new_value();
	  add_value (obj, val);
	}

      obj->height = nr (20,40);
      obj->weight = MAX(30, obj->level+30);
      
      val->type = VAL_WEAPON;
      
      val->val[0] = nr (lev_sqrt/2,lev_sqrt*2/3);
      val->val[1] = nr(lev_sqrt, lev_sqrt*2);
      val->val[0] += nr (1,2);
      val->val[2] += nr (2,4);
      if (weapon_type < 0 || 
	  weapon_type >= WPN_DAM_MAX)
	weapon_type = WPN_DAM_SLASH;
      
      val->val[2] = weapon_type;
      
      /* Adjust damage based on wpn type. */

      if (weapon_type == WPN_DAM_PIERCE)
	{
	  val->val[0] -= nr (0, val->val[0]/8);
	  val->val[1] -= nr (0, val->val[1]/8);
	}
      if (weapon_type == WPN_DAM_WHIP ||
	  weapon_type == WPN_DAM_CONCUSSION)
	{
	  val->val[0] += nr (0, val->val[0]/10);
	  val->val[1] += nr (0, val->val[1]/10);
	}
      
      /* Add extra attacks and weaken the object if it's too powerful. */
      
      if (nr (1,3) == 2 &&
	  (weapon_type == WPN_DAM_WHIP ||
	   weapon_type == WPN_DAM_CONCUSSION))
	val->val[3] += nr (0,obj->level)/120;
      
      if (val->val[3] > 0)
	{
	  val->val[0] -= nr (0, val->val[0]/5);
	  val->val[1] -= nr (0, val->val[1]/5);
	}
      
   
    }
  else /* Armors */
    {
      if ((val = FNV (obj, VAL_ARMOR)) == NULL)
	{
	  val = new_value();
	  add_value (obj, val);
	}
      val->type = VAL_ARMOR;
      
      main_part = wear_data[obj->wear_pos].part_of_thing;
      val->val[main_part] = lev_sqrt;
      
      /* Add the warmth. Can be + or - */
      
      val->val[6] = nr (0, lev_sqrt/2) - nr (0, lev_sqrt/2);
      if (wear_data[obj->wear_pos].how_many_worn > 1)
	{
	  obj->weight = nr (2,10);
	  obj->height = nr (2,10);
	  obj->cost *= nr (3,5);
	  val->val[main_part] = val->val[main_part]*2/3;
	}
      else
	{
	  obj->height = nr (10, 30);
	  if (obj->wear_pos != ITEM_WEAR_BODY)
	    obj->weight = 10+obj->level/10;
	  else
	    obj->weight = 40+obj->level/5;
	}
		
      
      if (obj->wear_pos == ITEM_WEAR_ABOUT ||
	  obj->wear_pos== ITEM_WEAR_SHIELD)
	{
	  for (i = 0; i < PARTS_MAX; i++)
	    {
	      if (i != main_part)
		val->val[i] = val->val[main_part]/2;
	    }
	}
    }
  
  /* Now set up the flags on the object. */
  
  total_flags = nr (obj->level/50, obj->level/25);  
  if (total_flags > 6)
    total_flags = 6;
  if (total_flags == 0 && nr (1,3) == 2)
    total_flags = 1;
  
  curr_total_flags = 0;
  for (i = 0; i < AFF_RANK_MAX; i++)
    {
      num_flags[i] = MIN(MAX_AFF_PER_RANK, obj->level/aff_rank_levels[i]);
      curr_total_flags += num_flags[i];
    }
  
  
  /* If we want more flags than we can put on the thing...
     remove something. */

  if (curr_total_flags < total_flags)
    total_flags = curr_total_flags;
  else if (curr_total_flags > total_flags)
    {
      while (curr_total_flags > total_flags)
	{
	  rank_choice = nr (0, AFF_RANK_MAX-1);
	  
	  if (num_flags[rank_choice] > 0)
	    {
	      num_flags[rank_choice]--;
	      curr_total_flags--;
	    }
	}
    }

  /* Loop through all ranks and find flags from each rank. */
  
  for (rank = 0; rank < AFF_RANK_MAX; rank++)
    {
      for (times = 0; times < MIN (MAX_AFF_PER_RANK, num_flags[rank]); times++)
	{
	  aff_used[times] = 0;
	  gen_tries = 0;
	  do 
	    {
	      aff_used[times] = find_aff_with_rank (rank);
	      
	      for (i = 0; i < times; i++)
		{
		  if (aff_used[i] == aff_used[times])
		    {
		      aff_used[times] = 0;
		      break;
		    }
		}
	    }
	  while (aff_used[times] == 0 && gen_tries++ < 20);
	  
	  
	  /* Now add all of the flags. */
	  if (aff_used[times] >= AFF_START &&
	      aff_used[times] < AFF_MAX)
	    {
	      flg = new_flag();
	      flg->from = 0;
	      flg->type = aff_used[times];
	      flg->timer = 0;
	      

	      /* These are the powers of the items based on the
		 level_sqrt number. */
	      
	      if (rank == AFF_RANK_POOR)		
		flg->val = nr (1, lev_sqrt*5/4);
	      else if (rank == AFF_RANK_FAIR)
		flg->val = nr (1, lev_sqrt/3);
	      else if (rank == AFF_RANK_GOOD)
		flg->val = nr (1, lev_sqrt/4);
	      else 
		flg->val = nr (1, lev_sqrt/9);
	      if (obj->wear_pos != ITEM_WEAR_WIELD &&
		  wear_data[obj->wear_pos].how_many_worn > 1)
		flg->val = flg->val*2/3;
	      if (flg->val < 1)
		flg->val = 1;
	      flg->next = obj->flags;
	      obj->flags = flg;
	    }
	}
    }

  /* Now add special object flags like glow/hum/power etc... so that
     weapons can hit better creatures. */

  if (nr (1,1000) <= obj->level)
    obj_flags |= OBJ_MAGICAL;
  
  if (LEVEL(obj) > 150 || (num_flags[AFF_RANK_EXCELLENT] > 0))
    obj_flags |= OBJ_NOSTORE;

  if (obj->wear_pos != ITEM_WEAR_WIELD)
    {
      if (nr (1,500) <= obj->level)
	obj_flags |= OBJ_GLOW;
      if (nr (1,500) <= obj->level)
	obj_flags |= OBJ_HUM;
    }
  else
    {
      for (i = 0; obj1_flags[i].flagval != 0 &&
	     obj1_flags[i].flagval <= OBJ_EARTH; i++)
	{
	  if (IS_SET (obj1_flags[i].flagval, OBJ_GLOW | OBJ_HUM))
	    {
	      if (nr (1,250) < obj->level)
		obj_flags |= obj1_flags[i].flagval;
	    }
	  else if (obj->level >= 100 && nr (1,500) < obj->level)
	    {
	      obj_flags |= obj1_flags[i].flagval;
	    }
	}
    }

  if (obj_flags)
    add_flagval (obj, FLAG_OBJ1, obj_flags);
  

  return;
}

void
do_metalgen (THING *th, char *arg)
{
  THING *obj;
  if (!IS_PC (th) || LEVEL (th) != MAX_LEVEL)
    return;
  
  generate_metal_names();
  if ((obj = find_gen_object (OBJECTGEN_AREA_VNUM, "metal_gen_names")) == NULL ||
      !obj->desc || !*obj->desc)
    {
      stt ("Couldn't generate metal names.\n\r", th);
      return;
    }
  stt ("The metal names:\n\n\r", th);
  stt (obj->desc, th);
  return;
}

/* This clears and generates a list of "fake metal" names for use when making
   objects...*/

void
generate_metal_names (void)
{
  THING *obj; /* The object where the metal names will be stored. */
  char buf[10*STD_LEN]; /* Where the complete string gets stored. */
  char name[STD_LEN];   /* Where we store the name. */
  char colorname[STD_LEN]; /* Where we store the color name. */
  char *t;
  int i;
  bool name_ok = FALSE;
  int num_generated; /* How many metal names to generate. */
  /* Keeping track of the colors. */
  int color[3], num_colors;
  int name_length; /* Used when setting up colors. */
  
  if ((obj = find_gen_object (OBJECTGEN_AREA_VNUM, "metal_gen_names")) == NULL)
    return;

  
 
  buf[0] = '\0';
  
  num_generated = nr (7,15) + 8;
  for (i = 0; i < num_generated; i++)
    {
      do
	{
	  name_ok = TRUE;
	  strcpy (name, create_society_name (NULL));
	  
	  /* Crop the name. */
	  name[nr(4,5)] = '\0';
	  
	  for (t = name; *t; t++)
	    /* Make sure that everything is a letter and not a 
	       weird letter. */
	    if (!isalpha(*t))
	      name_ok = FALSE;
	  
	  /* Move back to a consonant with a vowel before it. */
	 
	  t--;
	  while (t > name)
	    {
	      if (!isvowel (*t) &&
		  isvowel (*(t-1)))
		break;
	      t--;
	    }
	  *(t+1) = '\0';
	  
	  /* Make sure the name is long enough. */
	  if (t < name + 2)
	    name_ok = FALSE;
	}
      while (!name_ok);
      
      switch (nr (1,4))
	{
	  case 1:
	    strcat (name, "ite");
	    break;
	  case 2:
	    strcat (name, "ate");
	    break;
	  case 3:
	  case 4:
	  default:
	    strcat (name, "ium");
	    break;
	}
      
      name_length = strlen(name);
      if (nr (1,9) == 2)
	num_colors = 0;
      else if (nr (1,2) == 1)
	num_colors = 1;
      else if (nr (1,5) == 2)
	num_colors = 2;
      else 
	num_colors = 3;
      colorname[0] = '\0';
      if (num_colors > 0)
	{
	  sprintf (colorname, " & ");
	  /* Get the first colors all set to one thing. */
	  color[2] = color[1] = color[0] = nr (1,15);
	  
	  /* Make up the second color. */
	  while (color[1] == color[0])
	    color[1] = nr (1,15);
	  sprintf (colorname + strlen(colorname), "$%c", int_to_hex_digit (color[0]));
	  for (t = name; *t; t++)
	    {
	      if (t-name == name_length/num_colors)
		sprintf (colorname + strlen (colorname), "$%c",
			 int_to_hex_digit (color[1]));
	      if (t-name == 2*name_length/num_colors)
		sprintf (colorname + strlen (colorname), "$%c",
			 int_to_hex_digit (color[2]));
	      sprintf (colorname + strlen (colorname), "%c", *t);
	    }
	  strcat (colorname, "$7");

	}
      strcat (buf, name);
      strcat (buf, colorname);
      strcat (buf, "\n\r");
    }
  free_str (obj->desc);
  obj->desc = new_str (add_color(buf));

  return;
}

/* This generates and object name using some kind of animal parts name. 
   It loops through all of the randmob generators that have edescs
   of the appropriate type: armor weapon jewelry, and then if that
   thing has the correct part type, it then counts the number of
   words in kname and (armor/jewelry/weapon) and multiplies them to
   get the total number of combinations possible. In the second pass, it
   of course then finds the correct word to use by subtracting these
   products until their choice is <= 0. The weapon types can only be
   slashing and piercing, or concussion if it has the word "bone" in its
   weapon location. */


char *
objectgen_animal_part_name (int wear_loc, int weapon_type)
{
  THING *proto, *area;
  
  char object_type[STD_LEN]; /* Armor/wpn/jewelry */
  /* Number of diff mob names and body part names for that thing. */
  int num_proto_names, num_object_names, total_names;
  
  int count, num_choices = 0, num_chose = 0;
  
  char animal_name[STD_LEN];
  char part_name[STD_LEN];
  static char whole_name[STD_LEN];
  char suffix_name[STD_LEN]; /* Used to put suffixes on items like
				-studded or -embedded...*/
  /* The extra descriptions with the object types allowed and the name
     of the thing. (Must use kname) */
  EDESC *typedesc = NULL, *namedesc = NULL;
  /* Make it more likely for things other than those with massive
     amounts of prefixes and item types to get a chance. */
  int square_total_names;
  
  if (wear_loc <= ITEM_WEAR_NONE ||
      wear_loc >= ITEM_WEAR_MAX ||
      wear_loc == ITEM_WEAR_BELT)
    return nonstr;
  
  if (wear_loc == ITEM_WEAR_WIELD && 
      (weapon_type < 0 || weapon_type >= WPN_DAM_MAX))
    return nonstr;
  
  if ((area = find_thing_num (MOBGEN_PROTO_AREA_VNUM)) == NULL)
    return nonstr;
  
  if (wear_loc == ITEM_WEAR_WIELD)
    strcpy (object_type, "weapon");
  else if (wear_data[wear_loc].how_many_worn <= 1)
    strcpy (object_type, "armor");
  else
    strcpy (object_type, "jewelry");
  
  whole_name[0] = '\0';
  for (count = 0; count < 2; count++)
    {
      for (proto = area->cont; proto; proto = proto->next_cont)
	{
	  /* Check if it has the proper edesc. */
	  if ((typedesc = find_edesc_thing (proto, object_type, TRUE)) == NULL ||
	      (namedesc = find_edesc_thing (proto, "kname", TRUE)) == NULL)	    
	    continue;
	  
	  num_proto_names = find_num_words (namedesc->desc);
	  num_object_names = find_num_words (typedesc->desc);
	  
	  square_total_names = num_proto_names*num_object_names;
	  
	  total_names = 0;
	  while (total_names*total_names < square_total_names)
	    total_names++;
	  
	  if (total_names < 1)
	    continue;
	  
	  if (count == 0)
	    num_choices += total_names;
	  else 
	    {
	      num_chose -= total_names;
	      if (num_chose < 1)
		break;
	    }
	}

      if (count == 0)
	{
	  if (num_choices < 1)
	    return nonstr;
	  num_chose = nr (1, num_choices);
	}
    }

  if (!proto || !namedesc || !typedesc)
    return nonstr;
  
  strcpy (animal_name, find_random_word (namedesc->desc, NULL));
  strcpy (part_name, find_random_word (typedesc->desc, NULL));
  suffix_name[0] = '\0';
  /* Animal weapon parts in weapons have to be handled more delicately.
     "bones" can go into piercing/slashing/conc. 
     
     teeth/fangs/claws/talons/pincers only go into slashing/piercing
     but can be embedded or spiked into conc/whipping. */

  if (wear_loc == ITEM_WEAR_WIELD)
    {
      if (weapon_type == WPN_DAM_WHIP)
	{
	  if (!named_in (part_name, "fang claw tooth"))
	    return nonstr;
	}
      else if (weapon_type == WPN_DAM_CONCUSSION)
	{
	  if (str_cmp (part_name, "bone"))
	    {
	      strcpy (suffix_name, find_random_word ("-studded -embedded -spiked", NULL));
	    }
	}
      else 
	suffix_name[0] = '\0';
    }
  /* Jewelry may have a -studded suffix for non feather things. */
  else if (!str_cmp (object_type, "jewelry"))
    {
      if (str_cmp (part_name, "feather") &&
	  str_cmp (part_name, "feathers") &&
	  nr (1,5) == 2)
	{
	  strcpy (suffix_name, find_random_word ("-studded", NULL));
	}
    }
  
  /* Now set up the name: animal_name(middle)part_name-suffix_name */
 
  /* No space in middle. */
  if (nr (1,5) != 3 &&
      named_in (part_name, "hide skin scale tooth claw fang bone"))
    sprintf (whole_name, "%s%s%s", animal_name, part_name, suffix_name);
  else if (nr (1,5) == 2 && !*suffix_name) /* Hyphen -- only if no suffix */
    sprintf (whole_name, "%s-%s", animal_name, part_name);
  else /* Space */
    {
      /* Check for possessive form. */
      if (nr (1,3) == 2)
	possessive_form (animal_name);
      sprintf (whole_name, "%s %s%s", animal_name, part_name, suffix_name);
    }
  
  return whole_name;
}


/* This generates a suffix for an item based on an animal name from
   the mobgen area. The prototype must be level 40+ and it picks
   randomly from all names. */

char *
objectgen_animal_suffix_name (void)
{
  int num_choices = 0, num_chose = 0, count;
  
  THING *area = NULL, *proto = NULL;
  EDESC *edesc = NULL;
  static char name[STD_LEN];
  
 if ((area = find_thing_num (MOBGEN_PROTO_AREA_VNUM)) == NULL)
    return nonstr;
 
  for (count = 0; count < 2; count++)
    {
      for (proto = area->cont; proto; proto = proto->next_cont)
	{
	  /* Check if it has the proper edesc and it's high enough level. */
	  if ((edesc = find_edesc_thing (proto, "kname", TRUE)) == NULL ||
	      LEVEL (proto) < OBJECTGEN_ANIMAL_SUFFIX_MINLEVEL)	    
	    continue;
	  
	  if (count == 0)
	    num_choices += find_num_words (edesc->desc);
	  else 
	    {
	      num_chose -= find_num_words (edesc->desc);
	      if (num_chose < 1)
		break;
	    }
	}

      if (count == 0)
	{
	  if (num_choices < 1)
	    return nonstr;
	  num_chose = nr (1, num_choices);
	}
    }

  if (!proto || !edesc)
    return nonstr;
  
  /* Now put the name into its proper place. */
  
  sprintf (name, "the %s", find_random_word (edesc->desc, NULL));
  return name;
}

int
find_aff_with_rank (int rank)
{
  int num_choices = 0, num_chose = 0, count, afftype;
  
  if (rank < AFF_RANK_POOR ||
      rank >= AFF_RANK_MAX)
    return 0;
  
  /* This is kind of a hack but I want more +damroll eq, so... */

  if (rank == AFF_RANK_EXCELLENT && nr (1,4) == 2)
    return FLAG_AFF_DAM;
  
  /* Find a random affect with the correct rank. */

  for (count = 0; count < 2; count++)
    {
      for (afftype = 0; afftype < AFF_MAX-AFF_START; afftype++)
	{
	  if (aff_power_ranks[afftype][AFF_POWER_RANK] == rank)
	    {
	      if (count == 0)
		num_choices += aff_power_ranks[afftype][AFF_POWER_CHANCE];
	      else
		{
		  num_chose -= aff_power_ranks[afftype][AFF_POWER_CHANCE];
		  if (num_chose < 1)
		    break;
		}
	    }
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return 0;
	  num_chose = nr (1, num_choices);
	}
    }

  afftype += AFF_START;
  return (afftype);
}


/* this calculates the reset percent of an object based on its level. */

int
objectgen_reset_percent (int level)
{
  return MID (2,  70-level*2/3, 50);
}
