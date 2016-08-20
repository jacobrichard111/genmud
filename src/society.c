#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "rumor.h"


int society_ticks = 0;
SOCIETY *society_free = NULL;
/* Flags societies have that affect how they act. */

const struct flag_data society1_flags[] =
  {
    {"aggressive", " Aggressive", SOCIETY_AGGRESSIVE, "This society attacks when its population is too big."},
    {"settler", " Settler", SOCIETY_SETTLER, "This society settles new societies when its population is too big."},
    {"xenophobic", " Xenophobic", SOCIETY_XENOPHOBIC, "This society attacks any outsiders it encounters in its home."},
    {"nuke", " Nuke", SOCIETY_NUKE, "This society is not saved."},
    {"destructible", " Destructible", SOCIETY_DESTRUCTIBLE, "This society can end if all the population is killed off."},
    {"noresources",  " NoResources", SOCIETY_NORESOURCES, "This society kills for resources instead of finding them."},
    {"fixed_align", " Fixed_Align", SOCIETY_FIXED_ALIGN, "This society cannot switch alignments."},
    {"nocturnal", " Nocturnal", SOCIETY_NOCTURNAL, "Sleeps during the day and is active at night."},
    {"nosleep", " Nosleep", SOCIETY_NOSLEEP, "This society never sleeps."},
    {"nonames", " Nonames", SOCIETY_NONAMES, "No names given to members."},
    {"necrogenerate", " NecroGenerate", SOCIETY_NECRO_GENERATE, "Society mages/clerics raise new dead members of the society."},
    {"marked", " Marked", SOCIETY_MARKED, "This society is marked for some purpose."},
    {"fastgrow", " Fastgrow", SOCIETY_FASTGROW, "This society is larger and grows faster than others."},
    {"overlord", " Overlord", SOCIETY_OVERLORD, "This society has an overlord,"},
    {"none", "none", 0, "nothing"},
  };

/* Different kinds of castes within a society. */
/* The names at the end like "Barracks" are names used to dynamically
   generate room names within the society cities. */

const struct flag_data caste1_flags[] = 
  {
    /* YOU MUST ALWAYS PUT THIS CHILDREN'S CASTE FIRST!!! OR
       ELSE SOCIETY GENERATION OR NEW MEMBER GENERATION 
       DOESN'T WORK SO WELL! */
    {"children", " Children", CASTE_CHILDREN, "School"},
    {"warrior", " Warrior", CASTE_WARRIOR, "Barracks"},
    {"leader", " Leader", CASTE_LEADER, "Pavilion"},
    {"worker", " Worker", CASTE_WORKER, "Workers' Hall"},
    {"builder", " Builder", CASTE_BUILDER, "Hall of Builders"},
    {"healer", " Healer", CASTE_HEALER, "Temple"},
    {"wizard", " Wizard", CASTE_WIZARD, "Academy"},
    {"thief", " Thief", CASTE_THIEF, "Den"},
    /* These are things that I don't feel like having in right now. */
    /* {"shopkeeper", " Shopkeeper", CASTE_SHOPKEEPER, "Marketplace"}, */
    /* {"crafter", " Crafter", CASTE_CRAFTER, "Craftsmens' Guild"}, */
    /* Generally CASTE_FARMER should be last since their caste
       houses take up a lot more rooms (they overwrite many field
       or underground rooms rather than just a few in a small area. 
    I wlll not use this right now. */
    /*   {"farmer", " Farmer", CASTE_FARMER, "Farm"}, */
    {"none", "none", 0, "nothing"},
  };




SOCIETY *
new_society (void)
{
  SOCIETY *soc;
  if (society_free)
    {
      soc = society_free;
      society_free = society_free->next;
    }
  else
    {
      soc = (SOCIETY *) mallok (sizeof (*soc));
      society_count++;
    }
  clear_society_data (soc);
  return soc;
}

/* Clears data in the society (does not free strings and flags!) */

void
clear_society_data (SOCIETY *soc)
{
  int i;
  if (!soc)
    return;

  bzero (soc, sizeof (*soc));
  soc->name = nonstr;
  soc->aname = nonstr;
  soc->pname = nonstr;
  soc->adj = nonstr;
  soc->next_hash = NULL;
  soc->flags = NULL;
  soc->next = NULL;
  soc->last_killer = NULL;
  soc->warrior_percent = 50;
  soc->recent_maxpop = 0;
  soc->alife_home_x = -1;
  soc->alife_home_y = -1;
  for (i = 0; i < CASTE_MAX; i++)
    {
      soc->max_pop[i] = 3;
      soc->curr_pop[i] = 0;
      soc->curr_tier[i] = 1;
      soc->max_tier[i] = 1;
      soc->cflags[i] = 0;
      soc->base_chance[i] = 2;
      soc->chance[i] = 2;
    }
}

/* This removes a society from the society list and table and
   puts it on the free list. */

void
free_society (SOCIETY *soc)
{
  FLAG *flg, *flgn;
  THING *player;
  /* Only destructible societies get nuked. */
  if (!soc || !IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE))
    return;
  
  for (player = thing_hash[PLAYER_VNUM % HASH_SIZE]; player; player = player->next)
    {
      if (IS_PC (player) && player->pc->editing == soc)
	{
	  player->pc->editing = NULL;
	  if (player->fd)
	    player->fd->connected = CON_ONLINE;
	}
    }

  society_from_list (soc);

  society_from_table (soc);

  free_str (soc->name);
  free_str (soc->aname);
  free_str (soc->pname);
  free_str (soc->adj);
  
  for (flg = soc->flags; flg; flg = flgn)
    {
      flgn = flg->next;
      free_flag (flg);
    }
  clear_society_data (soc);
  
  soc->next = society_free;
  society_free = soc;
}

/* Remove a society from the society list. */

void
society_from_list (SOCIETY *soc)
{
  SOCIETY *prev;
  
  if (!soc)
    return;
  
  if (soc == society_list)
    {
      society_list = soc->next;
      soc->next = NULL;
    }
  else
    {
      for (prev = society_list; prev; prev = prev->next)
	{
	  if (prev->next == soc)
	    {
	      prev->next = soc->next;
	      soc->next = NULL;
	      break;
	    }
	}
    }
  return;
}

/* Remove a society from the society hash table. */

void 
society_from_table (SOCIETY *soc)
{
  int num;
  SOCIETY *prev;
  
  if (!soc)
    return;
  
  num = soc->vnum % SOCIETY_HASH;
  
  if (soc == society_hash[num])
    {
      society_hash[num] = soc->next_hash;
      soc->next_hash = NULL;
    }
  else
    {
      for (prev = society_hash[num]; prev; prev = prev->next_hash)
	{
	  if (prev->next_hash == soc)
	    {
	      prev->next_hash = soc->next_hash;
	      soc->next_hash = NULL;
	      break;
	    }
	}
    }
  return;
}

/* This finds a society based on a given number. The list is
   hashed for faster lookup. */

SOCIETY *
find_society_num (int num)
{
  SOCIETY *soc;
  for (soc = society_hash[num % SOCIETY_HASH]; soc; soc = soc->next_hash)
    if (soc->vnum == num)
      return soc;
  return NULL;
}
/* Finds which society something is in. */
 
 SOCIETY *
   find_society_in (THING *th)
   {
     VALUE *soc;
     if (!th || (soc = FNV (th, VAL_SOCIETY)) == NULL)
    return NULL;
  return find_society_num (soc->val[0]);
}

/* Finds which caste something is in. It checks to make sure that the
   society exists and that the society has a caste setup (up to a point.) */

int
find_caste_in (THING *th)
{
  VALUE *soc;
  SOCIETY *soci;
  
  if ((soc = FNV (th, VAL_SOCIETY)) == NULL ||
      (soci = find_society_num (soc->val[0])) == NULL ||
      soc->val[1] < 0 || soc->val[1] >= CASTE_MAX ||
      soci->start[soc->val[1]] == 0)
    return CASTE_MAX;
  
  return soc->val[1];
}

void
soc_to_list (SOCIETY *soc)
{
  SOCIETY *prev;
  if (!soc)
    return;
  if (!society_list)
    {
      society_list = soc;
      soc->next = NULL;
    }
  else if (society_list->vnum > soc->vnum)
    {
      soc->next = society_list;
      society_list = soc;
    }
  else
    {
      for (prev = society_list; prev != NULL; prev = prev->next)
	{
	  if (!prev->next || prev->next->vnum > soc->vnum)
	    {
	      soc->next = prev->next;
	      prev->next = soc;
	      break;
	    }
	}
    }	  
  
  /* Add the society to the hash table, also. */
  
  soc->next_hash = society_hash[soc->vnum % SOCIETY_HASH];
  society_hash[soc->vnum % SOCIETY_HASH] = soc;
  return;
}

/* Basically this does a byte by byte copy of the society struct,
   then it cleans up the pointers and strings. */


void
copy_society (SOCIETY *osoc, SOCIETY *nsoc)
{
  int i, sz;
  
  char *o, *n;
  FLAG *oflg, *nflg;

  if (!osoc || !nsoc)
    return;
  
  sz = sizeof (*osoc);
  n = (char *) nsoc;
  o = (char *) osoc;
  
  while (o - (char *) osoc < sz)
    *n++ = *o++;
  
  nsoc->name = nonstr;
  nsoc->aname = nonstr;
  nsoc->pname = nonstr;
  nsoc->adj = nonstr;
  if (osoc->name && *osoc->name)
    nsoc->name = new_str (osoc->name);
 if (osoc->pname && *osoc->pname)
    nsoc->pname = new_str (osoc->pname);
 if (osoc->aname && *osoc->aname)
    nsoc->aname = new_str (osoc->aname);
 
  if (osoc->adj && *osoc->adj)
    nsoc->adj = new_str (osoc->adj);
  
  nsoc->flags = NULL;
  for (oflg = osoc->flags; oflg; oflg = oflg->next)
    {
      nflg = new_flag();
      copy_flag (oflg, nflg);
      nflg->next = nsoc->flags;
      nsoc->flags = nflg;
    }
  
  
  nsoc->vnum = 0;
  for (i = 1; i <=top_society; i++)
    {
      if (find_society_num (i) == NULL)
	{
	  nsoc->vnum = i;
	  break;
	}
    }
  if (nsoc->vnum == 0)
    nsoc->vnum = ++top_society;
  
  nsoc->next = NULL;
  nsoc->next_hash = NULL;
  
  /* Set the current pops to 0. */
  
  for (i = 0; i < CASTE_MAX; i++)
    nsoc->curr_pop[i] = 0; 
  
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    nsoc->guard_post[i] = 0;
  
  for (i = 0; i < RAW_MAX; i++)
    nsoc->raw_curr[i] = 0;
  
  return;
}




  

void
do_society (THING *th, char *arg)
{
  int i, num_times, num_castes, total_population = 0, vnum,
    num_societies = 0;
  char arg1[STD_LEN];
  char arg2[STD_LEN];
  char arg3[STD_LEN];
  char buf[STD_LEN];
  char areaname[STD_LEN];
  SOCIETY *soc = NULL;
  THING *area;
  if (!th || !IS_PC (th) || LEVEL (th) < MAX_LEVEL || !th->fd)
    {
      stt ("Huh?\n\r", th);
      return;
    }
  
  arg = f_word (arg, arg1);
  arg = f_word (arg, arg2);
  arg = f_word (arg, arg3);
  
  /* Update the societies a certain number of times. */
  
  if (!str_cmp (arg1, "update"))
    {
      if (is_number (arg2))
	num_times = MID (1, atoi (arg2), 1000);
      else
	num_times = 1;

      if ((soc = find_society_num (atoi (arg3))) != NULL)
	{
	  for (i = 0; i < num_times; i++)
	    {
	      update_society  (soc, (i != num_times));
	    }
	  sprintf (buf, "Society %d updated %d times\n\r", 
		   atoi (arg3), atoi(arg2));
	  stt (buf, th);
	  return;
	}
      
      for (i = 0; i < num_times; i++)
	{
	  update_societies ((i != num_times));
	}
      sprintf (buf, "All societies updated %d time%s.\n\r", num_times, (num_times > 1 ? "s" : ""));
      stt (buf, th);
      return;
    }

  /* List info about the societies (very briefly). */
  
  if (!str_cmp (arg1, "list") || !str_cmp (arg1, "count"))
    {
      char namebuf[STD_LEN];
      bool list_them;
      bigbuf[0] = '\0';
      if (!str_cmp (arg1, "list"))
	list_them = TRUE;
      else
	list_them = FALSE;
      
      if (list_them)
	add_to_bigbuf ("\x1b[1;31m+++++++\x1b[1;37m          The Societies         \x1b[1;31m+++++++\x1b[0;37m\n\n\r");
      for (soc = society_list; soc != NULL; soc = soc->next)
	{
	  /* Allow a name to be specified */
	  if (arg2 && str_prefix (arg2, soc->name) &&
	      str_prefix (arg2, soc->pname) &&
	      str_prefix (arg2, soc->adj))
	    continue;
	  
	  if (list_them)
	    {
	      num_castes = 0;
	      for (i = 0; i < CASTE_MAX; i++)
		if (soc->start[i] != 0)
		  num_castes++;
	      if (soc->adj && *soc->adj)
		sprintf (namebuf, "%s %s", soc->adj, soc->pname);
	      else
		sprintf (namebuf, "%s", soc->pname);
	      if ((area = find_area_in (soc->room_start)) != NULL &&
		  area->short_desc)
		{
		  strncpy (areaname, NAME(area), 21);
		  areaname[20] = '\0';
		}
	      else
		*areaname = '\0';
	      sprintf (buf, "[\x1b[1;37m%3d\x1b[0;37m] \x1b[1;32m%-22s\x1b[0;37m R:\x1b[1;36m%7d\x1b[0;37m (%-20s) C:\x1b[1;35m%2d\x1b[0;37m P:\x1b[1;31m%3d\x1b[0;37m A:\x1b[1;34m%d\x1b[0;37m\n\r", 
		       soc->vnum, namebuf, soc->room_start, 
		       areaname, num_castes,
		       soc->population, soc->align);
	      add_to_bigbuf (buf);
	    }
	  total_population += soc->population;
	  num_societies++;
	}
      if (list_them)
	sprintf (buf, "\n\n\r");
      else
	buf[0] = '\0';
      sprintf (buf + strlen(buf), "There %s \x1b[1;33m%d\x1b[0;37m societ%s with \x1b[1;33m%d\x1b[0;37m member%s total.\n\r", 
	       (num_societies == 1 ? "is" : "are"),
	       num_societies, 
	       (num_societies == 1 ? "y" : "ies"),
	       total_population,
	       (total_population == 1 ? "" : "s"));
      add_to_bigbuf (buf);
      send_bigbuf (bigbuf, th);
      return;
    }
  
  /* Show the info about a certain society. */

  if (!str_cmp (arg1, "show") && 
      (soc = find_society_num (atoi(arg2))) != NULL)
    {
      show_society (th, soc);
      return;
    }
  if (!str_cmp (arg1, "generate"))
    {
      generate_societies (th);
      return;
    }

  if (!str_cmp (arg1, "create"))
    {
      if (IS_SET (th->fd->flags, FD_EDIT_PC))
	{
	  stt ("You cannot edit anything else while you are editing a pc.\n\r", th);
	  return;
	}
      if ((soc = create_new_society ()) != NULL)
	{
	  th->fd->connected = CON_SOCIEDITING;
	  th->pc->editing = soc;
	  show_society (th, soc);
	  stt ("Editing this new society.\n\r", th);
	}
      else
	stt ("Error. No society made\n\r", th);
      return;
    }

  /* Edit a certain society. */

  if (!str_cmp (arg1, "edit"))
    {
      if (IS_SET (th->fd->flags, FD_EDIT_PC))
	{
	  stt ("You cannot edit anything else while you are editing a pc.\n\r", th);
	  return;
	}
      if (is_number (arg2) &&
	  (vnum = atoi (arg2)) > 0 && (soc = find_society_num (vnum)) != NULL)
	{
	  th->fd->connected = CON_SOCIEDITING;
	  th->pc->editing = soc;
	  show_society (th, soc);
	  stt ("Editing this society now.\n\r", th);
	  return;
	}
      stt ("That society doesn't exist. Type society list for a list of them.\n\r", th);
      return;
    }


  /* Copy one society onto another. */

  if (!str_cmp (arg1, "copy"))
    {
      SOCIETY *oldsoc, *newsoc;
      if ((oldsoc = find_society_num (atoi (arg2))) == NULL)
	{
	  stt ("That society doesn't exist to copy!\n\r", th);
	  return;
	}
      newsoc = new_society ();
      copy_society (oldsoc, newsoc);
      soc_to_list (newsoc);
      sprintf (buf, "Ok, society %d copied to society %d.\n\r", oldsoc->vnum, newsoc->vnum);
      stt (buf, th);
      return;
    }
  
  /* This clears the city structures for this society (or all societies). */

  if (!str_cmp (arg1, "clearall"))
    {
      society_clearall (th);
      return;
    } 
  stt ("Society update, list, show, edit <num>, create, copy clear <num>.\n\r", th);
  return;
}

void
society_clearall (THING *th)
{
  SOCIETY *soc = NULL;
  VALUE *build;
  THING *area, *room, *mob;
  
  for (area = the_world->cont; area; area = area->next_cont)
    {
      for (room = area->cont; room; room = room->next_cont)
	{
	  if ((build = FNV (room, VAL_BUILD)) != NULL)
	    {
	      remove_flagval (room, FLAG_ROOM1, ROOM_EXTRAMANA | ROOM_EXTRAHEAL);
	      remove_value (room, build);
	      area->thing_flags |= TH_CHANGED;
	    }
	}
    }      
  stt ("All cities for all societies cleared.\n\r", th);
  
  for (soc = society_list; soc; soc = soc->next)
    {
      if (soc->generated_from)
	    soc->society_flags |= SOCIETY_NUKE;
    }
  
  if ((area = find_thing_num (SOCIETY_MOBGEN_AREA_VNUM)) != NULL &&
      IS_AREA (area))
    {
      for (mob = area->cont; mob; mob = mob->next_cont)
	{
	  if (IS_ROOM (mob))
	    continue;
	  SBIT (mob->thing_flags, TH_NUKE);
	}
      area->thing_flags |= TH_CHANGED;
    }
  
  stt ("All generated societies will be cleared after next reboot.\n\r", th);    
  return;
}
  
  
 

/* This creates a new society, gives it a vnum and adds it to the
   various lists. */


SOCIETY *
create_new_society (void)
{
  int i;
  SOCIETY *soc;
  soc = new_society ();
  soc->vnum = 0;
  soc->name = new_str ("New Society");
  soc->aname = new_str ("New Society");
  soc->pname = new_str ("New Society");
  for (i = 1; i <= top_society; i++)
    {
      if (find_society_num (i) == NULL)
	{
	  soc->vnum = i;
	  break;
	}
    }
  if (soc->vnum == 0)
    soc->vnum = ++top_society;
  soc_to_list (soc);
  return soc;
}

/* This updates societies with a false arg. */

void
update_societies_event (void)
{
  update_societies (FALSE);
  return;
}

/* This updates the societies. If it is a rapid update, all societies
   get updated and the society_ticks doesn't get incremented. If not,
   then we look at society_ticks and update only certain societies. */

void
update_societies (bool rapid /* Are we rapidly updating... */)
{
  SOCIETY *soc, *socn;
  int i, jumpsize;

  /* If you have a rapid update, update everything. */
  
  if (rapid)
    {
      jumpsize = 1;
      i = 0;
    }
  else /* Otherwise only update one part of the society list. */
    {
      i = society_ticks;
      jumpsize = PULSE_SOCIETY;
      if (++society_ticks >= PULSE_SOCIETY)
	society_ticks = 0;
    }
  for (; i < SOCIETY_HASH; i += jumpsize)
    {
      for (soc = society_hash[i]; soc; soc = socn)
	{          
	  socn = soc->next_hash;
	  /* Update one society. */
	  update_society (soc, rapid);
	}
    }
  
  
  if (society_ticks == 0 &&
      (!IS_SET (server_flags, SERVER_SPEEDUP) || nr (1,50) == 3))
    write_societies();	  
  
  return;
}

/* This updates a single society. */

void
update_society (SOCIETY *soc, bool rapid)
{
  int hour = 12, i;
  bool daytime = FALSE;

  if (wt_info)
    hour = wt_info->val[WVAL_HOUR];
  
  if (hour >= HOUR_DAYBREAK && hour < HOUR_NIGHT)
    daytime = TRUE;
  
  
  
  /* Update all of the timers. */
  
  if (!rapid)
    {  
      update_society_population(soc);

      if (!find_society_overlord (soc))
	RBIT (soc->society_flags, SOCIETY_OVERLORD);
      else
	SBIT (soc->society_flags, SOCIETY_OVERLORD);
      if (soc->alert > 0)
		soc->alert--;
      if (soc->last_alert > 0)
	soc->last_alert--;
      if (soc->settle_hours > 0)
	soc->settle_hours--;
      if (soc->raid_hours > 0)
	soc->raid_hours--;
      if (soc->assist_hours > 0)
	soc->assist_hours--;
      if (soc->morale < 0)
	soc->morale += MAX(1,-soc->morale/20);
      if (soc->morale > 0)
	soc->morale -= MAX (1,soc->morale/20);      
      
      /* Overlords make morale tend to 0 fast. */
      
      if (IS_SET (soc->society_flags, SOCIETY_OVERLORD))
	soc->morale = soc->morale * 3/4;

      if (soc->alert_hours > 0) /* If the alert hours go to 0, attack. */
	{
	  if (--soc->alert_hours == 0)
	    society_fight(soc);
	}
      if (soc->delete_timer > 0)
	{
	  if (--soc->delete_timer < 1 &&
	      soc->population == 0)
	    {
	      free_society (soc);
	    }
	}
      if (soc->relic_raid_hours > 0)
	soc->relic_raid_hours--;
      /* Update kills...*/
      
      for (i = 0; i < ALIGN_MAX; i++)
	{
	  if (soc->kills[i] > 0)
	    {
	      /* Removes 1 or kills/100...faster decay? */
	      soc->kills[i] -= MAX (1, soc->kills[i]/100);
	    }
	  
	  if (soc->killed_by[i] > 0)
	    {
	      /* Removes 1 or killed_by/100...faster decay? */
	      soc->killed_by[i] -= MAX (1, soc->killed_by[i]/100);
	    }
	  if (--soc->align_affinity[i] < 0)
	    soc->align_affinity[i] = 0;
	  
	}
      


      if (nr (1,5) == 3 &&
	  soc->recent_maxpop > soc->population)
	soc->recent_maxpop--;
    }
  
  /* If the society is destructible it means that it will not
     keep making new members if it's reduced to 0 members.
     A society struggling on the edge isn't destroyed outright. 
     It is assumed that societies like that can "hide". The way
     to destroy it is to take a big society and hit it hard all
     at once. */
  
  if (IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE) &&
      soc->population == 0 && 
      soc->recent_maxpop > 0 &&
      !IS_SET (soc->society_flags, SOCIETY_NUKE))
    {
      soc->delete_timer = RUMOR_HOURS_UNTIL_DELETE *2;
      soc->base_chance[0] = 0;
      soc->chance[0] = 0;
      soc->society_flags |= SOCIETY_NUKE;
      add_rumor (RUMOR_DEFEAT, soc->vnum, 0, 0, 0);
      return;
    }
  
  
  /* This makes the society try to improve itself. */
  
  update_society_goals (soc);
  
  
  /* Make the society members more powerful by letting them
     "grow". */
  
  update_society_members (soc);

  /* Now the society will set up its area as a "City" :P so that it will
     build up a small part of the world. */
  
  if (nr (1,20) == 3)
    setup_city (soc);
  
  /* Check to see if guard posts need to be altered. */
  
  if (soc->guard_post[0] == 0)
    set_up_guard_posts(soc);
  
  
  
  /* Check if the tiers are all mostly full, and that the society
     is large enough, and the random number works out. If so, attempt
     to raid or settle. */
  
  /* These things only happen if it's not during sleep
     time. */
  
  
  if ((daytime == !IS_SET (soc->society_flags, SOCIETY_NOCTURNAL)) ||
      IS_SET (soc->society_flags, SOCIETY_NOSLEEP))
    {
      /* Maybe move to a new location if you're getting your butt
	 kicked. */
      
      society_run_away (soc);
      

      /* Attempt to make a new society. */
      
      settle_new_society (soc);
      
      /* Attempt to raid another society. */
      
      update_raiding (soc, 0);
      
      /* Possibly send out a patrol. */
      
      update_patrols (soc);
      
      

      /* Now the society will help other societies if needed. If there are
	 other societies with the same mobs in their population on the
	 same alignment and this society is big, and those are small, then
	 send a few mobs their way to help them out. */
      
      reinforce_other_societies(soc);
      
      if (!rapid)
	update_society_population(soc);

    }
  
  return;
}


/* This alters the population for a society based on a single thing
   joining or leaving. */


void
update_society_population_thing (THING *th, bool joining)
{
  SOCIETY *society;
  VALUE *socval;
  
  if ((socval = FNV (th, VAL_SOCIETY)) == NULL)
    return;
  
  if ((society = find_society_in (th)) == NULL)
    return;
  
  if (socval->val[1] < 0 || socval->val[1] >= CASTE_MAX)
    return;
  
  if (joining)
    {
      society->population++;
      society->power += POWER(th);
      if (IS_SET (socval->val[2], BATTLE_CASTES))
	society->warriors++;
      society->curr_pop[socval->val[1]]++;
      if (society->recent_maxpop < society->population)
	society->recent_maxpop = society->population;
    }
  else
    {
      if (society->population > 0)
	society->population--;
      society->power -= POWER(th);
      if (IS_SET (socval->val[2], BATTLE_CASTES) &&
	  society->warriors > 0)
	society->warriors--;
      if (society->curr_pop[socval->val[1]] > 0)
	society->curr_pop[socval->val[1]]--;
    }
  return;
}

bool
is_in_society (THING *th)
{
  VALUE *soc;
  
  if (!th || (soc = FNV (th, VAL_SOCIETY)) == NULL)
    return FALSE;
  return TRUE;
}

bool
in_same_society (THING *th1, THING *th2)
{
  VALUE *soc1, *soc2;

  if (!th1 || !th2 || (soc1 = FNV (th1, VAL_SOCIETY)) == NULL ||
      (soc2 = FNV (th2, VAL_SOCIETY)) == NULL ||
      soc1->val[0] != soc2->val[0])
    return FALSE;
  return TRUE;
}

/* This goes down all the tiers of all the castes in the society
   and calculates the total population. */

int
society_curr_pop (SOCIETY *soc)
{
  int i, total = 0;
 
  if (!soc)
    return 0;
  
  for (i = 0; i < CASTE_MAX; i++)
    {
      if (soc->start[i] != 0)
	total += soc->curr_pop [i];
    }
  return total;
}

int
society_max_pop (SOCIETY *soc)
{
  int i, total = 0;
 
  if (!soc)
    return 0;
  
  for (i = 0; i < CASTE_MAX; i++)
    {
      if (soc->start[i] != 0)
	total += soc->max_pop[i];
    }
  return total;
}









/* This finds the population of a certain tier in a certain caste in a
   society. */

int 
find_pop_caste_tier (int socnum, int caste, int tier)
{
  SOCIETY *soc;
  THING *th;
  VALUE *socval;
  int vnum, count = 0;
  
  if ((soc = find_society_num (socnum)) == NULL ||
      caste < 0 || caste >= CASTE_MAX ||
      soc->start[caste] == 0 ||
      tier < 0 || tier >= soc->max_tier[caste])
    return 0;
  
  vnum = soc->start[caste] + tier;
  
  for (th = thing_hash[vnum % HASH_SIZE]; th; th = th->next)
    {
      if (th->vnum == vnum &&
	  (socval = FNV (th, VAL_SOCIETY)) != NULL &&
	  socval->val[0] == soc->vnum &&
	  socval->val[1] == caste)
	count++;
    }
  
  return count;
}



/* This returns the number of members in the society in castes with any
   flags in common with caste_flags. If caste_flags is 0, then this
   gives the total population of the society. */

int
find_num_members (SOCIETY *soc, int caste_flags)
{
  int i, total = 0;
  
  if (!soc)
    return 0;
  
  if (caste_flags == 0)
    return soc->population;
  
  
  for (i = 0; i < CASTE_MAX; i++)
    {
      if (IS_SET (soc->cflags[i], caste_flags))
	total += soc->curr_pop[i];
    }
  return total;
}

/* This changes the alignment of all mobs in the society to the
   new alignment. */

void
change_society_align (SOCIETY *soc, int align)
{
  int i;
  char buf[STD_LEN];
  if (!soc || align < 0 || align >= ALIGN_MAX ||
      !align_info[align])
    return;
  
 
  
  sprintf (buf, "all attr al %d", align);
  society_do_activity (soc, 100, buf);
  
  soc->align = align;
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (!DIFF_ALIGN (soc->align, i))
	soc->kills[i] = 0;
      soc->killed_by[i] = 0;
    }
  
  return;
}

/* This updates the society population, power, and warrior count
   numbers. */


void
update_society_population (SOCIETY *soc)
{
  int i, j;
  THING *th;
  VALUE *socval;
  
  if (!soc)
    return;
  
  soc->power = 0;
  soc->population = 0;
  soc->warriors = 0;
  
  for (i = 0; i < CASTE_MAX; i++)
    {  
      soc->curr_pop[i] = 0;
      for (j = 0; j < soc->max_tier[i];j++)
	{  
	  for (th = thing_hash[(soc->start[i] + j) % HASH_SIZE]; th; th = th->next)
	    { 
	      if (!CAN_MOVE (th) || !CAN_FIGHT (th) ||
		  (socval = FNV (th, VAL_SOCIETY)) == NULL ||
		  socval->val[0] != soc->vnum ||
		  socval->val[1] != i)	    
		continue;
	      soc->curr_pop[i]++;
	      soc->power += POWER(th);
	      soc->population++;
	      if (IS_SET (socval->val[2], BATTLE_CASTES))
		soc->warriors++;
	    }
	}
      soc->max_pop[i] = MAX (soc->max_pop[i], soc->curr_pop[i]);
    }
  if (soc->recent_maxpop < soc->population)
    soc->recent_maxpop = soc->population;
  return;
}

/* This makes society members "grow" into more powerful things. */
	  
void
update_society_members (SOCIETY *soc)
{
  
  VALUE *socval, *socval2;
  THING *th, *thn, *proto, *newmem, *relic;
  int i, j;
  int c_rank, t_rank, vnum, caste_num, new_cnum = 0;
  char buf[STD_LEN];
  char oldname[STD_LEN];
  int warrior_percent;
  int craft_type, shop_type;
  bool all_warriors = FALSE;
  FLAG *flag;
  if (!soc || soc->population < 1) 
    return;
  
  warrior_percent = find_num_members (soc, BATTLE_CASTES)*100/soc->population;
  
  if (warrior_percent < 20)
    all_warriors = TRUE;
  for (i = 0; i < CASTE_MAX; i++)
    {
      if (soc->start[i] == 0 ||
	  soc->curr_tier[i] == 0 ||
	  soc->max_pop[i] == 0)
	continue;      
      
      for (j = soc->max_tier[i] - 1; j >= 0; j--)
	{  
	  for (th = thing_hash[(soc->start[i] + j) % HASH_SIZE]; th; th = thn)
	    {
	      thn = th->next;
	      craft_type = PROCESS_MAX;
	      shop_type = SOCIETY_SHOP_MAX;
	      /* Make sure this thing can move and fight (mob), and is a 
		     thing in a society, and a caste and not a pc. */
	      
	      if (!CAN_MOVE (th) || !CAN_FIGHT (th) ||
		  (socval = FNV (th, VAL_SOCIETY)) == NULL ||
		  socval->val[0] != soc->vnum ||
		  (caste_num = find_caste_in (th)) == CASTE_MAX)
		continue;
	      
	      /* What rank we are within the caste atm. */
	      /* c_rank starts at 0. */
	      c_rank = th->vnum - soc->start[caste_num];
	      
	      /* What the highest rank in the caste is. */
	      t_rank = soc->curr_tier[caste_num] - 1;
	      
	      /* Leader caste has an extra tier that isn't used very
		 often. It's the "overlord" tier that only makes 
		 societies get overlords when there are 3 things just
		 below overlord level. */
	      if (IS_SET (soc->cflags[caste_num], CASTE_LEADER) &&
		  c_rank == t_rank - 1)
		{
		  if (!society_can_add_overlord (soc))
		    continue;
		}
	      
	      vnum = 0;
		  
	      /* Check if a "child" chooses a path or not. It must be
		 in caste[0] and must be at the top rank. We do this by
		 finding a random caste based on the percentages given
		 for tier 0 in each of those castes. */
	      
	      /* Otherwise, it is a regular "growth", and we require that
		 the thing must not be of the max tier, and there must be
		 population room for it to advance and the new thing must
		 exist and it must be an acceptable type of thing. */
	      
	      
	      if (caste_num == 0 && c_rank == t_rank)
		{
		  
		  if ((new_cnum = find_caste_from_percent (soc, FALSE, all_warriors)) != -1 &&
		      /* Keep the builder population low... */
		      (!IS_SET (soc->cflags[new_cnum], CASTE_BUILDER) ||
		       soc->curr_pop[new_cnum] == 0 ||
		       nr (1,15) == 3) &&
		       
		      soc->curr_pop[new_cnum] < soc->max_pop[new_cnum] &&
		      (proto = find_thing_num (soc->start[new_cnum])) != NULL &&
		      CAN_MOVE(proto) && CAN_FIGHT(proto))
		    {
		      vnum = soc->start[new_cnum];
		    } 
		  
		  /* Don't ever have too many shopkeepers...or 
		     crafters starts to look stupid. */
		  
		  if (IS_SET (soc->cflags[new_cnum],
			      CASTE_CRAFTER) &&
		      (craft_type = find_crafter_needed (soc)) == PROCESS_MAX)
		    continue;
		  
		  
		  if (IS_SET (soc->cflags[new_cnum], CASTE_SHOPKEEPER) &&
		      (shop_type = find_shop_needed (soc)) == SOCIETY_SHOP_MAX)
		    continue;
		  
		  
		} /* Increase tier within a caste. */
	      else if (c_rank < t_rank && 
		       nr (0, c_rank+3) == (c_rank + 1)/2)
		{
		  int c_rank_pop, n_rank_pop;
		  c_rank_pop = find_pop_caste_tier (soc->vnum,
						    caste_num,
						    c_rank);
		  n_rank_pop = find_pop_caste_tier (soc->vnum,
						    caste_num,
						    c_rank+1);
		  
		  /* Don't let the max tier get too big. It is restricted
		     to 1/N the total pop of the caste. Except for
		     children that can max as many as they want. */
		  
		  if (caste_num > 0 && 
		      c_rank + 1 >= soc->max_tier[caste_num] &&
		      soc->max_pop[caste_num] <  10 * n_rank_pop)
		    continue;
		  
		  


		  /* Don't allow an advancement unless the current tier
		     population is larger than the next one up. Ignore this
		     if the populations are too small. */
		  
		  if (c_rank_pop <= n_rank_pop &&
		      c_rank_pop > 2)
		    continue;
		  
		  /* Check if the prototype exists. */
		  
		  if ((proto = find_thing_num (th->vnum + 1)) != NULL &&
		      !IS_SET (proto->thing_flags, TH_NO_FIGHT | TH_NO_MOVE_SELF | TH_IS_ROOM | TH_IS_AREA))
		    vnum = th->vnum + 1;
		  new_cnum = caste_num;
		  
		  /* Need to pay mineral costs to have battlers advance 
		     tiers. Cost is 10*the new tier. */
		  
		  if (IS_SET (soc->cflags[caste_num], BATTLE_CASTES) &&
		      
		      soc->raw_curr[RAW_MINERAL] < 
		      BATTLE_CASTE_ADVANCE_COST * (c_rank + 1))
		    {
		      soc->raw_want[RAW_MINERAL] += 
			BATTLE_CASTE_ADVANCE_COST * (c_rank + 1);
		      continue;
		    }		  
		}
	      
	      if (vnum ==0 ||
		  (newmem = create_thing (vnum)) == NULL)
		continue;
	      
	      /* Bonus for getting beat up. */
	      
	      if (newmem->proto)
	      newmem->level = newmem->proto->level + 
		soc->level_bonus + soc->quality;
	      
	      /* Do we want to add in costs here? or not? */
	      
	      /* Add in a part where we copy the name (by hand for now...) */
	      
	      if (socval->word && *socval->word)
		strcpy (oldname, socval->word);
	      else
		*oldname = '\0';

	      /* Replaces a thing with another...including what fighting 
		 and all that. */
	      
	      if (replace_thing (th, newmem))
		{
		  reset_thing (newmem, 0);
		  if ((socval2 = FNV (newmem, VAL_SOCIETY)) == NULL)
		    {
		      socval2 = new_value ();
		      socval2->type = VAL_SOCIETY;
		      add_value (newmem, socval2);
		    }
		  socval2->val[0] = soc->vnum;
		  socval2->val[1] = new_cnum;
		  socval2->val[2] = soc->cflags[new_cnum];
		  socval2->val[3] = socval->val[3];
		  /* Now update populations. */
		  newmem->align = soc->align;
		  update_society_population_thing (newmem, TRUE);
		  
		  /* Now add the new name in. Crafters/shopkeepers
		     need generated names due to their different
		     kinds of jobs.*/
		  
		  if (IS_SET (soc->cflags[new_cnum], 
			      CASTE_CRAFTER | CASTE_SHOPKEEPER))
		    {
		      if (caste_num == 0 && c_rank == t_rank)
			{
			  if (IS_SET (soc->cflags[new_cnum],
				      CASTE_CRAFTER))
			    socval2->val[3] = craft_type;
			  else 
			    {
			      if (IS_SET (soc->cflags[new_cnum],
					  CASTE_SHOPKEEPER) &&
				  shop_type == SOCIETY_SHOP_MAX)
			      socval2->val[3] = shop_type;
			      society_setup_shopkeeper(newmem);
			    }
			}
		      
		      setup_society_maker_name (newmem);
		    }
		  else if (!IS_SET (soc->society_flags, SOCIETY_NONAMES))
		    {
		      if (!*oldname)
			create_society_name (socval2);
		      setup_society_name (newmem);
		    }
		  
		  for (flag = soc->flags; flag; flag = flag->next)
		    add_flagval (newmem, flag->type, flag->val);
		  
		  /* If the society member is a leader and if its
		     rank is the max rank and if it's lucky, it
		     gets a relic made. */
		  
		  if (c_rank == (t_rank-1) &&
		      IS_SET (soc->cflags[new_cnum], CASTE_LEADER) &&
		      nr (1,RELIC_CHANCE) == RELIC_CHANCE/2 &&
		      (relic = create_thing (RELIC_VNUM)) != NULL)
		    {
		      /* The relic's names are slightly modified. */
		      free_str (relic->short_desc);
		      free_str (relic->name);
		      free_str (relic->long_desc);
		      sprintf (buf, "A relic of the %s", soc->pname);
		      relic->short_desc = new_str (buf);
		      strcat (buf, " is here.");
		      relic->long_desc= new_str (buf);
		      sprintf (buf, "relic %s", soc->name);
		      relic->name = new_str (buf);
		      thing_to (relic, newmem);		      
		      SBIT (soc->society_flags, SOCIETY_OVERLORD);
		    }		  

		  /* Reset some objects on this thing if it's not
		     in the children's caste. */
		  if (new_cnum > 0)
		    add_society_objects (newmem);
		}
	    }
	}	
    }
  return;
}

/* What this does is if the player asks a mob for news, the mob will
   search through all society members that it has in its society 
   (if any) and it will tell the player that some nasty other society
   has captured someone and ask them to get that person back. */

bool 
send_kidnapped_message (THING *th, THING *mob)
{
  THING *thg, *victim = NULL, *area_in;
  int caste, tier, count;
  SOCIETY *society, *enemy_society;
  VALUE *socval, *v_socval;
  int vnum;
  int num_choices = 0, num_chose = 0, choice = 0;
  char buf[STD_LEN];
  /* Some sanity checking and setup. */

  if (!th || !mob || !IS_PC (th) || IS_PC (mob) ||
      (socval = FNV (mob, VAL_SOCIETY)) == NULL ||
      (society = find_society_num (socval->val[0])) == NULL ||
      th->align == 0 || DIFF_ALIGN (th->align, mob->align) ||
      nr (1,3) != 2)
    return FALSE;
  
  /* Now see if the society has any members who have been
     kidnapped. */
  
  for (count = 0; count < 2; count++)
    {
      for (caste = 0; caste < CASTE_MAX; caste++)
	{
	  for (tier = 0; tier < society->max_tier[caste]; tier++)
	    {
	      vnum = society->start[caste] + tier;
	      for (thg = thing_hash[vnum % HASH_SIZE]; thg; thg = thg->next)
		{
		  if (!thg->in || !IS_ROOM (thg->in))
		    continue;
		  
		  /* We only care about thigns that are in the same
		     society as the mob and which are prisoners and they
		     have been captured by some other society. */

		  if ((v_socval = FNV (thg, VAL_SOCIETY)) == NULL ||
		      v_socval->val[0] != socval->val[0] ||
		      !IS_ACT1_SET (thg, ACT_PRISONER) ||
		      (enemy_society = find_society_num (v_socval->val[5])) == NULL ||
		      (area_in = find_area_in (enemy_society->room_start)) == NULL)
		    continue;
		  
		  if (count == 0)
		    num_choices++;
		  else if (++choice >= num_chose &&
			   victim == NULL)
		    {
		      victim = thg;
		      break;
		    }
		}
	    }
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    break;
	  num_chose = nr (1, num_choices);
	  choice = 0;
	}
    }
  
  /* If a victim was found and it was in the proper society and it had
     an enemy society that kidnapped it, give us the info. */
  
  if (victim &&
      (v_socval = FNV (victim, VAL_SOCIETY)) != NULL &&
      (enemy_society = find_society_num (v_socval->val[5])) != NULL &&
      (area_in = find_area_in (enemy_society->room_start)) != NULL)
    {
      sprintf (buf, "Please help us! %s has been captured by the %s of %s!",
	       NAME(victim), enemy_society->pname, NAME(area_in));
      do_say (mob, buf);
      return TRUE;
    }
  return FALSE;
}
      
  
/* This clears the recent maxpop (and alert) for a society so that if
   you nuke the mobs within an area, you don't make the other societies
   assist this one (since it wasn't a regular loss of population) */

void
society_clear_recent_maxpop (THING *area)
{
  SOCIETY *soc;
  THING *soc_area;

  for (soc = society_list; soc; soc = soc->next)
    {
      if (!area ||
	  (soc_area = find_area_in (soc->room_start)) == area)
	{
	  soc->recent_maxpop = soc->population;
	  soc->alert = 0;
	}
    }
  return;
}
  
/* This gives society objects to a society member when it's upgraded. */
  
void
add_society_objects (THING *th)
{
  SOCIETY *soc;
  THING *new_obj, *obj, *proto;
  int vnum, num_tries, tries, num_already_have, reset_pct;
  VALUE *socval;
 

  if (!th || !th->in || (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_num (socval->val[0])) == NULL)
    return;

  /* Must be in nonchildren's caste. */

  if (socval->val[1] <= 0 || socval->val[1] >= CASTE_MAX)
    return;
  
  /* Get from 1-3 tries. More if you have less tiers in your caste. */
  
  num_tries = MID (2, 5-soc->max_tier[socval->val[1]], 4);
  
  for (tries = 0; tries < num_tries; tries++)
    {
      vnum = nr (soc->object_start, soc->object_end);
      
   
      /* Make sure the proto exists and is appropriate. */
      if ((proto = find_thing_num (vnum)) == NULL ||
	  IS_ROOM (proto) || IS_AREA (proto) || CAN_MOVE (proto) ||
	  CAN_FIGHT (proto) ||
	  proto->wear_pos <= ITEM_WEAR_NONE ||
	  proto->wear_pos >= ITEM_WEAR_MAX ||
	  proto->level > th->level*3/2)
	continue;
      

      
      num_already_have = 0;
      for (obj = th->cont; obj; obj = obj->next_cont)
	{
	  if (obj->vnum == vnum)
	    num_already_have++;
	}
      
      /* See if we can even wear another copy of this item. */
      if (num_already_have >= wear_data[proto->wear_pos].how_many_worn)
	continue;
      
      /* Now see if the object even resets. */
      if (!IS_SET (server_flags, SERVER_AUTO_WORLDGEN))
	reset_pct = MAX (2, 70-proto->level);
      else
	reset_pct = 50;
      if (nr (1,100) > reset_pct)
	continue;

      /* We know that the object is appropriate and not too powerful and
	 that the mob doesn't have too many already. So, make the object
	 and give it to the mob. */

      if ((new_obj = create_thing (vnum)) == NULL)
	continue;
      
      thing_to (new_obj, th);
    }
  do_wear (th, "all");
  return;
}


/* This finds a random society name out of all or just 
   those that were generated! The name_type is 'n' 'a' or 'p' to
   return the name, aname or pname, and defaults to name if
   'a' or 'p' aren't there. */


char *
find_random_society_name (char name_type, int generated_from)
{
  SOCIETY *soc;
  int num_choices = 0, num_chose = 0, count = 0;
  static char retbuf[STD_LEN];
  char *t;
  
  retbuf[0] = '\0';
  for (count = 0; count < 2; count++)
    {
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (!soc->name || !*soc->name ||
	      /* Organization/ancient races only choose those types. */
	      
	      (generated_from >= ANCIENT_RACE_SOCIGEN_VNUM &&		
	       soc->generated_from != generated_from) ||
	      /* If generated from is reasonable, only pick those in
		 the generated range. */
	      (generated_from > 0 && generated_from <= 1000000 &&
	       (soc->generated_from == 0 || soc->generated_from > 1000000)))       	    continue;
	  
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
  
  if (!soc)
    return "Society";
  
  if (soc->adj)
    sprintf (retbuf, "%s", soc->adj);
  if (*retbuf)
    strcat (retbuf, " ");
  if (LC(name_type) == 'a' && soc->aname && *soc->aname)
    strcat (retbuf, soc->aname);
  else if (LC (name_type) == 'p' && soc->pname && *soc->pname)
    strcat (retbuf, soc->pname);
  else
    strcat (retbuf, soc->name);
  for (t = retbuf; *t; t++)
    *t = LC(*t);
  return retbuf;
}


  

/* This finds a random society out of the list of */


SOCIETY *
find_random_society (bool only_generated)
{
  SOCIETY *soc;
  int num_choices = 0, num_chose = 0, count = 0;
  static char retbuf[STD_LEN];

  retbuf[0] = '\0';
  for (count = 0; count < 2; count++)
    {
      for (soc = society_list; soc; soc = soc->next)
	{
	  if ((only_generated && soc->generated_from == 0) ||
	      IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE))
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
  
  return soc;
}
  

/* This finds a generated society that isn't nuked of the appropriate
   alignment...or any alignment if align is < 0 or >= ALIGN_MAX */
SOCIETY *
find_random_generated_society (int align)
{
  SOCIETY *soc;
  int num_choices = 0, num_chose = 0, count;
  
  for (count = 0; count < 2; count++)
    {
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (IS_SET (soc->society_flags, SOCIETY_NUKE) ||
	      soc->generated_from == 0 ||
	      (align >= 0 && align < ALIGN_MAX &&
	       soc->align != align))
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;
	  num_chose = nr (1, num_choices);
	}
    }
  return soc;
}
  
/* This finds a random caste name based on the "acceptable" castes passed
   as an argument to this. */

char *
find_random_caste_name_and_flags (int caste_flags, int *return_flags)
{
  static char ret[STD_LEN];
  int count, num_choices = 0, num_chose = 0;
  THING *area, *obj;
  
  if ((area = find_thing_num (SOCIETY_CASTE_AREA_VNUM)) == NULL)
    return NULL;
  

  for (count = 0; count < 2; count++)
    {
      for (obj = area->cont; obj; obj = obj->next_cont)
	{
	  if (IS_ROOM (obj) || !obj->desc || !*obj->desc ||
	      (caste_flags && 
	       !IS_SET (flagbits (obj->flags, FLAG_CASTE), caste_flags)))
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	  
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;
	  
	  num_chose = nr (1, num_choices);
	}
    }
  
  if (!obj)
    return NULL;
  
  strcpy (ret, find_random_word (obj->desc, NULL));
  if (return_flags)
    *return_flags = flagbits (obj->flags, FLAG_CASTE);
  return ret;
}
  
/* This unmarks all societies. */

void 
unmark_all_societies (void)
{
  SOCIETY *soc;
  
  for (soc = society_list; soc; soc = soc->next)
    RBIT (soc->society_flags, SOCIETY_MARKED);
  return;
}
	  
/* Give lots of society members disease. */
  
void
society_get_disease (void)
{
  SOCIETY *soc;
  int caste, tier, vnum;
  int count, num_choices = 0, num_chose = 0;
  THING *mob;
  VALUE *socval;
  FLAG *flg;
  
  if (nr (1,10) != 2)
    return;

  /* Choose a society with a big population. */

  for (count = 0; count < 2; count++)
    {
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (soc->population < soc->population_cap*4/5 ||
	      !soc->generated_from)
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
	  num_chose = nr (1, num_choices);
	}
    }
  
  if (!soc)
    return;

  for (caste = 0; caste < CASTE_MAX; caste++)
    {
      for (tier = 0; tier < soc->max_tier[caste]; tier++)
	{
	  vnum = soc->start[caste] + tier;
	  for (mob = thing_hash[vnum % HASH_SIZE]; mob; mob = mob->next)
	    {
	      if ((socval = FNV (mob, VAL_SOCIETY)) == NULL ||
		  socval->val[0] != soc->vnum)
		continue;
	      
	      flg = new_flag();
	      flg->timer = nr (5, 10);
	      flg->from = MAX_SPELL;
	      flg->type = FLAG_HURT;
	      flg->val = AFF_DISEASE;
	      aff_update (flg, mob);
	    }
	}
    }
  add_rumor (RUMOR_PLAGUE, soc->vnum, 0, 0, 0);
  return;
}


/* This returns the name of a society or the plural name of a 
   society, respectively. */

char *
society_name (SOCIETY *soc)
{
  static char name[STD_LEN];
  if (!soc || !soc->name || !*soc->name)
    return "people";
  
  name[0] = '\0';

  if (soc->adj && *soc->adj)
    {
      strcat (name, soc->adj);
      strcat (name, " ");
    }
  strcat (name, soc->name);
  return name;
}
/* This returns the name of a society or the plural name of a 
   society, respectively. */

char *
society_pname (SOCIETY *soc)
{
  static char pname[STD_LEN];
  if (!soc || !soc->pname || !*soc->pname)
    return "people";
  
  pname[0] = '\0';

  if (soc->adj && *soc->adj)
    {
      strcat (pname, soc->adj);
      strcat (pname, " ");
    }
  strcat (pname, soc->pname);
  return pname;
}


/* This adds morale to a society. */

void
add_morale (SOCIETY *soc, int amount)
{
  if (!soc)
    return;

  soc->morale += amount;
  soc->morale = MID (-MAX_MORALE, soc->morale, MAX_MORALE);
  return;
}

/* This checks if a society can add an overlord. It can only happen if
   there are three+ society leaders one rank below the max rank and
   if there is no overlord. Then, the overlord gets made. */

bool
society_can_add_overlord (SOCIETY *soc)
{
  THING *mob, *proto;
  int vnum, count;
  int leader_count = 0, overlord_count = 0;
  int cnum;
  VALUE *socval;
  
  if (!soc)
    return FALSE;

  for (cnum = 0; cnum < CASTE_MAX; cnum++)
    {
      if (IS_SET (soc->cflags[cnum], CASTE_LEADER))
	{
	  /* Loop through 2 times. First time, get the number of maximal
	     leaders besides the overlord, then next time check for
	     the overlord. */
	  for (count = 0; count < 2; count++)
	    {
	      vnum = soc->start[cnum] + soc->max_tier[cnum] - 2 + count;
	      
	      /* Make sure that the overlord proto exists and that
		 it's called an "overlord" */
	      if (count == 1)
		{
		  if ((proto = find_thing_num (vnum)) == NULL ||
		      !is_named (proto, "overlord"))
		    return FALSE;
		}
	      
	      for (mob = thing_hash[vnum % HASH_SIZE]; mob; mob = mob->next)
		{
		  if ((socval = FNV (mob, VAL_SOCIETY)) != NULL &&
		      socval->val[0] == soc->vnum)
		    {
		      if (count == 0)
			leader_count++;
		      else
			overlord_count++;
		    }
		}
	    }
	}
    }
  
  /* Only return true if the leader_count is >= 3 and there is no
     overlord. */

  if (leader_count >= 3 && overlord_count == 0)
    return TRUE;
  return FALSE;
}

/* This checks if a society even has an overlord. */
THING * find_society_overlord (SOCIETY *soc)
{
  THING *mob;
  int vnum;
  int cnum;
  VALUE *socval;
  
  if (!soc)
    return NULL;
  
  for (cnum = 0; cnum < CASTE_MAX; cnum++)
    {
      if (IS_SET (soc->cflags[cnum], CASTE_LEADER))
	{
	  vnum = soc->start[cnum] + soc->max_tier[cnum] - 1;
	  for (mob = thing_hash[vnum % HASH_SIZE]; mob; mob = mob->next)
	    {
	      /* An overlord is a leader of max tier. */
	      if ((socval = FNV (mob, VAL_SOCIETY)) != NULL &&
		  socval->val[0] == soc->vnum &&
		  is_named (mob, "overlord"))
		return mob;
	    }
	}
    }
  return NULL;
}
