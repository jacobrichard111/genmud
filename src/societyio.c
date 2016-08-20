#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "mapgen.h"
#include "track.h"

void
read_societies (void)
{
  SOCIETY *soc = NULL;
  
  FILE_READ_SETUP ("society.dat");
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY ("SOCIETY")
	{
	  soc = read_society (f);
	  if (soc->vnum == 0)
	    free2(soc);
	  else
	    {
	      /* Put societies in order based on vnum from smallest to biggest. */
	      top_society = MAX (top_society, soc->vnum);
	      soc_to_list (soc);
	    }
	}
      FKEY ("END_OF_SOCIETIES")
	break;
      FILE_READ_ERRCHECK ("society.dat");
    }
  fclose (f);
  return;
}

SOCIETY *
read_society (FILE *f)
{
  SOCIETY *soc;
  THING *room;
  FLAG *flg;
  int i;
  FILE_READ_SINGLE_SETUP;
  if (!f)
    return NULL;
  
  soc = new_society ();
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("Name")
	soc->name = new_str (read_string (f));
      FKEY("Objects")
	{
	  soc->object_start = read_number (f);
	  soc->object_end = read_number (f);
	}
      FKEY("PName")
	soc->pname = new_str (read_string (f));
      FKEY("AName")
	soc->aname = new_str (read_string (f));
      FKEY("Adj")
	soc->adj = new_str (read_string (f));
      FKEY("Kills")
	{
	  for (i = 0; i < ALIGN_MAX; i++)
	    soc->kills[i] = read_number (f);
	}
      FKEY("Affinity")
	{
	  for (i = 0; i < ALIGN_MAX; i++)
	    soc->align_affinity[i] = read_number (f);
	}
      FKEY("KilledBy")
	{
	  for (i = 0; i < ALIGN_MAX; i++)
	    soc->killed_by[i] = read_number (f);
	}
      FKEY("Needs")
	{
	  soc->crafters_needed = read_number (f);
	  soc->shops_needed = read_number (f);
	}
      FKEY("RumorsKnown")
	{
	  for (i = 0; i < SOCIETY_RUMOR_ARRAY_SIZE; i++)
	    soc->rumors_known[i] = read_number (f);
	}
      FKEY("Gen")
	{
	  soc->vnum = read_number (f);
	  soc->society_flags = read_number (f);
	  soc->room_start = read_number (f);
	  soc->room_end = read_number (f);
	  soc->settle_hours = read_number (f);
	  soc->quality = read_number (f);
	  soc->goals = read_number (f);
	  soc->align = read_number (f);
	  soc->level_bonus = read_number (f);
	  soc->warrior_percent = read_number (f);
	  soc->raid_hours = read_number (f);
	  soc->assist_hours = read_number (f);
	  soc->recent_maxpop = read_number (f);
	  soc->generated_from = read_number (f);
	  soc->level = read_number (f);
	  soc->abandon_hours = read_number (f);
	  soc->morale = read_number (f);
	  soc->lifespan = read_number (f);
	  if ((room = find_thing_num (soc->room_start)) == NULL ||
	      !IS_ROOM (room))
	    {
	      soc->room_start = 0;
	      soc->room_end = 0;
	    }

	}
      FKEY("Alife")
	{
	  soc->alife_combat_bonus = read_number (f);
	  soc->alife_growth_bonus = read_number (f);
	  soc->alife_home_x = read_number (f);
	  soc->alife_home_y = read_number (f);
	}
      FKEY("Alert")
	{
	  soc->alert = read_number(f);
	  soc->last_alert = read_number(f);
	  soc->alert_hours = read_number(f);
	  soc->alert_rally_point = read_number(f);
	}
      FKEY("CStart")
	{
	  for (i = 0; i < CASTE_MAX; i++)
	    soc->start[i] = read_number (f);
	}
      FKEY("GuardPosts")
	{
	  for (i = 0; i < NUM_GUARD_POSTS; i++)
	    soc->guard_post[i] = read_number (f);
	}
      FKEY("CMaxPop")
	{
	  for (i = 0; i < CASTE_MAX; i++)
	    soc->max_pop[i] = read_number (f);
	}
      FKEY("CMaxTier")
	{
	  for (i = 0; i < CASTE_MAX; i++)
	    soc->max_tier[i] = read_number (f);
	}
      FKEY("CCurrTier")
	{
	  for (i = 0; i < CASTE_MAX; i++)
	    soc->curr_tier[i] = read_number (f);
	}
      FKEY("CChance")
	{
	  for (i = 0; i < CASTE_MAX; i++)
	    soc->chance[i] = read_number (f);
	}
      FKEY("CBaseChance")
	{
	  for (i = 0; i < CASTE_MAX; i++)
	    soc->base_chance[i] = read_number (f);
	}
      FKEY("CFlags")
	{
	  for (i = 0; i < CASTE_MAX; i++)
	    soc->cflags[i] = read_number (f);
	}      
      FKEY("RawCurr")
	{
	  for (i = 0; i < RAW_MAX; i++)
	    soc->raw_curr[i] = read_number(f);
	}
      FKEY ("RelicRaid")
	{
	  soc->relic_raid_hours = read_number (f);
	  soc->relic_raid_gather_point = read_number (f);
	  soc->relic_raid_target = read_number (f);
	}
      FKEY("RawWant")
	{
	  for (i = 0; i < RAW_MAX; i++)
	    soc->raw_want[i] = read_number(f);
	}
      FKEY("Flag")
	{
	  flg = read_flag (f);
	  flg->next = soc->flags;
	  soc->flags = flg;
	}
      FKEY("END_SOCIETY")
	{
	  set_up_guard_posts (soc);
	  return soc;
	}
      FILE_READ_ERRCHECK("society.dat");
    }
  return soc;
}

void
write_societies (void)
{
  FILE *f;
  SOCIETY *soc;
  char buf[STD_LEN];
  
  if ((f = wfopen ("society.bak", "w")) == NULL)
    return;
  
  
  /* Save societies that haven't been nuked and they either
     aren't destructible, or they can make new members or they
     have actual members in them. */
  
  for (soc = society_list; soc != NULL; soc = soc->next)
    {
      if (IS_SET (soc->society_flags, SOCIETY_NUKE) ||
	  soc->vnum == 0)
	continue;
      
      if (IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE) &&
	  (soc->room_start == 0 ||
	   soc->recent_maxpop == 0))
	continue;
   
	write_society (f, soc);
    }
  fprintf (f, "\nEND_OF_SOCIETIES\n");
  fclose (f);
  sprintf (buf, "cp %ssociety.bak %ssociety.dat",
	   WLD_DIR, WLD_DIR);
  system (buf);
  return;
}

void
write_society (FILE *f, SOCIETY *soc)
{
  int i;
  FLAG *flag;
  
  if (!f || !soc)
    return;
  fprintf (f, "SOCIETY\n");
  write_string (f, "Name", soc->name);
  write_string (f, "PName", soc->pname);
  write_string (f, "AName", soc->aname);
  write_string (f, "Adj", soc->adj);
  fprintf (f, "Gen %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
	   soc->vnum, 
	   soc->society_flags, 
	   soc->room_start, 
	   soc->room_end, 
	   soc->settle_hours,	   
	   soc->quality, 
	   soc->goals,    
	   soc->align, 	   
	   soc->level_bonus, 
	   soc->warrior_percent,
	   soc->raid_hours,
	   soc->assist_hours,
	   soc->recent_maxpop,
	   soc->generated_from,
	   soc->level,
	   soc->abandon_hours,
	   soc->morale,
	   soc->lifespan);
  fprintf (f, "Needs %d %d\n",
	   soc->crafters_needed, soc->shops_needed);

  fprintf (f, "Objects %d %d\n",
	   soc->object_start, soc->object_end);
  fprintf (f, "Alert %d %d %d %d\n", soc->alert, soc->last_alert, soc->alert_hours, soc->alert_rally_point);
  
  fprintf (f, "Alife %d %d %d %d\n", soc->alife_combat_bonus, soc->alife_growth_bonus, soc->alife_home_x, soc->alife_home_y);
 
/* Write what affinity this society has vs diff aligns. */
  
  fprintf (f, "Affinity");
  for (i = 0; i < ALIGN_MAX; i++)
    fprintf (f, " %d", soc->align_affinity[i]);
  fprintf (f, "\n");
  
  /* Write the kills this society has vs diff aligns. */
  
  fprintf (f, "Kills");
  for (i = 0; i < ALIGN_MAX; i++)
    fprintf (f, " %d", soc->kills[i]);
  fprintf (f, "\n");
  
  /* Write what other aligns have killed this society. */

  fprintf (f, "KilledBy");
  for (i = 0; i < ALIGN_MAX; i++)
    fprintf (f, " %d", soc->killed_by[i]);
  fprintf (f, "\n");

  /* Write what rumors are known by this society. */
  
  fprintf (f, "RumorsKnown");
  for (i = 0; i < SOCIETY_RUMOR_ARRAY_SIZE; i++)
    fprintf (f, " %d", soc->rumors_known[i]);
  fprintf (f, "\n");
  
  fprintf (f, "RelicRaid %d %d %d\n", 
	   soc->relic_raid_hours, soc->relic_raid_gather_point,
	   soc->relic_raid_target);
  /* Write the resources/goals info. */
  
  fprintf (f, "RawWant");
  for (i = 0; i < RAW_MAX; i++)
    fprintf (f, " %d", soc->raw_want[i]);
  fprintf (f, "\n");
  
  fprintf (f, "RawCurr");
  for (i = 0; i < RAW_MAX; i++)
    fprintf (f, " %d", soc->raw_curr[i]);
  fprintf (f, "\n");
  
  /* Write the caste info */

  fprintf (f, "CStart");
  for (i = 0; i < CASTE_MAX; i++)
    fprintf (f, " %d", soc->start[i]);
  fprintf (f, "\n");

  fprintf (f, "CMaxPop");
  for (i = 0; i < CASTE_MAX; i++)
    fprintf (f, " %d", soc->max_pop[i]);
  fprintf (f, "\n");
      
   fprintf (f, "CMaxTier");
  for (i = 0; i < CASTE_MAX; i++)
    fprintf (f, " %d", soc->max_tier[i]);
  fprintf (f, "\n");
  
  fprintf (f, "CCurrTier");
  for (i = 0; i < CASTE_MAX; i++)
    fprintf (f, " %d", soc->curr_tier[i]);
  fprintf (f, "\n");
 
 fprintf (f, "CChance");
  for (i = 0; i < CASTE_MAX; i++)
    fprintf (f, " %d", soc->chance[i]);
  fprintf (f, "\n");
  
  fprintf (f, "CBaseChance");
  for (i = 0; i < CASTE_MAX; i++)
    fprintf (f, " %d", soc->base_chance[i]);
  fprintf (f, "\n");

  fprintf (f, "CFlags");
  for (i = 0; i < CASTE_MAX; i++)
    fprintf (f, " %d", soc->cflags[i]);
  fprintf (f, "\n");
  
  fprintf (f, "GuardPosts");
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    fprintf (f, " %d", soc->guard_post[i]);
  fprintf (f, "\n");
  
  for (flag = soc->flags; flag != NULL; flag = flag->next)
    write_flag (f, flag);
  
  fprintf (f, "END_SOCIETY\n\n");
  return;
}
  
	       
	  










































