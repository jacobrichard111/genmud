#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serv.h"


/* This is very very primitive color adding code. It basically works 
   so that if you put down a '$', it moves to the next char, and if the
   next char is a '$', it prints a single '$'. If it is '0'-'9' or
   'A' - 'F', it treats that character as a hexadecimal digit and then
   lets num equal that hex number. It then adds "\x1b[(num / 8);3(num % 8)m"
   to the string in place of the '$#'. This doesn't work with flash or
   inverse or underline, but I am sure if you really want that stuff
   you can add it in as a special case or two. Note that '$7' is 
   considered to be "normal text" */


char *
add_color (char *txt)
{
  static char ret[20000];
  char *s = ret, *t = txt;
  int num = 0;
  *s = '\0';
  if (!txt || !*txt)
    return ret;
  for (; *t; t++, s++)
    {
      if (*t != '$')
	{
	  *s = *t;
	  continue;
	}
      t++;
      if (*t == '\0')
	break;
      if (*t == '$')
	{	
	  *s = '$'; 
	  continue;
	}
      if (*t >= '0' && *t <= '9')
	num = *t - '0';
      else if ((LC(*t) >= 'a' && LC(*t) <= 'f'))
	num = LC(*t) - 'a' + 10;
      if (num != -1)
	{
	  *s = '\x1b';
	  *(s + 1) = '[';
	  *(s + 2) = ((num / 8) + '0');
	  *(s + 3) = ';';
	  *(s + 4) = '3';
	  *(s + 5) = ((num % 8) + '0');
	  *(s + 6) = 'm';
	  s += 6;
	}
      else
	{
	  t++;
	}
    }
  *s = '\0';
  return ret;
}

/* This will take a string and remove actual ansi color codes from it
   and will also remove color codes of the form '$_'. */

char *
remove_color (char *txt)
{
  static char ret[5000];
  char *s, *t;
  ret[0] = '\0';
  if (!txt)
    return ret;
  t = txt;
  s = ret;
  for (; *t; t++)
    {
      /* Remove the actual ansi codes if needed. */

      if (*t == '\x1b')
	{
	  while (*t && *t != 'm')
	    t++;
	}
      else if (*t == '$')
	{
	  t++;
	  if (*t == '$')
	    {
	      *s = '$';
	      s++;
	    }
	}
      else 
	{
	  *s = *t;
	  s++;
	}
    }
  *s = '\0';
  return ret;
}
      
/* This takes a text string that starts with an escape character and
   converts the color code following the text to the internal coloring
   used in the game so messy control chars aren't in save files. */

char *
convert_color (FILE *f, char *t) 
{
  int num = 0;
  if (!f || !t || *t != '\x1b')
    return t;
  
  t++;
  if (*t != '[')
    return t;
  
  t++;
  
  if (*t == '0' || *t == '1')
    {
      num += 8*(*t - '0');
      t++;
      if (*t != ';')
	return t;
      t++;
    }

  if (*t != '3')
    return t;

  t++;
  if (*t >= '0' && *t <= '7')
    num += *t - '0';
  else
    return t;
  
  t++;
  if (*t != 'm')
    return t;
  
  t++;
  if (num < 0 || num > 15)
    return t;
  
  fprintf (f, "$%c", (num < 10 ? (num + '0') :
		      (num - 10 + 'A')));
  return t;
}
  
/* Makes an int into a single hex digit. Capped at 0 and F. */

char 
int_to_hex_digit (int num)
{
  if (num < 0)
    return '0';
  else if (num > 15)
    return 'F';
  
  if (num <= 9)
    return '0' + num;
  
  return 'A' + num - 10;
}
