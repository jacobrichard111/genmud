#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* These functions let you do stuff at the bank. You can deposit or
   withdraw money, check your account balance, and store and unstore
   equipment (for a fee). Bankers re just things set with a certain
   flag, and yes you can set up ATM's by just making objects with
   the flag set. */


/* This function takes money you have on you and places it into your
   bank account. */


void
do_deposit (THING *th, char *arg)
{
  int value;
  char buf[STD_LEN];
  THING *banker;
  
  if (!th || !IS_PC (th) || !th->in)
    return;

  /* Find out how much to deposit. */

  if (!str_cmp (arg, "all"))
    value = total_money (th);

  else if (!is_number (arg) || atoi (arg) < 1)
    {
      stt ("Syntax: deposit all or <number>.\n\r", th);
      return;
    }
  else
    value = atoi (arg);
  
  /* Check if a banker is here. */
  
  for (banker = th->in->cont; banker != NULL; banker = banker->next_cont)
    {
      if (IS_ACT1_SET (banker, ACT_BANKER))
	break;
    }

  if (!banker)
    {
      stt ("There is no banker here.\n\r", th);
      return;
    }
  
  /* If it can't see the player, then bail out. */

  if (!can_see (banker, th))
    {
      do_say (banker, "I won't do business with someone I can't see.");
      return;
    }

  /* You can't deposit more than you have. */

  if (value > total_money (th))
    {
      stt ("You don't have that much money to deposit.\n\r", th);
      return;
    }

  /* Deposit the money. */

  sprintf (buf, "Ok, you deposit %d coins.\n\r", value);
  stt (buf, th);
  th->pc->bank += value;
  sub_money (th, value);
  return;
}
  
/* This command lets you see how much money you have in your bank
   account. */

void
do_account (THING *th, char *arg)
{
  char buf[STD_LEN];
  THING *banker;

  if (!th || !IS_PC (th) || !th->in)
    return;

  /* Find a banker. */

  for (banker = th->in->cont; banker != NULL; banker = banker->next_cont)
    {
      if (IS_ACT1_SET (banker, ACT_BANKER))
	break;
    }

  if (!banker)
    {
      stt ("There is no banker here.\n\r", th);
      return;
    }

  /* Make sure it can see the player. */

  if (!can_see (banker, th))
    {
      do_say (banker, "I won't do business with someone I can't see.");
      return;
    }

  /* Give the account data. */

  sprintf (buf, "You have %d coins in your account. Have a nice day!! =D\n\r", th->pc->bank);
  stt (buf, th);
}

/* This command takes money out of the bank and places it into your
   inventory. */

void
do_withdraw (THING *th, char *arg)
{
  int value;
  char buf[STD_LEN];
  THING *banker;
  
  if (!th || !IS_PC (th) || !th->in)
    return;

  /* Find out how much you want to withdraw. */

  if (!str_cmp (arg, "all"))
    value = th->pc->bank;
  else if (!is_number (arg) || atoi (arg) < 1)
    {
      stt ("Syntax: withdraw all or <number>.\n\r", th);
      return;
    }
  else
    value = atoi (arg);  

  /* Find a banker. */

  for (banker = th->in->cont; banker != NULL; banker = banker->next_cont)
    {
      if (IS_ACT1_SET (banker, ACT_BANKER))
	break;
    }

  if (!banker)
    {
      stt ("There is no banker here.\n\r", th);
      return;
    }

  /* If the banker can't see the player, bail out. */

  if (!can_see (banker, th))
    {
      do_say (banker, "I won't do business with someone I can't see.");
      return;
    }

  /* Don't let people in auctions withdraw money... Nice try tho. :) */

  if (is_in_auction (th))
    {
      stt ("You cannot withdraw money while you are involved in an auction.\n\r", th);
      return;
    }

  /* Don't let them withdraw more than their account balance. */

  if (value > th->pc->bank)
    {
      stt ("You don't have that much money to withdraw!\n\r", th);
      return;
    }

  sprintf (buf, "You withdraw %d coins. You keep %d coins and the bank keeps %d coins as interest. Have a nice day!! =D\n\r", value, value - MAX(1,value/WITHDRAWAL_DIVISOR), MAX(1,value/WITHDRAWAL_DIVISOR));
  stt (buf, th);

  /* Remove the money from the bank. */

  th->pc->bank -= value;
  
  /* Give the player the amount they withdrew, minus a few percent for
     the "withdrawal fee" so that players don't constantly add and
     remove money from the bank. */

  add_money (th, value - MAX(1,value/WITHDRAWAL_DIVISOR));
  return;
}
  
/* This command lets you take most items from your inventory and store
   them in the bank. This costs money, and some items aren't allowed to
   be stored. */


void
do_store (THING *th, char *arg)
{
  THING *obj, *banker;
  int i, slot = MAX_STORE;
  char buf[STD_LEN];
  bool found = FALSE;
  bool actually_storing = FALSE;
  char arg1[STD_LEN];

  if (!th || !IS_PC (th) || !th->in)
    return;
  
  /* Find the thing to store in the bank. */

  arg = f_word (arg, arg1);
  if (!str_cmp (arg, "yes"))
    actually_storing = TRUE;

  /* Can't store if you have a timer. */
  
  if (th->pc->no_quit > 0)
    {
      stt ("You can't store anything while your quit timer is active.\n\r", th);
      return;
    }


  /* Find a banker. */

  for (banker = th->in->cont; banker != NULL; banker = banker->next_cont)
    {
      if (IS_ACT1_SET (banker, ACT_BANKER))
	break;
    }

  if (!banker)
    {
      stt ("There is no banker here.\n\r", th);
      return;
    }


  /* If the banker can't see the player, bail out. */

  if (!can_see (banker, th))
    {
      do_say (banker, "I won't do business with someone I can't see.");
      return;
    }

  /* If you just want to see what you have stored, you can get a list
     at the bank. */

  if (arg1[0] == '\0')
    {
      stt ("You have the following items in storage:\n\n\r", th);
      for (i = 0; i < MAX_STORE; i++)
	{
	  if (th->pc->storage[i] != NULL)
	    {
	      found = TRUE;
	      sprintf (buf, "[%2d]  %-30s.  %d coins to get it back.\n\r", 
		       i + 1, NAME (th->pc->storage[i]), th->pc->storage[i]->cost/STORAGE_DIVISOR);
	      stt (buf, th);
	    }
	}
      if (!found)
	stt ("Nothing.\n\r", th);
      return;
    }
  

  /* Find the item to store. */

  if ((obj = find_thing_me (th, arg1)) == NULL)
    {
      stt ("You don't have that item to store.\n\r", th);
      return;
    }

  /* Don't let people store containers with stuff in them. This could
     lead to too much cheaterness. Nice try tho. :) */

  if (obj->cont)
    {
      stt ("Sorry, the bank won't store anything unless it's empty.\n\r", th);
      return;
    }

  /* Don't store really cheap items, or items that are set as nostore. 
     OBJ_NOSTORE is a good flag for powerful items so that players
     don't store good kits so that people can pk and get the good eq
     back. */
  
  if (obj->cost/STORAGE_DIVISOR < 5 || IS_OBJ_SET (obj, OBJ_NOSTORE))
    {
      stt ("We will not store that item.\n\r", th);
      return;
    }

  /* You must type store <obj> yes to store an item, so if you just
     wanted to know how much it costs, don't put the yes there and
     you get this: */
  
  if (!actually_storing)
    {
      sprintf (buf, "The cost to store this item is %d coins.\n\r", obj->cost/STORAGE_DIVISOR);
      stt (buf, th);
      return;
    }
  
  /* If you're in an auction you can't store anything (because you 
     are charged for storing and you might have too little money to
     cover your bid on an item. */

  if (is_in_auction (th))
    {
      stt ("You cannot do any banking while you are involved in an auction.\n\r", th);
      return;
    }
  
  /* Find out if you have a free storage slot. */
  
  for (i = 0; i < MAX_STORE; i++)
    {
      if (th->pc->storage[i] == NULL)
	{
	  slot = i;
	  break;
	}
    }

  /* If no free storage slots, bail out. */

  if (i == MAX_STORE)
    {
      stt ("You don't have any more storage slots!\n\r", th);
      return;
    }

  /* If you can't afford the storage fee, bail out. */

  if (th->pc->bank < (int) obj->cost/STORAGE_DIVISOR)
    {
      stt ("You don't have enough money to store this item!\n\r", th);
    }
  else
    /* At this point, you have the item, and it's not too cheap and
       not nostore and you're not in an auction and you have a free
       storage slot, so store the item. */    
    {      
      thing_from (obj);
      th->pc->storage[i] = obj;
      stt ("Ok, the item is stored.\n\r", th);
      th->pc->bank -= obj->cost/STORAGE_DIVISOR;
    }
  return;
}

/* This lets you go to the bank and unstore an item. It costs money to
   do this. */

void
do_unstore (THING *th, char *arg)
{
  THING *banker, *obj = NULL;
  int slot = MAX_STORE;
  char buf[STD_LEN];
  char arg1[STD_LEN];
  bool actually_unstoring = FALSE;

  if (!th || !IS_PC (th) || !th->in)
    return;
  
  /* Find the item to unstore. */

  arg = f_word (arg, arg1);  
  if (!str_cmp (arg, "yes"))
    actually_unstoring = TRUE;
  
  /* Find a banker. */

  for (banker = th->in->cont; banker != NULL; banker = banker->next_cont)
    {
      if (IS_ACT1_SET (banker, ACT_BANKER))
	break;
    }

  if (!banker)
    {
      stt ("There is no banker here.\n\r", th);
      return;
    }

  /* If the banker can't see you, bail out. */

  if (!can_see (banker, th))
    {
      do_say (banker, "I won't do business with someone I can't see.");
      return;
    }
  
  /* You must specify something to unstore. */

  if (arg1[0] == '\0')
    {
      stt ("Unstore what?\n\r", th);
      return;
    }
  
  /* If you're in an auction, you can't unstore anything. */
  
  if (is_in_auction (th))
    {
      stt ("You cannot unstore something while you are involved in an auction.\n\r", th);
      return;
    }

  /* Unstore by number. */

  if (isdigit (arg1[0]))
    {
     slot = atoi (arg1);
     slot--;
     if (slot < 0 || slot >= MAX_STORE ||
	 th->pc->storage[slot] == NULL)
       {
	 stt ("There is nothing in that slot!\n\r", th);
	 return;
       }
     else
       obj = th->pc->storage[slot];
   }
  else /* Unstore by name. */
   {
     for (slot = 0; slot < MAX_STORE; slot++)
       {
	 if (th->pc->storage[slot] != NULL &&
	     is_named (th->pc->storage[slot], arg1))
	   {
	     obj = th->pc->storage[slot];
	     break;
	   }
       }
     if (obj == NULL)
       {
	 stt ("You don't have that item stored!\n\r", th);
	 return;
       }
   }

  /* If you aren't unstoring now, tell how much it costs to unstore and
     bail out. */
 
 if (!actually_unstoring)
   {
     sprintf (buf, "The cost to unstore this item is %d coins.\n\r", obj->cost/STORAGE_DIVISOR);
      stt (buf, th);
      return;
   }

 /* If you don't have the money, then give an error message and
    bail out. */
 
 if (th->pc->bank < (int) obj->cost/STORAGE_DIVISOR)
   {
     stt ("You don't have enough money to unstore this item!\n\r", th);
     return;
   }     

 /* Transfer the item. */
 
 thing_from (obj);
 thing_to (obj, th);
 th->pc->storage[slot] = NULL;
 stt ("Ok, the item has been retrieved.\n\r", th);
 return;
}

/* Shows you how much money you have and how much you have in the bank. */
void
do_purse (THING *th, char *arg)
{
  int money, bank;
  char buf[STD_LEN];

  if (!IS_PC (th))
    {
      stt ("Players only.\n\r", th);
      return;
    }

  money = total_money (th);
  bank = th->pc->bank;
  
  sprintf (buf, "You have %d coin%s on you and %d coin%s in the bank, for a total of %d coin%s.\n\r",
	   money, (money != 1 ? "s" : ""), bank,  (bank != 1 ? "s" : ""),
	   (money + bank), (money + bank != 1 ? "s" : ""));
  stt (buf, th);
  return;

}


