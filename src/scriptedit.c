#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "script.h"

/* This finds a piece of code based on a name given. */

CODE *
find_code_chunk (char *arg)
{
  char *t;
  CODE *code;
  if (!arg || *arg < 'a' || *arg > 'z' || *(arg + 1) < 'a' || *(arg + 1) > 'z')
    return NULL;
  for (t = arg; *t; t++)
    {
      if (!isalpha (*t) && !isdigit (*t))
	return NULL;
    }
  for (code = code_hash[*arg - 'a'][*(arg + 1) - 'a']; code; code = code->next)
    {
      if (!str_cmp (code->name, arg))
	return code;
    }
  return NULL;
}
	 




TRIGGER *
find_trigger (char *arg)
{
  int i;
  TRIGGER *trig;
  if (!arg || !*arg)
    return NULL;
  
  for (i = 0; i < HASH_SIZE; i++)
    {
      for (trig = trigger_hash[i]; trig != NULL; trig = trig->next)
	{
	  if (!str_cmp (arg, trig->name))
	    return trig;
	}
    }
  return NULL;
}



void
do_trigedit (THING *th, char *arg)
{
  char arg1[STD_LEN];
  TRIGGER *trig = NULL, *prev;
  int i;
  char buf[STD_LEN];
  
  if (LEVEL (th) < MAX_LEVEL || !th->fd || !th->pc)
    {
      stt ("Huh?\n\r", th);
      return;
    }
  
  if (IS_SET (th->fd->flags, FD_EDIT_PC))
    {
      stt ("You cannot edit anything else while you are editing a PC.\n\r", th);
      return;
    }
  arg = f_word (arg, arg1);
  if (!str_cmp (arg1, "list"))
    {
      bigbuf[0] = '\0';
      add_to_bigbuf ("Name        Number     Type              Code\n\r");
      for (i = 0; i < HASH_SIZE; i++)
	{
	  for (trig = trigger_hash[i]; trig != NULL; trig = trig->next)
	    {
	      sprintf (buf, "%-10s  #%-5d     %-10s        %s\n\r",
		       trig->name, trig->vnum, show_trigger_type(trig->type), 
		       trig->code);
	      add_to_bigbuf (buf);
	    }
	}
      send_bigbuf (bigbuf, th);
      return;
    }
  if (!str_cmp (arg1, "delete") && *arg)
    {
      if ((trig = find_trigger (arg)) == NULL)
        {
          stt ("That trigger doesn't exist!\n\r", th);
	  return;
	}	
      if (trig == trigger_hash[trig->vnum % HASH_SIZE])
	trigger_hash[trig->vnum % HASH_SIZE] = trigger_hash[trig->vnum % HASH_SIZE]->next;
      else
	{
	  for (prev = trigger_hash[trig->vnum % HASH_SIZE]; prev != NULL; prev = prev->next)
	    {
	      if (prev->next == trig)
		{
		  prev->next = trig->next;
		  break;
		}
	    }
	}
      free_trigger (trig);
      stt ("Ok, trigger removed.\n\r", th);
      return;
    }
  if (!str_cmp (arg1, "create") && *arg)
    {
      if ((trig = find_trigger (arg)) != NULL)
	stt ("This trigger exists. Editing now.\n\r", th);
      else
	{
	  trig = new_trigger ();
	  trig->name = new_str (arg);
	  trig->next = trigger_hash[0];
	  trigger_hash[0] = trig;
	}
    }
  else 
    trig = find_trigger (arg1);
  
  if (!trig)
    {
      stt ("trigedit create <name> or trigedit <name>.\n\r", th);
      return;
    }
  
  th->pc->editing = trig;
  th->fd->connected = CON_TRIGEDITING;
  show_trigger (th);
  return;
}

char *
show_trigger_type (int num)
{
  int i;
  if (num == 0)
    return (char *) trigger_types[0];
  for (i = 1; (1 << i) <= TRIG_MAX; i++)
    {
      if (IS_SET (num, (1 << i)))
	return (char *) trigger_types[i+1];
    }
  return "(unknown)";
}

void
show_trigger (THING *th)
{
  char buf[STD_LEN];
  TRIGGER *trig;
  if (!th )
    return;
  if (!th->pc || !th->pc->editing || LEVEL (th) < MAX_LEVEL || 
      !th->fd || th->fd->connected != CON_TRIGEDITING)
    {
      if (th->pc)
	th->pc->editing = NULL;
      if (th->fd)
	th->fd->connected = CON_ONLINE;
      return;
    }
  trig = (TRIGGER *) th->pc->editing;
  
  sprintf (buf, "Trigger: \x1b[1;32m%-10s\x1b[0;37m   Type: \x1b[1;35m%-10s\x1b[0;37m   Thing: \x1b[1;37m%d\x1b[0;37m\n\r", trig->name, show_trigger_type (trig->type), trig->vnum);
  stt (buf, th);
  sprintf (buf, "Timer: \x1b[1;31m%d\x1b[0;37m  Pct: \x1b[1;34m%d\x1b[0;37m  Flags: \x1b[1;36m%s\x1b[0;37m \n\r", trig->timer, trig->pct, list_flagnames (FLAG_TRIGGER, trig->flags));
  stt (buf, th);
  sprintf (buf, "Code: \x1b[1;37m%-10s\x1b[0;37m,   Keywords: \x1b[1;33m%s\x1b[0;37m\n\r", trig->code, trig->keywords);
  stt (buf, th);
  return;
}
	   
	   
void
trigedit (THING *th, char *arg)
{ 
  char arg1[STD_LEN];
  TRIGGER *trig;
  char *oldarg = arg;
  int value, i;
  if (!th)
    return;
  if (!IS_PC (th) || !th->pc->editing || LEVEL (th) < MAX_LEVEL || 
      !th->fd || th->fd->connected != CON_TRIGEDITING)
    {
      if (th->pc)
	th->pc->editing = NULL;
      if (th->fd)
	th->fd->connected = CON_ONLINE;
      return;
    }
  trig = (TRIGGER *) th->pc->editing;
  
  if (!arg || !*arg)
    {
      show_trigger (th);
      return;
    }
  
  arg = f_word (arg, arg1);
  if (!str_cmp (arg1, "flag"))
    {
      for (i = 0; trigger1_flags[i].flagval != 0; i++)
	{
	  if (!str_prefix (arg, trigger1_flags[i].name))
	    {
	      trig->flags ^= trigger1_flags[i].flagval;
	      stt ("Flag toggled.\n\r", th);
	      show_trigger (th);
	      return;
	    }
	}
    }
  if (!str_cmp (arg1, "code"))
    {
      free_str (trig->code);
      trig->code = new_str (arg);
      show_trigger (th);
      return;
    }

 if (!str_cmp (arg1, "name"))
    {
      free_str (trig->name);
      trig->name = new_str (arg);
      show_trigger (th);
      return;
    }
 if (!str_cmp (arg1, "pct") || !str_cmp (arg1, "percent"))
   {
     if (!isdigit (*arg))
       trig->pct = 100;
     else 
       trig->pct = MID (0, atoi(arg), 100);
     show_trigger (th);
     return;
   }
 if (!str_prefix ("key", arg1))
   {
     free_str (trig->keywords);
     trig->keywords = new_str (arg);
     show_trigger (th);
     return;
   }
 if (!str_cmp ("done", arg1))
   {
     stt ("Ok, all done trig editing.\n\r", th);
     if (IS_PC (th))
       th->pc->editing = NULL;
     if (th->fd)
       th->fd->connected = CON_ONLINE;
     setup_timed_triggers ();
     return;
   }
 
 if (!str_cmp ("type", arg1))
   {
     for (i = 0; str_cmp ((char *) trigger_types[i], "max"); i++)
       if (!str_cmp (arg, (char *) trigger_types[i]))
	 {
	   trig->type = (i > 0 ? (1 << (i-1)) : 0);
	   stt ("Ok, trigger type set.\n\r", th);
	   show_trigger (th);
	   return;
	 }
     stt ("type <trigger_type>\n\r", th);
     return;
   }
 if (!str_cmp (arg1, "thing") || !str_cmp (arg1, "vnum"))
   {
     TRIGGER *prev;
     if (!is_number (arg))
       {
	 stt ("Thing <number>\n\r", th);
	 return;
       }
     
     value = atoi (arg);
     if (value < 0)
       {
	 stt ("The value must be nonnegative.\n\r", th);
	 return;
       }
     
     if (trigger_hash[trig->vnum % HASH_SIZE] == trig)
       {
	 trigger_hash[trig->vnum % HASH_SIZE] = trigger_hash[trig->vnum % HASH_SIZE]->next;
	 trig->next = NULL;
       }
     else
       {
	 for (prev = trigger_hash[trig->vnum % HASH_SIZE]; prev != NULL; prev = prev->next)
	   {
	     if (prev->next == trig)
	       {
		 prev->next = prev->next->next;
		 trig->next = NULL;
		 break;
	       }
	   }
       }
     trig->vnum = value;
     trig->next = trigger_hash[trig->vnum % HASH_SIZE];
     trigger_hash[trig->vnum % HASH_SIZE] = trig;
     show_trigger (th);
     return;
   }

 if (!str_prefix ("time", arg1) || !str_prefix ("wait", arg1))
   {
     if (!is_number (arg))
       {
	 stt ("Wait <number>\n\r", th);
	 return;
       }
     
     value = atoi (arg);
     if (value < 0)
       {
	 stt ("The value must be nonnegative.\n\r", th);
	 return;
       }

     trig->timer = value;
     show_trigger (th);
     return;
   }
 
 interpret (th, oldarg);
 return;
}
