#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"



void 
do_buy (THING *th, char *arg)
{
  THING *keeper, *obj = NULL;
  VALUE *shop = NULL, *shop2;
  char buf[STD_LEN];
  char arg1[STD_LEN];
  THING *proto;
  int i;
  int prefix_offset = 0; /* Offset due to 5.leather. */
  int current_prefix_offset;
  int item_offset = 0;    /* Which item you buy (buy item 20) */
  int current_item_offset; 
  int total_count = 1;   /* Total number of items to buy. */
  char *itemname = nonstr; /* The final name of the item. */
  int shoptime = wt_info->val[WVAL_HOUR];
  int item_cost;
  if (!th || !th->in)
    return;
  
  
  for (keeper = th->in->cont; keeper; keeper = keeper->next_cont)
    {
      if (keeper == th)
	continue;
      if ((shop = FNV (keeper, VAL_SHOP)) != NULL)
	break;
    }
  if (!keeper || !shop)
    {
      stt ("There is no shop here!\n\r", th);
      return;
    }
  if (!can_see (keeper, th))
    {
      do_say (keeper, "I won't do business with someone I can't see.");
      return;
    }
  if ((shop->val[1] > shop->val[0] && (shoptime < shop->val[0] || shoptime > shop->val[1]))  || (shop->val[1] < shop->val[0] && (shoptime < shop->val[0] && shoptime > shop->val[1])))
    {
      stt ("Sorry, we're closed.\n\r", th);
      return;
    }

  arg = f_word (arg, arg1);
  
  if (is_number (arg1))
    {      
      if (!*arg) /* Buy number 10. */
	item_offset = atoi (arg1);
      else /* Arg is the name, arg1 is the number to buy. */
	{
	  total_count = atoi (arg1);
	  itemname = find_first_number (arg, &prefix_offset);
	}
    }
  else
    {
      itemname = find_first_number (arg1, &prefix_offset);
    }
  
  for (i = 0; i < total_count; i++)
    {
      current_prefix_offset = 0;
      current_item_offset = 0;
      for (obj = keeper->cont; obj; obj = obj->next_cont)
	{
	  if (!is_valid_shop_item (obj, shop) ||
	      (*itemname && !is_named (obj, itemname)))
	    continue;
	  if (prefix_offset > 0 && ++current_prefix_offset < prefix_offset)
	    continue;
	  if (item_offset > 0 && ++current_item_offset < item_offset)
	    continue;
	  
	  /* Cost is 150pct of item cost. */
	  item_cost = find_item_cost (th, obj, keeper, SHOP_COST_BUY);
	  
	  if ((shop2 = FNV (keeper, VAL_SHOP2)) != NULL &&
	      !str_cmp (shop2->word, th->name))
	    {
	      stt ("You own the shop, so here you are!\n\r", th);
	    }
	  else if (total_money (th) < item_cost)
	    {
	      sprintf (buf, "This item costs %d coins, and you only have %d on you.\n\r", item_cost, total_money (th));
	      stt (buf, th);
	      return;
	    }
	  else
	    {
	      sub_money (th, item_cost);
	      add_money (keeper, item_cost);
	    }
	  act ("@1n buy@s @2n from @3n.", th, obj, keeper, NULL, TO_ALL);
	  thing_from (obj);
	  if ((proto = find_thing_num (obj->vnum)) != NULL &&
	      proto->in && IS_AREA (proto->in))
	    {
	      proto->cost++;
	      SBIT (proto->in->thing_flags, TH_CHANGED);
	    }
	  if (CAN_MOVE (obj))
	    {
	      VALUE *pet;
	      
	      if (!IS_PC (th))
		{
		  free_thing (obj);
		  return;
		}
	      thing_to (obj, th->in);
	      if ((pet = FNV (obj, VAL_PET)) == NULL)
		{
		  pet = new_value ();
		  pet->type = VAL_PET;
		  add_value (obj, pet);
		}
	      obj->align = th->align;
	      set_value_word (pet, NAME (th));
	      do_follow (obj, NAME (th));
	    }
	  else
	    thing_to (obj, th);
	  break;
	}
      if (!obj)
	{
	  stt ("The shopkeeper doesn't have that for sale!\n\r", th);
	  return;
	}
    }
  return;
}
 
	

void 
do_sell (THING *th, char *arg)
{
  THING *keeper, *obj;
  VALUE *shop = NULL;
  THING *proto;
  char buf[STD_LEN];
  int shoptime = wt_info->val[WVAL_HOUR];
  int item_cost;
  if (!th || !th->in)
    return;


  for (keeper = th->in->cont; keeper; keeper = keeper->next_cont)
    {
      if (keeper == th)
	continue;
      if ((shop = FNV (keeper, VAL_SHOP)) != NULL)
	break;
    }
  if (!keeper || !shop)
    {
      stt ("There is no shop here!\n\r", th);
      return;
    }
    
  if (!can_see (keeper, th))
    {
      do_say (keeper, "I won't do business with someone I can't see.");
      return;
    } 
  if ((shop->val[1] > shop->val[0] && (shoptime < shop->val[0] || shoptime > shop->val[1]))  || (shop->val[1] < shop->val[0] && (shoptime < shop->val[0] && shoptime > shop->val[1])))
    {
      stt ("Sorry, we're closed.\n\r", th);
      return;
    } 
  if ((obj = find_thing_me (th, arg)) == NULL)
    {
      stt ("You don't have that to sell.\n\r", th);
      return;
    }
  if (!is_valid_shop_item (obj, shop))
    {
      stt ("You can't sell that here!\n\r", th);
      return;
    }
  if (obj->cost < 1)
    {
      stt ("That isn't even worth anything!\n\r", th);
      return;
    }
  item_cost = find_item_cost (th, obj, keeper, SHOP_COST_SELL);
  if (total_money (keeper) < item_cost)
    {
      stt ("The shopkeeper does not have enough money to buy that!\n\r", th);
      return;
    }
  
  thing_to (obj, keeper);
  if ((proto = find_thing_num (obj->vnum)) != NULL &&
      proto->in && IS_AREA (proto->in))
    {
      proto->cost = MAX (2, (proto->cost -1));
      SBIT (proto->in->thing_flags, TH_CHANGED);
    }
  add_money (th, item_cost);
  sub_money (keeper, item_cost);
  
  act ("@1n sell@s @2n to @3n.", th, obj, keeper, NULL, TO_ALL);
  sprintf (buf, "You receive %d coins.\n\r",  item_cost);
  stt (buf, th);
  return;
  

}

void 
do_list (THING *th, char *arg)
{
  THING *keeper;
  VALUE *shop = NULL;
  int shoptime = wt_info->val[WVAL_HOUR];
  if (!th || !th->in)
    return;


  for (keeper = th->in->cont; keeper; keeper = keeper->next_cont)
    {
      if (keeper == th)
	continue;
      if ((shop = FNV (keeper, VAL_SHOP)) != NULL)
	break;
    }
  if (!keeper || !shop)
    {
      stt ("There is no shop here!\n\r", th);
      return;
    }

  if (!can_see (keeper, th))
    {
      do_say (keeper, "I won't do business with someone I can't see.");
      return;
    } 
  if ((shop->val[1] > shop->val[0] && (shoptime < shop->val[0] || shoptime > shop->val[1]))  || (shop->val[1] < shop->val[0] && (shoptime < shop->val[0] && shoptime > shop->val[1])))
    {
      stt ("Sorry, we're closed.\n\r", th);
      return;
    }
  show_contents_list (th, keeper, LOOK_SHOW_SHOP);
  return;
  	       
}

void 
do_appraise (THING *th, char *arg) 
{
  THING *keeper, *item;
  VALUE *shop = NULL;
  char buf[STD_LEN];

  int shoptime = wt_info->val[WVAL_HOUR];
  if (!th || !th->in)
    return;


  for (keeper = th->in->cont; keeper; keeper = keeper->next_cont)
    {
      if (keeper == th)
	continue;
      if ((shop = FNV (keeper, VAL_SHOP)) != NULL)
	break;
    }
  if (!keeper || !shop)
    {
      stt ("There is no shop here!\n\r", th);
      return;
    }
  
  if ((item = find_thing_me (th, arg)) == NULL)
    {
      stt ("Appraise what?\n\r", th);
      return;
    }

  if ((shop->val[1] > shop->val[0] && (shoptime < shop->val[0] || shoptime > shop->val[1]))  || (shop->val[1] < shop->val[0] && (shoptime < shop->val[0] && shoptime > shop->val[1])))
    {
      stt ("Sorry, we're closed.\n\r", th);
      return;
    }

  if (!is_valid_shop_item (item, shop))
    {
      stt ("I don't know how to evaluate that item. You should try another shop.\n\r", th);
      return;
    }
  
  sprintf (buf, "Your %s appears to be worth about %d coins, %s.\n\r", NAME (item), find_item_cost (th, item, keeper, SHOP_COST_SELL), NAME (th));
  stt (buf, th);
  return;


}
  
/* This checks to see if ANY value on the item matches up with
   ANY of the words in the shop values list. */


bool
is_valid_shop_item (THING *item, VALUE *shop)
{
  VALUE *itemval;
  if (!shop || shop->type != VAL_SHOP || !item)
    return FALSE;
  
  if (!item->values)
    return TRUE;

  for (itemval = item->values; itemval; itemval = itemval->next)
    {
      if (itemval->type >= VAL_MAX)
	continue;
      if (named_in (shop->word, (char *) value_list[itemval->type]))
	{
	  return TRUE;
	}
    }
  return FALSE;
}

/* Repair items... */

void 
do_repair (THING *th, char *arg) 
{
  THING *keeper, *item, *start_item;
  VALUE *shop = NULL, *shop2;
  char buf[STD_LEN];
  char arg1[STD_LEN];
  bool all = FALSE, repairing_now = FALSE, done_once = FALSE, is_your_shop = FALSE;
  int shoptime = wt_info->val[WVAL_HOUR], cost, cmoney = 0;

  if (!th || !th->in || !IS_PC (th))
    return;

  for (keeper = th->in->cont; keeper; keeper = keeper->next_cont)
    {
      if (keeper == th)
	continue;
      if ((shop = FNV (keeper, VAL_SHOP)) != NULL)
	break;
    }
  if (!keeper || !shop)
    {
      stt ("There is no shop here!\n\r", th);
      return;
    }
  
  if ((shop->val[1] > shop->val[0] && (shoptime < shop->val[0] || shoptime > shop->val[1]))  || (shop->val[1] < shop->val[0] && (shoptime < shop->val[0] && shoptime > shop->val[1])))
    {
      stt ("Sorry, we're closed.\n\r", th);
      return;
    }
  if ((shop2 = FNV (keeper, VAL_SHOP2)) != NULL &&
      !str_cmp (shop2->word, th->name))
    is_your_shop = TRUE;
  arg = f_word (arg, arg1);
  
  if (!str_cmp (arg1, "all"))
    {
      start_item = th->cont;
      all = TRUE;
    }
  else 
    {
      if ((item = find_thing_me (th, arg1)) == NULL &&
	  (item = find_thing_worn (th, arg1)) == NULL)
	{
	  stt ("Repair what?\n\r", th);
	  return;
	}
      start_item = item;
    }
  if (!str_cmp (arg, "yes"))
    repairing_now = TRUE;  
  cmoney = total_money (th);
  for (item = start_item; item && (all || !done_once); item = item->next_cont)
    {
      done_once = TRUE;
      if (!is_valid_shop_item (item, shop))
	{
	  if (!all)
	    {
	      stt ("I don't know how to repair that!\n\r", th);
	      return;
	    }
	  continue;
	}
      if (item->hp >= item->max_hp || item->max_hp == 0)
	{
	  if (!all)
	    {
	      stt ("That item needs no repair.\n\r", th);
	      return;
	    }
	  continue;
	}
      if (is_your_shop)
	cost = 0;
      else
	cost = ((item->max_hp - item->hp)*item->cost)/item->max_hp/3 + 1;
      cost -= cost *guild_rank (th, GUILD_TINKER)/10;
      if (!repairing_now)
	{
	  sprintf (buf, "I will repair your %s for %d coins.\n\r", 
		   NAME (item), cost);
	  stt (buf, th);
	}
      else
	{
	  if (cost > cmoney)
	    {
	      stt ("You don't have enough money to repair this!\n\r",th);
	      return;
	    }	  
	  sub_money (th, cost);
	  cmoney -= cost;
	  act ("@1n repair@s @3p @2n!", keeper, item, th, NULL, TO_ALL);
	  if (item->max_hp > 3)
	    item->max_hp--;
	  item->hp = item->max_hp;
	}
    }
  fix_pc (th);
  return;
}


/* Resize armors...syntax is like repair. */

void 
do_resize (THING *th, char *arg) 
{
  THING *keeper, *item, *start_item;
  VALUE *shop = NULL, *armor, *shop2;
  char buf[STD_LEN];
  char arg1[STD_LEN];
  bool all = FALSE, resizing_now = FALSE, done_once = FALSE, is_your_shop = FALSE;
  int shoptime = wt_info->val[WVAL_HOUR], cost, cmoney = 0;

  if (!th || !th->in || !IS_PC (th))
    return;

  for (keeper = th->in->cont; keeper; keeper = keeper->next_cont)
    {
      if (keeper == th)
	continue;
      if ((shop = FNV (keeper, VAL_SHOP)) != NULL)
	break;
    }
  if (!keeper || !shop)
    {
      stt ("There is no shop here!\n\r", th);
      return;
    }
  
  if ((shop->val[1] > shop->val[0] && (shoptime < shop->val[0] || shoptime > shop->val[1]))  || (shop->val[1] < shop->val[0] && (shoptime < shop->val[0] && shoptime > shop->val[1])))
    {
      stt ("Sorry, we're closed.\n\r", th);
      return;
    }
  
  if ((shop2 = FNV (keeper, VAL_SHOP2)) != NULL &&
      !str_cmp (shop2->word, th->name))
    is_your_shop = TRUE;
  arg = f_word (arg, arg1);
  
  if (!str_cmp (arg1, "all"))
    {
      start_item = th->cont;
      all = TRUE;
    }
  else 
    {
      if ((item = find_thing_me (th, arg1)) == NULL &&
	  (item = find_thing_worn (th, arg1)) == NULL)
	{
	  stt ("Resize what?\n\r", th);
	  return;
	}
      start_item = item;
    }
  if (!str_cmp (arg, "yes"))
    resizing_now = TRUE;  
  cmoney = total_money (th);
  for (item = start_item; item && (all || !done_once); item = item->next_cont)
    {
      done_once = TRUE;
      if (!is_valid_shop_item (item, shop))
	{
	  if (!all)
	    {
	      stt ("I don't know how to resize that!\n\r", th);
	      return;
	    }
	  continue;
	}
      if ((armor = FNV (item, VAL_ARMOR)) == NULL)
	{
	  if (!all)
	    {
	      stt ("That is not armor, so it needs no resizing!\n\r", th);
	      return;
	    }
	  continue;
	}
      if (is_your_shop)
	cost = 0;
      else
	cost = item->cost/5;
      if (armor->val[4] >= (int) th->height * 4/5 &&
	  armor->val[4] <= (int) th->height * 6/5)
	{
	  if (!all)
	    {
	      stt ("That item doesn't need any resizing!\n\r", th);
	    }
	  continue;
	}
      if (!resizing_now)
	{
	  sprintf (buf, "I will resize your %s for %d coins.\n\r", 
		   NAME (item), cost);
	  stt (buf, th);
	}
      else
	{
	  if (cost > cmoney)
	    {
	      stt ("You don't have enough money to resize this!\n\r",th);
	      return;
	    }	  
	  sub_money (th, cost);
	  cmoney -= cost;
	  act ("@1n resize@s @3p @2n!", keeper, item, th, NULL, TO_ALL);
	  armor->val[4] = th->height;
	}
    }
  fix_pc (th);
  return;
}

/* Skim lets you remove some of the profits from your shop. :) */

void
do_skim (THING *th, char *arg)
{
  THING *keeper;
  VALUE *shop = NULL, *shop2 = NULL;
  char arg1[STD_LEN];
  int keeper_money = 0, amount = 0, percent;
  bool all = FALSE;
  

  if (!IS_PC (th) || !th->in)
    return;

  for (keeper = th->in->cont; keeper; keeper = keeper->next_cont)
    {
      if ((shop = FNV (keeper, VAL_SHOP)) != NULL &&
	  (shop2 = FNV (keeper, VAL_SHOP2)) != NULL &&
	  !str_cmp (shop2->word, th->name))
	break;
    }
  
  if (!shop || !keeper || !shop2)
    {
      stt ("You don't own a shop here!\n\r", th);
      return;
    }
 
  keeper_money = total_money (keeper);

  if (!*arg || !str_cmp (arg, "all"))
    amount = keeper_money;
  else
    {
      arg = f_word (arg, arg1);      
      if (!is_number (arg1))
	all = TRUE;
      else if (!str_cmp (arg, "percent"))
	{
	  percent = atoi (arg1);
	  percent = MID (0, percent, 100);
	  amount = keeper_money * percent/100;
	}
      else
	amount = MID (0, atoi (arg1), keeper_money);
    }
  sprintf (arg1, "%d coins", amount);
  act ("@1n skim@s @t from @2n.", th, keeper, NULL, arg1, TO_ALL);
  sub_money (keeper, amount);
  add_money (th, amount);
  return;
}

 

/* Manage lets you take over a shop, given that you have enough money.
   it clears the inventory and the money in the shop. If you attack it,
   it no longer works for you. The shop must be currently unowned or
   it is very expensive to buy it. */


void
do_manage (THING *th, char *arg)
{
  THING *keeper, *obj, *objn;
  VALUE *shop, *shop2;
  char arg1[STD_LEN];
  char buf[STD_LEN];
  int cost = 0, your_money = 0;
  bool buying_now = FALSE;

  if (!IS_PC (th))
    return;
  
  arg = f_word (arg, arg1);
  if (!str_cmp (arg, "yes"))
    buying_now = TRUE;
  
  if ((keeper = find_thing_in (th, arg1)) == NULL)
    {
      stt ("That thing isn't here to manage.\n\r", th);
      return;
    }
  
  if ((shop = FNV (keeper, VAL_SHOP)) == NULL)
    {
      stt ("That thing doesn't run a shop!\n\r", th);
      return;
    }
      
  if ((shop2 = FNV (keeper, VAL_SHOP2)) == NULL)
    {
      stt ("That thing cannot be managed by anyone!!!\n\r", th);
      return;
    }
  
  cost = calculate_shop_value (keeper, shop, shop2);
  your_money = total_money (th);
  
  if (!buying_now || (your_money < cost))
    {
      sprintf (buf, "This shop costs %d coins to buy, and you currently have %d coins on you.\n\r", cost, your_money);
      stt (buf, th);
      return;
    }
  
  sub_money (th, cost);
  
  set_value_word (shop2, KEY(th));
  shop2->val[1]++;
  shop2->val[2] = cost;
  shop2->val[0] = cost;
  sub_money (keeper, total_money (keeper));
  for (obj = keeper->cont; obj; obj = objn)
    {
      objn = obj->next_cont;
      free_thing (obj);
    }
  act ("@1n begin@s to manage the shop run by @2n.", th, keeper, NULL, NULL, TO_ALL);
  return;

}


/* This tallies up how much it will cost for a person to buy a shop.
   It is based on the maximum amount of revenue + money the shop
   has ever had, and if the shop is owned. */


int
calculate_shop_value (THING *keeper, VALUE *shop, VALUE *shop2)
{
  THING *obj;
  int cost = 20000;
  
  if (!keeper || !shop || !shop2)
    return 0;
  
 
  cost += 10 * total_money (keeper);
  for (obj = keeper->cont; obj; obj = obj->next_cont)
    {
      if (is_valid_shop_item (obj, shop))
	cost += 5*obj->cost;
    }
  
  if (shop2->word && *shop2->word)
    cost *= 3;
  
  if (shop2->val[1] > 1)
    cost *= shop2->val[1];
  
  cost = MAX (cost, shop2->val[0]);
  shop2->val[0] = cost;
  return cost;
}


/* This lets you compare two weapons to see which does more damage
   on average. */

void
do_compare (THING *th, char *arg)
{
  char arg1[STD_LEN];
  THING *obj1, *obj2;
  int ave1 = 0, ave2 = 0, armv1 = 0, armv2 = 0, i;
  bool found = FALSE;
  VALUE *wpn1, *wpn2, *arm1, *arm2;
 
  arg = f_word (arg, arg1);
  
  if ((obj1 = find_thing_me (th, arg1)) == NULL ||
      (obj2 = find_thing_me (th, arg)) == NULL)
    {
      stt ("Compare <item 1> <item 2>\n\r", th);
      return;
    }
  
  if (obj1 == obj2)
    {
      stt ("You can't compare something to itself.\n\r", th);
      return;
    }
  if((wpn1 = FNV (obj1, VAL_WEAPON)) != NULL)
    ave1 = wpn1->val[0] + wpn1->val[1];
  if((wpn2 = FNV (obj2, VAL_WEAPON)) != NULL)
    ave2 = wpn2->val[0] + wpn2->val[1];

  if ((arm1 = FNV (obj1, VAL_ARMOR)) != NULL)
    {
      for (i = 0; i < PARTS_MAX; i++)
	armv1 += arm1->val[i];
    }
  
   if ((arm2 = FNV (obj2, VAL_ARMOR)) != NULL)
    {
      for (i = 0; i < PARTS_MAX; i++)
	armv2 += arm2->val[i];
    }
  
  
  
   if (wpn1 && wpn2)
     {
       if (ave1 == ave2)
	 act ("@2n and @3n appear to be about the same.", th, obj1, obj2, NULL, TO_CHAR);
       else if (ave1 < ave2)
	 act ("@2n appears to be better than @3n.", th, obj2, obj1, NULL, TO_CHAR);
       else 
	 act ("@2n appears to be worse than @3n.", th, obj2, obj1, NULL, TO_CHAR);
       
       if (wpn1->val[3] != wpn2->val[3])
	 act ("@2n can be used @t than @3n.", th, obj1, obj2, (char *) (wpn1->val[3] > wpn2->val[3] ? "faster" : "slower"), TO_CHAR);
       found = TRUE;
     }
   
   if (arm1 && arm2)
     {
       if (obj1->wear_pos != obj2->wear_pos)
	 {
	   stt ("Those armors can't be compared since they don't protect the same location.\n\r", th);
	 }
       else
	 {
	   if (armv1 == armv2)
	     act ("@2n and @3n appear to be about the same.", th, obj1, obj2, NULL, TO_CHAR);
	   else if (armv1 < armv2)
	     act ("@2n appears to be better than @3n.", th, obj2, obj1, NULL, TO_CHAR);
	   else 
	     act ("@2n appears to be worse than @3n.", th, obj2, obj1, NULL, TO_CHAR);
	 }
       found = TRUE;
     }
   
   if (!found)
     {
       stt ("You can only compare two weapons or two armors.\n\r", th);
       return;
     }
   
  return;
}


/* This retuns an item cost based on the thing asking for the item, the
   item in question, and the shop in question. */

int
find_item_cost (THING *th, THING *item, THING *shopkeeper, int type)
{
  int cost;
  VALUE *shop;
  
  if (!th || !item || !shopkeeper || 
      (shop = FNV (shopkeeper, VAL_SHOP)) == NULL ||
      (type != SHOP_COST_BUY && type != SHOP_COST_SELL))
    return 0;
  
  /* Get the base cost. */
  cost = item->cost*shop->val[2]/100;
  /* If you're selling, make it worth less, and add some back for
     charisma. */
  if (type == SHOP_COST_SELL)
    {
      cost = cost * 2/3;
      cost += cost * (get_stat(th, STAT_CHA)-STAT_MAX/2)/200;
    }
  /* When buying, charisma makes things cost less. */
  else if (type == SHOP_COST_BUY)
    {
      cost -= cost * (get_stat (th, STAT_CHA)-STAT_MAX/2)/200;
    }
  return cost;
}
