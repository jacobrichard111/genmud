#include<stdio.h>
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#include "serv.h"
#include "track.h"
#include "society.h"
#include "historygen.h"
#include "event.h"
#include "areagen.h"
#include "objectgen.h"


/* Each align gets an ancient race that can help it if it's found. 
   The neutral race is designed to be BUFFED so that players have a
   damn hard time dealing with it. */

char *old_races[ALIGN_MAX];

/* Each align gets certain gods they can worship. */

char *old_gods[ALIGN_MAX][MAX_GODS_PER_ALIGN];

/* Lists of what the god is a god of...*/

char *old_gods_spheres[ALIGN_MAX][MAX_GODS_PER_ALIGN];

void
init_historygen_vars (void)
{
  int al, gd;
  
  for (al = 0; al < ALIGN_MAX; al++)
    {
      old_races[al] = nonstr;
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  old_gods[al][gd] = nonstr;
	  old_gods_spheres[al][gd] = nonstr;
	}
    }
  return;
}

/* Read in the history data on the races and gods and so forth. */

void
read_history_data (void)
{
  int al, gd;
  FILE *f;
  
  if ((f = wfopen ("history.dat", "r")) == NULL)
    return;
  
  for (al = 0; al < ALIGN_MAX; al++)
    {
      free_str (old_races[al]);
      old_races[al] = new_str (read_string (f));
    }
  
  for (al = 0; al < ALIGN_MAX; al++)
    {
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  free_str (old_gods[al][gd]);
	  old_gods[al][gd] = new_str (read_string (f));
	}
    }
  for (al = 0; al < ALIGN_MAX; al++)
    {
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  free_str (old_gods_spheres[al][gd]);
	  old_gods_spheres[al][gd] = new_str (read_string (f));
	}
    }
  fclose (f);
  return;
}

/* This writes out the history data: ailgn races and align gods and
   spheres. */

void
write_history_data (void)
{
  FILE *f;
  int al, gd;
  if ((f = wfopen ("history.dat", "w")) == NULL)
    {
      log_it ("Couldn't open history.dat for writing!");
      return;
    }

  for (al = 0; al < ALIGN_MAX; al++)
    {
      fprintf (f, "%s`\n", old_races[al]);
    }
  for (al = 0; al < ALIGN_MAX; al++)
    {
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  fprintf (f, "%s`\n", old_gods[al][gd]);
	}
    }
  for (al = 0; al < ALIGN_MAX; al++)
    {
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  fprintf (f, "%s`\n", old_gods_spheres[al][gd]);
	}
    }
  fclose (f);
  return;
}

/* This generates the history of the world...by using the society names
   and areas and deities and ancient races and alignments and
   organizations to make up a little story in 3 parts.
   
   1. Ancient history..things were good for a long time.
   2. Disaster...Things went bad for some reason.
   3. Rebirth...Now things are changing so you go make your mark. */

void
historygen (void)
{
  HELP *help;
  bigbuf[0] = '\0';

  /* Make sure the helpfile exists. */

  if ((help = find_help_name (NULL, "HISTORY")) == NULL &&
      (help = find_help_name (NULL, "BACKSTORY")) == NULL)
    {
      help = new_help();
      help->keywords = new_str ("HISTORY BACKSTORY");
      setup_help (help);
    }
  
  /* Set up the ancient races and the gods and so forth. */

  historygen_setup ();

  /* Now generate each of the ages. */
  
  /* The idea is that there was a long period of peace and harmony
     then something bad happened and the ancient powers went away
     and the world was thrown into turmoil. Now, ancient things
     are stirring and here's the state of the world, and you
     have to find the ancient powers blah blah blah... */
  add_to_bigbuf (historygen_past());
  add_to_bigbuf (historygen_disaster());
  add_to_bigbuf (historygen_present());
  echo (bigbuf);
  
  free_str (help->text);
  help->text = new_str (bigbuf);
  write_helps();
  return;
}

/* This shows the mythology of the world to a player. */

void
do_mythology (THING *th, char *arg)
{
  int al, gd;
  RACE *align;
  char buf[STD_LEN];

  if (LEVEL (th) == MAX_LEVEL && IS_PC (th))
    {
      if (!str_cmp (arg, "setup"))
	{
	  historygen_generate_gods(th);
	  historygen_generate_races();
	}
    }

  if (!str_cmp (arg, "history"))
    {
      historygen();
      return;
    }
  
  for (al = 0; al < ALIGN_MAX; al++)
    {
      if ((align = find_align (NULL, al)) == NULL ||
	  !old_gods[al][0] || !*old_gods[al][0])
	continue;
      
      sprintf (buf, "The \x1b[1;3%dm%s\x1b[0;37m Deities:\n\r", 
	       MID(0,align->vnum,7), align->name);
      stt (buf, th);
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  if (old_gods[al][gd] && *old_gods[al][gd])
	    {
	      sprintf (buf, "%-30s %s\n", old_gods[al][gd], old_gods_spheres[al][gd]);
	      stt (buf, th);
	    }
	}
      stt ("\n\r", th);
      sprintf (buf, "The %s race: %s\n\n\r", align->name, old_races[al]);
      stt (buf, th);
    }
  return;
}
  

/* This sets up the historygen data. */

void
historygen_setup (void)
{

  historygen_generate_races();
  historygen_generate_gods(NULL);
  historygen_generate_organizations();
  write_history_data();  
  return;
}

/* This sets up the historygen races. */

void
historygen_generate_races (void)
{
  int al;
  RACE *align;
  THING *proto;
  char buf[STD_LEN];
  char *t;
  bool race_name_ok = FALSE;
  /* If we have genned races and we aren't worldgenning then return. */
  
  if (!IS_SET (server_flags, SERVER_WORLDGEN))
    {
      SOCIETY *soc;
      
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (soc->generated_from)
	    return;
	}
    }

  proto = new_thing();
  proto->level = 250;
  proto->vnum = ANCIENT_RACE_SOCIGEN_VNUM;
  add_flagval (proto, FLAG_DET, AFF_INVIS | AFF_HIDDEN | AFF_SNEAK | AFF_CHAMELEON | AFF_FOGGY);
  add_flagval (proto, FLAG_AFF, AFF_FLYING | AFF_SANCT);
  for (al = 0; al < ALIGN_MAX; al++)
    {
      if ((align = find_align (NULL, al)) != NULL)
	{
	  free_str (old_races[al]);
	  
	  /* A race name has 5+ letters in it ending with the last
	     letter being a consonant. After a name like that
	     is created, the race then adds a random vowel onto
	     the end. */
	  
	  do
	    {
	      race_name_ok = TRUE;
	      
	      strcpy (buf, create_society_name (NULL));
	      
	      for (t = buf; *t; t++);
	      
	      /* Go backwards until we hit a letter that isn't a vowel
		 that's preceeded by a vowel. */
	      do 
		{
		  t--;
		  if (t <= buf) /* Stop if we go back to the beginning
				   of the buffer. */
		    break;
		  if (!isalpha (*t)) /* Stop on a letter. */
		    continue;
		  if (isvowel (*t)) /* Must not be a vowel. */
		    continue;
		  if (!isvowel (*(t-1))) /* Previous must be a vowel. */
		    continue;
		  break;
		}
	      while (1);
	      
	      /* Make sure the name's long enough...allow - and ' in the
		 names. */
	      
	      if (t - buf < 4 || t - buf > 6)
		race_name_ok = FALSE;
	      
	      /* Append a vowel...*/
	      
	      *(t + 1) = vowels[nr(0,NUM_VOWELS-1)];
	      *(t + 2) = '\0';
	    }
	  while (!race_name_ok);

	  free_str (proto->name);
	  old_races[al] = new_str (buf);
	  sprintf (buf, "%s %s %s", old_races[al], old_races[al], old_races[al]);	  
	  proto->name = new_str (buf);
	  free_str (proto->short_desc);
	  proto->short_desc = new_str ((nr (1,3) == 2 ? "Ancient" :
					(nr (1,2) == 1 ? "Mighty" : "Powerful")));
	  
	  /* They get random mob protect flags...*/
	  remove_flagval (proto, FLAG_MOB, ~0);
	  add_flagval (proto, FLAG_MOB, nr (1, MOB_AIR * 2 - 1));
 	  proto->align = al;
	  add_flagval (proto, FLAG_SOCIETY, SOCIETY_SETTLER);
	  generate_society (proto);
	}
    }
  free_thing (proto);
  /* Now give them raw materials. */
  
  return;
}

/* This generates the organizations that players use to get their
   skills. These organizations are societies that can't change align
   and which get put into the world in random areas. */


void
historygen_generate_organizations (void)
{
  RACE *align;
  int al, count, num_organizations, caste_flag, num_choices,
    num_chose;
  THING *proto;
  char name[STD_LEN];
  char prefix_name[STD_LEN]; /* From a list of prefixes in the
				organizations area. */
  char tiernames[SOCIETY_GEN_CASTE_TIER_MAX][STD_LEN];

  /* If we have genned races and we aren't worldgenning then return. */
  
  if (!IS_SET (server_flags, SERVER_WORLDGEN))
    {
      SOCIETY *soc;
      
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (soc->generated_from)
	    return;
	}
    }


  /* Need a proto just to make things work...*/
  if ((proto = new_thing()) == NULL)
    return;
  
  proto->vnum = ORGANIZATION_SOCIGEN_VNUM;
  /* These flags are added because societies that are generated have 
     AUTOMATIC flags added to them... (SOCIETY_BASE_FLAGS) where they're
     set to aggressive and settler atm. Therefore, the fix align is added,
     but settler is removed. */
  add_flagval (proto, FLAG_SOCIETY, SOCIETY_SETTLER );
  /* Loop through aligns and give a society to each nonzero align. */

  for (al = 1; al < ALIGN_MAX; al++)
    {
      if ((align = align_info[al]) == NULL)
	continue;
      
      /* Different numbers of organizations per align...*/
      num_organizations = nr(3,5);
      

      for (count = 0; count < num_organizations; count++)
	{
	  switch (nr (1,9))
	    {
	      case 1:
		caste_flag = CASTE_THIEF;
		break;
	      case 2:
	      case 3:
	      case 4:
		caste_flag = CASTE_WARRIOR;
		break;
	      case 5:
	      case 6:
	      case 7:
		caste_flag = CASTE_WIZARD;
		break;
	      case 8:
	      case 9:
	      default:
		caste_flag = CASTE_HEALER;
		break;
	    }
	  
	  /* Pick the name. */
	  if ((num_choices = num_caste_tiernames (tiernames, caste_flag, proto)) < 1)
	    continue;
	  
	  /* Make sure it's within the acceptable range. */
	  num_chose = nr (num_choices/3,num_choices)-1;
	  if (num_chose < 1)
	    num_chose = 1;

	  if (num_chose >= SOCIETY_GEN_CASTE_TIER_MAX)
	    num_chose = SOCIETY_GEN_CASTE_TIER_MAX-1;
	  
	  /* Make sure it's actually a name. */
	  if (!*tiernames[num_chose])
	    continue;

	  
	  /* Get the prefix. */

	  strcpy (prefix_name, find_gen_word (ORGANIZATION_AREA_VNUM,
					      "organization_prefixes", NULL));
		  
	  if (!*prefix_name)
	    continue;
	  
	  
	  /* Now set up the names. */

	  /* I had to take this magicians' assembly thing out
	     since the code wasn't working correctly in terms of
	     making the names. Will have to try to fix this. */
	  /* Magician's assembly...*/
	  /*
	  if (nr (1,2) == 1)
	    {
	      sprintf (name, "%s %s %s", prefix_name, prefix_name, prefix_name);
	      free_str (proto->name);
	      capitalize_all_words(name);
	      proto->name = new_str (name);
	      
	      
	      sprintf (name, tiernames[num_chose]);
	      possessive_form (name);
	      capitalize_all_words(name);
	      free_str (proto->short_desc);
	      proto->short_desc = new_str (name);
	    }
	  else 
	  
	  */
	  /* Assembly of magicians. */

	    {

	      /* This code looks strange putting the words in
		 there three times..but it's needed since societies
		 have three names: name, plural name, possessive name.
		 Since all three are needed for the society and since
		 English sucks and there are so many exceptions, I tried
		 to put them into the code in one spot and so when
		 you create a society, you need all three names. */
	      
	      /* The single quotes are necessary below so that when
		 the words are read in, any multiple word names are not
		 split into pieces. That split can occur because when
		 the words are read out of the list, the quotes around
		 strings of words get removed. */
	      sprintf (name, "'%s' '%s", tiernames[num_chose],
		       tiernames[num_chose]);

	      /* Add the plural form name. */
	      plural_form (name);
	      
	      /* Add the possessive name.. */
	      strcat (name, "' '");
	      strcat (name, tiernames[num_chose]);
	      strcat (name, "'");
	      free_str (proto->name);
	      capitalize_all_words(name);
	      proto->name = new_str (name);
	      
	      /* Set up prefix now...*/
	      
	      /* The single quotes in the below line are necessary since
		 the society_generate code looks at the proto->short_desc
		 and starts doing f_words to get the arguments out
		 of it. If the quotes were not there, the first f_word
		 would strip off the first word like "assembly" and the
		 second would strip off "of"...which would lead to a
		 society called "Of Magicians" and one called 
		 "Assembly Magicians"...neither of which is what we
		 really want. */
	      sprintf (name, "'%s of'", prefix_name);
	      capitalize_all_words(name);
	      free_str (proto->short_desc);
	      proto->short_desc = new_str (name);
	    }
	  
	  proto->align = al;
	  proto->level = nr (80,130);
	  generate_society (proto);
	}
      
    }
  free_thing (proto);
  return;
}
  


/* This sets up the historygen gods and their spheres of influence. */

void
historygen_generate_gods (THING *th)
{
  int al, gd, max_gods;
  
  int num_spheres, sph, count;
  char buf[STD_LEN]; /* Big buffer for all spheres. */
  char buf2[STD_LEN]; /* Small buffer for each sphere. */
  char spheretype[STD_LEN]; /* Good or bad spheres. */
  RACE *align;
  int name_tries;
  bool name_ok = FALSE;
  bool deity_things_exist = FALSE;
  THING *area;
  if ((area = find_thing_num (DEITY_AREA_VNUM)) == NULL ||
      (area->cont && area->cont->next_cont))
    {
      deity_things_exist = TRUE;
      stt ("Could not set up deity things. They already exist. Do a worldgen clear yes to clear them.\n\r", th);
    }
  
  /* Must be fixed for all aligns so divine summoning is fair. */
  max_gods = nr (MAX_GODS_PER_ALIGN/2, MAX_GODS_PER_ALIGN);
  for (al = 0; al < ALIGN_MAX; al++)
    {
      
      for (gd = 0; gd < MAX_GODS_PER_ALIGN; gd++)
	{
	  free_str (old_gods[al][gd]);
	  old_gods[al][gd] = nonstr;
	  free_str (old_gods_spheres[al][gd]);
	  old_gods_spheres[al][gd] = nonstr;
	}
      
      if ((align = find_align (NULL, al)) == NULL)
	continue;
      
      for (gd = 0; gd < max_gods; gd++)
	{
	  if (nr (1,6) != 3)
	    strcpy (spheretype, "godly_spheres_good");
	  else
	    strcpy (spheretype, "godly_spheres_bad");
	  
	  name_tries = 0;
	  do 
	    {
	      name_ok = TRUE;
	      free_str (old_gods[al][gd]);
	      old_gods[al][gd] = nonstr;
	      old_gods[al][gd] = new_str (create_society_name (NULL));
	      
	      
	      if (strlen(old_gods[al][gd]) < 4)
		name_ok = FALSE;
	      /* Truncate name */
	      old_gods[al][gd][8] = '\0';
	    }
	  while (!name_ok && ++name_tries < 100);
	  
	  num_spheres = 2;
	  if (nr (1,5) == 3)
	    num_spheres--;
	  if (nr (1,5) == 1)
	    num_spheres++;
	  if (nr (1,5) == 2)
	    num_spheres++;
	  buf[0] = '\0';
	  for (sph = 0; sph < num_spheres; sph++)
	    {
	      /* Keep trying for words while none are made or the
		 word has already been used for this deity. */
	      count = 0;
	      do
		{
		  buf2[0] = '\0';
		  strcpy (buf2, find_gen_word (HISTORYGEN_AREA_VNUM, spheretype, NULL));
		}
	      while ((!*buf2 || strstr (buf, buf2)) && ++count < 100);
	      
	      /* Now add the new sphere to the god's list. */
	      if (*buf && *buf2)
		{
		  if (sph == num_spheres - 1)
		    strcat (buf, " and ");
		  else
		    strcat (buf, ", ");
		}
	      capitalize_all_words (buf2);
	      strcat (buf, buf2);
	    }
	  old_gods_spheres[al][gd] = new_str (buf);
	  if (!deity_things_exist)
	    generate_deity (old_gods[al][gd], old_gods_spheres[al][gd], al);
	}
    }
  return;
}



/* This generates a deity...almost unkillable and with tons of 31337 
   eq on it. */

void
generate_deity (char *name, char *spheres, int align)
{
  THING *proto, *area, *obj;
  int curr_vnum, min_vnum, max_vnum;
  int wearloc, wearnum; /* For making the eq for the character. */
  char buf[STD_LEN];
  if (!name || !*name || !spheres || !*spheres)
    return;
  
  if (align < 0 || align >= ALIGN_MAX)
    return;
  if ((area = find_thing_num (DEITY_AREA_VNUM)) == NULL)
    return;
  area->thing_flags |= TH_CHANGED;
  min_vnum = area->vnum + area->mv + 1;
  max_vnum = area->vnum + area->max_mv -1;
  
  
  for (curr_vnum = min_vnum; curr_vnum <= max_vnum; curr_vnum++)
    if ((proto = find_thing_num (curr_vnum)) == NULL)
      break;
  
  if (curr_vnum < min_vnum && curr_vnum > max_vnum)
    return;
  
  /* Base stats like level and flags. */
  proto = new_thing();
  proto->vnum = curr_vnum;
  proto->timer = DEITY_SUMMON_HOURS; /* Only a few hours to use this thing...*/
  proto->height = nr (100,200);
  proto->weight = nr (3000,9000);
  proto->max_mv = 1;
  thing_to (proto, area);
  add_thing_to_list (proto);
  
  proto->thing_flags = MOB_SETUP_FLAGS;
  proto->align = align;
  add_flagval (proto, FLAG_DET, AFF_INVIS | AFF_HIDDEN | AFF_CHAMELEON | AFF_FOGGY);
  add_flagval (proto, FLAG_AFF, AFF_FLYING | AFF_SANCT | AFF_HASTE | AFF_WATER_BREATH | AFF_AIR | AFF_EARTH | AFF_FIRE | AFF_WATER | AFF_PROTECT | AFF_PROT_ALIGN);
  add_flagval (proto, FLAG_MOB, (MOB_AIR *2 - 1));
  add_flagval (proto, FLAG_PROT, ~0);
  add_flagval (proto, FLAG_ACT1, ACT_KILL_OPP | ACT_FASTHUNT | ACT_DEITY);
  proto->level = DEITY_LEVEL;
  
  sprintf (buf, "%s deity", name);
  proto->name = new_str (buf);
  sprintf (buf, "%s the %s of %s", name, 
	   (nr (1,3) == 2 ? "God" : (nr (1,2) == 1 ? "Deity" : "Lord")),
	   spheres);
  proto->short_desc = new_str (buf);
  proto->long_desc = new_str ("A being of amazing and awesome power stands before you...");
  
  sprintf (buf, "owner \"%s\"", name);
  for (wearloc = ITEM_WEAR_NONE + 1; wearloc < ITEM_WEAR_MAX; wearloc++)
    {
      for (wearnum = 0; wearnum < wear_data[wearloc].how_many_worn; wearnum++)
	{
	  curr_vnum++;
	  if ((obj = objectgen (area, wearloc, DEITY_LEVEL, curr_vnum, buf)) != NULL)
	    add_reset (proto, obj->vnum, 100, 1, 1);
	}
    }
  free_thing (proto);
  return;
}
	  

void
do_divine (THING *th, char *arg)
{
  THING *proto, *deity_area, *deity = NULL;
  char buf[STD_LEN];
  if (!th || !th->in || !arg || !*arg || !IS_PC (th))
    {
      stt ("Divine <deity name>\n\r", th);
      return;
    }
  
  if ((deity_area = find_thing_num (DEITY_AREA_VNUM)) == NULL)
    {
      stt ("The deities ignore your prayers.\n\r", th);
      return;
    }

  /* Find the deity with the proper name. */
  
  for (proto = deity_area->cont; proto; proto = proto->next_cont)
    {
      if (is_named (proto, arg) && 
	  !IS_SET (proto->thing_flags, TH_NO_FIGHT | TH_NO_MOVE_SELF) &&
	  proto->wear_pos == ITEM_WEAR_NONE)
	break;
    }

  if (!proto)
    {
      stt ("There is no deity by that name that you can summon!\n\r", th);
      return;
    }

  /* Make sure it's of the right alignment. */
  if (proto->align != th->align)
    {
      stt ("That deity will not assist you! You are not of the proper alignment!\n\r", th);
      return;
    }

  if (guild_rank (th, GUILD_HEALER) < GUILD_TIER_MAX - 1 ||
      guild_rank (th, GUILD_MYSTIC) < GUILD_TIER_MAX - 1)
    {
      sprintf (buf, "You must attain tier %d in the Healer and Mystic Guilds before the deities will answer your call!\n\r", GUILD_TIER_MAX - 1);
      stt (buf, th);
      return;
    }

  /* Make sure that the summoner has enough wps. */

  if (th->pc->pk[PK_WPS] < DEITY_SUMMON_COST)
    {
      sprintf (buf, "You need at least %d warpoints to summon a deity.\n\r", DEITY_SUMMON_COST);
      stt (buf, th);
      return;
    }

  /* Make sure only one copy out at a time. */

  if (proto->mv > 0)
    {
      stt ("The deity ignores your pleas.\n\r", th);
      return;
    }

  /* Make sure we can create this thing. */
  if ((deity = create_thing (proto->vnum)) == NULL)
    {
      stt ("The deity ignores your pleas.\n\r", th);
      return;
    }

  reset_thing (deity, 0);
  do_wear (deity, "all");
  th->pc->pk[PK_WPS] -= DEITY_SUMMON_COST;
  act ("$9@1n reach@s up and bellow@s forth asking for succor!$7", th, NULL,NULL, NULL, TO_ALL);
  thing_to (deity, th->in);
  act ("$F@1n appear@s!$7", deity, NULL, NULL, NULL, TO_ALL);
  create_repeating_event (deity, PULSE_PER_SECOND/2, deity_hunt); 
  if (FIGHTING (th))
    multi_hit (deity, FIGHTING (th));
  else
    deity_hunt (deity);
  return;
}

/* Lets a deity hunt really fast. */
void
deity_hunt (THING *th)
{
  THING *enemy = NULL;
  int times;
  if (!th || LEVEL(th) != DEITY_LEVEL ||
      !th->proto || !th->proto->in ||
      th->proto->in->vnum != DEITY_AREA_VNUM)
    return;
  
  if (is_hunting (th))
    {
      hunt_thing (th, 0);
      return;
    }
  
  for (times = 1; times < 5; times++)
    {
      if ((enemy = find_enemy_nearby (th, times*5)) != NULL)
	break;
    }
  
  if (!enemy)
    return;

  if (enemy->in == th->in)
    {
      multi_hit (th, enemy);
      return;
    }
  start_hunting (th, NAME(enemy), HUNT_KILL);
  return;
}




/* This clears the things created for the history data. */

void
historygen_clear (void)
{
  THING *area, *mobject;

  if ((area = find_thing_num (DEITY_AREA_VNUM)) != NULL)
    {
      area->thing_flags |= TH_CHANGED;
      for (mobject = area->cont; mobject; mobject = mobject->next_cont)
	{
	  if (IS_ROOM (mobject))
	    continue;
	  mobject->thing_flags |= TH_NUKE;
	}
    }
  
  /* More things will be added later. */

  return;
}



char * 
historygen_past(void)
{
  static char ret[STRINGGEN_LEN];
   *ret = '\0';
   strcat (ret, string_gen 	   
	   ("Looking_back the olden ages z_the_past, I cannot_help feeling_feeling extreme feeling_loss prep_for what_was_lost. It was the age when the united gods followed list_controller_deities. list_ancient_race_names enforced justice at the behest of the gods.", HISTORYGEN_AREA_VNUM));
   
   
   strcat (ret, "\n\n");
   /* Now do some wars...*/
   
   return ret;
}

char *
historygen_disaster (void)
{
  static char ret[STRINGGEN_LEN];
  ret[0] = '\0';
  strcat (ret, string_gen ("But_then came_disaster. The myriad nations of the world were destroyed, and the nations such_as the genned_society_name were cast_out from their home_names. Many battle_names were fought and the ancient_race_name and the other ancient races were driven_from_the_world. And thus did the power of the deities lessen_word. This_was_what the Demons were_waiting_for as they surged into the world, but were cast back to the Pits at_a_terrible_cost.", HISTORYGEN_AREA_VNUM));
  
  strcat (ret, "\n\n");
  return ret;

}

char *
historygen_present (void)
{

  char buf[STD_LEN*3];
  static char ret[STRINGGEN_LEN];

  ret[0] = '\0';
  sprintf (buf, "The nations descended into dark_days, and the_world stayed the_same for many ages. But now, the Demons are demon_stirring. The races are broken and disorganized and squabbling among themselves. They must be brought together to fight the demons in this %s", remove_color(game_name_string));

  strcat (ret, string_gen (buf, HISTORYGEN_AREA_VNUM));
  
  strcat (ret, "\n\n");
  return ret;
}
