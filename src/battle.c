#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>
#include"serv.h"


/* Update the battleground hour by hour. */

void
update_bg_hour (void)
{
  THING *player;
  
  /* Reduce BG hours to 0, and show the update status until we get to 0
     hours. */
  
  if (bg_hours <= 0)
    bg_hours = 0;
  if (--bg_hours > 0)
    {
      show_bg_status();
      return;
    }
  
  /* At this point, bg_hours just got set to 0. We now transfer everyone
     who wants to BG into the BG. */

  for (player = thing_hash[PLAYER_VNUM % HASH_SIZE]; player; player = player->next)
    {
      if (IS_PC (player) && player->pc->battling &&
	  LEVEL (player) >= bg_minlev &&
	  LEVEL (player) <= bg_maxlev)
	{
	  act ("@1n disappear@s in a puff of smoke!", player, NULL, NULL, NULL, TO_ALL);
	  /* They get moved to a random room in the BG. If you are
	     missing rooms, this will screw things up, so be careful
	     about these numbers and the rooms between them. */
	  
	  thing_to (player, find_thing_num (nr (BG_MIN_VNUM, BG_MAX_VNUM)));
	  act ("@1n appear@s in a bright flash!", player, NULL, NULL, NULL, TO_ALL);
	}
    }
  
  bg_message ("The battleground has begun!\n\r");
  return;
}


/* This sends out a message to everyone regarding the bg. The message
   gets wrapped in two ugly banners declaring that it's a battleground 
   message. */

void 
bg_message (char *txt)
{
  if (!txt || !*txt)
    return;
  
  echo ("\x1b[1;32m******** \x1b[1;31mThe Battleground\x1b[1;32m ********\x1b[0;37m\n\r");
  echo (txt); 
  echo ("\x1b[1;32m******** \x1b[1;31mThe Battleground\x1b[1;32m ********\x1b[0;37m\n\r");
  return;
}

/* This sends out a message giving the status of the current bg. */

void
show_bg_status (void)
{
  int i;
  char buf[STD_LEN];
  if (bg_hours == 0)
    return;

  /* Show the base battleground data. */
  
  echo ("\x1b[1;32m******** \x1b[1;31mThe Battleground\x1b[1;32m ********\x1b[0;37m\n\r");
  sprintf (buf, "\x1b[1;34mThe battleground for levels \x1b[1;37m%d\x1b[1;34m to \x1b[1;37m%d\x1b[1;34m will begin in \x1b[1;36m%d\x1b[1;34m hours.\x1b[0;37m\n\r", bg_minlev, bg_maxlev, bg_hours);
  echo (buf);

  /* Show the prize money (if any). */

  if (bg_money > 0)
    {
      sprintf (buf, "\x1b[1;34mThe cash prize is \x1b[1;33m%d\x1b[1;34m coin%s.\x1b[0;37m\n\r", bg_money, (bg_money == 1 ? "" : "s"));
      echo (buf);
    }
  
  /* Show the prizes awarded (if any). */

  for (i = 0; i < BG_MAX_PRIZE; i++)
    {
      if (bg_prize[i])
	{
	  sprintf (buf, "\x1b[1;34m[\x1b[1;37m%2d\x1b[1;34m]:\x1b[0;37m %s\n\r",
		   i + 1, NAME (bg_prize[i]));
	  echo (buf);
	}
    }  
  echo ("\x1b[1;32m******** \x1b[1;31mThe Battleground\x1b[1;32m ********\x1b[0;37m\n\r");
  return;
}

/* This function checks how many players/mobs are in the bg. If only
   one is left, then that player is the winner and gets all
   of the prizes and the bg is shut down. */

void
check_for_bg_winner (void)
{
  char buf[STD_LEN];
  int i;
  int count = 0;
  THING *lastmob = NULL, *room, *mob;

  /* See how many creatures are left in the BG. */
  
  for (i = BG_MIN_VNUM; i <= BG_MAX_VNUM; i++)
    {
      if ((room = find_thing_num (i)) == NULL)
	continue;
      
      for (mob = room->cont; mob; mob = mob->next_cont)
	{
	  if (CAN_FIGHT(mob) || CAN_MOVE (mob))
	    {
	      lastmob = mob;
	      count++;
	    }
	}
    }

  /* Bail out unless there is exactly one creature left. */
  
  if (count != 1 || !lastmob)
    return;

  /* That creature wins the bg. */
  
  sprintf (buf, "%s has won the battleground!\n\r", NAME (lastmob));
  bg_message (buf);

  /* Transfer the money and prizes to the creature. */

  add_money (lastmob, bg_money);
  for (i = 0; i < BG_MAX_PRIZE; i++)
    {
      thing_to (bg_prize[i], lastmob);
      bg_prize[i] = NULL;
    }
  bg_hours = 0;
  bg_minlev = 0;
  bg_maxlev = 0;
  bg_money = 0;
  
  /* If a player wins, send it home, otherwise delete the winner. */

  if (IS_PC (lastmob))
    thing_to (lastmob, find_thing_num(lastmob->pc->in_vnum));
  else
    free_thing (lastmob);
  return;
}

/* This command lets players either join or stay out of the battle.
   It also lets admins set up and shut down battlegrounds. */


void
do_battleground (THING *th, char *arg)
{
  int i, prize_count = 0;
  char buf[STD_LEN];
  char arg1[STD_LEN];
  THING *obj;
  
  /* Only players can battle. */
  
  if (!IS_PC (th))
    return;

  /* Let a player join. */
  
  if (LEVEL (th) < MAX_LEVEL)
    {
      if (bg_hours == 0)
	{
	  stt ("There's no battle going on right now.\n\r", th);
	  return;
	}

      if (LEVEL(th) < bg_minlev || LEVEL (th) > bg_maxlev)
	{
	  sprintf (buf, "The battle is for levels %d to %d and you're level %d.\n\r", bg_minlev, bg_maxlev, LEVEL (th));
	  stt (buf, th);
	  return;
	}
      
      /* The player can join the bg at this point. */
      
      if (th->pc->battling)
	{
	  th->pc->battling = FALSE;
	  stt ("Ok, you won't join the battle.\n\r", th);
	  return;
	}
      th->pc->battling = TRUE;
      stt ("Ok, you will join the battleground when it starts!\n\r", th);
      return;
    }

  /* From this point on, it's all admin commands. First, an admin
     can halt a battleground. */
  
  if (!str_cmp (arg, "halt") || !str_cmp (arg, "stop"))
    {
      if (bg_hours == 0)
	{
	  stt ("There is no battle in preparation to halt!\n\r", th);
	  return;
	}
      echo ("The battle has been halted.\n\r");
      bg_hours = 0;
      bg_money = 0;
      bg_minlev = 0;
      bg_maxlev = 0;
      for (i = 0; i < BG_MAX_PRIZE; i++)
	{
	  if (bg_prize[i])
	    thing_to (bg_prize[i], th);
	  bg_prize[i] = NULL;
	}
      return;
    }
  
  /* At this point, the thing is a player who is max level. 
     Let them set up a battleground. */

  /* Set the min level. */

  arg = f_word (arg, arg1);
  if ((bg_minlev = atoi(arg1)) < 1 || bg_minlev >= BLD_LEVEL)
    {
      bg_minlev = 0;
      stt ("Battle <minlev> <maxlev> [money] [prizes..up to 10.\n\r", th);
      return;
    }

  /* Set the max level. */

  arg = f_word (arg, arg1);
  if ((bg_maxlev = atoi(arg1)) < 1 ||
      bg_maxlev < bg_minlev || bg_maxlev >= BLD_LEVEL)
    {
      bg_minlev = 0;
      bg_maxlev = 0;
      stt ("Battle <minlev> <maxlev> [money] [prizes..up to 10.\n\r", th);
      return;
    }
  
  /* Set the money prize (if any). */

  arg = f_word (arg, arg1);
  if ((bg_money = atoi (arg1)) > 0)
    {
      arg = f_word (arg, arg1);
    }
  
  /* Set up the object prizes (if any). */

  while (prize_count < BG_MAX_PRIZE && *arg1)
    {
      if ((obj = find_thing_me (th, arg1)) != NULL)
	{
	  thing_from (obj);
	  bg_prize[prize_count++] = obj;
	}
      arg = f_word (arg, arg1);
    }
  
  bg_hours = BG_SETUP_HOURS;
  show_bg_status();
  return;
}
      
















