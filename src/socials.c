#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* This socials code is waaaay simple since I don't feel like 
   making an RPI MUD. If you feel the need to have all those messages
   for different situations...go for it. You r0XX0r. */


SOCIAL *
new_social (void)
{
  SOCIAL *nsoc;
  
  if (social_free)
    {
      nsoc = social_free;
      social_free = social_free->next;
    }
  else
    {
      nsoc = (SOCIAL *) mallok (sizeof (*nsoc));
      social_count++;
    }
  
  nsoc->next = NULL;
  nsoc->next_list = NULL;
  nsoc->name = nonstr;
  nsoc->no_vict = nonstr;
  nsoc->vict = nonstr;
  return nsoc;
}

/* This frees a social for reuse. */

void
free_social (SOCIAL *social)
{
  if (!social)
    return;
  
  free_str (social->vict);
  free_str (social->name);
  free_str (social->no_vict);
  social->next = NULL;
  social->next_list = NULL;
  social->name = nonstr;
  social->no_vict = nonstr;
  social->vict = nonstr;
  social->next = social_free;
  social_free = social;
  return;
}

/* This reads socials in from the data file. */

void
read_socials (void)
{
  FILE *f;
  char word[STD_LEN];
  SOCIAL *soc;
  if ((f = wfopen ("socials.dat", "r")) == NULL)
    return;
  
  while (TRUE)
    {
      strcpy (word, read_word (f));
      
      if (!str_cmp (word, "END_OF_SOCIALS") || !isalnum(word[0]))
	break;
      soc = new_social ();
      soc->name = new_str (word);
      soc->no_vict = new_str (read_string (f));
      soc->vict = new_str (read_string (f));
      soc->next_list = social_list;
      social_list = soc;
      soc->next  = social_hash[LC (*soc->name)];
      social_hash[LC (*soc->name)] = soc;
    }
  fclose (f);
  return;
}

/* This clears all of the socials out of the social table and puts
   them into the free list for reuse. */


void
clear_socials (void)
{
  SOCIAL *social, *socialn;
  int i;
  for (social = social_list; social; social = socialn)
    {
      socialn = social->next_list;
      free_social (social);
    }
  social_list = NULL;
  
  for (i = 0; i < 256; i++)
    social_hash[i] = NULL;
}

void
do_random_social (THING *th)
{
  int choice, count = 0, num_choices = 0, num_chose = 0;
  char buf[STD_LEN];
  char name[STD_LEN];
  SOCIAL *soc;
  THING *vict;
  
  if (IS_PC (th) || !CAN_TALK (th) || !th->in || !CAN_MOVE (th))
    return;
  
  choice = nr (1, social_count);
  
  for (soc = social_list; soc; soc = soc->next_list)
    {
      if (++count == choice)
	break;
    }
  
  strcpy (buf, soc->name);
  if (nr (1,5) != 2)
    {
      for (count = 0; count < 2; count++)
	{
	  for (vict = th->in->cont; vict; vict = vict->next_cont)
	    {
	      if (CAN_MOVE (vict) && CAN_TALK (vict) && vict != th &&
		  !is_enemy (vict, th) && !is_enemy (th, vict))
		{
		  if (count == 0)
		    num_choices++;
		  else if (--num_chose < 1)
		    break;
		}
	    }
	  if (count == 0)
	    {
	      if (num_choices < 1)
		return;
	      num_chose = nr (1, num_choices);
	    }
	}
      if (vict && th != vict)
	{
	  strcat (buf,  " ");
	  f_word ((char *) KEY (vict), name);
	  strcat (buf, name);
	}
    }
  
  if (soc)
    found_social (th, buf);
  return;
}
  


bool
found_social (THING *th, char *arg)
{
  THING *vict = NULL;
  char arg1[STD_LEN];
  char *oldarg = arg;
  if (th->position < POSITION_FIGHTING)
    return FALSE;
  arg = f_word (arg, arg1);
  
  if (*arg)
    {
      vict = find_thing_in (th, arg);
    }
  else
    vict = NULL;
  
  return do_social_to (th, vict, oldarg);
  
}


bool
do_social_to (THING *th, THING *vict, char *arg)
{
  SOCIAL *soc;
  char socname[STD_LEN];
  if (!th || !arg || !*arg)
    return FALSE;
  
  if (!IS_PC (th) && th == vict)
    return FALSE;
  
  arg = f_word (arg, socname);
  
  for (soc = social_hash[LC (*socname)]; soc != NULL; soc = soc->next)
    {
      if (!str_prefix (socname, soc->name))
	{
	  if (!vict || (!CAN_FIGHT (vict) && !CAN_MOVE (vict)))
	    { 
	      /* No vict and no extra word in the social name means 
		 do the no vict social. Make sure the victim doesn't
		 have the intitiator ignored! */
	      if (!*arg && !ignore (vict, th))
		act (soc->no_vict, th, NULL, NULL, NULL, TO_ALL);
	      else /* Otherwise we wanted to find a victim but couldn't */
		stt ("They aren't here.\n\r", th);
	      return TRUE;
	    }
	  else
	    act (soc->vict, th, NULL, vict, NULL, TO_ALL);
	  return TRUE;
	}
    }
  return FALSE;  
}

void
do_socials (THING *th, char *arg)
{
  int i, count = 0;
  char buf[STD_LEN];
  SOCIAL *soc;
  
  bigbuf[0] = '\0';
  buf[0] = '\0';
  
  if (IS_PC (th) && LEVEL (th) == MAX_LEVEL &&
      !str_cmp (arg, "reload"))
    {
      clear_socials();
      read_socials();
      stt ("Socials reloaded.\n\r", th);
      return;
    }

  for (i = 0; i < 256; i++)
    {
      for (soc = social_hash[i]; soc != NULL; soc = soc->next)
	{
	  sprintf (buf + strlen (buf), "%-13s", soc->name);
	  if (++count == 6)
	    {
	      strcat (buf, "\n\r");
	      add_to_bigbuf (buf);
	      count = 0;
	      buf[0] = '\0';
	    }
	}
    }
  if (buf[0])
    {
      strcat (buf, "\n\r");
      add_to_bigbuf (buf);
    }
  send_bigbuf (bigbuf, th);
  return;
}
