#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "rumor.h"

/* This function deals with a society raiding another society. The
   main idea is that the society picks another society (an enemy
   society) and then sets out to raid it. There are two stages,
   1) setup, and 2) raiding that have their own countdowns and such. */

   
void
update_raiding (SOCIETY *soc, int raid_target)
{
  SOCIETY *soc2 = NULL;
  THING *room, *mob = NULL, *oldarea = NULL, *area;
  int i, j;
  int post_choices = 0, post_chose = 0, post_num;
  int soc_chose = 0, soc_choices = 0, count;
  char buf[STD_LEN];
  bool can_reach_post[NUM_GUARD_POSTS]; /* Tell if we can get to these
					   guardposts or not. */
  
  RAID *raid;
  int raid_power;               /* How much power we will use to raid.
				   A number from 3 to 9 representing 30
				   to 90 pct of available warriors
				   being used. */
  
  int num_posts_raided;         /* Number of posts we will raid. */
  
  bool raid_adjacent = FALSE;   /* Do we raid an adjacent area only, or 
				   can we pick ones farther away. */
  /* The society must exist and must be aggressive. */
  /* If the society is a noresources society, it sends out raids
     frequently for no reason because it uses life energy to grow. */
  
  if (!soc)
    return;
  
  
  /* Cannot be forced to raid a nonexistent society. If you raid a
     friendly society, you are actually going to defend it. */
  
  if (raid_target > 0 &&
      ((soc2 = find_society_num (raid_target)) == NULL ||
       soc2->vnum == soc->vnum))
    return;
  
  if ((oldarea = find_area_in (soc->room_start)) == NULL)
    return;
      
  /* If it's not a force raid, find something to raid. */
  
  if (!soc2)
    {
      if (soc->raid_hours > 0 || nr (1,SOCIETY_RAID_CHANCE) != 
	  (SOCIETY_RAID_CHANCE/2))
	return;
      
      /* Small chance to raid an align homeland. */
      if (nr (1,651) == 43 && relic_raid_start (soc))
	return;
      
      /* Make sure we have enough members to raid. */
      
      if ((find_num_members (soc, BATTLE_CASTES) < 
	   nr (soc->population_cap/5, soc->population_cap*2/3)) &&
	  (!IS_SET (soc->society_flags, SOCIETY_NORESOURCES) || 
	   nr (1,SOCIETY_RAID_CHANCE) != (SOCIETY_RAID_CHANCE/2)))
	return;
      
      /* Must be aggressive or use kills for resources. */
      
      if (!IS_SET (soc->society_flags, SOCIETY_AGGRESSIVE | SOCIETY_NORESOURCES))
	return;

      /* Most of the time we raid adjacent areas only. This is something
	 to keep the mobs from going too far out into the world from where
	 they started. */
      
      if (nr (1,SOCIETY_RAID_ADJACENT_CHANCE) != 
	  (SOCIETY_RAID_ADJACENT_CHANCE/2))
	{
	  raid_adjacent = TRUE;
	  mark_adjacent_areas (oldarea);
	}
      else
	{
	  raid_adjacent = FALSE;
	  unmark_areas();
	}
      
      
      /* The society choices are done by looking at the relative sizes of
	 the societies... smaller ones = 5, same = 3, larger = 1. */
      for (count = 0; count < 2; count++)
	{      
	  for (soc2 = society_list; soc2; soc2 = soc2->next)
	    {
	      if (soc2 != soc &&
		  (soc->align == 0 ||
		   DIFF_ALIGN (soc2->align, soc->align)) &&
		  /* If it's an adjacent raid, the other society must be 
		     located in an adjacent area...as determined by the 
		     mark_adjacent_areas() code above. */	      
		  (!raid_adjacent ||
		   ((area = find_area_in (soc2->room_start)) != NULL &&
		    IS_MARKED (area))))
		{
		  if (soc2->power < 1)
		    continue;
		  if (soc2->power < soc->power*2/3)
		    soc_choices += 5;
		  else if (soc2->power < soc->power*4/3)
		    soc_choices += 3;
		  else 
		    soc_choices++;		      
		  
		  /* Keep increasing soc_choices, and if it's the first
		     time through, we will use soc_choices to find
		     a random society to hit. If it's the second time
		     through, we stop when we reach the number we
		     chose. The societies are weighted so that
		     raids are more likely against weaker opponents. */
		  

		  if (count == 1 &&
		      soc_choices >= soc_chose)
		    break;		  
		}
	    }
	  if (count == 0)
	    {
	      if (soc_choices < 1)
		{
		  unmark_areas ();
		  return;
		}
	      else
		soc_chose = nr (1,soc_choices);
	      soc_choices = 0;
	    }
	}
      
      unmark_areas();
      if (!soc2)
	return;
    }
  
  
  
  /* Find out the raiding power to be sent to attack. It can be 
     a number from 5 to 10 (50 to 100 pct avail warriors sent) and
     it can be modified by the relative powers of the two societies
     involved in raiding. */
  
  raid_power = 4;
  for (i = 0; i < 6; i++)
    raid_power += nr (0,1);
  
  if (soc->power < soc2->power/2)
    raid_power +=4;
  else if (soc->power > soc2->power*2)
    raid_power -=2;
  raid_power = MID (5,raid_power,10);
  
  /* Find a battle mob not doing anything. */
  
  /* Find a battle mob in the home area that isn't sentinel. */
  
  if ((mob = society_do_activity (soc, 100, "home battle one nosent")) == NULL)
    return;
  
  
  /* Check to see which gates exist and are accessible to our mob. */
  /* This may have problems in that the path goes through other
     societies...will have to work on that to fix it. */
  
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    {
      can_reach_post[i] = FALSE;
      if ((room = find_thing_num (soc2->guard_post[i])) != NULL &&
	  IS_ROOM (room))
	{
	  sprintf (buf, "%d", room->vnum);
	  start_hunting (mob, buf, HUNT_RAID);
	  if (hunt_thing (mob, 0))
	    {
	      can_reach_post[i] = TRUE;
	      post_choices++;
	    }
	  stop_hunting (mob, TRUE);
	}
    }
  /* If the mob cannot reach any posts, bail out. */
  if (post_choices < 1)      
    return;
  
  
  if (nr(1,5) != 3)
    num_posts_raided = MAX(1, post_choices/2);
  else if (nr (1,2) == 1)
    num_posts_raided = 1;
  else
    num_posts_raided = post_choices;
  
  
  /* We can get to a post in this society, so now set up the raid. */
  
  /* Only add a raid rumor if the targeted victim is actually an
     enemy. Otherwise, the rumor is an assist rumor. */
  
  if (soc->align == 0 || DIFF_ALIGN (soc->align, soc2->align))
    add_rumor (RUMOR_RAID, soc->vnum, soc2->vnum, 0, 0);
  
  soc->raid_hours = SOCIETY_RAID_HOURS;
  /* Create a new raid and give it a vnum and set up the society attacker
     and victim defender stats. */
  
  raid = new_raid();
  raid->vnum = find_open_raid_vnum ();
  add_raid_to_list(raid);
  raid->raider_society = soc->vnum;
  raid->victim_society = soc2->vnum;
  raid->hours = (raid_power+2)*5;
  raid->victim_start_power = soc2->power;
  raid->raider_power_lost = 0;
  raid->victim_power_lost = 0;
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    raid->post[i] = 0;
  
  /* Set up the guard posts to be raided by looping through the
     available guard post choices and picking random ones off
     until we reach the number of guard posts we wanted to raid 
     originally. */
  
  post_choices = MID (1, num_posts_raided, NUM_GUARD_POSTS);
  for (i = 0; i < num_posts_raided; i++)
    {    
      if (post_choices < 1)
	break;
      post_chose = nr (1,post_choices);
      post_num = 0;
      for (j = 0; j < NUM_GUARD_POSTS; j++)
	{
	  if (can_reach_post[j] && ++post_num == post_chose)
	    {
	      can_reach_post[j] = FALSE;
	      raid->post[i] = soc2->guard_post[j];
	      post_choices--;
	    }
	}
    }
  
  /* Now send the battle mobs to the correct post. */
  
  for (i = 0; i < num_posts_raided; i++)
    {
      post_chose = nr (0, num_posts_raided-1);
      
      sprintf (buf, "battle nosent end raid vnum %d %d attr v:society:4 %d attr v:society:3 %d",
	       raid->post[post_chose], raid->post[post_chose],
	       raid->vnum, WARRIOR_HUNTER);
      society_do_activity (soc, (raid_power*10)/num_posts_raided, buf);
    }
  return;
}



/* This creates a new RAID for use. */

RAID *
new_raid (void)
{
  RAID *newraid;
  if (raid_free)
    {
      newraid = raid_free;
      raid_free = raid_free->next;
    }
  else
    {
      raid_count++;
      newraid = (RAID *) mallok (sizeof (RAID));
    }
  bzero (newraid, sizeof (RAID));
  return newraid;
}

/* This removes a raid from the raiding list and sends it to the free
   list for reuse. */

void
free_raid (RAID *raid)
{
  int vnum;
  RAID *prev;
  if (!raid)
    return;
  
  vnum = raid->vnum % RAID_HASH;
  
  if (raid == raid_hash[vnum])
    raid_hash[vnum] = raid->next;
  else
    {
      for (prev = raid_hash[vnum]; prev; prev = prev->next)
	{
	  if (prev->next == raid)
	    {
	      prev->next = raid->next;
	      break;
	    }
	}
    }
  
  bzero (raid, sizeof (RAID));
  raid->next = raid_free;
  raid_free = raid;
  return;
}

/* This adds a raid to the table of raids. */

void
add_raid_to_list (RAID *raid)
{
  int vnum;
  if (!raid)
    return;

  vnum = raid->vnum % RAID_HASH;
  
  raid->next = raid_hash[vnum];
  raid_hash[vnum] = raid;
  return;
}

/* This finds a raid based on a vnum given. */

RAID *
find_raid_num (int vnum)
{
  RAID *raid;
  if (vnum < 1 || vnum > RAID_MAX_VNUM)
    return NULL;
  
  for (raid = raid_hash[vnum % RAID_HASH]; raid; raid = raid->next)
    {
      if (raid->vnum == vnum)
	return raid;
    }
  return NULL;
}

/* This finds the next available number for raids. */

int
find_open_raid_vnum (void)
{
  int vnum;
  RAID *raid;
   
  /* Note that we don't start at 0 because raid vnum of 0 is BAD. The
     vnum will be put into the society value of the raiders so that
     the raid knows who to send home at the end of the raid. If this
     number is not kept above 0, then the raid won't know who to
     send home. */
  
  for (vnum = 1; vnum < RAID_MAX_VNUM; vnum++)
    {
      if ((raid = find_raid_num (vnum)) == NULL)
	return vnum;
    }
  return 0;
}

/* This writes all of the raids out to a file. */


void
write_raids (void)
{
  FILE *f;
  int i, j;
  RAID *raid;
  
  if ((f = wfopen ("raid.dat", "w")) == NULL)
    return;
  
  for (i = 0; i < RAID_HASH; i++)
    {
      for (raid = raid_hash[i]; raid; raid = raid->next)
	{
	  fprintf (f, "RAID %d %d %d %d %d %d %d %d \n",
		   raid->vnum,
		   raid->hours, 
		   raid->raider_society, 
		   raid->raider_start_power,
		   raid->raider_power_lost,
		   raid->victim_society, 
		   raid->victim_start_power,
		   raid->victim_power_lost);
	  fprintf (f, "Posts ");
	  for (j = 0; j < NUM_GUARD_POSTS; j++)
	    fprintf (f, "%d ", raid->post[j]);
	  fprintf (f, "\n");
	}
    }
  fprintf (f, "END_OF_RAIDS\n");
  fclose (f);
  return;
}

/* This reads in the raids from the raid file. */


void
read_raids (void)
{
  RAID *raid;
  FILE_READ_SETUP ("raid.dat");
  /* If file isn't there, just return. (This is possible.) */
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY ("RAID")
	{
	  if ((raid = read_raid (f)) != NULL)
	    add_raid_to_list (raid);
	}      
      FKEY ("END_OF_RAIDS")
	     break;
      FILE_READ_ERRCHECK ("raid.dat");
    }
  fclose (f);
  return;
}
	 

/* This reads a single raid in from a file. */

RAID *
read_raid (FILE *f)
{
  RAID *raid;
  int i;

  if (!f)
    return NULL;

  /* Read the base data. */

  raid = new_raid();
  raid->vnum = read_number (f);  
  raid->hours = read_number (f);
  raid->raider_society = read_number (f);
  raid->raider_start_power = read_number (f);
  raid->raider_power_lost = read_number (f);
  raid->victim_society = read_number (f);
  raid->victim_start_power = read_number (f);
  raid->victim_power_lost = read_number (f);

  read_word (f); /* Strip off the word "Posts" */
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    raid->post[i] = read_number (f);
  
  return raid;
}



/* This updates the raids in the world. If the raid gets to 0 hours,
   the raid is removed and the raiders are sent home. */

void
update_raids (void)
{
  RAID *raid, *raidn;
  SOCIETY *society;
  int rhash;
  char buf[STD_LEN];
  int raid_count = 0;
  int raid_end_count = 0;
  for (rhash = 0; rhash < RAID_HASH; rhash++)
    {
      for (raid = raid_hash[rhash]; raid; raid = raidn)
	{
	  raid_count++;
	  raidn = raid->next;
	  if (--raid->hours < 1)
	    {
	      raid_end_count++;
	      if ((society = find_society_num (raid->raider_society)) == NULL)
		{
		  free_raid (raid);
		  continue;
		}
	      sprintf (buf, "raid %d all vnum %d %d end kill attr v:society:4 0 attr v:society:3 %d",		       
		       raid->vnum,
		       society->room_start,
		       society->room_end,
		       WARRIOR_GUARD);
	      /* Send all of the raiders home. */
	      society_do_activity (society, 100, buf);			  
	      free_raid (raid);
	    }
	}
    }
  write_raids();
  return;
}




/* This makes a society attack a thing with a certain name. */

void
society_name_attack (SOCIETY *soc, char *name)
{
  RAID *raid;
  char buf[STD_LEN];
  
  if (!soc || !name || !*name || strlen (name) > 200)
    return;
  
  raid = new_raid();
  raid->vnum = find_open_raid_vnum ();
  raid->hours = 50;
  add_raid_to_list(raid);
  raid->raider_society = soc->vnum;
  
  sprintf (buf, "battle nosent nohunt end kill raid 0 name '%s' ",
	   name);
  
  society_do_activity (soc, 25, buf);
  
  return;
}



/* This marks all areas listed as adjacent to a particular area. Adjacency
   is determined by finding if there's an exit from a room in the 
   area passed as an argument to the area being tested. This is similar
   to the do_exlist code. */


void
mark_adjacent_areas (THING *area_arg)
{
  THING *exarea;
  THING *room, *exroom;
  VALUE *exit;
  
  /* Unmark all areas first. */
  
  unmark_areas();
  
  /* The area must exist and be an area and not the start area. */

  if (!area_arg || !IS_AREA (area_arg) || area_arg == the_world->cont)
    return;
  
  SBIT (area_arg->thing_flags, TH_MARKED);
  
  /* Loop through all rooms in our designated area. */

  for (room = area_arg->cont; room; room = room->next_cont)
    {
      if (!IS_ROOM (room))
	continue;

      /* Loop through all exits in each room and see which ones link
	 to different areas. All of these adjacent areas get marked. */
      
      for (exit = room->values; exit; exit = exit->next)
	{
	  if (exit->type <= 0 || exit->type > REALDIR_MAX ||
	      (exroom = find_thing_num (exit->val[0])) == NULL ||
	      !IS_ROOM (exroom) ||
	      (exarea = find_area_in (exit->val[0])) == NULL ||
	      exarea == the_world->cont ||
	      exarea == area_arg)
	    continue;
	  
	  SBIT (exarea->thing_flags, TH_MARKED);
	}
    }
       
  return;
}


/* This makes a society try to raid the relic room of a different
   alignment. */

bool
relic_raid_start (SOCIETY *soc)
{
  THING *room = NULL, *mob;
  int choices = 0;
  int num_choices = 0, num_chose = 0, choice = 0, count, i;
  RACE *align;
  RAID *raid;
  char buf[STD_LEN];
  /* If no society exists bail out. Also bail out most of the time on
     general principles. */
  
  if (!soc || soc->relic_raid_hours > 0)
    return FALSE;

  /* Society must be big enough. */

  if (soc->population < soc->population_cap*2/3)
    return FALSE;
  
  /* Find something to attempt to track the room we want. */
  
  if ((mob = society_do_activity (soc, 100, "home battle one nosent")) == NULL)
    return FALSE;
  
  /* Loop through aligns and see how many have relic rooms that are
     findable and raidable. */
  
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if ((align = align_info[i]) != NULL &&
	  DIFF_ALIGN(align->vnum, soc->align) &&
	  (room = find_thing_num (align->relic_ascend_room)) != NULL &&
	  IS_ROOM (room))
	{
	  stop_hunting (mob, TRUE);
	  start_hunting_room (mob, room->vnum, HUNT_RAID);
	  if (hunt_thing (mob, 0))
	    choices |= (1 << i);
	}
    }
  
  stop_hunting (mob, TRUE);
  
  if (!mob->in || !IS_ROOM (mob->in))
    return FALSE;

  /* The bits with the proper alignments should be set. */

  /* Now find the number of bits set in the choices integer. I should
     use the neato masking algorithm for this, but I don't feel like it. */

  for (count = 0; count < 2; count++)
    {
      for (i = 0; i < 32; i++)
	{
	  if (IS_SET (choices, (1 << i)))
	    {
	      if (count == 0)
		num_choices++;
	      else
		if (++choice == num_chose)
		  break;
	    }
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    return FALSE;
	  
	  num_chose = nr (1, num_choices);
	}
    }

  if (i >= ALIGN_MAX ||  !IS_SET (choices, (1 << i)))
    return FALSE;

  if ((align = align_info[i]) == NULL ||
      (room = find_thing_num (align->relic_ascend_room)) == NULL ||
      !IS_ROOM (room))
    return FALSE;
  
  /* Set up the relic raid gathering point now. */

  soc->relic_raid_hours = 10;
  soc->relic_raid_gather_point = mob->in->vnum;
  soc->relic_raid_target = align->relic_ascend_room;
  
  if (soc->relic_raid_gather_point == 0)
    {
      soc->relic_raid_hours = 0;
      soc->relic_raid_target = 0;
      return FALSE;
    }
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (align_info[i] &&
	  align_info[i]->relic_ascend_room == soc->relic_raid_target)
	{
	  align = align_info[i];
	  break;
	}
    }
  
  /* Set up the raid. */
  raid = new_raid();
  raid->vnum = find_open_raid_vnum ();
  add_raid_to_list(raid);
  raid->raider_society = soc->vnum;
  raid->victim_society = 0;
  raid->hours = nr (SOCIETY_RAID_HOURS/2, SOCIETY_RAID_HOURS*3/4);
  raid->victim_start_power = 0;
  raid->raider_power_lost = 0;
  raid->victim_power_lost = 0;
  for (i = 0; i < NUM_GUARD_POSTS; i++)
    raid->post[i] = 0;
  raid->post[0] = soc->relic_raid_target;
  
  add_rumor (RUMOR_RELIC_RAID, soc->vnum, align->vnum, 0, 0);
  sprintf (buf, "battle nosent home end raid vnum %d %d  attr v:society:3 %d attr v:society:4 %d", align->relic_ascend_room, 
	   align->relic_ascend_room, WARRIOR_HUNTER, raid->vnum);
  society_do_activity (soc, 10, buf);
  soc->relic_raid_hours = SOCIETY_RAID_HOURS;
  soc->relic_raid_gather_point = 0;
  soc->relic_raid_target = 0;
  return TRUE;
}
