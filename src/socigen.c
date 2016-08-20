#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "serv.h"
#include "society.h"
#include "areagen.h"
#include "objectgen.h"


/* This attempts to generate societies based on the list of things
   in the area with vnum SOCIETYGEN_AREA_VNUM. It first checks if
   there are any generated societies and if so it bails out. If
   not, it continues and goes down the list generating a society
   (with all of the mobs made) for every society name in the area. */

void
generate_societies (THING *th)
{
  SOCIETY *soc;
  THING *genarea, *mobarea, *proto;
  char buf[STD_LEN];
  /* First check if there are any generated societies already. */
  
  for (soc = society_list; soc; soc = soc->next)
    {
      if (soc->generated_from)
	{
	  stt ("There are generated societies already! Type society clearall to clear them, then reboot.\n\r", th);
	  return;
	}
    }

  
  if ((genarea = find_thing_num(SOCIETYGEN_AREA_VNUM)) == NULL ||
      !IS_AREA (genarea))
    {
      sprintf (buf, "The society generator area %d doesn't exist.\n\r", 
	       SOCIETYGEN_AREA_VNUM);
      stt (buf, th);
      return;
    }
  
  if ((mobarea = find_thing_num(SOCIETY_MOBGEN_AREA_VNUM)) == NULL ||
      !IS_AREA (mobarea))
    {
      sprintf (buf, "The society generated mob area %d doesn't exist.\n\r", 
	       SOCIETY_MOBGEN_AREA_VNUM);
      stt (buf, th);
      return;
    }
  
  if (mobarea->max_mv < 5000)
    {
      stt ("The society mob area must allow at least 5000 things into it.\n\r", th);
      return;
    }
  if (mobarea->mv > 5)
    {
      stt ("The society mob area can only allow 5 rooms in it.\n\r", th);
      return;
    }
  
  
  /* Now make sure that the mobarea is empty. */

  for (proto = mobarea->cont; proto; proto = proto->next_cont)
    {
      if (IS_ROOM (proto))
	continue;
      
      stt ("The society mob area has something in it other than rooms. You must delete everything from it except for the rooms!\n\r", th);
      return;
    }
  
  /* Now set all ungenned societies to align 0. */
  
  for (soc = society_list; soc; soc = soc->next)
    soc->align = 0;
  
  for (proto = genarea->cont; proto; proto = proto->next_cont)
    {
      if (IS_ROOM (proto))
	continue;
      if (!proto->name || !*proto->name)
	{
	  stt ("Object with no name in societygen area.\n\r", th);
	  continue;
	}
      generate_society(proto);
    }
  mobarea->thing_flags |= TH_CHANGED;
  return;
}


/* This now generates a society based on a thing that's a prototype
   for the society's name and attributes. */


SOCIETY *
generate_society (THING *proto)
{
  SOCIETY *soc = NULL;
  char arg1[STD_LEN];
  char *arg;
  int caste_tiers[CASTE_MAX];
  int caste_flags[CASTE_MAX];
  int caste_start[CASTE_MAX];
  int tier, castenum, cnum;
  int society_flags, proto_society_flags;
  /* The base caste level for this caste. */
  int base_caste_level; 
  bool mobs_done_once = FALSE;
  FLAG *oflg, *nflg, *cflg;
  char buf[STD_LEN];
  /* Used for stripping off adjectives to make several societies. */
  char *adjarg, adjbuf[STD_LEN];
  
  /* Names used to generate the society mobs. */

  char socinamebuf[STD_LEN];  /* The name of the society. */
  
  char tierbuf[STD_LEN]; /* The name of the current tier. */
  
  /* Used for picking what tier names we will end up using. */
  
  int tiernames_used, tiers_left, tier_choice, tier_choices_left,
    current_tier_place;
  char tiernames[SOCIETY_GEN_CASTE_TIER_MAX][STD_LEN];
  int num_tiernames; /* Number of tiernames found to choose from for
			the caste being generated. */
  bool did_adjective_society = FALSE;

  THING *mob, *mobarea;
  int curr_vnum = 0; /* This is the next vnum available to use
				 to make a mob for this society. */
  int max_vnum; /* This is the max vnum we might use to make a mob for
		   this society. */


  if (!proto || !proto->name || !*proto->name)
    return NULL;
  
  /* Max level of proto has to be set to something decent. */
  if (LEVEL (proto) < 10)
    proto->level = 100;
  
  if (!proto->short_desc)
    adjarg = nonstr;
  else
    adjarg = proto->short_desc;
  
  /* Loop through all adjectives. */

  do
    {
      adjarg = f_word (adjarg, adjbuf);
      
      /* If you've done an adjective and there are no more 
	 adjectives, don't allow this society to be made. */
      if (!*adjbuf && did_adjective_society)
	continue;
      
      /* If you put the word noadj into the adjective list then 
	 do create a society without an adjective. */
      if (!str_cmp (adjbuf, "noadj"))
	adjbuf[0] = '\0';
      
      if (*adjbuf)
	did_adjective_society = TRUE;

      soc = create_new_society();
      soc->level = proto->level;
      
      /* Generate the names. */
      
      arg = proto->name;
      free_str (soc->name);
      soc->name = nonstr;
      arg = f_word (arg, arg1);
      capitalize_all_words (arg1);
      soc->name = new_str (arg1);
      
      free_str (soc->pname);
      soc->pname = nonstr;
      arg = f_word (arg, arg1);
      capitalize_all_words (arg1);
      soc->pname = new_str (arg1);
      
      
      free_str (soc->aname);
      soc->aname = nonstr;
      arg = f_word (arg, arg1);
      capitalize_all_words (arg1);
      soc->aname = new_str (arg1);
      
      free_str (soc->adj);
      soc->adj = nonstr;
      capitalize_all_words (adjbuf);
      soc->adj = new_str (adjbuf);
      
      /* Now find out what castes we need. */

      proto_society_flags = flagbits (proto->flags, FLAG_SOCIETY);
      
      /* The flags are the base flags + the proto flags - any
	 proto flags that are base flags. */
      
      society_flags = SOCIETY_BASE_FLAGS ^ proto_society_flags;
      
      /* Now add flags to the society. */
      
      for (oflg = proto->flags; oflg; oflg = oflg->next)
	{
	  if (oflg->type == FLAG_SOCIETY)
	    continue;
	  for (cflg = soc->flags; cflg; cflg = cflg->next)
	    {
	      if (cflg->type == oflg->type &&
		  cflg->type <= NUM_FLAGTYPES)
		{
		  cflg->val |= oflg->val;
		  break;
		}
	    }
	  if (!cflg)
	    {
	      nflg = new_flag();
	      copy_flag (oflg, nflg);
	      nflg->next = soc->flags;
	      soc->flags = nflg;
	    }
	}
      if (!str_cmp (adjbuf, "sea"))
	{
	  nflg = new_flag();
	  nflg->type = FLAG_AFF;
	  nflg->val = AFF_WATER_BREATH;
	  nflg->next = soc->flags;
	  soc->flags = nflg;
	}
      if (!str_cmp (adjbuf, "cave") ||
	  !str_cmp (adjbuf, "deep"))
	{
	  nflg = new_flag();
	  nflg->type = FLAG_DET;
	  nflg->val = AFF_DARK;
	  nflg->next = soc->flags;
	  soc->flags = nflg;
	  nflg = new_flag();
	  nflg->type = FLAG_ROOM1;
	  nflg->val = ROOM_UNDERGROUND;
	  nflg->next = soc->flags;
	  soc->flags = nflg;
	  
	}
      

      if (!IS_ROOM_SET (soc, AREAGEN_SECTOR_FLAGS))
	{
	  nflg = new_flag();
	  nflg->type = FLAG_ROOM1;
	  nflg->val = AREAGEN_SECTOR_FLAGS &~ BADROOM_BITS;
	  nflg->next = soc->flags;
	  soc->flags = nflg;
	}
      
      /* Now find the mob area and figure out where we can start placing mobs
	 into the mob area. */
      
      if (!mobs_done_once)
	{
	  mobs_done_once = TRUE;
	  
	  if ((mobarea = find_thing_num(SOCIETY_MOBGEN_AREA_VNUM)) == NULL ||
	      !IS_AREA (mobarea))
	    {
	      return NULL;
	    }
	  
	  /* Init current vnum to 1 spot past the rooms in the mob area. */
	  
	  curr_vnum = mobarea->vnum + mobarea->mv;
	  
	  /* Set the max vnum to the area vnum +  level - 1. */
	  max_vnum = mobarea->vnum + mobarea->max_mv -1;
	  
	  
	  for (mob = mobarea->cont; mob; mob = mob->next_cont)
	    {
	      curr_vnum = MAX (curr_vnum, mob->vnum);
	    }
	  
	  /* Give some room after the last mob from the previous society. */
	  curr_vnum += 10;
	  
	  
	  
	  /* Castenum iterates through the various castes adding them
	     to the society, but only if the society needs these castes. */
	  
	  castenum = 0;
	  for (cnum = 0; cnum < CASTE_MAX; cnum++)
	    {
	      caste_tiers[cnum] = 0;
	      caste_flags[cnum] = 0;
	      caste_start[cnum] = 0;
	    }
	  
	  /* Generate the numbers of tiers and the caste flags for each
	     society caste. */
	  
	  for (cnum = 0; cnum < CASTE_MAX; cnum++)
	    {
	      caste_tiers[cnum] = nr (4,6);
	      if (cnum == 0)
		{
		  caste_flags[cnum] = CASTE_CHILDREN;
		  castenum++;
		}
	      else
		{
		  /* If the society is noresources, ONLY make warrior castes. */
		  if (IS_SET (society_flags, SOCIETY_NORESOURCES))
		    {
		      while (!IS_SET (caste1_flags[castenum].flagval, BATTLE_CASTES) &&
			     caste1_flags[castenum].flagval != 0)
			castenum++;
		    }
		  
		  if (caste1_flags[castenum].flagval == 0)
		    {
		      caste_tiers[cnum] = 0;
		      break;
		    }
		  caste_flags[cnum] = caste1_flags[castenum].flagval;
		  if (IS_SET (caste_flags[cnum], BATTLE_CASTES & ~CASTE_WARRIOR))
		    caste_tiers[cnum]--;
		  castenum++;
		}
	      
	    }
	  
	  /* So now we know the castes we have and the amount of tiers and
	     the various flags that we will need. */
	  
	  /* We also know the curr_vnum where we can start making mobs, and
	     the max_vnum where we must not go past when making mobs. */
	  
	  for (cnum = 0; cnum < CASTE_MAX; cnum++)
	    {
	      if (caste_tiers[cnum] < 1)
		continue;
	      
	      if ((num_tiernames = 
		   num_caste_tiernames(tiernames, caste_flags[cnum], proto)) < 1)
		continue;
	      
	      if (caste_tiers[cnum] > num_tiernames)
		caste_tiers[cnum] = num_tiernames;
	      
	      if (IS_SET (caste_flags[cnum], CASTE_CHILDREN))
		base_caste_level = LEVEL(proto)/10;
	      else if (IS_SET (caste_flags[cnum], BATTLE_CASTES))
		base_caste_level = LEVEL(proto)/2;
	      else 
		base_caste_level = LEVEL(proto)/4;
	      
	      /* Now choose which names we will actually put on mobs. */
	      
	      tier_choices_left = num_tiernames;
	      tiernames_used = 0;
	      tiers_left = caste_tiers[cnum];
	      tier_choice = 0;
	      /* For each tier, pick a random name that's left from the
		 list of available names. */
	      
	      for (tiers_left = caste_tiers[cnum] - 1; tiers_left >= 0; tiers_left--)
		{
		  if (tier_choices_left > 0)
		    tier_choice = nr (1, tier_choices_left);
		  else
		    break;
		  
		  for (tier = 0; tier < num_tiernames; tier++)
		    {
		      if (!IS_SET (tiernames_used, (1 << tier)) && --tier_choice < 1)
			{
			  SBIT (tiernames_used, (1 << tier));
			  break;
			}
		    }
		  tier_choices_left--;	  
		}
	      
	      /* Get the actual number of tiers we'll be making in this
		 caste. */
	      
	      caste_tiers[cnum] = 0;
	      for (tier = 0; tier < SOCIETY_GEN_CASTE_TIER_MAX; tier++)
		{
		  if (!IS_SET (tiernames_used, (1 << tier)) ||
		      !*tiernames[tier])
		    caste_tiers[cnum]++;
		}
	      

	      if (caste_tiers[cnum] < 1)
		continue;
	      
	      /* Now for leader's caste, add another tier -- overlord
		 tier. */
	      if (IS_SET (caste_flags[cnum], CASTE_LEADER))
		{
		  caste_tiers[cnum]++;
		  tiernames_used |= (1 << caste_tiers[cnum]);
		  strcpy (tiernames[caste_tiers[cnum]], "Overlord");
		}
	      
	      /* Generate a mob for each tier and then add a couple of extra
		 spaces after the last one for some room to grow. */
	      
	      current_tier_place = 0;
	      
	      for (tier = 0; tier < SOCIETY_GEN_CASTE_TIER_MAX; tier++)
		{
		  if (!IS_SET (tiernames_used, (1 << tier)) ||
		      !*tiernames[tier])
		    continue;
		  
		  /* Get the tier name set up. */
		  strcpy (tierbuf, tiernames[tier]);
		  
		  /* tierbuf[0] = UC (tierbuf[0]); */
		  if (!soc->aname || !*soc->aname || nr (1,2) == 1 ||
		      proto->vnum == DEMON_SOCIGEN_VNUM ||
		      proto->vnum == ORGANIZATION_SOCIGEN_VNUM)
		    strcpy (socinamebuf, soc->name);
		  else 
		    strcpy (socinamebuf, soc->aname);
		  
		  if (curr_vnum >= max_vnum - 10 ||
		      (mob = new_thing ()) == NULL)
		    break;
		  
		  /*	  socinamebuf[0] = UC(socinamebuf[0]); */
		  
		  /* Set the start location for this caste. */
		  if (current_tier_place == 0)
		    caste_start[cnum] = curr_vnum;
		  
		  mob->vnum = curr_vnum++;
		  mob->thing_flags = MOB_SETUP_FLAGS;
		  thing_to (mob, mobarea);
		  add_thing_to_list (mob);
		  
		  /* Calculate the level of the mob. */
		  
		  mob->level = (base_caste_level *
				(caste_tiers[cnum]+current_tier_place))
		    /(caste_tiers[cnum]);
		  
		  /* Leaders are MUCH tougher. */
		  if (IS_SET (caste1_flags[cnum].flagval, CASTE_LEADER))
		    {
		      mob->level += mob->level/5;
		      mob->max_hp = 25;
		      /* Overlords are MUCH tougher than that. */
		      if (!str_cmp (tiernames[tier], "Overlord"))
			{
			  mob->level += mob->level/5;
			  mob->max_hp = 40;
			}
		    }
		  
		  
		  current_tier_place++;
		  
		  sprintf (buf, "%s %s", socinamebuf, tierbuf);
		  free_str (mob->name);
		  mob->name = new_str (buf);
		  /* For demons, this is reversed. */

		  if (proto->vnum == DEMON_SOCIGEN_VNUM)
		    sprintf (buf, "%s %s %s", a_an (tierbuf), tierbuf, socinamebuf);
		  else
		    sprintf (buf, "%s %s %s", a_an (socinamebuf), socinamebuf, tierbuf);
		  free_str (mob->short_desc);
		  mob->short_desc = new_str (buf);
		  
		  strcat (buf, " is here.");
		  buf[0] = UC(buf[0]);
		  free_str (mob->long_desc);
		  mob->long_desc = new_str (buf);
		  
		  if (proto->height == 0)
		    mob->height = nr (50,70);
		  else
		    mob->height = nr (proto->height*4/5,proto->height*6/5);
		  
		  if (proto->weight == 0)
		    mob->weight = nr (1500,2500);
		  else
		    mob->weight = nr (proto->weight*4/5,proto->weight*6/5);
		  
		  if (cnum == 0)
		    {
		      mob->height /= 10;
		      mob->weight /= 10;
		    }
		  mob->sex = SEX_MALE;
		  
		  copy_values (proto, mob);
		  /* Now add some money...*/

		  add_money (mob, nr (LEVEL(mob),LEVEL(mob)*LEVEL(mob))/20);
		  
		}
	      /* Give some buffer room after these things are made. */
	      curr_vnum += 5;
	      caste_tiers[cnum] = current_tier_place;
	    }
	}
      
      
      for (cnum = 0; cnum < CASTE_MAX; cnum++)
	{
	  soc->start[cnum] = caste_start[cnum];
	  soc->cflags[cnum] = caste_flags[cnum];
	  
	  soc->max_tier[cnum] = caste_tiers[cnum];
	  
	  if (cnum == 0)
	    soc->curr_tier[cnum] = soc->max_tier[cnum];
	  /* Set the base chance for this to appear. */
	  
	  if (IS_SET (caste_flags[cnum], CASTE_WARRIOR))
	    soc->base_chance[cnum] = nr (9,11);
	  else if (IS_SET (caste_flags[cnum], CASTE_WORKER))
	    soc->base_chance[cnum] = nr (9,11);
	  else if (IS_SET (caste_flags[cnum], BATTLE_CASTES))
	    soc->base_chance[cnum] = nr (2,3);
	  else
	    soc->base_chance[cnum] = 3;
	  soc->chance[cnum] = soc->base_chance[cnum];
	  soc->max_pop[cnum] = 5;
	  soc->curr_pop[cnum] = 0;
	}
      
      soc->align = proto->align;
      soc->generated_from = proto->vnum;
      soc->lifespan = proto->timer;
      soc->society_flags = society_flags;
      
      curr_vnum += generate_society_objects (soc, proto->level, curr_vnum);
      /* more room to grow... */
      curr_vnum += 3;
    }
  while (*adjbuf);
  top_vnum = curr_vnum;
  return soc;
}
  
  
/* This finds the number of caste tiernames available to a society
   for making castes. The tiernames is where we put the names 
   (if any) and the caste_flags are what flags we look for in the
   caste names area and the proto can be used */

int
num_caste_tiernames (char tiernames[SOCIETY_GEN_CASTE_TIER_MAX][STD_LEN],
		     int caste_flags, THING *proto)
{
  int num, num_tiernames = 0;
  /* The name of the caste we want. */
  THING *caste_area, *obj;
  char *arg = NULL;
  char descline[STD_LEN*10];
  char *s;
  int i;
  
  /* Do we use names unique to this society? */
  
  bool use_unique_names = FALSE;
  if (caste_flags == 0)
    return 0;
  
  for (num = 0; num < SOCIETY_GEN_CASTE_TIER_MAX; num++)
    tiernames[num][0] = '\0';
  
  /* First see if the proto has a list of caste names in it someplace. */
  
  if (proto && proto->desc && *proto->desc)
    {
      arg = proto->desc;
      while (*arg)
	{
	  arg = f_word (arg, descline);
	  /* See if the first word in the line is caste. */
	  if (str_cmp (descline, "CASTE"))
	    {
	      /* If it's not caste, go to the next line. */
	      while (*arg != '\n' && *arg != '\r' && *arg)
		arg++;
	      continue;
	    }
	  /* See if the second word is a caste name. */
	  arg = f_word (arg, descline);
	  for (i = 0; caste1_flags[i].flagval != 0; i++)
	    {
	      if (IS_SET (caste1_flags[i].flagval, caste_flags) &&
		  !str_cmp (descline, caste1_flags[i].name))
		break;
	    }
	  
	  
	  /* If not, then don't use this line. */
	  if (!IS_SET (caste1_flags[i].flagval, caste_flags))
	    {
	      while (*arg && *arg != '\n' && *arg != '\r')
		arg++;
	      continue;
	    }
	  /* If so, strip the rest of the line off and put it
	     into descline and get the names from that. */
	  
	  s = descline;
	  while (*arg && *arg != '\n' && *arg != '\r')
	    {
	      *s++ = *arg++;
	    }
	  *s = '\0';
	  /* We have the list of words now. */
	  if (*arg)
	    {
	      use_unique_names = TRUE;
	      arg = descline;
	      break;
	    }
	}
    }

  if (!use_unique_names)
    {
      if ((caste_area = find_thing_num (SOCIETY_CASTE_AREA_VNUM)) == NULL ||
	  !IS_AREA (caste_area))
	return 0;
      
      /* Find the object in the caste area with the correct caste
	 flags on it. */
      
      for (obj = caste_area->cont; obj; obj = obj->next_cont)
	{
	  if (IS_ROOM (obj))
	    continue;
	  
	  if (IS_SET (flagbits (obj->flags, FLAG_CASTE), caste_flags))
	    break;
	}
      
      if (!obj || !obj->desc || !*obj->desc)
	return 0;
      
      /* Now loop through the words in the obj description to find the
	 tier names. */
      
      arg = obj->desc;
    }
  for (num = 0; num < SOCIETY_GEN_CASTE_TIER_MAX; num++)
    {
      arg = f_word (arg, tiernames[num]);
      if (*tiernames[num])
	num_tiernames++;
    }
  
  return num_tiernames;
}
  
/* This returns the number of society objects generated. */

int
generate_society_objects (SOCIETY *soc, int level, int curr_vnum)
{
  int num_objects = 0, max_vnum;
  int vnum;
  THING *area, *obj;
  char sociname[STD_LEN];
  char names[STD_LEN];
  if (!soc || !soc->name || !*soc->name)
    return 0;
  
  num_objects = nr (30,50);
  
  
 /* If this is a "pk world" -- autogenerated -- then the societies get
     more objects. */
  
  if (IS_SET (server_flags, SERVER_AUTO_WORLDGEN) &&
      IS_SET (server_flags, SERVER_WORLDGEN))
    {
      num_objects += nr (num_objects/2, num_objects);
    }
  max_vnum = curr_vnum + num_objects;
  for (max_vnum = curr_vnum; max_vnum < curr_vnum + num_objects; max_vnum++)
    {
      if ((area = find_area_in (max_vnum)) == NULL ||
	  max_vnum <= area->vnum + area->mv ||
	  max_vnum >= area->vnum + area->max_mv ||
	  (obj = find_thing_num (max_vnum)) != NULL)
	break;
    }
  
  if (max_vnum < curr_vnum + num_objects)
    num_objects = max_vnum - curr_vnum - 1;
  
  soc->object_start = curr_vnum;
  soc->object_end = max_vnum;

  for (vnum = curr_vnum; vnum < max_vnum; vnum++)
    {
      /* Don't let the organizations send the adjectives. */
      if (soc->generated_from == ORGANIZATION_SOCIGEN_VNUM)
	{
	  sprintf (sociname, soc->name);
	  possessive_form (sociname);
	}
      else if (soc->adj && *soc->adj)
	{
	  sprintf (sociname, "'%s %s'",
		   soc->adj,
		   (soc->aname && *soc->aname && nr (1,2) == 1 ?
		    soc->aname : soc->name));
	}
      else
	sprintf (sociname, "'%s'",
		 (soc->aname && *soc->aname && nr (1,2) == 1 ?
		  soc->aname : soc->name));
      sprintf (names, "society %s", sociname);
      objectgen (NULL, ITEM_WEAR_NONE, nr (level*2/3, level), vnum,
		 names);
    }
  return num_objects;
}
  
  
