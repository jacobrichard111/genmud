#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"

/* Ok Arkane finally wore me down so here are these #&$%&$@&
   commands. :P */


void
do_skills (THING *th, char *arg)
{
  do_practice (th, "zskill");
}

void 
do_spells (THING *th, char *arg)
{
  do_practice (th, "zspell");
}

void
do_proficiencies (THING *th, char *arg)
{
  do_practice (th, "zprof");
}

void 
do_poisons (THING *th, char *arg)
{
  do_practice (th, "zpoison");
}

void 
do_traps (THING *th, char *arg)
{
  do_practice (th, "ztrap");
}

void
do_practice (THING *th, char *arg)
{
  SPELL *spell = NULL;
  THING *teacher = NULL;
  VALUE *tlist[3];
  char buf[STD_LEN];
  int i, max_prac = MAX_PRAC, count = 0, j, type = SPELLT_MAX;
  bool found = FALSE, is_teacher = FALSE, missing_prereq = FALSE;
  bool nolearn = FALSE;
  char arg1[STD_LEN];
  
  if (!th || !th->in || !IS_PC (th))
    return;
  
  
  if (*arg == 'z')
    for (type = 0; type < SPELLT_MAX; type++)
      {
	if (!str_prefix (arg + 1, spell_types[type]))
	  {
	    arg = f_word (arg, arg1);
	    break;
	  }
      }
  
  fix_pc (th);
  for (i = 0; i < 3; i++)
    tlist[i] = NULL;
  
  /* Check for practitioners */
  
  /* Skip over practicing if we type prac spells or prac skills etc... */
  
  if (type == SPELLT_MAX)
    {
  
      for (teacher = th->in->cont; teacher; teacher = teacher->next_cont)
	{
	  if (!CAN_MOVE (teacher) && !CAN_FIGHT(teacher))
	    continue;
	  tlist[0] = FNV (teacher, VAL_TEACHER0);
	  tlist[1] = FNV (teacher, VAL_TEACHER1);
	  tlist[2] = FNV (teacher, VAL_TEACHER2);
	  if (tlist[0] || tlist[1] || tlist[2])
	    {
	      is_teacher = TRUE;
	      break;
	    }
	}
      
      /* Check for "books" */
      
      if (!teacher || !is_teacher)
	{
	  for (teacher = th->cont; teacher; teacher = teacher->next_cont)
	    {
	      tlist[0] = FNV (teacher, VAL_TEACHER0);
	      tlist[1] = FNV (teacher, VAL_TEACHER1);
	      tlist[2] = FNV (teacher, VAL_TEACHER2);
	      if (tlist[0] || tlist[1] || tlist[2])
		{
		  is_teacher = TRUE;
		  break;
		}
	    } 
	}
      
      /* Check if the room teaches? */
      
      if (!teacher || !is_teacher)
	{
	  tlist[0] = FNV (th->in, VAL_TEACHER0);
	  tlist[1] = FNV (th->in, VAL_TEACHER1);
	  tlist[2] = FNV (th->in, VAL_TEACHER2);
	  if (tlist[0] || tlist[1] || tlist[2])
	    {
	      is_teacher = TRUE;
	      teacher = th->in;
	    }
	} 
    }
  if (arg[0] == '\0' && teacher && is_teacher)
    {
      sprintf (buf, "%s teaches:\n\n\r--------------------------------------------------------------------\n\n\r", name_seen_by (th, teacher));
      stt (buf, th);
      for (j = 0; j < 3; j++)
	{
	  if (!tlist[j])
	    continue;
	  for (i = 0; i < NUM_VALS; i++)
	    {
	      if ((spell = find_spell (NULL, tlist[j]->val[i])) != NULL &&
		  spell->level <= LEVEL (th))
		{
		  found = TRUE;
		  sprintf (buf, "%-30s %s\n\r", spell->name, prac_pct (th, spell));
		  stt (buf, th);
		}
	    }
	}
      if (!found)
	stt ("Nothing.\n\r", th);
      return;
    }
  else if (!str_cmp (arg, "all") || type != SPELLT_MAX || !*arg ||
	   !str_cmp (arg, "nolearn"))
    {
      bigbuf[0] = '\0';
      count = 0;
      if (!str_cmp (arg, "nolearn"))
	nolearn = TRUE;
      
      if (type >= 0 && type < SPELLT_MAX)
	sprintf (buf, "You know the following %s%s:\n\n\r",
		 (type == SPELLT_PROF ? "proficiencies" : spell_types[type]),
		 (type == SPELLT_PROF ? "" : "s"));
      else
	sprintf (buf, "This is a list of all the spells you know:\n\n\r--------------------------------\n\r");
      add_to_bigbuf (buf);
      
      for (i = 0; i < MAX_SPELL; i++)
	{
	  if ((spell = find_spell (NULL, i)) != NULL &&
	      (type == SPELLT_MAX || spell->spell_type == type))
	    {
	      if (th->pc->prac[i] > 0 && !nolearn) 
		{
		  sprintf (buf, "%-25s %s      ", spell->name, prac_pct (th, spell)); 
		  add_to_bigbuf (buf);
		  if (++count % 2 == 0)
		    add_to_bigbuf ("\n\r");
		}
	      else if (th->pc->nolearn[i])
		{
		  sprintf (buf, "%-25s \x1b[1;31m(Nolearn)  \x1b[0;37m",
			   spell->name);
		  add_to_bigbuf (buf);
		  if (++count % 2 == 0)
		    add_to_bigbuf ("\n\r");
		}
	    }
	  
	}
      add_to_bigbuf ("\n\r");
      send_bigbuf (bigbuf, th);
      return;
    }
  
  if (!teacher || !is_teacher)
    {
      stt ("There is no teacher here.\n\r", th);
      return;
    }
  
  if ((spell = find_spell (arg, 0)) == NULL || LEVEL (th) < spell->level)
    {
      stt ("That spell doesn't exist!\n\r", th);
      return;
    }
  
  found = FALSE;
  for (j = 0; j < 3; j++)
    {
      if (!tlist[j])
	continue;
      for (i = 0; i < NUM_VALS; i++)
        {
          if (tlist[j]->val[i] == spell->vnum)
	    {
	      found = TRUE;
	      break;
   	    }
        }
    }

  if (!found)
    {
      stt ("You can't practice that here!\n\r", th);
      return;
    }
  
  /* Check for practices */

  if (th->pc->practices < 1)
    {
      stt ("You don't have any practices left!\n\r", th);
      return;
    }
  
  /* Validate the spell type */
  
  if (spell->spell_type < 0 || spell->spell_type >= SPELLT_MAX)
    {
      stt ("That spell is of an unknown type!\n\r", th);
      return;
    }

  
  
  /* Atm books let anyone learn anything..but this may
     change. */
  
  if (teacher-> in != th)
    {
      /* See if it can be learned. NOTE: This used to require "learns", but
	 this will get around that in essentially the same way. */
      
      if (th->pc->prac[spell->vnum] == 0 &&
	  total_spells (th, spell->spell_type) >= 
	  allowed_spells (th, spell->spell_type))
	{
	  sprintf (buf, "You don't have any %s slots left!\n\r",
		   spell_types[spell->spell_type]);
	  stt (buf, th);
	  return;
	}
      
      /* Now check for guilds... */
      
      for (i = 0; i < GUILD_MAX; i++)
	{
	  if (spell->guild[i] > th->pc->guild[i])
	    {
	      stt ("You don't have the proper guilds for this!\n\r", th);
	      return;
	    }
	}
      
      /* Now check for sufficient stats. For each stat point a person is
	 below those required for practicing, the "max_prac" percentage is
	 reduced by 5. The lowest it can go is MIN_PRAC so you
	 can ALWAYS improve..feel free to lower this if you're 
	 feeling sadistic. */
      
      for (i = 0; i < STAT_MAX; i++)
	{
	  max_prac -= 5 * MAX (0, spell->min_stat[i] - get_stat (th, i));
	}
      
      if (max_prac < MIN_PRAC)
	max_prac = MIN_PRAC;
  
      if (th->pc->prac[spell->vnum] >= max_prac)
	{
	  sprintf (buf, "You have learned this %s as much as you can!\n\r", 
		   spell_types[spell->spell_type]);
	  stt (buf, th);
	  return;
	}
      
      if (th->pc->practices < 1)
	{
	  stt ("You are out of practices!\n\r", th);
	  return;
	}

      for (i = 0; i < NUM_PRE; i++)
	{
	  if (spell->prereq[i] && spell->prereq[i]->vnum >= 0 &&
	      spell->prereq[i]->vnum < MAX_SPELL &&
	      th->pc->prac[spell->prereq[i]->vnum] < PREREQ_PCT)
	    {
	      sprintf (buf, "You must practice %s more before you learn %s.\n\r", spell->prereq[i]->name, spell->name);
	      stt (buf, th);
	      missing_prereq = TRUE;
	    }
	}
      if (missing_prereq)
	return;
      
    }
      
      /* Now, at this point we have the following. A person goes to a room
	 and types prac <something>. The <something> refers to a spell that
	 exists and there is a teacher there who can teach that spell. The
	 person can learn the spell if needed, and they have a practice. 
	 They also have the proper levels, guilds and (hopefully) the stats.
     So, now we know they can prac higher. So, we let them do it. */ 
  act ("@1n practice@s @t.", th, NULL, NULL, spell->name, TO_ALL); 
  if (teacher->in != th)
    {
      th->pc->prac[spell->vnum] += MID (4, 3 + get_stat (th, STAT_INT)*3/4, 25);
      th->pc->practices --;
    }
  else /* book */
    {
      th->pc->prac[spell->vnum] = MID (MIN_PRAC, th->pc->prac[spell->vnum] + 20, MAX_TRAIN_PCT);
      act ("@3n disappears in a puff of smoke.", th, NULL, teacher, NULL, TO_ALL);
      free_thing (teacher);
    }
  if (th->pc->prac[spell->vnum] >= max_prac)
    {
      sprintf (buf, "You have learned this %s as much as you can!\n\r", 
	       spell_types[spell->spell_type]);
      stt (buf, th);
      th->pc->prac[spell->vnum] = max_prac;  
    } 
  return;
}


void
do_unlearn (THING *th, char *arg)
{
  int i;
  SPELL *spl, *spl2;
  
  if (!IS_PC (th))
    return;


  if (arg[0] == '\0')
    {
      stt ("Unlearn <spellname> or 'all yes yes oh please really yes'.\n\r", th);
      return;
    }
  
  /* This wipes all spells and gives back practices. */
  
  if (!str_cmp (arg, "all yes yes oh please really yes"))
    {
      for (i = 0; i < MAX_SPELL; i++)
	{
	  th->pc->prac[i] = 0;
	}
      for (i = 0; i < NATSPELL_MAX; i++)
	{
	  th->pc->prac[FRACE(th)->nat_spell[i]] = MAX_TRAIN_PCT;
	  th->pc->prac[FALIGN(th)->nat_spell[i]] = MAX_TRAIN_PCT;
	}
      stt ("Ok, all spells wiped.\n\r", th);

      /* Give back pracs */

      th->pc->practices = (LEVEL (th) * (1 + get_stat (th, STAT_WIS)/3));
      return;
      
    }

  /* Does it exist */

  
  if ((spl = find_spell (arg, 0)) == NULL)
    {
      stt ("Unlearn what?\n\r", th);
      return;
    }

  /* Can't remove racial/alignment spells. */
  
  for (i = 0; i < NATSPELL_MAX; i++)
    {
      if (spl->vnum == FRACE(th)->nat_spell[i] ||
	  spl->vnum == FALIGN(th)->nat_spell[i])
	{
	  stt ("You cannot unlearn a natural racial or alignment spell!!\n\r", th);
	  return;
	}
    }
  
  /* Can't unlearn something if you know any spells that it's a 
     prereq of. */

  for (i = 0; i < MAX_PREREQ_OF; i++)
    {
      if ((spl2 = find_spell (NULL, spl->prereq_of[i])) != NULL &&
	  spl2->vnum >= 0 && spl2->vnum < MAX_SPELL &&
	  th->pc->prac[spl2->vnum] > 0)
	{
	  stt ("You can't unlearn a spell if you know a spell that comes after it.\n\r", th);
	  return;
	}
    }
	  

  /* Note no pracs given back for this, but it does open up a slot! */
  
  th->pc->prac[spl->vnum] = 0;
  stt ("Ok, spell unlearned.\n\r", th);
  return;
}
  
  

int
total_spells (THING *th, int type)
{
  int total = 0, i;
  SPELL *spl;

  if (!th || !IS_PC (th) || type < 0 || type >= SPELLT_MAX)
    return 0;
  
  /* Clear racial/align nat spells... */
  
  for (i = 0; i < NATSPELL_MAX; i++)
    {
      th->pc->prac[FRACE(th)->nat_spell[i]] = 0;
      th->pc->prac[FALIGN(th)->nat_spell[i]] = 0;
    }
  
  /* Add up remaining... */
  
  for (spl = spell_list; spl; spl = spl->next)
    {
      if (spl->spell_type == type && th->pc->prac[spl->vnum] > 0)
	total++;
    }
  
  for (i = 0; i < NATSPELL_MAX; i++)
    {
      th->pc->prac[FRACE(th)->nat_spell[i]] = MAX_TRAIN_PCT;
      th->pc->prac[FALIGN(th)->nat_spell[i]] = MAX_TRAIN_PCT;
    }
  
  return total;

}


int
allowed_spells (THING *th, int type)
{
  if (type < 0 || type >= SPELLT_MAX || !th || !IS_PC (th))
    return 0;
  
  return FRACE(th)->max_spell[type] +
    FALIGN(th)->max_spell[type] +
    ((th->pc->remorts + 1) * 
     (1 + (type == SPELLT_SPELL))/2);
}


/* This function sets up the trainers in the game. The way it works
   is that all spells of level >= 20 are randomly placed onto
   mobs that are generated and placed into random locations each
   reboot. The mobs aren't able to fight and they don't save over
   reboots so you never know where they will end up. */

void
set_up_teachers (void)
{
  THING *room, *area;
  THING *mob;
  int num_spells_left = 0, spells_this_time, choice, i, spell_chose;
  SPELL *spl;
  VALUE *teach;
  char name[STD_LEN];
  char buf[STD_LEN];
  char title[STD_LEN];
  int bad_rooms = 0;
  int area_count = 0;
  bool has_a_prereq;

  /* First if we don't have at least 15 areas, this won't automatically
     set up teachers. */

  for (area = the_world->cont; area; area = area->next_cont)
    {
      if (IS_AREA(area))
	area_count++;
    }
  
  if (area_count < 20)
    return;
  
  find_teacher_locations();
  for (spl = spell_list; spl; spl = spl->next)
    {
      if (spl->teacher[0] != 0  || spl->level < 20)
	continue;
      
      has_a_prereq = FALSE;
      
      for (i = 0; i < NUM_PRE; i++)
	{
	  if (spl->prereq[i])
	    has_a_prereq = TRUE;
	}
      if (!has_a_prereq)
	num_spells_left++;
    }
  
  /* Loop through all spells...*/
  while (num_spells_left > 0)
    {
      if (bad_rooms > 5000)
	return;
      /* Make the trainer mob...*/
      
      if ((room = find_random_room (NULL, FALSE, 0, BADROOM_BITS)) == NULL ||
	  !room->in || IS_AREA_SET (room->in, AREA_NOREPOP) ||
	  LEVEL(room->in) > 60)
	{
	  bad_rooms++;
	  continue;
	}
      
      if ((mob = create_thing (TEACHER_VNUM)) == NULL)
	return;
      
      
      /* Add the teaching value if needed */
      if ((teach = FNV (mob, VAL_TEACHER0)) == NULL)
	{
	  teach = new_value();
	  teach->type = VAL_TEACHER0;
	  add_value (mob, teach);
	}
      
      switch (nr(1,5))
	{
	  case 1: 
	    strcpy (title, "Master");
	    break;
	  case 2: 
	    strcpy (title, "Wise");
	    break;
	  case 3:
	    strcpy (title, "Enlightened");
	    break;
	  case 4: 
	    strcpy (title, "Ancient");
	    break;
	  case 5:
	  default:
	    strcpy (title, "Wizened");
	    break;
	}
      
      /* Set up the name and descs for this thing. */
      
      strcpy (name, create_society_name (NULL));
      
      free_str(mob->name);
      free_str(mob->short_desc);
      free_str(mob->long_desc);
      
      sprintf (buf, "%s trainer teacher %s", name, title);
      mob->name = new_str (buf);
      sprintf (buf, "%s the %s", name, title);
      mob->short_desc = new_str (buf);
      sprintf (buf, "%s the %s is here.", name, title);
      mob->long_desc = new_str (buf);
      
      mob->thing_flags |= TH_NO_FIGHT;
      
      for (i = 0; i < NUM_VALS; i++)
	teach->val[i] = 0;
      
      /* Get the number of spells to add onto this mob. */
      /* I put it at one for now but if you don't use that "auto practice"
	 code that lets you learn skills automatically, you should use
	 more spells per teacher. */
      spells_this_time = 1 /*nr (NUM_VALS/2, NUM_VALS) */;
      if (spells_this_time > num_spells_left)
	spells_this_time = num_spells_left;
      
      for (i = 0; i < spells_this_time; i++)
	{
	  int j = 0;
	  spell_chose = nr (1, num_spells_left);
	  choice = 0;
	  for (spl = spell_list; spl; spl = spl->next)
	    {
	      if (spl->teacher[0] != 0 || spl->level < 20)
		continue;

	      has_a_prereq = FALSE;

	      for (j = 0; j < NUM_PRE; j++)
		{
		  if (spl->prereq[j])
		    has_a_prereq = TRUE;
		}
	      
	      if (has_a_prereq)
		continue;
	      
	      if (++choice == spell_chose)
		break;
	    }
	  if (spl)
	    {
	      spl->teacher[0] = TEACHER_VNUM;
	      teach->val[i] = spl->vnum;
	    }
	  num_spells_left--;
	}
      
      thing_to (mob, room);
    }
  return;
}

      
void
do_nolearn (THING *th, char *arg)
{
  SPELL *spl;
  char buf[STD_LEN];
  
  if (!th || !IS_PC (th) || !arg)
    return;
  
  if ((spl = find_spell (arg, 0)) == NULL)
    {
      stt ("Nolearn <spellname\n\r", th);
      return;
    }
  
  if (spl->vnum < 0 || spl->vnum >= MAX_SPELL)
    return;
  
  th->pc->nolearn[spl->vnum] = (th->pc->nolearn[spl->vnum] ? 0 : 1);
  sprintf (buf, "You will%s learn %s.\n\r", 
	   (th->pc->nolearn[spl->vnum] ? "" : " not"),
	   spl->name);
  stt (buf, th);
  return;
}
