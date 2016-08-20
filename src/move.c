#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "script.h"
#include "track.h"
#include "event.h"
#ifdef USE_WILDERNESS
#include "wilderness.h"
#include "wildalife.h"
#endif
/* Ok, this function is the thing that handles ALL kinds of functions like
   do_get do_put do_move do_enter do_leave etc. etc. etc. The basic idea
   is this. Every thing is inside another thing (unless it is an area
   or one of the "real rooms". These things don't EVER move. They
   only be things which are start_in or end_in. 
   
   Any time a thing called mover moves, it is because the initiator
   is making it happen. The thing starts in the start_in thing, and
   tries to end up in the end_in thing. If any of these four things 
   involved will not allow this kind of move, then the move fails
   and everything stays where it is. The flags are listed in serv.h 
   and they should cover most of the cases which are likely. */

bool 
move_thing (THING *initiator, THING *mover, THING *start_in, THING *end_in)
{
  char buf[STD_LEN];
  char buf2[STD_LEN];
  int flags = 0;
  bool was_editing_this = FALSE;
  if (!initiator || !mover || !start_in || !end_in)
    return FALSE;
  
  if (end_in == mover)
    return FALSE;
  if (start_in == end_in)
    return TRUE;
  
  if (IS_WORN(mover))
    return FALSE;

  if (initiator == mover && mover->position == POSITION_MEDITATING)
    {
      stt ("You cannot move while you are meditating!\n\r", mover);
      return FALSE;
    }


  if (end_in->size > 0)
    {
      if ( (int) (mover->carry_weight + mover->weight) >
	  (int) (end_in->size -  end_in->carry_weight))
	{
	  sprintf (buf, "%s is too big to fit inside of ",
		   name_seen_by (initiator, mover));
	  sprintf (buf2, "%s!!\n\r", name_seen_by (initiator, end_in));
	  strcat (buf, buf2);
	  stt (buf, initiator);
	  return FALSE;
	}
    }

  /* Now check all these flags. They restrict certain kinds of
     movement depending on how the things above are all
     related. Most should be able to be figured out after a bit. */


  if (mover != initiator && initiator == end_in)
    {
      if (IS_SET (mover->thing_flags, TH_NO_TAKE_BY_OTHER))
        {
          if (CAN_MOVE (mover))
	    {
	      sprintf (buf, "%s won't let you take it!\n\r", name_seen_by (initiator, mover));
              stt (buf, initiator);
	      return FALSE;
	    }
          else
	    {
              stt ("You can't take that!\n\r", initiator);
              return FALSE;
            }
        }
      /* You cannot pick up a boat or something with people in it. */
      else if (!IS_SET (mover->thing_flags, TH_NO_MOVE_INTO) &&
	       mover->cont)
	{
	  stt ("You can't pick that up! It is occupied!\n\r", initiator);
	  return FALSE;
	}
       else
	  flags |= TRIG_TAKEN;
    }

  /* Note that these two below are more powerful than the "no take by
     other" above. */
        
  if (mover != initiator)
    {
      if(IS_SET (initiator->thing_flags, TH_NO_MOVE_OTHER) ||
	 IS_SET (mover->thing_flags, TH_NO_MOVE_BY_OTHER))
	{  
	  sprintf (buf, "You can't budge %s!\n\r", name_seen_by (initiator, mover));
	  stt (buf, initiator);
	  return FALSE;
	}
      else
	{
	  flags |= TRIG_MOVED;
	}
    }
  
  if (mover == initiator)
    {
      if (!IS_OBJ_SET (mover, OBJ_DRIVEABLE))
	{
	  if (IS_SET (initiator->thing_flags, TH_NO_MOVE_SELF))
	    return FALSE;
	  else
	    flags |= TRIG_MOVING;
	}
      
      if (IS_SET (end_in->thing_flags, TH_NO_MOVE_INTO))
	{
	  sprintf (buf, "You can't move into %s!\n\r", name_seen_by (initiator, end_in));
	  stt (buf, initiator);
	  return FALSE;
	}
      else
	{
	  flags |= TRIG_ENTER;
	}
    }
  
  if (IS_SET (end_in->thing_flags, TH_NO_CONTAIN))
    {
      sprintf (buf, "%s cannot contain anything!\n\r", name_seen_by (initiator, end_in));
       stt (buf, initiator);
       return FALSE;
    }
  
  if (initiator != end_in)
    {
      if (IS_SET (end_in->thing_flags, TH_NO_GIVEN))
	{
	  sprintf (buf, "%s won't take this!\n\r", name_seen_by (initiator, end_in));
	  stt (buf, initiator);
	  return FALSE;
	}
      else
	{
	  flags |= TRIG_GIVEN;
	}
      if (!IS_ROOM (end_in) && IS_SET (mover->thing_flags, TH_NO_GIVE))
	{
	  sprintf (buf, "%s won't let you give it to anyone else!", name_seen_by (mover, initiator));
	  stt (buf, initiator);
	  return FALSE;
	}
    }
  
  if (initiator == start_in && initiator != end_in)
    {
      if (IS_SET (mover->thing_flags, TH_NO_DROP))
	{
	  sprintf (buf, "You can't let go of %s!\n\r", name_seen_by (initiator, mover));
	  stt (buf, initiator);
	  return FALSE;
	}
      else
	{
	  flags |= TRIG_DROPPED;
	}
      if (IS_SET (mover->thing_flags, TH_DROP_DESTROY))
	{
	  act ("@1p @2n goes *poof* as @1h drop@s it!", initiator, mover, NULL, NULL, TO_ALL);
	  free_thing (mover);
	  return FALSE;
	}
    }
  
  if (initiator != start_in)
    {
      if (IS_SET (start_in->thing_flags, TH_NO_TAKE_FROM))
	{
	  sprintf (buf, "%s won't let you take ", name_seen_by (initiator, start_in));
	  sprintf (buf2, "%s from it!\n\r", name_seen_by (initiator, mover));
	  strcat (buf, buf2);
	  stt (buf, initiator);
	  return FALSE;
	}
      else
	{
	  flags |= TRIG_TAKEN_FROM;
	}
    }
  
  if (initiator == end_in)
    {
      if (IS_SET (initiator->thing_flags, TH_NO_CAN_GET))
	{
	  stt ("You can't pick anything up!\n\r", initiator);
	  return FALSE;
	}
      else
	{
	  flags |= TRIG_GET;
	}
    }
  
  if (initiator == mover)
    {
      if(IS_SET(start_in->thing_flags, TH_NO_LEAVE))
	{
	  sprintf (buf, "%s won't let you leave it!\n\r", name_seen_by (initiator, start_in));
	  stt (buf, initiator);
	  return FALSE;
	} 
      else
	{
	  flags |= TRIG_LEAVE;
	}
    }
  
  /* If a bunch of stuff is on top of the mover item, it is all moved
     by the initiator at the same time. Maybe some of it will go,
     maybe some won't. */

  /* Leave this out for a while... */
  
  if (FIGHTING (mover))
    stop_fighting (mover);
  
  check_triggers (initiator, mover, start_in, end_in, (flags & before_trig));

  if (mover->pc && mover->pc->editing == start_in)
    was_editing_this = TRUE;
  
  if (IS_ROOM (end_in) && IS_PC (mover) && LEVEL (mover) < BLD_LEVEL &&
      end_in->in && IS_AREA(end_in->in) && 
      IS_AREA_SET (end_in->in, AREA_CLOSED))
    {
      stt ("That area isn't open!\n\r", mover);
      return FALSE;
    }
 

  /* Now do some checks against problems when saving the world. This
     will not handle all problems, but I doubt I will have the
     resources to run this with a real RDBMS. I don't make this
     happen with pc's because the players would notice. Oh well.

     The idea is don't move an unsaved thing to something that is
     saved or else it won't get saved. */
  
  if (IS_SET (server_flags, SERVER_SAVING_WORLD) &&
      !IS_PC (initiator) && !IS_PC (mover) && !IS_PC (start_in) &&
      !IS_PC (end_in) && !IS_SET (mover->thing_flags, TH_SAVED) &&
      IS_SET (end_in->thing_flags, TH_SAVED))
    {
      /* evidence shows that this problem only creeps up a few times
	 per save cycle...so it's close enough to correct for my
	 purposes. */
      return FALSE;
    }
  

  thing_to (mover, end_in);
  if (was_editing_this && mover->pc)
    mover->pc->editing = end_in;

  /* Set up pk timer. */

  if (IS_PC (mover) && mover->in && LEVEL (mover) < BLD_LEVEL)
    {
      THING *vict;
      bool found = FALSE;
      for (vict = mover->in; vict != NULL; vict = vict->next_cont)
        {
           if (IS_PC (vict) && DIFF_ALIGN (vict->align, mover->align) &&
                LEVEL (vict) < BLD_LEVEL)
             {
               found = TRUE;
               vict->pc->no_quit = NO_QUIT_PK;
             }
         }
      if (found)
        mover->pc->no_quit = NO_QUIT_PK;
     } 
  check_triggers (initiator, mover, start_in, end_in, (flags & after_trig));

  
  return TRUE;
}



/* This function removes a thing from whatever it is in
   and updates the linked list inside of that other thing. */

void 
thing_from (THING *th)
{
  THING *th_in, *in_now = NULL;
  if (!th || IS_AREA (th) || !th->in ||
      (IS_AREA (th->in) && !IS_PC (th)
#ifdef USE_WILDERNESS
       && !IS_WILDERNESS_ROOM (th)
#endif
       ))
    {
      if (!th->in)
	{
	  th->next_cont = NULL;
	  th->prev_cont = NULL;
	}
      return;
    }
  
  in_now = th->in;
  th_in = th;
  while ((th_in = th_in->in) != NULL && !IS_AREA (th_in))
    {
      th_in->carry_weight = MAX (0, th_in->carry_weight - (th->weight + th->carry_weight));
      th_in->light = MAX (0, th_in->light - th->light);
    }		
  update_kde (th->in, KDE_UPDATE_WEIGHT);	 
  stop_fighting (th);
  
  if (thing_cont_pointer == th)
    thing_cont_pointer = thing_cont_pointer->next_cont;
  
  
  if (th->in->cont == th)
    {
      th->in->cont = th->next_cont;
      if (th->next_cont)
	th->next_cont->prev_cont = NULL;
      if (th->prev_cont)
	{
	  char errbuf[STD_LEN];
	  sprintf (errbuf, "ERROR_THING_FROM %d %s %s %d\n", th->in->vnum, NAME(th), NAME (th->prev_cont), th->prev_cont->vnum);
	  log_it (errbuf);
	  if (th->prev_cont->next_cont == th)
	    th->prev_cont->next_cont = NULL;
	}
	      
    }
  else
    {
      if (th->next_cont)
	th->next_cont->prev_cont = th->prev_cont;
      if (th->prev_cont)
	th->prev_cont->next_cont = th->next_cont;
    }

  th->wear_loc = ITEM_WEAR_NONE;
  th->wear_num = 0;
  th->next_cont = NULL;  
  th->prev_cont = NULL;
  th->in = NULL;
  th->last_in = in_now;
  /* If someone is riding this thing, it is removed from the room. */

  if (th->fgt && th->fgt->rider)
    {
      if (th->fgt->rider->in == in_now)
	thing_from (th->fgt->rider);
      else 
	{
	  if (th->fgt->rider->fgt)
	    th->fgt->rider->fgt->mount = NULL;
	  th->fgt->rider = NULL;
	}
    }

  /* This deals with things that were in rooms (sitting there) and
     that were given timers (so they didn't stack up too badly,
     and are now being picked up (so that they get their timers cleared). */
  
  if (!IS_PC (th))
    {
      if (IS_ROOM (in_now) && 
	  th->timer > 0 && !CAN_FIGHT (th) && !CAN_MOVE (th) &&
	  th->proto && th->proto->timer == 0)
	th->timer = 0;
    }
  /* This is here to stop people from casting offensive spells
     then moving...if you want that, then remove this block. */
  
  else
    remove_typed_events (th, EVENT_COMMAND);

  /* Check for mules. */

  if (IS_PC (in_now))
    in_now->pc->login_item_xfer[in_now->pc->login_times % MAX_LOGIN] +=
      num_nostore_items (th);
  return;
}

/* This functions moves a thing into another thing and updates the
   various pointers. */

void
thing_to (THING *th, THING *to)
{
  THING *prev, *is_in;
  if (!th  || !to)
    return;
  if (th->in)
    thing_from (th); 
  
  if (th == to)
    { 
      if (th->vnum != 2)
	to = find_thing_num (2);
      else
	to = find_thing_num (3);
    }
  is_in = to;
  th->in = to;
  th->prev_cont = NULL;
  th->next_cont = NULL;
  while (is_in && !IS_AREA (is_in))
    {
      is_in->carry_weight = MAX (0, is_in->carry_weight + (th->weight + th->carry_weight));
      is_in->light = MAX (0, is_in->light + th->light);
      is_in = is_in->in;
    }			 
  
  
  
  if (!IS_AREA (to))
    {
      if (!to->cont) /* If this contains nothing, th becomes the head. */
	{
	  th->next_cont = NULL;
	  th->prev_cont = NULL;
	  to->cont = th;
	} /* If the head is either not worn, or its worn and it's
	     supposed to go "after" th, th gets put to the head.
	     Also, now the rooms put into the wilderness area just go 
	     to the head of the list because they will be inserted/
	     removed so often. */
      else if (!IS_WORN (to->cont) || 
#ifdef USE_WILDERNESS
	       to == wilderness_area || 
#endif
	       (IS_WORN (th) && 
		(to->cont->wear_loc > th->wear_loc ||
		 (to->cont->wear_loc == th->wear_loc && 
		  to->cont->wear_num > th->wear_num))))
	{
	  th->next_cont = to->cont;
	  to->cont->prev_cont = th;
	  th->prev_cont = NULL;
	  to->cont = th;
	}
      else
	{ /* Otherwise, to->cont is before the proper slot, so
	     we find the place...prev that will go BEFORE where
	     we want th to go in the list. */
	  bool found = FALSE;
	  for (prev = to->cont; prev; prev = prev->next_cont)
	    {
	      if (!prev->next_cont ||
		  !IS_WORN (prev->next_cont) ||
		  (IS_WORN (th) && 
		   (prev->next_cont->wear_loc > th->wear_loc ||
		    (prev->next_cont->wear_loc == th->wear_loc &&
		     prev->next_cont->wear_num > th->wear_num))))
		{
		  th->next_cont = prev->next_cont;
		  th->prev_cont = prev;
		  if (prev->next_cont)
		    prev->next_cont->prev_cont = th;
		  prev->next_cont = th;
		  found = TRUE;
		  break;
		}
	    }
	}
    }
  else
    { 
      /* Don't order the rooms in the wilderness area since so
	 many of them get created and destroyed so often. */
      if (!to->cont || to->cont->vnum > th->vnum 
#ifdef USE_WILDERNESS
	  || to == wilderness_area
#endif
	  )
	{	  
	  th->prev_cont = NULL;
	  th->next_cont = to->cont;
	  if (to->cont)
	    to->cont->prev_cont = th;
	  to->cont = th;	  
	}
      else
	{
	  for (prev = to->cont; prev; prev = prev->next_cont)
	    {
	      if (!prev->next_cont || prev->next_cont->vnum > th->vnum)
		{
		  th->prev_cont = prev;
		  th->next_cont = prev->next_cont;
		  if (prev->next_cont)
		    prev->next_cont->prev_cont = th;
		  prev->next_cont = th;
		  break;
		}
	    }
	}
    }
  
  /* Update pc->in_vnum and tracking */
  
  if (IS_PC (th))
    {
      if (IS_ROOM (to) && !IN_BG (th))
	th->pc->in_vnum = to->vnum;
      if (is_hunting (th))
	add_track_event (th);
#ifdef USE_WILDERNESS
      if (IS_WILDERNESS_ROOM (to))
	{
	  update_wilderness_room (to);
	  if (nr (1,20) == 16)
	    create_encounter (th);
	}
#endif
    }
  /* Now deal with mounts and stuff. */
  
  /* If you have a rider that isn't in anything, bring it to your
     location */
  
  if (RIDER(th) && !RIDER(th)->in)
    thing_to (RIDER(th), to);
  
  /* If you are riding something and it isn't in the same room
     as you are going to, then dismount. */

  if (MOUNT(th) && MOUNT(th)->in != to)
    {
      if (MOUNT(th)->fgt)
	MOUNT(th)->fgt->rider = NULL;
      if (th->fgt)
	th->fgt->mount = NULL;
    }
  
  update_kde (to, KDE_UPDATE_WEIGHT);
  
  /* If this is an object that can be taken, and if it is
     not a mob, and if it goes into a room, and if it is storable,
     then it gets a timer while sitting on the ground so that
     things don't take up so much room on the ground. */
     
  if (!CAN_FIGHT (th) && !CAN_MOVE (th) && 
      th->proto && th->proto->timer == 0 &&
      IS_ROOM (to) && !IS_SET(th->thing_flags, TH_NO_TAKE_BY_OTHER | TH_NO_MOVE_BY_OTHER) &&
      !IS_OBJ_SET (th, OBJ_NOSTORE))
    th->timer = 20; /* storable objects get N ticks on the ground */
  

  /* Check for mules. */

  if (IS_PC (to))
    to->pc->login_item_xfer[to->pc->login_times % MAX_LOGIN] +=
      num_nostore_items (th);
  
  return;
}



void 
do_get (THING *th, char *arg)
{
  THING *mover = NULL, *start_in, *movern, *guarder;
  char arg1[STD_LEN];
  char arg2[STD_LEN];
  char buf[STD_LEN];
  char name[STD_LEN];
  VALUE *mony, *nmoney, *guard;
  int coin_num = 0, coin_type = NUM_COINS, i;
  bool all = FALSE;
  bool coins = FALSE;
  bool once = FALSE;
  bool got_item = FALSE;
  char *t;
  arg1[0] = '\0';
  arg2[0] = '\0';
  name[0] = '\0';
  arg = f_word (arg, arg1);
  arg = f_word (arg, arg2);
  if (!str_cmp (arg2, "from"))
    arg = f_word (arg, arg2);
  
  if (!th->in)
    return;
  start_in = th->in;
  if (arg1[0] == '\0')
    {
      stt ("Syntax: get <something> or get <something> from <something>\n\r", th);
      return;
    }


  if (!str_cmp (arg1, "all"))
    all = TRUE;
  else if (!str_cmp (arg1, "coins") || !str_cmp (arg1, "money"))
    {
      coins = TRUE;
      all = TRUE;
    }
  else if (((coin_num = atoi (arg1)) > 0 &&
	    ((coin_type = get_coin_type (arg2)) != NUM_COINS)))
    {
      coins = TRUE;
      arg = f_word (arg, arg2);
      if (!str_cmp (arg2, "from"))
	arg = f_word (arg, arg2);
    }
  else if (!str_prefix ("all", arg1))
    {
      t = arg1;
      t += 3;
      if (*t == '.')
	{
	  t++;
	  f_word (t, name);
	  strcpy (arg1, name);
	  all = TRUE;
	}
    }
  
  if (arg2[0])
    {
      if ((start_in = find_thing_here (th, arg2, TRUE)) == NULL)
	{
	  stt ("You can't find that item!\n\r", th);
	  return;
	}
    }
  if (start_in == th)
    {
      stt ("Get from yourself? How cute.\n\r", th);
      return;
    }

  if (!all && !coins && 
      (mover = find_thing_thing (th, start_in, arg1, FALSE)) == NULL)
    {
      if (start_in != th->in)
	act ("There is no @t in @3n.", th, NULL, start_in, arg1, TO_CHAR);
      else
	act ("There is no @t here.", th, NULL, NULL, arg1, TO_CHAR);
      return;
    }	
  if (!IS_SET (start_in->thing_flags, TH_NO_MOVE_SELF | TH_IS_ROOM))
    {
      act ("@1n just tried to take something from @3n.\n\r", th, NULL, start_in, NULL, TO_ALL);
      return;
    }
  
  mony = FNV (start_in, VAL_MONEY);
  
  if (coins)
    {
      if (!mony)
	{
	  if (start_in == th->in)
	    stt ("I see no money here!\n\r", th);
	  else
	    stt ("That thing has no money!\n\r", th);
	  return;
	}
      nmoney = find_money (th);
      if (all)
	{
	  
	  for (i = 0; i < NUM_COINS; i++)
	    {
	      nmoney->val[i] += mony->val[i];
	    }
	  remove_value (start_in, mony);
	  if (start_in == th->in)
	    act ("@1n pick@s up some money.", th, NULL, NULL, NULL, TO_ALL);
	  else
	    act ("@1n get@s some money from @3n.", th, NULL, start_in, NULL, TO_ALL);
	  return;
	}
      if (mony->val[coin_type] < coin_num)
	{
	  stt ("There weren't enough coins!\n\r", th);
	}
      coin_num = MIN (coin_num, mony->val[coin_type]);
      nmoney->val[coin_type] += coin_num;
      mony->val[coin_type] -= coin_num;
      if (start_in == th->in)
	{
	  sprintf (buf, "@1n pick@s up %d %s coin%s.", coin_num, money_info[coin_type].app, (coin_num > 1 ? "s" : ""));
	  act (buf, th, NULL, NULL, NULL, TO_ALL);
	}
      else
	{
	  sprintf (buf, "@1n get@s %d %s coin%s from @3n.", coin_num, money_info[coin_type].app, (coin_num > 1 ? "s" : ""));
	  act (buf, th, NULL, start_in, NULL, TO_ALL);
	}
      if (total_money (start_in) == 0)
	remove_value (start_in, mony);
      return;
    }

  if (all)
    {
      if (mony != NULL && !name[0])
	{ 
	  nmoney = find_money (th);
	  for (i = 0; i < NUM_COINS; i++)
	    {
	      if (mony->val[i] > 0)
		got_item = TRUE;
	      nmoney->val[i] += mony->val[i];
	    }
	  remove_value (start_in, mony);
	  if (start_in == th->in)
	    act ("@1n pick@s up some money.", th, NULL, NULL, NULL, TO_ALL);
	  else
	    act ("@1n get@s some money from @3n.", th, NULL, start_in, NULL, TO_ALL);
	  
	}
    }
  if (all)
    mover = start_in->cont;
  
  for (; mover && th->in && !FIGHTING (th) && (!once || all); mover = movern)
    {
      movern = mover->next_cont;
      
      if (IS_PC (mover) || mover == th ||
	  (name[0] && !is_named (mover, name)))
	continue;
      if ((CAN_FIGHT (mover) || CAN_MOVE (mover)) &&
	  IS_SET (mover->thing_flags, TH_NO_TAKE_BY_OTHER))
	continue; 
      once = TRUE;
      if (start_in == th->in)
	{
	  for (guarder = start_in->cont; guarder; guarder = guarder->next_cont)
	    {
	      /* We only worry about nonpcs who can fight and who can
		 see the getter and who are guards. */
	      if (IS_PC (guarder) || !CAN_FIGHT (guarder) ||
		  !can_see (guarder, th) || IS_PC (mover) ||
		  (guard = FNV (guarder, VAL_GUARD)) == NULL ||
		  guarder->position <= POSITION_SLEEPING)
		continue;
	      for (i = 1; i < 4; i++)
		{
		  if (guard->val[i] == mover->vnum)
		    {
		      do_say (guarder, "Put that down RIGHT NOW!");
		      multi_hit (guarder, th);
		      return;
		    }
		}
	    }
	}
      
      got_item = TRUE;
      if (move_thing (th, mover, start_in, th))
	{
	  if (start_in == th->in)
	    act ("@1n pick@s up @2n.", th, mover, NULL, NULL, TO_ALL);
	  else
	    act ("@1n get@s @2n from @3n.", th, mover, start_in, NULL, TO_ALL);
	}
    }
  
  /* If you try to get all and nothing is here... */
  
  if (all && !got_item)
    {
      if (!*arg2 || !start_in)
	{
	  if (name[0])	
	    stt ("There nothing like that here to pick up.\n\r", th);
	  else
	    stt ("There's nothing here to pick up!\n\r", th);
	}
      else
	{
	  if (name[0])
	    act ("@3n doesn't have anything like that in it.", 
		 th, NULL, start_in, NULL, TO_CHAR);
	  else
	    act ("@3n doesn't have anything in it.", 
		 th, NULL, start_in, NULL, TO_CHAR);
	}
    }
  return;
}

void
do_drop (THING *th, char *arg)
{
  THING *mover = NULL, *movern;
  VALUE *tmoney, *rmoney;
  int coin_num = 0,coin_type = 0, i;
  char arg1[STD_LEN];
  char arg2[STD_LEN];
  char buf[STD_LEN];
  char name[STD_LEN];
  bool all = FALSE;
  bool coins = FALSE;
  int end_in_room_flags = 0;
  char *t;
  arg1[0] = '\0';
  arg2[0] = '\0';
  name[0] = '\0';
  if (!th || !th->in)
    return;
  
  arg = f_word (arg, arg1);
  arg = f_word (arg, arg2);
  if (arg1[0] == '\0')
    {
      stt ("Syntax: drop <something>\n\r", th);
      return;
    }
  /* Can't drop in bg. */
  if (IN_BG (th))
    {
      stt ("You can't drop things in the battleground.\n\r", th);
      return;
    }

  if (!str_cmp (arg1, "all"))
    all = TRUE;
  else if (!str_cmp (arg1, "coins") || !str_cmp (arg1, "money"))
    {
      coins = TRUE;
      all = TRUE;
    }
  else if (((coin_num = atoi (arg1)) > 0 &&
	    ((coin_type = get_coin_type (arg2)) != NUM_COINS)))
    {
      coins = TRUE;
    }
  else if (!str_prefix ("all", arg1))
    {
      t = arg1;
      t += 3;
      if (*t == '.')
	{
	  t++;
	  f_word (t, name);
	  strcpy (arg1, name);
	  all = TRUE;
	}
    }
  if (!all && !coins && (mover = find_thing_me (th, arg1)) == NULL)
    {
      stt ("Drop <something>\n\r", th);
      return;
    }
  
  tmoney = FNV (th, VAL_MONEY);
  
  if (coins)
    {
      if (tmoney == NULL)
	{
	  stt ("You don't have any coins to drop.\n\r", th);
	  return;
	}
      rmoney = find_money (th->in);
      if (all)
	{
	  for (i = 0; i < NUM_COINS; i++)
	    rmoney->val[i] += tmoney->val[i];
	  remove_value (th, tmoney);
	  act ("@1n drop@s some money.", th, NULL, NULL, NULL, TO_ALL);
	  return;
	}
      if (tmoney->val[coin_type] < coin_num)
	{
	  stt ("You don't have enough coins of that type!\n\r", th);
	  return;
	}
      rmoney->val[coin_type] += coin_num;
      tmoney->val[coin_type] -= coin_num;
      sprintf (buf, "@1n drop@s %d %s coin%s.", coin_num, money_info[coin_type].app, (coin_num > 1 ? "s" : ""));
      act (buf, th, NULL, NULL, NULL, TO_ALL);
      return;
    }
  
  if (IS_ROOM (th->in))
    end_in_room_flags = flagbits (th->in->flags, FLAG_ROOM1);
  
  if (all)
    {
      if (tmoney != NULL && !name[0] && IS_PC(th))
	{ 
	  rmoney = find_money (th->in);
	  for (i = 0; i < NUM_COINS; i++)
	    rmoney->val[i] += tmoney->val[i]; 
	  remove_value (th, tmoney);
	  act ("@1n drop@s some money.", th, NULL, NULL, NULL, TO_ALL);
	}
      for (mover = th->cont; mover; mover = movern)
	{
	  movern = mover->next_cont;
	  if (IS_PC (mover) || mover == th || 
	      (name[0] && !is_named (mover, name)))
	    continue;
	  if (move_thing (th, mover, th, th->in))
	    act ("@1n drop@s @3n.", th, NULL, mover, NULL, TO_ALL);
	  
	  if (IS_SET (end_in_room_flags, ROOM_WATERY))
	    {
	      if (!CAN_MOVE (mover) && 
		  (!IS_OBJ_SET (mover, OBJ_DRIVEABLE) ||
		   !IS_ROOM_SET (mover, ROOM_WATERY)))
		{
		  act ("@1n sink@s into the water.", mover, NULL, NULL, NULL, TO_ALL);
		  free_thing (mover);
		}
	      else
		act ("@1n begin@s to bob on the surface.", mover, NULL, NULL, NULL, TO_ALL);
	    }
	  else if (IS_SET (end_in_room_flags, ROOM_AIRY))
	    {
	      if (!CAN_MOVE (mover) && 
		  (!IS_OBJ_SET (mover, OBJ_DRIVEABLE) ||
		   !IS_ROOM_SET (mover, ROOM_AIRY)))
		{
		  act ("@1n drop@s out of sight!.", mover, NULL, NULL, NULL, TO_ALL);
		  free_thing (mover);
		}
	      else
		act ("@1n begin@s to float in the air.", mover, NULL, NULL, NULL, TO_ALL);
	    }
	}
      write_playerfile (th);
      return;
    }
  
  if (!move_thing (th, mover, th, th->in))
    return; 
  
  if (IS_SET (end_in_room_flags, ROOM_WATERY))
    {
      if (!CAN_MOVE (mover) && 
	  (!IS_OBJ_SET (mover, OBJ_DRIVEABLE) ||
	   !IS_ROOM_SET (mover, ROOM_WATERY)))
	{
	  act ("@1n sink@s into the water.", mover, NULL, NULL, NULL, TO_ALL);
	  free_thing (mover);
	}
      else
	act ("@1n begin@s to bob on the surface.", mover, NULL, NULL, NULL, TO_ALL);
    }
  else if (IS_SET (end_in_room_flags, ROOM_AIRY))
    {
      if (!CAN_MOVE (mover) && 
	  (!IS_OBJ_SET (mover, OBJ_DRIVEABLE) ||
	   !IS_ROOM_SET (mover, ROOM_AIRY)))
	{
	  act ("@1n drop@s out of sight!.", mover, NULL, NULL, NULL, TO_ALL);
	  free_thing (mover);
	}
      else
	act ("@1n begin@s to float in the air.", mover, NULL, NULL, NULL, TO_ALL);
    }
  act ("@1n drop@s @3n.", th, NULL, mover, NULL,  TO_ALL);



  write_playerfile (th);
  return;
}

/* This should technically be used for things which are animated,
   and not objects, but there are no distinctions atm. */

void
do_give (THING *th, char *arg)
{
  THING *mover = NULL, *movern, *end_in;
  VALUE *tmoney, *rmoney;
  int coin_num = 0, coin_type = 0, i;
  char arg1[STD_LEN];
  char arg2[STD_LEN];
  char buf[STD_LEN];
  char name[STD_LEN];
  char coinbuf[STD_LEN];
  char *t;
  int coin_value = 0;
  bool all = FALSE;
  bool coins = FALSE;
  arg1[0] = '\0';
  arg2[0] = '\0';
  name[0] = '\0';
  coinbuf[0] = '\0';
  
  if (!th->in)
    return;
  
  arg = f_word (arg, arg1);
  arg = f_word (arg, arg2);
  if (!str_cmp (arg2, "to"))
    arg = f_word (arg, arg2);

  if (arg1[0] == '\0')
    {
      stt ("Syntax: give <something> <something> \n\r", th);
      return;
    }
  /* Can't give in battleground. */
  if (IN_BG(th))
    {
      stt ("You can't give things in the battleground.\n\r", th);
      return;
    }

  if (!str_cmp (arg1, "all"))
    all = TRUE;
  else if (!str_cmp (arg1, "coins") || !str_cmp (arg1, "money"))
    {
      coins = TRUE;
      all = TRUE;
    }
  else if ((coin_num = atoi (arg1)) > 0 &&
	   (((coin_type = get_coin_type (arg2)) != NUM_COINS) ||
	    !str_cmp (arg2, "coins")))
    {
      coins = TRUE;
      if (!str_cmp (arg2, "coins"))
	coin_type = -1;
      arg = f_word (arg, arg2);
      if (!str_cmp (arg2, "to"))
	arg = f_word (arg, arg2);
    }
  
  else if (!str_prefix ("all", arg1))
    {
      t = arg1;
      t += 3;
      if (*t == '.')
	{
	  t++;
	  f_word (t, name);
	  strcpy (arg1, name);
	  all = TRUE;
	}
    }
  if (!all && !coins && (mover = find_thing_me (th, arg1)) == NULL)
    {
      stt ("Give <something> <something>\n\r", th);
      return;
    }
  /* Can only give to things that can move... bad distinction? */
  if ((end_in = find_thing_here (th, arg2, FALSE)) == NULL ||
      IS_SET (end_in->thing_flags, TH_NO_MOVE_SELF))
    {
      stt ("There is nothing here like that to give to.\n\r", th);
      return;
    }
  if (end_in == th)
    {
      stt ("Giving something to yourself?\n\r", th);
      return;
    }

  tmoney = FNV (th, VAL_MONEY);
  
  if (coins)
    {
      if (tmoney == NULL)
	{
	  stt ("You don't have any coins to give.\n\r", th);
	  return;
	}
      rmoney = find_money (end_in);
      if (all)
	{
	  coin_value = total_money (th);
	  sprintf (coinbuf, "%d", coin_value);
	  for (i = 0; i < NUM_COINS; i++)
	    rmoney->val[i] += tmoney->val[i];
	  remove_value (th, tmoney);
	  act ("@1n give@s @3n some money.", th, NULL, end_in, NULL, TO_ALL); 
	  check_trigtype (end_in, th, NULL, coinbuf, TRIG_BRIBE);
	}
      else if (coin_type == -1)
	{
	  sprintf (coinbuf, "%d", coin_num);
	  sub_money (th, coin_num);
	  add_money (end_in, coin_num); 
	  act ("@1n give@s @3n some money.", th, NULL, end_in, NULL, TO_ALL); 
	  check_trigtype (end_in, th, NULL, coinbuf, TRIG_BRIBE);
	  return;
	}
      else if (tmoney->val[coin_type] < coin_num)
	{
	  stt ("You don't have enough coins of that type!\n\r", th);
	  return;
	}
      sprintf (coinbuf, "%d", money_info[coin_type].flagval*coin_num);
      rmoney->val[coin_type] += coin_num;
      tmoney->val[coin_type] -= coin_num;
      sprintf (buf, "@1n give@s %d %s coin%s to @3n.", coin_num, money_info[coin_type].app, (coin_num > 1 ? "s" : ""));
      act (buf, th, NULL, end_in, NULL, TO_ALL); 
      check_trigtype (end_in, th, NULL, coinbuf, TRIG_BRIBE);
      return;
    }
  
  
  if (all)
    {
      if (tmoney != NULL && !name[0])
	{ 
	  rmoney = find_money (end_in);
	  sprintf (coinbuf, "%d", total_money(th));
	  for (i = 0; i < NUM_COINS; i++)
	    rmoney->val[i] += tmoney->val[i]; 
	  remove_value (th, tmoney);
	  act ("@1n give@s @3n some money.", th, NULL, end_in, NULL, TO_ALL);
	  check_trigtype (end_in, th, NULL, coinbuf, TRIG_BRIBE);
	}
      for (mover = th->cont; mover; mover = movern)
	{
	  movern = mover->next_cont;
	  if (IS_PC (mover) || mover == th ||
	      (name[0] && !is_named (mover, name)))
	    continue;
	  if (move_thing (th, mover, th, end_in ))
	    {
	      act ("@1n give@s @2n to @3n.", th, mover, end_in, NULL, TO_ALL);
	      society_item_move (th, mover, end_in);
	    }
	}
      write_playerfile (th);
      return;
    }
  
  if (!move_thing (th, mover, th, end_in))
    return;
  act ("@1n give@s @2n to @3n.", th, mover, end_in, NULL, TO_ALL);
  society_item_move (th, mover, end_in);  
  write_playerfile (th);
  return;
}


  
/* This function lets you take an item and put it into or onto or
   below something. I think it will in general be used for things
   that are not "animated" ---but no such distinctions now. */


void 
do_put (THING *th, char *arg)
{
  THING *mover = NULL, *movern, *end_in;
  VALUE *tmoney, *rmoney;
  int coin_num = 0, coin_type = 0, i, coin_value = 0;
  char arg1[STD_LEN];
  char arg2[STD_LEN];
  char buf[STD_LEN];
  char name[STD_LEN];
  char coinbuf[STD_LEN];
  char *t;
  bool all = FALSE;
  bool coins = FALSE;
  arg1[0] = '\0';
  arg2[0] = '\0';
  name[0] = '\0';
  coinbuf[0] = '\0';
  if (!th->in)
    return;
  
  arg = f_word (arg, arg1);
  arg = f_word (arg, arg2);
  if (!str_cmp (arg2, "into") || !str_cmp (arg2, "in"))
    arg = f_word (arg, arg2);

  if (arg1[0] == '\0')
    {
      stt ("Syntax: put <something> <something> \n\r", th);
      return;
    }
  if (!str_cmp (arg1, "all"))
    all = TRUE;
  else if (!str_cmp (arg1, "coins") || !str_cmp (arg1, "money"))
    {
      coins = TRUE;
      all = TRUE;
    }
  else if ((coin_num = atoi (arg1)) > 0 &&
	   (((coin_type = get_coin_type (arg2)) != NUM_COINS) ||
	    !str_cmp (arg2, "coins")))
    {
      coins = TRUE;
      if (!str_cmp (arg2, "coins"))
	coin_type = -1;
      arg = f_word (arg, arg2);
      if (!str_cmp (arg2, "to"))
	arg = f_word (arg, arg2);      
    }
  
  else if (!str_prefix ("all", arg1))
    {
      t = arg1;
      t += 3;
      if (*t == '.')
	{
	  t++;
	  f_word (t, name);
	  strcpy (arg1, name);
	  all = TRUE;
	}
    }
  if (!all && !coins && (mover = find_thing_me (th, arg1)) == NULL)
    {
      stt ("put <something> <something>\n\r", th);
      return;
    }
  /* Can only give to things that can move... bad distinction? */
  if ((end_in = find_thing_here (th, arg2, FALSE)) == NULL ||
      !IS_SET (end_in->thing_flags, TH_NO_MOVE_SELF))
    {
      stt ("There is nothing here like that you can put things into.\n\r", th);
      return;
    }
  if (end_in == th)
    {
      stt ("Putting something into yourself?\n\r", th);
      return;
    }
  tmoney = FNV (th, VAL_MONEY);
  if (coins)
    {
      if (tmoney == NULL)
	{
	  stt ("You don't have any coins to put anywhere.\n\r", th);
	  return;
	}
      rmoney = find_money (end_in);
      if (all)
	{
	  coin_value = total_money (th);
	  sprintf (coinbuf, "%d", coin_value);
	  for (i = 0; i < NUM_COINS; i++)
	    rmoney->val[i] += tmoney->val[i];
	  remove_value (th, tmoney);
	  act ("@1n put@s some money in @3n.", th, NULL, end_in, NULL, TO_ALL); 
	  check_trigtype (end_in, th, NULL, coinbuf, TRIG_BRIBE);
	}
      else if (coin_type == -1)
	{
	  sprintf (coinbuf, "%d", coin_num);
	  sub_money (th, coin_num);
	  add_money (end_in, coin_num); 
	  act ("@1n put@s some money in @3n.", th, NULL, end_in, NULL, TO_ALL); 
	  check_trigtype (end_in, th, NULL, coinbuf, TRIG_BRIBE);
	  return;
	}
      else if (tmoney->val[coin_type] < coin_num)
	{
	  stt ("You don't have enough coins of that type!\n\r", th);
	  return;
	}
      sprintf (coinbuf, "%d", money_info[coin_type].flagval*coin_num);
      rmoney->val[coin_type] += coin_num;
      tmoney->val[coin_type] -= coin_num;
      sprintf (buf, "@1n put@s %d %s coin%s into @3n.", coin_num, money_info[coin_type].app, (coin_num > 1 ? "s" : ""));
      act (buf, th, NULL, end_in, NULL, TO_ALL); 
      check_trigtype (end_in, th, NULL, coinbuf, TRIG_BRIBE);
      return;
    }
  
  
  if (all)
    {
      if (tmoney != NULL && !name[0])
	{ 
	  rmoney = find_money (end_in);
	  sprintf (coinbuf, "%d", total_money(th));
	  for (i = 0; i < NUM_COINS; i++)
	    rmoney->val[i] += tmoney->val[i]; 
	  remove_value (th, tmoney);
	  act ("@1n put@s some money into @3n.", th, NULL, end_in, NULL, TO_ALL);
	  check_trigtype (end_in, th, NULL, coinbuf, TRIG_BRIBE);
	}
      for (mover = th->cont; mover; mover = movern)
	{
	  movern = mover->next_cont;
	  if (IS_PC (mover) || mover == th ||
	      (name[0] && !is_named (mover, name)))
	    continue;
	  if (move_thing (th, mover, th, end_in ))
	    {
	      act ("@1n put@s @2n into @3n.", th, mover, end_in, NULL, TO_ALL);
	      society_item_move (th, mover, end_in);
	    }
	}
      write_playerfile (th);
      return;
    }
  
  if (!move_thing (th, mover, th, end_in))
    return;
  act ("@1n put@s @2n into @3n.", th, mover, end_in, NULL, TO_ALL); 
  society_item_move (th, mover, end_in);
  write_playerfile (th);
  return;
}


void
do_leave (THING *th, char *arg)
{
  char buf[STD_LEN];
  THING *start_in;
  if (!th || !th->in || !th->in->in)
    return;
  start_in = th->in;
  if (IS_AREA (th->in->in))
    {
      stt ("You can't seem to leave.\n\r", th);
      return;
    }
  
  if (!move_thing (th, th, th->in, th->in->in))
    return;
  
  
  stt ("You leave.\n\r", th);
  sprintf (buf, "%s leaves you.\n\r", name_seen_by (th->in, th));
  stt (buf, th->in);
  act ("@1n arrives from @2n.", th, start_in, NULL, NULL, TO_ROOM);
  if (IS_PC (th))    
    do_look (th, "");
  place_track (th, th->in, DIR_MAX, DIR_MAX, TRUE);
  
 create_map (th, th->in, SMAP_MAXX, SMAP_MAXY);

  return;
}

/* This command lets a thing attempt to go into a thing which is in
   the same thing as the initiator. If the thing being entered has no in_exit,
   then you attempt to enter the thing. If it does have one, then it acts
   like a portal, sending you to another place. */


void
do_enter (THING *th, char *arg)
{
  THING *entering = NULL, *to_place = NULL, *start_in = NULL;
  char buf[STD_LEN];
  VALUE *inexit;
  if ((start_in = th->in) == NULL)
    return;
  if ((entering = find_thing_in (th, arg)) == NULL)
    {
      stt ("What did you want to enter exactly?\n\r", th);
      return;
    }
  if ((inexit = FNV (entering, VAL_EXIT_I)) != NULL)
    to_place = find_thing_num (inexit->val[0]);
  else
    to_place = entering;
  
  if (!to_place)
    {
      stt ("That doesn't lead anywhere!\n\r", th);
      return;
    }
  if (!move_thing (th, th, start_in, to_place))
    return;  
  if (start_in->cont)
    act ("@2n enters @3n.", start_in->cont, th, entering, NULL, TO_ALL);
  sprintf (buf, "You enter %s.\n\r", name_seen_by (th, entering));
  stt (buf, th);
  act ("@1n enters from @2n.", th, entering, NULL, NULL, TO_ROOM);
  if (IS_PC(th))
    do_look (th, "");

  /* Now place the ending track...*/
  
  place_track (th, to_place, DIR_MAX, DIR_MAX, TRUE);
 create_map (th, th->in, SMAP_MAXX, SMAP_MAXY);

  return;
}


/* This lets you move many rooms in the same direction all at once. */

void
do_run (THING *th, char *arg)
{
  int dir, count = 0;
  bool done_once = FALSE;
  if ((dir = find_dir (arg)) == DIR_MAX)
    {
      stt ("Run <dir>.\n\r", th);
      return;
    }

  if (th->position != POSITION_STANDING)
    {
      stt ("You cannot start running unless you are standing!\n\r", th);
      return;
    }
  if (!done_once)
  while (move_dir (th, dir, MOVE_RUN) && count++ < 100)
    done_once = TRUE;
  if (!done_once)
    stt ("You can't run that way!\n\r", th);
  return;
}


void
do_north (THING *th, char *arg)
{
  move_dir (th, DIR_NORTH, 0);
  return;
}

void
do_south (THING *th, char *arg)
{
  move_dir (th, DIR_SOUTH, 0);
  return;
}

void
do_east (THING *th, char *arg)
{
  move_dir (th, DIR_EAST, 0);
  return;
}

void
do_west (THING *th, char *arg)
{
  move_dir (th, DIR_WEST, 0);
  return;
}

void
do_up (THING *th, char *arg)
{
  move_dir (th, DIR_UP, 0);
  return;
}
void
do_down (THING *th, char *arg)
{
  move_dir (th, DIR_DOWN, 0);
  return;
}


bool
is_blocking_dir (THING *blocker, THING *mover, int dir)
{
  VALUE *guard;
  char *t;
  bool dir_guarded = FALSE;
  if (!blocker || !mover || mover == blocker)
    return FALSE;
  

  /* Check if it's an object or a mob that can see the mover. */
  
  if (!IS_SET (blocker->thing_flags, TH_NO_MOVE_SELF) &&
      (!can_see (blocker, mover) || 
       blocker->position == POSITION_SLEEPING))
    return FALSE;
  
  /* Find out if it guards. */

  if ((guard = FNV (blocker, VAL_GUARD)) == NULL)
    return FALSE;
  
  for (t = guard->word; *t; t++)
    {
      if (LC (*t) == LC (*dir_name[dir]))
	{
	  dir_guarded = TRUE;
	  break;
	}
    }
  if (!dir_guarded)
    return FALSE;
  
 
  
  /* Check for simple kinds of guarding like basic guard, 
     guard vs pc/npc, guard vs align. */

  if (guard->val[0] && 
      (IS_SET (guard->val[0], GUARD_DOOR) ||
       (IS_SET (guard->val[0], GUARD_DOOR_PC) && IS_PC (mover)) ||
       (IS_SET (guard->val[0], GUARD_DOOR_NPC) && !IS_PC (mover)) ||
       (IS_SET (guard->val[0], GUARD_DOOR_ALIGN) && 
	(DIFF_ALIGN (blocker->align, mover->align) ||
	 is_enemy (blocker, mover)))))
    {
      if (IS_SET (blocker->thing_flags, TH_NO_MOVE_SELF))
	act ("A strange force keeps you back...", mover, NULL, NULL, NULL, TO_CHAR);
      else
	act ("@1n won't let you by!", blocker, NULL, mover, NULL, TO_VICT);
      
      if (IS_SET (guard->val[0], GUARD_DOOR_ALIGN) && 
	  (DIFF_ALIGN (blocker->align, mover->align) ||
	   is_enemy (blocker, mover)))
	{
	  char buf[STD_LEN];
	  sprintf (buf, "%ss may not pass!", FALIGN(mover)->name);
	  do_say (blocker, buf);
	}
      


      if (IS_SET (guard->val[0], GUARD_DOOR_ALIGN) && 
	  (is_enemy (blocker, mover) ||
	   DIFF_ALIGN (blocker->align, mover->align)) &&
	  !FIGHTING (blocker) &&
	  !IS_ACT1_SET (mover, ACT_PRISONER))
	multi_hit (blocker, mover);
      return TRUE;
    }
  /* Check clan/sect/temple guard... v5 must be > 0 for this to work. */
  
  if (guard->val[5] > 0 && guard->val[4] >= 0 &&  guard->val[4] < CLAN_MAX &&
      mover->pc && mover->pc->clan_num[guard->val[4]] != guard->val[5])
    {
      act ("@3n must be a member of our @t to pass.", blocker, NULL, mover, clantypes[guard->val[4]], TO_ALL);
      return TRUE;
    }
  /* Now special checks... on hps levels remorts etc. This is easy to
     add stuff to if you want to. Don't use numbers < 10 for this.
     Those are essentially reserved for clans and stuff. Note the checking
     for stats and guilds at the end. */
  
  
  if (guard->val[4] >= 10 && guard->val[5] > 0 && IS_PC (mover))
    {
      int type = guard->val[4], val = guard->val[5];
      bool stopped = FALSE;
      /* If this is not set, we stop things which have values BELOW
	 the minimum we are looking for. Otherwise, we stop things
	 which are >= the value we put down. */
      bool below = !IS_SET (guard->val[0], GUARD_VS_ABOVE);
      
      if (type == 10 && ((mover->level < val) == below))
	stopped = TRUE;
      else if (type == 11 && ((mover->pc->remorts < val) == below))
	stopped = TRUE;
      else if (type == 12 && ((mover->pc->mana < val) == below))
	stopped = TRUE;
      else if (type == 13 && ((mover->pc->max_mana < val) == below))
	stopped = TRUE;
      else if (type == 14 && ((mover->hp < val) == below))
	stopped = TRUE;
      else if (type == 15 && ((mover->max_hp < val) == below))
	stopped = TRUE;
      else if (type == 16 && ((mover->pc->bank < val) == below))
	stopped = TRUE;
      else if (type >= 40 && type < 40 + STAT_MAX &&
	       (mover->pc->stat[type - 40] < val) == below)
	stopped = TRUE;
      else if (type  >= 60 && type < 60 + GUILD_MAX &&
	       (mover->pc->guild[type - 60] < val) == below)
	stopped = TRUE;
      
      if (stopped)
	{
	  stt ("A strange force keeps you back...\n\r", mover);
	  return TRUE;
	}
    }

  return FALSE;
}


/* This checks for fleeing and then checks for an exit and then
   checks if that exit is open or not. Then it checks if anything
   in the room is keeping you there or not. */


bool
can_move_dir (THING *th, int dir)
{
  THING *room, *thg;
  VALUE *exit = NULL, *socval;
  int pcbits;
  
  if (!th || dir < 0 || dir >= REALDIR_MAX || (room = th->in) == NULL)
    return FALSE;

  pcbits = flagbits (th->flags, FLAG_PC1);
  
  if ((exit = FNV (room, dir + 1)) == NULL ||
      IS_SET (exit->val[2], EX_WALL) ||
      (!IS_SET (~(exit->val[1]), (EX_HIDDEN | EX_CLOSED | EX_DOOR)) &&
       !IS_SET (pcbits, PC_HOLYWALK)))
    {
      stt ("You can't go in that direction!\n\r", th);
      return FALSE;
    }
  
  if (IS_SET (pcbits, PC_HOLYWALK))
    return TRUE;
  
  if (IS_SET (exit->val[1], EX_DOOR) && IS_SET (exit->val[1], EX_CLOSED))
    {
      if (IS_SET (exit->val[1], EX_HIDDEN) && !IS_SET (pcbits, PC_HOLYLIGHT))
	{
	  stt ("You can't go in that direction!\n\r", th);
	  return FALSE;
	}
      act ("@1n bump@s into the @t!", th, NULL, NULL, 
	   (exit->word && *exit->word ? exit->word : (char *) "door"), TO_ALL);
      return FALSE;
    }
  
  /* Now check for blocking things! Blocking things do not stop mobs
     moving to new society locations!!!! Nor do they stop builders/
     workers from collecting resources! */
  
  if ((socval = FNV (th, VAL_SOCIETY)) != NULL &&
      (IS_SET (socval->val[2], CASTE_WORKER | CASTE_BUILDER) ||
       (th->fgt && th->fgt->hunting_type == HUNT_SETTLE)))
    return TRUE;
  
  for (thg = th->in->cont; thg; thg = thg->next_cont)
    {
      if (is_blocking_dir (thg, th, dir))
	return FALSE;
    }
  
  return TRUE;
}


bool 
move_dir (THING *th, int dir, int flags)
{
  VALUE *val = NULL;
  THING *to, *start_in, *foll, *folln, *scanner, *room, *croom, *viewer;
  char buf[200];
  int room_bits = 0, moves = 1, to_room_bits = 0, mover_aff_bits = 0, i, range, th_bits = 0;
  bool has_boots = FALSE, has_vehicle = FALSE;
  VALUE *exit, *soc;
  if (!th || !th->in)
    {
      stt ("You can't go in that direction.\n\r", th);      
      return FALSE;
    }
  
  if ((!IS_PC (th) || !IS_PC1_SET (th, PC_HOLYWALK) ||
      LEVEL (th) < BLD_LEVEL) && IS_HURT (th, AFF_PARALYZE))
    {
      stt ("You can't move!\n\r", th);
      return FALSE;
    }
  
  if (!IS_ROOM (th->in) && th->in->in &&
      IS_ROOM (th->in->in) && IS_OBJ_SET (th->in, OBJ_DRIVEABLE))
    {
      if (move_dir (th->in, dir, flags))
	{
	  do_look (th, "out");
	  return TRUE;
	}
      else
	{
	  act ("Your @2n won't go that way!", th, th->in, NULL, NULL, TO_CHAR);
	  return FALSE;
	}
    }
  
  if (!IS_ROOM (th->in))
    {
      stt ("You can't go in that direction.\n\r", th);      
      return FALSE;
    }
  
  
  if (th->position < POSITION_FIGHTING || th->position == POSITION_MEDITATING)
    {
      sprintf (buf, "You cannot move while you are %s.\n\r", position_names[th->position]);
      stt (buf, th);
      return FALSE;
    }
  
  if (MOUNT(th))
    return move_dir (MOUNT (th), dir, flags);
  
  
  if (!IS_SET (flags, MOVE_FLEE) && FIGHTING(th))
    {
      stt ("You cannot walk away while fighting!\n\r", th);
      return FALSE;
    }
  
  if (!can_move_dir (th, dir))
    return FALSE;
  start_in = th->in;
  
  if (dir != DIR_OUT && (val = FNV (th->in, dir + 1)) == NULL)
    {
      stt ("There is no exit in that direction. \n\r", th);
      return FALSE;
    }
  
  if ((to = find_thing_num (val->val[0])) == NULL ||
      !IS_ROOM (to))
    {
      stt ("That exit leads nowhere.\n\r", th);
      return FALSE;
    }
  if (start_in == to)
    {
      stt ("You end up in the same spot.\n\r", th);
      return FALSE;
    }
  th_bits = flagbits (th->flags, FLAG_ROOM1);
  room_bits = flagbits (start_in->flags, FLAG_ROOM1);
  to_room_bits = flagbits (to->flags, FLAG_ROOM1);

  /* For vehicles, deal only with the important room flags.
     If the vehicle has a certain room flag set, then make it so
     the start or the end room must have one of that flag, too.
     If the to_room has any bits, make sure the vehicle has
     all of the important ones. */
  
  mover_aff_bits = flagbits (th->flags, FLAG_AFF);

  if (IS_OBJ_SET (th, OBJ_DRIVEABLE))
    {
      th_bits &= BADROOM_BITS;
      to_room_bits &= BADROOM_BITS;
      room_bits &= BADROOM_BITS;
      
      /* The vehicle cannot go there... not powerful enough */
      if (to_room_bits & ~th_bits)
	return FALSE;
      
      /* The vehicle is set for some special terrain, and it isn't there. */
      if (!(th_bits & (to_room_bits | room_bits)))
	return FALSE;
    }
  else if (to_room_bits && !IS_PC1_SET (th, PC_HOLYWALK))
    {
      THING *boots;
      VALUE *tool;
      for (boots = th->cont; boots; boots = boots->next_cont)
	{
	  if (boots->wear_loc == ITEM_WEAR_NONE)
	    break;
	  if ((tool = FNV (boots, VAL_TOOL)) != NULL)
	    {
	      if (tool->val[2] == 1)
		has_vehicle = TRUE;
	    }
	  if (boots->wear_loc == ITEM_WEAR_FEET)
	    has_boots = TRUE;
	}
      
      /* Must fly through air zones */

      if (IS_SET (to_room_bits, ROOM_AIRY))
	{
	  if (!IS_SET (mover_aff_bits, AFF_FLYING))
	    {
	      stt ("You need to be flying to go there!\n\r", th);
	      th->mv -= MIN (th->mv, 1);
	      return FALSE;
	    }
	  moves += 2;
	}
      
      /* Must have boots to move through mountain zones. Mobs can
	 climb up mountains using haste. */
      
      if (IS_SET (to_room_bits, ROOM_MOUNTAIN) && 
	  !has_vehicle &&
	  (IS_PC (th) || !IS_SET (mover_aff_bits, AFF_FLYING | AFF_HASTE)))	
	{
	  stt ("It's too steep for you to climb unaided.\n\r", th);
	  return FALSE;
	}

      /* Need passdoor to go through solid earth. */
      
      if (IS_SET (to_room_bits, ROOM_EARTHY))
	{
	  if (!IS_SET (mover_aff_bits, AFF_FOGGY))
	    {
	      stt ("You must be in mist form to pass through there!\n\r", th);
	      th->mv -= MIN (th->mv, 5);
	      return FALSE;
	    }
	  moves += 5;
	}
      
      /* Need swim or UW breath to go through water */
      
      if (IS_SET (to_room_bits, ROOM_WATERY | ROOM_UNDERWATER))
	{
	  /* Cannot move if you don't have water breath and (it's
	     underground or (you suck at swimming and arent flying)). */
	  
	  if (!IS_SET (mover_aff_bits, AFF_WATER_BREATH) &&
	      (IS_SET (to_room_bits, ROOM_UNDERWATER) ||
	       (!IS_SET (mover_aff_bits, AFF_FLYING) &&
		!check_spell (th, NULL, 303 /* swim */))))
	    {
	      stt ("You can't seem to swim that way!\n\r", th);
	      th->mv -= MIN (th->mv, 4);
	      return FALSE;
	    }
	  moves += 2;
	}
      if (IS_SET (to_room_bits, ROOM_UNDERGROUND | ROOM_WATERY | ROOM_FOREST |
		  ROOM_DESERT | ROOM_UNDERWATER | ROOM_SWAMP))
	moves ++;
      
      if (IS_SET (to_room_bits, ROOM_ROUGH))
	moves += 3;
      if (IS_SET (to_room_bits, ROOM_EASYMOVE))	
	moves /= 2;
      if (IS_SET (to_room_bits, ROOM_MOUNTAIN))
	moves += 4;
      else if (IS_SET (to_room_bits, ROOM_MOUNTAIN | ROOM_FOREST | ROOM_ROUGH | ROOM_FIERY | ROOM_EARTHY | ROOM_DESERT | ROOM_SWAMP ) && !has_boots && !IS_SET (mover_aff_bits, AFF_FLYING))
	{
	  stt ("Ouch! You need to find some shoes!\n\r", th);
	  moves += 3;
	}  
    }
  
  /* Fly reduces moves unless it doesn't make sense there. */
  
  if (!IS_SET (to_room_bits, ROOM_WATERY | ROOM_UNDERWATER | ROOM_UNDERGROUND | ROOM_INSIDE | ROOM_FIERY | ROOM_EARTHY) && IS_SET (mover_aff_bits, AFF_FLYING))
    moves -= moves/2;
  if (th->mv < moves)
    {
      stt ("You are too exhausted to move that way!\n\r", th);
      return FALSE;
    }

  if (!move_thing (th, th, th->in, to))
    return FALSE;
  
  th->mv -= moves;
  update_kde (th, KDE_UPDATE_HMV);
  if (RIDER (th) && (IS_PC (RIDER (th)) || RIDER(th)->fd) && MOUNT(RIDER(th)) == th)
    {
      sprintf (buf, "You go %s riding %s.", dir_name[dir], NAME(MOUNT(RIDER(th))));
      stt (buf, RIDER (th));
      
      if (IS_SET (flags, MOVE_RUN))
	show_thing_to_thing (RIDER(th), RIDER(th)->in, LOOK_SHOW_SHORT | LOOK_SHOW_EXITS | LOOK_SHOW_CONTENTS);
      else
	do_look (RIDER (th), "zzduhql");
    }
  if (start_in->cont)
    {
      if (RIDER (th) && RIDER(th)->in == to)
	sprintf (buf, "@1n goes %s riding @2n.", dir_name[dir]);
      else
	sprintf (buf, "@1n goes %s.", dir_name[dir]);
      for (viewer = start_in->cont; viewer; viewer = viewer->next_cont)
	{
	  if (!IS_SET (mover_aff_bits, AFF_SNEAK) || nr (1,15) == 4 ||
	      IS_DET (viewer, AFF_SNEAK))
	    act (buf, th, RIDER(th), viewer, NULL, TO_VICT);
	}
    }
  
  
  if (RIDER (th))
    sprintf (buf, "You go %s carrying %s.", dir_name[dir], NAME (RIDER(th)));
  else    
    sprintf (buf, "You go %s.\n\r", dir_name[dir]);
  stt (buf, th);
  
  /* Make blood and tracks */
  
  if (IS_ROOM (start_in) && th->max_hp > 0 && th->hp < th->max_hp/2 &&
      !IS_SET (room_bits, ROOM_NOBLOOD))
    SBIT (start_in->hp, (1 << dir) | NEW_BLOOD);
  
  
  /* THIS NEXT LINE IS PUT IN HERE SO THAT THINGS THAT ARE NOT PCS
     AND NOT HURT DONT MAKE TRACKS TO SAVE SOME CPU IN THE TRACKING
     CODE. ALSO HIGHLEVEL MOBS NOW LEAVE TRACKS TOO SO YOU CAN FIND THEM IF
     YOU NEED TO! */
  
  if (IS_PC (th) || th->hp < th->max_hp || 
      ((soc = FNV (th, VAL_SOCIETY)) != NULL &&
       soc->val[2] == CASTE_BUILDER))
    { /* Place tracks... */
      if (!IS_SET (room_bits, ROOM_NOTRACKS))
	place_track (th, start_in, DIR_MAX, dir, TRUE);
      if (!IS_SET (to_room_bits, ROOM_NOTRACKS))
	place_track (th, to, RDIR (dir), DIR_MAX, TRUE); 
    }
  /* End room message. */
  

  if (th->in)
    {
      if (RIDER(th) && RIDER(th)->in == th->in)
	sprintf (buf, "@1n arrives from %s carrying @2n.",
		 dir_track_in[RDIR(dir)]);
      else
	sprintf (buf, "@1n arrives from %s.", dir_track_in[RDIR(dir)]);
      
      for (viewer = th->in->cont; viewer; viewer = viewer->next_cont)
	{
	  if (viewer != th && viewer != RIDER(th) &&
	      (!IS_SET (mover_aff_bits, AFF_SNEAK) || nr (1,15) == 3 ||
	       IS_DET (viewer, AFF_SNEAK)) &&
	      viewer->position > POSITION_SLEEPING)
	    act (buf, th, RIDER(th), viewer, NULL, TO_VICT);
	}
    }
  
  /* Look around... */
  
  if (IS_PC (th) || th->fd)
    {
      if (IS_SET (flags, MOVE_RUN))
	show_thing_to_thing (th, th->in, LOOK_SHOW_SHORT | LOOK_SHOW_EXITS | LOOK_SHOW_CONTENTS);
      else
	do_look (th, "zzduhql");
    }
  
  /* Make followers move also. */
  
  for (foll = start_in->cont; foll != NULL; foll = folln)
    {
      folln = foll->next_cont;
      if (CAN_MOVE (foll) && 
	  (FOLLOWING (foll) == th || 
	   (RIDER(th) && FOLLOWING(foll) == RIDER(th))))
	move_dir (foll, dir, flags);
    }
  
  
  
  if (IS_PC2_SET (th, PC2_MAPPING))
    {
      if (!USING_KDE (th))
	create_map (th, th->in, SMAP_MAXX, SMAP_MAXY);
      else
	update_kde (th, KDE_UPDATE_MAP);
    }
  if (nr(1,8) == 2 && IS_PC (th) && th->pc->prac[300 /* Search */] > 50)
    {
      SBIT (th->pc->pcflags, PCF_SEARCHING);
    }
  
  
  /* Now do autoscan...for opp align people. */
  if (IS_PC (th) && LEVEL (th) < BLD_LEVEL &&
      (!FOLLOWING (th) || FOLLOWING(th)->in != th->in))
    {	  
      for (i = 0; i < REALDIR_MAX; i++)
	{
	  croom = th->in;
	  sprintf (buf, "@1n arrives @t %s.",dir_rev[i]);
	  for (range = 0; range < SCAN_RANGE; range++)
	    {
	      sprintf (buf, "@1n arrives @t %s.",dir_track[RDIR(i)]);
	      if ((exit = FNV (croom, i + 1)) != NULL &&
		  !IS_SET (exit->val[1], EX_WALL) &&
		  (!IS_SET (exit->val[1], EX_DOOR) ||
		   !IS_SET (exit->val[1], EX_CLOSED)) &&
		  (room = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM (room) && room != th->in)
		{
		  for (scanner = room->cont; scanner != NULL; scanner = scanner->next_cont)
		    {
		      if (IS_PC (scanner) && 
			  DIFF_ALIGN (scanner->align, th->align) &&
			  can_see (scanner, th) &&
			  IS_PC2_SET (scanner, PC2_AUTOSCAN) &&
			  check_spell (scanner, NULL, 324 /* 6th sense */))
			/* Check for skill??? */
			{
			  act (buf, th, NULL, scanner, dir_dist[range], TO_VICT);
			}
		    }
		  croom = room;
		}
	      else
		break;
	    }
	}
    }	      
  return TRUE;  

}

void 
do_transfer (THING *th, char *arg)
{
  THING *vict;
  
  if (!th->in)
    {
      stt ("You aren't in anything!\n\r", th);
      return;
    }

  /* Transfer all at once! */

  if (!str_cmp (arg, "all the players right now"))
    {
      for (vict = thing_hash[PLAYER_VNUM % HASH_SIZE]; vict; vict = vict->next)
	{
	  if (!IS_PC (vict) || vict == th || vict->in == th->in)
	    continue;
	  act ("@1n disappear@s in a *poof* of smoke!", vict, NULL, NULL, NULL, TO_ALL);
	  thing_to (vict, th->in);
	  act ("@1n arrive@s in a *poof* of smoke!", vict, NULL, NULL, NULL, TO_ALL);
	}
      return;
    }
  if ((vict = find_thing_world (th, arg)) == NULL ||
      vict == th || !vict->in ||
      (!CAN_MOVE (vict) && !CAN_FIGHT (vict)))
    {
      stt ("You couldn't find that thing to transfer!\n\r", th);
      return;
    }
  act ("@1n leave@s in a *poof* of smoke!", vict, NULL, NULL, NULL, TO_ALL);
  thing_to (vict, th->in);
  act ("@1n arrive@s in a *poof* of smoke!", vict, NULL, NULL, NULL, TO_ALL);
  return;
}

/* This lets an imm go to a certain named thing or a mob or a room.
   It now supports going to a wilderness room if you give two coordinates. */




void
do_goto (THING *th, char *arg)
{
#ifdef USE_WILDERNESS
  int x, y; /* Used for going to a wilderness coordinate. */
  char arg1[STD_LEN];
  char *oldarg = arg;
#endif 
  THING *dest = NULL, *thg, *vict, *victn, *in_now;

  if (!th || !th->in)
    return;
  in_now = th->in;
  if (!*arg)
    {
#ifdef USE_WILDERNESS
      stt ("goto <number/name> or <x> <y> in wilderness.\n\r", th);
#else
      stt ("goto <number/name>.\n\r", th);
#endif
      return;
    }

#ifdef USE_WILDERNESS
  arg = f_word (arg, arg1);
  if (*arg) /* 2 arguments given...both coords. */
    {
      x = atoi (arg1);
      y = atoi (arg);
      if (x >= 0 && x < WILDERNESS_SIZE && y >= 0 && y < WILDERNESS_SIZE)
	{
	  /* If this is a valid coordinate, go to it. */
	  sprintf (arg1, "%d", (WILDERNESS_MIN_VNUM + x + y*WILDERNESS_SIZE));
	  arg = arg1;
	}
      else
	{
	  stt ("Those coordinates don't exist!\n\r", th);
	  return;	
	}
    }  
  else
    arg = oldarg;
#endif
  if ((is_number (arg) &&
       (dest = find_thing_num (atoi(arg))) == NULL) ||
      (!is_number (arg) && 
       (dest = find_thing_world (th, arg)) == NULL)||
      !dest->in || dest->in == th)
    {
      if (dest && dest->in == th)
	stt ("You're carrying that thing!\n\r", th);
      else
	stt ("That location doesn't exist!\n\r", th);
      return;      
    }
  while (dest)
    {
      if (IS_ROOM (dest))
        break;
      if (IS_AREA (dest))
        {
          dest = NULL;
          break;
        }
      dest = dest->in;
    }
  if (!dest)
    {
      stt ("You can't go there.\n\r", th);    
      return;
    }
  if (!IS_PC (th))
    return;
  for (vict = in_now->cont; vict; vict = victn)
    {
      victn = vict->next_cont;
      /* Lets followers goto also? Restrict to admins? */
      if ((LEVEL(vict) >= BLD_LEVEL && FOLLOWING(vict) == th) ||
      vict == th)
	{		 
	  act ("@1n leave@s in a swirling mist of smoke.", vict, NULL, NULL, NULL, TO_ALL);
	  thing_to (vict, dest);
	  act ("@1n arrive@s with a loud band. :P", vict, NULL, NULL, NULL, TO_ALL);
	  do_look (vict, "");
	  if (!IS_PC (vict))
	    continue;
	  if (IS_PC2_SET (vict, PC2_MAPPING))
	    {
	      if (!USING_KDE (vict))
		create_map (vict, vict->in, SMAP_MAXX, SMAP_MAXY);
	      else
		update_kde (vict, KDE_UPDATE_MAP);
	    }
	  if (vict->fd && vict->fd->connected == CON_EDITING &&
	      (thg = (THING *) vict->pc->editing ) != NULL && IS_ROOM (vict->in) &&
	      IS_ROOM (thg))
	    {
	      vict->pc->editing = vict->in;
	      show_edit (vict);
	    }
	}
    }
  return;
}

/* MOBS NOW CHEAT AND CAN OPEN HIDDEN LOCKED DOORS!!! IF YOU WANT THEM
   TO ACT LIKE PLAYERS AND NEED KEYS AND KEYWORDS AND ALL THAT, THEN
   CHANGE THIS BY TAKING OUT ALL THE IS_PC LINES. */

void 
do_open (THING *th, char *arg)
{
  char buf[STD_LEN];
  char doorname[200];
  THING *obj = NULL;
  VALUE *door = NULL;
  int dir;
  dir = find_dir (arg);
  
  if (!th->in)
    return;
  for (door = th->in->values; door != NULL; door = door->next)
    {
      if (dir != DIR_MAX && door->type == dir + 1 &&
	  (!IS_SET (door->val[2], EX_HIDDEN) || !IS_PC (th)))
	break;
      if (door->word && door->word[0] && 
	  !str_cmp (door->word, arg))
	break;
    }
  if (!door && (obj = find_thing_here (th, arg, TRUE)) != NULL)
    door = FNV (obj, VAL_EXIT_O);
  if (!door || IS_SET (door->val[1], EX_WALL))
    {
      stt ("There isn't anything like that here to open!\n\r", th);
      return;
    }
  if (obj)
    sprintf (doorname, "%s", NAME(obj));
  else
    sprintf (doorname, "The %s", 
	     (door->word && door->word[0] ? door->word : "door"));
  if (!IS_SET (door->val[1], EX_DOOR))
    {
      stt ("That doesn't have a door!\n\r", th);
      return;
    }
  if (!IS_SET (door->val[1], EX_CLOSED))
    {
      sprintf (buf, "%s is not closed!\n\r", doorname);
      stt (buf, th);
      return;
    }
  
  if (IS_SET (door->val[1], EX_LOCKED) && IS_PC (th) && 
      !IS_PC1_SET (th, PC_HOLYWALK))
    {
      sprintf (buf, "%s is locked!\n\r", doorname);
      stt (buf, th);
      return;
    }
  
  if (!IS_SET (door->val[1], EX_HIDDEN) || IS_PC1_SET (th, PC_HOLYLIGHT) ||
      dir == DIR_MAX || !IS_PC (th))
    {
      RBIT (door->val[1], EX_CLOSED);    
      act ("@1n open@s @t.", th, NULL, NULL, doorname, TO_ALL);
      if (!obj)
	reverse_door_do (th->in, door, "opened");
      else if (obj->cont)
	act ("@t opens up.", obj->cont, NULL, NULL, doorname, TO_ALL);
      if (IS_SET (door->val[2], EX_DRAINING))
	{
	  act ("@t drains @1n.", th, NULL, NULL, doorname, TO_ALL);
	  th->hp /= 2;
	  th->mv /= 2;
	  if (IS_PC (th))
	    th->pc->mana /= 2;
	}
    }
  else
    stt ("Open what?\n\r", th);
  return;
}

void 
do_close (THING *th, char *arg)
{
  
 char buf[STD_LEN];
  char doorname[200];
  THING *obj = NULL;
  VALUE *door = NULL;
  int dir;
  dir = find_dir (arg);
  
  if (!th->in)
    return;
  for (door = th->in->values; door != NULL; door = door->next)
    {
      if (dir != DIR_MAX && door->type == dir + 1 &&
	  !IS_SET (door->val[2], EX_HIDDEN))
	break;
      if (door->word && door->word[0] && !str_cmp (door->word, arg))
	break;
    }  
  if (!door && (obj = find_thing_here (th, arg, TRUE)) != NULL)
    {
      door = FNV (obj, VAL_EXIT_O);
    }
  if (!door || IS_SET (door->val[1], EX_WALL))
    {
      stt ("There isn't anything like that here to close!\n\r", th);
      return;
    }
  if (obj)
    sprintf (doorname, "%s", NAME(obj));
  else
    sprintf (doorname, "The %s", 
	     (door->word && door->word[0] ? door->word : "door"));
  if (!IS_SET (door->val[1], EX_DOOR))
    {
      stt ("That doesn't have a door!\n\r", th);
      return;
    }
  if (IS_SET (door->val[1], EX_CLOSED))
    {
      sprintf (buf, "%s is not open!\n\r", doorname);
      stt (buf, th);
      return;
    }
  SBIT (door->val[1], EX_CLOSED);
  act ("@1n close@s @t.", th, NULL, NULL, doorname, TO_ALL);
  
  if (!obj)
    reverse_door_do (th->in, door, "closed");
  else if (obj->cont)
    act ("@t closes up.", obj->cont, NULL, NULL, doorname, TO_ALL);
  return;  
}

void
do_pick (THING *th, char *arg)
{
  char buf[STD_LEN];
  THING *lockpicks, *obj = NULL;
  char doorname[200];
  VALUE *door, *tool;
  int dir;

  if (!th->in)
    return;
  dir = find_dir (arg);
  for (door = th->in->values; door != NULL; door = door->next)
    {
      if (dir != DIR_MAX && door->type == dir + 1 &&
	  !IS_SET (door->val[2], EX_HIDDEN))
	break;
      if (door->word && door->word[0] && !str_cmp (door->word, arg))
	break;
    }
  if (!door && (obj = find_thing_here (th, arg, TRUE)) != NULL)
    {
      door = FNV (obj, VAL_EXIT_O);
    }
  if (!door || IS_SET (door->val[1], EX_WALL))
    {
      stt ("There isn't anything like that here to pick!\n\r", th);
      return;
    }
  
  if (IS_SET (~door->val[1], EX_DOOR | EX_CLOSED | EX_LOCKED))
    {
      sprintf (buf, "%s is not closed and locked!\n\r", doorname);
      stt (buf, th);
      return;
    }
  for (lockpicks = th->cont; lockpicks; lockpicks = lockpicks->next_cont)
    {
      if ((tool = FNV (lockpicks, VAL_TOOL)) != NULL &&
	  tool->val[0] == TOOL_LOCKPICKS && IS_WORN (lockpicks))
	break;
    }
  if (!lockpicks)
    {
      stt ("You must be using lockpicks to pick a lock!\n\r", th);
      return;
    }
  if (IS_SET (door->val[1], EX_PICKPROOF))
    {
      stt ("That lock cannot be picked!\n\r", th);
      return;
    }
  
  if (obj)
    sprintf (doorname, "%s", NAME(obj));
  else
    sprintf (doorname, "The %s", 
	     (door->word && door->word[0] ? door->word : "door"));
  stt ("*Click*\n\r", th);
  act ("@1n pick@s @t.", th, NULL, NULL, doorname, TO_ALL);
  RBIT (door->val[1], EX_LOCKED);
  if (!obj)
    reverse_door_do (th->in, door, "picked");
  else if (obj->cont)
    act ("The @t is picked.", obj->cont, NULL, NULL, doorname, TO_ALL);
  return;  
}


/* This lets you try to destroy a door by ramming into it. */


void
do_break (THING *th, char *arg)
{
  THING *obj = NULL;
  char doorname[200];
  VALUE *door;
  int dir, dam;

  if (!th->in)
    return;
  dir = find_dir (arg);
  for (door = th->in->values; door != NULL; door = door->next)
    {
      if (dir != DIR_MAX && door->type == dir + 1 &&
	  !IS_SET (door->val[2], EX_HIDDEN))
	break;
      if (door->word && door->word[0] && !str_cmp (door->word, arg))
	break;
    }
  if (!door && (obj = find_thing_here (th, arg, TRUE)) != NULL)
    {
      door = FNV (obj, VAL_EXIT_O);
    }
  if (!door || IS_SET(door->val[1], EX_WALL))
    {
      stt ("There isn't anything like that here to break!\n\r", th);
      return;
    }

  if (IS_SET (door->val[2], EX_NOBREAK) || door->val[4] == 0)
    {
      door->val[4] = 0;
      stt ("You can't break this down!\n\r", th);
      return;
    }
  
  if (door->val[3] < 1  && door->val[4] > 0)
    {
      stt ("It's already broken!\n\r", th);
      return;
    }
  dam = nr (1, get_stat (th, STAT_STR));
  dam -= door->val[3]/10;

  if (dam > 0)
    door->val[3] -= MIN (dam, door->val[3]);
  else
    dam = -dam + 2;  

  if (obj)
    sprintf (doorname, "%s",  NAME(obj));
  else
    sprintf (doorname, "The %s", 
	     (door->word && door->word[0] ? door->word : "door"));
  
  act ("@1n slam@s into the @t.", th, NULL, NULL, doorname, TO_ALL);
  
  if (!obj)
    reverse_door_do (th->in, door, "pounded");
  
  if (door->val[3]  < 1)
    {
      door->val[3] = 0;
      act ("@t is destroyed!", th, NULL, NULL, doorname, TO_ALL);
      RBIT (door->val[1], EX_DOOR | EX_CLOSED | EX_HIDDEN | EX_LOCKED);
      if (!obj)
	reverse_door_do (th->in, door, "broken");
      else if (obj->cont)
	act ("The @t is broken open.", obj->cont, NULL, NULL, doorname, TO_ALL);
    }
  /* Do damage AFTER all the messages and crap get done. Much easier. */

  damage ((obj ? obj : th->in), th, dam, "hit");
  return;
}


void
do_lock (THING *th, char *arg)
{
  THING *key, *obj = NULL;
  char doorname[200];
  VALUE *door;
  int dir;
  
  if (!th->in)
    return;
  dir = find_dir (arg);
  for (door = th->in->values; door != NULL; door = door->next)
    {
      if (dir != DIR_MAX && door->type == dir + 1 &&
	  !IS_SET (door->val[2], EX_HIDDEN))
	break;
      if (door->word && door->word[0] && !str_cmp (door->word, arg))
	break;
    }
  if (!door && (obj = find_thing_here (th, arg, TRUE)) != NULL)
    {
      door = FNV (obj, VAL_EXIT_O);
    }
  if (!door)
    {
      stt ("There isn't anything like that here to lock!\n\r", th);
      return;
    }
  if (!IS_SET (door->val[1], EX_DOOR))
    {
      stt ("That isn't a door.\n\r", th);
      return;
    }
  if (!IS_SET (door->val[1], EX_CLOSED))
    do_close (th, arg);
  if (IS_SET (door->val[1], EX_LOCKED))
    {
      stt ("It is already locked!\n\r", th);
      return;
    }
  if (door->val[6] == 0)
    {
      stt ("You aren't holding the proper key!\n\r", th);
      return;
    }
  for (key = th->cont; key; key = key->next_cont)
    {
      if (key->wear_loc == ITEM_WEAR_WIELD &&
	  key->vnum == door->val[6])
	break;
    }
  if (!key)
    {
      stt ("You aren't holding the proper key!\n\r", th);
      return;
    }
  if (obj)
    sprintf (doorname, "%s", NAME(obj));
  else
    sprintf (doorname, "The %s", 
	     (door->word && door->word[0] ? door->word : "door"));
  
  act ("@1n lock@s @t.", th, NULL, NULL, doorname, TO_ALL);
  SBIT (door->val[1], EX_LOCKED);
 if (!obj)
    reverse_door_do (th->in, door, "locked");
 else if (obj->cont)
   act ("@t is locked.", obj->cont, NULL, NULL, doorname, TO_ALL);
  return;

}

void
do_unlock (THING *th, char *arg)
{
  THING *key, *obj = NULL;
  char doorname[200];
  VALUE *door;
  int dir;
  
  if (!th->in)
    return;
  dir = find_dir (arg);
  for (door = th->in->values; door != NULL; door = door->next)
    {
      if (dir != DIR_MAX && door->type == dir + 1 &&
	  !IS_SET (door->val[2], EX_HIDDEN))
	break;
      if (door->word && door->word[0] && !str_cmp (door->word, arg))
	break;
    }
  if (!door && (obj = find_thing_here (th, arg, TRUE)) != NULL)
    {
      door = FNV (obj, VAL_EXIT_O);
    }
  if (!door)
    {
      stt ("There isn't anything like that here to lock!\n\r", th);
      return;
    }
  if (!IS_SET (door->val[1], EX_DOOR))
    {
      stt ("That isn't a door.\n\r", th);
      return;
    }
  if (!IS_SET (door->val[1], EX_CLOSED))
    {
      stt ("It isn't closed!\n\r", th);
      return;
    }
  if (!IS_SET (door->val[1], EX_LOCKED))
    {
      stt ("It isn't locked!\n\r", th);
      return;
    }
  if (door->val[6] == 0)
    {
      stt ("You aren't holding the proper key!\n\r", th);
      return;
    }
  for (key = th->cont; key; key = key->next_cont)
    {
      if (key->wear_loc == ITEM_WEAR_WIELD &&
	  key->vnum == door->val[6])
	break;
    }
  if (!key)
    {
      stt ("You aren't holding the proper key!\n\r", th);
      return;
    }
  if (obj)
    sprintf (doorname, "%s", NAME(obj));
  else
    sprintf (doorname, "The %s", 
	     (door->word && door->word[0] ? door->word : "door"));
  
  act ("@1n unlock@s @t.", th, NULL, NULL, doorname, TO_ALL);
  RBIT (door->val[1], EX_LOCKED);
  if (!obj)
    reverse_door_do (th->in, door, "unlocked");
  else if (obj->cont)
    act ("@t is unlocked.", obj->cont, NULL, NULL, doorname, TO_ALL);
  return;

}

  

int
find_dir (char *arg)
{
  int dir;  
  if (!arg || !*arg)
    return DIR_MAX;
  /*  if (isdigit(*arg) &&
      atoi (arg) >= 0 && atoi (arg) < DIR_MAX)
      return atoi(arg);  */
  for (dir = 0; dir < DIR_MAX; dir++)
    {
      if (!str_prefix (arg, dir_name[dir]))
	return dir;	  
    }
  return DIR_MAX;
}
  

void
reverse_door_do (THING *oldroom, VALUE *door, char *msg)
{
  THING *room;
  VALUE *revdoor;
  char buf[200];
  if (!oldroom || !door || !msg)
    return;
  if ((room = find_thing_num (door->val[0])) == NULL)
    return;
  if ((revdoor = FNV (room, (RDIR(door->type -1)) + 1)) == NULL || 
      IS_SET (revdoor->val[1], EX_WALL))
      return;
  if (revdoor->val[0] != oldroom->vnum || !IS_SET (revdoor->val[1], EX_DOOR))
    return;
  if (!str_cmp (msg, "closed"))
    SBIT (revdoor->val[1], EX_CLOSED);
  else if (!str_cmp (msg, "picked") || !str_cmp (msg, "unlocked"))
    RBIT (revdoor->val[1], EX_LOCKED);
  else if (!str_cmp (msg, "locked"))
    SBIT (revdoor->val[1], EX_LOCKED);
  else if (!str_cmp (msg, "opened"))
    RBIT (revdoor->val[1], EX_CLOSED);
  else if (!str_cmp (msg, "broken"))
    {
      RBIT (revdoor->val[1], EX_CLOSED | EX_HIDDEN | EX_DOOR | EX_LOCKED);
      revdoor->val[3] -= revdoor->val[3] % 1000;
    }  
  else if (!str_cmp (msg, "knock"))
    {
      if (revdoor->type >= 1 && revdoor->type <= DIR_UP)
	{
	  sprintf (buf, "Someone knocks on the %sern %s.\n\r",
		   dir_name[revdoor->type - 1], 
		   (revdoor->word && *revdoor->word ? revdoor->word :
		    "door"));
	}
      else if (revdoor->type > DIR_UP && revdoor->type <= DIR_MAX)
	{
	  sprintf (buf, "Someone knocks on the %s %s.\n\r",
		   (revdoor->word && *revdoor->word ? revdoor->word :
		    "door"), dir_name[revdoor->type]);
	}
      else 
	return;
    }
  else
    {
      sprintf (buf, "The %s is %s from the other side.", 
	       (revdoor->word && revdoor->word[0] ? revdoor->word : "door"),
	       msg);
    }
  if (room->cont)
    act (buf, room->cont, NULL, NULL, NULL, TO_ALL);
  return;
}


void
do_stand (THING *th, char *arg)
{
  char buf[STD_LEN];
  THING *thg;
  if (th->position == POSITION_STANDING)
    {
      stt ("You are standing.\n\r", th);
      return;
    }
  if (th->position == POSITION_STUNNED ||
      th->position == POSITION_FIGHTING)
    {
      sprintf (buf, "You cannot stand while you are %s!\n\r", position_names[th->position]);
      stt (buf, th);
      return;
    }
  
  
  
  if (th->position == POSITION_TACKLED)
    {
      if (nr (1,3) == 2 ||
	  (FIGHTING (th) && 
	   ((check_spell (FIGHTING(th), NULL, 564 /* Grapple */) &&
	     nr (1,3) == 2) ||
	    (check_spell (FIGHTING(th), NULL, 564 /* Grapple */) &&
	     nr (1,2) != 2))))
	{
	  stt ("You try to stand up, but fail!\n\r", th);
	  return;
	}
      act ("$9@1n stand@s up from being tackled!$7", th, NULL, NULL, NULL, TO_ALL);
      if (FIGHTING (th) && FIGHTING(th)->in == th->in)
	th->position = POSITION_FIGHTING;
      else
	th->position = POSITION_STANDING;
      if (!th->fgt)
	add_fight (th);
      th->fgt->bashlag = 2;
      for (thg = th->in->cont; thg; thg = thg->next_cont)
	{
	  if (FIGHTING (thg) == th && thg->position == POSITION_TACKLED)
	    {
	      thg->position = POSITION_FIGHTING;
	    }
	}
      return;
    }
  act ("@1n stand@s up.", th, NULL, NULL, NULL, TO_ALL);
  th->position = POSITION_STANDING;
  return;
}


void
do_sleep (THING *th, char *arg)
{
  char buf[STD_LEN];
  if (th->position == POSITION_SLEEPING)
    {
      stt ("You are sleeping.\n\r", th);
      return;
    }
  if (th->position == POSITION_STUNNED ||
      th->position == POSITION_FIGHTING ||
      th->position == POSITION_TACKLED)
    {
      sprintf (buf, "You cannot stand while you are %s!\n\r", position_names[th->position]);
      stt (buf, th);
      return;
    }
  if (MOUNT (th))
    {
      act ("@1n dismount@s from @2n.", th, MOUNT (th), NULL, NULL, TO_ALL);
      if (MOUNT(th)->fgt)
	MOUNT(th)->fgt->rider = NULL;
      th->fgt->mount = NULL;
    }
  act ("@1n go@s to sleep.", th, NULL, NULL, NULL, TO_ALL);
  th->position = POSITION_SLEEPING;
  return;
}


void
do_rest (THING *th, char *arg)
{
  char buf[STD_LEN];
  if (th->position == POSITION_RESTING)
    {
      stt ("You are resting.\n\r", th);
      return;
    }
  if (th->position == POSITION_STUNNED ||
      th->position == POSITION_FIGHTING ||
      th->position == POSITION_TACKLED)
    {
      sprintf (buf, "You cannot stand while you are %s!\n\r", position_names[th->position]);
      stt (buf, th);
      return;
    }
  if (MOUNT (th))
    {
      act ("@1n dismount@s from @2n.", th, MOUNT (th), NULL, NULL, TO_ALL);
      if (MOUNT(th)->fgt)
	MOUNT(th)->fgt->rider = NULL;
      th->fgt->mount = NULL;
    }
  act ("@1n sit@s down and begin@s to rest.", th, NULL, NULL, NULL, TO_ALL);
  th->position = POSITION_RESTING;
  return;
}


void
do_wake (THING *th, char *arg)
{
  THING *thg;
  
  /* See if we are waking self, or other. */
  
  if ((thg = find_thing_in (th, arg)) == NULL)
    thg = th;
  
  if (thg->position != POSITION_SLEEPING)
    {
      if (thg == th)
	stt ("You aren't sleeping!\n\r", th);
      else
	act ("@3n isn't sleeping!", th, NULL, thg, NULL, TO_CHAR);
      return;
    }


  if (IS_HURT (thg, AFF_SLEEP))
    {
      act ("You can't seem to wake @3f up!", th, NULL, thg, NULL, TO_CHAR);
      return;
    }
  
  thg->position = POSITION_STANDING;
  if (thg == th)
    act ("@1n wake@s @3f and stand@s.", th, NULL, thg, NULL, TO_ALL);
  else
    act ("@1n wake@s @3n up.", th, NULL, thg, NULL, TO_ALL);
  return;
}
  


void
do_eat (THING *th, char *arg)
{
  THING *food, *in_now;
  SPELL *spl;
  VALUE *fod;
  int i;
  
  if ((in_now = th->in) == NULL)
    return;

  if ((food = find_thing_me (th, arg)) == NULL &&
      (food = find_thing_worn (th, arg)) == NULL)
    {
      stt ("You don't have that food to eat!\n\r", th);
      return;
    }
  
  if ((fod = FNV (food, VAL_FOOD)) == NULL &&
      LEVEL (th) < MAX_LEVEL)
    {
      stt ("You can't eat that.\n\r", th);
      return;
    }
  
  if (IS_PC (th) && LEVEL (th) < MAX_LEVEL &&
      th->pc->cond[COND_HUNGER] > COND_FULL - 10)
    {
      stt ("You are too full to eat!\n\r", th);
      return;
    }
  
  act ("@1n eat@s @3n.", th, NULL, food, NULL, TO_ALL);

  if (fod)
    {
      if (IS_PC (th))
	{
	  th->pc->cond[COND_HUNGER] = MIN (COND_FULL, th->pc->cond[COND_HUNGER] + fod->val[0]);
	  if (th->pc->cond[COND_HUNGER] >= COND_FULL - 10)
	    {
	      stt ("You are full.\n\r", th);
	    }
	}
      
      for (i = 3; i < NUM_VALS && th->in == in_now; i++)
	{
	  if ((spl = find_spell (NULL, fod->val[i])) != NULL &&
	      (spl->spell_type == SPELLT_SPELL ||
	       spl->spell_type == SPELLT_POISON))
	    {
	      cast_spell (food, th, spl, FALSE, FALSE, NULL);
	      if (IS_PC (th))
		th->pc->wait += 8;
	    }
	}
      
    }
  free_thing (food);
  return;

}




void
do_drink (THING *th, char *arg)
{

  THING *drink, *in_now;
  SPELL *spl;
  VALUE *drk;
  int i;
  bool did_cast = FALSE;
  
  if ((in_now = th->in) == NULL)
    return;
  
  if (IS_PC (th) && LEVEL (th) < MAX_LEVEL &&
      th->pc->cond[COND_THIRST] > COND_FULL - 10)
    {
      stt ("Your bladder will explode if you drink anymore!\n\r", th);
      return;
    }
  
  if ((drink = find_thing_here (th, arg, TRUE)) == NULL &&
      (drink = find_thing_worn (th, arg)) == NULL)
    {
      if (IS_ROOM_SET (in_now, ROOM_WATERY | ROOM_UNDERWATER))
	{
	  act ("@1n gulp@s down some of the water around @1m.", th, NULL, NULL, NULL, TO_ALL);
	  if (IS_PC(th))
	    th->pc->cond[COND_THIRST] = 
	      MIN (COND_FULL, th->pc->cond[COND_THIRST] + 10);
	  return;
	}
      
      
      stt ("You don't have anything like that to drink!\n\r", th);
      return;
    }
  
  if ((drk = FNV (drink, VAL_DRINK)) == NULL)
    {
      stt ("You can't drink that.\n\r", th);
      return;
    }
  
  if (drk->val[1] > 0 && drk->val[0] < 1)
    {
      stt ("Your drink container is empty!\n\r", th);
      return;
    }
  
  act ("@1n drink@s from @3n.", th, NULL, drink, NULL, TO_ALL);
  
  if (IS_PC (th))
    {
      th->pc->cond[COND_THIRST] = MIN (COND_FULL, th->pc->cond[COND_THIRST] + 20);
      if (th->pc->cond[COND_THIRST] >= COND_FULL - 10)
	{
	  stt ("You're not thirsty.\n\r", th);
	}
    }
  
  for (i = 3; i < NUM_VALS && th->in == in_now; i++)
    {
      if ((spl = find_spell (NULL, drk->val[i])) != NULL &&
	  (spl->spell_type == SPELLT_SPELL ||
	   spl->spell_type == SPELLT_POISON))
	{
	  cast_spell (drink, th, spl, FALSE, FALSE, NULL);
	  did_cast = TRUE;
	  if (IS_PC (th))
	    th->pc->wait += 8;
	}
    }
  
  if (drk->val[1] > 0 && --drk->val[0] < 1)
    {
      if (!did_cast)
	stt ("Your container is now empty.\n\r", th);
      else
	free_thing (drink);
    }
  return;  
}


void
do_climb (THING *th, char *arg)
{
  THING *obj, *room;
  int dir;
  char arg1[STD_LEN];
  VALUE *climb;
  
  arg = f_word (arg, arg1);
  
  
  if (!*arg || !arg1[0])
    {
      stt ("Climb <direction> <object>.\n\r", th);
      return;
    }
  
  
  if ((dir = find_dir (arg1)) == DIR_MAX)
    {
      stt ("Climb <direction> <object>.\n\r", th);
      return;
    }
  
  if ((obj = find_thing_in (th, arg)) == NULL)
    {
      stt ("You don't see that here.\n\r", th);
      return;
    }
  
  if ((climb = FNV (obj, VAL_OUTEREXIT_N + dir)) == NULL)
    {
      stt ("You can't climb on that thing in that direction!\n\r", th);
      return;
    }
  
  if ((room = find_thing_num (climb->val[0])) == NULL ||
      !IS_ROOM (room) ||
      room == th->in)
    {
      act ("@1n attempt@s to climb @t @3n, but it doesn't lead anywhere.",
	   th, NULL, obj, dir_name[dir], TO_ALL);
      return;
    }
  
  if (MOUNT (th))
    {
      act ("@1n dismount@s from @2n.", th, MOUNT (th), NULL, NULL, TO_ALL);
      if (MOUNT(th)->fgt)
	MOUNT(th)->fgt->rider = NULL;
      th->fgt->mount = NULL;
    }
  if (RIDER (th))
    {
      act ("@1n fall@s off @2n.", RIDER(th), th, NULL, NULL, TO_ALL);
      if (RIDER(th)->fgt)
	RIDER(th)->fgt->mount = NULL;
      th->fgt->rider = NULL;
    }
  act ("@1n climb@s @t @3n.", th, NULL, obj, dir_name[dir], TO_ALL);
  thing_to (th, room);
  act ("@1n step@s off @3n.", th, NULL, obj, NULL, TO_ROOM);
   create_map (th, th->in, SMAP_MAXX, SMAP_MAXY);

  return;
}



void
do_mount (THING *th, char *arg)
{
  THING *vict;
  
  if ((vict = find_thing_in (th, arg)) == NULL)
    {
      stt ("Mount what?\n\r", th);
      return;
    }
  
  if (!CAN_MOVE (vict)) 
    {
      stt ("You can't mount that.\n\r", th);
      return;
    }
  
  if (th->position != POSITION_STANDING || vict->position != POSITION_STANDING)
    {
      stt ("You and your victim must both be standing for you to mount it.\n\r", th);
      return;
    }
  
  if (!IS_ACT1_SET (vict, ACT_MOUNTABLE) &&
      (!IS_PC (th) || LEVEL (th) < MAX_LEVEL))
    {
      stt ("You can't ride that thing.\n\r", th);
      return;
    }
  
  if (MOUNT(vict))
    {
      stt ("That thing is already riding something.\n\r", th);
      return;
    }
  
  if (RIDER (th))
    {
      stt ("You have a rider already.\n\r", th);
      return;
    }
  
  if (MOUNT (th))
    {
      stt ("You have a mount already.\n\r", th);
      return;
    }
  
  if (RIDER (vict))
    {
      stt ("Someone is already riding your victim.\n\r", th);
      return;
    }
  
  add_fight (th);
  add_fight (vict);
  
  th->fgt->mount = vict;
  vict->fgt->rider = th;
  
  act ("@1n begin@s riding @3n.", th, NULL, vict, NULL, TO_ALL);
  return;
}
				

void
do_dismount (THING *th, char *arg)
{

    if (!MOUNT(th))
    {
      stt ("You aren't riding anything.\n\r", th);
      return;
    }
  
  if (RIDER(MOUNT(th)) == th) 
    th->fgt->mount->fgt->rider = NULL;

  act ("@1n dismount@s from @3n.", th, NULL, th->fgt->mount, NULL, TO_ALL);
  th->fgt->mount = NULL;
  return;
}


void
do_buck (THING *th, char *arg)
{
  if (!RIDER(th))
    {
      stt ("Noone is riding you!\n\r", th);
      return;
    }
  
  if (MOUNT(RIDER(th)) == th)
    RIDER(th)->fgt->mount = NULL;
  
  act ("@1n buck@s @3n from @1s back!", th, NULL, RIDER(th), NULL, TO_ALL);
  th->fgt->rider = NULL;
  return;
}



void
do_light (THING *th, char *arg)
{
  THING *obj, *th_in;
  VALUE *light;
  
  
  if ((obj = find_thing_here (th, arg, TRUE)) == NULL &&
      (obj = find_thing_worn (th, arg)) == NULL)
    {
      stt ("Light what?\n\r", th);
      return;
    }
  
  if ((light = FNV (obj, VAL_LIGHT)) == NULL)
    {
      stt ("That isn't a light!\n\r", th);
      return;
    }
  
  if (light->val[1] > 0 && light->val[0] < 1)
    {
      if (IS_SET (light->val[2], LIGHT_FILLABLE))
	{
	  act ("@2n needs some more oil.", th, obj, NULL, NULL, TO_CHAR);
	  return;
	}
      else
	{
	  act ("@2n is burnt out and cannot be relit!\n\r", th, obj, NULL, NULL, TO_CHAR);
	  return;
	}
    }
  
  if (IS_SET (light->val[2], LIGHT_LIT))
    {
      stt ("It's already lit!\n\r", th);
      return;
    }
  
  SBIT (light->val[2], LIGHT_LIT);
  act ("@1n light@s @2n.", th, obj, NULL, NULL, TO_ALL);
  obj->light++;
  th_in = obj;
  while ((th_in = th_in->in) != NULL && !IS_AREA (th_in))
    th_in->light++;
  return;
}


void
do_extinguish (THING *th, char *arg)
{
  THING *obj, *th_in;
  VALUE *light;
  
  if ((obj = find_thing_here (th, arg, TRUE)) == NULL &&
      (obj = find_thing_worn (th, arg)) == NULL)
    {
      stt ("Extinguish what?\n\r", th);
      return;
    }
  
  if ((light = FNV (obj, VAL_LIGHT)) == NULL)
    {
      stt ("That isn't a light!\n\r", th);
      return;
    }
  
  if (!IS_SET (light->val[2], LIGHT_LIT))
    {
      stt ("It's already lit!\n\r", th);
      return;
    }
  
  RBIT (light->val[2], LIGHT_LIT);
  act ("@1n extinguish@s @2n.", th, obj, NULL, NULL, TO_ALL);
  obj->light = MAX (0, obj->light - 1);
  th_in = obj;
  while ((th_in = th_in->in) != NULL && !IS_AREA (th_in))
    th_in->light = MAX (0, th_in->light - 1);
  return;
}
  
void
do_sneak (THING *th, char *arg)
{
  SPELL *spl;
  FLAG *flg;
  
  if ((spl = find_spell ("sneak", 0)) == NULL)
    {
      stt ("You can't sneak!\n\r", th);
      return;
    }
  if (th->position != POSITION_STANDING)
    {
      stt ("You must be standing to sneak.\n\r", th);
      return;
    }
  if (IS_PC (th) && th->pc->prac[spl->vnum] < 25)
    {
      stt ("You don't know how to sneak!\n\r", th);
      return;
    }
  
  stt ("You begin to sneak silently...\n\r", th);
  if (IS_PC (th) && !check_spell (th, NULL, spl->vnum))
    {
      return;
    }

  flg = new_flag ();
  flg->from = spl->vnum;
  flg->type = FLAG_AFF;
  flg->val = AFF_SNEAK;
  flg->timer = MAX (3, math_parse (spl->duration, LEVEL (th)));
  aff_update (flg, th);
  return;
}
  
void
do_hide (THING *th, char *arg)
{
  SPELL *spl;
  FLAG *flg;
  
  if ((spl = find_spell ("hide", 0)) == NULL)
    {
      stt ("You can't hide!\n\r", th);
      return;
    }
  if (th->position != POSITION_STANDING)
    {
      stt ("You must be standing to hide.\n\r", th);
      return;
    }
  if (IS_PC (th) && th->pc->prac[spl->vnum] < 25)
    {
      stt ("You don't know how to hide!\n\r", th);
      return;
    }
  
  stt ("You begin to hide...\n\r", th);
  if (IS_PC (th) && !check_spell (th, NULL, spl->vnum))
    {
      return;
    }

  flg = new_flag ();
  flg->from = spl->vnum;
  flg->type = FLAG_AFF;
  flg->val = AFF_HIDDEN;
  flg->timer = MAX (3, math_parse (spl->duration, LEVEL (th)));
  aff_update (flg, th);
  return;
}
  
void
do_chameleon (THING *th, char *arg)
{
  SPELL *spl;
  FLAG *flg;
  
  if ((spl = find_spell ("chameleon", 0)) == NULL)
    {
      stt ("You can't chameleon!\n\r", th);
      return;
    }
  if (th->position != POSITION_STANDING)
    {
      stt ("You must be standing to chameleon.\n\r", th);
      return;
    }
  if (IS_PC (th) && th->pc->prac[spl->vnum] < 25)
    {
      stt ("You don't know how to chameleon!\n\r", th);
      return;
    }
  
  stt ("You fade into the environment...\n\r", th);
  if (IS_PC (th) && !check_spell (th, NULL, spl->vnum))
    {
      return;
    }

  flg = new_flag ();
  flg->from = spl->vnum;
  flg->type = FLAG_AFF;
  flg->val = AFF_CHAMELEON;
  flg->timer = MAX (3, math_parse (spl->duration, LEVEL (th)));
  aff_update (flg, th);
  return;
}

void
do_visible (THING *th, char *arg)
{
  if (th->position != POSITION_STANDING)
    {
      stt ("You cannot go visible unless you are standing!\n\r", th);
      return;
    }
  
  remove_flagval (th, FLAG_AFF, AFF_INVIS | AFF_HIDDEN | AFF_DARK | AFF_SNEAK | AFF_CHAMELEON);
  stt ("You become visible\n\r", th);
  return;
}


/* This function lets a thing fill a drink in its inventory from
   another thing. The thing that is doing the filling must have unlimited
   liquids in it. */

void
do_fill (THING *th, char *arg)
{
  THING *drink, *fountain;
  char arg1[STD_LEN];
  VALUE *drval, *fnval;
  int dr_int, fn_int;
 
  arg = f_word (arg, arg1);

  if (!*arg1 || !*arg)
    {
      stt ("Fill <drink container> <liquid source>\n\r", th);
      return;
    }

  if ((drink = find_thing_me (th, arg1)) == NULL ||
      (drval = FNV (drink, VAL_DRINK)) == NULL ||
      drval->val[1] == 0 /* unlim drinks */)
    {
      stt ("You can't fill that!\n\r", th);
      return;
    }
  
  if ((fountain = find_thing_here (th, arg, FALSE)) == NULL ||
      (fnval = FNV (fountain, VAL_DRINK)) == NULL)
    {
      stt ("You can't find that here to use to fill!\n\r", th);
      return;
    }
  
  if (fnval->val[1] != 0 /* Not unlim */)
    {
      stt ("That source can't be used to fill something else up!\n\r", th);
      return;
    }
  
  /* Now we have our drink and the fountain we will use to fill the drink.
     This part is cute...if you fill a drink from a fountain, it
     takes on the spell attributes of the fountain. =D */
  
  drval->val[0] = drval->val[1];
  
  dr_int = 3;
  for (fn_int = 3; fn_int < NUM_VALS; fn_int++)
    {
      if (fnval->val[fn_int] > 0)
	{
	  while (drval->val[dr_int] != 0 && dr_int < NUM_VALS)
	    dr_int++;
	  if (dr_int < NUM_VALS)
	    drval->val[dr_int++] = fnval->val[fn_int];
	}
    }
  
  act ("@1n fill@s @2n from @3n.", th, drink, fountain, NULL, TO_ALL);
  return;
}


void
do_push (THING *th, char *arg)
{
  int dir = DIR_MAX;
  THING *vict, *to_room, *from_room;
  char arg1[STD_LEN];
  VALUE *exit;

  if (!th->in)
    return;
  
  from_room = (IS_ROOM (th) ? th : th->in);
  if (!IS_ROOM (from_room))
    {
      stt ("You can't push anyone around here!\n\r", th);
      return;
    }
  
  if (!*arg)
    {
      stt ("Push <victim> <direction>\n\r", th);
      return;
    }
  
  arg = f_word (arg, arg1);
  
  if ((vict = find_thing_in (th, arg1)) == NULL)
    {
      stt ("They aren't here!\n\r", th);
      return;
    }
  
  if (!CAN_MOVE (vict))
    {
      stt ("You can't push that thing around.\n\r", th);
      return;
    }
  
  if ((dir = find_dir (arg)) == DIR_MAX)
    {
      stt ("Push <victim> <direction>\n\r", th);
      return;
    }

  if ((exit = FNV (from_room, dir + 1)) == NULL ||
      (to_room = find_thing_num (exit->val[0])) == NULL ||
      !IS_ROOM (to_room) || to_room == from_room ||
      (IS_SET (exit->val[1], EX_DOOR) && IS_SET (exit->val[1], EX_CLOSED)) ||
      IS_SET (exit->val[1], EX_WALL))
    {
      stt ("You can't push anyone that way!\n\r", th);
      return;
    }
  
  if (nr (1,3) == 3 && 
      nr (1,4*get_stat(vict, STAT_STR)) < get_stat (th, STAT_STR))
    {
      thing_to (vict, to_room);
      act ("@1n push@s @2n @t!", th, vict, NULL, dir_name[dir], TO_ALL);
      act ("@1n get@s pushed in from @t!", vict, NULL, NULL, dir_track[dir], TO_ALL);
    }
      else
    {
      stt ("They won't go!\n\r", th);
    }
  if (IS_PC (th))
    th->pc->wait += 20;

  return;
}




/* This function finds a new location for a mob that's randomly
   patrolling to go to. It only works if the mob is not a 
   sentinel and if either its in a society and is a warrior, or
   it's got a guard value, or its a KILL_OPP_ALIGN mob. */


void
find_new_patrol_location (THING *th)
{
  THING *room;
  VALUE *guardval = NULL, *socval = NULL;
  int actbits, low = 0, high = 0, roombits;
  int roomnum, times = 0;
  
  
  if (!th || !th->in || !IS_ROOM (th->in) || !CAN_MOVE (th) || IS_PC (th) ||
      !th->in->in || !IS_AREA (th->in->in))
    return;

  /* get actbits and check for sentinel. */
  
  actbits = flagbits (th->flags, FLAG_ACT1);
  roombits = flagbits (th->flags, FLAG_ROOM1);
  
  
  /* Only warrior caste members and guards and kill_opps patrol! */

  if (IS_SET (actbits, ACT_SENTINEL) ||
      ((guardval = FNV (th, VAL_GUARD)) == NULL &&
      ((socval = FNV (th, VAL_SOCIETY)) == NULL ||
       !IS_SET (socval->val[2], CASTE_WARRIOR) ||
       socval->val[3] == WARRIOR_SENTRY) &&
      !IS_SET (actbits, ACT_KILL_OPP)))
    return;
  
  /* Patrolling society members either look for a new patrol location,
     or they go home after a while. */

  if (socval && IS_SET (socval->val[2], BATTLE_CASTES) &&
      (socval->val[3] == WARRIOR_PATROL || socval->val[3] == WARRIOR_GUARD))
    {
      SOCIETY *society;
      THING *room, *area = NULL;
      
      if ((society = find_society_in (th)) == NULL)
	return;
      socval->val[3] = WARRIOR_PATROL;
      if (socval->val[4] >= MAX_WAYPOINTS)
	area = find_area_in (society->room_start);
      else if (th->in && th->in->in && IS_AREA (th->in->in))
	area = th->in->in;
      
      if (!area || (room = find_random_room (area, FALSE, 0, 0)) == NULL)
	return;
      
      
      if (socval->val[4] < MAX_WAYPOINTS)
	{
	  start_hunting_room (th, room->vnum, HUNT_PATROL);
	  socval->val[4]++;
	}
      else
	{
	  start_hunting_room (th, room->vnum, HUNT_KILL);
	  socval->val[4] = 0;
	  socval->val[3] = WARRIOR_GUARD;
	}
      return;
    }

  
  /* Don't interfere with hunting except for patrolling. */
  
  if (is_hunting (th) &&
      th->fgt->hunting_type != HUNT_PATROL &&
      th->fgt->hunting_type != HUNT_FAR_PATROL)
    return;
  
  if (th->in && th->in->in && IS_AREA(th->in->in))
    {
      low = th->in->in->vnum + 1;
      high = th->in->in->vnum + th->in->in->armor;
    }
  
  if (low >= high)
    return;
  
  /* Don't feel like doing this properly, so I will just pick a
     random room from the range and see if it exists. */

  room = NULL;
  for (times = 0; times < 10; times++)
    {
      roomnum = nr (low, high);
      if (roomnum != th->in->vnum &&
	  (room = find_thing_num (roomnum)) != NULL &&
	  IS_ROOM (room) &&
	  (!roombits || IS_ROOM_SET (room, roombits)))
	break;
      room = NULL;
    }
  
  if (!room)
    return;

  start_hunting_room (th, room->vnum, HUNT_PATROL);
  if (!hunt_thing (th, 50))
    stop_hunting (th, TRUE);
  
  return;
}


/* This command sends a little message to an adjoining room that
   someone knocked on a door. */

void
do_knock (THING *th, char *arg)
{
  int dir;
  VALUE *exit = NULL;

  if (!th || !th->in || !IS_ROOM (th->in))
    {
      stt ("You can't knock on anything here.\n\r", th);
      return;
    }
  
  if ((dir = find_dir (arg)) != DIR_MAX)
    exit = FNV (th->in, dir + 1);
  
  if (dir == DIR_MAX)
    {
      for (exit = th->in->values; exit; exit = exit->next)
	{
	  if (exit->type <= DIR_MAX &&
	      exit->word && *exit->word &&
	      !str_prefix (arg, exit->word))
	    {
	      dir = exit->type -1;
	      break;
	    }
	}
    }
  
  if (!exit ||
      IS_SET (exit->val[2], EX_HIDDEN | EX_WALL))
    {
      stt ("There's no door there!\n\r", th);
      return;
    }

  
  if (!IS_SET (exit->val[1], EX_CLOSED) ||
      !IS_SET (exit->val[1], EX_DOOR))
    {
      stt ("You can only knock on closed doors.\n\r", th);
      return;
    }
  
  act ("@1n knock@s on the @t.\n\r", th, NULL, NULL, 
       (exit->word && *exit->word ? exit->word : "door"), TO_ALL);
  
  reverse_door_do (th->in, exit, "knock");
  return;
}
	     






