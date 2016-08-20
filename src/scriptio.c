#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "script.h"


/* These first six functions create and free the various structs needed
   for scripts. */

SCRIPT_EVENT *
new_script_event (void)
{
  SCRIPT_EVENT *newscript_event;
  
  if (script_event_free)
    {
      newscript_event = script_event_free;
      script_event_free = script_event_free->next;
    }
  else
    {
      newscript_event = (SCRIPT_EVENT *) mallok (sizeof (*newscript_event));
      script_event_count++;
    }
  bzero (newscript_event, sizeof (*newscript_event));
  newscript_event->code = nonstr;
  newscript_event->start_of_line = nonstr;
  return newscript_event;
}
  

void
free_script_event (SCRIPT_EVENT *script_event)
{
  if (!script_event)
    return;
  if (script_event->code)
    free_str (script_event->code);
  bzero (script_event, sizeof (*script_event));
  script_event->code = nonstr;
  script_event->start_of_line = nonstr;
  script_event->next = script_event_free;
  script_event_free = script_event;
  return;
}


CODE *
new_code (void)
{
  CODE *newcode;
  if (code_free)
    {
      newcode = code_free;
      code_free = code_free->next;
    }
  else
    {
      newcode = (CODE *) mallok (sizeof (*newcode));
      code_count++;
    }
  bzero (newcode, sizeof (*newcode));
  newcode->name = nonstr;
  newcode->code = nonstr;
  newcode->flags = 0;
  return newcode;

}
  
void 
free_code (CODE *code)
{
  if (!code)
    return;
  free_str (code->name);
  free_str (code->code);
  bzero (code, sizeof (*code));
  code->name = nonstr;
  code->code = nonstr;
  code->next = code_free;
  code_free = code;
  return;
}


TRIGGER *
new_trigger (void)
{
  TRIGGER *newtrigger;
  
  if (trigger_free)
    {
      newtrigger = trigger_free;
      trigger_free = trigger_free->next;
    }
  else
    {
      newtrigger = (TRIGGER *) mallok (sizeof (*newtrigger));
      trigger_count++;
    }
  bzero (newtrigger, sizeof (*newtrigger));
  newtrigger->name = nonstr;
  newtrigger->code = nonstr;
  newtrigger->keywords = nonstr;
  newtrigger->pct = 100;
  return newtrigger;
}

void
free_trigger (TRIGGER *trigger)
{
  free_str (trigger->name);
  free_str (trigger->code);
  free_str (trigger->keywords);
  bzero (trigger, sizeof (*trigger));
  trigger->name = nonstr;
  trigger->code = nonstr;
  trigger->keywords = nonstr;
  trigger->next = trigger_free;
  trigger_free = trigger;
  return;
}
  
void
read_triggers (void)
{

  FILE_READ_SETUP("trigger.dat");
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("TRIGGER")
	read_trigger (f);
      FKEY("END_OF_TRIGGERS")
	break;
      FILE_READ_ERRCHECK("trigger.dat");
    }
  fclose (f);
  return;
}
  


void
read_trigger (FILE *f)
{
  TRIGGER *newtrig;
  FILE_READ_SINGLE_SETUP;
  if (!f)
    return;
  newtrig = new_trigger ();

  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("Name")
	newtrig->name = new_str (read_string (f));
      FKEY("Keywords")
	newtrig->keywords = new_str (read_string (f));
      FKEY("Code")	
	newtrig->code = new_str (read_string (f));
      FKEY("Gen")
	{
	  newtrig->type = read_number (f);
	  newtrig->timer = read_number (f);
	  newtrig->flags = read_number (f);
	  newtrig->vnum = read_number (f);
	}
      FKEY("Pct")
	newtrig->pct = read_number (f);
      FKEY("END_TRIGGER")
	{
	  newtrig->next = trigger_hash[newtrig->vnum % HASH_SIZE];
	  trigger_hash[newtrig->vnum % HASH_SIZE] = newtrig;
	  return;
	}
      FILE_READ_ERRCHECK("trigger.dat-single");
    }
  return;
}
	      
void
write_triggers (void)
{
  FILE *f;
  TRIGGER *trig;
  int i;
  
  if ((f = wfopen ("trigger.dat", "w")) == NULL)
    return;
  
  for (i = 0; i < HASH_SIZE; i++)
    {
      for (trig = trigger_hash[i]; trig; trig = trig->next)
	{
	  if (!IS_SET (trig->flags, TRIG_NUKE))
	    {
	      write_trigger (f, trig);
	    }
	}
    }
  fprintf (f, "\nEND_OF_TRIGGERS\n");
  fclose (f);
  return;
}
 
void
write_trigger (FILE *f, TRIGGER *trig)
{
  if (!f || !trig)
    return;
  fprintf (f, "TRIGGER\n");
  if (trig->name && trig->name[0])
    write_string (f, "Name", trig->name);
  fprintf (f, "Gen %d %d %d %d\n", trig->type, trig->timer,
	   trig->flags, trig->vnum);
  fprintf (f, "Pct %d\n", trig->pct);
  if (trig->keywords && trig->keywords[0])
    write_string (f, "Keywords", trig->keywords);
  if (trig->code && trig->code[0])
    write_string (f, "Code", trig->code);
  fprintf (f, "END_TRIGGER\n");
  return;
}
 
  
void
read_codes (void)
{
  CODE *code;
  FILE_READ_SETUP ("code.dat");
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("CODE")
	{
	  if ((code = read_code(f)) != NULL)
	    add_code_to_table (code);
	}
      FKEY("END_OF_CODES")
	break;
      FILE_READ_ERRCHECK("code.dat");
    }
  fclose (f);
  return;
}
  

CODE *
read_code (FILE *f)
{
  CODE *code;  
  if (!f)
    return NULL;
  code = new_code ();
  
  code->name = new_str (read_string (f));
  code->code = new_str (add_color(read_string (f)));
  code->flags = read_number (f);
  return code;
}

/* This adds a piece of code to the hash table. */

void
add_code_to_table (CODE *code)
{
  int num1 = CODE_HASH -1, num2 = CODE_HASH -1;
  char *t;
  if (!code)
    return;
  
  for (t = code->name; *t; t++)
    *t = LC (*t);
  t = code->name;
  if (*t >= 'a' && *t <= 'z')
    num1 = *t - 'a';
  if (*(t + 1) >= 'a' && *(t + 1) <= 'z')
    num2 = *(t + 1)  - 'a';
  
  code->next = code_hash[num1][num2];
  code_hash[num1][num2] = code;
  return;
}

void
write_codes (void)
{
  FILE *f;
  CODE *code;
  int i, j;
  if ((f = wfopen ("code.dat", "w")) == NULL)
    return;


  for (i = 0; i < CODE_HASH; i++)
    {
      for (j = 0; j < CODE_HASH; j++)
	{
	  for (code = code_hash[i][j]; code; code = code->next)
	    {
	      write_code (f, code);
	    }
	}
    }
  fprintf (f, "\nEND_OF_CODES\n");
  fclose (f);
  return;
}

void
write_code (FILE *f, CODE *code)
{
  if (!f || !code || IS_SET (code->flags, CODE_NUKE))
    return;
  write_string (f, "CODE", code->name);
  write_string (f, "", code->code);
  fprintf (f, "%d\n\n", code->flags);
  return;
}

/**********************************************************************/

/*       END OF new_, free_ and file I/O for script things!!!!        */

/**********************************************************************/

