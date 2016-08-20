#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "script.h"


/* The idea is to get a number, then a symbol, then a number and so
   forth until we are all done with this. */

/* The 'n' means that if the total is smaller than what follows the n,
   the total becomes the 'n'. The 'x' means that if the total is 
   bigger than what follows the 'x', then we make it the same as the 'x'. */

int
math_parse (char *txt, int val)
{
  bool num_now = TRUE;
  int total = 0;
  char *t, op = '=';
  int curr_parse_number = 0;
  if (!txt || !*txt)
    return 0;
  
  
  for (t = txt; *t;)
    {
      while (isspace (*t))
	t++;
      if (!num_now)
	{
	  if (is_op (*t))
	    op = *t;
	  else
	    op = '=';
	  num_now = TRUE;
	  t++;
	}
      else
	{
	  curr_parse_number = 0;
	  t = get_parse_number (t, val, &curr_parse_number);
	  total = do_operation (total, op, curr_parse_number);
	  while (isspace (*t))
	    t++;
	  num_now = FALSE;
	}
    }
  return total;
}

/* This scans the string for a single number. */

char *
get_parse_number (char *txt, int val, int *curr_parse_number)
{
  char *s, *t;
  bool minus = FALSE;
  char newbuf[STD_LEN];
  int num_parens = 0;
  t = txt;
  while (isspace (*t))
    t++;
  if (*t == '-')
    {
      minus = TRUE;
      t++;
    }
  if (*t == '(')
    {
      num_parens = 1;
      s = newbuf;
      t++;
      while (*t && num_parens > 0)
	{
	  if (*t == ')' && --num_parens < 1)
	    {
	      t++;
	      continue;
	    }
	  else if (*t == '(')
	    num_parens++;
	  *s = *t;
	  s++;
	  t++;
	}
      *s = '\0';
      *curr_parse_number = math_parse (newbuf, val);
      return t;
    }
  if (LC (*t) == 'r')
    {
      int leftnum = 0, rightnum = 0;
      t++;
      while (isspace (*t) && *t)
	t++;
      if (*t != '(')
	{
	  *curr_parse_number = 0;
	  t++;
	  return t;
	}
      s = newbuf;
      for (; *t && *t != ','; s++, t++)
	*s = *t;
      *s = '\0';
      leftnum = math_parse (newbuf, val);
      if (*t != ',')
	{
	  t++;
	  *curr_parse_number = 0;
	  return t;
	}
      t++;
      s = newbuf;
      num_parens = 1;
      while (*t && num_parens > 0)
	{
	  if (*t == ')' && --num_parens < 1)
	    {
	      t++;
	      continue;
	    }
	  else if (*t == '(')
	    num_parens++;
	  *s = *t;
	  s++;
	  t++;
	}
      *s = '\0';
      rightnum = math_parse (newbuf, val);
      *curr_parse_number = nr (leftnum, rightnum);
      return t;
    }
  if (LC (*t) == 'l')
    {
      *curr_parse_number = val;
      t++;
      return t;
    }
  if (*t == '-')
    {
      minus = TRUE;
      t++;
    }
  if (isdigit (*t))
    {
      while (isdigit (*t) && *curr_parse_number < 1000000000)
	{
	  *curr_parse_number *= 10;
	  *curr_parse_number += *t - '0';
	  t++;
	}
    }
  else
    t++;
  if (minus)
    *curr_parse_number = -*curr_parse_number;
  return t;
}


/* This replaces strings with information about things..it works a
   lot like the parser for scripts, but it doesn't require the thing
   to be specified. It does use rep_one_value, though. */

char *
string_parse (THING *th, char *txt)
{
  static char buf[30000];
  char colorbuf[20];
  char oldbuf[STD_LEN];
  char repbuf[STD_LEN];
  char newbuf[STD_LEN];
  int place = PLACE_RIGHT;
  char *r, *s, *t;
  int num = -1, i, space, pad;
  

  if (!txt || !*txt || !th)
    return nonstr;
  
  

  s = buf;
  for (t = txt; *t; t++)
    {
      if (*t == '$')
	{
	  t++;
	  num = -1;
	  if (*t == '$')
	    {
	      *s = '$';
	      s++;
	    }
	  else if (isdigit (*t))
	    num = *t - '0';
	  else if (LC (*t) >= 'a' && LC (*t) <= 'f')
	    num = LC(*t) -'a' + 10;
	  if (num != -1)
	    {
	      sprintf (colorbuf, "\x1b[%d;3%dm", num/8, num % 8);
	      sprintf (s, colorbuf);
	      s += strlen (colorbuf);
	    }
	}
      else if (*t == '@')
	{
	  t++;
	  r = oldbuf;
	  pad = 0;
	  place = PLACE_RIGHT;
	  for (; *t && *t != '@'; r++, t++)
	    *r = *t;
	  *r = '\0';
	  r = oldbuf;
	  while (isspace (*r) && *r)
	    r++;
	  if (*r == '-')
	    {
	      place = PLACE_LEFT;
	      r++;
	    }
	  else if (*r == '!')
	    {
	      place = PLACE_CENTER;
	      r++;
	    }
	  space = 0;
	  while (isdigit (*r) && space < 100)
	    {
	      space *= 10;
	      space += *r - '0';
	      r++;
	    }
	  if (space > 20)
	    space = 20;
	  strcpy (repbuf, replace_one_value (NULL, r, th, FALSE, 0, nonstr));
	  pad = space - strlen (repbuf);
	  r = newbuf;
	  
	  if (pad > 0)
	    {
	      if (place == PLACE_RIGHT)
		for (i = 0; i < pad; i++, r++)
		  *r = ' ';
	      if (place == PLACE_CENTER)
		for (i = 0; i < pad/2 + (pad % 2); i++, r++)
		  *r = ' ';
	    }
	  sprintf (r, repbuf);
	  r += strlen (repbuf);
	  if (pad > 0 && place == PLACE_LEFT)
	    for (i = 0; i < pad; i++, r++)
	      *r = ' ';
	  if (pad > 1 && place == PLACE_CENTER)
	    for (i = 0; i < pad/2; i++, r++)
	      *r = ' ';
	  *r = '\0';
	  strcpy (s, newbuf);
	  s += strlen (newbuf);
	}
      else
	{
	  *s++ = *t;
          if (*t == '\n' && *(t + 1) != '\r')
            {
              *s++ = '\r';
            }
	}
    }
  *s = '\0';
  return buf;
}
	      


void
do_checkparse (THING *th, char *arg)
{
  int i;
  char buf[STD_LEN];

  if (!*arg)
    {
      stt ("checkparse <string>\n\r", th);
      return;
    }
  
  for (i = 0; i < 10; i++)
    {
      sprintf (buf, "The value is %d\n\r", math_parse (arg, LEVEL (th)));
      stt (buf, th);
    }
  return;
}
	       
  
void
do_chs (THING *th, char *arg)
{
  if (!*arg)
    {
      stt ("chs <string to be checked>\n\r", th);
      return;
    }
  
  stt (string_parse (th, arg), th);
  return;
}
