#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"




void
do_wear (THING *th, char *arg)
{
  THING *obj, *item, *itemn;
  
  if (FIGHTING (th))
    {
      stt ("Not while you are fighting!!\n\r", th);
      return;
    }
  if (th->position < POSITION_FIGHTING)
    {
      stt ("You can't do that right now!\n\r", th);
      return;
    }
  

  if (!str_cmp (arg, "all"))
    {
      for (item = th->cont; item; item = itemn)
	{
	  itemn = item->next_cont;
	  if (IS_WORN(item))
	    continue;
	  wear_thing (th, item);
	}
      return;
    }

  
  
  if ((obj = find_thing_me (th, arg)) == NULL)
    {
      stt ("You don't have that item to wear!\n\r", th);
      return;
    }
  
  if (obj->wear_pos == ITEM_WEAR_NONE)
    {
      stt ("That thing can't be worn.\n\r", th);
      return;
    }
  
  if (IS_WORN(obj))
    {
      stt ("That item is already being worn!\n\r", th);
      return;
    }
  
  wear_thing (th, obj);
  return;
}
  
void
wear_thing (THING *th, THING *obj) 
{
  int slot = -1, loc = ITEM_WEAR_NONE, num = 0, i;
  FLAG *aff;
  VALUE *armor, *power, *gem;

  /* Check that the object can be worn and is not being worn */
  
  if (!obj || !th || obj->in != th || 
      (loc = obj->wear_pos) == ITEM_WEAR_NONE || IS_WORN(obj))
    return;
  
  /* Find a free slot */
  
  if ((slot = find_free_eq_slot(th, obj)) == -1)
    {   
      act ("You can't find a place to wear @3n.", th, NULL, obj, NULL, TO_CHAR);
      return;
    }
  if ((armor = FNV (obj, VAL_ARMOR)) != NULL && IS_PC (th))
    { /* Builders/admins reset size to 0, and others must check size. */ 
      if (armor->val[4] == 0 || IS_PC1_SET (th, PC_HOLYWALK))
	armor->val[4] = th->height;
      else if ((armor->val[4] - th->height) > th->height/5)
	{
	  act ("@2n is too large and falls right off!", th, obj, NULL, NULL, TO_CHAR);
	  return;
	}
      else if ((th->height - armor->val[4]) > th->height/5)
	{
	  act ("@2n is too small for you to wear!", th, obj, NULL, NULL, TO_CHAR);
	  return;
	}
    }
  thing_from (obj);
  obj->wear_loc = loc;
  obj->wear_num = slot;
  thing_to (obj, th);
  
  /* Update affects and such here.... */


  /* Add modifiers... */

  if (IS_PC (th))
    {
      for (aff = obj->flags; aff; aff = aff->next)
	{
	  if (aff->type >= AFF_START && aff->type < AFF_MAX)
	    {
	      th->pc->aff[aff->type] += aff->val;
	      if (aff->type == FLAG_AFF_HP)
		th->max_hp += aff->val;
	      if (aff->type == FLAG_AFF_MV)
		th->max_mv += aff->val;
	      
	    }
	}
    }
  
  /* Drain gems/powershields */
  
  if ((gem = FNV (obj, VAL_GEM)) != NULL)
    {
      gem->val[1] = 0;
      stt ("Your gem drains as you hold it.\n\r", th);
      if (IS_PC (th))
	th->pc->mana = 0;
    }
  if ((power = FNV (obj, VAL_POWERSHIELD)) != NULL)
    {
      power->val[1] = 0;
      stt ("Your powershield drains as it begins floating.\n\r", th);
    }

  /* Add/remove armor values. */

  if (armor)
    {
      
      if (IS_PC (th))
	{
	  for (i = 0; i < PARTS_MAX; i++)
	    {
	      
	      num = (obj->max_hp > 0 ? armor->val[i]*obj->hp/obj->max_hp :
		     armor->val[i]);
	      if (IS_PC (th))
		th->pc->armor[i] += num;
	    }
	}
      else
	{
	  for (i = 0; i < PARTS_MAX; i++)
	    th->armor += armor->val[i]*ARMOR_DIVISOR/3;
	}
    }
  

  if (loc == ITEM_WEAR_SHIELD)
    {
      act ("@1n wear@s @3n as a shield.", th, NULL, obj, NULL, TO_ALL);
    }
  else if (loc == ITEM_WEAR_WIELD)
    {
      act ("@1n wield@s @3n.", th, NULL, obj, NULL, TO_ALL);
    }
  else if (loc == ITEM_WEAR_ABOUT)
    {
      act ("@1n wear@s @3n about @1s body.", th, NULL, obj, NULL, TO_ALL);
    }
  else if (loc == ITEM_WEAR_FLOAT)
    {
      act ("@3n starts floating about @1p head.", th, NULL, obj, NULL, TO_ALL);
    }
  else 
    {  
      act ("@1n wear@s @3n on @1s @t.", th, NULL, obj, (char *) wear_data[loc].name, TO_ALL);
    }

  update_kde (th, KDE_UPDATE_HMV | KDE_UPDATE_STAT | KDE_UPDATE_COMBAT);
  return;
}




int 
find_free_eq_slot (THING *th, THING *obj)
{
  int slots_used = 0, i;
  THING *item;
  int num_slots = 0,  wloc = 0, num_shields = 0,
    num_weapons = 0,  num_held = 0, num_hands;
  VALUE *wpn;
  
  if ((wloc = obj->wear_pos) == ITEM_WEAR_NONE)
    return -1;
  
  num_slots = find_max_slots (th, wloc);
  num_hands = find_max_slots (th, ITEM_WEAR_WIELD);
  
  
  for (item = th->cont; item; item = item->next_cont)
    {
      if (!IS_WORN(item))
	break;
      if (item->wear_loc == wloc)
	SBIT (slots_used, (1 << item->wear_num));
      if (item->wear_loc == ITEM_WEAR_SHIELD)
	num_shields++;
      if (item->wear_loc == ITEM_WEAR_WIELD)
	{
	  /* Count all wpns */
	  if ((wpn = FNV (item, VAL_WEAPON)) != NULL 
	      || (wpn = FNV (item, VAL_RANGED)) != NULL)
	    {
	      num_weapons++;
	      /* Count twice if its two-handed */
	      if (IS_OBJ_SET (item, OBJ_TWO_HANDED))
		num_weapons++;
	    }
	  else
	    num_held++;
	}
    }
  
  if (wloc == ITEM_WEAR_WIELD || wloc == ITEM_WEAR_SHIELD)
    {
      if (wloc == ITEM_WEAR_WIELD)
	{
	  if ((wpn = FNV (obj, VAL_WEAPON)) != NULL ||
	      (wpn = FNV (obj, VAL_RANGED)) != NULL)
	    {
	      num_weapons++;
	      if (IS_OBJ_SET (obj, OBJ_TWO_HANDED))
		num_weapons++;
	    }
	  else
	    num_held++;
	}
      else if (wloc == ITEM_WEAR_SHIELD)
	num_shields++;
      
      if (num_weapons + MAX (num_shields, num_held) > num_hands)
	{
	  stt ("Too many shields and held items!\n\r", th);
	  return -1;
	}
    }
  for (i = 0; i < num_slots; i++)
    {
      if (!IS_SET (slots_used, (1 << i)))
	return i;
    }


  return -1;
}
      

void
do_remove (THING *th, char *arg)
{
  THING *obj, *objn;


  if (FIGHTING (th))
    {
      stt ("Not while you are fighting!!!\n\r", th);
      return;
    }
  


  if ((!str_cmp ("all", arg)))
    {
      for (obj = th->cont; obj; obj = objn)
	{
	  objn = obj->next_cont;
	  remove_thing (th, obj, FALSE);
	}
      return;
    }
  
  if ((obj = find_thing_worn (th, arg)) == NULL)
    {
      stt ("You don't have that item!\n\r", th);
      return;
    }
  
  if (!IS_WORN(obj))
    {
      stt ("That item isn't even equipped now!\n\r", th);
      return;
    }
  
  remove_thing (th, obj, FALSE);
  return;
  
}




void
remove_thing (THING *th, THING *obj, bool guaranteed)
{
  FLAG *aff;
  char buf[STD_LEN];
  VALUE *armor, *gem, *power;
  int num, i;
  
  if (!obj || !th || obj->in != th || !IS_WORN(obj))
    return;
  
  
  if (!guaranteed && IS_SET (obj->thing_flags, TH_NO_PRY | TH_NO_REMOVE))
    {
      sprintf (buf, "You can't seem to remove %s", NAME (obj));
      stt (buf, th);
      return;
    }
  
  thing_to (obj, th);
  /* Drain gems/powershields */
  
  if ((gem = FNV (obj, VAL_GEM)) != NULL)
    {
      gem->val[1] = 0;
      stt ("Your gem drains as you remove it.\n\r", th);
    }
  if ((power = FNV (obj, VAL_POWERSHIELD)) != NULL)
    {
      power->val[1] = 0;
      stt ("Your powershield drains as it stops floating.\n\r", th);
    }
  

  /* Remove armor values */
  
  if ((armor = FNV (obj, VAL_ARMOR)) != NULL)
    {
      if (IS_PC (th))
	{
	  for (i = 0; i < PARTS_MAX; i++)
	    {
	      num = (obj->max_hp > 0 ? armor->val[i]*obj->hp/obj->max_hp :
		     armor->val[i]);
	      if (IS_PC (th))
		th->pc->armor[i] -= num;
	    }
	}
      else
	{
	  for (i = 0; i < PARTS_MAX; i++)
	    th->armor -= armor->val[i]*ARMOR_DIVISOR/3;
	}
    }
  
  /* Remove modifiers... */

  if (IS_PC (th))
    {
      for (aff = obj->flags; aff; aff = aff->next)
	{
	  if (aff->type >= AFF_START && aff->type < AFF_MAX)
	    {
	      th->pc->aff[aff->type] -= aff->val;
	      if (aff->type == FLAG_AFF_HP)
		th->max_hp -= aff->val;
	      if (aff->type == FLAG_AFF_MV)
		th->max_mv -= aff->val;
	    }
	} 
    }
  
  obj->wear_loc = ITEM_WEAR_NONE;
  obj->wear_num = 0;
  if (!guaranteed)
    act ("@1n remove@s @3n.", th, NULL, obj, NULL, TO_ALL);
  update_kde (th, KDE_UPDATE_HMV | KDE_UPDATE_STAT | KDE_UPDATE_COMBAT);
  return;
}


THING *
find_eq (THING *th, int loc, int num)
{
  THING *obj;
  if (loc < 1 || loc >= ITEM_WEAR_MAX)
    return NULL;
  
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if (!IS_WORN(obj))
	break;
      if (obj->wear_loc == loc && obj->wear_num == num)
	return obj;
    }
  return NULL;
}



/* Moo, I hope this looks the same as the old armor function, since
   I am making it up as I go along :) */

void
do_armor (THING *th, char *arg)
{
  char buf[STD_LEN];
  int total = 0, curr;
  
  if (!IS_PC (th))
    return;

  curr = th->pc->armor[PART_HEAD] + th->armor/ARMOR_DIVISOR;
  total += curr;
  sprintf (buf, "\n\r         O      <------------- Head armor     %s\n\r", armor_name (curr));
  stt (buf, th);
  curr = th->pc->armor[PART_ARMS] + th->armor/ARMOR_DIVISOR;
  total += curr;
  sprintf (buf, "      /--|--\\   <------------- Arm armor      %s\n\r", armor_name (curr));
  stt (buf, th);
  curr = th->pc->armor[PART_BODY] + th->armor/ARMOR_DIVISOR;
  total += curr;
  sprintf (buf, "         |      <------------- Body Armor     %s\n\r", armor_name (curr));
  stt (buf, th);
  curr = th->pc->armor[PART_LEGS] + th->armor/ARMOR_DIVISOR;
  total += curr;
  sprintf (buf, "       _/ \\_    <------------- Leg Armor      %s\n\r", armor_name (curr));
  stt (buf, th);
  stt ("                                             ----------------------\n\r", th);
  sprintf (buf, "Your average armor is                         %s.\n\r", armor_name (total/PARTS_MAX));
  stt (buf, th);  
  return;
}
  

char *
armor_name (int val)
{
  if (val < 2)
    return "Defenseless";
  if (val < 5)
    return "Barely Armored";
  if (val < 10)
    return "Poorly Armored";
  if (val < 17)
    return "Fairly Armored";
  if (val < 23)
    return "Armored";
  if (val < 31)
    return "Well Armored";
  if (val < 39)
    return "Very Well Armored";
  if (val < 48)
    return "Extremely Well Armored";
  if (val < 58)
    return "Superbly Armored";
  if (val < 68)
    return "Incredibly Armored";
  if (val < 78)
    return "Divinely Armored";
  return "Woohoo!!!! Keeb Alert!";
}


int
find_max_slots (THING *th, int wloc)
{
  if (wloc <= ITEM_WEAR_NONE || wloc >= ITEM_WEAR_MAX)
    return 0;
  
  return (IS_PC (th) ? FRACE(th)->parts[wear_data[wloc].part_of_thing] +
	  FALIGN(th)->parts[wear_data[wloc].part_of_thing] : 1)
    * wear_data[wloc].how_many_worn;
}

/* This finds your weight. */

void
do_weight (THING *th, char *arg)
{
  stt (string_parse (th, "You are carrying @cn@ item(s) (Weight: @ca@) - @cp@\n\r"), th);
  return;
}
	   
/* This lets you sheath held items in your belt. */

void
do_sheath (THING *th, char *arg)
{
  THING *obj, *sheathobj;
  int num_belts = 0, num_sheathed = 0, slots_used = 0, i;
  
  if (th->position != POSITION_STANDING)
    {
      stt ("You must be standing to sheath something.\n\r", th);
      return;
    }

  /* Do we have this item? */

  if ((sheathobj = find_thing_worn (th, arg)) == NULL)
    {
      stt ("You don't have that item!\n\r", th);
      return;
    }

  /* Is it wielded? */
  
  if (sheathobj->wear_loc != ITEM_WEAR_WIELD)
    {
      stt ("You must be wielding an object to sheath it!\n\r", th);
      return;
    }
 
  /* Make sure you can't cheat this way. */

  if (IS_SET (sheathobj->thing_flags, TH_NO_PRY | TH_NO_REMOVE))
    {
      stt ("You can't seem to let go of it!\n\r", th);
      return;
    }
  
  /* Do we have any belt slots left? */

  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if (obj->wear_loc == ITEM_WEAR_WAIST)
	num_belts++;
      if (obj->wear_loc == ITEM_WEAR_BELT)
	{
	  slots_used |= (1 << obj->wear_num);
	  num_sheathed++;
	}
    }
  
  if (num_sheathed > 2*num_belts - 1)
    {
      stt ("You can only have 2 sheathed items per belt!\n\r", th);
      return;
    }
  
  for (i = 0; i < 2 * num_belts; i++)
    {
      if (!IS_SET (slots_used, (1 << i)))
	break;
    }
  
  

  act ("@1n sheath@s @1s @2n.", th, sheathobj, NULL, NULL, TO_ALL);
  thing_from (sheathobj);
  sheathobj->wear_loc = ITEM_WEAR_BELT;
  
  sheathobj->wear_num = i;
  thing_to (sheathobj, th);
}


/* This lets you draw an item from your belt to put in your hand. */

void
do_draw (THING *th, char *arg)
{
  THING *obj;
  int slot;
  
  if (th->position != POSITION_STANDING)
    {
      stt ("You must be standing to draw something from your belt.\n\r", th);
      return;
    }

  if (!*arg)
    {
      for (obj = th->cont; obj; obj = obj->next_cont)
	{
	  if (obj->wear_loc == ITEM_WEAR_BELT)
	    break;
	}
      if (!obj)
	{
	  stt ("You don't have anything sheathed!\n\r", th);
	  return;
	}
    }
  else if ((obj = find_thing_worn (th, arg)) == NULL)
    {
      stt ("Draw <object>\n\r", th);
      return;
    }
  
  if (obj->wear_loc != ITEM_WEAR_BELT)
    {
      stt ("You can only draw items from your belt!\n\r", th);
      return;
    }
  
  if ((slot = find_free_eq_slot (th, obj)) == -1)
    {
      stt ("Your hands are too full!\n\r", th);
      return;
    }
  thing_from (obj);
  obj->wear_loc = ITEM_WEAR_WIELD;
  obj->wear_num = slot;
  thing_to (obj, th);
  act ("@1n draw@s @2n from @1s belt.", th, obj, NULL, NULL, TO_ALL);
  return;
}

  
/* This lets things pick up things and try to wear them. */
/* Maybe it should let them try to pick up and use ranged wpns if
   they're warriors in a society. Dunno. */

void
find_eq_to_wear (THING *th)
{
  THING *obj = NULL, *objn = NULL, *equip = NULL;
  int armorsum = 0, weaponsum = 0, curr_armorsum, curr_weaponsum;
  bool new_is_better = FALSE;
  VALUE *armor, *weapon = NULL, *curr_armor, *curr_weapon, *money;
  VALUE *raw = NULL, *socval = NULL, *shopval;
  int pass; /* Two passes. 1. Check my eq. 2. Check ground here. */
  bool is_worker = FALSE;
  int corpse_count = 0;
  char buf[STD_LEN];
  /* The thing must be smart and able to move things. */
  
  if (!th || !th->in  || !IS_ROOM (th->in) || IS_PC (th) || 
      IS_SET (th->thing_flags, TH_NO_TALK | TH_NO_MOVE_OTHER | 
	      TH_NO_MOVE_SELF | TH_NO_FIGHT) ||
      /* Shopkeeprers don't pick stuff up and wear it. */
      (shopval = FNV (th, VAL_SHOP)) != NULL)
    return;
  
  /* Once in a while they dump the junk in their inventories out. */

  if ((socval = FNV (th, VAL_SOCIETY)) != NULL &&
      IS_SET (socval->val[2], CASTE_WORKER))
    is_worker = TRUE;
  else if (nr (1,164) == 37)
    {
      do_wear (th, "all");
      do_drop (th, "all");      
      do_get (th, "all coins");
      return;
    }
  if (nr (1,25) == 13)
    {
      do_wear (th, "all");
      return;
    }
  else if (nr (1,35) == 18)
    {
      do_get (th, "all");
      do_drop (th, "all.corpse");
      return;
    }
  else if (nr (1,23) == 2)
    {
  /* Remove nonwpn/nonarmor items. */
      
      for (obj = th->cont; obj; obj = objn)
	{
	  objn = obj->next_cont;
	  if (obj->wear_loc != ITEM_WEAR_NONE &&
	      (armor = FNV (obj, VAL_ARMOR)) == NULL &&
	      (weapon = FNV (obj, VAL_WEAPON)) == NULL)
	    remove_thing (th, obj, TRUE);
	}
      return;
    }
	 
  if ((money = FNV (th->in, VAL_MONEY)) != NULL &&
      nr (1,8) == 3)
    {
      do_get (th, "money");
      return;
    }
  

  for (pass = 0; pass < 2; pass++)
    {
      if (pass == 0)
	obj = th->cont;
      else
	obj = th->in->cont;
      
      for (;obj; obj = objn)
	{
	  objn = obj->next_cont;
	  
	  if (pass == 0)
	    {
	      if (!is_worker && 
		  (raw = FNV (obj, VAL_RAW)) != NULL)
		{
		  if (IS_WORN(obj))
		    remove_thing (th, obj, TRUE);
		  do_drop (th, NAME(obj));
		}
	      if (IS_WORN (obj))
		continue;
	    }
	  /* Loot corpses. Don't loot player corpses. */
	  if (obj->vnum == CORPSE_VNUM || is_named (obj, "corpse"))
	    {
	      corpse_count++;
	      if (nr (1,15) == 3 &&
		  /* Don't let them take player corpses. */
		  !IS_SET (obj->thing_flags, TH_NO_TAKE_BY_OTHER))	
		{
		  if (!obj->cont)
		    {
		      sprintf (buf, "%d.corpse", corpse_count);
		      do_sacrifice (th, buf);
		      break;
		    }
		  else
		    {
		      sprintf (buf, "all %d.corpse", corpse_count);
		      do_get (th, buf);
		      break;
		    }
		}
	    }
	  
	  /* Small chance to pick this, and the item needs to be armor or wpn. */
	  
	  if (nr (1,24) != 3 || CAN_MOVE (obj) ||
	      ((armor = FNV (obj, VAL_ARMOR)) == NULL &&
	       (weapon = FNV (obj, VAL_WEAPON)) == NULL &&
	       (!is_worker || pass == 0 ||
		(raw = FNV (obj, VAL_RAW)) == NULL)) ||
	      obj->wear_pos <= ITEM_WEAR_NONE ||
	      obj->wear_pos >= ITEM_WEAR_MAX)
	    continue;
	  
	  /* Workers pick up raw materials. */

	  if (is_worker && raw)
	    {
	      new_is_better = TRUE;
	      break;
	    }
	  
	  /* Things pick up armors...but only if they have
	     3 armor or more...no crap eq used. */

	  armorsum = 0;
	  if (armor)
	    armorsum = find_item_power (armor);
	  
	  weaponsum = 0;
	  if (weapon)
	    weaponsum = find_item_power (weapon);
	  
	  /* If the armor and weapon values aren't high enough, don't
	     consider this equipment. If the item isn't a raw material
	     and either there's no armor, or the armor isn't good and
	     there's either no weapon or the weapon isn't that good, don't
	     pick this item. */
	  
	  if (!new_is_better && 
	      (!armor || armorsum < 2) &&
	      (!weapon || weaponsum < 12))
	    continue;
	  
	  
	      
	  /* Have armor or weapon at this point. See if we have space
	     to wear it or not. Check quality. */
	  
	  if ((find_free_eq_slot (th, obj)) >= 0)
	    {
	      new_is_better = TRUE;
	      break;
	    }
	  
	  for (equip = th->cont; equip; equip = equip->next_cont)
	    {
	      if (!IS_WORN (equip))
		break;
	      if (equip->wear_pos == obj->wear_pos)
		{
		  /* Check if the new armor is better than the current
		     armor. */
		  
		  if (armor && (curr_armor = FNV (equip, VAL_ARMOR)) != NULL)
		    {
		      curr_armorsum = find_item_power (curr_armor);
		      
		      if (curr_armorsum < armorsum)
			{
			  new_is_better = TRUE;
			  break;
			}
		    }
		  
		  if (weapon && obj->wear_pos == ITEM_WEAR_WIELD &&
		      (curr_weapon = FNV (equip, VAL_WEAPON)) != NULL)
		    {
		      curr_weaponsum = find_item_power (curr_weapon);
		      
		      if (curr_weaponsum < weaponsum)
			{
			  new_is_better = TRUE;
			  break;
			}
		    }
		}
	    }
	  
	  if (new_is_better)
	    break;	  
	}
      if (new_is_better && obj)
	{
	  if (equip && equip->wear_loc == obj->wear_loc)
	    {
	      act ("@1n remove@s @3n.", th, NULL, equip, NULL, TO_ALL);
	      remove_thing (th, equip, TRUE);
	    }
	  if (obj->in != th &&
	      (move_thing (th, obj, obj->in, th)))
	    {
	      act ("@1n get@s @3n.", th, NULL, obj, NULL, TO_ALL);
	      if (!is_worker)
		wear_thing (th, obj);
	    }
	  else if (!is_worker) 
	    wear_thing (th, obj);
	  break;
	}
      else if (obj && nr (1,2) == 2 && obj->in == th &&
	       obj->wear_loc == ITEM_WEAR_NONE)
	{
	  free_thing (obj);
	  break;
	}
    }
  
  return;
}


/* This function takes a value of a certain type and returns
   the "power" of the item of that type. */

int
find_item_power (VALUE *val)
{
  int i;

  if (!val)
    return 0;
  
  if (val->type == VAL_WEAPON)
    return val->val[0]+val->val[1];

  if (val->type == VAL_ARMOR)
    {
      int power = 0;
      for (i = 0; i < PARTS_MAX; i++)
	power += val->val[i];
      return power;
    }
  return 0;
}
	







