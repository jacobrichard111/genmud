#include<ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serv.h"

void
combat_ai (THING *th)
{
  THING *weakest_opponent = NULL, *fighting, *vict, *weakest_ally = NULL;
  int min_hps, weakest_ally_hp_pct;
  THING *strongest_pc = NULL, *attacker;
  int max_pc_dam;
  
  int relative_power;
  
  /* These are used to calculate the odds and relative power of the
     two sides. */
  
  int friend_count, enemy_count;
  int friend_power , enemy_power;
  int friend_hp, enemy_hp;
  
  
  /* Make sure this thing is in the correct state to attempt combat
     tactics. */
  
  if (nr(1,3) != 2 || (fighting = FIGHTING (th)) == NULL || !th->in ||
      th->position != POSITION_FIGHTING || IS_PC (th) || 
      !CAN_TALK (th) || IS_ACT1_SET (th, ACT_DUMMY))
    return;
  
  /* Init vars. */
  min_hps = 2000000;
  weakest_ally_hp_pct = 40;
  max_pc_dam = 0;
  relative_power = 0;
  
  /* These are used to calculate the odds and relative power of the
     two sides. */
  
  friend_count = 0;
  enemy_count = 0;
  friend_power = 0;
  enemy_power = 0;
  friend_hp = 0;
  enemy_hp = 0;
  
  
  
  /* Sometimes */

  if (nr (1,5) == 2)
    {
  /* Find the strongest and weakest opponents and the weakest member
     on your side. This code really sucks because it requires things
     to be updated for each thing fighting each combat pulse. This may
     be fixed up someday so it's a lot faster like saving the data for use
     over several combat pulses. Except that the weakest, strongest
     enemies/friends can change so fast that it might not be worth it. */
  
  for (vict = th->in->cont; vict; vict = vict->next_cont)
    {
      if (!CAN_FIGHT (vict))
	continue;
      
      if (is_friend (th, vict))
	{
	  /* Find the weakest ally who is being attacked. */
	  
	  if (vict != th && 
	      th->hp > th->max_hp/2 &&    FIGHTING(vict) &&
	      vict->max_hp > 0 &&
	      vict->hp*100/vict->max_hp < weakest_ally_hp_pct)
	    {
	      for (attacker = th->in->cont; attacker; attacker = attacker->next_cont)
		if (FIGHTING(attacker) == vict)
		  {
		    weakest_ally_hp_pct = vict->hp*100/vict->max_hp;
		    weakest_ally = vict;
		    break;
		  }		
	    } 
	  friend_hp += vict->hp;
	  friend_power += POWER(vict);
	  friend_count++;
	}
      else if (is_enemy (th, vict) && can_see (th, vict))
	{
	  if (vict != fighting)
	    {
	      if (IS_PC (vict) && vict->pc->damage_rounds > 0 &&
		  vict->pc->damage_total/vict->pc->damage_rounds > max_pc_dam)
		{
		  max_pc_dam = vict->pc->damage_total/vict->pc->damage_rounds;
		  strongest_pc = vict;
		}
	      /* Weakest opponent must be fighting. */
	      
	      if (vict->hp < min_hps && FIGHTING (vict))
		{
		  min_hps = vict->hp;
		  weakest_opponent = vict;
		}
	    }
	  enemy_hp += vict->hp;
	  enemy_power += POWER(vict);
	  enemy_count++;	  
	}
    }
    } /* Only sometimes. */
  /* Sanity checks to avoid divide by 0. */
  

  if (enemy_hp < 1)
    enemy_hp = 1;
  if (enemy_power < 1)
    enemy_power = 1;
  if (enemy_count < 1)
    enemy_count = 1;

  
  /* The relative strength is a combination of the relative numbers,
     powers and hps in the room. If you are too weak relative to the
     other side, you will tend to start fleeing.

     The strength is determined by the following.

     For each of the three things: hp, power, count, we find

     MID (0, 4*friend_xxx/enemy_xxx, 8)
     So, if you have the same power, you end up with a 4.
     
     We then take these numbers and add them to relative_power.
     We obtain a strength from 0 to 24 and the larger the number, the
     more we are kicking butt. In general, the smaller the number is,
     the more likely we are to get the heck out of town. If it is
     close to even or if we're winning, we attack, sometimes by rescuing
     someone on our own side who's hurt, sometimes by attacking the
     weakest enemy, sometimes by trying to keep them from leaving. */
  
  relative_power = 
    MID (0, (4*friend_hp/enemy_hp), 8) +
    MID (0, (4*friend_power/enemy_power), 8) +
    MID (0, (4*friend_count/enemy_count), 8);
  
  /* Flee if you're getting beaten. */

  if (nr (1,6) > relative_power && th->hp < th->max_hp/3)
    {
      do_flee (th, "");
      return;
    }
  
  /* If someone in your group is weak, and they're lower than your level,
     and you're not hurt too badly, attempt to rescue them. */
  
  if (weakest_ally && relative_power >= nr (5,12) &&
      th->hp > th->max_hp*2/3 &&
      nr (1,3) == 2)
    { /* Rescue_victim is here instead of do_rescue so we don't have
	 to deal with funky name conversion problems. */
      rescue_victim (th, weakest_ally);
      return;
    }
  
  /* If you're winning by a lot, try to attack the strongest
     player. */

  if (relative_power >= 20 && strongest_pc && nr (1,2) == 2 &&
      fighting != strongest_pc)
    {
      multi_hit (th, strongest_pc);
      return;
    }
  
  /* If you're not fighting the weakest opponent, attempt to attack
     it. */
  
  
  if (weakest_opponent && weakest_opponent != fighting &&
      nr (1,5) == 2)
    {
      multi_hit (th, weakest_opponent);
      return;
    }
  
  
  /* If your victim is getting close to dead, or if you're winning,
     attempt to bash or tackle. */
  
  if (fighting->position != POSITION_TACKLED &&
      fighting->position != POSITION_STUNNED &&
      (nr (1,12) == 3 || 
       fighting->hp < fighting->max_hp/3 ||
       relative_power >= nr (12,24)) &&
      nr (1,6) == 2)
    {
      if (nr (1,8) == 2)
	do_tackle (th, "");
      else
	do_bash (th, "");
      return;
    }
  
  /* At this point, attempt to kick or flurry. */
  
  if (nr (1,3) != 2)
    do_kick (th, "");
  else
    do_flurry (th, "");
  return;
}
  


