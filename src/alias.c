#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* This file deals with setting up aliases. The work of making sure
   aliases work is done in read_in_command() in serv.c */

/* This sets up and deletes aliases, and it also lists out any aliases
   that you happen to have atm. It does some checking to keep the
   lamer aliases away, but it is quite possible that there are ways
   to be a lamer and mess this up. :P */

void
do_alias (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  char tempbuf[STD_LEN]; /* Used in recursives checking */
  char tempword[STD_LEN]; /* Used in recursives checking */
  int number = 0, i;
  char *s, *t, *oldarg = arg;
  bool found = FALSE, is_recursive = FALSE;
  if (!th || !IS_PC (th) || !th->fd || !arg)
    return;
  
  /* If no argument, just list the current aliases. */

  if (arg[0] == '\0')
    {
      for (i = 0; i < MAX_ALIAS; i++)
	{
	  if (th->pc->aliasname[i] && *th->pc->aliasname[i] &&
	      th->pc->aliasexp[i] && *th->pc->aliasexp[i])
	    {
	      sprintf (buf, "\x1b[1;34m[\x1b[1;37m%2d\x1b[1;34m]\x1b[1;33m %-13s \x1b[0;37m:\x1b[1;36m%s\x1b[0;37m\n\r", i + 1, th->pc->aliasname[i], th->pc->aliasexp[i]);
	      found = TRUE;
	      stt (buf, th);
	    }
	}
      if (!found)
	stt ("\n\rNone.\n\r", th);
      return;
    }
  

 /* Get the number of the alias we will work with. */
  arg = f_word (arg, arg1);
  number = atoi (arg1);
  
  for (t = oldarg; *t; t++)
    {
      if (*t == '\"')
	{
	  stt ("You cannot put a \" inside of an alias.\n\r", th);
	  return;
	}
    }
  
  /* Check if the alias name is one we already have. */

  for (i = 0; i < MAX_ALIAS; i++)
    {
      if (!str_cmp (th->pc->aliasname[i], arg1))
	{
	  found = TRUE;
	  number = i;
	  break;
	}
    }
  
  /* If the alias name was not one we had, then we see if the name
     we are looking for is really a number, and if it is, we use that
     number. */
     

  if (!found && number >= 1 && number <= MAX_ALIAS && !*arg)
    {
      number--;
      found = TRUE;
    }
  
  /* If we don't have an argument, we assume we delete the alias. */
  
  if (*arg == '\0')
    {
      if (!found)
	{
	  stt ("That alias doesn't exist to delete!\n\r", th);
	  return;
	}
      free_str (th->pc->aliasname[number]);
      th->pc->aliasname[number] = nonstr;
      free_str (th->pc->aliasexp[number]);
      th->pc->aliasexp[number] = nonstr;
      stt ("Ok, alias cleared.\n\r", th);
      return;
    }
  
  /* Now we are attempting to add a new alias. We need to have alias
     <name> <expansion> here. We do some checking for recursives and
     all that. Note that alias <number> <expansion>, does not put an
     alias into slot <number> it aliases the string <number>... so that
     you can do alias 1 tackle enemy or stuff like that. */
     
  if (!found)
    {
      number = MAX_ALIAS;
      for (i = 0; i < MAX_ALIAS; i++)
	{
	  if (!th->pc->aliasname[i] ||
	      !*th->pc->aliasname[i] ||
	      !th->pc->aliasexp[i] ||
	      !*th->pc->aliasexp[i])
	    {
	      number = i;
	      break;
	    }
	}
    }

  /* If we are not replacing and can't find any free slots, we bitch at
     the player and return. */

  if (number == MAX_ALIAS)
    {
      sprintf (buf, "You are limited to %d alias slots! Try freeing some up.\n\r", MAX_ALIAS);
      stt (buf, th);
      return;
    }

  /* At this point, we check for recursiveness. Need to check the new
     command vs ALL possible expansions and the new expansion vs
     all possible commands...including the new one. Now, we put the 
     new alias into slot <number>. We THEN check for recursiveness. */
  
  
  free_str (th->pc->aliasname[number]);
  th->pc->aliasname[number] = new_str (arg1);
  free_str (th->pc->aliasexp[number]);
  th->pc->aliasexp[number] = new_str (arg);

  /* Now check arg1 vs the "f_word" of every piece of every aliasexp. */
  
  for (i = 0; i < MAX_ALIAS && !is_recursive; i++)
    {
      if ((t = th->pc->aliasexp[i]) != NULL && *t)
	{
	  while (*t && !is_recursive)
	    {
	      s = tempbuf;
	      while (*t && *t != '*')
		{
		  *s = *t;
		  s++;
		  t++;
		}
	      *s = '\0'; /* End the tempbuf string */
	      if (*t) /* Move t past the '*' */
		t++;
	      s = tempbuf;
	      s = f_word (s, tempword);
	      if (!str_cmp (arg1, tempword))
		is_recursive = TRUE;
	    }
	}
    }
  
  /* Check expansion string vs all aliases. */
  
  if (!is_recursive) 
    {
      t = th->pc->aliasexp[number];
      while (*t && !is_recursive)
	{
	  s = tempbuf;
	  while (*t && *t != '*')
	    {
	      *s = *t;
	      s++;
	      t++;
	    }
	  *s = '\0';
	  if (*t)
	    t++;
	  s = tempbuf;
	  s = f_word (s, tempword);
	  for (i = 0; i < MAX_ALIAS && !is_recursive; i++)
	    {
	      if (th->pc->aliasname[i] && *th->pc->aliasname[i])
		{
		  if (!str_cmp (tempword, th->pc->aliasname[i]))
		    {
		      is_recursive = TRUE;
		      break;
		    }
		}
	    }
	}
    }

  /* If it's a recursive alias...dump it. */
  
  if (is_recursive)
    {
      free_str (th->pc->aliasname[number]);
      th->pc->aliasname[number] = nonstr;      
      free_str (th->pc->aliasexp[number]);
      th->pc->aliasexp[number] = nonstr;
      stt ("Recursive aliases are not acceptable!!!\n\r", th);
      return;
    }
  
  stt ("Ok, alias set.\n\r", th);
  return;
}

 













