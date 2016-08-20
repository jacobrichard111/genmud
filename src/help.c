#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* These are the helpfiles. They are hashed by the first two letters
   of the keywords. Hopefully this will make it sorta faster to find
   helpfiles. BE DARN CAREFUL WHEN FREEING THINGS SINCE MANY MANY
   HELP_KEY'S POINT TO THE SAME HELP!!!! */

/* The helpfile format was changed since they're more complicated, but
   this should work to read them in. */


void
do_help (THING *th, char *arg)
{
  HELP *hlp;
  CMD *com;
  char buf[STD_LEN];
  SPELL *spl;
  int i, count = 0;
  if (arg[0] == '\0')
    {
      stt ("Type help <keyword> to get help on that subject.\n\r", th);
      stt ("Some important commands include quit, who, score, l, n, e, s, w, d, u\n\r", th);
      stt ("i, eq, chat, say, tell, commands, rules.\n\r", th);
      return;
    }
  if (LEVEL (th) == MAX_LEVEL && 
      (!str_cmp (arg, "nohelps") || !str_cmp (arg, "nohelp")))
    {
      bigbuf[0] = '\0';
      add_to_bigbuf ("The following commands don't have helpfiles: \n\n\r");
      for (i = 0; i < 256; i++)
	{
	  for (com = com_list[i]; com != NULL; com = com->next)
	    {
	      if ((hlp = find_help_name (th, com->name)) != NULL)
		continue;
	      sprintf (buf, "%-18s", com->name);
	      add_to_bigbuf (buf);
	      if (++count == 4)
		{
		  count = 0;
		  add_to_bigbuf ("\n\r");
		}
	    }
	}
      add_to_bigbuf ("\n\n\rThe following spells don't have helpfiles:\n\r");
      count = 0;
      for (spl = spell_list; spl; spl = spl->next)
	{
	  if ((hlp = find_help_name (th, spl->name)) != NULL)
	    continue;
	  sprintf (buf, "%-18s", spl->name);
	  add_to_bigbuf (buf);
	  if (++count == 4)
	    {
	      count = 0;
	      add_to_bigbuf ("\n\r");
	    }
	}
      add_to_bigbuf ("\n\r");
      send_bigbuf (bigbuf, th);
      return;
    }
  if (!str_cmp (arg, "write_helps") && LEVEL(th) == MAX_LEVEL)
    {
      write_helps();
      stt ("Helps written.\n\r", th);
      return;
    }
  if (!str_cmp (arg, "reload_helps") && LEVEL (th) == MAX_LEVEL)
    {
      /* Wipe out the old helpfiles */

      clear_helps();
   
      read_helps ();
      stt ("Ok, helpfiles all reloaded!\n\r", th);
      return;
    }
  if ((hlp = find_help_name (th, arg)) == NULL)
    {
      stt ("There is no help on that word.\n\r", th);
      return;
    }
  show_help_file (th, hlp);
  return;
}

/* This clears all of the helpfiles up. */

void
clear_helps (void)
{
  HELP *hlp, *hlpn;
  HELP_KEY *hlpk, *hlpkn;
  int i, j;
  
  for (hlp = help_list; hlp; hlp = hlpn)
    {
      hlpn = hlp->next;
      free_help (hlp);
    }
  help_list = NULL;
  for (i = 0; i < 27; i++)
    {
      for (j = 0; j < 27; j++)
	{
	  for (hlpk = help_hash[i][j]; hlpk; hlpk = hlpkn)
	    {
	      hlpkn = hlpk->next;
	      free_help_key (hlpk);
	    }
	  help_hash[i][j] = NULL;
	}
    }
  return;
}

/* This reads in all of the helpfiles. It is assumed that if the
   first word read in is not HELP that this is the old helpfile
   format, so it will read all in and then write them out in the
   new format. */

void
read_helps (void)
{
  HELP *newhelp;
  FILE_READ_SETUP ("help.dat");
  
  clear_helps();
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY ("HELP")
	{
	  if ((newhelp = read_help (f)) != NULL)
	    setup_help (newhelp);
	}	  
      FKEY ("END_HELPS")
	break;
      FILE_READ_ERRCHECK("help.dat");
    }
  fclose (f);
  return;
}

/* This reads in a single helpfile. */

HELP *read_help (FILE *f)
{
  HELP *help;
  FILE_READ_SINGLE_SETUP;
  if (!f)
    return NULL;
  
  if ((help = new_help ()) == NULL)
    return NULL;
  
  /* Get the help level. */
  strcpy (word, read_word (f));
  if (is_number (word) || !str_cmp ("MAX", word) ||
      !str_cmp ("BLD", word) || !str_cmp ("MORT", word))
    {
      help->level = atoi (word);
      if (!str_cmp (word, "MAX"))
	help->level = MAX_LEVEL;
      else if (!str_cmp (word, "BLD"))
	help->level = BLD_LEVEL;
      else if (!str_cmp (word, "MORT"))
	help->level = 0;
    }
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("Key")
	help->keywords = new_str (read_string (f));
      FKEY("Admin")
	help->admin_text = new_str (add_color (read_string (f)));
      FKEY("Text")
	help->text = new_str (add_color (read_string (f)));
      FKEY("SeeAlso")
	help->see_also = new_str (read_string (f));
      FKEY("END_HELP")
	break;
      FILE_READ_ERRCHECK("help.dat-single");
    }
  return help;
}
  

/* This writes out all helps. */

void
write_helps (void)
{
  FILE *f;
  
  HELP *help;
  
  if ((f = wfopen ("help.dat", "w")) == NULL)
    return;

  for (help = help_list; help; help = help->next)
    write_help (f, help);
  fprintf (f, "\n\nEND_HELPS\n");
  fclose (f);
  return;
}

/* This writes a single help to the help.dat file. */

void 
write_help (FILE *f, HELP *help)
{
  if (!f || !help)
    return;

  fprintf (f, "HELP ");
  if (help->level <= 0)
    fprintf (f, "MORT\n");
  else if (help->level >= MAX_LEVEL)
    fprintf (f, "MAX\n");
  else if (help->level == BLD_LEVEL)
    fprintf (f, "BLD\n");
  else
    fprintf (f, "%d\n", help->level);
  write_string (f, "Key", help->keywords);
  write_string (f, "Admin", help->admin_text);
  write_string (f, "Text", help->text);
  write_string (f, "SeeAlso", help->see_also);
  fprintf (f, "END_HELP\n\n");
  return;
}
  


  /* This sets up a single helpfile. */

void
setup_help (HELP *newhelp)
{
  char *s, *t;
  int start_letter, second_letter;
  HELP_KEY *nhlpk, *nhlpk2;
  char keyword[STD_LEN];
  if (!newhelp)
    return;
  
  s = newhelp->keywords;
  for (; *s; s++)
    *s = UC (*s);
  s = newhelp->keywords;
  
  /* We now add the keywords to the hash table so that
     lookups are faster. */
  
  while (*s != '\0')
    {
      s = f_word (s, keyword);
      t = keyword;
      if (LC(*t) >= 'a' && LC(*t) <= 'z')
	start_letter = LC (*t) - 'a';
      else
	start_letter = 26;
      t++;
      if (LC(*t) >= 'a' && LC(*t) <= 'z')
	second_letter = LC (*t) - 'a';
      else
	second_letter = 26;
      nhlpk = new_help_key ();
      nhlpk->helpfile = newhelp;
      nhlpk->keyword = new_str (keyword);
      /* Place the new helpkey in the list in alphabetical order. 
	 Note that I use the real strcasecmp here since I want
	 the difference between the letters not a bool. */
      if (!help_hash[start_letter][second_letter] || 
	  strcasecmp (nhlpk->keyword, 
		      help_hash[start_letter][second_letter]->keyword
		      ) < 0)
	{
	  nhlpk->next = help_hash[start_letter][second_letter];
	  help_hash[start_letter][second_letter] = nhlpk;
	}
      else
	{
	  for (nhlpk2 = help_hash[start_letter][second_letter];
	       nhlpk2 != NULL; nhlpk2 = nhlpk2->next)
	    {
	      if (!nhlpk2->next ||
		  strcasecmp (nhlpk->keyword, nhlpk2->next->keyword) < 0)
		{
		  nhlpk->next = nhlpk2->next;
		  nhlpk2->next = nhlpk;
		  break;
		}
	    }
	}
      
    }
  newhelp->next = help_list;
  help_list = newhelp;
  return;
}


HELP *
new_help (void)
{
  HELP *hlp;
  
  if (help_free)
    {
      hlp = help_free;
      help_free = help_free->next;
    }
  else
    {
      hlp = (HELP *) mallok (sizeof (*hlp));
      help_count++;
    }
  bzero (hlp, sizeof (*hlp));
  
  hlp->admin_text = nonstr;
  hlp->text = nonstr;
  hlp->keywords = nonstr;
  hlp->see_also = nonstr;
  return hlp;
}


void
free_help (HELP *hlp)
{
  if (!hlp)
    return;
  free_str (hlp->keywords);
  hlp->keywords = nonstr;
  free_str (hlp->admin_text);
  hlp->admin_text = nonstr;
  free_str (hlp->text);
  hlp->text = nonstr;
  free_str (hlp->see_also);
  hlp->see_also = nonstr;
  hlp->level = 0;
  hlp->next = help_free;
  help_free = hlp;
  return;
}


HELP_KEY *
new_help_key (void)
{
  HELP_KEY *newhelpkey;
  
  newhelpkey = (HELP_KEY *) mallok (sizeof (*newhelpkey));
  help_key_count++;
  bzero (newhelpkey, sizeof (*newhelpkey));
  newhelpkey->keyword = nonstr;
  newhelpkey->helpfile = NULL;
  newhelpkey->next = NULL;
  return newhelpkey;
}


void
free_help_key (HELP_KEY *hlpk)
{
  if (!hlpk)
    return;
  
 free_str (hlpk->keyword);
 free2 (hlpk);
 help_key_count--;
 return;
}

/* This finds a helpfile based on a word or phrase entered. */  

HELP *
find_help_name (THING *th, char *arg)
{
  char *t;
  char start_letter, second_letter;
  HELP_KEY *hlpk;

  t = arg;

  if (LC (*t) >= 'a' && LC (*t) <= 'z')
    {
      start_letter = LC (*t) - 'a';
    }
  else
    start_letter = 26;
  t++;
  if (LC (*t) >= 'a' && LC (*t) <= 'z')
    {
      second_letter = LC (*t) - 'a';
    }
  else
    second_letter = 26;
  for (hlpk = help_hash[(int) start_letter][(int) second_letter]; hlpk; hlpk = hlpk->next)
    {
      if (!str_prefix (arg, hlpk->keyword) && hlpk->helpfile && 
          (!th ||
	  hlpk->helpfile->level <= LEVEL (th)))
	{
	  return (hlpk->helpfile);
	}
    }
  return NULL;
}

void
show_help_file (THING *th, HELP *help)
{
  char buf[STD_LEN];
  int len, pad, i;
  if (!th || !help)
    return;
  bigbuf[0] = '\0';
  add_to_bigbuf (HELP_BORDER);
  len = strlen (help->keywords);
  buf[0] = '\0';
  if (len < 77)
    {
      pad = (77 - len)/2;
      for (i = 0; i < pad; i++)
	strcat (buf, " ");
    }
  strcat (buf, "\x1b[1;32m");
  strcat (buf, help->keywords);
  strcat (buf, "\n\r");
  add_to_bigbuf (buf);
  add_to_bigbuf (HELP_BORDER);
  if (LEVEL (th) == MAX_LEVEL && help->admin_text != nonstr)
    {
      add_to_bigbuf ("\n\r                         \x1b[1;32m        ADMIN REF: \x1b[0;37m\n\n\r");
      add_to_bigbuf (help->admin_text);
      add_to_bigbuf ("\n\r");
      add_to_bigbuf (HELP_BORDER);
    }
  add_to_bigbuf ("\n\r");
  add_to_bigbuf (help->text);
  if (help->see_also && *help->see_also)
    {
      add_to_bigbuf ("\n\rSee Also Help\x1b[1;36m ");
      add_to_bigbuf (help->see_also);
      add_to_bigbuf ("\x1b[0;37m\n\n\r");
    }
  add_to_bigbuf (HELP_BORDER);
  send_bigbuf (bigbuf, th);
  return;
}
