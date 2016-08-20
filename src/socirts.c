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




/* This makes a society member do something. */

void
society_activity (THING *th)
{
  THING *healer, *vict, *room, *thg;
  VALUE *soc, *soc2;
  int flags, guard_count, postnum;
  SPELL *spl;
  SOCIETY *rsoc;
  
  if(!th || !th->in)
    return;
  
  /* First deal with prisoners. */

  if (update_prisoner_status (th))
    return;
  

  /* Society things sleep at night unless they're guarding a guardpost. */

  if (sleeping_time (th) || th->position == POSITION_SLEEPING)
    return;
  
  /* Make sure this thing exists and its in a society and its caste does
     something. */
  
  if (((soc = FNV (th, VAL_SOCIETY)) == NULL) ||
      (flags = soc->val[2]) == 0 ||
      is_hunting (th) ||
      (rsoc = find_society_num (soc->val[0])) == NULL)
    return;
  
  /* Societies are much less likely to do something if they're 
     demoralized. */
  
  if (nr (-MAX_MORALE,0) > rsoc->morale)
    return;
  

  if (nr (1,79) == 24)
    share_rumors (th);
  
  /* Hurt things look for help. */
  
  if (th->hp < th->max_hp/3)
    {
      if (!is_hunting (th) || th->hp < th->max_hp/4)
	{
	  if ((healer = find_society_member_nearby (th, CASTE_HEALER, 5)) != NULL)
	    {
	      start_hunting (th, KEY (healer), HUNT_HEALING);
	      return;
	    }
	}
    }

  /* Now it checks itself for any "needs" that it should
     try to satisfy or carry out. */
  
  if (0 && create_society_member_need (th))
    return;

  
  /* Warriors patrol...*/
  
  
  
  if (IS_SET (flags, BATTLE_CASTES))
    {
      /* If the warrior type hasn't been set yet, then do it. */
      if ((soc->val[3] <= WARRIOR_NONE || soc->val[3] >= WARRIOR_MAX) &&
	  rsoc && !is_hunting(th) && nr(1,8) == 3)
	{
	  
	  /* Go through all "guard post" rooms the society has. */
	  for (postnum = 0; postnum < NUM_GUARD_POSTS; postnum++)
	    {
	      guard_count = 0;
	      if ((room = find_thing_num (rsoc->guard_post[postnum])) != NULL &&
		  IS_ROOM (room))
		{
		  for (thg = room->cont; thg; thg = thg->next_cont)
		    {
		      if ((soc2 = FNV (thg, VAL_SOCIETY)) != NULL &&
			  IS_SET (soc2->val[2], BATTLE_CASTES) &&
			  IS_ACT1_SET (thg, ACT_SENTINEL | ACT_PRISONER))
			guard_count++;
		    }
		  if (guard_count < (rsoc->population/30 + 1) &&
		      nr (1,3) == 2)
		    {
		      soc->val[3] = WARRIOR_SENTRY;
		      start_hunting_room (th, room->vnum, HUNT_GUARDPOST);
		      break;
		    }		  
		}
	    }
	  if (!is_hunting (th) && soc->val[3] != WARRIOR_HUNTER)
	    soc->val[3] = WARRIOR_GUARD;
	}
      else if (!is_hunting (th) && nr (1,80) == 2)
	find_new_patrol_location (th);
      if (!IS_SET (flags, CASTE_WIZARD | CASTE_HEALER))
	return;
    }
  
  /* Necros raise the dead. */

  if (IS_SET (flags, CASTE_WIZARD | CASTE_HEALER) &&
      IS_SET (rsoc->society_flags, SOCIETY_NECRO_GENERATE) &&
      nr (1,30) ==  2 &&
      society_necro_generate (th))
    return;
    
  /* Healers heal... */
  
  if (IS_SET (flags, CASTE_HEALER) && nr (1,12) == 7)
    {
      for (vict = th->in->cont; vict; vict = vict->next_cont)
	{
	  if (in_same_society (th, vict) && 
	      vict->hp < vict->max_hp *9/10 &&
	      nr (1,2) == 1 && 
	      vict != th &&
	      /* Ok, healing curative spells go from 20-28...bad
		 hack. */
	      (spl = find_spell (NULL, nr (20, 28)))  != NULL &&
	      spl->level <= th->level)
	    {
	      cast_spell (th, vict, spl, (nr(1,5) == 2), FALSE, NULL);
	      break;
	    }
	}
      if (vict || !IS_SET (flags, CASTE_WIZARD))
	return;
    }
  
  if (IS_SET (flags, CASTE_WIZARD) && nr (1,12) == 5)
    {
      if (is_hunting (th) && th->fgt->hunting_type == HUNT_KILL && nr (1,2) == 1 &&
	  (vict = find_thing_near (th, th->fgt->hunting, LEVEL (th)/30)) != NULL &&
	  (spl = find_spell (NULL, nr(50,100))) != NULL &&
	  spl->spell_type == TAR_OFFENSIVE && spl->level < LEVEL(th))
	{
	  cast_spell (th, vict, spl, (nr(1,5) == 3), 
		      (th->in == vict->in ? FALSE : TRUE), NULL);
	  return;
	}
    }
  
  
  if (IS_SET (flags, CASTE_WORKER) && 
      !IS_SET (rsoc->society_flags, SOCIETY_NORESOURCES))
    society_worker_activity (th);
  
  if (nr (1,2) == 1 || IS_SET (rsoc->society_flags, SOCIETY_OVERLORD))
    {

      /* Builders slowly build cities, and if they have raw 
	 materials on them, they absorb the raw materials and 
	 add the bonuses to the society. They add minerals at 
	 the mineral rank instead of just +1 since minerals are 
	 fairly useful outside of societies. */
      
      if (IS_SET (flags, CASTE_BUILDER))
	do_citybuild (th, "");
      
      /* Shopkeepers want to get items to sell. They know what
	 they want since if people try to buy something and
	 they don't have it, they decide that they want it. */
      
     
      if (IS_SET (flags, CASTE_SHOPKEEPER))
	society_shopkeeper_activity (th);
      
      
      if (IS_SET (flags, CASTE_CRAFTER))
	society_crafter_activity(th);
    }
  if (!is_hunting (th) && nr (1,3) == 2)
    society_satisfy_needs(th);
  return;
}
/* This is the command that a mob looking for raw materials uses to
   gather those materials. Only members of worker castes do this. */

void
society_gather (THING *th)
{
  THING *obj;
  VALUE *soc;
  int count = 0, room_flags;
  GATHER *gt;
  SOCIETY *society;
  
  if (IS_PC (th) || !CAN_MOVE (th) || is_hunting (th))
    return;
  
  /* Make sure this mob is in a society and in a worker caste. */ 
  if ((soc = FNV (th, VAL_SOCIETY)) == NULL ||
      !IS_SET (soc->val[2], CASTE_WORKER) ||
      (society = find_society_num (soc->val[0])) == NULL ||
      IS_SET (society->society_flags, SOCIETY_NORESOURCES))
    return;
  
  /* Get the type of raw needed. If none is present, pick one. */
  
  if (soc->val[3] <= RAW_NONE || soc->val[3] >= RAW_MAX ||
      (gt = (GATHER *) &gather_data[soc->val[3]]) == NULL ||
      !gt->command[0] ||
      (gt->room_flags && 
       !IS_ROOM_SET (th->in, gt->room_flags)))
    {    
      find_new_gather_type (th);
      return;
    }
  
  /* If this thing has >= 10 raw materials of the type it's looking for,
     goto the dropoff location instead of getting more raw materials. */
  
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      VALUE *raw;
      if ((raw = FNV (obj, VAL_RAW)) != NULL)
	count++;
    }
  
  
  if (count >= SOCIETY_GATHER_COUNT)
    {
      find_dropoff_location (th);
      return;
    }
  
  /* Is that gathering command implemented or not? */
  
  
  /* Check for the proper room flags... if not found, search for a 
     new location. Or once in a while change locations for no reason. */
  
  if (((room_flags = gather_data[soc->val[3]].room_flags) != 0 &&
       !IS_SET (flagbits (th->in->flags, FLAG_ROOM1), room_flags)) ||
      nr (1,50) == 4)
    {
      find_gather_location (th);
      return;
    }  
  
  /* So, at this point the mob can gather, it isn't too overloaded, the
     room is of the proper type..so we do teh gather command. */
  
  
  gather_raw (th, (char *) gather_data[soc->val[3]].command);	      
  return;
}


/* This function updates the society resources if a raw material 
   was given to a builder in a society. It also checks if you give a
   useful item to a society member. */

void
society_item_move (THING *th, THING *item, THING *society_member)
{
  SOCIETY *society;
  VALUE *raw, *socval, *val, *package;
  int reward = 0, raw_power_added;
  RACE *align;
  char buf[STD_LEN];
  /* Do some sanity checking and setup to get all of the
     necessary variables. */

  if (item == NULL || th == NULL || society_member == NULL ||
      !IS_PC (th) || DIFF_ALIGN (th->align, society_member->align) ||
      (socval = FNV (society_member, VAL_SOCIETY)) == NULL ||
      (society = find_society_num (socval->val[0])) == NULL )
    return;
  
  /* Move a raw material. It works if you give the item to anyone
     except a child and a military person. */
  if ((raw = FNV (item, VAL_RAW)) != NULL &&
      IS_SET (socval->val[2], ~(BATTLE_CASTES | CASTE_CHILDREN)) &&
      raw->val[0] >= 0 && raw->val[0] < RAW_MAX)
    {  
      if (raw->val[0] == RAW_MINERAL)
        raw_power_added = (raw->val[1] + 1) * 
	  SOCIETY_RAW_POWER;
      else
	raw_power_added = SOCIETY_RAW_POWER;
      if (IS_SET (society->society_flags, SOCIETY_OVERLORD))
	raw_power_added = raw_power_added * 5/3;
      
      society->raw_curr[raw->val[0]] += raw_power_added;
      
      /* If the society has an overlord, they work better. */
      
      reward = 1;

      /* If the society or align needs this, give more of a reward. */
      
      /* Now give the bonus if any. */
      if (((society->raw_want[raw->val[0]] > 0 ||
	    society->raw_curr[raw->val[0]] <= RAW_TAX_AMOUNT) ||
	   ((align = FALIGN (th)) != NULL &&
	    (align->raw_curr[raw->val[0]] <= RAW_TAX_AMOUNT ||
	     align->raw_want[raw->val[0]] > 0))))
	reward = 8;
      
      add_society_reward (th, item, REWARD_ITEM, reward);
      add_morale (society, reward/2);
      /* This gets freed so builders don't have tens of thousands
	 of objects on them constantly. */
      free_thing (item);
      return;
    }
  
  /* Move a weapon or armor that a society battle member (or other
     nonchild) may need. If you give the item to a nonbattle person, it's
     worth less of a reward. */
  
  if (((val = FNV (item, VAL_WEAPON)) != NULL ||
       (val = FNV (item, VAL_ARMOR)) != NULL) &&
      IS_SET (socval->val[2], ~CASTE_CHILDREN))
    {
      reward = find_item_power (val);
      if (find_free_eq_slot (th, item) == -1)
        reward /= 3;
      if (!IS_SET (socval->val[2], BATTLE_CASTES))
	reward /= 2;
      if (reward < 1)
	reward = 1;
      add_society_reward (th, item, REWARD_ITEM, reward);
      add_morale (society, reward/2);
      return;
    }
  
  /* Now deal with packages. A package delivered to the wrong society
     gives the giver *NOTHING*. The package is destroyed and the
     player sucks. If it's given to the correct society, then the
     player gets a BIG reward given to everyone in the room and
     the society gets a big boost to its raw materials. (3x the amt
     lost at the start). */

  if ((package = FNV (item, VAL_PACKAGE)) != NULL &&
      IS_SET (socval->val[2], ~CASTE_CHILDREN))
    {
      int i;
      /* Oopsie, wrong society! */
      if (society->vnum != package->val[0])
	{
	  do_say (society_member, "Thank you, but I'm not sure what to do with this.");
	  return;
	}
      
      /* Correct society. */
      
      sprintf (buf, "Aaah, %s\x1b[1;37m. We have been in desperate need of this item. Thank you for delivering it, %s.", NAME(item), NAME(th));
      do_say (society_member, buf);
      
      if (package->val[1] < RAW_MAX)
	package->val[1] = RAW_MAX;
      
      /* Give the raw materials bonus to this society. */
      for (i = 0; i < RAW_MAX; i++)
	society->raw_curr[i] += (3*package->val[1])/RAW_MAX;
      
      add_society_reward (th, item, REWARD_PACKAGE, package->val[1]);
      add_morale (society, reward/2);
      item->timer = 1;
      package->val[0] = 0;
      package->val[1] = 0;
    }
  return;
}





/* Used for a society member to find a place to gather more raw materials. 
   It searches all rooms from say 5 to 40 rooms away and picks a random one? */

void
find_gather_location (THING *th)
{
  THING *room, *nroom;
  VALUE *socval = NULL, *build;
  int flags;
  SOCIETY *soc = NULL;
  int choice = 0, i;
  /* As we search the rooms, we put the first N we find into a list an
     array and pick from them. */
  
  THING *room_choices[RAW_GATHER_ROOM_CHOICES];
  BFS *bfs_choices[RAW_GATHER_ROOM_CHOICES];
  int num_room_choices = 0;
  
  if (!th ||  (room = th->in) == NULL || is_hunting (th))
    return;
  
  /* Only workers gather stuff. Also, if the society is currently on
     alert or fighting, then these things don't go looking for
     more stuff to gather. */
  
  if ((socval = FNV (th, VAL_SOCIETY)) == NULL ||
      !IS_SET (socval->val[2], CASTE_WORKER) ||
      (soc = (find_society_num (socval->val[0]))) == NULL ||
      IS_SET (soc->society_flags, SOCIETY_NORESOURCES))
    return;
  
  /* Dangerous..possible error here. */
  if (socval->val[3] <= RAW_NONE || socval->val[3] >= RAW_MAX ||
      !gather_data[socval->val[3]].command[0])
    socval->val[3] = RAW_MINERAL;
  
  /* Get the flags */

  flags = gather_data[socval->val[3]].room_flags;

  /* Clear the data... */

  for (i = 0; i < RAW_GATHER_ROOM_CHOICES; i++)
    {
      room_choices[i] = NULL;
      bfs_choices[i] = NULL;
    }

  /* Do a bfs search out. Stop if we get out past RAW_GATHER_DEPTH or
     if we find at least RAW_GATHER_ROOM_CHOICES rooms to pick from
     or if we get out past RAW_GATHER_DEPTH/3 and find ANY
     rooms that satisfy our needs. Note that if the gathering doesn't
     need specific room flags, then we move a little farther to find
     a room that works. */


  clear_bfs_list ();
  undo_marked (room);
  add_bfs (NULL, room, REALDIR_MAX);
  
  while (bfs_curr)
    {
      if ((room = bfs_curr->room) != NULL && IS_ROOM (room))
	{
	  if (num_room_choices < RAW_GATHER_ROOM_CHOICES &&
	      ((flags && IS_ROOM_SET (room, flags)) ||
	       (!flags && nr (1,2) == 2)) &&
	      (build = FNV (room, VAL_BUILD)) == NULL)
	    {
	      room_choices[num_room_choices] = room;
	      bfs_choices[num_room_choices] = bfs_curr;
	      num_room_choices++;
	    }
	  /* Stop if we get past the max depth allowed, or if we
	     run out of choices, or if we get past 1/3 the max
	     depth and we have ANY choices. */
	  
	  if (bfs_curr->depth > RAW_GATHER_DEPTH ||
	      num_room_choices >= RAW_GATHER_ROOM_CHOICES ||
	      (bfs_curr->depth >= RAW_GATHER_DEPTH/10 &&
	       num_room_choices > 0))
	    break;
	  
	  for (i = 0; i < REALDIR_MAX; i++)
	    {
	      
	      if ((nroom = FTR (room, i, th->move_flags)) != NULL &&
		  bfs_curr->depth  < RAW_GATHER_DEPTH)
		add_bfs (bfs_curr, nroom, i);	     
	    }
	}
      bfs_curr = bfs_curr->next;
    }
  
  if (num_room_choices < 1 || num_room_choices > RAW_GATHER_ROOM_CHOICES)
    {
      clear_bfs_list();
      return;
    }
  
  choice = nr (1, num_room_choices) - 1;
  
  if (choice >= 0 && choice < RAW_GATHER_ROOM_CHOICES)
    {
      bfs_curr = bfs_choices[choice];
      place_backtracks (room_choices[choice], 3);
      start_hunting_room (th, room_choices[choice]->vnum, HUNT_GATHER);
    }
  /* Now find the number of "acceptable rooms" First check those 
     nearby. */
  
  clear_bfs_list();
  
  return;
}

/* Used when a society member is "full" of raw materials to go look for
   a dropoff location. The dropoff is the closest mob in the same
   society that is in a "builder" caste. */

void
find_dropoff_location (THING *th)
{
  THING *builder;
  VALUE *soc;
  SOCIETY *society;
  int num_builder;
  if (!th ||  th->in == NULL || is_hunting (th) ||
      (soc = FNV (th, VAL_SOCIETY)) == NULL || 
      !IS_SET (soc->val[2], CASTE_WORKER) ||
      (society = find_society_num (soc->val[0])) == NULL)
    return;
  if ((num_builder = find_num_members (society, CASTE_BUILDER)) < 1 ||
      (builder = find_society_member_nearby (th, CASTE_BUILDER, MAX_SHORT_HUNT_DEPTH/2 + (nr(1,10) == 2 ? (MAX_SHORT_HUNT_DEPTH/2) : 0))) == NULL)
    {
      stop_hunting (th, TRUE);
    }
  else
    { 
      start_hunting (th, KEY (builder), HUNT_DROPOFF);
      if (th->fgt)
	th->fgt->hunt_victim = builder;
    }
  return;
  
}
  



void
society_worker_activity (THING *th)
{ 
  THING *builder, *obj,    *objn;
  VALUE *soc, *raw, *soc2;
  int raw_count = 0;
  SOCIETY *rsoc;
  
  if (!th || !th->in || (soc = FNV (th, VAL_SOCIETY)) == NULL ||
      (rsoc = find_society_num (soc->val[0])) == NULL)
    return;
  
  

  /* Check if we're in a "gather room" or a "Dropoff room" */
  /* Find the number of raw materials we have on us. */
  
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if ((raw = FNV (obj, VAL_RAW)) != NULL)
	raw_count++;
    }
  
  if (dropoff_raws_for_needs (th))
    return;
  
  /* If a "builder" is here, we can drop off resources. This
     might get tweaked a bit later on to have them drop off
     at the "correct" kind of builder or mine...but that's a 
     bit far off still. */
  
  for (builder = th->in->cont; builder; builder = builder->next_cont)
    {
      if ((soc2 = FNV (builder, VAL_SOCIETY)) != NULL &&
	  soc2->val[0] == soc->val[0] &&
	  IS_SET (soc2->val[2], CASTE_BUILDER) &&
	  builder->position > POSITION_SLEEPING)
	break;
    }
  
  
  /* If the builder's there, see if we have anything to drop off. */
  
  if (builder)
    {
      if (raw_count > 0)
	{ /* Update society raw status here !!!!! for now just
	     delete the item and ignore any overall society effect. */
	  
	  for (obj = th->cont; obj; obj = objn)
	    {
	      objn = obj->next_cont;
	      if ((raw = FNV (obj, VAL_RAW)) != NULL &&
		  move_thing (th, obj, th, builder))
		{
		  act ("@1n give@s @2n to @3n.", th, obj, builder, NULL, TO_ALL);	      
		  if (raw->val[0] == RAW_MINERAL)
		    rsoc->raw_curr[raw->val[0]] += (raw->val[1] + 1); 
		  else if (raw->val[0] >= 0 && raw->val[0] < RAW_MAX)
		    rsoc->raw_curr[raw->val[0]]++;	      	      
		  free_thing (obj);
		  break;
		}
	    }
	}
      else
	{  /* Make sure the society gather type is ok! */
	  
	  /* If we're done dropping off and we need to find a new
	     thing to gather, try to help out with other needs. */
	  if (nr (1,5) != 4 || !worker_generate_need (th))	    
	    find_new_gather_type (th);
	  
	}
      return;
    }
  else if (raw_count < SOCIETY_GATHER_COUNT)
    {
      society_gather (th);
    }
  else
    {
      find_dropoff_location (th);
    }
  return;
}

void
find_new_gather_type (THING *th)
{
  SOCIETY *rsoc;
  VALUE *soc;
  int flags, needed;
  
  
  /* Make sure this thing exists and its in a society and its caste does
     something. */
  
  if ((rsoc = find_society_in (th)) == NULL ||
      is_hunting (th) ||
      IS_SET (rsoc->society_flags, SOCIETY_NORESOURCES) ||
      (soc = FNV (th, VAL_SOCIETY)) == NULL ||
      !IS_SET ((flags = soc->val[2]), CASTE_WORKER))
    return;

  
  /* should really be a check vs what the society needs. */
  
  if ((needed = raw_needed_by_society (rsoc)) == RAW_MAX)
    soc->val[3] = nr (RAW_NONE + 1, RAW_MAX - 1);
  else
    soc->val[3] = needed;
  
  if (!gather_data[soc->val[3]].command[0])
    {
      soc->val[3] = RAW_MINERAL;
      if (!gather_data[RAW_MINERAL].command[0])
	{
	  log_it ("No mine command!\n");
	  return;
	}
    }

  find_gather_location (th);
  return;
}

/* This finds the raw materials needed by the society. First, it
   checks if the society has any specific pressing needs for raw
   materials as determined by what upgrades they want but cannot
   afford. If the society has enough for its current upgrades, then 
   it will try to balance all of its raw materials by putting people
   to work searching for raw materials for which the society has
   a smaller reserve. */

int
raw_needed_by_society (SOCIETY *soc)
{
  int i, total = 0, chose, max_reserve = 0, min_reserve = 1000000;

  if (!soc)
    return RAW_MAX;
  
  for (i = 1; i < RAW_MAX; i++)
    {
      total += soc->raw_want[i];
    }
  if (total > 0)
    {
      chose = nr (1, total);
  
      for (i = 1; i < RAW_MAX; i++)
	{
	  chose -= soc->raw_want[i];
	  if (chose < 1)	
	    break;
	}
      return i;
    }  
  else /* Have enough...fill up the bottom ranks first. */
    {
      for (i = 1; i < RAW_MAX; i++)
	{ /* Could weight this to require 2x food or something. */
	  if (/* wgt * */soc->raw_curr[i] >= max_reserve)
	    max_reserve = soc->raw_curr[i];
	  if (/* wgt * */soc->raw_curr[i] <= min_reserve)
	    min_reserve = soc->raw_curr[i];
	}
      
      /* If all equal, just pick anything. */
      
      if (max_reserve <= min_reserve)
	return RAW_MAX;
      
      /* Otherwise add up how much is missing from the smaller 
	 reserves. */

      total = 0;
      for (i = 1; i < RAW_MAX; i++)
	{/* Could weight this to require 2x food or something. */
	  total += max_reserve - /* wgt * */soc->raw_curr[i];
	}
      
      if (total == 0)
	return RAW_MAX;
      
      /* Now pick something to build up. */

      chose = nr (1, total);
      
      /* Now find which thing we build up. */

      for (i = 1; i < RAW_MAX; i++)
	{/* Could weight this to require 2x food or something. */
	  chose -= (max_reserve - /* wgt  * */soc->raw_curr[i]);
	  if (chose < 1)
	    return i;
	}
    }
	  
  return RAW_MAX;
}

  


/* This finds a random caste based on using the percents for the
   1st tier in each caste to determine how likely this caste is
   chosen. It gets used for increasing the max pop of the
   society and for determining what children do once they've grown
   up. */

int
find_caste_from_percent (SOCIETY *soc, bool include_children, bool warriors_only)
{
  int i, total_pct = 0, num_chose = 0, count;
  
  if (!soc)
    return -1;
  
  /* Get the total of the percents */
  
  for (count = 0; count < 2; count++)
    {
      for (i = (include_children ? 0 : 1); i < CASTE_MAX; i++)
	{	  
	  if (soc->start[i] != 0 &&
	      (!warriors_only || IS_SET (soc->cflags[i], BATTLE_CASTES)))
	    {
	      if (count == 0)
		total_pct += soc->chance[i];
	      else
		{
		  num_chose -= soc->chance[i];
		  if (num_chose <= 0)
		    break;
		}
	    }
	}
      if (count == 0)
	{
	  if (total_pct > 0)
	    num_chose = nr (1, total_pct);
	  else
	    break;
	}
    }
  
  
  
  /* Make sure the caste we picked exists! */
  
  if (i >= CASTE_MAX || soc->start[i] == 0)
    return -1;
  
  return i;
  
}






/* This increases the max pop for some caste in the population. The
   way the caste is chosen is by looking at the percent chances
   you have to enter that caste when you grow out of being a child. */
   

void
increase_max_pop (SOCIETY *soc)
{
  int cnum;
  int worker_max_pop = 0; /* Total max pop of worker castes. */
  int num_worker_castes = 0;  /* Number of worker castes. */
  int max_pop = 0; /* Max pop of whole society. */
  int times, max_times; /* Do this increase up to 4 times per use. */
  if (!soc)
    return;
  
  /* If the workers castes (< 1/3 total_max_pop) or the children's
     caste is too small (< 1/6 total_max_pop) increase them first. 
     Do this because the society must keep its pool of new members
     increasing, and it must keep its production (economy) up. */
  
  
  
  max_pop = society_max_pop (soc);

  
  max_times = nr (SOCIETY_MAXPOP_INCREASE_TRIES/2,
		  SOCIETY_MAXPOP_INCREASE_TRIES);
  
  for (times = 0; times < max_times; times++)
    {
      if (soc->max_pop[0] < max_pop/6 && soc->max_pop[0] < 20)
	soc->max_pop[0]++;      
      /* Check if workers castes are large enough. */
      else
	{
	  for (cnum = 0; cnum < CASTE_MAX; cnum++)
	    {
	      if (IS_SET (soc->cflags[cnum], CASTE_WORKER))
		{
		  worker_max_pop += soc->max_pop[cnum];
		  num_worker_castes++;
		}
	    }	  
	  /* If no worker castes, don't try to increase a worker caste. */
	  
	  if (worker_max_pop < max_pop/4 && num_worker_castes > 0 )
	    {
	      int num_chose = 0, choice = 0;
	      num_chose = nr (1, num_worker_castes);
	      for (cnum = 0; cnum < CASTE_MAX; cnum++)
		{
		  if (IS_SET (soc->cflags[cnum], CASTE_WORKER) &&
		      ++choice == num_chose)
		    break;
		}
	      if (cnum < CASTE_MAX)
		soc->max_pop[cnum] += SOCIETY_MAX_POP_INCREASE;
	    }
	  /* Otherwise just pick something at random to increase.
	   Make sure that this caste isn't too big relative to the
	   size of the current population. */	  
	  else if ((cnum = find_caste_from_percent (soc, TRUE, FALSE)) >= 0 &&
		   cnum < CASTE_MAX &&
		   soc->max_pop[cnum] < (soc->curr_pop[cnum]+1)*10)
	    soc->max_pop[cnum] += SOCIETY_MAX_POP_INCREASE;
	}
    }
  return;
}



/* This checks if the society has enough resources to do some kind of
   improvement on itself or not. */

/* This has now been modified. A society cannot run out of enough
   resources to build more workers or more warriors. If the
   society does not have at least 1 worker and 1 builder and the
   type of upgrade is to make a new member, and if there are no
   more members of the "children's caste", then the society gets
   freebie resources to make a new member. */


bool
has_resources (SOCIETY *soc, int val)
{
  int i, type, builder_pop = 0, worker_pop = 0;
  bool has_enough = TRUE;
  if (!soc)
    return FALSE;
  
  /* Find the correct build type from the list. */
  
  
  for (type = 0; type < BUILD_MAX; type++)
    {
      if (build_data[type].val == val)
	break;
    }
  
  /* Build type not found. */

  if ((type = find_build_type(val)) == BUILD_MAX)
    return FALSE;

  
  /* Now see if the society has enough resources based on the
     build list of resources. */
  
  for (i = 1; i < RAW_MAX; i++)
    {
      if (find_build_cost (soc, type, i) > soc->raw_curr[i])
	has_enough = FALSE;
    }
  
  /* If this is for building a new member and we don't have
     enough resources and the children's caste doesn't have
     anyone left in it, then give the society the resources to
     make a new member. This is to avoid societies just
     dying off if too many things die in them. */

  if (!has_enough && val == BUILD_MEMBER)
    {
      for (i = 0; i < CASTE_MAX; i++)
	{
	  if (soc->start[i] == 0)
	    continue;
	  
	  if (IS_SET (soc->cflags[i], CASTE_WORKER))
	    worker_pop += soc->curr_pop[i];
	  
	  if (IS_SET (soc->cflags[i], CASTE_BUILDER))
	    builder_pop += soc->curr_pop[i];
	}
      
      /* If we have few workers or few builders, then give the 
	 society the ability to make new members anyway. */      
      if (worker_pop < 15 || builder_pop < 4)
	has_enough = TRUE; /* Can build now. */
    }
  return has_enough;
}

/* This subtracts the building costs from the society based on the
   type of building being done. */

bool
subtract_build_cost (SOCIETY *soc, int val)
{
  int i, type;
  
  if (!soc)
    return FALSE;
  
  /* Find the correct build type from the list. */
  
  if ((type = find_build_type (val)) == BUILD_MAX)
    return FALSE;
  
  
  /* Now see if the society has enough resources based on the
     building list of resources. */
  
  /* Assume this works here since we will call this only after a 
     check to see if the society has enough raw materials on hand. */
  
  for (i = 0; i < RAW_MAX; i++)
    soc->raw_curr[i] -= MIN (soc->raw_curr[i], find_build_cost (soc, type, i));

  return TRUE;
}


/* This finds the kind of building wanted based on the flag set. */

int
find_build_type (int val)
{
  int type;
  
  for (type = 0; type < BUILD_MAX; type++)
    if (build_data[type].val == val)
      break;
  return type;
}

/* This gives the raw material cost for a society to build
   something. It increases as the society gets bigger. It used to
   have huge jumps at the SOCIETY_COST_MULTIPLIER points, but
   now it's more linear. */

int
find_build_cost (SOCIETY *soc, int build_type, int raw_type)
{
  if (!soc || soc->population < 0 ||
      build_type < 0 || build_type >= BUILD_MAX ||
      raw_type <= RAW_NONE || raw_type >= RAW_MAX)
    return 0;
  
  return (build_data[build_type].cost[raw_type] *
	  (SOCIETY_COST_MULTIPLIER + soc->population))/SOCIETY_COST_MULTIPLIER;
}


/* This updates the kinds of raw materials the society wants based
   on what goals it wants to carry out now. */

void
update_raws_wanted (SOCIETY *soc)
{
  int i, build_type;
  
  if (!soc)
    return;
  
  /* Clear costs. */
  
  
  for (i = 0; i < RAW_MAX; i++)
    soc->raw_want[i] = 0;
  
  /* Add costs for everything we want to do. */
  
  for (build_type = 0; build_type < BUILD_MAX; build_type++)
    {
      if (IS_SET (soc->goals, build_data[build_type].val))
	{
	  for (i = 1; i < RAW_MAX; i++)
	    {
	      soc->raw_want[i] += find_build_cost (soc, build_type, i);
	    }
	}
    }
  
  for (i = 1; i < RAW_MAX; i++)
    soc->raw_want[i] -= MIN (soc->raw_want[i], soc->raw_curr[i]);
  
  return;
}

bool
make_new_member (SOCIETY *soc)
{
  THING *room, *proto, *newmem; 
  /* Get N tries at making new members. */ 
  int i, num_times = SOCIETY_NEW_MEMBER_TIMES;
  bool built_member = FALSE;
  
  if (!soc)
    return FALSE;

  /* The number of times we do this is dependent on how badly this
     society is losing. If it's losing, we attempt to make more
     members to compensate. */
  if (soc->align > 0 &&
      align_info[soc->align] &&
      align_info[soc->align]->ave_pct <= 30)
    num_times++;
  
  if (IS_SET (soc->society_flags, SOCIETY_FASTGROW))
    num_times *= 2;
      

  /* Make a new society member. Check in caste 1, check for pct, 
     check for population room, check for prototype, check for room to
     goto, and check if the thing is made. */
  
  if ((proto = find_thing_num (soc->start[0])) == NULL ||
      !CAN_FIGHT (proto) || !CAN_MOVE (proto) || 
      soc->start[0] == 0)
    return FALSE;

  
  for (i = 0; i < num_times; i++)
    {     
      if (nr (1,10) <= soc->chance[0] &&
	  soc->curr_pop[0] < soc->max_pop[0] &&
	  (proto->max_mv == 0 ||
	   proto->mv < proto->max_mv) &&
	  ((room = find_thing_num (nr (soc->room_start, soc->room_end))) != NULL) &&
	  IS_ROOM (room) && !IS_AREA (room) &&
	  !IS_ROOM_SET (room, BADROOM_BITS & ~flagbits(soc->flags, FLAG_ROOM1)) &&
	  /* This is to make powerful mobs pop more slowly. Like crap mobs
	     like elves pop fast, but dragons pop slow. If the level of
	     the starting mob is really high, then there is only a small
	     chance of a new mob being created. */
	  nr (1,LEVEL(proto)) < 10 &&
	  ((newmem = create_thing (soc->start[0])) != NULL))
	{
	  set_new_member_stats (newmem, soc);
	  thing_to (newmem, room);
	  built_member = TRUE;
	}
    }
  return built_member;
}

/* This sets up the stats for a new member of a society. */

void
set_new_member_stats (THING *newmem, SOCIETY *soc)
{
  VALUE *socval;
  FLAG *flag;
  if (!newmem || !soc)
    return;
  
  newmem->align = soc->align;
  if ((socval = FNV (newmem, VAL_SOCIETY)) == NULL)
    {
      socval = new_value ();
      socval->type = VAL_SOCIETY;
      add_value (newmem, socval);
    }
  socval->val[0] = soc->vnum;
  socval->val[1] = 0;
  socval->val[2] = soc->cflags[0];
  
  /* Set each mob to a specific lifespan. */
  if (soc->lifespan > 0)
    {
      newmem->timer = MAX (1, nr (soc->lifespan*4/5, soc->lifespan*6/5));
    }
  if (!IS_SET (soc->society_flags, SOCIETY_NONAMES))
    {
      create_society_name (socval);
      setup_society_name (newmem);
    }
  add_flagval (newmem, FLAG_DET, AFF_DARK);
  for (flag = soc->flags; flag; flag = flag->next)
    add_flagval (newmem, flag->type, flag->val);
  update_society_population_thing (newmem, TRUE);
  return;
}
/* This carries out the actual upgrading process for societies and
   checks for raw materials and subtracts them. */


void
carry_out_upgrades (SOCIETY *soc)
{
  
  if (!soc)
    return;

  /* Add a new member to the society. */
  
  if (IS_SET (soc->goals, BUILD_MEMBER) &&
      has_resources (soc, BUILD_MEMBER))
    {
      if (make_new_member (soc))
	{
	  subtract_build_cost (soc, BUILD_MEMBER);
	  RBIT (soc->goals, BUILD_MEMBER);
	}
    }
  
  /* Add a new tier onto a caste in the society. */
  
  if (IS_SET (soc->goals, BUILD_TIER) &&
      has_resources (soc, BUILD_TIER))
    {
      if (increase_tier (soc))
	{
	  subtract_build_cost (soc, BUILD_TIER);
	  RBIT (soc->goals, BUILD_TIER);
	}
    }
  
  
 
 /* Add 1 to maxpop for the society. */


  if (IS_SET (soc->goals,  BUILD_MAXPOP) &&
      has_resources (soc, BUILD_MAXPOP))
    {
      increase_max_pop (soc);
      subtract_build_cost (soc, BUILD_MAXPOP);
      RBIT (soc->goals, BUILD_MAXPOP);
    }

 
  
   /* Add to the overall "quality" of the society...for combat? */
   
   if (IS_SET (soc->goals, BUILD_QUALITY) &&
       has_resources (soc, BUILD_QUALITY))
     {
       soc->quality++;
       subtract_build_cost (soc, BUILD_QUALITY);
       RBIT (soc->goals, BUILD_QUALITY);
     }
 
   /* Add to chance of warriors or nonwarriors appearing based on how
      the population is stacking up. */
   
   
   if (IS_SET (soc->goals, BUILD_WARRIOR))
     {
       increase_caste_chances(soc, TRUE);
       subtract_build_cost(soc, BUILD_WARRIOR);
       RBIT (soc->goals, BUILD_WARRIOR);
     }
   
   if (IS_SET (soc->goals, BUILD_WORKER))
     {
       increase_caste_chances(soc, FALSE);
       subtract_build_cost(soc, BUILD_WORKER);
       RBIT (soc->goals, BUILD_WORKER);
     }
   


      
   
  return;
}
  
void 
weaken_society (SOCIETY *soc)
{
  int curr_pop, max_pop, i, chance;
  
  
  if (!soc)
    return;
  
  /* Find the current and max pop for the society. */
  
  curr_pop = society_curr_pop (soc);
  max_pop = society_max_pop (soc);
  
  
  
  if (soc->quality > 0)
    {
      /* Make sure the max_pop is at least 20x the quality. */

      if (max_pop < 10)
	{
	  if (nr (1,15) > max_pop)
	    soc->quality--;
	}
      else
	{
	  /* Need to check quality/(level/4) > curr/pop_cap, but since
	     divides suck, do this as a product. */
	  if (soc->quality *soc->population_cap > curr_pop*soc->level/10 &&
	      nr (1,5) == 3)
	    {
	      soc->quality--;
	    }  
	}
    }
  
  /* Reduce castes where the curr_pop is less than the max_pop */
  /* Do not do this to castes where the current pop is at least 4/5
     the max, or where the current pop is within 2 of the max. This
     will keep small castes from getting obliterated. */
  
  for (i = 0; i < CASTE_MAX; i++)
    {
      /* Make the shops and crafters castes big enough. */
      if (IS_SET (soc->cflags[i], CASTE_SHOPKEEPER))
	soc->max_pop[i] = MAX (soc->max_pop[i], SOCIETY_SHOP_MAX);
      
      if (IS_SET (soc->cflags[i], CASTE_CRAFTER))
	soc->max_pop[i] = MAX (soc->max_pop[i], PROCESS_MAX);
      
      if (soc->start[i] == 0 || 
	  (soc->max_pop[i] > 0 &&
	   (soc->curr_pop[i] >= soc->max_pop[i] * 4/5 ||
	    soc->max_pop[i] - soc->curr_pop[i] < 5)))
	continue;
      
      /* The chance is essentially the percent the caste is empty 
	 expressed as a number from 1 to 10. */
      
      if (soc->max_pop[i] < 1)
	soc->max_pop[i] = 1;

      chance = MIN ((soc->max_pop[i] - soc->curr_pop[i]) *10/soc->max_pop[i], 8);
      
      /* Make it very hard to let the children's caste and worker's caste
	 go down below ?7? people? */

      if (nr (1,10) <= chance && 
	  (soc->max_pop[i] > 10 ||
	   nr (1,25) == 3))
	soc->max_pop[i]--;
      
      /* Don't let 0'th caste get too small. */
      /* For societies that gather raws, make the worker caste stay
	 at least 10 big. For the societies that don't gather raws,
	 make the warrior castes big enough. */
      
      
      if ((i == 0  || IS_SET (soc->cflags[i], CASTE_WORKER)) &&
	  soc->max_pop[i] < 15)
	soc->max_pop[i] = 15;

      if (IS_SET (soc->cflags[i], CASTE_WARRIOR))
	soc->max_pop[i] = MAX (10, soc->max_pop[i]);
      
      if (IS_SET (soc->cflags[i], (BATTLE_CASTES &~CASTE_WARRIOR)))
	soc->max_pop[i] = MAX (3, soc->max_pop[i]);
      
    }
  
  /* Now lower the curr tiers for the society if the caste pop
     isn't large enough. */
  
  for (i = 1 /* <- DONT DESTROY CHILDREN CASTE! */; i < CASTE_MAX; i++)
    {
      /* Don't drop curr_tiers down to 0. */
      
      if (soc->start[i] == 0 || soc->max_tier[i] <= 1 ||
	  soc->curr_tier[i] <= 1)
	continue;
      
      if (soc->max_pop[i] < 10 * soc->curr_tier[i] &&
	  soc->curr_pop[i] < soc->max_pop[i]/3)
	{
	  /* The chance is obtained by taking the max pop we need for this
	     level of current tier and subtracting off the max pop then
	     multiplying by 5 to get how bad the situation is. */
	  chance = (10 * soc->curr_tier[i] - soc->max_pop[i]) * 5/soc->curr_tier[i];
	  if (nr (1,10) <= chance)
	    soc->curr_tier[i]--; 
	  
	}
    }
  
  return;
}

	      
void
update_society_goals (SOCIETY *soc)
{
  if (!soc)
    return;

  /* First, check to see if the society changes alignment. This can
     happen if the society is attacked too many times by things on 
     its own side, or by too many attacks from another side. If
     this happens, the society changes alignment. */
  
  check_society_change_align (soc);
  
  /* See if the society is weakening. */
  
  weaken_society (soc); 

  /* Figure out what we want to upgrade. */

  find_upgrades_wanted (soc);

  
  /* Do any upgrades we can */
  
  carry_out_upgrades (soc);
  
  
  /* Update the chances for things being "born" based on how far we
     have strayed from the base chances. */
  
  update_caste_chances(soc);

  /* Find what raw materials are missing for future upgrade use. */
  
  update_raws_wanted (soc);


  
  return;
}

/* This function determines what things the society wants to do
   based on its current status. */

void
find_upgrades_wanted (SOCIETY *soc)
{
  int max_pop, curr_pop, chance, 
    warrior_percent = (WARRIOR_PERCENT_MIN + WARRIOR_PERCENT_MAX)/2;
  RACE *align;
  int start_member_level;
  THING *start_member;
  
  /* If no society, or if the society is getting too big, then
     stop letting it update its goals. That way, larger societies 
     don't spiral out of control. They just keep getting more 
     resources to give to the alignment. And they keep raiding. */
  
  
  if (!soc)
    return;
  
  max_pop = society_max_pop (soc);
  curr_pop = society_curr_pop (soc);
  
  
  /* Build new members...more likely if the population is lower. */
  
  /* Find the effective max pop for this society. */

  if ((start_member = find_thing_num (soc->start[0])) == NULL)
    start_member_level = POP_LEVEL_MULTIPLIER;
  else
    start_member_level = MAX (5, LEVEL(start_member));
  
  soc->population_cap = (POP_BEFORE_SETTLE*POP_LEVEL_MULTIPLIER)/(start_member_level);
  if (soc->population_cap < POP_BEFORE_SETTLE/2)
    soc->population_cap = POP_BEFORE_SETTLE/2;


  /* Check to build new members...do this more if the population is
     small. */

  if (soc->population < soc->population_cap && 
      (nr (1, soc->population_cap*3) > soc->population ||
       nr (1,30) == 15)) /* small chance to increase pop at any level. */
    {
      if (max_pop < 30)
	{
	  if (nr (1,100) > max_pop)
	    soc->max_pop[0]++; 
	  soc->goals |= BUILD_MEMBER;
	}      
      else 
	{
	  chance = MID (1,  (max_pop - curr_pop) * 10/max_pop, 7);
	  if (nr (1,10) <= chance)
	    soc->goals |= BUILD_MEMBER;
	}
    }
  
  
  /* Upgrade quality when the society is large enough compared to
     its max pop. */
  
  if (soc->quality < soc->level/4 &&
      curr_pop >= max_pop*7/10 &&
      nr (1,10) == 6)
    soc->goals |= BUILD_QUALITY;
  
  /* Upgrade max_pop when the curr_pop is too big relative to max_pop. */


  if (curr_pop > max_pop * 2/3 && 
      nr (1,12) == 3)
    {
      if (curr_pop > max_pop *9/10)
	soc->goals |= BUILD_MAXPOP;
      else if (nr (1,30) <= (curr_pop)*100/max_pop- 60)
	soc->goals |= BUILD_MAXPOP;
    }

  /* Now see if the society wants to increase a tier in some caste. 
     This is like getting technology since it lets the society gain
     more powerful units. */
  
  if (curr_pop > max_pop*2/3)
    {
      if (nr (1,5) == 3)
	soc->goals |= BUILD_TIER;
    }
  
  
  if (max_pop > 0)
    warrior_percent = find_num_members (soc, BATTLE_CASTES)*100/max_pop;
  
  /* Find if we change our desired warrior percent based on what
     the alignment needs. */
  
  if (soc->align >= 0 && soc->align < ALIGN_MAX &&
      (align = align_info[soc->align]) != NULL)
    {
      if (soc->align == 0)
	{
	  if (soc->warrior_percent <
	      (WARRIOR_PERCENT_MIN + WARRIOR_PERCENT_MAX)/2)
	    soc->warrior_percent++;
	  if (soc->warrior_percent >
	      (WARRIOR_PERCENT_MIN + WARRIOR_PERCENT_MAX)/2)
	    soc->warrior_percent--;
	}
      else
	{
	  if (soc->warrior_percent < align->warrior_goal_pct)
	    soc->warrior_percent++;
	  else if (soc->warrior_percent > align->warrior_goal_pct)
	    soc->warrior_percent--;
	}
      if (soc->warrior_percent < WARRIOR_PERCENT_MIN)
	soc->warrior_percent = WARRIOR_PERCENT_MIN;
      if (soc->warrior_percent > WARRIOR_PERCENT_MAX)
	soc->warrior_percent = WARRIOR_PERCENT_MAX;
    }
  
  
  /* Current warrior percent is too high...increase other percents. */
  
  if (warrior_percent - soc->warrior_percent >= 20 &&
      has_resources (soc, BUILD_WORKER))
    soc->goals |= BUILD_WORKER;
  
  
  /* Current nonwarrior percent is too high, increase warriors. */
  
  if (soc->warrior_percent - warrior_percent >= 20 &&
      has_resources (soc, BUILD_WARRIOR))
    soc->goals |= BUILD_WARRIOR;
      
  return;
}

/* This determines if a society member can find a society member in
   a certain kind of caste within a certai depth. If so, it returns it
   and it starts hunting it. */


THING *
find_society_member_nearby (THING *th, int caste_flags, int max_depth)
{
  THING *vict = NULL, *room, *nroom;
  VALUE *soc, *soc2, *exit;
  int dir;
  int th_move_flags;
  /* Sanity checks. */

  if (!th || !th->in || !IS_ROOM ((room = th->in)) || !caste_flags ||
      (soc = FNV (th, VAL_SOCIETY)) == NULL || 
      IS_SET (soc->val[2], caste_flags))
    return NULL;
  
  if (max_depth < 1 || max_depth >= MAX_SHORT_HUNT_DEPTH)
    max_depth = MAX_SHORT_HUNT_DEPTH;
  th_move_flags = th->move_flags;
  
  
  undo_marked (room);
  clear_bfs_list();
  add_bfs (NULL, room, DIR_MAX);
 
  while (bfs_curr && (room = bfs_curr->room) != NULL)
    {    
      for (vict = room->cont; vict; vict = vict->next_cont)
	{
	  if (CAN_MOVE (vict) && 
	      (soc2 = FNV (vict, VAL_SOCIETY)) != NULL &&
	      soc2->val[0] == soc->val[0] &&
	      IS_SET (soc2->val[2], caste_flags) &&
	      vict != th)
	    break;
	}
      
      if (vict)
	break;
      else if (!vict && bfs_curr->depth < max_depth)
	{
	  for (exit = room->values; exit; exit = exit->next)
	    {
	      if ((dir = exit->type-1) < REALDIR_MAX &&
		  (nroom = FTR (room, dir, th_move_flags)) != NULL)
		add_bfs (bfs_curr, nroom, dir);
	    }
	}
      bfs_curr = bfs_curr->next;
    }
  clear_bfs_list ();
  return vict;
}

/* Attempts to raise the max tier a society has in a certain
   area. It works when the caste in question becomes 
   populated enough (relative to its expected size) 
   to allow for a tier increase. */

bool
increase_tier (SOCIETY *soc)
{
  int pass, choice = 0, i;
  int sum_advance_chances = 0;
  int caste_pop_needed;
  
  if (!soc)
    return FALSE;
  
  for (i = 0; i < CASTE_MAX; i++)
    sum_advance_chances += soc->chance[i];
  
  if (sum_advance_chances < 1)
    sum_advance_chances = 1;
  
  for (pass = 0; pass < 2; pass++)
    {
      for (i = 0; i < CASTE_MAX; i++)
	{
	  caste_pop_needed = soc->population_cap*soc->chance[i]/(sum_advance_chances*3);
	  if (caste_pop_needed < 2*(soc->curr_tier[i]+1))
	    caste_pop_needed = 2*(soc->curr_tier[i]+1);
	  if (soc->start[i] > 0 &&
	      soc->max_tier[i] > 0 &&
	      soc->curr_pop[i] >= caste_pop_needed &&
	      soc->curr_tier[i] < soc->max_tier[i])
	    {
	      if (pass == 0)
		choice++;
	      else if (--choice == 0)
		break;
	    }
	}
      
      if (pass == 0)
	{
	  if (choice == 0)
	    return FALSE;
	  choice = nr (1, choice);
	}
    }
  
  /* Failed to find the proper caste... */

  if (i == CASTE_MAX)
    return FALSE;
  
  /* Or else we did find a caste to upgrade, and we do it. */
  
  soc->curr_tier[i]++;
  return TRUE;
}


/* This checks if the society will change alignments due to being
   beaten up too badly. */

void
check_society_change_align (SOCIETY *soc)
{
  int max_killed_by = 0, max_killed_by_align = -1, i;
  int second_killed_by = 0, second_killed_by_align = -1;
  char buf[STD_LEN];
  THING *area;

  if (!soc || IS_SET (soc->society_flags, SOCIETY_FIXED_ALIGN | SOCIETY_OVERLORD) ||
      soc->align < 0 || soc->align >= ALIGN_MAX)
    return;
  
  /* A society checks how it's being treated by its own align
     and by other aligns and may switch to them if it's being
     treated badly. */
  
  /* You can usually only make a society split from an alignment 
     if that alignment is losing (less than 85 pct of the power it 
     "should" have. This means you have to have a broad war on many
     fronts and really beat the crap out of the alignment all over
     before any places will switch sides. */
  
  if (align_info[soc->align] &&
      soc->align > 0 &&
      align_info[soc->align]->ave_pct > 85 && 
      nr (1,5) != 2)
    return;
  
  /* Now check for some switching. We check if a society has been beaten
     up by its own alignment a lot causing it to go neutral, and if it's
     been beaten up by another alignment causing it to go over to that
     alignment. */
  
  if (soc->population < 0)
    soc->population = 0;

  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (soc->killed_by[i] > max_killed_by)
	{
	  max_killed_by = soc->killed_by[i];
	  max_killed_by_align = i;
	}
    }
  
  if (max_killed_by_align == -1)
    return;
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (i != max_killed_by_align &&
	  soc->killed_by[i] > second_killed_by)
	{
	  second_killed_by = soc->killed_by[i];
	  second_killed_by_align = i;
	}
    }
  
  /* Find the second highest number now. */

  
  /* Don't let them switch if this is really small. Don't switch if the
     number of kills is much smaller than the recent maxpop. Also don't
     switch if the second_killed_by is close to max_killed_by (since then
     it can't decide who's worse... Instead see if there are any big
     affinities, and if so, then do switch. */
      
  /* Need to have lots of things killed. */
  if (max_killed_by < 150 || 
      /* It must shock the society by killing half. */
      max_killed_by < soc->recent_maxpop*2/3 ||
      nr (1,70) != 17 ||
      /* The next align up can't be too close in terms of hate. */
      second_killed_by > max_killed_by/2 ||
      /* Can't be too wedded to its own align. */
      soc->align_affinity[soc->align] >= MAX_AFFINITY/4)
    return;
  
  /* Now, once we have the percents and the opp align deaths,
     check the percentages. */
  
  /* Check if the society switches to neutral due to same side killings. */
  
  if (soc->align > 0 && !DIFF_ALIGN (soc->align, max_killed_by_align) && 
      nr (1,10) == 4)
    {
      if (align_info[soc->align] && align_info[0])
	{
	  area = find_area_in (soc->room_start);
	  sprintf (buf, "\x1b[1;37mThe %s of %s have gotten sick of the abuse from the %s warriors, and have decided to become %s.\x1b[0;37m\n\r",
		   soc->pname, (area ? NAME (area) : "somewhere"),
		   align_info[soc->align]->name, align_info[0]->name);
	  echo (buf);
	}
      change_society_align (soc, 0); 
      add_rumor (RUMOR_SWITCH, soc->vnum, 0, 0, 0);
      soc->level_bonus += 2;
      return;
    }
  
  else if (DIFF_ALIGN (soc->align, max_killed_by_align) &&
	   nr (1,10) == 4)
    {
      
      if (align_info[soc->align] && align_info[max_killed_by_align])
	{
	  area = find_area_in (soc->room_start);
	  sprintf (buf, "\x1b[1;37mThe %s of %s have gotten sick of being beaten up by the %s side, and have decided to join it.\x1b[0;37m\n\r",
		   soc->pname, (area ? NAME (area) : "somewhere"), 
		   align_info[max_killed_by_align]->name);
	  echo (buf);
	}
      change_society_align (soc, max_killed_by_align); 
      add_rumor (RUMOR_SWITCH, soc->vnum, max_killed_by_align, 0, 0);
      soc->level_bonus += 2;
      return;
    }
  return;
}


/* This function sets up the guard posts that a society will need by
   looking for exits from its allotted block of rooms into other
   parts of the world. */

void
set_up_guard_posts (SOCIETY *soc)
{
  int roomnum;         /* Loop through the rooms the society owns. */
  THING *room, *room2 , *nroom;       /* Current room and other rooms. */
  int room_choices[GUARD_POST_CHOICES]; /* Possible room choices. */
  int room_dir[GUARD_POST_CHOICES];    /* What direction these rooms lead out to. */
  int i, j;
  int dir; /* Used for checking all exits from this room. */
  int lvnum, uvnum; /* Lower and upper bounds for rooms for the society. */
  
  bool adj_to_inner[GUARD_POST_CHOICES];  /* Is this guard post adjacent to an inner room? */

  /* Get rid of most guardposts that are adjacent to others. */
  
  bool found_adjacent_post= FALSE; 
  
  /* I don't want guard posts attached to each other, so I will just
     pico 1 guard post out from a row if necessary. */
  
  /* Used for picking posts if many are available. */
  int num_choices = 0, num_chose = 0, choice = 0;
  VALUE *exit;

  if (!soc)
    return;

  lvnum = soc->room_start;
  uvnum = soc->room_end; 

  for (i = 0; i < GUARD_POST_CHOICES; i++)
    {
      room_choices[i] = 0;
      room_dir[i] = DIR_MAX;
      adj_to_inner[i] = FALSE;
    }
  
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    soc->guard_post[i] = 0;
      
  /* Loop through all rooms checking for exits that lead out of the
     society zone. When one is encountered, see if the reverse exit
     leads back into the society zone. If so, add this to the list of
     possible guard posts if the exit eventually leads to another area. */
  
  for (roomnum = lvnum; roomnum <= uvnum &&
	 num_choices < GUARD_POST_CHOICES; roomnum++)
    {
      if ((room = find_thing_num (roomnum)) != NULL &&
	  IS_ROOM (room) && 
	  !IS_ROOM_SET (room, BADROOM_BITS) &&
	  IS_ROOM_SET (room, ROOM_EASYMOVE))
	{
	  undo_marked (room);
	  
	  if ((dir = dir_to_other_area (room, room, lvnum, uvnum)) >= 0 &&
	      dir < REALDIR_MAX)
	    {
	      room_choices[num_choices] = room->vnum;
	      room_dir[num_choices] = dir;
	      num_choices++;
	    }
	  undo_marked (room);
	}
    }

  if (num_choices < 1)
    {
      for (roomnum = lvnum; roomnum <= uvnum &&
	     num_choices < GUARD_POST_CHOICES; roomnum++)
	{
	  if ((room = find_thing_num (roomnum)) != NULL &&
	  IS_ROOM (room) && 
	      !IS_ROOM_SET (room, BADROOM_BITS))
	    {
	      undo_marked (room);
	      
	      if ((dir = dir_to_other_area (room, room, lvnum, uvnum)) >= 0 &&
		  dir < REALDIR_MAX)
		{
		  room_choices[num_choices] = room->vnum;
		  room_dir[num_choices] = dir;
		  num_choices++;
		}
	      undo_marked (room);
	    }
	}
      if (num_choices < 1)
	return;
    }
  
  /* Now strip off any guard posts that don't lead to "inner" rooms in the
     society. Loop through all guard posts marking those rooms. */
  
  for (i = 0; i < num_choices; i++)
    {
      if ((room = find_thing_num (room_choices[i])) != NULL &&
	  IS_ROOM (room))
	SBIT (room->thing_flags, TH_MARKED);
    }
  
  /* Now check all of those rooms to see if they have an exit leading to
     a society room number that does not have a mark on it. */
  
  for (i = 0; i < num_choices; i++)
    {
      if ((room = find_thing_num (room_choices[i])) != NULL &&
	  IS_ROOM (room))
	{
	  for (j = 0; j < REALDIR_MAX; j++)
	    {
	      if ((exit = FNV (room, j + 1)) != NULL &&
		  exit->val[0] >= lvnum && exit->val[0] <= uvnum &&
		  (room2 = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM (room2) &&
		  !IS_MARKED (room2))
		adj_to_inner[i] = TRUE;
	    }
	}
    }

  /* Now see how many room choices we have left. All of the guard post
     rooms adjacent to inner rooms get left alone, but the other ones
     get set to 0 so we don't pick them. */
  
  num_choices = 0;
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    {
      if (adj_to_inner[i])      
	num_choices++;
      else
	room_choices[i] = 0;
      if ((room = find_thing_num (room_choices[i])) != NULL)
	RBIT (room->thing_flags, TH_MARKED);
    }
  
  if (num_choices < 1)
    return;
    

  /* Now get rid of all guard posts adjacent to other guard posts.
     This is not necessary since the societies will eventually build
     city walls that will leave only the guard posts in use. */
  
  /* Loop through all guard posts. Get rid of most "adjacent" guard posts. */
  
  for (i = 0; i < GUARD_POST_CHOICES; i++)
    {
      if ((room = find_thing_num (room_choices[i])) != NULL &&
	  IS_ROOM (room))
	{
	  found_adjacent_post = FALSE;
	  for (dir = 0; dir < REALDIR_MAX; dir++)
	    {
	      if ((exit = FNV (room, dir + 1)) != NULL &&
		  exit->val[0] != room->vnum &&
		  (nroom = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM (nroom))
		{
		  for (j = 0; j < GUARD_POST_CHOICES; j++)
		    {
		      /* If a guard post matches, kill this post. If
			 this post is in the middle of 2 other posts,
			 start killing the posts after the first one
			 found. */
		      if (room_choices[j] == exit->val[0])
			{
			  if (!found_adjacent_post)
			    room_choices[i] = 0;
			  else
			    room_choices[j] = 0;
			  found_adjacent_post = TRUE;
			}
		    }
		}			
	    }  
	}
    }
  
  /* Now randomly assign rooms to the choices we make. */

  for (i = 0; i < NUM_GUARD_POSTS && num_choices > 0; i++)
    {
      num_chose = nr (1, num_choices);
      choice = 0;
      /* Pick a random number from the list of possibilities, and zero
	 them out as we go along. */
      
      for (j = 0; j < GUARD_POST_CHOICES; j++)
	{
	  if (room_choices[j] && ++choice == num_chose)
	    {
	      soc->guard_post[i] = room_choices[j];
	      room_choices[j] = 0;
	      room_dir[j] = DIR_MAX;
	    }
	}
      num_choices--;
    }
  return;
}




/* If a thing ends up in its guard post, it stops hunting
   and begins to guard. */

void
start_guarding_post (THING *th)
{
  VALUE *soc, *guardval, *exit;
  SOCIETY *society;
  int i;
  char *t;
  THING *room;
  char buf[STD_LEN];
  if (!th)
    return;
  
  /* Make sure it's in a society. */
  

  if ((soc = FNV (th, VAL_SOCIETY)) == NULL
      || (society = find_society_num (soc->val[0])) == NULL ||
      !th->in || !IS_ROOM (th->in))
    return;


  /* This sets up a guard value so this mob
     will guard vs everything going into the society
     zone. */
  
  /* Make sure it's at a guard post for the society. */
  
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    {
      if (society->guard_post[i] == th->in->vnum)
	break;
    }

  /* If it's not a post, then don't start guarding. */
  
  if (i == NUM_GUARD_POSTS)
    return;

  /* Give it a guard value. */
  
  if ((guardval = FNV (th, VAL_GUARD)) == NULL)
    {
      guardval = new_value();
      guardval->type = VAL_GUARD;
      add_value (th, guardval);
    }
  
  guardval->val[0] |= GUARD_DOOR_ALIGN;
  
  /* Set all of the other directions to false except ones that don't lead
     into the society rooms. */
  
  t = buf;
  
  for (exit = th->in->values; exit; exit = exit->next)
    {
      /* Put blocks on all directions leading to society rooms. */
      
      if (exit->type >= 1 &&
	  exit->type <= REALDIR_MAX &&
	  exit->val[0] >= society->room_start &&
	  exit->val[0] <= society->room_end &&
	  (room = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (room))
	{
	  *t++ = *dir_name[exit->type-1];
	}
    }
  *t = '\0';
  set_value_word (guardval, buf);
		  
  /* Now add a sentinel flag to th. */
  
  add_flagval (th, FLAG_ACT1, ACT_SENTINEL);
  return;
}
      
		      
/* This checks if it's possible to get from a room in an area to another
   area avoiding all rooms controlled by a society. */

int
dir_to_other_area (THING *start_room, THING *room,  int lvnum, int uvnum)
{
  int dir;
  VALUE *exit, *rexit;
  THING *nroom, *back_room;

  if (!room || !IS_ROOM (room) || IS_MARKED (room))    
    return REALDIR_MAX;
  
  room->thing_flags |= TH_MARKED;
  
  if (room != start_room && room->vnum >= lvnum && room->vnum <= uvnum)
    return REALDIR_MAX;
  
  if (room->in && IS_AREA (room->in) && room->in != start_room->in &&
      (room->in != the_world->cont))
    return DIR_NORTH;
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      /* Search in every direction for an exit, and see if it
	 leads to another room, and that room has an exit back into 
	 the society zone, and there is a path from that next room
	 to another area outside the one where start_room is. */
      
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  !IS_SET (exit->val[2], EX_WALL) && 
	  (rexit = FNV (nroom, RDIR(dir) + 1)) != NULL &&
	  (rexit->val[0] == room->vnum ||
	   (rexit->val[0] >= lvnum && rexit->val[0] <= uvnum)) &&
	  (back_room = find_thing_num (rexit->val[0])) != NULL &&
	  IS_ROOM (back_room) &&
	  dir_to_other_area (start_room, nroom, lvnum, uvnum) != REALDIR_MAX)
	return dir;
    }
  return REALDIR_MAX;
}

/* Every once in a while, bring the caste chances back to the 
   base levels. */

void
update_caste_chances (SOCIETY *soc)
{
  int i;
  int diff; /* Difference between base and current chances. */
  
  if (!soc || nr (1,10) != 2)
    return;
  
  for (i = 0; i < CASTE_MAX; i++)
    {
      /* Find the current difference between the caste chance
	 and the base chance where it should be. If this difference
	 is bigger than the random number chosen, bring the chance
	 back closer to the base chance. */
      
      if ((diff = soc->chance[i] - soc->base_chance[i]) != 0)
	{    
	  if (diff > 0 && nr (1,20) < diff)
	    soc->chance[i]--;
	  else if (diff < 0 && nr (1,20) < -diff)
	    soc->chance[i]++;
	}
    }
  return;
}

/* This function increases the chances associated with picking certain 
   castes. It is used so that if there are too many or too few warriors
   or nonwarriors, then more of that type will get made over time. It
   is not some instant maker of new troops...it takes time for things
   to even out. If warriors is true, the battle castes are increased,
   otherwise the other castes are increased. Note that these get sent
   back down to the base_chances slowly over time. */


void
increase_caste_chances (SOCIETY *soc, bool warriors)
{
  int i, num_choices = 0, num_chose = 0, choice = 0, count;
  
  if (!soc)
    return;
  
  /* This may need some work because if you have a big caste like
     15 on warriors and a small one like 2 on wizards, the warriors
     will get bigger proportionally. Perhaps this will have to
     increase all chances and then rescale somehow. Or, perhaps
     there will have to be a finer-grained distinction over what
     to improve...so we will need warrior/worker/healer/builder/wizard
     improves. The chance can also never get higher than 2x the
     base chance. */

  for (count = 0; count < 2; count++)
    {
      for (i = 0; i < CASTE_MAX; i++)
	{
	  if (soc->base_chance[i] == 0 ||
	      (soc->chance[i] >= 2*soc->base_chance[i]) ||
	      warriors != IS_SET (soc->cflags[i], BATTLE_CASTES))
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (++choice >= num_chose)
	    break;	  
	}
      if (count == 0)
	{
	  if (num_choices == 0)
	    return;	  
	  num_chose = nr (1, num_choices);
	}
    }
  
  
  if (i >= 0 && i < CASTE_MAX)
    soc->chance[i]++;
  return;
}


/* This function determines if the society will settle`  a new society
   out someplace or not. */

void
settle_new_society (SOCIETY *soc)
{

  int ar_num = 0, ar_chose = 0;
  THING *room, *ar, *mob = NULL, *oldarea = NULL;
  char buf[STD_LEN];
  int i, max_room_num = 0, room_num, count, vnum;
  bool settle_adjacent = FALSE;
  int society_badroom_bits;
  
  if (!soc)
    return;
  

  /* Must be a settler society to settle, must pass the random percentage
     chance, and it has to be able to do activity now. 
     The society must also be large enough, and it cannot have settled
     within the recent past. */
  
  
  if (!IS_SET (soc->society_flags, SOCIETY_SETTLER) ||
      soc->population < nr (soc->population_cap/2, 
			    soc->population_cap) || 
      soc->settle_hours > 0 || 
      nr (1,SOCIETY_SETTLE_CHANCE) != SOCIETY_SETTLE_CHANCE/2)
    return;
  
  if ((oldarea = find_area_in (soc->room_start)) == NULL)
    return;
  
  /* Skip the start_area. */
  
  ar = the_world->cont;
  if (!ar->next_cont)
    return;
  else
    ar = ar->next_cont;
  
  /* Most of the time we settle in an adjacent area. */
  
  if (nr (1,13) !=  5)
    {
      settle_adjacent = TRUE;
      mark_adjacent_areas(oldarea);
    }
  
  for (count = 0; count < 2; count++)
    {     
      for (; ar != NULL; ar = ar->next_cont)
	{      
	  /* Settlers move to different areas.  Also, don't settle
	     in the same area where you started! Most of the time they
	     settle in adjacent areas, so only check those, too. */
	  if (society_can_settle_in_area (soc, ar) &&
	      (!settle_adjacent || IS_MARKED (ar)))
	    {
	      if (count == 0)
		ar_num++;
	      else if (--ar_chose < 1)
		break;	      
	    }
	}
      
      if (count == 0)
	{
	  if (ar_num < 1)
	    {
	      unmark_areas();
	      return;
	    }
	  else
	    {
	      ar_chose = nr (1, ar_num);
	      ar = the_world->cont->next_cont;
	    }
	}
    }
  
  
  unmark_areas();
  
  if (!ar)
    return;
  
  /* Find the maximum room number in the area. Usually rooms are filled
     from the front...so this SHOULD be a good measure. */
  
  society_badroom_bits = flagbits (soc->flags, FLAG_ROOM1);

  for (room = ar->cont; room; room = room->next_cont)
    {
      if (IS_ROOM (room) && !IS_ROOM_SET (room, BADROOM_BITS & ~society_badroom_bits))
	max_room_num = room->vnum;
    }
  
  /* The area is too small. */
  
  if (max_room_num - ar->vnum < 30)
    return;
  
  
  room_num = ar->vnum + 1 + nr (0, (max_room_num - ar->vnum)*3/4);
  

  /* Now see how many rooms exist and are accessible from the room we
     chose to use to start. */
  
  for (vnum = room_num - 5; vnum <= room_num + 5; vnum++)
    {
      if ((room = find_thing_num (vnum)) != NULL &&
	  !IS_ROOM_SET (room, BADROOM_BITS))
	break;
    }
  
  room_num = vnum;
  if (!room)
    return;
  
  undo_marked (room);
  mark_track_room (room, room->in, society_badroom_bits);
  
  /* Now see how many rooms we have that are connected to the original
     room and in the same area. */
  
  for (i = 0; i < MAX (10, soc->room_end - soc->room_start); i++)
    {
      if ((room = find_thing_num (room_num + i)) == NULL ||
	  !IS_ROOM (room) || !IS_MARKED (room))
	break;
    }
  
  room = find_thing_num (room_num);  
  undo_marked (room);
  
  /* Not enough connected rooms at this location. */

  if (i < 10 || i < (soc->room_end-soc->room_start)/2)
    return;
  
  
  /* Find a single mob to see if a path can be found. */
  
  sprintf (buf, "home all one nosent end settle vnum %d %d",
	   room->vnum, room->vnum);
  
  if ((mob = society_do_activity  (soc, 100, buf)) == NULL)
    return;
  
  /* If no path is found, give up. */
  
  if (!hunt_thing (mob, 0))
    {
      stop_hunting (mob, TRUE);
      return;
    }
  
  /* Some big number to keep them from settling too often. */
  soc->settle_hours = SOCIETY_SETTLE_HOURS; 
  
  
  sprintf (buf, "all nosent end settle vnum %d %d",
	   room->vnum, room->vnum);
  
  society_do_activity (soc, 33, buf);
  soc->settle_hours = 30;
  /* Send 1/3 of the mobs out settling. */
  
  return;
}


/* This checks if a certain society can settle in an area by seeing
   if there are already too many societies in the area or not. */

bool
society_can_settle_in_area (SOCIETY *soc, THING *ar)
{
  SOCIETY *soc2;
  THING *ar2, *oldarea;
  int soc_in_area = 0;
  int society_badroom_bits;


  /* Don't allow societies to settle in lowlevel areas. */
  
  if (!soc || !ar || !IS_AREA (ar) || LEVEL(ar) < 30)
    return FALSE;
  
  
  /* Don't allow settlers to settle in opp align aligned areas. */
  
  if (ar->align > 0 && DIFF_ALIGN (ar->align, soc->align))
    return FALSE;
  
  society_badroom_bits = flagbits (soc->flags, FLAG_ROOM1);
  /* Make sure that the society starts in an old area. Don't let it settle
     in its own starting area. */
  if ((oldarea = find_area_in (soc->room_start)) == NULL ||
      oldarea == ar ||
      IS_AREA_SET (ar, AREA_NOSETTLE) ||
      IS_ROOM_SET (ar, BADROOM_BITS & ~society_badroom_bits)) /* Cannot settle in certain areas. */
    return FALSE;
  
  /* Now there are more rules. You don't settle in an area where there
     are at least 3 societies of same align there, and you don't settle
     in an area where there's a society made up of the same
     mobs as you (on any side), which cuts down on the number of 
     areas you can settle into. */
 
  for (soc2 = society_list; soc2; soc2 = soc2->next)
    {
      if ((ar2 = find_area_in (soc2->room_start)) != NULL &&
	  ar2 == ar &&
	  soc2->base_chance[0] > 0)
	{
	  /* If a society of the same align is in the area in
	     question, make a note of it to keep things from 
	     getting too complicated. */
	  soc_in_area++;
	  if (soc2->align == soc->align)
	    {
	      soc_in_area++;
	      /* Diff copies of the same society on the same align */
	      if (soc->start[0] == soc2->start[0] )
		{
		  soc_in_area++;
		  if (!str_cmp (soc->adj, soc2->adj) ||
		      (!*soc->adj && !*soc2->adj))
		    soc_in_area += 10;
		}
	    }
	}
    }
  
  if (soc_in_area > 3)
    return FALSE;
  return TRUE;
}

/* Society sends out a patrol every once in a while if it has
   enough things sitting around in its area. */

/* Only send patrols if there are enough warriors and the random
   chance is met. */


void
update_patrols (SOCIETY *soc)
{
  THING *mob = NULL, *area, *oldarea, *room;
  int total_warriors;
  char buf[STD_LEN];
  
  if (!soc || nr (1,120) != 47 ||
      nr (1,15) != 3 ||
      (total_warriors = find_num_members (soc, BATTLE_CASTES)) < soc->population_cap/3 ||
      (oldarea = find_area_in (soc->room_start)) == NULL)
    return;
  
  /* Have to pick a neutral area other than our own area, and pick a 
     room in it. */

  if ((area = find_random_area(0)) == NULL ||
      area == oldarea || ALIGN(area) > 0 ||
      (room = find_random_room (area, FALSE, 0, BADROOM_BITS)) == NULL ||
      !IS_ROOM (room) ||
      IS_AREA_SET (area, AREA_NOSETTLE | AREA_NOLIST | AREA_NOREPOP))
    return;
  
  /* Find a battle mob in our society zone not doing anything,
     and see if it can get to the desired room. */
  
  sprintf (buf, "home battle one nosent nohunt end far_patrol vnum %d %d attr v:society:3 %d",
	   room->vnum, room->vnum, WARRIOR_PATROL);
  
  if ((mob = society_do_activity (soc, 100, buf)) == NULL)
    return;
  
  /* See if the mob can "track" the desired room. If not, stop the hunting
     and bail out. If so, continue on. */
  
  if (!hunt_thing (mob, 0))
    {
      stop_hunting (mob, TRUE);
      return;
    }
  
  add_rumor (RUMOR_PATROL, soc->vnum, area->vnum, 0, 0);

  
  sprintf (buf, "home battle nosent nohunt end far_patrol vnum %d %d attr v:society:3 %d",
	   room->vnum, room->vnum, WARRIOR_PATROL);
  society_do_activity (soc, 5, buf);
  return;
}



/* This finds the weakest room in a society. That is, the room within the
   range of numbers that the society owns that has the most enemies in
   it. */

THING *
find_weakest_room (SOCIETY *soc)
{
  THING *room, *thg;
  int max_enemy_power = 0, curr_enemy_power;
  int vnum, worst_vnum = 0;
  VALUE *socval;
  
  if (!soc)
    return NULL;
  
  for (vnum = soc->room_start; vnum <= soc->room_end; vnum++)
    {
      if ((room = find_thing_num (vnum)) == NULL ||
	  !IS_ROOM (room))
	continue;
      
      curr_enemy_power = 0;
      for (thg = room->cont; thg; thg = thg->next_cont)
	{
	  if (!CAN_FIGHT (thg) && !CAN_MOVE (thg))
	    continue;
	  
	  if (soc->align > 0 && DIFF_ALIGN (thg->align, soc->align))
	    curr_enemy_power += POWER (thg);
	  
	  if (soc->align == 0 &&
	      ((socval = FNV (thg, VAL_SOCIETY)) == NULL ||
	       socval->val[0] != soc->vnum))
	    curr_enemy_power += POWER (thg);
	}
      if (curr_enemy_power > max_enemy_power)
	{
	  max_enemy_power = curr_enemy_power;
	  worst_vnum = vnum;
	}
    }
  
  if (worst_vnum == 0)
    return NULL;
  
  return find_thing_num (worst_vnum);
}


/* This checks if society mobs are going to sleep. */


bool
sleeping_time (THING *th)
{
  int hour;
  THING *room;
  SOCIETY *soc;
  bool nocturnal = FALSE;
  bool daytime = FALSE;
  VALUE *socval;

  
  if (!th || 
      (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_in (th)) == NULL)
    return FALSE;
  
  /* Maybe have nocturnal creatures later on? */

  if (!wt_info)
    return FALSE;
  
  hour = wt_info->val[WVAL_HOUR];

  /* Some societies don't sleep. Also, not everything goes to sleep at once. */

  if (IS_SET (soc->society_flags, SOCIETY_NOSLEEP) || 
      (th->position != POSITION_SLEEPING &&
       nr (1,25) != 2))
    return FALSE;

  /* Find if we have a nocturnal society or not. */
  
  if (IS_SET (soc->society_flags, SOCIETY_NOCTURNAL))
    nocturnal = TRUE;
  
  if (hour >= HOUR_DAYBREAK && hour < HOUR_NIGHT)
    daytime = TRUE;
  
  /* Here is the idea. If it's daytime and you're not nocturnal, you
     wake up. If it's not daytime and you are nocturnal, you wake up.
     So, we check if daytime != nocturnal. */

  /* Waking up time. */


  if (daytime != nocturnal)
    {
      if (th->position == POSITION_SLEEPING  && nr (1,3) == 2)
	{
	  do_wake (th, "");
	  if (nr (1,5) == 2)
	    interpret (th, "yawn");
	}
      return FALSE;
    }
  /* Otherwise we are out of waking time. */
 
  /* Usually don't try to go to sleep right away so there isn't a mad
     rush to sleep. */
 
  if (th->position == POSITION_SLEEPING || nr (1,3) == 2)
    return FALSE;

  /* Guards don't sleep...ok it's stupid but so what. :P */

  if (IS_ACT1_SET (th, ACT_SENTINEL | ACT_PRISONER))
    return FALSE;

  /* If it's doing something, don't let it sleep. */

  if (is_hunting (th))
    return FALSE;

  /* Find where this thing should sleep. This has to be updated from time
     to time since things can change castes and get new sleeping 
     locations. */
  
  find_sleeping_room (th);
  
  /* Go to its sleeping room.  */
  
  if ((room = find_thing_num (socval->val[NUM_VALS-1])) != NULL &&
      IS_ROOM (room))
    {
      start_hunting_room (th, room->vnum, HUNT_SLEEP);
      if (!hunt_thing (th, 50))
	{
	  stop_hunting (th, FALSE);
	  return FALSE;
	}
      return TRUE;
    }
  
  /* If no room found find any old room and go there. */
  

  if ((room = find_thing_num (nr (soc->room_start, soc->room_end))) != NULL
      && IS_ROOM (room))
    {
      start_hunting_room (th, room->vnum, HUNT_SLEEP);
      if (!hunt_thing (th, 30))
	{
	  stop_hunting (th, FALSE);
	  return FALSE;
	}
    }
  return TRUE;
}



/* This tries to repopulate other societies if their population is
   too low. Basically if the population of this society is big and
   if there are other societies with the same starting vnum for
   caste 0 and the same align that are really small, then some
   members of the society get sent their way. */

void
reinforce_other_societies(SOCIETY *soc)
{
  
  SOCIETY *soc2;
  THING *room;
  int pass, choice = 0;
  char buf[STD_LEN];
  
  /* This society must exist and be big enough and it must not
     be neutral. */

  if (!soc || soc->population < soc->population_cap/2 ||
      soc->align == 0 || nr (1,250) != 35)
    return;
  
  for (pass = 0; pass < 2; pass++)
    {
      /* Only reinforce societies that aren't too big and aren't
	 basically gone already. */
      
      for (soc2 = society_list; soc2; soc2 = soc2->next)
	{
	  if (soc2 == soc ||
	      DIFF_ALIGN (soc2->align, soc->align) ||
	      soc2->population > soc2->population_cap/10 ||
	      soc2->start[0] != soc->start[0] ||
	      soc2->chance[0] == 0 || soc2->base_chance[0] == 0 ||
	      soc2->population < 10 || 
	      str_cmp (soc2->adj, soc->adj))
	    continue;
	  
	  if (pass == 0)
	    choice++;
	  else if (--choice == 0)
	    break;
	}
      
      if (pass == 0)
	{
	  if (choice == 0)
	    return;
	  else
	    choice = nr (1,choice);
	}
    }
 
  /* If the society does not exist or if it's been defeated, then
     don't bother helping it. */
 
  if (!soc2 || (IS_SET (soc2->society_flags, SOCIETY_DESTRUCTIBLE) &&
		soc2->base_chance[0] == 0))
    return;

  if ((room = find_thing_num (soc2->room_start)) == NULL ||
      !IS_ROOM (room) || IS_ROOM_SET (room, BADROOM_BITS))
    return;

 
  
  add_rumor (RUMOR_REINFORCE, soc->vnum, soc2->vnum, 0, 0);
  
  sprintf (buf, "home all nosent end kill vnum %d %d v:society:0 %d",
	   room->vnum, room->vnum, soc2->vnum);
  society_do_activity (soc, 15, buf);
  

  /* Update populations */

  update_society_population (soc);
  update_society_population (soc2);
  
  
  return;
}
  


/* This is where a society member that's actually settling
   founds a new society wheni t gets to its proper location. */


void
settle_new_society_now (THING *th)
{
  SOCIETY *oldsoc, *newsoc;
  int i, room_count, start_room_vnum, max_room_vnum = 0;
  THING *room;
  VALUE *socval;
  char buf[STD_LEN];
  
  if ((socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (oldsoc = find_society_in (th)) == NULL ||
      !th->in || !IS_ROOM (th->in) || !th->in->in ||
      !IS_AREA (th->in->in) || 
      !society_can_settle_in_area (oldsoc, th->in->in))
    return;
  
  /* Find out how many rooms the new society will take up. This is to
     avoid stupid things like splitting the society members into several
     disconnected components. Technically these checks should not be
     necessary since they're also done when the society members first
     move out to settle a new society, but it's good to do them anyway. */
  
  
  start_room_vnum = th->in->vnum;
  

  for (room = th->in->in->cont; room; room = room->next_cont)
    {
      if (IS_ROOM (room) && room->vnum > max_room_vnum)
	max_room_vnum = room->vnum;
    }

  room_count = (max_room_vnum - start_room_vnum) * 4/5;
 
  /* Divide the raw materials and "quality" in half. */
  
  for (i =0; i < RAW_MAX; i++)
    oldsoc->raw_curr[i] /= 2;
  oldsoc->quality /= 2;
  oldsoc->level_bonus /= 2;
  
  newsoc = new_society ();
  copy_society (oldsoc, newsoc);
  soc_to_list (newsoc);
  
  for (i = 0; i < CASTE_MAX; i++)
    newsoc->curr_pop[i] = 0;
  
  
  for (i = 0; i < NUM_GUARD_POSTS; i++)
      newsoc->guard_post[i] = 0;
  
  /* Now make it so the society has stopped "moving" */
  
  newsoc->room_start = th->in->vnum;
  newsoc->room_end = th->in->vnum + room_count;
  oldsoc->settle_hours = SOCIETY_SETTLE_HOURS;
  newsoc->settle_hours = SOCIETY_SETTLE_HOURS;
  oldsoc->alert /= 2;
  newsoc->alert /= 2;
  newsoc->recent_maxpop /= 2;
  oldsoc->recent_maxpop /= 2;
  /* Allow the societies to keep this flag because they will not
     generally settle more than 1 area away from them, so the
     overexpansion is lessened. */
  
  /* RBIT (newsoc->society_flags, SOCIETY_SETTLER); */
  SBIT (newsoc->society_flags, SOCIETY_DESTRUCTIBLE);
  RBIT (newsoc->society_flags, SOCIETY_FIXED_ALIGN);
  /* Now make all mobs which were in the old society, but
     are hunting to settle get put in the new society. */
  
  sprintf (buf, "all start settle vnum %d %d end healing attr v:society:0 %d v:society:6 0",
	   newsoc->room_start, newsoc->room_end, newsoc->vnum);
  society_do_activity (oldsoc, 100, buf);
  update_society_population (oldsoc);
  update_society_population (newsoc);
  set_up_guard_posts (newsoc);
  
  /* Let everyone know that they settled. */
 
  
  add_rumor (RUMOR_SETTLE, oldsoc->vnum, th->in->in->vnum, 0, 0);
  return;
}	      


/* This function lets the society start to engage in some activity like
   raiding a city or changing a stat. This basically passes a society
   then does a ton of argument parsing to get the desired activity. 
   There are some default args in here, also. The activity can be 
   sending troops to do something, or it can be making the society 
   members change somehow. The percent is the percent chance that
   each thing actually does this activity. */

THING *
society_do_activity (SOCIETY *soc, int percent, char *arg)
{
  int caste_flags = 0;  /* Which castes listen to this activity. */
  int start_hunting_type = HUNT_NONE; /* Starting hunt type required. */
  int end_hunting_type = HUNT_NONE; /* End hunting type given */
  int raid_number = 0; /* Number of the raid you must be in for this. */
  int room_start_in = 0; /* What room you have to start in. */
  bool be_in_home_area = FALSE; /* Do you have to start in the home area? */
  bool out_of_home_area = FALSE; /* Do you have to be out of home area? */
  bool ignore_hunters = FALSE;   /* Do you ignore hunting things? */
  bool ignore_sentinels = FALSE; /* Do sentinels ignore this? */
  bool find_just_one = FALSE;   /* Do we just find one mob and return it? */
  int target_start_vnum = 0;   /* Starting vnums you're shooting for. */
  int target_end_vnum = 0; /* End vnum you shoot for. */
  char victim_name[STD_LEN]; /* The name of the victim you hunt. */
  char word[STD_LEN]; /* Word used for stripping off arguments. */
  bool found = FALSE; /* Did we find the word we wanted this loop? */
  bool all = FALSE; /* Do we update all? */
  /* List of attribute types and values you will change the society
     members to as a result of this activity. */
  THING *room_start = NULL, *room_end = NULL;
  char attr_type[SOCIETY_ATTR_CHANGE_MAX][STD_LEN];
  char attr_op[SOCIETY_ATTR_CHANGE_MAX];
  char attr_value[SOCIETY_ATTR_CHANGE_MAX][STD_LEN];
  int attr_count = 0; /* How many attributes we are changing already. */
  int i;  /* General counters. */
  int castenum, tiernum, vnum; /* Looping through soc members. */
  THING *th, *thn; /* Loop variables used in the hash table. */
  VALUE *socval;
  THING *in_room = NULL; /* The room the thing is in. */
  THING *target_room = NULL; /* The room where the thing is going. */
  THING *society_start_area = NULL; 
  int room_trial; /* If we search for a random room, we try 6 times before
		     giving up in case certain rooms are missing that should
		     be there. */
  /* A little sanity checking. */
  if (!soc || percent < 1 || percent > 100 || !arg || !*arg)
    return NULL;
  
  society_start_area = find_area_in (soc->room_start);
  
  /* Set a few variables. */
  
  victim_name[0] = '\0';
  
  for (i = 0; i < SOCIETY_ATTR_CHANGE_MAX; i++)
    {
      attr_type[i][0] = '\0';
      attr_op[i] = '=';
      attr_value[i][0] = '\0';
    }
  
  /* This loops through the arguments to set up what the loop through the
     society members will look like. It's kind of slow and ugly, but
     this shouldn't be called too often, so it should be ok. */
  
  while (arg && *arg)
    {
      found = FALSE;
      arg = f_word (arg, word);
      
      /* Check for caste flags. */
      
      if (!str_cmp (word, "battle"))
	{
	  caste_flags |= BATTLE_CASTES;
	  continue;
	}
      
      for (i = 0; caste1_flags[i].flagval != 0; i++)
	{
	  if (!str_prefix (word, caste1_flags[i].name))
	    {
	      found = TRUE;
	      caste_flags |= caste1_flags[i].flagval;
	    }
	}
      
      if (found)
	continue;
      
      /* Find the starting and ending hunting types if needed. */

      if (!str_prefix (word, "start_hunting_type"))
	{
	  arg = f_word (arg, word);
	  if (isdigit (word[0]))
	    start_hunting_type = MID (0, atoi (word), HUNT_MAX-1);
	  else
	    {
	      for (i = 0; i < HUNT_MAX; i++)
		{
		  if (!str_cmp (word, hunting_type_names[i]))
		    {
		      start_hunting_type = i;
		      found = TRUE;
		      break;
		    }
		}
	      if (!found)
		start_hunting_type = HUNT_NONE;
	    }	  
	  continue;
	}
	  
      if (!str_prefix (word, "end_hunting_type"))
	{
	  arg = f_word (arg, word);
	  if (isdigit (word[0]))
	    end_hunting_type = MID (0, atoi (word), HUNT_MAX-1);
	  else
	    {
	      for (i = 0; i < HUNT_MAX; i++)
		{
		  if (!str_cmp (word, hunting_type_names[i]))
		    {
		      end_hunting_type = i;
		      found = TRUE;
		      break;
		    }
		}
	      if (!found)
		end_hunting_type = HUNT_NONE;
	    }	  
	  continue;
	}
      /* Tell if this is a raid or not. */
      
      if (!str_cmp (word, "raid"))
	{
	  arg = f_word (arg, word);
	  raid_number = atoi (word);
	  continue;
	}
      
      if (!str_cmp (word, "in"))
	{
	  arg = f_word (arg, word);
	  room_start_in = atoi (word);
	  continue;
	}

      /* Check for a few flags that may need to be set. */

      if (!str_prefix (word, "home_area"))
	{
	  be_in_home_area = TRUE;
	  continue;
	}
      if (!str_prefix (word, "other_area"))
	{
	  out_of_home_area = TRUE;
	  continue;
	}
      if (!str_cmp (word, "all"))
	{
	  all = TRUE;
	  continue;
	}
      if (!str_prefix (word, "sentinels_ignored") ||
	  !str_prefix (word, "nosentinels") ||
	  !str_prefix (word, "no_sentinels"))
	{
	  ignore_sentinels = TRUE;
	  continue;
	}
      if (!str_prefix (word, "hunters_ignored") ||
	  !str_prefix (word, "nohunters") ||
	  !str_prefix (word, "no_hunters"))
	{
	  ignore_hunters = TRUE;
	  continue;
	}

      if (!str_prefix (word, "find_just_one") ||
	  !str_cmp (word, "one") ||
	  !str_prefix (word, "find_one"))
	{
	  find_just_one = TRUE;
	  continue;
	}
      
      /* If we are hunting a range of vnums, then find the starting
	 and ending rooms. If this is to work, the starting and
	 ending rooms must be rooms in the same area and end must be
	 >= start. */
      
      if (!str_cmp (word, "target") || !str_cmp (word, "vnum") ||
	  !str_cmp (word, "target_rooms") || !str_cmp (word, "vnums"))
	{
	  arg = f_word (arg, word);
	  target_start_vnum = atoi (word);
	  arg = f_word (arg, word);
	  target_end_vnum = atoi (word);
	  continue;
	}
      
      /* Find the name of a victim to hunt. */

      if (!str_cmp (word, "victim") || !str_cmp (word, "victim_name") ||
	  !str_cmp (word, "name"))
	{
	  arg = f_word (arg, word);
	  strcpy (victim_name, word);
	  victim_name[HUNT_STRING_LEN] = '\0';
	  continue;
	}
      
      /* If you want to change attributes of things in a society,
	 you can do it with this function. You specify the kind
	 of attribute you want to change, whether its a
	 modification of the current attribute, or equal to a 
	 number (optional if you don't want to specify the = sign), and
	 then the value it changes to or the amount it changes by. */
      
      if (!str_prefix (word, "attributes"))
	{
	  arg = f_word (arg, word);
	  if (attr_count < SOCIETY_ATTR_CHANGE_MAX)
	    strcpy (attr_type[attr_count], word);
	  
	  /* If no operation given, it is assumed to be = */
	  if (is_op (*arg))
	    {
	      if (attr_count < SOCIETY_ATTR_CHANGE_MAX)
		attr_op[i] = *arg;
	      arg++;
	    }
	  
	  arg = f_word (arg, word);
	  
	  if (attr_count < SOCIETY_ATTR_CHANGE_MAX)
	    strcpy (attr_value[attr_count], word);
	  if (attr_count < SOCIETY_ATTR_CHANGE_MAX)
	    attr_count++;
	}
      
    }
  
  /* Now check to see if the target_rooms are ok. If not, nuke them. */
  
  if (target_start_vnum > 0 || target_end_vnum > 0)
    {
      room_start = find_thing_num (target_start_vnum);
      room_end = find_thing_num (target_end_vnum);
      
      if (!room_start || !room_end || !IS_ROOM (room_start) ||
	  !IS_ROOM (room_end) || room_start->in != room_end->in ||
	  target_start_vnum > target_end_vnum)
	{
	  target_start_vnum = 0;
	  target_end_vnum = 0;
	}
      else /* If you have to decide between room numbers and a
	      victim name, go with the room numbers. */
	{
	  victim_name[0] = '\0';
	}
	
    }
  
  /* Now this loop goes through the society members and does things
     to them based on what we specified with the above options. */
  
  
  /* Loop through the castes. */
  
  for (castenum = 0; castenum < CASTE_MAX; castenum++)
    {
      if (soc->start[castenum] == 0 ||
	  soc->chance[castenum] == 0 ||
	  soc->curr_pop[castenum] == 0)
	continue;
      
      /* Now loop through the tiers within a caste. */
      
      for (tiernum = 0; tiernum < soc->max_tier[castenum]; tiernum++)
	{
	  
	  /* Now loop through the hash table. */
	  
	  vnum = soc->start[castenum] + tiernum;
	  
	  for (th = thing_hash[vnum % HASH_SIZE]; th; th = thn)
	    {
	      thn = th->next;
	      
	      /* Check only things of the correct vnum in the 
		 correct society that can fight and move
		 and have the other necessary attributes. */

	      if (th->vnum != vnum ||
		  (socval = FNV (th, VAL_SOCIETY)) == NULL ||
		  socval->val[0] != soc->vnum ||
		  (in_room = th->in) == NULL ||
		  (!CAN_MOVE(th) && !CAN_FIGHT(th)) ||
		  nr (1,100) > percent ||
		  /* Check if the caste flags are ok. */
		  (caste_flags && !IS_SET (socval->val[2], caste_flags)) ||
		  /* Check if the start hunting type is ok. */		  
		  (start_hunting_type && 
		   (!th->fgt ||
		    th->fgt->hunting_type != start_hunting_type)) ||
		  /* Check if we ignore things that are hunting/sentinel. */
		  (!all &&
		   ((ignore_hunters && is_hunting (th)) ||
		    (ignore_sentinels && IS_ACT1_SET (th, ACT_SENTINEL | ACT_PRISONER)))) ||
		  /* Check if we have to be in a raid. */
		  (raid_number > 0 && socval->val[4] != raid_number) ||
		  /* See if you need to be in a particular room or not. */
		  (room_start_in && 
		   (!th->in || th->in->vnum != room_start_in)) ||
		  /* Check if you must be in the home area. */
		  (be_in_home_area &&
		   (!IS_ROOM(in_room) || 
		    in_room->in != society_start_area)) ||
		  /* Check if you must be out of the home area. */
		  (out_of_home_area &&
		   (!IS_ROOM(in_room) || 
		    in_room->in == society_start_area)))
		continue;
	      /* At this point we should be acting on the member. */
	      
	      /* Now alter attributes. */
	      
	      for (i = 0; i < attr_count; i++)
		{
		  replace_one_value (NULL, attr_type[i], th, TRUE, 
				     attr_op[i], attr_value[i]);
		}
	      
	      /* Start hunting if necessary. */
	      
	      
	      if (target_start_vnum > 0 &&
		  target_end_vnum > 0)
		{
		  target_room = NULL;
		  for (room_trial = 0; room_trial < 40 && !target_room; room_trial++)
		    {
		     
		      int target_room_bits = 0;
		      if ((target_room = find_thing_num (nr (target_start_vnum, target_end_vnum))) == NULL ||
			  !IS_ROOM (target_room))
			continue;
		      /* The target room must not have badroom bits or
			 the society must have those same badroom bits. */
		      if((target_room_bits = 
			  (flagbits (target_room->flags, FLAG_ROOM1) & BADROOM_BITS)) != 0 &&
			 IS_SET (~target_room_bits, flagbits (soc->flags, FLAG_ROOM1)))
			continue;
		      
		      stop_hunting (th, TRUE);
		      start_hunting_room (th, target_room->vnum, end_hunting_type);
		      break;
		    }
		}
	      else if (victim_name[0])
		{
		  stop_hunting (th, FALSE);
		  start_hunting (th, victim_name, end_hunting_type);
		}
	      
	      /* If we just wanted a mob with certain properties,
		 find it and return it. (After affecting it maybe). */

	      if (find_just_one)
		return th;
	    }
	}
    }
  
  return NULL;
}
		  

/* This finds a room where a thing can sleep if it doesn't have a place
   already. It scans all of the rooms that the society owns and
   sees which ones are part of the society city and are set to be
   houses. The different castes get different places to sleep within
   the city. Things in battle castes go to their own special rooms
   and things without caste flags cannot sleep in those special 
   places. */


void
find_sleeping_room (THING *th)
{
  VALUE *socval, *build;
  THING *room;
  SOCIETY *soc;
  int num_choices = 0, num_chose = 0, choice = 0;
  int caste_flags;
  int vnum, count;

  if (!th || 
      (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_in (th)) == NULL || 
      IS_ACT1_SET (th, ACT_PRISONER))
    return;
  
  
  caste_flags = socval->val[2];
  
  /* Check if the old room is ok, otherwise find a new room. */
  
  /* Checks if the current sleeping room is ok. */

  if ((room = find_thing_num (socval->val[NUM_VALS-1])) != NULL &&
      IS_ROOM (room) &&
      (build = FNV (room, VAL_BUILD)) != NULL &&
      build->val[0] == socval->val[0] &&
      build->val[3] &&
      (!caste_flags || IS_SET (build->val[3], caste_flags)))
    return;
  
  /* Find a new room. */
  
  for (count = 0; count < 2; count++)
    {
      for (vnum = soc->room_start; vnum <= soc->room_end; vnum++)
	{
	  if ((room = find_thing_num (vnum)) == NULL ||
	      !IS_ROOM (room) ||
	      (build = FNV (room, VAL_BUILD)) == NULL ||
	      build->val[0] != socval->val[0] ||
	      !build->val[3] ||
	      (caste_flags && !IS_SET (build->val[3], caste_flags)))
	    continue;
	  
	  /* If we get to this point, it's obvious that the
	     room is still ok. */

	  if (vnum == socval->val[NUM_VALS-1])
	    return;
	  
	  if (count == 0)
	    num_choices++;
	  else if (++choice >= num_chose)
	    break;
	}
      if (count == 0)
	{
	  if (num_choices == 0)
	    return;	  
	  num_chose = nr (1, num_choices);
	}
    }
 
  /* Set the room. It should be ok. */
 
  socval->val[NUM_VALS-1] = vnum;
  return;
}
  


/* This updates a prisoner's status. There are a few things that
   can happen. If it's in a room where there aren't any enemies
   it stops being a prisoner. If it's in the enemy city, it doesn't
   try to escape. If it is rescued, it follows its rescuer and
   when it gets back home, it rewards the rescuer. 

   Incidentally, all of this "kidnap" code is my implemenetation
   of the orc breeder/princess discussion on MUD-Dev from '97.
   It isn't really idea per se, but I haven't seen anyone else 
   doing this, so here ya go. :) */

bool
update_prisoner_status (THING *th)
{
  THING *other, *room, *th_area, *other_area;
  SOCIETY *th_soc, *other_soc;
  VALUE *socval;
  int enemies_in_room = 0;
  int friends_in_room = 0;
  int nonfollowing_friends_in_room = 0; /* Don't want to follow someone
					   unless they're not following
					   anyone else. */
  char buf[STD_LEN];
  /* Make sure this thing is a prisoner. */
  
  if (!th || !th->in || !IS_ACT1_SET (th, ACT_PRISONER) ||
      (socval = FNV (th, VAL_SOCIETY)) == NULL)
    return FALSE;
  
  /* Make sure that it's in a room. */

  if ((room = th->in) == NULL || !IS_ROOM (room))
    return FALSE;
  
  /* Get the thing's society. */

  if ((th_soc = find_society_num (socval->val[0])) == NULL)
    return FALSE;
  
  /* See if there are any enemies in the room. */

  for (other = th->in->cont; other; other = other->next_cont)
    {
      if (!CAN_MOVE (other) || !CAN_FIGHT (other) || other == th)
	continue;
      if ((DIFF_ALIGN (other->align, th->align) ||
	  (th->align == 0 && 
	   (other_soc = find_society_in (other)) != th_soc)) &&
	  /* Other prisoners don't cause problems. */
	  !IS_ACT1_SET (other, ACT_PRISONER))
	enemies_in_room++;
      else if (IS_PC (other))
	{
	  friends_in_room++;
	  if (!FOLLOWING(other))
	    nonfollowing_friends_in_room++;
	}
    }
  
  
  /* If there's an enemy in the room and no friend, we only 
     check if we are in the enemy's stronghold and if so we stop
     following the enemy. */

  if (enemies_in_room > 0)
    {
      if (friends_in_room == 0)

	/* If only enemies are in the room, you stop following once they
	   have brought you back to their city. */
	{
	  if (th->fgt && th->fgt->following && 
	      (other_soc = find_society_in (th->fgt->following)) != NULL &&
	      th->in->vnum >= other_soc->room_start &&
	      th->in->vnum <= other_soc->room_end &&
	      nr (1,4) == 3)
	    {
	      th->fgt->following = NULL;
	    }
	}
      /* Both enemies and friends in the room. */
      else if (nr (1,3) == 2)
	{
	  if ((other_soc = find_society_num (socval->val[5])) != NULL &&
	      (th_area = find_area_in (th_soc->room_start)) != NULL &&
	      (other_area = find_area_in (other_soc->room_start)) != NULL &&
	      (!FOLLOWING(th) || 
	       is_enemy (FOLLOWING(th), th)))
	    {
	      char buf[STD_LEN];
	      other_soc = find_society_num (socval->val[5]);
	      sprintf (buf, "Help me, I am being held by the %s of %s and want to get back to the %s of %s!",
		       other_soc->pname, NAME(other_area), th_soc->pname, NAME(th_area));
	      do_say (th, buf);
	    }
	  else if (!FOLLOWING (th) ||
		   is_enemy (th, FOLLOWING (th)))
	    
	    do_say (th, "Please help me. I'm being held prisoner!");
	}
      return TRUE;
    }
  /* No enemies in sight, and only friends here. */
  else if (friends_in_room > 0)
    {
      /* If back in homeland, give reward. */
      if (th->in->vnum >= th_soc->room_start &&
	  th->in->vnum <= th_soc->room_end)
	{
	  /* Give a reward to all persons of the correct alignment in
	     the room. Hopefully players will use this to plevel newbies
	     "safely". :) */
	  THING *person;
	  for (person = th->in->cont; person; person = person->next_cont)
	    {
	      if (!DIFF_ALIGN (person->align, th->align) &&
		  IS_PC (person))		
		{
		  add_society_reward (person, th, REWARD_KIDNAP, LEVEL(th)*5);
		}
	    }
	  if (th->fgt && th->fgt->following)
	    th->fgt->following = NULL;
	  socval->val[5] = 0;
	  remove_flagval (th, FLAG_ACT1, ACT_PRISONER);
	  return TRUE;
	}
      
      /* Otherwise follow if needed, or just tag along if not
	 following. */

      add_fight (th);
      if (!th->fgt->following)
	{
	  for (other = th->in->cont; other; other = other->next_cont)
	    {
	      if (!CAN_MOVE(other) || !CAN_FIGHT (other) || other == th)
		continue;
	      
	      if (!DIFF_ALIGN (other->align, th->align)  &&
		  !FOLLOWING(other) &&
		  th->align > 0 && nr (1,nonfollowing_friends_in_room) == 1)
		{
		  act ("$E@1n follow@s @3n.$7", th, NULL, other, NULL, TO_ALL);
		  th->fgt->following = other;
		  th_area = find_area_in (th_soc->room_start);
		  if (th_area)
		    sprintf (buf, "%s, could you please take me back to the %s of %s?",  NAME(other), th_soc->pname, NAME(th_area));
		  else
		    sprintf (buf, "%s, could you please take me back to the %s?", NAME (other), th_soc->pname);
		  do_say (th, buf);
		  break;
		}
	    }
	}
    }
  else 
    {
      VALUE *build;
     
      /* If the thing is not in a society room and it's not attended
	 by anyone, then make it go free. */
      if ((build = FNV (th->in, VAL_BUILD)) == NULL ||
	  build->val[0] == socval->val[0])
	{
	  socval->val[5] = 0;
	  remove_flagval (th, FLAG_ACT1, ACT_PRISONER);
	}
    }
  
  return TRUE;
}





/* This command lets you pay money to a society to cause it to come
   to your side. */

void
do_bribe (THING *th, char *arg)
{
  SOCIETY *soc;
  VALUE *build;
  THING *area;
  int convert_cost, stay_cost, cost_reduction_percent;
  char buf[STD_LEN];
  
  if (!th || !th->in || !IS_PC (th) || th->align >= ALIGN_MAX)
    return;

  if ((build = FNV (th->in, VAL_BUILD)) == NULL ||
      (soc = find_society_num (build->val[0])) == NULL)
    {
      stt ("This location isn't owned by a society.\n\r", th);
      return;
    }
  area = find_area_in (soc->room_start);
  /* Cost is society power in gold (x100 copper). */
  convert_cost = MAX (1000000, soc->power*BRIBE_POWER_MULTIPLIER);
  stay_cost = MAX (200000, soc->power*BRIBE_SAME_ALIGN_STAY);
  /* The more times it's been switching sides, the more expensive it is to
     get it to change. */
  convert_cost += convert_cost*(soc->level_bonus*soc->level_bonus)/5;
  stay_cost += stay_cost*(soc->level_bonus*soc->level_bonus)/5;
  
  if (str_cmp (arg, "yes now"))
    {
      /* Mess up the costs a bit...*/
      convert_cost += nr (0,convert_cost/3);
      convert_cost -= nr(0, convert_cost/2);
      stay_cost += nr (0,stay_cost/3);
      stay_cost -= nr(0, stay_cost/2);
      
      cost_reduction_percent = MID (0, soc->align_affinity[th->align], 95);
      
      convert_cost = (100-cost_reduction_percent)*convert_cost/100;
      stay_cost = (100-cost_reduction_percent)*stay_cost/100;

      /* Now alter the costs based on the affinity involved. */
      
      

      th->pc->wait += 100;
      if (DIFF_ALIGN (th->align, soc->align))
	{
	  if (IS_SET (soc->society_flags, SOCIETY_OVERLORD))
	    sprintf (buf, "The %s of %s are held in thrall by a powerful leader.\n\r", society_pname(soc), (area ? NAME(area) : "these lands"));
	  else if (!IS_SET (soc->society_flags, SOCIETY_FIXED_ALIGN))
	    sprintf (buf, "It would cost about %d coins to get the %s of %s to change sides.\n\r", convert_cost, society_pname(soc), (area ? NAME(area) : "these lands"));
	  else 
	    sprintf (buf, "The %s of %s can't be bought. Sorry.\n\r",
		     society_pname(soc), (area ? NAME(area) : "these lands"));
	}
      else
	sprintf (buf, "It would cost about %d coins to strengthen the ties the %s of %s have to your alignment.\n\r", 
	stay_cost,  society_pname(soc) , (area ? NAME(area) : "these lands"));
      
      stt (buf, th);
      return;
    }
  
  
  /* If they're of a different alignment, they must be able to switch...*/
  
  if (DIFF_ALIGN (th->align, soc->align))
    {
      if (IS_SET (soc->society_flags, SOCIETY_OVERLORD))
	{
	  sprintf (buf, "The %s of %s are held in thrall by a powerful leader.\n\r", society_pname (soc), (area ? NAME(area) : "these lands"));	  
	  stt (buf, th);
	  return;
	}
      else if(IS_SET (soc->society_flags, SOCIETY_FIXED_ALIGN))
	{
	  sprintf (buf, "The %s of %s can't be bought. Sorry.\n\r", 
		   society_pname(soc), (area ? NAME(area) : "these lands"));
	  stt (buf, th);
	  return;
	}
    }
  
  /* Keep them in this alignment...*/

  if (soc->align > 0 && !DIFF_ALIGN (th->align, soc->align))
    {
      if (total_money (th) >= stay_cost)
	{
	  sprintf (buf, "The %s of %s are happy to receive your generous donation of seed money that they can now use to help their widows and orphans to better their lives.\n\r", society_pname(soc), (area ? NAME(area) : "these lands"));
	  stt (buf, th);
	  soc->alert /= 2;
	  soc->level_bonus++;
	}
      else
	{
	  sprintf (buf, "The %s of %s are glad to take your money, but you'll have to dig a little deeper next time to keep them happy.\n\r",
		   society_pname(soc), (area ? NAME(area) : "these lands"));
	  soc->align_affinity[th->align] = 0;
	  stt (buf, th);
	}
      sub_money (th, stay_cost);
    }
  else
    {
      if (total_money (th) >= convert_cost)
	{
	  sprintf (buf, "After careful consideration, the %s of %s have accepted your offer and have joined the %s cause!\n\r", 
		   society_pname(soc), (area ? NAME(area) : "these lands"),
		   FRACE(th)->name);
	  stt (buf, th);
	  soc->level_bonus++;
	  soc->alert = 0;
	  change_society_align (soc, th->align);
	  add_rumor (RUMOR_SWITCH, soc->vnum, th->align, 0, 0);
	}
      else
	{
	  sprintf (buf, "You are way too cheap. The %s of %s don't want to deal with someone that cheap.\n\r",
		   society_pname(soc), (area ? NAME(area) : "these lands"));
	  stt (buf, th);
	}
      sub_money (th, convert_cost);
    }
  return;
}

/* This attempts to raise the dead. */

bool
society_necro_generate (THING *th)
{
  THING *corpse, *newmem , *obj, *objn;
  int num_choices = 0, num_chose = 0, count;
  VALUE *socval;
  SOCIETY *soc;

  if (!th || !th->in || !IS_ROOM (th->in) ||
      (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_in (th)) == NULL ||
      !IS_SET (soc->society_flags, SOCIETY_NECRO_GENERATE) ||
      !IS_SET (socval->val[2], CASTE_HEALER | CASTE_WIZARD))
    return FALSE;

  for (count = 0; count < 2; count++)
    {
      for (corpse = th->in->cont; corpse; corpse = corpse->next_cont)
	{
	  if (corpse->vnum != CORPSE_VNUM || corpse == th)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 0)
	    break;
	}

      if (count == 0)
	{
	  if (num_choices < 1)
	    return FALSE;
	  num_chose = nr (1, num_choices);
	}
    }
  
  if (!corpse || corpse->vnum != CORPSE_VNUM)
    return FALSE;
  
  if ((newmem = create_thing (soc->start[0])) == NULL)
    return FALSE;
  
  thing_to (newmem, th->in);
  set_new_member_stats (newmem, soc);
  act ("$9@1n$9 wave@s @1s claw and @2n$9 turn@s into @3n$9!$7",
       th, corpse, newmem, NULL, TO_ALL);
  add_money (newmem, total_money(corpse));
  for (obj = corpse->cont; obj; obj = objn)
    {
      objn = obj->next_cont;
      thing_to (obj, newmem);
    }
  do_wear (newmem, "all");
  free_thing (corpse);
  return TRUE;
}
