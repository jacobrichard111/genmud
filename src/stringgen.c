#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <stdarg.h>
#include "serv.h"
#include "society.h"
#include "historygen.h"
#include "mobgen.h"



/* This creates a string based on an incoming format string and 
   area(s) where words are supposed to be located to fill in the
   string. */

char *
string_gen (char *txt, int area_vnum)
{
  THING *area;
  static char ret[STRINGGEN_LEN];
  char word[STD_LEN], *pos, *ret_pos, *t, *s;
  char punctuation[STD_LEN];
  char curr_word[STD_LEN];
  bool current_uppercase;
  bool need_a_an_here = FALSE;
  
  /* Must have text */
  if (!txt || !*txt)
    return nonstr;

  /* If no area, just leave the string alone. */
  if ((area = find_thing_num (area_vnum)) == NULL ||
      !IS_AREA (area))
    {
      strncpy (ret, txt, STRINGGEN_LEN-STD_LEN);
      return ret;
    }

  pos = txt;
  *ret = '\0';

  do
    {

      /* Check if we're currently uppercase or not. */
      
      if (*pos == UC(*pos))
	current_uppercase = TRUE;
      else
	current_uppercase = FALSE;
      
      ret_pos = ret + strlen(ret);
      pos = f_word (pos, word);
 
      
      /* Strip off punctuation and save it for later use. */
      
      *punctuation = '\0';
      s = punctuation;
      for (t = word; *t; t++);      
      t--;
      while (!isalnum (*t) && t > word)
	t--;
      t++;
      
      if (t < word + strlen (word))
	{
	  while (*t)
	    {
	      *s++ = *t++;
	      if (t > word)
		*(t-1) = '\0';
	    }
	}
      *s = '\0';
      
      /* Copy the new word into the string...note that find_gen_word
	 returns the original word if no replacement is found... */
      
      if (!str_cmp (word, "a_an"))
	{
	  need_a_an_here = TRUE;
	  continue;
	}
      strcpy (curr_word, find_gen_word (area_vnum, word, NULL));

      /* Generate a word, and if it exists, we then check if 
	 we need an a/an and if we do, we add it in first, then
	 we add the newest word. */
      if (*curr_word)
	{
	  if (need_a_an_here)
	    {
	      strcat (ret, a_an (curr_word));
	      strcat (ret, " ");
	      need_a_an_here = FALSE;
	    }
	  strcat (ret, curr_word);
	}
      else
	{
	  if (need_a_an_here)
	    {
	      strcat (ret, a_an (word));
	      strcat (ret, " ");
	      need_a_an_here = FALSE;
	    }
	  strcat (ret, word);
	}
      if (current_uppercase)
	*ret_pos = UC(*ret_pos);
      strcat (ret, punctuation);
      if (pos && *pos)
	strcat (ret, " ");
      if (strlen(ret) >= STD_LEN*19)
	break;      
    }
  while (pos && *pos);
  
  /* Now format the paragraph and return it. */
  format_string (NULL, ret);
  
  /* Capitalize all I's */

  for (t = ret; *t; t++)
    {
      if (*t == 'i' &&
	  (t == ret || isspace (*(t-1))) &&
	  (!*(t+1) || isspace (*(t+1))))
	*t = 'I';
    }
  /* Remove the last enter from the formatted string. */
  
  t--;
  while ((*t == '\n' || *t == '\r') && t >= ret)
    {
      *t = '\0';
      t--;
    }
  return ret;
}

/*  This finds a descriptive word from the various lists of words
   found in the objectgen area. type tells what kind of wordlist
   we're trying to access, and the word gets placed into buf. 
   The color is the return value that's the word with the color codes
   around it. The color codes will probably be real ANSI codes but
   they could be in the internal $0-F format. */


char *
find_gen_word (int area_vnum, char *typename, char *color)
{
  THING *obj;
  int num_words = 0, num_chose = 0, count;
  static char buf[STD_LEN];
  char word[STD_LEN], *loc;
  if (!typename || !*typename)
    return NULL;
  
  buf[0] = '\0';
  word[0] = '\0';
  

  if (!str_cmp (typename, "area_name"))
    {
      THING *area;
      
      if ((area = find_random_area (AREA_NOLIST | AREA_NOREPOP | AREA_NOSETTLE)) == NULL)
	return "The Wilderness";
      return area->short_desc;
    }
  
  
  if (!str_cmp ("organization_name", typename))
    return find_random_society_name ('n', ORGANIZATION_SOCIGEN_VNUM);
  if (!str_cmp ("ancient_race_name", typename))
    return find_random_society_name ('n', ANCIENT_RACE_SOCIGEN_VNUM);
  else if (!str_cmp ("genned_society_name", typename))
    return find_random_society_name ('n', 1);
  else if (!str_cmp (typename, "list_ancient_race_names"))
    return list_ancient_race_names();
  else if (!str_cmp (typename, "list_controller_deities"))
    return list_controller_deities();
  else if (!str_prefix ("randpop_mob", typename))
    {
      int flags = 0;
      if (strstr (typename, "strong"))
	flags |= RANDMOB_NAME_STRONG;
      if (strstr (typename, "a_an"))
	flags |= RANDMOB_NAME_A_AN;
      if (strstr (typename, "full"))
	flags |= RANDMOB_NAME_FULL;
      if (strstr (typename, "two"))
	flags |= RANDMOB_NAME_TWO_WORDS;
      
      return find_randpop_mob_name (flags);
    }
      
  
  
  if ((obj = find_gen_object (area_vnum, typename)) == NULL)
    return buf;
  
  
  for (count = 0; count < 2; count++)
    {
      loc = obj->desc;     
      do
	{
	  loc = f_word (loc, word);
	  
	  if (*word != '&' && *word)
	    {
	      if (count == 0)
		num_words++;
	      else if (--num_chose < 1)
		break;
	    }  /* *word == '&' so the next thing is a color code. */
	  else if (*word == '&')
	    {  /* Read past the color code. */
	      loc = f_word (loc, word);
	    }
	}
      while (*word || *loc);
      
      if (count == 0)
	{
	  if (num_words < 1)
	    {
	      *word = '\0';
	      break;
	    }
	  else
	    num_chose = nr (1, num_words);
	}
    }
  
  if (!*word)
    return buf;
  
  /* Copy the word into the buffer. */

  strcpy (buf, word);
  /* If the next word was an '&' then get the color. */
  if (*loc == '&' && color)
    {
      loc = f_word (loc, word);
      f_word (loc, color);
    }
  if (area_vnum == AREAGEN_AREA_VNUM)
    buf[0] = UC(buf[0]);
  return buf;
}

/* This finds a generator object with a certain name inside of a 
   certain generator area. */

THING *
find_gen_object (int area_vnum, char *typename)
{
  THING *obj, *area;
  
  if (area_vnum < GENERATOR_NOCREATE_VNUM_MIN ||
      area_vnum >= GENERATOR_NOCREATE_VNUM_MAX ||
      (area = find_thing_num (area_vnum)) == NULL ||
      !IS_AREA (area) || !typename || !*typename)
    return NULL;
  
  
  /* Find the object with the correct name. */
  for (obj = area->cont; obj; obj = obj->next_cont)
    {
      if (!str_cmp (obj->name, typename))
	break;
    }
  
  
  if (!obj || !obj->desc || !*obj->desc)
    return NULL;
  
  return obj;
  
}


/* This lists out the ancient race names. */

char *
list_ancient_race_names (void)
{
  RACE *align;
  int al;
  SOCIETY *soc;
  int num_races = 0, curr = 1, count;
  static char ret[STD_LEN*4];
  *ret = '\0';

  for (count = 0; count < 2; count++)
    {      
      for (al = 0; al < ALIGN_MAX; al++)
	{
	  /* Find the race for each given align...*/
	  if ((align = find_align (NULL, al)) != NULL)
	    {
	      for (soc = society_list; soc; soc = soc->next)
		{
		  if (soc->align == al &&
		      soc->generated_from == ANCIENT_RACE_SOCIGEN_VNUM)
		    break;
		}
	      if (!soc)
		continue;
	      /* If it's the first time through, find the number of races */
	      if (count == 0)
		num_races++;
	      else
		{
		  sprintf (ret + strlen(ret), "the %s %s %s", soc->adj, 
			  align->name, soc->pname);
		  if (curr < num_races - 1)
		    strcat (ret, ", ");
		  else if (curr == num_races -1)
		    strcat (ret, " and ");
		  curr++;
		}
	    }
	}
    }
  if (!*ret)
    return "Old Races";
  return ret;
}
  
/* This lists out the controller deities. */

char *
list_controller_deities (void)
{
  RACE *align;
  int al;
  int num_gods = 0, curr = 1, count;
  static char ret[STD_LEN*4];
  *ret = '\0';

  for (count = 0; count < 2; count++)
    {      
      for (al = 0; al < ALIGN_MAX; al++)
	{
	  /* Find the race for each given align...*/
	  if ((align = find_align (NULL, al)) == NULL ||
	      !old_gods[al][0] || !*old_gods[al][0])
	    continue;
	  
	  if (count == 0)
	    num_gods++;
	  else
	    {
	      sprintf (ret + strlen(ret), "%s the %s %s of %s", 
		      old_gods[al][0],
		      align->name,
		      (nr (1,2) == 1 ? "god" : "deity"),
		      old_gods_spheres[al][0]);
		  if (curr < num_gods -1)
		    strcat (ret, ", ");
		  else if (curr == num_gods - 1)
		    strcat (ret, " and ");
		  curr++;
	    }
	}
    }
  return ret;
}

