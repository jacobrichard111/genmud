#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* These have a lot less error checking than the rest of the I/O functions
   since we have a fixed length struct and a fixed number of them to deal
   with each time. This makes it a leetle faster writing this. */

void
read_pkdata (void)
{
  char word[STD_LEN];
  FILE *f;
  int i, j, num;
  if ((f = wfopen ("pkdata.dat", "r")) == NULL)
    return;
  
  for (i = 0; i < PK_MAX; i++)
    {
      for (j = 0; j < PK_LISTSIZE; j++)
	{
	  strcpy (word, read_word (f));
	  word[29] = '\0';
	  strcpy (pkd[i][j].name, word);
	  num = read_number (f);
	  pkd[i][j].value = (float) num;
	  pkd[i][j].value /= 10000.0;
	  pkd[i][j].align = read_number (f);
	}
    }
  fclose (f);
  return;
}

void
write_pkdata (void)
{
  int i, j;
  FILE *f;
  if ((f = wfopen ("pkdata.dat", "w")) == NULL)
    return;

  
  for (i = 0; i < PK_MAX; i++)
    {
      for (j = 0; j < PK_LISTSIZE; j++)
	{
	  fprintf (f, "%s %d %d\n", pkd[i][j].name, 
		   (int) (pkd[i][j].value * 10000),
		   pkd[i][j].align);
	}
    }
  fclose (f);
  return;
}

/* This finds a "rating" for your character based on your various
   stats. Plz don't bitch at me if you think it isn't "perfect",
   since you are free to take this code and make your own
   game and make your perfect rating code. */


int
find_rating (THING *th)
{
  int rating = 0, i;
  THING *obj;
  VALUE *val;
  
  if (!IS_PC (th))
    return 0;
  
  rating += LEVEL (th)/3;
  rating += th->pc->remorts * 5;
  
  /* Add stats, implants, armor */

  for (i = 0; i < PARTS_MAX; i++)
      rating += get_stat (th, i) +
      th->pc->implants[i] * 2 + th->pc->armor[i]/6;
  
  /* Add guilds */

  for (i = 0; i < GUILD_MAX; i++)
    rating += th->pc->guild[i] * 4;
  
  /* Add battle stats */

  rating += get_hitroll(th)/4;
  rating += get_damroll(th)/2;
  rating += get_evasion(th);

  rating += th->max_hp/25 + th->max_mv/10 + th->pc->max_mana/5;
  
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if (!IS_WORN(obj))
	break;
      
      for (val = obj->values; val; val = val->next)
	{
	  if (val->type == VAL_POWERSHIELD)
	    rating += val->val[2]/10;
	  else if (val->type == VAL_WEAPON)
	    rating += (val->val[0] + val->val[1])*2 + 
	      (val->val[5] != 0 ? 10 : 0) /* poison */ +
	      val->val[3] * 15 /* speed */;
	  else if (val->type == VAL_GEM)
	    {
	      for (i = 0; i < MANA_MAX; i++)
		if (val->val[0] & (1 << i))
		  rating += 5; /* 5 pts per color */
	      rating += val->val[3]; /* Level */
	      rating += val->val[2]/3; /* amt of mana */
	    }
	  else if (val->type == VAL_RANGED)
	    {
	      rating += val->val[2] * 5; /* Range */
	      rating += val->val[1] * 4; /* Num shots */
	      rating += val->val[3]; /* Multiplier */
	    }
	}
    }
  return rating;
}

void
update_pkdata (THING *th)
{
  int i, j, k;
  float curr_val[PK_MAX];

  if (!IS_PC (th) || LEVEL (th) >= BLD_LEVEL)
    return;


  for (i = 0; i < PK_MAX; i++)
    {
      curr_val[i] = 0.0;
      curr_val[i] = (float) th->pc->pk[i];
    }
  
  /* Pkstat ave and pkstat num are levels or helpers / number of pkills..
     if any. */

  if (th->pc->pk[PK_PKILLS] > 1.0)
    {
      curr_val[PK_LEVELS] /= th->pc->pk[PK_PKILLS];
      
      curr_val[PK_HELPERS] /= th->pc->pk[PK_PKILLS];;
      curr_val[PK_BGPKW] = th->pc->pk[PK_HELPERS]/th->pc->pk[PK_PKILLS];
    }
  else
    {
      curr_val[PK_LEVELS] = 0.0;
      curr_val[PK_HELPERS] = 0.0;
    }
  
  curr_val[PK_RATING] = find_rating (th);
  
  /* Clear the old name off the list */

  for (i = 0; i < PK_MAX; i++)
    {
      for (j = 0; j < PK_LISTSIZE; j++)
	{
	  if (!str_cmp (pkd[i][j].name, NAME (th)))
	    {
	      /* Move stuff up */
	      for (k = j; k < PK_LISTSIZE - 1; k++)
		{
		  strcpy (pkd[i][k].name, pkd[i][k + 1].name);
		  pkd[i][k].value = pkd[i][k + 1].value;
		  pkd[i][k].align = pkd[i][k + 1].align;
		}
	      strcpy (pkd[i][PK_LISTSIZE - 1].name, "<Free-Slot>");
	      pkd[i][PK_LISTSIZE - 1].value = 0.0;
	      pkd[i][PK_LISTSIZE - 1].align = 0;
	      break;
	    }
	}
    }

  /* Now find the new slot (if any) and put the name in. */
  
  for (i = 0; i < PK_MAX; i++)
    {
      for (j = 0; j < PK_MAX; j++)
	{
	  if (curr_val[i] > 0.0 && 
	      ((i != PK_HELPERS && curr_val[i] > pkd[i][j].value) ||
	       (i == PK_HELPERS && curr_val[i] < pkd[i][j].value)))
	    {
	      for (k = PK_LISTSIZE -1; k > j; k--)
		{
		  strcpy (pkd[i][k].name, pkd[i][k - 1].name);
		  pkd[i][k].value = pkd[i][k - 1].value;
		  pkd[i][k].align = pkd[i][k - 1].align;
		}
	      strcpy (pkd[i][j].name, NAME (th));
	      pkd[i][j].value = curr_val[i];
	      pkd[i][j].align = th->align;
	      break;
	    }
	}
    }
  if (nr (1,20) == 2)
    write_pkdata ();
  return;
}


void
update_trophies (THING *th, THING *vict, int num_points, int lev_helpers)
{
  int wps, i, k, old_times = 0, new_lev, new_rem;
  
  /* Must be pkill */
  
   if (!th || !vict || !IS_PC (th) || !IS_PC (vict) || !DIFF_ALIGN
       (th->align, vict->align) || LEVEL (th) >= BLD_LEVEL || LEVEL
       (vict) >= BLD_LEVEL)
    return;

  th->pc->pk[PK_HELPERS] += num_points;
  th->pc->pk[PK_LEVELS] += LEVEL (vict);
  th->pc->pk[PK_PKILLS]++;
  if (num_points == find_gp_points (th) &&
      vict->pc->remorts >= th->pc->remorts &&
      vict->pc->remorts > 0 &&
      LEVEL (vict) >= LEVEL (th))
    th->pc->pk[PK_SOLO]++;
  
  wps = LEVEL (vict)/3;

  /* Some stuff on warpoints for groupsize and relative remorts and levels. */
  
  if (SACRIFICING)
    wps *= 5;
  else if (num_points > MAX_GROUP_POINTS)
    wps = wps * (MAX_GROUP_POINTS)/num_points;
  
  wps += LEVEL (vict)/30;
  
  wps += vict->pc->remorts/4;
  
  wps += (vict->pc->remorts - th->pc->remorts) * 2;
  
  wps += (LEVEL (vict) - LEVEL (th))/5;
  
  /* If we get 0 wps, we don't get a trophy :) */
  
  if (wps < 1)
    return;
  
  th->pc->pk[PK_WPS] += wps;
  th->pc->pk[PK_TOT_WPS] += wps;
  
  /* Remove the old trophy from the list */
  
  new_lev = LEVEL (vict);
  new_rem = vict->pc->remorts;
  
  for (i =0; i < MAX_TROPHY; i++)
    {
      if (!str_cmp (th->pc->trophy[i]->name, NAME (vict)))
	{
	  old_times = th->pc->trophy[i]->times;
	  for (k = i; k < MAX_TROPHY - 1; k++)
	    {
	      strcpy (th->pc->trophy[k]->name, th->pc->trophy[k + 1]->name);
	      th->pc->trophy[k]->level = th->pc->trophy[k + 1]->level;
	      th->pc->trophy[k]->remorts = th->pc->trophy[k + 1]->remorts;
	      th->pc->trophy[k]->times = th->pc->trophy[k + 1]->times;
	      th->pc->trophy[k]->align = th->pc->trophy[k + 1]->align;
	      th->pc->trophy[k]->race = th->pc->trophy[k + 1]->race;
	    }
	  bzero (th->pc->trophy[MAX_TROPHY -1], sizeof (TROPHY));
	  break;
	}
    }
  
  /* Now see where the new trophy goes. Add it if the remorts are >
   than the remort being checked or if the remorts are the same and
   the level is >= the prev level. */
  
  for (i =0; i < MAX_TROPHY; i++)
    {
      if (new_rem > th->pc->trophy[i]->remorts ||
	  (new_rem == th->pc->trophy[i]->remorts &&
	   new_lev > th->pc->trophy[i]->level))
	{
	  for (k = MAX_TROPHY -1; k > i; k--)
	    {
	      strcpy (th->pc->trophy[k]->name, th->pc->trophy[k - 1]->name);
	      th->pc->trophy[k]->level = th->pc->trophy[k - 1]->level;
	      th->pc->trophy[k]->remorts = th->pc->trophy[k - 1]->remorts;
	      th->pc->trophy[k]->times = th->pc->trophy[k - 1]->times;
	      th->pc->trophy[k]->align = th->pc->trophy[k - 1]->align;
	      th->pc->trophy[k]->race = th->pc->trophy[k - 1]->race;
	    }
	  strcpy (th->pc->trophy[i]->name, NAME (vict));
	  th->pc->trophy[i]->level = LEVEL (vict);
	  th->pc->trophy[i]->remorts = vict->pc->remorts;
	  th->pc->trophy[i]->align = vict->align;
	  th->pc->trophy[i]->race = RACE(vict);
	  th->pc->trophy[i]->times = ++old_times;
	  break;
	}
    }
  update_pkdata (th);
  return;
}


void
do_rating (THING *th, char *arg)
{
  do_pkdata (th, "rating");
  return;
}

void 
do_topten (THING *th, char *arg)
{
  do_pkdata (th, "wps");
  return;
}

void
do_pkdata (THING *th, char *arg)
{
  char buf[STD_LEN];
  char toptext[STD_LEN];
  char bottomtext[STD_LEN];
  char buf2[STD_LEN];
  bool is_int = FALSE;
  int i, type;

  if (!IS_PC (th))
    return;

  update_pkdata (th);
  
  if (!arg || !*arg)
    {
      stt ("pkdata wps, twps, lev, num, pkills, pkilled, killed, solo, bgpkw brigack\n\r", th);
      return;
    }
  
  if (!str_cmp (arg, "wps"))
    {
      sprintf (toptext ,"Warpoints gained through pkill:\n\r");
      sprintf (bottomtext, "Your warpoints: %d\n\r", (int)th->pc->pk[PK_WPS]);
      is_int = TRUE;
      type = PK_WPS;
    }
  else if (!str_cmp (arg, "twps"))
    {
      sprintf (toptext ,"Total warpoints gained through pkill:\n\r");
      sprintf (bottomtext, "Your total warpoints: %d\n\r", (int)th->pc->pk[PK_TOT_WPS]);
      is_int = TRUE;
      type = PK_TOT_WPS;
    }
  else if (!str_cmp (arg, "rating"))
    {
      sprintf (toptext ,"Top ratings in the game:\n\r");
      sprintf (bottomtext, "Your rating: %d\n\r", find_rating (th));
      is_int = TRUE;
      type = PK_RATING;
    }
      
  else if (!str_cmp (arg, "pkills"))
    {
      sprintf (toptext ,"Total number of pkills:\n\r");
      sprintf (bottomtext, "Your total pkills: %d\n\r", (int)th->pc->pk[PK_TOT_WPS]);
      is_int = TRUE;
      type = PK_PKILLS;
    }
  else if (!str_cmp (arg, "killed"))
    {
      sprintf (toptext ,"Largest number of deaths:\n\r");
      sprintf (bottomtext, "Your total deaths: %d\n\r", (int)th->pc->pk[PK_KILLED]);
      is_int = TRUE;
      type = PK_KILLED;
    }
  else if (!str_cmp (arg, "pkilled"))
    {
      sprintf (toptext ,"People pkilled the most:\n\r");
      sprintf (bottomtext, "Your total pdeaths: %d\n\r", (int)th->pc->pk[PK_PKILLED]);
      is_int = TRUE;
      type = PK_PKILLED;
    }
  else if (!str_cmp (arg, "solo"))
    {
      sprintf (toptext ,"Top solo hard pkillers:\n\r");
      sprintf (bottomtext, "Your total solo hard pkills %d\n\r", (int)th->pc->pk[PK_SOLO]);
      is_int = TRUE;
      type = PK_SOLO;
    }
  else if (!str_cmp (arg, "lev"))
    {
        sprintf (toptext ,"Top average level vs number of pkills:\n\r");
	sprintf (bottomtext, "Your average pkill level vs number of pkills: %d\n\r", th->pc->pk[PK_PKILLS] > 0 ? (int) (th->pc->pk[PK_LEVELS]/th->pc->pk[PK_PKILLS]) : 0);
      is_int = FALSE;
      type = PK_LEVELS;
    }
  
  else if (!str_cmp (arg, "num"))
    {
      sprintf (toptext ,"Lowest number of group points needed for a pkill:\n\r");
      sprintf (bottomtext, "Your average number of group points per pkill: %d\n\r", th->pc->pk[PK_PKILLS] > 0 ? (int) (th->pc->pk[PK_HELPERS]/th->pc->pk[PK_PKILLS]) : 0);
      is_int = FALSE;
      type = PK_HELPERS;
    }
  else if (!str_cmp (arg, "bgpkw"))
    {
      sprintf (toptext ,"Highest number of group points needed for a pkill:\n\r");
      sprintf (bottomtext, "Your average number of group points per pkill: %d\n\r", th->pc->pk[PK_PKILLS] > 0 ? (int) (th->pc->pk[PK_HELPERS]/th->pc->pk[PK_PKILLS]) : 0);
      is_int = FALSE;
      type = PK_BGPKW;
    } 
  else if (!str_cmp (arg, "brigack"))
    {
      sprintf (toptext ,"Highest brigack.\n\r");
      sprintf (bottomtext, "Your brigack is: %d\n\r", th->pc->pk[PK_BRIGACK]);
      is_int = TRUE;
      type = PK_BRIGACK;
    }
  else
    {
      stt ("pkdata wps, twps, lev, num, pkills, pkilled, killed, solo, bgpkw brigack\n", th);
      return;
    }
  
  sprintf (buf, "\x1b[1;36m%s\x1b[0;37m", toptext);
  stt (buf, th);
  for (i = 0; i < PK_LISTSIZE; i++)
    {
      if (is_int)
	sprintf (buf2, "%d", (int) pkd[type][i].value);
      else
	sprintf (buf2, "%2.4f", pkd[type][i].value);
      sprintf (buf, "\x1b[1;3%dm*\x1b[1;35m%-2d \x1b[1;36m%-40s  \x1b[1;37m-------> \x1b[1;31m%s\x1b[0;37m\n\r", pkd[type][i].align % 8, i + 1,
	       pkd[type][i].name, buf2);
      stt (buf, th);
    }
  
  sprintf (buf, "\n\r\x1b[1;34m%s\x1b[0;37m", bottomtext);
  stt (buf, th);
  return;
}


void
do_trophy (THING *th, char *arg)
{
  int i;
  char buf[STD_LEN];
  bool shown = FALSE;
  RACE *race, *align;
  int racenum, alignnum;
  if (!IS_PC (th))
    return;
  
  bigbuf[0] = '\0';
  add_to_bigbuf ("\x1b[1;37m A list of the most powerful enemies you have vanquished:\x1b[0;37m\n\n\r");
  
  
  for (i = 0; i < MAX_TROPHY; i++) 
    {
      if (th->pc->trophy[i] && th->pc->trophy[i]->name[0] != '\0')
	{
	  alignnum = th->pc->trophy[i]->align;
	  racenum = th->pc->trophy[i]->race;
	  sprintf (buf, "\x1b[0;36m[x%d]%s \x1b[1;33m%-20s\x1b[0;37m the level \x1b[1;32m%d\x1b[0;37m %s %s with \x1b[1;31m%d\x1b[0;37m remort%s.\n\r", 
		   th->pc->trophy[i]->times,
		   (th->pc->trophy[i]->times < 10 ?  "  " :
		    th->pc->trophy[i]->times < 100 ? " ": ""),
		   th->pc->trophy[i]->name,
		   th->pc->trophy[i]->level,
		   ((align = find_align (NULL, alignnum)) != NULL ?
		    align->name : "No Align"),
		   ((race = find_race (NULL, racenum)) != NULL ?
		    race->name : "No Race"),
		   th->pc->trophy[i]->remorts,
		   th->pc->trophy[i]->remorts != 1 ? "s" : "");
	  add_to_bigbuf (buf);
	  shown = TRUE;
	}
    }
  if (!shown)
    {
      add_to_bigbuf ("None.\n\r");
    }
  
  send_bigbuf (bigbuf, th);
  return;
}
