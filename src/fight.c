#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "event.h"
#include "historygen.h"
#ifdef USE_WILDERNESS
#include "wildalife.h"
#endif
/* This updates combat for each thing. */

void
update_combat (THING *th)
{
  THING *other, *other_opp, *attacker;
  int num_opp;
  FIGHT *fgt;
  if (!th || !th->in || (fgt = th->fgt) == NULL)
    {
      remove_fight (th);
      return;
    }

  if (FIGHTING(th) && (FIGHTING(th)->in != th->in))
    stop_fighting (th);
    
  /* Remove any residual following/riding etc... */
  
  if (fgt->following && !fgt->following->in)
    fgt->following = NULL;
  if (fgt->gleader && !fgt->gleader->in)
    fgt->gleader = NULL;
  if (fgt->rider && !fgt->rider->in)
    fgt->rider = NULL;
  if (fgt->mount && !fgt->mount->in)
    fgt->mount = NULL;
  if (fgt->hunt_victim && !fgt->hunt_victim->in)
    stop_hunting (th, FALSE);
  
  /* Update combat timers. */
  
  /* This timer lets you do combat stuff again. */
  if (th->fgt->delay > 0)
    th->fgt->delay--;
  

  /* This timer keeps track of how bashed you are. */
  
  if (th->fgt->bashlag > 10)
    th->fgt->bashlag = 10;
  if (IS_PC (th) && IS_PC1_SET (th, PC_HOLYSPEED))
    th->fgt->bashlag = 0;
  else if (th->fgt->bashlag > 0)
    th->fgt->bashlag--;
  
  if (th->fgt->bashlag < 3 && th->position == POSITION_STUNNED)
    {
      if (!FIGHTING(th))
	th->position = POSITION_STANDING;
      else
	th->position = POSITION_FIGHTING;
      act ("$9@1n scramble@s to @1s feet!$7", th, NULL, NULL, NULL, TO_ALL);
    }
  
  
  
  if (FIGHTING (th))
    {
      if (IS_PC (th))
	th->pc->damage_rounds++;
      if (th->position == POSITION_TACKLED)
	ground_attack (th);
      else
	multi_hit (th, FIGHTING (th));
    }
  else if (th->fgt->bashlag == 0 && 
	   is_hunting (th) &&
	   nr (1,3) == 2 &&
	   (th->fgt->hunting_type != HUNT_KILL ||
	    nr (1,100) <= LEVEL (th) ||
	    nr (1,3) == 1 ||
	    IS_ACT1_SET (th, ACT_FASTHUNT)))
    {
      hunt_thing (th, 0);
    }
  if (FIGHTING (th) && th->in)
    {
      if (!IS_PC (th))
	{
	  for (other = th->in->cont; other && FIGHTING (th); other = other->next_cont)
	    { 
	      if (other == th || !CAN_FIGHT (other) ||
		  FIGHTING (other) || IS_PC (other) ||
		  nr(1,3) != 2 ||
		  other->position == POSITION_SLEEPING ||
		  !can_see (other, FIGHTING (th)))
		continue;
	      
	      if (is_friend (other, th) ||
		  (IS_ACT1_SET (other, ACT_ASSISTALL) &&
		   IS_PC (FIGHTING(th))))
		{
		  do_say (other, "Now you will DIE!");
		      multi_hit (other, FIGHTING (th));
		}
	    }	
	  if (FIGHTING (th) && th->in)
	    combat_ai (th);      
	}
      else
	{ /* Guard and assist */
	  for (other = th->in->cont; other && FIGHTING (th); other = other->next_cont)
	    {
	      VALUE *pet;
	      
	      if ((pet = FNV (other, VAL_PET)) != NULL && 
		  !FIGHTING(other) && FIGHTING (th) && 
		  CAN_FIGHT (other) && nr (1,4) == 2 &&
		  pet->word && *pet->word &&
		  other->position != POSITION_SLEEPING &&
		  !str_cmp (pet->word, NAME(th)) &&
		  can_see (other, FIGHTING(th)))
		{
		  act ("@1p faithful servant assist@s @1m.", th, NULL, other, NULL, TO_ALL);
		  multi_hit (other, FIGHTING(th));
		  continue;
		}
	      
	      if (other == th ||
		  !IS_PC (other) || DIFF_ALIGN (other->align, th->align))
		continue;
	      
	      /* First check assist... */
	      
	      if (!FIGHTING (other) && FIGHTING (th) &&
		  !IS_PC (FIGHTING (th)) &&
		  in_same_group (other, th) &&
		  IS_PC2_SET (other, PC2_ASSIST) &&
		  other->position == POSITION_STANDING)
		{
		  act ("@1n assist@s @3n!", other, NULL, th, NULL, TO_ALL);
		  multi_hit (other, FIGHTING (th));
		}
	      
	      /* Now check for guard (autorescue) */
	      
	      if (nr (1,3) != 2 || other->pc->guarding != th)
		continue;
	      num_opp = 0;
	      for (other_opp = th->in->cont; other_opp !=  NULL; other_opp = other_opp->next_cont)
		{
		  if (FIGHTING (other_opp) == other)
		    num_opp++;
		}
	      /* The more people who fight you, the less effective you
		 are at guarding. */
	      if (nr (1, num_opp) > 1)
		continue;
	      
	      if (check_spell (other, NULL, 562 /* Guard */))
		{
		  act ("@1n heroically step@s in front of @3n!", other, NULL, th, NULL, TO_ALL);
		  for (attacker = th->in->cont; attacker != NULL; attacker = attacker->next_cont)
		    {
		      if (FIGHTING (attacker) == th)
			attacker->fgt->fighting = other;
		    }
		}
	    }
	}
    }
  return;
}

/* This does one round of combat hits from one thing to another. */

void 
multi_hit (THING *th, THING *vict)
{
  int num_hits, i = 0, parts;
  bool in_melee = TRUE, has_ranged = FALSE;
  THING *obj, *objn;
  VALUE *wpn, *rng;
  bool has_killed = FALSE, used_weapon = FALSE;
  
  if (!th || !vict || th->in != vict->in || th == vict ||
     th->position < POSITION_FIGHTING ||
      th->position == POSITION_CASTING ||
      IS_HURT (th, AFF_PARALYZE) ||
      IS_ACT1_SET (th, ACT_DUMMY) ||
      IS_SET (vict->thing_flags, TH_NO_FIGHT))
    return;
  
  if (!CONSID)
    {
      start_fighting (th, vict);
      in_melee = check_if_in_melee (th, vict);
    }
  if (IS_PC (th))
    parts = MAX (1, th->pc->parts[PART_ARMS]);
  else
    parts = 1;

  /* Extra attacks for creatures...up front. Also sets the creature's
     attack string. */
  
  if ((wpn = FNV (th, VAL_WEAPON)) != NULL && !IS_PC (th) &&
      wpn->val[0] > 0 && wpn->val[1] >= wpn->val[0])
    for (i = 0; i < MAX (0, wpn->val[3]) && !has_killed; i++)
      has_killed = one_hit (th, vict, th, SP_ATT_ATTACK);
  
  for (obj = th->cont; obj && !has_killed; obj = obj = objn)
    {
      objn = obj->next_cont;      
      if (!IS_WORN(obj) || has_killed)
	break;
      
      /* Check if we have a weapon, and if we have hit with
	 a weapon already, we must check multiwield to see
	 if we can hit again...other wpn hits from subsequent
	 weapons don't hit as often, but I tried to make it as
	 random as possible so people don't really notice that
	 they get a huge chunk of hits or none at all from multiply
	 wielded weapons. */

      /* Check if this thing has a ranged item. */
      
      if (!has_ranged && (rng = FNV (obj, VAL_RANGED)) != NULL)
	has_ranged = TRUE;
      
      if (obj->wear_loc == ITEM_WEAR_WIELD &&
	  (wpn = FNV (obj, VAL_WEAPON)) != NULL  &&
	  (!used_weapon || check_spell (th, NULL, 570 /* Multi Wield */)))
	{
	  num_hits = 0;
	  
	  
	  if (!CONSID && !in_melee && !IS_OBJ_SET (obj, OBJ_TWO_HANDED))
	    {
	      stt ("\x1b[1;32m<->\x1b[1;35mYou aren't within melee range!\x1b[1;32m<->\x1b[0;37m\n\r", th);
	      continue;
	    }
	  
	  
	  if (!used_weapon || RAVAGE || nr (1,2) != 1)
	    num_hits++;
	  if (nr (3, 15) < implant_rank (th, PART_ARMS))
	    num_hits++;
	  if (!IS_HURT (th, AFF_SLOW)) /* Slow makes you hit less. */
	    {
	      
	      /* Add in extra hits for haste and skills. */
	      if (IS_AFF (th, AFF_HASTE) &&
		  nr (1,3) != 1 &&
		  (!used_weapon || RAVAGE || nr (1,2) != 2))
		num_hits++;
	      if (nr (1,4) != 2 &&
		  check_spell (th, NULL,  571 /* 2nd attack */) &&
		  (!used_weapon || RAVAGE || nr (1,2) != 1))
		num_hits++;
	      if (nr (1,3) != 2 &&
		  check_spell (th, NULL,  572 /* 3rd attack */) && 
		  (!used_weapon || RAVAGE || nr (1,2) != 1))
		num_hits++;
	    }
	  /* Now do the actual hits... */
  	  num_hits += nr (0, wpn->val[3]);

	  /* Reduce mob hits somewhat. */
	  if (!IS_PC (th))
	    {
	      if (used_weapon)
		num_hits /= 2;
	      if (num_hits > 3)
		num_hits = nr (1,num_hits);
	    }

	  for (i = 0; i < num_hits && !has_killed; i++)
	    {
	      has_killed = one_hit (th, vict, obj, SP_ATT_NONE);
	      
	      /* if weapon breaks...we hit with hands... */
	      
	      if (obj  && obj->in != th)
		obj = NULL;
	      if (th->in != vict->in)
		break;
	    }
	  used_weapon = TRUE;
	}
    }
  
  /* Possibly allow for "monks" or something to kick ass with unarmed combat,
     like multiple hits and destroying mob resistances... */
  
  if (!used_weapon && !has_killed)
    {
     
      if (!CONSID && !in_melee)
	stt ("\x1b[1;32m<->\x1b[1;35m You aren't within melee range!\x1b[1;32m<->\x1b[0;37m\n\r", th);
      if (!IS_PC (th))
	{
	  if (has_ranged && nr (1,2) == 1)
	    {
	      do_flee (th, "");
	      return;
	    }
	  else
	    num_hits = MID (1, nr (1, LEVEL (th)/20), 8);
	}
      else
	num_hits = 1;
      if (nr (3, 15) < implant_rank (th, PART_ARMS))
	num_hits++;
      for (i = 0; i < num_hits && !has_killed; i++)
	has_killed = one_hit (th, vict, th, SP_ATT_NONE); 
    }
  if (!wpn && !IS_PC (th) && !has_killed && nr (1,65) == 42)
    {
      int mob_flags = flagbits (th->flags, FLAG_MOB);
      FLAG *flg = NULL;
      if (IS_SET (mob_flags, MOB_UNDEAD | MOB_DEMON) && nr (1,15) == 3)
	{
	  flg = new_flag();
	  flg->type = FLAG_HURT;
	  flg->val = AFF_DISEASE;
	  flg->timer = nr (3,6);
	  flg->from = 0;
	  aff_update (flg, vict);
	}
    }
  return;
}

/* This returns TRUE if an attack was stopped or FALSE if it was not. */

bool
check_defense (THING *th, THING *vict)
{
  int attackers = 0, shields = 0, weapons = 0, weight_mult = 0, chance;
  THING *obj, *att;
  VALUE *arm, *wpn, *caster;
  THING *weapon = NULL;
  char attackname[STD_LEN];
  if (!th || !vict || !th->in || !vict->in)
    return TRUE;
  
  if (nr (1,2) == 1)
    sprintf (attackname, "hit");
  else
    sprintf (attackname, "attack");

  /* Weight mult is used because the more encumbered you are, the less
     able you are to dodge/parry. */
  
  weight_mult = th->carry_weight / (WGT_MULT * get_stat (th, STAT_STR));
  if (weight_mult < 1)
    weight_mult = 1;
  
  /* Being in a groundfight makes you harder to hit. */

  if (vict->position == POSITION_TACKLED && nr (1,3) != 2)
    {
      switch (nr(0, 2))
	{
	case 0:
	  act ("@1n can't @t @3n in the tangle of bodies on the ground.!", th, NULL, vict, attackname, TO_ALL + TO_SPAM);
	  break;
	case 1:
	  act ("@1n can't @t @3n while @3h's fighting on the ground!", th, NULL, vict, attackname, TO_ALL + TO_SPAM);
	  break;
	case 2:
	default:
	  act ("@1n can't find a clear line of attack to @t @3n!", th, NULL, vict, attackname, TO_ALL + TO_SPAM);
	  break;
	}
      return TRUE;
    }
  
  for (obj = vict->cont; obj; obj = obj->next_cont)
    {
      if (!IS_WORN(obj) ||
	  obj->wear_loc > ITEM_WEAR_WIELD)
	break;
      else if (obj->wear_loc == ITEM_WEAR_SHIELD &&
	  (arm = FNV (obj, VAL_ARMOR)) != NULL)
	shields++;
      if (obj->wear_loc == ITEM_WEAR_WIELD &&
	  (wpn = FNV (obj, VAL_WEAPON)) != NULL)
	{
	  if (!weapon)
	    weapon = obj;	  
	  weapons++;
	}
    }
  for (att = vict->in->cont; att; att = att->next_cont)
    {
      if (FIGHTING (att) == vict)
	attackers++;
    }
  if (attackers < 1)
    attackers = 1;

  /* Parry */
  
  if (weapons > 0 && vict->position >= POSITION_FIGHTING)
    {
      chance = (IS_PC (vict) ? 
		(vict->pc->prac[576] > 29 ? 
		 (check_spell (vict, NULL, 576) * 100 +
		  ((50 - vict->pc->off_def) * 1) + 
		  check_spell (vict, NULL,  585) * 80 +
		  guild_rank (vict, GUILD_WARRIOR) * 7 +
		  implant_rank (vict, PART_ARMS) * 3 +
		  vict->pc->aff[FLAG_AFF_DEFENSE]) : 0) :
		MIN (175, LEVEL (vict)));
      
      chance = (chance * MIN(2, weapons))/attackers;
      if (nr (1,1500) < chance)
	{
	  if (nr (1,3) == 2)
	    {
	      act ("$F@1n move@s quickly, and deflect@s @3p @t!$7", vict, NULL, th, attackname , TO_ALL + TO_SPAM);
	      return TRUE;
	    }
	  else if (nr (1,2) == 1)
	    {
	      act ("$F@1n parry@s @3p @t!$7", vict, NULL, th, attackname , TO_ALL + TO_SPAM);
	      return TRUE;
	    }
	  act ("$F@1n slam@s @3p @t aside!$7", vict, NULL, th,  attackname, TO_ALL + TO_SPAM);
	  if (weapon && nr (1,15) == 3 && 
	      check_spell (vict, NULL, 585 /* Riposte */))
	    {
	      act ("$E@1n get@s in a quick return strike!$7", vict, NULL, NULL, NULL, TO_ALL);
	      one_hit (vict, th, weapon, 0);	      
	    }
	  return TRUE;
	}
    }

  /* Dodge. */
  
  if ((vict->position >= POSITION_FIGHTING) ||
      vict->position == POSITION_STUNNED)
    {
      chance = (IS_PC (vict) ? 
		(vict->pc->prac[577] > 29 ?
		 (check_spell (vict, NULL, 577) * 100 +
		  ((50 - vict->pc->off_def)) + 
		  check_spell (vict, NULL, 583) * 80  + 
		  guild_rank (vict, GUILD_THIEF) * 7 +
		  implant_rank (vict, PART_LEGS) * 3 +
		  vict->pc->aff[FLAG_AFF_DEFENSE]) : 0) : 
		MIN (75, LEVEL (vict)));
      
      chance /= attackers*weight_mult;
      
      if (nr (1, 1100) < chance)
	{
	  if (vict->position == POSITION_STUNNED)
	    {
	      act ("$A@1n roll@s away from @3p @t!$7", vict, NULL, th, attackname, TO_ALL + TO_SPAM);
	      return TRUE;
	    }
	  else
	    {
	      switch (nr (0,2))
		{
		case 0:
		  act ("$A@1n dodge@s @3p @t!$7", vict, NULL, th, attackname , TO_ALL + TO_SPAM);
		  break;
		case 1:
		  act ("$A@1n slip@s away from @3p @t!$7", vict, NULL, th, attackname , TO_ALL + TO_SPAM);
		  break;
		case 2:
		default:
		  act ("$A@1n move@s quickly aside, avoiding @3p @t!$7", vict, NULL, th,  attackname, TO_ALL + TO_SPAM);
		  break;		  
		}
	      return TRUE;
	    }
	}
    }

  /* Shield block. */

  if (shields > 0 && vict->position >= POSITION_FIGHTING)
    {
      
      chance =  (IS_PC (vict) ? 
		 (vict->pc->prac[578] > 29 ?
		  (check_spell (vict, NULL, 578) * 100 +
		   check_spell (vict, NULL, 586) * 80 +
		   ((50 - vict->pc->off_def) *1) + 
		   guild_rank (vict, GUILD_KNIGHT) +
		   implant_rank (vict, PART_BODY) +
		   vict->pc->aff[FLAG_AFF_DEFENSE]) : 0) :
		 MIN (175, LEVEL (vict)));
      
      chance = chance * MIN (2,shields)/attackers;
      
      if (nr (1,1000) < chance)
	{
	  if (nr (1,3) == 2)
	    {
	      act ("$B@1n block@s @3p @t with @1s shield!$7", vict, NULL, th, attackname , TO_ALL + TO_SPAM);
	      return TRUE;
	    }
	  else if (nr (1,2) == 1)
	    {
	      act ("$B@1n stop@s @3p @t with @1s shield!$7", vict, NULL, th, attackname , TO_ALL + TO_SPAM);
	      return TRUE;
	    }
	  act ("$B@1p shield catches @3p @t!$7", vict, NULL, th,  attackname, TO_ALL + TO_SPAM);
	  if (nr (1,15) == 3 && check_spell (vict, NULL, 586 /* Shield bash */))
	    {
	      act ("$E@1n bash@s @3n with @1s shield!$7", vict, NULL, th, NULL, TO_ALL);
	      damage (vict, th, nr (10, get_stat (vict, STAT_STR)), (char *) "shieldbash");
	    }
	  return TRUE;
	}
    }

  /* Blink. */

  if (vict->position >= POSITION_FIGHTING)
    {
      if (IS_PC (vict))
	{
	  if (vict->pc->prac[591 /* Blink */] < 30)
	    chance = 0;
	  else
	    chance = (check_spell (vict, NULL, 591) * 100 +
		      check_spell (vict, NULL, 592) * 100 +
		      ((50 - vict->pc->off_def) *1) + 
		      guild_rank (vict, GUILD_WIZARD) * 7 +
		      implant_rank (vict, PART_HEAD) * 3 +
		      vict->pc->aff[FLAG_AFF_SPELL_RESIST]);
	}
      else if ((caster = FNV (vict, VAL_CAST)) != NULL)
	chance = MIN (175, LEVEL (vict));
      else
	chance = 0;
      
      if (nr (1,1200) < chance)
	{
	  if (IS_PC (th) && th->pc->mana > 0 && nr(1,4) == 2)
	    th->pc->mana--;
	  if (nr (1,3) == 2)
	    {
	      act ("$E@1n blink@s away from @3p @t!$7", vict, NULL, th,   attackname, TO_ALL + TO_SPAM);
	      return TRUE;
	    }
	  else if (nr (1,2) == 1)
	    {
	      act ("$E@1n wink@s out of existence momentarily, avoiding @3p @t!$7", vict, NULL, th,   attackname, TO_ALL + TO_SPAM);
	      return TRUE;
	    }
	  act ("$E@1n blur@s and escape@s from @3p @t!$7", vict, NULL, th,   attackname, TO_ALL + TO_SPAM);
	  return TRUE;
	}
    }
  return FALSE;
}

/* This does one combat hit from a thing to a thing using a weapon (or not).
   The special hits are for things like backstabs that do more dam. 
   They run through this so we don't have to make special checks for
   them outside of this regular attack checking. */


bool
one_hit (THING *th, THING *vict, THING *weapon, int special)
{
  int dam =0,  /* Damage done. */
    hit,    /* Hitroll..chance to hit. */
    mobflags, /* Mobile defense flags. */
    objflags, /* Object offensive flags to counter mobile defenses. */
    basedam,  /* Base damage before massaging it. */
    hit_pos,
    armor, removed, resist_bits, resistance = 0, num_worn_that_loc = 0,
    count = 0, num_chosen = 0;
  VALUE *wpn = NULL;
  THING *obj = NULL;
  bool killed = FALSE;
  char *damage_word = nonstr;
  char buf[STD_LEN];
  int wpn_prac_percent = 100; /* Weapon practiced percent determines
				 how hard you hit based on how well you
				 know your weapon. */

  if (!th || !vict || !th->in || !vict->in)
    {
      stop_fighting (th);
      stop_fighting (vict);
      return FALSE;
    }

   /* Holypeace stops combat. */
  if (IS_PC (vict) && LEVEL(vict) >= BLD_LEVEL &&
      IS_PC1_SET (vict, PC_HOLYPEACE))
    {
      stt ("You can't attack them!\n\r",  th);
      stop_fighting (th);
      stop_fighting (vict);
      return FALSE;
    }
  
  
  
  if (weapon)
    wpn = FNV (weapon, VAL_WEAPON);
  
  /* Check if you hit or not. */

  if (special == SP_ATT_NONE)
    {
      hit = (5* get_stat (th, STAT_DEX) - 3 * get_stat (vict, STAT_DEX)) +
	(LEVEL (th)/3 - LEVEL (vict) / 4) +
	5*implant_rank (th, PART_ARMS) -
        (IS_PC (vict) ? (25 - vict->pc->off_def/2) : 0) +
	(IS_PC (th) ? th->pc->prac[575 /* Accuracy */]/6 +
	 th->pc->aff[FLAG_AFF_HIT] + (th->pc->off_def/2 - 25) : 0)+
	(vict->position < POSITION_FIGHTING ? 30 : 0);
      
      if (wpn && wpn->val[2] >= WPN_DAM_SLASH && wpn->val[2] < WPN_DAM_MAX)
	{
	  if (IS_PC (th))
	    {
	      /* Weapon skills are 579-582 */
	      check_spell (th, NULL, 579 + wpn->val[2]);
	      wpn_prac_percent = MAX (40, th->pc->prac[579 + wpn->val[2]]*5/4);
	      
	      if (check_spell (th, NULL, 620 + wpn->val[2]))
		wpn_prac_percent += th->pc->prac[620 + wpn->val[2]]/3;
	      
	    }
	  else /* Yes mobs do kick more ass at higher levels than players.
		  They do more damage with weapons. */
	    wpn_prac_percent = 50 + LEVEL(th)/2;
	  
	  hit += wpn_prac_percent/3;
	}

      /* You always get a 50/50 chance of hitting, just to keep it from being
	 dozens of rounds of misses. */
      
      if (nr (1, 150) > hit && 
	  (nr(1,3) == 2 || !IS_PC (th))
	  && !RAVAGE)
	{
	  act ("@1n miss@s @3n.", th, NULL, vict, NULL, TO_ALL + TO_SPAM);
	  return FALSE;
	}
      
      
    }

  /* If this is a player, or if the victim is a player, then check
     to see if resistances stop the damage. I guess it's good to
     let players have these resistances so that you can make spells
     that give you "dragonform" or some such thing and actually
     confer the dragon damage resistances, too. */
  
  if (IS_PC (th) || IS_PC (vict))
    {
      
      if (weapon && wpn)
	objflags = flagbits (weapon->flags, FLAG_OBJ1);
      else 
	objflags = 0;
      
      mobflags = flagbits (vict->flags, FLAG_MOB);
      
      if (IS_SET (mobflags, MOB_NOHIT))
	{
	  act ("@1p attack goes through @3n as if @3h were not there!", th, NULL, vict, NULL, TO_ALL);
	  return FALSE;
	}
      
  
      /* This checks if a mob has a certain defense like being a demon
	 etc...and if it does and one of those bits matches ~objflags, then
	 the mob has a resistance the wpn cannot overcome, so the attack
	 fails. */
      
      
      if (((mobflags % (MOB_AIR + 1)) & ~objflags) & !CONSID)
	{
	  int flags_missing = ((mobflags % (MOB_AIR + 1)) & ~objflags), i;
	  int num_enchants_done = 0;
	  act ("$C@1n shrug@s off @3p pitiful attack!$7", vict, NULL, th, NULL, TO_ALL);
	  sprintf (buf, "\x1b[1;31mYou need ");
	  for (i = 0; obj1_flags[i].flagval != 0 &&
		 obj1_flags[i].flagval <= OBJ_EARTH; i++)
	    {
	      if (IS_SET (flags_missing, obj1_flags[i].flagval))
		{
		  if (num_enchants_done == 0)
		    {
		      strcat (buf, a_an (obj1_flags[i].name));
		      strcat (buf, "\x1b[1;36m");
		    }		  
		  strcat (buf, obj1_flags[i].app);
		  strcat (buf, " ");
		  num_enchants_done++;
		}
	    }
	  strcat (buf, "\x1b[1;31menchanted weapon to hit this creature!\x1b[0;37m\n\r");
	  if (num_enchants_done > 0)
	    stt (buf, th);
	  
	  return FALSE;
	}
      
      /* Try to stop the attack */
      
    }
  
  if (special == SP_ATT_NONE && check_defense (th, vict))
    return FALSE;
  
  hit_pos = nr (0, PARTS_MAX-1);

  /* Now calc base damage */
  
  if (wpn)
    {
      dam = (nr (wpn->val[0], wpn->val[1]) + nr (wpn->val[0], wpn->val[1]))/2;
      if (wpn->val[2] == WPN_DAM_CONCUSSION)
	dam += nr (0, guild_rank (th, GUILD_TINKER));
      
    }
  else if (IS_PC (th))      
    dam = nr (1, 1 + LEVEL (th)/ 10);
  
  /* If you are a PC, you can't do more than double your basedam damage */
  
  basedam = dam;
  if (!IS_PC (th))
    {
      if (special != SP_ATT_ATTACK)
	{	  
	  dam += nr (LEVEL(th)/6, LEVEL(th)/3);
	  dam -= 5;
	  if (dam < 1)
	    dam = 1;
	}
    }
  else
    {
      dam += nr (0, get_damroll (th)/2);
      /* off_def makes you do more damage or less damage by upping or
	 lowering your damroll. It is also calced within get_damroll () */
      /* off_def makes damage go from 84 pct to 116 pct of damage. */
      dam = dam * (th->pc->off_def/3 + 84) / 100;
    } 
  
  /* Add enhanced damage */
  
  if (check_spell (th, NULL, 573 /* Enhanced damage */))
    {
      dam += nr (0, dam/3);
      if (wpn && wpn->val[2] == WPN_DAM_CONCUSSION)
	dam += nr (0, dam/8);
    }
  
  /* Damroll/enhanced damage do not override base wpn damage by more than
     a factor of 2. Sucks, don't it? :P */
  
  if (IS_PC (th))
    dam = MIN (2*basedam, dam);
  
  dam += nr (0, implant_rank (th, PART_ARMS));
  

  /* Ravage increases damage. */
  
  if (RAVAGE)
    dam = dam * 5/4;
  
  /* Weapon practice percent also increases or decreases damage. */
  
  dam = (dam *wpn_prac_percent)/100;
  
  if (special == SP_ATT_BACKSTAB)
    {
      dam += (guild_rank (th, GUILD_THIEF) + 1)/2;
      dam = dam * 
	(((2 + LEVEL (th)/10 + guild_rank(th, GUILD_THIEF))/2)
	 * (100 + (IS_PC (th) ? th->pc->aff[FLAG_AFF_THIEF_ATTACK] : 0))/100);
      if (FIGHTING (vict))
	dam /= 2;
      if (IS_HURT (vict, AFF_SLEEP))
	dam *= 3;
    }

  /* Now add possibility of a critical hit. 1/350 chance of this. :P */
   
  if ((IS_PC (th) || LEVEL (th) > 150) &&
      check_spell (th, NULL, 584 /* Critical hit */) &&
      nr (1,350) == 231) 
    {
      act ("$9C$FR$9I$FT$9I$FC$9A$FL $9H$FI$9T$F!$9!$F!$7", th, NULL, NULL, NULL, TO_ALL);
      dam *= 17;
    }
  
  
  /* Remove armor...point per point basis, not percents. */

  armor = vict->armor/ARMOR_DIVISOR + 
    (IS_PC (vict) ? vict->pc->armor[hit_pos] : 0);
  
  removed = nr (0, armor);
  /* Armor penetration removes some of the armor involved... 
     only for slash/pierce wpns */
  
  if (wpn && (wpn->val[2] == WPN_DAM_PIERCE ||
	      wpn->val[2] == WPN_DAM_SLASH) 
      && check_spell (th, NULL, 574 /* Armor Pen */))
    removed -= nr(0, removed/3);
  
  dam -= MIN(dam, removed);

  
  /* Now check for PERCENTAGE!!! resistances... */
  
  if (IS_PC (vict))
    resistance = vict->pc->aff[FLAG_AFF_DAM_RESIST];
  resistance += nr (0, 5 * implant_rank (vict, PART_BODY));
  
  if ((resist_bits = flagbits (vict->flags, FLAG_AFF)) != 0)
    {
      if (IS_SET (resist_bits, AFF_SANCT))
	resistance += nr (25, 42);
      if (IS_SET (resist_bits, AFF_PROTECT))
	resistance += nr (45, 68);
      if (DIFF_ALIGN (th->align, vict->align) &&
	  IS_SET (resist_bits, AFF_PROT_ALIGN))
	resistance += nr (8, 27);
      if (IS_SET (resist_bits, AFF_AIR))
	resistance += nr (2,7);
      if (IS_SET (resist_bits, AFF_FIRE))
	resistance += nr (2,7);
      if (IS_SET (resist_bits, AFF_EARTH))
	resistance += nr (2,7);
      if (IS_SET (resist_bits, AFF_WATER))
	resistance += nr (2,7);
    }
  
  /* Resistance is capped at 90 pct, but all values above 75 pct
     are halved and a random number from 0 to res-75 is added to 75..
     making it very hard to get above 75pct resistance. */

  if (resistance > 75)
    {
      resistance -= 75;
      resistance = MIN (90, nr (0, resistance/2) + 75);
    }  
  dam -= (dam * resistance)/100;
  
  
  if (dam < 1)
    return FALSE;
  
  if (!CONSID)
    if (nr (1,10) == 4)
      th->mv--;

  /* Now do special attacks on the attacker from elemental shields. */
  
  if (IS_SET (resist_bits, AFF_AIR | AFF_FIRE | AFF_EARTH | AFF_WATER))
    {
      if (IS_SET (resist_bits, AFF_AIR) && nr (1,8) == 2)
	cast_spell (vict, th, find_spell ("lightning bolt", 0), FALSE, FALSE, NULL);
      if (th->in == vict->in && IS_SET (resist_bits, AFF_EARTH) && nr(1,8) == 3)
	cast_spell (vict, th, find_spell ("spike", 0), FALSE, FALSE, NULL);
      if (th->in == vict->in && IS_SET (resist_bits, AFF_EARTH) && nr (1,8) == 4)
	cast_spell (vict, th, find_spell ("flame ray", 0), FALSE, FALSE, NULL);
      if (th->in == vict->in && IS_SET (resist_bits, AFF_EARTH) && nr (1,8) == 5)
	cast_spell (vict, th, find_spell ("cold ray", 0), FALSE, FALSE, NULL);
      if (th->in != vict->in)
	return FALSE;      
    }
     
  
  /* Add mob special attack names here? */
  
  mobflags = flagbits (th->flags, FLAG_MOB);

  /* If using a weapon, find the damage name used. */

  if (wpn)
    {
      if (wpn->word && *wpn->word)
	{
	  char *arg;
	  char arg1[STD_LEN];
	  int num_words = 0, choice, count = 0;
	  arg = wpn->word;
	  while (*arg)
	    {
	      arg = f_word (arg, arg1);
	      num_words++;
	    }
	  choice = nr (1, num_words);
	  arg = wpn->word;
	  while (count < choice)
	    {
	      arg = f_word (arg, arg1);
	      count++;
	    }
	  damage_word = arg1;
	}
      else if (wpn->val[2] >= WPN_DAM_SLASH &&
	       wpn->val[2] < WPN_DAM_MAX)
	damage_word = (char *) weapon_damage_names[wpn->val[2]];
      else
	damage_word = "hit";
    }
  else /* Otherwise use a name based on the type of mob/pc hitting. */
    {
      if (mobflags)
	{
	  if (IS_SET (mobflags, MOB_GHOST))
	    damage_word = "touch";
	  else if (IS_SET (mobflags, MOB_DRAGON))
	    damage_word = "bite";
	  else if (IS_SET (mobflags, MOB_FIRE))
	    damage_word = "burn";
	  else
	    damage_word = "claw";
	}
      else
	damage_word = "punch";
    }
  
  /* Do the damage. killed tells if the victim was killed or not. */

  killed = damage (th, vict, dam, damage_word);
  
  
  if (LEVEL (th) > 12 && 
      th->in && IS_ROOM(th->in) && IS_SET (th->in->hp, BLOOD_POOL) &&
      nr(1,80) == 3 && !IS_AFF (th, AFF_FLYING) &&
      !IS_ROOM_SET (th->in, ROOM_NOBLOOD))
    {
      act ("@1n slip@s on a pool of blood!", th, NULL, NULL, NULL, TO_ALL);
      if (damage (th->in, th, nr(1,20), "smack"))
        return TRUE;
    }
      
  
  if (CONSID)
    return FALSE;
      
  /* Weapons cast spells/poisons. */

  if (weapon && wpn)
    {
      SPELL *poison, *spl;
      VALUE *cast;
      /* Poisons */
      if (!killed && wpn->val[4] != 0 &&
	  (poison = find_spell (NULL, wpn->val[5])) != NULL &&
	  poison->spell_type == SPELLT_POISON &&
	  (IS_PC (th) || nr (1,500) < LEVEL (th)))
	{
	  cast_spell (weapon, vict, poison, FALSE, FALSE, NULL);
	  if (FIGHTING (th) != vict)
	    killed = TRUE;
	  if (IS_PC (th) && wpn->val[4] > 0 && --wpn->val[4] == 0)
	    wpn->val[5] = 0;
	}
      
      if (!killed && (cast = FNV (weapon, VAL_CAST)) != NULL &&
	  !str_cmp (cast->word, "rapid"))
	{
	  int i, num_choices = 0, num_chose = 0, choice = 0;
	  for (i = 0; i < NUM_VALS; i++)
	    {
	      if ((spl = find_spell (NULL, cast->val[i])) != NULL &&
		  spl->spell_type == SPELLT_SPELL &&
		  spl->target_type == TAR_OFFENSIVE)
		num_choices++;
	    }
	  if (num_choices > 0)
	    {
	      num_chose = nr (1, num_choices);
	      for (i = 0; i < NUM_VALS; i++)
		{
		  if ((spl = find_spell (NULL, cast->val[i])) != NULL &&
		      spl->spell_type == SPELLT_SPELL &&
		      spl->target_type == TAR_OFFENSIVE &&
		      ++choice == num_chose)
		    break;
		}
	      if (i < NUM_VALS)
		{
		  cast_spell (weapon, vict, spl, FALSE, FALSE, NULL);
		}
	      if (FIGHTING (th)  != vict)
		killed = TRUE;
	    }
	}
      
      if (weapon != th && nr (1,35) == 2 && dam > MIN (weapon->level/5, 20) &&
	  weapon->max_hp > 0)
	{
	  weapon->hp--;
	  if (weapon->hp < 1)
	    {
	      act ("ACK! Your @3n just broke!", th, NULL, weapon, NULL, TO_CHAR);
	      free_thing (weapon);
	    }
	  else if (nr (1,5) == 14)
	    {
	      act ("@3n is dented.", th, NULL, weapon, NULL,  TO_CHAR);
	    }
	}
    }

  
  /* Armor/weapons get damaged. */

  if (nr (1,65) == 23)
    {
      for (obj = vict->cont; obj; obj = obj->next_cont)
	{
	  if (!IS_WORN(obj))
	    break;
	  if (wear_data[obj->wear_loc].part_of_thing == hit_pos &&
	      obj->max_hp > 0)
	    num_worn_that_loc++;
	}
      if (num_worn_that_loc > 0)
	{
	  num_chosen = nr (1, num_worn_that_loc);
	  for (obj = vict->cont; obj; obj = obj->next_cont)
	    {
	      if (!IS_WORN(obj))
		break;
	      if (wear_data[obj->wear_loc].part_of_thing == hit_pos &&
		  ++count == num_chosen && obj->max_hp > 0)
		{
		  if (--obj->hp < 1)
		    {
		      act ("Your @3n just fell apart!", th, NULL, obj, NULL, TO_CHAR);
		      free_thing (obj);		      
		    }
		  else if (nr (1,20) == 2)
		    {
		      act ("@3n gets dented.", th, NULL, obj, NULL, TO_CHAR);
		    }
		  break;
		}
	    }
	}
    }
  return killed;
}

/* I added a lot of junk in here so that attacks are modified by a lot
   of affects and by permadeath etc... Feel free to make this
   simpler if this is too much :P */

  

bool
damage (THING *th, THING *vict, int dam, char *word)
{
  VALUE *power, *soc = NULL, *build;
  DAMAGE *damg = NULL;
  THING *obj;
  RACE *align;
  SOCIETY *society, *tsoc, *vsoc;
  int reduced = 0;
  int flags;
  int th_hurt, vict_hurt; /* Hurt flags for th and vict. */
  
  if (!th || !vict || !th->in || !vict->in)
    return TRUE;
  
  /* Check to see if combat can occur here or not. */
  
  if (IS_ROOM_SET (vict->in, ROOM_PEACEFUL))
    {
      stop_fighting (vict);
      if (FIGHTING (th) == vict)
	stop_fighting (th);
      act ("A wave of peaceful energy stop@s @1n from attacking @3n.", th, NULL, vict, NULL, TO_ALL);
      return FALSE;
    }

  /* Don't players damage other things at a range if the targets can't
     move. */

  if (IS_PC (th) && th->in != vict->in &&
      (IS_ACT1_SET (vict, ACT_SENTINEL)
       || !CAN_MOVE (vict)))
    return FALSE;

  if (!CONSID)
    start_fighting (th, vict);
  


  /* Deal with relics and permadeath for pc's */
  
  if (IS_PC (th))
    {
      flags = flagbits (th->flags, FLAG_PC1);
      if (IS_SET (flags, PC_PERMADEATH))
	dam *= 2;
      if (IS_SET (flags, PC_ASCENDED))
	dam *= 2;
      if ((align = FALIGN(th)) != NULL)
	dam += dam*(align->relic_amt+align->power_bonus)/100;
    }
  /* Raiders do more damage. */
  else if ((soc = FNV (th, VAL_SOCIETY)) != NULL)
    {
      if (IS_SET (soc->val[2], BATTLE_CASTES) &&
	  soc->val[4] > 0)
	dam += dam/3;
      
      /* When defending a homeland, you get an attack bonus. */
      if ((build = FNV (th->in, VAL_BUILD)) != NULL &&
	  (society = find_society_num (build->val[0])) != NULL &&
	  (((th->align > 0 && !DIFF_ALIGN (th->align, society->align)) ||
	    ((soc = FNV (th, VAL_SOCIETY)) != NULL &&
	     soc->val[0] == build->val[0]))))
	dam += (dam*2*build->val[1])/100;
      
      if ((tsoc = find_society_num (soc->val[0])) != NULL)
	{
	  dam += dam*tsoc->morale/(2*MAX_MORALE);
	  
	  if (IS_SET (tsoc->society_flags, SOCIETY_OVERLORD))
	    dam += dam/5;
	}
    }
  if (IS_PC (vict))
    { 
      flags = flagbits (vict->flags, FLAG_PC1);
      if (IS_SET (flags, PC_PERMADEATH))
	dam /= 2;
      if (IS_SET (flags, PC_ASCENDED))
	dam /= 2;
      if ((align = FALIGN(vict)) != NULL)
	dam -= dam*(align->relic_amt+align->power_bonus)/100;
    }
  
  /* Defensive bonus for built up cities. */
  
  if ((soc = FNV (vict, VAL_SOCIETY)) != NULL)
    {      
      if ((build = FNV (vict->in, VAL_BUILD)) != NULL &&
	  (society = find_society_num (build->val[0])) != NULL &&
	  (((vict->align > 0 && !DIFF_ALIGN (vict->align, society->align)) ||
	    (soc->val[0] == build->val[0]))))
	dam -= (dam*3*build->val[1])/100;
      if ((vsoc = find_society_num (soc->val[0])) != NULL)
	{
	  dam -= dam*vsoc->morale/(2*MAX_MORALE);
	  if (IS_SET (vsoc->society_flags, SOCIETY_OVERLORD))
	    dam -= dam/6;
	}
    }  
  
  
  /* Get the hurt bits for th and vict. */
  
  th_hurt = flagbits (th->flags, FLAG_HURT);
  vict_hurt = flagbits (vict->flags, FLAG_HURT);

  /* Raw and outline on the victim increase damage. */
  
  if (IS_SET (vict_hurt, AFF_RAW))
    dam *=2;
  
  if (IS_SET (vict_hurt, AFF_OUTLINE))
    dam = (dam * (100 + LEVEL (th))/100);
  
  /* Weakness causes all attacks to do less damage. */
  
  if (IS_SET (th_hurt, AFF_WEAK))
    dam /= 2;
  
  /* Lower hps = less dam done. */
  
  if (!IS_SET (th->thing_flags, TH_IS_ROOM | TH_IS_AREA | TH_NO_FIGHT) && 
      th->max_hp > 0)
    dam -= dam * ((th->max_hp - th->hp) * 2/3)/th->max_hp;
  
  
  /* A society member inside of a built up area absorbs damage. */
  
  dam -= society_defensive_damage_reduction (vict, dam);
  
  if (CONSID)
    {
      cons_hp -= dam;
      return FALSE;
    }
  
  
  /* Powershields absorb damage. */
  
  for (obj = vict->cont; obj; obj = obj->next_cont)
    {
      if (!IS_WORN(obj) || dam < 1)
	break;
      if ((power = FNV (obj, VAL_POWERSHIELD)) != NULL)
	{
	  reduced = MIN( MIN (power->val[1], power->val[2]/10), dam);
	  if (reduced > 0)
	    {
	      dam -= reduced;
	      power->val[1] -= reduced;
	    }
	}
    }
  
  if (dam < 1 && reduced > 0)
    {
      act ("$8@1p powershields absorb @3p attack!$7", vict, NULL, th, NULL, TO_ALL);    
      return FALSE;
    }
  
  /* Combat stops any timed commands the player is attempting. */
  
  if (dam > (5 + LEVEL (vict)/10))
    {
      if (vict->position > POSITION_MEDITATING)
	{
	  stt ("\x1b[0;31mOuch! You just lost your concentration!\x1b[0;37m\n\r", vict);
	  vict->position = POSITION_FIGHTING;
	  remove_typed_events (vict, EVENT_COMMAND);
	}
    }
  
  /* Add this to the attacker's total...used for statistical purposes. */
  
  if (IS_PC (th))
    th->pc->damage_total += dam;
  
  /* Remove damage from the victim. */
  
  vict->hp -= dam;
  
  /* Update for the GUI client. */
  
  update_kde (vict, KDE_UPDATE_HMV);
  
  /* Send the damage message to everyone. */
  
  for (damg = dam_list; damg; damg = damg->next)
    {
      if (dam >= damg->low && dam <= damg->high)
	break;
    }
  
  if (!damg)
    damg = dam_list;
  
  current_damage_amount = dam;
  act (damg->message, th, NULL, vict, word, TO_COMBAT);
  current_damage_amount = 0;
  if (IS_PC (th) && (!IS_PC (vict) || DIFF_ALIGN (th->align, vict->align)))
    {
      int added = MIN (10 * LEVEL (th), dam *dam/2 + dam*3)*
	find_remort_bonus_multiplier (th);
      add_exp (th, added);
      th->pc->fight_exp += added;
    }
  
  if (vict->in && IS_ROOM (vict->in) &&
      vict->hp < vict->max_hp/2 && nr (1,50) == 2)
    {
      act ("$9A pool of blood forms here.", vict, NULL, NULL, NULL, TO_ALL);
      vict->in->hp |= BLOOD_POOL;
    }
  
  if (vict->hp <= 0)
    {
      get_killed (vict, th);
      return TRUE;
    }
  
  if (IS_PC (vict))
    {
      if (!vict->fd || vict->hp < vict->pc->wimpy)
	do_flee (vict, "");
    }
  else if (vict->hp < vict->max_hp*2/5 && 
	   (IS_ACT1_SET (vict, ACT_WIMPY) ||
	    (soc &&
	     IS_SET (soc->val[2], CASTE_WIZARD | CASTE_HEALER))))
    do_flee (vict, "");
  
  return FALSE;
}




void 
stop_fighting (THING *th)
{
  THING *thg;
  if (!th || !th->fgt)
    return;
  if (th->in)
    {
      for (thg = th->in->cont; thg; thg = thg->next_cont)
	{
	  if (thg->fgt)
	    {
              if (thg->fgt->fighting == th ||
		  (thg->fgt->fighting && thg->fgt->fighting->in != thg->in))
                {
	          thg->fgt->fighting = NULL;
	          thg->position = POSITION_STANDING;
		      
	        }
            }
	}
    }
  if (th->fgt)
    {
      th->fgt->fighting = NULL;
    }
  if (th->position == POSITION_FIGHTING || 
      th->position == POSITION_TACKLED)
    th->position = POSITION_STANDING;
  return;
}


/* This adds a thing to the fighting/aggro list and it also makes
   it so that the thing gets a FIGHT struct. */

void
add_fight (THING *th)
{
  if (!th || th->fgt) 
    return;
  
  th->fgt = new_fight ();
  bzero (th->fgt, sizeof (FIGHT));
  th->next_fight = fight_list; 
  th->prev_fight = NULL;
  if (fight_list)
    fight_list->prev_fight = th;  
  fight_list = th;
  create_repeating_event (th, PULSE_COMBAT, update_combat);
  return;
}


void
remove_fight (THING *th)
{
  THING *thg;
  FIGHT *fgt;
  stop_fighting (th);
  
  if (!th)
    return;
  
  if (fight_list_pos == th)
    fight_list_pos = fight_list_pos->next_fight;
  
  
  if (th->next_fight)
    th->next_fight->prev_fight = th->prev_fight;
  if (th->prev_fight)
    th->prev_fight->next_fight = th->next_fight;
  if (th == fight_list)
    fight_list = th->next_fight;
  
  th->next_fight = NULL;
  th->prev_fight = NULL;
  
  if (th->in)
    {
      for (thg = th->in->cont; thg; thg = thg->next_cont)
	{
	  if ((fgt = thg->fgt) == NULL)
	    continue;	  
	  if (fgt->following == th)
	    fgt->following = NULL;
	  if (fgt->gleader == th)
	    fgt->gleader = NULL;
	  if (fgt->mount == th)
	    fgt->mount = NULL;
	  if (fgt->rider == th)
	    fgt->rider = NULL;
	  if (fgt->fighting == th)
	    fgt->fighting = NULL;
	  if (fgt->hunt_victim == th)
	    stop_hunting (th, FALSE);
	}
    }
  if (th->last_in)
    {
      for (thg = th->last_in->cont; thg; thg = thg->next_cont)
	{
	  if ((fgt = thg->fgt) == NULL)
	    continue;	  
	  if (fgt->following == th)
	    fgt->following = NULL;
	  if (fgt->gleader == th)
	    fgt->gleader = NULL;
	  if (fgt->mount == th)
	    fgt->mount = NULL;
	  if (fgt->rider == th)
	    fgt->rider = NULL;
	  if (fgt->fighting == th)
	    fgt->fighting = NULL;
	  if (fgt->hunt_victim == th)
	    stop_hunting (th, FALSE);
	}
    }
  if (th->fgt)
    free_fight (th->fgt);
  th->fgt = NULL;
  return;
}
/* Max number of attackers on a thing is based on its height! */      

void
start_fighting (THING *att, THING *vict)
{
  VALUE *soc;
  if (!CAN_FIGHT (att) || !CAN_FIGHT (vict) ||
      att->in != vict->in || att == vict)
    return;
  
  /* Holypeace stops combat. */
  if (IS_PC (vict) && LEVEL(vict) >= BLD_LEVEL &&
      IS_PC1_SET (vict, PC_HOLYPEACE))
    {
      stt ("You can't attack them!\n\r", att);
      return;
    }
  
  if (!att->fgt)
    add_fight (att);
  if (!vict->fgt)
    add_fight (vict);
  
  

  if (IS_PC (att) && LEVEL (att) < BLD_LEVEL)
    {   
      if (IS_PC (vict) && DIFF_ALIGN (vict->align, att->align) &&
	  LEVEL(vict) < BLD_LEVEL)
	att->pc->no_quit = MAX(att->pc->no_quit, NO_QUIT_PK);
      else
        att->pc->no_quit = MAX(att->pc->no_quit, NO_QUIT_REG);
      /* If you start fighting a society mob, you get your align_hate
	 level going up. */
      if (vict->align < ALIGN_MAX &&
	  (find_society_in (vict) != NULL))
	att->pc->align_hate[vict->align]++;
    }
  if (IS_PC (vict) && LEVEL (vict) < BLD_LEVEL)
    {   
      if (IS_PC (att) && DIFF_ALIGN (vict->align, att->align) &&
          LEVEL (att) < BLD_LEVEL)
	vict->pc->no_quit = MAX(vict->pc->no_quit, NO_QUIT_PK);
      else
        vict->pc->no_quit = MAX(vict->pc->no_quit, NO_QUIT_REG);
    }
  
  if (IS_AFF (att, AFF_CHAMELEON))
    {
      remove_flagval (att, FLAG_AFF, AFF_CHAMELEON);
      act ("$A@1n $Bcome@s back from the $Ae$Bn$Av$Bi$Ar$Bo$An$Bm$Ae$Bn$At$B!$7", att, NULL, NULL, NULL, TO_ALL);
    }
  
  remove_flagval (vict, FLAG_HURT, AFF_SLEEP);
  
  
  if (!FIGHTING (att))
    {
      att->fgt->fighting = vict; 
      if (IS_PC (vict) && !IS_PC (att))
	{
	  start_hunting (att, KEY(vict), HUNT_KILL);
	  att->fgt->hunting_timer = 30;
	}
    } 
  
  if (att->position != POSITION_CASTING && att->position != POSITION_BACKSTABBING && att->position != POSITION_TACKLED && att->position != POSITION_STUNNED)
    att->position = POSITION_FIGHTING;
  
  if (!FIGHTING (vict))
    {
      vict->fgt->fighting = att; 
      if (IS_PC (att) && !IS_PC (vict))
	start_hunting (vict, KEY(att), HUNT_KILL);
      /* When protectors attack, they yell for help. */
      
      if (nr (1,11) == 4)
	{
	  if (!IS_PC (att))
	    {
	      if (((soc = FNV (att, VAL_SOCIETY)) != NULL) ||
		  (IS_ACT1_SET (att, ACT_KILL_OPP) &&
		   DIFF_ALIGN (vict->align, att->align)))
		{
		  char buf[STD_LEN];
		  if (!IS_SET (server_flags, SERVER_BEING_ATTACKED_YELL))
		    {
		      SBIT (server_flags, SERVER_BEING_ATTACKED_YELL);
		      sprintf (buf, "yell Come help! %s is at %s!",
			       name_seen_by (att, vict), NAME (att->in));
		      interpret (att, buf);
		    }
		  RBIT (server_flags, SERVER_BEING_ATTACKED_YELL);
		}
	    }
	  else
	    {
	      if (!IS_PC (vict) && 
		  (is_in_society (vict) ||
		   (DIFF_ALIGN (att->align, vict->align))))
		{
		  char buf[STD_LEN];
		  if (!IS_SET (server_flags, SERVER_BEING_ATTACKED_YELL))
		    {
		      SBIT (server_flags, SERVER_BEING_ATTACKED_YELL);
		      sprintf (buf, "yell Help! I'm being attacked by %s at %s!",
			       name_seen_by (vict, att), NAME (vict->in));
		      interpret (vict, buf);
		    }
		  RBIT (server_flags, SERVER_BEING_ATTACKED_YELL);
		}		   	      
	    }	
	}
    }
  
  if (vict->position != POSITION_CASTING && vict->position != POSITION_BACKSTABBING && vict->position != POSITION_TACKLED && vict->position != POSITION_STUNNED)
    vict->position = POSITION_FIGHTING;
  
  if (MOUNT(att) == vict)
  att->fgt->mount = NULL;
  if (MOUNT(vict) == att)
  vict->fgt->mount = NULL;
  if (RIDER(att) == vict)
    att->fgt->rider = NULL;
  if (RIDER(vict) == att)
    vict->fgt->rider = NULL;
  if (FOLLOWING(att) == vict)
    {
      att->fgt->following = NULL;
      vict->fgt->gleader = NULL;
    }
  if (FOLLOWING(vict) == att)
    {
      vict->fgt->following = NULL;
      vict->fgt->gleader = NULL;
    }
      
  return;
  
}

/* This searches down all the attackers of vict, and if any of them
   are att, we return true. If none are att, but a slot is open,
   we put att into that slot and return true. If all the slots
   are full, and att is not attacking, we return FALSE. */

bool
check_if_in_melee (THING *att, THING *vict)
{
  int attacker_count = 0, attacker_max = 4;
  THING *thg;
  
  if (!att || !vict || !vict->fgt || !att->fgt || !vict->in ||
      FIGHTING (att) != vict)
    return FALSE;
  
  for (thg = vict->in->cont; thg; thg = thg->next_cont)
    {
      if (thg == att ||
	  (FIGHTING(thg) == vict && ++attacker_count >= attacker_max) || 
	  nr (1,10) == 3)
	break;
    }
  
  return (attacker_count < attacker_max);
}
    
  


THING *
find_thing_in_combat (THING * th, char *arg, bool can_switch)
{
  THING *vict;
  char buf[STD_LEN];
  if (IS_AREA (th->in)) /* No fighting in areas!!! */
    return NULL;

  if (th->position < POSITION_FIGHTING)
    {
      sprintf (buf, "You cannot perform combat actions while you are %s.\n\r", position_names[th->position]);
      stt (buf, th);
      if (IS_PC (th))
	th->pc->wait += 5;
      return NULL;
    }  
  if (IS_HURT (th, AFF_FEAR))
    {
      stt ("You are too afraid to attack!\n\r", th);
      return NULL;
    }
  if (th->fgt && (th->fgt->bashlag > 0 || th->fgt->delay > 0))
    {
      stt ("You cannot perform this action while you are under delayed.\n\r", th);
      return NULL;
    }

  if (!can_switch && (vict  = FIGHTING (th)) != NULL)
    return vict;


  if ((vict = find_thing_in (th, arg)) == NULL)
    {
     if ((FIGHTING(th)) == NULL)
       {
         stt ("Who?\n\r", th);
         return NULL;
        }
     else
       vict = FIGHTING(th);
    }
  if (vict == th)
    {
      stt ("You can't fight yourself!\n\r", th);
      return NULL;
    }
  if (IS_SET (vict->thing_flags, TH_NO_FIGHT))
    {
      stt ("You can't attack that!\n\r", th);
      return NULL;
    }

  if (th->fgt && th->fgt->delay > 0)
    {
      stt ("You are still delayed from your last combat action!\n\r", th);
      return NULL;
    }
  
  if (!th->fgt)
    add_fight (th);
  
  if (!vict->fgt)
    add_fight (vict);
  
  return vict;
}

void
do_kill (THING *th, char *arg)
{
  THING *vict, *oldfight;


  oldfight = FIGHTING (th);
  


  if ((vict = find_thing_in_combat (th, arg, TRUE)) == NULL)
    return;

  if (vict == oldfight)
    {
      stt ("You are already fighting that thing!\n\r", th);
      return;
    }
  
  multi_hit (th, vict);
  if (IS_PC (th))    
    th->pc->wait += 15;
  return;
}


void
do_bash (THING *th, char *arg)
{
  THING *vict;
  int smash_num = 0;
  THING *new_room;
  VALUE *exit;
  
  if (!th->in)
    return;
  
  if ((vict = find_thing_in_combat (th, arg, FALSE)) == NULL)
    return;
  
  start_fighting (th, vict);
  if (vict->position == POSITION_TACKLED)
    {
      stt ("Your victim is on the ground!\n\r", th);
      return;
    }
  
  if (vict->position == POSITION_STUNNED)
    {
      stt ("Your victim is already slammed to the ground!\n\r", th);
      return;
    }
  
  if (vict->fgt && vict->fgt->bashlag > 0)
    {
      stt ("Your victim is still too alert.\n\r", th);
      return;
    }

  
  /* Doctor this up some more... */
  
  if (!check_spell (th, NULL, 550 /* Bash */) || nr (1,4) == 2 ||
      (IS_PC (vict) && implant_rank (vict, PART_LEGS) > nr (3,15)))
    {
      act ("@1n miss@s a bash at @3n!", th, NULL, vict, NULL, TO_ALL);
      th->fgt->delay = 1;
      if (IS_PC (th))
	th->pc->wait += 10;
      return;
    }
  
  if (check_spell (th, NULL, 563 /* Smash */))
    {
      smash_num = 1;
      if (damage (th, vict, nr (get_stat (th, STAT_STR)/3, get_stat (th, STAT_STR)), "smash"))
        return;
    }
  
  vict->fgt->bashlag = 6 + nr (0, smash_num);
  th->fgt->delay = 6 - nr (0, smash_num);
  if (!FIGHTING(vict) )
    vict->fgt->fighting = th;

  act ("$A@1n send@s @3n sprawling with a powerful bash!$7", th, NULL, vict, NULL, TO_ALL);
  if (smash_num && nr (1,3) == 2)
    {      
      act ("$9@1p amazing smash just slammed @3n across the room!$7", th, NULL, vict, NULL, TO_ALL);
      if (damage (th, vict, nr (get_stat (th, STAT_STR), 10*get_stat(th,STAT_STR)), "smash"))
        return;
      vict->position = POSITION_STUNNED;
      if (IS_ROOM (th->in) && (exit = FNV (th->in, nr(1,REALDIR_MAX))) != NULL
	  && (new_room = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (new_room) && (IS_PC (th) || nr (1,253) == 125))
	{
	  thing_to (vict, new_room);
          act ("$F@2n was just booted straight out of the room!$7", th, vict, NULL, NULL, TO_ALL);
          vict->fgt->bashlag = 7 + nr (0, 5);
          th->fgt->delay = 0;
        } 
      else
	act ("$F@3p body is thrown around like a ragdoll!$7", th, NULL,vict, NULL, TO_ALL);
    }
  
  vict->position = POSITION_STUNNED;
  return;
}


void
do_tackle (THING *th, char *arg)
{
  THING *vict, *obj;
  
  
  if ((vict = find_thing_in_combat (th, arg, FALSE)) == NULL)
    return;
  
  start_fighting (th, vict);
  
  if (vict->position == POSITION_TACKLED)
    {
      stt ("Your victim is already on the ground!\n\r", th);
      return;
    }
  
  if (vict->position == POSITION_STUNNED)
    {
      stt ("Your victim is bashed to the ground!\n\r", th);
      return;
    }
  
  if (vict->fgt && vict->fgt->bashlag > 0)
    {
      stt ("Your victim is still too alert.\n\r", th);
      return;
    }

  
  if (IS_PC (th) && !IS_SET (flagbits (th->flags, FLAG_PC2), PC2_TACKLE))
    {
      stt ("You aren't ready to tackle anyone. Try typing config tackle!\n\r", th);
      return;
    }

  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if (obj->wear_loc == ITEM_WEAR_WIELD)
	{
	  stt ("You can't tackle with anything in your hands!\n\r", th);
	  return;
	}
    }

  /* Doctor this up some more... */
  
  if ((!check_spell (th, NULL, 552 /* Tackle */) &&
       !check_spell (th, NULL, 597 /* Overpower */)) || nr (1,3) == 2 ||
      (IS_PC (vict) && (implant_rank (vict, PART_LEGS) > nr (3,15) ||
      (!IS_PC2_SET (vict, PC2_TACKLE) && nr (1,2) == 2))))
    {
      act ("@1n leap@s at @3n, but miss@s!", th, NULL, vict, NULL,TO_ALL);
      th->fgt->delay = 1;
      if (IS_PC (th))
	th->pc->wait += 10;
      return;
    }
  vict->position = POSITION_TACKLED;
  th->position = POSITION_TACKLED;
  add_fight (vict);
  vict->fgt->fighting = th;
  act ("$D@1n tackle@s @3n! The fight is taken to the ground!$7", th, NULL, vict, NULL, TO_ALL);
  return;
}


      

void
get_killed (THING *vict, THING *killer)
{
  int i, pcflags, tmoney;
  THING *corpse, *obj, *objn, *mob;
  char splitbuf[100];
  VALUE *feed, *pet;
  
  if (!vict->in || IS_ROOM (vict) || IS_AREA (vict->in))
    return;
  act ("$9@1n just got killed!$7", vict, NULL, NULL, NULL, TO_ALL);
  
  stop_fighting (vict);
  /* If an object casts a spell, the thing wearing the object 
     gets the credit for the kill. */

  if (killer)
    {
      if (IS_WORN(killer) && !FIGHTING (killer) &&
	  killer->in && FIGHTING (killer->in) == vict)
	killer = killer->in;
      
      if (IS_PC (killer) || 
	  ((pet = FNV (killer, VAL_PET)) != NULL &&
	   pet->word && (find_pc (killer, pet->word)) != NULL))
	kill_exp (killer, vict);
      
      if (!IS_PC (killer) && nr (1,10*LEVEL(killer)) == 3 && 
	  nr (1,200) == 34 &&
	  killer->proto && LEVEL(killer) <= 3*LEVEL(killer->proto)/2)
	killer->level++;      
    }

  /* Feeding for start of "ecology" */
  
  if ((corpse = make_corpse (vict, killer)) != NULL && corpse->in && 
      killer && (feed = FNV (killer, VAL_FEED)) != NULL)
    {
      for (i = 0; i < 4; i++)
	{
	  if (feed->val[i] > 0 && feed->val[i] == vict->vnum)
	    {
	      act ("@1n feed@s on @2n!", killer, corpse, NULL, NULL, TO_ROOM);
	      for (obj = corpse->cont; obj; obj = objn)
		{
		  objn = obj->next_cont;
		  thing_to (obj, corpse->in);
		}
	      free_thing (corpse);
	      feed->val[5] = 0;
	      interpret (killer, "burp");
	      break;
	    }
	}
      /* Most of the time mob vs mob fights end with corpse sac, otherwise
	 most of the time its a corpse loot, otherwise it gets left alone
	 1/12 of the time. */
      if (!IS_PC (vict) && !IS_PC(killer))
	{
	  if (nr(1,3) != 2)
	    do_sacrifice (killer, "corpse");
	  else if (nr (1,4) != 4)
	    do_get (killer, "all corpse");
	}

    }
  
  /* Deal with society problems when something gets killed. */

  society_get_killed (killer, vict);
  
  /* Maybe some  stuff on bg/arena here? for later? */
  
  if (IS_PC (vict)) /* PERMADEATH!!!!! :) */
    {
      if (IN_BG (vict))
	{
	  restore_thing (vict);
	  thing_to (vict, find_thing_num (vict->pc->in_vnum));
	  return;
	}      
      
      /* Stop everyone except players from hunting/fighting this thing. */

      for (mob = fight_list; mob; mob = mob->next_fight)	
	{
	  if (!mob->fgt)
	    continue;
	  
	  if (FIGHTING (mob) == vict)
	    mob->fgt->fighting = NULL;
	  
	  if (mob->fgt->hunt_victim == vict ||
	      !str_cmp (mob->fgt->hunting, NAME(vict)))
	    stop_hunting (mob, TRUE);
	}
      

      if (IS_PC1_SET (vict, PC_PERMADEATH))
	{
	  char filename[200];
	  sprintf (filename, "%s%s", PLR_DIR, NAME (vict));
	  unlink (filename); 
	  if (vict->fd)
	    {
	      send_output (vict->fd);
	      close_fd (vict->fd);
	    }
	  vict->fd = NULL;
	  free_thing (vict);	  
	  return;
	}
      
      /* Clean up the victim, and send it back home */
      
      remove_all_affects (vict, FALSE);
      vict->pc->pk[PK_KILLED]++;
      vict->hp = 5;
      vict->position = POSITION_RESTING;
      vict->mv = vict->max_mv;
      vict->pc->no_quit = 0;
      for (i = 0; i < COND_MAX; i++)
	vict->pc->cond[i] = COND_MAX;
      vict->pc->cond[COND_DRUNK] = 0;
      for (i = 0; i < MAX_SPELL; i++)
	{
	  if (vict->pc->prac[i] > 1 && nr (1,7) == 2)
	    vict->pc->prac[i] -= nr(1,2);
	} 
     
      
      write_playerfile (vict);
      if (!vict->fd)
	{
#ifdef USE_WILDERNESS
	  update_sector_population_thing (vict, FALSE);
#endif
	  free_thing (vict);
	}
      else
	thing_to (vict, find_thing_num (vict->align + 100)); 
    }
  else
    free_thing (vict);
  
  if (killer && IS_PC (killer) && corpse)
    {
      pcflags = flagbits (killer->flags, FLAG_PC2);
      tmoney = total_money (corpse);
      if (IS_SET (pcflags, PC2_AUTOLOOT) && !IS_PC (vict))
        do_get (killer, "all corpse");
      else if (IS_SET (pcflags, PC2_AUTOGOLD))
        do_get (killer, "coins corpse");
      if (IS_SET (pcflags, PC2_AUTOSAC))
        do_sacrifice (killer, "corpse");
      if (tmoney > 0 && IS_SET (pcflags, PC2_AUTOLOOT | PC2_AUTOGOLD) &&
	  IS_SET (pcflags, PC2_AUTOSPLIT))
	{
	  sprintf (splitbuf, "%d", tmoney);
	  do_split (killer, splitbuf);
	}
    }
  return;
}


THING *
make_corpse (THING *vict, THING *killer)
{
  char buf[STD_LEN];
  THING *obj, *objn;
  THING *corpse;
  bool lost_corpse = FALSE;
  bool empty_corpse = FALSE;
  int roomflags;
  VALUE *shop;

  
  if (!vict || !vict->in || IN_BG(vict))
    return NULL;
  
  /* Corpses get lost in harsh rooms...also possibly no corpses from
     ghosts or something like that? Dunno. Also shops don't put stuff in
     corpses either...*/

  
  roomflags = flagbits (vict->in->flags, FLAG_ROOM1);
  
  if ((shop = FNV (vict, VAL_SHOP)) != NULL)
    empty_corpse = TRUE;
  else if (IS_SET (roomflags, ROOM_FIERY))
    {
      act ("The corpse of @1n burns up in the flames!", vict, NULL, NULL, NULL, TO_ROOM);
      lost_corpse = TRUE;
    }
  else if (IS_SET (roomflags, ROOM_WATERY))
    {
      act ("The corpse of @1n sinks to the bottom...", vict, NULL, NULL, NULL, TO_ROOM);
      lost_corpse = TRUE;
    }
  else if (IS_SET (roomflags, ROOM_AIRY))
    {
      act ("The corpse of @1n falls to the earth below, lost forever.", vict, NULL, NULL, NULL, TO_ROOM);
      lost_corpse = TRUE;
    }
  else if (IS_SET (roomflags, ROOM_EARTHY))
    {
      act ("The corpse of @1n is lodged in the rocks forever!", vict, NULL, NULL, NULL, TO_ROOM);
      lost_corpse = TRUE;
    }

  /* Note that if you want 'savable items' that players can't lose...
     although that is a really wussy idea...you can do that by
     altering some of the code here so that even if you have
     an empty corpse, you don't lose that equipment, like a line like
     if (IS_OBJ_SET (obj, OBJ_SAVE)) continue; both in the loop
     just below this, and in the loop where stuff gets moved to the
     corpse otehrwise. You will of course need to make the OBJ_SAVE
     flag. :) */
  
  if (lost_corpse || empty_corpse)
    {
      
      for (obj = vict->cont; obj != NULL; obj = objn)
	{
	  objn = obj->next_cont;
	  /* if (IS_OBJ_SET (obj, OBJ_SAVE)) continue; */
	  free_thing (obj);
	}
      if (lost_corpse)
	return NULL;
    }
  if ((corpse = create_thing (CORPSE_VNUM)) == NULL)
    {
      log_it( "FAILED TO MAKE A CORPSE!!!\n");
      return NULL;
    }
  thing_to (corpse, vict->in);
  corpse->timer = (IS_PC (vict) ? nr (20, 40) : nr (5, 10));
  corpse->level = vict->level;
  /* I don't give the corpse a "name" since then it interfere with
     killing multiple creatures with the same name in the same
     room...the corpse will suck up commands like "kill dragon",
     since "dragon corpse" is the first item in the room. */
  
  sprintf (buf, "\x1b[0;31mthe corpse of \x1b[1;31m%s\x1b[0;37m", NAME (vict));
  if (strlen (buf) > 200)
    strcpy (buf, "a corpse");
  free_str (corpse->short_desc);
  corpse->short_desc = nonstr;
  corpse->short_desc = new_str (buf);
  free_str (corpse->long_desc);
  corpse->long_desc = nonstr;
  sprintf (buf, "\x1b[0;31mThe corpse of \x1b[1;31m%s\x1b[0;31m is lying here.\x1b[0;37m", NAME(vict)); 
  if (strlen (buf) > 300)
    strcpy (buf, "A corpse is here.");
  corpse->long_desc = new_str (buf);
  if (killer)
    corpse->type = new_str (NAME(killer));
  else
    corpse->type = nonstr;
  
  if (IS_PC (vict))
    corpse->thing_flags |= TH_NO_TAKE_BY_OTHER;
  

  /* Npc on Npc kills don't drop loot in Arkane's code. */
#ifndef ARKANE
  if (!empty_corpse)  
    add_money (corpse, total_money (vict));
  sub_money (vict, total_money (vict));

  for (obj = vict->cont; obj; obj = objn)
    {
      objn = obj->next_cont;
      /* if (IS_OBJ_SET (obj, OBJ_SAVE)) continue; */
      remove_thing (vict, obj, TRUE);
      if (corpse)
	thing_to (obj, corpse);
      else
	thing_to (obj, vict->in);
    }
#endif
  return corpse;
}

void
do_flee (THING *th, char *arg)
{
  THING *thg, *start_in, *room;
  int i, dirs = 0 ,count = 0, chose = 0, dirchose = 0;
  bool fighting = TRUE;
  VALUE *exit, *soc;
  if ((start_in = th->in) == NULL)
    return;
  
  /* Sentinel creatures don't flee. */
  if (!IS_PC (th) && IS_ACT1_SET (th, ACT_SENTINEL))
    return;
  

  /* Cannot flee while bashed. */
  
  if (th->fgt && th->fgt->bashlag > 2)
    {
      stt ("You can't flee while you are bashed!\n\r", th);
      return;
    }

  /* Cannot flee during combat delay. */
  
  if (th->fgt && th->fgt->delay > 0)
    {
      stt ("You can't flee so soon!\n\r", th);
      return;
    }  

  /* Cannot flee if you're not fighting. */
  
  if (!FIGHTING (th))
    {
      fighting = FALSE;
      for (thg = start_in->cont; thg; thg = thg->next_cont)
	{
	  if (FIGHTING(thg) == th)
	    {
	      fighting = TRUE;
	      break;
	    }
	}
    }
  
  if (!fighting)
    {
      stt ("You don't have to flee if you are not fighting!\n\r", th);
      return;
    }
  

  if (!IS_ROOM (th->in) && th->in->in)
    { 
      if (nr (1,8) == 2)
        {
          stt ("You couldn't escape!\n\r", th);
          if (IS_PC (th))
            th->pc->wait += 10;
          return;
        }
      act ("$A@1n panic@s and flee@s out!", th, NULL, NULL, NULL,
TO_ALL);
      do_leave (th, "");
      return;
    }
  for (exit = th->in->values; exit != NULL; exit = exit->next)
    {
      if (exit->type > 0 && exit->type <= REALDIR_MAX &&
	  !IS_SET (exit->val[1], EX_WALL) &&
	  (!IS_SET (exit->val[1], EX_DOOR) ||
	   !IS_SET (exit->val[1], EX_CLOSED)) &&
	  (room = find_thing_num (exit->val[0])) != NULL &&
	  IS_ROOM (room) &&
	  room != start_in)
	dirs |= (1 << (exit->type - 1));
    }
  
  for (i = 0; i < REALDIR_MAX; i++)
    if (IS_SET (dirs, (1 << i)))
      count++;
  
  if (IS_PC (th) &&
      (count == 0 || np () < (40 - 3 * implant_rank (th, PART_LEGS) 
			      + (IS_PC (th) ? (th->pc->off_def/2 -25) : 0))))
    {
      stt ("You couldn't escape!\n\r", th);
      return;
    }
  
  chose = nr (1, count);
  
  for (i = 0;  i < REALDIR_MAX; i++)
    if (IS_SET (dirs, (1 << i)) && ++dirchose == chose)
      break;  
  
  if (move_dir (th, i, MOVE_FLEE))
    {
      if (start_in->cont)
	act ("$A@2n panics and flees @t!$7", start_in->cont, th, NULL,dir_name[i], TO_ALL);
      

      if (IS_PC (th))
	{
	  dirs = 0;
	  for (exit = th->in->values; exit != NULL; exit = exit->next)
	    {
	      
	      if (exit->type > 0 && exit->type <= REALDIR_MAX &&
		  !IS_SET (exit->val[1], EX_WALL) &&
		  (!IS_SET (exit->val[1], EX_DOOR) ||
		   !IS_SET (exit->val[1], EX_CLOSED)) &&
		  (room = find_thing_num (exit->val[0])) != NULL &&
		  IS_ROOM (room) && 
		  room != start_in && room != th->in)
		dirs |= (1 << (exit->type - 1));
	    }
	  count = 0;
	  dirchose = 0;
	  chose = 0;
	  for (i = 0; i < REALDIR_MAX; i++)
	    if (IS_SET (dirs, (1 << i)))
	      count++;
	  
	  if (count > 0)
	    {
	      chose = nr (1, count);
	      
	      for (i = 0;  i < REALDIR_MAX; i++)
		if (IS_SET (dirs, (1 << i)) && ++dirchose == chose)
		  break;  
	      
	      move_dir (th, i, 0);
	    }
	}
      else if (th->hp < th->max_hp/4)
	{
	  if ((soc = FNV (th, VAL_SOCIETY)) != NULL &&
	      !IS_SET (soc->val[2], CASTE_HEALER))
	    {
	      THING *healer;
	      if ((healer = find_society_member_nearby (th, CASTE_HEALER, 5)) != NULL)
		start_hunting (th, KEY (healer), HUNT_HEALING);
	    }
	  else if (nr (1,5) == 3 && th->in && IS_ROOM (th->in))
	    {
	      THING *newroom = find_random_room (th->in->in, FALSE, 0, 0);
	      if (newroom && IS_ROOM (newroom))
		{
		  start_hunting_room (th, newroom->vnum, HUNT_HEALING);
		}
	    }
	}
      
    }
  else
    {
      stt ("You couldn't escape!\n\r", th);
    }
  

  /* Players get lagged by fleeing. */
  if (IS_PC (th))
    th->pc->wait += 12;
  
  return;
}
  
void
do_kick (THING *th, char *arg)
{
  THING *vict;
  int dam = 0, times = 1, num_legs = 0, i;
  
  if ((vict = find_thing_in_combat (th, arg, FALSE)) == NULL)
    return;
  
  num_legs = (IS_PC (th) ? FRACE(th)->parts[PART_LEGS] +
	      FALIGN(th)->parts[PART_LEGS] : 1);
  if (num_legs < 1 || (IS_PC (th) && th->pc->prac[551 /* Kick */] < 10))
    {
      stt ("You can't kick.\n\r", th);
      return;
    }
  
  times = 1 + nr (0, num_legs - 1);
  
  if (nr (1,4) == 2 && check_spell (th, NULL, 560 /* Multi kick */))
    times += 1 + nr (0, num_legs - 1);
  
  start_fighting (th, vict);
  
  for (i = 0; i < times; i++)
    {
    
      dam = MIN (nr (1, LEVEL (th)*2/3), 42) + nr (0, implant_rank (th, PART_LEGS));      
      if (IS_PC (th))
        {
          dam += nr (0, th->pc->aff[FLAG_AFF_KICKDAM]/2);
          dam += nr(0, dam*th->pc->prac[566 /* Enh Kick */]/500);
        }
      else
	dam += nr (0, LEVEL (th)/5);

      if (!check_spell (th, NULL, 551) || (nr (1,6) == 2 &&
          !check_spell (th, NULL, 566 /* Enhanced kick */)) ||
          (check_spell (vict, NULL, 587 /* Balance*/) && nr(1,25) ==11) ||
          (check_spell (vict, NULL, 589 /* Stead */) && nr(1,17) ==10) ||
          (check_spell (vict, NULL, 590 /* Immo */) && nr (1,12) == 9))
	{
	  act ("$B@1n miss@s @3n with @1s kick.$7", th, NULL, vict, NULL, TO_ALL);
	  continue;
	}
      if (check_spell (vict, NULL, 588 /* Block */) && nr (1,8) == 3)
        {
          if (nr(1,3) == 2)
            act ("@1n block@s @3p kick!", vict, NULL, th, NULL, TO_ALL);
          else if (nr(1,2) == 2)
            act ("@1n stop@s @3p kick!", vict, NULL, th, NULL, TO_ALL);
          else
            act ("@1n push@s @3p kick aside!", vict, NULL, th, NULL,TO_ALL);
          continue;
        }
      if (dam < 5)
	{
	  if (nr (1,2) == 2)
	    act ("$B@1p weak kick barely hit@s @3n.$7", th, NULL, vict, NULL, TO_ALL);
	  else
	    act ("$B@1p kick glances off @3n.$7", th, NULL, vict, NULL, TO_ALL);
	}
      else if (nr (1,4) == 2)
	act ("$9@1n lift@s @1s leg high and kick@s @3n in the head!$7", th, NULL, vict, NULL, TO_ALL);
      else if (nr (1,3) == 2)
	act ("$9@1n lift@s @1s leg high and kick@s @3n in the mouth!$7", th, NULL, vict, NULL, TO_ALL);
      else if (nr (1,2) == 1)
	act ("$B@1n kick@s @3n right in the stomach, knocking the wind right out of @3m!$7", th, NULL, vict, NULL, TO_ALL);
      else
	act ("$B@1n kick@s the living crap out of @3n!$7", th, NULL, vict, NULL, TO_ALL);
      if (damage (th, vict, dam, "kick"))
	break;
    }
  if (IS_PC (th))
    th->pc->wait += 15;
  return;
  
 
}


void
do_flurry (THING *th, char *arg)
{
  THING *vict;
  if (!FIGHTING (th))
    {
      stt ("You must be fighting to flurry!\n\r", th);
      return;
    }
  
  if ((vict = find_thing_in_combat (th, arg, FALSE)) == NULL)
    return;
  
  if (th->mv < 25)
    {
      stt ("You are too weak to flurry!\n\r", th);
      return;
    }



  if (IS_PC (th) && check_spell (th, NULL, 556 /* Ravage */))
    {
      SBIT (server_flags, SERVER_RAVAGE);
      act ("$9whirl@s about in a furious RAGE!$7", th, NULL, vict, NULL, TO_ALL);
    }
  else
    act ("@1n open@s up a wild flurry of attacks!", th, NULL, NULL, NULL, TO_ALL);
  
  multi_hit (th, vict);
  
  if (FIGHTING(th) == vict && nr (1,3) != 2)
    multi_hit (th, vict);
  
  if (FIGHTING (th) == vict && check_spell (th, NULL, 554 /* Flurry */))
    multi_hit (th, vict);
  
  th->mv -= 25;
  if (IS_PC (th))
    th->pc->wait += 15 + 15 * (IS_SET (server_flags, SERVER_RAVAGE) ?1:0);
  RBIT (server_flags, SERVER_RAVAGE);
  return;
}    


void 
do_backstab (THING *th, char *arg)
{
  THING *vict, *obj, *objn;
  VALUE *wpn, *guard;
  bool found_dagger = FALSE, done_once = FALSE, sleeping = FALSE;
  SPELL *spl;
  bool fighting_now = FALSE;
  char buf[STD_LEN];
  
  if ((spl = find_spell ("backstab", 0)) == NULL)
    {
      stt ("Backstab skill doesn't exist.\n\r", th);
      return;
    }

  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if (!IS_WORN(obj) ||
	  obj->wear_loc > ITEM_WEAR_WIELD)
	break;
      
      if (obj->wear_loc == ITEM_WEAR_WIELD &&
	  (wpn = FNV (obj, VAL_WEAPON)) != NULL &&
	  wpn->val[2] == WPN_DAM_PIERCE)
	{
	  found_dagger = TRUE;
	  break;
	}
    }
  
  if (!found_dagger)
    {
      stt ("You must be wielding a piercing weapon to backstab someone!\n\r", th);
      return;
    }
  
  /* Find the victim */

  if ((vict = find_thing_in_combat (th, arg, FALSE)) == NULL)
    {
      stt ("Backstab who?\n\r", th);
      return;
    }

  /* You autobs sleeping victims... I will make this cheater for
     now, but will nerf it if it's too cheater. :) */
  
  if (IS_HURT (vict, AFF_SLEEP))
    sleeping = TRUE;
  /* Moved this to after the sleep check.... anyone can bs sleepers. */
  else if (IS_PC (th) && th->pc->prac[spl->vnum] < 10)
    {
      stt ("You don't know how to backstab.\n\r", th);
      return;
    }
  
  /* The victim cannot be fighting you. */

  if (FIGHTING (vict) == th)
    {
      stt ("You cannot backstab someone who is fighting you!\n\r", th);
      th->position = POSITION_FIGHTING;
      return;
    }
  
  /* The victim cannot be a guard. */

  if (!sleeping && (guard = FNV (vict, VAL_GUARD)) != NULL)
    {
      stt ("You can't backstab someone who is guarding something. They're too alert.\n\r", th);
      return;
    }

  /* If you're fighting, the victim must also be fighting for you to
     backstab/circle it. */
  
  if (!IS_PC (vict) && FIGHTING (th) && !FIGHTING (vict))
    {
      stt ("Your intended victim isn't fighting and you are, so you can't sneak around...\n\r", th);
      return;
    }
  
  if (!sleeping && !DOING_NOW && IS_PC (th))
    {
      act ("You stealthily begin to move behind @3n.....", th, NULL, vict, NULL, TO_CHAR);
      sprintf (buf, "backstab %s", arg);
      add_command_event (th, buf, spl->ticks);
      th->position = POSITION_BACKSTABBING;
      if (th->fgt && th->fgt->fighting == vict)
	th->fgt->fighting = NULL;
      return;
    }

  /* Make sure they have the circle skill to bs in combat. */
  
  if (FIGHTING (th) && IS_PC (th))
    {
      if (th->pc->prac[558 /* Circle */] < MIN_PRAC)
        {
          stt ("You don't know how to circle people in combat!\n\r", th);
          return;
        }
      fighting_now = TRUE; 
    }

  for (obj = th->cont; obj; obj = objn)
    {
      objn = obj->next_cont;
      if (!IS_WORN(obj) ||
	  obj->wear_loc > ITEM_WEAR_WIELD)
	break;
      
      if (done_once && nr (1,2) != 2 &&
	  (sleeping || !check_spell (th, NULL, 557 /* Multi Backstab */)))
	continue;
      
      if (obj->wear_loc == ITEM_WEAR_WIELD &&
	  (wpn = FNV (obj, VAL_WEAPON)) != NULL &&
	  wpn->val[2] == WPN_DAM_PIERCE)
	{
	  done_once = TRUE;
	  if (!check_spell (th, NULL, 553 /* Backstab */) ||
            (fighting_now && !check_spell (th, NULL, 558 /* Circle */)))
	    {
	      act ("@1n miss@s with @1s backstab!", th, NULL, vict, NULL, TO_ALL);
	    }	  
	  else
	    {
	      act ("$9@1n$9 place@s @2n$9 into @3p$9 back!$7", th, obj, vict, NULL, TO_ALL);
	      if (one_hit (th, vict, obj, SP_ATT_BACKSTAB))
		break;
	    }
	  
	  start_fighting (th, vict);
	  if (vict->fgt && vict->fgt->fighting != th)
	    vict->fgt->fighting = th;
	  if (th->fgt && th->fgt->fighting != vict)
	    th->fgt->fighting = vict;
	}
    }
  
  if (FIGHTING (th))
    th->position = POSITION_FIGHTING;
  else
    th->position = POSITION_STANDING;
  if (IS_PC (th))
    th->pc->wait += 10;
  return;
  

}


void
do_load (THING *th, char *arg)
{
  char arg1[STD_LEN];
  THING *ranged = NULL, *ammo = NULL;
  VALUE *rng = NULL, *amo = NULL;
  bool loaded = FALSE;
  arg = f_word (arg, arg1);

  if (arg1[0] != '\0') /* Specified an item */
    {
      if (((ranged = find_thing_worn (th, arg1)) == NULL &&
	   (ranged = find_thing_here (th, arg1, TRUE)) == NULL) ||
	  (rng = FNV (ranged, VAL_RANGED)) == NULL)
	{
	  stt ("load <ranged item> <<ammo>>\n\r", th);
	  return;
	}
      
      if (ranged->cont)
	{
	  stt ("That ranged weapon is already loaded!\n\r", th);
	  return;
	}
      
      if (*arg) /* Specicied ammo */
	{
	  if ((ammo = find_thing_me (th, arg)) == NULL ||
	      (amo = FNV (ammo, VAL_AMMO)) == NULL)
	    {	  
	      stt ("load <ranged item> <ammo>\n\r", th);
	      return;
	    }
	  if (amo->val[4] != rng->val[4])
	    {
	      stt ("That ammo won't fit into that ranged weapon!\n\r", th);
	      return;
	    }
	}
      else
	{
	  for (ammo = th->cont; ammo; ammo = ammo->next_cont)
	    {
	      if (ammo == ranged)
		continue;
	      if ((amo = FNV(ammo, VAL_AMMO)) != NULL &&
		  amo->val[4] == rng->val[4])
		break;
	    }
	  if (!ammo)
	    {
	      stt ("You don't have any ammo for that weapon!\n\r", th);
	      return;
	    }
	}
      thing_to (ammo, ranged);
      act ("@1n load@s @2n into @1s @3n.", th, ammo, ranged, NULL, TO_ALL);
      return;
    }
  
  /* Now deal with load all. */
  
  for (ranged = th->cont; ranged; ranged = ranged->next_cont)
    {
      if (!IS_WORN (ranged))
	break;
      
      if (ranged->cont ||
	  (rng = FNV (ranged, VAL_RANGED)) == NULL)
	continue;
      
      for (ammo = th->cont; ammo; ammo = ammo->next_cont)
	{
	  if (IS_WORN (ammo) || ammo == ranged || 
	      (amo = FNV (ammo, VAL_AMMO)) == NULL ||
	      amo->val[4] != rng->val[4])
	    continue;
	  loaded = TRUE;
	  act ("@1n load@s @2n into @1s @3n.", th, ammo, ranged, NULL, TO_ALL);
	  thing_to (ammo, ranged);
	  break;
	}
    }
  if (!loaded)
    stt ("You don't have anything you can load.\n\r", th);
  return;
}


void
do_unload (THING *th, char *arg)
{
  
  char arg1[STD_LEN];
  THING *ranged = NULL, *obj, *objn;
  VALUE *rng = NULL;

  arg = f_word (arg, arg1);
  
  if (((ranged = find_thing_worn (th, arg1)) == NULL &&
       (ranged = find_thing_here (th, arg1, TRUE)) == NULL) ||
      (rng = FNV (ranged, VAL_RANGED)) == NULL)
    {
      stt ("unload <ranged item>\n\r", th);
      return;
    }
  
  if (!ranged->cont)
    {
      stt ("That ranged weapon is not loaded!\n\r", th);
      return;
    }
  
  for (obj = ranged->cont; obj != NULL; obj = objn)
    {
      objn = obj->next_cont;
      
      act ("@1n unload@s @2n from @3n.", th, obj, ranged, NULL, TO_ALL);
      thing_to (obj, th);
    }
  return;
}

/* This handles reg ranged wpns wielded Fires all weapons worn all at
   once. This could lead to cyborgs with weapons in each armor slot :) */

void
do_fire (THING *th, char *arg)
{
  int maxrange = 0, maxwait = 0, shots_left, dam, armor, removed;
  THING *vict, *ranged, *ammo;
  VALUE *rng, *amo;
  bool found = FALSE, killed = FALSE;
  char *oldarg = arg;
  char buf[STD_LEN];
  
  if (IS_PC (th) && th->pc->prac[555 /* Fire */] < 30)
    {
      stt ("You don't know how to fire a weapon!\n\r", th);
      return;
    }
  
  if (FIGHTING (th))
    {
      stt ("You cannot fire at someone while you are fighting!\n\r", th);
      return;
    }

  if (th->position < POSITION_FIGHTING)
    {
      stt ("You cannot fire from this position.\n\r", th);
      return;
    }

  /* Find maximum range... and maximum waiting speed. */

  for (ranged = th->cont; ranged != NULL; ranged = ranged->next_cont)
    {
      if (!IS_WORN(ranged))
	break;
      if ((rng = FNV (ranged, VAL_RANGED)) != NULL &&
	  (ammo = ranged->cont) != NULL &&
	  (amo = FNV (ammo, VAL_AMMO)) != NULL &&
	  amo->val[4] == rng->val[4] && amo->val[2] > 0)
	{
	  found = TRUE;
	  maxrange = MAX (maxrange, rng->val[1]);
	  maxwait = MAX (maxwait, rng->val[0]);
	}
    }
  
  if (!found)
    {
      stt ("You aren't using any ranged weapons at the moment!\n\r", th);
      return;
    }
  
  if ((vict = find_thing_near (th, arg, maxrange)) == NULL)
    {
      stt ("Fire at what?\n\r", th);
      return;
    }
  
  /* Make the person wait to fire... */
  
  if (IS_PC (th) && !DOING_NOW)
    {
      act ("@1n begin@s to aim at @1s target...", th, NULL, NULL, NULL, TO_ALL);
      th->position = POSITION_FIRING;
      sprintf (buf, "aim %s", oldarg);
      add_command_event (th, buf, maxwait);
      return;
    }

  /* Now we actually fire. Check all worn items, see if they are ranged,
     and if they have ammo as a content, and if the ammo matches,
     and if there are shots left, and if there is a victim within range.
     Then fire. */
  
   for (ranged = th->cont; ranged != NULL; ranged = ranged->next_cont)
     {
      if (!IS_WORN(ranged))
	break;
      
      if ((rng = FNV (ranged, VAL_RANGED)) != NULL &&
	  (ammo = ranged->cont) != NULL &&
	  (amo = FNV (ammo, VAL_AMMO)) != NULL &&
	  amo->val[4] == rng->val[4] && 
	  amo->val[2] > 0 &&
	  (vict = find_thing_near (th, arg, rng->val[1])) != NULL)
	{
	  killed = FALSE;
	  for (shots_left = MIN (amo->val[2], MAX(1, rng->val[2])); shots_left > 0; shots_left--)
	    {
	      if (!check_spell (th, NULL, 555 /* fire */) ||
		  IS_ACT1_SET (vict, ACT_SENTINEL))
		{
		  act ("@1n shoot@s at @3n with @2n, but misses.", th, ranged, vict, NULL, TO_ALL);
		}
	      else
		{
		  dam = dice (amo->val[0], amo->val[1]);
		  dam = dam * MID (2, rng->val[3], 50) /10;
		  dam += nr (0, dam/2);
		  armor = vict->armor/ARMOR_DIVISOR + 
		    (IS_PC (vict) ? vict->pc->armor[nr (0, PARTS_MAX - 1)] : 0);
		  
		  removed = nr (0, armor);
		  dam = MAX (1, dam - removed);
		  act ("@1n shoot@s @3n with @2n!", th, ranged, vict, NULL, TO_ALL);
		  killed = damage (th, vict, dam, "hit");
		}
	      if (IS_PC (th) && --amo->val[2] == 0)
		{
		  act ("@3n is empty and gets ejected from @2n.", th, ranged, ammo, NULL, TO_ALL);
		  free_thing (ammo);
		  break;
		}
	      if (killed)
		break;
              
	    }
	}
    }
   
   return;
}

/* Rescue makes an attacker stop attacking a friend and instead 
   attack you. They must be regular fighting, and they must be 
   pc's or npc's same as you, and same align. */


void
do_rescue (THING *th, char *arg)
{
  THING *victim;
  
  if (!th->in || (victim = find_thing_in (th, arg)) == NULL)
    {
      stt ("They aren't here!\n\r", th);
      return;
    }
  
  /* This is split up to allow for direct rescuing based on something
     other than name... Otherwise we would have to calc crap like 
     "rescue 15.elf" in combat_ai. */
  
  rescue_victim (th, victim);
  return;
}
 
/* This is the actual algorithm for the rescue command. It picks
   a random person attacking the victim and makes them attack
   the rescuer. This was split off from do_rescue to allow for
   direct rescuing of a thing by another, avoiding names parsing
   and junk like that. */

void
rescue_victim (THING *th, THING *victim)
{
  
  THING *attacker;
  
  int num_choices = 0, count = 0, num_chose = 0, num_opp = 0;
  
  if (!th || !th->in || !victim || !victim->in ||
      th->in != victim->in)
    return;
  
  if (!FIGHTING (victim) || (victim->position != POSITION_FIGHTING &&
			     victim->position != POSITION_CASTING &&
			     victim->position != POSITION_FIRING &&
			     victim->position != POSITION_BACKSTABBING))
    {
      stt ("That person is not fighting normally!\n\r", th);
      return;
    }
  
  if (IS_PC (victim) != IS_PC (th) || DIFF_ALIGN (victim->align, th->align))
    {
      stt ("You can only rescue players of your own align!\n\r", th);
      return;
    }

  

  for (attacker = th->in->cont; attacker != NULL; attacker = attacker->next_cont)
    {
      if (FIGHTING (attacker) == victim)
	num_choices++;
      if (FIGHTING (attacker) == th)
	num_opp++;
    }
  
  if(!check_spell (th, NULL, 561 /* rescue */) || 
     (num_opp > 0 && nr (1, num_opp) > 2))
    {
      act ("@1n try@s to rescue @3n, but fail@s!", th, NULL, victim, NULL, TO_ALL);
    }
  else
    {
      act ("@1n heroically rescue@s @3n!", th, NULL, victim, NULL, TO_ALL);
     
      if (num_choices > 0)
	{
	  num_chose = nr (1, num_choices);
	  for (attacker = th->in->cont; attacker != NULL; attacker = attacker->next_cont)
	    if (FIGHTING (attacker) == victim && ++count == num_chose)
	      break;
	}
      if (attacker && attacker->fgt)
	attacker->fgt->fighting = th;
    }
  if (IS_PC (th))
    th->pc->wait += 15;
  if (th->fgt)
    th->fgt->delay++;
  
  return;
}

/* This changes the order of your items in your hands (if you
   have 2 or more things in your hands. */

void
do_swap (THING *th, char *arg)
{
  THING *obj, *obj0 = NULL, *obj1 = NULL;
  int wear;
  for (obj = th->cont; obj != NULL; obj = obj->next_cont)
    {
      if ((wear = obj->wear_loc) == ITEM_WEAR_NONE ||
	  wear > ITEM_WEAR_WIELD)
	break;
      if (wear != ITEM_WEAR_WIELD)
	continue;
      
      if (obj->wear_num == 0)
	obj0 = obj;
      else if (obj->wear_num == 1)
	obj1 = obj;
      
    }
  
  if (!obj0 && !obj1)
    {
      stt ("You don't have anything in your hands!\n\r", th);
      return;
    }
  
  if (obj0 && !obj1)
    {
      act ("@2n is now in @1p secondary hand.", th, obj0, NULL, NULL, TO_ALL);
      obj0->wear_num = 1;
      return;
    }
  
  if (!obj0 && obj1)
    {
      act ("@2n is now in @1p primary hand.", th, obj1, NULL, NULL, TO_ALL);
      obj1->wear_num = 0;
      return;
    }
  
  if (obj0 && obj1)
    {
      act ("@1n swap@s @2n and @3n in @1s hands.", th, obj0, obj1, NULL, TO_ALL);
      obj1->wear_loc = ITEM_WEAR_NONE;
      obj0->wear_num = 1;
      thing_from (obj1);
      obj1->wear_loc = ITEM_WEAR_WIELD;
      obj1->wear_num = 0;
      thing_to (obj1, th);
      return;
    }
  
}

/* This lets you stop fighting without fleeing. */

void
do_disengage (THING *th, char *arg)
{
  THING *thg;
  
  if (!th->fgt || !FIGHTING (th) || !th->in)
    {
      stt ("You aren't fighting!\n\r", th);
      return;
    }
  
  for (thg = th->in->cont; thg; thg = thg->next_cont)
    {
      if (FIGHTING (thg) == th)
	{
	  stt ("You can't disengage while someone's fighting you!\n\r", th);
	  return;
	}
    }
  
  for (thg = th->in->cont; thg; thg = thg->next_cont)
    {
      if (FIGHTING (thg) == th)
	{
	  stt ("You can't disengage while someone's fighting you!\n\r", th);
	  return;
	}
    }

  act ("$A@1n disengage@s from combat.$7", th, NULL, NULL, NULL, TO_ALL);
  th->fgt->fighting = NULL;
  if (th->position == POSITION_FIGHTING)
    th->position = POSITION_STANDING;
  return;
}
  
/* This skill sets it up so you automatically attempt to rescue someone
   every round. */

void
do_guard (THING *th, char *arg)
{
  THING *vict;
  
  if (!IS_PC (th))
    return;

  if (th->pc->prac[562 /* Guard */] < 30)
    {
      stt ("You don't know how to guard!\n\r", th);
      return;
    }
  
  if (!str_cmp (arg, "me"))
    vict = th;
  else if ((vict = find_thing_in (th, arg)) == NULL)
    {
      stt ("They aren't here!\n\r", th);
      return;
    }
  
  if (!IS_PC (vict) || DIFF_ALIGN (vict->align, th->align))
    {
      stt ("You can only guard players of your own alignment!\n\r", th);
      return;
    }

  if (vict == th)
    {
      if (!th->pc->guarding)
	stt ("You weren't guarding anyone!\n\r", th);
      else
	act ("@1n stop@s guarding @3n.", th, NULL, th->pc->guarding, NULL, TO_ALL);
      th->pc->guarding = NULL;
      return;
    }

  th->pc->guarding = vict;
  act ("@1n start@s guarding @3n.\n\r", th, NULL, vict, NULL, TO_ALL);
  return;
}


/* This lets you set your wimpy. */

void
do_wimpy (THING *th, char *arg)
{
  int amount;
  
  if (!IS_PC (th))
    return;

  if (!str_cmp (arg, "max") || !str_cmp (arg, "full"))
    {
      th->pc->wimpy = th->max_hp/3;
      stt ("Ok, your wimpy is set to max.\n\r", th);
      return;
    }
  
  if (!is_number (arg))
    {
      stt ("Wimpy <number> or wimpy full or wimpy max.\n\r", th);
      return;
    }
  
  if ((amount = atoi (arg)) < 0)
    {
      stt ("Your wimpy cannot be negative.\n\r", th);
      return;
    }
  
  if (amount > th->max_hp/3)
    {
      stt ("Your wimpy must be no more than 1/3 of your hps.\n\r", th);
      return;
    }
  
  th->pc->wimpy = amount;
  stt ("Ok, wimpy set.\n\r", th);
  return;
}

void
do_peace (THING *th, char *arg)
{
  THING *rth, *rthn;

  if (LEVEL (th) < MAX_LEVEL || !th->in)
    return;

  for (rth = th->in->cont; rth; rth = rthn)
    {
       rthn = rth->next_cont;
       stop_fighting(rth);
       if (rth->fgt && rth->fgt->hunting_type == HUNT_KILL)
	 stop_hunting (rth, TRUE);
    } 
  return;
}


void
do_capture (THING *th, char *arg)
{
  THING *vict, *prison;
  FLAG *flg;

  for (prison = th->cont; prison; prison = prison->next_cont)
    if (prison->vnum == SOUL_PRISON)
      break;
  
  if (!prison)
    {
      stt ("You don't have a soul prison to capture someone with!\n\r", th);
      return;
    }
  
  if ((vict = find_thing_in_combat (th, arg, FALSE)) == NULL)
    {
      stt ("Capture whom?\n\r", th);
      th->pc->wait += 40;
      return;
    }
  
  if (vict->hp > 50)
    {
      stt ("Your victim is still too healthy!\n\r", th);
      th->pc->wait += 40;
      return;
    }
  
  if (!check_spell (th, "capture", 0) || nr (1,3) != 2)
    {
      stt ("You fail to capture your victim!\n\r", th);
      if (nr (1,5) == 3)
	{
	  stt ("Your soul prison disappears under the stress!\n\r", th);
	  free_thing (prison);
	}
      return;
    }

  act ("$E@1n reach@s out with @1s soul prison and draw@s @3p soul into it!$7", th, NULL, vict, NULL, TO_ALL);
  stop_fighting (vict);
  thing_to (vict, prison);
  
  flg = new_flag ();
  flg->type = FLAG_PC1;
  flg->timer = nr (5,7);
  flg->val = PC_FREEZE;
  aff_update (flg, vict);
  th->pc->wait += 40;
  return;
}


/* This command lets you try to disarm an enemy. You can disarm ANYTHING
   that they're holding. */


void
do_disarm (THING *th, char *arg)
{
  THING *vict, *obj;
  int obj_choices = 0, choice = 0, obj_chose = 0;
  VALUE *wpn;

  /* Is the thing we want to disarm here? */

  if ((vict = find_thing_in_combat (th, arg, FALSE)) == NULL)
    {
      stt ("Disarm who?\n\r", th);
      return;
    }

  /* Are we using a weapon? */

  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if (obj->wear_loc == ITEM_WEAR_WIELD &&
	  (wpn = FNV (obj, VAL_WEAPON)) != NULL)
	break;
    }
  
  if (!obj)
    {
      stt ("You can't disarm someone unless you're using a weapon!\n\r", th);
      return;
    }
  
  /* Is the victim holding something? And if so, pick something to disarm. */

  for (obj = vict->cont; obj; obj = obj->next_cont)
    {
      if (obj->wear_loc == ITEM_WEAR_WIELD)
	obj_choices++;
      if (obj->wear_loc == ITEM_WEAR_NONE)
	break;
    }
  
  if (obj_choices < 1)
    {
      stt ("Your intended victim isn't holding anything!\n\r", th);
      return;
    }
  
  choice = nr(1,obj_choices);
  
  for (obj = vict->cont; obj; obj = obj->next_cont)
    {
      if (obj->wear_loc == ITEM_WEAR_WIELD && ++obj_chose == choice)
	break;
    }
  
  if (!obj)
    {
      stt ("Your intended victim isn't holding anything!\n\r", th);
      return;
    }
  
  
  /* Do we succeed at this? */
  
  if (!check_spell (th, "disarm", 0) ||
      (nr (1,40) < get_stat (vict, STAT_STR) ||
       nr (1,30) > get_stat (th, STAT_STR)))
    {
      act ("@1n try@s to disarm @3n, but fail@s!\n\r", th, NULL, vict, NULL, TO_ALL);
      return;
    }

  
  thing_to (obj, th->in);
  act ("$9@1n disarm@s @3n and send@s @2n$9 flying!$7", th, obj, vict, NULL, TO_ALL);
  if (IS_PC (th))
    th->pc->wait += 20;
  return;
}

/* This lets you assist people in combat. It will let you assist a groupie
   first, then if that isn't true, it will let you assist members of your
   same alignment as long as they're fighting mobs or players of a diff
   alignment with no argument. */


void
do_assist (THING *th, char *arg)
{
  THING *needs_help, *fighting;

  if (!th->in)
    return;

  
  if (!*arg)
    {
      if (!IS_PC (th))
	return;
      for (needs_help = th->in->cont; needs_help; needs_help = needs_help->next_cont)
	{
	  /* They have to be fighting and of the same align and not fighting
	     same aligned pc. */
	  if ((fighting = FIGHTING (needs_help)) == NULL || 
	      DIFF_ALIGN (needs_help->align, th->align) ||
	      (IS_PC (fighting) && !DIFF_ALIGN (fighting->align, th->align)))
	    continue;
	  
	  act ("@1n heroically assist@s @3n!", th, NULL, needs_help, NULL, TO_ALL);
	  multi_hit (th, fighting);
	  return;
	}
      stt ("There's noone here to assist.\n\r", th);
      return;
    }

  if ((needs_help = find_thing_in (th, arg)) == NULL)
    {
      stt ("Assist who?\n\r", th);
      return;
    }
  
  if ((fighting = FIGHTING (needs_help)) == NULL)
    {
      stt ("They aren't fighting!\n\r", th);
      return;
    }
  
  if (DIFF_ALIGN (needs_help->align, th->align))
    {
      stt ("You can't help someone of that alignment!\n\r", th);
      return;
    }
  
  act ("@1n heroically assist@s @3n!", th, NULL, needs_help, NULL, TO_ALL);
  multi_hit (th, fighting);
  return;
}

/* This checks if the victim is a friend of th. The vict is a friend
   if it's of the same align and th is a kill_opp mob, or if they
   are of the same align, the align is not zero (neutral), and
   th is in a society. */

bool 
is_friend (THING *th, THING *vict)
{
  VALUE *soc, *soc2;
  if (!th || IS_PC (th) || !vict || IS_PC (vict) || 
      !CAN_FIGHT (vict) || IS_ACT1_SET (th, ACT_PRISONER))
    return FALSE;
  
  soc = FNV (th, VAL_SOCIETY);
  soc2 = FNV (vict, VAL_SOCIETY);
  
  if (!DIFF_ALIGN (th->align, vict->align))
    {
      if (IS_ACT1_SET (th, ACT_KILL_OPP))
	return TRUE;
      if (!soc)
	{
	  if (!soc2 && (th->vnum == vict->vnum))
	    return TRUE;
	}
      else if (th->align == 0)
	{
	  if (soc2 && (soc2->val[0] == soc->val[0]))
	    return TRUE;
	}
      else
	return TRUE;
    }
  return FALSE;
}

/* This determines if a given thing is an enemy of the thing asking
   the question. A thing is an enemy if it's of opp align and
   th is in a society or if its a kill_opp. Or, if its the same
   align, and th is in a society. There are some other details
   dealing with when certain society members attack. */

bool
is_enemy (THING *th, THING *vict)
{
  SOCIETY *soc, *soc2, *esoc;
  VALUE *socval, *vsocval;
  int actbits = 0;
  bool align_hate_attack = FALSE;
  if (!th || !vict || !th->in || !vict->in || IS_PC (th) || th == vict ||
      IS_SET (vict->thing_flags, TH_NO_FIGHT) ||
      (IS_PC (vict) && IS_PC1_SET (vict, PC_HOLYWALK | PC_HOLYPEACE)))
    return FALSE;
  
  if (IS_PC (vict) && th->align < ALIGN_MAX &&
      vict->pc->align_hate[th->align] >= ALIGN_HATE_ATTACK)
    align_hate_attack = TRUE;
  
  actbits = flagbits (th->flags, FLAG_ACT1);
  
  /* Check for aggressive/angry vs pc's */
  
  if (IS_PC (vict) && !BEING_ATTACKED && /* Yells don't call aggros */
      (IS_SET (actbits, ACT_AGGRESSIVE) ||
       (IS_SET (actbits, ACT_ANGRY) && nr (1,5) == 2)))
    return TRUE;
  
  
  /* If th is kill_opp and th and vict have different aligns, return
     true. */
  
  if (IS_SET (actbits, ACT_KILL_OPP) &&
      (DIFF_ALIGN (th->align, vict->align) ||
       align_hate_attack) &&
      (IS_PC (vict) || vict->align > 0 ||
       CAN_TALK (vict)))
    return TRUE;
  
  /* If th is in a society and its in the proper caste, and if
     the society is in a state that requires fighting, then find
     out if that society will attack that victim. */
  
  /* All society members in battle castes attack anything else
     not in their society on sight! */
  
  if ((socval = FNV (th, VAL_SOCIETY)) != NULL &&
      (soc = find_society_in (th)) != NULL &&
      IS_ROOM (vict->in) &&
      ((IS_SET (socval->val[2], BATTLE_CASTES) &&
	IS_SET (soc->society_flags, SOCIETY_AGGRESSIVE)) ||
       ((IS_SET (soc->society_flags, SOCIETY_XENOPHOBIC) &&
	 socval->val[2] != CASTE_CHILDREN) &&
	vict->in->vnum >= soc->room_start &&
	vict->in->vnum <= (soc->room_end))))
    {
      
      /* At this point, we know that the thing may try to attack,
	 depending on what the status of the victim */
      
      /* If the victim is a prisoner of this society or an ally, don't
	 attack. */
      
      if ((vsocval = FNV (vict, VAL_SOCIETY)) != NULL &&
	  IS_ACT1_SET (vict, ACT_PRISONER) &&
	  (esoc = find_society_num (vsocval->val[5])) != NULL &&
	  /* If vict is following...all bets are off. */
	  (esoc == soc ||
	   (soc->align > 0 && 
	    !DIFF_ALIGN (esoc->align, soc->align))) &&
	  (!FOLLOWING(vict) || 
	   (FOLLOWING(vict)->align > 0 &&
	    !DIFF_ALIGN (FOLLOWING(vict)->align, th->align))))
	return FALSE;
      
      
      /* Neutral societies attack everything that's not in them. */
      
      
      if (soc->align == 0)
	{
	  if ((soc2 = find_society_in (vict)) != soc)
	    return TRUE;
	  return FALSE;
	}
      else  /* Soc align > 0 */
	{
	  if (DIFF_ALIGN (th->align, vict->align) ||
	      align_hate_attack)
	    return TRUE;
	  return FALSE;
	}
    }
  
  return FALSE;
}



/* This function causes a person to do a ground attack on another
   person. */

void 
ground_attack (THING *th)
{
  int num_attacks = 1, i, hit_location;
  int ground_damage, damage_type;
  
  char msg[STD_LEN];
  if (!th)
    return;
  
  if (nr(1,5) == 3 && check_spell (th, NULL, 599 /* Spetznatz */))
    num_attacks++;
   
   
   for (i = 0; i < num_attacks; i++)
     {
       
       if (!th || !FIGHTING(th) || th->position != POSITION_TACKLED ||
	   FIGHTING (th)->position != POSITION_TACKLED)
	 return;
       /* Calc base ground_damage. */
       ground_damage = 10 +nr (0, (get_stat (th, STAT_STR) + get_stat (th, STAT_DEX))/2);
       
       /* Mobs do more ground_damage. */
       if (!IS_PC (th))
	 ground_damage += nr (LEVEL(th)/5, LEVEL(th)/3);
       
       
       /* Skills do more ground_damage. */
       if (check_spell (th, NULL, 594 /* Punch */))
	 ground_damage += nr (0, LEVEL (th)/12);
       if (check_spell (th, NULL, 596 /* Pound */))
	 ground_damage += nr (0, LEVEL (th)/8);
       if (check_spell (th, NULL, 601 /* Pummel */))
	 ground_damage += nr (0, LEVEL (th)/6);
       
       /* Skills absorb/block ground_damage. */
       
       if (check_spell (FIGHTING (th), NULL, 595 /* Absorb Blow */))
	 ground_damage -= nr (1, ground_damage/8);
       if (check_spell (FIGHTING (th), NULL, 600 /* Deflect Blow */))
	 ground_damage -= nr (1, ground_damage/5);
       if (check_spell (FIGHTING (th), NULL, 602 /* Stop Blow */))
	 ground_damage -= nr (1, ground_damage/3);
       
       hit_location = nr (0, PARTS_MAX -1);
       damage_type = nr (0, NUM_GROUNDFIGHT_DAMAGE_TYPES -1);
       
       sprintf (msg, "@1n %s@s @3p %s!", groundfight_damage_types[damage_type],
		parts_names[hit_location]);
       act (msg, th, NULL, FIGHTING(th), NULL, TO_ALL);
       
       damage (th, FIGHTING (th), ground_damage, 
	       groundfight_damage_types[damage_type]);
     }
   return;

}


