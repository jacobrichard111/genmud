#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* There are 4 "parts", and 7 tiers of implants. So, there are a total
   of 28*4 = 112 implant points needed to max all of them out. You will
   get 6 points per remort to spend on this. So, you get 66 total, which
   means you can roughly max out 2 of the implants, or get all of them
   up to about level 4-5? This code is basically warmed over guild code
   :P */

/* This totals up the number of points you have spent on implants 
   already. */

int
total_implants (THING *th)
{
  int i, total = 0;
  if (!th || !IS_PC (th))
    return 0;  
  for (i = 0; i < PARTS_MAX; i++)
    total += (th->pc->implants[i]  * (th->pc->implants[i] + 1))/2;
  return total;
}

/* This gives the total number of implant points a pc should have at any
   moment. */

int
total_implant_points (THING *th)
{
  if (!th || !IS_PC (th))
    return 0;

  return (IMPLANT_POINTS_PER_REMORT + (IS_PC1_SET (th, PC_ASCENDED) ? 3 : 0))*(th->pc->remorts + 1);
}
/* Gives the level of implant you have of this type. */

int 
implant_rank (THING *th, int type)
{
  if (!th || !IS_PC (th))
    return IMPLANT_TIER_MAX;
  if (type < 0 || type >= PARTS_MAX)
    return 0;
  
  return th->pc->implants[type];
}

/* This is the actual implant command. */

void
do_implant (THING *th, char *arg)
{
  char buf[STD_LEN];
  char arg1[STD_LEN];
  int i = 0, total = 0, curr_wps, curr_money, curr_points, th_wps, th_money,
    th_points, remorts, type = PARTS_MAX, new_tier;
  if (!th || !IS_PC (th) || !th->in)
    return;
  
  total = total_implants (th);
  remorts = th->pc->remorts;
  th_points = total_implant_points (th) - total;
  th_money = total_money (th);
  th_wps = th->pc->pk[PK_WPS];

  if (!*arg || !str_cmp (arg, "list"))
    {      
      stt ("A list of the implants and brief descriptions:\n\n\r", th);      
      for (i = 0; i < PARTS_MAX; i++)
	{
	  sprintf (buf, "[%2d] (Tier %d) %s\n\r", 
		   i + 1, th->pc->implants[i], implant_descs[i]);
	  stt (buf, th);
	}      
      sprintf (buf, "You have used a total of %d implant points, and have %d left over for more implants this remort.\n\r", total, th_points);
      stt (buf, th);
      return;
    }
  
  if (!str_cmp (arg, "costs"))
    {
       stt ("If your total number of remorts is R, and the tier you wish to implant is T, you need T implant points, 50*T*R warpoints to advance, and 500*T*(R + 1)*(R + 1) coins to advance.\n\r", th);
      stt ("So for example your current costs are:\n\r", th);
      for (i = 0; i < IMPLANT_TIER_MAX; i++)
	{
	  curr_wps = IMPLANT_WPS_COST * (i + 1) * remorts;
	  curr_money = IMPLANT_MONEY_COST*(i + 1)*(remorts + 1) * (remorts 
+ 1);
	  curr_points = i + 1;
	  sprintf (buf, "Tier %d: \x1b[1;3%dm%d\x1b[0;37m implant point%s, \x1b[1;3%dm%d\x1b[0;37m warpoints, \x1b[1;3%dm%d\x1b[0;37m coins.\n\r",
		   i + 1,
		   (1 + (th_points > curr_points + i + 1)), curr_points, 
		   (curr_points == 1 ? " " : "s"), 
		   (1 + (th_wps > curr_wps)), curr_wps,
		   (1 + (th_money > curr_money)), curr_money);
	  stt (buf, th);
	}
      sprintf (buf, "\n\rYou have %d implant points, %d warpoints, and %d coins on you.\n\r",th_points, th_wps, th_money);
      stt (buf, th);
      return;
    }
  

  if (!th->in || th->in->vnum != IMPLANT_ROOM_VNUM)
    {
      stt ("You aren't in the implant room!\n\r", th);
      return;
    }
  
  arg = f_word (arg, arg1);
  
  for (type = 0; type < PARTS_MAX; type++)
    {
      if (!str_cmp (arg1, parts_names[type]))
	break;
    }
  
  if (type == PARTS_MAX)
    {
      stt ("Implant <location> or implant <location> remove\n\r", th);
      return;
    }
  
  /* Allow people to remove implant ranks. */
  if (!str_cmp (arg, "remove"))
    {
      if (th->pc->implants[type] < 1)
	{
	  stt ("You don't have an implant of that type.\n\r", th);
	  return;
	}
      
      th->pc->implants[type]--;
      stt ("Implant rank reduced.\n\r", th);
      return;
    }


  

  new_tier = th->pc->implants[type] + 1;


  if (new_tier > IMPLANT_TIER_MAX)
    {
      stt ("This implant is as powerful as we can make it!\n\r", th);
      return;
    }
      
  curr_wps = IMPLANT_WPS_COST * (new_tier) * remorts;
  curr_money = IMPLANT_MONEY_COST*(new_tier)*(remorts + 1) * (remorts + 1);
  curr_points = new_tier;
  
  if (th_points < curr_points)
    {
      stt ("You don't have enough implant points left to get this implant!\n\r", th);
      return;
    }
  if (th_wps < curr_wps)
    {
      stt ("You don't have enough warpoints to get this implant!\n\r", th);
      return;
    }
  if (th_money < curr_money)
    {
      stt ("You don't have enough money to get this implant!\n\r", th);
      return;
    }

  sub_money (th, curr_money);
  th->pc->pk[PK_WPS] -= curr_wps;
  th->pc->implants[type]++;
  
  act ("@1n just got @1s @t implant upgraded.", th, NULL, NULL, (char *) parts_names[type], TO_ALL);
  update_kde (th, KDE_UPDATE_IMPLANT);
  return;
}

















