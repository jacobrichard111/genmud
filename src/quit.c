#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "serv.h"
#include "track.h"
#include "society.h"
#include "worldgen.h"

void
do_qui (THING *th, char *arg)
{
  stt ("You must type QUIT out fully to quit.\n\r", th);
  return;
}

void
do_quit (THING *th, char *arg)
{
  char buf[STD_LEN];
  THING *rth;
  if (!th || !IS_PC (th))
    {
      stt ("Only PC's can quit.\n\r", th);
      return;
    }
  if (is_in_auction (th))
    {
      stt ("You cannot quit while you are part of an auction!\n\r", th);
      return;
  
    }
  if (IN_BG (th))
    {
      stt ("You can't quit out in the battleground!\n\r", th);
      return;
    }

  if (!th->in || !IS_ROOM (th->in))
    {
      stt ("You can only quit in rooms.\n\r", th);
      return;
    }
  if (th->in->in && IS_AREA (th->in->in) && 
      IS_AREA_SET (th->in->in, AREA_NOQUIT))
    {
      stt ("You can't quit in this area!\n\r", th);
      return;
    }

  if (current_time - th->pc->login < 20 && LEVEL (th) < MAX_LEVEL)
    {
      stt ("You cannot log out so quickly after logging in.\n\r", th);
      return;
    }

  if (LEVEL (th) < BLD_LEVEL && th->pc->no_quit > 0)
    {
       sprintf (buf, "You have recently been in or near combat, so you must wait %d hours before quitting.\n\r", th->pc->no_quit);
       stt (buf, th);
       return;
     }

  for (rth = thing_hash[PLAYER_VNUM % HASH_SIZE]; rth; rth = rth->next)
    {
      if (rth->fgt)
	{
	  if (rth->fgt->following == th)
	    {
	      act ("@1n stop@s following @3n.", rth, NULL, th, NULL, TO_ALL);
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

  write_playerfile (th);

  if (IS_PC2_SET (th, PC2_MAPPING))
    do_cls (th, "");

  sprintf (buf, "%s logs out.", NAME (th));
  log_it (buf);

  if (th->fd)
    {
      th->fd->write_buffer[0] = '\0';
      th->fd->write_pos = th->fd->write_buffer;
      write_to_buffer ("Bye Bye Now!\n\r", th->fd);
      th->fd->th = NULL;
      close_fd (th->fd);
      th->fd = NULL;
    }
  
  check_pbase (th);

  free_thing (th);

  return;
}
  
  
/* This tells you how much exp you need per level. */

void
do_levels (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  int start, end, i, temp, num_lines, j, lev;
  
  if (!*arg)
    {
      start = 1;
      end = MORT_LEVEL;
    }
  else
    {
      arg = f_word (arg, arg1);
      if (!*arg)
	{
	  start = 1;
	  end = MORT_LEVEL;
	}	
      start = MID (0, atoi (arg1), MORT_LEVEL);
      end = MID (0, atoi(arg), MORT_LEVEL);
      
      if (start == 0 || end == 0)
	{
	  start = 1;
	  end = MORT_LEVEL;
	}
      if (end <= start)
	{
	  temp = end;
	  end = start;
	  start = temp;
	}
    }
  
  stt ("You need the following exp for the following levels.\n\r", th);
  stt ("Note that exp is zeroed out after every level, so you\n\r", th);
  stt ("Really need the experience amounts listed below.\n\n\r", th);
  
  
  num_lines = (end - start + 3)/4;
 
  for (i = 0; i < num_lines; i++)
    {
      buf[0] = '\0';
      for (j = 0; j < 4; j++)
	{
	  lev = start + i + j * num_lines;
	  if (lev <= end)
	    {
	      sprintf (buf + strlen(buf), "Lev%3d: %-11d", lev,
		       exp_to_level (lev));
	    }
	}
      strcat (buf, "\n\r");
      stt (buf, th);
    }
  return;
}


void
do_advance (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  THING *victim;
  int value;

  if (LEVEL (th) != MAX_LEVEL || !IS_PC (th))
    {
      stt ("Huh?\n\r", th);
      return;
    }

  arg = f_word (arg, arg1);
  
  
  if ((victim = find_pc (th, arg1)) == NULL)
    {
      stt ("That person isn't around to advance.\n\r", th);
      return;
    }
  if (victim == th)
    {
      stt ("You cannot advance yourself.\n\r", th);
      return;
    }
  if (!IS_PC (victim))
    {
      stt ("This only works on players.\n\r", th);
      return;
    }
  if (!is_number (arg))
    {
      stt ("Syntax advance <name> <level>\n\r", th);
      return;
    }
  
  value = atoi (arg);
  
  if (value < 1 || value > MAX_LEVEL)
    {
      sprintf (buf, "The level must be between 1 and %d\n\r", MAX_LEVEL);
      stt (buf, th);
      return;
    }
  if (value < victim->level)
    {
      THING *obj, *objn;
      for (obj = victim->cont; obj; obj = objn)
        {
          objn = obj->next_cont;
          remove_thing (victim, obj, TRUE);
        }
      victim->level = 1;
      victim->max_hp = 20;
      victim->hp = 20;
      victim->max_mv = 100;
      victim->mv = 100;
    }
  advance_level (victim, value);
  victim->pc->exp = 0;
  victim->pc->fight_exp = 0;
  stt ("Ok, victim level set.\n\r", th);
  return;
}

/* This is what happens to you when you raise a level */

void
advance_level (THING *th, int level)
{
  char buf[STD_LEN];
  RACE *race, *align;
  FLAG *flag;
  int hpbonus = 0, mvbonus = 0;
  if ( !th || !IS_PC (th) || level < 1 || level > MAX_LEVEL)
    return;
  race = FRACE(th);
  align = FALIGN(th);
  if (race && align)
    {
      for (flag = race->flags; flag; flag = flag->next)
        {
          if (flag->type == FLAG_AFF_HP)
            hpbonus += flag->val;
          if (flag->type == FLAG_AFF_MV)
            mvbonus += flag->val;
        }
      for (flag = align->flags; flag; flag = flag->next)
        {
          if (flag->type == FLAG_AFF_HP)
            hpbonus += flag->val;
          if (flag->type == FLAG_AFF_MV)
            mvbonus += flag->val;
        }
    }
  while (th->level < level)
    {
      th->level++;
      th->max_mv += nr(1,1 + get_stat (th, STAT_DEX)/15) + mvbonus;
      th->max_hp += 5 + get_stat (th, STAT_CON)/2 + hpbonus;
      th->pc->practices += 1 + get_stat (th, STAT_WIS)/3;
      th->hp = th->max_hp;
      th->mv = th->max_mv;
      th->pc->mana = th->pc->max_mana;
      sprintf (buf, "You raise a level! You are now level %d.\n\r", LEVEL (th));
      stt (buf, th);
      sprintf (buf, "%s is now level %d!", NAME (th), LEVEL (th));
      group_notify (th, buf);
    }
  find_max_mana (th);
  update_kde (th, KDE_UPDATE_NAME | KDE_UPDATE_HMV | KDE_UPDATE_EXP);
  return;
}


void
add_exp (THING *th, int amt)
{
  int l;
  if (amt < 1 || !th || !IS_PC (th) || LEVEL (th) >= BLD_LEVEL - 1)
    return;
  if ((l = th->level) < 1)
    th->level = 1;
  
  th->pc->exp += amt;
  l = th->level;
  while (th->pc->exp >= exp_to_level (LEVEL(th)) && th->level < BLD_LEVEL - 1)
    {
      th->pc->exp -= exp_to_level (LEVEL (th));
      advance_level (th, th->level + 1);
      l++;
    }
  if (th->pc->exp < 0)
    th->pc->exp = 0;
  update_kde (th, KDE_UPDATE_EXP);
  return;
}

/* Exp for killing a mob or pc. */
/* This has some new code in it whereby if there are people on opposite
   sides of the pkill from the same site, everyone from that site
   who is within range of this combat gets frozen. */

void 
kill_exp (THING *killer, THING *vict)
{
  int dir, range, num_points = 0, exp, lev_helpers = 0,
    top_level = 1, curr_exp, vict_host = 0;
  bool pkill = FALSE, mplay_pkill = FALSE;
  THING *rth, *room, *curr_room;
  VALUE *exit;
  char buf[STD_LEN];
  
  
  /* Killer and victim must exist, and the victim must be in someplace. */
  
  if (!killer || !vict || !vict->in)
    return;
  
  if (IS_PC (vict) && DIFF_ALIGN (vict->align, killer->align))
    {
      pkill = TRUE;
      vict_host = vict->pc->hostnum;
    }

  if (IS_PC (killer) && vict->align < ALIGN_MAX &&
      !DIFF_ALIGN (killer->align, vict->align))
    {
      /* Same side pk gives you a heavy price to be paid...*/
      if (IS_PC (vict))
	killer->pc->align_hate[vict->align] += 
	  MAX(0, MORT_LEVEL + 20 - LEVEL(vict) - 
	      (10*vict->pc->remorts));
      else
	killer->pc->align_hate[vict->align] += (1+ LEVEL(vict)/20);
    }
  clear_bfs_list ();
  undo_marked (vict->in);
  add_bfs (NULL, vict->in, DIR_MAX);
  
  
  /* Set up bfs list of appropriate rooms nearby. Note, this only gets
     done if the victim died in a regular room. */
  
  if (IS_ROOM (vict->in))
    {
      for (dir = 0; dir < 6; dir ++)
	{
	  curr_room = vict->in;
	  for (range = 0; range < 3; range++)
	    {
	      if ((exit = FNV (curr_room, dir + 1)) == NULL ||
		  (room = find_thing_num (exit->val[0])) == NULL ||
		  !IS_ROOM (room) ||
		  IS_MARKED(room))
		break;
	      add_bfs (NULL, room, dir);
	      curr_room = room;
	    }
	}
    }
  
  for (bfs_curr = bfs_list; bfs_curr != NULL; bfs_curr = bfs_curr->next)
    {
      if (bfs_curr->room)
	{
	  for (rth = bfs_curr->room->cont; rth; rth = rth->next_cont)
	    {      
	      if (IS_PC (rth) &&
		  ((pkill && !DIFF_ALIGN (rth->align, killer->align)) ||
		   in_same_group (rth, killer)))
		{			  
		  num_points+= find_gp_points (rth);
		  lev_helpers += LEVEL (rth);	  
		  top_level = MAX (top_level, LEVEL (rth));
		  if (IS_PC (vict) && DIFF_ALIGN (rth->align, vict->align) && 
		      vict->align > 0 && rth->align > 0 && 
		      LEVEL (vict) < BLD_LEVEL && LEVEL (rth) < BLD_LEVEL &&
		      vict->pc->hostnum == rth->pc->hostnum)
		    {
		      sprintf (buf, "%s and %s from the same site, diff aligns in pkill.\n\r", NAME (rth), NAME (vict));
		      echo (buf);
		      log_it (buf);
		      mplay_pkill = TRUE;

		      
		    }
		}
	    }
	}
    }
  
  
  if (mplay_pkill)
    return;

  /* Now we have the total number of people who are sharing in the kill.
     Note that if the killed thing is a pkill, the attackers don't need
     to be grouped.... :) I am not doing a lot of crazy checking for
     group sizes here...I am sick of dealing with that. People will
     have to learn to deal with horders themselves. */
  
  
  exp = ((1 + LEVEL (vict)) * (2 + LEVEL (vict)) * (3 + LEVEL (vict)))/5;
  
  /* Lots of exp for opp align pkill, none for same side pkill. */

  if (IS_PC (vict) && pkill)
    exp *= LEVEL (vict) * DIFF_ALIGN (killer->align, vict->align);
  
  /* Don't allow hugeass groups to get TOO much exp. Note this exp system
     might get sick for really highlevel mobs..let's see how it works. */
  
  if (SACRIFICING)
    {
      exp *= 10;
    }
  else
    {
      
      if (num_points > MAX_GROUP_POINTS)
	exp/= (num_points/3);
      
      if (num_points >= 2 *MAX_GROUP_POINTS)
	exp /= (num_points/3);
    }

  exp += nr (0, exp/15);
  if (pkill)
    {
      vict->pc->pk[PK_PKILLED]++;
      vict->pc->pk[PK_WPS] -= 	
	MIN (LEVEL (vict)/10 + vict->pc->remorts, vict->pc->pk[PK_WPS]);
      vict->pc->pk[PK_TOT_WPS] -= 
	MIN (LEVEL (vict)/10 + vict->pc->remorts, vict->pc->pk[PK_TOT_WPS]);
     
      update_kde (vict, KDE_UPDATE_PK);
    }
  /* Now we add exp. The big mess about curr_exp and all that is to
     prevent powerlevelling of a highlevel for lower levels. The
     restrictions are not too bad...i.e. at least a 15 level difference
     and the highest person has to be twice your level...so a person
     who is 30 or so can group with people who are 60 or so. This
     is so that newbies can go along on expeditions and contribute 
     something and actually expect to get something out of it. */

  for (bfs_curr = bfs_list; bfs_curr != NULL; bfs_curr = bfs_curr->next)
    {
      if (bfs_curr->room)
	{
	  for (rth = bfs_curr->room->cont; rth; rth = rth->next_cont)
	    {	      
	      if (IS_PC (rth) && 
		  ((pkill && !DIFF_ALIGN (rth->align, killer->align)) ||
		   in_same_group (rth, killer)))
		{
		  if (top_level - LEVEL (rth) > 15 && top_level >= 2*LEVEL (rth))
		    curr_exp = (((exp * LEVEL (rth))/top_level) * LEVEL (rth))/top_level;
		  else
		    curr_exp = exp;
		  curr_exp *= find_remort_bonus_multiplier (rth);
		  add_exp (rth, curr_exp);
		  sprintf (buf, "You have gained %d experience for killing this creature, and %d experience for fighting.\n\r", curr_exp, rth->pc->fight_exp);
		  stt (buf, rth);
		  rth->pc->fight_exp = 0;
		  rth->pc->pk[PK_KILLS]++;

		  if (LEVEL (vict) > LEVEL (rth) &&
		      LEVEL(vict) >= 50)
		    rth->pc->pk[PK_KILLPTS] += 
		      MIN(3, (LEVEL (vict) - LEVEL (rth) - 5)/5);
		  
		  update_trophies (rth, vict, num_points, lev_helpers);
		  update_kde (rth, KDE_UPDATE_PK);
		  if (DIFF_ALIGN (rth->align, vict->align) &&
		      CAN_TALK (vict) &&
		      find_society_in (vict))
		    add_society_reward (rth, vict, REWARD_KILL, LEVEL(vict)/5);
		}
	    }
	}
    }
  clear_bfs_list ();
  return;
}
  
/* This lets you configure how offensive or defensive you are. */
/* It affects how often you hit, how hard you hit, how often things
hit you, and how likely you are to flee. */

void
do_offensive (THING *th, char *arg)
{
  char buf[STD_LEN];
  int num;
  if (!IS_PC (th))
    return;

  if (FIGHTING (th))
    { 
      stt ("You are too engrossed in combat to change your tactics now!\n\r", th);
      return;
    }

  if (th->pc->off_def > 100)
    th->pc->off_def = 50;

  if (!*arg)
    {
        sprintf (buf, "You put \x1b[1;36m%d\x1b[0;37m percent of your efforts toward offense, and \x1b[1;36m%d\x1b[0;37m percent of your efforts toward defense.\n\r", th->pc->off_def, (100 - th->pc->off_def));
       stt (buf, th);
       return;
    }

  if (!is_number (arg ))
    {
      stt ("Offensive <percent>\n\r", th);
      return;
    }
 
  if ((num = atoi (arg)) < 0 || num > 100)
    { 
      stt ("Offensive <percent>\n\r", th);
      return;
    }

   
  th->pc->off_def = num;
  sprintf (buf, "Offensive percent set to %d. 50 percent is normal.\n\r", 
  th->pc->off_def);
  stt (buf, th);
  th->pc->wait += 2*PULSE_PER_SECOND;
  return;
}


/* This lets you configure a lot of flags. */

  
void
do_config (THING *th, char *arg)
{
  int value;
  char buf[STD_LEN];
  char buf2[STD_LEN];
  FLAG *flg;
  FLAG_DATA *flgd;
  int current_flags;
  char *t;
  int pass =0;
  bool flag_is_set = FALSE;
  if (!th || !IS_PC (th))
    return;
  
  
  if (arg[0] == '\0')
    {
      if (LEVEL (th) >= BLD_LEVEL)
	pass = 0;
      else
	pass = 1;
      
      do
	{
	  if (pass == 0)
	    {
	      stt ("This is your admin configuration.\n\n\r", th);	      
	      flgd = (FLAG_DATA *) pc1_flags;
	      current_flags = flagbits (th->flags, FLAG_PC1);
	    }	  
	  else if (pass == 1)
	    {
	      flgd = (FLAG_DATA *) pc2_flags;
	      stt ("\nThese are your configuration options:\n\n\r", th);
	      current_flags = flagbits (th->flags, FLAG_PC2);
	    }
	  else
	    break;
	  
	  for (; flgd->flagval != 0; flgd++)
	    {
	      if (IS_SET (current_flags, flgd->flagval))
		flag_is_set = TRUE;
	      else
		flag_is_set = FALSE;
	      
	      sprintf (buf2, "%-15s", flgd->app);
	      
	      for (t = buf2; *t; t++)
		{
		  if (flag_is_set)
		    *t = UC(*t);
		  else
		*t = LC(*t);
		}
	      sprintf (buf, "\x1b[1;33m[\x1b[1;3%dm%s\x1b[1;33m]\x1b[0;37m",
		       (flag_is_set ? 7: 1),
		       buf2);
	      strcat (buf, "   You ");
	      
	      if (!flag_is_set)
		{
		  if (pass == 0)
		    {
		      if (IS_SET (flgd->flagval, PC_SILENCE | PC_FREEZE | PC_DENY | PC_UNVALIDATED))
			strcat (buf, "are ");
		      else
			strcat (buf, "aren't ");
		    }
		  else
		    strcat (buf, "don't ");
		}
	      else if (pass == 0)
		{
		  if (IS_SET (flgd->flagval, PC_SILENCE | PC_FREEZE | PC_DENY  | PC_UNVALIDATED))
		    strcat (buf, "aren't ");
		  else
		    strcat (buf, "are ");
		}
	      strcat (buf, flgd->help);
	      strcat (buf, "\n\r");
	      stt (buf, th);
	    }
	}
      while (++pass < 2);
      return;
    }
  
  if ((value = find_bit_from_name (FLAG_PC1, arg)) != 0)
    {
      if (value < PC_WIZQUIET && LEVEL (th) < BLD_LEVEL)
	{
	  stt ("config <option> to toggle on and off.\n\r", th);
	  return;
	}
      if ((flg = find_next_flag (th->flags, FLAG_PC1)) == NULL)
	{
	  flg = new_flag ();
	  flg->next = th->flags;
	  th->flags = flg;
	  flg->type = FLAG_PC1;
	}
      flg->val ^= value;
      sprintf (buf, "\n\r\x1b[1;36m%s\x1b[0;37m flag toggled \x1b[1;3%dm%s\x1b[0;37m.\n\r", arg,
	       IS_SET (flg->val, value) ? 7 : 1,	       
	       IS_SET(flg->val, value) ? "On" : "Off");
      stt (buf, th);
      if (value == PC2_MAPPING)
	{
	  if (!USING_KDE (th))
	    {
	      if (!IS_SET (flg->val, value))
		{
		  do_cls (th, "");
		}
	      else
		{
		  create_map (th, th->in, SMAP_MAXX, SMAP_MAXY);
		}
	    }
	  else
	    update_kde (th, KDE_UPDATE_MAP);
	}
      return;
    }
   if ((value = find_bit_from_name (FLAG_PC2, arg)) != 0)
    {
      if ((flg = find_next_flag (th->flags, FLAG_PC2)) == NULL)
	{
	  flg = new_flag ();
	  flg->next = th->flags;
	  th->flags = flg;
	  flg->type = FLAG_PC2;
	}
      flg->val ^= value;
      sprintf (buf, "\n\r\x1b[1;36m%s\x1b[0;37m flag toggled \x1b[1;3%dm%s\x1b[0;37m.\n\r", arg,
	       IS_SET(flg->val, value) ?  7 : 1,
	       IS_SET(flg->val, value) ? "On" : "Off");
      buf[0] = UC (buf[0]);
      stt (buf, th);
      if (value == PC2_MAPPING) /* Clear map box. */
	do_cls (th, "");
      if (value == PC2_ANSI)
	{
	  if (IS_SET (flg->val, PC2_ANSI) && th->fd)
	    RBIT (th->fd->flags, FD_NO_ANSI);
	  else
	    SBIT (th->fd->flags, FD_NO_ANSI);
	}
      return;
    }
   
   stt ("config <option> to toggle on and off.\n\r", th);
   return;
}


void
do_silence (THING *th, char *arg)
{
  THING *vict;
  FLAG *flg;
  char buf[STD_LEN];
  
  if (!IS_PC (th) || LEVEL (th) < MAX_LEVEL)
    {
      stt ("Huh?\n\r", th);
      return;
    }

  if ((vict = find_pc (th, arg)) == NULL || !IS_PC (vict) ||
      LEVEL (vict) == MAX_LEVEL)
    {
      stt ("Silence who?\n\r", th);
      return;
    }
  
  if ((flg = find_next_flag (vict->flags, FLAG_PC1)) == NULL)
    {
      flg = new_flag ();
      flg->type = FLAG_PC1;
      flg->next = vict->flags;
      vict->flags = flg;
    }
  
  sprintf (buf, "Silence bit toggled %s.\n\r", (IS_SET (flg->val, PC_SILENCE ) ? "Off" : "On"));
  flg->val ^= PC_SILENCE;
  stt (buf, th);
  write_playerfile (vict);
  return;
}


  
			     

void
do_freeze (THING *th, char *arg)
{
  THING *vict;
  FLAG *flg;
  char buf[STD_LEN];
  
  if (!IS_PC (th) || LEVEL (th) < MAX_LEVEL)
    {
      stt ("Huh?\n\r", th);
      return;
    }

  if ((vict = find_pc (th, arg)) == NULL || !IS_PC (vict) ||
      LEVEL (vict) == MAX_LEVEL)
    {
      stt ("Freeze who?\n\r", th);
      return;
    }
  
  if ((flg = find_next_flag (vict->flags, FLAG_PC1)) == NULL)
    {
      flg = new_flag ();
      flg->type = FLAG_PC1;
      flg->next = vict->flags;
      vict->flags = flg;
    }
  
  sprintf (buf, "Freeze bit toggled %s.\n\r", (IS_SET (flg->val, PC_FREEZE ) ? "Off" : "On"));
  flg->val ^= PC_FREEZE;
  stt (buf, th);
  write_playerfile (vict);
  return;
}


void
do_beep (THING *th, char *arg)
{
  THING *vict;
  char buf[STD_LEN];
  if ((vict = find_pc (th, arg)) == NULL)
    {
      stt ("Who do you want to beep?\n\r", th);
      return;
    }

  if (vict == th)
    {
      stt ("Why would you beep yourself? To annoy yourself?\n\r", th);
      return;
    }
  
 
  sprintf (buf, "%s beeps you!\7\n\r", name_seen_by (vict, th));
  stt (buf, vict);
  sprintf (buf, "You beep %s.", name_seen_by (th, vict));
  stt (buf, th);
  return;
}

  
			     

void
do_deny (THING *th, char *arg)
{
  THING *vict;
  FLAG *flg;
  char buf[STD_LEN];
  
  if (!IS_PC (th) || LEVEL (th) < MAX_LEVEL)
    {
      stt ("Huh?\n\r", th);
      return;
    }

  if ((vict = find_pc (th, arg)) == NULL || !IS_PC (vict) ||
      LEVEL (vict) == MAX_LEVEL)
    {
      stt ("Deny who?\n\r", th);
      return;
    }
  
  if ((flg = find_next_flag (vict->flags, FLAG_PC1)) == NULL)
    {
      flg = new_flag ();
      flg->type = FLAG_PC1;
      flg->next = vict->flags;
      vict->flags = flg;
    }
  
  sprintf (buf, "Deny bit toggled %s.\n\r", (IS_SET (flg->val, PC_DENY ) ? "Off" : "On"));
  flg->val ^= PC_DENY;
  stt (buf, th);
  write_playerfile (vict);
  return;
}


void
do_delet (THING *th, char *arg)
{
  stt ("You must type out delete fully!\n\r", th);
  return;
}
			     
void
do_delete (THING *th, char *arg)
{
  char filename[100];
  THING *vict;
  FILE_DESC *fd;
  
  if (LEVEL (th) < MAX_LEVEL && str_cmp (arg, "character forever yes oh yes now"))
    {
      stt ("You must type delete character forever yes oh yes now\n\r", th);
      return;
    }
  
  if (LEVEL (th) < MAX_LEVEL && current_time - th->pc->login < 100)
    {
      stt ("You cannot delete so quickly after logging in.\n\r", th);
      return;
    }
  
  if (LEVEL (th) == MAX_LEVEL)
    {
      if ((vict = find_pc (th, arg)) != NULL &&
	  vict != th)
	{
	  if ((fd = vict->fd) != NULL)
	    {
	      vict->fd = NULL;
	      fd->th = NULL;
	      close_fd (fd);
	      free_thing (vict);
	    }
	}
      sprintf (filename, "%s%s", PLR_DIR, capitalize (arg));
      unlink (filename);
      stt ("Ok, character deleted.\n\r", th);
      return;
    }

  if ((fd = th->fd) != NULL)
    {
      fd->th = NULL;
      close_fd (fd);
    }
  th->fd = NULL;
  sprintf (filename, "%s%s", PLR_DIR, NAME (th));
  free_thing (th);
  unlink (filename);
  return;
}

void
do_log (THING *th, char *arg)
{
  THING *vict;
  FLAG *flg;
  char buf[STD_LEN];
  
  if (!IS_PC (th))
    return;
  
  if (!str_cmp (arg, "all"))
    {
      if (IS_SET (server_flags, SERVER_LOG_ALL))
	{
	  RBIT (server_flags, SERVER_LOG_ALL);
	  stt ("Ok, not logging all anymore!\n\r", th);
	  return;
	}
      SBIT (server_flags, SERVER_LOG_ALL);
      stt ("Ok, logging all now.\n\r", th);
      return;
    }
  
  if ((vict = find_pc (th, arg)) == NULL)
    {
      stt ("That person isn't here to log.\n\r", th);
      return;
    }
  if ((flg = find_next_flag (vict->flags, FLAG_PC1)) == NULL)
    {
      flg = new_flag ();
      flg->type = FLAG_PC1;
      flg->next = vict->flags;
      vict->flags = flg;
    }
  
  sprintf (buf, "Log bit toggled %s.\n\r", (IS_SET (flg->val, PC_LOG ) ? "Off" : "On"));
  flg->val ^= PC_LOG;
  stt (buf, th);
  
  return;
}

void
do_remort (THING *th, char *arg)
{
  int wps, money, lev, kps, i, j;
  int bonus[STATS_PER_REMORT];
  int totals[STAT_MAX];
  int guildstats[STAT_MAX];
  char arg1[STD_LEN];
  char buf[STD_LEN];
  RACE *race, *align;
 
  if (!IS_PC (th))
    return;
  
  if ((race = FRACE(th)) == NULL ||
      (align = FALIGN(th)) == NULL)
    {
      stt ("You are missing a race or alignment.\n\r", th);
      return;
    }
  
 


  if (!*arg || th->position != POSITION_STANDING)
    {
      stt ("remort costs or remort <stat> <stat> <stat> <stat>\n\r", th);
      return;
    }

  if (!str_cmp (arg, "cost") || !str_cmp (arg, "costs"))
    {
      stt ("These are the costs for obtaining each remort. All of the costs are lost each time.\n\n\r", th);
      
      stt ("   Num     Level       Warpoints           Killpoints      Money\n\r", th);
      for (i = 1; i <= MAX_REMORTS; i++)
	{
	  wps = (i > 1 ? 100 * (i - 1) * (i - 1) : 0); 
	  lev = 40 + 5* (i/2);
	  kps = 50 * i * i;
	  money = (i > 1 ? 1000 * i * i : 0);
	  if (i <= th->pc->remorts)
	    stt ("\x1b[1;30m", th);
	  else if (i == th->pc->remorts + 1)
	    {
	      stt (" \x1b[0;37m --------------------This is your next remort------------------\n\r", th);
	      sprintf (buf, "Status:    \x1b[1;3%dm%-3d         \x1b[1;3%dm%-5d               \x1b[1;3%dm%-5d           \x1b[1;3%dm%-6d\x1b[1;37m\n\r",
		       (LEVEL (th) < lev ? 1 : 2), LEVEL (th),
		       (th->pc->pk[PK_WPS] < wps ? 1 : 2), th->pc->pk[PK_WPS],
		       (th->pc->pk[PK_KILLPTS] < kps ? 1 : 2), th->pc->pk[PK_KILLPTS],
		       (total_money (th) + th->pc->bank < money ? 1 : 2), (total_money(th) + th->pc->bank));
	      stt (buf, th);
	    }		
	  else
	    stt ("\x1b[0;37m", th); 

	  sprintf (buf, "%3s[%-2d]    %-3d         %-5d               %-5d           %-6d\n\r",
		   (i == th->pc->remorts + 1 ? "-->" : "   "),
		   i, lev, wps, kps, money);
	  stt (buf, th);
	}
      return;
    }
  
  i = th->pc->remorts + 1;
  wps = (i > 1 ? 100 * (i - 1) * (i - 1) : 0); 
  lev = 40 + 5* (i/2);
  kps = 50 * i * i;
  money = (i > 1 ? 1000 * i * i : 0);
  
  if (th->pc->remorts >= MAX_REMORTS)
    {
      stt ("You have already remorted the maximum number of times!\n\r", th);
      return;
    }

  if (LEVEL (th) < lev)
    {
      stt ("Your level isn't high enough to remort.\n\r", th);
      return;
    }
  
  if (th->pc->pk[PK_WPS] < wps)
    {
      stt ("You don't have enough warpoints to remort!\n\r", th);
      return;
    }
  
  if (th->pc->pk[PK_KILLPTS] < kps)
    {
      stt ("You need more killpoints to remort!\n\r", th);
      return;
    }
  
  if (total_money (th) + th->pc->bank < money)
    {
      stt ("You don't have enough money to remort!\n\r", th);
      return;
    }
      

  if (!th->in || !IS_ROOM(th->in) || th->in->vnum != REMORT_ROOM_VNUM)
    {
      stt ("You must be in the remort room to remort!\n\r", th);
      return;
    }

  /* Get bonuses */

  
  for (i = 0; i < STATS_PER_REMORT; i++)
    {
      bonus[i] = STAT_MAX;
      arg = f_word (arg, arg1);
      for (j = 0; j < STAT_MAX; j++)
	{
	  if (!str_prefix (arg1, stat_name[j]))
	    {
	      bonus[i] = j;
	      break;
	    }
	}
      if (bonus[i] == STAT_MAX)
	{
	  stt ("You didn't pick a valid stat name to improve!\n\r", th);
	  return;
	}
    }
  
  /* Check if for remorts < 2, there aren't any of the same stats picked. */
  
  if (th->pc->remorts < 2)
    {
      for (i = 0; i < STATS_PER_REMORT; i++)
	{
	  for (j = i + 1; j < STATS_PER_REMORT; j++)
	    {
	      if (bonus[i] != STAT_MAX && bonus[i] == bonus[j])
		{
		  stt ("You cannot improve the same stat more than once until your third remort!\n\r", th);
		  return;
		}
	    }
	}
    }
  
  /* Now check race/align caps. */
  
  for (i = 0; i < STAT_MAX; i++)
    {      
      totals[i] = 0;
      guildstats[i] = 0;
    }
  
  /* Add up totals! */
  
  for (i = 0; i < STATS_PER_REMORT; i++)
    totals[bonus[i]]++;

  /* Figure guild bonuses */
  
  for (i = 0; i < GUILD_MAX; i++)
    {
      if (guild_info[i].flagval >= 0 && 
	  guild_info[i].flagval < STAT_MAX)
	{
	  if (guild_rank (th, i) >= 3)	  
	    guildstats[guild_info[i].flagval]++;
	  if (guild_rank (th, i) == GUILD_TIER_MAX)
	    guildstats[guild_info[i].flagval]++;
	}
    }
  
  /* Now check how high stats are...we add the old stats plus the
     new bonuses minus align bonuses, minus guild bonuses. */
  
  for (i = 0; i < STAT_MAX; i++)
    {
      if (((totals[i] + th->pc->stat[i]) - 
	   (align->max_stat[i] + guildstats[i])) >
	  race->max_stat[i])
	{
          sprintf (buf, "You cannot push %s over your racial and alignment max!\n\r", stat_name[i]);
          stt (buf, th); 
	  return;
	}
    }
  
  
  /* Now we know we have the proper costs and chosen stats to raise.
     We now remort. */
  
  /* Remove costs */
  
  th->pc->pk[PK_WPS] -= wps;
  th->pc->pk[PK_KILLPTS] -= kps;
  if (total_money (th) >= money)
    sub_money (th, money);
  else
    {
      money -= total_money (th);
      sub_money (th, total_money (th));
      th->pc->bank -= money;
      if (th->pc->bank < 0)
	th->pc->bank = 0;
    }
  /* Increase remort times. */
  th->pc->remorts++;
  calc_max_remort(th);
  /* Add stat bonuses. */
  for (i = 0; i < STAT_MAX; i++)
    th->pc->stat[i] += totals[i];
  
  /* This clears the pc stats during the remorting process. */
  remort_clear_stats (th);
  
  act ("@1n just remorted.", th, NULL, NULL, NULL, TO_ALL);
  thing_to (th, find_thing_num (th->align + 100));
}

void
remort_clear_stats (THING *th)
{
  int i;
  THING *obj, *objn;
  if (!th || !IS_PC (th))
    return;
  
  /* Clear spells */
  
  for (i = 0; i < MAX_SPELL; i++)
    {
      th->pc->prac[i] = 0;
      th->pc->learn_tries[i] = 0;
    }
  /* Remove eq */

  for (obj = th->cont; obj != NULL; obj = objn)
    {
      objn = obj->next_cont;
      if (IS_WORN(obj))
	remove_thing (th, obj, TRUE);
    }
  
  /* Remove affects */
  
  remove_all_affects (th, FALSE);
  
  
  /* Clear worldgen quests for this player. */

  clear_player_worldgen_quests (th);

  /* Now set up the character anew */

  th->level = 1;
  th->max_hp = 20 + 10 * th->pc->remorts;
  th->hp = th->max_hp;
  th->max_mv = 100;
  th->mv = th->max_mv;
  th->pc->exp = 0;
  th->pc->fight_exp = 0;
  if (th->pc->remorts >= 5)
    {
      th->pc->cond[COND_THIRST] = COND_FULL;
      if (th->pc->remorts >= 8)
        th->pc->cond[COND_HUNGER] = COND_FULL;
    } 
  find_max_mana (th);
  fix_pc (th);
  return;
}

/* This lets you ascend: (start over with a better race that you
   couldn't get from the start...) */



void
do_ascend (THING *th, char *arg)
{
  char buf[STD_LEN];
  char arg1[STD_LEN];
  THING *thg;
  RACE *race;
  int i;
  if (!th || !IS_PC (th))
    return;
  
  if ((LEVEL (th) < BLD_LEVEL && th->pc->remorts < MAX_REMORTS/2) ||
      !*arg)
    {
      stt ("Ascend will let you start over with a new, more powerful race once you've maxxed out your character.\n\r", th);
      return;
    }
  
  if (th->pc->no_quit > 0)
    {
      stt ("You can't ascend until you can quit.\n\r",th);
      return;
    }
  
  
  
  arg = f_word (arg, arg1);
  if (str_cmp (arg1, "yes"))
    {
      stt ("You must type ascend yes (race name)\n\r", th);
      stt ("WHEN YOU ASCEND YOU ARE DELETED AND YOU START WITH NOTHING EXCEPT A NEW RACE!!!\n\r", th);
      stt ("The game doesn't check if you pick one of the sucky races, so be careful.\n\r", th);
      return;
    }
  
  if ((race = find_race (arg, -1)) == NULL)
    {
      stt ("That isn't a valid race name.\n\r", th);
      return;
    }
  
  if (race->ascend_from != FRACE(th)->vnum)
    {
      stt ("You can't ascend to that race!\n\r", th);
      return;
    }


  if(th->pc->remorts != MAX_REMORTS ||
     LEVEL (th) != MORT_LEVEL)
    {
      sprintf (buf, "You must have %d remorts and be level %d to ascend to a new race.\n\r", MAX_REMORTS, MORT_LEVEL);
      stt (buf, th);
      return;
    }
  
  sprintf (buf, "%s ASCENDS\n", NAME(th));
  log_it (buf);
  
  if ((thg = new_thing()) == NULL ||
      (thg->pc = new_pc()) == NULL)
    {
      stt ("Error creating a new character. Bailing out.\n\r", th);
      free_thing (thg);
      return;
    }
  
  thing_from (th);
  thg->fd = th->fd;
  thg->fd->th = thg;
  th->fd = NULL;
  
 
  
  /* Don't copy over most information. Only copy stuff like the
     description and the ignore list and the aliases. */
  thg->vnum = PLAYER_VNUM;
  thg->hp = 20;
  thg->max_hp = 20;
  thg->mv = 100;
  thg->max_mv = 100;
  thg->level = 1;
  thg->pc->remorts = 0;    
  thg->thing_flags = MOB_SETUP_FLAGS;
  /* Copy some minor data. */
  thg->name = new_str (th->name);
  thg->short_desc = new_str (th->short_desc);
  thg->long_desc = new_str (th->long_desc);
  thg->pc->pwd = new_str (th->pc->pwd);
  thg->pc->prompt = new_str (th->pc->prompt);
  thg->desc = new_str (th->desc);
  thg->pc->title = new_str (th->pc->title);
  thg->pc->email = new_str (th->pc->email);
  thg->pc->hostnum = th->pc->hostnum;
  thg->sex = th->sex;
  thg->align = th->align;
  thg->pc->race = race->vnum;
  set_stats(thg);
  thg->pc->curr_note = th->pc->curr_note;
  /* Copy pk data? Naaah...just this. */
  thg->pc->pk[PK_BRIGACK] = th->pc->pk[PK_BRIGACK];
  
  /* Copy ignores and aliases and other configuration stuff... */
  for (i = 0; i < MAX_IGNORE; i++)
    {
      thg->pc->ignore[i] = new_str (th->pc->ignore[i]);
    }
  
  for (i = 0; i < MAX_ALIAS; i++)
    {
      thg->pc->aliasname[i] = new_str (th->pc->aliasname[i]);
      thg->pc->aliasexp[i] = new_str (th->pc->aliasexp[i]);
    }

  for (i = 0; i < CLAN_MAX; i++)
    thg->pc->clan_num[i] = th->pc->clan_num[i];
  
  for (i = 0; i < MAX_CHANNEL; i++)
    thg->pc->channel_off[i] = th->pc->channel_off[i];

  /* Copy permadeath...*/
  if (IS_PC1_SET (th, PC_PERMADEATH))
    add_flagval (thg, FLAG_PC1, PC_PERMADEATH);
  add_flagval (thg, FLAG_PC1, PC_ASCENDED);
  /* Copy player set flags. */
  add_flagval (thg, FLAG_PC2, 
	       (flagbits(th->flags, FLAG_PC2) & ~PC2_MAPPING));
  sprintf (buf, "\x1b[1;31mWoot! %s just ascended!\x1b[0;37m\n\r", NAME(th));
  free_thing (th);
  thg->fd->connected = CON_GO_INTO_SERVER;
  write_playerfile (thg);
  thing_to (thg, find_thing_num (thg->align+100));
  connect_to_game (thg->fd, "");
  echo (buf);
  echo (buf);
  echo (buf);
  echo (buf);
  echo (buf);
  return;
}
/* Finds the amount of exp needed for the next level. */

int
exp_to_level (int lev)
{
  if (lev < 1 || lev > MORT_LEVEL)
    return 0;
  return lev *lev *lev *lev + 15 * lev * lev +
    5 * lev * lev * lev;
}


/* This lets you slay (monsters only). The first function is to keep 
   accidental slays from happening. */

void 
do_sla (THING *th, char *arg)
{
  stt("You must type slay fully.\n\r", th);
  return;
}


void
do_slay (THING *th, char *arg)
{
  THING *vict, *victn;
  
  if (!th || !IS_PC (th) || LEVEL (th) != MAX_LEVEL ||
      !th->in)
    return;

  if (!arg || !*arg)
    {
      stt ("slay <victim> or slay all\n\r", th);
      return;
    }

  if (!str_cmp (arg, "all"))
    {
      for (vict = th->in->cont; vict; vict = victn)
	{
	  victn = vict->next_cont;
	  if (IS_PC (vict) || !CAN_FIGHT(vict))
	    continue;
	  act ("@1n slay@s @3n.", th, NULL, vict, NULL, TO_ALL);
	  get_killed (vict, th);
	}
      return;
    }

  if ((vict = find_thing_in (th, arg)) == NULL)
    {
      stt ("They aren't here to slay!\n\r", th);
      return;
    }

  if (IS_PC (vict))
    {
      stt ("You can't slay players!\n\r", th);
      return;
    }
  
  act ("@1n slay@s @3n.", th, NULL, vict, NULL, TO_ALL);
  get_killed (vict, th);
  return;
}
  
  
