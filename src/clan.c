#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"


/* My clan code is designed to allow you to make as many different
   kinds of "clans" as you want, and let everyone join one of each
   kind. If you want to change the numbers of types of clans, 
   just edit the MAX_CLAN in serv.h and then in const.c go edit the
   clantypes structure and change the names. The various functions
   in here aren't really documented too well, but the idea is there
   are some ways of finding a clan based on numbers or names and
   there are the regular read/write functions and the new/free functions
   and of course "show_clan" and "clan" which let you edit/look at
   clans. Note that it is important that you edit const.c the
   clantypes struct because this is checked in int.c to see if
   this clan stuff gets called. */





CLAN *
find_clan (int vnum, int type)
{
  CLAN *cln;
  if (type < 0 || type >= MAX_CLAN_MEMBERS || vnum < 1)
    return NULL;
  
  for (cln = clan_list[type]; cln; cln = cln->next)
    {
      if (cln->vnum == vnum)
	return cln;
    }
  return NULL;
}

/* This will find which clan someone is leading if the leader
   bit is set to TRUE, otherwise it just looks to see if the person
   is in the clan at all. */


CLAN *
find_clan_in (THING *th, int type, bool leader)
{
  CLAN *cln;
  int i;
  
  if (type < 0 || type >= MAX_CLAN_MEMBERS || !th || !IS_PC (th))
    return NULL;
  
  for (cln = clan_list[type]; cln; cln = cln->next)
    {
      if (!str_cmp (th->name, cln->leader))
	return cln;
      if (leader)
	continue;
      for (i = 0; i < MAX_CLAN_MEMBERS; i++)
	{
	  if (!str_cmp (th->name, cln->member[i]))
	    return cln;
	}
    }
  return NULL;
}
	


void
free_clan (CLAN *cln)
{
  int i;
  free_str (cln->name);
  free_str (cln->motto);
  free_str (cln->history);
  free_str (cln->leader);
  for (i = 0; i < MAX_CLAN_MEMBERS; i++)
    free_str (cln->member[i]);
  for (i = 0; i < MAX_CLAN_STORE; i++)
    {
      if (cln->storage[i])
        {
          cln->storage[i]->in = cln->storage[i];
          free_thing (cln->storage[i]);
        }
      cln->storage[i] = NULL;
    }
  cln->next = clan_free;
  clan_free = cln;
  
  return;
}

CLAN *
new_clan (void)
{
  CLAN *newclan;
  int i;
  if (clan_free)
    {
      newclan = clan_free;
      clan_free = clan_free->next;
    }
  else
    {
      newclan = (CLAN *) mallok (sizeof (*newclan));
      clan_count++;
    }
  newclan->next = NULL;
  newclan->name = nonstr;
  newclan->leader = nonstr;
  newclan->motto = nonstr;
  newclan->history = nonstr;
  newclan->room_start = 0;
  newclan->num_rooms = 0;
  newclan->flags = 0;
  newclan->clan_store = 0;
  newclan->type = 0;
  newclan->minlev = 0;
  newclan->vnum = 0;
  for (i = 0; i < MAX_CLAN_STORE; i++)
    newclan->storage[i] = NULL;
  for (i = 0; i < MAX_CLAN_MEMBERS; i++)
    newclan->member[i] = nonstr;
  return newclan;
}



void
write_clans (void)
{
  FILE *f;
  int i;
  CLAN *cln;

  if ((f = wfopen ("clans.dat", "w")) == NULL)
    return;
  
  
  for (i = 0; i < CLAN_MAX; i++)
    {
      for (cln = clan_list[i]; cln; cln = cln->next)
	{
	  if (!IS_SET (cln->flags, CLAN_NOSAVE))
	    write_clan (f, cln);
	}
    }
  fprintf (f, "\n\nEND_OF_CLANS\n");
  fclose (f);
  return;
}


void
write_clan (FILE *f, CLAN *cln)
{
  int i;
  
  
  if (!cln || !f)
    return;
  
  fprintf (f, "CLAN %d %d\n", cln->type, cln->vnum);
  if (cln->name && cln->name[0])
    write_string (f, "Name", cln->name);
  if (cln->leader && cln->leader[0])
      write_string (f, "Leader", cln->leader);
  if (cln->motto && cln->motto[0])
    write_string (f, "Motto", cln->motto);
  if (cln->history && cln->history[0])
    write_string (f, "History", cln->history);
  fprintf (f, "Gen %d %d %d %d %d\n", cln->room_start, cln->num_rooms,
	   cln->flags, cln->minlev, cln->clan_store);
  for (i = 0; i < MAX_CLAN_MEMBERS; i++)
    {
      if (cln->member[i] && *cln->member[i])
	write_string (f, "Member", cln->member[i]);
    }
  for (i = 0; i < MAX_CLAN_STORE; i++)
    {
      if (cln->storage[i])
	{
	  fprintf (f, "STO");
	  write_short_thing (f, cln->storage[i], 0);
	  RBIT (cln->storage[i]->thing_flags, TH_SAVED);
	}
    }
  fprintf (f, "END_CLAN\n\n");
  return;
}


void
read_clans (void)
{
  FILE_READ_SETUP ("clans.dat");
  

  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY ("CLAN")
	read_clan(f);
      FKEY ("END_OF_CLANS")
	break;
      FILE_READ_ERRCHECK("clans.dat");
    }
  fclose (f);
  return;
}

void
read_clan (FILE *f)
{
  CLAN *cln, *prev;
  int member_count= 0;
  FILE_READ_SINGLE_SETUP;
  if (!f)
    return;
  
  cln = new_clan ();
  cln->type = read_number (f);
  cln->vnum = read_number (f);
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY ("Name")
	cln->name = new_str (read_string (f));
      FKEY ("Motto")
	cln->motto = new_str (read_string (f));
      FKEY ("History")
	cln->history = new_str (read_string (f));
      FKEY ("Leader")
	cln->leader = new_str (read_string (f));
      FKEY ("Gen")
	{
	  cln->room_start = read_number (f);
	  cln->num_rooms = read_number (f);
	  cln->flags = read_number (f);
	  cln->minlev = read_number (f);
	  cln->clan_store = read_number (f);
	}
      FKEY ("Member")
	{
	  if (member_count >= MAX_CLAN_MEMBERS)
	    read_string (f);
	  else
	    {
	      cln->member[member_count] = new_str (read_string (f));
	      member_count++;
	    }
	}
      FKEY ("STOTH")
	read_short_thing (f, NULL, cln);  
      FKEY ("END_CLAN")
	{	  
	  /* Make the "top clan" and then add the clan to the list */

	  if (cln->type < 0 || cln->type >= CLAN_MAX)
	    {
	      free_clan (cln);
	      return;
	    }
	  top_clan[cln->type] = MAX (top_clan[cln->type], cln->vnum);
	  if (!clan_list[cln->type] || clan_list[cln->type]->vnum > cln->vnum)
	    {
	      cln->next = clan_list[cln->type];
	      clan_list[cln->type] = cln;
	      return;
	    }
	  else
	    {
	      for (prev = clan_list[cln->type]; prev; prev = prev->next)
		{
		  if (!prev->next ||
		      prev->next->vnum > cln->vnum)
		    {
		      cln->next = prev->next;
		      prev->next = cln;
		      break;
		    }
		}
	    }
	  return;
	}
      FILE_READ_ERRCHECK ("clans.dat");
      
    }
  return;
}
	  
void
show_clan (THING *th, CLAN *cln)
{
  int i, mem_shown = 0;
  char buf[STD_LEN];
  char buf2[STD_LEN];
  CLAN *lclan, *iclan;
  if (!cln || !th || cln->type < 0 || cln->type >= CLAN_MAX)
    return;
  
  lclan = find_clan_in (th, cln->type, TRUE);
  iclan = find_clan_in (th, cln->type, FALSE);

  sprintf (buf, "%s (%d):  %s\n\r", clantypes[cln->type], cln->vnum,
	   cln->name);
  stt (buf, th);
  sprintf (buf, "Led by: %s      Motto: %s\n\r", cln->leader, cln->motto);
  stt (buf, th);
  sprintf (buf, "Min Level to Join: %d\n\r", cln->minlev);
  stt (buf, th);
  if (LEVEL (th) == MAX_LEVEL || lclan == cln)
    {
      sprintf (buf, "Room Start: %d     Num Rooms:   %d    Clan Store:     %d\n", cln->room_start, cln->num_rooms, cln->clan_store);
      stt (buf, th);
      stt (list_flagnames (FLAG_CLAN, cln->flags), th);      
    }
  if (LEVEL (th) == MAX_LEVEL || iclan == cln)
    {
      stt ("Clan History\n\n\r", th);
      stt (cln->history, th);
      stt ("\n\rMembers:\n\n\r", th);
      buf[0] = '\0';
      for (i = 0; i < MAX_CLAN_MEMBERS; i++)
	{
	  if (cln->member[i] && *cln->member[i])
	    {
	      sprintf (buf2, "%-15s", cln->member[i]);
	      strcat (buf, buf2);
	      mem_shown++;
	    }
	  if (mem_shown == 5 || i == MAX_CLAN_MEMBERS -1)
	    {
	      strcat (buf, "\n\r");
	      stt (buf, th);
	      mem_shown = 0;
	    }
	}
    }
  return;
}
      


void
clan (THING *th, char *arg, int type)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  CLAN *iclan, *lclan, *cln, *prev;
  int i, j, clan_num = -1, newslot = MAX_CLAN_MEMBERS, value, slot = MAX_CLAN_STORE;
  THING *obj;
  bool found = FALSE;
  if (!IS_PC (th) || type < 0 || type >= CLAN_MAX)
    return;
  
  arg = f_word (arg, arg1);
  
  
  /* If an admin types "clan 4 <something>" it can affect something
     without having to be the leader. */
  
  lclan = find_clan_in (th, type, TRUE);
  if (!lclan)
    iclan = find_clan_in (th, type, FALSE);
  else
    iclan = lclan;
  
  if (is_number (arg1))
    {
      clan_num = atoi (arg1);
      arg = f_word (arg, arg1);
    }
  
  if (clan_num != -1 && LEVEL(th) == MAX_LEVEL)
    {
      if ((lclan = find_clan (clan_num, type)) == NULL)
	{
	  sprintf (buf, "That %s %d does not exist to edit!\n\r", 
		   clantypes[type], clan_num);
	  stt (buf, th);
	  return;
	}
      if (lclan)
	iclan = lclan;
    }
  
  if (!str_cmp (arg1, "show"))
    {
      if (iclan)
	{
	  show_clan (th, iclan);
	  return;
	}
      else
	{ 
	  sprintf (buf, "You arent in %s %s!\n\r", 
		   a_an (clantypes[type]),
		   clantypes[type]);
	  stt (buf, th);
	  return;
	}
    }
  if (!str_cmp (arg1, "store"))
    {
      if (!th->in || !IS_ROOM (th->in))
	{
	  sprintf (buf, "You aren't in %s %s bank!\n\r", 
		   a_an (clantypes[type]), clantypes[type]);
	  stt (buf, th);
	  return;
	}
      for (cln = clan_list[type]; cln != NULL; cln = cln->next)
	{
	  if (cln->clan_store == th->in->vnum)
	    break;
	}

      if (!cln)
	{
	  sprintf (buf, "You aren't in %s %s bank!\n\r", 
		   a_an (clantypes[type]), clantypes[type]);
	  stt (buf, th);
	  return;
	}
      
      if (arg[0] == '\0')
	{
	  sprintf (buf, "The %s has the following things in storage:\n\n\r", 
		   clantypes[type]);
	  stt (buf, th);
	  for (i = 0; i < MAX_CLAN_STORE; i++)
	    {
	      if (cln->storage[i] != NULL)
		{
		  found = TRUE;
		  sprintf (buf, "\x1b[1;30m[\x1b[1;37m%2d\x1b[1;30m]\x1b[0;37m  %s. \n\r", 
			   i + 1, NAME (cln->storage[i]));
		  stt (buf, th);
		}
	    }
	  if (!found)
	    {
	      stt ("Nothing.\n\r", th);
	    }
	  return;
	}
      
      if ((obj = find_thing_me (th, arg)) == NULL)
	{
	  stt ("You don't have that item.\n\r", th);
	  return;
	}

      if (obj->cont)
	{
	  stt ("Sorry, the clan won't accept any items unless they're empty.\n\r", th);
	  return;
	}
      
      for (i = 0; i < MAX_CLAN_STORE; i++)
	{
	  if (cln->storage[i] == NULL)
	    {
	      slot = i;
	      break;
	    }
	}
      
      if (slot == MAX_CLAN_STORE)
	{
	  stt ("You don't have enough space left to store this!\n\r", th);
	  return;
	}
      
      thing_from (obj);
      cln->storage[slot] = obj;
      sprintf (buf, "Ok, %s is stored away in the %s bank.\n\r",
	       NAME (obj), clantypes[type]);
      stt (buf, th);
      write_clans ();
      return;
    }
  
  if (!str_cmp (arg1, "unstore"))
    {
      obj = NULL;
      if (!th->in || !IS_ROOM (th->in))
	{
	  sprintf (buf, "You aren't in %s %s bank!\n\r", 
		   a_an (clantypes[type]), clantypes[type]);
	  stt (buf, th);
	  return;
	}
      
      for (cln = clan_list[type]; cln != NULL; cln = cln->next)
	{
	  if (cln->clan_store == th->in->vnum)
	    break;
	}
      
      if (!cln)
	{
	  sprintf (buf, "You aren't in %s %s bank!\n\r", 
		   a_an (clantypes[type]), clantypes[type]);
	  stt (buf, th);
	  return;
	}
      
      if (arg[0] == '\0')
	{
	  sprintf (buf, "%s unstore <what>\n\r", clantypes[type]);
	  stt (buf, th);
	  return;
	}
      if (str_cmp (arg, "all"))
	{
	  if (is_number (arg))
	    {
	      slot = atoi (arg);
	      
	      if (slot < 1 || slot > MAX_CLAN_STORE ||
		  cln->storage[slot - 1] == NULL)
		{
		  stt ("There is no item stored there!\n\r", th);
		  return;
		}
	      obj = cln->storage[slot - 1];
	    }
	  else
	    {
	      slot = MAX_CLAN_STORE;
	      for (i = 0; i < MAX_CLAN_STORE; i++)
		{
		  if (cln->storage[i] != NULL &&
		      is_named (cln->storage[i], arg1))
		    {
		      obj = cln->storage[i];
		      slot = i + 1;
		      break;
		    }
		}
	    }
	  if (!obj)
	    {
	      stt ("You can't find that item to unstore!\n\r", th);
	      return;
	    }
	  obj->in = NULL;
	  obj->prev_cont = NULL;
	  obj->next_cont = NULL;
	  thing_to (obj, th);
	  cln->storage[slot - 1] = NULL;
	  write_clans ();
	  sprintf (buf, "You retrieve the item from the %s store.\n\r",
		   clantypes[type]);
	  stt (buf, th);
	  return;
	}
      for (i = 0; i < MAX_CLAN_STORE; i++)
	{
	  if (cln->storage[i])
	    {
	      thing_to (cln->storage[i], th);
	      cln->storage[i] = NULL;
	    }
	}
      write_clans ();
      sprintf (buf, "All items removed from the %s store.\n\r",
	       clantypes[type]);
      stt (buf, th);
      return;
    }
 
	  
      
      
  
  if (LEVEL (th) == MAX_LEVEL && !str_cmp (arg1, "create"))
    {
      int clans_used[10];
      if (clan_num != -1)
	{
	  sprintf (buf, "You cannot specify a specific %s to make. The number chosen is the smallest number available.\n\r", clantypes[type]);
	  return;
	}
      for (i = 0; i < 10; i++)
	clans_used[i] = 0;
      SBIT (clans_used[0], (1 << 0));
      for (cln = clan_list[type]; cln; cln = cln->next)
	SBIT (clans_used[cln->vnum / 32], (1 << cln->vnum % 32));
      for (i = 0; i < 10 && clan_num == -1; i++)
	{
	  for (j = 0; j < 32 && clan_num == -1; j++)
	    {
	      if (!IS_SET (clans_used[i], (1 << j)))
		{
		  clan_num = 32*i + j;
		}
	    }
	}
      cln = new_clan ();
      cln->type = type;
      cln->vnum = clan_num;
      if (!clan_list[type] || clan_list[type]->vnum > cln->vnum)
	{
	  cln->next = clan_list[type];
	  clan_list[type] = cln;
	}
      else 
	{
	  for (prev = clan_list[type]; prev; prev = prev->next)
	    {
	      if (!prev->next || prev->next->vnum > cln->vnum)
		{
		  cln->next = prev->next;
		  prev->next = cln;
		  break;
		}
	    }
	}
      write_clans ();
      show_clan (th, cln);
      return;
    }
	
  if (!str_cmp (arg1, "list"))
    {
      
      sprintf (buf, "The %s list:\n\n\r", clantypes[type]);
      stt (buf, th);
      if (!clan_list[type])
	{
	  stt ("None.\n\r", th);
	  return;
	}
      for (cln = clan_list[type]; cln; cln = cln->next)
	{
	  sprintf (buf, "%s %2d: %-25s Leader: %s\n\r", clantypes[type],
		   cln->vnum, cln->name, cln->leader);
	  stt (buf, th);
	}
      stt ("\n\r", th);
      return;
    }
  if (!str_cmp (arg1, "quit"))
    {
      if (lclan)
	{
	  sprintf (buf, "You cannot quit %s %s which you are leading!\n\r",
		   a_an (clantypes[type]), clantypes[type]); 
	  stt (buf, th);
	  return;
	}
      if (!iclan)
	{
	  sprintf (buf ,"You aren't even in %s %s!\n\r", 
		   a_an (clantypes[type]), clantypes[type]); 
	  stt (buf, th);
	  return;
	}
      else
	{
	  sprintf (buf, "Ok, you are leaving your %s.\n\r", clantypes[type]);
	  stt (buf, th);
	  for (i = 0; i < MAX_CLAN_MEMBERS; i++)
	    {
	      if (!str_cmp (iclan->member[i], NAME(th)))
		{
		  free_str (iclan->member[i]);
		  break;
		}
	    }
	  write_clans ();
	  if (IS_PC (th))
	    th->pc->clan_num[type] = 0;
	  return;
	}
    }
  if (!str_cmp (arg1, "add") || !str_cmp (arg1, "remove"))
    {
      bool remove  = !str_cmp (arg1, "remove");
      
      if (!lclan)
	{
	  sprintf (buf, "You must be the leader of %s %s to add or remove someone!\n\r",  a_an (clantypes[type]), clantypes[type]); 
	  return;
	}
      if (!str_cmp (arg, NAME (th)))	
	{
	  sprintf (buf, "You cannot %s yourself %s the %s!\n\r",
		   (remove ? "remove" : "add"),
		   (remove ? "from" : "to"),
		   clantypes[type]);
	  stt (buf, th);
	  return;
	}
      if (remove)
	{
	  for (i = 0; i < MAX_CLAN_MEMBERS; i++)
	    {
	      if (!str_cmp (lclan->member[i], arg))
		{
		  free_str (lclan->member[i]);
		  lclan->member[i] = nonstr;
		  stt ("Ok, member removed.\n\r", th);		
		  write_clans ();
		  return;
		}
	    }
	  stt ("That person doesn't seem to be in your clan.\n\r", th);
	  return;
	}
      else
	{
	  for (i = 0; i < MAX_CLAN_MEMBERS; i++)
	    {
	      if (lclan->member[i] == nonstr || 
		  lclan->member[i] == NULL)
		{
		  newslot = i;
		  break;
		}
	    }
	  if (newslot == MAX_CLAN_MEMBERS)
	    {
	      stt ("You don't have any more room to add anyone!\n\r", th);
	      return;
	    }
	  for (cln = clan_list[type]; cln; cln = cln->next)
	    {
	      if (!str_cmp (cln->leader, arg))
		{
		  sprintf (buf, "That person leads another %s!\n\r", clantypes[type]);
		  stt (buf, th);
		  return;
		}
	      for (i = 0; i < MAX_CLAN_MEMBERS; i++)
		{
		  if (!str_cmp (arg, cln->member[i]))
		    {
		      sprintf (buf, "That person is already in another %s!\n\r", clantypes[type]);
		      stt (buf, th);
		      return;
		    }
		}
	    }
	  lclan->member[newslot] = new_str (arg);
	  stt ("Ok, member added.\n\r", th);
	  write_clans ();
	  return;
	}
    }
  if (lclan)
    {
      if (!str_cmp (arg1, "motto"))
	{
	  free_str (lclan->motto);
	  lclan->motto = new_str (arg);
	  show_clan (th, lclan);
	  return;
	}
      if (!str_cmp (arg1, "name"))
	{
	  free_str (lclan->name);
	  lclan->name = new_str (arg);
	  show_clan (th, lclan);	 
	  return;
	}
      if (!str_cmp (arg1, "history"))
	{
	  new_stredit (th, &lclan->history);
	  return;
	}
      if (LEVEL (th) == MAX_LEVEL)
	{
	  if (!str_cmp (arg1, "leader"))
	    {
	      free_str (lclan->leader);
	      lclan->leader = new_str (arg);
	      stt ("Ok, leader changed.\n\r", th);
		  show_clan (th, lclan);
	      return;
	    }
	  if (!str_cmp (arg1, "room_start"))
	    {
	      if (is_number (arg))
		{
		  lclan->room_start = atoi (arg);
		  stt ("Ok, room start number set.\n\r", th);
		  show_clan (th, lclan);
		  return;
		}
	      else
		{
		  stt ("Syntax: clan <number> room_start <number>\n\r", th);
		  return;
		}
	    }
	  if (!str_cmp (arg1, "clanstore") || !str_cmp (arg1, "clan_store"))
	    {
	      if (is_number (arg))
		{
		  lclan->clan_store = atoi (arg);
		  stt ("Ok clan store room number set.\n\r", th);
		  show_clan (th, lclan);
		  return;
		}
	    }
	  if (!str_cmp (arg1, "num_rooms"))
	    {
	      if (is_number (arg))
		{
		  lclan->num_rooms = atoi (arg);
		  stt ("Ok, number of rooms set.\n\r", th);
		  show_clan (th, lclan);
		  return;
		}
	      else
		{
		  stt ("Syntax: clan <number> num_rooms <number>\n\r", th);
		  return;
		}
	    }
	  if (!str_cmp (arg1, "minlev"))
	    {
	      if (is_number (arg))
		{
		  lclan->minlev = atoi (arg);
		  stt ("Ok, minimum level to join set.\n\r", th);
		  show_clan (th, lclan);
		  return;
		}
	      else
		{
		  stt ("Syntax: clan <number> minlev <number>\n\r", th);
		  return;
		}
	    }
	  if ((value =find_bit_from_name (FLAG_CLAN, arg)) != 0)
	    {
	      sprintf (buf, "Clan flag %s toggled %s.\n\r", arg, 
		       IS_SET (lclan->flags, value) ? "Off": "On");
	      stt (buf, th);
	      lclan->flags ^= value;
	      show_clan (th, lclan);
	      return;
	    }
	}
      stt ("Clan What????\n\r", th);
      return;
    }
  stt ("Clan list, clan show, clan quit.\n\r", th);
  return;
}
	  
