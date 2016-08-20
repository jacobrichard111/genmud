#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"


/* Checks if a and b are pc's and if either one has the other (or self) as a leader, or they both have the same person as a leader. */


bool
in_same_group (THING *a, THING *b)
{
  if (a && b && IS_PC(a) && IS_PC (b) &&
      (((GLEADER(a) && (GLEADER(a) == GLEADER (b))) ||
	GLEADER(a) == b || GLEADER(b) == a) || a == b))
    return TRUE;
  return FALSE;
}


/* Can the joiner join a group lead by leader..based on the number in
   the group and the joiners remorts etc... */
bool
can_follow (THING *joiner, THING *leader)
{
  int points = 0;
  THING *th;
  VALUE *pet;

  /* Pc's and mobs must remain separate, unless they are pets. */


  if (!leader || !joiner)
    return FALSE;
  
  if ((pet = FNV (joiner, VAL_PET)) != NULL)
    {
      if (pet->word && *pet->word)
	{
	  if (!str_cmp (pet->word, NAME (leader)))
	    {
	      return TRUE;
	    }
	}
    }
  if (IS_PC (leader) != IS_PC (joiner) && LEVEL (joiner) < MAX_LEVEL)
    return FALSE;
  
  
  /* Mobs can always join. */
  
  
  if (!IS_PC (leader))
    return TRUE;
  
  
  /* Gotta be same align */
  
  if (DIFF_ALIGN (leader->align, joiner->align))
    return FALSE;
  
  
  /* Now add up points. You get 2 points just for existing. Then,
     you add remorts/4 to this to get your total points. This way,
     newbies are weighted heavier.... total points atm is 16? which
     allows for 8 newbies or 4 highbees. */

  for (th = thing_hash[PLAYER_VNUM % HASH_SIZE]; th; th = th->next)
    {
      if (in_same_group (th, leader))
	points += find_gp_points (th);
    }
  points += find_gp_points (joiner);
  
  if (points > MAX_GROUP_POINTS)
    {
      stt ("That bunch is already too powerful.\n\r", joiner);
      return FALSE;
    }

  /* So, at this point we have a pc of the same align as the leader, and
     the points are ok, so we allow them to start following. */
  
  return TRUE;
}



void
do_follow (THING *th, char *arg)
{
  THING *vict, *pth;
  char buf[STD_LEN];
  char arg1[STD_LEN];

  f_word (arg, arg1);
  if (!str_cmp (arg1, "me") || !str_cmp (arg1, NAME (th)))
    {
      if (th->fgt)
	{
	  if (th->fgt->following)
	    {
	      act ("@1n stop@s following @3n.", th, NULL, th->fgt->following, NULL, TO_ALL);
	      th->fgt->following = NULL;
	    } 
	  if (th->fgt->gleader)
	    {
	      sprintf (buf, "%s has left the group.", NAME (th));
	      group_notify (th, buf);
	      th->fgt->gleader = NULL;
	    }
	}
      return;
    }
  
  if ((vict = find_thing_in (th, arg1)) == NULL)
    {
      stt ("That person isn't here to follow.\n\r", th);
      return;
    }
  
  if (ignore (vict, th))
    {
      stt ("That person is ignoring you so you can't follow them.\n\r", th);
      return;
    }

  if (FOLLOWING (vict) || GLEADER(vict))
    {
      stt ("That person is already following someone else!\n\r", th);
      return;
    }


  if (IS_PC (th))
    {
      for (pth = thing_hash[PLAYER_VNUM % HASH_SIZE]; pth; pth = pth->next)
	{
	  if (pth->fgt && pth->fgt->following == th)
	    {
	      stt ("Someone is already following you!\n\r", th);
	      return;
	    }
	}
    }
  
  if (!can_follow (th, vict))
    {
      stt ("You can't seem to follow that person.\n\r", th);
      return;
    }
  
  if (!th->fgt)
    {
      add_fight (th);
    }
  if (th->fgt->following)
    {
      act ("@1n stop@s following @3n.", th, NULL, th->fgt->following, NULL, TO_ALL);	      
      th->fgt->following = NULL;
    }
  if (th->fgt->gleader)
    {
      sprintf (buf, "%s has left the group.", NAME (th));
      group_notify (th, buf);
      th->fgt->gleader = NULL;
    }
  
  th->fgt->following = vict;
  act ("@1n start@s following @3n.", th, NULL, vict, NULL, TO_ALL);
  return;
  }

/* Finds group points for this pc... */


int
find_gp_points (THING *th)
{
  if (!IS_PC (th))
    return 0;
  return 2 + th->pc->remorts/4;
}



void
do_group (THING *th, char *arg)
{
  THING *rth;
  char buf[STD_LEN];
  
  if (arg[0] == '\0')
    {
      stt ("[-Char Power-] --Name--     +-Health-+    *-Stamina-*      =-Exp2Level-=\n\r", th);
      
      for (rth = thing_hash[PLAYER_VNUM % HASH_SIZE]; rth; rth = rth->next)
	{
	  if (in_same_group (rth, th))
	    {
	      sprintf (buf, "[\x1b[1;37m%-12s\x1b[0;37m] %-14s \x1b[1;31m%-10s\x1b[0;37m   \x1b[1;34m%-10s\x1b[0;37m       (%d)\n\r", relative_power_to (rth, th), 
		       NAME (rth), hps_condit(rth), mvs_condit(rth),
		       (rth->pc ? exp_to_level (LEVEL(th)) - rth->pc->exp : 0));
	      stt (buf, th);
	    }
	}
      return;
    }


  if (!str_cmp (arg, "all"))
    {
      for (rth = thing_hash[PLAYER_VNUM % HASH_SIZE]; rth; rth = rth->next)
	{
	  if (rth->fgt)
	    {
	      if (FOLLOWING (rth) == th && GLEADER (rth) != th)
		{
		  rth->fgt->gleader = th;
		  sprintf (buf, "%s joins the group.", NAME (rth));
		  group_notify (rth, buf);
		}
	    }
	}
      return;
    }
  
  if ((rth = find_pc (th, arg)) == NULL)
    {
      stt ("That person is not around.\n\r", th);
      return;
    }
  if (FOLLOWING (rth) != th)
    {
      stt ("That person is not following you.\n\r", th);
      return;
    }
  if (GLEADER (rth) == th)
    {
      stt ("That person is already in your group!\n\r", th);
      return;
    }
  
  rth->fgt->gleader = th;
  sprintf (buf, "%s joins the group.", NAME (rth));
  group_notify (rth, buf);
  return;
}


void
do_ditch (THING *th, char *arg)
{
  THING *rth;
  char buf[STD_LEN];
  
  if (!IS_PC (th))
    return;
  
  if (!str_cmp (arg, "all"))
    {  
      for (rth = thing_hash[PLAYER_VNUM % HASH_SIZE]; rth; rth = rth->next)
	{
	  if (rth->fgt)
	    {
	      if (rth->fgt->following == th)
		{
		  act ("@1n stop@s following @3n.", th, NULL, rth->fgt->following, NULL, TO_ALL);	      
		  rth->fgt->following = NULL;
		}
	      if (rth->fgt->gleader == th)
		{
		  sprintf (buf, "%s has left the group.", NAME (rth));
		  group_notify (rth, buf);
		  rth->fgt->gleader = NULL;
		}
	    }
	}
      return;
    }
  
  if ((rth = find_pc (th, arg)) == NULL)
    {
      stt ("That person isn't here to ditch!\n\r", th);
      return;
    }
  
  if (rth->fgt)
    {
      if (rth->fgt->following == th)
	{
	  act ("@1n stop@s following @3n.", th, NULL, rth->fgt->following, NULL, TO_ALL);	      
	  rth->fgt->following = NULL;
	}
      else
	{
	  stt ("That person isn't following you!\n\r", th);
	  return;
	}
      if (rth->fgt->gleader == th)
	{
	  sprintf (buf, "%s has left the group.", NAME (rth));
	  group_notify (rth, buf);
	  rth->fgt->gleader = NULL;
	}
      else
	{
	  stt ("That person isn't in your group!\n\r", th);
	  return;
	}
    }
  else
    {
      stt ("That person isn't following you!\n\r", th);
      return;
    }
  return;
}
  



void
group_notify (THING *th, char *msg)
{
  FILE_DESC *fd;
  char buf[STD_LEN];
  bool is_in_group = FALSE;
  if (!IS_PC (th) || !msg || !*msg)
    return;

  for (fd = fd_list; fd; fd = fd->next)
    {
      if (fd->th && fd->th != th &&
	  in_same_group (fd->th, th))
	{
	  is_in_group = TRUE;
	  break;
	}
    }
  
  if (!is_in_group)
    return;


  sprintf (buf, "\x1b[1;33m* Group Notify: %s\x1b[0;37m\n\r", msg);
  


  for (fd = fd_list; fd; fd = fd->next)
    {
      if (fd->th && in_same_group (th, fd->th))
	{
	  stt (buf, fd->th);
	}
    }
  return;
}

/* Shows your hps condition in the group command. */
char *
hps_condit (THING *th)
{
  if (th->max_hp < 1)
    return "Error";
  
  if (th->hp >= th->max_hp)
    return "fine";
  else if (th->hp >= th->max_hp/3*2)
    return "good";
  else if (th->hp >= th->max_hp/3)
    return "fair";
  else if (th->hp >= th->max_hp/10)
    return "bad";
  return "awful";
}

/* Shows your movement left */


char *
mvs_condit (THING *th)
{
  if (th->max_mv < 1)
    return "Error";
  
  
  if (th->mv >= th->max_mv)
    return "excellent";
  else if (th->mv >= th->max_mv *3/4)
    return "good";
  else if (th->mv >= th->max_mv/2)
    return "tired";
  else if (th->mv >= th->max_mv/4)
    return "winded";
  return "fainty";
}

/* This shows your relative power level. */
  
char *
relative_power_to (THING *th, THING *observer)
{
  static char buf[STD_LEN];
  if (!IS_PC (th))
    return "Mob";

  if (th == observer)
    {
      sprintf (buf, "%d", LEVEL(th));
      return buf;
    }
  
  if (LEVEL (th) < 9)
    return "Inexperienced";
  else if (LEVEL (th) < 26)
    return "Knowledgeable";
  else if (LEVEL (th) < 45)
    return "Well-Known";
  else if (LEVEL (th) < 73)
    return "Famous";
  else if (LEVEL (th) < 90)
    return "Hero";
  else if (LEVEL (th) < BLD_LEVEL)
    return "Legend";
  else if (LEVEL (th) < MAX_LEVEL)
    return "Builder";
  return "Admin";
}

 
