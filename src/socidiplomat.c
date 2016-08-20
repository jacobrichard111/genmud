#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "track.h"


/* This gives a chance for a player to make a society more amenable to
   coming over to a player's side. */

/* Diplomacy works like this. You must be in a room that's owned by a
   society and if they hate you due to you killing them (align_hate in
   pc struct) you must work to gain their trust. If they hate you
   because many people of your align have killed them (killed_by in
   society struct) then those must be decreased first. After you have
   their trust, you can use diplomacy to make them like you more. */
   

void
do_diplomacy (THING *th, char *arg)
{
  SOCIETY *soc;
  VALUE *build;
  int i;
  char buf[STD_LEN];
  int max_affinity; /* The max affinity your character can inspire. 
		       It's your remorts+1/10 capped by 95. */
  int change_amount; /* Amount you change their views of you. */

  
  if (!th || !IS_PC (th) || guild_rank (th, GUILD_DIPLOMAT) < GUILD_TIER_MAX ||
      !th->in)
    {
      stt ("You aren't enough of a diplomat to try diplomacy!\n\r", th);
      return;
    }

  if ((build = FNV (th->in, VAL_BUILD)) == NULL ||
      (soc = find_society_num (build->val[0])) == NULL ||
      soc->align >= ALIGN_MAX)
    {
      stt ("You aren't in a society village.\n\r", th);
      return;
    }
  

  change_amount = LEVEL(th)/10+th->pc->remorts/2+get_stat(th, STAT_CHA)-STAT_MAX*2/3+guild_rank(th, GUILD_DIPLOMAT);
  
  /* Check for a failed skill check. */

  if (!check_spell (th, "diplomacy", -1) || nr (1,7) == 3)
    {
      stt ("You made things worse.\n\r", th);
      soc->align_affinity[th->align] -= change_amount;
      soc->killed_by[th->align] += change_amount;
      return;
    }
  
  /* See if your alignment hate is too high. If so, they won't listen to
     you. */
  
  if (th->pc->align_hate[soc->align] >= ALIGN_HATE_WARN/5)
    {
      stt ("They are too wary of you to listen right now.\n\r", th);
      return;
    }

  if (th->mv < 20)
    {
      stt ("You're too tired to do this right now.\n\r", th);
      return;
    }
  
  sprintf (buf, "%s engage@s the %s of %s in diplomacy.\n\r",
	   NAME (th), 
	   society_pname (soc),
	   (th->in && th->in->in && IS_AREA (th->in->in) ?
	    NAME (th->in->in) : "these lands"));
  
  act (buf, th, NULL, NULL, NULL, TO_ALL);
  

  /* Basically, diplomacy decreases how much a society hates your align
     and it decreases the society affinity for other aligns and it
     increases the society affinity for your align. Affinity makes
     it more or less likely that something will switch align or allow
     itself to be bribed to change sides. */

  if (soc->killed_by[th->align] > 0)
    {
      stt ("They hate you less.\n\r", th);
      soc->killed_by[th->align] -= MIN (soc->killed_by[th->align], change_amount);
      add_society_reward (th, th->in, REWARD_DIPLOMACY, change_amount);
    }
  
  if (soc->killed_by[th->align] < 5)
    {
      /* Now reduce other affinities for other alignments. */
      
      for (i = 0; i < ALIGN_MAX; i++)
	{
	  if (!DIFF_ALIGN (i, soc->align))
	    continue;
	  
	  soc->align_affinity[i] -= MIN (soc->align_affinity[i],change_amount);
	}
      
      max_affinity = MIN(MAX_AFFINITY, 3*change_amount);
      if (soc->align_affinity[th->align] < max_affinity)
	{
	  stt ("They like you more.\n\r", th);
	  soc->align_affinity[th->align] += change_amount;
	  add_society_reward (th, th->in, REWARD_DIPLOMACY, change_amount);
	}
      else
	stt ("They already like you a lot.\n\r", th);
      if (soc->align_affinity[th->align] > MAX_AFFINITY)
	soc->align_affinity[th->align] = MAX_AFFINITY;
    }
  th->pc->wait += 50;
  th->mv -= 20;
  return;
}

  
/* This raises the morale of a friendly society where you are. */

void
do_inspire (THING *th, char *arg)
{
  SOCIETY *soc;
  VALUE *build;
  char buf[STD_LEN];
  int change_amount;

  if (!th || !IS_PC (th) || !th->in)
    return;
  
  
  if ((build = FNV (th->in, VAL_BUILD)) == NULL ||
      (soc = find_society_num (build->val[0])) == NULL ||
      soc->align >= ALIGN_MAX)
    {
      stt ("You aren't in a society village.\n\r", th);
      return;
    }
  
  if (DIFF_ALIGN (th->align, soc->align))
    {
      stt ("This isn't a friendly village. You don't want to inspire them!\n\r", th);
      return;
    }

  change_amount = LEVEL(th)/10+(th->pc->remorts/2)+get_stat(th, STAT_CHA)-STAT_MAX*2/3+guild_rank(th, GUILD_DIPLOMAT);
  
  /* Check for a failed skill check. */

  if (!check_spell (th, "inspire", -1) || nr (1,7) == 3)
    {
      stt ("You didn't make a very inspirational speech.\n\r", th);
      add_morale (soc, -change_amount);
      return;
    }
  
  /* See if your alignment hate is too high. If so, they won't listen to
     you. */
  
  if (th->pc->align_hate[soc->align] >= ALIGN_HATE_WARN/5)
    {
      stt ("They are too wary of you to listen right now.\n\r", th);
      return;
    }
  
  if (th->mv < 20)
    {
      stt ("You're too tired to do this right now.\n\r", th);
      return;
    }
  
  sprintf (buf, "@1n give@s a rousing speech and inspire@s the %s of %s.\n\r", 
	   society_pname (soc),
	   (th->in && th->in->in && IS_AREA (th->in->in) ?
	    NAME (th->in->in) : "these lands"));
  
  act (buf, th, NULL, NULL, NULL, TO_ALL);
  
  /* Add morale to your side and get rewarded. */
  add_morale (soc, change_amount);
  add_society_reward (th, th->in, REWARD_INSPIRE, change_amount);
  
  th->pc->wait += 50;
  th->mv -= 20;
  return;
}
  
  
  

  
/* This lowers the morale of an enemy society where you are. */

void
do_demoralize (THING *th, char *arg)
{
  SOCIETY *soc;
  VALUE *build;
  char buf[STD_LEN];
  int change_amount;

  if (!th || !IS_PC (th) || !th->in)
    return;
  
  
  if ((build = FNV (th->in, VAL_BUILD)) == NULL ||
      (soc = find_society_num (build->val[0])) == NULL ||
      soc->align >= ALIGN_MAX)
    {
      stt ("You aren't in a society village.\n\r", th);
      return;
    }
  
  if (!DIFF_ALIGN (th->align, soc->align))
    {
      stt ("This isn't an enemy village. You don't want to demoralize them!\n\r", th);
      return;
    }

  change_amount = LEVEL(th)/10+(th->pc->remorts/2)+get_stat(th, STAT_CHA)-STAT_MAX*2/3+guild_rank(th, GUILD_DIPLOMAT);
  
  /* Check for a failed skill check. */

  if (!check_spell (th, "demoralize", -1) || nr (1,7) == 3)
    {
      stt ("You're only making it worse on yourself!.\n\r", th);
      add_morale (soc, change_amount);
      return;
    }
  
  if (th->mv < 20)
    {
      stt ("You're too tired to do this right now.\n\r", th);
      return;
    }
  
  sprintf (buf, "%s insult@s and taunt@s the %s of %s, sapping their resolve.\n\r",
	   NAME (th), 
	   society_pname (soc),
	   (th->in && th->in->in && IS_AREA (th->in->in) ?
	    NAME (th->in->in) : "these lands"));
  
  act (buf, th, NULL, NULL, NULL, TO_ALL);
  


  add_morale (soc, -change_amount);
  add_society_reward (th, th->in, REWARD_DEMORALIZE, change_amount);
  
  th->pc->wait += 50;
  th->mv -= 20;
  return;
}
  
  
  
