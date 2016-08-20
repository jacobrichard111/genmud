#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "event.h"
#include "society.h"

/* Quick note about gems and mana from spells. The way it works is this.
   
   You have a certain amount of natural mana at a certain casting level.
   When you wear gems, the highest gem level of each mana type is added
   to your nat casting level...up to double your nat casting level. 

   The total amount of mana of any color you can use is your nat mana
   plus the mana of any gem with that same color in it. 

   So, for example, if a spell is air water mana, and a gem is air fire
   mana, it can be used to cast the spell, same for a water earth gem.

   This basically gives the players as much leeway as possible in
   finding power to cast spells. Also, this means a player can wield
   a gem with no mana, but very high level and then a lowlevel gem with
   lots of power and have a lot of mana and a high casting level.
   I like it this way, but if you don't you can make some of these
   or's into ands and require each gem be high enough to cast and
   whatnot, but I think this is more fun. */


void
find_max_mana (THING *th)
{
  int mana, bonus_pct = 0;
  RACE *race, *align;
  if (!IS_PC (th))
    return;
  
  mana = (get_stat (th, STAT_INT) + 
	  get_stat (th, STAT_WIS))*
    (30 + LEVEL (th) + 4*th->pc->remorts)/45;
  
  if ((race = FRACE(th)) != NULL)
    bonus_pct += race->mana_pct_bonus;
  if ((align = FALIGN(th)) != NULL)
    bonus_pct += align->mana_pct_bonus;
  mana = (mana * (100+bonus_pct))/100;
  th->pc->max_mana = mana;
  return;
}

/* Find the natural casting level of the player. */

int
curr_nat_cast_level (THING *th)
{
  if (!IS_PC (th))
    return MAX_LEVEL;
  
  return (get_stat (th, STAT_INT) + 
	  get_stat (th, STAT_WIS) +
	  th->pc->remorts +  
          th->pc->implants[PART_HEAD] +
	  (guild_rank (th, GUILD_WIZARD) +
	   guild_rank (th, GUILD_MYSTIC) +
	   guild_rank (th, GUILD_ALCHEMIST) +
	   guild_rank (th, GUILD_HEALER))/2 )/2;
}

/* Finds all gem mana of a certain type the player can use. */


int
find_gem_mana (THING *th, int type)
{
  THING *obj;
  VALUE *gem;
  int mana = 0;

  if (type == 0)
    return 0;
  
  for (obj = th->cont; obj && IS_WORN(obj) && obj->wear_loc <= ITEM_WEAR_WIELD; obj = obj->next_cont)
    {
      if (obj->wear_loc == ITEM_WEAR_WIELD &&
	  (gem = FNV (obj, VAL_GEM)) != NULL &&
	  IS_SET (gem->val[0], type))
	mana += gem->val[1];
    }
  return mana;
}


/* The current casting level for a thing of a certain type of mana
   is the natural level plus the MAX!!! of all the levels of all the 
   gems they are wearing (of the proper color). */

int
curr_cast_level (THING *th, int mana_types)
{
  int nat_level = curr_nat_cast_level (th), gem_level = 0;
  int casting_level_bonus = 0, mn;
  THING *obj;
  VALUE *gem;

  
  if (!IS_PC(th) || LEVEL(th) == MAX_LEVEL)
    return MAX_LEVEL;
  
  if (mana_types == 0)
    return 0;
  
  
  /* First get the bonus (if any). */
  
  for (mn = 0; mn < MANA_MAX; mn++)
    {
      if (IS_SET (mana_types, mana_info[mn].flagval))
	casting_level_bonus += th->pc->aff[FLAG_ELEM_LEVEL_START+mn];
    }
  
  for (obj = th->cont; obj && IS_WORN(obj) && obj->wear_loc <= ITEM_WEAR_WIELD; obj = obj->next_cont)
    {
      if (obj->wear_loc == ITEM_WEAR_WIELD &&
	  (gem = FNV (obj, VAL_GEM)) != NULL &&
	  IS_SET (gem->val[0], mana_types))
	gem_level = MAX (gem_level, gem->val[3]);
    }
  
  return nat_level + MIN(nat_level,gem_level) + casting_level_bonus;
}


/* This returns how much mana a player has to use for a certain level
   spell of a certain type. It first checks the level of the char
   with all gems taken into account, and then it checks the mana
   the character has of that type and returns all available mana. */


int
find_curr_mana (THING *th, int type, int level)
{
  if (!th || !IS_PC (th) || level == 0 || LEVEL(th) == MAX_LEVEL)
    return 1000;

  if (type == 0)
    return 0;
  
  if (curr_cast_level (th, type) < level)
    return 0;
  
  return th->pc->mana + find_gem_mana (th, type);
}

void
do_mana (THING *th, char *arg)
{
  char buf[STD_LEN];
  int level, mana, i;
  
  if (!IS_PC (th))
    return;

  stt ("Your current mana levels and mana stores are:\n\n\r", th);
  
  for (i = 0; i < MANA_MAX; i++)
    {
      level = curr_cast_level (th, mana_info[i].flagval);
      mana = th->pc->mana + find_gem_mana (th, mana_info[i].flagval);
      sprintf (buf, "\x1b[1;34mYou currently have \x1b[1;37m%3d%s%7s\x1b[1;34m mana for use at level \x1b[1;30m[\x1b[1;37m%2d\x1b[1;30m]\x1b[1;34m.\x1b[0;37m\n\r", mana, mana_info[i].help, mana_info[i].app, level);
      stt (buf, th);
    }
  stt ("\n\r", th);
  return;
}
	



void
do_meditate (THING *th, char *arg)
{

  if (!IS_PC (th))
    return;
  
  if (th->position == POSITION_MEDITATING)
    {
      stt ("You are already meditating!\n\r", th);
      return;
    }


  if (th->position != POSITION_STANDING &&
      th->position < POSITION_CASTING)
    {
      stt( "You must be standing or casting to meditate.\n\r", th);
      return;
    }
  
  if (FIGHTING (th))
    {
      stt ("You will never rule enough to meditate while you are fighting!\n\r", th);
      return;
    }
  
  act ("@1n cross@s @1s legs and begin@s to meditate.", th, NULL, NULL, NULL, TO_ALL);
  th->position = POSITION_MEDITATING;


  return;

}



void
do_cast (THING *th, char *arg)
{
  SPELL *spl;
  THING *vict = NULL, *viewer, *comp[MAX_COMP], *obj;
  char arg1[STD_LEN], splname[STD_LEN], *t, *oldarg;
  bool ranged = FALSE, area = FALSE, has_comp[MAX_COMP], need_comp;
  int mana, level, ticks, spell_bits, mult = 0, i, j;
  
  for (i = 0; i < MAX_COMP; i++)
    {
      comp[i] = NULL;
      has_comp[i] = TRUE;
    }
  if (!IS_PC (th))
    return;
  
  
  if (!*arg)
    {
      stt ("Cast <spellname> <target>\n\r", th);
      return;
    }

  oldarg = arg;
  
  arg = f_word (arg, arg1);
  if (!str_cmp (arg1, "mass") || !str_cmp (arg1, "area"))
    {
      arg = f_word (arg, arg1);
      area = TRUE;
    }
  if ((spl = find_spell (arg1, 0)) == NULL)
    {
      stt ("That spell doesn't exist!\n\r", th);
      return;
    }

  if (spl->spell_type != SPELLT_SPELL)
    {
      stt ("That isn't a spell.\n\r", th);
      return;
    }
  
  
  if (th->pc->prac[spl->vnum] < 30)
    {
      stt ("You don't know how to cast that spell! Try practicing it!\n\r", th);
      return;
    }
  
  if (th->position < spl->position)
    {
      stt ("You aren't in a proper position to cast that spell!\n\r", th);
      return;
    }

  
  switch (spl->target_type)
    {
	case TAR_OFFENSIVE:
	  if (FIGHTING (th))
	    vict = FIGHTING (th);
	  else if ((vict = find_thing_near (th, arg, (guild_rank (th, GUILD_WIZARD) * th->pc->remorts) / 10 )) == NULL)
	    {
	      stt ("Cast what at who?\n\r", th);
	      return;
	    }
	  if (vict == th)
	    {
	      stt ("You can't cast offensive spells at yourself.\n\r", th);
	      return;
	    }
	  if (vict->in != th->in && vict != th->in)
	    ranged = TRUE;
	  break;
	case TAR_DEFENSIVE:
	  if (!*arg)
	    vict = th;
	  else if ((vict = find_thing_in (th, arg)) == NULL)
	    {
	      stt ("You don't see that here.\n\r", th);
	      return;
	    }
	  if (DIFF_ALIGN (vict->align, th->align) && (!vict->in || !IS_AREA (vict->in)))
	    {
	      stt ("You cannot help members of that alignment!\n\r", th);
	      return;
	    }
	  break;
	case TAR_SELF:
	  vict = th;
	  area = FALSE;
	  break;
	case TAR_ANYONE:
	  if (!DOING_NOW)
	    vict = th;
	  else if ((vict = find_thing_world (th, arg)) == NULL)
	    {
	      stt ("You can't seem to find anything like that round.\n\r", th);
	      return;
	    }
	  break;
	case TAR_INVEN:
	  if ((vict = find_thing_here (th, arg, TRUE)) == NULL)
	    {
	      stt ("You don't appear to have that thing.\n\r", th);
	      return;
	    }
	  area = FALSE;
	  break;
    }
  spell_bits = flagbits (spl->flags, FLAG_SPELL);
  
  if (IS_SET (spell_bits, SPELL_ROOM_ONLY) && !IS_ROOM (vict))
    {
      stt ("This only works on rooms.\n\r", th);
      return;
    }
  
  if (IS_SET (spell_bits, SPELL_OBJ_ONLY) && 
      (IS_AREA (vict) || IS_ROOM (vict) || CAN_MOVE (vict) ||
       CAN_FIGHT (vict)))
    {
      stt ("You can't cast this spell on anything but objects.\n\r", th);
      return;
    }
  
  if (IS_SET (spell_bits, SPELL_MOB_ONLY) &&
      (IS_AREA (vict) || IS_ROOM (vict) ||
       (!CAN_FIGHT (vict) && !CAN_MOVE (vict))))
    {
      stt ("You can only cast this spell on creatures!\n\r", th);
      return;
    }

  if (IS_SET (spell_bits, SPELL_OPP_ALIGN) && !DIFF_ALIGN (th->align, vict->align))
    {
      stt ("This spell must be cast on someone of opposite alignment!\n\r", th);
      return;
    }
  
  if (IS_SET (spell_bits, SPELL_SAME_ALIGN) && DIFF_ALIGN (th->align, vict->align))
    {
      stt ("This spell must be cast on someone of the same alignment!\n\r", th);
      return;
    }
  
  if (IS_SET (spell_bits, SPELL_PC_ONLY) && !IS_PC (vict))
    {
      stt ("This can only be cast on a player!\n\r", th);
      return;
    }
  
  if (IS_SET (spell_bits, SPELL_NPC_ONLY) && IS_PC (vict))
    {
      stt ("This spell cannot be cast on a player!\n\r", th);
      return;
    }
  

  
  if (!vict)
    {
      stt ("You couldn't find your victim!\n\r", th);
      return;
    }
  
  if ((area || ranged) && spl->level < 20)
    {
      stt ("Spells below level 20 cannot be area or ranged spells!\n\r", th);
      return;
    }

  if (IS_PC (th) && !IS_PC1_SET(th, PC_HOLYWALK))
    {
      /* Deal with components, see which ones are needed. */      
      for (i = 0; i < MAX_COMP; i++)
	{
	  comp[i] = NULL;
	  if (spl->comp[i] > 0 && 
	      (obj = find_thing_num (spl->comp[i])) != NULL &&
	      !IS_SET (obj->thing_flags, TH_IS_ROOM | TH_IS_AREA))
	    has_comp[i] = FALSE;
	  else
	    has_comp[i] = TRUE;
	}
      /* Now see if we need any components. */

      need_comp = FALSE;
      for (i = 0; i < MAX_COMP; i++)
	{
	  if (!has_comp[i])
	    {
	      need_comp = TRUE;
	      break;
	    }
	}
      
      if (need_comp)
	{
	  for (obj = th->cont; obj && need_comp; obj = obj->next_cont)
	    {
	      for (i = 0; i < MAX_COMP; i++)
		{
		  if (!has_comp[i] && obj->vnum == spl->comp[i] &&
		      (!spl->compname || !*spl->compname || 
		       named_in (NAME (obj), spl->compname)))
		    {
		      comp[i] = obj;
		      has_comp[i] = TRUE;
		      need_comp = FALSE;
		      for (j = 0; j < MAX_COMP; j++)
			{
			  if (!has_comp[j])
			    need_comp = TRUE;
			}
		      break;
		    }
		}
	    }
	}
      
      if (need_comp)
	{
	  for (i = 0; i < MAX_COMP; i++)
	    {
	      if (!has_comp[i] && 
		  (obj = find_thing_num (spl->comp[i])) != NULL &&
		  !IS_SET (obj->thing_flags, TH_IS_ROOM | TH_IS_AREA))
		{
		  act ("You need @3n to cast this spell!", th, NULL, obj, NULL, TO_CHAR);
		}
	    }
	  stt ("Spell failed.\n\r", th);
	  return;
	}
    }
	      
  
	     
  
  level = spl->level * (3 + ranged + area)/3;
  mana = spl->mana_amt * (3 + ranged + area)/3;
  ticks = spl->ticks * (2 + ranged + area)/2;
  
  /* Now adjust casting time for guilds etc... 
     So, max bonus is 5*4 + 10 + 7 = 37 so maxxed chars cast about
     50 pct faster than newbies. */
  
  if (IS_PC (th))
    mult = 100 -
      (guild_rank (th, GUILD_WIZARD) + guild_rank (th, GUILD_HEALER) +
       guild_rank (th, GUILD_MYSTIC) + guild_rank (th, GUILD_ALCHEMIST) +
       th->pc->remorts + th->pc->implants[PART_HEAD]);
  else 
    mult = 100;
  
  ticks = ticks *mult/100;
  
  
  if (LEVEL (th) < level)
    {
      stt ("You are not powerful enough to cast this spell, perhaps don't cast it ranged or area?\n\r", th);
      return;
    }
  
  
  if (find_curr_mana (th, spl->mana_type, level) < mana)
    {
      stt ("You don't have enough mana of the proper type to cast this spell!\n\r", th);
      return;
    }
  
  if (!DOING_NOW)
    {
      char buf[STD_LEN];
      if (nr (1,23546) != 4534)
	act ("@1n begin@s to utter some strange incantations...", th, NULL, NULL, NULL, TO_ALL);
      else
	act ("@1n begin@s to udder some strange incowtations...", th, NULL, NULL, NULL, TO_ALL);
      sprintf (buf, "cast %s", oldarg);
      
      /* There is a constant divisor here because the number of pulses
	 per second is different from the number of updates per second,
	 so we need to divide the ticks so they aren't so long. */
      
      ticks = UPD_PER_SECOND*ticks/PULSE_PER_SECOND;
      add_command_event (th, buf, ticks);
      th->position = POSITION_CASTING;
      return;
    }
  
  if (!check_spell (th, NULL, spl->vnum) || 
      /* Befuddle makes you mess up casting a lot. */
      (nr (1,4) != 2 && IS_HURT (th, AFF_BEFUDDLE)))
    {
      stt ("\x1b[0;13mYou lost your concentration!\x1b[0;37m\n\r", th);
      take_mana (th, spl->mana_type, mana/2);
      return;
    }
  /* Jumble the spell so people can't tell what is being cast if they
     can't cast the spell. */
  strcpy (splname, spl->name);
  
  for (t = splname; *t; t++)
    {
      if (*t >= 'A' && *t <= 'Z')
	*t = (((*t - 'A') * 17 + 5) % 26) + 'A';
       if (*t >= 'a' && *t <= 'z')
	*t = (((*t - 'a') * 17 + 5) % 26) + 'a';
    }
  for (viewer = th->in->cont; viewer; viewer = viewer->next_cont)
    {
      if (!IS_PC (viewer) || viewer->pc->prac[spl->vnum] > 19)
	act ("@1n utter@s the words '@t'.", th, NULL, viewer, spl->name, TO_VICT);
      else
	act ("@1n utter@s the words '@t'.", th, NULL, viewer, splname, TO_VICT);
    }
  take_mana (th, spl->mana_type, mana);
  cast_spell (th, vict, spl, area, ranged, NULL);
  update_kde (th, KDE_UPDATE_HMV);
  for (i = 0; i < MAX_COMP; i++)
    {
      if (comp[i] && comp[i]->in == th)
	free_thing (comp[i]);
    }
  th->pc->wait += ticks/3;
  th->position = POSITION_STANDING;
  return;
}
	



void
take_mana (THING *th, int type, int amount)
{
  THING *obj;
  VALUE *gem;
  
  
  if (!th || !IS_PC (th) || type == 0 || amount < 1)
    return;
  
  if (th->pc->mana >= amount)
    {
      th->pc->mana -= amount;
      return;
    }
  amount -= th->pc->mana;
  th->pc->mana = 0;
  
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if (amount < 1 || !IS_WORN(obj) ||
	  obj->wear_loc > ITEM_WEAR_WIELD)
	break;
      
      if (obj->wear_loc == ITEM_WEAR_WIELD &&
	  (gem = FNV (obj, VAL_GEM)) != NULL &&
	  IS_SET (gem->val[0], type))
	{
	  if (gem->val[1] > amount)
	    {
	      gem->val[1] -= amount;
	      return;
	    }
	  amount -= gem->val[1];
	  gem->val[1] = 0;
	}
    }
  
  return;
}
 
int
heal_spell_wis (int level)
{
  return MIN (MAX_STAT_VAL, 10 + level/4);
}

int 
att_spell_int (int level)
{
  return MIN (MAX_STAT_VAL, 15 + level/4);
}

/* This actually casts a spell... */


void
cast_spell (THING *caster, THING *vict, SPELL *spl, bool area, bool ranged, EVENT *event)
{
  THING *newthing, *targ, *targn, *newthing2 = NULL, *th;
  bool done = FALSE, killed = FALSE;
  int casting_power_bonus = 0, mn;
  int spell_bits, damg, mult, i;
  char buf[STD_LEN];
  char *t, *s;
  FLAG *newflag, *flg;
  VALUE *portal;
  if (!caster || !vict || !spl || !vict->in)
    return;
  
  /* This deals with worn objects casting spells so that everything
     works out ok. */

  if (caster->in && spl->target_type == TAR_OFFENSIVE &&
      FIGHTING (caster->in) == vict && caster->wear_loc != ITEM_WEAR_NONE)
    th = caster->in;
  else
    th = caster;
  
  if (spl->target_type == TAR_OFFENSIVE &&
      th->in != vict->in &&
      (IS_ACT1_SET (vict, ACT_SENTINEL) ||
       !CAN_MOVE(vict)) && 0)
    return;
  
  if (IS_ROOM_SET (vict->in, ROOM_NOMAGIC))
    {
      stt ("The spell fizzles....\n\r", th);
      return;
    }

  if (IS_PC (caster))
    {
      for (mn = 0; mn < MANA_MAX; mn++)
	{
	  if (IS_SET (spl->mana_type, mana_info[mn].flagval))
	    casting_power_bonus += caster->pc->aff[FLAG_ELEM_POWER_START+mn];
	}
      /* Bonus over 40 is 1/5 power. C -= 40, C /= 5, C += 40, it
	 simplifies to C = (C-40)/5+40 = C/5 + 32 */
      if (casting_power_bonus > 40)
	{
	  casting_power_bonus = casting_power_bonus/5 + 32;
	}
    }
      
  
  if (area && vict->in && vict->in->cont)
    targ = vict->in->cont;
  else 
    targ = vict;

  spell_bits = flagbits (spl->flags, FLAG_SPELL);
  
  if (targ->in && IS_AREA (targ->in))
    area = FALSE;

  if (IS_SET (spell_bits, SPELL_TRANSPORT) &&
      IS_HURT (th, AFF_CURSE))
    {
      stt ("You are cursed!\n\r", th);
      return;
    }
  /* If a spell is delayed, it actually does something when 
     the event finally gets called. */
  
  
  if (!IS_SET (spell_bits, SPELL_DELAYED) || event)
    {
      
      if (spl->creates &&
	  (newthing = create_thing (spl->creates)) != NULL)
	{
	  newthing->wear_loc = ITEM_WEAR_NONE;
	  newthing->wear_num = 0;
	  reset_thing (newthing, 0);
	  if ((portal = FNV (newthing, VAL_EXIT_I)) != NULL)
	    {
	      if ((!IS_ROOM (th->in) || !vict->in || !IS_ROOM (vict->in)))
		{
		  free_thing (newthing);
		  stt ("The spell failed!\n\r", th);
		  return;
		}	
	      portal->val[0] = vict->in->vnum;
	      newthing2 = create_thing (spl->creates);
	      if (newthing2 &&
		  (portal = FNV (newthing2, VAL_EXIT_I)) != NULL)
		portal->val[0] = th->in->vnum;
	    }  
	  if (!CAN_MOVE(newthing) && 
	      !IS_SET (newthing->thing_flags, TH_NO_MOVE_BY_OTHER | TH_NO_TAKE_BY_OTHER))
	    thing_to (newthing, th);
	  else
	    {
	      VALUE *pet;
	      
	      thing_to (newthing, th->in);
	      if ((pet = FNV (newthing, VAL_PET)) == NULL)
		{
		  pet = new_value ();
		  pet->type = VAL_PET;
		  add_value (newthing, pet);
		}
	      newthing->align = th->align;
	      set_value_word (pet, NAME (th));
	      do_follow (newthing, NAME (th));
	    }
	  if (newthing2)
	    thing_to (newthing2, vict->in);
	  for (i = 0; i < 2; i++)
	    {
	      if (i == 0)
		{
		  if (newthing->short_desc[0])
		    t = newthing->short_desc;
		  else if (newthing->proto && newthing->proto->short_desc[0])
		    t = newthing->proto->short_desc;
		  else 
		    t = NULL;
		}
	      else
		{
		  if (newthing->long_desc[0])
		    t = newthing->long_desc;
		  else if (newthing->proto && newthing->proto->long_desc[0])
		    t = newthing->proto->long_desc;
		  else 
		    t = NULL;
		}
	      if (t)
		{
		  for (s = buf; *t; t++)
		    {
		      if (*t != '@')
			{
			  *s = *t;
			  s++;
			}
		      else
			{
			  t++;
			  if (LC (*t) == 'n')
			    {
			      *s = '\0';
			      strcat (buf, NAME (th));
			      s = buf + strlen (buf);
			    }
			  else
			    {
			      *s = *t;
			      s++;
			    }
			}
		    }
		  *s = '\0';
		  if (i == 0) 
		    {
		      free_str (newthing->short_desc);
		      newthing->short_desc = new_str (buf);
		    }
		  else
		    {
		      free_str (newthing->long_desc);
		      newthing->long_desc = new_str (buf);
		    }
		}
	    }
	  if (newthing2)
	    {
	      newthing2->short_desc = new_str (newthing->short_desc);
	      newthing2->long_desc = new_str (newthing->long_desc);
	    }
	}
    }
  /* This gives you knowledge about something. (identify) */
  
  if (IS_SET (spell_bits, SPELL_KNOWLEDGE))
    {
      if (targ == th->in)
	{
	  sprintf (buf, "You are in: %s\n\r", name_seen_by (targ, th));
	  stt (buf, th);
	  stt (show_flags (targ->flags, 0, (LEVEL(th) >= BLD_LEVEL ? TRUE : FALSE)), th);
	  if (targ->in && IS_AREA (targ->in))
	    {
	      sprintf (buf, "%s is located within %s.\n\r", 
		       NAME (targ), NAME (targ->in));
	      stt (buf, th);
	    }
	  return;
	}
      
      /* Otherwise this thing must be in the same room as you or
	 it must be in your inventory. */
      if (targ->in != th && targ->in != th->in)
	{
	  stt ("The knowledge is obscured by the great distance...\n\r", th);
	  return;
	}
      if (targ->in == th->in && !CAN_FIGHT (targ) && !CAN_MOVE (targ))
	{
	  stt ("You cannot gain knowledge of this item without it being in your inventory.\n\r", th);
	  return;
	}
      
      sprintf (buf, "----- This is %s. -----\n\r", NAME (targ));
      stt (buf, th);
      
      if (targ->in == th)
	{
	  sprintf (buf, "It weighs %d stone%s and %d pebble%s. It costs %d coin%s.\n\r", 
		   targ->weight/WGT_MULT, 
		   (targ->weight / WGT_MULT == 1 ? "" : "s"),
		   targ->weight % WGT_MULT,
		   (targ->weight % WGT_MULT == 1 ? "" : "s"),
		   targ->cost,
		   (targ->cost == 1 ? "" : "s"));
	  stt (buf, th);
	}
      else
	{
	  sprintf (buf, "This thing has %d/%d health points and %d/%d stamina points.\n\r",
		   targ->hp, targ->max_hp, targ->mv, targ->max_mv);
	  stt (buf, th);
	}
      
      if (!IS_SET (targ->thing_flags, TH_NO_FIGHT))
	stt ("This thing can fight.\n\r", th);
      if (!IS_SET (targ->thing_flags, TH_NO_MOVE_SELF))
	stt ("This thing can move itself.\n\r", th);
      if (!IS_SET (targ->thing_flags, TH_NO_CONTAIN) &&
	  targ->in == th)
	{
	  stt ("This thing can contain other things.\n\r", th);
	  if (targ->size == 0)
	    stt ("It can contain an unlimited number of things.\n\r", th);
	  else
	    {
	      sprintf (buf, "It can contain %d stone%s, %d pebble%s of weight.\n\r", 
		       targ->size / WGT_MULT, 
		       (targ->size / WGT_MULT == 1 ? "" : "s"),
		       targ->size % WGT_MULT, 
		       (targ->size % WGT_MULT == 1 ? "" : "s"));
	    }
	}      
      if (IS_SET (targ->thing_flags, TH_NO_TAKE_BY_OTHER))
	stt ("This thing cannot be picked up.\n\r", th);
      if (IS_SET (targ->thing_flags, TH_NO_DROP))
	stt ("This thing cannot be dropped.\n\r", th);
      stt (show_values (targ->values, 0, TRUE), th);
      stt (show_flags (targ->flags, 0, LEVEL (th) >= BLD_LEVEL), th);	   
      return;      
    }
  
  for (; targ && (area || !done); targ = targn)
    {
      done = TRUE;
      killed = FALSE;
      targn = targ->next_cont;
      
      
      if (spl->target_type == TAR_OFFENSIVE)
	{
	  if (!CAN_FIGHT (targ) || IS_ROOM (targ) || 
	      is_friend (th, targ) || targ == th ||
	      (!IS_PC (th) && !IS_PC (targ) && area &&
	       !DIFF_ALIGN (th->align, targ->align)))
	    continue;
	  if (IS_PC (targ) && LEVEL(targ) >= BLD_LEVEL &&
	      IS_PC1_SET (targ, PC_HOLYWALK | PC_HOLYPEACE))
	    continue;
	}
	  
      if (!DIFF_ALIGN (th->align, targ->align) && IS_PC (targ) &&
	  ((spl->target_type == TAR_OFFENSIVE && area) ||
	   IS_SET (spell_bits, SPELL_OPP_ALIGN))) 
	continue;
      
      
      if (DIFF_ALIGN (th->align, targ->align) && !IS_ROOM (targ) &&
	  (CAN_MOVE (th) || CAN_FIGHT (th)) &&
	  (spl->target_type == TAR_DEFENSIVE ||
	   IS_SET (spell_bits, SPELL_SAME_ALIGN)))
	continue;
      
      if (spl->target_type == TAR_DEFENSIVE &&
	  (!CAN_MOVE (targ) && !CAN_FIGHT (targ)))
	continue;


      if (IS_MOB_SET (targ, MOB_NOMAGIC))
	{
	  act ("@1p spell fizzles as it hits @3n!", th, NULL, targ, NULL, TO_ALL);
	  continue;
	}
      
      /* Spell reflect works once, then it's gone. */

      if (IS_AFF (targ, AFF_REFLECT))
	remove_flagval (targ, FLAG_AFF, AFF_REFLECT);

      /* Message uses "caster" instead of "th" so that you see
	 "A flaming longsword burns Bob" as opposed to "Fred burns Bob" */
      
      
      if (event && spl->message_repeat)
	act (spl->message_repeat, targ, NULL, caster, NULL, TO_ALL);
      else
	act (spl->message, caster, NULL, targ, NULL, TO_ALL);
      
      
      if (!event)
	{
	  add_damage_event (th, targ, spl);

	  /* If the spell is only delayed, don't make the damage
	     or other stuff happen now. */
	  if (IS_SET (spell_bits, SPELL_DELAYED))
	    {
	      if (spl->target_type == TAR_OFFENSIVE &&
		  th->in == targ->in)
		start_fighting (th, targ);	      
	      continue;
	    }
	}
      
      
      if (spl->target_type == TAR_OFFENSIVE)
	{
	  
	  /* No ranged spells on sentinel mobs. */
	  
	  if (th->in != targ->in &&
	      IS_ACT1_SET (targ, ACT_SENTINEL))
	    continue;
	  
	  
	  mult = MIN (120, 100 * get_stat (th, STAT_INT) / att_spell_int (spl->level));
	  if (mult < 90)
	    mult /= 2;
	  
	  /* Add pc bonuses for guilds remorts implants etc... */
	  
	  if (IS_PC (th))
	    {
	      mult += th->pc->remorts * 3;
	      mult += spl->level * guild_rank (th, GUILD_WIZARD)/4;
	      mult += nr (0, th->pc->aff[FLAG_AFF_SPELL_ATTACK]);
	      mult += spl->level * implant_rank (th, PART_HEAD)/8;
	      mult += casting_power_bonus;
	    }	  
	  else
	    mult += LEVEL(th) + spl->level * 2;
	  
	  /* Ok, now do spell resistances and extra damage.
	     The way it works is this. If you resist any of the mana
	     types, you resist the whole spell == less damage.
	     Otherwise, if you are vulnerable to any of the spell
	     mana types, you take more damage. */
	  
	  /* The 14 is the number of bits it takes to shift 0x0001 to 0x8000 */
	  
	  if (flagbits (vict->flags, FLAG_PROT) & (spl->mana_type << 14))
	    mult = mult*2/3;
	  else if (flagbits (vict->flags, FLAG_HURT) & (spl->mana_type << 14))
	    mult = mult * 3/2;
	           
	  /* Add pc bonuses for guilds/remorts/implants etc... */
	  
	  if (IS_PC (targ))
	    mult -= nr (targ->pc->aff[FLAG_AFF_SPELL_RESIST]/3,
			targ->pc->aff[FLAG_AFF_SPELL_RESIST]) +
	      6 * implant_rank (vict, PART_HEAD); 
	  mult /= (1 + area + ranged);
	  damg = (math_parse (spl->damg, LEVEL (th)) +
		  math_parse (spl->damg, LEVEL (th)))/2;
	  damg = damg * mult/100;
	  if (area && ranged)
	    damg /= 2;
	  else if (area || ranged)
	    damg = damg * 3/4;
	  
	  /* If this is repeated damage due to an event, find the percent
	     of damage left this time. */
	  
	  if (event)
	    {
	      int repeat_count;
	      for (repeat_count = 0; 
		   repeat_count < MAX(0, spl->repeat - event->times_left);
		   repeat_count++)
		damg = (100-spl->damage_dropoff_pct)*damg/100;
	    }
	  killed = damage (th, targ, damg, spl->ndam);
	  /* Now deal with vampirism...giving half hps to the caster. */
	  
	  if (IS_SET (spell_bits, SPELL_VAMPIRIC))
	    {
	      act ("$9@1n feast@s on the blood.$7", th, NULL, NULL, NULL, TO_ALL);
	      th->hp += MIN (damg/2, th->max_hp - th->hp);
	    }
	}
      /* Now add affects. */
      
      
      if (!killed)
	{
	  /* Check if we have flags and that they aren't just spell flags. */
	  if (spl->flags && 
	      (spl->flags->type != FLAG_SPELL ||
	       (spl->flags->next && spl->flags->next->type != FLAG_SPELL)))
	    {
	      FLAG *vflag, *vflagn;
	      
	      if (IS_SET (spell_bits, SPELL_REMOVE_BIT))
		{
		  int spell_flagbits[NUM_FLAGTYPES];
		  int j;
		  int spell_from_vnum[10];
		  
		  for (j = 0; j < 10; j++)
		    spell_from_vnum[j] = 0;
		  
		  /* This finds all the flags that the spell can set. */
		  
		  for (j = 0; j < NUM_FLAGTYPES; j++)
		    spell_flagbits[j] = flagbits (spl->flags, j);
		  
		  /* We now go down the target's flags and we check each 
		     that is timed, came from a spell, and is of the proper
		     type. Then, we check if any of the bits set in the spell
		     are set in the flag here. If they are, we attempt to
		     remove the flag. This also will attempt to remove flags
		     which are associated with spells which were dispelled.
		     So, if a spell blinds and gives - damroll, the first pass
		     removes the blindness (maybe), and the second pass
		     removes the damroll (maybe). */
		  
		  for (vflag = targ->flags; vflag != NULL; vflag = vflagn)
		    {
		      vflagn  = vflag->next;		      
		      if ((!IS_SET(spell_bits, SPELL_PERMA) &&
			   (vflag->from == 0 || 
			    vflag->timer == 0)) ||
			  vflag->type >= NUM_FLAGTYPES)
			continue;
		      
		      
		      /* Now see if this is one of the spell affects we can
			 remove since the bits are correct. */
		      
		      /* So, if there are no common bits between the spell flag
			 bits, and the targim flag bits, we continue. */
		      
		      if (!IS_SET (spell_flagbits[vflag->type], vflag->val))
			continue;
		      
		      
		      /* So now we look for an open slot to remove spell 
			 affects from in the second pass. */
		      
		      /* If we are removing permanent affects, we 
			 can do it for any flag, so, we remove the
			 affects from this flag since it matches the
			 correct type...unless it has a spell that it's
			 from. */
		      
		      if (vflag->from > 0)
			{
			  for (j = 0; j < 10; j++)
			    {
			      if (spell_from_vnum[j] == 0)
				{
				  spell_from_vnum[j] = vflag->from;
				  break;
				}
			    }			  
			  /* Now we remove vflag from the target. */
			  			 
			  aff_from (vflag, targ); 
			  
			
			}
		      else if (IS_SET(spell_bits, SPELL_PERMA))
			{
			  /* Remove the spell bits from the flag. */
			  RBIT (vflag->val, spell_flagbits[vflag->type]);
			  if (vflag->val == 0)			    
			    aff_from (vflag, targ);
			  
			 
			  continue;
			}
		      else
			continue; 
		      /* Bonus for healing. */
		      if (vflag->type == FLAG_HURT &&
			  find_society_in (targ)  &&
			  !DIFF_ALIGN (targ->align, th->align))
			add_society_reward (th, targ, REWARD_HEAL, (LEVEL (spl)+LEVEL(targ)));
		    }
		  
		  
		      
		      
		  /* Now the second pass. If any of the spell_from_vnum's are
		     > 0, we look for other flags which are FROM the same 
		     thing, and those are removed. */
		  
		  
		  for (vflag = targ->flags; vflag != NULL; vflag = vflagn)
		    {
		      vflagn = vflag->next;
		      
		      if (vflag->type < AFF_START || vflag->type >= AFF_MAX ||
			  vflag->from == 0 || vflag->timer == 0)
			continue;
		      
		      for (j = 0; j < 10 && spell_from_vnum[j] > 0; j++)
			{
			  if (vflag->from == spell_from_vnum[j])
			    {
			      if (IS_SET(spell_bits, SPELL_PERMA))
				aff_from (vflag, targ);
			      break;
			    }
			}
		    }
		}
	      else if ((spl->target_type != TAR_OFFENSIVE ||
		       !resists_spell (targ, th, spl)) &&
                       (!area || nr(1,3) != 2))
		{
		  if ((spl->duration && *spl->duration) ||
		      IS_SET (spell_bits, SPELL_PERMA))
		    {
		      for (flg = spl->flags; flg != NULL; flg = flg->next)
			{
			  if (flg->type < NUM_FLAGTYPES &&
			      (flg->type <= FLAG_THING ||
				   flaglist[flg->type].whichflag == NULL ||
			       flg->type == FLAG_SPELL))
			    continue;
			  
			  if (flg->type >= AFF_MAX)
			    continue;
			  
			  newflag = new_flag ();
			  newflag->type = flg->type;
			  newflag->from = spl->vnum;
			  if (flg->type < NUM_FLAGTYPES || flg->val != 0)
			    newflag->val = flg->val;
			  else
			    newflag->val = math_parse (spl->damg, LEVEL (th)); 
			  if (flg->type == FLAG_HURT &&
			      (!IS_PC (targ) || /* Admins don't sleep. */
			       LEVEL (targ) < BLD_LEVEL) &&
			      IS_SET (flg->val, AFF_SLEEP))
			    {
			      stop_fighting (targ);
			      targ->position = POSITION_SLEEPING;
			    }
			  newflag->timer = math_parse (spl->duration, LEVEL (th));
			  if (newflag->timer > 0)			
			    aff_update (newflag, targ);
			  else if (IS_SET (spell_bits, SPELL_PERMA))
			    {
			      newflag->timer = 0;
			      aff_update (newflag, targ);
			    }
			  else
			    free_flag (newflag);
			}
			}
		  if (spl->target_type == TAR_OFFENSIVE)
		    {
		      act ("@3n succumbs to your spell!", th, NULL, targ, NULL, TO_CHAR);		   
		      stt ("You succumb to the spell!\n\r", targ);
		    }
		}
	      else
		{
		  act ("@3n resists your spell!", th, NULL, targ, NULL, TO_CHAR);
		  stt ("You resist the spell!\n\r", targ);
		    }
	    }

	  /* Now deal with specialized spells like healing/
	     transport etc... */

	  if (spell_bits)
	    {
	      if (IS_SET (spell_bits, SPELL_HEALS | SPELL_REFRESH))
		{
		  mult = MIN (120, 100 * get_stat (th, STAT_WIS) / heal_spell_wis (spl->level));
		  if (mult < 90)
		    mult /= 2;
		  else if (IS_PC (th))
		    {
		      mult += th->pc->remorts * 3;
		      mult += spl->level * guild_rank (th, GUILD_HEALER)/4;
		      mult += nr (0, th->pc->aff[FLAG_AFF_SPELL_HEAL]);
		      mult += spl->level * implant_rank (th, PART_HEAD)/8;
		      mult += casting_power_bonus;
		    }
		  else
		    mult += LEVEL(th) + spl->level * 2;
		  
		  mult /= (1 + area + ranged);
		  damg = (math_parse (spl->damg, LEVEL (th)) +
			  math_parse (spl->damg, LEVEL (th)))/2;
		  damg = damg * mult/100;
		  if (area && ranged)
		    damg /= 2;
		  else if (area || ranged)
		    damg = damg * 3/4; 
		  if (event)
		    {
		      int repeat_count;
		      for (repeat_count = 0; 
			   repeat_count < MAX(0, spl->repeat - event->times_left);
			   repeat_count++)
			damg = (100-spl->damage_dropoff_pct)*damg/100;
		    }
		  if (IS_SET (spell_bits, SPELL_HEALS))
		    {
		      if (targ->hp < targ->max_hp &&
			  find_society_in (targ) &&
			  !DIFF_ALIGN (targ->align, th->align))
			add_society_reward (th, targ, REWARD_HEAL, damg);
		      
			  targ->hp += damg;
		      if (targ->hp > targ->max_hp)
			targ->hp = targ->max_hp;
		    }
		  if (IS_SET (spell_bits, SPELL_REFRESH))
		    {  
		      if (targ->mv < targ->max_mv &&
			  find_society_in (targ) &&
			  !DIFF_ALIGN (targ->align, th->align))
			add_society_reward (th, targ, REWARD_HEAL, damg);
		      
		      targ->mv += damg;
		      if (targ->mv > targ->max_mv)
			targ->mv = targ->max_mv;
		    }
		  update_kde (targ, KDE_UPDATE_HMV);
		}
	      /* Manadrain spells...maybe doctor this up so that
		 it just drains a little bit so you can have a real
		 spell with a big damage and a small manadrain. Dunno. */
	      if (IS_SET (spell_bits, SPELL_DRAIN_MANA))
		{
		  int drained = math_parse (spl->damg, LEVEL (th));
		  if (IS_PC (targ))
		    targ->pc->mana -= MIN (targ->pc->mana, drained);
		  if (IS_PC (th) /* && IS_SET (spell_bits, SPELL_VAMPIRIC_MANA) */)
		    {
		      th->pc->mana += drained/2;
		      update_kde (th, KDE_UPDATE_HMV);
		      if (th->pc->mana > th->pc->max_mana)
			th->pc->mana = th->pc->max_mana;
		    }
		  update_kde (targ, KDE_UPDATE_HMV);
		}
	      if (IS_SET (spell_bits, SPELL_SHOW_LOCATION) && targ && targ->in)
		{
		  if (!area)
		    {
		      sprintf (buf, "%s is in %s.\n\r", NAME (targ), NAME (targ->in));
		      stt (buf, th);
		      stt ("\n\rPING\n\n\r      PING\n\n\r           PING!\n\n\r", targ);
		    }
		  else
		    {
		      show_thing_to_thing (th, targ->in, LOOK_SHOW_SHORT | LOOK_SHOW_EXITS | LOOK_SHOW_CONTENTS | LOOK_SHOW_DESC);
		      sprintf (buf, "%s is watching you!\n\n\r", NAME (th));
		      stt (buf, targ);
		      area = FALSE;
		    }
		}
	      
	      if (IS_SET (spell_bits, SPELL_TRANSPORT) && th->in && targ)
		{
		  THING *end_in, *mover;
                  if (!targ->in)
		    continue;
		  if (IS_SET (spell_bits, SPELL_TRANSPORT) &&
		      spl->target_type != TAR_OFFENSIVE &&
		      IS_HURT (targ, AFF_CURSE))
		    {
		      act ("@3n is cursed!\n\r", th, NULL, NULL, NULL, TO_CHAR);
		      continue;
		    }
 
		  if (spl->transport_to == 0) /* recall */
		    {		
		      mover = targ; 
		      if (IS_ROOM_SET (mover->in, ROOM_NORECALL))
			end_in = NULL;
		      else
			end_in = find_thing_num (100 + targ->align);
		    }		  
		  else if (spl->transport_to == 1) /* teleport */
		    {
		      mover = targ;
		      if (IS_ROOM_SET (mover->in, ROOM_NORECALL))
			end_in = NULL;
		      else
			end_in = find_random_room (NULL, FALSE, 0, 0);
		    }
		  else if (spl->transport_to == 2) /* summon */
		    {
		      mover = targ;
		      if (IS_ROOM_SET (mover->in, ROOM_NORECALL) ||
			  IS_ROOM_SET (th->in, ROOM_NOSUMMON))
			end_in = NULL;
		      else
			end_in = th->in;
		    }
		  else if (spl->transport_to == 3) /* inverse summon */
		    {
		      mover = th;
		      if (IS_ROOM_SET (mover->in, ROOM_NORECALL))
			end_in = NULL;
		      else if (IS_ROOM_SET (targ->in, ROOM_NOSUMMON))
			end_in = NULL;
		      else
			end_in = targ->in;
		    }
		  else
		    { 
		      mover = targ;
		      if (IS_ROOM_SET (mover->in, ROOM_NORECALL))
			end_in = NULL;
		      else
			{ /* specific */
			  end_in = find_thing_num (spl->transport_to);
			  if (end_in && IS_ROOM_SET (end_in, ROOM_NOSUMMON))
			    end_in = NULL;
			}
		    }
		  if (end_in == NULL ||
		      (end_in != th->in && !IS_ROOM (end_in)))
		    {
		      stt ("The spell failed!\n\r", th);
		      continue;
		    }
		  act ("@1n disappear@s!", mover, NULL, NULL, NULL, TO_ALL);
		  thing_to (mover, end_in);
		  act ("@1n appear@s in a bright flash!", mover, NULL, NULL, NULL, TO_ALL);
		  do_look (mover, "");
		}	      	            
	    }  
	}
      else if (targ == th)
      break;
    }
  return;
}



bool
resists_spell (THING *vict, THING *th, SPELL *spl)
{
  int num;
  
  if (!th || !vict || !spl)
    return TRUE;
  
  num = 50 + (LEVEL (vict) - LEVEL (th)  - LEVEL (spl)/2 +
	      get_stat (vict, STAT_WIS) - get_stat (th, STAT_WIS))/2 +
    3 * implant_rank (vict, PART_HEAD) - 3 * implant_rank (th, PART_HEAD);
  if (IS_PC (vict))
    num += vict->pc->aff[FLAG_AFF_SPELL_RESIST];
  num = MID (10, num, 85);
  
  return 
    np () <= num;
}


void
find_spell_to_cast (THING *th)
{
  SPELL *spl;
  VALUE *caster;
  int spell_chose = 0, j, num_spells = 0;
  THING *vict;

  /* Check if this thing can cast and if it chooses to atm....*/
  
  if ((caster = FNV (th, VAL_CAST)) == NULL ||
      nr (1,10) >= MIN (7, LEVEL (th)/30 + 1) ||
      th->position < POSITION_FIGHTING ||
      (!CAN_MOVE (th) && !IS_WORN(th) &&       
       !IS_ROOM (th)))
    return;
  
  /* Find all the spells it can pick from in its caster value. */

  for (j = 0; j < NUM_VALS; j++)
    if ((spl = find_spell (NULL, caster->val[j])) != NULL &&
	spl->spell_type == SPELLT_SPELL)
      num_spells++;
  
  /* Pick one of them. */

  if (num_spells > 0)
    spell_chose = nr (1, num_spells);
  num_spells = 0;
  
  /* Now find which one we picked. (if any) */
  
  for (j = 0; j < NUM_VALS; j++)
    {
      if ((spl = find_spell (NULL, caster->val[j])) != NULL &&
	  spl->spell_type == SPELLT_SPELL &&
	  ++num_spells == spell_chose)
	break;
    }
  
  if (!spl)
    return;
  
  
  /* Now, if the thing is fighting, do an offensive cast. */
  
  if (FIGHTING (th) || 
      (IS_WORN(th) && th->in && FIGHTING (th->in)))
    {
      if (spl->target_type == TAR_OFFENSIVE)
	{
	  if ((vict = FIGHTING (th)) == NULL)
	    vict = FIGHTING (th->in);
	  cast_spell (th, vict, spl, (nr (1,7) == 3 && np () <LEVEL (th)), FALSE, NULL);
	}
      else
	cast_spell (th, th, spl,  FALSE, FALSE, NULL); 
    }
  else if (spl->target_type == TAR_SELF) /* Cast on self */
    cast_spell (th, th, spl, FALSE, FALSE, NULL);
  else if ((spl->target_type == TAR_DEFENSIVE && th->in) ||
	   (spl->target_type == TAR_OFFENSIVE && IS_ROOM (th)))
    { /* Otherwise find a random target in the room to cast it on. */
      int num_vict = 0, vict_chose = 0;
      if (IS_WORN(th)) /* If this is worn, cast on wearer. */
	vict = th->in;
      else /* Otherwise search for a victim nearby... */
	{
	  if (IS_ROOM (th))
	    vict = th->cont;
	  else 
	    vict = th->in->cont;


	  /* Only choose certain kinds of victims */

	  for (; vict != NULL; vict = vict->next_cont)
	    {
	      if (!CAN_FIGHT (vict))
		continue;
	      if (IS_PC (vict) &&
		  (is_enemy (th, vict) == 
		   (spl->target_type == TAR_OFFENSIVE)))
		num_vict++;
	    }
	  
	  /* Pick a number */

	  if (num_vict > 0)
	    vict_chose = nr (1, num_vict);
	  num_vict = 0;
	  
	  /* Find it in the room. */
	  
	  if (IS_ROOM (th))
	    vict = th->cont;
	  else 
	    vict = th->in->cont;
	  for (; vict != NULL; vict = vict->next_cont)
	    { 
	      if (!CAN_FIGHT (vict))
		continue;
	      if (IS_PC (vict) && 
		  (is_enemy(th, vict) == 
		   (spl->target_type == TAR_OFFENSIVE)) &&
		  ++num_vict == vict_chose)
	      break;
	    }
	}
      if (vict)
	cast_spell (th, vict, spl, (vict == th->in ? FALSE :(nr (1,15) == 2 && np () < LEVEL (th))), FALSE, NULL);
    }
  return;
}
