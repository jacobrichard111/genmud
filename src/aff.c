#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* This file deals with affects on things. Affects are flags that
   wear off after a while. */

/* This is what gets called so that if you have an affect on you from a
   spell, and you have the spell cast on you again, it checks if there
   is an affect of the same kind on you already, and it just updates
   that affect, rather than making a new affect to go along with the
   old one. */

void
aff_update (FLAG *aff, THING *th)
{
  FLAG *flg;
  
  if (!aff || !th)
    return;
  
  if (aff->type >= AFF_MAX || (aff->type >= AFF_START && !IS_PC (th)))
    {
      free_flag (aff);
      return;
    }
  
  
  for (flg = th->flags; flg; flg = flg->next)
    {
      if (flg->from == aff->from && flg->type == aff->type)
	{
	  if ((aff->timer == 0) == (flg->timer == 0))
	    {
	      if (aff->type < AFF_START || IS_PC (th))
		{
		  aff_from (flg, th);
		  aff_to (aff, th);
		}
	      else 
		free_flag (aff);
	      return;
	    }
	}
    }  
  
  aff_to (aff, th);

  update_kde (th, KDE_UPDATE_HMV | KDE_UPDATE_COMBAT | KDE_UPDATE_STAT);

  
  return;
}

/* This adds a flag to a thing, and it updates some extra information
   if the thing happens to be a PC. You should call this only from
   inside of aff_update() so that you don't end up with 10 flags of 
   the same type on one thing. */

void
aff_to (FLAG *aff, THING *th)
{
  if (!aff || !th)
    return;
  
  if (!th)
    {
      free_flag (aff);
      return;
    }
  
  aff->next = th->flags;
  th->flags = aff;

  set_up_move_flags (th);
  if (!IS_PC (th) || aff->type < NUM_FLAGTYPES || aff->type >= AFF_MAX)
    return;
  
  th->pc->aff[aff->type] += aff->val;
  if (aff->type == FLAG_AFF_HP)
    th->max_hp += aff->val;
  if (aff->type == FLAG_AFF_MV)
    th->max_mv += aff->val;
  if (aff->type == FLAG_AFF_ARMOR)
    th->armor += aff->val; 
  
  if (aff->type == FLAG_HURT &&
      IS_SET (aff->val, AFF_FIRE | AFF_WATER))
    calculate_warmth (th);
  
  update_kde (th, KDE_UPDATE_HMV | KDE_UPDATE_COMBAT | KDE_UPDATE_STAT);
  return;
}

/* This removes an affect from a thing, and if it is a PC, some other 
   information gets affected also. */

void
aff_from (FLAG *aff, THING *th)
{
  FLAG *prev;
  bool found = FALSE;
  
  if (!aff)
    return;
  
  if (!th)
    {
      free_flag (aff);
      return;
    }
  
  if (aff == th->flags)
    {
      th->flags = th->flags->next;
      aff->next = NULL;
      found = TRUE;
    }
  else
    {
      for (prev = th->flags; prev; prev = prev->next)
	{
	  if (prev->next == aff)
	    {
	      prev->next = prev->next->next;
	      aff->next = NULL;
	      found = TRUE;
	      break;
	    }
	}
    }
  
  if (IS_PC (th) && aff->type >= AFF_START && aff->type < AFF_MAX)
    {
      th->pc->aff[aff->type] -= aff->val;
      if (aff->type == FLAG_AFF_HP)
	th->max_hp -= aff->val;
      if (aff->type == FLAG_AFF_MV)
	th->max_mv -= aff->val;
      if (aff->type == FLAG_AFF_ARMOR)
        th->armor -= aff->val;
       update_kde (th, KDE_UPDATE_HMV | KDE_UPDATE_COMBAT | KDE_UPDATE_STAT);
      
    }
  if (aff->type == FLAG_HURT && IS_SET (aff->val, AFF_SLEEP) && !IS_PC (th))
    do_wake (th, "");
  
  if (IS_PC (th) && aff->type == FLAG_HURT &&
      IS_SET (aff->val, AFF_FIRE | AFF_WATER))
    calculate_warmth (th);
  if (IS_ROOM (th) && aff->type == FLAG_ROOM1)
    set_up_map_room (th);
  free_flag (aff); 
  set_up_move_flags (th);
  return; 
}

/* This removes flags based on what the flag is from (like which spell),
   or based on the kind of flag (such as hurt, or dam or whatever).
   If the type or the from is nonzero, we remove things which either
   have the proper from or the proper type. It also allows you to 
   only pick things with the proper value. Type + value should go
   together. */

void
afftype_remove (THING *th, int from, int type, int val)
{
  FLAG *flg, *flgn;
  
  if (!th || (!from && !type && !val))
    return;
  
  for (flg = th->flags; flg; flg = flgn)
    {
      flgn = flg->next;
      if ((from == 0 || flg->from != from) &&
	  (type == 0 || flg->type != type))
	continue;
      if (type && val && !IS_SET (flg->val, val))
	continue;
      aff_from (flg, th);
    }
  return;
}



/* This lists the affects you have on you. It could use some cleaning up */

void
do_affects (THING *th, char *arg)
{
  if (!IS_PC (th))
    return;
  stt ("You are affected by:\n\n\r", th);
      
  stt (show_flags (th->flags, 0, LEVEL (th) >= BLD_LEVEL), th);
  return;
}

/* Removes all affects except permanent ones (unless specified) */

void
remove_all_affects (THING *th, bool remove_perma)
{
  FLAG *flg, *flgn;
  
  for (flg = th->flags; flg != NULL; flg = flgn)
    {
      flgn = flg->next;      
      if (flg->type == FLAG_PC1 || 
	  flg->type == FLAG_PC2)
	continue;
      
      if (flg->timer != 0 || flg->from != 0 || remove_perma)
	aff_from (flg, th);
    }
  return;
}
  


/* This adds or updates a permaflag of the appropriate type with the
   appropriate value. */

void
add_flagval (THING *th, int type, int val)
{
  FLAG *flg;
  
  if (!th || type < 0 || type >= NUM_FLAGTYPES ||
      flaglist[type].whichflag == NULL || val == 0)
    return;
  
  for (flg = th->flags; flg; flg = flg->next)
    {
      if (flg->type == type && flg->timer == 0)
	{
	  flg->val |= val;
	  return;
	}
    }
  
  flg = new_flag ();
  flg->type = type;
  flg->val = val;
  aff_update (flg, th);
  return;
}

/* This removes certain bits from all perma flags that this thing has. */

void
remove_flagval (THING *th, int type, int val)
{
  FLAG *flg, *flgn;
  
   if (!th || type <0 || type >= NUM_FLAGTYPES ||
      flaglist[type].whichflag == NULL || val == 0)
    return;

   for (flg = th->flags; flg; flg = flgn)
     {
       flgn = flg->next;
       if (flg->type == type)
	 RBIT (flg->val, val);
       if (flg->val == 0)
	 aff_from (flg, th);
     }
   return;
}

  
