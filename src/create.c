#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "event.h"


/* These commands gather raw materials. */

void
do_mine (THING *th, char *arg)
{
  gather_raw (th, "mine");
  return;
}

void
do_chop (THING *th, char *arg)
{
  gather_raw (th, "chop");
  return;
}

void 
do_find (THING *th, char *arg)
{
  gather_raw (th, "find");
  return;
}

void
do_collect (THING *th, char *arg)
{
  gather_raw (th, "collect");
  return;
}

void
do_harvest (THING *th, char *arg)
{
  gather_raw (th, "harvest");
  return;
}

void 
do_hew (THING *th, char *arg)
{
  gather_raw (th, "hew");
  return;
}

void 
do_extrude (THING *th, char *arg)
{
  gather_raw (th, "extrude");
  return;
}

/* This function encompasses all the various different kinds
   of "gathering" activities you might need. I put it together
   so you could have all of the ugly requirements for each type...
   from needing eq to do it, or using up eq while doing it,
   or needing something to put the end result in. The function
   checks if you have all of the needed eq then lets you wait
   a bit before carrying out the gathering process. NOTE THAT NON
   PCS IGNORE ALL OF THE EQUIPMENT REQUIREMENTS!!!! */

void
gather_raw (THING *th, char *comm_name)
{
  THING *equip, *use_up[GATHER_EQUIP], *create = NULL, *put_into = NULL, *obj = NULL, *need_equip = NULL;
  int i, j, gtype, valnum, ocount;
  bool has_equip, need_eqval, need_eqname, repeated;
  char buf[STD_LEN], *nm;
  int vnum, rank;
  VALUE *eqval, *flap;
  GATHER *gt;
  SPELL *spl;
  FLAG *flg;
  
  
  if (!th || !th->in)
    return;
  
  /* Get max number of "gathering" commands. */

  for (i = 0; i < GATHER_EQUIP; i++)
    use_up[i] = NULL;
  
  
  
  if ((gtype = find_gather_type (comm_name)) == -1)
    {
      stt ("You cannot gather raw materials in this way.\n\r", th);
      return;
    }

  if (IS_PC (th) && th->pc->gathered[gtype] >= GATHER_RAW_AMT)
    {
      sprintf(buf, "You cannot %s right now. Wait %d hour%s and try again.\n\r",
            comm_name, th->pc->gathered[gtype] - GATHER_RAW_AMT,
             (th->pc->gathered[gtype] - GATHER_RAW_AMT == 1 ? "" : "s"));
      stt (buf, th);
         
      if (th->pc->gathered[gtype] < GATHER_RAW_AMT * 3)
        th->pc->gathered[gtype]++;
      else
        {
           stt ("Your hours will not go up anymore and \"loop\" over. Please stop checking this so frequently.\n\r", th);      
           th->pc->wait += 60;
        }
      return;
    }
  
  if (!IS_ROOM (th->in))
    {
      stt ("You aren't in a proper place in the world to do this.\n\r", th);
      return;
    }
  if (th->position != POSITION_STANDING && th->position != POSITION_GATHERING)
    {
      sprintf (buf, "You cannot %s anything unless you are standing.\n\r", comm_name);
      stt (buf, th);
      return;
    }
  
  gt = (GATHER *) &gather_data[gtype];
  
  /* NOTE THAT MOBS DON'T NEED ALL THIS EQUIPMENT TO GATHER SOMETHING! */

  if (IS_PC (th))
    {
      
      if (gt->need_skill)
	{      
	  if ((spl = find_spell (comm_name, 0)) == NULL)
	    {
	      sprintf (buf, "%s is supposed to have a spell with it, but the spell is missing. Sorry.\n\r", comm_name);
	      buf[0] = UC (buf[0]);
	      stt (buf, th);
	      return;
	    }
	  if (th->pc->prac[spl->vnum] < 20)
	    {
	      sprintf (buf, "You really don't know how to %s. Try practicing it more.\n\r", comm_name);
	      stt (buf, th);
	      return;
	    }
	}
      
      /* Check if we have the proper eq to do this. It either requires a
	 specific item number, or it can also be a named item or an item
	 with a special value. */
      
      for (i = 0; i < GATHER_EQUIP; i++)
	{
	  has_equip = TRUE;
	  if (gt->equip[i] > 0 &&
	      (need_equip = find_thing_num (gt->equip[i])) != NULL &&
	      !IS_SET (need_equip->thing_flags, TH_IS_AREA | TH_IS_ROOM) &&
	      !CAN_FIGHT (need_equip) && !CAN_MOVE (need_equip))
	    {
	      has_equip = FALSE;
	      for (equip = th->cont; equip && !has_equip; equip = equip->next_cont)
		{
		  if (!IS_WORN(equip))
		    break;
		  if (equip->vnum == need_equip->vnum)
		    has_equip = TRUE;	      
		}
	    }
	  if (!has_equip)
	    {
	      act ("You need @2n to be able to @t.", th, need_equip, NULL, comm_name, TO_CHAR);
	      return;
	    }
	}
      
      for (i = 0; i < GATHER_EQUIP; i++)
	{
	  need_eqname = FALSE;
	  need_eqval = FALSE;
	  has_equip = TRUE;
	  
	  if ((nm = gt->equip_name[i]) != NULL && *nm)
	    need_eqname = TRUE;
	  if ((valnum = gt->equip_value[i]) > 0 &&
	      valnum < VAL_MAX)
	    need_eqval = TRUE;
	  
	  if (need_eqname || need_eqval)
	    {
	      has_equip = FALSE;
	      for (equip = th->cont; equip && !has_equip; equip = equip->next_cont)
		{
		  if (!IS_WORN (equip))
		    break;
		  
		  /* Now, if the thing needs a name and we find the name,
		     and if it needs a val, and we find the val, we can
		     continue. */
		  
		  if ((!need_eqname || is_named (equip, nm)) &&
		      (!need_eqval || (eqval = FNV (equip, valnum)) != NULL))
		    has_equip = TRUE;
		}
	      if (!has_equip)
		{
		  buf[0] = '\0';
		  if (need_eqname && !need_eqval)
		    sprintf (buf, "You need %s %s to %s.\n\r", a_an (nm), nm, comm_name);
		  if (!need_eqname && need_eqval)
		    sprintf (buf, "You need something that is %s %s to %s.\n\r", a_an (value_list[valnum]), value_list[valnum], comm_name);
		  if (need_eqname && need_eqval)
		    sprintf (buf, "You need %s named %s which is also %s %s to %s.\n\r",  a_an (nm), nm,  a_an (value_list[valnum]), value_list[valnum], comm_name);
		  stt (buf, th);
		  return;
		}
	    }
	}
      
      /* We will need to use this item up, so we will need to make sure
	 we have it to start with. We need to get the item to gather 
	 from it. */
      
      for (i = 0; i < GATHER_EQUIP; i++)
	{      
	  has_equip = TRUE;
	  if (gt->use_up[i] > 0 &&
	      (need_equip = find_thing_num (gt->use_up[i])) != NULL &&
	      !IS_SET (need_equip->thing_flags, TH_IS_AREA | TH_IS_ROOM) &&
	      !CAN_FIGHT (need_equip) && !CAN_MOVE (need_equip))
	    {
	      has_equip = FALSE;
	      for (use_up[i] = th->cont; use_up[i]; use_up[i] = use_up[i]->next_cont)
		{
		  if (use_up[i]->vnum == need_equip->vnum)
		    {
		      repeated = FALSE;
		      for (j = 0; j < i; j++)
			{
			  if (use_up[j] == use_up[i])
			    {
			      repeated = TRUE;	      
			      break;
			    }
			}
		      if (repeated)
			continue;
		      has_equip = TRUE;
		      break;
		    }
		}
	      if (!has_equip)
		{
		  sprintf (buf, "You need to find %s before you %s.\n\r", 
			   NAME (need_equip), comm_name);
		  stt (buf, th);
		  return;
		}
	    }
	}
      
      /* If we need some special container to put the stuff in, this is
	 where we check for it. The container must be open. */
      
      if (gt->put_into > 0 &&
	  (need_equip = find_thing_num (gt->put_into)) != NULL &&
	  !IS_SET (need_equip->thing_flags, TH_IS_AREA | TH_IS_ROOM) &&
	  !CAN_FIGHT (need_equip) && !CAN_MOVE (need_equip))
	{
	  has_equip = FALSE;
	  for (put_into = th->cont; put_into; put_into = put_into->next_cont)
	    {
	      if (put_into->vnum == need_equip->vnum)
		{
		  if (IS_SET (put_into->thing_flags, TH_NO_CONTAIN))
		    act ("@2n cannot contain anything.", th, put_into, NULL, NULL, TO_CHAR);
		  else if ((flap = FNV (put_into, VAL_EXIT_O)) != NULL &&
			   (IS_SET (flap->val[1], EX_WALL) ||
			    (IS_SET (flap->val[1], EX_DOOR) && 
			 IS_SET (flap->val[1], EX_CLOSED))))
		    act ("@2n is closed.", th, put_into, NULL, NULL, TO_CHAR);
		  else  
		    {
		      ocount = 0;
		      for (obj = put_into->cont; obj; obj = obj->next_cont)
			ocount++;
		      if (ocount > 9)
			stt ("Your container can only hold 10 raw materials.\n\r", th);
		      else
			{
			  has_equip = TRUE;	
			  break;
			}
		    }
		}      
	    }      
	  
	  if (!has_equip)
	    {
	      sprintf (buf, "You need %s to put the stuff in that you %s.\n\r", 
		       NAME (need_equip), comm_name);
	      stt (buf, th);
	      return;
	    }
	}
    }
  /* Can only do certain kinds of gathering in certain kinds of rooms. */
	
  if (gt->room_flags > 0 &&
      !IS_ROOM_SET (th->in, gt->room_flags))
    {
      sprintf (buf, "You cannot %s on this kind of terrain.\n\r", comm_name);
      stt (buf, th);
      return;
    }
  if (IS_PC (th))
    {
      
      if (LEVEL (th) < gt->level)
	{
	  sprintf (buf, "You are not powerful enough to %s yet.\n\r", comm_name);
	  stt (buf, th);
	  return;
	}
      
      /* Delayed actions.... */
      
      if (gt->time > 0 && !DOING_NOW && IS_PC (th))
	{
	  add_command_event (th, comm_name, MIN (10*UPD_PER_SECOND, gt->time));
	  th->position = POSITION_GATHERING;
	  act ("@1n begin@s to @t.", th, NULL, NULL, comm_name, TO_ALL);
	  return;
	}
      
      /* Now check for skill and check for failure. */
      
      if ((gt->need_skill && !check_spell (th, comm_name, 0)) ||
	  (gt->fail[0] > 0 && gt->fail[1] > 0 && nr (1, gt->fail[0]) > gt->fail[1]))
	{
	  sprintf (buf, "You fail to %s successfully!\n\r", comm_name);
	  stt (buf, th);
	  for (i = 0; i < GATHER_EQUIP; i++)
	    {	  
	      if (use_up[i])
		{
		  act ("@1n ruin@s @2n!", th, use_up[i], NULL, NULL, TO_ALL);
		  free_thing (use_up[i]);
		}
	    }
	  return;
	}
      
      /* Possibly remove minerals like for mining etc.... */
      
      if (gt->need_minerals)
	{
	  if (!IS_ROOM (th->in) || 
	      (!IS_ROOM_SET (th->in, ROOM_MINERALS) &&
	       !IS_PC1_SET (th, PC_HOLYWALK)) ||
	      !th->in->in || !IS_AREA (th->in->in) ||
	      th->in->in->align > 0)
	    {
	      sprintf (buf, "You fail to %s successfully!\n\r", comm_name);
	      stt (buf, th);
	      for (i = 0; i < GATHER_EQUIP; i++)
		{	  
		  if (use_up[i])
		    {
		      act ("@1n ruin@s @2n!", th, use_up[i], NULL, NULL, TO_ALL);
		      free_thing (use_up[i]);
		    }
		}
	      return;
	    }
	  if (nr (1,10) == 3)
	    {
	      for (flg = th->in->flags; flg != NULL; flg = flg->next)
		if (flg->type == FLAG_ROOM1)
		  RBIT (flg->val, ROOM_MINERALS);
	    }
	}
      
      /* Now find what number item we get. If we have several ranks, we 
	 must set the "stop_here_chance" stuff so that we know what
	 pct chance there is to stop. For example, if shc[0] = 4, & shc[1] =1,
	 then if (nr (1, 4) > 1) we stop..i.e. 3/4 of the time. */
    }
  
  rank = 0;
  while (rank < gt->gather_ranks)
    {
      if (gt->stop_here_chance[0] > 0 && gt->stop_here_chance[1] > 0 && 
	  nr (1, gt->stop_here_chance[0]) > gt->stop_here_chance[1])
	break;
      rank++;
    }
  if (rank >= gt->gather_ranks)
    rank = 0;
  
  /* Find the vnum. start + (ranks*items_per_rank)*/

  vnum = gt->min_gather + (rank*gt->items_per_rank)+ nr (0, gt->items_per_rank-1);

  /* Make the raw material */
  
  if ((create = create_thing (vnum)) == NULL)
    {
      stt ("There isn't anything here...odd. Tell an admin.\n\r", th);
      return;
    }
  /* Put it someplace */
  
  if (!put_into)
    {
      act ("@1n find@s @t!", th, create, NULL, NAME (create), TO_ALL);
      thing_to (create, th);
    }
  else
    {
      act ("@1n find@s @2n and put@s it into @3n!", th, create, put_into, NULL, TO_ALL);
      thing_to (create, put_into);
    }
   if (IS_PC (th) && LEVEL (th) < MAX_LEVEL)
     th->pc->gathered[gtype]++;


  for (i = 0; i < GATHER_EQUIP; i++)
    {	  
      if (use_up[i])
	{
	  act ("@1n ruin@s @2n!", th, use_up[i], NULL, NULL, TO_ALL);
	  free_thing (use_up[i]);
	}
    }
  return;
}
  

  
/* This does nothing yet. It will eventually make scrolls and roughly
   follow the template for brewing. */

void
do_scribe (THING *th, char *arg)
{
  stt ("This does nothing. I haven't decided if I want scrolls in.\n\r", th);
  return;
}

/* The brew function works like this. You need to know the spells
   you will try to brew into a potion. You must be an alchemist
   and powerful enough to brew the spells. You need the empty
   potion bottle (number 706 atm). You need the herbs which are
   randomly generated for each potion based on the spell numbers
   and the random seed. If everything works, you get a potion
   created with a "random" name so you can differentiate them
   all, hopefully. */

void
do_brew (THING *th, char *arg)
{
  int herb_num[HERB_MAX_NEEDED][3]; /* What herbs we need */
  bool have_herb[HERB_MAX_NEEDED][3]; /* Do we have the herbs? */
  int i, j, total_levels = 0, num_herbs_needed,
    potion_number = 0, old_rand;
  
  THING *bottle = NULL; /* The potion bottle */
  SPELL *spell[3]; /* What spells we actually end up using */
  THING *herb, *herbn; /* What herbs we have and need */
  bool failed = FALSE; /* Did we fail to brew this? */
  bool found = FALSE; /* Used in loops to avoid gotos */
  int num_spells = 0;
  char arg1[STD_LEN];
  char buf[STD_LEN];
  VALUE *drink;
  FLAG *flg;  
  if (!IS_PC (th))
    return;
  
  if (guild_rank (th, GUILD_ALCHEMIST) < 2)
    {
      stt ("You aren't good enough at alchemy to try this!\n\r", th);
      return;
    }
  

  /* Set up variables */

  for (i = 0; i < 3; i++)
    {
      spell[i] = NULL;
      for (j = 0; j < HERB_MAX_NEEDED; j++)
	{
	  herb_num[j][i] = 0;
	  have_herb[j][i] = FALSE;
	}
    }
  
  /* First get the spells we want to put in the potion */

  for (i = 0; i < 3; i++)
    {
      arg = f_word (arg, arg1);
      if ((spell[i] = find_spell(arg1, 0)) == NULL)
	continue;
      if (spell[i]->spell_type != SPELLT_SPELL &&
	  spell[i]->spell_type != SPELLT_POISON)
	{
	  stt ("You can't brew a potion of something that isn't a spell or a poison!\n\r", th);
	  return;
	}
      num_spells++;
    }
  
  if (num_spells == 0)
    {
      stt ("Brew <spellname> <spellname> <spellname>\n\r", th);
      return;
    }

  /* Find an unused potion bottle */
  
  for (bottle = th->cont; bottle; bottle = bottle->next_cont)
    {
      if (bottle->vnum == POTION_BOTTLE)
	{
	  found = TRUE;
	  if ((drink = FNV (bottle, VAL_DRINK)) != NULL)
	    {
	      for (i = 3; i < 5; i++)
		{
		  if (drink->val[i] != 0)
		    {
		      found = FALSE;
		      break;
		    }
		}
	    }
	  if (found)
	    break;
	}
    }
	      
		    
  if (!bottle)
    {
      stt ("You need a potion bottle to brew a potion!\n\r", th);
      return;
    }
  
  /* Now see if the bottle already has spells in it... */
  
	     
  /* Now check what herbs we need. */
  
  for (i = 0; i < 3; i++)
    {
      if (!spell[i])
	continue;
      
      num_herbs_needed = MID (1, spell[i]->level/15, HERB_MAX_NEEDED);
      
      old_rand = random ();
      my_srand(POTION_SEED + spell[i]->vnum);
      for (j = 0; j < num_herbs_needed; j++)
	{
	  herb_num[j][i] = HERB_START + nr(0, HERB_COUNT-1);
	}
      my_srand(old_rand); /* This increments the rng a little only...*/
    }
  
  
  /* Check for the herbs */
  
  for (herb = th->cont; herb; herb = herb->next_cont)
    {
      found = FALSE;
      for (i = 0; i < 3 && !found; i++)
	{
	  for (j = 0; j < HERB_MAX_NEEDED && !found; j++)
	    {
	      if (have_herb[j][i] == FALSE &&
		  herb->vnum == herb_num[j][i])
		{
		  have_herb[j][i] = TRUE;
		  found = TRUE;
		}
	    }
	}
    }
  
  /* Now see if we have them all or not. */
  
  for (i = 0; i < 3; i++)
    {
      if (spell[i] == NULL)
	continue;
      for (j = 0; j < HERB_MAX_NEEDED; j++)
	{
	  if (herb_num[j][i] != 0 &&
	      have_herb[j][i] == FALSE)
	    {
	      if ((herb = find_thing_num (herb_num[j][i])) != NULL)
		act ("You can't brew a potion of @t because you're missing @3n.", th, NULL, herb, spell[i]->name, TO_CHAR);
	      else
		stt ("You don't have the right herbs!\n\r", th);
	      return;
	    }
	}
    }
  
  /* We have all the herbs, so move on to getting the total levels. */
  
  for (i = 0; i < 3; i++)
    {
      if (spell[i])
	total_levels += spell[i]->level;      
    }
  
  /* We can brew more powerful potions in terms of our level as
     our alchemist guild skill goes up. */

  if (total_levels > LEVEL(th) * guild_rank (th, GUILD_ALCHEMIST)/2)
    {
      stt ("That potion is too powerful for you to brew!\n\r", th);
      return;
    }
  
  /* See if we brew the spell... If you fail, you lose 1/3 of the
     herbs...else you lose them all. */
  
  if (!check_spell (th, "brew", 0) || nr (1,20) == 13)
    {
      stt ("You failed to brew your potion, and your bottle is ruined!\n\r", th);
      failed = TRUE;
    }
  
  /* Remove the bottle if we fail */
  
  if (failed)
    free_thing (bottle);
  
  /* Now remove the herbs. If we succeeded, we lose all of them,
     else we lose 1/3 of them. */
  
  for (herb = th->cont; herb; herb = herbn)
    {
      herbn = herb->next_cont;
      found = FALSE;
      for (i = 0; i < 3 && !found; i++)
	{
	  for (j = 0; j < HERB_MAX_NEEDED && !found; j++)
	    {
	      if (have_herb[j][i] == TRUE &&
		  herb->vnum == herb_num[j][i])
		{
		  have_herb[j][i] = FALSE;
		  free_thing (herb);
		  found = TRUE;
		}
	    }
	}
    }
  if (failed)
    return;
  
  /* Now we got rid of the herbs, so we will setup the potion. */
  
  bottle->level = LEVEL(th) * guild_rank (th, GUILD_ALCHEMIST)/(2*num_spells);
  
  if ((drink = FNV (bottle, VAL_DRINK)) == NULL)
    {
      drink = new_value ();
      drink->type = VAL_DRINK;
      add_value (bottle, drink);
    }
  
  drink->val[0] = 1;
  drink->val[1] = 1;
  
  /* Put the spells in the potion and get the number */

  for (i = 0; i < 3; i++)
    {
      if (spell[i])
	{
	  drink->val[i + 3] = (spell[i] ? spell[i]->vnum : 0);
	  potion_number += spell[i]->vnum;
	}
    }
  sprintf (buf, "potion %s %s", potion_types[potion_number % POTION_TYPES],
	   potion_color_names[potion_number % POTION_COLORS]);
  free_str (bottle->name);
  bottle->name = new_str (buf);
  
  sprintf (buf, "a %s%s potion",
	   potion_types[potion_number % POTION_TYPES],
	   potion_colors[potion_number % POTION_COLORS]);
  free_str (bottle->short_desc);
  bottle->short_desc = new_str (buf);
  
  sprintf (buf, "A %s%s potion was dropped here.",
	   potion_types[potion_number % POTION_TYPES],
	   potion_colors[potion_number % POTION_COLORS]);
  free_str (bottle->long_desc);
  bottle->long_desc = new_str (buf);
  
  bottle->timer = MIN(20,bottle->level/2);
  flg = new_flag();
  flg->type = FLAG_OBJ1;
  flg->val = OBJ_NOSTORE;
  aff_to (flg, bottle);
  act ("@1n just created @3n!", th, NULL, bottle, NULL, TO_ALL);
  return;
}


/* Graft a weapon value onto a gem */

void
do_graft (THING *th, char *arg)
{
  graft_value (th, "graft", arg);
}

/* Graft a gem value onto a weapon */

void 
do_imbue (THING *th, char *arg)
{
  graft_value (th, "imbue", arg);
}

/* This lets you put a ranged value onto armor */

void
do_emplace (THING *th, char *arg)
{
  graft_value (th, "emplace", arg);
}

/* This lets you graft a value onto another thing assuming that the
   old thing has the proper value, and the new things being
   grafted are all of the same type. */

void
graft_value (THING *th, char *graft_typename, char *arg)
{
  THING *oldth, *newth, *obj, *objn; 
  char arg1[STD_LEN];
  char buf[STD_LEN];
  int type, vnum, count, i;
  VALUE *oldval, *newval, *add_val, *prev_val;
  SPELL *spl;
  GRAFT *gf;  /* Graft info */

  if (!IS_PC (th))
    return;

  if (th->position < POSITION_STANDING)
    {
      stt ("You do this while in this position.\n\r", th);
      return;
    }
  
  arg = f_word (arg, arg1);
  

  /* Find the type of grafting being done */

  for (type = 0; graft_data[type].command[0]; type++)
    {
      if (!str_cmp (graft_data[type].command, graft_typename))
	break;
    }
  
  if (graft_data[type].command[0] == '\0')
    {
      stt ("You can't graft anything onto an item in that way.\n\r", th);
      return;
    }
  
  gf = (GRAFT *) &graft_data[type];

  /* Make sure there's a proficiency with the same name */
  
  if ((spl = find_spell (graft_typename, 0)) == NULL ||
      spl->spell_type != SPELLT_PROF)
    {
      sprintf (buf, "The %s skill doesn't appear to exist.\n\r",
	       graft_typename);
      stt (buf, th);
      return;
    }
  
  /* Make sure the player can use it. */
  
  if (th->pc->prac[spl->vnum] < 40)
    {
       stt ("You don't know how to use this skill!\n\r", th);
       return;
    }
  
  /* Make sure the old and new values are ok */
  
  if (gf->oldval < 1 || gf->oldval > VAL_MAX || gf->newval < 1 ||
      gf->newval > VAL_MAX)
    {
      stt ("It isn't possible to graft any powers onto things in this way.\n\r", th);
      return;
    }

  /* Make sure the person has the items in question and that these
     items are of the proper type */
  
  if ((oldth = find_thing_me (th, arg1)) == NULL ||
      (oldval = FNV (oldth, gf->oldval)) == NULL ||
      (newth = find_thing_me (th, arg)) == NULL ||
      (newval = FNV (newth, gf->newval)) == NULL)
    {
      sprintf (buf, "%s <%s> <%s> (You need %d of the same kind of %s to %s).\n\r",
	       graft_typename, value_list[gf->oldval],
	       value_list[gf->newval], gf->num_needed,
	       value_list[gf->newval], graft_typename);
      stt (buf, th);
      return;
    }
  
  
   if (oldth->vnum == newth->vnum) 
     {
       sprintf (buf, "You cannot %s an item with itself.\n\r", graft_typename);
       stt (buf, th);
       return;
     }
   
   /* Get the number of the new thing we want to graft on in the inven. */
   
   vnum = newth->vnum;
   
   count = 0;
   for (obj = th->cont; obj != NULL; obj = obj->next_cont)
     {
       if (IS_WORN(obj))
         continue;
       if (obj->vnum == vnum)
         count++;
     }
   
   /* Check if we have enough of the item to be used up. */
   
   if (count < gf->num_needed)
     {
       sprintf (buf, "You need %d %ss of the same type to %s an item.\n\r", 
		gf->num_needed, value_list[gf->newval], graft_typename);
       stt (buf, th);
       return;
     }
   
   /* Check if the item being grafted onto already has a value of
      the type we are trying to add. */

   if ((prev_val = FNV (oldth, gf->newval)) != NULL)
     {
       sprintf (buf, "That %s already had a %s grafted onto it!\n\r",
		value_list[gf->oldval], value_list[gf->newval]);
       stt (buf, th);
       damage (oldth, th, 3000, "blast");
       free_thing (oldth);
       return;
     } 

   if (!DOING_NOW)
     {
       sprintf (buf, "%s %s %s", graft_typename, arg1, arg);
       add_command_event (th, buf, gf->time);
       th->position = POSITION_GRAFTING;
       act ("@1n begin@s to @t @1s @2n with @3n.", th, oldth, newth, graft_typename, TO_ALL);
       return;
     }


   /* Check if we succeed, and if not, bail out and possibly destroy
      some of the equipment. */
   
   if (!check_spell (th, graft_typename, 0) || nr (1,3) == 2)
     {
       act ("@1n fail@s to @t @2n using @3n!", th, oldth, newth, graft_typename, TO_ALL);
       if (nr(1,90) == 4)
	 {
	   act ("@2n gets destroyed!", th, oldth, NULL, NULL, TO_ALL);
	   free_thing (oldth);
	 }
       
       count = 0;
       for (obj = th->cont; obj != NULL; obj = objn)
	 {
	   objn = obj->next_cont;
	   if (IS_WORN(obj) ||
	       obj->vnum != vnum || ++count > gf->num_needed 
	       || nr (1,150) != 111)
	     continue;
	   act ("@2n gets destroyed!", th, obj, NULL, NULL, TO_ALL);
	   free_thing (obj); 
	 }
       return;
     }
   
   /* Now set up the new value. Note that if the new value has a
      number in value, and the add value would change that number to 0
      due to the percentage losses, then the add value gets a 1 or -1
      depending on what sign the original value had. */

   add_val = new_value();
   add_val->type = gf->newval;
   add_value (oldth, add_val);
   for (i = 0; i < NUM_VALS; i++)
     {
       add_val->val[i] = newval->val[i]*gf->copy_pct[i]/100;
       if (newval->val[i] != 0 && add_val->val[i] == 0)
	 add_val->val[i] = newval->val[i] > 0 ? 1 : -1;
     }
   if (newval->word && *newval->word)
     strcpy (add_val->word, newval->word);

   act ("@1n @t@s @2n with @3n!", th, oldth, newth, graft_typename, TO_ALL);
   /* Now get rid of all the components we needed to make this item. */

   count = 0;
   for (obj = th->cont; obj != NULL; obj = objn)
     { 
       objn = obj->next_cont;
       if (IS_WORN(obj) || obj->vnum != vnum)
         continue;
       free_thing (obj);
       if (++count == gf->num_needed)
         break;
     }
   
   return;
}   
  

/* This lets you fortify a piece of armor...v5 keeps track of how many
   times it's been done so far. */
  
  
void
do_fortify (THING *th, char *arg)
{
  THING *item;
  VALUE *armor;
  int cost = 0, points = 0, i;
  char buf[STD_LEN];

  if (!IS_PC (th))
    return;
  
  if ((item = find_thing_here (th, arg, TRUE)) == NULL)
    {
      stt ("Fortify <armor>\n\r", th);
      return;
    }
  
  if ((armor = FNV (item, VAL_ARMOR)) == NULL)
    {
      stt ("That isn't armor!\n\r", th);
      return;
    }
  
  for (i = 0; i < MIN (NUM_VALS - 1, PARTS_MAX); i++)
    points += armor->val[i];
  

  /* Check for money and get rid of it if you have it. */

  cost = item->cost * points/(1 + guild_rank (th, GUILD_TINKER));
  
  if (total_money (th) < cost)
    {
      sprintf (buf, "You need %d coins to fortify this item, and you only have %d on you.\n\r", cost, total_money (th));
      stt (buf, th);
      return;
    }
  
  sub_money (th, cost);
  
  /* Now check if the item is destroyed due to too many fortifications */
  
  if (!check_spell (th, "fortify", 0))
    {
      stt ("You fail to fortify this item!\n\r", th);
      return;
    }
  
  
  if (nr (2,5) < armor->val[5])
    {
      stt ("You tried to fortify this item one too many times, and it breaks!\n\r", th);
      free_thing (item);
      return;
    }
  
  for (i = 0; i < MIN (NUM_VALS - 1, PARTS_MAX); i++)
    armor->val[i] += nr (0,1);
  
  armor->val[5]++;
  
  act ("@1n fortify@s @1s @3n.", th, NULL, item, NULL, TO_ALL);
  
    
  return;
}


/* This command lets you forge items. */





/* This sets up the forged eq. It uses numbers 100000-101000. */

void
make_forged_eq (void)
{

  THING *ar, *thg;
  int min, type; /* Counters for mineral and eq type. */
  int i, flagval, wear_loc, val_num;
  char buf[STD_LEN];
  char *t;
  VALUE *val;
  FLAG *flg;
  
  /* Do not remake the eq if it already exists. */

  if ((ar = find_thing_num (FORGED_EQ_AREA_VNUM)) != NULL)
    return;

  /* Don't do this if the area is blocked! */

  for (i = FORGED_EQ_AREA_VNUM; i < FORGED_EQ_AREA_VNUM+1000; i++)
    {
      if (find_thing_num (i))
	{
	  log_it( "FORGED EQ AREA BLOCKED!!!\n");
	  return;
	}
    }
  
  ar = new_thing ();
  ar->name = new_str ("forgedeq.are");
  ar->short_desc = new_str ("The Forged Eq Area");
  ar->vnum = FORGED_EQ_AREA_VNUM;
  ar->mv = 5;
  ar->max_mv = 1000;
  ar->thing_flags = TH_IS_AREA | TH_CHANGED;
  thing_to (ar, the_world);
  add_thing_to_list (ar);
  
  /* Now set up the actual items. */

  for (type = 0; type < MAX_FORGE && forge_data[type].name[0]; type++)
    {
      for (min = 0; min < MAX_MINERAL; min++)
	{
	  thg = new_thing();

	  /* Vnum */
	  
	  thg->vnum = FORGED_EQ_AREA_VNUM+10 + MAX_FORGE*type + min;
	  top_vnum = MAX (thg->vnum, top_vnum);
	  /* Thing flags */
	  
	  thg->thing_flags = OBJ_SETUP_FLAGS;
	  
	  thing_to(thg, ar);
	  add_thing_to_list (thg);
	  /* Set up names and descriptions */

	  sprintf (buf, "%s %s", forge_data[type].name,
		   mineral_names[min]);
	  thg->name = new_str (buf);
	  for (t = (char *) mineral_names[min]; *t; t++);
	  t--;
	  if (*t == 's' && forge_data[type].val_type == VAL_ARMOR)
	    {
	      sprintf (buf, "some %s %s", mineral_colornames[min],
		       forge_data[type].name);
	      thg->short_desc = new_str (buf);
	      sprintf (buf, "Some %s %s are here.", mineral_colornames[min],
		       forge_data[type].name);
	      thg->long_desc = new_str (buf);
	    }
	  else
	    {
	      sprintf (buf, "%s %s %s", a_an(mineral_names[min]),
		       mineral_colornames[min], forge_data[type].name);
	      thg->short_desc = new_str (buf); 
	      sprintf (buf, "%s %s %s is here.", a_an(mineral_names[min]),
		       mineral_colornames[min], forge_data[type].name);
	      buf[0] = UC(buf[0]);
	      thg->long_desc = new_str (buf);
	    }
	  
	  /* Do we want a "real" desc here too?? maybe later. */
	  
	  /* Set up wear location */
	  
	  thg->wear_pos = forge_data[type].wear_loc;

	  /* Set up the hps mult (10 = normal) */
	  
	  thg->max_hp = 10;
	     
	  /* Set up level */

	  thg->level = 10 * (min + 1);
	  
	  /* Set up cost */
	  
	  thg->cost = forge_data[type].cost * (min + 1)*(min + 1)/7;
	  
	  /* Set up weight */
	  
	  thg->weight = forge_data[type].weight * (MIN (min + 2, 15 - min));
	  
	  /* Set up value. */

	  val = new_value();
	  add_value (thg, val);
	  val->type = forge_data[type].val_type;
	  val_num = forge_data[type].val_num;
	  /* Set up the special number if needed */
	  
	  if (val_num >= 0 && val_num < NUM_VALS)
	    val->val[val_num] = forge_data[type].val_set_to;
	  
	  /* Tools need a rank */
	  
	  if (val->type == VAL_TOOL)
	    val->val[1] = min;

	  /* Minerals also need a rank */
	  
	  if (val->type == VAL_RAW)
	    val->val[1] = min;
	  
	  /* Set up weapon damage */
	  
	  if (val->type == VAL_WEAPON)
	    {
	      val->val[0] = min + 2;
	      val->val[1] = min *5/4 + 6;
	      
	      if (val->val[2] == WPN_DAM_PIERCE)
		{
		  val->val[0]--;
		  val->val[1]--;
		  val->val[3] += min/9; /* Extra attack at tier 9 */
		}
	      else if (val->val[2] == WPN_DAM_CONCUSSION)
		{
		  val->val[0]++;
		  val->val[1]++;
		}
	    }
	  
	  /* Now, based on where it's worn, find the armor location. */
	  
	  if (val->type == VAL_ARMOR)
	    {
	      for (i = 0; i < ITEM_WEAR_MAX; i++)
		{
		  if (thg->wear_pos == wear_data[i].flagval)
		    {
		      val->val[wear_data[i].part_of_thing] = 2 + min*2/3;
		      break;
		    }
		}

	      /* Shields get TONS of armor. */
	      
	      if (thg->wear_pos == ITEM_WEAR_SHIELD)
		{
		  for (i = 0; i < PARTS_MAX; i++)
		    val->val[i] = 1 + min/2;
		}
	    }
	  
	  /* At this point, the thing has its level, wear loc, cost,
	     hp mult, name, short, long, weight, thing_flags and value. */
	  
	  /* Now, we have to do the flags. */
	     
	  
	  if (val->type == VAL_WEAPON)
	    {
	      /* Hitroll */
	      
	      if ((flagval = min/3) != 0)
		{
		  flg = new_flag();
		  flg->next = thg->flags;
		  thg->flags = flg;
		  flg->type = FLAG_AFF_HIT;
		  flg->val = flagval;
		}

	      /* Damroll */
	      
	      if ((flagval = min/5) != 0)
		{
		  flg = new_flag();
		  flg->next = thg->flags;
		  thg->flags = flg;
		  flg->type = FLAG_AFF_DAM;
		  flg->val = flagval;
		}
	      if (forge_data[type].stat_type >= 0 &&
		  forge_data[type].stat_type < STAT_MAX &&
		  (flagval = min/9) != 0)
		{
		  flg = new_flag();
		  flg->next = thg->flags;
		  thg->flags = flg;
		  flg->type = AFF_START + forge_data[type].stat_type;
		  flg->val = flagval;
		}
	    }
	  
	  if (val->type == VAL_ARMOR)
	    {
	      
	      /* Hps */
	      
	      if ((flagval = min) != 0)
		{
		  flg = new_flag();
		  flg->next = thg->flags;
		  thg->flags = flg;
		  flg->type = FLAG_AFF_HP;
		  flg->val = flagval;
		} 
	      
	      /* Moves */

	      if ((flagval = min*2/3) != 0)
		{
		  flg = new_flag();
		  flg->next = thg->flags;
		  thg->flags = flg;
		  flg->type = FLAG_AFF_MV;
		  flg->val = flagval;
		} 

	      /* Stat */
	      
	      if (forge_data[type].stat_type >= 0 &&
		  forge_data[type].stat_type < STAT_MAX &&
		  (flagval = min/9) != 0)
		{
		  flg = new_flag();
		  flg->next = thg->flags;
		  thg->flags = flg;
		  flg->type = AFF_START + forge_data[type].stat_type;
		  flg->val = flagval;
		}
	      
	      
	      /* Hitroll */
	      
	      if ((flagval = min/4) != 0)
		{
		  flg = new_flag();
		  flg->next = thg->flags;
		  thg->flags = flg;
		  flg->type = FLAG_AFF_HIT;
		  flg->val = flagval;
		}
	      
	      /* Now work with special locations and special affects. */
	      
	      wear_loc = forge_data[type].wear_loc;
	      
	      /* legs/feet = kickdam */

	      if (wear_loc == ITEM_WEAR_LEGS || wear_loc == ITEM_WEAR_FEET)
		{
		  if ((flagval = min/2) != 0)
		    {
		      flg = new_flag();
		      flg->next = thg->flags;
		      thg->flags = flg;
		      flg->type = FLAG_AFF_KICKDAM;
		      flg->val = flagval;
		    }
		}
	      
	      /* Head = spell heal */
	      if (wear_loc == ITEM_WEAR_HEAD)
		{
		  if ((flagval = min/3) != 0)
		    {
		      flg = new_flag();
		      flg->next = thg->flags;
		      thg->flags = flg;
		      flg->type = FLAG_AFF_SPELL_HEAL;
		      flg->val = flagval;
		    }
		}

	      /* Face = spell attack */
	      
	      if (wear_loc == ITEM_WEAR_FACE)
		{
		  if ((flagval = min/3) != 0)
		    {
		      flg = new_flag();
		      flg->next = thg->flags;
		      thg->flags = flg;
		      flg->type = FLAG_AFF_SPELL_ATTACK;
		      flg->val = flagval;		      
		    }
		}
	      
	      /* Body/waist = resist dam */
	      
	      if (wear_loc == ITEM_WEAR_BODY || wear_loc == ITEM_WEAR_WAIST)
		{
		  if ((flagval = min/3) != 0)
		    {
		      flg = new_flag();
		      flg->next = thg->flags;
		      thg->flags = flg;
		      flg->type = FLAG_AFF_DAM_RESIST;
		      flg->val = flagval;		      
		    }
		}
	      
	      /* Arms/shield = defense */

	      if (wear_loc == ITEM_WEAR_ARMS || wear_loc == ITEM_WEAR_SHIELD)
		{
		  if ((flagval = min/3) != 0)
		    {
		      flg = new_flag();
		      flg->next = thg->flags;
		      thg->flags = flg;
		      flg->type = FLAG_AFF_DEFENSE;
		      flg->val = flagval;		      
		    }
		}
	      
	      /* Hands = thief attack */

	      if (wear_loc == ITEM_WEAR_HANDS)
		{
		  if ((flagval = min/3) != 0)
		    {
		      flg = new_flag();
		      flg->next = thg->flags;
		      thg->flags = flg;
		      flg->type = FLAG_AFF_THIEF_ATTACK;
		      flg->val = flagval;		      
		    }
		}
	    }
	}
    }
  return;
}


/* This command lets you forge equipment. In order to forge, you must
   be in the room where a forge is. You must have a hammer and anvil
   of the proper type. and you must have the proper number of chunks of
   the proper mineral to do this. */

void
do_forge (THING *th, char *arg)
{
  THING *forge = NULL, *anvil = NULL, *hammer = NULL, *obj,
    *forgeditem, *objn;
  int hammer_rank = 0, anvil_rank = 0, 
    chunk_cost, chunk_count;
  int mintype, eqtype, itemnum;
  SPELL *spl;
  VALUE *tool, *raw;
  char *oldarg = arg;
  char arg1[STD_LEN];
  char buf[STD_LEN];

  /* Must be a PC */

  if (!IS_PC (th) || !th->in)
    return;
  
  if (th->position < POSITION_STANDING)
    {
      stt ("You cannot forge starting from this position!\n\r", th);
      return;
    }

  /* Check for correct arguments. */
  
  arg = f_word (arg, arg1);
  
  /* Check for the mineral type. */

  for (mintype = 0; mintype < MAX_MINERAL; mintype++)
    {
      if (!str_prefix (arg1, mineral_names[mintype]))
	break;
    }
  
  if (mintype == MAX_MINERAL)
    {
      stt ("Syntax: forge <mineral name> <item name>.\n\r", th);
      return;
    }
  
  /* Check if your tinker's guild rank is enough. */
  
  if (mintype > 2*guild_rank (th, GUILD_TINKER))
    {
      stt ("You aren't enough of a tinker to forge something this powerful!\n\r", th);
      return;
    }

  /* Find the item type being forged */
  
  for (eqtype = 0; forge_data[eqtype].name[0]; eqtype++)
    {
      if (!str_prefix (arg, forge_data[eqtype].name))
	break;
    }
  
  if (forge_data[eqtype].name[0] == '\0' || eqtype == 0)
    {
      stt ("Syntax: forge <mineral name> <item name>.\n\r", th);
      return;
    }
  
  
  /* Find the skill */
  
  if ((spl = find_spell ("forge", 0)) == NULL ||
      spl->vnum < 1 || spl->vnum >= MAX_SPELL ||
      spl->spell_type != SPELLT_PROF)
    {
      stt ("There's no forge skill, sorry.\n\r", th);
      return;
    }

  /* See if the player knows it */
  
  if (th->pc->prac[spl->vnum] < 50)
    {
      stt ("You don't know how to forge well enough!\n\r", th);
      return;
    }
  
  /* Find the forge */
  
  for (forge = th->in->cont; forge; forge = forge->next_cont)
    {
      if (forge->vnum == FORGE_VNUM)
	break;
    }

  if (!forge)
    {
      stt ("There's no forge here!\n\r", th);
      return;
    }
  

  /* Get the hammer and anvil */
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if ((tool = FNV(obj, VAL_TOOL)) != NULL)
	{
	  if (IS_WORN (obj) && tool->val[0] == TOOL_HAMMER &&
	      !hammer)
	    {
	      hammer = obj;
	      hammer_rank = tool->val[1];
	    }
	  else if (tool->val[0] ==  TOOL_ANVIL && !anvil)
	    {
	      anvil = obj;
	      anvil_rank = tool->val[1];
	    }
	  
	  if (hammer && anvil)
	    break;
	}
    }
     
  if (!hammer)
    {
      stt ("You need a forging hammer to forge equipment!\n\r", th);
      return;
    }
  
  /* Check for the proper hammer and anvil for the item being forged. */

  /* Make an anvil -- need hammer of same rank, anvil one rank lower. */


  if (eqtype == 1)
    {
      if (hammer_rank < mintype)
	{
	  stt ("Your hammer isn't powerful enough to forge this anvil!\n\r", th);
	  return;
	}
      if (anvil_rank < mintype - 1)
	{
	  stt ("Your anvil isn't powerful enough to forge this anvil!\n\r", th);
	  return;
	}
    }
  /* Make a hammer need hammer/anvil of one rank lower. */


  if (eqtype == 2)
    {
      if (hammer_rank < mintype - 1)
	{
	  stt ("Your hammer isn't powerful enough to forge this hammer!\n\r", th);
	  return;
	}
      if (anvil_rank < mintype - 1)
	{
	  stt ("Your anvil isn't powerful enough to forge this hammer!\n\r", th);
	  return;
	}
    }
  /* Make other items...same ranks required. */
  

  if (eqtype > 2)
    {
      if (hammer_rank < mintype)
	{
	  stt ("Your hammer isn't powerful enough to forge this item!\n\r", th);
	  return;
	}
      if (anvil_rank < mintype)
	{
	  stt ("Your anvil isn't powerful enough to forge this item!\n\r", th);
	  return;
	}
    }
  /* Now get the cost in chunks to forge these items. */
  
  if (eqtype < 3)
    chunk_cost = 20;
  else chunk_cost = 8;
  
  /* See if we have enough chunks of the proper type. */
  
  chunk_count = 0;
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if ((raw = FNV (obj, VAL_RAW)) != NULL &&
	  raw->val[0] == RAW_MINERAL &&
	  raw->val[1] == mintype)
	chunk_count++;
      
    }
  
  /* OOPs! not enough chunks. */
  
  if (chunk_count < chunk_cost)
    {
      sprintf (buf, "Forging %s %s %s costs %d chunks %s chunks, and you only have %d on you.\n\r",
	       a_an (mineral_names[mintype]), mineral_names[mintype],
	       forge_data[eqtype].name, chunk_cost, mineral_names[mintype],
	       chunk_count);
      stt (buf, th);
      return;
    }
  
  /* Begin to forge... */
  
  if (!DOING_NOW)
    {
      sprintf (buf, "%s %s", "forge", oldarg);
      add_command_event (th, buf, chunk_cost);
      act ("@1n begin@s to work @1s @t chunks into something...", th, NULL, NULL, (char *) mineral_names[mintype], TO_ALL);
      th->position = POSITION_FORGING;
      return;
    }
  
  chunk_count = 0;
  if (!check_spell (th, NULL, spl->vnum) || nr (1,5) == 3)
    {
      act ("@1n botch@s the job and the item is ruined!\n\r", th, NULL, NULL, NULL, TO_ALL);
      
      /* Destroy some of the chunks */
      
      for (obj = th->cont; obj && chunk_count < chunk_cost; obj = objn)
	{
	  objn = obj->next_cont;
	  if ((raw = FNV (obj, VAL_RAW)) != NULL &&
	      raw->val[0] == RAW_MINERAL &&
	      raw->val[1] == mintype)
	    {
	      chunk_count++;
	      if (nr (1,5) == 3)
		free_thing (obj);
	    }
	}
      return;
    }
  
  /* Now find the item number created */
  
  itemnum = 100010 + eqtype * MAX_FORGE + mintype;
  
  if ((forgeditem = create_thing (itemnum)) == NULL)
    {
      sprintf (buf, "There is an error in the forging. Tell an admin what item of what mineral type you tried to forge. (num %d)\n\r", itemnum);
      stt (buf, th);
      return;
    }
  
  /* Now the item, exists, so get rid of the old eq. */
  
  if (eqtype == 1) /* Anvil */
    free_thing (anvil);
  
  if (eqtype == 2) /* Hammer */
    free_thing (hammer);
  
  
  /* Remove the appropriate number of chunks */
  
  for (obj = th->cont; obj && chunk_count < chunk_cost; obj = objn)
    {
      objn = obj->next_cont;
      if ((raw = FNV (obj, VAL_RAW)) != NULL &&
	  raw->val[0] == RAW_MINERAL &&
	  raw->val[1] == mintype)
	{
	  chunk_count++;
	  free_thing (obj);
	}
    }
  thing_to (forgeditem, th);
  act ("@1n just forged @3n!", th, NULL, forgeditem, NULL, TO_ALL);
  return;
}
    



/* The enchant skill lets you put affects onto items. It
   has several restrictions. First, the item must not have a 
   magical flag on it, or it will fail. The item must not
   have an affect of the same type already, or it will fail.
   You must have the proper guilds to make it work, or
   it will fail. You must also give up warpoints to make
   it work. */


void
do_enchant (THING *th, char *arg)
{
  THING *obj;
  FLAG *flag;
  int wps_cost = 0, enchant_amount, enchant_type,
    guild_amount = 0, remort_amount = 0;
  char arg1[STD_LEN];
  char buf[STD_LEN];
  
  if (!IS_PC (th))
    return;
  
  if (!*arg)
    {
      stt ("Enchant <item> <type of enchantment>\n\r", th);
      return;
    }

  arg = f_word (arg, arg1);
  
  if ((obj = find_thing_thing (th, th, arg1, FALSE)) == NULL)
    {
      stt ("You don't have that item in your inventory.\n\r", th);
      return;
    }
  
  for (enchant_type = AFF_START; enchant_type < AFF_MAX; enchant_type++)
    {
      if (!str_cmp (arg, affectlist[enchant_type - AFF_START].name))
	break;
    }
  
  if (enchant_type == AFF_MAX)
    {
      stt ("You can't put that kind of an enchantment on this item.\n\r", th);
      return;
    }

  /* Check for an enchant flag. */
  

  if (IS_OBJ_SET (obj, OBJ_MAGICAL))
    {
      stt ("Ooops! That was already enchanted!\n\r", th);
      damage (obj, th, 3000, "blast");
      free_thing (obj);
      return;
    }
  
  /* Check if it has an affect of the same type already. */

  for (flag = obj->flags; flag; flag = flag->next)
    {
      if (flag->type == enchant_type && flag->timer == 0)
	{
	  stt ("That item already has an enchantment like that on it!\n\r", th);
	  damage (obj, th, 3000, "blast");
	  free_thing (obj);
	  return;
	}
    }
  
  /* Now figure out amounts and costs based on the type of enchant */

  /* These are the most powerful enchantments and are therefore
     the most expensive. */

  if ((enchant_type >= FLAG_AFF_STR && enchant_type < FLAG_AFF_DAM) ||
      enchant_type == FLAG_AFF_SPELL_HEAL ||
      enchant_type == FLAG_AFF_SPELL_ATTACK)
    {
      wps_cost = 30;
      remort_amount = 8;
      guild_amount = 10;
      enchant_amount = 1;
    } 
  else if (enchant_type == FLAG_AFF_HP ||
	   enchant_type == FLAG_AFF_MV ||
	   enchant_type == FLAG_AFF_ARMOR ||
	   enchant_type == FLAG_AFF_HIT)
    { /* Let anyone do hps/armor/hit/mv
	 but it gets better with remorts/guilds. */
      wps_cost = 5;
      remort_amount = 0;
      guild_amount = 0;
      enchant_amount = (guild_rank (th, GUILD_ALCHEMIST) +
			guild_rank (th, GUILD_WIZARD) +
			th->pc->remorts)*3/4;
      if (enchant_type == FLAG_AFF_MV)
	enchant_amount = enchant_amount*2/3;
      else if (enchant_type == FLAG_AFF_ARMOR)
	enchant_amount /=2;
      else if (enchant_type == FLAG_AFF_HIT)
	enchant_amount /= 3;
    }
  else /* The other stuff */
    {
      wps_cost = 15;
      remort_amount = 5;
      guild_amount = 8;
      enchant_amount = 1 +(guild_rank (th, GUILD_WIZARD) +
			   guild_rank (th, GUILD_ALCHEMIST) +
			   th->pc->remorts)/17;
    }
  
  /* Check if we have enough remorts */
  
  if (th->pc->remorts < remort_amount)
    {
      sprintf(buf, "You need %d remorts to enchant this item in this way.\n\r", remort_amount);
      stt (buf, th);
      return;
    }

  /* Check if we have enough guild tiers */
  
  if (guild_rank (th, GUILD_WIZARD) + guild_rank (th, GUILD_ALCHEMIST) < guild_amount)
    {
      sprintf (buf, "You need a total of %d tiers between your wizard and alchemist guild ranks to enchant in this way!\n\r", guild_amount);
      stt (buf, th);
      return;
    }
  
  /* Check if we have enough warpoints */

  if (th->pc->pk[PK_WPS] < wps_cost)
    {
      sprintf (buf, "You need %d warpoints to enchant this item in this way, and you only have %d right now.\n\r", wps_cost, th->pc->pk[PK_WPS]);
      stt (buf, th);
      return;
    }
  
  /* At this point, the item doesn't have the enchantment, it
     and you have the guilds, remorts and wps to enchant the
     item. */
  
  if (!check_spell (th, "enchant", 0) || nr (1,5) == 3)
    {
      stt ("You failed to enchant the item!\n\r", th);
      th->pc->pk[PK_WPS] -= wps_cost/3;
      if (nr (1,50) == 25)
	{
	  act ("@1p @2n just disappeared!", th, obj, NULL, NULL, TO_ALL);
	  free_thing (obj);
	}
      return;
    }
  
  
  /* Add the "enchanted" flag */

  for (flag = obj->flags; flag; flag = flag->next)
    {
      if (flag->type == FLAG_OBJ1)
	{
	  flag->val |= OBJ_MAGICAL;
	  break;
	}
    }
  
  if (!flag) /* Add the enchanted flag if not found */
    {
      flag = new_flag ();
      flag->type = FLAG_OBJ1;
      flag->from = 0;
      flag->timer = 0;
      flag->val = OBJ_MAGICAL;
      flag->next = obj->flags;
      obj->flags = flag;
    }
  
  
  /* Now add the new flag. */
  
  flag = new_flag ();
  flag->next = obj->flags;
  obj->flags = flag;
  flag->timer = 0;
  flag->from = 0;
  flag->type = enchant_type;
  flag->val = enchant_amount;
  
  
  act ("@1n enchant@s @1s @2n!", th, obj, NULL, NULL, TO_ALL);
  return;
}


/* This skill lets you increase the damage a weapon does. Be warned, it
   will only work a few times. It checks the strength of the item
vs the base item type. If the base item wasn't a weapon, then it won't work.*/




void
do_sharpen (THING *th, char *arg)
{
  THING *item, *baseitem;
  VALUE *weapon, *baseweapon;
  int cost = 0, points = 0, basepoints = 14;
  char buf[STD_LEN];

  if (!IS_PC (th))
    return;
  
  if ((item = find_thing_here (th, arg, TRUE)) == NULL)
    {
      stt ("Sharpen <weapon>\n\r", th);
      return;
    }
  
  if ((weapon = FNV (item, VAL_WEAPON)) == NULL)
    {
      stt ("That isn't a weapon!\n\r", th);
      return;
    }
  
  points = weapon->val[0] + weapon->val[1];
  
  /* Check for slashing/piercing or else it doesn't make sense. :P */

  if (weapon->val[2] != WPN_DAM_SLASH && weapon->val[2] != WPN_DAM_PIERCE)
    {
      stt ("You can only sharpen slashing or piercing weapons.\n\r", th);
      return;
    }
  
  /* Check for money and get rid of it if you have it. */
  
  cost = item->cost * points/(1 + guild_rank (th, GUILD_TINKER));
  
  if (total_money (th) < cost)
    {
      sprintf (buf, "You need %d coins to sharpen this item, and you only have %d on you.\n\r", cost, total_money (th));
      stt (buf, th);
      return;
    }
  
  sub_money (th, cost);
  
  /* Now check if the item is destroyed due to not knowing how to sharpen */
  
  if (!check_spell (th, "sharpen", 0))
    {
      stt ("You failed to sharpen this item!\n\r", th);
      return;
    }

  /* Check if we have too many sharpens already. */
  
  if ((baseitem = find_thing_num (item->vnum)) != NULL &&
      (baseweapon = FNV (baseitem, VAL_WEAPON)) != NULL)
    {
      basepoints = baseweapon->val[0] + baseweapon->val[1];
    }
  
  basepoints = MIN (basepoints, points);
  
  if (nr (2, 4 + guild_rank (th, GUILD_TINKER)) < points - basepoints)
    {
      stt ("You tried to sharpen this item one too many times, and it breaks!\n\r", th);
      free_thing (item);
      return;
    }
  

  weapon->val[1]++;
  weapon->val[0] += nr (0,1);
  
  
  act ("@1n sharpen@s @1s @3n.", th, NULL, item, NULL, TO_ALL);
  
    
  return;
}


/* This command lets you convert bad chunks into better chunks at the
   rate of 1/4 of the time. */

void
do_alchemy (THING *th, char *arg)
{
  int max_rank, curr_rank, i;
  THING *obj, *objn, *newchunk;
  VALUE *raw;
  char buf[STD_LEN];
  bool failed = FALSE;
  
  if (!IS_PC (th))
    return;

  if (guild_rank (th, GUILD_ALCHEMIST) < 2)
    {
      stt ("You aren't enough of an alchemist to do this!\n\r", th);
      return;
    }
  /* Get the max rank we go up to, if not all the way to adam. */
  
  for (max_rank = 0; max_rank < MAX_MINERAL; max_rank++)
    {
      if (!str_cmp (mineral_names[max_rank], arg))
	break;
    }
  
  if (max_rank == MAX_MINERAL)
    max_rank = MAX_MINERAL - 1;

  
  if (max_rank >= 2*guild_rank (th, GUILD_ALCHEMIST))
    {
      max_rank = 2*guild_rank (th, GUILD_ALCHEMIST) - 1;
      sprintf (buf, "You can only alter chunks up to %s power.\n\r", mineral_names[max_rank]);
      stt (buf, th);
      return;
    }

  if (!check_spell (th, "alchemy", 0) || nr (1,7) == 4)
    {
      stt ("You failed to convert your chunks!\n\r", th);
      th->pc->wait += 30;
      failed = TRUE;
    }

  /* Find all approprate objects */
  
  for (obj = th->cont; obj; obj = objn)
    {
      objn = obj->next_cont;
      
      if ((raw = FNV (obj, VAL_RAW)) == NULL ||
	  raw->val[0] == RAW_MINERAL ||
	  (curr_rank = raw->val[1]) >= max_rank)
	continue;
	  
      /* Any item down here has a raw value, it's a mineral,
	 and it's rank is less than the rank we're shooting for. */
      /* If we failed, just remove some of the items... */
      
      if (failed && nr (1,16) == 5)
	{
	  free_thing (obj);
	  continue;
	}

      /* Otherwise... */

      /* For each rank we have to go up, give a 1/4 chance of improving. */
      /* If we fail any of the checks, bail out and nuke the item,
	 otherwise nuke the item and make a new item of the new rank. */

      for (i = curr_rank; i <= max_rank; i++)
	if (nr (1,4) != nr (1,4))
	  break;
      
      free_thing (obj);
      
      /* If we succeeded */
      
      if (i == max_rank)
	{
	  if ((newchunk = create_thing (100010 + max_rank)) != NULL)
	    thing_to (newchunk, th);
	}
    }
  act ("@1n magically transform@s minerals into new forms...", th, NULL, NULL, NULL, TO_ALL);
  return;
}

/* This tells whether or not a given string is a gathering raw materials
   command or not. If so, it returns the placement in the list, and
   if not, it returns -1. */

int
find_gather_type (char *comm_name)
{
  int gtype = 0;
  
  if (!comm_name || !*comm_name)
    return -1;
  for (gtype = 1; str_cmp ("max", gather_data[gtype].command); gtype++)
    if (!str_cmp (comm_name, gather_data[gtype].command))
      return gtype;
  
  return -1;
}
