#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <stdarg.h>
#include "serv.h"



  /* This strips off the first "word" in a string. Had to add code
   for multiple words in one..like c 'magic missile' blah. */

char *
f_word (char *ol, char *nw)
{
  if (!ol)
    {
      *nw = '\0';
      return nonstr;
    }
  
  while (isspace (*ol))
    ol++;
  if (*ol == '\'')
    {
      ol++;
      while (*ol && *ol != '\'')
	{
	  *nw = LC(*ol);
	  nw++;
	  ol++;
	}
      if (*ol == '\'')
	ol++;
    }
  else if (*ol == '\"')
    {
      ol++;
      while (*ol && *ol != '\"')
	{
	  *nw = LC(*ol);
	  nw++;
	  ol++;
	}
      if (*ol == '\"')
	ol++;
    }
  else
    {
      while (!isspace (*ol) && *ol)
	{
	  *nw = LC(*ol);
	  nw++;
	  ol++;
	}
    }
  *nw = '\0';

  while (isspace (*ol))
    ol++;
  return ol;
}  

/* This returns the next line of text. It assumes that it's not
   more than STD_LEN long. */

char *
f_line (char *ol, char *nw)
{
  if (!ol)
    {
      if (nw)
	*nw = '\0';
      return ol;
    }
  
  while (*ol && *ol != '\n' && *ol != '\r')
    {
      *nw = *ol;
      nw++;
      ol++;
    }
  /* If this isn't the end, go past the \r in the \n\r EOL. */
  if (*ol)
    {
      ol++;
      if (*ol == '\r')
	ol++;
    }
  *nw = '\0';
  return ol;
}


/* This checks if the small string (a) is the same as the beginning of
   the big string (b). If the big string (b) ends too soon, then it
   returns as not the same, but if the small string ends first then
   the strings are considered different. This makes it so case matters.
   This is an optimization (BOO! HISS!) because believe it or not the
   biggest drain on CPU for this game is name parsing. :P */

bool 
str_case_prefix (const char *a, const char *b)
{
  if (!a || !b)
    return TRUE;

  while (*a && *b)
    {
      if (*a != *b)
	return TRUE;
      a++;
      b++;
    }
  if (*a && !*b)
    return TRUE;
  return FALSE;
}
/* This checks if the small string (a) is the same as the beginning of
   the big string (b). If the big string (b) ends too soon, then it
   returns as not the same, but if the small string ends first then
   the strings are considered different. */

bool 
str_prefix (const char *a, const char *b)
{
  if (!a || !b)
    return TRUE;

  while (*a && *b)
    {
      if (LC (*a) != LC (*b))
	return TRUE;
      a++;
      b++;
    }
  while (*a && *b);
  if (*a && !*b)
    return TRUE;
  return FALSE;
}

/* This checks if the end of the string (a)...which must be longer than
   (b) is the same or not. It goes from the back to the front of (a)
   and checks each character vs those in (b). If (b) is shorter or 
   if any character is different, it returns TRUE, else it returns FALSE. */

bool 
str_suffix (char *a, char *b)
{
  char *as = a, *bs = b;
  if (!a || !b || !*b || !*a)
    return TRUE;
  while (*a)
    a++;
  while (*b) 
    b++;
  do
    {
      a--;
      b--;
      if (LC(*b) != LC(*a))
	return TRUE;
    }
  while (b > bs && a > as);  
  if (a > as && b <= bs)
    return TRUE;
  return FALSE;
}

/* I think this is roughly what strcmp does, but it's just my implementation
   where empty strings are always set to not be the same as anything... and 
   it's not case sensitive. (strcasecmp). */


bool
str_cmp (const char *a, const char *b)
{
  if (!a || !*a || !b || !*b)
    return TRUE;
  do
    {
      if (LC (*a) != LC (*b))      
	return TRUE;
      a++;
      b++;
    }
  while (*a && *b);
  if (*a || *b)
    return TRUE;
  return FALSE;
}

/* This function takes a search_in string (big string) and starts
   to strip off words from it until we either find the 
   same word as we are looking for, or until the end of search_in
   at which point we give up and set search_in back to where it
   was. */


bool 
named_in (char *search_in, char *look_for)
{
  char search_in_word[STD_LEN*2], look_for_word[STD_LEN*2];
  if (!search_in || !look_for || !*search_in || !*look_for)
    return FALSE;
  
  /* Strip off the first word in the list of names to look for
     and look for it. */
  
  look_for = f_word (look_for, look_for_word);
 
  do
    {
      search_in = f_word (search_in, search_in_word);
      
      if (!str_case_prefix (look_for_word, search_in_word))
	return TRUE;
    }
  while (*search_in);
  return FALSE;
}

/* This is like named_in, but it requires the ENTIRE word to match,
   not just a str_prefix...*/

bool
named_in_full_word (char *search_in, char *look_for)
{
   char search_in_word[STD_LEN*2], look_for_word[STD_LEN*2];
  if (!search_in || !look_for || !*search_in || !*look_for)
    return FALSE;
  
  /* Strip off the first word in the list of names to look for
     and look for it. */
  
  look_for = f_word (look_for, look_for_word);
 
  do
    {
      search_in = f_word (search_in, search_in_word);
      
      if (!str_cmp (look_for_word, search_in_word))
	return TRUE;
    }
  while (*search_in);
  return FALSE;
}
/* This loops through an entire list instead of just checking for one
   word in the list. It also checks for the full word as a strstr that
   starts at a blank or at the beginning of a word, not as a partial
   string. */

char *
string_found_in (char *search_in, char *look_for)
{ 
  static char look_for_word[STD_LEN*2];
  char *pos;
  
  if (!search_in || !look_for || !*search_in || !*look_for)
    return nonstr;
  
  
  do 
    {
      look_for = f_word (look_for, look_for_word);
      if (*look_for_word &&
	  ((pos = strstr (search_in, look_for_word)) == NULL || 
	   (pos != search_in || isspace (*(pos - 1)))))
	return look_for_word;
    }
  while (*look_for);
  
  return nonstr;
}

/* This checks to see if every word in the list we have is found in the
   string of words to search in. */

bool
named_in_all_words (char *search_in, char *look_for)
{
  char look_for_word[STD_LEN];
  
  if (!search_in || !*search_in)
    return FALSE;
  
  
  while (look_for && *look_for)
    {
      look_for = f_word (look_for, look_for_word);
      
      if (!named_in (search_in, look_for_word))
	return FALSE;
    }
  return TRUE;
}

/* This assumes that the buffer has room for a few more characters
   and that we return most of the same string in place. Since I 
   use big fixed-length buffers a lot, this shouldn't be too 
   much of a problem. */

void
possessive_form (char *txt)
{
  char *t, c;
  
  if (!txt || !*txt)
    return;
  
  /* This takes a phrase and turns it into a possessive form. */
  
  for (t = txt; *t; t++);  
  t--;
  /* Go back past any spaces. */
  
  while (isspace(*t) && t > txt)
    t--;
  c = LC(*t);
  *t = '\0';
  
  /* DO NOT MOVE t BELOW HERE SINCE SEVERAL LINES DOWN
     WE HAVE A *t = c; LINE SO IF YOU MOVE t THAT LINE
     WILL NOT WORK!!! IF YOU NEED TO MOVE t, THEN REPLACE
     THE LINE SEVERAL LINES BELOW WITH A sprintf OR SOMETHING. */
  /* Ending in y o f means not just adding a s' or a '. */
  if (c == 'y')
    strcat (txt, "ies'");
  else if (c == 'o')
    strcat (txt, "oes'");
  /* thief -> thieves, but hippogriff -/> hippogrifves. :P */
  else if (c == 'f' && t > txt && (LC(*(t-1)) != 'f'))
    strcat (txt, "ves'");
  else /* Add the last letter back on and then add the s' or ' */
    {
      *t = c;
      if (c != 's')
	strcat (txt, "'s");	      
      else
	strcat (txt, "'");
    }
  return;
}

/* This should return the plural form of a word. Again since English sucks
   and there are many exceptions, this won't always work and you may
   need to tweak this to get it working better. As before, it is assumed
   that txt has a few extra spaces in it for new letters.*/


void
plural_form (char *txt)
{
  char *t, c;
  
  if (!txt || !*txt)
    return;
  
  for (t = txt; *t; t++);
  t--; /* Move back to the letter before the last ' */      
  c = LC(*t);
  *t = '\0';
  /* DO NOT MOVE t BELOW HERE SINCE SEVERAL LINES DOWN
     WE HAVE A *t = c; LINE SO IF YOU MOVE t THAT LINE
     WILL NOT WORK!!! IF YOU NEED TO MOVE t, THEN REPLACE
     THE LINE SEVERAL LINES BELOW WITH A sprintf OR SOMETHING. */
  
  if (c == 'y')
    strcat (txt, "ies");	      
  else if (c == 'o')
    strcat (txt, "oes");
  else if (c == 'f')
    strcat (txt, "ves");
  else if (c == 'n' && LC(*(t-1)) == 'a' && LC (*(t-2)) == 'm')
    {
      *t = 'n';
      *(t-1) = 'e';
    }
  else
    {
      *t = c;
      if (c != 's')
	strcat (txt, "s");		  
      else
	strcat (txt, "");
    }
  return;
}
  

/* This finds the number of lines of text in a string. */


int
find_num_lines (char *txt)
{
  char *t;
  int lines = 0;
  if (!txt || !*txt)
    return 0;
  for (t = txt; *t; t++)
    if (*t == '\n' || *t == '\r')
      {
	lines++;
	if (*(t + 1) == '\r')
	  t++;
      }
  return lines;
}

/* This finds the number of words within a string of text. */

int
find_num_words (char *txt)
{
  int num_words = 0;
  char arg1[STD_LEN*3], *arg;

  if (!txt || !*txt)
    return 0;
  
  arg = txt;
  
  do
    {
      arg = f_word (arg, arg1);
      
      if (*arg1)
	num_words++;
      
    }
  while (*arg && *arg1);
  return num_words;
}



	  
