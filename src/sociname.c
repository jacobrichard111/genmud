#include <stdio.h>
#include<ctype.h>
#include<stdlib.h>
#include <string.h>
#include "serv.h"
#include "society.h"

/* These are weighted by how often I think these letters should 
   show up in words....*/
const char vowels[NUM_VOWELS+1] = "aaeeeiioou";
static char *consonants = 
"bbbcccccccdddfffffggggghhhhhhhhhjjkkkkllllllllllmmmmmmmmmnnnnnnnnnnnpppppppppqrrrrrrrrrssssssssssttttttttttttvwwwwwwxyyz";

/* These are consonants that must be near a vowel, not a consonant due
   to being too hard to pronounce otherwise. */

char *
create_society_name (VALUE *socval)
{
  SOCIETY *soc;
  int num_syllables;
  char syll[STD_LEN], *t, c = '\0', *s;
  int i, count;
  bool added_spacer = FALSE; /* Tells if we added a - or ' to space out the
				name or not. If not, we don't go to
				a fourth syllable. */
  bool did_first_consonant = FALSE; /* Did we put the first consonant
				       into the name? */
/* Can we have a consonant here or not -- based on whether the previous
   letter must be followed by a vowel or not. */
  bool can_have_consonant_here = TRUE; 
  static char namebuf[STD_LEN];
  /* This array is used to look at the word as a whole and see if
     any cuss words are in it. If they are, we don't allow the name
     to be made. */
  
  char nospacerbuf[STD_LEN];
  static bool done_once = FALSE;
  static int consonant_length = 0;
 
  /* This tells if a letter is allowed to appear at this point. 
     This is for letters that must be surrounded by vowels. */
  bool letter_ok = FALSE;
   if (!done_once)
    {
      consonant_length = strlen (consonants);
      done_once = TRUE;
    }
  namebuf[0] = '\0';
  /* Socval is NOT strictly necessary */
  
  if (socval && socval->type != VAL_SOCIETY)
    return namebuf;
  if (socval && 
      ((soc = find_society_num (socval->val[0])) == NULL ||
       IS_SET (soc->society_flags, SOCIETY_NONAMES)))
    return namebuf;
  
  num_syllables = 1 + nr (0,1) + nr (0,1);
  
  if (num_syllables == 1 && nr (1,6) != 2)
    num_syllables++;
  else if (nr (0,15) == 3)
    num_syllables++;
  
  set_value_word (socval, "");
  
  namebuf[0] = '\0';
  nospacerbuf[0] = '\0';
  for (i = 0; i < num_syllables; i++)
    {      
      did_first_consonant = FALSE;
      t = syll;
      *t = '\0';
      for (count = 0; count < 2; count++)
	{	  
	  can_have_consonant_here = TRUE;
	  if (*nospacerbuf && !*syll)
	    {
	      for (s = nospacerbuf; *s; s++);
	      s--;
	      if (must_be_near_vowel(*s))
		can_have_consonant_here = FALSE;
	      
	      /* If this is the first letter in a syllable and the
		 previous two letters are consonants, this one cannot
		 also be a consonant. */
	      if (!isvowel(*s) && s > nospacerbuf &&
		  !isvowel(*(s-1)))
		can_have_consonant_here = FALSE;
	    }
	  
	  
	  
	  /* Almost always do a consonant on the first iteration,
	     then on the second usually do it, but always do it
	     if there wasn't a first consonant. */
	  if (((count == 0 && nr (1,3) != 2) ||
	       (count == 1 && (nr (1,3) != 3 || !did_first_consonant))) &&
	      can_have_consonant_here)
	    {
	      do
		{
		  c = consonants[nr(0,consonant_length -1)];
		  letter_ok = TRUE;
		  /* If this is the first part of a syllable make
		     sure that the starting letter can go after the
		     previous syllabue (certain letters must
		     come after vowels only... These are also rarer
		     within words so, quite often, they're just
		     discarded to try to find a "normal" letter. */
		  if (must_be_near_vowel(c))
		    {
		      /* Many times this letter is discarded to repick. */
		      if (nr (1,10) != 2)
			letter_ok = FALSE;
		      /* Make sure it's after a vowel. */
		      if (!*syll)
			{		      
			  for (s = nospacerbuf; *s; s++);
			  s--;
			  if (!isvowel(*s))
			    letter_ok = FALSE;
			}
		    }
		}
	      while (!letter_ok);
	      

	      /* Sometimes certain common letters have common following
		 letters added onto them. */

	      if (nr (1,10) == 3)
		{
		  if (c == 'l')
		    {
		      switch (nr (1,4))
			{
			  case 1:
			    *t++ = 'c';
			    break;
			  case 2:
			    *t++ = 'b';
			    break;
			  case 3:
			    *t++ = 'f';
			    break;
			  case 4:
			    *t++ = 'g';
			    break;
			}
		      *t++ = 'l';
		    }
		  else if (c == 'r')
		    {
		      switch (nr (1,8))
			{
			  case 1:
			    *t++ = 'b';
			    break;
			  case 2:
			    *t++ = 'c';
			    break;
			  case 3:
			    *t++ = 'd';
			    break; 
			  case 4:
			    *t++ = 'f';
			    break;
			  case 5:
			    *t++ = 'g';
			    break;
			  case 6:
			    *t++ = 'k';
			    break;
			  case 7:
			    *t++ = 'p';
			    break;
			  case 8:
			    *t++ = 't';
			    break;
			}
		      *t++ = 'r';
		    }
		  else if (c == 't')
		    {
		      if (nr (1,2) == 1)
			{
			  *t++ = 's';
			  *t++ = 't';
			}
		      else
			{
			  *t++ = 't';
			  *t++ = 'h';
			}
		    }
		  else if (c == 'k' && nr (1,4) == 2)
		    {
		      *t++ = 'h';
		    }
		  else		    
		    *t++ = c;
		}
	      else
		*t++ = c;
	      if (count == 0)
		did_first_consonant = TRUE;
	    }
	  if (c == 'q')
	    *t++ = 'u';
	  else if (count == 0)
	    *t++ = vowels[nr(0, NUM_VOWELS-1)];
	}
      
      *t = '\0';
      
      strcat (namebuf, syll);
      strcat (nospacerbuf, syll);
      /* Possibly add ' and - to the name...*/
      if (i < num_syllables - 1)
	{
	  if (nr (0,50) == 14)
	    {
	      strcat (namebuf, "'");
	      added_spacer = TRUE;
	    }
	  else if (nr (0,50) == 9)
	    {
	      strcat (namebuf, "-");
	      added_spacer = TRUE;
	    }
	}
      if (!added_spacer && i >= 2)
	break;
    }
  if (namebuf[strlen(namebuf)-1] == 't' &&
      nr (1,2) == 1)
    {
      strcat (namebuf, "h");
      strcat (nospacerbuf, "h");
    }
  
  /* Could be dangerous...no 2 letter names anymore. */
  if (strlen (namebuf) < 3)
    strcpy (namebuf, create_society_name (socval));
	    
  if (!contains_bad_word (nospacerbuf))
    {
      namebuf[0] = UC(namebuf[0]);
      set_value_word (socval, namebuf);
    }
  else /* This is kind of dangerous, but technically the number of
	  bad words is so small, that it shouldn't make a difference
	  in practice. */
    create_society_name (socval);
  
  for (t = namebuf; *t; t++);
  t--;
  if (*t == 'f' && nr (1,5) == 3)
    {
      *(t+1) = 't';
      *(t+2) = '\0';
    }
  return namebuf;
}

/* This sets up the name and short desc for a society member who just got
   created. */

void
setup_society_name (THING *th)
{
  VALUE *socval;
  SOCIETY *soc;
  char namebuf[STD_LEN];
  char buf2[STD_LEN], *t;
  char buf3[STD_LEN];
  
  if (!th)
    return;

  if ((socval = FNV (th, VAL_SOCIETY)) == NULL ||
      !socval->word || !*socval->word || !th->proto ||
      !th->proto->name || !*th->proto->name ||
      (soc = find_society_num (socval->val[0])) == NULL)
    return;
  
  /* So the socval exists. Set up the name.  */
  
  sprintf (namebuf, "%s %s", th->proto->name, socval->word);
  if (soc->adj && *soc->adj && 
      soc->generated_from != ORGANIZATION_SOCIGEN_VNUM )    
    sprintf (namebuf + strlen(namebuf), " %s", soc->adj);
  
  free_str (th->name);
  th->name = new_str (namebuf);
  
  
  /* Set up the short desc. */
  
  strcpy (buf2, th->proto->short_desc);
  buf2[0] = LC(buf2[0]);
  /* Move past the a or an or whatever word comes first in the short desc..*/
  
  t = f_word (buf2, buf3);
  strcpy (namebuf, socval->word);
  strcat (namebuf, " the ");
  /* Add adjectives if necessary! */
  if (soc->adj && *soc->adj 
      && soc->generated_from != ORGANIZATION_SOCIGEN_VNUM )
    {
      strcat (namebuf, soc->adj);
      strcat (namebuf, " ");
    }
  strcat (namebuf, t);
  free_str (th->short_desc);
  th->short_desc = new_str (namebuf);
  for (t = th->short_desc + 1; *t; t++)
    *t = LC(*t);
  
  /* Set up the long desc. */
  
  strcpy (buf2, th->proto->long_desc);
  buf2[0] = LC(buf2[0]);
  /* Move past the a or an or whatever word comes first in the long desc..*/
  
  t = f_word (buf2, buf3);
  strcpy (namebuf, socval->word);
  strcat (namebuf, " the ");
  /* Add adjectives if necessary! */
  if (soc->adj && *soc->adj && soc->generated_from != ORGANIZATION_SOCIGEN_VNUM)
    {
      strcat (namebuf, soc->adj);      
      strcat (namebuf, " ");
    }
  strcat (namebuf, t);
  free_str (th->long_desc);
  th->long_desc = new_str (namebuf); 
  for (t = th->long_desc + 1; *t; t++)
    *t = LC(*t);
  
  return;
}
  

/* This sets up the name/short/long desc for a society member
   who's a crafter or shopkeeper. */

void
setup_society_maker_name (THING *th)
{
  VALUE *socval;
  SOCIETY *soc;
  char namebuf[STD_LEN];
  char jobname[STD_LEN];
  char adjbuf[STD_LEN];
  if (!th)
    return;

  if ((socval = FNV (th, VAL_SOCIETY)) == NULL ||
      !socval->word || !*socval->word || !th->proto ||
      !th->proto->name || !*th->proto->name ||
      (soc = find_society_num (socval->val[0])) == NULL)
    return;
  
  if (IS_SET (socval->val[2], CASTE_CRAFTER))
    {
      if (socval->val[3] >= 0 && socval->val[3] < PROCESS_MAX)
	strcpy (jobname, process_data[socval->val[3]].worker_name);
      else
	strcpy (jobname, "crafter");
    }
  else if (IS_SET (socval->val[2], CASTE_SHOPKEEPER))
    {
      if (socval->val[3] >= 0 && socval->val[3] < SOCIETY_SHOP_MAX)
	strcpy (jobname,  society_shop_data[socval->val[3]].name);
      else
	strcpy (jobname, "shopkeeper");
    }
  
  if (soc->adj && *soc->adj)
    sprintf (adjbuf, "%s ", soc->adj);
  else
    adjbuf[0] = '\0';
  
  /* So the socval exists. Set up the name. */

  sprintf (namebuf, "%s %s %s %s",
	   th->proto->name,
	   socval->word,
	   jobname,
	   adjbuf);
  free_str (th->name);
  th->name = new_str (namebuf);
  
  
  /* Set up the short desc. */
  
  sprintf (namebuf, "%s the %s%s %s",
	   socval->word, adjbuf, soc->name, jobname);
  free_str (th->short_desc);
  th->short_desc = new_str (namebuf);
  
  strcat (namebuf, " is here.");
  free_str (th->long_desc);
  th->long_desc = new_str (namebuf);
  
  return;
}
  
