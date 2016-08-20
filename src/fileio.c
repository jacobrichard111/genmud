#include <sys/types.h> 
#include <ctype.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* These things used to be fscanfs, but apparently fscanf is way slow.
   So, I replaced them with similar getc() calls..and checked the
   return values vs EOF and vs digits or END_STRING_CHAR and isspace
   as necessary. There is some checking to make sure you don't have
   buffer overflows, but it doesn't print any error messages or
   anything. */

char *
read_word (FILE *f)
{
  static char buf[STD_LEN * 2], c;
  int pos = 0;
  buf[0] = '\0';
  if (!f)
    return buf;
  while (isspace (c = getc (f))); /* Eat up whitespace */
  while (c != END_STRING_CHAR && c != EOF && !isspace (c) &&
	 pos < STD_LEN - 1)
    {
      buf[pos] = c;
      if (pos++ > STD_LEN)
	break;
      c = getc(f);
    }
  buf[pos] = '\0';
  if (c == END_STRING_CHAR)
    c = getc (f);
  return (buf);
}

/* Read in a number (int) from a file. */

int
read_number (FILE *f)
{
  int val = 0;
  char c;
  bool minus = FALSE;
  if (!f)
    return 0; 
  while (isspace (c = getc (f))); 
  if (!isdigit (c) && c != '-')
    {
      ungetc (c, f);
      return 0;
    }
  if (c == '-')
    {
      minus = TRUE;
      c = getc(f);
    }
  do
    {
      val *= 10;
      val += c - '0';
    }
  while (isdigit (c = getc (f)) && c != EOF);
  if (minus)
    val = -val;
  return val;
}

/* Read a string in from a file...strings end with the character `. */
char *
read_string (FILE *f)
{
  static char buf[STD_LEN * 10];
  char c;
  int pos = 0;
  buf[0] = '\0';
  if (!f) 
    return buf;
  while (isspace (c = getc (f))); /* Eat up whitespace */
  while (c != END_STRING_CHAR && c != EOF && pos < (STD_LEN*10 - 3))
    {
      buf[pos] = c;
      while ((c = getc(f)) == '\r');
      pos++;
    }
  buf[pos] = '\0';
  if (c == END_STRING_CHAR)
    c = getc (f);
  return buf; 
}

/* Write a string out into a world file. */

void
write_string (FILE *f, char *label, char *text)
{
  if (!f || !label || !text)
    return;
  fprintf (f, "%s ", label);
  while (*text)
    {
      if (*text == '\x1b')
	text = convert_color (f, text);
      else
	{
	  putc (*text, f);
	  text++;
	}
    }
  fprintf (f, "%c\n", END_STRING_CHAR);
  
  return;
}

/* Open up a file in the world directory (since the game doesn't run from
   that directory. */

FILE *
wfopen (char *name, char *type)
{
  char filename[STD_LEN];
  FILE *f;

  if (!name  || !*name || !type || !*type)
    {
      log_it ("Bad name or type passed to wfopen.\n");
      return NULL;
    }
  
  sprintf (filename, "%s%s", WLD_DIR, name);
  if ((f = fopen (filename, type)) == NULL)
    {
      char errbuf[STD_LEN];
      sprintf (errbuf, "%s couldn't be opened for %s.\n",
	       name,
	       (LC (*type) == 'r' ? "reading" : 
		(LC (*type) == 'w' ? "writing" : 
		 (LC (*type) == 'a' ? "appending" : "something"))));
      log_it (errbuf);
    }
  return f;
}
