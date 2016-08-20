#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "script.h"


/* This appends a new string onto whatever string the person is
   editing at the moment. The string .d ends the edit. There are
   other options making this a crappy but usable line editor. */


void
append_string (THING *th)
{
  char buf[20000];
  char *t, *s;
  bool ending = FALSE;
  if (!IS_PC (th) || !th->pc->curr_string || !th->fd ||
      !th->fd->command_buffer[0])
    return;
  if (!th->fd)
    {
      th->pc->curr_string = NULL;
      th->pc->editing = NULL;
      return;
    }
  /* Set up the buffer. */
  
  if (*th->pc->curr_string)
    strcpy (buf, *th->pc->curr_string);
  else
    buf[0] = '\0';
  t = buf + strlen (buf);
  s = th->fd->command_buffer;
  
  if (*s == '.')
    {
      s++;
      switch (UC(*s))
	{
	  case 'F':
	    if (!IS_SET (th->pc->pcflags, PCF_EDIT_CODE) &&
		*th->pc->curr_string)
	      format_string (th->pc->curr_string, NULL);
	    else
	      stt ("You cannot format code.\n\r", th);	  
	    return;
	  case 'P':
	    if (!IS_SET (th->pc->pcflags, PCF_EDIT_CODE) &&
		*th->pc->curr_string)
	      paragraph_format (th->pc->curr_string);
	    else
	      stt ("You cannot format code.\n\r", th);	
	    return;
	  case 'D':
	    ending = TRUE;
	    break;
	  case 'S':
	    show_stredit (th, *th->pc->curr_string, FALSE);
	    return;
	  case 'R': /* Replace */
	    {
	      char oldstr[STD_LEN];
	      char newstr[STD_LEN];
	      s++;
	      s = f_word (s, oldstr);
	      s = f_word (s, newstr);
	      if (!*oldstr)
		
		{
		  stt (".r <old> <new>\n\r", th);
		  return;
		}
	      
	      /* The idea is we take the curr_string and search it for
		 any instances of oldstr...and if we don't find it at
		 a given character, we just keep copying the string.
		 But, if we do find a copy of oldstr, it gets replaced
		 by newstr (WHICH MAY BE BLANK!!!!), and then we continue
		 on our way. */
            
	      s = buf;
	      for (t = *th->pc->curr_string; *t;)
		{
		  if (LC (*t) != LC (*oldstr) || str_prefix (oldstr, t))
		    {
		      *s = *t;
		      t++;
		      s++;
		    }
		  else
		    {
		      t += strlen (oldstr);
		      *s = '\0';
		      strcat (s, newstr);
		      s += strlen (newstr);
		    }
		  if (s - buf > 19000)
		    break;
		}
	      *s = '\0';
	      if (!bad_pc_description (th, buf))
		{
		  free_str (*th->pc->curr_string);
		  *th->pc->curr_string = new_str (add_color(buf));
		  stt ("String replaced.\n\r", th);
		  show_stredit (th, *th->pc->curr_string, FALSE);
		}
	      return;
	    }
	    break;	  
	  case 'C':
	    if (atoi (s + 1) > 0)
	      {
		clear_line (th, atoi (s + 1));
		stt ("Ok, line cleared.\n\r", th);
	      }
	    else
	      {
		free_str (*th->pc->curr_string);
		*th->pc->curr_string = nonstr;
		stt ("Ok, string cleared.\n\r", th);
	      }
	    return;
	  case 'I':
	    { /* Insert get number of lines to skip */
	      char arg1[STD_LEN];
	      char *r;
	      int num_lines = 1, curr= 1;
	      s++;
	      s = f_word (s, arg1);          
	      if (!is_number (arg1))
		s = th->fd->command_buffer + 2;
	      else
		num_lines = atoi (arg1);
	      t = buf; 
	      *t = '\0';
	      r = *th->pc->curr_string;
	      for (; *r && curr < num_lines; r++, t++)
		{
		  *t = *r;
		  if (*r == '\n')
		    curr++;
		}
	      *t = '\0';
	      strcat (buf, s);
	      strcat (buf, "\n");
	      strcat (buf, r);
	      if (!bad_pc_description (th, buf))
		{
		  free_str (*th->pc->curr_string); 
		  *th->pc->curr_string = new_str (add_color (buf));	       
		  stt ("String inserted.\n\r", th);
		  show_stredit (th, *th->pc->curr_string, FALSE);
		}         
	    }
	    return;
	  default:
	    break;
	}
    }
  
  
  
  if (!ending)
    {
      for (; *s; t++, s++)
	{
	  if ((t - buf > 19500))
	    {
	      ending = TRUE;
	      break;
	    }
	  *t = *s;
	}
    }
  *t = '\0';
  if (!ending)
    strcat (buf, "\n");
  
  /* Now we just delete the old string, and we make a new string. */

 
  if (!bad_pc_description (th, buf))    
    {
      free_str (*th->pc->curr_string);
      *th->pc->curr_string = new_str (add_color(buf));
    }
  
  /* If we stop editing, then we don't keep inside of the editor. */
  
  if (ending)
    {
      RBIT (th->pc->pcflags, PCF_EDIT_CODE);
      th->pc->curr_string = NULL;
    }

  
  return;
}


void
show_stredit (THING* th, char *txt, bool show_inst)
{
  
  int line = 0;
  char buf[STD_LEN];
  bool new_line;
  char *t = txt, *s;
  if (!th || !txt)
    return;
  if (show_inst)
    {
      stt ("-------------------------------------------------------------------\n\r", th);
      stt ("  At the start of a line, type .d to end editing, .f to format the \n\r", th);
      stt ("  string, .c to clear the string; .c <num> clears line <num>,\n\r", th); 
      stt ("  .p will format the string as a paragraph,\n\r", th);
      stt ("  .r <old> <new> to substitute text, .i <num> <text> inserts <text>\n\r", th);
      stt ("  at line <num>, and .s to show the whole string again. \n\r", th);
      stt ("-------------------------------------------------------------------\n\r", th);
    }
  while (*t)
    {
      line++;
      s = buf;
      sprintf (buf, "%d> ", line);
      new_line = FALSE;
      while (*s)
	s++;
      while (*t && !new_line)
	{
	  if (*t == '\n' || *t == '\r')
	    {
	      *s = *t;
	      s++;
	      t++;
	      if (*t == '\n' || *t == '\r')
		{
		  *s = *t;
		  s++;
		  t++;
		}
	      *s = '\0';
	      stt (buf, th);
	      new_line = TRUE;
	    }
	  else
	    {
	      *s = *t; 
	      s++;
	      if (!*t) 
		return;
	      t++;
	    }
	}
    }
  return;
}


/* This puts you into the editor you use to edit long strings. Note
   that the second argument is THE ADDRESS OF THE ADDRESS OF A STRING!!!
   WE NEED TO PUT THE ACTUAL LOCATION OF THE POINTER VALUE INTO THIS SO
   WE CAN FIND IT AGAIN...SINCE WE WILL NEED TO NUKE THIS STRING
   AND REPLACE IT... BE CAREFUL WITH THIS!!!*/


void
new_stredit (THING *th, char **txt)
{
  /* If any of this is missing, we just return. */


  if (!th->fd || !th->pc || th->pc->curr_string)
    return;
  
  th->pc->curr_string = txt;
  show_stredit (th, *txt, TRUE);
  
  return;
}

/* This takes a string the thing is editing and it fixes it up some. */

/* If the first argument is used, the text is copied and then the
   old buffer is deleted and a new one is made. Otherwise, it
   copies the data back into the original buffer. */

void
format_string (char **text, char *fixed_buffer)
{
  char oldbuf[20000];
  static char newbuf[20000];
  char *t, *s, *last_space, *curr, *new_last_space, *txt;
  int len;
  
  if (text)
    txt = *text;
  else
    txt = fixed_buffer;
  
  if (!txt || !txt[0])
    return;
  s = oldbuf;
  *s = '\0';
  for (t = txt; *t; t++)
    {
      if (*t == ' ' || *t == '\n' || *t == '\r')
	{
	  if (s > oldbuf && *(s - 1) != ' ')
	    *s++ = ' ';
	  continue;
	}
      else
	*s++ = *t;
    }
  *s = '\0';
  s = newbuf;
  curr = oldbuf;
  *s = '\0';
  
  while (*curr)
    {
      last_space = NULL;
      new_last_space = NULL;
      len = 0;
      while (isspace (*curr) && *curr)
	curr++;
      
      for (t = curr; *t; t++, s++)
	{
	  if (*t == '\x1b')
	    {
	      do
		*s++ = *t++;
	      while (*t && *t != 'm');
	      *s = *t;
	    }
	  else
	    {
	      *s = *t;
	      len++;
	    }
	  if (isspace (*t))
	    {
	      last_space = t;
	      new_last_space = s;
	    }

	  if (len >= 75)
	    {
	      if (last_space)
		{
		  curr = last_space;
		  *new_last_space++ = '\n';
                  *new_last_space = '\r';
		  s = new_last_space + 1;
 
		  last_space = NULL;
		  new_last_space = NULL;
		  len = 0;
		}
	      else
		{
		  s++;
		  *s++ = '\n';
                  *s = '\r';
		  curr = t;
		}
	      len = 0;
	      break;
	    }
	}
      if (!*t)
	break;	  
    }
  *s = '\0';
  strcat (newbuf, "\n\r");
  
  if (text)
    {
      free_str (txt);    
      *text = new_str (newbuf);
    }
  else
    strcpy (txt, newbuf);  
  return;
}



void
clear_line (THING *th, int line_number)
{
  int i;
  char buf[20000];
  char *r, *s, *t;

  if (!th || !th->pc || !th->pc->curr_string)
    return;
  
  t = *th->pc->curr_string;
  
  /* Find the beginning of the line_numberth line. */

  strcpy (buf, t);
  
  t = buf;
  
  for (i = 1; i < line_number; i++)
    {      
      for (; *t; t++)
	{
	  if (*t == '\n' || *t == '\r')
	    {
	      t++;
	      if (*t == '\r')
		t++;
	      break;
	    }
	}
    }
  
  /* If the string does not have that many lines, just return. */
  
  if (!*t)
    return;
  
  
  /* Find the end of that line either as a '\0' or an '\n' or '\r'. */
  
  for (r = t; *r; r++)
    {
      if (*r == '\n' || *r == '\r')
	{
	  r++;
	  if (*r == '\r')
	    r++;
	  break;
	}
    }
  
  /* So now, t should be the beginning of the line to cut, r should
     be the end, and we cut the line out of the string. */

  
  
  s = t;
  for (; *r; r++)
    {
      *s = *r;
      s++;
    }
  *s = '\0';
  free_str (*th->pc->curr_string);
  *th->pc->curr_string = new_str (buf);
  show_stredit (th, *th->pc->curr_string, FALSE);
  return;
}



/* Be careful..if you don't use this function to add to the bigbuf,
   bad things might happen. Oh well. This is like this since the
   bigbuf is 80k. Say you do something with 2000 strcats of length
   40, then you are talking a multiple of 40*2000*2000 = 80,000,000
   operations. That's just nuts, so we have a pointer to turn the
   O(N^{2}) operation into an O(N) operation. */

void
add_to_bigbuf (char *txt)
{
  if (!txt || !*txt)
    return;
  if (bigbuf[0] == '\0')
    bigbuf_length = 0;
  bigbuf_length += strlen (txt);
  if (bigbuf_length >= BIGBUF)
    {
      bigbuf_length = strlen (bigbuf) + strlen (txt);
      if (bigbuf_length >= BIGBUF)
	return;
    }
  strcat (bigbuf, txt);
  return;
}

/* Copies the buffer to a char * and then starts sending n-1 lines
   where n is the pagelength of the person. */

void
send_bigbuf (char *txt, THING *th)
{
  if (!th || !txt)
    return;
  if (th->fd)
    th->fd->command_buffer[0] = '\0';
  if (!th->pc || th->pc->big_string)
    return;
  th->pc->big_string = new_str (txt);
  th->pc->big_string_loc = th->pc->big_string;
  send_page (th);
  return;
}

/* Sends a page of text to the character */

void
send_page (THING *th)
{
  int lines, max_lines;
  char tempbuf[STD_LEN * 10];
  char *t, *s, *last_newline;

  /* Make sure the string exists and the person has a network 
     connection. */

  if (!th->pc || !th->pc->big_string || !th->pc->big_string_loc)
    return;
  
  if (!th->fd || !*th->pc->big_string || !*th->pc->big_string_loc)
    {
      free_str (th->pc->big_string);
      th->pc->big_string = NULL;
      th->pc->big_string_loc = NULL;
      return;
    }
  
  /* Quit out. */

  if (th->fd->command_buffer[0] != '\0')
    {
      free_str (th->pc->big_string);
      th->pc->big_string = NULL;
      th->pc->big_string_loc = NULL;
      return;
    }
  
  
  t = th->pc->big_string_loc;
  tempbuf[0] = '\0';
  s = tempbuf;
  
  max_lines = MAX (5, th->pc->pagelen - 1 -
		   (IS_PC2_SET (th, PC2_MAPPING) ? SMAP_MAXY : 0));
  lines = 1;
  last_newline = t;
  for (; *t && (s - tempbuf < STD_LEN * 10); t++)
    {
      *s = *t;
      s++;
      if (*t == '\n')
	{
	  if (++lines >= max_lines)
	    break;
	  if (t - th->pc->big_string_loc > STD_LEN * 10 - STD_LEN/2)
	    break;
	}
    }
  *s = '\0';
  
  th->pc->big_string_loc = t;
  
  if (*t)
    strcat (tempbuf, "(Press <Enter> to continue, and any other key to quit.)\n\r");
  
  stt (tempbuf, th);
  if (!*t)
    {
      free_str (th->pc->big_string);
      th->pc->big_string = NULL;
      th->pc->big_string_loc = NULL;
      return;
    }
  return;
}

/* This takes a string and edits it into paragraphs with indentations
   and double enters marking spaces between paragraphs. */

void
paragraph_format (char **text)
{
  char oldbuf[20000];
  static char newbuf[20000];
  char *t, *s, *last_space, *curr, *new_last_space, *txt;
  int len = 0;

  if (!text)
    return;
  txt = *text;
  if (!txt || !text[0])
    return;
  s = oldbuf;
  *s = '\0';
  for (t = txt; *t; t++)
    {
      if (*t == ' ' || *t == '\n' || *t == '\r')
	{
	  if (s > oldbuf && *(s - 1) != ' ')
	    *s++ = ' ';
	  continue;
	}
      else
	*s++ = *t;
    }
  *s = '\0';
  s = newbuf;
  curr = oldbuf;
  *s = '\0';
  
  if (*curr)
    {
      int i;
      for (i = 0; i < 5; i++)
	*s++ = ' ';
      len = 5;
    }
  while (*curr)
    {
      last_space = NULL;
      new_last_space = NULL;
      if (curr != oldbuf)
	len = 0;
      while (isspace (*curr) && *curr)
	curr++;
      
      for (t = curr; *t; t++, s++)
	{
	  if (*t == '\x1b')
	    {
	      do
		*s++ = *t++;
	      while (*t && *t != 'm');
	      *s = *t;
	    }
	  else
	    {
	      *s = *t;
	      len++;
	    }
	  if (isspace (*t))
	    {
	      last_space = t;
	      new_last_space = s;
	    }

	  if (len >= 75)
	    {
	      if (last_space)
		{
		  curr = last_space;
		  *new_last_space++ = '\n';
                  *new_last_space = '\r';
		  s = new_last_space + 1;
 
		  last_space = NULL;
		  new_last_space = NULL;
		  len = 0;
		}
	      else
		{
		  s++;
		  *s++ = '\n';
                  *s = '\r';
		  curr = t;
		}
	      len = 0;
	      break;
	    }
	}
      if (!*t)
	break;	  
    }
  *s = '\0';
  
  free_str (txt);
  strcat (newbuf, "\n\r");
  *text = new_str (newbuf);
  return;
}

/* This tells if a player has a bad description or not. A bad description
   is one that has too many characters or too many lines in it. */

bool
bad_pc_description (THING *th, char *buf)
{
  /* Bail out on these descriptions since they're ok... */
  if (!th || !buf || !*buf || !IS_PC (th) || LEVEL (th) >= BLD_LEVEL)
    return FALSE;

  /* If this isn't the description of the player, then bail out. */

  if (&th->desc != th->pc->curr_string)
    return FALSE;

  if (strlen (buf) < 2000 && find_num_lines (buf) < 20)
    return FALSE;
  
  /* At this point the buf is >= 3000 chars and >= 20 lines so
     declare it to be a bad description and bail out. */
  
  stt ("This description is too long!!!!! New text ignored!\n\r", th);
  return TRUE;
}
     
