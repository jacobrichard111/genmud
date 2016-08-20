#include<stdio.h>
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#include "serv.h"
#include "craft.h"

/* I started thinking about this more seriously after I saw how much
   crafting they have in ATITD...:) I hate busywork so this is an
   attempt at general crafting code. */

/* Table of crafting commands. */
static CMD *crafting_commands[256];

/* This clears the craft items. */

void
clear_craft_items (THING *th)
{
  THING *area, *obj;

  if ((area = find_thing_num (CRAFT_LOAD_AREA_VNUM)) == NULL)
    {
      stt ("The craft load area doesn't exist.\n\r", th);
      return;
    }

  for (obj = area->cont; obj; obj = obj->next_cont)
    {
      if (IS_ROOM (obj))
	continue;
      obj->thing_flags |= TH_NUKE;
    }
  area->thing_flags |= TH_CHANGED;
  stt ("Craft items cleared.\n\r", th);
  return;
}

/* This generates craft items from the lists given in
   area CRAFTGEN_AREA_VNUM. */

void
generate_craft_items (THING *th)
{
  THING *proto_area, *load_area, *proto, *obj, *base_proto;
  
  int min_vnum, max_vnum, curr_vnum;
  
  bool noname;   /* Don't use base resource names for this...just make one. */
  bool plural;   /* Make this a plural name. Some foo vs a foo. */
  bool base;     /* Is this a base item or not? */
  char *arg;
  char basename[STD_LEN];
  char name[STD_LEN];
  char itemname[STD_LEN];
  char sdesc[STD_LEN];  
  int pass; /* Two passes at this...base objects first, then the
	       other ones second. */
  char *wd, word[STD_LEN];
  char *wd2, word2[STD_LEN];
  char *ln, line[STD_LEN];
  int num_unmatched_words;
  THING *replace_by_item;
  bool matched_name;
  bool using_made_item;
  EDESC *edesc;
  THING *proto2;
  EDESC *process;
  VALUE *craft, *craft2;
  VALUE *replace_by; /* Used for making items revert to previous
			states. */
  THING *obj2, *made_from_item = NULL;
  
  if ((proto_area = find_thing_num (CRAFTGEN_AREA_VNUM)) == NULL)
    {
      stt ("The craftgen area doesn't exist!\n\r", th);
      return;
    }
  
  if ((load_area = find_thing_num (CRAFT_LOAD_AREA_VNUM)) == NULL)
    {
      stt ("The craft load area doesn't exist!\n\r", th);
      return;
    }
  load_area->thing_flags |= TH_CHANGED;
  
  /* Loop through the objects in the proto area and make objects
     in the load area based on those protos. */
  
  min_vnum = load_area->vnum + load_area->mv + 1;
  curr_vnum = min_vnum;
  max_vnum = load_area->vnum + load_area->max_mv;
  
  for (pass = 0; pass < 2; pass++)
    {
      for (proto = proto_area->cont; proto; proto = proto->next_cont)
	{
	  if (IS_ROOM (proto) || !proto->name || !*proto->name)
	    continue;
	  
	  
	  
	  base = FALSE;
	  noname = FALSE;
	  plural = FALSE;
	  
	  /* Set up some things dealing with generation. */
	  
	  if ((edesc = find_edesc_thing (proto, "gen", TRUE)) != NULL)
	    {
	      arg = edesc->desc;
	      while (arg && *arg)
		{
		  arg = f_word (arg, word);
		  if (!str_cmp (word, "base"))
		    base = TRUE;
		  else if (!str_cmp (word, "noname"))
		    noname = TRUE;
		  else if (!str_cmp (word, "plural"))
		    plural = TRUE;
		}
	    }
	  
	  if ((base && (pass == 1)) ||
	      (!base && (pass == 0)))
	    continue;

	  /* Have to find the base item for the thing that
	     makes this item...only allow one so the names can
	     work better. The base item is determined by looking at the
	     "made" arguments of the crafting commands. */
	  
	  if (base)
	    base_proto = proto;
	  else if ((base_proto = find_base_craft_proto (proto)) == NULL)
	    {
	      continue;
	    }
	  
	  
	  /* If we don't use names, don't parse the names of the base item.
	     Otherwise, DO parse the names of the base item. */
	  if (noname)
	    arg = NULL;
	  else 
	    {
	      if ((edesc = find_edesc_thing (base_proto, "names", TRUE)) == NULL)
		arg = NULL;
	      else
		arg = edesc->desc;
	    }
	  
	  /* Now loop through all names or no names and make the objects. */
	  
	  do
	    {
	      if (arg)
		arg = f_word (arg, basename);
	      else
		basename[0] = '\0';
	      /* Get the proto name into the item name since multiword
		 names have quotes around them and that messes things
		 up. */
	      f_word (proto->name, itemname);
	    
	      if (curr_vnum < min_vnum || curr_vnum > max_vnum)		
		break;
	     
	      obj = new_thing();
	      craft = new_value();
	      craft->type = VAL_CRAFT;
	      craft->val[0] = base_proto->vnum;
	      craft->val[1] = proto->vnum;
	      
	      
	      /* Find the name of the thing that makes this item. */
	      for (process = proto->edescs; process; process = process->next)
		{
		  if (str_cmp (process->name, "gen") &&
		      str_cmp (process->name, "names"))
		    {
		      ln = process->desc;
		      while (ln && *ln)
			{
			  ln = f_line (ln, line);
			  wd = line;
			  wd = f_word (wd, word);
			  if (!str_cmp (word, "made"))
			    {
			      wd = f_word (wd, word);
			      if (atoi (word) > 0)
				wd = f_word (wd, word);
			      if ((proto2 = find_thing_thing (proto_area, proto_area, word, FALSE)) != NULL)
				craft->val[2] = proto2->vnum;
			      break;
			    }
			  if (craft->val[2] > 0)
			    break;
			}
		    }
		}
	      
	      add_value (obj, craft);
	      obj->vnum = curr_vnum++;
	      obj->level = nr (10,30);
	      obj->timer = proto->timer;
	      obj->wear_pos = proto->wear_pos;
	      copy_values (proto, obj);
	      copy_flags (proto, obj);
	      obj->thing_flags = proto->thing_flags;
	      if (IS_SET (obj->thing_flags, TH_NO_MOVE_BY_OTHER | TH_NO_TAKE_BY_OTHER))
		add_flagval (obj, FLAG_OBJ1, OBJ_NOSTORE);
	      thing_to (obj, load_area);
	      add_thing_to_list (obj);
	      
	      if (*basename)
		sprintf (name, "%s %s",  basename, itemname);
	      else
		strcpy (name, itemname);
	      obj->name = new_str (name);
	      
	      if (plural)
		sprintf (sdesc, "some %s", name);
	      else
		sprintf (sdesc, "%s %s", a_an(name), name);
	  
	      obj->short_desc = new_str (sdesc);
	      sdesc[0] = UC(sdesc[0]);
	      strcat (sdesc, " is here.");
	      obj->long_desc = new_str (sdesc);
	    }
	  while (arg && *arg);
	}
    }

  /* Now deal with all things that revert to previous/other states
     when they expire. */

  for (obj = load_area->cont; obj; obj = obj->next_cont)
    {
      if ((replace_by = FNV (obj, VAL_REPLACEBY)) == NULL ||
	  (craft = FNV (obj, VAL_CRAFT)) == NULL || 
	  !replace_by->word || !*replace_by->word)
	continue;
      
      replace_by_item = NULL;
      using_made_item = FALSE;
      
      if (!str_cmp (replace_by->word, "made") ||
	  !str_cmp (replace_by->word, "made_from") ||
	  !str_cmp (replace_by->word, "made from"))
	{
	  using_made_item = TRUE;
	  if ((made_from_item = find_thing_num (craft->val[2])) == NULL)
	    continue;
	}
      else if ((replace_by_item = find_thing_thing (proto_area, proto_area, replace_by->word, FALSE)) == NULL)
	continue;
      
      
      if (using_made_item)
	{
	  /* Now find the load item with the made_from_item base and
	     the same prefix as our object. That's what it turns into. */
	
	  matched_name = FALSE;
	  for (obj2 = load_area->cont; obj2; obj2 = obj2->next_cont)
	    {
	      if ((craft2 = FNV (obj2, VAL_CRAFT)) == NULL ||
	      craft2->val[1] != craft->val[2])
		continue;
	      
	      /* Check the names of the other load objects. */
	      
	      wd2 = obj2->name;
	      num_unmatched_words = 0;
	      while (wd2 && *wd2 && !matched_name)
		{

		  /* We set the replace by item to null at the start
		     of each loop. It gets set to the object if the
		     name matches the made_from_item name or the
		     made item. This is because I can then 
		     match up items to the things with the same 
		     prefixes that map to them. */
		  
		  replace_by_item = NULL;
		  /* Get the names from the possible object. */
		  wd2 = f_word (wd2, word2);
		  
		 
		  if (named_in (made_from_item->name, word2))
		    {
		      replace_by_item = obj2;
		      continue;
		    }
		  num_unmatched_words++;
		  wd = obj->name;
		  
		  while (wd && *wd && !matched_name)
		    {
		      wd = f_word (wd, word);
		      
		      if (!str_cmp (word, word2))
			{
			  matched_name = TRUE;
			  replace_by_item = obj2;
			  break;
			}
		    }
		}
	      if (matched_name || num_unmatched_words == 0)
		{
		  matched_name = TRUE;
		  break;
		}
	    }
	}
      if (replace_by_item)
	{
	  replace_by->val[0] = replace_by_item->vnum;
	  replace_by->val[1] = 1;
	  replace_by->val[2] = 1;
	}
    }
  add_craft_commands ();
  return;
}


/* This adds all of the craft commands that are available to the game. */

void
add_craft_commands (void)
{
  int i;
  CMD *cmd, *cmdn;
  THING *proto_area, *proto;
  EDESC *process;
  char cmd_name[STD_LEN];
  char item_name[STD_LEN];
  /* On bootup, clear the table, if not on bootup, have to clear old
     crafting commands, then make the new ones. */
  if (IS_SET (server_flags, SERVER_BOOTUP))
    {
      for (i = 0; i < 256; i++)
	crafting_commands[i] = NULL;
    }
  else
    {
      for (i = 0; i < 256; i++)
	{
	  for (cmd = crafting_commands[i]; cmd; cmd = cmdn)
	    {
	      cmdn = cmd->next;
	      free_cmd (cmd);
	    }
	  crafting_commands[i] = NULL;
	}
    }

  /* Now the table is cleared so go down all lists of items and their
     processes and add commands for each of them. */

  if ((proto_area = find_thing_num (CRAFTGEN_AREA_VNUM)) == NULL)
    return;
  
  for (proto = proto_area->cont; proto; proto = proto->next_cont)
    {
      if (IS_ROOM (proto) || !proto->name || !*proto->name)
	continue;
      
      /* Find the method name. */
      for (process = proto->edescs; process; process = process->next)
	{
	  if (str_cmp (process->name, "gen") &&
	      str_cmp (process->name, "names"))
	    break;
	}
      if (!process || !process->name || !*process->name)
	continue;
      
      cmd = new_cmd();
      f_word (process->name, cmd_name);
      cmd->name = new_str (cmd_name);
      f_word (proto->name, item_name);
      cmd->word = new_str (item_name);
      cmd->level = 0;
      cmd_name[0] = LC (cmd_name[0]);
      cmd->called_function = do_craft;
      cmd->next = crafting_commands[(int)cmd_name[0]];
      crafting_commands[(int)cmd_name[0]] = cmd;
    }
  return;
}

  /* Check to see if the player used a crafting command or not. */


bool
check_craft_command (THING *th, char *arg)
{
  char command[STD_LEN];
  char itemname[STD_LEN];
  char *wd = arg;
  bool found_command = FALSE;
  CMD *cmd;
  char buf[STD_LEN];
  if (!th || !arg || !*arg)
    return FALSE;
  
  wd = f_word (wd, command);
  strcpy (itemname, wd);
  /* Now see if we can find this command. */
  
  for (cmd = crafting_commands[(int)command[0]]; cmd; cmd = cmd->next)
    {
      if (!str_prefix (command, cmd->name))
	{
	  found_command = TRUE;
	  /* Make sure this command can be used with this item type. */
	  if (named_in (itemname, cmd->word))
	    break;
	}
    }
  
  /* If there's no command, then return false. */
  if (!found_command)
    return FALSE;
  
  /* Now complain if there's no itemname specified... After this point
     return true because we DID find the command. */
  
  if (!*itemname)
    {
      sprintf (buf, "What did you want to %s exactly?\n\r", command);
      stt (buf, th);
      return TRUE;
    }
  
  do_craft (th, itemname);
  return TRUE;
}
  

  /* This returns the base object used to craft this object. 
     This is needed because it can be difficult to tell where
     an object started after it's been crafted for several steps. */
  
  THING *
find_base_craft_proto (THING *obj)
{  
  static int depth = 0;
  EDESC *gen, *process;
  THING *area, *proto, *proto_area, *base_proto;
  char *lptr, line[STD_LEN], *wptr, word[STD_LEN];
  char *arg, arg1[STD_LEN];

  if (!obj ||
      (area = obj->in) == NULL || 
      (area->vnum != CRAFTGEN_AREA_VNUM &&
       area->vnum != CRAFT_LOAD_AREA_VNUM))      
    return NULL;
  
  /* Proto area MUST exist or we won't know where to find the base
     item for this item. */
  
  if ((proto_area = find_thing_num (CRAFTGEN_AREA_VNUM)) == NULL)
    return NULL;
  
  /* Catch loops eventually..*/
  if (depth > 100)
    return NULL;

  /* If it's in the craft_load_area, find the corresponding object in
     the craft_proto_area to find the base item. */

  if (area->vnum == CRAFT_LOAD_AREA_VNUM)
    {
      arg = obj->name;
      proto = NULL;
      while (arg && *arg && !proto)
	{
	  arg = f_word (arg, arg1);
	  proto = find_thing_thing (obj, proto_area, arg1, FALSE);
	}
      depth++;
      base_proto =  find_base_craft_proto (proto);
      depth--;
      return base_proto;
    }
	
  
  if ((gen = find_edesc_thing (obj, "gen", TRUE)) != NULL &&
      named_in (gen->desc, "base"))
    return obj;
  
  for (process = obj->edescs; process; process = process->next)
    {
      if (!str_cmp (process->name, "gen") ||
	  !str_cmp (process->name, "names"))
	continue;
      
      /* Find the name of the precursor for this by scanning the desc for
	 a "made foo" or "made N foo" line then scanning for foo. */
      lptr = process->desc;
      
      while (lptr && *lptr)
	{
	  lptr = f_line (lptr, line);
	  
	  wptr = line;
	  wptr = f_word (wptr, word);
	  if (str_cmp (word, "made"))
	    continue;
	  
	  wptr = f_word (wptr, word);
	  if (atoi (word) > 0)
	    f_word (wptr, word);
	  if ((proto = find_thing_thing (obj, proto_area, word, FALSE)) != NULL)
	    {
	      depth++;
	      base_proto = find_base_craft_proto (proto);
	      depth--;
	      return base_proto;
	    }
	}
    }
  return NULL;
}


/* Craft things that are not base items. Can give typename and
   item name. */

void
do_craft (THING *th, char *arg)
{
  bool show_materials = FALSE;
  char arg1[STD_LEN];
  char *oldarg = arg;
  arg = f_word (arg, arg1);
  if (!str_cmp (arg1, "materials") ||
      !str_cmp (arg1, "supplies") ||
      !str_cmp (arg1, "material"))
    {
      show_materials = TRUE;
      oldarg = arg;
      arg = f_word (arg, arg1);
      if (!*arg)
	arg = oldarg;
    }
  else if (!str_prefix (arg1, "commands"))
    {
      show_crafting_commands (th);
      return;
    }
  else if (!*arg)
    arg = oldarg;
  craft_item (th, arg, arg1, show_materials);
  return;
}


/* This lets a player craft or gather an item. What it does it go down the
   list of requirements inside the "process" edesc for this item and
   check to see if a player has each item listed. 

   The itemname is the base name of the item, and the typename is the
   type of item like silver or oak. */

void
craft_item (THING *th, char *itemname, char *typename, bool show_materials)
{
  THING *base_item, *item, *base_area, *load_area;
  int count, num_choices = 0, num_chose = 0;
  /* Since an item may be made in several ways, allow more than 
     one process and loop through them checking each one. */
  EDESC *process; /* This is the edesc that is not called gen or names. */
  char buf[STD_LEN];
  VALUE *craft;
  EDESC *gen;
  
  if (!th || !itemname || !*itemname || !th->in)
    {
      stt ("craft_command <itemname>\n\r", th);
      return;
    }
  
  

  /* Make sure that the two areas we need are here. */

  if ((base_area = find_thing_num (CRAFTGEN_AREA_VNUM)) == NULL ||
      (load_area = find_thing_num (CRAFT_LOAD_AREA_VNUM)) == NULL)
    {
      stt ("Crafting has been disabled. Ask an admin to add the necessary objects back into the game.\n\r", th);
      return;
    }
  
  /* DO THIS LATER -- if we gather, find the base item in the gen area
     with a corresponding name, then find all items in the load area
     with the corresponding name and pick one at random. */
  
  

  /* If you gather, pick one of the many base items that have
     the name you pick. Base items have the word "base" in a "gen"
     edesc. */

  if ((base_item = find_thing_thing (base_area, base_area, itemname, FALSE)) == NULL)
    {
      stt ("You can't craft something like this.\n\r", th);
      return;
    }
  
  if ((gen = find_edesc_thing (base_item, "gen", TRUE)) != NULL &&
      named_in (gen->desc, "base"))
    {
      for (count = 0; count < 2; count++)
	{
	  
	  for (item = load_area->cont; item; item = item->next_cont)
	    {
	      if (!is_named (item, itemname))
		continue;
	      if ((base_item = find_base_craft_proto (item)) == NULL ||
		  !is_named (base_item, itemname))
		continue;
	      
	      if (count == 0)
		num_choices++;
	      else if (--num_chose < 1)
		break;
	    }
	  if (count == 0)
	    {
	      if (num_choices > 0)
		num_chose = nr (1, num_choices);
	      else
		break;
	    }
	}
      
      if (!item || (craft = FNV (item, VAL_CRAFT)) == NULL)
	{
	  sprintf (buf, "You can't gather %s here!\n\r", itemname);
	  stt (buf, th);
	  return;
	}
      if (craft->val[0] != craft->val[1])
	{
	  sprintf (buf, "%s isn't a raw material. You can't gather it.\n\r", itemname);
	  stt (buf, th);
	  return;
	}
    }
	      
  /* Otherwise, find the item in the load area with the proper name
     and create it if you can. */

  /* Check if this item we want to craft even exists. */
  
  else
    {
      for (item = load_area->cont; item; item = item->next_cont)
	{
	  if (is_named (item, itemname) &&
	      (!typename || 
	       !*typename || 
	       is_named (item, typename)))
	    break;
	}

  
      /* Make sure the item exists and that its proto exists in the
	 craftgen area. */
      
      if (!item || (craft = FNV (item, VAL_CRAFT)) == NULL)
	{
	  stt ("You can't craft that kind of an item. Sorry.\n\r", th);
	  return;
	}
      
      if ((base_item = find_thing_num (craft->val[1])) == NULL)
	{ 
	  stt ("You can't craft that kind of an item. Sorry.\n\r", th);
	  return;
	}
    }
  
  /* At this point we know the item we want to make and we have
     a base_itemtype item that will let us make that item, so now
     we have to see if the person making the item has all of the
     requirements to make that item. */
  
  /* Try all processes. */

  /* First pass is to see if the items are there, the second is to
     delete the items that need to be deleted and to make the
     new item. */
  
  for (process = base_item->edescs; process; process = process->next)
    {
      if (!str_cmp (process->name, "gen") ||
	  !str_cmp (process->name, "names"))
	continue;
      
      /* Process name found. Loop through once and just check to 
	 see if everything's there. If it's there, go through the
	 process again and remove the items needed and make the new 
	 stuff. */
      
      if (show_materials)
	{
	  check_craft_materials (th, item, base_item, process, CRAFT_SHOW_MATERIALS);
	}
      
      else if (check_craft_materials (th, item, base_item, process, 0))
	{
	  check_craft_materials (th, item, base_item, process, CRAFT_DO_PROCESS);
	  return;
	}
    }
  return;
}

/* This checks if the person doing the crafting has all of the materials 
   on hand, and if so, it returns true. If this is set to do the
   process, then it loops through again deleting everything as
   it's found. It is assumed that the CRAFT_DO_PROCESS argument will
   ONLY be used if this has been called once just checking. 
   If the SHOW flag is set, we don't attempt to do the process or 
   check the items, we just show the person what they need to do this. */


bool
check_craft_materials (THING *th, THING *item, THING *base_item, EDESC *process, int flags)
{
  char *wd, word[STD_LEN];
  char *ln, line[STD_LEN];

  /* Since an item may be made in several ways, allow more than 
     one process and loop through them checking each one. */
  THING *use_obj = NULL, *in_obj, *base_obj, *load_area, *use_objn, *made_item;
  int num_needed, num_found;
  int min_num_to_make = 1, max_num_to_make = 1, num_to_make = 1;
  int use_obj_vnum;
  int pass = 0;
  int i;
  int room_bit;
  bool room_is_ok = TRUE;
  char item_name[STD_LEN];
  char type_name[STD_LEN];
  char buf[STD_LEN];
  char full_item_name[STD_LEN]; /* type_name + item_name */
  
  VALUE *craft;
  if (!th || !item || !base_item || !process || !th->in)
    {
      stt ("You can't make this item!\n\r", th);
      return FALSE;
    }

  /* Find any words in the item name that aren't in the base name...this
     is the type name needed for the made from argument to make sure
     the stuff we use to make things has the same type name. */
  wd = item->name;
  type_name[0] = '\0';
  while (wd && *wd)
    {
      wd = f_word (wd, word);
      if (!is_named (base_item, word))
	{
	  strcpy (type_name, word);
	  break;
	}
    }
  /* Loop through the lines in the edesc to see if we have everything. */
  ln = process->desc;  
  do
    {
      ln = f_line (ln, line);
      
      wd = line;
      
      wd = f_word (wd, word);
      
      if (!str_cmp (word, "make"))
	{
	  wd = f_word (wd, word);
	  min_num_to_make = MAX (atoi(word), 1);
	  if (*wd)
	    max_num_to_make = MAX (atoi(wd), min_num_to_make);
	  
	  if (IS_SET (flags, CRAFT_SHOW_MATERIALS))
	    {
	      if (min_num_to_make < max_num_to_make)
		{
		  strcpy (item_name, item->name);
		  plural_form (item_name);
		  sprintf (buf, "This process will make from %d to %d %s.\n\r",
			   min_num_to_make, max_num_to_make, item_name);
		  stt (buf, th);
		}
	      else
		{
		  strcpy (item_name, item->name);
		  if (min_num_to_make > 1)
		    plural_form (item_name);
		  sprintf (buf, "This process will make %d %s.\n\r", 
			   min_num_to_make, item_name);
		}
	    }
	}
      if (!str_cmp (word, "room"))
	{
	  /* Loop through all room flags and see if ANY match this
	     current room's flags. */
	  if (wd && *wd)
	    room_is_ok = FALSE;
	  while (wd && *wd)
	    {
	      wd = f_word (wd, word);
	      

	      room_bit = find_bit_from_name (FLAG_ROOM1, word);
	      
	      if (IS_SET (flags, CRAFT_SHOW_MATERIALS))
		{
		  room_is_ok = TRUE;
		  sprintf (buf, "This process can be carried out in %s %s location.\n\r", a_an (word), word);
		  stt (buf, th);
		}
	      
	      
	      else if (room_bit && IS_ROOM_SET (th->in, room_bit))
		{
		  room_is_ok = TRUE;
		 
		  break;
		}
	    }
	     
	  if (!room_is_ok)
	    { 
	      sprintf (buf, "You can't gather these raw materials here!\n\r");
	      stt (buf, th);
	      return FALSE;
	    }
	}

      
      
      /* Get a held item if needed. */
      else if (!str_cmp (word, "hold") ||
	       !str_cmp (word, "wield") ||
	       !str_cmp (word, "wear"))
	{
	  char action_word[STD_LEN];
	  strcpy (action_word, word);
	  f_word (wd, word);
	  num_needed = 1;
	  num_found = 0;
	  
	  if (IS_SET (flags, CRAFT_SHOW_MATERIALS))
	    {
	      sprintf (buf, "You must be %sing %s %s.\n\r",
		       action_word, a_an(word), word);
	      stt (buf, th);
	    }
	  else
	    {
	      for (use_obj = th->cont; use_obj; use_obj = use_obj->next_cont)
		{
		  if (IS_WORN (use_obj) &&
		      (craft = FNV (use_obj, VAL_CRAFT)) != NULL &&
		      (base_obj = find_thing_num (craft->val[1])) != NULL &&
		      is_named (base_obj, word))
		    {
		      num_found++;
		      if (num_found >= num_needed)
			break;
		    }
		}
	      
	      if (num_needed > num_found)
		{
		  sprintf (buf, "You need to be %sing %s %s.\n\r", 
			   action_word, a_an (word), word);
		  stt (buf, th);
		  return FALSE;
		}
	    }
	}	
      
      /* Check for items that have to be here someplace...but not used. */
      else if (!str_cmp (word, "here") ||
	       !str_cmp (word, "use")  ||
	       !str_cmp (word, "made"))	
	{
	  char use_type_word[STD_LEN];
	  VALUE *drink;
	  bool made_from_this = FALSE;
	  bool need_water = FALSE;
	  bool can_delete = FALSE;
	   strcpy (use_type_word, word);
	  
	  if (!str_cmp (word, "here"))
	    can_delete = FALSE;
	  else 
	    can_delete = TRUE;

	  if (!str_cmp (word, "made"))
	    made_from_this = TRUE;
	  use_obj_vnum = 0;
	  in_obj = NULL;
	  wd = f_word (wd, word);
	  if (atoi (word) > 0)
	    {
	      num_needed = atoi (word);
	      wd = f_word (wd, word);
	    }
	  else
	    num_needed = 1;
	  num_found = 0;
	  use_obj_vnum = 0;
	  
	  strcpy (item_name, word);
	  if (!str_cmp (item_name, "corpse"))
	    use_obj_vnum = CORPSE_VNUM;
	  else if (!str_cmp (item_name, "water"))
	    need_water = TRUE;
	  /* Used for the error messages below. */
	  if (made_from_this)
	    sprintf (full_item_name, "%s%s%s",
		     type_name, 
		     (*type_name ? " " : ""),
		     item_name);
	  else
	    strcpy (full_item_name, item_name);
	  if (num_needed > 1)
	    plural_form (full_item_name);
	  
	  /* Get the "in foo" part of these lines. */
	  wd = f_word (wd, word);
	  
	  if (!str_cmp (word, "in"))
	    {
	      wd = f_word (wd, word);
	      if ((in_obj = find_thing_here (th, word, TRUE)) == NULL)
		{
		  
		  sprintf (buf, "You need %s %s to put %d %s into.\n\r", 
			   a_an (word), word, num_needed,
			   full_item_name);
		  stt (buf, th);
		      
		}
	      else
		{
		  pass = 1;
		  use_obj = in_obj->cont;
		  if (need_water && 
		      (drink = FNV (in_obj, VAL_DRINK)) != NULL)
		    {
		      if (drink->val[1] == 0)
			num_found = 1000000;
		      else
			{
			  num_found = drink->val[0];
			  if (IS_SET (flags, CRAFT_DO_PROCESS))
			    drink->val[0] -= num_needed;
			}
		    }
		  pass = 2;
		}
	    }
	  else
	    {
	      pass = 0;
	      use_obj = th->cont;
	      if (need_water)
		{
		  stt ("You need to have water in some kind of a container.\n\r", th);
		  pass = 2;
		}
	    }
	  
	  if (IS_SET (flags, CRAFT_SHOW_MATERIALS))
	    {
	      if (num_needed > 1)
		plural_form (item_name);
	      if (in_obj)
		{
		  sprintf (buf, "You must have %d %s(s) in %s %s ", num_needed,  item_name, a_an(NAME(in_obj)), NAME(in_obj));
			   stt (buf, th);
		}
	      else
		{
		  sprintf (buf, "You must have %d %s ", num_needed, item_name);
		  stt (buf, th);
		}
	      if (!str_cmp (use_type_word, "here"))
		stt ("here to carry out this process.\n\r", th);
	      else if (!str_cmp (use_type_word, "use"))
		stt ("to use up in this process.\n\r", th);
	      else 
		stt ("to make the item in the process.\n\r", th);
	      continue;
	    }
	  
	  
	  /* Make 1-2 passes. If the objects must be in something,
	     only check that they're in that thing. Otherwise, check
	     that they're in me and then out in the world. */
	  
	  for (;pass < 2; pass++)
	    {	     
	      for (; use_obj; use_obj = use_objn)
		{
		  use_objn = use_obj->next_cont;

		  
		  if ((!use_obj_vnum &&
		       is_named (use_obj, item_name) &&

		       /* This part is needed for things like
			  brass anvils so we know that we're using
			  the correct items to make them. */
		       (type_name[0] == '\0' ||
			!made_from_this ||
			is_named (use_obj, type_name)) &&
		       
		       (load_area = find_area_in (use_obj->vnum)) &&
		       load_area->vnum == CRAFT_LOAD_AREA_VNUM) ||
		      (use_obj_vnum &&
		       use_obj_vnum == use_obj->vnum))
		    {	
		      if (!use_obj_vnum)
			use_obj_vnum = use_obj->vnum;
		      if (can_delete && IS_SET (flags, CRAFT_DO_PROCESS))
			free_thing (use_obj);	
		      num_found++;
		      if (num_found >= num_needed)
			break;
		    }
		  if (num_found >= num_needed)
		    break;
		} 
	      if (pass == 0)
		use_obj = th->in->cont;	      
	    }
	  
	  if (num_found < num_needed)
	    {
	      if (num_needed > 1)
		plural_form (item_name);
	      if (in_obj)
		{
		  sprintf (buf, "You need to have %d %s in %s, but there %s only %d in there.\n\r", num_needed,  full_item_name, NAME (in_obj), 
			   (num_found == 1 ? "is" : "are"),
			   num_found);
		  stt (buf, th);
		  return FALSE;
		}
	      else
		{ 
		  sprintf (buf, "You need to have %d %s, but you only have %d here.\n\r", num_needed, full_item_name, num_found);
		  stt (buf, th);
		  return FALSE;
		}
	    }
	}      
    }
  while (ln && *ln);
  
  if (IS_SET (flags, CRAFT_DO_PROCESS) && !IS_SET (flags, CRAFT_SHOW_MATERIALS))
    {
      num_to_make = nr (min_num_to_make, max_num_to_make);
      if (num_to_make < 1)
	num_to_make = 1;
      for (i = 0; i < num_to_make; i++)
	{
	  if ((made_item = create_thing (item->vnum)) == NULL)
	    continue;
	  reset_thing (made_item, 0);            
	  if (IS_SET (made_item->thing_flags, TH_NO_MOVE_BY_OTHER | TH_NO_TAKE_BY_OTHER))
	    thing_to (made_item, th->in);
	  else
	    thing_to (made_item, th);
	  act ("@1n @t@s @3n.", th, NULL, made_item, (process->name && *process->name ? process->name : "craft"), TO_ALL);
	}
    }
  return TRUE;
}


/* This shows the crafting commands that are available to a player. */

void
show_crafting_commands (THING *th)
{
  CMD *cmd;
  int i, count = 0;
  char buf[STD_LEN];
  
  if (!IS_PC (th))
    return;

  stt ("A list of crafted items and the commands that make them.\n\r", th);
  stt ("Try out the commands to see what you need to craft these things.\n\n\r", th);
  for (i = 0; i < 256; i++)
    {
      for (cmd = crafting_commands[i]; cmd; cmd = cmd->next)
	{
	  if (!cmd->name || !*cmd->name || !cmd->word || !*cmd->word)
	    continue;
	  
	  count++;
	  sprintf (buf, "%-10s:%-20s", cmd->name, cmd->word);
	  stt (buf, th);
	  if (count % 3 == 0)
	    stt ("\n\r", th);
	}
    }
  if (count % 3 != 0)
    stt ("\n\r", th);
  return;
}


  
  
  

  
