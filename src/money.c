#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* Total the money starting with the lowest denomination and moving up
   to the highest. THERE IS NO CHECKING FOR OBNOXIOUSLY BIG AMOUNTS
   OF MONEY SO IF THIS IS A PROBLEM, YOU MUST FIGURE OUT HOW IT GOT 
   THAT WAY AND DEAL WITH IT!!!!!! */

int
total_money (THING *th)
{
  int i, total = 0;
  VALUE *money;
  
  if (!th || (money = FNV (th, VAL_MONEY)) == NULL)
    {
      return 0;
    }
  
  for (i = 0; i < NUM_COINS; i++)
    {
      /* Make the multiplier bigger */
      total += money->val[i] * money_info[i].flagval;
     
    }
  return total;
}


void 
add_money (THING *th, int amount)
{
  VALUE *money;
  int i, amount_left = amount;
  
  if (!th || !amount)
    return;
  
  money = find_money (th);
  
  
  for (i = NUM_COINS -1; i >= 0; i--)
    {
      /* Add the new coins */ 
      money->val[i] += amount_left / money_info[i].flagval;
      
      /* Subtract off the value of the coins already used. 
	 Basically this is the amount mod the current coin amt. */
      
      amount_left %= money_info[i].flagval;
    }
  
  return;
}

/* This will remove a certain amount of money from a thing. If it has
   no money, then nothing gets done. If it has money, but not enough,
   it gets zeroed out and the money value is removed. */


void
sub_money (THING *th, int amount)
{
  VALUE *money;
  int i, amount_left = amount, curr_coins;
  



  /* If it has no money or the amount is 0, don't bother. */

  if (amount == 0 ||
      (money = FNV (th, VAL_MONEY)) == NULL)
    return;
  
  /* If the thing lacks the money to pay this price, just remove the money
     value. */


  if (total_money (th) < amount)
    {
      remove_value (th, money);
      return;
    }
  

  /* At this point, the amount is less than the money the thing has, and
     the thing actually has money. We now make sure it has enough
     "change"...that is we will go down the list of money to make sure
     that is has enough of the lower coin types for each big coin it has
     just so there isn't alot of adding and subtracting. No, this is
     not "perfect" in a strict anal-retentive roleplaying intensive OMG!!!!!!
     sense, but it is close enough for me and mostly makes the illusion of
     coins being exchanged. */

  /* If we have coins of this big type, and there aren't enough 
     coins of the next smaller type down to totally "change" a coin
     of this type, we "change" one of the bigger coins for them
     and put the change into the smaller value. */
  
  for (i = NUM_COINS -1; i > 0; i--)
    {
      if (money->val[i] > 0 && money->val[i - 1] < (int) (money_info[i].flagval/ money_info[i - 1].flagval))
	{
	  money->val[i]--;
	  money->val[i - 1] += money_info[i].flagval/money_info[i - 1].flagval;
	}
    }
  
  /* We now have enough "change" */

  for (i = NUM_COINS - 1; i >= 0; i--)
    {
      /* The total number of coins of this type we will remove will
	 be the minimum of the amount of coins we have at this level,
	 and the amount we have left/the value of each of these coins. */

      curr_coins = MIN (money->val[i], (int) (amount_left/money_info[i].flagval));
      
      /* Reduce by this many coins */
      
      money->val[i] -= curr_coins;
      
      /* Reduce the amount by the number of coins times the value of each. */

      amount_left -= curr_coins * money_info[i].flagval;
      
      if (amount_left < 1)
	break;
    }

  amount_left = 0;
  
  /* If we got rid of ALL of our coins, we get rid of the money struct. */
  
  
  if (total_money (th) == 0)
    {
      remove_value (th, money);
    }
  
  return;
}



void
show_money (THING *th, THING *target, bool inside_person)
{
  char buf[STD_LEN];
  char buf2[STD_LEN];
  VALUE *money;
  int i, top_coin = 0, cointypes = 0, cointypes_used = 0;

  if (!th || !target ||
      (money = FNV (target, VAL_MONEY)) == NULL)
    {
      return;
    }

  for (i = 0; i < NUM_COINS; i++)
    if (money->val[i] > 0)
      {
	top_coin = money->val[i];
	cointypes++;
      }
  if (cointypes == 0)
    return;
  if (inside_person)
    sprintf (buf, "%s %s", name_seen_by (th, target), IS_SET (target->thing_flags, TH_NO_MOVE_SELF) ? "contains" : (target == th ? "have" : "has"));
  else
    sprintf (buf, "There %s", top_coin < 2 ? "is" : "are");
  buf[0] = UC(buf[0]); 
  for (i = NUM_COINS - 1; i >= 0; i--)
    {
      if (money->val[i] == 0)
	continue;
      
      if (cointypes_used > 0 && cointypes_used < cointypes - 1)
	strcat (buf, ", ");
      else if (cointypes > 1 && cointypes_used == cointypes - 1)
	strcat (buf, " and ");
      else
	strcat (buf, " ");
      
      sprintf (buf2, "%d %s coin%s", money->val[i], money_info[i].app, 
	       (money->val[i] > 1 ? "s" : ""));
      strcat (buf, buf2);
      cointypes_used++;
    }
  if (!inside_person)
    strcat (buf, " here");
  strcat (buf, ".\n\r");
  stt (buf, th);
  return;
}


int
get_coin_type (char *arg)
{
  int i;
  if (!arg || !*arg)
    return NUM_COINS;
  
  for (i = 0; i < NUM_COINS; i++)
    {
      if (!str_cmp (money_info[i].name, arg))
	return i;
    }
  return NUM_COINS;
}




VALUE *
find_money (THING *th)
{
  VALUE *money;
  
  if ((money = FNV (th, VAL_MONEY)) == NULL)
    {
      money = new_value ();
      money->type = VAL_MONEY;
      add_value (th, money);
      money->word[0] = '\0';
    }
  return money;
}



void
do_split (THING *th, char *arg)
{
  THING *vict;
  int gm_here = 0, i = 0, type = NUM_COINS, amt = 0, tmoney = 0;
  int to_each = 0, remainder = 0, gm_used = 0, curr_amt;
  char arg1[STD_LEN];
  char buf[STD_LEN];
  VALUE *money;
  
  if (!IS_PC (th) || !th->in)
    return;
  
  arg = f_word (arg, arg1);
  
  
  if (!is_number (arg1))
    {
      stt ("split <amount> or split <amount> <cointype>.\n\r", th);
      return;
    }
  
  if ((amt = atoi (arg1)) < 1)
    {
      stt ("Try splitting a positive amount.\n\r", th);
      return;
    }
  
  if ((money = FNV (th, VAL_MONEY)) == NULL ||
      (tmoney = total_money (th)) == 0)
    {
      stt ("You don't have any money.\n\r", th);
      return;
    }

  /* Get num groupies in the room. */
  
  for (vict = th->in->cont; vict != NULL; vict = vict->next_cont)
    {
      if (in_same_group (vict, th))
	gm_here++;
    }
  
  if (gm_here < 2)
    {
      stt ("You keep everything.\n\r", th);
      return;
    }
  
  if (*arg && (type = get_coin_type (arg)) == NUM_COINS)
    {
      stt ("Syntax: split <amount> <cointype>.\n\r", th);
      return;
    }
  
  /* Do split a certain kind of money. */

  if (type < NUM_COINS)
    {
      if (money->val[type] < amt)
	{
	  sprintf (buf, "You don't have that many %s coins to split!\n\r", money_info[type].name);
	  stt (buf, th);
	  return;
	}
      
      to_each = amt/gm_here;
      remainder = amt % gm_here;
      sub_money (th, amt*money_info[i].flagval);
          

      act ("@1n split@s some money.", th, NULL, NULL, NULL, TO_ALL);
      for (vict = th->in->cont; vict != NULL; vict = vict->next_cont)
	{
	  gm_used++;
	  curr_amt = to_each + (gm_used < remainder ? 1 : 0);
	  sprintf (buf, "You receive %d %s coin%s.\n\r", curr_amt, money_info[type].app, (curr_amt != 1 ? "s" : ""));
	  stt (buf, vict);
	  add_money (vict, curr_amt* money_info[i].flagval);
	}
      return;
    }
	
  if (tmoney < amt)
    {
      stt("You don't have enough money to split that much!\n\r", th);
      return;
    }
  
  to_each = amt/gm_here;
  remainder = amt % gm_here;
  sub_money (th, amt);
  
  
  act ("@1n split@s some money.", th, NULL, NULL, NULL, TO_ALL);
  for (vict = th->in->cont; vict != NULL; vict = vict->next_cont)
    {
      gm_used++;
      curr_amt = to_each + (gm_used < remainder ? 1 : 0);
      sprintf (buf, "You receive %d coin%s.\n\r", curr_amt, (curr_amt != 1 ? "s" : ""));
      stt (buf, vict);
      add_money (vict, curr_amt);
    }
  return;
}

      
