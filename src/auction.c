#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* This auction code is designed to allow every player to auction 
   something at once, and the auctions are indexed by number. The
   auctions are only visible to your own align, and the code should
   make sure that a person cannot bid on more auctions than the
   amount of money they have in the bank. */

/* This makes a new auction struct */

AUCTION *
new_auction (void)
{
  AUCTION *newauction;
  
  if (auction_free)
    {
      newauction = auction_free;
      auction_free = auction_free->next;
    }
  else
    {
      newauction = (AUCTION *) mallok (sizeof (*newauction));
      auction_count++;
    }

  bzero (newauction, sizeof (*newauction));
  newauction->next = auction_list;
  auction_list = newauction;
  newauction->number = ++top_auction;
  return newauction;
}

/* This ends an auction and sends the item back to the seller. */

void
free_auction (AUCTION *auct)
{
  AUCTION *prev;

  if (!auct)
    return;

  if (auct == auction_list)
    {
      auction_list = auction_list->next;
      auct->next = NULL;
    }
  else
    {
      for (prev = auction_list; prev != NULL; prev = prev->next)
	{
	  if (prev->next == auct)
	    {
	      prev->next = auct->next;
	      auct->next = NULL;
	      break;
	    }
	}
    }  

  if (auct->item && auct->seller)
    thing_to (auct->item, auct->seller);	
  
  bzero (auct, sizeof (*auct));
  auct->next = auction_free;
  auction_free = auct;
  return;
}

/* This checks if a thing is either the buyer/seller or item in an auction. */

bool
is_in_auction (THING *th)
{
  AUCTION *auct; 
 
  for (auct = auction_list; auct != NULL; auct = auct->next)
    {
      if (auct->buyer == th || auct->seller == th || auct->item == th)
	return TRUE;
    }
  return FALSE;
}


/* This removes all auctions which contain the thing in question. */

void
remove_from_auctions (THING *th)
{
  AUCTION *auct, *auctn;  

  for (auct = auction_list; auct != NULL; auct = auctn)
    {
      auctn = auct->next;
      if (auct->buyer == th || auct->seller == th || auct->item == th)
	free_auction (auct);
    }
  return;
}

/* This adds up how much money the character has tied up in bids. */

int
total_auction_bids (THING *th)
{
  AUCTION *auct;
  int total = 0;

  for (auct = auction_list; auct != NULL; auct = auct->next)
    {
      if (auct->buyer == th)
	total += auct->bid;
    }
  return total;
}	

/* This sends a message to all of the proper people regarding this auction. */

void
auction_message (AUCTION * auct, char *msg)
{
  char buf[STD_LEN];
  THING *th;

  if (!auct || !msg || !*msg || !auct->item || !auct->seller)
    return;  

  sprintf (buf, "\x1b[1;34m<\x1b[1;32m%s\x1b[1;34m>\x1b[0;37m\n\r", msg);

  for (th = thing_hash[PLAYER_VNUM % HASH_SIZE]; th != NULL; th = th->next)
    {
      if (!IS_PC (th) || DIFF_ALIGN (th->align, auct->align))
	continue;
      show_auction (auct, th);
      stt (buf, th);
    }
  return;
}

/* This shows the information for one auction. */

void
show_auction (AUCTION *auct, THING *th)
{
  char buf[STD_LEN];

  if (!auct || !th || !IS_PC (th))
    return;  

  sprintf (buf, "\x1b[1;32m#\x1b[1;34m[\x1b[1;37m%d\x1b[1;34m]\x1b[0;37m: \x1b[1;37m%s\x1b[0;37m for sale by \x1b[1;34m%s\x1b[0;37m for \x1b[1;32m%d\x1b[0;37m coins to \x1b[1;34m%s\x1b[0;37m.\n\r",  auct->number, NAME (auct->item), NAME (auct->seller), auct->bid,
	   (auct->buyer ? NAME (auct->buyer) : "Noone"));

  stt (buf, th);
  return;
}

/* This command lets you bid on an item up for auction. It checks to 
   make sure that you have enough money in the bank for all the auctions
   you are participating in. */

void
do_bid (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  AUCTION *auct;
  int auction_number = 0, bid_amount = 0, curr_bids = 0;

  if (!IS_PC (th) || !th->in)
    return;

  curr_bids = total_auction_bids (th);
  arg = f_word (arg, arg1);

  if (!is_number (arg1) || !is_number (arg))
    {
      stt ("Syntax bid <amount> <auction number>.\n\r", th);
      return;
    }  

  /* Find the amount to bid and the auction to bid on. */

  bid_amount = atoi (arg1);
  auction_number = atoi (arg);


  /* See if that auction exists. */

  for (auct = auction_list; auct != NULL; auct = auct->next)
    {
      if (auct->number == auction_number)
	break;
    }    

  /* See if the auction exists and if this person can bid on an
     auction for that alignment. */

  if (!auct || DIFF_ALIGN (auct->align, th->align))
    {
      stt ("That auction doesn't exist. Type auction to see a list of current auctions.\n\r", th);
      return;
    }

  /* You can't rebid up your bid. */
  
  if (auct->buyer == th)
    {
      stt ("You already have the highest bid on this item!\n\r", th);
      return;
    }  

  if (auct->seller == th)
    {
      auction_message (auct, "BGPKW!!! The seller just tried to bid up the price on this item, and has been fined 5000 coins.");
      if (th->pc->bank > 0)
	th->pc->bank -= MIN (th->pc->bank, 5000);
      return;
    }  

  /* You can't bid more than you have in the bank. */

  if (bid_amount + curr_bids > th->pc->bank)
    {
      stt ("You don't have enough money for this auction! You may be involved in other already, or you might just be poor.\n\r", th);
      return;
    }

  /* If you're the first bidder, set it up. */

  if (!auct->buyer)
    {
      if (bid_amount >= auct->bid)
	{
	  sprintf (buf, "%s bids %d coins for this item!", NAME (th), bid_amount);
	  auction_message (auct, buf);
	  auct->buyer = th;
	  auct->bid = bid_amount;
	  auct->ticks = 0;
	  auct->num_bids++;
	}
      else
	{
	  sprintf (buf, "You must bid at least %d coins on this item.\n\r", auct->bid);
	  stt (buf, th);
	  return;
	}
    }
  else /* Otherwise someone has already bid. Make sure you have
	  bid enough above their previous bid (at least 100 more). */
    {
      if (bid_amount >= auct->bid + 100)
	{
	  sprintf (buf, "%s bids %d coins for this item!", NAME (th), bid_amount);
	  auction_message (auct, buf);
	  auct->buyer = th;
	  auct->bid = bid_amount;
	  auct->ticks = 0;
	  auct->num_bids++;
	}
      else
	{
	  sprintf (buf, "You must bid at least %d coins on this item. (The minimum bid increment is 100 coins).\n\r", auct->bid + 100);
	  stt (buf, th);
	  return;
	}
    }  
  return;
}

/* This lets you auction an item, list all the current auctions, and halt
   an auction in progress. */

void
do_auction (THING *th, char *arg)
{
  char arg1[STD_LEN];
  THING *obj;
  AUCTION *auct;
  int price;
  bool found = FALSE;
  
  if (!IS_PC (th) || !th->in)
    return;

  /* List all auctions */

  if (arg[0] == '\0' || !str_cmp (arg, "list")) 
    {
      stt ("Current auctions:\n\n\r", th);
      for (auct = auction_list; auct != NULL; auct = auct->next)
	{
	  if (!DIFF_ALIGN (auct->align, th->align))
	    {
	      show_auction (auct, th);
	      found = TRUE;
	    }
	}
      if (!found)
	{
	  stt ("None.\n\r", th);
	}
      return;
    }
  
   /* Stop an auction..it costs to do this. */
  
  if (!str_cmp (arg, "halt"))
    {
      for (auct = auction_list; auct != NULL; auct = auct->next)
	{
	  if (auct->seller == th)
	    {
	      if (auct->item)
		{
		  th->pc->bank -= MAX (250, auct->item->cost/AUCTION_DIVISOR);
		  if (th->pc->bank < 0)
		    th->pc->bank = 0;
		}
	      auction_message (auct, "Auction halted.");
	      free_auction (auct);
	      return;
	    }
	}
      stt ("You don't seem to have anything up for sale.\n\r", th);
      return;
    }
  
  /* Find an item to auction. */
  
  arg = f_word (arg, arg1);  
  if ((obj = find_thing_me (th, arg1)) == NULL)
    {
      stt ("You don't have that item.\n\r", th);
      return;
    }

  /* If it can't be auctioned, don't let it be auctioned. */

  if (IS_OBJ_SET (obj, OBJ_NOAUCTION))
    {
      stt ("You cannot auction this item!\n\r", th);
      return;
    }

  /* Only allow one auction per seller at a time. */

  for (auct = auction_list; auct != NULL; auct = auct->next)
    {
      if (auct->seller == th)
	{
	  stt ("You are already selling something!\n\r", th);
	  return;
	}
    }
    
  /* They must auction the item for a price. And, the price cannot be
     too cheap. */
  
  if (!is_number (arg) || (price = atoi (arg)) < 100)
    {
      stt ("The minimum price is 100.\n\r", th);
      price = 100;
    }

  /* Add the transfer fee. */
  
  price += MAX (250, obj->cost/AUCTION_DIVISOR);
  auct = new_auction ();  
  thing_from (obj);
  auct->item = obj;

  auct->seller = th;
  auct->buyer = NULL;
  auct->align = th->align;
  auct->bid = price;
  auct->ticks = 0;
  auct->num_bids = 0;
  auction_message (auct, "This item is now up for sale!");
  return;
}

/* This goes down the list of auctions and updates them all. They don't
   last forever, and after 4 ticks of no extra bids, the auction either 
   ends if noone bid, or it goes to the highest bidder. Note that there
   are none of those retarded minimum numbers of bids things here. */

void
update_auctions (void)
{
  AUCTION *auct, *auctn;
  char buf[STD_LEN];

  for (auct = auction_list; auct != NULL; auct = auctn)
    {
      auctn = auct->next;
      auct->ticks++;
      
      /* Give the appropriate message for this item. */

      if (auct->ticks <= 1)
	auction_message (auct, "\x1b[1;36mGoing Once!\x1b[0;37m");
      else if (auct->ticks == 2)
	auction_message (auct, "\x1b[1;36mGoing Twice!\x1b[0;37m");
      else if (auct->ticks == 3)
	auction_message (auct, "\x1b[1;36mGoing Thrice!\x1b[0;37m");
      else
	{
	  /* If noone wants the item, return it to the seller. */
	  if (!auct->buyer)
	    {
	      auction_message (auct, "\x1b[1;36mNoone wanted it, so it goes back to its owner!\x1b[0;37m");
	      if (auct->item && auct->seller)
		thing_to (auct->item, auct->seller);
	      auct->item = NULL;
	      free_auction (auct);
	    }
	  else 
	    { /* Otherwise give it to the buyer and transfer the money. */
	      sprintf (buf, "\x1b[1;36mSold to %s for %d coins!\x1b[0;37m", 
		       NAME (auct->buyer), auct->bid);
	      auction_message (auct, buf);
	      thing_to (auct->item, auct->buyer);
	      if (IS_PC (auct->buyer))
		auct->buyer->pc->bank -= MIN (auct->bid, auct->buyer->pc->bank);
	      if (IS_PC (auct->seller))
		auct->buyer->pc->bank += auct->bid;
	      auct->item = NULL;
	      free_auction (auct);
	    }
	}
    }
  return;
}
		 










