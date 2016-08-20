#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>
#include "serv.h"
#include "track.h"
#include "society.h"
#include "rumor.h"


/* This file deals with societies handling their "needs" for certain
   kinds of items. The items have ways in which they are made and
   certain processes used to make them. Societies will assign workers
   to certain tasks to get these things made. */
   
NEED *need_free = NULL;
int need_count = 0;

/* These are the kinds of processes that a society can
   carry out. It sucks but the tools are numbered from 1-N so you
   need to set bits and shift them to set this up correctly. Sorry
   about that but it was easier for the rest of the code to do it
   this way. */

const struct _process_data process_data[PROCESS_MAX] = 
  {
    {"forge", "smith", (1 <<TOOL_HAMMER) },
    {"smelt", "smelter", (1 << TOOL_HAMMER) },
    {"carve", "carver", (1 << TOOL_KNIFE) |  (1 << TOOL_PLANE)},
    {"bake", "baker", (1 << TOOL_PLATE) | (1 << TOOL_PLATE) | (1 << TOOL_BOWL) },
    {"tan", "tanner", (1 << TOOL_KNIFE) | (1 << TOOL_JAR) },
    {"assemble", "crafter", (1 << TOOL_HAMMER) | (1 << TOOL_SAW)},
    {"weave", "weaver", (1 << TOOL_NEEDLE) },
    {"spin", "spinner", (1 << TOOL_NEEDLE) },
    {"shape", "potter", (1 << TOOL_KNIFE)  | (1 << TOOL_BOWL) },
    {"mill", "miller", 0 }, 
    {"butcher", "butcher", (1 << TOOL_KNIFE) },
    {"blow", "glassblower", (1 << TOOL_PIPE) },  
  };

/* These are the kinds of shops that a society will make. */

const struct _society_shop_data society_shop_data[SOCIETY_SHOP_MAX] =
  {
    {"armorer", {VAL_ARMOR, 0, 0}, {{271,30}, {0,0}}},
    {"weaponsmith", {VAL_WEAPON, VAL_RANGED, VAL_AMMO}, {{270,20},{0,0}}},
    {"provisioner", {VAL_TOOL, VAL_LIGHT, VAL_MAP},{{300,5},{349,20}}},
    {"magicseller", {VAL_GEM, VAL_MAGIC, VAL_POWERSHIELD},{{277,15},{0,0}}},
    {"foodseller", {VAL_FOOD, VAL_DRINK, 0},{{702,20},{741,0}}},
  };

const char *need_names[NEED_MAX] = 
  {
    "get",
    "give",
    "make"
  };

/* These create and free needs. */

NEED *
new_need (void)
{
  NEED *need;
  
  if (need_free)
    {
      need = need_free;
      need_free = need_free->next;
    }
  else
    {
      need = (NEED *) mallok (sizeof (NEED));
      need_count++;
    }

  need->next = NULL;
  need->prev = NULL;
  need->th = NULL;
  need->num = 0;
  need->last_updated = 0;
  need->times = 0;
  need->type = 0;
  need->helper_sent = 0;
  return need;
}


void 
free_need (NEED *need)
{
  if (!need)
    return;
  
  if (need->th)
    need_from_thing (need);
  
  need->next = need_free;
  need_free = need;
  return;
}

void
write_need (FILE *f, NEED *need)
{
  if (!f || !need)
    return;
  
  /* THIS IS IMPORTANT...THE "LAST_UPDATED IS SET TO THE 
     CURRENT_TIME-LAST_UPDATED SO WHEN YOU READ THIS BACK IN YOU KNOW
     HOW MANY SECONDS BEFORE THE REBOOT THAT THE UPDATE OCCURED. 
     SAME FOR HELPER_SENT */
  fprintf (f, "NEED %d %d %d %d %d\n",
	   MIN(NEED_UPDATE_TIMER, (current_time - need->last_updated)),
	   need->num, need->times, need->type,
	   MIN(NEED_REQUEST_TIMER, (current_time - need->helper_sent)));
  return;
}

NEED *
read_need (FILE *f)
{
  NEED *need;
  if (!f)
    return NULL;
  
  if (( need = new_need()) == NULL)
    return NULL;
  
  /* NOTE THAT THE LAST UPDATED IS SET TO CURRENT TIME - THE TIME THAT
     WE ALREADY KNEW WAS IN THE NEED!! SAME FOR HELPER_SENT */
  need->last_updated = current_time - read_number (f);
  need->num = read_number (f);
  need->times = read_number (f);
  need->type = read_number (f);
  need->helper_sent = current_time - read_number (f);
  need->next = NULL;
  need->prev = NULL;
  return need;
}
  
/* Adds a need to a society. It does nothing if another need of the
   same type exists in the society. */

void 
need_to_thing (THING *th, NEED *need)
{
  NEED *need2;
  if (!th)
    {
      if (need)
	free_need(need);
      return;
    }
  
  if (!need)
    return;
  
  if (need->th)
    need_from_thing (need);
  need->next = NULL;
  need->prev = NULL;
  need->th = NULL;
  
  for (need2 = th->needs; need2; need2 = need2->next)
    {
      if (need2->type == need->type && 
	  need2->num == need->num)
	break;
    }
  
  /* If the old need exists, either update or ignore. */
  
  if (need2 && need2 != need)
    {
      if (current_time - need2->last_updated >= NEED_UPDATE_TIMER)
	{
	  need2->times += need->times;
	  need2->last_updated = current_time;
	  need_from_thing (need2);
	  /* Possibly badly recursive...*/
	  need_to_thing (th, need2);	    
	}
      free_need (need);
      return;
    }

  /* Old need not found. */

  /* Either society has no needs, or its first need is of lower
     priority. */
  
  if (!th->needs || th->needs->times <= need->times)
    {
      if (th->needs)
	th->needs->prev = need;
      need->th = th;
      need->next = th->needs;      
      need->prev = NULL;
      th->needs = need;
    }
  else /* Otherwise it's not the first slot... */
    {
      for (need2 = th->needs; need2; need2 = need2->next)
	{
	  if (!need2->next || 
	      need2->next->times <= need->times)
	    {
	      need->next = need2->next;
	      if (need2->next)
		need2->next->prev = need;
	      need->prev = need2;	      
	      need2->next = need;
	      need->th = th;
	      break;
	    }
	}
    }
  
  return;
}

/* This removes a need from a thing. IT DOES NOT FREE IT! */

void
need_from_thing (NEED *need)
{
  if (!need || !need->th)
    return;
  
  if (need->next)
    need->next->prev = need->prev;
  if (need->prev)    
    need->prev->next = need->next;
  if (need == need->th->needs)
    need->th->needs = need->next;
  
  need->prev = NULL;
  need->next = NULL; 
  need->th = NULL;
  return;
}

/* This copies all of the needs from one thing to another. */

void
copy_needs (THING *told, THING *tnew)
{
  NEED *need, *needn;

  for (need = told->needs; need; need = needn)
    {
      needn = need->next;
      need_from_thing (need);
      need_to_thing (tnew, need);
    }
  return;
}

/* This removes a need from a thing if it's been satisfied.
   ONLY USE THIS FUNCTION IF THE REASON THE NEED IS BEING REMOVED
   IS THAT IT'S BEEN SATISFIED! */

void
remove_satisfied_need (NEED *need)
{ /* 
  if (need->type >= 0 && need->type < NEED_MAX && need->th &&
      need->th->in)
    {
	       NAME (need->th), need->th->vnum, need->th->in->vnum,
	       need_names[need->type], need->num);
	       } */
  free_need (need);
  return;
}
  

/* These tell what kinds of crafters or shopkeepers that a society
   needs. */

int
find_crafter_needed (SOCIETY *soc)
{
  int num_choices = 0, num_chose = 0, count, choice = 0, i;
  int cpop;
  
  
  /* No society or don't need anything. */
  
  if (!soc)
    return PROCESS_MAX;
  
  cpop = find_num_members (soc, CASTE_CRAFTER);
  
  if (soc->crafters_needed == 0)
    {
      return nr (0, PROCESS_MAX-1);
    }

  for (count = 0; count < 2; count ++)
    {
      for (i = 0; i < PROCESS_MAX; i++)
	{
	  if (IS_SET (soc->crafters_needed, (1 << i)))
	    {
	      if (count == 0)
		num_choices++;
	      else if (++choice == num_chose)
		break;
	    }
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return PROCESS_MAX;
	  
	  num_chose = nr (1, num_choices);
	}
    }
  RBIT (soc->crafters_needed, (1 << i));
  return i;
}

int
find_shop_needed (SOCIETY *soc)
{
  int num_choices = 0, num_chose = 0, count, choice = 0, i;
  
  /* No society or don't need anything. */
  
  if (!soc)
    return SOCIETY_SHOP_MAX;

  
  if (soc->shops_needed == 0)
    return nr (0, SOCIETY_SHOP_MAX -1);

  for (count = 0; count < 2; count ++)
    {
      for (i = 0; i < SOCIETY_SHOP_MAX; i++)
	{
	  if (IS_SET (soc->shops_needed, (1 << i)))
	    {
	      if (count == 0)
		num_choices++;
	      else if (++choice == num_chose)
		break;
	    }
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return SOCIETY_SHOP_MAX;
	  
	  num_chose = nr (1, num_choices);
	}
    }
  RBIT (soc->shops_needed, (1 << i));
  return i;
}


void 
society_crafter_activity (THING *th)
{
  NEED *need, *needn;
  VALUE *socval, *madeof, *tool;
  SOCIETY *soc;
  int i, ctype;
  THING *item_to_make, *made_item, *item;

  /* The tools and things we need. */
  int tools_in_hand = 0;
  int tools_needed;
  /* Which components needed to make the item we have on us. */
  
  /* Do we have all the objects we need? */
  bool have_obj[NUM_VALS];
  bool have_all_objects;
  THING *obj[NUM_VALS];
  
  
  if (!th || !th->in ||
      (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_num (socval->val[0])) == NULL ||
      !IS_SET (socval->val[2], CASTE_CRAFTER))
    {
      if (th)
	{
	  for (need = th->needs; need; need = needn)
	    {
	      needn = need->next;
	      free_need (need);
	    }
	}
      return;
    }


  if (socval->val[3] < 0 || socval->val[3] >= PROCESS_MAX)
    {
      socval->val[3] = find_crafter_needed (soc);
      /* Never too many forgers I guess. :) */
      if (socval->val[3] >= PROCESS_MAX || socval->val[3] < 0)
	socval->val[3] = 0;
      /* May have to reset the name to some new profession. */
      setup_society_maker_name (th);
    }
  
  ctype = socval->val[3];
  /* First find if we have the proper tools here and in hand. */
  
  for (item = th->cont; item; item = item->next_cont)
    {
      /* Only worn objects count?? */
      if ((tool = FNV (item, VAL_TOOL)) == NULL)
	continue;
      tools_in_hand |= (1 << tool->val[0]);
    }
  
  
  tools_needed = process_data[ctype].tools_in_hand & ~tools_in_hand;
  
  if (tools_needed)
    {
      for (i = 0; i < TOOL_MAX; i++)
	{
	  if (IS_SET (tools_needed, (1 << i)))
	    generate_need (th, TOOL_START_VNUM + i, NEED_GET);
	}
      return;
    }
  
	      
  
  /* Now go down the list of things you need to make and
     try to make something. We only make one item per update so 
     that even if there's a problem with using tools as resources,
     we don't have a problem. If you make more than one item per
     update, then make sure that you check for the tools before you 
     make each item. */
  
  for (need = th->needs; need; need = needn)
    {
      needn = need->next;
      
      /* Only deal with make needs here. */
      if (need->type != NEED_MAKE)
	continue;
      
      /* Check if this is a legit item for us to make. If not,
	 don't bother worrying about it anymore. */
      if ((item_to_make = find_thing_num(need->num)) == NULL ||
	  IS_SET (item_to_make->thing_flags, TH_IS_ROOM | TH_IS_AREA) ||
	  (madeof = FNV (item_to_make, VAL_MADEOF)) == NULL ||
	  /* The kind of process needed to make the item must be the
	     kind that we are trained to do. */
	  madeof->val[0] != socval->val[3])
	{
	  free_need (need);
	  continue;
	}

      /* NOTE: down here in the MADEOF value the first number val[0] is
	 the process and the rest are the ingredients. I have made
	 these loops loop from 1 to NUM_VALS -1 for that reason, but
	 wherever possible I've set the 0th piece of data to something
	 safe so that I don't have to worry if for some reason this
	 gets changed to something else that uses the 0th val it
	 won't be as likely to crash. */
      
      /* Find the objects needed to create this item. */
      
      obj[0] = NULL;
      for (i = 1; i < NUM_VALS; i++)
	{
	  if ((obj[i] = find_thing_num (madeof->val[i])) == NULL ||
	      IS_SET (obj[i]->thing_flags, TH_IS_ROOM | TH_IS_AREA))
	    obj[i] = NULL;
	}
      
      /* Set the ones we don't need to already being had. */
      
      have_obj[0] = TRUE;
      for (i = 1; i < NUM_VALS; i++)
	{
	  if (obj[i])
	    have_obj[i] = FALSE;
	  else
	    have_obj[i] = TRUE;
	}
      
      /* Check our inventory for the items we need. */

      for (item = th->cont; item; item = item->next_cont)
	{
	  for (i = 1; i < NUM_VALS; i++)
	    {
	      if (obj[i] && !have_obj[i] &&
		  item->proto == obj[i])
		{
		  have_obj[i] = TRUE;
		}
	    }
	}

      /* If we're missing any of the needed objects then bail out. */
      
      have_all_objects = TRUE;
      for (i = 0; i < NUM_VALS; i++)
	{
	  if (!have_obj[i] && obj[i])
	    {
	      have_all_objects = FALSE;
	      generate_need (th, obj[i]->vnum, NEED_GET);
	    }
	}
      
      if (!have_all_objects)
	continue;

      /* Now we have the tools, we are a crafter and we have the 
	 resource that we're going to put into the item to be created. */
      
      if ((made_item = create_thing (item_to_make->vnum)) == NULL)
	continue;
      
      act ("@1n @t@s @3n!", th, NULL, made_item, process_data[i].name, TO_ALL);
      thing_to (made_item, th);
      for (i = 0; i < NUM_VALS; i++)
	{
	  if (obj[i])
	    {
	      free_thing (obj[i]);
	      obj[i] = NULL;
	    }
	}
      remove_satisfied_need (need);
      
      /* You break out of this since it's possible that one of the things
	 you need will be used up in the process. You can delete this
	 if you wish to have many things made at once. */
      /* You should be able to remove this safely if you don't care
	 as much about consistency or if you want to check tools each
	 time you check a need. */
      break;
    }
  return;
}



/* This shows the needs that a thing has. */

void
see_needs (THING *th, THING *targ)
{
  NEED *need;  
  THING *obj;
  static char buf[STD_LEN];
  VALUE *socval;
  SOCIETY *soc;

  /* The target must be in a society. */
  
  if ((socval = FNV (targ, VAL_SOCIETY)) == NULL ||
      (soc = find_society_num (socval->val[0])) == NULL)
    return;

  for (need = targ->needs; need; need = need->next)
    {
      if (need->type < 0 || need->type >= NEED_MAX ||
	  need->num < 0)
	continue;
      
      buf[0] = '\0';
      if (need->num < VAL_MAX)
	{
	  sprintf (buf, "Needs to %s an item of type %s (%d)\n\r",
		   need_names[need->type],
		   value_list[need->num],
		   need->num);
	}
      else if ((obj = find_thing_num (need->num)) != NULL &&
	       !IS_ROOM (obj) && !IS_AREA (obj))
	{
	  sprintf (buf, "Needs to %s item %d: %s\n\r",
		   need_names[need->type],
		   need->num,
		   NAME(obj));
	}
      stt (buf, th);
    }
  return;
}
      



				     

  /* The final place where all created items go is to the 
     shopkeepers... so when shopkeeprers get made, they have 
     to go to the market. */

void
society_shopkeeper_activity (THING *th)
{  
  
  /* Give this thing a shop if it doesn't have one. */
  society_setup_shopkeeper(th);
  
  return;
}

/* This function makes a mob try to make items. */

void
society_setup_shopkeeper (THING *th)
{
  VALUE *shop, *shop2, *socval, *socval2, *build;
  SOCIETY *soc;
  THING *thg, *room =  NULL, *obj;
  RESET *reset, *resetn, *old_resets;
  int shop_type, val_type, i;
  char shopitems[STD_LEN];
  /* Used to pick which marketplace room to go to. */
  int num_choices = 0, num_chose = 0, choice = 0, count = 0, vnum;
  bool found_room = FALSE;
  
  if (!th || !th->in || (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_num (socval->val[0])) == NULL)
    return;
  
  if ((shop = FNV (th, VAL_SHOP)) == NULL)
    {
      shop = new_value();
      shop->type = VAL_SHOP;
      if (IS_SET (soc->society_flags, SOCIETY_NOSLEEP))
	{
	  shop->val[0] = 0;
	  shop->val[1] = NUM_HOURS;
	}
      else if (IS_SET (soc->society_flags, SOCIETY_NOCTURNAL))
	{
	  shop->val[0] = HOUR_NIGHT;
	  shop->val[1] = HOUR_DAYBREAK;
	}
      else
	{
	  shop->val[0] = HOUR_DAYBREAK;
	  shop->val[1] = HOUR_NIGHT;
	}
      shop->val[2] = 100;
      shop->val[3] = 0;
      shop->val[4] = 0;
      shop->val[5] = nr (10, 15);
      add_value (th, shop);

      /* At this point we need to add resets to this thing. The way
	 I will do it is kind of a hack but basically I remove the
	 resets from the prototype for this thing, then 
	 create new resets based on what kind of shop this is
	 and add them to the proto, then reset this thing, then remove the
	 resets and put the old ones back. */
      
      if (socval->val[3] >= 0 && socval->val[3] < SOCIETY_SHOP_MAX &&
	  th->proto && th->proto->vnum == th->vnum)
	{
	  /* Save old resets. */
	  old_resets = th->proto->resets;
	  th->proto->resets = NULL;
	  
	  /* Put the new resets onto the proto. */
	  for (i = 0; i < SOCIETY_SHOP_RESETS_MAX; i++)
	    {
	      if ((obj = find_thing_num (society_shop_data[socval->val[3]].resets[i][0])) != NULL)
		{
		  reset = new_reset();
		  reset->pct = 50;
		  reset->nest = 1;
		  reset->vnum = obj->vnum;
		  reset->times = society_shop_data[socval->val[3]].resets[i][1];
		  reset->next = th->proto->resets;
		  th->proto->resets = reset;
		}
	    }

	  /* Reset this. */
	  reset_thing (th, 0);
	  
	  /* Clean up the new resets. */
	  for (reset = th->proto->resets; reset; reset = resetn)
	    {
	      resetn = reset->next;
	      free_reset (reset);
	    }
	  
	  /* Put the old resets back. */
	  th->proto->resets = old_resets;
	}
      
    }
  
  if (!shop->word || !*shop->word)
    {
      /* If no items are set up to be bought/sold, set them up.
	 Societies care about 5 kinds of shops so we search for them in
	 order and create them as needed. */
      
      if (socval->val[3] < 0 || socval->val[3] >= SOCIETY_SHOP_MAX)
	{
	  if ((shop_type = find_shop_needed (soc)) == SOCIETY_SHOP_MAX)
	    shop_type = nr (0,4);
	}
      else
	shop_type = socval->val[3];
      
      shopitems[0] = '\0';
      for (i = 0; i < SOCIETY_SHOP_VALS_MAX; i++)
	{
	  val_type = society_shop_data[shop_type].values[i];
	  if (val_type > 0 && val_type < VAL_MAX)
	    {
	      strcat (shopitems, value_list[val_type]);
	      strcat (shopitems, " ");
	    }
	}
      shop->word = new_str (shopitems);
    }
  
  
  
  /* If we are in the market, check for another shopkeeper? */
  if ((th->in && (build = FNV (th->in, VAL_BUILD)) != NULL &&
       build->val[0] == socval->val[0] &&
       IS_SET (build->val[2], CASTE_SHOPKEEPER)))      
    return;
  
  /* At this point we have a word an we are not in the marketplace. We
     must move there. */
  
  for (count = 0; count < 2; count++)
    {
      for (vnum = soc->room_start; vnum < soc->room_end; vnum++)
	{
	  /* Find a marketplace room */
	  if ((room = find_thing_num (vnum)) != NULL &&
	      (build = FNV (room, VAL_BUILD)) != NULL &&
	      build->val[0] == soc->vnum &&
	      IS_SET (build->val[2], CASTE_SHOPKEEPER))
	    {
	      /* Make sure it doesn't have a shopkeeper...*/
	      for (thg = room->cont; thg; thg = thg->next_cont)
		{
		  if ((socval2 = FNV (thg, VAL_SOCIETY)) != NULL &&
		      socval->val[0] == socval2->val[0] &&
		      (shop2 = FNV (thg, VAL_SHOP)) != NULL &&
		      shop2->word && *shop2->word)
		    break;		  
		}
	      if (!thg) /* If no shopkeeper here we can pick this room. */
		{
		  if (count == 0)
		    num_choices++;
		  else if (++choice == num_chose)
		    {
		      found_room = TRUE;
		      break;
		    }
		}
	    }
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    break;
	  num_chose = nr (1, num_choices);
	  choice = 0;
	}
    }
  
  if (room && found_room)
    start_hunting_room (th, room->vnum, HUNT_HEALING);
  return;
}

/* This function updates whether or not a society member needs to
   get something from the society or not. As of now, there are
   only simple needs. */


bool
create_society_member_need (THING *th)
{
  SOCIETY *soc;
  VALUE *socval, *val;
  THING *obj, *shopmob = NULL;
  int type;
  int num_of_that_type = 0;
  
  if (nr (1,50) != 17 || !th || th->needs ||
      (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_num (socval->val[0])) == NULL ||
      IS_ACT1_SET (th, ACT_SENTINEL))
    return FALSE;
  
  if (nr (1,10) == 7)
    type = VAL_FOOD;
  else if (nr (1,5) == 4)
    type = VAL_WEAPON;
  else 
    type = VAL_ARMOR;
  
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if ((val = FNV (obj, type)) != NULL)
	num_of_that_type++;
    }
  
  if (num_of_that_type > 6)
    return FALSE;
  else if (type != VAL_ARMOR && 
	   num_of_that_type > 2)
    return FALSE;
  
  if (type <= 0 || type >= VAL_MAX)
    return FALSE;

  /* Now we know that we don't have enough to just ignore this
     problem, and we must find some way to satisfy these needs. */
  
  /* Look for a shop caste and try to find a creature within it that
     has the proper equipment types to make these items. */


  if ((shopmob = find_shop_to_satisfy_need (soc, type)) != NULL)
    {      
      generate_need (th, type, NEED_GET);  
      start_hunting (th, KEY(shopmob), HUNT_NEED);
      if (th->fgt)
	th->fgt->hunt_victim = shopmob; 
      return TRUE;
    }
  return FALSE;
}

/* This finds if there's a shop that can satisfy a need and if not
   a request is put in to have this shop added to the society shop
   list. */

THING *
find_shop_to_satisfy_need (SOCIETY *soc, int type)
{
  VALUE *shop, *socval2;
  THING *mob = NULL, *shopmob = NULL, *obj = NULL;
  int vnum, i;
  int shop_caste;
  
  if (!soc)
    return NULL;
  
  if (type < 0 || (type >= VAL_MAX && 
		   ((obj = find_thing_num (type)) == NULL ||
		    IS_ROOM (obj) || IS_AREA (obj))))
    return NULL;
  
  /* Search through all castes of mobs for shop mobs that either 
     sell the right kind of item, or they can sell this particular item. */

  for (shop_caste = 0; shop_caste < CASTE_MAX && !shopmob; shop_caste++)
    {
      if (!IS_SET (soc->cflags[shop_caste], CASTE_SHOPKEEPER) ||
	  soc->curr_pop[shop_caste] < 1)
	continue;
      
      for (vnum = soc->start[shop_caste]; vnum <= 
	     soc->start[shop_caste] + soc->max_tier[shop_caste] &&
	     !shopmob; vnum++)
	{
	  for (mob = thing_hash[vnum % HASH_SIZE]; mob; mob = mob->next)
	    {	     
	      if ((socval2 = FNV (mob, VAL_SOCIETY)) != NULL &&
		  socval2->val[0] == soc->vnum &&
		  (shop = FNV (mob, VAL_SHOP)) != NULL &&
		  ((obj && is_valid_shop_item (obj, shop)) ||
		   (type < VAL_MAX && shop->word && *shop->word &&
		    named_in ((char *) value_list[type], shop->word) &&
		    !shopmob)))
		{
		  shopmob = mob;
		  break;
		}
	    }
	}
    }
  
  if (!shopmob || !shopmob->in || !IS_ROOM (shopmob->in))
    {
      for (i = 0; i < SOCIETY_SHOP_MAX; i++)
	{
	  if (!IS_SET (soc->shops_needed, (1 << i)))
	    soc->shops_needed |= (1 << i);
	}
      return NULL;
    }
  return shopmob;
}

/* This tries to find a crafter to either carry out a process if
   type < PROCESS_MAX, or to create an object based on the 
   object's VAL_MADEOF (if the crafter can do that process in the
   VAL_MADEOF or not. */

THING *
find_crafter_to_satisfy_need (SOCIETY *soc, int type)
{
  VALUE *madeof, *socval;
  THING *mob = NULL, *obj = NULL;
  int vnum;
  int craft_caste;
  
  if (!soc)
    return NULL;
  
  if (type < 0 || (type >= PROCESS_MAX && 
		   ((obj = find_thing_num (type)) == NULL ||
		    IS_ROOM (obj) || 
		    IS_AREA (obj) ||
		    (madeof = FNV (obj, VAL_MADEOF)) == NULL ||
		    (type = madeof->val[0]) < 0 || 
		    madeof->val[0] >= PROCESS_MAX)))
    return NULL;
  
  /* Search through all castes of mobs for shop mobs that either 
     sell the right kind of item, or they can sell this particular item. */

  for (craft_caste = 0; craft_caste < CASTE_MAX; craft_caste++)
    {
      if (!IS_SET (soc->cflags[craft_caste], CASTE_CRAFTER) ||
	  soc->curr_pop[craft_caste] < 1)
	continue;
      
      for (vnum = soc->start[craft_caste]; vnum <= 
	     soc->start[craft_caste] + soc->max_tier[craft_caste]; vnum++)
	{
	  for (mob = thing_hash[vnum % HASH_SIZE]; mob; mob = mob->next)
	    {	     
	      if ((socval = FNV (mob, VAL_SOCIETY)) != NULL &&
		  socval->val[0] == soc->vnum && 
		  socval->val[3] == type)
		return mob;
	    }
	}
    }
  
  /* Tell the society that we need this kind of crafter. */
  soc->crafters_needed |= (1 << type);
  return NULL;
}
/* A mob goes to a room and gets there and tries to satisfy some
   need starting with its first one. It then goes down its whole list
   and tries to do as much as possible here before moving on. */
   
void
society_satisfy_needs (THING *th)
{
  THING *mob, *mobn;  /* Mob we interact with. */
  THING *obj = NULL, *objn;  /* The object(s) we're moving around. */
  VALUE *socval, *socval2; /* Society values. */
  VALUE *shop; /* Shop value if needed. */
  VALUE *val; /* Generic value for diff kinds of society needs. */
  VALUE *madeof, *raw;
  NEED *need, *needn, *need2, *need2n, *first_unsatisfied_get_need = NULL;
  SOCIETY *soc;
  bool did_a_need = FALSE;
  /* Crafters only satisfy each other's needs if they have 2 or more
     copies of the tools that they want to give. */
  
  int did_this_need_count;   /* How many times we did this need. */
  int raw_needed = RAW_MAX;
  
  if (!th || !th->in || !th->needs ||
      (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_num (socval->val[0])) == NULL ||

      /* This is important...shopkeepers only let things be done
	 to them. They don't initiate these kinds of activities. */

      IS_SET (socval->val[2], CASTE_SHOPKEEPER))
    return;
  
  
  
  /* We have needs and we're in a society. Now start to check the needs. */ 

  /* Loop through needs. */
  for (need = th->needs; need; need = needn)
    {
      needn = need->next;
      
      did_this_need_count = 0;
      for (mob = th->in->cont; mob; mob = mobn)
	{
	  mobn = mob->next_cont;
	  
	  if (mob == th ||
	      (socval2 = FNV (mob, VAL_SOCIETY)) == NULL ||
	  socval2->val[0] != socval->val[0] ||
	      !IS_SET (socval2->val[2], CASTE_SHOPKEEPER | CASTE_CRAFTER))
	    continue;
	  
	  /* Might be NULL. */
	  shop = FNV (mob, VAL_SHOP);
	  
	  


	  /* If we need to give/get something. */
	  
	  /* If we give, we search ourselves for objects to move
	     to the mob, otherwise we search the mob for items
	     to move to us. */
	  
	  obj = NULL;
	  if (need->type == NEED_GIVE)
	    {
	      obj = th->cont;
	    }
	  else if (need->type == NEED_GET)
	    {
	      obj = mob->cont;
	    }
	  
	  /* Then give all copies of that thing to the mob. */
	  for (; obj; obj = objn)
	    {
	      objn = obj->next_cont;
	      val = NULL;
	      if (need->type == NEED_MAKE ||
		  (need->num < VAL_MAX &&
		   (val = FNV (obj, need->num)) == NULL) ||
		  (need->num >= VAL_MAX && obj->vnum != need->num))
		continue;
	      
	     
	      
	      did_this_need_count++;
	      
	      /* If these are two crafters, don't share unless you
		 have more than one thing to share. */
	      
	      if (IS_SET (socval->val[2], CASTE_CRAFTER) &&
		  IS_SET (socval2->val[2], CASTE_CRAFTER) &&
		  did_this_need_count == 1)
		continue;
	      
	      if (need->type == NEED_GIVE)
		{
		  thing_to (obj, mob);
		  act ("@1n give@s @2n to @3n.", th, obj, mob, NULL, TO_ALL);		      
		}
	      else if (need->type == NEED_GET)
		{
		  thing_to (obj, th);
		  act ("@1n give@s @2n to @3n.", mob, obj, th, NULL, TO_ALL);	    
		}
	      /* If this is a need by value, then break. */
	      if (val)
		break; 

	      /* When crafters share the second item, they stop. */

	      if (IS_SET (socval->val[2], CASTE_CRAFTER) &&
		  IS_SET (socval2->val[2], CASTE_CRAFTER))
		break;
	      
	      /* Craftsmen only go through this once. */
	      
	    }
	  
	  /* If we took care of this need, try to take care of it
	     for the target also. */
	  
	  if (did_this_need_count > 0)
	    {
	      for (need2 = mob->needs; need2; need2 = need2n)
		{
		  need2n = need2->next;
		  
		  if (need2->num == need->num &&			  
		      ((need->type == NEED_GET && 
			need2->type == NEED_GIVE) ||
		       (need->type == NEED_GIVE && 
			need2->type == NEED_GET)))
		    remove_satisfied_need (need2);
		}
	      
	      did_a_need = TRUE;
	      /* Break out of mob loop...to go to next need iteration. */
	      remove_satisfied_need (need);
	      break;
	    } 
	  /* We couldn't help this need directly, but if we CAN help
	     do something about this need eventually, we add new needs 
	     for the mob. */
	  
	  /* The mob can help out if its a shopkeeper and it can 
	     sell objects of the type listed in the need, or if
	     it's a crafter and it can craft the object. */
	  
	  else if (need->type == NEED_GET && nr (1,3) == 2 &&
		   (obj = find_thing_num (need->num)) != NULL)
	    {
	      if ((IS_SET (socval2->val[2], CASTE_SHOPKEEPER) &&
		   shop && shop->word && *shop->word &&
		   ((need->num < VAL_MAX && need->num > 0 &&
		     named_in ((char *)value_list[need->num], shop->word)) ||
		    (is_valid_shop_item (obj, shop)))))
		generate_need (mob, need->num, NEED_GET);
	      
	      if (IS_SET (socval2->val[2], CASTE_CRAFTER) &&
		  (madeof = FNV (obj, VAL_MADEOF)) != NULL &&
		  madeof->val[0] == socval2->val[3])
		generate_need (mob, need->num, NEED_MAKE);
	      
	      /* Make new crafters if needed. */
	      
	      if (nr (1,20) == 3 && obj &&
		  (madeof = FNV (obj, VAL_MADEOF)) &&
		  madeof->val[0] >= 0 && madeof->val[0] < PROCESS_MAX)
		soc->crafters_needed |= (1 << madeof->val[0]);
	      
	      
	      
	    }
	}
      
      
      /* If we need to get something and we haven't been able
	 to do it we should try looking for what we want. */
      if (did_this_need_count == 0 && need->type == NEED_GET)
	{
	  
	  if (first_unsatisfied_get_need == NULL)
	    first_unsatisfied_get_need = need;
	  
	  /* If no needs are satisfied and this is a worker, look for
	 things to get. */
	  
	  if (first_unsatisfied_get_need != NULL &&
	      IS_SET (socval->val[2], CASTE_WORKER))
	    {
	      if (need->num == VAL_RAW)
		raw_needed = raw_needed_by_society(soc);
	      else if ((obj = find_thing_num (need->num)) != NULL &&
		       (raw = FNV (obj, VAL_RAW)) != NULL)
		raw_needed = raw->val[0];	  
	    }
	}
    }
  
  
  /* Search for a mob where you can deal with this need, or search
     for a raw material to go get. Crafters don't go hunting. They
     let other people come to them. */
  if (!did_a_need && !IS_SET (socval->val[2], CASTE_CRAFTER))
    {
      if (first_unsatisfied_get_need && nr (1,3) == 2)
	{
	  need = first_unsatisfied_get_need;
	  if (need->num < VAL_MAX &&
	      (mob = find_shop_to_satisfy_need (soc, need->num)) != NULL)
	    {
	      start_hunting (th, KEY(mob), HUNT_NEED);
	      if (th->fgt)
		th->fgt->hunt_victim = mob;
	      return;
	    }
	  if (need->num >= VAL_MAX &&
	      (mob = find_crafter_to_satisfy_need (soc, need->num)) != NULL &&
	     mob != th)
	    {
	      start_hunting (th, KEY(mob), HUNT_NEED);
	      if (th->fgt)
		th->fgt->hunt_victim = mob;
	      return;
	    }
	}

      if (raw_needed > RAW_NONE && raw_needed < RAW_MAX && nr (1,3) == 2)
	{ 	  
	  socval->val[3] = raw_needed;
	  find_gather_location (th);
	  return;
	}
    }
  
  return;
}
  /* This adds or creates a need for a thing when it finds that
     it needs something. The vnum is the number we need and the
   for is who it has to go to (if not the generator) and the type
   is the kind of need: get, give, make. */


void
generate_need (THING *th, int vnum, int type)
{
  NEED *need;
  THING *proto;
  
  if (!th || !th->in)
    return;

  
  if (vnum >= VAL_MAX &&
      ((proto = find_thing_num (vnum)) == NULL ||
       IS_SET (proto->thing_flags, TH_IS_ROOM | TH_IS_AREA)))
    return;
  
  need = new_need();
  
  need->num = vnum;
  need->type = type;
  need_to_thing (th, need);
  return;
}

/* This scans the crafters and shops for any needs they have
   and this thing tries to satisfy those needs. */


bool
worker_generate_need (THING *th)
{
  SOCIETY *soc;
  THING *mob;
  int cnum, tier;
  VALUE *socval, *socval2;
  NEED *need2, *need2n;
  
  /* Sanity checks plus if you already have things to do, you don't
     get more jobs! */
  
  if (!th || !th->in ||
      (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      !IS_SET (socval->val[2], CASTE_WORKER) ||
      (soc = find_society_num (socval->val[0])) == NULL ||      
      th->needs || nr (1,25) != 2)
    return FALSE;
  
  /* No needs, now scan for something to do. */
  
  for (cnum = 0; cnum < CASTE_MAX; cnum++)
    {
      if (!IS_SET (soc->cflags[cnum], CASTE_SHOPKEEPER | CASTE_CRAFTER))
	continue;
      
      for (tier = 0; tier < soc->max_tier[cnum]; tier++)
	{
	  for (mob = thing_hash[(soc->start[cnum] + tier) % HASH_SIZE]; mob; mob = mob->next)
	    {
	      if ((socval2 = FNV (mob, VAL_SOCIETY)) == NULL ||
		  socval->val[0] != socval2->val[0])
		continue;
	      
	      for (need2 = mob->needs; need2; need2 = need2n)
		{
		  need2n = need2->next;
		  /* If this isn't a "get" need or if it's too soon
		     after someone else tried to help out then 
		     don't use this need. */
		  if (need2->type != NEED_GET ||
		      current_time - need2->helper_sent <  NEED_REQUEST_TIMER ||
		      nr (1,15) != 4)
		    continue;
		  
		  
		  /* Now add needs to get the item and to give it
		     to the thingin need. Then bail out. */
		  
		  generate_need (th, need2->num, NEED_GET);
		  generate_need (th, need2->num, NEED_GIVE);
		  return TRUE;
		}	      
	    }
	}
    }
  return FALSE;
}
	      
	      
/* This makes a worker check to see if they have a need they can
   satisfy with their raw materials and if so go do it. */

bool
dropoff_raws_for_needs (THING *th)
{
  VALUE *socval, *val;
  SOCIETY *soc;
  THING *obj, *objn;
  NEED *need, *needn;
  bool found_this_need;
  
  if (!th || !th->in || !th->needs ||
      (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_num (socval->val[0])) == NULL)
    return FALSE;

  /* Loop through all needs and see if we have anything that we need. */
  for (need = th->needs; need; need = needn)
    {
      needn = need->next;
      
      found_this_need = FALSE;
      if (need->type == NEED_GET)
	{
	  for (obj = th->cont; obj; obj = objn)
	    {
	      objn = obj->next_cont;
	      if (obj->vnum == need->num ||
		  (val = FNV (obj, need->num)) != NULL)
		found_this_need = TRUE;
	    }
	  if (found_this_need)
	    remove_satisfied_need (need);
	}
    }
  
  /* Check for things to give these items to. */
  
  for (need = th->needs; need; need = needn)
    {
      needn = need->next;
      
      if (need->type == NEED_GIVE)
	{
	  for (obj = th->cont; obj; obj = objn)
	    {
	      objn = obj->next_cont;
	      if (obj->vnum == need->num ||
		  (val = FNV (obj, need->num)) != NULL)
		{
		  if (start_hunting_dropoff_mob (th, need))
		    {
		      return TRUE;
		    }
		}
	    }
	}
    }
  return FALSE;
}
  
/* This lets someone with a need to give something a place to
   go and drop it off if at all possible. */

bool
start_hunting_dropoff_mob (THING *th, NEED *need)
{
  SOCIETY *soc;
  VALUE *socval, *socval2;
  THING *mob;
  NEED *need2, *need2n;
  int cnum, tier;
  
  if (!th || !need || !th->in || 
      (socval = FNV (th, VAL_SOCIETY)) == NULL ||
      (soc = find_society_num (socval->val[0])) == NULL ||
      need->type != NEED_GIVE)
    return FALSE;
  
  /* Search through all castes and tiers to find something that
     needs what we have. */
  
  for (cnum = 0; cnum < CASTE_MAX; cnum++)
    {
      if (!IS_SET (soc->cflags[cnum], CASTE_SHOPKEEPER | CASTE_CRAFTER))
	continue;
      
      for (tier = 0; tier < soc->max_tier[cnum]; tier++)
	{
	  for (mob = thing_hash[(soc->start[cnum] + tier) % HASH_SIZE]; mob; mob = mob->next)
	    {
	      if ((socval2 = FNV (mob, VAL_SOCIETY)) == NULL ||
		  socval->val[0] != socval2->val[0])
		continue;
	      
	      for (need2 = mob->needs; need2; need2 = need2n)
		{
		  need2n = need2->next;
		  if (need2->num == need->num &&
		      need2->type == NEED_GET)
		    {
		      stop_hunting (th, TRUE);
		      start_hunting (th, KEY(mob), HUNT_NEED);
		      if (th->fgt)
			th->fgt->hunt_victim = mob;
		      /* Yes jumping out in the middle is bad but
			 for now I don't need to do anything after
			 this. */
		      return TRUE;
		    }
		}
	    }
	}
    }
  return FALSE;
}
