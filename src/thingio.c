
#include <sys/types.h> 
#include <ctype.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "serv.h"
#include "society.h"
#include "script.h"
#include "track.h"
#include "worldgen.h"

/* The next two functions deal with reading and writing a thing
   to and from the game. */

void
write_thing (FILE *f, THING *th)
{
  THING *thg;
  VALUE *value;
  RESET *rst;
  FLAG *flag;
  EDESC *eds;
  if (!th || !f)
    return;
  
  fprintf (f, "THING %d\n", th->vnum);
  if (th->name && th->name[0])
    write_string (f, "Name", th->name);
  if (th->level)
    fprintf (f, "Level %d\n", th->level);
  if (IS_PC (th) || IS_AREA (th))
    fprintf (f, "HpMv %d %d %d %d\n", th->hp, th->max_hp, th->mv, th->max_mv);
  else if (th->max_hp != 10 && !IS_AREA (th))
    fprintf (f, "HpMult %d\n", th->max_hp);
  if (th->armor)
    fprintf (f, "Armor %d\n", th->armor);
  fprintf (f, "HWT %d %d\n", th->height, th->weight);
  if (th->align)
    fprintf (f, "Align %d\n", th->align);
  if (th->color)
    fprintf (f, "Color %d\n", th->color);
  if (th->cost)
    fprintf (f, "Cost %d\n", th->cost);
  if (!IS_PC (th) && th->timer)
    fprintf (f, "Timer %d\n", th->timer);
  if (th->desc && th->desc[0])
    write_string (f, "Desc", th->desc);
  
  if (th->short_desc && th->short_desc[0])
    write_string (f, "Short", th->short_desc);
  if (th->size)
    fprintf (f, "Size %d\n", th->size);
  
  if (th->wear_pos > ITEM_WEAR_NONE &&
      th->wear_pos < ITEM_WEAR_MAX)
    write_string (f, "Wear", (char *) wear_data[th->wear_pos].name);
  if (th->max_mv > 0 && !IS_PC (th) && !IS_AREA (th))
    fprintf (f, "MIW %d\n", th->max_mv);
  if (th->long_desc && th->long_desc[0])
    write_string (f, "Long", th->long_desc);
  if (th->symbol)
    fprintf (f, "Symbol %c%c\n", th->symbol, END_STRING_CHAR);
  if (th->sex)
    fprintf (f, "Sex %d\n", th->sex);
  if (th->type && th->type[0])
    write_string (f, "type", th->type);
  if (th->thing_flags)
    fprintf (f, "ThFl %d\n", th->thing_flags);
  for (value = th->values; value != NULL; value = value->next)
    write_value (f, value);
  for (flag = th->flags; flag != NULL; flag = flag->next)
    write_flag (f, flag);
  for (rst = th->resets; rst; rst = rst->next)
    write_reset (f, rst);
  for (eds = th->edescs; eds; eds = eds->next)
    write_edesc (f, eds);
  if (IS_PC (th))
    write_pcdata (f, th);
  fprintf (f, "END_THING\n\n");

  /* Only write the contents of areas... i.e. the prototypes...and
   short contents of players. */
  
  if (IS_AREA (th))
    {
      for (thg = th->cont; thg; thg = thg->next_cont)
	{
	  if (!IS_SET (thg->thing_flags, TH_NUKE))
	    write_thing (f, thg);
	}
    }
  return;
}


/* This gets called for loading areas and for pc's */

THING *
read_thing (FILE *f)
{
  THING *th;
  int vnum;
  char buf[STD_LEN];
  bool found = FALSE;
  FILE_READ_SINGLE_SETUP;
  
  if (!f)
    return NULL;
  vnum = read_number (f);
 
  th = new_thing ();
  th->vnum = vnum;
  top_vnum = MAX (top_vnum, vnum);
  
  FILE_READ_LOOP
    {
      found = FALSE;
      switch (UC(word[0]))
	{
	case 'A':
	  if (!str_cmp (word, "Armor"))
	    {
	      th->armor = read_number (f);
	      found = TRUE;
	    }
	  if (!str_cmp (word, "Align"))
	    {
	      th->align = read_number (f);
	      found = TRUE;
	    }
	  break;
	case 'C':
	  if (!str_cmp (word, "Color"))
	    {
	      th->color = *(read_word (f));
	      found = TRUE;
	    }
	  if (!str_cmp (word, "Cost"))
	    {
	      th->cost = read_number (f);
	      found = TRUE;
	    }
	  
	  break;
	case 'D':
	  if (!str_cmp (word, "Desc"))
	    {
	      th->desc = new_str (add_color(read_string (f)));
	      found = TRUE;
	    }
	  break;
	case 'E':	  
	  if (!str_cmp (word, "END_THING"))
	    { 
	      return th;
	    }
	  if (!str_cmp (word, "EDESC"))
	    {
	      EDESC *eds = read_edesc (f);
	      if (eds)
		add_edesc (th, eds);
	      found = TRUE;
	    }	   
	break;
	case 'F':
	  if (!str_cmp (word, "Flag"))
	    {
	      FLAG *flg, *prev;
	      flg = read_flag (f);
	      if (!th->flags)
		{
		  flg->next = th->flags;
		  th->flags = flg;
		}
	      else 
		{
		  for (prev = th->flags; prev != NULL; prev = prev->next)
		    {
		      if (!prev->next)
			{
			  prev->next = flg;
			  flg->next = NULL;
			  break;
			}
		    }
		}
	      found = TRUE;
	    }	    
	  break;  
	case 'H':
	  if (!str_cmp (word, "HpMult") && !IS_PC (th))
	    {
	      th->max_hp = read_number (f);
	      found = TRUE;
	    }
          if (!str_cmp (word, "HWT"))
            { 
              th->height = read_number (f);
              th->weight = read_number (f);
              found = TRUE;  
            }
	  if (!str_cmp (word, "HpMv"))
	    {
	      th->hp = read_number (f);
	      th->max_hp = read_number (f);
	      th->mv = read_number (f);
	      th->max_mv = read_number (f);
	      found = TRUE;
	    }
	  break;
	case 'L':
	  if (!str_cmp (word, "Level"))
	    {
	      th->level = read_number (f);
	      found = TRUE;
	    }
	  if (!str_cmp (word, "Long"))
	    {
	      th->long_desc = new_str (add_color(read_string (f)));
	      found = TRUE;
	    }
	  break;
	  case 'M':
	  if (!str_cmp (word, "MIW"))
	    {
	      th->max_mv = read_number (f);
	      th->mv = 0;
	      found = TRUE;
	    }
	  break;
	case 'N':
	  if (!str_cmp (word, "Name"))
	    {
	      th->name = new_str (read_string (f));
	      found = TRUE;
	    }
	  break;
	  if (!str_cmp (word, "Need"))
	    {
	      need_to_thing (th, read_need(f));
	      found = TRUE;
	    }
	case 'P':
	  if (!str_cmp (word, "PCDATA"))
	    {
	      read_pcdata (f, th);
	      found = TRUE;
	    }
	  break;
	case 'R':
	  if (!str_cmp (word, "Reset"))
	    {
	      RESET *rst, *rstn;
	      rst = read_reset (f);
	      if (!th->resets)
		{
		  th->resets = rst;
		  rst->next = NULL;
		}
	      else
		{
		  for (rstn = th->resets; rstn; rstn = rstn->next)
		    {
		      if (!rstn->next)
			break;
		    }
		  rstn->next = rst;
		  rst->next = NULL;
		}
	      found = TRUE;
	    }
	break;
	case 'S':
	  if (!str_cmp (word, "Size"))
	    {
	      th->size = read_number (f);
	      found = TRUE;
	    }
	  if (!str_cmp (word, "Symbol"))
	    {
	      th->symbol = *(read_word (f));
	      found = TRUE;
	    }
	  if (!str_cmp (word, "Sex"))
	    {
	      th->sex = read_number (f);
	      found = TRUE;
	    }	
	  if (!str_cmp (word, "Short"))
	    {
	      th->short_desc = new_str (add_color(read_string (f)));
	      found = TRUE;
	    }  
	  break;
	case 'T':
	  if (!str_cmp (word, "Type"))
	    {
	      th->type = new_str (read_string (f));
	      found = TRUE;
	    }
	  if (!str_cmp (word, "ThFl"))
	    {
	      th->thing_flags = read_number (f);
	      found = TRUE;
	    }
	  if (!str_cmp (word, "Timer"))
	    {
	      th->timer = read_number (f);
	      found = TRUE;
	    }
	  break;
	case 'V':
	  if (!str_cmp (word, "Val"))
	    {
	      VALUE *val;
	      val = read_value (f);
	      if (val)
		add_value (th, val);
	      found = TRUE;
	    }
	  break;
	case 'W':
	  if (!str_cmp (word, "Wear"))
	    {
	      int wl;
	      strcpy (word, read_word(f));
	      if (isdigit (word[0]))
		th->wear_pos = atoi(word);
	      else
		{
		  th->wear_pos = ITEM_WEAR_NONE;
		  for (wl = 0; wl < ITEM_WEAR_MAX; wl++)
		    if (!str_cmp (word, wear_data[wl].name))
		      {
			th->wear_pos = wl;
			break;
		      }
		  found = TRUE;
		}
	    }
	  break;
	}
      sprintf (buf, "Th: %d", (th ? th->vnum : 0));
      if (found);      
      FILE_READ_ERRCHECK(buf);
    }
  return NULL;
}

void
write_short_thing (FILE *f, THING *th, int nest)
{
  THING *thg, *bt, *owner, *thgn;
  VALUE *value, *bv, *pet, *shopval;
  FLAG *flag, *btflag;
  NEED *need;
  int i, hp_pct = 0;
  bool sameval = TRUE, sameflag = FALSE;
  
  if (!th || !f || nest < 0 || IS_PC (th))
    return;
  
  if ((bt = find_thing_num (th->vnum)) == NULL||
      IS_SET (th->thing_flags, TH_NUKE | TH_SAVED) ||
      IS_SET (bt->thing_flags, TH_NUKE))
    return;
  
  /* Crappy items lying around the world don't get stored. */
  
  if (th->in && IS_ROOM (th->in) && !CAN_MOVE (th) &&
      !CAN_FIGHT (th) && !IS_OBJ_SET (th, OBJ_NOSTORE) &&
      th->vnum != CORPSE_VNUM)
    return;
  
  /* Pets are only saved to pc files. This should only be called if
     the pet is in the same room as the owner, so we don't check it here. */
  
  if ((pet = FNV (th, VAL_PET)) != NULL && pet->word && *pet->word)
    {
      if (!th->in)
	return;
      for (owner = th->in->cont; owner; owner = owner->next_cont)
	{
	  if (IS_PC (owner) && !str_cmp (pet->word, NAME (owner)))
	    break;
	}
      
      /* If the owner is there, save it as a pet. */

      if (owner)
	{
	  if (IS_SET (server_flags, SERVER_WRITE_PFILE))
	    fprintf (f, "PET");
	  else
	    return;
	}
      
      /* If the owner is there and it's a pfile save...don't save. */
      
      if (!owner && IS_SET (server_flags, SERVER_WRITE_PFILE))
	return;

      /* This leaves the case of !owner and !pfile...in this case
	 the poor little pet is away from its owner so we will be nice
	 and save it as part of the background world save. If people
	 cheat too much  with this, I will remove this and
	 they will then lose any pets that aren't in the same room as
	 they are. =D */
      
    }
  
   
  
  /* Basic thing info..the rest is stored in the "prototype" in the area
     files this was recently updated some...so some eq will be
     screwy. I took out the hps info, the moves info, the armor,
     the wear_pos and the thing_flags...to compress the savefiles. */
  
  if (th->max_hp > 0)
    hp_pct = th->hp*100/th->max_hp;

  /* If we're writing to a playerfile and this thing isn't in anything,
     then it's a stored item. So, write STO at the front of the save
     file. */
  
  if (IS_SET (server_flags, SERVER_WRITE_PFILE) && !th->in)
    fprintf (f, "STO");

  fprintf (f, "TH %d %d %d %d %d %d %d %d %d %d %d\n",
	   th->vnum, 
	   nest, 
	   hp_pct, 
	   th->timer,
	   th->cost,
	   th->wear_loc,
	   th->wear_num,
	   th->level,
	   th->align,
           th->light, 
	   th->position);
  if (th->name && *th->name && str_cmp (th->name, bt->name))
    write_string (f, "Name", th->name);
  if (th->short_desc && *th->short_desc && str_cmp (th->short_desc, bt->short_desc))
    write_string (f, "Short", th->short_desc);
  if (th->long_desc && *th->long_desc && str_cmp (th->long_desc, bt->long_desc))
    write_string (f, "Long", th->long_desc);
  if (th->type && *th->type && str_cmp (th->type, bt->type))
    write_string (f, "Type", th->type);
  if (CAN_MOVE (th) && CAN_FIGHT (th) && !IS_PC (th))
    {
      if (is_hunting (th))
	fprintf (f, "Hunting %s` %d\n", th->fgt->hunting, th->fgt->hunting_type);
    }
  
  
  /* I don't write the description of items in short thing format no
     matter what...??? Maybe it should be done. Dunno Here is the
     code anyway.
     if (th->desc && *th->desc && str_cmp (th->desc, bt->desc))
     write_string (f, "Desc", th->desc);
     
     We save values that are NOT EXITS!!!!, and which are different from the
     original specifications. Note: if you want exits dynamically
     saved, go for it, but don't blame me when your datafile is 50 megs. 
     Then, if the value either does not exist on the original
     base thing, or if the value is different in any way,
     we write the value. */
  
  
  for (value = th->values; value; value = value->next)
    { 
      if (value->type <= VAL_OUTEREXIT_I)
        continue;
      sameval = FALSE;
      
      if ((bv = FNV (bt, value->type)) != NULL)
	{
	  sameval = TRUE;
	  for (i = 0; i < NUM_VALS; i++)
	    {
	      if (bv->val[i] != value->val[i])
		{
		  sameval = FALSE;
		  break;
		}
	    }
	  if (value->word && *value->word && bv->word &&
	      strcmp (value->word, bv->word))
	    sameval = FALSE;
	}
      if (!sameval)
	write_value (f, value);
    }

  /* Do not write EDESCS on SHORT THINGS!!!! */
  
  /* Only write flags that a) were put there by something like a spell or
     b) are timed. Otherwise, they are just extraneous data. */
  
  for (flag = th->flags; flag; flag = flag->next)
    {
      if (flag->timer != 0 || flag->from != 0)
	write_flag (f, flag);
      else
	{
	  sameflag = FALSE;
	  for (btflag = bt->flags; btflag; btflag = btflag->next)
	    {
	      if (btflag->timer == 0 && btflag->from == 0 &&
		  btflag->type == flag->type &&
		  btflag->val == flag->val)
		{
		  sameflag = TRUE;
		  break;
		}
	    }
	  if (!sameflag)
	    write_flag (f, flag);
	}
    }
  
  /* Write out tracks...only write room tracks. */
  write_tracks (f, th->tracks);
  
  for (need = th->needs; need; need = need->next)
    write_need (f, need);
  
  fprintf (f, "EN_TH\n\n");

  if (IS_SET (server_flags, SERVER_SAVING_WORLD) &&
      th->in && IS_SET (th->in->thing_flags, TH_SAVED))
    SBIT (th->thing_flags, TH_SAVED);
  /* Note that this saves all things inside of other things, except for 
     PC's. Use with caution. This should save rooms and all
     contents, and not save the "prototypes" which are kept in the
     area files. Shops do not save their inventories. This is intentional. */

  if ((shopval = FNV (th, VAL_SHOP)) == NULL)
    {
      for (thg = th->cont; thg; thg = thgn)
	{
	  thgn = thg->next_cont;
	  if (!IS_PC (thg))
	    write_short_thing (f, thg, nest + 1);
	}
    }
  return;
}

/* This gets done AFTER all prototypes are created. It reads a 
   "short thing" data info out of the file, and then it
   attempts to make the thing based on the info given, and it
   attempts to put the thing into the world in the proper place.
   Note that if nest == 0, the thing MUST be a room!!! */

void
read_short_thing (FILE *f, THING *to, CLAN *cln)
{
  THING *th, *bt; /* base thing - prototype */
  int i, vnum, nest, hp_pct=  0;
  FLAG *newflag;
  TRACK *newtrack;
  VALUE *pet;
  SOCIETY *soc;
  FILE_READ_SINGLE_SETUP;
  vnum = read_number (f);
  
  /* Find if this number has a base prototype, or it is a room
     or an area. */
  
  if ((bt = find_thing_num (vnum)) == NULL)
    {  
      vnum = CATCHALL_VNUM;
      bt = find_thing_num (vnum);    
      if (!bt)
	{
	  log_it ("Missing catchall item!\n");
	  exit (5);
	}
    }                                    
  
  if (IS_SET (bt->thing_flags, TH_IS_ROOM | TH_IS_AREA))
    {
      th = bt;
    }
  else if ((th = create_thing (bt->vnum)) == NULL)
    {
      vnum = CATCHALL_VNUM;
      th = create_thing (vnum);
      if (!th)
	{
	  log_it ("Missing catchall item!\n");
	  exit (5);
	}
    }
  
  
  nest = read_number (f);
 
  for (i = nest + 1; i < MAX_NEST && thing_nest[i]; i++)
    thing_nest[i] = NULL;
  thing_nest[nest] = th; 
  hp_pct = read_number (f);
  if (th->max_hp > 0)
    th->hp = th->max_hp*hp_pct/100;
  th->timer = read_number (f);
  th->cost = read_number (f);
  th->wear_loc = read_number (f);
  th->wear_num = read_number (f);
  th->level = read_number (f);
  th->align = read_number (f);
  th->light = read_number (f);
  th->position = read_number (f);
  if (IS_ROOM (th))
    th->light = 0;

  /* DO NOT READ EDESCS ON SHORT THINGS! */

  FILE_READ_LOOP
    {
      FKEY_START;
      /* Values are read in, and if the setup thing has no value of
	 that type, a new one is made, otherwise the file numbers
	 overwrite the already created numbers. */

      FKEY("Val")
	{
	  VALUE *newval;
	  newval = read_value (f);
	  if (newval)
	    {
	      add_value (th, newval);
	      if (newval->type == VAL_SOCIETY &&
		  (soc = find_society_num (newval->val[0])) != NULL)
		{
		  update_society_population_thing (th, TRUE);
		  th->align = soc->align;
		}
	    }
	}
      FKEY("Flag")
	{
	  newflag = read_flag (f);	  
	  aff_update (newflag, th);
	}
      FKEY("Name")
	{
	  free_str (th->name);
	  th->name = new_str (read_string (f));
	}
      FKEY("Short")
	{
	  free_str (th->short_desc);
	  th->short_desc = new_str (add_color(read_string (f)));
	}
      FKEY("Long")
	{
	  free_str (th->long_desc);
	  th->long_desc = new_str (add_color(read_string (f)));
	}
      FKEY("Type")
	{
	  free_str (th->type);
	  th->type = new_str (read_string (f));
	}
      FKEY ("Track")
	{
	  newtrack = read_track (f);
	  if (newtrack)
	    {
	      newtrack->next = th->tracks;
	      th->tracks = newtrack;
	    }
	}
      FKEY("Hunting")
	{
	  int type;
	  add_fight (th);
	  strcpy (word, read_string (f));
	  type = read_number (f);
	  start_hunting (th, word, type);
	}
      FKEY("Need")
	need_to_thing (th, read_need (f));
      /* FKEY("Desc")
	{
	  free_str (th->desc);
	  th->desc = new_str (read_string (f));
	} */
      FKEY("EN_TH")
	{
	  
	  if (vnum == CATCHALL_VNUM)
	    {
	      free_thing(th);
	      thing_nest[nest] = NULL;
	      return;
	    }
	  if ((pet = FNV (th, VAL_PET)) != NULL &&
	      pet->word && *pet->word && to && IS_PC (to))
	    {
	      th->next_cont = to->pc->pets;
	      to->pc->pets = th;
	      return;
	    }
	  
	  if (nest == 0)
	    {
	      if (to && IS_PC (to))
		{
		  for (i = 0; i < MAX_STORE; i++)
		    {
		      if (to->pc->storage[i] == NULL)
			{
			  to->pc->storage[i] = th;
			  break;
			}
		    }
		  if (i == MAX_STORE)
		    {
		      free_thing (th);
		      thing_nest[nest] = NULL;
		    }
		  return;
		}
	      else if (cln)
		{
		  for (i = 0; i < MAX_CLAN_STORE; i++)
		    {
		      if (cln->storage[i] == NULL)
			{
			  cln->storage[i] = th;
			  break;
			}
		    }
		  if (i == MAX_CLAN_STORE)
		    {
		      free_thing (th);
		      thing_nest[nest] = NULL;
		    }
		  return;
		}
	      if (!IS_SET (th->thing_flags, TH_IS_ROOM | TH_IS_AREA))
		{
		  free_thing (th);
		  thing_nest[nest] = NULL;
		}
	    }
	  else if (to)
	    {
	      thing_to (th, to);
	    }
	  else
	    {
	      for (i = nest - 1; i >= 0; i--)
		{
		  if (thing_nest[i])
		    {
		      thing_to (th, thing_nest[i]);
		      break;
		    }
		}
	      if (!th->in)
		{
		  free_thing (th);
		  thing_nest[nest] = NULL;
		} 
	    }
	  
	  badcount = 0;
	  return;
	}
      FILE_READ_ERRCHECK("read_short_thing");
      word[0] = '\0';
    } 
  if (th)
    {
      free_thing (th);
      thing_nest[nest] = NULL;
    }
  return;
}

/* These functions read and write a value */


VALUE *
read_value (FILE *f)
{
  int i;
  VALUE *newval;
  char word[STD_LEN];
  if (!f) 
    return NULL;
  newval = new_value ();
  strcpy (word, read_word(f));
  if (is_number (word))
    newval->type = atoi(word);
  else
    {
      for (i = 0; i < VAL_MAX; i++)
	{
	  if (!str_cmp (word, value_list[i]))
	    {
	      newval->type = i;
	      break;
	    }
	}
      if (i == VAL_MAX)
	newval->type = 0;
    }
  for (i = 0; i < NUM_VALS; i++)
    newval->val[i] = read_number (f);
  newval->timer = read_number (f);
  set_value_word (newval, read_string (f));
  newval->next = NULL;
  return newval;
}

void 
write_value (FILE *f, VALUE *value)
{
  int i;
  bool any_info = FALSE;
  if (!f || !value || value->type >= VAL_MAX || 
      value->type <= VAL_NONE)
    return;
  for (i = 0; i < NUM_VALS; i++)
    if (value->val[i] != 0)
      any_info = TRUE;
  if (value->word && *value->word)
    any_info = TRUE;
  if (!any_info)
    return;
  fprintf (f, "Val %s ", value_list[value->type]);
  for (i = 0; i < NUM_VALS; i++)
    fprintf (f, "%d ", value->val[i]);
  fprintf (f, "%d %s%c\n", value->timer, value->word, END_STRING_CHAR);
  return;
}


/* These read and write extra descriptions. */

EDESC *
read_edesc (FILE *f)
{
  EDESC *eds;
  if (!f || (eds = new_edesc()) == NULL)
    return NULL;
  eds->name = new_str (read_string (f));
  eds->desc = new_str (read_string (f));
  return eds;
}
  
/* This writes an edesc to a file. */

void
write_edesc (FILE *f, EDESC *eds)
{
  if (!f || !eds || !eds->name || !eds->desc)
    return;
  
  write_string (f, "EDESC", eds->name);
  fprintf (f, "%s%c\n", eds->desc, END_STRING_CHAR);
  return;
}

/* These two functions read and write a reset. */

RESET *
read_reset (FILE *f)
{
  RESET *newreset;
  if (!f || (newreset = new_reset ()) == NULL)
    return NULL;

  newreset->vnum = read_number (f);
  newreset->pct = read_number (f);
  newreset->times = read_number (f);
  newreset->nest = read_number (f);
  return newreset;
}

void
write_reset (FILE *f, RESET *reset)
{
  if (!f || !reset)
    return;
  fprintf (f, "Reset %d %d %d %d\n", reset->vnum, reset->pct, reset->times, reset->nest);
  return;
}

/* These two functions read and write a single flag to or from a 
   text file. They are together so they can be edited together if the
   structure of the flags changes. */

FLAG *
read_flag (FILE *f)
{
  FLAG *newflag;
  char word[STD_LEN];
  if (!f)
    return NULL;
  newflag = new_flag ();
  strcpy (word, read_word(f));
  if (is_number (word))
    newflag->type = atoi(word);
  else
    newflag->type = find_flagtype_from_name(word);
  
  newflag->from = read_number (f);
  newflag->timer = read_number (f);
  newflag->val = read_number (f);
  return newflag;
}

void 
write_flag (FILE *f, FLAG *flag)
{
  if (!f || !flag)
    return;
  if (flag->type == 0 && flag->from == 0 && flag->timer == 0 && flag->val == 0)
    return;
  if (flag->type < NUM_FLAGTYPES && flaglist[flag->type].whichflag != NULL)
    fprintf (f, "Flag %s %d %d %d\n",
	     capitalize(flaglist[flag->type].name), flag->from,
	     flag->timer, flag->val);
  else if (flag->type >= AFF_START && flag->type < AFF_MAX)
    fprintf (f, "Flag %s %d %d %d\n",
	     affectlist[flag->type - AFF_START].name, flag->from,
	     flag->timer, flag->val);
  return;
}






void
read_pcdata (FILE *f, THING *th)
{
  PCDATA *pc;
  char word[STD_LEN];
  int i, badcount = 0;
  SPELL *spl;
  
  bool found = FALSE;
  if (!IS_PC (th))
    th->pc = new_pc ();
  pc = th->pc;
  
  while (TRUE)
    {
      strcpy (word, read_word (f));
      found = FALSE;
      switch (UC(word[0]))
	{
	  case 'A':
	    if (!str_cmp (word, "AlignHate"))
	      {
		for (i = 0; i < ALIGN_MAX; i++)
		  pc->align_hate[i] = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "Alias"))
	      {
		for (i = 0; i < MAX_ALIAS; i++)
		  {
		    if (pc->aliasname[i] == nonstr &&
			pc->aliasexp[i] == nonstr)
		      break;
		  }
		if (i == MAX_ALIAS)
		  {
		    read_word (f);
		    read_string (f);
		  }
		else
		  {
		    pc->aliasname[i] = new_str (read_word (f));
		    pc->aliasexp[i] = new_str(read_string (f));
		  }
		found = TRUE;
	      }
	    if (!str_cmp (word, "Aff"))
	      {
		for (i = 0; i < (AFF_MAX - AFF_START); i++)
		  pc->aff[i] = read_number (f);
		found = TRUE;
	      }
	    break;
	  case 'B':
	    if (!str_cmp (word, "Bank"))
	      {
		pc->bank = read_number (f);
		found = TRUE;
	      }
	    break;
	  case 'C':
	    if (!str_cmp (word, "Cond"))
	      {
		for (i = 0; i < COND_MAX; i++)
		  pc->cond[i] = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "Clans"))
	      {
		for (i = 0; i < CLAN_MAX; i++)
		  pc->clan_num[i] = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "Channels"))
	      {
		for (i = 0; i < MAX_CHANNEL; i++)
		  pc->channel_off[i] = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "CurrNote"))
	      {
		pc->curr_note = read_number (f);
		found = TRUE;
	      }
	    break;
	  case 'D':
	    if (!str_cmp (word, "Damage"))
	      {
		pc->damage_total = read_number (f);
		pc->damage_rounds = read_number (f);
		found = TRUE;
	      }
	    break;
	  case 'E':
	    if (!str_cmp (word, "Email"))
	      {
		pc->email = new_str (read_string (f));
		found = TRUE;
	      }
	    if (!str_cmp (word, "Exp"))
	      {
		pc->exp = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "END_PCDATA"))
	      { 
		/* These are updated here since they must be done
		   after all items on the pc are loaded. */
		pc->login_item_xfer[pc->login_times % MAX_LOGIN] = 0;
		return;
	      }
	    break;
	  case 'G': 
	    if (!str_cmp (word, "Guilds"))
	      {
		for (i = 0; i < GUILD_MAX; i++)
		  pc->guild[i] = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "Gathered"))
	      {
		for (i = 0; i < RAW_MAX; i++)
		  pc->gathered[i] = read_number (f);
		found = TRUE;
	      }
	    break;
	  case 'I':
	    if (!str_cmp (word, "Implants"))
	      {
		for (i = 0; i < PARTS_MAX; i++)
		  pc->implants[i] = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "InVnum"))
	      {
		pc->in_vnum = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "Ignore"))
	      {
		for (i = 0; i < MAX_IGNORE; i++)
		  {
		    if (!pc->ignore[i] || !*pc->ignore[i])
		      {
			pc->ignore[i] = new_str (read_string (f));
			break;
		      }
		  }
		if (i == MAX_IGNORE)
		  read_string (f);
		found = TRUE;
	      }
	    
	    break;
	  case 'L':
	    /* Dependency: This part must come before the
	       items in the player inventory or else the item tracking
	       code won't work. It needs to be incremented so that it can be
	       cleared after the player logs in with all of his/her items. */
	    if (!str_cmp (word, "LoginXfer"))
	      {
		pc->login_times = read_number (f);
		pc->login_times++;
		for (i = 0; i < MAX_LOGIN; i++)
		  pc->login_item_xfer[i] = read_number (f);
		found = TRUE;
		
	      }
	    else if (!str_cmp (word, "LoginLength"))
	      {
		for (i = 0; i < MAX_LOGIN; i++)
		  pc->login_length[i] = read_number (f);
		found = TRUE;
	      }
	    break;
	  case 'M':
	    if (!str_cmp (word, "Mana"))
	      {
		pc->mana = read_number (f);
		pc->max_mana = read_number (f);
		found = TRUE;
	      }
	    break;
	  case 'O':
	    if (!str_cmp (word, "OffDef"))
	      {
		pc->off_def = read_number (f);
		if (pc->off_def > 100)
		  pc->off_def = 50;
		found = TRUE;
	      }
	    break;
	  case 'P':
	    if (!str_cmp (word, "Parts"))
	      {
		for (i = 0; i < PARTS_MAX; i++)
		  pc->parts[i] = read_number (f);
		found = TRUE;
	      } 
	    else if (!str_cmp (word, "PcArmor"))
	      {
		for (i = 0; i < PARTS_MAX; i++)
		  pc->armor[i] = read_number (f);
		found = TRUE;
	      }
	    else if (!str_cmp (word, "Pfile"))
	      {
		pc->pfile_sent = read_number (f);
		found = TRUE;
	      }
	    else if (!str_cmp (word, "Password"))
	      {
		pc->pwd = new_str (read_string (f));
		found = TRUE;
	      }
	    else if (!str_cmp (word, "Pagelen"))
	      {
		pc->pagelen = read_number (f);
		found = TRUE;
	      }
	    else if (!str_cmp (word, "Practices"))
	      {
		pc->practices = read_number (f);
		found = TRUE;
	      }
	    else if (!str_cmp (word, "Pkdata"))
	      {
		for (i = 0; i < PK_MAX; i++)
		  pc->pk[i] = read_number (f);
		found = TRUE;
	      }
	    else if (!str_cmp (word, "Prompt"))
	      {
		free_str (pc->prompt);
		pc->prompt = new_str (read_string (f));
		found = TRUE;
	      }
	    else if (!str_cmp (word, "PETTH"))
	      {
		read_short_thing (f, th, NULL);
	      }
	    
	    break;
	  case 'Q':
	    if (!str_cmp (word, "QFVal"))
	      {
		VALUE *nqf;
		nqf = read_value (f);
		nqf->next = pc->qf;
		pc->qf = nqf;
		found = TRUE;
	      } 
	    if (!str_cmp (word, "QuestPoints"))
	      {
		pc->quest_points = read_number(f);
		found = TRUE;
	      }
	    break;
	  case 'R':
	    if (!str_cmp (word, "Rumor"))
	      {
		pc->last_rumor = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "Race"))
	      {
		pc->race = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "Remorts"))
	      {
		pc->remorts = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "RoomIn"))
	      {
		int nest = read_number (f);
		int room_num = 0;
		if (nest == 0)
		  room_num = 2;
	      }
	    break;
	  case 'S': 
	    if (!str_cmp (word, "SPSK"))
	      {
		strcpy (word, read_string (f));
		if ((spl = find_spell (word, 0)) != NULL &&
		    spl->vnum < MAX_SPELL)
		  {
		    pc->prac[spl->vnum] = read_number (f);
		    pc->learn_tries[spl->vnum] = read_number (f);
		    pc->nolearn[spl->vnum] = read_number (f);
		  }
		else
		  {
		    read_number (f);
		    read_number (f);
		  }	       
		found = TRUE;
	      }
	    if (!str_cmp (word, "STOTH"))
	      {
		read_short_thing (f, th, NULL);
		found = TRUE;
	      }
	    if (!str_cmp (word, "Stats"))
	      {
		for (i = 0; i < STAT_MAX; i++)
		  pc->stat[i] = read_number (f);
		found = TRUE;
	      }
	    if (!str_cmp (word, "SocietyRewards"))
	      {
		for (i = 0; i < REWARD_MAX; i++)
		  pc->society_rewards[i] = read_number (f);
		found = TRUE;
	      }
	    break;
	  case 'T':
	    if (!str_cmp (word, "TH"))
	      {
		read_short_thing (f, th, NULL);
		found = TRUE;
	      }
	    if (!str_cmp (word, "Title"))
	      {
		free_str (pc->title);
		pc->title = new_str (add_color(read_string (f)));
		found = TRUE;
	      }
	    if (!str_cmp (word, "Time"))
	      {
		pc->time = read_number (f);
		pc->last_login = read_number (f);
		pc->login = current_time;
		found = TRUE;
	      }
	    if (!str_cmp (word, "Trophy"))
	      {
		int i;
		for (i = 0; i < MAX_TROPHY; i++)
		  {
		    if (pc->trophy[i] && !pc->trophy[i]->name[0])
		      {
			strcpy (pc->trophy[i]->name, read_word(f));
			pc->trophy[i]->level = read_number (f);
			pc->trophy[i]->remorts = read_number (f);
			pc->trophy[i]->times = read_number (f);
			pc->trophy[i]->align = read_number (f);
			pc->trophy[i]->race = read_number (f);
			break;
		      }
		  }
		if (i == MAX_TROPHY)
		  {
		    read_word (f);
		    read_number (f);
		    read_number (f);
		    read_number (f);
		    read_number (f);
		    read_number (f);
		  }
		found = TRUE;
	      }	 
	    break;
	  case 'W':
	    if (!str_cmp (word, "Warmth"))
	      {
		pc->warmth = read_number (f);
		found = TRUE;
	      }
	    else if (!str_cmp (word, "Wimpy"))
	      {
		pc->wimpy = read_number (f);
		found = TRUE;
	      }
	    break;
	}
      if (!found)
	{
	  char errbuf[STD_LEN];
	  sprintf (errbuf, "BAD WORD FOUND LOADING A PC!!! %s\n", word);
	  log_it (errbuf);
	  if (++badcount > 100)
	    {
	      return;
	    }
	}
    }
  return;
}


void
write_pcdata (FILE *f, THING *th)
{
  int i;
  PCDATA *pc;
  SPELL *spl;
  THING *thi, *thin, *thg;
  VALUE *qf, *pet;
  
  if (!f || !th || !IS_PC (th))
    return;
  pc = th->pc;
  fprintf (f, "\nPCDATA\n\n");
  if ((thi = th->in) != NULL)
    {
      while (!IS_ROOM (thi) && (thin = thi->in) != NULL)
	{
	  thi = thin;
	}
    }
  if (thi && IS_ROOM (thi) && (!thi->in || !IS_AREA_SET (thi->in, AREA_NOQUIT)))
    th->pc->in_vnum = thi->vnum;
  else
    th->pc->in_vnum = th->align + 100;
  fprintf (f, "InVnum %d\n", th->pc->in_vnum);
  fprintf (f, "Stats ");
  for (i = 0; i < STAT_MAX; i++)
    fprintf (f, "%d ", pc->stat[i]);
  fprintf (f, "\n");
  fprintf (f, "Guilds ");
  for (i = 0; i < GUILD_MAX; i++)
    fprintf (f, "%d ", pc->guild[i]);
  fprintf (f, "\n"); 
  fprintf (f, "Channels ");
  for (i = 0; i < MAX_CHANNEL; i++)
    fprintf (f, "%d ", pc->channel_off[i]);
  fprintf (f, "\n"); 
  fprintf (f, "SocietyRewards ");
  for (i = 0; i < REWARD_MAX; i++)
    fprintf (f, "%d ", pc->society_rewards[i]);
  fprintf (f, "\n"); 
  fprintf (f, "AlignHate ");
  for (i = 0; i < ALIGN_MAX; i++)
    fprintf (f, "%d ", pc->align_hate[i]);
  fprintf (f, "\n"); 
  fprintf (f, "Rumor %d\n", pc->last_rumor);
  fprintf (f, "Damage %d %d\n", pc->damage_total, pc->damage_rounds);
  fprintf (f, "Clans ");
  for (i = 0; i < CLAN_MAX; i++)
    fprintf (f, "%d ", pc->clan_num[i]);
  fprintf (f, "\n"); 
  fprintf (f, "Aff ");
  for (i = AFF_START; i < AFF_MAX; i++)
    fprintf (f, "%d ", pc->aff[i]);
  fprintf (f, "\n"); 

  fprintf (f, "Gathered ");
  for (i = 0; i < RAW_MAX; i++)
    fprintf (f, "%d ", pc->gathered[i]);
  fprintf (f, "\n"); 
  fprintf (f, "OffDef %d\n", pc->off_def);
  fprintf (f, "Pkdata ");
  for (i = 0; i < PK_MAX; i++)
    fprintf (f, "%d ", pc->pk[i]);
  fprintf (f, "\n"); 
  fprintf (f, "Cond ");
  for (i = 0; i < COND_MAX; i++)
    fprintf (f, "%d ", pc->cond[i]);
  fprintf (f, "\n");
  write_string (f, "Email", pc->email);
  write_string (f, "Title", pc->title);
  write_string (f, "Prompt", pc->prompt);
  fprintf (f, "QuestPoints %d\n", pc->quest_points);
  fprintf (f, "Time %d %d\n", pc->time + (current_time - pc->login),
	   pc->login);
  fprintf (f, "Exp %d\n", pc->exp);
  fprintf (f, "Pfile %d\n", pc->pfile_sent);
  write_string (f, "Password", pc->pwd);
  fprintf (f, "Race %d\n", pc->race);
  fprintf (f, "Remorts %d\n", pc->remorts);
  fprintf (f, "Bank %d\n", pc->bank);
  fprintf (f, "Implants ");
  for (i = 0; i < PARTS_MAX; i++)
    fprintf (f, "%d ", pc->implants[i]);
  fprintf (f, "\n");
  fprintf (f, "CurrNote %d\n", pc->curr_note);
  fprintf (f, "Mana %d %d\n", pc->mana, pc->max_mana);
  pc->in_vnum = (th->in ? th->in->vnum : 3);
  fprintf (f, "InVnum %d\n", pc->in_vnum);
  fprintf (f, "Pagelen %d\n", pc->pagelen);
  fprintf (f, "Practices %d\n", pc->practices);
  fprintf (f, "Wimpy %d\n", pc->wimpy);
  fprintf (f, "Warmth %d\n", pc->warmth);
  fprintf (f, "PcArmor ");
   for (i = 0; i < PARTS_MAX; i++)
     fprintf (f, "%d ", pc->armor[i]);
   fprintf (f, "\n");
  fprintf (f, "Parts ");
  for (i = 0; i < PARTS_MAX; i++)
    fprintf (f, "%d ", pc->parts[i]);
  fprintf (f, "\n");
  fprintf (f, "LoginXfer %d", pc->login_times);
  for (i = 0; i < MAX_LOGIN; i++)
    fprintf (f, " %d", pc->login_item_xfer[i]);
  fprintf (f, "\n"); 
  /* Set the current login length based on how much time was spent
     online... Since login_times only gets updated when the file is
     read, it should only be updated once per login. */
  pc->login_length[pc->login_times % MAX_LOGIN] = current_time - pc->login;
  fprintf (f, "LoginLength");
  for (i = 0; i < MAX_LOGIN; i++)
    fprintf (f, " %d", pc->login_length[i]);
  fprintf (f, "\n");  
  for (i = 0; i < MAX_IGNORE; i++)
    {
      if (pc->ignore[i] && *pc->ignore[i])
	{
	  write_string (f, "Ignore", pc->ignore[i]);
	}
    }
  for (i = 0; i < MAX_SPELL; i++)
    {
      if ((pc->prac[i] > 0 || pc->learn_tries[i] > 0 ||
	   pc->nolearn[i] > 0) 
	  && (spl = find_spell (NULL, i)) != NULL &&
	  spl->name && *spl->name)
	{
	  fprintf (f, "SPSK %s%c %d %d %d\n", 
		   spl->name, END_STRING_CHAR, 
		   pc->prac[i], pc->learn_tries[i],
		   pc->nolearn[i]);
	}
    }
  for (i = 0; i < MAX_ALIAS; i++)
    {
      if (pc->aliasname[i] && pc->aliasname[i] != nonstr &&
	  pc->aliasexp[i] && pc->aliasexp[i] != nonstr)
	{
	  fprintf (f, "Alias %s %s%c\n", pc->aliasname[i], pc->aliasexp[i], END_STRING_CHAR);
	}
    }
  for (i = 0; i < MAX_STORE; i++)
    {
      if (pc->storage[i] && pc->storage[i]->proto &&
	  !IS_SET (pc->storage[i]->proto->thing_flags, TH_NUKE))
	{
	  write_short_thing (f, pc->storage[i], 0);
	}
    }
  fprintf (f, "\n"); 
  for (i = 0; i < MAX_TROPHY; i++)
    {
      if (pc->trophy[i] && pc->trophy[i]->name[0])
	{
	  fprintf (f, "Trophy %s %d %d %d %d %d\n",
		   pc->trophy[i]->name, 
		   pc->trophy[i]->level,
		   pc->trophy[i]->remorts,
		   pc->trophy[i]->times,
		   pc->trophy[i]->align,
		   pc->trophy[i]->race);
	}
    }
  
  if (th->in)
    {
      for (thg = th->in->cont; thg; thg = thg->next_cont)
	{
	if ((pet = FNV (thg, VAL_PET)) != NULL &&
	    pet->word && *pet->word &&
	    !str_cmp (pet->word, NAME (th)))
	  write_short_thing (f, thg, 1);
	}
    }
  
  qf = pc->qf;
  while (qf)
    {
      fprintf (f, "QF");
      write_value (f, qf);
      qf = qf->next;
    }
  
  for (thg = th->cont; thg; thg = thg->next_cont)
    {
      write_short_thing (f, thg, 1);
      RBIT (thg->thing_flags, TH_SAVED);
    }	

  fprintf (f, "\nEND_PCDATA\n\n");
  return;
}




void
do_worldsave (THING *th, char *arg)
{
  if (!str_cmp (arg, "yesnow"))
    {
      stt ("Saving the world.\n\r", th);
      init_write_thread();
      return;
    }
  if (!str_cmp (arg, "changed"))
    {
      stt ("Saving changed areas.\n\r", th);
      write_changed_areas (th);
      
      if (IS_SET (server_flags, SERVER_CHANGED_SOCIETIES))
	{
	  write_societies ();
	  stt ("Societies saved.\n\r", th);
	  RBIT (server_flags, SERVER_CHANGED_SOCIETIES);
	}      
    }
  if (!str_cmp (arg, "code"))
    {
      stt ("Writing code and triggers.\n\r", th);
      write_codes ();
      write_triggers ();
    }
  if (IS_SET (server_flags, SERVER_CHANGED_SPELLS))
    {
      write_spells ();
      stt ("Spells changed.\n\r", th);
      RBIT (server_flags, SERVER_CHANGED_SPELLS);
    }
  if (!str_cmp (arg, "clans"))
    {
      stt ("Clans saved.\n\r", th);
      write_clans ();
    }
  if (!str_prefix ("race", arg) || !str_prefix ("align", arg))
    {
      write_races ();
      write_aligns ();
      stt ("Aligns and races saved.\n\r", th);
    }
  return;
}

