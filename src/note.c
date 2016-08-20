#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "note.h"


/* This is really simple code...not designed to be fancy with threads
   and all that, but I might do that later on if I am really bored.
   Ok. Chiron complained so I took out all of the fancy stuff that I 
   wanted and made it *exactly* like it was before, so no more replying
   to notes, no more absolute note numbers :P Bah. */

NOTE *note_free = NULL;
NOTE *note_list = NULL;
int note_count = 0;
int top_note = 0;


void
do_note (THING *th, char *arg)
{
  char buf[STD_LEN];
  char arg1[STD_LEN];
  NOTE *note, *prev;
  bool shown = FALSE;
  int number = 0, count, num_chose;
  bool removed = FALSE;
  buf[0] = '\0';
  arg = f_word (arg, arg1);
  
  if (!IS_PC (th))
    {
      stt ("Only players can access notes!\n\r", th);      
      return;
    }
  if (!str_cmp (arg1, "list"))
    {
      bigbuf[0] = '\0';
      number = 1;
      add_to_bigbuf ("Notes:\n\rNew Num   From        Subject                  To\n\r\x1b[1;35m_______________________________________________________________\x1b[0;37m\n\n\r");
      for (note = note_list; note; note = note->next)
	{
	  if (can_see_note (th, note))
	    {
	      sprintf (buf, "\x1b[1;31m%s \x1b[1;37m%-4d\x1b[0;37m: \x1b[1;34m%-14s\x1b[1;37m%-33s\x1b[0;37m '\x1b[1;32m%s\x1b[0;37m'\n\r", 
		       note->time < th->pc->curr_note ? " " : "\x1b[1;36mN",
		       number++, note->sender,  note->subject,  note->to);
              add_to_bigbuf (buf);
	      shown = TRUE;
	    }
	}
      if (!shown)
	{
	  add_to_bigbuf ("There are no notes.\n\r");
	}
      send_bigbuf (bigbuf, th);
      return;
    }

  if (!str_cmp (arg1, "read"))
    {
      if (is_number (arg))
	num_chose = atoi (arg);
     else
       num_chose = 0;
      number = 1;
      for (note = note_list; note; note = note->next)
	{
	  if (!can_see_note (th, note))
	    continue;
	  if ((num_chose == 0 && (note->time < th->pc->curr_note)) ||
	      (num_chose > 0 && (num_chose != number++)))
	    continue;
	  show_note (th, note);
	  th->pc->curr_note = MAX (th->pc->curr_note, note->time + 1);
	  shown = TRUE;
	  break;
	}
      if (!shown)
	{
	  stt ("You don't have any unread notes.\n\r", th);
	  return;
	}
      return;
    }
  
  else if (!str_cmp (arg1, "subject"))
    {
      get_new_note (th);
      if (*arg)
        {
          stt ("Note subject changed.\n\r", th);
	  th->pc->note->subject = new_str (arg);
        }
      else
	stt ("Note subject <subject string>\n\r", th);
      return;
    }
  else if (!str_cmp (arg1, "to"))
    {
      get_new_note (th);
      if (*arg)
        {
          stt ("Note to list changed.\n\r", th); 
          th->pc->note->to = new_str (arg);
        }
      else
	stt ("Note to <list of names>\n\r", th);
      return;
    }
  else if (!str_cmp (arg1, "write") || !str_cmp (arg1, "word") ||
	   !str_cmp (arg1, "text"))
    {
      get_new_note (th);
      new_stredit (th, &th->pc->note->message);
      return;
    }
  else if (!str_cmp (arg1, "remove"))
    {
      if (is_number (arg))
	number = atoi (arg);
      if (number < 1)
	number = 1;
      count = 1;
  
      if (number == 1 && note_list)
	{
	  note = note_list;
	  note_list = note_list->next;
	  free_note (note);
	  removed = TRUE;
	}
      else
	{
	  for (prev = note_list; prev; prev = prev->next)
	    {
	      if (++count == number && prev->next)
		{
		  note = prev->next;
		  prev->next = prev->next->next;
		  free_note (note);
		  removed = TRUE;
		  break;
		}
	    }
	}
      if (removed)
	{
	  write_notes ();
	  stt ("Note removed.\n\r", th);	  
	}
      else
	{
	  stt ("That note isn't there to remove.\n\r", th);
	}
      return;
    }
  else if (!str_cmp (arg1, "show"))
    {
      if (!th->pc->note)
	{
	  stt ("You aren't writing a note.\n\r", th);
	  return;
	}
      show_note (th, th->pc->note);
      return;
    }
  else if (!str_prefix (arg1, "adminresp") && LEVEL (th) == MAX_LEVEL)
    {
      count = 0;
      number = atoi (arg);
      if (number < 1)
	{
	  stt ("Note adminresp <number to reply to>.\n\r", th);
	  return;
	}
      for (note = note_list; note; note = note->next)
	if (++count == number)
	  break;
      if (note)
	{
	  note->time = current_time;
	  new_stredit (th, &note->adminresp);
	}
      else 
	stt ("Note adminresp <number to reply to>\n\r", th);
      return;
    }
  if (!str_cmp (arg1, "clear"))
    {
      if (!th->pc->note)
        {
          stt ("You aren't writing a note!\n\r", th);
          return;
        }
      free_note (th->pc->note);
      th->pc->note = NULL;
      stt ("Ok, you stop writing your note.\n\r", th);
      return;
    }
  if (!str_cmp (arg1, "send") || !str_cmp (arg1, "post"))
    {
      if (!th->pc->note)
	{
	  stt ("You aren't writing a note!\n\r", th);
	  return;
	}
      if (!th->pc->note->to ||
	  strlen ( th->pc->note->to) < 3)
	{
	  stt ("You must provide a list of recipients! Try note to <somebody>\n\r", th);
	  return;
	}
      if (!th->pc->note->subject ||
	  strlen (th->pc->note->subject) < 1)
	{
	  stt ("You must provide a subject!\n\r", th);
	  return;
	}
      if (!th->pc->note->message ||
	  strlen (th->pc->note->message) < 1)
	{
	  stt ("You must provide a message!\n\r", th);
	  return;
	}
      th->pc->note->time = current_time;
      th->pc->curr_note = current_time + 1;
      /* Add note to the BOTTOM of the list! */
      if (note_list)
	{
	  note = note_list;
	  while (note->next != NULL)
	    note = note->next;
	  note->next = th->pc->note;
	  th->pc->note->next = NULL;
	}
      else
	{
	  note_list = th->pc->note;
	  note_list->next = NULL;
	}
      th->pc->note = NULL;
      sprintf (buf, "Ok note sent.\n\r");
      stt (buf, th);
      write_notes ();
      return;
    }
  stt ("Note: read, write, send, subject, to, list\n\r", th);
  return; 
  
}


void
show_note (THING *th, NOTE *note)
{
  char buf[STD_LEN];
  NOTE *cnote;
  int notecount = 1;
  if (!th || !note)
    return;
  bigbuf[0] = '\0';
  for (cnote = note_list; cnote != NULL; cnote = cnote->next)
    {      
      if (note == cnote)
	break;
      notecount++;
    }
  sprintf (buf, "\n\r\x1b[1;37mNote\x1b[1;34m#\x1b[1;37m%5d\n\r\x1b[1;36m------------------------------------------------------------------------\n\r",
notecount);
  add_to_bigbuf (buf);
  sprintf (buf, "\x1b[1;37m%s\x1b[0;37m\n\r", c_time (note->time));
  add_to_bigbuf (buf);
  sprintf (buf, "From: \x1b[1;32m%-20s\x1b[0;37m     To: \x1b[1;35m%s\x1b[0;37m\n\r",  note->sender,  note->to);
  add_to_bigbuf (buf);
  sprintf (buf, "Subject \x1b[1;37m'\x1b[1;36m%s\x1b[1;37m'\x1b[0;37m\n\r",  note->subject);
  add_to_bigbuf (buf);
  add_to_bigbuf ("\x1b[1;36m------------------------------------------------------------------------\x1b[0;37m\n\r");
  add_to_bigbuf (note->message);
  if (note->adminresp && *note->adminresp)
    {
      add_to_bigbuf ("\n\r");
      add_to_bigbuf (HELP_BORDER);
      add_to_bigbuf ("                                \x1b[1;32mAdmin Response:\n\r");
      add_to_bigbuf (HELP_BORDER);
      add_to_bigbuf ("\x1b[1;37m");
      add_to_bigbuf (note->adminresp);
      add_to_bigbuf (HELP_BORDER);
    }
  send_bigbuf (bigbuf, th);
  return;
}

/* This checks to see if you can see a note or not. */

bool 
can_see_note (THING *th, NOTE *note)
{
  int pos, i;
  char *arg;
  char arg1[STD_LEN];
  CLAN *clan;

  if (!th || !note || !note->to || !*note->to)
    return FALSE;

  if (LEVEL (th) == MAX_LEVEL || !str_cmp (note->sender, th->name))
    return TRUE;
  
  arg = note->to;
  /* Go down the list of names. */
  while ((arg = f_word (arg, arg1)) != NULL && *arg1)
    {
      
      /* Note to all or note to you */
      
      if (!str_cmp (arg1, "all") || !str_cmp (arg1, th->name))
	return TRUE;
      
      /* Builder/imm notes */

      if (LEVEL (th) >= BLD_LEVEL &&
	  (!str_cmp (arg1, "imm") || !str_cmp (arg1, "immortal") ||
	   !str_cmp (arg1, "imms") || !str_cmp (arg1, "immortals") ||
	   !str_cmp (arg1, "builder") || !str_cmp (arg1, "builders")))
	return TRUE;

      /* Note to alignment */

      for (pos = 0; pos < ALIGN_MAX; pos++)
	if (align_info[pos] && align_info[pos]->name &&
	    *align_info[pos]->name && !str_cmp (align_info[pos]->name, arg1) &&
	    !DIFF_ALIGN (th->align, pos))
	  return TRUE;
      
      /* Check if this is a clan note */
      
      for (pos = 0; pos < CLAN_MAX; pos++)
	{
	  if (!str_cmp (arg1, clantypes[pos]) &&
	      (clan = find_clan_in (th, pos, TRUE)) != NULL)
	    {
	      for (i = 0; i < MAX_CLAN_MEMBERS; i++)
		{
		  if (clan->member[i] && *clan->member[i] &&
		      !str_cmp (clan->member[i], th->name))
		    return TRUE;
		}
	    }
	}
    }
  
  return FALSE;
}

void
get_new_note (THING *th)
{
  if (!IS_PC (th))
    return;
  
  if (th->pc->note)
    {
      th->pc->note->time  = current_time;
      return;
    }
  th->pc->note = new_note ();
  th->pc->note->sender = new_str (capitalize (th->name));
  th->pc->note->to = nonstr;
  th->pc->note->subject = nonstr;
  th->pc->note->message = nonstr;
  th->pc->note->adminresp = nonstr;
  th->pc->note->time = current_time;
  return;
}

NOTE *
new_note (void)
{
  NOTE *newnote;
  if (note_free)
    {
      newnote = note_free;
      note_free = note_free->next;
    }
  else
    {
      newnote = (NOTE *) mallok (sizeof (*newnote));
      note_count++;
    }
  bzero (newnote, sizeof (*newnote));
  newnote->sender = nonstr;
  newnote->to = nonstr;
  newnote->subject = nonstr;
  newnote->message = nonstr;
  return newnote;
}


void
free_note (NOTE *note)
{
  if (!note)
    return;
  free_str ( note->sender);
  free_str ( note->to);
  free_str ( note->subject);
  free_str (note->message);
  free_str (note->adminresp);
  note->time = 0;
  note->next = note_free;
  note_free = note;
  return;
}



void
write_notes (void)
{
  FILE *f;
  NOTE *note;
  if ((f = wfopen ("notes.dat", "w")) == NULL)
    return;

  
  for (note = note_list; note; note = note->next)
    {
      fprintf (f, "NOTE\n");
      if (note->sender && *note->sender)
	write_string (f, "Sender", note->sender);
      if (note->message && *note->message)
	write_string (f, "Message", note->message);
      if (note->adminresp && *note->adminresp)
	write_string (f, "Adminresp", note->adminresp);
      if (note->subject && *note->subject)
	write_string (f, "Subject", note->subject);
      if (note->to && *note->to)
	write_string (f, "To", note->to);
      fprintf (f, "Info %d\n END\n\n",
	       note->time);
    }
  fprintf (f, "\nEND_OF_NOTES %d", top_note);
  fclose (f);
  
  return;
}



void
read_notes (void)
{

  NOTE *newnote, *lastnote;
  
  FILE_READ_SETUP("notes.dat");
  
  lastnote = NULL;
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("NOTE")
	{
	  newnote = read_note(f);
	  if (lastnote)
	    {
	      lastnote->next = newnote;
	      newnote->next = NULL;
	    }
	  else
	    {
	      note_list = newnote;
	      newnote->next = NULL;
	    }
	  lastnote = newnote;
	}
      FKEY("END_OF_NOTES")
	{
	  top_note = read_number (f);
	  break;
	}
      FILE_READ_ERRCHECK("notes.dat");
    }
  fclose (f);
  return;
}	
NOTE *
read_note (FILE *f)
{
  NOTE *newnote;
  FILE_READ_SINGLE_SETUP;
  newnote = new_note ();
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("Sender")
	newnote->sender = new_str (read_string (f));
      FKEY("To")
	newnote->to = new_str (read_string (f));
      FKEY("Subject")
	newnote->subject = new_str (read_string (f));
      FKEY("Message")
	newnote->message = new_str (read_string (f));
      FKEY("Adminresp")
	newnote->adminresp = new_str (read_string (f));
      FKEY("Info")
	newnote->time = read_number (f);
      FKEY("END")
	break;
      FILE_READ_ERRCHECK("notes.dat-s");
    }
  return newnote;
}
  
