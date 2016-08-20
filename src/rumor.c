#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "rumor.h"
#include "objectgen.h"
#include "historygen.h"



RUMOR *rumor_list = NULL;
RUMOR *rumor_free = NULL;
int rumor_count = 0;
int top_rumor = 0;
int bottom_rumor = 2000000000;

const char *rumor_names[RUMOR_TYPE_MAX] =
  {
    "patrol",
    "settle",
    "raid", 
    "switch",
    "defeat",
    "assist",
    "reinforce",
    "relic raid",
    "abandon",
    "plague",
  };


/* This creates a new rumor (or takes one off the free list if it's
   there.) */


RUMOR *
new_rumor (void)
{
  RUMOR *newrumor;
  
  if (rumor_free)
    {
      newrumor = rumor_free;
      rumor_free = rumor_free->next;
    }
  else
    {
      newrumor = mallok (sizeof (RUMOR));
      rumor_count++;
    }
  bzero(newrumor, sizeof (RUMOR));
  return newrumor;
}

/* This removes a rumor from the rumor list and puts it into the free
   list. */


void
free_rumor (RUMOR *rumor)
{
  if (!rumor)
    return;
  
  
  if (rumor->prev)
    rumor->prev->next = rumor->next;
  if (rumor->next)
    rumor->next->prev = rumor->prev;
  if (rumor == rumor_list)
    rumor_list = rumor->next;
  
  rumor->prev = NULL;
  rumor->hours = 0;
  rumor->who = 0;
  rumor->to = 0;
  rumor->type = 0;
  rumor->vnum = 0;
  rumor->next = rumor_free;
  rumor_free = rumor;
  return;
}

/* This lets you add a rumor to the rumor list. */

void
add_rumor (int type, int from, int to, int hours, int vnum)
{
  SOCIETY *soc = NULL, *soc2 = NULL, *soc3 = NULL;
  RACE *align = NULL;
  THING *area = NULL;
  RUMOR *rumor;
  
  if (type < 0 || type >= RUMOR_TYPE_MAX || hours < 0)
    return;
  
  /* A society activity must initiate the rumor... (this will be expanded
     later on to all kinds of activities). */
  
  if ((soc = find_society_num (from)) == NULL)
    return;
  
  /* Defeat an abandon rumors let the whole align know. */
  
  if (type == RUMOR_DEFEAT || type == RUMOR_ABANDON)
    align = find_align (NULL, from);
  
  
  /* If it's a raid, the raid must target a victim. */
  
  if (type == RUMOR_RAID &&
      (soc2 = find_society_num (to)) == NULL)
    return;
  
  /* If it's a switch alignment, the target alignment must exist and
     be nonzero. */
  
  
  if (type == RUMOR_SWITCH &&
      (align = find_align (NULL, to)) == NULL)
     return;
  
  /* If it's a patrol or a settle, it must occur in a real area -- not the
     start area. If it's an abandon, we know where they left, but we
     don't know where they're going. */
  
  if ((type == RUMOR_SETTLE || type == RUMOR_PATROL ||
       type == RUMOR_ABANDON) &&
      ((area = find_thing_num (to)) == NULL ||
       !IS_AREA (area) || area == the_world->cont))
    return;
  
  

 /* So we know a society did something, and the thing it did it to is
     legit, so add this rumor to the list. */
  
  rumor = new_rumor();
  
  rumor->who = from;
  rumor->type = type;
  rumor->to = to;
  rumor->hours = hours;
  if (vnum > 0)
    rumor->vnum = vnum;
  else
    rumor->vnum = ++top_rumor;
  rumor->prev = NULL;
  rumor->next = rumor_list;
  if (rumor_list)
    rumor_list->prev = rumor;
  rumor_list = rumor;
  
  if (!BOOTING_UP)
    add_to_rumor_history (show_rumor (rumor, TRUE));
  
  /* Plague rumors should go to the same align societies, so set the
     align to the society alignment. Note that if the soc align is 0,
     then this still does nothing since those are all separate. */
  if (type == RUMOR_PLAGUE)
    align = find_align (NULL, soc->align);
  
  if (soc)
    add_rumor_to_society (soc, rumor->vnum);
  if (soc2)
    add_rumor_to_society (soc2, rumor->vnum);
  if (area || align)
    {
      for (soc3 = society_list; soc3; soc3 = soc3->next)
	{
	  if (area && find_area_in (soc3->room_start) == area)
	    add_rumor_to_society (soc3, rumor->vnum);
	  if (align && soc3->align > 0 &&
	      !DIFF_ALIGN (soc->align, align->vnum))
	    add_rumor_to_society (soc3, rumor->vnum);
	}
    }
return;
}

/* This adds a rumor to a society's known rumor list. */

void
add_rumor_to_society (SOCIETY *soc, int vnum)
{
  if (!soc)
    return;
  
  soc->rumors_known[(vnum % RUMOR_COUNT_MAX)/32] |= 
    (1 << vnum  % 32);
  return;
}

/* This clears the rumor_history.dat file. */

void
clear_rumor_history (void)
{
  char buf[STD_LEN];
  sprintf (buf, "\\rm %srumor_history.dat", WLD_DIR);
  system(buf);
  rumor_count = 0;
  top_rumor = 0;
  return;
}


/* This returns a rumor type based on the word given for the rumor name. 
   returns RUMOR_TYPE_MAX if it's an error. */

int
find_rumor_from_name (char *txt)
{
  int i;
  if (!txt || !*txt)
    return RUMOR_TYPE_MAX;
  
  for (i = 0; i < RUMOR_TYPE_MAX; i++)
    {
      if (!str_cmp (txt, rumor_names[i]))
	break;
    }
  return i;
}


/* This reads all of the rumors in from the rumor file. */


void
read_rumors (void)
{
  int type, from, to, hours, vnum;
  
  FILE_READ_SETUP("rumors.dat");
  
  FILE_READ_LOOP
    {
      if ((type = find_rumor_from_name (word)) < RUMOR_TYPE_MAX)
	{
	  from = read_number (f);
	  to = read_number (f);
	  hours = read_number (f);
	  vnum = read_number (f);
	  if (top_rumor < vnum)
	    top_rumor = vnum;
	  if (bottom_rumor > vnum)
	    bottom_rumor = vnum;
	  add_rumor (type, from, to, hours, vnum);
	} 
      FKEY("END_OF_RUMORS")
	break;
      FILE_READ_ERRCHECK("rumors.dat");
    }
  fclose (f);
  return;
}

/* This writes all of the rumors to a file. */

void
write_rumors (void)
{
  RUMOR *rumor;
  FILE *f;
  
  if ((f = wfopen ("rumors.dat", "w")) == NULL)
    return;

  /* Write them in reverse order so that they can be read back in
     in correct order. */
  
  for (rumor = rumor_list; rumor && rumor->next; rumor = rumor->next);
  
    
  
  for (; rumor; rumor = rumor->prev)
    {
      if (rumor->type >= 0 && rumor->type < RUMOR_TYPE_MAX &&
	  rumor->type != RUMOR_DEFEAT)
	{
	  fprintf (f, "%s %d %d %d %d\n",
		   rumor_names[rumor->type],
		   rumor->who, 
		   rumor->to, 
		   rumor->hours, 
		   rumor->vnum);
	}
    }
  fprintf (f, "END_OF_RUMORS");
  fclose (f);
  return;
}


/* This updates the rumors list and lets old rumors go away. */

void
update_rumors (void)
{
  RUMOR *rumor, *rumorn;
  SOCIETY *soc;

  int i, bottom_array, top_array;
  int curr_top, curr_bottom;
  

  bottom_rumor = top_rumor;
  
  for (rumor = rumor_list; rumor; rumor = rumorn)
    {
      rumorn = rumor->next;
      rumor->hours++;
      if (nr (RUMOR_HOURS_UNTIL_DELETE, RUMOR_HOURS_UNTIL_DELETE*3/2) < rumor->hours)
	{
	  free_rumor (rumor);      
	  continue;
	}
      if (rumor->vnum > top_rumor)
	top_rumor = rumor->vnum;
      if (rumor->vnum < bottom_rumor)
	bottom_rumor = rumor->vnum;      
    }
  
  /* We are not permitted to have too many rumors or else the propogation
     of rumors between societies won't work.  We don't check how many
     actually exist, we merely check how many slots there are between
     the top_rumor and the bottom_rumor. */

  /* Check how many rumors there are and add a buffer zone to
     deal with the endpoints...*/
  
  if ((top_rumor - bottom_rumor) > RUMOR_COUNT_MAX-100)
    {
      /* Remove all rumors too far back from the top_rumor. */

      for (rumor = rumor_list; rumor; rumor = rumorn)
	{
	  rumorn = rumor->next;
	  if (top_rumor - rumor->vnum > RUMOR_COUNT_MAX-100)
	    free_rumor (rumor);
	}

      /* Now recalc the bottom_rumor. */
      bottom_rumor = top_rumor;
      for (rumor = rumor_list; rumor; rumor = rumor->next)
	{
	  if (rumor->vnum < bottom_rumor)
	    bottom_rumor = rumor->vnum;
	}
    }
  
  if (top_rumor - bottom_rumor > RUMOR_COUNT_MAX - 100)
    {
      log_it ("ERROR in rumors! Couldn't collapse top and bottom!\n");
      return;
    }
  
  /* Now we know that the rumors are close enough together so we can 
     edit the societies to clear their rumor flags as needed. */

  /* Get where the top and bottom fall into the total number of rumors
     allowed. */
  
  curr_top = top_rumor % RUMOR_COUNT_MAX;
  curr_bottom = bottom_rumor % RUMOR_COUNT_MAX;
  
  /* Now find where they fall inside of the arrays of rumors that
     societies have. Make it so we move up to the next Clear out all
     rumors in the array outside of the area we care about atm. */

  top_array = (curr_bottom)/32;
  bottom_array = (curr_top + 31)/32;
  
  if (bottom_array < top_array)
    {
      for (soc = society_list; soc; soc = soc->next)
	{
	  for (i = 0; i < SOCIETY_RUMOR_ARRAY_SIZE; i++)
	    {
	      if (i < bottom_array || i > top_array)
		soc->rumors_known[i] = 0;
	    }
	}
    }
  else /* top_array <= bottom_array */
    {
      for (soc = society_list; soc; soc = soc->next)
	{ 
	  for (i = 0; i < SOCIETY_RUMOR_ARRAY_SIZE; i++)
	    {
	      if (i < bottom_array && i > top_array)
		soc->rumors_known[i] = 0;
	    }
	}
    }
  
  write_rumors();
  return;
}




/* This is the beginnings of the rumor command. */

void
do_rumors (THING *th, char *arg)
{
  RUMOR *rumor;
  char buf[STD_LEN];
  int rumor_type = RUMOR_TYPE_MAX;
  bigbuf[0] = '\0';
  if (!th || !th->in || !IS_PC (th))
    return;
  
  if (LEVEL (th) < MAX_LEVEL || !str_cmp (arg, "mortal"))
    {
      send_mortal_rumor (th);
      return;
    }
  
  if (arg && *arg)
    {
      for (rumor_type =0 ; rumor_type < RUMOR_TYPE_MAX; rumor_type++)
	{
	  if (!str_prefix (arg, rumor_names[rumor_type]))
	    break;
	}
    }
			   
  sprintf (buf, "Bottom rumor: %d Top rumor: %d\n\n\n\r",
	   bottom_rumor, top_rumor);
  add_to_bigbuf (buf);
  add_to_bigbuf ("A list of the current rumors:\n\n\r");
  for (rumor = rumor_list; rumor; rumor = rumor->next)
    {
      if (rumor->type < 0 || rumor->type >= RUMOR_TYPE_MAX || 
	  rumor->hours < 0 ||
	  /* See rumors by type. */
	  (rumor_type < RUMOR_TYPE_MAX && rumor->type != rumor_type))
	continue;
      
      /* A society activity must initiate the rumor... (this will be expanded
	 later on to all kinds of activities). */
      
      sprintf (buf, "%s%s\n\r", show_rumor (rumor, TRUE),
	       show_rumor_time (rumor, TRUE));
      add_to_bigbuf (buf);
      
    }
  send_bigbuf (bigbuf, th);
  return;
}

/* This gives a mort an update about pending quests and rumors and things
   that the player might be interested in doing or knowing about. */

void
do_news (THING *th, char *arg)
{
  if (!th || !IS_PC (th))    
    return;

  /* First, if the person asks for delivery news, give a small chance
     of having something to deliver. */
  
  if ((!str_cmp (arg, "delivery") || !str_cmp (arg, "package")) &&
      get_delivery_quest (th))
    return;
  
  send_mortal_rumor (th);
  return;
}

/* This tries to make a mob tell a rumor to a player. */

void
send_mortal_rumor (THING *th)
{
  THING *mob, *area;
  VALUE *socval;
  RACE *align;
  SOCIETY *soc = NULL, *soc2;
  int count;
  int mob_choices = 0, mob_chose = 0;
  char buf[STD_LEN];
  

  /* If a mortal wants to get a rumor...first find a mob
     that can give rumors. It must be able to talk and
     it must be of the same alignment as the player. */

  for (count = 0; count < 2; count++)
    {      
      for (mob = th->in->cont; mob; mob = mob->next_cont)
	{
	  if (!CAN_TALK (mob) || !CAN_MOVE (mob) ||
	      DIFF_ALIGN (th->align, mob->align) ||
	      IS_PC (mob) || mob->position <= POSITION_SLEEPING)
	    continue;      
	  if (count == 0)
	    mob_choices++;
	  else if (--mob_chose < 1)
	    break;
	}
      if (count == 0)
	{
	  if (mob_choices < 1)
	    {
	      stt ("No one here wants to trade information.\n\r", th);
	      return;
	    }
	  
	  mob_chose = nr (1, mob_choices);
	}
    }
  
  if (!mob)
    {
      stt ("No one around here has any news.\n\r", th);
      return;
    }
  
  /* If the mob exists, it will only give rumors once in a while
     and players can only get rumors once every N seconds rl. */
  
  
  if (nr (1,3) == 2 || 
      (LEVEL(th) < MAX_LEVEL &&
       (current_time - th->pc->last_rumor) < 5))
    {
      do_say (mob, no_rumor_message());
      /* th->pc->last_rumor = current_time; */
      return;	  
    }
  
  /* Otherwise find some rumor to show the player. */
  
  /* First try kidnapped messages. */
  
  if ((socval = FNV (mob, VAL_SOCIETY)) != NULL &&
      (soc = find_society_num (socval->val[0])) != NULL)
    {
      if (mob->align > 0 && send_kidnapped_message (th, mob))
	return;
      
      if (send_overlord_message (th, mob))
	return;
    }
  

  if ( nr (1,32) == 4 && socval && soc &&
       (align = find_align (NULL, soc->align)) != NULL &&
       (soc2 = find_society_num (align->most_hated_society)) != NULL &&
       (soc2->align == 0 || DIFF_ALIGN (soc2->align, th->align)) &&
       (area = find_area_in (soc2->room_start)) != NULL)
    {
      sprintf (buf, "The %s could really use your help. The %s of %s have been attacking us a lot lately and we need your help to stop them!",
	       align->name, soc2->pname, NAME(area));
      do_say (mob, buf);
      return;
    }
  
  if (nr (1,3) != 2)
    do_say (mob, show_random_rumor_message(soc));
  else
    do_say (mob, show_random_quest_message(th->align));  
  return;
}



/* This takes a rumor and formats it the way it will look to an admin. */

char *
show_rumor (RUMOR *rumor, bool is_admin)
{
  static char buf[STD_LEN*2];
  
  SOCIETY *soc = NULL, *soc2 = NULL;
  RACE *align = NULL;
  THING *area = NULL, *oldarea = NULL, *to_area = NULL;
  buf[0] = '\0';
  
  if (!rumor || rumor->type < 0 || 
      rumor->type >= RUMOR_TYPE_MAX || rumor->hours < 0)
    {
      sprintf (buf, "Error. Bad rumor.\n\r");
      return buf;
    }
  
  /* Assist rumors are a special case. They are a rumor of an align
     helping a society, so they get done separately. */
  
  if (rumor->type == RUMOR_ASSIST)
    {
      if ((align = find_align (NULL, rumor->who)) == NULL ||
	  (soc = find_society_num (rumor->to)) == NULL ||
	  (to_area = find_area_in (soc->room_start)) == NULL)
	{
	  if (is_admin)
	    sprintf (buf, "Bad Rumor: Type: %d Who: %d To: %d ",
		     rumor->type, rumor->who, rumor->to);
	  else
	    sprintf (buf, "Nothing happened ");
	}
      else
	sprintf (buf, "The %ss %sed the %s%s%s of %s ",
		 align->name, rumor_names[rumor->type],
		 soc->adj, (*soc->adj ? " " : ""),
		 soc->pname, NAME(to_area));
      return buf;
    }

  else if ((soc = find_society_num (rumor->who)) == NULL ||
      (oldarea = find_area_in (soc->room_start)) == NULL ||
      !IS_AREA (oldarea))
    {
      if (is_admin)
	sprintf (buf, "Bad Rumor: Type: %d Who: %d To: %d ",
		 rumor->type, rumor->who, rumor->to);
      else
	sprintf (buf, "Nothing happened ");
      return buf;
      
    }

  /* Plague rumors say a society was stricken with the plague. */
  
  else if (rumor->type == RUMOR_PLAGUE)
    {
      sprintf (buf, "The %s%s%s of %s were stricken with the %s ",
	       soc->adj, (*soc->adj ? " " : ""),
	       soc->pname, NAME(oldarea), rumor_names[rumor->type]);
    }
  
  /* Raid/reinforce has a from society and a to society. 
     : society a raids/reinforces society b. */
  
  else if (rumor->type == RUMOR_RAID || 
      rumor->type == RUMOR_REINFORCE)
    {
      if ((soc2 = find_society_num (rumor->to)) == NULL ||
	  (to_area = find_area_in (soc2->room_start)) == NULL ||
	  !IS_AREA (to_area))
	{
	  if (is_admin)
	    sprintf (buf, "Bad Rumor: Type: %d Who: %d To: %d ",
		     rumor->type, rumor->who, rumor->to);
	  else
	    sprintf (buf, "Nothing happened ");
	}
      else 
	sprintf (buf, "The %s%s%s of %s %s%sd the %s%s%s of %s ",
		 soc->adj, (*soc->adj ? " " : ""),
		 soc->pname, NAME(oldarea), rumor_names[rumor->type],
		 (rumor->type == RUMOR_REINFORCE ? "" : "e"),
		 soc2->adj, (*soc2->adj ? " " : ""),
		 soc2->pname, NAME(to_area));
    }
  /* Relic raid is from a society to an alignment. */
  else if (rumor->type == RUMOR_RELIC_RAID)
    {
      if ((soc = find_society_num (rumor->who)) == NULL ||
	  (align = find_align (NULL, rumor->to)) == NULL ||
	  (to_area = find_area_in (align->relic_ascend_room)) == NULL ||
	  (oldarea = find_area_in (soc->room_start)) == NULL)
	{  
	  if (is_admin)
	    sprintf (buf, "Bad Rumor: Type: %d Who: %d To: %d ",
		     rumor->type, rumor->who, rumor->to);
	  else
	    sprintf (buf, "Nothing happened ");
	}
      else
	{
	  sprintf (buf, "The %s%s%s of %s %sed the %s in %s ",
		   soc->adj, (*soc->adj ? " " : ""),
		   soc->pname, NAME(oldarea), rumor_names[rumor->type],
		   align->name, 
		   NAME(to_area));
	}
    }
  /* Defeat rumors just say someone got defeated. */
  else if (rumor->type == RUMOR_DEFEAT)
    {
      sprintf (buf, "The %s%s%s of %s were %sed ",
	       soc->adj, (*soc->adj ? " " : ""),
	       soc->pname, 
	       NAME(oldarea),
	       rumor_names[rumor->type]);
    }
  /* Abandon rumors just say they abandoned their homes. */
  else if (rumor->type == RUMOR_ABANDON)
    {
      if ((area = find_thing_num (rumor->to)) == NULL)
	{
	   
	  if (is_admin)
	    sprintf (buf, "Bad Rumor: Type: %d Who: %d To: %d ",
		     rumor->type, rumor->who, rumor->to);
	  else
	    sprintf (buf, "Nothing happened ");
	}
      
      sprintf (buf, "The %s%s%s %sed %s ",
	       soc->adj, (*soc->adj? " " : ""), soc->pname,
	       rumor_names[rumor->type],
	       NAME (area));
    }
  /* Switch rumors say they switched to an alignment. */
  else if (rumor->type == RUMOR_SWITCH)
    {
      if ((align = find_align (NULL, rumor->to)) == NULL)
	{
	  if (is_admin)
	    sprintf (buf, "Bad Rumor: Type: %d Who: %d To: %d ",
		     rumor->type, rumor->who, rumor->to);
	  else
	    sprintf (buf, "Nothing happened ");
	}
      sprintf (buf, "The %s%s%s of %s %sed to the %s side ",
	       soc->adj, (*soc->adj ? " " : ""),
	       soc->pname, NAME (oldarea), rumor_names[rumor->type],
	       align->name);
    }
  /* Settle/patrol rumors say they went to a different area and did
     something. */
  
  else if (rumor->type == RUMOR_SETTLE || rumor->type == RUMOR_PATROL)
    {
      if (((area = find_thing_num (rumor->to)) == NULL ||
	   !IS_AREA (area) || area == the_world->cont))
	{
	  if (is_admin)
	    sprintf (buf, "Bad Rumor: Type: %d Who: %d To: %d ",
		     rumor->type, rumor->who, rumor->to);
	  else
	    sprintf (buf, "Nothing happened ");
	}
      sprintf (buf, "The %s%s%s of %s %s%s in %s ",
	       soc->adj, (*soc->adj ? " " : ""),
	       soc->pname, NAME (oldarea), rumor_names[rumor->type],
	       (rumor->type == RUMOR_SETTLE ? "d" :
		rumor->type == RUMOR_PATROL ? "led" : "ed"),
	       NAME (area));
    }
  return buf;
}



/* This appends the current rumor to the end of the rumor history file. */

void
add_to_rumor_history (char *txt)
{
  FILE *f;
  
  
  if (!txt || !*txt ) 
    return;
  
  if ((f = wfopen ("rumor_history.dat", "a+")) == NULL)
    return;
  
  
  fprintf (f, "%s: %s\n", c_time (current_time), txt);
  fclose (f);
  return;
}


/* This sends the time that a rumor happened to a player. */

char *
show_rumor_time (RUMOR *rumor, bool is_admin)
{
  static char buf[STD_LEN];
  
  int hours;
  
  if (!rumor)
    return ".";

  if (is_admin)
    sprintf (buf, "%d hour%s ago.", rumor->hours, 
	     (rumor->hours == 1 ? "" : "s"));
  else
    {
      /* Get approximate hours. */
      hours = rumor->hours + 
	nr (0, rumor->hours/4)-
	nr(0, rumor->hours/4);
      
      if (hours < NUM_HOURS / 3)
	sprintf (buf, "a few hours ago.");
      else if (hours < NUM_HOURS *2/3)
	sprintf (buf, "several hours ago.");
      else if (hours < NUM_HOURS *4/3)
	sprintf (buf, "about a day ago.");
      else
	{
	  hours /= NUM_HOURS;
	  if (hours < NUM_DAYS /3)
	    sprintf (buf, "a few days ago.");
	  else if (hours < NUM_DAYS*2/3)
	    sprintf (buf, "several days ago.");
	  else if (hours < NUM_DAYS*4/3)
	    sprintf (buf, "about a week ago.");
	  else
	    {
	      hours /= NUM_DAYS;
	      if (hours < NUM_WEEKS/2)
		sprintf (buf, "a few weeks ago.");
	      else if (hours < NUM_WEEKS*4/3)
		sprintf (buf, "about a month ago.");
	      else
		sprintf (buf, "over a month ago.");
	    }
	}
    }
  return buf;
}
		     

/* This creates a random message if the person doesn't have any
   rumors to tell right now. This just builds a random message
   in pieces using a sequence of random sentence fragments. */

char *
no_rumor_message (void)
{
  int num;
  static char buf[STD_LEN];
  buf[0] = '\0';
  
  num = nr (1,9);
  
  if (num == 1)
    strcat (buf, "Well, ");
  else if (num == 2)
    strcat (buf, "Ya know, ");
  else if (num == 3)
    strcat (buf, "Sorry, ");
  else if (num == 4)
    strcat (buf, "You know, ");
  else if (num == 5)
    strcat (buf, "I'm sorry, ");
  else if (num == 6)
    strcat (buf, "I'm sorry, but ");
  else if (num == 7)
    strcat (buf, "Wish I could say more, but ");
  else if (num == 8)
    strcat (buf, "I wish I cold help you, but ");
  else if (num == 9)
    strcat (buf, "");
  
  strcat (buf, "I");

  num = nr (1,10);
  
  if (num == 1)
    strcat (buf, " haven't heard about ");
  else if (num == 2)
    strcat (buf, " don't know ");
  else if (num == 3)
    strcat (buf, " don't recall ");
  else if (num == 4)
    strcat (buf, " don't seem to recall ");
  else if (num == 5)
    strcat (buf, " can't recall ");
  else if (num == 6)
    strcat (buf, " don't know of ");
  else if (num == 7)
    strcat (buf, " don't remember ");
  else if (num == 8)
    strcat (buf, " can't think of ");
  else if (num == 9)
    strcat (buf, " can't recall ");
  else if (num == 10)
    strcat (buf, "'m drawing a blank about ");
  
  num = nr(1,6);

  
  if (num == 1)
    strcat (buf, "anything");
  else if (num == 2)
    strcat (buf, "any interesting things");
  else if (num == 3)
    strcat (buf, "anything interesting");
  else if (num == 4)
    strcat (buf, "anything noteworthy");
  else if (num == 5)
    strcat (buf, "anything you would be interested in");
  else if (num == 6)
    strcat (buf, "anything of use");

  
  num = nr (1,4);
  
  if (num == 1)
    strcat (buf, ".");
  else if (num == 2)
    strcat (buf, " lately.");
  else if (num == 3)
    strcat (buf, " recently.");
  else if (num == 4)
    strcat (buf, " over the last few weeks.");
  
  return buf;
}

/* This picks and shows a random rumor message to a player. It also
   wraps the message in a little bit of "wordiness" so it doesn't
   get too boring. */


char *
show_random_rumor_message(SOCIETY *soc)
{
  static char buf[STD_LEN];
  RUMOR *rumor;
  
  int rumor_choices = 0, rumor_chose = 0, num, count, choice = 0;
  int rumor_mod_vnum;
  
  /* If no society given then just choose any old rumor. Or if the random
     number comes up since rumors don't propogate too well. */
  if (!soc || nr (1,8) == 3)
    {   
      for (count = 0; count < 2; count++)
	{
	  for (rumor = rumor_list; rumor; rumor = rumor->next)
	    {
	      if (count == 0)
		rumor_choices++;
	      else if (++choice == rumor_chose)
		break;
	    }
	  if (count == 0)
	    {
	      if (rumor_choices < 1)
		return no_rumor_message();
	      rumor_chose = nr (1, rumor_choices);
	    }
	}
    }
  else  /* Society mob so only give rumors for things we know about. */
    {
      for (count = 0; count < 2; count++)
	{	  
	  for (rumor = rumor_list; rumor; rumor = rumor->next)
	    {
	      rumor_mod_vnum = rumor->vnum % RUMOR_COUNT_MAX;
	      if (IS_SET (soc->rumors_known[rumor_mod_vnum/32],
			  (1 << rumor_mod_vnum % 32)))
		{
		  if (count == 0)
		    rumor_choices++;
		  else if (--rumor_chose < 1)
		    break;
		}
	    }
	  if (count == 0)
	    {
	      if (rumor_choices < 1)
		return no_rumor_message();
	      rumor_chose = nr (1, rumor_choices);
	    }
	}
    }
  
  /* Try for raw material rumor. */
  if (nr (1,5) == 3 && soc)
    {
      int raw_choices = 0;
      int raw_chose = RAW_MAX;
      int i;
      for (count = 0; count < 2; count++)
	{
	  for (i = 1; i < RAW_MAX; i++)
	    {
	      if (soc->raw_want[i] ||
		  soc->raw_curr[i] < RAW_TAX_AMOUNT/2)
		{
		  if (count == 0)
		    raw_choices++;
		  else if (--raw_chose < 1)
		    break;
		}
	    }
	  if (count == 0)
	    {
	      if (raw_choices == 0)
		break;
	      raw_chose = nr (1, raw_choices);
	    }
	}
      /* Found an appropriate raw we need... */
      if (i >= 0 && i < RAW_MAX)
	{
	  return show_raw_need_message ("we", i);
	}
    }
  
  
  
  
  if (!rumor)
    return no_rumor_message();
  
  num = nr (1,5);
  
  buf[0] = '\0';
  if (num == 1)
    strcat (buf, "");
  else if (num == 2)
    strcat (buf, "I heard that ");
  else if (num == 3)
    strcat (buf, "Someone told me that ");
  else if (num == 4)
    strcat (buf, "It seems that ");
  else if (num == 5)
    strcat (buf, "It's been going around that ");
  
  strcat (buf, show_rumor (rumor, FALSE));
  strcat (buf, show_rumor_time (rumor, FALSE));
  return buf;
}
	
/* This sends a random quest message to a player depending on if the alignment
   or the society is in need of raw materials. */
 
char *
show_random_quest_message (int align_num)
{
  SOCIETY *soc;
  RACE *align;
  THING *area;
  int i;
  static char buf[STD_LEN];
  int rawtype = RAW_MAX;
  int num_choices = 0, num_chose = 0, choice = 0;

  /* This is the name of the alignment or the society and area the
     society is in in question. */
  char namebuf[STD_LEN];
  
  /* Some checks */
  
  if (align_num < 1 || align_num >= ALIGN_MAX ||
      (align = align_info[align_num]) == NULL)
    return no_rumor_message();
  
  namebuf[0] = '\0';
  buf[0] = '\0';
  /* Find alignment needs */
  
  if (nr (1,4) == 3)
    {
      for (i = 1; i < RAW_MAX; i++)
	{
	  if (align->raw_want[i] > 0 ||
	      align->raw_curr[i] < RAW_TAX_AMOUNT)
	    num_choices++;
	}
      if (num_choices < 1)
	return no_rumor_message();
      
      num_chose = nr (1, num_choices);
      
      for (i = 1; i < RAW_MAX; i++)
	{
	  if ((align->raw_want[i] > 0 ||
	       align->raw_curr[i] < RAW_TAX_AMOUNT) &&
	      ++choice == num_chose)
	    break;
	}
      
      if (i == RAW_MAX)
	return no_rumor_message ();
      
      rawtype = i;
      sprintf (namebuf, "the %s", align->name);
    }
  else
    {
      /* Find society needs. */
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (DIFF_ALIGN (soc->align, align_num))
	    continue;
	  
	  num_choices++;
	}
      
      if (num_choices < 1)
	return no_rumor_message ();
  
      num_chose = nr (1, num_choices);
      
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (DIFF_ALIGN (soc->align, align_num))
	    continue;
	  
	  if (++choice == num_chose)
	    break;
	}
      
      if (!soc)
	return no_rumor_message();
      
      if ((area = find_area_in (soc->room_start)) == NULL ||
	  !IS_AREA (area))
	return no_rumor_message();
      
      num_choices = 0;
      num_chose = 0;
      choice = 0;
      
      for (i = 1; i < RAW_MAX; i++)
	{
	  if (soc->raw_want[i] > 0 ||
	      soc->raw_curr[i] < RAW_TAX_AMOUNT)
	    num_choices++;
	}
      
      if (num_choices < 1)
	return no_rumor_message ();
      
      num_chose = nr (1, num_choices);
      for (i = 1; i < RAW_MAX; i++)
	{
	  if ((soc->raw_want[i] > 0 ||
	       soc->raw_curr[i] < RAW_TAX_AMOUNT) &&
	      ++choice == num_chose)
	    break;
	}
      
      if (i == RAW_MAX)
	return no_rumor_message();
      
      rawtype = i;      
      sprintf (namebuf, "the %s of %s", soc->pname, NAME (area));
    }
  
  
  return show_raw_need_message (namebuf, rawtype);
}

char *
show_raw_need_message (char *namebuf, int rawtype)
{
  static char buf[STD_LEN];
  int num;
  num = nr (1,5);
  buf[0] = '\0';
  if (num == 1)
    strcat (buf, "");
  else if (num == 2)
    strcat (buf, "I heard that ");
  else if (num == 3)
    strcat (buf, "Someone told me that ");
  else if (num == 4)
    strcat (buf, "It seems that ");
  else if (num == 5)
    strcat (buf, "It's been going around that ");
  
  strcat (buf, namebuf);
  buf[0] = UC(buf[0]);
  
  num = nr (1,6);
  strcat (buf, " ");
  if (num == 1)
    strcat (buf, "need ");
  else if (num == 2)
    strcat (buf, "want ");
  else if (num == 3)
    strcat (buf, "are in need of ");
  else if (num == 4)
    strcat (buf, "are short on ");
  else if (num == 5)
    strcat (buf, "have exhausted their stores of ");
  else
    strcat (buf, "could use some ");
  
  strcat (buf, gather_data[rawtype].raw_pname);
  strcat (buf, ".\n\r");
  return buf;
}


/* This lets a thing in a society share rumors with allies. */

void
share_rumors (THING *th)
{
  VALUE *socval, *socval2;
  SOCIETY *soc, *soc2;
  THING *mob;
  int i, choice;

  /* Sanity checks plus if this thing is of align 0, it has no
     allies outside its society so there's no reason to even 
     do this. */
  
  if (!th || !th->in || th->align == 0 || !CAN_TALK (th) ||
      (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_num (socval->val[0])) == NULL)
    return;

  for (mob = th->in->cont; mob; mob = mob->next_cont)
    {
      if (mob == th || mob->align == 0 || !CAN_TALK(mob) ||
	  DIFF_ALIGN (th->align, mob->align) ||
	  (socval2 = FNV(mob, VAL_SOCIETY)) == NULL ||
	  socval->val[0] == socval2->val[0] ||
	  (soc2 = find_society_num (socval2->val[0])) == NULL)
	continue;
      
      /* Pick some random blocks of rumors to share. */
      for (i = 0; i < 10; i++)
	{
	  /* Pick which rumors to update...*/
	  choice = nr (1, SOCIETY_RUMOR_ARRAY_SIZE)-1;
	  if (nr (1,2) == 1)
	    soc2->rumors_known[choice] |= soc->rumors_known[choice];
	  else
	    soc->rumors_known[choice] |= soc2->rumors_known[choice];
	}
      break;
    }
  return;
}



/* This sends a message about an enemy overlord. It checks all nonallied
   societies and picks one with an overlord to tell the player about. */

bool
send_overlord_message (THING *th, THING *mob)
{
  THING *overlord = NULL;
  
  SOCIETY *soc;
  int count, num_choices = 0, num_chose = 0;
  VALUE *socval;
  char buf[STD_LEN];
  THING *area = NULL;
  if (nr (1,45) != 2 ||
      !th || !mob || !IS_PC (th) || IS_PC (mob) ||
      (socval = FNV (mob, VAL_SOCIETY)) == NULL ||
      DIFF_ALIGN (th->align, mob->align))
    return FALSE;
		     
  for (count = 0; count < 2; count++)
    {
      for (soc = society_list; soc; soc = soc->next)
	{
	  if (!DIFF_ALIGN (soc->align, mob->align) ||
	      (overlord = find_society_overlord (soc)) == NULL)
	    continue;
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}

      if (count == 0)
	{
	  if (num_choices < 1)
	    return FALSE;
	  num_chose = nr (1, num_choices);
	}
    }
  
  if (!overlord || !soc || 
      (socval = FNV (overlord, VAL_SOCIETY)) == NULL ||
      socval->val[0] != soc->vnum ||
      !is_named (overlord, "overlord"))
    return FALSE;
  
  area = find_area_in (soc->room_start);

  sprintf (buf, "The %s of %s have a powerful leader named %s.",
	   society_pname (soc), (area ? NAME(area) : "these lands"),
	   (socval->word && *socval->word ? socval->word : "something that I can't recall right now"));
  do_say (mob, buf);
  return TRUE;
}
      

/* This gives a player a "Delivery" quest to take a package to a
   leader of another society. It is a timed quest and other
   societies and alignments will try to stop the player from 
   completing the quest. */

bool
get_delivery_quest (THING *th)
{
  THING *leader_mob = NULL, *society_mob = NULL, *obj = NULL, *mob;
  SOCIETY *start_soc = NULL, *weakest_society = NULL, *soc = NULL;
  VALUE *socval = NULL, *package = NULL;
  int lowest_power = 100000000;
  int count, num_choices = 0, num_chose = 0;
  char buf[STD_LEN];
  int weapon_type = -1;
  int i;
  char name[OBJECTGEN_NAME_MAX][STD_LEN];
  char color[OBJECTGEN_NAME_MAX][STD_LEN];
  THING *area = NULL;
  
  if (!th || !IS_PC (th) || !th->in)
    return FALSE;
  for (count = 0; count < 2; count++)
    {
      for (mob = th->in->cont; mob; mob = mob->next_cont)
	{
	  if ((socval = FNV (mob, VAL_SOCIETY)) == NULL ||
	      (start_soc = find_society_num (socval->val[0])) == NULL ||
	      DIFF_ALIGN (start_soc->align, th->align) ||
	      DIFF_ALIGN (mob->align, th->align) || 
	      mob->position < POSITION_STANDING ||
	      mob->align == 0 ||
	      !CAN_TALK (mob))
	    continue;
	  
	  if (IS_SET (socval->val[2], CASTE_LEADER))
	    {
	      if (count == 0)		
		num_choices++;
	      else if (--num_chose < 1)
		{
		  leader_mob = mob;
		  society_mob = mob;
		  break;
		}
	    }
	  else
	    society_mob = mob;
	}
      
      if (count == 0)
	{
	  if (num_choices <1)
	    break;
	  num_chose = nr (1, num_choices);
	}
    }
  
  /* Nothing at all...*/
  if (!society_mob)
    {
      stt ("Nobody here has any packages to deliver.\n\r", th);
      return TRUE;
    }

  /* No leader...*/
  if (!leader_mob)
    {
      do_say (society_mob, "Sorry, I don't have anything for you to deliver. You should talk to one of our leaders.");
      return TRUE;
    }
  /* Make sure there's a target society to send supplies to. It sends them
     to the weakest society. If this is the weakest society on
     this side, don't do anything. */
  
  for (soc = society_list; soc; soc = soc->next)
    {
      if (DIFF_ALIGN (soc->align, start_soc->align))
	continue;
      
      if (soc->power < lowest_power &&
	  (area = find_area_in (soc->room_start)) != NULL)
	{
	  weakest_society = soc;
	  lowest_power = soc->power;
	}
    }
  
	
  /* Most of the time the leader ignores you. They also ignore you
     if you've asked for a quest recently or if the weakest society on
     this side doesn't exist, or if this is the weakest society or
     if the package can't be made. */
  if (nr (1,15) != 4 || 
      (LEVEL (th) < MAX_LEVEL && current_time - th->pc->last_rumor < 120) ||
      !weakest_society || weakest_society == soc ||
      (obj = create_thing (PACKAGE_VNUM)) == NULL)
    {
      do_say (leader_mob, "Sorry, I don't have anything for you to deliver right now."); 
      if (current_time - th->pc->last_rumor >= 120)
	th->pc->last_rumor = current_time;
      return TRUE;
    }

  
  
  /* Make up a delivery. */
  
  do
    obj->wear_pos = nr (ITEM_WEAR_NONE + 1, ITEM_WEAR_MAX-1);
  while (obj->wear_pos == ITEM_WEAR_BELT);
  weapon_type = generate_weapon_type();
  objectgen_generate_names (name, color, obj->wear_pos,
			    weapon_type, DEITY_LEVEL, NULL);
  
  for (i = 0; i < OBJECTGEN_NAME_MAX; i++)
    {
      echo (name[i]);
      echo ("\n\r");
    }
  objectgen_setup_names (obj, name, color, NULL);
  
  /* Only 30 ticks to deliver this. */
  obj->timer = PACKAGE_DELIVERY_HOURS;
  if ((package = FNV (obj, VAL_PACKAGE)) == NULL)
    {
      package = new_value();
      package->type = VAL_PACKAGE;
      add_value (obj, package);
    }
  package->val[0] = weakest_society->vnum;
  
  /* Find the power of the package. 1/3 of all resources. */
  
  
  package->val[1] = 0;

  
  /* Maybe players will give resources to a society then take a package
     from it to another to "double dip" in the rewards. =D */

  for (i = 0; i < RAW_MAX; i++)
    {
      package->val[1] += start_soc->raw_curr[i]/20;
      start_soc->raw_curr[i] -= start_soc->raw_curr[i]/20;
    }
  
  
  sprintf (buf, "%s, lease deliver %s \x1b[1;37mto the %s of %s within the next day.",
	   NAME(th), NAME(obj), society_pname(weakest_society), NAME(area));
  do_say (leader_mob, buf); 
  do_say (leader_mob, "Be careful and never let it out of your possession.");
  thing_from (obj);
  thing_to (obj, th);
  act ("@1n give@s @2n to @3f.", leader_mob, obj, th, NULL, TO_ALL);
  th->pc->no_quit = PACKAGE_DELIVERY_HOURS + 4;
  th->pc->last_rumor = current_time;
  return TRUE;
}
  
