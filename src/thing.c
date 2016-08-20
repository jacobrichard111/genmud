#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "note.h"
#include "script.h"
#include "track.h"
#include "mapgen.h"
#include "event.h"
#ifdef USE_WILDERNESS
#include "wilderness.h"
#include "wildalife.h"
#endif




/* This function makes a new thing and sets up all the data note that
   this takes an argument which is a pointer to an index type. */
	  
THING *
new_thing (void)
{
  THING *newthing = NULL, *prev;
  int i;
  /* This first part is to store the track data so that we don't
     get null tracks. */

  thing_made_count++;
  if (IS_SET (server_flags, SERVER_READ_PFILE))
    {
      RBIT (server_flags, SERVER_READ_PFILE);
      if (thing_free_pc)
	{
	  if (!str_cmp (thing_free_pc->name, pfile_name))
	    {
	      newthing = thing_free_pc;
	      thing_free_pc = thing_free_pc->next;
	    }
	  else
	    {
	      for (prev = thing_free_pc; prev != NULL; prev = prev->next)
		{
		  if (!prev->next)
		    break;
		  if (!str_cmp (prev->next->name, pfile_name))
		    {
		      newthing = prev->next;
		      prev->next = newthing->next;
		      break;
		    }
		}
	    }
	} 
      if (newthing)
	{
	  free_str (newthing->name);
	  free_str (newthing->short_desc);
	}
    }
  if (!newthing)
    {
      if (!thing_free)
	ADD_TO_MEMORY_POOL(THING,thing_free,thing_count);
      newthing = thing_free;
      thing_free = thing_free->next; 
      if (thing_count >= MAX_NUMBER_OF_THINGS)
	{
	  log_it ("TOO MANY THINGS!!!\n\r");
	  exit (1);
	}
    }
  bzero (newthing, sizeof (THING));
  newthing->pc = fake_pc;
  newthing->in = NULL;
  newthing->last_in = NULL;
  newthing->cont = NULL;
  newthing->next = NULL;
  newthing->prev = NULL;
  newthing->proto = NULL;
  newthing->next_cont = NULL;
  newthing->prev_cont = NULL;
  newthing->fgt = NULL;
  newthing->edescs = NULL;
  newthing->next_fight = NULL;
  newthing->prev_fight = NULL;
  newthing->flags = NULL;
  newthing->events = NULL;
  newthing->resets = NULL;
  newthing->values = NULL;
  newthing->fgt = NULL;
  newthing->fd = NULL;
  newthing->needs = NULL;
  newthing->name = nonstr;
  newthing->short_desc = nonstr;
  newthing->long_desc = nonstr;
  newthing->type = nonstr;
  newthing->desc = nonstr;
  newthing->wear_pos = ITEM_WEAR_NONE;
  newthing->wear_loc = ITEM_WEAR_NONE;
  newthing->wear_num = ITEM_WEAR_NONE;
  newthing->max_hp = 10;
  newthing->position = POSITION_STANDING;
  for (i = 0; i < REALDIR_MAX; i++)
    {
      newthing->adj_room[i] = NULL;
      newthing->adj_goodroom[i] = NULL;
    }
  return (newthing);
}

/* This function gets rid of a thing..if its in the world, it is removed,
   if it contains anything, all that stuff gets nuked too. Note there
   is some checking to prevent infinite loops..the things all get
   marked as they are looked at, should prevent infinite looping
   from builders. */

void
free_thing (THING *th)
{
  VALUE *pet;
  THING *basething, *reply, *thg, *thgn;
  bool is_pc = FALSE;
  int vnum = -1;
  char pcname[200];
  if (!th)
    return;
  
  /* Cannot remove prototypes... */
  if (!th || 
      (IS_AREA(th) || 
       th == the_world ||
       (th->in && IS_AREA (th->in) 
#ifdef USE_WILDERNESS
	&& !IS_WILDERNESS_ROOM (th)
#endif
	)))
    return;
  
  
  if (th->fgt)
    {
      remove_fight (th); /* Also does free_fight () */
      th->fgt = NULL;
    }
  
#ifdef USE_WILDERNESS
  /* Add the power of this mob back into the population. */
  update_sector_population_thing (th, TRUE);
#endif
  
  /* Update the society population associated with this thing. */
  update_society_population_thing (th, FALSE);
  
  
  for (reply = thing_hash[PLAYER_VNUM % HASH_SIZE]; reply; reply = reply->next)
    {
      if (IS_PC (reply) && reply->pc->reply == th)
        reply->pc->reply = NULL;
    }
  
  if (th->fd)
    {
      if (th->fd->oldthing)
	do_return (th, "");
      else
	{
	  th->fd->th = NULL;
	  close_fd (th->fd);
	  th->fd = NULL;
	}
    }

  if (IS_WORN(th) && th->in)
    remove_thing (th->in, th, TRUE);
  
  
  remove_thing_from_list (th);
  
  remove_events_from_thing (th); /* Free all of the events on th. */
  
  
  if (IS_PC (th))
    {
      act ("@1n leave@s the world.", th, NULL, NULL, NULL, TO_ROOM);
      is_pc = TRUE;
      strcpy (pcname, th->name);
      if (th->in)
	{
	  for (thg = th->in->cont; thg; thg = thgn)
	    {
	      thgn = thg->next_cont;
	      if (thg != th && (pet = FNV (thg, VAL_PET)) != NULL &&
		  pet->word && !str_cmp (pet->word, NAME (th)))
		{
		  free_thing (thg);
		}
	    }
	}
      free_pc (th->pc);
      th->pc = NULL;
    }
  else if (!IS_ROOM (th))
    {
      vnum = th->vnum; 
      if ((basething = find_thing_num (th->vnum)) != NULL &&
	  basething->mv > 0)
	basething->mv--;
    }
  
  
  remove_from_script_events (th);
  thing_from (th);
  
  remove_from_auctions (th);
  
  thg = th->cont;
  th->cont = NULL;
  
  for (; thg; thg = thgn)
    {
      thgn = thg->next_cont;
      free_thing (thg);
    }
  
  if (is_pc)
    free_thing_final (th, is_pc);
  else
    add_free_thing_event (th);
  
  return;
}

void
free_thing_final_event (THING *th)
{
  free_thing_final (th, FALSE);
  return;
}

/* This function removes all of the data chunks from a thing. The reason
   this is here is because the saving of the world is in a different 
   thread, so if I free a thing and remove it from the world, I don't
   want to mess up the data inside of it right away. I want to keep the
   data intact for a few seconds so that the save thread can finish
   working on it. Then, I can delete it more safely. */

/* NEVER CALL THIS FUNCTION EXCEPT INSIDE OF free_thing() OR IN A
   EVENT_FREE_THING EVENT OR ELSE YOU WILL SCREW UP DATA INSIDE OF
   SOMETHING THAT SHOULD NOT BE SCREWED UP! THIS IS THE SECOND PART OF
   THE FREE_THING PROCESS BECAUSE THERE IS A THREAD FOR SAVING THE WORLD
   SNAPSHOT AND I DON'T WANT TO CORRUPT THE DATA IN ONE THREAD AS IT'S
   BEING WRITTEN TO A FILE IN ANOTHER THREAD. */

void
free_thing_final (THING *th, bool is_pc)
{
  FLAG *flg, *flgn;
  VALUE *val,*valn;
  RESET *rst, *rstn;
  char pcname[200];
  TRACK *track;
  NEED *need, *needn;
  
  if (!th)
    return;


  /* Clean up all of the flags. */
  
  for (flg = th->flags; flg; flg = flgn)
    {
      flgn = flg->next;
      free_flag (flg);
    }  
  th->flags = NULL;
  
  /* Clean up needs. */

  for (need = th->needs; need; need = needn)
    {
      needn = need->next;
      free_need (need);
    }
  th->needs = NULL;
  
  /* Clean up all of the values. */

  for (val = th->values; val; val = valn)
    {
      valn = val->next;
      free_value (val);     
    }
  th->values = NULL;

  /* Clean up all of the resets. (But there really shouldn't be any.) */

  for (rst = th->resets; rst; rst = rstn)
    {
      rstn = rst->next;
      free_reset (rst);
    }  
  th->resets = NULL;
  
  /* Clean up all of the tracks. (But there really shouldn't be any.) */

  while (th->tracks)
    {
      track = th->tracks;
      th->tracks = th->tracks->next;
      free_track (track);
    }
  th->tracks = NULL;
  
  /* Clean up all of the strings on the thing. */

  free_str (th->name);
  free_str (th->short_desc);
  free_str (th->long_desc);
  free_str (th->type);
  free_str (th->desc);
  
  bzero (th, sizeof (THING));
  th->short_desc = nonstr;
  th->long_desc = nonstr;
  th->type = nonstr;
  th->desc = nonstr;
  th->name = nonstr; 

  /* Players get put onto the list of free pcs so that you can track
     them if needed even when they're gone. Don't alter this part or
     they will not be trackable when they quit out. */
  
  if (is_pc)
    {
      th->prev = NULL;
      th->next = thing_free_pc;
      thing_free_pc = th;
      th->name = new_str (pcname);
      pcname[0] = UC (pcname[0]);
      th->short_desc = new_str (pcname);
    }
  else 
    
    /* Other things get put onto this list so that when they quit out,
       we don't have tracks pointing to them that point to the wrong 
       thing when the data structure gets reused. Every once in a while,
       all of the tracks in the game are cleaned up so that the tracks
       pointing to things not in something (like the things on this list)
       all get deleted. Then, this list gets moved to the real free
       list where new things can be picked from. */

    {
      th->prev = NULL;
      th->next = thing_free_track;
      thing_free_track = th;
      thing_free_count++;
      th->name = nonstr;
    }
  return;
}

/* This makes a new flag struct to use, but it first tries to
   grab one from the free list. */

FLAG *
new_flag (void)
{
  FLAG *newflag;
  if (!flag_free)
    ADD_TO_MEMORY_POOL(FLAG,flag_free,flag_count);
  newflag = flag_free;
  flag_free = flag_free->next;
  bzero (newflag, sizeof (FLAG));
  newflag->next = NULL;
  return newflag;
}

/* This just clears a flag and puts it into the free list. */

void
free_flag (FLAG *flg)
{
  if (!flg)
    return;
  flg->next = flag_free;
  flag_free = flg;
  return;
}

/* This makes a new value struct. */

VALUE *
new_value (void)
{
  VALUE *newval;
  
  if (!value_free)
    ADD_TO_MEMORY_POOL(VALUE,value_free,value_count);
  newval = value_free;
  value_free = value_free->next;
  bzero (newval, sizeof (VALUE));
  newval->word = nonstr;
  return newval;
}

/* This takes a value and sets it onto the free list...note that
   we need to get rid of the char * */

void
free_value (VALUE *val)
{
  if (!val)
    return;
  set_value_word (val, "");
  val->next = value_free;
  value_free = val;
  return;
}

/* This clears any current word in a value, and sets it to nonstr if there
   is no no word. Otherwise, it makes a copy of the new word. */

void
set_value_word (VALUE *val, char *word)
{
  if (!val)
    return;
  
  free_str (val->word);
  val->word = nonstr;
  if (word && *word)
    val->word = new_str (word);
  return;
}

/* This finds a random thing in another thing. Only 'mobs' are checked */

THING *
find_random_thing (THING *in)
{
  THING *th;
  int choices = 0, chose = 0, count = 0;
  if (!in)
    return NULL;

  for (th = in->cont; th; th = th->next_cont)
    {
      if (CAN_MOVE (th) || CAN_FIGHT (th))
        choices++;
    }
  if (choices < 1)
    return NULL;

  chose = nr (1, choices);
  for (th = in->cont; th; th = th->next_cont)
    {
      if (CAN_MOVE (th) || CAN_FIGHT (th))
        { 
          if (++count == chose)
            break;
        }
    }
  
  return th;
}

/*  This finds a random area (not start area) */

THING *
find_random_area (int avoid_flags)
{
  THING *ar;
  int num_choices = 0, num_chose = 0;
  int count = 0;

  for (count = 0; count < 2; count++)
    {
      for (ar = the_world->cont; ar; ar = ar->next_cont)
	{
	  if (ar->vnum == START_AREA_VNUM ||
	      (avoid_flags && IS_AREA_SET (ar, avoid_flags)))
	    continue;
	  if (count == 0)	  
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;
	  else
	    num_chose = nr (1, num_choices);
	}
    }  
  return ar;
}

/* All of these functions let things find other things near them.
   It was much easier coding this this way than letting people find
   other people, or objects, or rooms and other crap. It does make
   sense after you look at it for a while. */


THING *
find_thing_me (THING *th, char *arg)
{
  return find_thing_thing (th, th, arg, FALSE);
}

/* This returns a worn item on the thing. */

THING *
find_thing_worn (THING *th, char *arg)
{
  return find_thing_thing (th, th, arg, TRUE);
}



/* Rooms find things in themselves, not in the things they are in. */

THING *
find_thing_in (THING *th, char *arg)
{
  if (!str_cmp (arg, "here") || !str_cmp (arg, "this"))
    return th->in;
  return find_thing_thing (th, (IS_ROOM(th) ? th : th->in), arg, FALSE);
}
  

/* This searches for a thing inside the same room as th, and if it
   does not exist, we search a few rooms out in each direction. */


THING *
find_thing_near (THING *th, char *arg, int range)
{
  THING *vict, *newroom, *curr_room;
  int dir, rng;
  VALUE *exit;
  
  if (!th || !th->in || !arg || range < 0)
    return NULL;
  
  if ((vict = find_thing_in (th, arg)) != NULL)
    return vict;
  
  if (!IS_ROOM (th->in))
    return NULL;

  
  if (range > 0)
    {
      for (dir = 0; dir < REALDIR_MAX; dir++)
	{
	  curr_room = th->in;
	  for (rng = 0;  rng < range; rng++)
	    {
	      if ((exit = FNV (curr_room, dir + 1)) != NULL &&
		  !IS_SET (exit->val[1], EX_CLOSED | EX_WALL) &&
		  (newroom = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM (newroom))
		{
		  if ((vict = find_thing_thing (th, newroom, arg, FALSE)) != NULL)
		    return vict;
		}
	    }
	}
    }
  return NULL;
}

/* This finds an enemy nearby...not necessarily in LOS range. */

THING *
find_enemy_nearby (THING *th, int max_depth)
{
  THING *enemy = NULL, *room, *nroom;
  int i, dir, start_dir;

  if (!th || !th->in)
    return NULL;
  
  if (max_depth < 0)
    max_depth = 0;
  clear_bfs_list();
  undo_marked (th->in);
  add_bfs (NULL, th->in, REALDIR_MAX);
  
  while (bfs_curr)
    { 
      if ((room = bfs_curr->room) != NULL && IS_ROOM (room))
	{
	  for (enemy = room->cont; enemy; enemy = enemy->next_cont)
	    {
	      if (is_enemy (th, enemy))
		break;
	    }
	}
      if (enemy)
	break;
      
      if (bfs_curr->depth > max_depth)
	break;
      start_dir = nr (0, REALDIR_MAX - 1);
      for (i = 0; i < REALDIR_MAX; i++)
	{
	  dir = (i + start_dir) % REALDIR_MAX;
	  if ((nroom = FTR (room, dir, th->move_flags)) != NULL)
	    add_bfs (bfs_curr, nroom, dir);
	}
      bfs_curr = bfs_curr->next;
    }
  clear_bfs_list();
  
  if (enemy && is_enemy (th, enemy))
    return enemy;
  return NULL;
}
	      
      
  

/* This searches for something either in the same place as the thing,
   or inside of the thing. */


THING *
find_thing_here (THING *th, char *arg, bool me_first)
{
  THING *thg;
  if (!arg || !arg[0] || !th)
    return NULL;
  
  if (!str_cmp (arg, "self") || !str_cmp (arg, "me"))
    return th;

  if (!str_cmp (arg, "here") || !str_cmp (arg, "this"))
    return th->in;
  if (!str_cmp (arg, "out") && th->in)
    return th->in->in;
  

  if (me_first)
    {
      if ((thg = find_thing_me (th, arg)) != NULL)
	return thg;
      else
	return find_thing_in (th, arg);
    }
  else
    {
      if ((thg = find_thing_in (th, arg)) != NULL)
	return thg;
      else
	return find_thing_me (th, arg);
    }
  return NULL;
}


THING *
find_thing_world (THING *th, char *arg)
{
  
  THING *thg;
  char *newarg;
  int i = 0, num= 0, count = 0;
  if (!th || !arg || !arg[0])
    return NULL;
  
  if ((thg = find_pc (th, arg)) != NULL)
    return thg;

  if ((thg = find_thing_here (th, arg, TRUE)) != NULL)
    return thg;
  
  newarg = find_first_number (arg, &num);
  /* If the num 0, we assume no proper number argument and go
     with it assuming num = 1. */

  if (num == 0)
    num = 1;

  for (i = 0; i < HASH_SIZE; i++)
    {
      for (thg = thing_hash[i]; thg; thg = thg->next)
	{ 
	  if (!can_see (th, thg) || !thg->in || IS_AREA (thg) ||
	      (IS_AREA(thg->in) && !IS_ROOM (thg)) ||
	      !is_named (thg, newarg) ||
	      /* Only go to mobs or things in rooms. */
	      (!IS_ROOM (thg->in) && !CAN_MOVE (thg) && !CAN_FIGHT (thg)))
	    continue;
	  if (++count == num)
	    return thg;
	}
    }
  return NULL;
} 
  

/* This function finds a PC...if they are online. It should even
   find linkdead people. */

  
THING *
find_pc (THING *th, char *arg)
{
  THING *thg;
 
  if (!th || !arg || !arg[0])
    return NULL;

  for (thg = thing_hash[PLAYER_VNUM % HASH_SIZE]; thg; thg = thg->next)
    {
      if (IS_PC (thg) && !str_cmp (thg->name, arg))
	return thg;
    }
  return NULL;
}

/* This lets the user specify looking for a thing inside of any
   other thing. This way, the other find_thing_* commands are
   a lot easier to code. The basic syntax is that the first
   argument is the thing looking for something else. The second
   argument is where they are looking...usually within themselves
   or in the same thing they are in. The third is the argument
   which they are looking for. The argument checks the keywords
   of all things inside the proper thing to find the correct
   thing to return. It also allows you to "get 4.corpse here" with
   numbers as part of the argument you are looking for. You must
   specify if you want to do worn or unworn eq. */

THING *
find_thing_thing (THING *th, THING *in, char *arg, bool is_worn)
{
  THING *vict;
  int num = -1, count = 0;
  RACE *race = NULL, *align = NULL;
  bool enemy = FALSE;
  
  if (!th || !in || !arg || arg[0] == '\0')
    return NULL;
  
  /* Removed this...will it screw things up? */
  /* if (IS_AREA (in) || in == the_world)
    {
      if (in->vnum == START_AREA_VNUM || in == the_world)
	in = find_random_room (NULL, FALSE, 0, 0);
      else
	in = find_random_room (in, FALSE, 0, 0);
	} */
  
  if (!in)
    return NULL;

  arg = find_first_number (arg, &num);
  
  /* If find_first_number returns no number there..or of the idjit
     typed 0.something, it returns 1.something automatically. */
  
  if (num == 0)
    num = 1;
  
  if (!str_prefix (arg, "enemy"))
    enemy = TRUE;
  else if ((race = find_race (arg, -1)) == NULL)
    align = find_align (arg, -1);
  
  for (vict = in->cont; vict; vict = vict->next_cont)
    {
      if (!can_see (th, vict) || (is_worn != IS_WORN(vict)))
	continue;
      
      if (IS_PC (vict) && IS_PC (th) && DIFF_ALIGN (th->align, vict->align)
	  && LEVEL (th) < BLD_LEVEL && LEVEL (vict) < BLD_LEVEL)
	{
	  if (!enemy && (!align || DIFF_ALIGN (vict->align, align->vnum)) &&
	      (!race || vict->pc->race != race->vnum) &&
	      !is_named (vict, arg))
	    continue;
	}
      else if (!is_named (vict, arg))
	continue;
      
      if (++count == num)
	return vict;
    }
  
  return NULL;
}

      
/* These two things create and remove fights from the game. */

FIGHT *
new_fight (void)
{
  FIGHT *newfgt;
  if (fight_free)
    {
      newfgt = fight_free;
      fight_free = fight_free->next;
    }
  else
    {
      newfgt = (FIGHT *) mallok (sizeof (*newfgt));
      fight_count++;
    }
  bzero (newfgt, sizeof (*newfgt));
  newfgt->hunting[0] = '\0';
  newfgt->old_hunting[0] = '\0';
  newfgt->gleader = NULL;
  newfgt->following = NULL;
  newfgt->mount = NULL;
  newfgt->rider = NULL;
  newfgt->next = NULL;
  newfgt->hunt_victim = NULL;
  newfgt->hunting_timer = 30;
  return newfgt;
}

void
free_fight (FIGHT *fight)
{
  if (!fight)
    return;
  
  bzero (fight, sizeof (*fight));
  fight->next = fight_free;
  fight_free = fight;  
  return;
}
     
/* These two functions create and destroy extra descriptions. */

EDESC *
new_edesc (void)
{
  EDESC *eds;

  if (edesc_free)
    {
      eds = edesc_free;
      edesc_free = eds->next;
    }
  else
    {
      eds = mallok (sizeof (EDESC));
      edesc_count++;
    }
  eds->next = NULL;
  eds->name = nonstr;
  eds->desc= nonstr;
  return eds;
}

/* This frees the strings in an edesc and adds the edesc to the free list. */

void
free_edesc (EDESC *eds)
{
  if (!eds)
    return;
  
  free_str (eds->name);
  eds->name = nonstr;
  free_str (eds->desc);
  eds->desc = nonstr;
  eds->next = edesc_free;
  edesc_free = eds;
  return;
}



/* These two things create and remove fights from the game. */

RESET *
new_reset (void)
{
  RESET *newreset;
  if (reset_free)
    {
      newreset = reset_free;
      reset_free = reset_free->next;
    }
  else
    {
      newreset = (RESET *) mallok (sizeof (*newreset));
      reset_count++;
    }
  bzero (newreset, sizeof (*newreset));
  return newreset;
}

void
free_reset (RESET *reset)
{
  if (!reset)
    return;
  reset->next = reset_free;
  reset_free = reset;
  return;
}
      
/* These two deal with making a new pc_data structure available, and also
   returning it to the free list for use sometime later. */


PCDATA *
new_pc (void)
{
  int i, j;
  PCDATA *newpc;
  if (pc_free)
    {
      newpc = pc_free;
      pc_free = pc_free->next;
    }
  else
    {
      newpc = (PCDATA *) mallok (sizeof (PCDATA));
      pc_count++;
    }
  bzero (newpc, sizeof (PCDATA));
  newpc->email = nonstr;
  newpc->pwd = nonstr;
  newpc->title = nonstr;
  newpc->prompt = nonstr;
  newpc->editing = NULL;
  newpc->reply = NULL;
  newpc->curr_string = NULL;
  newpc->note = NULL;
  newpc->next = NULL;
  newpc->guarding = NULL;
  newpc->qf = NULL;
  newpc->big_string = NULL;
  newpc->big_string_loc = NULL;
  newpc->mapgen = NULL;
  newpc->last_saved = current_time;
  newpc->pets = NULL;
  newpc->in_vnum = 2;
  newpc->wait = 0;
  newpc->off_def = 50;
  newpc->no_quit = 0;
  for (i = 0; i < MAX_STORE; i++)
    newpc->storage[i] = NULL;
  for (i = 0; i < MAX_ALIAS; i++)
    {
      newpc->aliasname[i] = nonstr;
      newpc->aliasexp[i] = nonstr;
    }
  
  for (i = 0; i < REWARD_MAX; i++)
    newpc->society_rewards_thing[i] = NULL;
  /* Set up scrollback stuff. */

  for (i = 0; i < MAX_CHANNEL; i++)
    {
      newpc->sback_pos[i] = 0;
      for (j = 0; j < MAX_SCROLLBACK; j++)
	{
	  newpc->sback[i][j] = NULL;
	}
    }


  for (i = 0; i < MAX_TROPHY; i++)
    {
      newpc->trophy[i] = new_trophy();
      bzero (newpc->trophy[i], sizeof (TROPHY));
    }
  for (i = 0; i < MAX_IGNORE; i++)
    newpc->ignore[i] = nonstr;
  return newpc;
}

void
free_pc (PCDATA *pc)
{
  int i, j;
  VALUE *qf, *qfn;
  THING *pet, *petn;
  if (!pc)
    return;
  /* Clean up any strings */

  free_str (pc->email);
  pc->email = nonstr;
  free_str (pc->pwd);
  pc->pwd = nonstr;
  free_str (pc->title);
  pc->title = nonstr;
  free_str (pc->prompt);
  pc->prompt = nonstr;
  if (pc->big_string)
    free_str (pc->big_string);
  pc->big_string = NULL;
  pc->big_string_loc = NULL;

  for (i = 0; i < MAX_CHANNEL; i++)
    {
      for (j = 0; j < MAX_CHANNEL; j++)
	{
	  free_scrollback(pc->sback[i][j]);
	  pc->sback[i][j] = NULL;
	}
    }

  for (pet = pc->pets; pet; pet = petn)
    {
      petn = pet->next_cont;
      pet->next_cont = NULL;
      free_thing (pet);
    }
  pc->pets = NULL;
  
  for (i = 0; i < MAX_STORE; i++)
    {
      free_thing (pc->storage[i]);
      pc->storage[i] = NULL;
    }
  for (i = 0; i < MAX_ALIAS; i++)
    {
      free_str (pc->aliasname[i]);
      pc->aliasname[i] = nonstr;
      free_str (pc->aliasexp[i]);
      pc->aliasexp[i] = nonstr;
    }
  for (i = 0; i < MAX_IGNORE; i++)
    {
      free_str (pc->ignore[i]);
    }
  for (i = 0; i < MAX_TROPHY; i++)
    {
      if (pc->trophy[i])
	free_trophy (pc->trophy[i]);
      pc->trophy[i] = NULL;
    }
  pc->editing = NULL;
  pc->curr_string = NULL;
  if (pc->note)
    free_note (pc->note);
  pc->note = NULL;
  
  if (pc->mapgen)
    free_mapgen (pc->mapgen);
  pc->mapgen = NULL;

  qf = pc->qf;
  while (qf)
    {
      qfn = qf->next;
      free_value (qf);
      qf = qfn;
    }
  pc->qf = NULL;

  pc->next = pc_free;
  pc_free = pc;
  return;
}


/* This finds the next value in the linked list..good for when there
   are several values of one kind...such as several exits or
   several attacks. */

VALUE *
find_next_value (VALUE *val, int type)
{  
  while (val)
    {
      if (val->type == type)
	return val;
      val = val->next;
    }
  return NULL;
}

/* This adds a timed value to a thing. */

void
add_value_timed (THING *th, VALUE *val, int timer)
{
  if (!th || !val)
    return;
  add_value (th, val);
  
  if (timer > 0)
    val->timer = timer;
  return;
}

/* This adds a value to a thing. */

void
add_value (THING *th, VALUE *val)
{
  VALUE *oldval;
  
  if (!th || !val)
    return;
  
  for (oldval = th->values; oldval; oldval = oldval->next)
    if (oldval->type == val->type)
      break;
  
  /* If you have an old value and it's a VAL_REPLACEBY you don't
     replace the value so you can have chains of things that get
     replaced. Also VAL_MADEOF needs the same treatment so you
     don't keep repeating the same steps over and over. */
  if (oldval)
    {
      if (oldval->type != VAL_REPLACEBY &&
	  oldval->type != VAL_MADEOF)
	remove_value (th, oldval);
      else
	{
	  free_value (val);
	  return;
	}
    }
  /* Minor change. Society values go first just because they
     are used so much. */

  if (th->values && th->values->type == VAL_SOCIETY)
    {
      val->next = th->values->next;
      th->values->next = val;
    }
  else 
    {
      val->next = th->values;
      th->values = val;
    }
  return;
}


 
/* This removes a value from a thing and then frees it. */

void
remove_value (THING *th, VALUE *remove_me)
{
  VALUE *prev;
  if (!th || !th->values || !remove_me)
    return;
  
  if (remove_me == th->values)
    {
      th->values = th->values->next;
      remove_me->next = NULL;
      free_value (remove_me);
      return;
    }
  else
    {
      for (prev = th->values; prev; prev = prev->next)
	{
	  if (prev->next == remove_me)
	    {
	      prev->next = remove_me->next;
	      remove_me->next = NULL;
	      free_value (remove_me);
	      return;
	    }
	}
    }
  return;
}

/* These add and remove edescs from things. */

void 
add_edesc (THING *th, EDESC *eds)
{
  if (!th || !eds)
    return;
  
  eds->next = th->edescs;
  th->edescs = eds;
  return;
}

void
remove_edesc (THING *th, EDESC *eds)
{
  EDESC *prev;
  
  if (!th || !eds)
    return;

  if (eds == th->edescs)
    {
      th->edescs = eds->next;
      eds->next = NULL;
    }
  else
    {
      for (prev = th->edescs; prev; prev = prev->next)
	{
	  if (prev->next == eds)
	    {
	      prev->next = eds->next;
	      eds->next = NULL;
	      break;
	    }
	}
    }
  return;
}

/* This finds an edesc with a certain name on a thing. */

EDESC *
find_edesc_thing (THING *th, char *name, bool fullname)
{
  EDESC *eds;
  
  if (!th || !name || !*name || !th->edescs)
    return NULL;
  
  for (eds = th->edescs; eds; eds = eds->next)
    {
      if (fullname)
	{
	  if (!str_cmp (eds->name, name))
	    return eds;
	}
      else if (named_in (eds->name, name))
	return eds;
    }
  return NULL;
}



/* This returns the next flag in the list...may be useful? */

FLAG *
find_next_flag (FLAG *start, int type)
{
  FLAG *flag;
  if (!start || type < 0)
    return NULL;
  for (flag = start; flag; flag = flag->next)
    {
      if (flag->type == type)
	return flag;
    }
  return NULL;
}

/* Copy values from one thing to another. Do not make multiple copies of
   values appearing in both. Overwrite duplicate values with the copy
   from the old thing. */


void
copy_values (THING *tho, THING *thn)
{
  VALUE *oldval, *newval;
  for (oldval = tho->values; oldval; oldval = oldval->next)
    {
      newval = new_value ();
      copy_value (oldval, newval);
      add_value (thn, newval);
    }
  return;
}

void
copy_resets (THING *tho, THING *thn)
{
  RESET *oldr, *newr, *currr = NULL;
  for (oldr = tho->resets; oldr; oldr = oldr->next)
    {
      newr = new_reset ();
      copy_reset (oldr, newr);
      
      newr->next = NULL;
      if (currr == NULL)
	{
	  currr = newr;
	  thn->resets = newr;
	  newr->next = NULL;
	}
      else
	{
	  currr->next = newr;
	  newr->next = NULL;
	  currr = newr;
	}
    }
  return;
}

void
copy_reset (RESET *oldr, RESET *newr)
{
  if (!oldr || !newr || oldr == newr)
    return;
  
  newr->pct= oldr->pct;
  newr->times = oldr->times;
  newr->vnum = oldr->vnum;
  newr->nest = oldr->nest;
  return;
}

void
copy_flags (THING *tho, THING *thn)
{
  FLAG *oldflag, *newflag;
  for (oldflag = tho->flags; oldflag; oldflag = oldflag->next)
    {
      if (oldflag->type < NUM_FLAGTYPES && oldflag->timer == 0)
	add_flagval (thn, oldflag->type, oldflag->val);
      else
	{
	  newflag = new_flag ();
	  copy_flag (oldflag, newflag);
	  aff_update (newflag, thn);
	}
    }
  return;
}

void
copy_value (VALUE *old, VALUE *newval)
{
  int i;
  if (!old || !newval)
    return;
  newval->type = old->type;
  for (i = 0; i < NUM_VALS; i++)
    newval->val[i] = old->val[i];
  set_value_word (newval, old->word);
  return;
}

void 
copy_flag (FLAG *old, FLAG *newflag)
{
  if (!old || !newflag)
    return;
  newflag->type = old->type;
  newflag->from = old->from;
  newflag->timer = old->timer;
  newflag->val = old->val;
  newflag->next = NULL;
  return;
}

char *
set_names_string (char *old)
{
  static char newstring[STD_LEN];
  newstring[0] = '\0';

  if (!old)
    return nonstr;
  strcpy (newstring, old);
  return newstring;
}



/* This creates a new copy of a thing that already exists.
   If we cannot find something in the thing lists which is not "in" something,
   then we know that we cannot make this item. Because: any things
   which are "in" something are items already in the game, or
   they are rooms (only one copy allowed so no more) or areas.
   This way we don't clone things that shouldn't be cloned. */

THING *
create_thing (int vnum)
{
  THING *tho, *thn;
  VALUE *light;
  if ((tho = find_thing_num (vnum)) == NULL)
    return NULL;
  
  /* Do not allow multiple copies of areas or rooms or the world. 
     Also don't allow mobgen mobs to be directly created. They are
     prototypes for other mob prototypes... :) */

  if (tho == the_world || IS_SET (tho->thing_flags, TH_IS_AREA | TH_IS_ROOM))
    return NULL;
  
  if (vnum >= GENERATOR_NOCREATE_VNUM_MIN &&
      vnum <= GENERATOR_NOCREATE_VNUM_MAX)
    return NULL;
  

  /* Check if there are too many of this thing around... We allow
     this to pass since we don't want to make crap items floating 
     around...the main thing is that we don't clone rooms or areas. */
  
  if (tho->max_mv > 0 && tho->mv >= tho->max_mv)
    {
      char errbuf[STD_LEN];
      sprintf (errbuf, "Tried to make too many of vnum %d\n", vnum);
      log_it (errbuf);
    }
  tho->mv++;
  
  /* If we have an old thing to clone, we do it now. */

  thn = new_thing ();
  copy_thing (tho, thn);
  
  set_up_thing (thn);
  add_thing_to_list (thn);
  if ((light = FNV (thn, VAL_LIGHT)) != NULL 
      && ((light->val[1] == 0 || light->val[0] > 0) &&
	  IS_SET (light->val[2], LIGHT_LIT)))
    thn->light++;
  set_up_move_flags (thn);
  return thn;
}

/* This copies a thing into a new thing. It does not "customize" it
   though. */

void
copy_thing (THING *tho, THING *thn)
{
  char *o, *n;
  int sz;
  FLAG *oldflag, *newflag;
  if (!tho || !thn)
    return;
  /* Memcpy sort of. */
  sz = sizeof (*tho);
  n = (char *) thn;
  o = (char *) tho;
  while (o - (char *) tho < sz)
    *n++ = *o++;
  
  /* This copied all the crap info. We  MUST COPY
     THE STRINGS AND POINTERS CORRECTLY NOW OR ELSE THE PROGRAM WILL
     PUKE AND DIE!!!! */
  
  /* Copy strings! */
  thn->name = nonstr;
  thn->short_desc = nonstr;
  thn->long_desc = nonstr;
  thn->type = nonstr;
  thn->desc = nonstr;
  thn->wear_pos = tho->wear_pos;
  thn->wear_loc = ITEM_WEAR_NONE;
  thn->wear_num = 0;
  thn->in = NULL;
  thn->cont = NULL;
  thn->prev = NULL;
  thn->next = NULL;
  thn->next_cont = NULL;
  thn->prev_cont = NULL;
  thn->next_fight = NULL;
  thn->prev_fight = NULL;
  thn->edescs = NULL;
  thn->proto = tho;
  /* Now fix up pointers! */
  
  if (tho->fgt)
    {
      add_fight (thn); 
      copy_fight (thn->fgt, tho->fgt);
    }
  thn->flags = NULL;
  for (oldflag = tho->flags; oldflag; oldflag = oldflag->next)
    {
      newflag = new_flag();
      copy_flag (oldflag, newflag);
      newflag->next = thn->flags;
      thn->flags = newflag;
    }
  thn->values = NULL;
  copy_values (tho, thn);
  thn->resets = NULL;
  /*  copy_resets (tho, thn);*/
  thn->pc = NULL;
  
  return;
}

/* This copies the contents of a fight struct to another one. */

void
copy_fight (FIGHT *nw, FIGHT *ol)
{
  if (!nw || !ol)
    return;
  
  nw->fighting = ol->fighting;
  nw->rider = ol->rider;
  nw->mount = ol->mount;
  nw->following = ol->following;
  nw->gleader = ol->gleader;
  nw->hunt_victim = ol->hunt_victim;
  nw->delay = ol->delay;
  nw->bashlag = ol->bashlag;
  nw->hunting_timer = ol->hunting_timer;
  nw->hunting_failed = ol->hunting_failed;
  nw->old_hunting_type = ol->old_hunting_type;
  strncpy (nw->hunting, ol->hunting, HUNT_STRING_LEN);
  strncpy (nw->old_hunting, ol->old_hunting, HUNT_STRING_LEN);
  return;
}
  
 

/* This customizes a thing like setting hps and stuff. */

void
set_up_thing (THING *th)
{
  THING *proto;
  if (!th)
    return;
  
  
  if ((proto = find_thing_num (th->vnum)) == NULL)
    return;
  
  th->max_hp = LEVEL(th)+1;

  /* Hps start to go up quadratically at level 40+...*/
  if (LEVEL(th) >= 40)    
    th->max_hp += (LEVEL(th)-40)*(LEVEL(th)-40)/5;
  
  
  /* Hpmult, 10 = normal, 2 = .2 hps, 100 = 10x hps..max dunno why,
     I guess to keep really stupid cheater mobs from being created. */
  
  if (proto->max_hp < 2)
    proto->max_hp = 10;
  th->max_hp = th->max_hp * MID (2, proto->max_hp, 100)/10;
  th->max_hp += nr (0, th->max_hp/10);
  if (LEVEL(th) >= 100) /* Hp bonus for ultra-high levels */
    {
      th->max_hp += (LEVEL(th)-100)*th->max_hp/300;
    }
  th->hp = th->max_hp;
  th->max_mv = (LEVEL (th) * 10) + 300;
  th->mv = th->max_mv;
  th->cost = nr (th->cost *9/10, th->cost *11/10);

  /* Things that can talk should have money. */
  if (CAN_TALK (th) && total_money (th) == 0 && LEVEL(th) >= 30)
    {
      add_money (th, nr (1, LEVEL(th)*3/2));
    }
  if (total_money (th) > 0)
    {
      sub_money (th, nr (0, total_money (th)/ 2));
      add_money (th, nr (0, total_money (th) / 2));
    }
  
  /* Armor for mobs/objs is calced differently. Mobs get a lot more,
     since they fight, and objs get very little since it is used for 
     breaking wpns etc... mostly. */
  
  
  
  if (th->armor == 0)
    {
      if (CAN_MOVE (th) || CAN_FIGHT (th))
	th->armor = nr (LEVEL (th)/2, LEVEL (th));
      else
	th->armor = MID (10, LEVEL (th)/2, 40);
    }
  

  return;
}


void
do_make (THING *th, char *arg)
{
  int vnum = 0, num_made = 1, i;
  THING *made, *going_to, *base_item = NULL;
  char arg1[STD_LEN];
  char buf[STD_LEN];
  VALUE *rnd = NULL;
  if (!th || !IS_PC (th) || LEVEL (th) < BLD_LEVEL)
    {
      stt ("Huh?\n\r", th);
      return;
    }
  arg = f_word (arg, arg1);
     
  if (!str_cmp (arg, "this") || !str_cmp (arg1, "this"))
    {
      if (th->pc->editing &&
	  th->fd && th->fd->connected == CON_EDITING)
	vnum = ((THING *) th->pc->editing)->vnum;
    }
  else if ((vnum = atoi (arg)) < 1)
    {
      if ((vnum = atoi(arg1)) < 1)
	{
	  stt ("Syntax make <num_copies> <vnum>\n\r", th);
	  return;
	}
    }
  else /* Can only make 100 of something */
    num_made = MID(1, atoi (arg1), 100);
  
  if (vnum >= GENERATOR_NOCREATE_VNUM_MIN &&
      vnum <= GENERATOR_NOCREATE_VNUM_MAX)
    {
      sprintf (buf, "You cannot make items with vnums between %d and %d. These items are used to generate other items internally.\n\r",
	       GENERATOR_NOCREATE_VNUM_MIN, GENERATOR_NOCREATE_VNUM_MAX);
      stt (buf, th);
      return;
    }
  
  if ((base_item = find_thing_num (vnum)) != NULL)
    rnd = FNV (base_item, VAL_RANDPOP);
  

  for (i = 0; i < num_made; i++)
    {
      if (rnd)
	vnum = calculate_randpop_vnum (rnd, LEVEL(th));
      
      if ((made = create_thing (vnum)) == NULL)
	{
	  stt ("That thing doesn't exist and cannot be made.\n\r", th);
	  continue;
	}
      
      going_to = th;
      if (IS_SET (th->thing_flags, TH_NO_CONTAIN | TH_NO_CAN_GET) ||
	  !IS_SET (made->thing_flags, TH_NO_MOVE_SELF) ||
	  IS_SET (made->thing_flags, TH_NO_MOVE_BY_OTHER | TH_NO_DROP | TH_NO_TAKE_BY_OTHER))
	{
	  if (th->in)
	    {
	      if (IS_SET (th->in->thing_flags, TH_NO_CONTAIN | TH_NO_GIVEN))
		{
		  stt ("This thing cannot go anywhere!!\n\r", th);
		  free_thing (made);
		  return;
		}
	      else
		thing_to (made, th->in);
	    }
	  else
	    {
	      stt ("This thing cannot go anywhere!\n\r", th);
	      free_thing (made);
	      return;
	    }
	}
      else
	thing_to (made, th);
      
      made->wear_loc = ITEM_WEAR_NONE;
      made->wear_num = 0;
      reset_thing (made, 0);
      act ("@1n create@s @3n.", th, NULL, made, NULL, TO_ALL);
    }
  return;
}

void
do_sacrifice (THING *th, char *arg)
{
  THING *obj, *altar = NULL, *dagger = NULL, *prison = NULL, *vict = NULL;

  if (!th->in)
    return;
  

  if (!arg || !*arg)
    {
      stt ("sacrifice <thing>\n\r", th);
      return;
    }



  if ((obj = find_thing_in (th, arg)) == NULL ||
      obj->vnum == SOUL_ALTAR)
    {
      bool failed = FALSE;
      if (str_cmp (arg, "soul"))
	{
	  stt ("You don't see that here!\n\r", th);
	  return;
	}
      for (altar = th->in->cont; altar; altar = altar->next_cont)
	if (altar->vnum == SOUL_ALTAR)
	  break;
      
      if (!altar)
	{
	  stt ("There is no soul altar here!\n\r", th);
	  return;
	}
      
      for (prison = altar->cont; prison; prison = prison->next_cont)
	if (prison->vnum == SOUL_PRISON)
	  break;
      
      if (!prison)
	{
	  stt ("The soul altar does not contain a soul prison!\n\r", th);
	  return;
	}
      
      for (vict = prison->cont; vict; vict = vict->next_cont)
	if (IS_PC (vict) && IS_PC (th) && DIFF_ALIGN (th->align, vict->align))
	  break;
      
      if (!vict)
	{
	  stt ("The soul prison is empty!\n\r", th);
	  return;
	}
	
      for (dagger = th->cont; dagger; dagger = dagger->next_cont)
	if (dagger->vnum == SOUL_DAGGER)
	  break;
      
      if (!dagger)
	{
	  stt ("You are not wielding the soul dagger!\n\r", th);
	  return;
	}
      
      if (!check_spell (th, "sacrifice", 0))
	{
	  stt ("You failed to sacrifice the victim!\n\r", th);
	  failed = TRUE;
	}
      
      if (!failed)
	{
	  if (nr (1,2) == 2)
	    {
	      act ("$F@1n tries to sacrifice @3n but the gods grant @3n mercy and send @3m home!$7", th, NULL, vict, NULL, TO_ALL);
	      stt ("You have been spared by the gods!\n\r", vict);
	      thing_to (vict, find_thing_num (vict->align + 100));
	      remove_flagval (vict, FLAG_PC1, PC_FREEZE);
	    }
	  else
	    {
	      act ("$9@1n sacrifices @3n to the gods! Blood everywhere!$7", th, NULL, vict, NULL, TO_ALL);
	      thing_to (vict, th->in);
	      SBIT (server_flags, SERVER_SACRIFICE);
	      get_killed (vict, th);
	      RBIT (server_flags, SERVER_SACRIFICE);
	    }
	}
      
      free_thing (prison);
      free_thing (dagger);
      return;

    }
  if (IS_SET (obj->thing_flags, TH_IS_ROOM | TH_IS_AREA |
      TH_NO_TAKE_BY_OTHER | TH_NO_MOVE_BY_OTHER) ||
      CAN_FIGHT (obj) || CAN_MOVE (obj) ||
      IS_PC (obj))
    {
      stt ("You can't sacrifice that!\n\r", th);
      return;
    }

  act ("@1n sacrifice@s @3n to Dog!", th, NULL, obj, NULL, TO_ALL);
  free_thing (obj);
  return;
}
   



/* This function unmarks all areas. */


void
unmark_areas (void)
{
  THING *ar;
  for (ar = the_world->cont; ar; ar = ar->next_cont)
    RBIT (ar->thing_flags, TH_MARKED);
  return;
}

/* This purges a room area or the whole world of all mobjects. */


void
do_purge (THING *th, char *arg)
{
  THING *room, *area, *start_in;
  if (th && LEVEL (th) < BLD_LEVEL)
    {
      stt ("Huh?\n\r", th);
      return;
    } 
  
  if (th && !IS_PC (th))
    {
      stt ("Only PC's can purge things!\n\r", th);
      return;
    }
  if (!str_cmp (arg, "all") || !str_cmp (arg, "world"))
    {
      for (area = the_world->cont; area; area = area->next_cont)
	{
	  for (room = area->cont; room; room = room->next_cont)
	    {
	      if (!IS_ROOM (room))
		continue;
	      purge_room (room);
	    }
	}
      society_clear_recent_maxpop (NULL);
      
      stt ("Ok, the whole world has been purged.\n\r", th);
      return;
    }
  
  if (!th->in)
    {
      stt ("You aren't in anything!\n\r", th);
      return;
    }
  if (th->in == the_world || IS_AREA(th->in))
    {
      stt ("You cannot purge an area or the world!\n\r", th);
      return;
    }

 
  if (str_cmp (arg, "area"))
    {
      purge_room (th->in);
      act ("@1n wave@s @1s hand and cleanse@s the entire area with holy fire!",
	   th, NULL, NULL, NULL, TO_ALL);
      return;
    }
  
  if ((start_in = th->in) == NULL || !IS_ROOM (start_in) ||
      (area = start_in->in) == NULL || !IS_AREA (area))
    {
      stt ("You must be in a room to purge an area.\n\r", th);
      return;
    }

  

  for (room = area->cont; room != NULL; room = room->next_cont)
    {
      if (!IS_ROOM (room))  
	continue;
      purge_room (room);
    }
  society_clear_recent_maxpop (area);
  stt ("You purge the whole area!\n\r", th);
  return;
}

void
purge_room (THING *room)
{
  THING *thg, *thgn;
  VALUE *money;
  
  if (!room)
    return;
  
  for (thg = room->cont; thg; thg = thgn)
    {
      thgn = thg->next_cont;
      if (!IS_PC (thg))
	free_thing (thg);
    }
  if ((money = FNV (room, VAL_MONEY)) != NULL)
    {
      remove_value (room, money);
    }
  return;
}


/* This takes a thing and replaces it within the world, keeping fighting,
   and tracking etc working...hopefully. */

bool
replace_thing (THING *told, THING *tnew)
{
  THING *th, *obj, *objn;
  
  char arg1[STD_LEN];
  if (!told || !tnew || IS_PC (told) || IS_PC (tnew) || 
      !told->in || IS_AREA (told->in))
    {
      if (tnew && !IS_PC (tnew))
	free_thing (tnew);
      return FALSE;
    }
  
  /* Move objects */
  

  for (obj = told->cont; obj != NULL; obj = objn)
    {
      objn = obj->next_cont;
      thing_from (obj);
      obj->wear_loc = ITEM_WEAR_NONE;
      obj->wear_num = 0;
      thing_to (obj, tnew);
    }
  
  /* Move fighting */
  
  for (th = told->in->cont; th != NULL; th = th->next_cont)
    {
      if (th->fgt)
	{
	  if (th->fgt->fighting == told)
	    th->fgt->fighting = tnew;
	  if (th->fgt->following == told)
	    th->fgt->following = tnew;
	  if (th->fgt->gleader == told)
	    th->fgt->gleader = tnew;
	  if (th->fgt->mount == told)
	    th->fgt->mount = tnew;
	  if (th->fgt->rider == told)
	    th->fgt->rider = tnew;
	}
    }
  
  /* Update hunting */
  

  for (th = thing_hash[PLAYER_VNUM % HASH_SIZE]; th != NULL; th = th->next)
    {
      if (IS_PC (th) && th->fgt &&
	  th->fgt->hunt_victim == told)
	{ 
	  f_word (KEY(tnew), arg1);
	  start_hunting (th, arg1, HUNT_KILL);
	  th->fgt->hunt_victim = tnew;
	} 
    }
  
  /* Move money */
  
  if (total_money (told) > 0)
    {
      add_money (tnew, total_money (told));
    }
  
  
  copy_flags (told, tnew);
  copy_values (told, tnew);
  copy_needs (told, tnew);

  /* Now re setup the new thing in case the old thing was higher
     level than it should have been...*/

  if (told->proto && LEVEL(told) > LEVEL(told->proto))
    {
      tnew->level += (LEVEL(told) - LEVEL(told->proto));
      set_up_thing (tnew);
    }
  
  /* When replacing items, if the old item had a timer, if the
     old and new items have timers, just update the timer. */
  if (told->timer > 0 && tnew->timer > 0)
    tnew->timer = told->timer;
  /* Otherwise, the new item gets its regular timer. */
  else
    tnew->timer = tnew->proto->timer;
  thing_to (tnew, told->in);
  free_thing (told);
  return TRUE;
}

/* Changed this to a doubly linked list to speed up removal of old items,
   since it can be kind of slow (O (N^2)) if we have N of a certain item. */

void 
add_thing_to_list (THING *th)
{
  int num;
  THING *prev;
  if (!th)
    return;
  
  num = th->vnum % HASH_SIZE;
  
  if (!th || th->prev || th->next || thing_hash[num] == th)
    return;
  
  
  /* This is important. All things that are in areas ( like rooms and
     prototypes) must be put at the FRONT of the thing_hash lists.
     If you don't do this, find_thing_num will not work properly and
     you will be hating life. */
  
  /* If the list head is empty or it's not in an area, we put this
     item here first. */
  if(!thing_hash[num] || 
     !thing_hash[num]->in  || 
     !IS_AREA (thing_hash[num]->in) ||
     (th->in && IS_AREA (th->in)))
    {
      th->prev = NULL;
      th->next = thing_hash[num];
      if (thing_hash[num])
	thing_hash[num]->prev = th;
      thing_hash[num] = th; 
    }
  /* Down here, the first item is in an area and this item isn't
     so it gets put into the list after the stuff that's inside of
     areas. */
  else
    {
      for (prev = thing_hash[num]; prev != NULL; prev = prev->next)
	{
	  if (!prev->next || !prev->next->in || !IS_AREA (prev->next->in))
	    {
	      th->next = prev->next;	      
	      if (prev->next)
		prev->next->prev = th;
	      prev->next = th;
	      th->prev = prev;
	      break;
	    }
	}
    }
  
  /* Now add the update events the thing will need. This is done in 
     every case except where a nonroom is inside of an area. */
  
  if (th->in && IS_AREA (th->in) && !IS_ROOM (th))
    return;  
  add_events_to_thing (th);
  return;
}

/* This removes a thing from the hash table. */

void
remove_thing_from_list (THING *th)
{
  int num;
  if (!th)
    return;

  num = th->vnum % HASH_SIZE;
  
  if (thing_hash_pointer == th)
    thing_hash_pointer = th->next;
  
  if (th == thing_hash[num])
    {
      th->prev = NULL;
      thing_hash[num] = th->next;
    }
  if (th->next)
    th->next->prev = th->prev;
  if (th->prev)
    th->prev->next = th->next;
  
  th->prev = NULL;
  th->next = NULL;
  
  
  return;
}


/* Create and destroy trophies...no saving here since these only
   get made/destroyed when a pc enters/leaves (infrequently). */

TROPHY *
new_trophy (void)
{
  TROPHY *newtrop;
  
  if (trophy_free)
    {
      newtrop = trophy_free;
      trophy_free = trophy_free->next;
    }
  else 
    newtrop = (TROPHY *) mallok (sizeof (*newtrop));
  bzero (newtrop, sizeof (*newtrop));
  newtrop->name[0] = '\0';
  return newtrop;
}

void
free_trophy (TROPHY *trop)
{
  if (!trop)
    return;
  
  trop->next = trophy_free;
  trophy_free = trop;
  
  return;
}


/* This looks for a thing. Either it is an area, or a room, or it is
   one of the "prototypes" of objs and mobs and other things which
   allow you to edit things and look at base cases of things. */

THING *
find_thing_num (int num)
{
  THING *thg;
  if (num < 1) 
    return NULL;
  
  if ((thg = thing_hash[num % HASH_SIZE]) == NULL)
    return NULL;
  
  do
    {
      if (thg->vnum == num)
	return thg;
      thg = thg->next;
    }
  /* This makes a BIG assumption. I assume that the things that are 
     protos are at the start of the thing hash lists and if not
     we are scrood...this will then travel down the entire list and
     start finding random things. */
  while (thg && thg->in && IS_AREA (thg->in));
  
  
  
#ifdef USE_WILDERNESS
  if (num >= WILDERNESS_MIN_VNUM)
    return make_wilderness_room (num);
#endif
  return NULL;
}



/* This finds a random marked (or unmarked) room in an area. It also allows
   you to specify room flags that the room needs or needs to avoid. */

THING *
find_random_room (THING *area, bool marked, int need_flags, int avoid_flags)
{
  THING *room, *ar;
  int count, num_choices = 0, num_chose = 0;
  int room_flags;


  if (!the_world || !the_world->cont ||
      (ar = the_world->cont->next_cont) == NULL)
    return NULL;
  if (area)
    ar = area;
  else
    ar = find_random_area (0);
  
  if (!ar || !IS_AREA (ar))
    return NULL;
  
  for (count = 0; count < 2; count++)
    {
      for (room = ar->cont; room; room = room->next_cont)
	{
	  if (!IS_ROOM (room) ||
	      (IS_MARKED (room) && !marked) ||
	      (!IS_MARKED (room) && marked))
	    continue;
	  
	  room_flags = flagbits (room->flags, FLAG_ROOM1);
	  	  
	  if ((need_flags && !IS_SET (room_flags, need_flags)) ||
	      (avoid_flags && IS_SET (room_flags, avoid_flags)))
	    continue;
	  
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;
	  num_chose = nr (1, num_choices);
	}
    }
  return room;
}

/* This makes a new room in a certain direction if the area that
   the original room is in has more space for rooms, and if
   the dir is ok and if there isn't already an exit in that
   direction and if the name given exists. */
	    
THING *
make_room_dir (THING *start_room, int dir, char *name, int room_flags)
{
  THING *room, *area;
  VALUE *exit;
  int curr_vnum = 0;
  
  if (!start_room || !IS_ROOM(start_room) || 
      dir < 0 || dir >= REALDIR_MAX ||
      !name || !*name ||
      (exit = FNV (start_room, dir + 1)) != NULL)
    return NULL;
  
  if ((area = find_area_in (start_room->vnum)) == NULL ||
      !IS_AREA (area))
    return NULL;
  
  curr_vnum = area->vnum;
  
  for (room = area->cont; room; room = room->next_cont)
    {
      if (IS_ROOM (room) && room->vnum > curr_vnum)
	curr_vnum = room->vnum;
    }
  
  curr_vnum++;

  if (curr_vnum > area->vnum + area->mv ||
      (room = find_thing_num (curr_vnum)) != NULL)
    return NULL;
  
  if ((room = new_thing()) == NULL)
    return NULL;
  area->thing_flags |= TH_CHANGED;
  room->vnum = curr_vnum;
  if (curr_vnum > top_vnum)
    top_vnum = curr_vnum;
  room->thing_flags = ROOM_SETUP_FLAGS;
  thing_to (room, area);
  add_thing_to_list (room);
  if (room_flags)
    add_flagval (room, FLAG_ROOM1, room_flags);
  
  /* Set up the exits. */
  if ((exit = FNV (start_room, dir + 1)) == NULL)
    {
      exit = new_value();
      exit->type = dir + 1;
      add_value (start_room, exit);
    }
  exit->val[0] = room->vnum;

  if ((exit = FNV (room, RDIR(dir) + 1)) == NULL)
    {
      exit = new_value();
      exit->type = RDIR(dir) + 1;
      add_value (room, exit);
    }
  exit->val[0] = start_room->vnum;
  
  free_str (room->short_desc);
  room->short_desc = new_str (name);
  return room;
}

  
/* This adds a reset to a thing based on the information given. */

void
add_reset (THING *to, int vnum, int pct, int max_num, int nest)
{
  RESET *reset, *newreset;
  
  if (!to || vnum < 1)
    return;
  
  /* Don't allow more than one reset of the same vnum on the
     same thing. */
  
  for (reset = to->resets; reset; reset = reset->next)
    {
      if (reset->vnum == vnum)
	break;
    }
  
  if (reset)
    newreset = reset;
  else
    newreset = new_reset();
  newreset->vnum = vnum;
  newreset->pct = MID (1, pct, 100);
  newreset->times = MID (1, max_num, 10000);
  newreset->nest = MID (1, nest, MAX_NEST-1);
  
  /* This an addition. If the world is set to be autogenerated and if
     the world is currently being generated, then all resets are MUCH
     higher percentages since this world is assumed to be a "pk fun" 
     world so the percents don't really matter. */

  if (IS_SET (server_flags, SERVER_AUTO_WORLDGEN) &&
      IS_SET (server_flags, SERVER_WORLDGEN))
    {
      newreset->pct += (100 - newreset->pct)/2;
    }
  
  if (reset == newreset)
    return;
  
  newreset->next= NULL;
  if (!to->resets)
    to->resets = newreset;
  else
    {
      for (reset = to->resets; reset; reset = reset->next)
	{
	  if (!reset->next)
	    {
	      reset->next = newreset;
	      break;
	    }
	}
    }
  return;
}

/* This removes all resets within the specified vnum range
   from all things inside of whatever is listed. */

void
remove_resets (THING *from, int min_vnum, int max_vnum)
{
  RESET *prev, *rst;
  THING *obj;

  if (min_vnum > max_vnum)
    return;

  if (!from)
    from = the_world;
  
  /* Remove all resets at the front of the lists. */
  
  while (from->resets &&
	 (from->resets->vnum >= min_vnum &&
	  from->resets->vnum <= max_vnum))
    {
      rst = from->resets;
      from->resets = from->resets->next;
      free_reset (rst);
    }

  /* Now get all of the "middle" resets. */

  for (prev = from->resets; prev; prev = prev->next)
    {
      if (prev->next &&
	  prev->next->vnum >= min_vnum &&
	  prev->next->vnum <= max_vnum)
	{
	  rst = prev->next;
	  prev->next = rst->next;
	  free_reset (rst);
	}
    }
  
  /* Now get all of the contents. */

  for (obj = from->cont; obj; obj = obj->next_cont)
    remove_resets (obj, min_vnum, max_vnum);
  return;
}


/* This marks all rooms nearby a given room. Must call
   clear_bfs_list() OUTSIDE Of this function or the rooms will 
   remain marked! */

void
mark_rooms_nearby (THING *room, int max_depth)
{
  THING *croom, *nroom;
  VALUE *exit;
  BFS *bfs;
  
  int dir;
  if (!room || max_depth < 1 || !IS_ROOM (room))
    return;
  

  clear_bfs_list();
  add_bfs (NULL, room, REALDIR_MAX);
  
  while (bfs_curr)
    {
      if ((croom = bfs_curr->room) != NULL &&
	  bfs_curr->depth < max_depth)
	{
	  for (dir = 0; dir < REALDIR_MAX; dir++)
	    {
	      if ((exit = FNV (croom, dir + 1)) != NULL &&
		  (nroom = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM (nroom))
		{
		  add_bfs (bfs_curr, nroom, dir);
		}
	    }
	}
      bfs_curr = bfs_curr->next;
    }
  
  /* Now remove all of the room references in the list and clear it,
     leaving a block of marked rooms. */
  
  for (bfs = bfs_list; bfs; bfs = bfs->next)
    bfs->room = NULL;
  
  clear_bfs_list();

  return;
}


/* This finds the number of rooms that are reachable from a certain room
   with certain room flags. */

int
find_num_adjacent_rooms (THING *room, int room_flags)
{
  VALUE *exit;
  THING *nroom;
  int dir, total_rooms = 0;
  
  if (IS_SET (room->thing_flags, TH_MARKED) ||
      (room_flags && !IS_ROOM_SET (room, room_flags)))
    return 0;

  SBIT (room->thing_flags, TH_MARKED);

  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      if ((exit = FNV (room, dir + 1)) != NULL &&
	  (nroom = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (nroom))
	total_rooms += find_num_adjacent_rooms (nroom, room_flags);
    }
  
  return total_rooms + 1;
}


/* This appends a name to a thing. */

void
append_name (THING *th, char *name)
{
  char buf[STD_LEN];
  
  if (!th || !name || !*name)
    return;
  
  if (th->name && *th->name)
    {
      strcpy (buf, th->name);
      strcat (buf, " ");
    }
  else
    buf[0] = '\0';
  
  strcat (buf, name);
  free_str (th->name);
  th->name = new_str (buf);
}

/* This makes and sets up a new room with this vnum. */

THING *
new_room (int vnum)
{
  THING *room;
  THING *area;
  
  if ((area = find_area_in (vnum)) == NULL)
    return NULL;
  if ((room = find_thing_num (vnum)) != NULL)
    {
      if (IS_ROOM (room))
	return room;
      else
	return NULL;
    }
  if ((room = new_thing ()) == NULL)
    return NULL;

  room->vnum = vnum;
  add_thing_to_list(room);
  thing_to (room, area);
  room->thing_flags = ROOM_SETUP_FLAGS;
  area->thing_flags |= TH_CHANGED;
  return room;
}


/* This is a diagnostic function admins can use to get info about
   things. Add more diagnostics as you feel you need them. */

void
do_thing (THING *th, char *arg)
{
 
  
  if (!th || LEVEL (th) < MAX_LEVEL || !IS_PC (th))
    return;

  if (!str_cmp (arg, "gap") || !str_cmp (arg, "made") || !str_cmp (arg, "max") || !str_cmp (arg, "count"))
    {
      int made = 0;
      int vnum = 0;
      THING *area, *proto;
      char buf[STD_LEN];
      for (area = the_world->cont; area; area = area->next_cont)
	{
	  for (proto = area->cont; proto; proto = proto->next_cont)
	    {
	      if (proto->mv > made)
		{
		  vnum = proto->vnum;
		  made = proto->mv;
		}
	    }
	}
      
      sprintf (buf, "Made %d of vnum %d.\n\r", made, vnum);
      stt (buf, th);
      return;
    }
}
	
		  
