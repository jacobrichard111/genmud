#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include<dirent.h>
#include "serv.h"


/* Btw, this isn't real security. It's just a few things to make
   twinks have a harder time screwing with the game. */

static int max_remorts_found_in_pbase = 0;

/* These create and destroy siteban data. */

SITEBAN *
new_siteban (void)
{
  SITEBAN *newsiteban;

  newsiteban = (SITEBAN *) mallok (sizeof (*newsiteban));
  bzero (newsiteban, sizeof (*newsiteban));
  siteban_count++;
  newsiteban->address = nonstr;
  newsiteban->who_set = nonstr;
  newsiteban->time_set = current_time;
  return newsiteban;
}

void
free_siteban (SITEBAN *ban)
{
  if (!ban)
    return;
  
  free_str (ban->address);
  free_str (ban->who_set);
  free2 (ban);
  siteban_count--;
  return;
}


/* These two functions read and write sitebans. There isn't really any
   error checking, so be careful if you change the format, although
   you probably shouldn't have too many of these I would guess. */

void
read_sitebans (void)
{
  SITEBAN *ban;

  FILE_READ_SETUP ("siteban.dat");
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("SITEBAN")
	{
	  ban = new_siteban ();
	  ban->next = siteban_list;
	  siteban_list = ban;
	  ban->address = new_str (read_string (f));
	  ban->type = read_number (f);
	  ban->who_set = new_str (read_string (f));
	  ban->time_set = read_number (f);
	}
      FKEY("END_OF_SITEBANS")
	break;
      FILE_READ_ERRCHECK("siteban.dat");
    }
  fclose (f);
  return;
}


void
write_sitebans (void)
{
  FILE *f;
  SITEBAN *ban;
  if ((f = wfopen ("siteban.dat", "w")) == NULL)
    return;
  
  for (ban = siteban_list; ban != NULL; ban = ban->next)
    {
      fprintf (f, "SITEBAN\n");
      fprintf (f, "%s%c\n", ban->address, END_STRING_CHAR);
      fprintf (f, "%d\n", ban->type);
      fprintf (f, "%s%c\n", ban->who_set, END_STRING_CHAR);
      fprintf (f, "%d\n", ban->time_set);
    }
  fprintf (f, "\n\rEND_OF_SITEBANS\n");
  fclose (f);
  return;
}

      
/* This is the siteban function. If there is no argument, the function
   lists all of the banned sites, if there is an argument, and it starts
   with "newbie", then the function makes a newbie ban, otherwise it
   makes a regular ban. */


void 
do_siteban (THING *th, char *arg)
{
  SITEBAN *ban, *prev;
  char arg1[STD_LEN];
  char buf[STD_LEN];
  bool newbie = FALSE;
  
  if (!IS_PC (th) || LEVEL (th) < MAX_LEVEL)
    return;
  
  if (!arg || !*arg)
    {
      stt ("This is a list of all the banned sites:\n\n\r", th);
      for (ban = siteban_list; ban != NULL; ban = ban->next)
	{
	  sprintf (buf, "%s %-18s set by %-12s on %s.\n\r",
		   (ban->type == 1 ? "\x1b[1;37m(N)\x1b[0;37m" : ""),
		   ban->address,
		   ban->who_set,
		   c_time (ban->time_set));
	  stt (buf, th);
	}
      return;
    }

  
  arg = f_word (arg, arg1);
  
  if (!str_prefix ("new", arg1))
    {
      arg = f_word (arg, arg1);
      newbie = TRUE;
    }
  
  if (!arg1[0])
    {
      stt ("Siteban <site> or Siteban newbie <site>.\n\r", th);
      return;
    }

  for (ban = siteban_list; ban != NULL; ban = ban->next)
    {
      if (!str_cmp (ban->address, arg1))
	{
	  if (!newbie && ban->type == 1)
	    {
	      stt ("Changing the siteban to a complete ban.\n\r", th);	      
	      ban->type = 0;
	      free_str (ban->who_set);
	      ban->who_set = new_str (NAME (th));
	      ban->time_set = current_time;
	      write_sitebans ();
	      return;
	    }
	  if (newbie && ban->type == 0)
	    {
	      stt ("Changing the siteban to a newbie ban only.\n\r", th);
	      free_str (ban->who_set);
	      ban->who_set = new_str (NAME (th));
	      ban->time_set = current_time;
	      ban->type = 1;
	      write_sitebans ();
	      return;
	    }
	  
	  stt ("Ok clearing the banned site!\n\r", th);
	  if (ban == siteban_list)
	    {
	      siteban_list = siteban_list->next;
	      free_siteban (ban);
	    }
	  else
	    {
	      for (prev = siteban_list; prev != NULL; prev = prev->next)
		{
		  if (prev->next == ban)
		    {
		      prev->next = ban->next;
		      free_siteban (ban);
		      break;
		    }
		}
	    }
	  write_sitebans ();
	  return;
	}
    }
  
  /* So now we know that the address we are putting in is nothing
     like any of the other addresses. */

  stt ("New siteban set.\n\r", th);
  ban = new_siteban ();
  ban->type = newbie;
  ban->address = new_str (arg1);
  ban->who_set  = new_str (NAME (th));
  ban->time_set = current_time;
  ban->next = siteban_list;
  siteban_list = ban;
  write_sitebans ();
  return;
}
  
/* Checks list of banned sites to see if this one matches it. */

bool
is_sitebanned (FILE_DESC *fd, int type)
{
  SITEBAN *ban;
  if (!fd || type < 0 || type > 2)
    return FALSE;
  
  for (ban = siteban_list; ban != NULL; ban = ban->next)
    {

      /* Newbie banned sites are not checked during a regular check. */

      if (type == 0 && ban->type == 1)
	continue;
      if (!str_prefix (ban->address, fd->num_address) ||
	  !str_cmp (ban->address, "*"))
	return TRUE;
    }
  return FALSE;
}



/* This creates a new piece of pbase data. */

PBASE *
new_pbase (void)
{
  PBASE *pbnew;
  
  pbnew = (PBASE *) mallok (sizeof (*pbnew));
  bzero (pbnew, sizeof (*pbnew));
  pbnew->name = nonstr;
  pbnew->email = nonstr;
  pbnew->site = nonstr;
  pbase_count++;
  return pbnew;
}

/* This does not recycle since it will be VERY rare that we actually
   clean up the pbase info */
void
free_pbase (PBASE *pb)
{
  if (!pb)
    return;
  
  free_str (pb->name);
  free_str (pb->email);
  free_str (pb->site);
  free2 (pb);
  pbase_count--;
  return;
}

/* This reads in the pbase data. It also checks each pfile as it
   reads the data in so players that delete don't have pbases anymore. */

void
read_pbase (void)
{
  PBASE *pb;
  FILE *pf; /* Playerfile we check. */
  char buf[STD_LEN];
  FILE_READ_SETUP ("pbase.dat");
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("PBASE")
	{ 
	  pb = new_pbase ();
	  pb->name = new_str (read_string (f));
	  pb->email = new_str (read_string (f));
	  pb->site = new_str (read_string (f));
	  pb->level = read_number (f);
	  pb->remorts = read_number (f);
	  pb->align = read_number (f);
	  pb->race = read_number (f);
	  pb->last_logout = read_number (f);
	  
	  /* Check if the player still exists. */
	  sprintf (buf, "%s%s", PLR_DIR, capitalize(pb->name));
	  if ((pf = fopen (buf, "r")) != NULL)
	    {
	      pb->next = pbase_list;
	      pbase_list = pb;
	      fclose (pf);
	    }
	  else
	    free_pbase (pb);
	}
      FKEY("END_OF_PBASE")
	break;
      FILE_READ_ERRCHECK("pbase.dat");
    }
  fclose (f);
  write_pbase();
  return;
}


void
write_pbase (void)
{
   FILE *f;
   PBASE *pb;
   if ((f = wfopen ("pbase.dat", "w")) == NULL)
     return;
  
  for (pb = pbase_list; pb != NULL; pb = pb->next)
    {
      fprintf (f, "PBASE\n");
      fprintf (f, "%s%c\n", pb->name, END_STRING_CHAR);
      fprintf (f, "%s%c\n", pb->email, END_STRING_CHAR);
      fprintf (f, "%s%c\n", pb->site, END_STRING_CHAR);
      fprintf (f, "%d %d %d %d %d\n", pb->level, pb->remorts, pb->align,
	       pb->race, pb->last_logout);
    }
  fprintf (f, "\n\rEND_OF_PBASE\n");
  fclose (f);
  return;
}



/* This checks if the character pointed to by th needs to be added to
   the pbase list, or whether it is already on there. In either case,
   the data about th is updated, and the file gets saved once in a while. */

void
check_pbase (THING *th)
{
  PBASE *pb;

  if (!th || !IS_PC (th))
    return;
  
  for (pb = pbase_list; pb != NULL; pb = pb->next)
    {
      if (!str_cmp (pb->name, NAME (th)))
	break;
    }
  
  if (!pb)
    {
      pb = new_pbase ();
      pb->next = pbase_list;
      pb->name = new_str (NAME (th));
      pbase_list = pb;
    }
  
  if (!pb->email || !pb->email[0])
    pb->email = new_str (th->pc->email);
  if (th->fd && 
      (!pb->site || !*pb->site || str_cmp (pb->site, th->fd->num_address)))
    {
      free_str (pb->site);
      pb->site = new_str (th->fd->num_address);
    }
  
  pb->align = th->align;
  pb->level = th->level;
  pb->remorts = th->pc->remorts; 
  pb->race = th->pc->race;
  pb->last_logout = current_time;
  write_pbase ();
  return;
}


/* Gives some pbase information. */

void
do_pbase (THING *th, char *arg)
{
  PBASE *pb, *pbn;
  THING *thg;
  int lev[ALIGN_MAX];
  int max_lev[ALIGN_MAX];
  int rem[ALIGN_MAX][MAX_REMORTS];
  int max_rem[ALIGN_MAX];
  int num[ALIGN_MAX];
  int num_pc = 0, num_lev = 0;
  char buf[STD_LEN];
  RACE *align;
  
  int i, j;
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      lev[i] = 0;
      max_lev[i] = 0;
      num[i] = 0;
      max_rem[i] = 0;
      for (j = 0; j < MAX_REMORTS; j++)	
	rem[i][j] = 0;
    }
  
  if (LEVEL (th) == MAX_LEVEL && !str_cmp (arg, "reset"))
    {
      for (pb = pbase_list; pb != NULL; pb = pbn)
	{
	  pbn = pb->next;
	  free_pbase (pb);
	}
      pbase_list = NULL;
      for (thg = thing_hash[PLAYER_VNUM % HASH_SIZE]; thg != NULL; thg = thg->next)
	{
	  if (IS_PC (thg))
	    check_pbase (thg);
	}
      write_pbase ();
      stt ("Ok, pbase reset and updated.\n\r", th);
      return;
    }
  
  for (pb = pbase_list; pb != NULL; pb = pb->next)
    {
      if (pb->align >= ALIGN_MAX || pb->level >= BLD_LEVEL)
	continue;
      
      lev[pb->align] += pb->level;
      num_lev += pb->level;
      num_pc++;
      num[pb->align]++;
      if (pb->level > max_lev[pb->align])
	max_lev[pb->align] = pb->level;
      if (pb->remorts < MAX_REMORTS)
	{
	  rem[pb->align][pb->remorts]++;
	  if (pb->remorts > max_rem[i])
	    max_rem[i] = pb->remorts;
	}
    }
  
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if ((align = find_align (NULL, i)) == NULL)
	continue;
      if (num[i] < 1)
	{
	  sprintf (buf, "\x1b[1;37m%d.\x1b[0;37m %-12s has no players at all.\n\r", i, align->name);
	}
      else
	sprintf (buf, "\x1b[1;37m%d.\x1b[0;37m %-12s has \x1b[1;36m%d\x1b[0;37m player%s with average level \x1b[1;31m%d\x1b[0;37m, and top remort \x1b[1;35m%d\x1b[0;37m\n\r", i, align->name, num[i], (num[i] == 1 ? "" : "s"),
		 lev[i]/num[i], max_rem[i]);
      stt (buf, th);
    }
  return;
}


/* This gives information about a player based on their pbase info. It
   gives more info to admins than just random players. */

void
do_finger (THING *th, char *arg)
{
  THING *vict;
  PBASE *pb;
  char buf[STD_LEN];
  RACE *race, *align;
  
  if (!*arg)
    {
      stt ("finger <playername>\n\r", th);
      return;
    }
  
  vict = find_pc (th, arg);
  
  for (pb = pbase_list; pb != NULL; pb = pb->next)
    {
      if (!str_prefix (arg, pb->name))
	break;
    }
  
  if (!pb || (LEVEL (th) < MAX_LEVEL && pb->level < MAX_LEVEL &&
	      DIFF_ALIGN (pb->align, th->align)))
    {
      stt ("Finger who?\n\r", th);
      return;
    }
  
  race = find_race (NULL, pb->race);
  align = find_align (NULL, pb->align);
  if (!vict || !can_see (th, vict))
    {
      int playtime = (current_time - pb->last_logout)/3600;
      if (LEVEL (th) < BLD_LEVEL || !race || !align)
	sprintf (buf, "%s logged on %d hour%s ago.\n\r", 
		 pb->name, playtime, (playtime == 1 ? "" : "s"));
      else
	{
	  sprintf (buf, "%s the level %d %s %s with %d remort%s logged on %d hour%s ago.\n\r", 
		   pb->name, 
		   pb->level,
		   align->name,
		   race->name,
		   pb->remorts,
		   (pb->remorts == 1 ? "" : "s"),
		   playtime, 
		   (playtime == 1 ? "" : "s"));
	}
      stt (buf, th);
    }
  else
    {
      if (LEVEL (th) < BLD_LEVEL || !race || !align)
	sprintf (buf, "%s is currently playing.\n\r", NAME (vict));
      else
	{
	  sprintf (buf, "%s the level %d %s %s with %d remort%s is currently playing.\n\r", 
		   pb->name, 
		   pb->level,
		   align->name,
		   race->name,
		   pb->remorts,
		   (pb->remorts == 1 ? "" : "s"));
	}
      stt (buf, th);
    }
  if (LEVEL (th) < MAX_LEVEL)
    return;

  sprintf (buf, "%s's Email: %s  Login: %s.\n\r",
	   pb->name, pb->email, pb->site);
  stt (buf, th);
  return;
}
  

/* Goes down the list of multiplayers looking for people who cheat
   and play from the same site, but are not on the same alignment. */



void
check_for_multiplaying (void)
{
  char buf[STD_LEN];
  THING *th, *th2;
  
  for (th = thing_hash[PLAYER_VNUM % HASH_SIZE]; th != NULL; th = th->next)
    {
      if (!IS_PC (th) || LEVEL (th) >= BLD_LEVEL)
	continue;
      for (th2 = th->next; th2 != NULL; th2 = th2->next)
	{
	  if (!IS_PC (th2) || LEVEL (th2) >= BLD_LEVEL ||
	      !DIFF_ALIGN (th->align, th2->align) ||
	      th->pc->hostnum != th2->pc->hostnum)
	    continue;
	  
	  sprintf (buf, "%s and %s are on different alignments and are playing from the same site!\n", NAME (th), NAME (th2));
	  echo (buf);
	  log_it (buf);
	}
    }
  return;
}


/* This gives the number of NOSTORE items within an object (including
   itself). This is used in thing_from and thing_to when an item gets
   removed or added to a pc so that we know how many good items are
   being transferred between characters. */

int
num_nostore_items (THING *obj)
{
  int num_nostore = 0;
  THING *cont;

  if (!obj)
    return 0;

  if (IS_OBJ_SET (obj, OBJ_NOSTORE))
    num_nostore++;
  
  for (cont = obj->cont; cont; cont = cont->next_cont)
    num_nostore += num_nostore_items (cont);
  
  return num_nostore;
}

/* This finds the max remort in the game. */


void
calc_max_remort (THING *th)
{
  DIR *dir_file; /* Directory pointer. */
  struct dirent *currentry; /* Ptr to current entry. */
  THING *thg, *player, *thgn;
  char name[STD_LEN];
  int th_remorts = 0;
  /* If a thing is passed to this function, then it updates if that
     thing has more remorts than the current max, then returns,
     otherwise it loops through all people. This is used when a player
     remorts or ascends to update the status. */

  if (th)
    {
      if (IS_PC (th) && LEVEL(th) <= BLD_LEVEL)
	{
	  th_remorts = th->pc->remorts;
	  if (IS_PC1_SET (th, PC_ASCENDED))
	    th_remorts = MAX_REMORTS;
	}
      
      if (max_remorts_found_in_pbase < th_remorts)
	max_remorts_found_in_pbase = th_remorts;
      return;
    }

  /* This is the static variable that holds the remort data. It is
     accessed through max_pc_remorts() function. */
  
  max_remorts_found_in_pbase = 0;
  
  /* Now open the player directory and clean up all playerfiles. */
  
  if ((dir_file = opendir (PLR_DIR)) == NULL)
    {
      log_it ("Couldn't open player directory for finding max remort!\n\r");
      return;
    }
 
  /* Search all names in the directory. If they're online, just get their
     data, and if they're offline, read in and get data, then write out. */
     
  while ((currentry = readdir (dir_file)) != NULL)
    {
      strcpy (name, currentry->d_name);
      
      for (thg = thing_hash[PLAYER_VNUM % HASH_SIZE]; thg; thg = thgn)
	{
	  thgn = thg->next;
	  if (!str_cmp (NAME(thg), name))
	    {
	      calc_max_remort (thg);
	      break;
	    }
	}
      if (thg)
	continue;
      
      if ((player = read_playerfile (name)) != NULL)
	{
	  calc_max_remort (player);
	  free_thing (player);
	}
    }
  closedir (dir_file); 
  return;
}
  
/* This finds the remort bonus multiplier for players who are
   behind the leaders in the game. It's expected that this number will be
   divided by 2 when it's finally used in the outside world. */
int
find_remort_bonus_multiplier (THING *th)
{
  int mult;
  if (!th || !IS_PC (th) || IS_PC1_SET (th, PC_ASCENDED))
    return 1;
  mult = (max_remorts_found_in_pbase - th->pc->remorts - 1);
  if (mult < 1 || mult > MAX_REMORTS)
    mult = 1;
  return mult;
}
