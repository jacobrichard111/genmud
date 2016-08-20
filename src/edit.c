#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "areagen.h"
#include "society.h"
#include "script.h"
#include "track.h"
#ifdef USE_WILDERNESS
#include "wilderness.h"
#endif
#include "worldgen.h"
/* This lets a character begin to edit a mob/room/obj/area, or
   edit code <name> lets the char edit code for scripts. */


void
do_edit (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char arg2[STD_LEN];
  char buf[STD_LEN];
  THING *thg, *ar;
  int vnum, areasize = 1000, i, j, codecount = 0;
  char *t;
  bool creating = FALSE;
  bool area = FALSE;
  
  if (!th || !th->fd || !IS_PC (th) || LEVEL (th) < BLD_LEVEL)
    {
      stt ("Huh?\n\r", th);      
      return;
    }
  if (LEVEL (th) == MAX_LEVEL)
    {
      if ((thg = find_pc (th, arg)) == NULL)
	{
	  arg1[0] = UC(arg1[0]);
	  thg = read_playerfile (arg1);
	}
      if (thg)
	{
	  th->pc->editing = thg;
	  SBIT (th->fd->flags, FD_EDIT_PC);
	  th->fd->connected = CON_EDITING;
	  show_edit (th);
	  return;
	}
    }
  arg = f_word (arg, arg1);
  arg = f_word (arg, arg2);
  if (arg1[0] == '\0')
    {
      if (th->in)
	{
	  stt ("Ok, editing this thing now!\n\r", th);
	  th->fd->connected = CON_EDITING;
	  th->pc->editing = (THING *) th->in;
	  show_edit (th);
	  return;
	}
      else
	{
	  stt ("What do you want to edit? Syntax edit <type> <number> or edit create> <type>\n\r", th);
      return;
	}
    }
  if (!str_cmp (arg1, "this") &&
      IS_PC (th) && th->pc->last_tstat > 0 &&
      (thg = find_thing_num (th->pc->last_tstat)) != NULL)
    {
      stt ("Ok, editing this thing now!\n\r", th);
      th->fd->connected = CON_EDITING;
      th->pc->editing = (THING *) thg;
      show_edit (th);
      return;
    }
  if (is_number (arg1) || !*arg1)
    {
      vnum = atoi (arg1);
     
      if (vnum > 0)
	{
	  if ((thg = find_thing_num (vnum))  == NULL)
	    {
	      stt ("That thing does not exist to edit!\n\r", th);
	      return;
	    }
	  stt ("Ok, editing this thing now!\n\r", th);
	  th->fd->connected = CON_EDITING;
	  th->pc->editing = (THING *) thg;
	  show_edit (th);
	  return;
	}
      else
	{
	  stt ("You must edit a vnum greater than 0.\n\r", th);
	  return;      
	}
    }
  else if (!str_cmp (arg1, "code"))
    {
      CODE *code;
      if (LEVEL (th) < MAX_LEVEL)
	{
	  stt ("Only admins can edit code.\n\r", th);
	  return;
	}
      if (!str_cmp (arg2, "list"))
	{
	  bigbuf[0] = '\0';
	  add_to_bigbuf ("All code chunk names:\n\n\r");
	  for (i = 0; i < CODE_HASH; i++)
	    {
	      for (j = 0; j < CODE_HASH; j++)
		{
		  for (code = code_hash[i][j]; code != NULL; code = code->next)
		    {
		      codecount++;
		      sprintf (buf, "%-12s", code->name);
		      add_to_bigbuf (buf);
		      if (++codecount == 6)
			{
			  add_to_bigbuf ("\n\r");
			  codecount = 0;
			}
		    }
		}
	    }
	  send_bigbuf (bigbuf, th);
	  return;
	}
      if (!str_cmp (arg2, "delete"))
	{
	  CODE *prev;
	  t = arg;
	  if (!*t || *t < 'a' || *t > 'z' ||
	      !*(t + 1) || *(t + 1) < 'a' || *(t + 1) > 'z'
	      || (code = find_code_chunk (t)) == NULL)
	    {
	      stt ("That code doesn't exist to delete!\n\r", th);
	      return;
	    }
	  if (code == code_hash[*t - 'a'][*(t + 1) - 'a'])
	    {
	      code_hash[*t - 'a'][*(t + 1) - 'a'] = code_hash[*t - 'a'][*(t + 1) - 'a']->next;
	      code->next = NULL;
	    }
	  else
	    {
	      for (prev =  code_hash[*t - 'a'][*(t + 1) - 'a']; prev; prev = prev->next)
		{
		  if (prev->next == code)
		    {
		      prev->next = code->next;
		      code->next = NULL;
		    }
		}
	    }
	  free_code (code);
	  stt ("Ok, code chunk deleted.\n\r", th);
	  return;
	}
      t = arg2;
      if (!*t || *t < 'a' || *t > 'z' ||
	  !*(t + 1) || *(t + 1) < 'a' || *(t + 1) > 'z')
	{
	  stt ("All code names must start with two lowercase letters.\n\r", th);
	  return;
	}
      for (t = arg2; *t; t++)
	{
	  if (!isalnum (*t))
	    {
	      stt ("The code name must be all letters and numbers.\n\r", th);
	      return;
	    }
	}
      if ((code = find_code_chunk (arg2)) == NULL)
	{
	  stt ("Ok creating a new code chunk.\n\r", th);
	  code = new_code ();
	  t = arg2;
	  /* Add the code to the hash_table */
	  th->pc->pcflags |= PCF_EDIT_CODE;
	  code->next =  code_hash[*t -'a'][*(t + 1) - 'a'];
	  code_hash[*t -'a'][*(t + 1) - 'a'] = code;
	  code->name = new_str (arg2);
	}
      new_stredit (th, &code->code);
      return;
    }
  
  else if (!str_cmp (arg1, "create"))
    {
      creating = TRUE;
      if (!str_cmp (arg2, "area"))
	{
	  arg = f_word (arg, arg1);
	  area = TRUE;
	  if (LEVEL (th) < MAX_LEVEL)
	    {
	      stt ("Only admins can make new areas.\n\r", th);
	      return;
	    }
	  if (!is_number (arg1))
	    {
	      stt ("Syntax: edit <num>, edit create <num>, edit create area <num>\n\r", th);
	      return;
	    }
	  else
	    vnum = atoi (arg1);
	  if (vnum < 1)
	    {
	      stt ("You can't edit things smaller than 1.\n\r", th);
	      return;
	    }
#ifdef USE_WILDERNESS
	  if (vnum >= WILDERNESS_MIN_VNUM && vnum <= WILDERNESS_MAX_VNUM)
	    {
	      stt ("You can't create an area within the wilderness area.", th);
	      return;
	    }
#endif
	  if (is_number (arg))
	    areasize = atoi (arg);
	  if (areasize < 20)
	    {
	      stt ("The area must be at least 20 rooms big.\n\r", th);
	      return;
	    }
	  
	  if(!check_area_vnums (NULL, vnum, vnum + areasize - 1))
	    {
	      stt ("That vnum is already inside of an area!\n\r", th);
	      return;
	    }	 
	  
	} 
      else if (!is_number (arg2) && *arg2)
	{
	  stt ("Syntax: edit <num> or edit create <num> or edit create area <num>\n\r", th);
	  return;
	}
      else 
	{
	  THING *edt = (THING *) th->pc->editing;
	  if (!*arg2 && edt && th->fd &&
	      th->fd->connected == CON_EDITING)
	    {
	      int j;
	      vnum = edt->vnum;
	      for (j = 0; j < 200; j++)
		{
		  if ((thg = find_thing_num (vnum + j)) == NULL &&
		      edt->in &&
		      (ar = find_area_in (vnum + j)) != NULL &&
		      ar->vnum == 
		      edt->in->vnum)
		    {
		      vnum += j;
		      break;
		    }
		}
	    }
	  else
	    {
	      vnum = atoi (arg2);
	      if ((thg = find_thing_num (vnum))  != NULL)
		{
		  stt ("Editing the old thing now!\n\r", th);
		  th->fd->connected = CON_EDITING;
		  th->pc->editing = (THING *) thg;
		  show_edit (th);
		  return;
		}
	    }
	  if ((ar = find_area_in (vnum)) == NULL &&
	      edt && !IS_AREA (edt))
	    {
	      stt ("That vnum is not assigned to an area.\n\r", th);
	      return;
	    }
	  if (LEVEL (th) < MAX_LEVEL && !named_in (ar->type, th->name))
	    {
	      stt ("You cannot edit this number.\n\r", th);
	      return;
	    }	  
	}
    }
  else
    {
      stt ("Syntax: edit <num> or edit create <num> or edit create area <num>\n\r", th);
      return;
    }  

  if ((thg = new_thing ()) != NULL)
    {
      /* Make the item and add it to the list... */
      thg->vnum = vnum;
      top_vnum = MAX (top_vnum, vnum);
      add_thing_to_list (thg);
      th->pc->editing = (THING *) thg;
      th->fd->connected = CON_EDITING;
      if (area)
	{
	  set_up_new_area (thg, areasize);
	}
      else
	{
	  THING *area_in = find_area_in (vnum);
	  if (!area_in)
	    {
	      free_thing (thg);
	      return;
	    }
	  /* Add rooms to the area in. --between vnum and vnum+armor */
	  area_in->thing_flags |= TH_CHANGED;
	  if (area_in)
	    {
	      thing_to (thg, area_in);
	      if (vnum > area_in->vnum && 
		  vnum <= (area_in->vnum + area_in->mv))
		thg->thing_flags = ROOM_SETUP_FLAGS;
	      
	    }
	}
      show_edit (th);
      return;
    }
  return;
}

/* This sets up a new area. */

void
set_up_new_area (THING *thg, int areasize)
{
  char arg1[STD_LEN];
  thg->max_mv = areasize;       /* Num things */
  thg->mv = areasize *3/4;  /* Num rooms */
  thg->thing_flags |= TH_IS_AREA | TH_CHANGED |TH_NO_TAKE_BY_OTHER |
    TH_NO_MOVE_SELF | TH_NO_DRAG | TH_NO_GIVEN | TH_NO_DROP |
    TH_NO_CAN_GET | TH_NO_MOVE_BY_OTHER | TH_NO_PRY |
    TH_NO_MOVE_OTHER | TH_NO_TAKE_FROM | TH_NO_MOVE_INTO |
    TH_NO_LEAVE | TH_NO_FIGHT;
  sprintf (arg1, "area%d.are", thg->vnum);
  thg->name = new_str (arg1);
  thing_to (thg, the_world);  
  return;
}
      
void
show_edit (THING * th)
{
  char buf[STD_LEN];
  char buf2[STD_LEN];
  char armbuf[STD_LEN];
  char levbuf[STD_LEN];
  char hpbuf[STD_LEN];
  char mvbuf[STD_LEN];
  int pad, i, len;
  char *t;
  char padbuf[STD_LEN];
  char huntbuf[STD_LEN];
  THING *thg;
  EDESC *eds;
  buf[0] = '\0';
  buf2[0] = '\0';
  

  if (!th || !th->fd || !IS_PC (th) || !th->pc->editing ||
     IS_SET (server_flags, SERVER_SUPPRESS_EDIT))
    return;

  thg = (THING *) th->pc->editing;
  last_thing_edit = current_time;
  
    {
      sprintf (armbuf, "Arm");
      sprintf (levbuf, "Lev");
    }
  
  if (is_hunting (thg))
    sprintf (huntbuf, "Hunt: %s (%d)", thg->fgt->hunting, thg->fgt->hunting_type);
  else
    huntbuf[0] = '\0';
  

  
  if (thg->in && (IS_AREA (thg->in)) && thg->in != the_world)
    { 
      thg->in->thing_flags |= TH_CHANGED;
      sprintf (hpbuf, "HpMult");
      sprintf (mvbuf, "MIW");
    }
  else if (IS_AREA (thg))
    {
      thg->thing_flags |= TH_CHANGED;
      sprintf (hpbuf, "Hp?");
      sprintf (mvbuf, "NumTh/Rm");
    }
  else
    {
      sprintf (hpbuf, "MaxHp");
      sprintf (mvbuf, "Mv/MaxMv");
    }
  
  sprintf (buf, "[\x1b[1;37m%d\x1b[0;37m]%s: Area: \x1b[1;36m%d\x1b[0;37m Lev: \x1b[1;32m%d\x1b[0;37m Arm: \x1b[1;37m%d\x1b[0;37m Light: \x1b[1;33m%d\x1b[0;37m sex: \x1b[1;36m%s\x1b[0;37m exits: %d (goodroom_exits: %d)\n\r",
	   thg->vnum, 
	   thg->desc && *thg->desc ?"\x1b[1;36m(D)\x1b[0;37m":"", 
	   (thg->in  && !IS_PC (thg) ? thg->in->vnum :  0), 
	   thg->level, thg->armor, thg->light,
	   (thg->sex < SEX_MAX ? sex_names[thg->sex] : "error"),
	   thg->exits, thg->goodroom_exits);
  
  stt (buf, th);
  
  sprintf (buf, "Align: \x1b[1;35m%d\x1b[0;37m  Wear \x1b[1;36m%s\x1b[0;37m  Cst: \x1b[1;33m%-6d\x1b[0;37m Timer: \x1b[1;31m%d\x1b[0;37m %s\x1b[0;37m \n\r",
	  thg->align, (thg->wear_pos > 0 && thg->wear_pos  < ITEM_WEAR_MAX ?
	    wear_data[thg->wear_pos].name : "none"),
	   thg->cost, thg->timer, huntbuf);
  stt (buf, th);
  
  sprintf (buf, "\x1b[1;37mName\x1b[1;36m :\x1b[0;37m %-20s %s: \x1b[1;36m%d/%d\x1b[0;37m  %s \x1b[1;37m%d\x1b[0;37m(\x1b[1;34m%d)\x1b[0;37m Size: \x1b[1;37m%d\x1b[0;37m\n\r", thg->name, hpbuf,
thg->hp, thg->max_hp,  mvbuf, thg->max_mv, thg->mv, thg->size);
  stt (buf, th);
  
  pad = 28 - strlen_color (thg->short_desc);
  if (pad < 0)
    pad = 0;
  for (i = 0; i < pad; i++)
    padbuf[i] = ' ';
  padbuf[pad] = '\0';
  sprintf (buf, "\x1b[1;37mShort\x1b[1;36m:\x1b[0;37m %s%s Wgt: \x1b[1;37m%d\x1b[0;37m Hgt: \x1b[1;32m%d\x1b[0;37m %s: %s\n\r" , thg->short_desc, padbuf, thg->weight, thg->height, (!IS_AREA(thg) ? "Type" : "Builders"), thg->type);  
  stt (buf, th);
  sprintf (buf, "\x1b[1;37mLong\x1b[1;36m :\x1b[0;37m %s\n\r", thg->long_desc);
  stt (buf, th);
  sprintf (buf, "%s\n\r", list_flagnames (FLAG_THING, thg->thing_flags));
  stt (buf, th);
  if (IS_PC (thg))
    {
      sprintf (buf, "C/M hp: %d/%d C/M mv: %d/%d\n\r", thg->hp, thg->max_hp, thg->mv, thg->max_mv);
      stt (buf, th);
      
    }
  if (IS_SET (server_flags, SERVER_TSTAT) &&
      thg->fgt && thg->fgt->hunt_victim)
    {
      sprintf (buf, "Hunting: %s: %s\n",
	       thg->fgt->hunting, NAME(thg->fgt->hunt_victim));
      stt (buf, th);
    }
  stt (show_values (thg->values, 0, (IS_SET (server_flags, SERVER_TSTAT) ? TRUE : FALSE)), th);
  stt (show_flags (thg->flags, 0, LEVEL (th) >= BLD_LEVEL), th);
  if (LEVEL (th) >= BLD_LEVEL)
    {
      stt (show_resets (thg->resets), th);
      /* Show the extra descriptions by showing the names then adding
	 a brief piece of the description. */
      for (eds = thg->edescs; eds; eds = eds->next)
	{
	  len = 0;
	  sprintf (buf, "EDesc: \x1b[1;35m%s\x1b[0;37m: ", eds->name);
	  for (t = eds->desc; *t && *t != '\n'; t++)
	    len++;	  
	  strncat (buf, eds->desc, MIN (len, 80-(int) strlen(buf)));
	  strcat (buf, "...\n\r");
	  stt (buf, th);
	}
      show_tracks (th, thg);  
      see_needs (th, thg);
    }
  return;
}


/* Check if a given vnum lies between two area numbers or not. */
/* It checks if the lower number is inside an area, or if the upper number
   is inside of an area or if an area lvnum lies between lvnum and uvnum
   or if an area uvnum lies in between lvnum and uvnum...I think this
   is all the cases we need. If not, fix it up. */

bool 
check_area_vnums (THING *thg, int l, int u)
{
  THING *ar;
  int al, au;
  for (ar = the_world->cont; ar != NULL; ar = ar->next_cont)
    {
      if (thg == ar ||
	  (ar->vnum >= l && ar->vnum <= u)) 
	continue;
      al = ar->vnum;
      au = ar->vnum + ar->max_mv; 
      
      
      /* Checks if the lower vnum you put in is between the two end vnums in
	 an area, it checks if the upper vnum is between the two end vnums
	 in an area, then it checks if the two vnums contain an area */
      
      if ((al <= l && l <  au) ||
	  (al <= u && u <  au) ||
	  (l <= al && au <= u))
	return FALSE;
    }
  return TRUE;
}



void
edit (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char arg2[STD_LEN];
  char buf[STD_LEN];
  int value, i, valnum, valtype, dir, nvnum;
  char *oldarg = arg;
  THING *thg, *rpc;
  THING *nroom, *ar, *nar = NULL;
  char *t;
  VALUE *exit, *nexit;
  bool explicit_dig = FALSE;
  arg = f_word (arg, arg1);
  if (!th->fd || th->fd->connected != CON_EDITING || 
      !IS_PC (th) || (thg = (THING *) th->pc->editing) == NULL)
    {
      if(IS_PC (th))
	th->pc->editing = NULL;
      if (th->fd)
	th->fd->connected = CON_ONLINE;
      return;
    }
  if (!str_cmp (arg1, "done"))
    {
      if (IS_SET (th->fd->flags, FD_EDIT_PC))
	{
	  if ((rpc = (THING *) th->pc->editing) != NULL)
	    {
	      write_playerfile (rpc);
	      if (!rpc->fd)
		free_thing (rpc);
	    }
	  RBIT (th->fd->flags, FD_EDIT_PC);
	}
      if (th->fd)
	th->fd->connected = CON_ONLINE;
      stt ("Ok, all done.\n\r", th);
      return;
    }
  if (LEVEL (th) < MAX_LEVEL && 
      (IS_PC (thg) ||
       (ar = find_area_in (thg->vnum)) == NULL ||
       !named_in (ar->type, th->name)))
    {
      stt ("You cannot edit this number.\n\r", th);
      if (!*oldarg)
	show_edit (th);
      else
	interpret (th, oldarg);      
      return;
    }	
  if (arg1[0] == '\0')
    {
      show_edit (th);
      return;
    }

  /* Deal with exit stuff. You must type <dir>, and have an argument left
     over, and be in a room which you are editing which must be in an area. */
  /* We only delete and dig/link. No point in doing room...just set 
     those up using values directly. */


  if ((dir = find_dir (arg1)) < REALDIR_MAX && *arg 
      && IS_ROOM (thg) && 
      (thg == th->in || IS_SET (server_flags, SERVER_SUPPRESS_EDIT)) &&
      (ar = thg->in) != NULL && IS_AREA(ar))
    {
      arg = f_word (arg, arg1);
      if (!str_prefix (arg1, "dig") || !str_prefix (arg1, "link"))
	{
	  if (!*arg || !is_number (arg))
	    nvnum = thg->vnum;
	  else
	    {
	      explicit_dig = TRUE;
              nvnum = atoi (arg);	    
	      if ((nar = find_area_in (nvnum)) != ar)
		{
		  stt ("You can only dig/link to the same area.\n\r", th);
		  return;
		}
	      if (nvnum < ar->vnum || nvnum >= ar->vnum + ar->mv)
		{
		  stt ("That would not end up being a room!\n\r", th);
		  return;
		}
	    }
	  sprintf (buf, "%d %d\n\r", thg->vnum, ar->vnum + ar->mv);
	  stt (buf, th);
	  if (nvnum == thg->vnum)
	    {
	      for (i = thg->vnum + 1; i <= ar->vnum + ar->mv; i++)
		{
		  if ((nroom = find_thing_num (i)) == NULL)
		    {
		      nvnum = i;
		      break;
		    }
		}
	    }
	  sprintf (buf, "%d %d\n", thg->vnum, nvnum);
	  stt (buf, th);
	  if ((nroom = find_thing_num (nvnum)) == NULL)
	    {
	      sprintf (buf, "create %d", nvnum);
	      do_edit (th, buf);
	    }
	  nroom = find_thing_num (nvnum);
	  if (!nroom || !thg)
	    return;
	  if ((exit = FNV (thg, dir + 1)) == NULL)
	    {
	      exit = new_value ();
	      exit->type = dir + 1;
	      add_value (thg, exit);
	    }
	  exit->val[0] = nvnum;
	  
	  if ((nexit = FNV (nroom, RDIR (dir) + 1)) == NULL)
	    {
	      nexit = new_value ();
	      nexit->type = RDIR (dir) + 1;
	      add_value (nroom, nexit);
	    }
	  /* If nexit was pointing to something, change the old exit
	     pointing to the new room. This isn't necessary, but I've
	     found that it makes building easier for me since I don't
	     have to clean up old exits. 
	     
	     Basically if you dig into a room and it has an exit going
	     to another room, and that other room has an exit to
	     the room being dug to, the exit gets nuked. */
	  
	  else
	    {
	      THING *oldroom;
	      VALUE *oldexit;
	      
	      if ((oldroom = find_thing_num (nexit->val[0])) != NULL &&
		  IS_ROOM (oldroom) &&
		  (oldexit = FNV (oldroom, dir + 1)) != NULL &&
		  oldexit->val[0] == nroom->vnum &&
		  thg != oldroom)
		{
		  remove_value (oldroom, oldexit);
		}
	    }
	  
	  nexit->val[0] = thg->vnum; 
	  
	  
	  /* This was changed so making new rooms send you to the room,
	     and just linking leaves you where you are. */
	  if (!explicit_dig)
	    {
	      thing_from (th);
	      thing_to (th, nroom);
	      th->pc->editing = nroom;
	      do_look (th, "zzduhql");
	    }
	  show_edit (th);
	  return;
	}
      if (!str_prefix (arg1, "delete"))
	{
	  if ((exit = FNV (thg, dir + 1)) == NULL)
	    stt ("There is no exit that way.\n\r", th);
	  else
	    {
	      stt ("Exit removed.\n\r", th);
	      remove_value (thg, exit);
	    }
	  show_edit (th);
	  return;
	}
      stt ("<dir> dig/link/delete\n\r", th);
      return;
    }
  
  switch (UC(arg1[0]))
    {
      case 'A':
	if (!str_cmp (arg1, "armor"))
	  {
	    value = atoi (arg);
	    
	    if (value < 0 || value > 20000)
	      {
		stt ("The armor must be between 0 and 20000.\n\r", th);
		return;
	      }
	    thg->armor = value;
	    stt ("Armor val set.\n\r", th);
	    show_edit (th);
	    return;
	  }
	if (!str_cmp (arg1, "align"))
	  {
	    value = atoi (arg);
	    if (value >= 0 && value < ALIGN_MAX && align_info[value])
	      {
		thg->align = value;
		stt ("Ok, alignment changed.\n\r", th);
		show_edit (th);
		return;
	      }
	    else
	      {
		stt ("Please pick an alignment number.\n\r", th);
		return;
	      }
	  }
	break;
      case 'C':
	if (!str_cmp (arg1, "cost"))
	  {
	    value = atoi (arg);
	    if (value < 0 || value > 20000000)
	      {
		stt ("The cost must be between 0 and 20000000.\n\r", th);
	      }
	    else
	      {
		thg->cost = value;
		stt ("Cost set.\n\r", th);
	      }
	    show_edit (th);
	    return;
	  }
	if (!str_cmp (arg1, "copyfrom"))
	  {
	    THING *oth, *from;
	    value = atoi (arg);	
	    
	    if ((oth = find_thing_num (value)) == NULL)
	      {
		stt ("copyfrom <vnum>\n\r", th);
		return;
	    }
	    
	    if (!oth->desc || !*oth->desc)
	      {
		stt ("That thing doesn't have a description!\n\r", th);
		return;	    
	      }
	    
	    /* Where are we copying this from...? */
	    value = 0;
	    t = oth->desc;
	    while (isdigit (*t))
	      {
		value *= 10;
		value += *t - '0';
		if (value >= 1000000000)
		  break;
		t++;
	      }
	    
	    if ((from = find_thing_num (value)) != NULL &&
		from->desc && *from->desc)
	    oth = from;
	    
	 	      
	    
	    if (oth == thg)
	      {
		stt ("You cannot copy over a thing's own desc.\n\r", th);
		return;
	      }
	    free_str (thg->desc);	  
	    sprintf (buf, "%d", oth->vnum);
	    thg->desc = new_str (buf);
	    stt ("Desc copied.\n\r", th);
	    show_edit (th);
	    return;
	  }
	if (!str_cmp (arg1, "copyto"))
	  {
	    THING *nth, *from;
	    char *t;
	    value = atoi (arg);
	    from = thg;
	    
	    if (!thg->desc || !*thg->desc)
	      {
		stt ("There is no description on this thing!\n\r", th);
		return;
	      }
	    
	    if ((nth = find_thing_num (value)) == NULL)
	      {
		stt ("copyto <vnum>\n\r", th);
		return;
	      }
	    t = thg->desc;
	    value = 0;
	    while (isdigit (*t))
	      {
		value *= 10;
		value += *t - '0';
		if (value >= 1000000000)
		  break;
		t++;
	      }
	    
	    if ((from = find_thing_num (value)) == NULL ||
		!from->desc || !from->desc)
	      from = thg;
	    
	    
	    
	    if (nth == from)
	      {
		stt ("You cannot copy over a thing's own desc.\n\r", th);
		return;
	      }
	    free_str (nth->desc);
	    sprintf (buf, "%d", from->vnum);
	    nth->desc = new_str (buf);
	    stt ("Desc copied.\n\r", th);
	    show_edit (th);
	    return;
	  }
	if (!str_cmp (arg1, "color"))
	  {
	    
	    if (*arg >= '0' && *arg <= '9')
	      thg->color = *arg - '0';
	    else if (LC (*arg) >= 'a' && LC (*arg) <= 'f')
	      thg->color = LC(*arg) - 'a';
	    else 
	      {
		stt ("Color 0-F.\n\r", th);
		return;
	      }
	    stt ("Ok, Color changed.\n\r", th);
	    show_edit (th);
	    return;
	  }
	break;
      case 'D':
	if (!str_cmp (arg1, "desc") || !str_prefix ("descr", arg1))
	  {
	    THING *newedit;
	    if ((value = atoi (thg->desc)) > 0 &&
		(newedit = find_thing_num (value)) != NULL &&
		newedit->desc && *newedit->desc)
	      {
		free_str (thg->desc);
		thg->desc = new_str (newedit->desc);
	      }
	    new_stredit (th, &thg->desc);
	    if (thg->in && IS_AREA (thg->in))
	      SBIT (thg->in->thing_flags, TH_CHANGED);
	    return;
	  }
	break;	
      case 'E':
	/* Extra descriptions...add auto on name and add descs after that. */
	
	if (named_in ("eds edesc", arg1) &&
	    strlen (arg1) > 1)
	  {
	    EDESC *eds;
	    char *eds_arg; /* Used for parsing new names. */
	    arg = f_word (arg, arg1);
	    if (!str_cmp (arg1, "delete"))
	      {
		if ((eds = find_edesc_thing (thg, arg, TRUE)) != NULL)
		  {
		    remove_edesc (thg, eds);
		    free_edesc (eds);
		    stt ("Extra description removed.\n\r", th);
		    show_edit (th);
		    return;
		  }
		else
		  {
		    stt ("That thing doesn't have that extra description on it.\n\r", th);
		    return;
		  }
	      }
	    if (!str_cmp (arg1, "add"))
	      {
		if (!*arg || isspace (*arg))
		  {
		    stt ("edesc add <names>\n\r", th);
		    return;
		  }

		if ((eds = find_edesc_thing (thg, arg, TRUE)) != NULL)
		  {
		    stt ("This extra description already exists!\n\r", th);
		    return;
		  }
		eds = new_edesc();
		eds->name = new_str (arg);
		add_edesc (thg, eds);		
		stt ("New edesc added.\n\r", th);
		show_edit (th);
		return;
	      }
	    if ((eds = find_edesc_thing (thg, arg1, TRUE)) != NULL)	      
	      {
		if (!str_cmp (arg, "desc") || !str_cmp (arg, "description"))
		  {
		    new_stredit (th, &eds->desc);
		    return;
		  }
		if (!arg || !*arg)
		  {
		    stt ("edesc <name> <new names>. The new names cannot be blank\n\r", th);
		    return;
		  }
		eds_arg = arg;
		while (*eds_arg && eds_arg)
		  {
		    eds_arg = f_word (eds_arg, arg1);		    
		    if (!str_cmp (arg1, "add") || !str_cmp (arg1, "delete") ||
			!str_cmp (arg1, "desc") || !str_cmp (arg1, "description"))
		      {
			stt ("Your object name cannot contain the words: add, delete, desc, or description.\n\r", th);
			return;
		      }
		  }
		
		free_str (eds->name);
		eds->name = new_str (arg);
		stt ("List of names changed on extra description.\n\r", th);
		show_edit (th);
		return;
	      }
	    stt ("edesc: add <name>, delete <name>,  <name> desc, <name> <new names>\n\r", th);
	    return;
	  }


      case 'F':
	if (!str_cmp (arg1, "flag") || !str_cmp (arg1, "flags"))
	{
	  edit_flags (th, &thg->flags, arg);
	  show_edit (th);
	  return;
	}    
      break;
    case 'H':
      if (!str_cmp (arg1, "hpmult") || !str_cmp (arg1, "hp_mult"))
	{
	  value = atoi (arg);
	  if (value > 1 && value < 1001)
	    {
	      thg->max_hp = value;
	      stt ("Hp_mult set.\n\r", th);
	    }
	  else
	    {
	      stt ("The value must be between 2 and 1000.\n\r", th);
	    }
	  show_edit (th);
	  return;
	}
      if (!str_cmp (arg1, "height"))
	{
	  value = atoi (arg);
	  if (value >= 0 && value <= 10000)
	    {
	      thg->height = value;
	      stt ("Height set.\n\r", th);
	    }
	  else
	    {
	      stt ("The value must be between 0 and 10000\n\r", th);
	    }
	  show_edit (th);
	  return;
	}
      
      break;
    case 'L':
      if (!str_cmp (arg1, "level"))
	{
	  value = atoi (arg);
	  
	  if (value >= 0 && value <= 500)
	    {
	      if (IS_PC (thg))
		value = MID (1, value, MAX_LEVEL);
	      thg->level = value;
	      stt ("Level set.\n\r", th);
	    }
	  else
	    {
	      stt ("The Level must be between 0 and 500\n\r", th);
	    }
	  show_edit (th);
	  return;
	}
      if (!str_cmp (arg1, "light"))
	{
	  value = atoi (arg);
	  if (value >= 0 && value <= 100)
	    {
	      thg->light = value;
	      stt ("Light set.\n\r", th);
	    }
	  else
	    {
	      stt ("The Light must be between 0 and 100\n\r", th);
	    }
	  show_edit (th);
	  return;
	}
      if (!str_cmp (arg1, "long"))
	{
	  char *t;
	  free_str (thg->long_desc);
	  thg->long_desc = new_str (add_color(arg));
	  /* Capitalize it */
	  t = thg->long_desc;
	  if (*t != '\x1b')
	    *t = UC(*t);
	  else
	    {
	      while (*t && *t != 'm')
		t++;
	      if (*t == 'm')
		*(t + 1) = UC(*(t + 1));
	    }
	  stt ("Long Desc changed.\n\r", th);
	  show_edit (th);
	  return;
	}
      break;
    case 'M':
      
      if (!str_cmp (arg1, "miw") || !str_cmp (arg1, "max_in_world") ||
	  !str_cmp (arg1, "max"))
	{
	  value = atoi (arg);
	  if (IS_AREA (thg) || IS_AREA (thg))
	    {
	      stt ("Areas and rooms set in areas cannot have a max in world different from 1. Sorry. Resetting miw to 1.\n\r", th);
	      thg->max_mv = 1;
	    }
	  else if (value >= 0 && value <= 5000)
	    {
	      thg->max_mv = value;
	      stt ("Max in world set. 0 = unlimited.\n\r", th);
	    }
	  else
	    {
	      stt ("The max in world must be between 0 and 5000\n\r", th);
	    }
	  show_edit (th);
	  return;
	}
      break; 
    case 'N':
      if (!str_cmp (arg1, "name"))
	{
          THING *ar2;
          char *t;
	  if (IS_AREA (thg))
	    {
              for (ar2 = the_world->cont; ar2 != NULL; ar2 =ar2->next_cont)
                {
                  if (ar2 != thg &&
		      !str_cmp (ar2->name, arg))
                    {
                      stt ("Another area is already using that name!\n\r",th);
                      return;
                    }
                }
              for (t = arg; *t; t++)
                if (!isalnum(*t) && *t != '.')
                  {
                    stt ("Areas name cans only have letters, digits and the . for .are\n\r", th);
                    return;
                  }
	      if (str_suffix (".are", arg))
		{
		  stt ("Area names must end in '.are'.\n\r", th);
		  return;
		}
	    }  
	  if (IS_PC (thg) && !is_valid_name (thg->fd, arg))
	    {
	      stt ("That's not a valid name.\n\r", th);
	      return;
	    }
	  if (IS_PC (thg))
	    {
	      PBASE *pb;
	      for (pb = pbase_list; pb; pb = pb->next)
		{
		  if (!str_cmp (pb->name, thg->name))
		    {
		      free_str (pb->name);
		      pb->name = new_str (arg);
		    }
		}
	      free_str (thg->short_desc);
	      thg->name = new_str (capitalize (arg));
	    }
	  free_str (thg->name);
	  thg->name = new_str (arg);
	  
	  stt ("Name changed.\n\r", th);
	  show_edit (th);
	  return;
	}
      if (IS_AREA (thg))
	{
	  if (!str_cmp (arg1, "num_rooms") ||
	      !str_cmp (arg1, "num_things"))
	    {
	      value = atoi(arg);
	      
	      if (value < 5 || value > 20000)
		{
		  stt ("Areas must have between 5 and 20000 things and rooms contained in them.\n\r", th);
		  return;
		}
	      if (!check_area_vnums (thg, thg->vnum, thg->vnum + value))
		{
		  stt ("You can't do that. The vnums would end up overlapping another area.\n\r", th);
		  return;
		}
	      
	      if (!str_cmp (arg1, "num_rooms"))
		{
		  if (value >= thg->max_mv)
		    {
		      stt ("You can't have more rooms assigned than you have things assigned for the whole area.\n\r", th);
		      return;
		    }
		  else
		    {
		      stt ("Ok, number of rooms set for this area.\n\r", th);
		      thg->mv = value;
		    }
		}
	      else
		{
		  if (value <= thg->mv)
		    {
		      stt ("You can't have more rooms than things assigned to this area.\n\r", th); 
		      return;
		    }
		  else
		    {
		      stt ("Ok, number of things set for this area.\n\r", th);
		      thg->max_mv = value;
		    }
		}
	      show_edit (th);
	      return;
	    }
	}
      break;
    
    case 'R':
      if (!str_cmp (arg1, "reset"))
	{
	  int count = 0, reset_num = 0, thing_num = 0, pct = 0, nest = 0, max_num = 0;
	  RESET *reset, *resetn, *newreset;
	  char *oldarg = arg;
	  
	  arg = f_word (arg, arg1);
	  
	  if (!str_cmp (arg1, "loc") || !str_cmp (arg1, "location") ||
	      !str_cmp (arg1, "find") || !str_cmp (arg1, "where") ||
	      !str_cmp (arg1, "place"))
	    {
	      do_reset (th, oldarg);
	      return;
	    }

	  if (!is_number (arg1) || !isdigit (*arg1))
	    {
	      stt ("Syntax: reset <number> <delete> or <thing_num pct max_num nest>\n\r", th);
	      return;
	    }
	  else
	    reset_num = atoi (arg1);
	  arg = f_word (arg, arg1);
	  if (!str_cmp (arg1, "delete"))
	    {
	      if (reset_num <= 1)
		{
		  if (thg->resets)
		    {
		      reset = thg->resets;
		      thg->resets = thg->resets->next;
		      free_reset (reset);
		      show_edit (th);
		    }
		  else
		    stt ("That reset doesn't exist!\n\r", th);
		  return;
		}
	      for (reset = thg->resets; reset; reset = resetn)
		{
		  resetn = reset->next;
		  if (++ count == reset_num - 1)
		    {
		      if (reset->next)
			{
			  reset->next = reset->next->next;
			  free_reset (resetn);
			  show_edit (th);
			  return;
			}
		      else
			{
			  stt ("That reset doesn't exist!\n\r", th);
			  return;
			}
		    }
		}
	      stt ("That reset doesn't exist!\n\r", th);
	      return;
	    }
	  if (!isdigit (*arg1))
	    {
	      stt ("Syntax: reset <number> <delete> or <thing_num pct max_num nest>\n\r", th);
	      return;
	    }
	  thing_num = atoi (arg1);
	  arg = f_word (arg, arg1);
	  
	  if (isdigit (*arg1)) /* Pct is 100 if we dont' specify. */
	    pct = atoi (arg1);
	  else
	    pct = 100;
	  arg = f_word (arg, arg1);
	  
	  if (isdigit (*arg1)) /* Max_num is 1 if we don't specify. */
	    max_num = atoi (arg1);
	  else
	    max_num = 1;
	  nest = atoi (arg);
	  if (nest <= 0)
	    {
	      stt ("Reset nest must be at least 1.\n\r", th);	      
	      nest = 1;
	    }
	  if (nest > 5)
	    nest = 5;
	  newreset = new_reset ();
	  newreset->pct = pct;
	  newreset->times = max_num;
	  newreset->nest = nest;
	  newreset->vnum = thing_num;
	  if (reset_num <= 1 || !thg->resets)
	    {
	      newreset->next = thg->resets;
	      thg->resets = newreset;
	      stt ("Ok reset added.\n\r", th);
	      show_edit (th);
	      return;
	    }
	  count = 0;	  
	  for (reset = thg->resets; reset; reset = resetn)
	    {
	      resetn = reset->next;
	      /* Add the reset to the proper spot OR the end of the list. */
	      if (++count == reset_num - 1 || !resetn)
		{
		  newreset->next = reset->next;
		  reset->next = newreset;
		  stt ("Ok, reset added.\n\r", th);
		  show_edit (th);
		  return;
		}
	    }
	}
      if (!str_cmp (arg1, "roomgen"))
	{
	  roomgen (th, thg, arg);
	  show_edit (th);
	  return;
	}
	
      break;
    case 'S':
      if (!str_cmp (arg1, "size"))
	{
	  value = atoi (arg);
	  if (value >= 0 && value <= 100000)
	    {
	      thg->size = value;
	      stt ("Interior Size set.\n\r", th);
	    }
	  else
	    {
 	      stt ("The value must be between 0 and 100000... 0 means unlimited size.\n\r", th);
	    }
	  show_edit (th);
	  return;
	}
      if (!str_cmp (arg1, "sex"))
	{
	  value = SEX_MAX;
	  if (isdigit (*arg))
	    value = atoi(arg);
	  else
	    for (i = 0; i < SEX_MAX; i++)
	      {
		if (!str_prefix (arg, sex_names[i]))
		  {
		    value = i;
		    break;
		  }
	      }
	  if (value >= 0 && value < SEX_MAX)
	    {
	      thg->sex = value;
	      stt ("Sex set.\n\r", th);
	    }
	  else
	    {
	      stt ("The sex must be between 0 and 2, or choose neuter, male or female.\n\r", th);
	    }
	  show_edit (th);
	  return;
	}
      if (!str_cmp (arg1, "setup"))
	{
	  if (IS_SET (thg->thing_flags, TH_IS_ROOM | TH_IS_AREA))
	    {
	      stt ("Rooms and areas cannot be setup to be mobies or objects.\n\r", th);
	      return;
	    }
	  if (!str_cmp (arg, "obj") || !str_cmp (arg, "object"))
	    {
	      thg->thing_flags = OBJ_SETUP_FLAGS;
	     
	    }
	  else if (!str_cmp (arg, "mob") || !str_cmp (arg, "mobile"))
	    {
	      thg->thing_flags = MOB_SETUP_FLAGS;
	    }
	  else
	    {
	      stt ("setup obj or setup mob.\n\r", th);
	      return;
	    }
	  show_edit (th);
	  return;
	} 
      if (!str_cmp (arg1, "short"))
	{
	  if (IS_PC (thg) && !is_valid_name (thg->fd, arg))
	    {
	      stt ("That's not a valid name.\n\r", th);
	      return;
	    }
	  if (IS_PC (thg))
	    {
	      PBASE *pb;
	      for (pb = pbase_list; pb; pb = pb->next)
		{
		  if (!str_cmp (pb->name, thg->name))
		    {
		      free_str (pb->name);
		      pb->name = new_str (arg);
		    }
		}
	      free_str (thg->name);
	      thg->name = new_str (arg);
	    }
	  free_str (thg->short_desc);
	  thg->short_desc = new_str (add_color(arg));
	  stt ("Short desc changed.\n\r", th);
	  show_edit (th);
	  return;
	}
      if (!str_prefix ("sym", arg1))
	{
	  if (*arg > ' ' && *arg != '&')
	    thg->symbol = *arg;
	  else if (*arg == '\0')
	    thg->symbol = '\0';
	  else 
	    {
	      stt ("Symbol <symbol> or <blank> to clear it.\n\r", thg);
	      return;
	    }
	  stt ("Symbol changed.\n\r", th);
	  return;
	}
      break;
    case 'T':
      if (!str_cmp (arg1, "type") || !str_cmp (arg1, "typename"))
	{
	  free_str (thg->type);
	  thg->type = new_str (arg);
	  stt ("Thing type name changed.\n\r", th); 
	  show_edit (th);
	  return;	  
	}
      if (!str_prefix (arg1, "thingflags"))
	{
	  
	  if ((value = find_bit_from_name (FLAG_THING, arg)) != 0)
	    {
	      if (value == TH_IS_AREA || value == TH_IS_ROOM)
		{
		  stt ("You cannot change the room and area flags by hand.\n\r", th);
		  return;
		}/*
	      if (value == TH_NUKE && IS_AREA (thg))
		{
		  stt ("You cannot nuke an area!!! Remove it by hand from areas.dat!!! This is for your own protection!\n\r", th);
		  return;
		  }*/
	      sprintf (buf, "Thing flag %s turned %s.\n\r", arg, 
		       IS_SET (thg->thing_flags, value) ? "off" : "on");
	      thg->thing_flags ^= value;
	      stt (buf, th);
	    }
	  else
	    {
	      stt ("Syntax: thing or thingflags <name of flag>\n\r", th);
	    }
	  show_edit (th);
	  return;
	}
      if (!str_cmp (arg1, "time") || !str_cmp (arg1, "timer"))
	{
	  value = atoi (arg);
	  if (value >= 0 && value <= 2000)
	    {
	      thg->timer = value;
	      if (value == 0)
		stt ("This thing is untimed now.\n\r", th);
	      else
		stt ("Timer set.\n\r", th);
	    }
	  else
	    {
 	      stt ("The value must be between 0 and 2000, 0 means unlimited time.\n\r", th);
	    }
	  show_edit (th);
	  return;
	}
      break;
    case 'V':
      if (!str_prefix (arg1, "value"))
	{
	  VALUE *currval = NULL, *prev;
	  valtype = 0;
	  valnum = 0;
	  
	  /* Type */
	  arg = f_word (arg, arg1);
	  /* Which thing */
	  arg = f_word (arg, arg2);
	  
	  /* Check if we are asking for a proper type */
	  valtype = find_value_from_name (arg1);
	  
	  if (valtype == VAL_MAX)
	    {
	      stt ("Syntax: val <valuetype> (v<num> or word) <number or string>..\n\r", th);
	      return;
	    }
	  /* Find if a value of that type exists */
	  for (prev = thg->values; prev; prev = prev->next)
	    {
	      if (prev->type == valtype)
		{
		  currval = prev;
		  break;
		}
	    }
	  
	  /* If we are going to delete it, do it. */
	  if (!str_cmp (arg2, "delete"))
	    {
	      if (currval)
		{
		  stt ("Value removed\n\r", th);
		  remove_value (thg, currval);
		}
	      else
		stt ("This doesn't have any values of that type.\n\r", th);
	      show_edit (th);
	      return;
	    }
	  /* Otherwise see if we are changing a value */
	  t = arg2;
	  if (UC(*t) == 'V')
	    {
	      while (*t && !isdigit (*t))
		t++;
	      if (isdigit (*t))
		{
		  valnum = (*t - '0');
		  if (valnum < 1 || valnum >= NUM_VALS)
		    valnum = 0;
		}	      
	    }
	  /* or a word */
	  else
	    {
	      if (!str_cmp (arg2, "word") || !str_cmp (arg2, "name"))
		valnum = NUM_VALS;
	    }
	  /* If we aren't doing any of that, then return */

	  if (valnum == -1 || valtype == -1)
	    {
	      stt ("Syntax: val <valuetype> (v<num> or word) <number or string>..like\n\r 'value nexit v0 4532'\n\r", th);
	      return;
	    }
	  
	  /* Make a new value if needed. */
	  
	  if (!currval)
	    {
	      currval = new_value ();
	      currval->type = valtype;
	      add_value (thg, currval);
	    }
	  /* Now change the value numbers if we are doing that */
	  if (valnum < NUM_VALS && is_number (arg))
	    currval->val[valnum] = atoi (arg);
	  /* Or add the new word */
	  else
	    {
	      set_value_word (currval, arg);
	    }
	  show_edit (th);
	  return;
	}
      break;
    case 'W':
      if (!str_cmp (arg1, "wear"))
	{
          if (IS_ROOM (thg))
            {
              thg->wear_pos = ITEM_WEAR_NONE;
              stt ("Rooms cannot be worn.\n\r", th);
              show_edit (th);
              return;
            }
	  for (i = 0; wear_data[i].flagval < ITEM_WEAR_MAX; i++)
	    {
	      if (!str_cmp (arg, wear_data[i].name))
		{
		  thg->wear_pos = wear_data[i].flagval;
		  stt ("Wear location set.\n\r", th);
		  show_edit (th);
		  return;
		}
	    }
	  stt ("Wear <wear location name>.\n\r", th);
	  return;
	}
      if (!str_cmp (arg1, "weight"))
	{
	  value = atoi (arg);
	  if (value >= 0 && value <= 100000)
	    {
	      thg->weight = value;
	      stt ("Weight set.\n\r", th);
	    }
	  else
	    {
	      stt ("The value must be between 0 and 100000\n\r", th);
	    }
	  show_edit (th);
	  return;
	}
       break;
      
    }
  
  interpret (th, oldarg);
  return;
}

/* This finds what area a vnum is in! */

THING *
find_area_in (int vnum)
{
  THING *ar;
  for (ar = the_world->cont; ar != NULL; ar = ar->next_cont)
    {
      if (IS_PC (ar))
	continue;
      if (vnum > ar->vnum && vnum < ar->vnum + ar->max_mv)
	return ar;
    }
  return NULL;
}



void
do_reset (THING *th , char *arg)
{
  THING *thg, *proto;
  int i, vnum, num = 1;
  char *oldarg = arg;
  char buf[STD_LEN];
  char arg1[STD_LEN];
  RESET *res;
  VALUE *randpop;
  if (arg[0] == '\0')
    {
      stt (show_resets (th->in->resets), th);
      return;
    }
  arg = f_word (arg, arg1);
  if (!str_cmp (arg1, "loc") || !str_cmp (arg1, "location") ||
      !str_cmp (arg1, "find") || !str_cmp (arg1, "where") ||
      !str_cmp (arg1, "place") || atoi(arg1) > 0)
    {
      if (atoi(arg1) > 0)
	vnum = atoi(arg1);
      else
	vnum = atoi (arg);
      if ((proto = find_thing_num (vnum)) == NULL)
        {
          stt ("Item not found - reset loc <vnum>\n\r", th);
          return;
        }
      for (i = 0; i < HASH_SIZE; i++)
        {
          for (thg = thing_hash[i]; thg; thg = thg->next)
            {
              if (!thg->in || !IS_AREA (thg->in))   
                break;
               for (res = thg->resets; res; res = res->next)
                 {
                   if (res->vnum != vnum) 
                     continue;
		   sprintf (buf, "Reset \x1b[1;34m%d\x1b[0;37m: (\x1b[1;36m%d\x1b[0;37m - %s (V:%d)) (\x1b[1;37m%d\x1b[0;37m pct) (\x1b[1;32m%d\x1b[0;37m max)\n\r",
			    num++, res->vnum, (thg ? NAME(thg) : "nothing"),
			    (thg ? thg->vnum : 0),
			    res->pct, res->times);
                   stt (buf, th);
                 }
	       if ((randpop = FNV (thg, VAL_RANDPOP)) != NULL &&
		   vnum >= randpop->val[0] &&
		   (vnum <= (randpop->val[0] + (randpop->val[1]*randpop->val[2]))))
		 {
		   sprintf (buf, "Reset \x1b[1;34m%d\x1b[0;37m: (\x1b[1;36m%d\x1b[0;37m - %s (V:%d)) (RANDPOP ITEM)\n\r",
			    num++, vnum, (thg ? NAME(thg) : "nothing"),
			    (thg? thg->vnum : 0));
		   stt (buf, th);
		 }
	    }
	}      
      return;
    } 
  arg = oldarg;
  if (!str_cmp (arg1, "room") || !str_cmp (arg1, "here"))
    {
      if (!IS_ROOM (th->in) && th->in->in && !IS_AREA (th->in->in))
	{
	  stt ("You can't reset right here.\n\r", th);
	  return;
	}
      if (th->in && IS_AREA (th->in) && IS_AREA_SET (th->in, AREA_NOREPOP))
	{
	  stt ("This room is in an area that can't be reset.\n\r", th);
	  return;
	}

      reset_thing (th->in, 0);
    }
  else if (!str_cmp (arg1, "area"))
    {
      thg = th->in;
      while (thg && !IS_AREA (thg))
	thg = thg->in;
      stt ("Ok, resetting this area.\n\r", th);
      if (thg && IS_AREA (thg))
	reset_thing (thg, 0);
    }
  else if (!str_cmp (arg1, "world"))
    {
      reset_world ();
    }
  return;
}



/* This resets a single thing with all stuff inside of it. 
 The way it works is the following. We go down a list of "Resets"
 inside the thing, and first we check if the thing referenced by
 the reset exists. Then, we make sure we are not trying to copy a
 room or area. Then, we check how many of those things with that
 number are already in th. Then, if this number is too big (>= 
 reset->times, we goto the next reset. If the number is smaller,
 we then attempt to reset things a number of times equal to the
 difference between how many things can be there, and how many
 are there. Each new reset chance gets a new percentage roll
 to see if it works or not. This should save on the number of
 resets enormously. This also performs "area resets" if the
 thing is reset into something which is an area, it checks for
 a random room within the area and resets the thing into that
 room instead. */


void
reset_thing (THING *th, int rdepth)
{
  int i, curr_num_here, times_through, j, rsvnum, th_room_flags;
  int min_vnum, max_vnum;
  RESET *reset;
  THING *newthing, *basething, *thg, *basethisthing, *room, *area_to, *stthing;
  bool repeated_vnum = FALSE;
  VALUE *exit, *rexit, *rnd;


  /* Check for proper depth and make sure it's a room if its a proto. */

  if (rdepth >= MAX_NEST - 1 || !th || 
      (th->in && IS_AREA (th->in) && !IS_ROOM (th) && !IS_AREA (th)) ||
      (th->vnum >= GENERATOR_NOCREATE_VNUM_MIN &&
       th->vnum < GENERATOR_NOCREATE_VNUM_MAX)
#ifdef USE_WILDERNESS      
       || th == wilderness_area
#endif
      
      )
    return;
  
  if (IS_AREA (th))
    {
      for (thg = th->cont; thg; thg = thg->next_cont)
	reset_thing (thg, 0);
    }
  
  /* Reset exits in rooms */

  for (exit = th->values; exit != NULL; exit = exit->next)
    {
      if (exit->type >0 && exit->type <= DIR_MAX)
	{
	  exit->val[1] = exit->val[2];

	  /* Exit strength for breaking works like this. If the door is
	     set to nobreak, or if there's no door, the curr/max str
	     are cleared. Otherwise, if there's no max str, it's set
	     to 50, and in either case the curr then gets set to the max. */
	  
	  
	  if (IS_SET (exit->val[2], EX_NOBREAK) ||
	      !IS_SET (exit->val[2], EX_DOOR))
	    exit->val[4] = 0;
	  else if (exit->val[4] < 1)
	    exit->val[4] = 50;
	  exit->val[3] = exit->val[4];
	  
	  if ((room = find_thing_num (exit->val[0])) != NULL &&
	      (rexit = FNV (room, RDIR (exit->type - 1) + 1)) != NULL)
	    {
	      rexit->val[1] = rexit->val[2];
	      if (IS_SET (rexit->val[2], EX_NOBREAK) ||
		  !IS_SET (rexit->val[2], EX_DOOR))
		rexit->val[4] = 0;
	      else if (rexit->val[4] < 1)
		rexit->val[4] = 50;
	      rexit->val[3] = rexit->val[4];
	    }
	}
    }

  /* Set up the nesting ladder. */
  
  for (i = rdepth + 1; i < MAX_NEST; i++)
    thing_nest[i] = NULL;
  thing_nest[rdepth] = th;
  
  /* Find the base thing with the resets. */
  
  if (IS_SET (th->thing_flags, TH_IS_AREA | TH_IS_ROOM))
    basethisthing = th;
  else
    basethisthing = find_thing_num (th->vnum);
  if (!basethisthing)
    return;
  
  /* Now go down the list of resets in the base thing. */

  for (reset = basethisthing->resets; reset; reset = reset->next)
    {  
      /* Check if the thing we want to reset exists. */
      
      if ((stthing = find_thing_num (reset->vnum)) == NULL ||
          IS_SET (stthing->thing_flags, TH_IS_ROOM | TH_IS_AREA) || 
	  reset->nest < 1)
        continue;
      
      /* We first find the number of times we will try to reset this
	 thing. If the thing is an area, we check for how many instances
	 of the thing are in the area. If it's anything else, we check
	 how many instances are in that thing already.
	 
	 If we have a randpop, we check how many of ANY of the randpop
	 things exist in the area/other thing already, and then only
	 pop the remaining amt. */
      
      rnd = FNV (stthing, VAL_RANDPOP); /* Check for random pops */  
      /* Find the lower and upper limits on vnums to be popped. */
      
      if (!rnd) /* Only check one vnum's worth of things. */
	min_vnum = max_vnum = reset->vnum; 
      else
	{
	  min_vnum = rnd->val[0];
	  max_vnum = rnd->val[0] + MAX(0, rnd->val[1]) * MAX(1, rnd->val[2]);
	}
      
      /* Now find the current amount here. For areas check how many things
	 of the correct vnums are in the area someplace. */
      
      curr_num_here = 0;
      
      

      if (IS_AREA (th))
	{
	  /* If the reset is in the start area, just check all things
	     that exist in the world in these vnums. */
	  if (th->vnum == START_AREA_VNUM)
	    {
	      for (i = min_vnum; i <= max_vnum; i++)
		{
		  if ((thg = find_thing_num(i)) != NULL &&
		      (CAN_MOVE (thg) || CAN_FIGHT (thg)))
		    curr_num_here += thg->mv;
		}
	    }
	  else /* Otherwise check things that are in this area only. */
	    {
	      for (i = min_vnum; i <= max_vnum; i++)
		{
		  for (thg = thing_hash[i % HASH_SIZE]; thg; thg = thg->next)
		    {
		      if (thg->vnum == i &&
			  thg->in &&
			  thg->in->in &&
			  IS_ROOM (thg->in) &&
			  thg->in->in == th)
			curr_num_here++;
		    }
		}
	    }
	}
      else /* Check how many are in this thing atm... */
	{  
	  for (thg = th->cont; thg; thg = thg->next_cont)
	    if (thg->vnum >= min_vnum && thg->vnum <= max_vnum)
	      curr_num_here++;
	}

      /* The number of times the reset is attempted is the total number
	 allowed minus the number already here. */
      
      times_through = MAX (0, reset->times - curr_num_here);
      for (j = 0; j < times_through; j++)
	{    
	  if (np () > reset->pct)
	    continue;	
	   
          if (rnd) /* Random pops */
            rsvnum = calculate_randpop_vnum (rnd, LEVEL (th));
          else
  	    rsvnum = reset->vnum;
	   
	  /* This doesn't quite work. This is the code that makes
	     things limited. It doesn't work since players may have
	     objects (or pets) that should really be limited, but 
	     since I don't feel like doing the bookkeeping for keeping
	     items really limited, they're only limited in the game..
	     so if a player has a limited item and isn't logged in,
	     another one may load. It isn't perfect, but I can't
	     think of how to do this without using a lot of extra
	     memory and CPU which I don't think is really worth it. */
          if ((basething = find_thing_num (rsvnum)) == NULL ||
	      IS_SET (basething->thing_flags, TH_IS_ROOM | TH_IS_AREA) ||
	      (basething->max_mv > 0 && 
	       basething->mv >= basething->max_mv))
	    continue;
	   
	  /* Now see if we have repeated nesting. */
	  
	  for (i = 0; i <= rdepth; i++)
	    {	  
	      repeated_vnum = FALSE;
	      if (thing_nest[i] &&
		  thing_nest[i]->vnum == basething->vnum)
		{
		  char errbuf[STD_LEN];
		  repeated_vnum = TRUE;
		  sprintf (errbuf, "Stupid builders!!! Repeating vnum %d in reset at different depths!!! %d %d\n\r",  thing_nest[i]->vnum, rdepth, i);
		  log_it (errbuf);
		  
		  break;
		}
	    }

	  if (repeated_vnum)
	    {
	      char errbuf[STD_LEN];
	      sprintf (errbuf, "Thing numbers: ");
	      for (i = 0; i <= rdepth; i++)
		{
		  if (thing_nest[i])
		    sprintf (errbuf + strlen(errbuf), " %d", thing_nest[i]->vnum);
		  else
		    strcat (errbuf, " 0");
		}
	      strcat (errbuf, "\n");
	      log_it (errbuf);
	      continue;
	    }
	  th_room_flags = flagbits (basething->flags, FLAG_ROOM1) &
	    ROOM_SECTOR_FLAGS;
	 
	  if ((newthing = create_thing (rsvnum)) == NULL)	    
	    continue;
	  
	  /* Now try to put the new thing into something. */
	  
	  if (rdepth + reset->nest < MAX_NEST)
	    thing_nest[rdepth + reset->nest] = newthing;
	  else
	    {
	      free_thing (newthing);
	      continue;
	    }
	  for (i = MIN(MAX_NEST - 1, rdepth + reset->nest - 1); i >= 0; i--)
	    {
	      if (thing_nest[i])
		{
		  /* If the next thing up from this level is not an area,
		     send the new thing to it. */
		  
		  if (!IS_AREA (thing_nest[i]))
		    {
		      thing_to (newthing, thing_nest[i]);
		      break;
		    }
		  /* Now send resets to areas. If the area is the area
		     starting with vnum 1, we assume it's a global reset. */
		  
		  area_to = thing_nest[i];
		  if (area_to->vnum == START_AREA_VNUM)
		    area_to = find_random_area (AREA_NOREPOP);
		  
		  if (!area_to)
		    break;
		  if ((room = find_random_room 
		       (area_to, FALSE, th_room_flags, 
			(BADROOM_BITS & ~th_room_flags))) != NULL)
		    {
		      
		      thing_to (newthing, room);
		    }
		  else
		    
		  break;
		}
	      /* Added to stop quest items from popping on the ground. 
		 Bleh. Bad hack. */
	      else 
		break;
	    }	  
	  if (!newthing->in)
	    {
	      free_thing (newthing);
	      continue;
	    }
	  rdepth += MAX(1, reset->nest);     
	  reset_thing (newthing, rdepth);
	  rdepth -= MAX (1, reset->nest);	  
	}
    }
  
  if (!IS_SET (th->thing_flags, TH_NO_MOVE_OTHER | TH_IS_ROOM | TH_IS_AREA) &&
      FNV (th, VAL_SHOP) == NULL && th->in &&
      !IS_PC (th->in))
    do_wear (th, "all");

  if (IS_AREA(th))
    {
      for (thg = th->cont; thg; thg = thg->next_cont)
	{
	  if (IS_ROOM (thg))
	    reset_thing (thg, 0);
	}
    }
  return;
}

void
reset_world (void)
{
  log_it ("Resetting world\n");
  reset_thing (the_world, 0);
}



  

void 
do_rightarrow (THING *th, char *arg)
{
  SPELL *spl = NULL;
  THING *thg = NULL;
  RACE *race = NULL;
  SOCIETY *soc = NULL;
  int i;
  
  if (!th->fd || !IS_PC (th) || !th->pc->editing)
    {
      stt ("Huh? You must be editing a spell or thing to use this.\n\r", th);      
      return;
    }
  
  if (th->fd->connected == CON_EDITING &&
      (thg = (THING *) th->pc->editing) != NULL)
    {
      for (i = thg->vnum + 1; i <= top_vnum; i++)
	{
	  if ((thg = find_thing_num (i)) != NULL)
	    {
	      th->pc->editing = thg;
	      show_edit (th);
	      return;
	    }
	}
      stt ("This is the highest thing number.\n\r", th);
      return;
    }

  if (th->fd->connected == CON_RACEDITING && IS_PC (th)  &&
      (race = (RACE *) th->pc->editing) != NULL)
    {
      for (i = 0; i < RACE_MAX; i++)
	if (race == race_info[i])
	  break;
      
      if (i < RACE_MAX)
	{
	  if ((i == RACE_MAX -1) || !race_info[i + 1])
	    {
	      stt ("You are editing the last race.\n\r", th);
	      return;
	    }
	  else 
	    {
	      th->pc->editing = race_info[i + 1];
	      show_race (th, race_info[i + 1]);
	      return;
	    }
	}
      for (i = 0; i < ALIGN_MAX; i++)
	if (race == align_info[i])
	  break;
      
      if (i < ALIGN_MAX)
	{
	  if (i >= ALIGN_MAX - 1 || !align_info[i + 1])
	    {
	      stt ("You are editing the last alignment.\n\r", th);
	      return;
	    }
	  else 
	    {
	      th->pc->editing = align_info[i + 1];
	      th->fd->connected = CON_RACEDITING;
	      show_race (th, align_info[i + 1]);
	      return;
	    }
	}
      stt ("This race appears to be outside the proper lists. Odd.\n\r", th);
      return;
    }
  if (th->fd->connected == CON_SEDITING &&
      (spl = (SPELL *) th->pc->editing) != NULL)
    {
      for (i = spl->vnum + 1; i <= MIN (top_spell, MAX_SPELL); i++)
	{
	  if ((spl = find_spell (NULL, i)) != NULL)
	    {
	      th->pc->editing = spl;
	      show_spell (th, spl);
	      return;
	    }
	}
      stt ("This is the highest spell number.\n\r", th);
      return;
    }
  
  
   if (th->fd->connected == CON_SOCIEDITING &&
      (soc = (SOCIETY *) th->pc->editing) != NULL)
    {
      for (i = soc->vnum + 1; i <= top_society; i++)
	{
	  if ((soc = find_society_num (i)) != NULL)
	    {
	      th->pc->editing = soc;
	      show_society (th, soc);
	      return;
	    }
	}
      stt ("This is the last society already.\n\r", th);
      return;
    }
  
   
  stt ("You must be editing a spell or thing or a race/align or a society to use this.\n\r", th);
  return;
}


void 
do_leftarrow (THING *th, char *arg)
{
  SPELL *spl = NULL;
  THING *thg = NULL;
  RACE *race = NULL;
  SOCIETY *soc = NULL;
  int i;
  
  if (!th->fd || !IS_PC (th) || !th->pc->editing)
    {
      stt ("Huh? You must be editing a spell or thing to use this.\n\r", th);      
      return;
    }
  
  if (th->fd->connected == CON_EDITING &&
      (thg = (THING *) th->pc->editing) != NULL)
    {
      for (i = thg->vnum - 1; i > 0; i--)
	{
	  if ((thg = find_thing_num (i)) != NULL)
	    {
	      th->pc->editing = thg;
	      show_edit (th);
	      return;
	    }
	}
      stt ("This is the lowest thing number.\n\r", th);
      return;
    }
  
  
  if (th->fd->connected == CON_RACEDITING && 
      (race = (RACE *) th->pc->editing) != NULL)
    {
      for (i = 0; i < RACE_MAX; i++)
	if (race == race_info[i])
	  break;
      
      if (i != RACE_MAX)
	{
	  if (i == 0 || !race_info[i - 1])
	    {
	      stt ("You are editing the first race.\n\r", th);
	      return;
	    }
	  else 
	    {
	      th->pc->editing = race_info[i - 1];
	      show_race (th, race_info[i - 1]);
	      return;
	    }
	}
      
      for (i = 0; i < ALIGN_MAX; i++)
	if (race == align_info[i])
	  break;
      
      if (i != ALIGN_MAX)
	{
	  if (i == 0 || !align_info[i - 1])
	    {
	      stt ("You are editing the first alignment.\n\r", th);
	      return;
	    }
	  else 
	    {
	      th->pc->editing = align_info[i - 1];
	      show_align (th, align_info[i - 1]);
	      return;
	    }
	}
      stt ("This race appears to be outside the proper lists. Odd.\n\r", th);
      return;
    }

  if (th->fd->connected == CON_SEDITING && 
      (spl = (SPELL *) th->pc->editing) != NULL)
    {
      for (i = spl->vnum - 1; i > 0; i--)
	{
	  if ((spl = find_spell (NULL, i)) != NULL)
	    {
	      th->pc->editing = spl;
	      show_spell (th, spl);
	      return;
	    }
	}
      stt ("This is the lowest spell number.\n\r", th);
      return;
    }
  
  if (th->fd->connected == CON_SOCIEDITING &&
      (soc = (SOCIETY *) th->pc->editing) != NULL)
    {
      for (i = soc->vnum - 1; i > 0; i--)
	{
	  if ((soc = find_society_num (i)) != NULL)
	    {
	      th->pc->editing = soc;
	      show_society (th, soc);
	      return;
	    }
	}
      stt ("This is the first society already.\n\r", th);
      return;
    }
  
  stt ("You must be editing a spell or thing or a race/align or a society to use this.\n\r", th);
  return;
}

/* This performs an edit command at many different places at once. 
   It is not "medit" so it does not interfere with meditate. */


void
do_zedit (THING *th, char *arg)
{
  int lvnum, uvnum;
  THING *lar, *uar;
  THING *thg, *oldedit;
  char arg1[STD_LEN];
  int i;
  if (LEVEL (th) < MAX_LEVEL)
    {
      stt ("Huh?\n\r", th);
      return;
    }
  if (!IS_PC (th) || !th->fd || IS_SET (th->fd->flags, FD_EDIT_PC))
    {
      stt ("You can't do this now.\n\r", th);
      return;
    }

  arg = f_word (arg, arg1);
  lvnum = atoi (arg1);
  arg = f_word (arg, arg1);
  uvnum = atoi (arg1);
  
  
  if (lvnum > uvnum ||
#ifdef USE_WILDERNESS
      lvnum > WILDERNESS_AREA_VNUM ||
      uvnum > WILDERNESS_AREA_VNUM ||
#endif
      uvnum - lvnum > 500 ||
      (lar = find_area_in (lvnum)) == NULL ||
      (uar = find_area_in (uvnum)) == NULL ||
      uar != lar)
    {
      stt ("Syntax: zedit <lvnum> <uvnum> command. The vnums cannot be more than 500 apart, and must both be in the same area.\n\r", th);
      return;
    }
  
  if (th->fd->connected == CON_EDITING)
    oldedit = (THING *) th->pc->editing;
  else
    oldedit = NULL;
  
  SBIT (server_flags, SERVER_SUPPRESS_EDIT);
  th->fd->connected = CON_EDITING;
  for (i = lvnum; i <= uvnum; i++)
    {
      if ((thg = find_thing_num (i)) == NULL)
	continue;
      if (IS_PC (th))
	{
	  th->pc->editing = thg;
	  edit (th, arg);	  
	}
    }
  RBIT (server_flags, SERVER_SUPPRESS_EDIT);
  if (oldedit && IS_PC (th))
    th->pc->editing = oldedit;
  return;
}

/* This shows a list of areas to the player, and if the player is an
   admin, it shows a few simple statistics. */

void
do_areas (THING *th, char *arg)
{
  THING *ar, *thg;
  char buf[STD_LEN];
  bool count_only = FALSE;
  int count = 0, room_count = 0, mob_count = 0, obj_count = 0, total_room_count = 0, total_mob_count = 0, total_obj_count = 0, total_area_count = 0;
  bigbuf[0] = '\0';
  
  if (LEVEL (th) >= BLD_LEVEL)
    {
      if (!str_cmp (arg, "count"))
	count_only = TRUE;
      else
	add_to_bigbuf ("Num      Vnums     Name                     Room Mob Obj File          Builder\n\r");
    }
  else
    add_to_bigbuf ("Num     Name                         Builders\n\r");
  for (ar = the_world->cont; ar != NULL; ar = ar->next_cont)
    {
      /* Builders get more info. */
      
      if (LEVEL (th) >= BLD_LEVEL)
	{
	  room_count = 0;
	  mob_count = 0;
	  obj_count = 0;
	  total_area_count++;
	  for (thg = ar->cont; thg; thg = thg->next_cont)
	    {
	      if (IS_ROOM (thg))
		room_count++;
	      else if (CAN_MOVE (thg) || CAN_FIGHT (thg))
		mob_count++;
	      else
		obj_count++;
	    }
	  if (!count_only)
	    sprintf (buf, "%3d: %6d-%-6d %-25s %3d %3d %3d %-13s %s\n\r",
		     ++count, ar->vnum, ar->vnum + ar->max_mv - 1,
		     ar->short_desc, room_count, mob_count, obj_count, 
		     ar->name, ar->type);
	  total_room_count += room_count;
	  total_mob_count += mob_count;
	  total_obj_count += obj_count;
	}
      
      /* Morts don't get as much info, and you can hide certain areas
	 from them using nolist area flag. */
      else if (!IS_AREA_SET (ar, AREA_NOLIST))
	{
	  sprintf (buf, "%3d: %-25s  %s\n\r", ++count, ar->short_desc, ar->type);
	}
      else
	continue;
      if (!count_only)
	add_to_bigbuf (buf);
    }
  if (LEVEL (th) >= BLD_LEVEL)
    {
      if (!count_only)
	add_to_bigbuf ("\n\n\r");
      sprintf (buf, "There are \x1b[1;37m%d\x1b[0;37m areas with \x1b[1;33m%d\x1b[0;37m rooms, \x1b[1;35m%d\x1b[0;37m mobiles, and \x1b[1;36m%d\x1b[0;37m objects.\n\r",
	       total_area_count, total_room_count, 
	       total_mob_count, total_obj_count);
      add_to_bigbuf (buf);
    }
  send_bigbuf (bigbuf, th);
  return;
}
  
/* Lets you find prototypes...will let you search for "obj", "mob", 
   "room", and also lets you search for things in a range, and also
   lets you search for a specific name. It also lets you search for
   items with certain "values" in them. */
     
void
do_tfind (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  char name[TFIND_NAME_MAX][STD_LEN];
  char padbuf[STD_LEN];
  char first_name[STD_LEN];
  bool found_name = TRUE;
  bool mob = FALSE, room = FALSE, obj = FALSE, found = FALSE, are = FALSE, nodesc = FALSE, noval = FALSE;
  int lvnum = -1, uvnum = -1, i, valnum = VAL_MAX, pad;
  THING *ar, *vict, *area = NULL;
  VALUE *val;
  for (i = 0; i < TFIND_NAME_MAX; i++)
    name[i][0] = '\0';
  
  if (arg[0] == '\0')
    {
      stt ("tfind <mob/obj/room> or <low_num> <up_num> or <value_name> or <name>\n\r", th);
      return;
    }
  /* Reads in arguments one at a time...lets you have numbers and "mob"
     etc, and it also finds values of specific types...if it is a random
     string, then the name string gets used. */
  
  while (arg && *arg)
    {
      arg = f_word (arg, arg1);
      if (isdigit (*arg1) && is_number (arg1))
	{
	  if (lvnum == -1)
	    lvnum = atoi (arg1);
	  else if (uvnum == 0)
	    uvnum = atoi (arg1);
	}
      else if (!str_cmp (arg1, "mob") || !str_cmp (arg1, "monsters") ||
	       !str_cmp (arg1, "creatures"))
	mob = TRUE;
      else if (!str_cmp (arg1, "obj") || !str_cmp (arg1, "object") ||
	       !str_cmp (arg1, "objects"))
	obj = TRUE;
      else if (!str_cmp (arg1, "room"))
	room = TRUE;
      else if (!str_cmp (arg1, "area"))
	are = TRUE;
      else if (!str_cmp (arg1, "nodesc"))
	nodesc = TRUE;
      else if (!str_cmp (arg1, "noval"))
	{
	  noval = TRUE;
	  valnum = VAL_MAX;
	}
      else 
	{
	  /* See if we search for things with a certain value like
	     tfind food tfind gem etc... */
	  
	  /* Only do this if we haven't found a value already. */
	  if (valnum == VAL_MAX && !noval)  
	    {
	      for (valnum = 1; valnum < VAL_MAX; valnum++)
		{
		  if (!str_cmp (arg1, value_list[valnum]))
		    break;
		}
	      /* If the value name isn't found, then assume it's a 
		 name we look for. */
	      if (valnum == VAL_MAX)
		{
		  for (i = 0; i < TFIND_NAME_MAX; i++)
		    {
		      if (!*name[i])
			{			
			  strcpy (name[i], arg1);
			  break;
			}
		    }
		}
	    }
	  else /* We have a value. Read in the name. */
	    {
	      for (i = 0; i < TFIND_NAME_MAX; i++)
		{
		  if (!*name[i])
		    {			
		      strcpy (name[i], arg1);
		      break;
		    }
		}
	    }
	}
    }
  if (lvnum > uvnum)
    {
      stt ("tfind <mob/obj/room> or <low_num> <up_num> or <value_name> or <names>\n\r", th);
      return;
    }
  if (are)
    {
      if (!th->in || !IS_SET (th->in->thing_flags, TH_IS_ROOM | TH_IS_AREA))
	{
	  stt ("You aren't in a room or area, so you cannot do tfind area.\n\r", th);
	  return;
	}
      if (IS_AREA (th->in))
	area = th->in;
      else 
	area = th->in->in;
    }
  bigbuf[0] = '\0';
  add_to_bigbuf ("Things of this type:\n\n\r");
  for (ar = the_world->cont; ar != NULL; ar = ar->next_cont)
    {
      if (area && ar != area)
	continue;
      for (vict = ar->cont; vict != NULL; vict = vict->next_cont)
	{/* Check for all things which don't fit the desired info. */
	  if ((room && !IS_ROOM (vict)) ||
	      (mob && (IS_ROOM (vict) || (!CAN_FIGHT (vict) && !CAN_MOVE (vict)))) ||     
	      (obj && (CAN_FIGHT (vict) || CAN_MOVE (vict))) ||
	      (nodesc && vict->desc && *vict->desc) ||
	      (lvnum > 0 && uvnum > 0 && 
	       (lvnum > vict->vnum || vict->vnum > uvnum)) ||
	      (valnum != VAL_MAX &&
	       ((val = FNV (vict, valnum)) == NULL ||
	       (valnum == VAL_WEAPON && 
		vict->wear_pos != ITEM_WEAR_WIELD))))
	    continue;
	  
	  found_name = TRUE;
	  
	  for (i = 0; i < TFIND_NAME_MAX; i++)
	    {
	      if (*name[i] && !is_named (vict, name[i]))
		{
		  found_name = FALSE;
		  break;
		}
	    }
	  if (!found_name)
	    continue;


	  found = TRUE;
	  pad = 50 - strlen_color (NAME (vict));
	  if (pad < 0)
	    pad = 0;
	  for (i =0; i < pad; i++)
	    padbuf[i] = ' ';
	  padbuf[pad] = '\0';
	  f_word (KEY(vict), first_name);
	  sprintf (buf, "[%5d] (Lev %2d) %s%s %s\n\r", vict->vnum, 
		   LEVEL(vict), NAME (vict), padbuf, first_name);
	  add_to_bigbuf (buf);
	}
    }
  if (!found)
    add_to_bigbuf ("Nothing.\n\r");
  send_bigbuf (bigbuf,th);
  return;
}



/* Twhere shows you the locations of all things named arg. */

void
do_twhere (THING *th, char *arg)
{
  
  THING *thg;
  int i, pad, j;
  char arg1[STD_LEN];
  char buf[STD_LEN];
  char padbuf[STD_LEN];
  bool found = FALSE;
  bool prisoner = FALSE;
  bool worldgen = FALSE;
  if (!*arg)
    {
      stt ("twhere [worldgen] <name> shows the locations of all things named this.\n\r", th);
      return;
    }
  arg = f_word (arg, arg1);
  if (!str_cmp (arg1, "prisoner") || 
      !str_cmp (arg1, "kidnapped"))
    prisoner = TRUE;
  if (!str_cmp (arg1, "worldgen"))
    {
      worldgen = TRUE;
      f_word (arg, arg1);
    }
  bigbuf[0] = '\0';
  sprintf (buf, "All locations of \"%s\":\n\n\r", arg);
  add_to_bigbuf (buf);
  for (i = 0; i < HASH_SIZE; i++)
    {
      for (thg = thing_hash[i]; thg != NULL; thg = thg->next)
	{
	  if ((thg->in && IS_AREA (thg->in) && !IS_ROOM (thg)) ||
	      IS_AREA (thg))
	    continue;
	  
	  /* Worldgen argument only finds worldgen objects. */
	  if (worldgen && 
	      (thg->vnum < WORLDGEN_START_VNUM || 
	       thg->vnum > WORLDGEN_END_VNUM) &&
	      thg->vnum < GENERATOR_NOCREATE_VNUM_MAX)
	    continue;
	  
	  if (prisoner) 
	    {
	      if (!IS_ACT1_SET (thg, ACT_PRISONER))
		continue;
	    }
	  else if (!is_named (thg, arg1))
	    continue;
	  
	  found = TRUE;
	  pad = 30 - strlen_color (NAME (thg));
	  if (pad < 0)
	    pad = 0;
	  for (j = 0; j < pad; j++)
	    padbuf[j] = ' ';
	  padbuf[pad] = '\0';
	  sprintf (buf, "[%5d] %s%s is in [%5d] %s\n\r", 
		   thg->vnum, NAME (thg), padbuf, 
		   (thg->in ? thg->in->vnum : 0),
		   (thg->in ? show_build_name(thg->in) : "nothing"));
	  add_to_bigbuf (buf);
	}
    }
  if (!found)
    add_to_bigbuf ("Nowhere.\n\r");
  send_bigbuf (bigbuf, th);
  return;
}


/* This command lets you set stats on a creature directly. It
   uses rep_one_value to do this. */

void
do_tset (THING *th, char *arg)
{
  THING *vict;
  char arg1[STD_LEN];
  
  arg = f_word (arg, arg1);
  if ((vict = find_thing_world (th, arg1)) == NULL)
    {
      stt ("That thing isn't here to set!\n\r", th);
      return;
    }
  arg = f_word (arg, arg1);
  if (!*arg)
    {
      stt ("tset <location> <value>\n\r", th);
      return;
    }
  replace_one_value (NULL, arg1, vict, TRUE, '=', arg);
  update_kde (vict, ~0);
  stt ("Ok, value set.\n\r", th);
  return;
}
  

void
do_tstat (THING *th, char *arg)
{
  THING *vict = NULL;
  void *old_edit;
  int old_conn;
  if (!th->fd || !IS_PC (th))
    return;
  
  if (is_number (arg) && (vict = find_thing_num (atoi (arg))) == NULL)
    {
      stt ("There are no things of that number!\n\r", th);
      return;
    }
  if (!vict && (vict = find_thing_world (th, arg)) == NULL)
    {
      stt ("That thing isn't around to stat.\n\r", th);
      return;
    }
  
  if (IS_PC (vict) && LEVEL (vict) > LEVEL (th))
    {
      stt ("Their level is too high for you to stat them!\n\r", th);
      return;
    }
  
  old_conn = th->fd->connected;
  old_edit = th->pc->editing;
  th->fd->connected = CON_EDITING;
  th->pc->editing = vict;
  SBIT (server_flags, SERVER_TSTAT);
  show_edit (th);
  RBIT (server_flags, SERVER_TSTAT);
  th->fd->connected = old_conn;
  th->pc->editing = old_edit;
  th->pc->last_tstat = vict->vnum;
  return;
}
  


/* This command lists all of the exits to/from the area you're
   currently in. */

void
do_exlist (THING *th, char *arg)
{
  THING *in_area, *ar, *exarea;
  THING *room, *exroom;
  VALUE *exit;
  char buf[STD_LEN];
  
  if (!th->in || !IS_ROOM (th->in) || !th->in->in ||
      (in_area = th->in->in) == NULL || !IS_AREA (in_area))
    {
      stt ("You must be in a room in an area to use this command.\n\r", th);
      return;
    }
  
  for (ar = the_world->cont; ar; ar = ar->next_cont)
    {
      if (!IS_AREA (ar)) 
	continue;
      
      for (room = ar->cont; room; room = room->next_cont)
	{
	  if (!IS_ROOM (room))
	    continue;
	  
	  for (exit = room->values; exit; exit = exit->next)
	    {
	      if (exit->type <= 0 || exit->type > REALDIR_MAX||
		  (exroom = find_thing_num (exit->val[0])) == NULL ||
		  !IS_ROOM (exroom) || (exarea = find_area_in (exit->val[0])) == NULL)
		continue;
	      
	      /* At this point, we have a room in an area and it has an
		 exit and the exit leads to another room and the
		 other room is in an area. Now, add the link if
		 one area XOR the other area is equal to the area we're
		 in currently. */

	      if ((exarea == in_area) != (ar == in_area))
		{
		  sprintf (buf, "An exit from room %d in %s to room %d in %s\n\r",
			   room->vnum, NAME(ar), exroom->vnum,
			   NAME(exarea));
		  stt (buf, th);
		}
	    }
	}
    }
  return;
}
		  


/* The at command lets you issue commands at several locations
   at once. */

void
do_at (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char arg2[STD_LEN];
  int min_vnum = 0;   /* Min/max vnums for several rooms. */
  int max_vnum = 0;   /* Min/max vnums for several rooms. */
  int i;
  THING *vict, *room, *in_now, *victn, *victn_in = NULL;
  if (!IS_PC (th) || (in_now = th->in) == NULL)
    return;
  
  if (!IS_ROOM (in_now))
    {
      stt ("You must be in a room to perform an at command.\n\r", th);
      return;
    }
  
  arg = f_word (arg, arg1);
  
  /* See if we have a range of numbers to work with. */
  if (is_number (arg1))
    {
      /* Get the first number. */
      min_vnum = atoi (arg1);
      arg = f_word (arg, arg1);
      /* Get the second number. */
      if (is_number (arg1))
	{
	  max_vnum = atoi (arg1);
	}

      /* If the first one was a number, and the second one not, the
	 second number becomes the first. */
      
      if (min_vnum > 0 && max_vnum <= min_vnum)
	max_vnum = min_vnum;

      if (max_vnum - min_vnum > 1000)
	{
	  stt ("You cannot perform at's over a range of numbers larger than 1000.\n\r", th);
	  return;
	}

      for (i = min_vnum; i <= max_vnum; i++)
	{
	  if ((room = find_thing_num (i)) != NULL &&
	      IS_ROOM (room))
	    {
	      thing_from(th);
	      thing_to (th, room);
	      interpret (th, arg);
	    }
	}
      thing_to (th, in_now);
      return;
    }
  
  if (!str_cmp (arg1, "all"))
    {
      arg = f_word (arg, arg1);
    
      for (vict = thing_hash[PLAYER_VNUM % HASH_SIZE]; vict; vict = victn)
	{
	  victn = vict->next;
	  if (victn)
	    victn_in = victn->in;
	  if ((room = vict->in) != NULL && IS_ROOM (room) &&
	      vict != th)
	    {
	      thing_to (th, room);
	      if (!str_cmp (arg, "@n"))
		sprintf (arg2, "%s %s", arg1, NAME (vict));
	      else
		sprintf (arg2, "%s %s", arg1, arg);
	      interpret (th, arg2);
	    }
	  /* Bail out on bad error. This is not "correct" code, but
	     it will keep infinite loops out (hopefully) */

	  if (victn && victn_in != victn->in)
	    break;
	}
      thing_to (th, in_now);
      return;
    }
  
  if ((vict = find_thing_world (th, arg1)) == NULL ||
      vict->in == NULL)
    {
      stt ("They aren't here!\n\r", th);
      return;
    }
  
  thing_to (th, vict->in);
  interpret (th, arg);
  thing_to (th, in_now);
  return;
}



/* This lets a thing edit flags on anything. It was combined because
   having duplicated code is bad. */

void
edit_flags (THING *th, FLAG**pflags, char *arg)
{
  char arg1[STD_LEN];
  char arg2[STD_LEN];
  char buf[STD_LEN];
  FLAG *currflag = NULL, *prev;
  FLAG_DATA *fld;
  int flagtype, value;

  
  if (!pflags || !arg || !*arg)
    return;
  
  /* Get the arguments. The first one is the flag type, the second is
     the flag name. */
  arg = f_word (arg, arg1);
  arg = f_word (arg, arg2);
  
  /* Find the type of flag to edit. */

  flagtype = find_flagtype_from_name(arg1);  
  if (flagtype == AFF_MAX)
    {
      stt ("Flag <flagtype> <word or number>\n\r", th);
      return;
    }
  
  /* See if you have a flag of this type already. */
  
  for (prev = *pflags; prev; prev = prev->next)
    {
      if (prev->type == flagtype)
	{
	  currflag = prev;
	  break;
	}
    }
  
  /* This is where you can delete the flag. */
  
  if (!str_cmp (arg2, "delete"))
    {
      if (currflag)
	{
	  /* If the flag to be deleted is first, move the pointer to the
	     next flag. */
	  
	  if (currflag == *pflags)
	    *pflags = (*pflags)->next;
	  else
	    /* Otherwise remove the flag from the list by finding
	       the previous element, then setting its next pointer
	       to the element after the thing to be removed. */
	    {
	      for (prev = *pflags; prev; prev = prev->next)
		{
		  if (prev->next == currflag)
		    prev->next = currflag->next;
		}
	    }
	  
	  currflag->next = NULL;
	  free_flag (currflag);
	  stt ("Ok, flag is deleted.\n\r", th);
	  return;
	}
      else
	{
	  stt ("This thing doesn't have any flags of that type.\n\r", th);
	  return;
	}
      return;
    }
  
  /* Now work on adding/removing flags from bitvectors. */

  if (flagtype < NUM_FLAGTYPES)
    {
      /* Search through the list of names. */
      for (fld = flaglist[flagtype].whichflag; fld->flagval != 0; fld++)
	{
	  if (!str_prefix (arg2, fld->name))
	    {
	      /* If no flag of this type exists, make one. */

	      if (!currflag)
		{
		  currflag = new_flag ();
		  currflag->type = flagtype;
		  currflag->next = *pflags;
		  *pflags = currflag;
		}

	      /* Toggle the bit and send the message. */
	      sprintf (buf, "%s flag %s bit toggled %s.\n\r", arg1, arg2, (IS_SET (currflag->val, fld->flagval) ? "Off" : "On"));
	      stt (buf, th);	      
	      currflag->val ^= fld->flagval;

	      /* Delete zeroed flags */
	      
	      if (currflag->val == 0)
		{
		  FLAG *delflag;
		  
		  /* If it's the start flag, delete it. */
		  if (currflag == *pflags)
		    {
		      *pflags = (*pflags)->next;
		      free_flag (currflag);
		    }
		  else /* Otherwise search the list until you find the
			  correct flag to delete. */
		    {
		      for (delflag = *pflags; delflag; delflag = delflag->next)
			{
			  if (delflag->next == currflag)
			    {
			      delflag->next = currflag->next;
			      free_flag (currflag);
			      break;
			    }
			}
		    }
		}
	      return;
	    }
	}
      sprintf (buf, "%s flags have no member named '%s'.\n\r", arg1, arg2);
      stt (buf, th);
      return;
    }

  /* At this point, we are not deleting a flag, and the flag to be
     edited is not a flag flag, it's an affect flag. */

  if (is_number (arg2))
    {
      value = atoi (arg2);
    }
  else 
    {
      stt ("Syntax flag <type> <number/name>.\n\r", th);
      return;
    }
  
  sprintf (buf, "Flag affect %s set to %d.\n\r", arg1, value);
  stt (buf, th);
  
  if (!currflag)
    {
      currflag = new_flag ();
      currflag->type = flagtype;
      currflag->next = *pflags;
      *pflags = currflag;
    }
  currflag->val = value;

  /* Delete the flag if the affect is zeroed out. */

  if (currflag->val == 0)
    {
      FLAG *delflag;
      
      /* If it's the start flag, delete it. */
      if (currflag == *pflags)
	{
	  *pflags = (*pflags)->next;
	  free_flag (currflag);
	}
      else /* Otherwise search the list until you find the
	      correct flag to delete. */
	{
	  for (delflag = *pflags; delflag; delflag = delflag->next)
	    {
	      if (delflag->next == currflag)
		{
		  delflag->next = currflag->next;
		  free_flag (currflag);
		  break;
		}
	    }
	}
    }
  return;
}

/* This calculates what vnum a "randpop" item came up with. It's used
   for resets and for wilderness things popping. */

int
calculate_randpop_vnum (VALUE *rnd, int level)
{
  int rcount, rank = 0;
  int rsvnum = 0;
  THING *newitem;
  VALUE *new_rnd;
  static int depth = 0;


  if (!rnd || (rnd->type != VAL_RANDPOP &&
	       rnd->type != VAL_REPLACEBY))
    return 0;

  
  for (rcount = 0; rcount < rnd->val[2] - 1; rcount++)
    {
      /* Minimum 10 percent chance to increase tier based on level. */
      if ((rnd->val[3] > 0 && 
	   level >= rnd->val[3]/10 &&
	   nr(1,rnd->val[3]) < MIN (level, rnd->val[3]*19/20)) ||
	  (rnd->val[4] > 0 &&
	   rnd->val[5] > 0 &&
	   nr(1,rnd->val[4]) <= rnd->val[5]))
	rank++;
    }
  
  /* Here is how these two random vnum things work. The
     "randpop" value has a val[0] which is the start number.
     There are rules which determine if we go up in "rank"
     for this particular repop. Then, each randpop has a
     certain number of tiers (v2) and a certain
     number of elements per tier (v1). If the pop is
     interlaced, that means the crappy items are v2 units
     apart (like gems) and so to do the randpop, we pick
     a random number from the number of elements per
     tier, and then multiply that value by the number of 
     tiers, then finally shift by the rank which gives us
     the power of the item. 
     
     If the repop is not interlaced, then we just have
     the v1 crappiest items first, so we pick a random
     number from 0 to v1, and then add the number of 
     elements in a tier for each rank we advance. This
     second type of reset is used for weapon and armor
     pops. 
     
     What this means is if you want to have a large
     number of randomly popping items, you can either
     set them up so all ranks of the same thing are
     next to each other, and so that all crap items come
     first, followed by slightly better items etc...
     and you can still repop them either as a randpop
     on ranks of a single kind of item, or as a whole group
     no matter which way you ended up doing this */
  
  if (rnd->val[6] != 0)
    rsvnum = rnd->val[0] +  (nr(0, rnd->val[1]-1) *
			     rnd->val[2]) + rank;
  else
    rsvnum = rnd->val[0] + nr (0, rnd->val[1] - 1) +
      rnd->val[1] * rank;
  
  /* Here's another level. If this new vnum exists and if it's also
     a randpop item, we go down another level and attempt to create
     THAT randpop item. Taken from AO idea of "categories" of items --
     slasher vs wpn vs item... */
  
  if (depth < 10 &&
      (newitem = find_thing_num (rsvnum)) != NULL &&
      ((new_rnd = FNV (newitem, VAL_RANDPOP)) != NULL ||
       (new_rnd = FNV (newitem, VAL_REPLACEBY)) != NULL))
    {
      depth++;
      rsvnum = calculate_randpop_vnum (new_rnd, level);
      depth--;
    }

  return rsvnum;
}
  

/* This lets you edit your own description. This is limited in stredit.c
   so asshole players can't make huge descs. */

void
do_description (THING *th, char *arg)
{
  if (!IS_PC (th))
    return;
  
  stt ("You may now edit your description.\n\r", th);
  new_stredit (th, &th->desc);
  return;
}
