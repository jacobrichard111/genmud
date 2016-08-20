#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "event.h"
#ifdef USE_WILDERNESS
#include "wilderness.h"
#endif
TRACK *track_free = NULL;
int track_count = 0;
BFS *bfs_free = NULL;
BFS *bfs_list = NULL;
BFS *bfs_last = NULL;
BFS *bfs_curr = NULL;
int bfs_count = 0;


/* These are kinds of hunting you can engage in. */

const char *hunting_type_names[HUNT_MAX] =
  {
    "none",
    "kill",
    "follow",
    "gather",
    "dropoff",
    "patrol",
    "settle",
    "guardpost",
    "healing",
    "sleep",
    "raid",
    "need",
    "far_patrol",
  };

TRACK *
new_track (void)
{
  TRACK *newtrack;
  if (!track_free)
    ADD_TO_MEMORY_POOL(TRACK,track_free,track_count);
  newtrack = track_free;
  track_free = track_free->next;
  newtrack->dir_from = DIR_MAX;
  newtrack->dir_to = DIR_MAX;
  newtrack->move_set = FALSE;
  newtrack->who = NULL;
  newtrack->timer = 0;
  newtrack->next = NULL;
  return newtrack;
}


void
free_track (TRACK *trk)
{
  if (!trk)
    return;
  
  trk->next = track_free;
  track_free = trk;
  return;
}

/* These read and write tracks. */

void
write_tracks (FILE *f, TRACK *trk_start)
{
  TRACK *trk;
  
  if (!f || !trk_start)
    return;
  
  for (trk = trk_start; trk; trk = trk->next)
    write_track (f, trk);
  return;
}

/* Only writes tracks to rooms. */

void
write_track (FILE *f, TRACK *trk)
{
  if (!f || !trk)
    return;

  if (!trk->who || !IS_ROOM (trk->who))
    return;
  
  fprintf (f, "Track %d %d %d %d %d\n",
	   trk->who->vnum, trk->dir_from, trk->dir_to, trk->timer, trk->move_set);
  return;
}

/* Must be done in read_short_thing so that the rooms all exist first! */

TRACK *
read_track (FILE *f)
{
  TRACK *trk;
  THING *room;
  int roomnum;
  if (!f)
    return NULL;
  
  roomnum = read_number (f);
  if ((room = find_thing_num (roomnum)) == NULL ||
      !IS_ROOM (room))
    return NULL;
  trk = new_track ();
  trk->who = room;
  trk->dir_from = read_number (f);
  trk->dir_to = read_number (f);
  trk->timer = read_number (f);
  trk->move_set = read_number (f);
  return trk;
}



/* Dir from = 00-60 on direction, dir to = 0-REALDIR_MAX on dir. */
/* Note that if the thing in question no longer exists, we cannot
   track it. */



TRACK *
place_track (THING *who, THING *where, int dir_from, int dir_to, bool move_set)
{
  TRACK *trk, *last = NULL;
  if (!who || !where || !IS_ROOM (where))
    return NULL;
  
  for (trk = where->tracks; trk != NULL; trk = trk->next)
    {
      last = trk;
      if (trk->who == who)
	break;
    }
  if (!trk)
    {
      trk = new_track ();
      trk->who = who;
      
      /* This is like this so if say 100 things are following a trail,
	 they all add their tracks at the end and when the trail
	 is being searched, they don't have to do 100 str_cmps to
	 find the correct thing they are tracking.  */
      
      if (last)
	{
	  last->next = trk;
	  trk->next = NULL;
	}
      else
	{
	  trk->next = where->tracks;
	  where->tracks = trk;
	}
    }
  
  /* Should set up the track directions. */
  
  
  if (dir_from < DIR_MAX)
    trk->dir_from = dir_from;
  if (dir_to < DIR_MAX)
    trk->dir_to = dir_to;
  if (move_set) /* Don't undo move_set if some other thing tracks it already */
    trk->move_set = TRUE;

  /* Too many tracks in the world. Make them last less time. */
  
#ifdef USE_WILDERNESS
  if (IS_WILDERNESS_ROOM(where))
    trk->timer = 2;
  else
#endif
    if (trk->timer < 4)
      trk->timer = 4;
  
  
   return trk;
}



void
show_tracks (THING *th, THING *room)
{
  TRACK *trk;
  char buf[STD_LEN];
  
  if (!room || !th || !room->tracks)
    return;
  
  for (trk = room->tracks; trk != NULL; trk = trk->next)
    {
      sprintf (buf, "Tracks of %s leading out to %s, in from the %s, with %d hours left.\n\r",  trk->who ? NAME (trk->who) : "Noone", 
	       trk->dir_to < DIR_MAX ? dir_name[trk->dir_to] : "nowhere",
	       trk->dir_from < DIR_MAX ? dir_name[trk->dir_from] : "nowhere",
	       trk->timer);
      if (LEVEL (th) >= BLD_LEVEL && trk->who)
	sprintf (buf + strlen (buf)-2, "(\x1b[1;34m%d\x1b[0;37m)\n\r", trk->who->vnum);
	  
      stt (buf, th);
    }
  return;
}



void
do_track (THING *th, char *arg)
{
  char *t;
  if (!*arg)
    {
      stt ("Track <name>, or Track me to stop tracking.\n\r", th);
      return;
    }

  
  if (strlen (arg) > 17)
    {
      stt ("That name is too long.\n\r", th);
      return;
    }
  
  if (strlen (arg) < 2)
    {
      stt ("That name is too short.\n\r", th);
      return;
    }
  
  if (!IS_PC (th)) /* Mob tracking */
    {
      if (!th->fgt)
	add_fight (th);
      start_hunting (th, arg, HUNT_KILL);
      return;
    }
  
  if (IS_PC (th) && th->pc->prac[325 /* track */] < MIN_PRAC/2)
    {
      stt ("You don't know how to track!\n\r", th);
      return;
    }
  
  
  for (t = arg; *t; t++)
    {
      if (!isalpha(*t))
	{
	  stt ("You can only track names made up of letters.\n\r", th);
	  return;
	}
    }
  
  
  /* So now we know they can track, the name is ok, so we copy that
     name into the hunting string. If the person was already hunting,
     we change to the new name no matter what it is. */


  if (!str_cmp (arg, "me") || !str_cmp (arg, th->name))
    {
      if (!is_hunting (th))
	{
	  stt ("You aren't tracking anyone!\n\r", th);
	  return;
	}      
 
      stt ("You stop tracking your quarry.\n\r", th);
      stop_hunting (th, TRUE);
      return;
    }
  
  if (!th->fgt)
    add_fight (th);
  
  if (!is_hunting(th))
    stt ("You begin to look for some tracks...\n\r", th);
  else
    stt ("You switch targets and begin to look for some tracks...\n\r", th);
  
  start_hunting (th, arg, HUNT_KILL);
  return;
}
  
  


void
track_victim (THING *th)
{
  TRACK *trk;
  char buf[STD_LEN];
  bool found = FALSE;
  if (!IS_PC (th) || !is_hunting (th) || !th->in)
    return;
  
  
  for (trk = th->in->tracks; trk != NULL; trk = trk->next)
    {
      if (trk->move_set && trk->who && 
	  (is_named (trk->who, th->fgt->hunting) ||
	   trk->who == th->fgt->hunt_victim))
	{
	  if (!check_spell (th, NULL, 325 /* Track */))
	    {
	      stt ("You find some tracks, but you are not sure how old they are or where they lead...\n\r", th);
	      return;
	    }

	  if (trk->dir_from < DIR_MAX)
	    {
	      if (trk->dir_to < DIR_MAX)
		sprintf (buf, "%s's tracks come from %s and lead %s.\n\r",
NAME (trk->who), dir_track_in[trk->dir_from], dir_track[trk->dir_to]);
	      else
		{
		  sprintf (buf,  "%s's tracks come from %s and stop.\n\r",
			   NAME (trk->who), dir_track_in[trk->dir_from]);
		  stop_hunting (th, TRUE);
		}
	    }
	  else if (trk->dir_to < DIR_MAX)
	    sprintf (buf, "%s's tracks lead %s.\n\r", NAME (trk->who), dir_track[trk->dir_to]);
	  else
	    sprintf (buf, "You find some tracks, but you are not sure how old they are or where they lead...\n\r");
	  found = TRUE;
	  stt (buf, th);
	  break;
	}
    }
  if (!found)
    stt ("You find some tracks, but you are not sure how old they are or where they lead...\n\r", th);
  return;
}

/* This used to be a dfs search, now it is a bfs. Dunno, maybe it's
   better for short hops. */



BFS *
new_bfs (void)
{
  BFS *newbfs;
  if (!bfs_free)
    ADD_TO_MEMORY_POOL(BFS,bfs_free,bfs_count);
  newbfs = bfs_free;
  bfs_free = bfs_free->next;
  
  /* This is dangerous not initializing here...so if you run into problems,
     this is where they originate. :P */
  /*  newbfs->next = NULL;
  newbfs->room = NULL;
  newbfs->from = NULL;
  newbfs->depth = 0;
  newbfs->dir = DIR_MAX;*/
  return newbfs;
}
void
clear_bfs_list (void)
{
  BFS *bfs, *bfsn;
  if (bfs_list)
    {
      bfs_times_count++;
      for (bfs = bfs_list; bfs ; bfs = bfsn)
	{
	  bfsn = bfs->next;
	  if (bfs->room)
	    RBIT (bfs->room->thing_flags, TH_MARKED);
	  bfs->next = bfs_free; 
	  bfs_free = bfs;
	  bfs_tracks_count++;
	}
    }
  bfs_curr = NULL;
  bfs_list = NULL;
  bfs_last = NULL;
  if (bfs_times_count > 100000)
    {
      bfs_times_count /= 10;
      bfs_tracks_count /= 10;
    }
}


/* Adds a room to the bfs list. */

void
add_bfs (BFS *from, THING *room, int dir)
{
  BFS *bfs;
  
  if (!room || IS_MARKED (room))
    return;
  
  bfs = new_bfs ();
  bfs->from = from;
  bfs->room = room;
  bfs->dir = dir;
  bfs->next = NULL;
  SBIT (room->thing_flags, TH_MARKED);
  
  if (!bfs_list)
    {
      bfs_list = bfs;
      bfs_curr = bfs;
      bfs_last = bfs;
    }
  else
    {
      bfs_last->next = bfs;
      bfs_last = bfs;
    }  
  if (from)
    bfs->depth = from->depth + 1;
  else
    bfs->depth = 1;
  return;
}

/* This lets mobs hunt things from rooms to other creatures to
   objects. It has a lot of special-purpose code to take care
   of a lot of cases like societies scanning for victims and
   things with ranged weapons or ranged spells that don't rush
   into combat. */

bool
hunt_thing (THING *th, int max_depth)
{ 
  THING *nroom,  /* Next room for adding bfses to list. */
    *room,       /* Current room being searched. */
    *target_room,  /* What room we're hunting. */
    *vict,       /* The victim (if you find it). */
    *your_vict,  /* If you already have a vict, use this. */
    *croom,      /* Current room -- used for ranged. */
    *targ,       /* Target for casting/shooting. */
    *closest_diff_society_vict, /* What thing we attack if we soci hunt. */
    *obj,        /* An object you have on you (ranged) or obj in room. */
    *objc;       /* Used for checking if an obj contains the vict. */
  bool found_within_range, found, tracks_ok, society_hunt;
  int vnum, i, hunting_type, range, dist;
  char *nme;
  int actbits;
  int dir;
  VALUE *feed, *exit, *ranged, *ammo, *soc;
  TRACK *trk = NULL;
  SOCIETY *society;
  int goodroom_bits = 0;  /* A list of room flags where this thing can go. */
  int track_hours = 4; /* Number of hours we make newly added tracks. */
  
 
  if (!th || !th->in || IS_PC (th) || !is_hunting (th) ||
      !IS_ROOM (th->in) || FIGHTING (th) || th->position <= POSITION_SLEEPING)
    {
      if (!FIGHTING (th) && !IS_PC (th))
	stop_hunting (th, TRUE);
      return FALSE;
    }

  
  
  if (th->fgt->hunting_failed > 0)
    {
      th->fgt->hunting_failed--;
      return FALSE;
    }

  if (th->fgt->hunt_victim == th)
    {
      stop_hunting(th, TRUE);
      return FALSE;
    }
  
  /* Sentinel nonfasthunt mobs only hunt in this room. */
  
  actbits = flagbits (th->flags, FLAG_ACT1);
  if (IS_SET (actbits, ACT_SENTINEL) &&
      !IS_SET (actbits, ACT_FASTHUNT))
    {
      max_depth = 1;
      if (isdigit (th->fgt->hunting[0]))
	{
	  stop_hunting (th, TRUE);
	  return FALSE;
	}
    }
  
  /* Sets max hunt distance if there is no distance 
     given. It should be enough assuming the world
     isn't one long stringy road. */
  
  if (max_depth < 1)
    {
      max_depth = MAX_HUNT_DEPTH;
    }
  
  
  if (max_depth >= MAX_SHORT_HUNT_DEPTH &&
      th->fgt->hunting_type != HUNT_RAID &&
      th->fgt->hunting_type != HUNT_SETTLE &&
      th->fgt->hunting_type != HUNT_KILL &&
      th->fgt->hunting_type != HUNT_FAR_PATROL)
    {
      max_depth = MAX_SHORT_HUNT_DEPTH;
      if (nr (1,20) == 3)
	max_depth *= 2;
    }
  
  /* Most of the time cap the max hunt depth at MAX_HUNT_DEPTH. */
 
  if ((soc = FNV (th, VAL_SOCIETY)) != NULL)
    {
      if (!str_cmp (th->fgt->hunting, "diff_society"))
	{
	  if (!soc ||
	      (society = find_society_num (soc->val[0])) == NULL)
	    {
	      stop_hunting (th, FALSE);
	      return FALSE; 
	    }       
	  closest_diff_society_vict = NULL;
	  society_hunt = TRUE;
	  max_depth = SOCIETY_HUNT_DEPTH;
	}
      else
	{
	  society_hunt = FALSE;
	  /* Attack things close by if possible. */
	  if (IS_SET (soc->val[2], BATTLE_CASTES) &&
	      nr (1,15) == 3)
	    {
	      for (vict = th->in->cont; vict; vict = vict->next_cont)
		{
		  if (CAN_FIGHT (vict) && nr (1,10) == 3 &&
		      is_enemy (th, vict))
		    {
		      do_say (th, "NOW YOU WILL DIE!");
		      multi_hit (th, vict);
		      return FALSE;
		    }
		}
	    }
	}
    }
  else
    society_hunt = FALSE;
      
  /* If we found the vict here, ignore all the searching. */
  
  if ((your_vict = th->fgt->hunt_victim) != NULL &&
      ((your_vict->in == th->in) ||
       your_vict == th->in))
    vict = your_vict;
  else
    {
      nme = th->fgt->hunting;
      vict = NULL;
      tracks_ok = TRUE;
      vnum = atoi (nme);
      clear_bfs_list ();
      undo_marked (th->in);
      add_bfs (NULL, th->in, DIR_MAX);
      goodroom_bits = th->move_flags;
      
      /* This is now split up some to cut down on the CPU somewhat. */
      
      /* If hunting for a room...*/
      
      if ((vnum = atoi (nme)) > 0) /* Start track a room. */
	{
	  
	  /* If the target doesn't exist, or isn't a room, return false. */
	  if ((target_room = find_thing_num (vnum)) == NULL ||
	      !IS_ROOM (target_room))
	    {
	      stop_hunting (th, FALSE);
	      clear_bfs_list ();
	      return FALSE;
	    }

	  /* Check if we can even get to this room. If it's a badroom,
	     then don't go there. If the target room has badroom bits
	     that aren't covered by the goodroom bits, then don't go 
	     there. */
	  
	  if (IS_ROOM_SET (target_room, BADROOM_BITS &~goodroom_bits)) 
	    {
	      stop_hunting (th, FALSE);
	      return FALSE;
	    }
	    
	  th->fgt->hunt_victim = target_room;	
	  while (bfs_curr && !vict)
	    {
	      if ((room = bfs_curr->room) == NULL || !IS_ROOM (room))
		{
		  clear_bfs_list ();
		  return FALSE;
		}
	      
	      if (room == target_room)
		{
		  vict = room;
		  break;
		}
#ifdef USE_WILDERNESS
	      if (IS_WILDERNESS_ROOM(room) &&
		  (max_depth == 0 || max_depth - bfs_curr->depth > 10))
		max_depth = bfs_curr->depth + 10;
#endif
	      if (tracks_ok && !vict)
		{
		  for (trk = room->tracks; trk; trk = trk->next)
		    {	
		      if (trk->who == target_room)
			{
			  if (trk->dir_to == RDIR (bfs_curr->dir))
			    tracks_ok = FALSE;
			  else
			    {
			      vict = trk->who;
			      track_hours = trk->timer;
			      break;
			    }	
			}
		    }
		  
		}
	      if (!vict && bfs_curr->depth < max_depth)
		{
		  for (exit = room->values; exit; exit = exit->next)
		    {
		      if ((dir = exit->type - 1) < REALDIR_MAX &&
			  (nroom = FTR (room, dir, goodroom_bits)) != NULL) 
			add_bfs (bfs_curr, nroom, dir);
		    }
		}	      
	      else
		break;
	      bfs_curr = bfs_curr->next;
	    }
	  if (vict)
	    {
	      if (!trk)
		track_hours += bfs_curr->depth/20;
	      place_backtracks (vict, track_hours);
	    }
	} /* End of track a room. */
      
      else if (society_hunt) /* Start of hunt society victims. */
	{
	  closest_diff_society_vict = NULL;
	  while (bfs_curr && !vict)
	    {
	      if ((room = bfs_curr->room) == NULL || !IS_ROOM (room))
		{
		  clear_bfs_list ();
		  return FALSE;
		}
#ifdef USE_WILDERNESS
	      if (IS_WILDERNESS_ROOM(room) &&
		  (max_depth == 0 || max_depth - bfs_curr->depth > 10))
		max_depth = bfs_curr->depth + 10;
#endif
	      for (vict = room->cont; vict != NULL; vict = vict->next_cont)
		{
		  if (vict == th)
		    continue;
		  /* If the mob is in a society and is looking for 
		     something to kill, do this! */		  
		  if (CAN_FIGHT (vict))
		    {
		      if (is_enemy (th, vict) && nr (1,4) == 2 &&
			  !closest_diff_society_vict &&
			  LEVEL (vict) <= LEVEL(th)*3/2)
			{
			  closest_diff_society_vict = vict;
			}
		      else if (FIGHTING (vict) && is_friend (th, vict))			
			{				  
			  vict = FIGHTING(vict);
			  stop_hunting (th, TRUE);
			  start_hunting (th, KEY(vict), HUNT_KILL);
			  th->fgt->hunt_victim = vict;
			  break; 
			}
		    } 
		}
	      
	      if (!vict) /* Add more rooms... */
		{
		  if (bfs_curr->depth < max_depth)
		    {
		      for (exit = room->values; exit; exit = exit->next)
			{
			  if ((dir = exit->type - 1) < REALDIR_MAX &&
			      (nroom = FTR (room, dir, goodroom_bits)) != NULL)
			    add_bfs (bfs_curr, nroom, exit->type - 1);
			}
		    }
		  /* If we found no ally to help within the allotted
		     radius, attack the closest enemy. */
		  else if (closest_diff_society_vict)
		    {
		      vict = closest_diff_society_vict;
		      stop_hunting (th, TRUE);
		      start_hunting (th, KEY(vict), HUNT_KILL);
		      th->fgt->hunt_victim = vict;
		    }
		}
	      
	      if (vict) /* Cannot be else since the vict may get set just above at
			   max depth if no allies needed help nearby. */
		{	
		  place_backtracks (vict, 4);
		  break;
		}	      
	      bfs_curr = bfs_curr->next;
	    }
	} /* End hunt society victim. */
      else /* General hunt by name or thing if we're already hunting. */
	{ 	   
	  /* Only search if you're not searching for a room. */
	  tracks_ok = TRUE;
	  while (bfs_curr && !vict)
	    {
	      if ((room = bfs_curr->room) == NULL || !IS_ROOM (room))
		{
		  clear_bfs_list ();
		  return FALSE;
		}
#ifdef USE_WILDERNESS
	      if (IS_WILDERNESS_ROOM(room) &&
		  (max_depth == 0 || max_depth - bfs_curr->depth > 10))
		max_depth = bfs_curr->depth + 10;
#endif

	      if (your_vict) /* Don't do name search if you know your victim. */
		{
		  for (vict = room->cont; vict; vict = vict->next_cont)
		    {
		      if (your_vict == vict)
			break;
		    } 
		  if (!vict && tracks_ok)
		    {
		      for (trk = room->tracks; trk; trk = trk->next)
			{
			  if (trk->who == your_vict && trk->who != th)
			    {
			      if (trk->dir_to == RDIR (bfs_curr->dir))
				tracks_ok = FALSE;
			      else
				{
				  vict = trk->who;
				  track_hours = trk->timer;
				}			  
			    }
			  break;
			}
		    }
		}		  
	      else
		{	      
		  for (vict = room->cont; vict != NULL; vict = vict->next_cont)
		    {
		      /* Only check things that can fight and are named the
			 correct name. May wish to remove CAN_FIGHT restriction
			 to let things track objects like a monument or something. */
		      
		      if (vict != th && CAN_FIGHT (vict) && is_named (vict, nme))
			{
			  th->fgt->hunt_victim = vict;
			  your_vict = vict;
			  break;
			}
		    }
		  
		  /* If the victim isn't here then look for tracks if we didn't
		     encounter a reversed track. */
		  
		  if (!vict && tracks_ok) 
		    {
		      for (trk = room->tracks; trk; trk = trk->next)
			{
			  if (trk->who != th && is_named (trk->who, nme))
			    {
			      if (trk->dir_to == RDIR (bfs_curr->dir))
				tracks_ok = FALSE;
			      else
				{
				  th->fgt->hunt_victim = trk->who;
				  vict = trk->who;
				  track_hours = trk->timer;
				}
			      break;
			    }
			}
		    }
		}
	      
	      if (!vict) /* Add more rooms... */
		{
		  if (bfs_curr->depth < max_depth)
		    { 
		      for (exit = room->values; exit; exit = exit->next)
			{
			  if ((dir = exit->type - 1) < REALDIR_MAX &&
			      (nroom = FTR (room, dir, goodroom_bits)) != NULL)
			    add_bfs (bfs_curr, nroom, dir);
			}
		    }
		}
	      else if (tracks_ok || vict == bfs_curr->room ||
		       vict->in == bfs_curr->room)
		{	
		  if (!trk)
		    track_hours += bfs_curr->depth/20;
		  place_backtracks (vict, track_hours);
		  break;
		}	
	   
	      else
		vict = NULL;
	      bfs_curr = bfs_curr->next;
	     
	    } /* End of search for nonroom-nonsociety hunt. */
	}
    }
  
  clear_bfs_list ();
 
  if (!vict)
    {
      th->fgt->hunting_failed = nr (10,20); /* To keep you from spam hunting
					       something that isn't there. */
      
      /* Now add a small chance to go through a portal. */
      
      if (nr (1,6) == 2 && th->in)
	move_through_portal (th, NULL);
      return FALSE;
    }
  /* Do ranged stuff if you're hunting to kill. */
  
  if (th->fgt->hunting_type == HUNT_KILL)  
    {
      
      /* Healers and wizards don't engage in combat. */
      if (soc)
	{
	  if (IS_SET (soc->val[2], CASTE_HEALER | CASTE_WIZARD) &&
	      (targ = find_thing_near (th, th->fgt->hunting, 2)) != NULL &&
	      (!IS_PC (targ) || LEVEL (targ) < MAX_LEVEL))
	    {
	      if (IS_SET (soc->val[2], CASTE_WIZARD))
		cast_spell (th, targ, find_spell (NULL, nr(50, 100)), (nr(1,7) == 3), FALSE, NULL);
	      else /* Healers don't engage. */
		stop_hunting (th, FALSE);
	      return TRUE;
	    }
	  
      /* Find any ranged equip it has on and find max range. For members
	 of warrior (archer maybe?) caste of societies. */
      
	  if (IS_SET (soc->val[2], CASTE_WARRIOR | CASTE_LEADER))
	    {
	      range = 0;
	      for (obj = th->cont; obj; obj = obj->next_cont)
		{
		  if (!IS_WORN (obj))
		    break;
		  if ((ranged = FNV (obj, VAL_RANGED)) != NULL &&
		      obj->cont &&
		      (ammo = FNV (obj->cont, VAL_AMMO)) != NULL &&
		      ammo->val[4] == ranged->val[4])
		    range = MAX (range, ranged->val[1]);
		}
	      
	      /* Only do this if it has ranged eq equipped. */
	      
	      if (range > 0)
		{
		  if (vict->in == th->in &&
		      !IS_SET (actbits, ACT_SENTINEL)) /* Run away from melee combat :) 
							  This should piss off the players. */
		    {
		      int dir;
		      for (i = 0; i < REALDIR_MAX; i++)
			{
			  dir = nr (0, REALDIR_MAX-1);
			  if ((exit = FNV (th->in, dir + 1)) != NULL &&
			      !IS_SET (exit->val[1], EX_CLOSED) &&		      
			      (room = find_thing_num (exit->val[0])) != NULL &&
			      IS_ROOM (room) && !IS_BADROOM (room))
			    {
			      move_dir (th, dir, 0);
			      return TRUE;
			    }
			}
		    }
		  
		  /* Otherwise try to shoot an enemy. */
		  
		  found_within_range = FALSE;
		  for (i = 0; i < REALDIR_MAX && !found_within_range; i ++)
		    {
		      croom = th->in;
		      for (dist = 0; dist < range; dist++)
			{
			  nroom = NULL;
			  if (croom && IS_ROOM (croom) &&
			      (exit = FNV (croom, i + 1)) != NULL &&
			      (nroom = find_thing_num (exit->val[0])) != NULL &&
			      vict->in == nroom)
			    {
			      found_within_range = TRUE;
			      break;
			    }
			  if (nroom)
			    croom = nroom;
			  else
			    break;
			}
		    }
		  
		  if (found_within_range)
		    {
		      do_fire (th, th->fgt->hunting);
		      return TRUE;
		    }
		} 
	    }
	}
    }
  
  if (vict->in == th->in)
    {
      hunting_type = th->fgt->hunting_type;
      stop_hunting (th, FALSE);
      switch (hunting_type)
	{
	  case HUNT_KILL:
	  default:
	    if ((feed = FNV (th, VAL_FEED)) != NULL)
	      {
		if (!CAN_FIGHT (vict))
		  {
		    act ("@1n eat@s @2n!", th, vict, NULL, NULL, TO_ROOM);
		    free_thing (vict);
		    feed->val[5] = 0;
		  }
		else
		  {
		    act ("@1n attack@s @2n!", th, vict, NULL, NULL, TO_ROOM);
		    multi_hit (th, vict);
		  }	  
	      }
	    else if (can_see (th, vict) && !IS_ACT1_SET (th, ACT_PRISONER))
	      {
		do_say (th, "Now you will Die!");
		multi_hit (th, vict);
	      }
	    break;
	    
	  case HUNT_FOLLOW:
	      do_follow (th, KEY (vict));	  
	      break;
	  case HUNT_GATHER:
	    society_gather (th);
	    break;
	  case HUNT_DROPOFF:
	    society_activity (th);
	    break;
	  case HUNT_NONE:
	  case HUNT_HEALING:
	    break;
	}
      return TRUE;
    }
  
  if (vict == th->in && IS_ROOM (vict))
    {	
      hunting_type = th->fgt->hunting_type;
      stop_hunting (th, FALSE);
      switch (hunting_type)
	{
	  case HUNT_FAR_PATROL:
	  case HUNT_PATROL: /* This thing patrols based on what it is...
			       society mob? align mob? whatever it is. */
	    /* Wait most of the time at the new loc. Unless it's a
	       society mob patrolling. */
	    if (nr (1,45) == 2 || soc)
	      find_new_patrol_location (th);
	    else
	      attack_stuff (th);
	      break;
	  case HUNT_SLEEP:
	    do_sleep (th, "");
	    stop_hunting (th, FALSE);
	    break;
	  case HUNT_SETTLE: /* Settling a new location */
	    if (soc && !IS_SET (soc->val[2], BATTLE_CASTES))
	      settle_new_society_now (th);
	    break;
	  case HUNT_GUARDPOST:	
	    start_guarding_post (th);
	    
	    break;	
	  case HUNT_RAID:
	    attack_stuff(th);
	    break;
	  case HUNT_NEED:
	    society_satisfy_needs (th);
	    break;
	  case HUNT_NONE:
	  default:
	    break;
	}
      return TRUE;
    }
  
  /* The vict isn't in its room and it isn't the room itself. */
  
  if (IS_SET (actbits, ACT_SENTINEL) && !IS_SET (actbits, ACT_FASTHUNT))
    return FALSE;
  
  for (trk = th->in->tracks; trk; trk = trk->next)
    if (trk->who == vict)
      break;
  
  /* If you find tracks either move or open a door to follow them. */
  
  if (trk && trk->dir_to != DIR_MAX)
    {
      if ((exit = FNV (th->in, trk->dir_to + 1)) != NULL &&
	  IS_SET (exit->val[1], EX_DOOR) &&
	  IS_SET (exit->val[1], EX_CLOSED))
	do_open (th, dir_name[trk->dir_to]);
      else
	move_dir (th, trk->dir_to, 0);
      return TRUE;
    }
  
  /* If the thing entered an object in the room, follow it. */
  
  found = FALSE;
  for (obj = th->in->cont; obj && !found; obj = obj->next_cont)
    {
      if (IS_PC (obj) || CAN_MOVE (obj) || CAN_FIGHT (obj))
	continue;
      
      /* First check if it contains the intended victim.. */
      
      for (objc = obj->cont; objc; objc = objc->next_cont)
	{
	  if (objc == vict)
	    {
	      
	      act ("@1n enter@s @3n.", th, NULL, obj, NULL, TO_ALL);
	      if (move_thing (th, th, th->in, obj))
		act ("@1n enter@s from @2n.", th, obj, NULL, NULL, TO_ALL);
	      found = TRUE;
	      break;
	    }
	}
    }
  
  if (!found && !IS_ROOM (th->in) && 
      th->in->in && IS_ROOM (th->in->in) &&
      nr (1,7) == 3)
    do_leave (th, "");
  
  if (!found)
    move_through_portal (th, vict);
  
  return TRUE;
}

/* This function lets a thing attempt to move through a portal or climbable
   object either following something, or just randomly so that they have a 
   chance to get to their targets. If the vict is NULL, we go through
   any old portal, but if it's something, then tracks or the victim
   itself must be on the other side of the portal. */

void
move_through_portal (THING *th, THING *vict)
{
  THING *obj = NULL, *room = NULL, *objc = NULL;
  VALUE *portal = NULL;
  bool found = FALSE;
  TRACK *trk = NULL;
  
  if (!th || !th->in)
    return;
  
  for (obj = th->in->cont; obj && !found; obj = obj->next_cont)
    {
      for (portal = obj->values; portal; portal = portal->next)
	{
	  /* Check if this is a portal or climbable. This is just
	     dumb luck that the portal value VAL_EXIT_I bumps up
	     against the climbing values VAL_OUTEREXIT_*. For this reason
	     if you change the order of the values someday, you have to be 
	     careful about fixing this. */
	  
	  if (portal->type >= VAL_EXIT_I &&
	      portal->type <= VAL_OUTEREXIT_I &&
	      (room = find_thing_num (portal->val[0])) != NULL &&
	      IS_ROOM (room) &&
	      nr (1,2) == 2)
	    {
	      
	      if (vict)
		{
	      
		  /* See if tracks of the person lie in the room. */
		  
		  for (trk = room->tracks; trk; trk = trk->next)
		    {
		      if (trk->who == vict)
			break;
		    }
	      
		  
		  if (!trk) /* Didn't find a track before. */
		    {
		      for (objc = room->cont; objc; objc = objc->next_cont)
			{
			  if (objc == vict)
			    break;
			}
		    }
		}
	      
	      /* If we found the victim or we found
		 tracks of the victim, or if there was no
		 victim, move through the
		 portal/climbable object. */
	      
	      if (objc || trk || !vict) 
		    
		{
		  if (portal->type == VAL_EXIT_I)
		    {
		      act ("@1n enter@s @3n.", th, NULL, obj, NULL, TO_ALL);
		      if (move_thing (th, th, th->in, room))
			act ("@1n enter@s from @2n.", th, obj, NULL, NULL, TO_ALL);
		    }
		  else
		    {
		      act ("@1n climb@s @t @3n.", th, NULL, obj, 
			   dir_name[portal->type - VAL_OUTEREXIT_N], TO_ALL);
		      if (move_thing (th, th, th->in, room))
			act ("@1n climb@s @t from @2n", th, obj, NULL,
			     dir_name[portal->type - VAL_OUTEREXIT_N]
			     , TO_ALL);
		    }
		  found = TRUE;
		  break;
		}
	    }
	}
    }
  return;
}

/* This determines if we can track out of a room in this direction. */
/* Goodroom bits are special abilities that the tracking creature
   has that allow it to pass through difficult terrain. */

THING *
find_track_room (THING *croom, int dir, int goodroom_bits)
{
  VALUE *exit; 
  THING *room;
  
  if (!croom || dir >= REALDIR_MAX)
    return NULL;
  
  if ((exit = FNV (croom, dir + 1)) != NULL &&
      !IS_SET (exit->val[1], EX_WALL) &&
      (room = find_thing_num (exit->val[0])) != NULL &&
      is_track_room (room, goodroom_bits))
    return room;
  
  return NULL;
}




/* This either tells what BADROOM_BITS a room has set, or what terrain
   a thing can pass through if it's not a room. */

void
set_up_move_flags (THING *th)
{
  if (!th)
    return;
  
  if (IS_ROOM (th))
    th->move_flags = 
      (flagbits (th->flags, FLAG_ROOM1) &
       BADROOM_BITS);
  else
    {
      int move_bits = flagbits (th->flags, FLAG_AFF);
      int prot_bits = flagbits (th->flags, FLAG_PROT);
      th->move_flags = 0;
      if (IS_SET (move_bits, AFF_FOGGY))
	th->move_flags |= ROOM_EARTHY;
      if (IS_SET (move_bits, AFF_FLYING))
	th->move_flags |= ROOM_AIRY | ROOM_MOUNTAIN;
      if (IS_SET (move_bits, AFF_WATER_BREATH))
	th->move_flags |= ROOM_UNDERWATER | ROOM_WATERY;
      if (IS_SET (move_bits, AFF_HASTE) && !IS_PC (th))
	th->move_flags |= ROOM_MOUNTAIN;
      if (IS_SET (prot_bits, AFF_FIRE))
	th->move_flags |= ROOM_FIERY;
      if (IS_AFF (th, AFF_PROTECT))
	th->move_flags |= ROOM_ASTRAL;
    }

  return;
}

/* This lets you hunt a room number. */

void
start_hunting_room (THING *th, int vnum, int type)
{
  char buf[STD_LEN];
  sprintf (buf, "%d", vnum);
  start_hunting (th, buf, type);
  return;
}

/* This starts someone hunting. A quick note. If the thing was
   hunting something on a "low priority" status, and the new 
   thing is more important (HUNT_KILL), then the lower priority
   hunting info gets pushed back to the "old hunting status. */

void
start_hunting (THING *th, char *name, int type)
{
  if (!th || th->position == POSITION_SLEEPING)
    return;
  
  add_fight (th);
  
    
  /* If we're hunting, and we aren't kill hunting and we want to start
     kill hunting, then push the current hunting back...this should
     probably be a real priority queue I guess, but this is close
     enough for now. */
  
  if (th->fgt->hunting[0])
    {
      if (th->fgt->hunting_type == HUNT_HEALING ||
	  th->fgt->hunting_type == HUNT_SETTLE ||
	  th->fgt->hunting_type == HUNT_RAID ||
	  th->fgt->hunting_type == HUNT_GUARDPOST)
	return;
      if (type != HUNT_HEALING && 
	  (th->fgt->hunting_type == HUNT_KILL || 
	   th->fgt->hunting_type == HUNT_HEALING ||
	   type != HUNT_KILL))
	return;
      th->fgt->old_hunting_type = th->fgt->hunting_type;
      strncpy (th->fgt->old_hunting, th->fgt->hunting, HUNT_STRING_LEN);
    }
  
  th->fgt->hunt_victim = NULL;
  th->fgt->hunting_type = type;  
  th->fgt->hunting_timer = 30;
  th->fgt->hunting_failed = 0;
  strncpy (th->fgt->hunting, name, HUNT_STRING_LEN);
  if (IS_PC (th))
    add_track_event (th);
  return;
}


/* This stops someone who is hunting. If they had an "old hunting type"
   then that gets moved up into the regular hunting slot, and the old
   slot is cleared. */

void
stop_hunting (THING *th, bool all)
{
  if (!th || !th->fgt)
    return;
  

  th->fgt->hunting[0] = '\0';
  th->fgt->hunting_type = HUNT_NONE;
  th->fgt->hunting_timer = 0;
  th->fgt->hunting_failed = 0;
  
  if (th->fgt->old_hunting[0] && !all)
    {
      strncpy (th->fgt->hunting, th->fgt->old_hunting, HUNT_STRING_LEN);
      th->fgt->hunting_type = th->fgt->old_hunting_type;
      th->fgt->hunting_timer = 30;
      th->fgt->hunting_failed = 0;
    }
  
  th->fgt->old_hunting[0] = '\0';
  th->fgt->old_hunting_type = HUNT_NONE;
  th->fgt->hunt_victim = NULL;
  if (IS_PC (th))
    remove_typed_events (th, EVENT_TRACK);
  return;
}

/* This places the series of tracks from the tracker to the victim. */

void
place_backtracks (THING *vict, int track_hours)
{
  int dir_from;
  BFS *prev_bfs;
  TRACK *trk;
  if (!vict || !bfs_curr)
    return;
  
  if (track_hours < 1)
    track_hours = 3;
  dir_from = bfs_curr->dir;
  prev_bfs = bfs_curr->from;
  while (prev_bfs)
    {
      trk = place_track (vict, prev_bfs->room,  DIR_MAX, dir_from, FALSE);
      if (trk && track_hours && trk->timer < track_hours)
	trk->timer = track_hours;
      dir_from = prev_bfs->dir;
      prev_bfs = prev_bfs->from;
    }
  clear_bfs_list();
  return;
}

/* This lets aggro/protector mobs/society mobs attack things. */
/* This goes down the list of contents of the room to find any
   acceptable things to attack, then it attacks one of them. */

/* A quick note. This now calls is_enemy which is _way_ inefficient,
   but it was getting unwieldy so I will do this and maybe
   optimize later if it gets too bad. Also note that there is a
   random number check in is_enemy to see if ACT_ANGRY works or not.
   This will lead to some possibly undesired behavior since you will
   need to have two checks succeed for that to kick in. That means you
   want to keep the chance that angry works pretty large, since the 
   overall chance that you actually attack something with it is roughly
   1/the square of the chance that you get any single check in. */

void
attack_stuff (THING *th)
{
  THING *room, *vict = NULL;
  int num_choices, num_chose, choice, times, count;
  VALUE *socval = NULL, *vsocval = NULL, *build;
  SOCIETY *society = NULL, *esociety = NULL;
  bool capture_now = TRUE;
  char buf[STD_LEN];
  if (!CAN_FIGHT (th) || FIGHTING (th) || 
      nr (1,4) == 2 || IS_PC (th) ||
      (room = th->in) == NULL || th->position < POSITION_STANDING ||
      IS_ACT1_SET (th, ACT_PRISONER))
    return;
  
  
  /* If it's hunting, it attacks what it's hunting if it's here. */
  
  if (is_hunting (th) && th->fgt->hunting_type == HUNT_KILL && 
      th->fgt->hunt_victim && th->fgt->hunt_victim->in == th->in &&
      th->fgt->hunt_victim != th && can_see (th, th->fgt->hunt_victim))
    {
      vict = th->fgt->hunt_victim;
    }

  num_choices = 0;
  num_chose = 0;
  choice = 0;
  
  /* Check for all enemies in the room, and if we find any, attack
     them. */
  
  if ((socval = FNV (th, VAL_SOCIETY)) != NULL)
    {
      if ((society = find_society_num (socval->val[0])) != NULL &&
	  nr (1,10) == 3 &&
	  (build = FNV (th->in, VAL_BUILD)) != NULL &&
	  IS_SET (socval->val[2], BATTLE_CASTES) &&
	  build->val[0] != socval->val[0])
	{      
	  do_raze (th, "");
	  return;
	}
    }
      
	
  if (!vict)
    {
      for (count = 0; count < 2; count++)
	{
	  for (vict = room->cont; vict; vict = vict->next_cont)
	    {
	      if (!CAN_FIGHT (vict) || 
		  th == vict || !CAN_MOVE (vict) ||
		  !can_see (th, vict) || 
		  !is_enemy (th, vict))
		continue;
	      
	      if (count == 0)
		num_choices++;
	      else if (++choice == num_chose)
		break;
	      
	    }
	  if (count == 0)
	    {
	      if (num_choices < 1) 
		{
		  /* Victim is not here...possibly search for something else? */
		  if (!is_hunting (th) && nr (1,8) == 2 && 
		      socval && society)	  	    
		    {
		      start_hunting (th, "diff_society", HUNT_KILL);
		      if (!hunt_thing (th, 5)) 
			stop_hunting (th, FALSE);
		    }    
		  return;
		}
	      else
		num_chose = nr (1, num_choices);
	    }
	}
    }
  
  if (vict)
    {
      act ("@1n is@s being attacked.", vict, NULL, NULL, NULL, TO_ALL);
      /* See if the victim has been captured. */
      if ((vsocval = FNV (vict, VAL_SOCIETY)) != NULL)
	esociety = find_society_num (vsocval->val[5]);
      else
	esociety = NULL;
      
      /* If this thing is a prisoner of an ally (or this thing's
	 society) don't attack it. If it's not in our city but it's
	 our prisoner, bring it back to the city. */
      
      if (vsocval && esociety && society && CAN_TALK(th) &&
	  (society == esociety || 
	   (society->align > 0 && 
	    !DIFF_ALIGN (society->align, esociety->align))))
	{
	  /* If it's following something here, then check if it's
	     following an enemy. If so, attack to stop the captive
	     from escaping. */
	  if (vict->fgt && vict->fgt->following &&
	      vict->fgt->following->in == vict->in)
	    {
	      if (is_enemy (th, vict->fgt->following))
		{
		  SBIT(server_flags, SERVER_BEING_ATTACKED_YELL);
		  start_fighting (th, FOLLOWING(vict));
		  sprintf(buf, "yell Help! %s is escaping with %s!",
			  NAME(vict), NAME(vict->fgt->following));
		  interpret (th, buf);
		  RBIT (server_flags, SERVER_BEING_ATTACKED_YELL);
		}
	      return;
	    }
	  
	  /* If we're in our homeland, then ignore this thing. */
	  else if (th->in->vnum >= society->room_start &&
	      th->in->vnum <= society->room_end)
	    capture_now = FALSE;
	  
	}
      
      /* Take a prisoner. */
      if ((capture_now || nr (1,CAPTURE_CHANCE) == CAPTURE_CHANCE/2) &&
	  nr (1,50) == 3 &&
	  LEVEL (th) >= LEVEL(vict)/2 && !FIGHTING (vict) &&
	  socval && CAN_TALK (th) &&
	  socval->val[3] == WARRIOR_HUNTER &&
	  vsocval)
	{ 
	  for (times = 0; times < 10; times++)
	    {
	      if ((room = find_thing_num (nr(society->room_start, 
					     society->room_end))) != NULL &&
		  IS_ROOM (room))
		{
		  vsocval->val[5] = socval->val[0];
		  add_flagval (vict, FLAG_ACT1, ACT_PRISONER);
		  add_fight (vict);
		  vict->fgt->following = th;
		  stop_fighting (vict);
		  stop_hunting (vict, TRUE);
		  stop_fighting (th); 
		  stop_hunting (th, TRUE);
		  act ("$9@1n capture@s @3n!$7", th, NULL, vict, NULL, TO_ALL);
		 
		  
		  start_hunting_room (th, room->vnum, HUNT_HEALING);
		  break;
		}
	    }
	  
	}
      else 
	{ 
	  do_say (th, "Now you WILL die!");
	  multi_hit (th, vict);
	}
    }
  return;
}



/* This marks all rooms adjacent to the current room if they are
   accessible as track rooms and the area argument is used to either
   keep the rooms inside of a certain area or not. */

void
mark_track_room (THING *room, THING *area, int goodroom_bits)
{
  THING *nroom;
  int dir;
  if (!room || !IS_ROOM (room) || IS_MARKED (room) ||
      (area && (!IS_AREA (area) || room->in != area)))
    return;
  
  SBIT (room->thing_flags, TH_MARKED);
  
  for (dir = 0; dir < REALDIR_MAX; dir++)
    {
      if ((nroom = FTR (room, dir, goodroom_bits)) != NULL)
	mark_track_room (nroom, area, goodroom_bits);
    }
  return;
}





