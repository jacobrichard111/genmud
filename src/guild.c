#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"


/* Total of the guild points the player has used. */
int
total_guilds (THING *th)
{
  int i, total = 0;
  if (!th || !IS_PC (th))
    return 0;  
  for (i = 0; i < GUILD_MAX; i++)
    total += (th->pc->guild[i]  * (th->pc->guild[i] + 1))/2;
  return total;
}

/* How many guild points the player has to play with. */

int 
total_guild_points (THING *th)
{
  if (!th || !IS_PC (th))
    return 0;
  
  return (GUILD_POINTS_PER_REMORT + (IS_PC1_SET (th, PC_ASCENDED) ? 3 : 0))*(th->pc->remorts + 1);
}

/* Do we increase the guild stat value this tier? */

bool
guild_stat_increase (int tier)
{
  if (tier == 3 || tier == GUILD_TIER_MAX)
    return TRUE;
  return FALSE;
}
void
do_guild (THING *th, char *arg)
{
  char buf[STD_LEN];
  THING *gm;
  VALUE *guild = NULL;
  int i, total = 0, curr_wps, curr_money, curr_points, th_wps, th_money,
    th_points, remorts, type = GUILD_MAX, new_tier;
  bool found = FALSE, removed_spell = FALSE;
  if (!th || !IS_PC (th) || !th->in)
    return;
  
  total = total_guilds (th);
  remorts = th->pc->remorts;
  th_points = total_guild_points (th) - total;
  th_money = total_money (th);
  th_wps = th->pc->pk[PK_WPS];
  
  

  if (!*arg || !str_cmp (arg, "list") || !str_cmp (arg, "info"))
    {      
      stt ("A list of the guilds and brief descriptions:\n\n\r", th);      
      for (i = 0; i < GUILD_MAX; i++)
	{
	  sprintf (buf, "Tier %d:%-18s (%s) %s\n\r", 
		   th->pc->guild[i], 
		   guild_info[i].app, 
                   (guild_info[i].flagval >= 0 && 
                    guild_info[i] .flagval< STAT_MAX ?
                   stat_short_name[guild_info[i].flagval] : "   "),
                   guild_info[i].help );
	  stt (buf, th);
	  
	}      
      sprintf (buf, "You have used a total of %d guild points, and have %d left over for more guild advancement this remort.\n\r", total, th_points);
      stt (buf, th);
      return;
    }
  
  if (!str_cmp (arg, "costs"))
    {
      stt ("If your total number of remorts is R, and the tier you wish to advance to in the guild is T, you need T guild points, 100*T*R warpoints to advance, and 1000*T*(R + 1)*(R + 1) coins to advance.\n\r", th);
      stt ("So for example your current costs are:\n\r", th);
      for (i = 0; i < GUILD_TIER_MAX; i++)
	{
	  curr_wps = 100 * (i + 1) * remorts;
	  curr_money = 1000*(i + 1)*(remorts + 1) * (remorts + 1);
	  curr_points = i + 1;
	  sprintf (buf, "Tier %d: \x1b[1;3%dm%d\x1b[0;37m guild point%s, \x1b[1;3%dm%d\x1b[0;37m warpoints, \x1b[1;3%dm%d\x1b[0;37m coins.\n\r",
		   i + 1, 
		   (1 + (th_points > curr_points + i + 1)), curr_points,
		   (curr_points == 1 ? " " : "s"),
		   (1 + (th_wps > curr_wps)), curr_wps,
		   (1 + (th_money > curr_money)), curr_money);
	  stt (buf, th);
	}
      sprintf (buf, "\n\rYou have %d guild points, %d warpoints, and %d coins on you.\n\r",th_points, th_wps, th_money);
      stt (buf, th);
      return;
    }
  
  for (gm = th->in->cont; gm && !found; gm = gm->next_cont)
    {
      if (IS_PC (gm) ||
	  (guild = FNV (gm, VAL_GUILD)) == NULL ||
	  guild->val[0] < 1 || guild->val[0] > GUILD_MAX)
	continue;      
      type = guild->val[0] -1;
      break;
    }
  
  if (!gm || type == GUILD_MAX)
    {
      stt ("There is no guildmaster here!\n\r", th);
      return;
    }
  
  /* Remove a guild tier....no wps/gold returned and all skills are
     checked to make sure you can still use them with new lower guild
     tier. Since guild points are dynamic, they are "returned" so
     you can choose different guilds. */
  
  if (!str_cmp (arg, "remove"))
    {
      SPELL *spl;
      if (th->pc->guild[type] < 1)
	{
	  stt ("You aren't in this guild!\n\r", th);
	  return;
	}
      /* Remove the guild bonus stat...*/
      
      if (guild_info[type].flagval >= 0 &&
	  guild_info[type].flagval < STAT_MAX &&
	  guild_stat_increase (th->pc->guild[type]))
	th->pc->stat[guild_info[type].flagval]--;

      /* Remove the guild tier. */
      th->pc->guild[type]--;
      stt ("Guild tier removed.\n\r", th);
      /* Update spells... loop through and check to see if you need
	 to unlearn spells due to losing the guild tier. */
      
      do
	{
	  removed_spell = FALSE;
	  for (spl = spell_list; spl; spl = spl->next)
	    {
	      if (spl->vnum < 0 || spl->vnum >= MAX_SPELL)
		continue;
	      
	      /* If the guild tier for the spell is higher than the
		 pc guild tier, remove the spell. */
	      
	      if (spl->guild[type] > th->pc->guild[type] &&
		  th->pc->prac[spl->vnum] > 0)
		{
		  th->pc->prac[spl->vnum] = 0;
		  removed_spell = TRUE;
		}
	      
	      /* Now check all prereqs. Loop through prereqs and if
		 the spell is missing some prereqs then this spell is
		 zeroed out also. This is needed because a spell may
		 be removed due to guilds and it may be a prereq for
		 another spell that doesn't have a guild tier that
		 high. It's a check against funky spell creation. */
	      
	      if (th->pc->prac[spl->vnum] > 0)
		{
		  for (i = 0; i < NUM_PRE; i++)
		    {
		      if (spl->prereq[i] && 
			  spl->prereq[i]->vnum >= 0 &&
			  spl->prereq[i]->vnum < MAX_SPELL &&
			  th->pc->prac[spl->prereq[i]->vnum] == 0)
			{
			  th->pc->prac[spl->vnum] = 0;
			  removed_spell = TRUE;
			}
		    }
		}
	    }
	}
      while (removed_spell);
      return;
    }
  if (str_cmp (arg, "advance"))
    {
      stt ("Guild info, costs, advance.\n\r", th);
      return;
    }
  
  new_tier = th->pc->guild[type] + 1;

  /* If you split up guildmasters by tier.... */

  if (guild->val[2] > 0 && 
      (new_tier < guild->val[1] || new_tier > guild->val[2]))
    {
      sprintf (buf, "This guildmaster can only advance you into tiers %d through %d.\n\r", guild->val[1], guild->val[2]);
      stt (buf, th);
      return;
    }


  if (new_tier > GUILD_TIER_MAX)
    {
      stt ("You have already advanced as high as you can go in this guild!\n\r", th);
      return;
    }
  
  curr_wps = 100 * (new_tier) * remorts;
  curr_money = 1000*(new_tier)*(remorts + 1) * (remorts + 1);
  curr_points = new_tier;
  
  if (th_points < curr_points)
    {
      stt ("You don't have enough guild points left to advance this high!\n\r", th);
      return;
    }
  if (th_wps < curr_wps)
    {
      stt ("You don't have enough warpoints to advance this high!\n\r", th);
      return;
    }
  if (th_money < curr_money)
    {
      stt ("You don't have enough money to advance this high!\n\r", th);
      return;
    }
  sub_money (th, curr_money);
  th->pc->pk[PK_WPS] -= curr_wps;
  th->pc->guild[type]++;

  /* Stats for even numbered guild tiers... */

  if (guild_info[type].flagval >= 0 && 
      guild_info[type].flagval < STAT_MAX &&
      guild_stat_increase (new_tier))
    th->pc->stat[guild_info[type].flagval]++;
  act ("Congratulations @3n, just advanced a tier in the @t!", gm, NULL, th, guild_info[type].app, TO_ALL);
  update_kde (th, KDE_UPDATE_GUILD | KDE_UPDATE_STAT | KDE_UPDATE_COMBAT);
  return;
}

int 
find_guild_name (char *arg)
{
  int guild;
  for (guild = 0; guild < GUILD_MAX; guild++)
    if (!str_prefix (arg, guild_info[guild].name))
      return guild;
  return guild;
}

int
guild_rank (THING *th, int guild_num)
{
  if (!th || !IS_PC (th))
    return GUILD_TIER_MAX;  
  if (guild_num < 0 || guild_num >= GUILD_MAX)
    return 0;  
  return (th->pc->guild[guild_num]);
}
