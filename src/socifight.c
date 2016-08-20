#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "track.h"

/* This file will contain some of the AI stuff used for societies
   that are forced into fighting. */


/* This first function makes the society go into alert mode if too
   many things get killed too quickly. */

void
check_society_alert (SOCIETY *soc)
{
  
  if (!soc)
    return;
  
  if (soc->alert < 10 || soc->alert - soc->last_alert < 5 ||
      soc->alert_rally_point || !soc->last_killer ||
      soc->alert_hours > 0 || nr (1,5) != 3)          
    return;
  
  /* If too many of the society members have gotten killed lately, some of
     the society regroups and fights as a group. */
  
  society_alert (soc);
  return;
}

/* This puts the society on alert and brings many units to a central spot. */

void
society_alert (SOCIETY *soc)
{
  int room_num = 0, i;
  
  THING *room = NULL, *oldarea;
  char buf[STD_LEN];
  
  if (!soc || soc->alert_hours > 0)
    return;

  /* Find a room to go to. */
   
  for (i = 0; i < 10; i++)
    {
      room_num = nr (soc->room_start, soc->room_end);      
      if ((room = find_thing_num (room_num)) != NULL &&
	  IS_ROOM (room))
	break;
    }
  if (!room)
    return;
  
  sprintf (buf, "%d", room_num);
  
  /* Set a few hours to get there. */
  
  soc->alert_hours = ALERT_SETUP_HOURS;
  soc->alert_rally_point = room_num;
  
  /* Last alert is now. */
  soc->last_alert = soc->alert;
  
  /* Find the original area where the mobs reset. Only have those go 
     to the central location. */
  
  if ((oldarea = find_area_in (soc->room_start)) == NULL)
    return;

  /* Now make some fighting society mobs hunt this one central location. */
  
  sprintf (buf, "home battle sent end none attr v:society:3 %d vnum %d %d", WARRIOR_GUARD, room_num, room_num);
	   
  society_do_activity (soc, soc->alert, buf);
  return;
}


/* This makes a horde of mobs go and fight raiders. When the raid
   is over, it also brings them back home. */


void
society_fight (SOCIETY *soc)
{
  THING *room;
  char buf[STD_LEN];

  if (!soc)
    return;

  /* Make sure this is a legit alert with a target and a gathering
     point. */
  
  if (!soc->alert_rally_point || !soc->last_killer ||
      !soc->last_killer->in ||
      !CAN_FIGHT (soc->last_killer))
    {
      soc->last_killer = NULL;
      soc->alert_rally_point = 0;
      soc->alert_hours = 0;      
      return;
    }
  
  /* If no room is set, it's the end of an alert, and time to
     send everyone back home. Try to find a random room, and if
     not possible, try again next hour. */
  
  if ((room = find_thing_num (soc->alert_rally_point)) == NULL ||
      !IS_ROOM (room))
    {
      soc->last_killer = NULL;
      soc->alert_rally_point = nr (soc->room_start, soc->room_end);
      if ((room = find_thing_num (soc->alert_rally_point)) == NULL ||
	  !IS_ROOM (room))
	{
	  soc->alert_rally_point = 0;
	  soc->alert_hours = 1;      
	  return;
	}
      soc->last_killer = NULL;
    }
  
  /* Get everything in the area to go toward the killer, 
     or get everything to calm. by going to a random room. */
  
  if (soc->last_killer && soc->last_killer->in &&
      IS_ROOM (soc->last_killer->in))
    {
      sprintf (buf, "home %s nosent end kill in %d name '%s' ",
	       (soc->alert < soc->population ? "battle" : "all"),
	       soc->last_killer->in->vnum, KEY (soc->last_killer));
    }
  else
    {
      sprintf (buf, "home all sent hunt end heal vnum %d %d",
	       soc->room_start, soc->room_end);
    }
  
  society_do_activity (soc, 100, buf);
  
  if (soc->last_killer)
    soc->alert_hours = ALERT_HOURS;
  else
    soc->alert_hours = 0;
  soc->alert_rally_point = 0;
  soc->last_killer = NULL;
  return;
}


/* This deals with what happens when a society member gets killed. 
   (or when a society member kills something) */

void
society_get_killed (THING *killer, THING *vict)
{
  VALUE *vsocval = NULL, *ksocval = NULL;
  SOCIETY *vsoc = NULL, *ksoc = NULL;
  RAID *raid = NULL;
  int raw_needed;

  if (!vict  || !vict->in || IS_ROOM (vict) || IS_AREA (vict->in))
    return;

  
  
   
  if (killer)
    {
      if (killer->fgt && vict == killer->fgt->hunt_victim)
	stop_hunting (killer, FALSE);
      if ((ksocval = FNV (killer, VAL_SOCIETY)) != NULL &&
	  (ksoc = find_society_num (ksocval->val[0])) != NULL &&
	  IS_SET (ksoc->society_flags, SOCIETY_NORESOURCES))
	{
	  /* Add resources to the killer's society if it's a no
	     resources society. This is in addition to "plunder". */
	  
	  if ((raw_needed = raw_needed_by_society (ksoc)) > RAW_NONE &&
	      raw_needed < RAW_MAX)
	    {
	      if (ksoc->raw_curr[raw_needed] < RAW_TAX_AMOUNT)
		ksoc->raw_curr[raw_needed] += nr (LEVEL(vict)/2, LEVEL(vict));
	      else
		ksoc->raw_curr[raw_needed] += 2;
	    }	  	  
	  /* Add small morale for killing an enemy. */
	  add_morale (ksoc, LEVEL(vict)/5);
	}
      if (ksoc && vict->align < ALIGN_MAX && vict->align > 0)
	ksoc->kills[vict->align] += LEVEL(vict)/10;
    }

  if (IS_PC (vict))
    return;

  /* If a society is getting beaten up, the mobs go on higher and
     higher alert. This only happens if the mobs are killed
     in the area where their society is spawned, or if they're
     workers or builders. That way, if raiding mobs are killed, 
     it doesn't matter. */
  
  if ((vsocval = FNV (vict, VAL_SOCIETY)) == NULL ||
      (vsoc = find_society_num (vsocval->val[0])) == NULL)
    return;
  
 
  /* Lower morale of societies with members that get killed. */
  add_morale (vsoc, -LEVEL(vict)/20);

  
  if ((raid = find_raid_num (vsocval->val[4])) != NULL)
    raid->raider_power_lost += POWER(vict);
  
  if (!IS_SET (vsocval->val[2], BATTLE_CASTES) ||
      (vict->in && vict->in->in &&
       (find_area_in (vsoc->room_start) == vict->in->in)))
    {
      vsoc->last_killer = killer;
      vsoc->alert++;
      check_society_alert (vsoc);	      
    }
  
  /* When an overlord is killed, I really should put an 
     update here to say that we remove SOCIETY_OVERLORD flag from
     the society, but I will not. I will leave it to be updated
     in society_update since it's cheaper there, and it's also
     more interesting since there's a delay before the society
     starts acting more normal even after the leader dies. */

  /* If a player kills and the vict is from the most hated 
     society that the player's alignment has, then the player gets
     a bonus. */
  
  if (!killer)
    return;
  
  /* Update killed_by within the society. */

  if (killer->align < ALIGN_MAX && (IS_PC (killer) || ksoc))
    vsoc->killed_by[killer->align] += LEVEL(vict)/75 + 1;
  
  

  /* Pcs get quest points and align hate reduced if they attack the
     most hated enemy of their alignment. */
  
  if (IS_PC (killer) && align_info[killer->align] != NULL &&
      vsoc->vnum == align_info[killer->align]->most_hated_society)
    {
      killer->pc->quest_points += LEVEL(vict)/25 + 1;
      if (killer->pc->align_hate[killer->align] > 0)
	killer->pc->align_hate[killer->align] -= 
	  MIN (killer->pc->align_hate[killer->align], LEVEL(vict)/75+1);
    }
  
  /* If an opp align thing kills, it may encourage the society 
     to turn to a new alignment. May also happen with align 0 vs
     align 0. */
  
  if (DIFF_ALIGN (killer->align, vsoc->align) ||
      (killer->align == 0 && vsoc->align == 0))
    {

    		  
      /* If a society member kills another in a different
	 society, when the victim dies on "home soil", 
	 transfer a few raw materials to the killer society 
	 to represent "plunder". */ 
      
      if (ksoc && ksoc != vsoc)
	{
	  if (vict->in && vict->in->vnum >= vsoc->room_start &&
	      vict->in->vnum <= vsoc->room_end)
	    {
	      
	      int i, raw_amt;
	      VALUE *build;

	      /* Build is only used when it's a pavillion since
		 that means a leader was killed in its own caste
		 house. */
	      if ((build = FNV (vict->in, VAL_BUILD)) != NULL &&
		  (build->val[0] != vsoc->vnum || 
		   !IS_SET (build->val[3], CASTE_LEADER)))
		build = NULL;
	      
	      for (i = 0; i < RAW_MAX; i++)
		{
		  raw_amt = MIN (vsoc->raw_curr[i], 
				 nr (0, LEVEL(vict)/10 - 20));
		  vsoc->raw_curr[i] -= raw_amt;
		  ksoc->raw_curr[i] += raw_amt;
		  /* If vict is a leader, remove some of total raws...*/
		  if (IS_SET (vsocval->val[2], CASTE_LEADER))
		    {
		      vsoc->raw_curr[i] -= vsoc->raw_curr[i]/10;
		      if (build)
			vsoc->raw_curr[i] -= vsoc->raw_curr[i]/8;
		    }
		}		      
	    }
	  if ((raid = find_raid_num (ksocval->val[4])) != NULL &&
	      vsocval->val[0] == raid->victim_society)
	    raid->victim_power_lost += POWER(vict);
	}    
    }	      
  return;
}

/* This checks to see if a society sends some assassins after a player
   messenger carrying a package for an enemy society. */

void
attack_package_holder (THING *obj)
{
  VALUE *package;
  THING *th;
  SOCIETY *soc_to, *soc; /* Attacker society. */
  THING *room, *area;
  int count, num_chose = 0, num_choices = 0;
  char buf[STD_LEN];

  if (!obj || (package = FNV (obj, VAL_PACKAGE)) == NULL || nr (1,2) != 2)
    return;
  th = obj;
  while (th && !IS_PC (th))
    th = th->in;
  
  if (!th || !IS_PC (th))
    return;
  
  if ((room = th->in) == NULL || !IS_ROOM (room))
    return;
  
  if ((area = room->in) == NULL || !IS_AREA (area))
    return;
  
  if ((soc_to = find_society_num (package->val[0])) == NULL ||
       soc_to->align == 0 || DIFF_ALIGN (soc_to->align, th->align))
    return;
  /* Find an enemy society in the area where the player is. */

  for (count = 0; count < 2; count++)
    {      
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (area != find_area_in (soc->room_start) ||
	      !DIFF_ALIGN (th->align, soc->align))
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    break;
	  num_chose = nr (1, num_choices);
	}
    }
  if (!soc || !DIFF_ALIGN (soc->align, th->align) ||
      find_area_in (soc->room_start) != area)
    return;

  
  sprintf (buf, "home battle one nosent end kill name %s", KEY(th));
  society_do_activity (soc, 100, buf);
  return;
}
