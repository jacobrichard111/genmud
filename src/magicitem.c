#include<stdio.h>
#include<ctype.h>
#include<stdlib.h>
#include "serv.h"

/* These functions let people use magic items such as scrolls, wands
   and staves. */

void
do_recite (THING *th, char *arg)
{
  use_magic_item (th, "recite", arg);
  return;
}

void
do_zap (THING *th, char *arg)
{
  use_magic_item (th, "zap", arg);
  return;
}

void 
do_brandish (THING *th, char *arg)
{
  use_magic_item (th, "brandish", arg);
  return;
}


/* This is the command that lets people use magic items they are
   using.

   Basically, they need to be holding the item and they need to type
   the proper command for the kind of item they're using. The arguments
   work as follows:

   if the first argument refers to a magic item of the proper type
   that they're holding, they use that one. Otherwise, they try
   to use the first magic item of the proper type.
   
   If the first argument refers to an item they're carrying of the
   proper type, the second arg refers to the victim. In the case
   of no args, if they're using a scroll or wand and have no victim, it's
   cast on them, unless they're fighting and the scroll has an offensive
   spell in which case it gets cast on their enemy. If they have a staff,
   it just gets cast on the room. */
   

void
use_magic_item (THING *th, char *usename, char *arg)
{
  char arg1[STD_LEN];
  VALUE *mval = NULL;
  THING *magic_item = NULL; /* The magic item being used. */
  THING *vict; /* The thing the magic item is used on. */
  int magic_item_type = 0;
  int i;
  SPELL *spl;
  bool offensive_spell = FALSE;
  bool self_spell = FALSE;
  
  if (!th || !th->in)
    return;
  
  if (!str_cmp (usename, "zap"))
    magic_item_type = MAGIC_ITEM_WAND;
  else if (!str_cmp (usename, "brandish"))
    magic_item_type = MAGIC_ITEM_STAFF;
  else if (!str_cmp (usename, "recite"))
    magic_item_type = MAGIC_ITEM_SCROLL;
  
  
  arg = f_word (arg, arg1);

  /* To use a magic item, you must be wielding it and it must be
     of the proper type. */

  if ((magic_item = find_thing_worn (th, arg1)) == NULL ||
      magic_item->wear_loc != ITEM_WEAR_WIELD ||
      (mval = FNV (magic_item, VAL_MAGIC)) == NULL ||
      mval->val[0] != magic_item_type)
    {
      for (magic_item = th->cont; magic_item; magic_item = magic_item->next_cont)
	{
	  if (magic_item->wear_loc != ITEM_WEAR_WIELD)
	    continue;
	  
	  if ((mval = FNV (magic_item, VAL_MAGIC)) == NULL ||
	      mval->val[0] != magic_item_type)
	    continue;
	  break;
	}
    }
  else /* We found the named item so we need to use the other 
	  arg for the vict. If we hadn't found the named item, we
	  would assume that it's the victim name and not strip off
	  another arg. */    
    arg = f_word (arg, arg1);

  
  /* Make sure that the magic item exists. */
  
  if (!magic_item || !mval)
    {
      stt ("You aren't holding anything like that!\n\r", th);
      return;
    }
  
  /* If it's not a scroll, it needs charges. */
  
  if (magic_item_type != MAGIC_ITEM_SCROLL &&
      mval->val[1] < 1)
    {
      act ("@2n is out of charges", th, magic_item, NULL, NULL, TO_CHAR);
      return;
    }

  /* Now find out what kinds of spells are in the magic item. */
  
  for (i = 3; i < NUM_VALS; i++)
    {      
      if ((spl = find_spell (NULL, mval->val[i])) != NULL)
	{
	  if (spl->spell_type == TAR_SELF)
	    self_spell = TRUE;
	  if (spl->spell_type == TAR_OFFENSIVE)
	    offensive_spell = TRUE;
	}
    }

  /* Staves hit all in room. */

  if (magic_item_type == MAGIC_ITEM_STAFF)
    vict = th->in->cont;

  /* If you don't find a victim by name, try to figure out who the vict is. */
  
  else if ((vict = find_thing_in (th, arg1)) == NULL)
    {
      if (arg1[0])
	{
	  stt ("They don't appear to be here.\n\r", th);
	  return;
	}
      
      /* If there was no victim name given, just pick based on the 
	 following: */
      
      /* Self spell items hit only the self. */
      if (self_spell)
	vict = th;
      else if (FIGHTING (th) && offensive_spell)
	vict = FIGHTING (th);
      else 
	vict = th;
    }
  

  /* Now you have the item and the victim. Now, cast the spells. */
  
  act ("@1n @t@s @2n.", th, magic_item, NULL, usename, TO_ALL);

  for (i = 3; i < NUM_VALS; i++)
    {
      if ((spl = find_spell (NULL, mval->val[i])) != NULL &&
	  th->in && magic_item->in == th)
	{
	  cast_spell (th, vict, spl, 
		      (magic_item_type == MAGIC_ITEM_STAFF ? TRUE : FALSE),
		      FALSE, NULL);
	}
    }
  
  if (magic_item->in == th &&
      (magic_item_type == MAGIC_ITEM_SCROLL ||
       --mval->val[1] < 1))
    {
      act ("@2n disappears in a flash of light!", th, magic_item, NULL, NULL, TO_ALL);
      free_thing (magic_item);
    }  
  return;
}
	
	   









