#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"


void
society_edit (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  int cnum, value, type, anum, i;
  SOCIETY *soc;
  char *oldarg = arg;
  
  if (!th || !IS_PC (th) || !th->fd || !th->pc->editing ||
      th->fd->connected != CON_SOCIEDITING)
    return;
  
  soc = (SOCIETY *) th->pc->editing;
  
  if (!arg || !*arg)
    {
      show_society (th, soc);
      return;
    }
  
  arg = f_word (arg, arg1);
  
  if (!str_cmp (arg1, "done"))
    {
      if (IS_PC (th))
	th->pc->editing = NULL;
      if (th->fd)
	th->fd->connected = CON_ONLINE;
      stt ("Ok all done editing.\n\r", th);
      return;
    }

  /* These first few things let you set information on castes. They
     make you do command <castenum> <amount> and automatically remove
     1 from the caste num just to make things work better. */

  if (!str_cmp (arg1, "kills"))
    {
      arg = f_word (arg, arg1);
      if (!str_cmp (arg1, "all"))
	{
	  for (i = 0; i < ALIGN_MAX; i++)
	    soc->kills[i] = atoi (arg);
	  show_society (th,soc);
	  return;
	}
      if ((anum = atoi (arg1)) <= 0 || anum > ALIGN_MAX)
	{
	  sprintf (buf, "You must choose an alignment between 0 and %d.\n\r",
		   ALIGN_MAX - 1);
	  stt (buf, th);
	  return;
	}
      soc->kills[anum] = atoi (arg);
      show_society (th, soc);
      return;
    }
  if (!str_cmp (arg1, "affinity"))
    {
      arg = f_word (arg, arg1);
      if (!str_cmp (arg1, "all"))
	{
	  for (i = 0; i < ALIGN_MAX; i++)
	    soc->align_affinity[i] = atoi (arg);
	  show_society (th,soc);
	  return;
	}
      if ((anum = atoi (arg1)) <= 0 || anum > ALIGN_MAX)
	{
	  sprintf (buf, "You must choose an alignment between 0 and %d.\n\r",
		   ALIGN_MAX - 1);
	  stt (buf, th);
	  return;
	}
      soc->align_affinity[anum] = atoi (arg);
      show_society (th, soc);
      return;
    }
  if (!str_cmp (arg1, "flag"))
    {
      edit_flags (th, &soc->flags, arg);
      show_society (th, soc);
      return;
    }
  if (!str_cmp (arg1, "killed_by"))
    {
      arg = f_word (arg, arg1);
      if (!str_cmp (arg1, "all"))
	{
	  for (i = 0; i < ALIGN_MAX; i++)
	    soc->killed_by[i] = atoi (arg);
	  show_society (th,soc);
	  return;
	}
      if ((anum = atoi (arg1)) <= 0 || anum > ALIGN_MAX)
	{
	  sprintf (buf, "You must choose an alignment between 0 and %d.\n\r",
		   ALIGN_MAX - 1);
	  stt (buf, th);
	  return;
	}
      soc->killed_by[anum] = atoi (arg);
      show_society (th, soc);
      return;
    }
   
  if (!str_cmp (arg1, "start"))
    {
      arg = f_word (arg, arg1);
      if ((cnum = atoi (arg1)) <= 0 || cnum > CASTE_MAX)
	{
	  sprintf (buf, "You must choose a caste between 1 and %d.\n\r",
		   CASTE_MAX);
	  stt (buf, th);
	  return;
	}
      cnum--;
      soc->start[cnum] = atoi (arg);
      show_society (th, soc);
      return;
    }
  else  if (!str_cmp (arg1, "max_pop") ||
	    !str_cmp (arg1, "maxpop") ||
	    !str_cmp (arg1, "max"))
    {
      arg = f_word (arg, arg1);
      if ((cnum = atoi (arg1)) <= 0 || cnum > CASTE_MAX)
	{
	  sprintf (buf, "You must choose a caste between 1 and %d.\n\r",
		   CASTE_MAX);
	  stt (buf, th);
	  return;
	}
      cnum--;
      soc->max_pop[cnum] = atoi (arg);
      show_society (th, soc);
      return;
    }
  else  if (!str_cmp (arg1, "max_tier"))
    {
      arg = f_word (arg, arg1);
      if ((cnum = atoi (arg1)) <= 0 || cnum > CASTE_MAX)
	{
	  sprintf (buf, "You must choose a caste between 1 and %d.\n\r",
		   CASTE_MAX);
	  stt (buf, th);
	  return;
	}
      cnum--;
      soc->max_tier[cnum] = atoi (arg);
      if (cnum == 0)
	soc->curr_tier[cnum] = soc->max_tier[cnum];
      show_society (th, soc);
      return;
    }
  else  if (!str_cmp (arg1, "curr_tier"))
    {
      arg = f_word (arg, arg1);
      if ((cnum = atoi (arg1)) <= 0 || cnum > CASTE_MAX)
	{
	  sprintf (buf, "You must choose a caste between 1 and %d.\n\r",
		   CASTE_MAX);
	  stt (buf, th);
	  return;
	}
      cnum--;
      soc->curr_tier[cnum] = atoi (arg); 
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "alignment"))
    {
      value = atoi (arg);
      if (value >= 0 && value < ALIGN_MAX && align_info[value])
	{
	  soc->align = value;
	  change_society_align (soc, value);
	  stt ("Ok, alignment changed.\n\r", th);
	  show_society (th, soc);
	  return;
	}
      else
	{
	  stt ("Please pick an alignment number.\n\r", th);
	  return;
	}
    }
  else if (!str_cmp (arg1, "object_start") ||
	   !str_cmp (arg1, "obj_start") ||
	   !str_cmp (arg1, "objstart"))
    {
      soc->object_start = atoi (arg);
      stt ("Object start vnum set.\n\r", th);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "object_end") ||
	   !str_cmp (arg1, "obj_end") ||
	   !str_cmp (arg1, "objend"))
    {
      soc->object_end = atoi (arg);
      stt ("Object end vnum set.\n\r", th);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "morale"))
    {
      soc->morale = MID (-MAX_MORALE, atoi(arg), MAX_MORALE);
      stt ("Morale changed.\n\r", th);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "assist"))
    {
      soc->assist_hours = atoi(arg);
      stt ("Assist hours set.\n\r", th);
      show_society (th, soc);
      return;
    }      
  else  if (!str_cmp (arg1, "chance"))
    {
      arg = f_word (arg, arg1);
      if ((cnum = atoi (arg1)) <= 0 || cnum > CASTE_MAX)
	{
	  sprintf (buf, "You must choose a caste between 1 and %d.\n\r",
		   CASTE_MAX);
	  stt (buf, th);
	  return;
	}
      cnum--;
      soc->chance[cnum] = atoi (arg); 
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "generated") ||
	   !str_cmp (arg1, "generated_from") ||
	   !str_cmp (arg1, "gen_from") ||
	   !str_cmp (arg1, "generator"))
    {
      soc->generated_from = atoi (arg);
      stt ("Society generator changed.\n\r", th);
      show_society (th, soc);
      return;
    }
  else  if (!str_cmp (arg1, "base_chance") || !str_cmp (arg1, "chance"))
    {
      arg = f_word (arg, arg1);
      if ((cnum = atoi (arg1)) <= 0 || cnum > CASTE_MAX)
	{
	  sprintf (buf, "You must choose a caste between 1 and %d.\n\r",
		   CASTE_MAX);
	  stt (buf, th);
	  return;
	}
      cnum--;
      soc->base_chance[cnum] = atoi (arg); 
      soc->chance[cnum] = atoi(arg);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "alife"))
    {
      arg = f_word (arg, arg1);
      if (!str_prefix (arg1, "combat"))
	{
	  soc->alife_combat_bonus = atoi(arg);
	  show_society (th, soc);
	  return;	  
	}
      if (!str_prefix (arg1, "growth"))
	{
	  soc->alife_growth_bonus = atoi(arg);
	  show_society (th, soc);
	  return;	  
	}
      if (!str_prefix (arg1, "x_home"))
	{
	  soc->alife_home_x = atoi(arg);
	  show_society (th, soc);
	  return;	  
	}
      if (!str_prefix (arg1, "y_home"))
	{
	  soc->alife_home_y = atoi(arg);
	  show_society (th, soc);
	  return;	  
	}
      stt ("Alife x or y or growth or combat <number>\n\r", th);
      return;      
    }
  
  else if (!str_cmp (arg1, "cflags") || !str_cmp (arg1, "cflag"))
    {
      arg = f_word (arg, arg1);
      if ((cnum = atoi (arg1)) <= 0 || cnum > CASTE_MAX)
	{
	  sprintf (buf, "You must choose a caste between 1 and %d.\n\r",
		   CASTE_MAX);
	  stt (buf, th);
	  return;
	}
      cnum--;
      if ((value = find_bit_from_name (FLAG_CASTE, arg)) != 0)
	{
	  soc->cflags[cnum] ^= value;
	  sprintf (buf, "Caste flag %s toggled %s.\n\r", arg, IS_SET (soc->cflags[cnum], value) ? "On" : "Off");
	  stt (buf, th);
	  show_society (th, soc);
	  return;
	}
      stt ("flag <castenum> <flagname>\n\r", th);
      return;
    }
  else if (!str_cmp (arg1, "name"))
    {
      free_str (soc->name);
      soc->name = new_str (arg);
      stt ("Name changed.\n\r", th);
      show_society (th, soc);
      return;
    } 
  else if (!str_cmp (arg1, "pname"))
    {
      free_str (soc->pname);
      soc->pname = new_str (arg);
      stt ("Plural Name changed.\n\r", th);
      show_society (th, soc);
      return;
    } 
  else if (!str_cmp (arg1, "aname"))
    {
      free_str (soc->aname);
      soc->aname = new_str (arg);
      stt ("Adjective Name changed.\n\r", th);
      show_society (th, soc);
      return;
    } 
  else if (!str_cmp (arg1, "raw"))
    {      
      arg = f_word (arg, arg1);
      if (!str_cmp (arg1, "all"))
	{
	  int i;
	  value = atoi (arg);
	  for (i = 0; i < RAW_MAX; i++)
	    soc->raw_curr[i] = value;
	  show_society (th, soc);
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
      soc->raw_curr[type] = value;
      stt ("Raw material value set.\n\r", th);
      show_society (th, soc);
      return;
    }			 
  else if (!str_cmp (arg1, "adjective"))
    {
      free_str (soc->adj);
      soc->adj = new_str (arg);
      stt ("Adjective changed.\n\r", th);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "room_start"))
    {
      sprintf (buf, "Room start changed to %d.\n\r", atoi (arg));
      soc->room_start = atoi (arg);
      show_society (th, soc);
      return;
    }  
  else if (!str_cmp (arg1, "warrior") || !str_cmp (arg1, "warrior_percent") ||
	   !str_cmp (arg1, "military") || !str_cmp (arg1, "percent"))
    {
      if (atoi(arg) < WARRIOR_PERCENT_MIN || atoi(arg) > WARRIOR_PERCENT_MAX)
	{
	  sprintf (buf, "The warrior percent must be between %d and %d\n\r",
		   WARRIOR_PERCENT_MIN, WARRIOR_PERCENT_MAX);
	  stt (buf, th);
	  return;
	}
      
      sprintf (buf, "Warrior percent changed to %d.\n\r", atoi (arg));
      soc->warrior_percent = atoi (arg);
      show_society (th, soc);
      return;
    } 
  else if (!str_cmp (arg1, "lifespan") ||
	   !str_cmp (arg1, "life"))
    {
      soc->lifespan = atoi(arg);
      show_society (th, soc);
      stt ("Lifepan changed.\n\r", th);
      return;
    }
  else if (!str_cmp (arg1, "level") ||
	   !str_cmp (arg1, "levels") || 
	   !str_cmp (arg1, "max_level") ||
	   !str_cmp (arg1, "maxlevel"))
    {
      soc->level = atoi(arg);
      if (soc->level < 0)
	soc->level = 0;
      stt ("Society max level changed.\n\r", th);
      show_society (th, soc);
      return;
    }     
  else if (!str_cmp (arg1, "level_bonus"))
    {
      soc->level_bonus = atoi(arg);
      if (soc->level_bonus < 0)
	soc->level_bonus = 0;
      stt ("Society level bonus changed.\n\r", th);
      show_society (th, soc);
      return;
    } 
  else if (!str_cmp (arg1, "quality"))
    {
      soc->quality = atoi(arg);
      stt ("Quality changed.\n\r", th);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "guardpost"))
    {
      arg = f_word (arg, arg1);
      if (atoi (arg1) < 1 || atoi (arg1) > NUM_GUARD_POSTS)
	{
	  sprintf (buf, "Guardpost <num> <location>. Num must be between 1 and %d.\n\r", NUM_GUARD_POSTS);
	  stt (buf, th);
	  return;
	}
      soc->guard_post[atoi (arg1) - 1] = atoi (arg);
      stt ("Guard post changed.\n\r", th);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "alert"))
    {
      sprintf (buf, "Alert level changed to %d.\n\r", atoi (arg));
      soc->alert = atoi (arg);
      show_society (th, soc);
      return;
    } 
  else if (!str_cmp (arg1, "last_alert"))
    {
      sprintf (buf, "Last alert level changed to %d.\n\r", atoi (arg));
      soc->last_alert = atoi (arg);
      show_society (th, soc);
      return;
    } 
  else if (!str_cmp (arg1, "alert_hours"))
    {
      sprintf (buf, "Alert hours changed to %d.\n\r", atoi (arg));
      soc->alert_hours = atoi (arg);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "relic_hours"))
    {
      sprintf (buf, "Relic raid hours changed to %d.\n\r", atoi (arg));
      soc->relic_raid_hours = atoi (arg);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "relic_gather"))
    {
      sprintf (buf, "Relic raid gather point changed to %d.\n\r", atoi (arg));
      soc->relic_raid_gather_point = atoi (arg);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "relic_target"))
    {
      sprintf (buf, "Relic raid target changed to %d.\n\r", atoi (arg));
      soc->relic_raid_target = atoi (arg);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "alert_rally_point"))
    {
      sprintf (buf, "Alert rally point changed to %d.\n\r", atoi (arg));
      soc->alert_rally_point = atoi (arg);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "room_end"))
    {
      if (atoi(arg) < soc->room_start ||
	  atoi(arg) > soc->room_start + 300)
	{
	  stt ("The room ending must be between 0 and 300 rooms after the room start.\n\r", th);
	  return;
	}
      sprintf (buf, "Room end changed to %d.\n\r", atoi (arg));
      soc->room_end = atoi (arg);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "hours") || !str_cmp (arg1, "settle_hours"))
    {
      sprintf (buf, "Settle hours changed to %d.\n\r", atoi (arg));
      soc->settle_hours = atoi (arg);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "raid") || !str_cmp (arg1, "raid_hours"))
    {
      sprintf (buf, "Raid hours changed to %d.\n\r", atoi (arg));
      soc->raid_hours = atoi (arg);
      show_society (th, soc);
      return;
    }
  else if (!str_cmp (arg1, "society_flag") || !str_cmp (arg1, "soci_flag"))
    {
      if ((value = find_bit_from_name (FLAG_SOCIETY, arg)) != 0)
	{
	  soc->society_flags ^= value;
	  sprintf (buf, "Society flag %s toggled %s.\n\r", arg, IS_SET (soc->society_flags, value) ? "On" : "Off");
	  stt (buf, th);
	  show_society (th, soc);
	  return;
	}
      else
	{
	  stt ("Flag <flagname>\n\r", th);
	  return;
	}
    }
  
  interpret (th, oldarg);
  return;
}
		
			  
 
	  

/* This shows the data for a society to an admin. */


void
show_society (THING *th, SOCIETY *soc)
{
  char buf[STD_LEN];
  char buf2[STD_LEN];
  int i;
  
  if (!th || !soc)
    return;
  
  SBIT (server_flags, SERVER_CHANGED_SOCIETIES);
  
  
  sprintf (buf, "Society %d: Name: %-15s  Pname: %-15s   Aname: %-15s\n\r",
	   soc->vnum, soc->name,
	   soc->pname, soc->aname);
  stt (buf, th);
  sprintf (buf, "LevBon: %d Align: %d Generator: %d Adj: %s. CraftNeed: %d  ShopNeed: %d Morale: %d\n\rPop: %d(%d)  Hrs(Setl.: %d  Raid: %d  Ast.: %d) Wr%%: %d Pow: %d Life: %d\n\r",
	   soc->level_bonus, 
	   soc->align,
	   soc->generated_from,
	   soc->adj, 
	   soc->crafters_needed,
	   soc->shops_needed,
	   soc->morale,
	   soc->population, 
	   soc->recent_maxpop,
	   soc->settle_hours,
	   soc->raid_hours,
	   soc->assist_hours,
	   soc->warrior_percent, 
	   soc->power,
	   soc->lifespan);
  stt (buf, th);
  sprintf (buf, "Rooms:  %d-%d Objs: %d-%d Goal: (%d) Qual: (%d) Popcap (%d) Level (%d)\n\r", soc->room_start, soc->room_end, soc->object_start, soc->object_end,
	   soc->goals, soc->quality, soc->population_cap, soc->level);
  stt (buf, th);
  sprintf (buf, "Alert: Current: %d  Last: %d  Hours: %d  RallyPt: %d\n\r",
	   soc->alert, soc->last_alert, soc->alert_hours, soc->alert_rally_point);
  stt (buf, th); 
  sprintf (buf, "Relic Raid: Target: %d  Hours: %d  GatherPt: %d\n\r",
	   soc->relic_raid_target, soc->relic_raid_hours, soc->relic_raid_gather_point);
  stt (buf, th);
  sprintf (buf, "Alife: Combat: %d  Growth: %d Home (%d,%d)\n\r",
	   soc->alife_combat_bonus, soc->alife_growth_bonus,
	   soc->alife_home_x, soc->alife_home_y);
  stt (buf, th);
  buf[0] = '\0';
  sprintf (buf, "Kills : ");
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (align_info[i])
	{
	  sprintf (buf2, "%d: %d  ",
		   i, soc->kills[i]);
	  strcat (buf, buf2);
	}
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Killed: ");
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (align_info[i])
	{
	  sprintf (buf2, "%d: %d  ",
		   i, soc->killed_by[i]);
	  strcat (buf, buf2);
	}
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  buf[0] = '\0';
  sprintf (buf, "Affinity: ");
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (align_info[i])
	{
	  sprintf (buf2, "%d: %d  ",
		   i, soc->align_affinity[i]);
	  strcat (buf, buf2);
	}
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Raws: ");
  for (i = 1; i < RAW_MAX; i++)
    {
      if (!gather_data[i].command[0])
	continue;
      if (i == 4)
	strcat (buf, "\n\r");
      sprintf (buf2, "%s: %d(%d)  ", gather_data[i].raw_name, soc->raw_curr[i],
	       soc->raw_want[i]);
      strcat (buf, buf2);
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  
  if (soc->society_flags)
    {
      sprintf (buf, "%s\n\r", list_flagnames (FLAG_SOCIETY, soc->society_flags));
      stt (buf, th);
    }
  /* Show caste info. This has changed massively from what it was before. */


  sprintf (buf, "Guard Posts:");
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    {
      if (soc->guard_post[i])
	{
	  sprintf (buf2, " %d", soc->guard_post[i]);
	  strcat (buf, buf2);
	}
      
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  stt (show_flags (soc->flags, 0, TRUE), th);
  for (i = 0; i < CASTE_MAX; i++)
    {
      if (soc->start[i] == 0)
	continue;
      sprintf (buf, "C\x1b[1;36m%2d\x1b[0;37m St: %5d Tier: %1d/%1d Pop: %3d/%-3d Chan(Base): %2d(%2d) %s\n\r",
	       i + 1,
	       soc->start[i],
	       soc->curr_tier[i], soc->max_tier[i],
	       soc->curr_pop[i], soc->max_pop[i],
	       soc->chance[i], soc->base_chance[i],
	       list_flagnames (FLAG_CASTE, soc->cflags[i]));
      stt (buf, th);
    }
  return;
}






