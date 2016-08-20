#include<stdio.h>
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#include "serv.h"
#include "society.h"
#include "objectgen.h"
#include "detailgen.h"


/* This generates provisions for the game. 
   The generators are in the provisiongen.are area and the idea is to
   go down the list of objects in the area and one by one attempt to
   generate using each combination of elements from each list as 
   a name for the item(s). 

   It uses the same name generation code as the detailgen code so it's
   kind of a hack, but it's also good in that it doesn't require me to
   recode the same stuff. */


void
generate_provisions (THING *th)
{
  THING *proto; /* Which object we're using to generate the current
		 name. */
  THING *obj; /* New object being created. */
  
  THING *old_obj; /* Used for checking the name of the obj vs
		     the other names in the area to avoid repeats...*/
  THING *proto_area, *load_area; /* Area where the prototypes are and
				     where the finished objects will go. */
  VALUE *val, *newval;
  /* For setting up the randpop item. */
  THING *randpop_item, *start_area;
  VALUE *randpop;
  int times, max_times; /* Number of times objects are made from a single
		proto object. */
  int min_vnum, max_vnum, curr_vnum;
  int name_times; /* Try to generate names several times. */
  
  bool name_is_ok = FALSE;
  if ((proto_area = find_thing_num (PROVISIONGEN_AREA_VNUM)) == NULL ||
      (load_area = find_thing_num (PROVISION_LOAD_AREA_VNUM)) == NULL ||
      (load_area->cont && load_area->cont->next_cont))
    {
      stt ("Either the provisions base area or the load area don't exist, or the load area isn't empty.\n\r", th);
      return;
    }

  min_vnum = load_area->vnum + load_area->mv + 1;
  max_vnum = load_area->vnum + load_area->max_mv -1;
  curr_vnum = min_vnum;
  load_area->thing_flags |= TH_CHANGED;
  
  for (proto = proto_area->cont; proto; proto = proto->next_cont)
    {
      
      if (IS_ROOM (proto) || 
	  ((val = proto->values) == NULL &&
	   IS_SET (proto->thing_flags, TH_NO_CONTAIN)))
	continue;
      
      /* Make sure that the object has a value of the correct type.
	 These values don't need to be filled in, just need to exist. */
      if (IS_SET (proto->thing_flags, TH_NO_CONTAIN) &&
	  (!val ||
	   (val->type != VAL_FOOD && 
	    val->type != VAL_DRINK &&
	    val->type != VAL_MAP &&
	    val->type != VAL_TOOL)))
	continue;
      
      /* Make up to max_timesitems of each type...*/
      max_times = nr (30,50);
      for (times = 0; times < max_times; times++)
	{
	  /* Find an open vnum...next one up SHOULD be ok...*/
	  if (find_thing_num (curr_vnum))
	    curr_vnum++;
	  if (find_thing_num (curr_vnum))
	    {
	      for (curr_vnum = min_vnum; curr_vnum <= max_vnum; curr_vnum++)
		{
		  if (find_thing_num(curr_vnum) == NULL)
		    break;
		}
	    }
	  if (curr_vnum >= max_vnum)
	    break;
	  /* Assume curr_vnum exists and is in the proper range now. */

	  obj = new_thing ();
	  if (proto->thing_flags)
	    obj->thing_flags = proto->thing_flags;
	  else
	    obj->thing_flags = OBJ_SETUP_FLAGS;
	  
	  obj->vnum = curr_vnum;
	  obj->wear_pos = ITEM_WEAR_NONE;
	  obj->level = nr (10,30);
	  name_times = 0;
	  do
	    {	      
	      name_is_ok = TRUE;
	      generate_detail_name (proto, obj);
	      
	      /* Check the new name vs old names. */
	      for (old_obj = load_area->cont; old_obj; old_obj = old_obj->next_cont)
		{
		  if (!str_cmp (obj->short_desc, old_obj->short_desc))
		    {
		      name_is_ok = FALSE;
		      break;
		    }
		}
	    }
	  while (!name_is_ok && ++name_times < 20);
	  /* If you can't make a unique name, then bail out. */
	  if (!name_is_ok)
	    free_thing (obj);

	  
	  /* So the name is ok. */

	  /* Now find the kind of item it is. */

	  if (val)
	    {
	      
	      if (val->type == VAL_FOOD)
		{
		  newval = new_value();
		  newval->type = VAL_FOOD;
		  add_value (obj, newval);
		  obj->height = nr (1,12);
		  obj->weight = nr (1,15);
		  obj->cost = nr (1,200);
		  newval->val[0] = nr (1,15);
		  if (nr (1,10) == 3)
		    newval->val[0] += nr (5,15);
		  /* Add a spell. */
		  if (nr (1,54) == 22)
		    {
		      newval->val[3] = nr (1,150);
		      if (nr (1,7) == 3)
			{
			  newval->val[4] = nr (1,150);
			  if (nr (1,3) == 2)
			    newval->val[5] = nr (1,150);
			}
		    }
		}
	      
	      if (val->type == VAL_DRINK)
		{
		  newval = new_value();
		  newval->type = VAL_DRINK;
		  add_value (obj, newval);
		  obj->height = nr (4,16);
		  obj->weight = nr (3,30);
		  obj->cost = nr (50,600);
		  newval->val[0] = nr (1,15);
		  if (nr (1,10) == 3)
		    newval->val[0] += nr (5,15);
		  /* Add a spell. */
		  if (nr (1,54) == 22)	       
		    {
		      newval->val[3] = nr (1,150);
		      if (nr (1,7) == 3)
			{
			  newval->val[4] = nr (1,150);
			  if (nr (1,3) == 2)
			    newval->val[5] = nr (1,150);
			}
		    }
		}
	      
	      if (val->type == VAL_MAP)
		{
		  THING *area, *room;
		  
		  newval = new_value();
		  newval->type = VAL_MAP;
		  add_value (obj, newval);
		  if ((area = find_random_area (AREA_NOLIST|AREA_NOSETTLE|AREA_NOREPOP)) != NULL)
		    {
		      if ((room = find_random_room (area, FALSE, 0, 0)) != NULL)
			{
			  char buf[STD_LEN];
			  newval->val[0] = room->vnum;
			  free_str (obj->desc);
			  sprintf (buf, "%s %s near %s.\n",
				   (nr (1,3) == 1 ? "The" :
				    (nr (1,2) == 1 ? "A map of the" :
				     "A drawing of the")),
				   (nr(1,4) == 2? "environs" :
				    (nr (1,3) == 1 ? "region" :
				     (nr (1,2) == 2 ? "area" : "wilderness"))),
				   area->short_desc);
			  obj->desc = new_str (buf);
			}		      
		    }
		}
	      if (val->type == VAL_TOOL)
		{
		  /* Don't do anything for now...probably will make the
		     item have the same type as the original tool and
		     a random number of charges or uses. */
		}
	    }
	  
	  /* Deal with containers...*/
	  if (!IS_SET (obj->thing_flags, TH_NO_CONTAIN))
	    {
	      obj->size = nr (50,100);
	      if (is_named (obj, "small"))
		obj->size /= 2;
	      if (is_named (obj, "tiny"))
		obj->size /= 3;
	      if (is_named (obj, "little"))
		obj->size /= 2;
	      if (is_named (obj, "large"))
		obj->size *= 2;
	      if (is_named (obj, "giant") || is_named (obj, "huge"))
		obj->size *= 3;
	    }	     	
	  obj->next_cont = NULL;
	  thing_to (obj, load_area);
	  add_thing_to_list (obj);
	}
    }
  /* Now set up the randpop item. */
  if ((start_area = find_thing_num (START_AREA_VNUM)) != NULL)
    start_area->thing_flags |= TH_CHANGED;
  if ((randpop_item = find_thing_num (PROVISION_RANDPOP_VNUM)) == NULL
      && start_area)
    {
      randpop_item = new_thing();
      randpop_item->vnum = PROVISION_RANDPOP_VNUM;
      randpop_item->thing_flags = OBJ_SETUP_FLAGS;
      randpop_item->name = new_str ("provision provisionpop");
      randpop_item->short_desc = new_str ("provisions randpop item");
      if (find_thing_num (START_AREA_VNUM))
	{
	  thing_to (randpop_item, start_area);
	  add_thing_to_list (randpop_item);
	}
    }
 
  if (randpop_item)
    {
      if ((randpop = FNV (randpop_item, VAL_RANDPOP)) == NULL)
	{
	  randpop = new_value();
	  randpop->type = VAL_RANDPOP;
	  add_value (randpop_item, randpop);
	}
      randpop->val[0] = min_vnum;
      randpop->val[1] = curr_vnum - min_vnum + 1;
      randpop->val[2] = 1;
    }
  

  return;
}


/* This clears all of the provisions from the provisions area for
   the next reboot. */


void
clear_provisions (THING *th)
{
  THING *area, *obj;


  if ((area = find_thing_num (PROVISION_LOAD_AREA_VNUM)) == NULL ||
      !IS_AREA (area))
    return;

  area->thing_flags |= TH_CHANGED;
  for (obj = area->cont; obj; obj = obj->next_cont)
    {
      if (!IS_ROOM (obj))
	obj->thing_flags |= TH_NUKE;
    }
  stt ("Provisions area cleared.\n\r", th);
  return;
}
    
  
  
