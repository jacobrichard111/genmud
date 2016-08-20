#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "rumor.h"
RACE *
new_race (void)
{
  RACE *newrace;
  int i;
  newrace = (RACE *) mallok (sizeof (*newrace));
  race_count++;
  bzero (newrace, sizeof (*newrace));
  for (i = 0; i < PARTS_MAX; i++) 
    newrace->parts[i] = 1;
  for (i = 0; i < STAT_MAX; i++)
    newrace->max_stat[i] = 25;
  newrace->warrior_goal_pct = 50;
  newrace->max_spell[SPELLT_SPELL] = 35;
  newrace->max_spell[SPELLT_SKILL] = 14;
  newrace->max_spell[SPELLT_PROF] = 5;
  newrace->name = nonstr;
  newrace->ascend_from = -1;
  return newrace;
}


void
read_races (void)
{
  int currnum = 0; 
  FILE_READ_SETUP("races.dat");  
  
  FILE_READ_LOOP
    {
      FKEY_START;      
      FKEY("RACE")
	{
	  race_info[currnum] = read_race (f);
	  top_race = currnum;
	  race_info[currnum]->vnum = currnum;
	  currnum++;
	}
      FKEY("END_OF_RACES")
	break;
      else if (currnum >= RACE_MAX)
	break;
      FILE_READ_ERRCHECK("races.dat");
    }
  fclose (f);
  return;
}

void
read_aligns (void)
{
  int currnum = 0;
  FILE_READ_SETUP ("align.dat");
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("ALIGN")
	{
	  align_info[currnum] = read_race (f);
	  top_align = currnum;
	  align_info[currnum]->vnum = currnum;
	  currnum++;
	}
      FKEY("END_OF_ALIGNS")
	break;
      else if (currnum >= ALIGN_MAX)
	break;
      FILE_READ_ERRCHECK("align.dat");
    }
  fclose (f);
  return;
} 



void
write_races (void)
{
  FILE *f;
  int i;
  if ((f = wfopen ("races.dat", "w")) == NULL)
    return;
  
  for (i = 0; i < RACE_MAX; i++)
    {
      if (race_info[i])
	{
	  fprintf (f, "RACE ");
	  write_race (f, race_info[i]);
	  fprintf (f, "END_RACE\n");
	}
    }
  fprintf (f, "\nEND_OF_RACES");
  fclose (f);
  return;
}

void
write_aligns (void)
{
  FILE *f;
  int i;
  if ((f = wfopen ("align.dat", "w")) == NULL)
    return;

  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (align_info[i])
	{
	  fprintf (f, "ALIGN\n");
	  write_race (f, align_info[i]);
	  fprintf (f, "END_ALIGN\n");
	}
    }
  fprintf (f, "\nEND_OF_ALIGNS");
  fclose (f);
  return;
}


RACE *
read_race (FILE * f)
{
  RACE *newrace;
  int i;
  FILE_READ_SINGLE_SETUP;
  newrace = new_race ();

  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("Stats")
	for (i = 0; i < STAT_MAX; i++)
	  newrace->max_stat[i] = read_number (f);
      FKEY ("Spell")
	for (i = 0; i < SPELLT_MAX; i++)
	  newrace->max_spell[i] = read_number (f);
      FKEY ("Parts")
	for (i = 0; i < PARTS_MAX; i++)
	  newrace->parts[i] = read_number (f);
      FKEY ("AscendFrom")
	newrace->ascend_from = read_number (f);     
      FKEY ("RegenBonus")
	{
	  newrace->hp_regen_bonus = read_number (f);
	  newrace->mv_regen_bonus = read_number (f);
	  newrace->mana_pct_bonus = read_number (f);
	}
      FKEY ("HWT")
	{
	  newrace->height[0] = read_number (f);
	  newrace->height[1] = read_number (f);
	  newrace->weight[0] = read_number (f);
	  newrace->weight[1] = read_number (f);
	}
      FKEY ("Relic")
	{
	  newrace->relic_ascend_room = read_number(f);
	  newrace->relic_amt = read_number (f);
	  newrace->outpost_gate = read_number (f);
	}
      FKEY ("FLAG")
	{
	  FLAG *newflag = read_flag (f);
	  newflag->next = newrace->flags;
	  newrace->flags = newflag;
	}
      FKEY ("Name")
	{
	  newrace->name = new_str (read_string (f));
	}
      FKEY ("RawCurr")
	{
	  for (i = 0; i < RAW_MAX; i++)
	    newrace->raw_curr[i] = read_number (f);
	}
      FKEY ("RawWant")
	{
	  for (i = 0; i < RAW_MAX; i++)
	    newrace->raw_want[i] = read_number (f);
	}
      
      FKEY ("NatSpells")
	{
	  for (i = 0; i < NATSPELL_MAX; i++)
	    {
	      newrace->nat_spell[i] = read_number (f);
	    }
	}
      FKEY ("Goals")
	{
	  newrace->warrior_goal_pct = read_number (f);
	}
      FKEY ("HatedSociety")
	{
	  newrace->most_hated_society = read_number (f);
	}
      FKEY("END_RACE")
	break;
      FKEY("END_ALIGN")
	break;
      FILE_READ_ERRCHECK("align/race.dat-single");
    }
  return newrace;
}

void
write_race (FILE *f, RACE *race)
{
  int i;
  FLAG *flg;
  if (!race || !f)
    return;
  
  if (race->name && *race->name)
    write_string (f, "Name", race->name);
  for (flg = race->flags; flg; flg = flg->next)
    write_flag (f, flg);
  
  fprintf (f, "Stats ");
  for (i = 0; i < STAT_MAX; i++)
    fprintf (f, "%d ", race->max_stat[i]);
  fprintf (f, "\n");
  fprintf (f, "Spell ");
  for (i = 0; i < SPELLT_MAX; i++)
    fprintf (f, "%d ", race->max_spell[i]);
  fprintf (f, "\n");
  fprintf (f, "RegenBonus %d %d %d\n",
	   race->hp_regen_bonus, 
	   race->mv_regen_bonus, 
	   race->mana_pct_bonus);
  fprintf (f, "NatSpells ");
  for (i = 0; i < NATSPELL_MAX; i++)
    fprintf (f, "%d ", race->nat_spell[i]);
  fprintf (f, "\n");
  fprintf (f, "AscendFrom %d\n", race->ascend_from);
  fprintf (f, "HatedSociety %d\n", race->most_hated_society);
  fprintf (f, "Goals %d\n", race->warrior_goal_pct);
  fprintf (f, "Relic %d %d %d\n", race->relic_ascend_room, race->relic_amt,
	   race->outpost_gate);
  fprintf (f, "Parts ");
  for (i = 0; i < PARTS_MAX; i++)
  fprintf (f, "%d ", race->parts[i]);
  fprintf (f, "\n");
  fprintf (f, "HWT %d %d %d %d\n", race->height[0], race->height[1], 
	   race->weight[0], race->weight[1]);
  
  fprintf (f, "RawCurr ");
  for (i = 0; i < RAW_MAX; i++)
    fprintf (f, "%d ", race->raw_curr[i]);
  fprintf (f, "\n");
  fprintf (f, "RawWant ");
  for (i = 0; i < RAW_MAX; i++)
    fprintf (f, "%d ", race->raw_want[i]);
  fprintf (f, "\n");
  return;
}

/* This tells if a race is ascended or not. */

bool
is_ascended_race (RACE *race)
{
  int i, total = 0;
  if (!race)
    return FALSE;
#ifdef ARKANE
  {
    RACE *oldrace;
    if ((oldrace = find_race (NULL, race->ascend_from)) != NULL)
      return TRUE;
    return FALSE;
  }
#endif
  
  for (i = 0; i < STAT_MAX; i++)
    total += race->max_stat[i];
  if (total > RACE_ASCENDED_STATS)
    return TRUE;
  return FALSE;
}

/* This tells if a person can see a race listed or shown to them so
   that morts don't see ascended races too soon. */

bool 
can_see_race (THING *th, RACE *race)
{
  if (!th || !IS_PC (th) || !race)
    return FALSE;
  
  if (!is_ascended_race(race))
    return TRUE;
  
  if (LEVEL (th) >= BLD_LEVEL ||
      IS_PC1_SET (th, PC_ASCENDED) ||
      th->pc->remorts >= MAX_REMORTS/2)
    return TRUE;
  return FALSE;
}

/* Show the data a race has. */

void
show_race (THING *th, RACE *race)
{
  int i;
  char buf[STD_LEN];
  char buf2[STD_LEN];
  HELP *racehelp;
  SPELL *spl;
  RACE *oldrace;
  if (!race || !th || !IS_PC (th))
    return;

  if (!can_see_race (th, race))
    return;
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (align_info[i] == race)
	{
	  show_align (th, race);
	  return;
	}
    }
  
  
  sprintf (buf, "Race %d: %s.", race->vnum, race->name);  
  stt (buf, th);
  if ((oldrace = find_race (NULL, race->ascend_from)) != NULL &&
      can_see_race (th, oldrace))
    {
      sprintf (buf, "   (Ascended from race \x1b[1;31m%d\x1b[0;37m: \x1b[1;37m%s\x1b[0;37m) in room %d", 
	       oldrace->vnum, oldrace->name, oldrace->relic_ascend_room);
      stt (buf, th);
    }
  stt ("\n\r", th);
      
  sprintf (buf, "Members of this race are between \x1b[1;35m%d\x1b[0;37m and \x1b[1;35m%d\x1b[0;37m inches tall.\n\rThey weigh between \x1b[1;33m%d\x1b[0;37m and \x1b[1;33m%d\x1b[0;37m stones.\n\r", race->height[0], race->height[1], race->weight[0], race->weight[1]);
  stt (buf, th);
  
  sprintf (buf, "Max Stats:  ");
  for (i = 0; i < STAT_MAX; i++)
    {
      sprintf (buf2, "\x1b[1;37m%s\x1b[0;37m: \x1b[1;32m%d\x1b[0;37m  ", stat_short_name[i], race->max_stat[i]);
      buf2[0] = UC(buf2[0]);
      strcat (buf, buf2);
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Can learn  ");
  for (i = 0; i < SPELLT_MAX; i++)
    {
      sprintf (buf2, "\x1b[1;31m%d\x1b[0;37m %ss%s", race->max_spell[i],spell_types[i],  (i < SPELLT_MAX - 2 ? ", " : (i == SPELLT_MAX-2 ? " and ": ".")));
      buf2[0] = UC(buf2[0]);
      strcat (buf, buf2);
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Natural spells:  ");
  for (i = 0; i < NATSPELL_MAX; i++)
    {
      if ((spl = find_spell (NULL, race->nat_spell[i])) != NULL)
	{
	  strcat (buf, spl->name);
	  strcat (buf, "  ");
	}
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Body Parts (1 is normal): ");
  for (i = 0; i < PARTS_MAX; i++)
    {
       sprintf (buf2, "\x1b[1;36m%d \x1b[0;36m%s(s)\x1b[0;37m%s ",
race->parts[i], parts_names[i],(i < PARTS_MAX - 2 ? "," : 
				i == PARTS_MAX - 2 ? " and" : "."));
      buf2[0] = UC(buf2[0]);
      strcat (buf, buf2);
    }  
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Members of this race regen hps at %d%% of normal.\n\r",
	   (100 + race->hp_regen_bonus));
  stt (buf, th);
  sprintf (buf, "Members of this race regen moves at %d%% of normal.\n\r",
	   (100 + race->mv_regen_bonus));
  stt (buf, th);
  sprintf (buf, "Members of this race store %d%% of the normal amount of mana.\n\r",
	   (100 + race->mana_pct_bonus));
  stt (buf, th);
  
  stt (show_flags (race->flags, 0, LEVEL (th) >= BLD_LEVEL), th);
  if ((racehelp = find_help_name (th, race->name)) != NULL)
    show_help_file (th, racehelp);
  return;
}

/* Show the data an alignment has. */



void
show_align (THING *th, RACE *race)
{
  int i;
  char buf[STD_LEN];
  char buf2[STD_LEN];
  HELP *racehelp;
  SPELL *spl;


  if (!race || !th || !IS_PC (th))
    return;
  
  sprintf (buf, "Align %d: %s PowerBonus: \x1b[1;36m%d\x1b[0;37m HateSoc: %d  OutpostGate: %d\n\r", race->vnum, race->name, race->power_bonus, race->most_hated_society, race->outpost_gate);
  stt (buf, th);
  sprintf (buf, "Stat Modifiers:  ");
  for (i = 0; i < STAT_MAX; i++)
    {
      sprintf (buf2, "\x1b[1;37m%s\x1b[0;37m: \x1b[1;32m%d\x1b[0;37m  ", stat_short_name[i], race->max_stat[i]);
      buf2[0] = UC(buf2[0]);
      strcat (buf, buf2);
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Learning Modifiers: ");
  for (i = 0; i < SPELLT_MAX; i++)
    {
      sprintf (buf2, "\x1b[1;31m%d\x1b[0;37m %ss%s",  race->max_spell[i],
	       spell_types[i], (i < SPELLT_MAX - 2 ? ", " : 
				(i == SPELLT_MAX -2 ? " and " : ".")));
      buf2[0] = UC(buf2[0]);
      strcat (buf, buf2);
    }
  strcat (buf, "\n\r");
  stt (buf, th); 
  sprintf (buf, "Natural spells:  ");
  for (i = 0; i < NATSPELL_MAX; i++)
    {
      if ((spl = find_spell (NULL, race->nat_spell[i])) != NULL)
	{
	  strcat (buf, spl->name);
	  strcat (buf, "  ");
	}
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Extra Parts (0 is normal): ");
  for (i = 0; i < PARTS_MAX; i++)
    {
      sprintf (buf2, "\x1b[1;36m%d \x1b[0;36m%s(s)\x1b[0;37m%s ", race->parts[i],
	       parts_names[i], (i < PARTS_MAX - 2 ? "," : 
				i == PARTS_MAX - 2 ? " and" : "."));
      buf2[0] = UC(buf2[0]);
      strcat (buf, buf2);
    }  
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Members of this alignment regen hps at %d%% of normal.\n\r",
	   (100 + race->hp_regen_bonus));
  stt (buf, th);
  sprintf (buf, "Members of this alignment regen moves at %d%% of normal.\n\r",
	   (100 + race->mv_regen_bonus));
  stt (buf, th);
  sprintf (buf, "Members of this alignment store %d%% of the normal amount of mana.\n\r",
	   (100 + race->mana_pct_bonus));
  stt (buf, th);
  if (race->vnum > 0 && LEVEL(th) >= BLD_LEVEL) 
      {
	  sprintf (buf, "Pow: %d Pop: %d  Warr: %d NumSoc: %d\n\rRelic Rm(Amt) %d(%d) WarrGoalPct: %d\n\r", 
		   race->power, race->population, race->warriors, race->num_societies, race->relic_ascend_room, race->relic_amt, race->warrior_goal_pct);
      stt (buf, th);
      buf[0] = '\0';
      for (i = 1; i < RAW_MAX; i++)
	{
	  if (!gather_data[i].command[0])
	    continue;
	  sprintf (buf2, "%s: %d(%d)  ", gather_data[i].raw_name, 
		   race->raw_curr[i],
		   race->raw_want[i]);
	  strcat (buf, buf2);
	  if (i == 4)
	    strcat (buf, "\n\r");
	}
      strcat (buf, "\n\r");
      stt (buf, th);
    }
  stt (show_flags (race->flags, 0, LEVEL (th) >= BLD_LEVEL), th);
  if ((racehelp = find_help_name (th, race->name)) != NULL)
    show_help_file (th, racehelp);
  return;
}


RACE *
find_race (char *name, int num)
{
  int i;
  if (num >= 0 && num < RACE_MAX && race_info[num])
    return race_info[num];
  if (!name || !*name)
    return NULL;
  if (!str_cmp (name, "zzg")) /* Don't call races zzg :P */
    return race_info[0];
  for (i = 0; i < RACE_MAX; i++)
    if (race_info[i] && !str_prefix (name, race_info[i]->name))
      return race_info[i];
  return NULL;
}

RACE *
find_align(char *name, int num)
{
  int i;
  if (num >=0 && num < ALIGN_MAX && align_info[num])
    return align_info[num];
  if (!name || !*name)
    return NULL;
  if (!str_cmp (name, "zzg")) /* Don't call aligns zzg */
    return align_info[0];
  for (i = 0; i < ALIGN_MAX; i++)
    if (align_info[i] && !str_prefix (name, align_info[i]->name))
      return align_info[i];
  return NULL;
}
  
void
show_short_race (THING *th, RACE *race)
{
  char buf[STD_LEN];
  int i;
  if (!race || !th)
    return;
  
  sprintf (buf, "\x1b[1;34m[\x1b[1;37m%2d\x1b[1;34m]\x1b[1;37m%-15s ", race->vnum, race->name);
  strcat (buf, "\x1b[1;31m");
  for (i = 0; i < STAT_MAX; i++)
    sprintf (buf + strlen (buf), "%2d ", race->max_stat[i]);
  strcat (buf, "\x1b[1;34m ");
  for (i = 0; i < SPELLT_MAX; i++)
    {
      sprintf (buf + strlen (buf), "%2d", race->max_spell[i]);
      if (i < SPELLT_MAX - 1)
	strcat (buf, "/");
      else
	strcat (buf, "\x1b[0;37m  ");
    }
  if (race->flags)
    strcat (buf, " \x1b[1;37mYes\x1b[0;37m");
  else
    strcat (buf, " \x1b[1;30mNo\x1b[0;37m");
  stt (buf, th);
  stt ("\n\r", th);	
  return;
}
  

void
do_align (THING *th, char *arg)
{
  char arg1[STD_LEN];
  RACE *align;
  int i;
  if (!IS_PC (th))
    return;
  
  if (arg[0] == '\0' || !str_cmp (arg, "list"))
    {
      stt ("    Name            St In Wi De Co Lu Ch  Sp/Sk/Pr/Tr/Po   Special\n\r", th);
      for (i = 0; i < ALIGN_MAX; i++)
	{
	  if (align_info[i])
	    show_short_race (th, align_info[i]);
	}
      return;
    }
  arg = f_word (arg, arg1);
  if ((align = find_align (arg1, (isdigit (*arg1) ? atoi(arg1) : -1))) != NULL)
    {
      show_align (th, align);
      return;
    }
  if (LEVEL (th) < MAX_LEVEL)
    {
      stt ("Align list or Align <number> or Align <Name>\n\r", th);
      return;
    }
  if (!str_cmp (arg1, "edit"))
    {
      if (IS_SET (th->fd->flags, FD_EDIT_PC))
	{
	  stt ("You cannot edit anything else while you are editing a PC.\n\r", th);
	  return;
	}
      if ((align = find_align (arg, (isdigit (*arg) ? atoi(arg) : -1))) == NULL)
	{
	  stt ("Align list, number, name, or Align create, or align edit <number/name>\n\r", th);
	  return;
	}
      if (th->fd)
	{
	  th->fd->connected = CON_RACEDITING;
	  th->pc->editing = align;
	  show_race (th, align);
	  return;
	}
    }
  if (!str_cmp (arg1, "create"))
    {
      int i;
      for (i = 0; i < ALIGN_MAX; i++)
	if (!align_info[i])
	  break;
      
      if (i >= ALIGN_MAX)
	{
	  stt ("Too many aligns. Sorry.\n\r", th);
	  return;
	} 
      align = new_race ();
      align_info[i] = align;
      align_info[i]->vnum = i;
      
      for (i = 0; i < STAT_MAX; i++)
	align->max_stat[i] = 0;
      for (i = 0; i < PARTS_MAX; i++)
	align->parts[i] = 0;
      for (i = 0; i < SPELLT_MAX; i++)
	align->max_spell[i] = 0;
      align->power = 0;
      align->population = 0;
      align->num_societies = 0;
      align->warriors = 0;
      for (i = 0; i < RAW_MAX; i++)
	{
	  align->raw_curr[i] = 0;
	  align->raw_want[i] = 0;
	}
      if (th->fd)
	th->fd->connected = CON_RACEDITING;
      th->pc->editing = align;
      show_race (th, align);
      write_aligns ();
      return;
    }
  
	  
  stt ("Align create, edit <name/number>, list, <name/number>\n\r", th);
  return;

  
		


}
 

void
do_race (THING *th, char *arg)
{
  char arg1[STD_LEN];
  RACE *race;
  int i;
  
  if (!th || !IS_PC (th))
    return;

  
  
  if (arg[0] == '\0' || !str_cmp (arg, "list"))
    {
      stt ("    Name            St In Wi De Co Lu Ch  Sp Sk Pr Po Tr   Special\n\r", th);
      for (i = 0; i < RACE_MAX; i++)
	{
	  if (race_info[i] &&
	      can_see_race (th, race_info[i]))
	    show_short_race (th, race_info[i]);
	}
      return;
    }
  arg = f_word (arg, arg1);

  if (!str_cmp (arg1, "change"))
    {
      do_racechange (th, arg);
      return;
    }
    

  if ((race = find_race (arg1, (isdigit (*arg1) ? atoi(arg1) : -1))) != NULL &&
      can_see_race (th, race))
    {
      show_race (th, race);
      return;
    }
  if (LEVEL (th) < MAX_LEVEL)
    {
      stt ("Race list or Race <number> or Race <Name>\n\r", th);
      return;
    }
  if (!str_cmp (arg1, "edit"))
    {
      if (IS_SET (th->fd->flags, FD_EDIT_PC))
	{
	  stt ("You cannot edit anything else while you are editing a PC.\n\r", th);
	  return;
	}
      if ((race = find_race (arg, (isdigit (*arg) ? atoi (arg) : -1))) == NULL)
	{
	  stt ("Race list, number, name, or Race create, or race edit <number/name>\n\r", th);
	  return;
	}
      if (th->fd)
	{
	  th->fd->connected = CON_RACEDITING;
	  th->pc->editing = race;
	  show_race (th, race);
	  return;
	}
      return;
    }
  if (!str_cmp (arg1, "create"))
    {
      int i;
      for (i = 0; i < RACE_MAX; i++)
	{
	  if (!race_info[i])
	    break;
	}
      if (i == RACE_MAX)
	{
	  stt ("Max number of races reached, sorry.\n\r", th);
	  return;
	}	  
      race = new_race ();
      race_info[i] = race;
      race_info[i]->vnum = i;
      if (th->fd)
	th->fd->connected = CON_RACEDITING;
      th->pc->editing = race;
      show_race (th, race);
      write_races ();
      return;
    }
  
	  
  stt ("Race create, edit <name/number>, list, <name/number>\n\r", th);
  return;


}

/* This lets a player change race. The price is a remort, or if you're
   a 0 remort character, then 0 remorts. You lose your highest stats
   and you must have guild and implant levels low enough to accept
   the loss in status. */

void
do_racechange (THING *th, char *arg)
{
  RACE *race, *oldrace;
  int i, max_stat_val, max_stat_num, stat_points_left = STATS_PER_REMORT;
  int stat_difference = 0;
  char arg1[STD_LEN];
  char buf[STD_LEN];
  bool can_change_race = TRUE;
  if (!th || !IS_PC (th))
    return;

  if (LEVEL (th) > MORT_LEVEL)
    {
      stt ("Uhh no.\n\r", th);
      return;
    }

  arg = f_word (arg, arg1);
  
  if (!arg || !*arg)
    {
      stt ("Racechange yes <racename>\n\r", th);
      return;
    }
  
  if ((race = find_race (arg, (isdigit (*arg1) ? atoi (arg1) : -1))) == NULL)
    {
      stt ("Racechange yes <racename>\n\r", th);
      return;
    }

  


  if (race->vnum == th->pc->race)
    {
      stt ("You can't racechange to the same race.\n\r", th);
      return;
    }
  
  /* Check to make sure the race is "close enough"...i.e. no changing
     from big burly troll to wimpy sprite in one go.. (Although
     multiple changes are possible). */
  
  if ((oldrace = find_race (NULL, th->pc->race)) == NULL)
    {
      stt ("Your current race doesn't seem to exist.\n\r", th);
      return;
    }
  
  /* Even though this shouldn't really be possible...I want to make
     sure that people don't racechange from a regular race to an
     ascended race. Also, by the time someone ascends, they should
     know what they want, so it isn't as much of a big deal to force
     them to stay in their old race. */
  if (is_ascended_race (race) || is_ascended_race (oldrace))
    {
      stt ("You can't racechange to or from an ascended race. It is assumed that by the time you get to ascended races, you know what kind of character you want.\n\r", th);
      return;
    }
  for (i = 0; i < STAT_MAX; i++)
    {
      stat_difference += ABS(race->max_stat[i]-oldrace->max_stat[i]);
    }
  
  if (stat_difference > 15)
    {
      sprintf (buf, "You can only racechange to a race where the stat differences between the races are no more than 15 apart. The %s and %s races are %d points apart.\n\r",
	       race->name, oldrace->name, stat_difference);
      stt (buf, th);
      return;
    }
  
  
  /* Check to make sure that we can change based on our guilds and 
     remorts. */
  
  if (th->pc->remorts > 0)
    {
      th->pc->remorts--;
      if (total_guild_points (th) < total_guilds (th))
	{
	  sprintf (buf, "You have used up \x1b[1;31m%d\x1b[0;37m guild points and you can only have \x1b[1;32m%d\x1b[0;37m guild points with \x1b[1;37m%d\x1b[0;37m remorts. Drop some guilds before trying to change your race.\n\r", 
		   total_guilds(th), total_guild_points (th), th->pc->remorts);
	  can_change_race = FALSE;
	  stt (buf, th);
	}
      if (total_implant_points (th) < total_implants (th))
	{
	  sprintf (buf, "You have used up \x1b[1;31m%d\x1b[0;37m implant points and you can only have \x1b[1;32m%d\x1b[0;37m implant points with \x1b[1;37m%d\x1b[0;37m remorts. Drop some implants before trying to change your race.\n\r", 
		   total_implants(th), total_implant_points (th), th->pc->remorts);
	  can_change_race = FALSE;
	  stt (buf, th);
	}
      th->pc->remorts++;
      if (!can_change_race)
	return;
    }
  
  /* Modify stats based on race...*/
  for (i = 0; i < STAT_MAX; i++)
    {
      th->pc->stat[i] += (race->max_stat[i]-oldrace->max_stat[i]);
    }
  
  /* Now remove the remort stats from the top stats that the
     character has. */

  if (th->pc->remorts > 0)
    {
      while (stat_points_left > 0)
	{
	  max_stat_val = 0;
	  max_stat_num = -1;
	  
	  for (i = 0; i < STAT_MAX; i++)
	    {
	      if (th->pc->stat[i] > max_stat_val)
		{
		  max_stat_val = th->pc->stat[i];
		  max_stat_num = i;
		}
	    }
	  if (max_stat_num >= 0 && max_stat_num < STAT_MAX)
	    {
	      th->pc->stat[max_stat_num]--;
	    }
	  stat_points_left--;
	}
      th->pc->remorts--;  
    }
  
  th->pc->race = race->vnum;
  remove_all_affects (th, TRUE);
  remort_clear_stats (th);
  stt ("Ok, your race has been changed.\n\r", th);
  fix_pc (th);
  return;
}

void
racedit (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  int i, value;
  RACE *race;
  char *oldarg = arg;
  SPELL *spl;
  
  
  if (!th)
    return;
  if (!IS_PC (th) || !th->pc->editing || !th->fd)
    {
      if (th->fd)
	{
	  th->fd->connected = CON_ONLINE;
	}
      if (IS_PC (th) && th->pc->editing)
	{
	  th->pc->editing = NULL;
	}
      return;
    }
  race = (RACE *) th->pc->editing;
  if (!*arg)
    {
      show_race (th, race);
      return;
    }
  arg = f_word (arg, arg1);
  
  
  if (!str_cmp (arg1, "flag") || !str_cmp (arg1, "flags"))
    {
      edit_flags (th, &race->flags, arg);
      show_race (th, race);
      return;
    }
  else if (!str_prefix ("goals", arg1) ||
	   !str_prefix ("warriors", arg1))
    {
      if (atoi(arg) >= WARRIOR_PERCENT_MIN && 
	  atoi (arg) <= WARRIOR_PERCENT_MAX)
	{
	  race->warrior_goal_pct = atoi(arg);
	  show_race (th, race);
	  return;
	}
      sprintf (buf, "warrior <num> where <num> is between %d and %d\n\r",
	       WARRIOR_PERCENT_MIN, WARRIOR_PERCENT_MAX);
      stt (buf, th);
      return;
    }
  else if (!str_prefix ("part", arg1))
    {
      arg = f_word (arg, arg1);
      for (i = 0; i < PARTS_MAX; i++)
	{
	  if (!str_prefix (arg1, parts_names[i]))
	    {
	      race->parts[i] = atoi (arg);
	      stt ("Parts set.\n\r", th);
	      show_race (th, race);
	      return;
	    }
	}
      stt ("Part <part name> <number>\n\r", th);
      return;
    }
  else if (!str_cmp (arg1, "gate") ||
	   !str_cmp (arg1, "outpost") ||
	   !str_cmp (arg1, "outpost_gate"))
    {
      race->outpost_gate = atoi (arg);
      stt ("Outpost gate changed.\n\r", th);
      show_race (th, race);
      return;
    }
  else if (!str_prefix (arg1, "ascend_from"))
    {
      race->ascend_from = atoi (arg);
      stt ("Ascended from changed.\n\r", th);
      show_race (th, race);
      return;
    }
  else if (!str_prefix (arg1, "relic_room") ||
	   !str_prefix (arg1, "ascend_room"))
    {
      race->relic_ascend_room = atoi (arg);
      stt ("Relic room set.\n\r", th);
      show_race (th, race);
      return;
    }
  else if (!str_prefix (arg1, "stat"))
    {
      arg = f_word (arg, arg1);
      for (i = 0; i < STAT_MAX; i++)
	{
	  if (!str_prefix (arg1, stat_name[i]))
	    {
	      race->max_stat[i] = atoi (arg);
	      stt ("Max stat set.\n\r", th);
	      show_race (th, race);
	      return;
	    }
	}
      stt ("Stat <stat name> <number>\n\r", th);
      return;
    }
  else if (!str_cmp ("hp_regen", arg1))
    {
      race->hp_regen_bonus = atoi (arg);
      stt ("Hp regen bonus percent set.\n\r", th);
      show_race (th, race);
      return;
    } 
  else if (!str_cmp ("mv_regen", arg1))
    {
      race->mv_regen_bonus = atoi (arg);
      stt ("Hp regen bonus percent set.\n\r", th);
      show_race (th, race);
      return;
    } 
  else if (!str_cmp ("mana_bonus", arg1))
    {
      race->mana_pct_bonus = atoi (arg);
      stt ("Mana pct bonus set.\n\r", th);
      show_race (th, race);
      return;
    }
  else if (!str_prefix ("nat", arg1))
    {
      arg = f_word (arg, arg1);
      if (!is_number (arg1) || atoi (arg1) < 0 || atoi (arg1) >= NATSPELL_MAX)
	{
	  stt ("Nat <number> <spellname/number>\n\r", th);
	  return;
	}
      if (!str_cmp (arg, "none") || !str_cmp ("0", arg))
	{
	  race->nat_spell[atoi(arg1)] = 0;
	  stt ("Ok, natural spell cleared.\n\r", th);
	  show_race (th, race);
	  return;
	}
      if ((spl = find_spell (arg, atoi (arg))) == NULL)
	{
	  stt ("That spell doesn't exist.\n\r", th);
	  return;
	}
      race->nat_spell[atoi(arg1)] = spl->vnum;
      stt ("Ok, natural spell set.\n\r", th);
      show_race (th, race);
      return;
    }
  else if (!str_cmp (arg1, "name"))
    {
      race->name = new_str (arg);
      stt ("Name set.\n\r", th);
      show_race (th, race);
      return;
    }
  else if (!str_cmp (arg1, "learn") || !str_cmp (arg1, "spell") ||
	   !str_cmp (arg1, "skill"))
    {
      arg = f_word (arg, arg1);
      for (i = 0; i < SPELLT_MAX; i++)
	{
	  if (!str_prefix (arg1, spell_types[i]))
	    {
	      race->max_spell[i] = atoi (arg);
	      stt ("Max spell set.\n\r", th);
	      show_race (th, race);
	      return;
	    }
	}
      stt ("Spell <spell type> <number>\n\r", th);
      return;
    }
  else if (!str_cmp (arg1, "raw"))
    {      
      int type;
      arg = f_word (arg, arg1);
      if (!str_cmp (arg1, "all"))
	{
	  int i;
	  value = atoi (arg);
	  for (i = 0; i < RAW_MAX; i++)
	    race->raw_curr[i] = value;
	  show_align (th, race);
	  sprintf (buf, "All raw material amounts set to %d.\n\r", value);
	  stt (buf, th);
	  return;
	}
      
      type = atoi (arg1);
      if (type <= RAW_NONE || type >= RAW_MAX)
	{
	  for (type = RAW_NONE; type < RAW_MAX; type++)
	    {
	      if (!str_cmp (gather_data[type].raw_name, arg1))
		break;
	    }
	  if (type <= RAW_NONE || type >= RAW_MAX)
	    {
	      stt ("Raw <raw type> <amount>\n\r", th);
	      return;
	    }
	}
      value = atoi (arg);
      race->raw_curr[type] = value;
      stt ("Raw material value set.\n\r", th);
      show_align (th, race);
      return;
    }		
  else if (!str_cmp (arg1, "height"))
    {
      arg = f_word (arg, arg1);
      race->height[0] = atoi (arg1);
      race->height[1] = atoi (arg);
      stt ("Height min and max set.\n\r", th);
      show_race (th, race);
      return;
    }
  else if (!str_cmp (arg1, "weight"))
    {
      arg = f_word (arg, arg1);
      race->weight[0] = atoi (arg1);
      race->weight[1] = atoi (arg);
      stt ("Weight min and max set.\n\r", th);
      show_race (th, race);
      return;
    }
  else if (!str_cmp (arg1, "done"))
    {
      if (th->fd)
	{
	  th->fd->connected = CON_ONLINE;
	}
      if (th->pc && th->pc->editing)
	th->pc->editing = NULL;
      stt ("Ok, all done with race/align editing.\n\r", th);
      write_races ();
      write_aligns ();
      return;
    }
  
  interpret (th, oldarg);
  return;
}

/* This loads the alliance data into the game. */ 

void
read_alliances (void)
{
  FILE *f;
  int i;
  
  if ((f = wfopen ("alliances.dat", "r")) == NULL)
    {
      for (i = 0; i < ALIGN_MAX; i++)
	alliance[i] = (1 << i);
      return;
    }
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      alliance[i] = read_number (f);
    }
  fclose (f);
  return;
}

/* This writes out the alliances. */
  
void
write_alliances (void)
{
  FILE *f;
  int i;

  if ((f = wfopen ("alliances.dat", "w")) == NULL)
    return;
  
  for (i = 0; i < ALIGN_MAX; i++)
    fprintf (f, "%d ", alliance[i]);
  fclose (f);
  return;
}

/* This lets players view the alliances and lets admins set
   and remove alliances. */

void 
do_alliances (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  RACE *align1 = NULL, *align2 = NULL;
  

  if (LEVEL (th) == MAX_LEVEL && *arg)
    {
      arg = f_word (arg, arg1);

      /* Check if there are two arguments, and try to figure out
	 what they mean. */
      
      if (*arg && arg1[0])
	{
	  if (isdigit (arg1[0]))
	    align1 = find_align (NULL, atoi(arg1));
	  else
	    align1 = find_align (arg1, -1);
	  if (isdigit (*arg))
	    align2 = find_align (NULL, atoi(arg));
	  else
	    align2 = find_align (arg, -1);
	    
	}
      
      /* Bail out if we couldn't find the proper alignments. */

      if (!align1 || !align2 || align1 == align2)
	{
	  stt ("Syntax: ally <align1> <align2>\n\r", th);
	  return;
	}
      
      alliance[align1->vnum] ^= (1 << align2->vnum);
      alliance[align2->vnum] ^= (1 << align1->vnum);
      
      sprintf (buf, "%s alliance between %s and %s has been %s.\n\r",
	       (IS_SET (alliance[align1->vnum], (1 << align2->vnum)) ?
		"An" : "The"), 
	       align1->name, align2->name, 
	       (IS_SET (alliance[align1->vnum], (1 << align2->vnum)) ?
		"created" : "destroyed"));
      echo (buf);
      write_alliances();
    }
  
  show_alliances (th);
  return;
}



/* This shows a list of the alliances in table form. */

void
show_alliances (THING *th)
{
  int i, j;
  char buf[STD_LEN];
  char aname[STD_LEN];
  
  stt ("These are the current alliances:\n\n\r", th);
  sprintf (buf, "Aligns           ");
  for (j = 0; j < ALIGN_MAX; j++)
    {
      if (align_info[j])
	{
	  sprintf (aname, "%-10s ", align_info[j]->name);
	  aname[10] = ' ';
	  aname[11] = '\0';
	  strcat (buf, aname);
	}
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (!align_info[i])
	continue;
      
      sprintf (buf, "\x1b[0;37m%d. %-10s ", i, align_info[i]->name);
      buf[20] = ' ';
      buf[21] = '\0';
      
      for (j = 0; j < ALIGN_MAX; j++)
	{
	  if (!align_info[j])
	    continue;
	  
	  if (IS_SET (alliance[i], (1 << j)))
	    strcat (buf, "\x1b[1;37m     Y     ");
	  else
	    strcat (buf, "\x1b[1;31m     N     ");
	}
      strcat (buf, "\n\r");
      stt (buf, th);
    }
  return;
}

/* This updates alignment power bonuses. It isn't related to the
   society/rts code, but instead it adds these bonuses for alignments
   that aren't winning in the game. */

void update_alignment_pkill_bonuses(void)
{
  int power[ALIGN_MAX]; /* Calc the powers of each align. */
  int pktype, pknum;
  int al, al2;
  PKDATA *pkdat;
  RACE *align;
  int num_align_sides = 0;

  int total_power = 0;
  int max_power = -1;
  int max_power_align = 0;

  /* Init the power array. */
  
  for (al = 0; al < ALIGN_MAX; al++)
    power[al] = 0;

  /* Loop through each type of pk data...*/

  for (pktype = 0; pktype < PK_GOOD_MAX; pktype++)
    {
      
      /* Loop through each list...*/
      
      for (pknum = 0; pknum < PK_LISTSIZE; pknum++)
	{
	  
	  /* Add PK_LISTSIZE-pknum to the appropriate power list. */
	  pkdat = &pkd[pktype][pknum];
	  
	  if (pkdat->name[0] &&
	      pkdat->align > 0 &&
	      pkdat->align < ALIGN_MAX &&
	      align_info[pkdat->align] != NULL)
	    {
	      power[pkdat->align] += (PK_LISTSIZE + 1 - pknum);
	    }
	}
    }
  
  /* Now we have the powers, we will loop through the aligns and
     see who's allied. */
  
  power[0] = 0;
  for (al = 1; al < ALIGN_MAX; al++)
    {
      if ((align = align_info[al]) == NULL)
	continue;
      
      for (al2 = al + 1; al2 < ALIGN_MAX; al2++)
	{
	  if (!DIFF_ALIGN (al, al2))
	    {
	      power[al] += power[al2];
	      power[al2] = 0;
	    }
	}
    }
  
  /* Now see if there are at least 2 align sides or not. */

  for (al = 1; al < ALIGN_MAX; al++)
    {
      if (power[al] > 0)
	num_align_sides++;
      total_power += power[al];
      if (power[al] > max_power)
	{
	  max_power = power[al];
	  max_power_align = al;
	}
    }
  
  /* If there aren't at least two of them, then bail out. */
  
  if (num_align_sides < 2)
    {
      for (al = 0; al < ALIGN_MAX; al++)
	if (align_info[al])
	  align_info[al]->power_bonus = 0;
      return;
    }
  
  /* So we have at least 2 different sides. Give a bonus to each
     side that's smaller than the max side. The powers get 100 added
     to them so that it smooths out the bonuses for small numbers
     of values in the pk data. Then copy the over all power for all
     of the allies into each ally's power slot. */

  max_power += 100;
  for (al = 1; al < ALIGN_MAX; al++)
    {
      if (power[al] > 0)
	power[al] += 100;	  
    }
  
  for (al = 1; al < ALIGN_MAX; al++)
    {
      for (al2 = al + 1; al2 < ALIGN_MAX; al2++)
	{
	  if (!DIFF_ALIGN (al, al2))
	    power[al2] = power[al];
	}
    }
  
  for (al = 1; al < ALIGN_MAX; al++)
    {      
      if (align_info[al] == NULL)
	continue;
      
      align_info[al]->power_bonus = 
	(max_power-power[al])*ALIGN_POWER_BONUS/max_power;                   
    }
  return;
  



  

}
/* Alignments are super-societies. They interact with societies
   and "tax" societies for raw materials, and distribute excess
   raw materials to other societies as needed (and as possible). 
   They also get to "cheat" and give extra resources to
   societies if the alignment is losing too badly. Alignments that
   get checked are those > 0 only (neutrals don't have this
   organization). Losing badly means their power or population
   is too small compared to the rest of the societies, or they
   don't have enough societies as a part of them. */


void
update_alignments (void)
{
 
  
  
  /* Update the resources that the alignments have and need. */
  
  update_alignment_resources();
  
  
  
  /* Now see if the alignment needs to assist a society in trouble. */
  
  help_societies_under_siege();
  
  /* This lets the alignments make broad decisions about military vs
     economic resource allocation. */

  update_alignment_goals();
  


  write_aligns();
  return;
}

  
/* This updates the resources that the alignments have and those
   that they need. It also spreads the resources around to societies
   that need them, and it also cheats for alignments that are losing
   by giving them access to more resources. */

void
update_alignment_resources (void)
{
  THING *relic, *relic_room;
  RACE *align, *align2;
  SOCIETY *society;
  int align_num, align_num2, i, num_aligns = 0;
  int raw_available; /* Amount of raw materials available for use by
			the societies who need it. */
  
  int raw_used; /* Actual amount given to the societies. */
  
  int total_population = 0;   /* Total population of all alignments. */
  int total_power = 0;        /* Total power of all alignments. */
  int total_warriors = 0;     /* Total number of warriors in all alignments. */
  
  int total_raw_wanted;     /* Amount of raw materials wanted by the
			       align after the update. */
  
  int total_raw_surplus;  /* Total amount of raw materials to be
			       traded to help societies. */

  /* Number of allies for this align, */
  
  int ally_count;

  /* Used to pool resources among allied alignments. */

  int resource_pool[RAW_MAX];
  
  /* Has this alignment shared resources yet. */

  bool shared_resources[ALIGN_MAX];
  /* Loop through alignments and distribute resources among allied
     alignments. */

  for (i = 0; i < ALIGN_MAX; i++)
    shared_resources[i] = FALSE;
  
  
  /* Loop through all aligns, only working with those that have
     not been updated yet. */
   for (align_num = 1; align_num < ALIGN_MAX; align_num++)
     {
      if ((align = align_info[align_num]) == NULL)
	continue;
      
      if (shared_resources[align_num])
	continue;
      

      for (i = 0; i < RAW_MAX; i++)
	resource_pool[i] = 0;
      ally_count = 0;
      
      /* Loop through all alignments and see how many are allied
	 with this one. For each of those, add their resources to
	 the pool. */

      for (align_num2 = 1; align_num2 < ALIGN_MAX; align_num2++)
	{
	  if ((align2 = align_info[align_num2]) == NULL)
	    continue;
	  
	  if (DIFF_ALIGN (align->vnum, align2->vnum))
	    continue;

	  shared_resources[align_num2] = TRUE;
	  
	  for (i = 0; i < RAW_MAX; i++)
	    resource_pool[i] += align2->raw_curr[i];
	  
	  ally_count++;
	}
      
      /* Don't do anything if there are no allies (but this shouldn't
	 happen since align is allied with itself (hopefully!)) */
      
      if (ally_count < 1)
	continue;
      
      for (align_num2 = 1; align_num2 < ALIGN_MAX; align_num2++)
	{
	  if ((align2 = align_info[align_num2]) == NULL)
	    continue;
	  
	  if (DIFF_ALIGN (align->vnum, align2->vnum))
	    continue;

	  
	  for (i = 0; i < RAW_MAX; i++)
	    {
	      align2->raw_curr[i] = resource_pool[i]/ally_count;
	      /* Get the fractional resources too..*/
	      if (align2 == align)
		align2->raw_curr[i] += resource_pool[i] % ally_count;
	      
	    }
	}
      
    }
  


  /* Loop through alignments and get their total pops and power
     from the societies they "represent". Then collect "taxes"
     on raw materials and dole them out to those who need them. */
  
  for (align_num = 0; align_num < ALIGN_MAX; align_num++)
    {
      if ((align = align_info[align_num]) == NULL)
	continue;
      if (align_num > 0)
	{
	  num_aligns++;
	  
	  /* Update relics in the relic room. */
	  
	  align->relic_amt = 0;
	  if ((relic_room = find_thing_num (align->relic_ascend_room)) != NULL &&
	      IS_ROOM (relic_room))
	    {
	      for (relic = relic_room->cont; relic; relic = relic->next_cont)
		if (relic->vnum == RELIC_VNUM)
		  align->relic_amt++;
	    }
	  
	}
      align->population = 0;
      align->power = 0;
      align->num_societies = 0;
      align->warriors = 0;

      /* Clear raws needed. */
      
      
      
      for (i = 0; i < RAW_MAX; i++)
	align->raw_want[i] = 0;
      
      for (society = society_list; society; society = society->next)
	{
	  if (society->align != align->vnum)
	    continue;
	  
	  align->num_societies++;
	  align->population += society->population;
	  align->power += society->power;
	  align->warriors += society->warriors;
	  
	  if (align_num == 0)
	    continue;
	  
	  for (i = 0; i < RAW_MAX; i++)
	    { 
	      /* Find raw materials needed. If the society needs something,
	       give it 5x what it asks for so it will have
	       enough for a while. */
	  
	      if (society->raw_want[i] > 0)
		align->raw_want[i] += society->raw_want[i] * RAW_WANT_MULT;
	      
	      /* Collect "taxes" if raw materials of a certain type
		 are needed. */
	      
	      if (align->raw_curr[i] < RAW_ALIGN_RESERVE)
		{
		  if (society->raw_curr[i] > RAW_TAX_AMOUNT)
		    {
		      align->raw_curr[i] += society->raw_curr[i] - RAW_TAX_AMOUNT;
		      society->raw_curr[i] = RAW_TAX_AMOUNT;
		    }
		  /* Tax 20pct of raws above 1/4 the total tax amount. */
		  /* In this case, 20pct of everything >=1/4 tax and < tax. */
		  
		  if (society->raw_curr[i] >= RAW_TAX_AMOUNT/4)
		    {
		      align->raw_curr[i] += (society->raw_curr[i]-RAW_TAX_AMOUNT/4)/5;
		      society->raw_curr[i] -= (society->raw_curr[i]-RAW_TAX_AMOUNT/4)/5;
		    }	     
		  else /* Take 2 pct of reserves if the society doesn't
			  have much to give. */
		    {		      
		      align->raw_curr[i] += society->raw_curr[i]/50;
		      society->raw_curr[i] -= society->raw_curr[i]/50;
		    }
		}
	    }	  
	}
      
      /* Now the power and population for this alignment are known,
	 so add these numbers to the total_power and total_population. */
      
      total_power += align->power;
      total_population += align->population;
      total_warriors += align->warriors;
      
      /* Align 0 doesn't do the tax thing. */
      if (align_num == 0)
	continue;
      
      /* Now the taxes are collected and the needs are found. See if
	 we have enough resources to dole out to everyone who
	 needs it. If noone needs it, just store it. */
      
      for (i = 0; i < RAW_MAX; i++)
	{
	  if (align->raw_want[i] == 0 || align->raw_curr[i] == 0)
	    continue;
	  
	  /* The amount available to dole out is the min of the 
	     current amount and the needed amount...so we may end
	     up with less available than we had hoped. */
	  
	  raw_available = align->raw_curr[i];
	  raw_used = 0;
	  
	  for (society = society_list; society != NULL; society = society->next)
	    {
	      /* Find out the total amount of raw materials we need. */
	      
	      if (society->raw_want[i] > 0 &&
		  !IS_SET (society->society_flags, SOCIETY_NUKE))
		{
		  raw_used += RAW_WANT_MULT * society->raw_want[i];
		}
	    }

	  if (raw_used < 1)
	    continue;
	  
	  /* See if we have enough available. If we have enough available
	     make the available amount the "raw_used" amount. */
	  
	  if (raw_used <= raw_available)
	    raw_available = raw_used;
	      
	  
	  for (society = society_list; society != NULL; society = society->next)
	    {
	      /* If the society needs raw materials, take the amount
		 they need and multiply by the amt available/amt wanted. */
	      
	      if (society->raw_want[i] > 0 &&
		  !IS_SET (society->society_flags, SOCIETY_NUKE))
		{
		  society->raw_curr[i] += 
		    RAW_WANT_MULT*society->raw_want[i]*raw_available/raw_used;
		  raw_used += RAW_WANT_MULT*society->raw_want[i]*raw_available/raw_used;	  
		}
	    }
	  
	  
	  /* Subtract the raw materials we used up from the alignment
	     stores. */
	  
	  if (raw_used > 0)
	    {
	      align->raw_curr[i] -= raw_available;
	      align->raw_want[i] = 0;
	    }
	}

      /* Now we shuffle raw materials around a little bit (at a penalty)
	 to reflect the fact that raw materials can be traded and
	 so are somewhat fungible. */
      
      total_raw_wanted = 0;
      total_raw_surplus = 0;
      
      for (i = 1; i < RAW_MAX; i++)
	{
	  if (align->raw_curr[i] > RAW_ALIGN_RESERVE/2)
	    total_raw_surplus += align->raw_curr[i] - RAW_ALIGN_RESERVE/2;
	  else if (align->raw_want[i] > 0)
	    total_raw_wanted += align->raw_want[i];
	}
      
      if (total_raw_wanted > 0 && total_raw_surplus > 0)
	{
	  /* Penalty for trading raw materials. Maybe let societies
	     research this to make it suck less. */
	  
	  total_raw_surplus /= 5;
	  for (i = 1; i < RAW_MAX; i++)
	    {
	      /* If we have a lot of the raw material, reduce the 
		 reserve. */
	      if (align->raw_curr[i] > RAW_ALIGN_RESERVE/2)
		align->raw_curr[i] = RAW_ALIGN_RESERVE/2;

	      /* If we need the raw material, increase it proportional to
		 how badly we need it. */
	      else if (align->raw_want[i] > 0)
		{
		  align->raw_curr[i] += 
		    (total_raw_surplus * align->raw_want[i])/total_raw_wanted;
		}
	    }
	}
    }
  
  /* Now all of the alignments have been updated, now compare
     them to each other to see how they're faring. */
  
  /* Generally, if the alignment is falling behind (on average)
     in terms of power and population, it gets some extra help
     in the form of extra resources. */      
  
  /* First do a little sanity checking vs "small" worlds.  */  
  
  if (total_population >= 300 && total_power >= 10000 && num_aligns >= 2 &&
      total_warriors > 150)
    {
      
      /* One thing to remember here is that let's say there are two
	 alignments, and one has 1/3 of what it should have. That
	 means it's at 1/3*1/2 of the total power, meaning the
	 actual power ratio is 5:1. That's why we check this when
	 things get to 1/3 and not some smaller amount below
	 where the society should be. */
      
      /* At 1/6 the total power, all societies in that align get
	 extra raws given directly to them, and they get ALL goals
	 set as a desperation measure to keep up with the rest of
	 the world. */
      
      for (align_num = 0; align_num < ALIGN_MAX; align_num++)
	{
	  if ((align = align_info[align_num]) == NULL)
	    continue;
	  
	  align->pop_pct = (100*align->population) /
	    (total_population);
	  align->power_pct = (100*align->power)/
	    (total_power);
	  align->warrior_pct = (100*align->warriors)/
	    (total_warriors);
	  align->ave_pct = (align->pop_pct + align->power_pct + align->warrior_pct)/3;
	  
	  if (align_num == 0)
	    continue;
	  
	  if (align->ave_pct < 20) /* In 1 on 1 aligns...this is 10:1 advantage. */
	    {
	      /* In this case, give every one of the societies on the
		 losing side a little boost in power. */
	      
	      for (society = society_list; society; society = society->next)
		{
		  if (society->align != align->vnum)
		    continue;
		  
		  if (society->population < society->population_cap*2/3)
		    society->goals |= BUILD_MEMBER | BUILD_MAXPOP;
		  
		  /* Boost the raw materials on hand. This should help
		     out some. */
		  
		  for (i = 0; i < RAW_MAX; i++)
		    if (society->raw_curr[i] < RAW_TAX_AMOUNT/8)
		      society->raw_curr[i] += 100;
		  
		  /* Boost the max_pops available, up to a point. */
		  
		  if (society_max_pop (society) < 300)
		    {
		      for (i = 0; i < 5; i++)
			increase_max_pop(society);
		    }
		  
		  /* Increase the quality up to a point. */
		  
		  if (society->quality < 7)
		    society->quality++;
		  
		  /* Increase any tiers that are too low in the society. */
		  
		  for (i = 0; i < 2; i++)
		    increase_tier (society);
		}
	    }
	}      
    }
  return;
}

/* In this routine, the alignments attempt to help their member
   societies that are under siege. */

void
help_societies_under_siege(void)
{
  RACE *align;
  int align_num;
  SOCIETY *society, *ok_society, *hurt_society;
  int num_ok_society = 0;
  int num_hurt_society = 0;   
  THING *target_room;
  
  
  for (align_num = 1; align_num < ALIGN_MAX; align_num++)
    {
     
      if ((align = align_info[align_num]) == NULL ||
	  align->warriors < 5000 || nr (1,200) != 37)
	continue;
      
      /* Number of hurt and ok societies. */
      num_ok_society = 0;
      num_hurt_society = 0;
      
      /* Find all of the hurt and ok societies in this alignment. */
      
      for (society = society_list; society; society = society->next)
	{
	  if (society->align != align->vnum)
	    continue;
	  if (society->alert >= society->population &&
	      society->alert >= 30 && society->assist_hours < 1)
	    num_hurt_society++;
	  if (society->alert < society->population/3 &&
	      society->population >= 100)
	    num_ok_society++;
	}
      
      /* Only do something if there are hurt societies, and other
	 societies that can help them. */
      if (num_hurt_society == 0 || num_ok_society == 0)
	continue;
      
      /* Each hurt society finds its "worst" room...the room with the
	 most enemy power in it, and this room gets returned so that
	 the hunters will know where to go. */
      
      
      for (hurt_society = society_list; hurt_society; hurt_society = hurt_society->next)
	{
	  if (hurt_society->align != align->vnum ||
	      hurt_society->alert < hurt_society->population ||
	      hurt_society->assist_hours > 0)
	    continue;
	  hurt_society->assist_hours = RAID_HOURS + 10;
	  if ((target_room = find_weakest_room (hurt_society)) == NULL)
	    continue;
	  
	  /* Assist rumors happen when an alignment assists a society. */
	  add_rumor (RUMOR_ASSIST, align->vnum, hurt_society->vnum, 0, 0);
	  for (ok_society = society_list; ok_society; ok_society = ok_society->next)
	    {
	      if (ok_society->align != align->vnum ||
		  ok_society->population < 100 ||
		  ok_society->alert >= ok_society->population/3 ||
		  nr (1, num_hurt_society) != 1 ||
		  nr (1,2) != 2)
		continue;
	      
	      update_raiding (ok_society, hurt_society->vnum);
	    }
	}
    }
  return;
}


/* This lets alignments update their goals. Right now the only goal they
   really have is winning, and they can only make changes by altering
   the percentage of money spent on economics vs military. */

void
update_alignment_goals (void)
{
  RACE *align, *best_align = NULL;
  int best_align_vnum = 0, i, j, best_align_power = 0;
  int total_raws_wanted, warrior_pct_diff;
  SOCIETY *soc;
  int best_enemy_power = 0;
  int best_enemy_vnum = 0;
  int hated_society[ALIGN_MAX];
  int society_kills[ALIGN_MAX];


  /* Find the "best" alignment. */
  
  for (i = 1; i < ALIGN_MAX; i++)
    {
      if ((align = align_info[i]) != NULL)
	{
	  if (align->power_pct > best_align_power)
	    {
	      best_align_power = align->power_pct;
	      best_align = align;
	      best_align_vnum = align->vnum;
	    }
	}
    }
  
  /* Let each alignment find out its most hated enemy. */
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      hated_society[i] = 0;
      society_kills[i] = 0;
    }

  for (soc = society_list; soc; soc = soc->next)
    {
      for (i = 0; i < ALIGN_MAX; i++)
	{
	  /* Check if this society has the max kills vs an alignment 
	     currently. */
	  if (soc->kills[i] > society_kills[i])
	    {
	      hated_society[i] = soc->vnum;
	      society_kills[i] = soc->kills[i];
	    }
	}
    }
  
  /* Now set the most hated enemy for each alignment. */
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if ((align = align_info[i]) != NULL && align->vnum >= 0 &&
	  align->vnum < ALIGN_MAX)
	{
	  align->most_hated_society = hated_society[align->vnum];
	}
    }
  

  /* Now check each align to see how its goals will change. */
  
  for (i = 1; i < ALIGN_MAX; i++)
    {
      best_enemy_power = 0;
      best_enemy_vnum = 0;
      if ((align = align_info[i]) == NULL)
	continue;
      total_raws_wanted = 0;
      for (j = 0; j < RAW_MAX; j++)
	total_raws_wanted += align->raw_want[i];
      
      /* If raws wanted are big enough, then increase economy. */

      if (nr (1,10000) < total_raws_wanted)
	{	  
	  align->warrior_goal_pct -= nr (1,5);
	}
      
      /* Otherwise see if we need to increase military. */
      
      else if (align->vnum != best_align_vnum && best_align)
	{
	  align->warrior_goal_pct += nr (1,5);
	}
      else
	{
	  /* If you're the best align, try to make your warrior goal
	     pct go to the middle range between min and max. 
	     If it's not there, make it go 1/5 of the distance to 
	     the middle. */
	  
	  warrior_pct_diff = align->warrior_goal_pct -
	    (WARRIOR_PERCENT_MIN + WARRIOR_PERCENT_MAX)/2;
	  
	  if (warrior_pct_diff > 0)
	    align->warrior_goal_pct -= MAX (1, warrior_pct_diff/5);
	  else if (warrior_pct_diff < 0)
	    align->warrior_goal_pct += MAX (1, warrior_pct_diff/5);
	}
      align->warrior_goal_pct = MID (WARRIOR_PERCENT_MIN,
				     align->warrior_goal_pct,
				     WARRIOR_PERCENT_MAX);
      
      /* Once in a while, have an all-out attack on the best alignment
	 if it's on the other side. */

      if (align->vnum != best_align_vnum && nr (1,250) == 35 &&
	  align->warriors >= 2000)
	{
	  for (soc = society_list; soc; soc = soc->next)
	    {
	      if (soc->align == best_align_vnum &&
		  soc->power > best_enemy_power)
		{
		  best_enemy_power = soc->power;
		  best_enemy_vnum = soc->vnum;
		}
	    }
	  
	  if (best_enemy_vnum > 0)
	    {
	      for (soc = society_list; soc; soc = soc->next)
		{
		  if (soc->align == align->vnum)
		    update_raiding (soc, best_enemy_vnum);
		}
	    }
	}      
    }

  /* Now go after a powerful player (if he/she is not in a
     protected area. Only do this after topten has enough
     people on it that the minimum number is 1000. */
  
  if (nr (1,600) == 451 && pkd[PK_WPS][PK_LISTSIZE-1].value > 1000)
    {
      THING *vict;
      int name_choice = nr (0, PK_LISTSIZE-1);
      if (pkd[PK_WPS][name_choice].name[0])      
	{
	  /* See if the victim is online and outside of a safe area. */
	  
	  for (vict = thing_hash[PLAYER_VNUM % HASH_SIZE]; vict; vict = vict->next)
	    {
	      if (IS_PC (vict) && is_named (vict, pkd[PK_WPS][name_choice].name) &&
		  vict->in && vict->in->in && IS_AREA (vict->in->in) &&
		  DIFF_ALIGN (vict->align, vict->in->in->align))
		break;
	    }
	

	  /* If a suitable victim exists, pick an alignment to use
	     to attack that person. */
	  
	  if (vict)
	    {
	      int align_choices = 0, align_chose = 0, choice = 0;
	      for (i = 1; i < ALIGN_MAX; i++)
		{
		  if ((align = align_info[i]) == NULL)
		    continue;
		  if (DIFF_ALIGN (align->vnum, vict->align))
		    align_choices++;
		}
	      if (align_choices > 0)
		align_chose = nr (1, align_choices);
	      
	      for (i = 1; i < ALIGN_MAX; i++)
		{
		  if ((align = align_info[i]) == NULL)
		    continue;
		  
		  if (DIFF_ALIGN (align->vnum, vict->align) &&
		      ++choice == align_chose)
		    break;
		}
	      
	      /* If an alignment exists to use to attack that person,
		 send creatures to that person. */
	      
	      if (align)
		{
		  for (soc = society_list; soc; soc = soc->next)
		    {
		      if (soc->align == align->vnum)
			society_name_attack (soc, pkd[PK_WPS][name_choice].name);
		    }
		}
	    }
	}
    }
  


  return;
}
      

