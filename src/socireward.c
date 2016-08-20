#include <sys/types.h>
#include<ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "script.h"
#include "society.h"
#include "track.h"
#include "rumor.h"
#include "event.h"


/* This is a list of reward descriptions used when players are rewarded
   for helping societies. */

const char *reward_desc_no_thing[REWARD_MAX] =
  {
    "watching our children",
    "building our village",
    "destroying the enemy village",
    "making us feel better",
    "stopping our awful enemies",
    "giving us those items",
    "rescuing out kidnap victim",
    "using diplomacy",
    "inspiring us",
    "demoralizing our enemies",
    "delivering this package",
  };

const char *reward_desc_thing[REWARD_MAX] =
  {
    "watching",
    "building",
    "destroying",
    "healing",
    "killing",
    "giving us",
    "rescuing",
    "talking to",
    "inspiring",
    "demoralizing",
    "delivering",
  };


/* This makes somebody in the room give the player a reward. */

bool
society_somebody_give_reward (THING *receiver, int reward_type, int reward)
{
  THING *person;
  int num_choices = 0, num_chose = 0, count;
  VALUE *socval;

  if (!receiver || !IS_PC (receiver) || !receiver->in ||
      reward <= 0)
    return FALSE;
  
  for (count = 0; count < 2; count++)
    {
      for (person = receiver->in->cont; person; person = person->next_cont)
	{
	  if ((socval = FNV (person, VAL_SOCIETY)) == NULL ||
	      DIFF_ALIGN (person->align, receiver->align) ||
	      IS_SET (socval->val[2], CASTE_CHILDREN) ||
	      person->position <= POSITION_SLEEPING)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    return FALSE;
	  num_chose = nr (1, num_choices);
	}
    }
  return society_give_reward (person, receiver, reward_type, reward);
}

/* This gives a reward of money, exp and maybe quest points to a person
   who does something good for a society member. */

bool
society_give_reward (THING *giver, THING *receiver, int reward_type, int reward)
{
  char buf[STD_LEN];
  THING *target;
  char target_name[STD_LEN];
  VALUE *build, *socval;
  SOCIETY *soc, *soc2;

  int obj_vnum;
  THING *obj;

  
  *target_name = '\0';
  if (!giver || !receiver || !IS_PC (receiver) ||
      ((soc = find_society_in (giver)) == NULL) ||
      DIFF_ALIGN (giver->align, receiver->align) || 
      giver->position <= POSITION_SLEEPING || 
      IS_ACT1_SET (giver, ACT_PRISONER))
    return FALSE;
  /* Give a description of the reward if possible. */
  if (reward_type >= 0 && reward_type < REWARD_MAX)
    {
      target = receiver->pc->society_rewards_thing[reward_type];
      
      if (target && target->in)
	{
	  if (reward_type != REWARD_DIPLOMACY && 
	      reward_type != REWARD_INSPIRE &&
	      reward_type != REWARD_DEMORALIZE)
	    {
	      if (target == giver)
		strcpy (target_name, "me");
	      else if ((socval = FNV (target, VAL_SOCIETY)) != NULL &&
		       socval->word && *socval->word)
		strcpy (target_name, socval->word);
	      else if (IS_ROOM (target))
		strcpy (target_name, show_build_name (target));
	      else
		strcpy (target_name, NAME(target));
	    }
	  else /* Diplomacy need society name..get it from the fact that
		  the diplomat had to be in room with a val build belonging
		  to a society and that's where we get the name from. */
	    {
	      if (IS_ROOM (target) &&
		  (build = FNV (target, VAL_BUILD)) != NULL &&
		  (soc2 = find_society_num (build->val[0])) != NULL)
		{
		  if (soc2 == soc)
		    strcpy (target_name, "us");
		  else
		    {
		      sprintf (target_name, "the ");
		      strcat (target_name, society_pname (soc2));
		    }
		}
	      else
		target = NULL;
	    }
	  sprintf (buf, "Thank you for %s %s, %s.",
		   reward_desc_thing[reward_type],
		   target_name,
		   NAME (receiver));
	}
      else
	sprintf (buf, "Thank you for %s, %s.", 
		 reward_desc_no_thing[reward_type], NAME (receiver));
    }
  else
    sprintf (buf, "Thank you %s.", NAME (receiver));
  do_say (giver, buf);
  /* Cap the reward. */
  reward = MID (1, reward, REWARD_AMOUNT_MAX);
  
  /* First add the exp. */
  add_exp (receiver, reward*REWARD_EXP_MULT *find_remort_bonus_multiplier(receiver));
  
  add_money (receiver, reward);
  act ("@1n give@s some money to @3f.", giver, NULL, receiver, NULL, TO_ALL);
  
  receiver->pc->quest_points += nr (reward, reward+REWARD_QPS_DIVISOR-1)/REWARD_QPS_DIVISOR;

  /* Give a small chance of giving out an item. */

  if (soc && nr (1,1000) < reward) 
    {
      obj_vnum = nr (soc->object_start, soc->object_end);
      
      if ((obj = create_thing (obj_vnum)) != NULL &&
	  !CAN_FIGHT(obj))
	{
	  do_say (giver, "Here's something extra.");
	  thing_from (obj);
	  thing_to (obj, receiver);
	  act ("@1n give@s @2n to @3n,", giver, obj, receiver, NULL, TO_ALL);
	}
    }
  return TRUE;
}


/* This adds a society reward to the player's total. */

void
add_society_reward (THING *th, THING *to, int type, int amount)
{
  if (!th || !IS_PC (th) || type < 0 || type >= REWARD_MAX ||
      amount < 1)
    return;
  
  /* Dork checking. */
  if (amount > REWARD_AMOUNT_MAX)
    amount = REWARD_AMOUNT_MAX;

  if (th->pc->society_rewards[type] < 0)
    th->pc->society_rewards[type] = 0;
  th->pc->society_rewards[type] += amount;
  th->pc->society_rewards_thing[type] = to;
  return;
}

/* This updates player rewards. The amount of the reward goes down over
   time and it gets updated every several seconds on the player. */

void
update_society_rewards (THING *th, bool payout_now)
{
  int count, num_choices = 0, num_chose = 0, i;
  if (!th || !IS_PC (th))
    return;

  /* Just decrement and exit this. */
  if (!payout_now)
    {
      for (i = 0; i < REWARD_MAX; i++)
	{
	  if (--th->pc->society_rewards[i] < 0)
	    {
	      th->pc->society_rewards[i] = 0;
	      th->pc->society_rewards_thing[i] = NULL;
	    }
	  if (th->pc->society_rewards_thing[i] &&
	      !th->pc->society_rewards_thing[i]->in)
	    th->pc->society_rewards_thing[i] = NULL;
	}
      return;
    }

  /* If your align hate is too high, things won't be nice to you. */
  
  if (th->align < ALIGN_MAX &&
      th->pc->align_hate[th->align] >= ALIGN_HATE_WARN/2)
    return;
     


  /* Do payout now..pick a type of payout and see if we can get paid off. */

  
  for (count = 0; count < 2; count++)
    {
      for (i = 0; i < REWARD_MAX; i++)
	{
	  if (th->pc->society_rewards[i] > 0)
	    {
	      if (count == 0)
		num_choices++;
	      else if (--num_chose < 1)
		break;
	    }
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    break;
	  num_chose = nr (1, num_choices);
	}
    }
  if (i == REWARD_MAX)
    return;
  
  /* Clear this reward if we were given it. */

  if (society_somebody_give_reward (th, i, th->pc->society_rewards[i]))
    {
      th->pc->society_rewards[i] = 0;
      th->pc->society_rewards_thing[i] = NULL;
    }
  return;
}



