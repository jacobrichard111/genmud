#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "script.h"

/* This is the script engine. It works by checking triggers when
   things do things, then if a thing is found with a script attached
   to it for the right kind of trigger, then code is added into a 
   queue and things get executed. The engine is pretty simple, but
   I am not interested in softcoding the entire game. Also, the
   idea of variables for characters was taken from I think LegendMUD?
   Someone from there I think posted that they used variables. A good
   idea no matter what. */


CODE *code_hash[CODE_HASH][CODE_HASH];
TRIGGER *trigger_hash[HASH_SIZE];
SCRIPT_EVENT *script_event_free = NULL;
CODE *code_free = NULL;
TRIGGER *trigger_free = NULL;
SCRIPT_EVENT *script_event_list = NULL;
int code_count = 0;
int trigger_count = 0;
int script_event_count = 0;

void
check_triggers (THING *initiator, THING *mover, THING *start_in, THING *end_in, int flags)
{
  THING *th;
  if (flags == 0)
    return;
  if (IS_SET (flags, TRIG_TAKEN))
    check_trigtype (mover, initiator, NULL, NULL, TRIG_TAKEN);
  if (IS_SET (flags, TRIG_MOVED))
    check_trigtype (mover, initiator, NULL, NULL, TRIG_MOVED);
  if (IS_SET (flags, TRIG_MOVING))
    check_trigtype (mover, NULL, NULL, NULL, TRIG_MOVING);
  if (IS_SET (flags, TRIG_GIVEN))
    check_trigtype (end_in, initiator, mover, NULL,  TRIG_GIVEN);
  if (IS_SET (flags, TRIG_DROPPED))
    check_trigtype (initiator, mover, NULL, NULL, TRIG_DROPPED);
  if (IS_SET (flags, TRIG_TAKEN_FROM))
    check_trigtype (start_in, initiator, mover, NULL, TRIG_TAKEN_FROM);
  if (IS_SET (flags, TRIG_ARRIVE))
    check_trigtype (mover, mover, NULL, NULL, TRIG_ARRIVE);
  if (IS_SET (flags, TRIG_GET))
    check_trigtype (end_in, mover, NULL, NULL, TRIG_GET);
  if (start_in && IS_SET (flags, TRIG_LEAVE))
    {
      check_trigtype (start_in, mover, NULL, NULL, TRIG_LEAVE);
      for (th = start_in->cont; th; th = th->next_cont)
	{
	  check_trigtype (th, mover, NULL, NULL, TRIG_LEAVE);
	}
    }
  if (IS_SET (flags, TRIG_ENTER) && end_in)
    {
      check_trigtype (end_in, mover, NULL, NULL, TRIG_ENTER);
      for (th = end_in->cont; th; th = th->next_cont)
	{
	  if (!IS_PC (th) && IS_SET (th->thing_flags, TH_SCRIPT))
	    check_trigtype (th, mover, NULL, NULL, TRIG_ENTER);
	}
    }
  return;
}


void
check_trigtype (THING *runner, THING *starter, THING *obj, char *arg, int flags)
{
  TRIGGER *trig;
  char *s, *t;
  char word[STD_LEN];
  char keyword[STD_LEN];
  bool found = FALSE;
  int vnum;
  
  if (!runner || !flags || !IS_SET (runner->thing_flags, TH_SCRIPT) || 
      IS_PC (runner) || 
      (flags != TRIG_ARRIVE && 
       (runner == starter || (starter && !IS_PC (starter)))))
    return;

  for (trig = trigger_hash[runner->vnum % HASH_SIZE]; trig; trig = trig->next)
    {
      
      if (IS_SET (trig->type, flags) && trig->vnum == runner->vnum &&
	  runner != starter && np() <= trig->pct &&
	  (!IS_SET (trig->flags, TRIG_PC_ONLY) || IS_PC (starter)))
	{
	  found = TRUE;
	  if (IS_SET (flags, TRIG_TELL | TRIG_SAY | TRIG_COMMAND))
	    {
	      found = FALSE;
	      if (!arg || !*arg || !trig->keywords || !*trig->keywords)
		continue;
	      t = arg;

	      while (*t && !found)
		{
		  t = f_word (t, word);
		  s = trig->keywords;
		  while (*s)
		    {
		      s = f_word (s, keyword);
		      if (!str_cmp (word, keyword))
			{
			  found = TRUE;

			  break;
			}
		    }
		}
	    }	  
	  else if (IS_SET (flags, TRIG_GIVEN | TRIG_TAKEN_FROM))
	    {
	      found = FALSE;
	      if (!trig->keywords || !*trig->keywords 
		  || !obj || (vnum = obj->vnum) == 0)
		continue;
	      s = trig->keywords;
	      while (*s)
		{
		  s = f_word (s, keyword);
		  if (atoi (keyword) == vnum)
		    {
		      found = TRUE;
		      break;
		    }
		}
	    }
	  /* Bribe trigger, do something if given money. */
	  else if (IS_SET (flags, TRIG_BRIBE))
	    {
	      if (!trig->keywords || !*trig->keywords || 
		  atoi(trig->keywords) < 1 || !arg || !*arg)
		continue;
	      if (atoi (arg) < atoi (trig->keywords) &&
		  CAN_TALK (runner))
		{
		  char buf[STD_LEN];
		  sprintf (buf, "I need %s coins, %s",
			   trig->keywords, NAME(starter));
		  do_say (runner, buf);
		  continue;
		}
	    }
	  if (found)
	    {
	      run_script (trig, runner, starter, obj);
	    }	  
	}
    }
  return;
}
  
/* This actually sets up and runs a script. */

void
run_script (TRIGGER *trig, THING *runner, THING *starter, THING *obj)
{
  SCRIPT_EVENT *script_event, *evt;
  CODE *code;
  /* We need a thing to run the script AND we need to have a trigger. */
  if (!trig || !runner)
    return;

  /* If a trigger is timed or on a thing's creation or on a thing's death,
     then we don't need an initiator. Otherwise we DO need it, so if
     it is missing, we must then not run the script */
  
  if (!starter &&
      !IS_SET (trig->type, TRIG_CREATION | TRIG_DEATH | TRIG_MOVING) &&
      trig->timer < 1)
    return;
  
  /* Any script where someone is given something or where something has
     something removed from it reqiures an object also. */

  if (IS_SET (trig->type, TRIG_GIVEN | TRIG_TAKEN_FROM))
    {
      if (!obj || 
	  (obj->vnum != 0 && obj->vnum != atoi (trig->keywords)))
	return;
    }
  
  /* Now check if the "code" exists or not. If it doesn't, then just
     return. */
  
  if ((code = find_code_chunk (trig->code)) == NULL)
    return;
  /* Now check to see that the code actually has something in it. */

  if (!code->code || code->code == nonstr)
    return;

  /* We now check to see if this is already running. If it is,
     we may have to either interrupt it, or start it over depending
     on whether the script trigger is set to allow an interrupt or not. */
  
  for (evt = script_event_list; evt; evt = evt->next)
    {
      if (evt->trig == trig)
	{
	  /* IF a timed trigger or a script or we have a runner or
	     starter the same and we cannot interrupt, we don't start
	     a new script. */
	  if ((trig->type == TRIG_TIMED || 
	       (evt->runner == runner ||
		evt->starter == starter)) &&
	      !IS_SET (trig->flags, TRIG_INTERRUPT))
	    return;
	  else
	    {
	      remove_script_event (evt);
	      free_script_event (evt);
	    }
	  break;
	}
    }
  
  /* Now we know that the runner exists, the starter exists if needed,
     and the obj exists if needed. The code we need to run this also
     exists, so we can create an script_event and have this script done. */
  
  script_event = new_script_event ();
  script_event->time = current_time + trig->timer;
  script_event->code = new_str (code->code);
  script_event->start_of_line = script_event->code;
  script_event->trig = trig;
  script_event->runner = runner;
  script_event->starter = starter;
  script_event->called_from = NULL;
  script_event->obj = obj;
  add_script_event (script_event);

  return;
}

/* Adds an script_event to the script_event list. It checks to see if the even time is
   close to the current time or not before it goes.  We do not assume that
   the script_event is NOT in the script_events list.*/

void
add_script_event (SCRIPT_EVENT *script_event)
{
  SCRIPT_EVENT *prev;
  if (!script_event)
    return;

  if (script_event->next || script_event == script_event_list)
    remove_script_event (script_event);
  
  if ((script_event->time - current_time) > 100000 ||
      (current_time  - script_event->time) > 100000)
    {
      log_it ("Error, script_event time is more than 100000 seconds away from the current time. Probably an error. Bailing out.\n");
      free_script_event (script_event);
      return;
    }
  if (!script_event_list || (script_event_list->time > script_event->time))
    {
      script_event->next = script_event_list;
      script_event_list = script_event;
    }
  else
    {
      for (prev = script_event_list; prev != NULL; prev = prev->next)
	{
	  if ((prev->next && (prev->next->time > script_event->time)) ||
	      !prev->next)
	    {
	      script_event->next = prev->next;
	      prev->next = script_event;
	      break;
	    }
	}
    }
  
  return;
}

void
remove_script_event (SCRIPT_EVENT *script_event)
{
  SCRIPT_EVENT *prev;
  
  if (!script_event)
    return;

  if (script_event == script_event_list)
    {
      script_event_list = script_event->next;
    }
  else
    {
      for (prev = script_event_list; prev; prev = prev->next)
	{
	  if (prev->next == script_event)
	    {
	      prev->next = script_event->next;
	      break;
	    }
	}
    }
  script_event->next = NULL;
  return;
}
  
/* As we check script_events, we go down and we attempt to run them.
   If they are interrupted and have a "wait state", then they are
   put back into the script_event list for continued execution at a later
   time. If they are all done for whatever reason, they are removed
   from the list. Note that timed scripts automatically get put
   back into the list at a later time, but their code is updated
   each time so you can see what they do if you are editing them. */


void
check_script_event_list (void)
{
  SCRIPT_EVENT *script_event, *script_event_n, *nscript_event;
  int newtime = 0, i, j;
  /* This line allows for called scripts/returns to happen faster. */
     
  for (j = 0; j < 3; j++)
    {
      for (script_event = script_event_list; script_event != NULL; script_event = script_event_n)
	{
	  script_event_n = script_event->next;
	  if (script_event->time > current_time)
	    break;
	  remove_script_event (script_event);
	  newtime = run_script_event (script_event);
	  
	  if (newtime > 0)
	    {
	      script_event->time = current_time + newtime;
	      add_script_event (script_event);
	      continue;
	    }
	  if (script_event->calling)
	    continue;
	  
	  
	  if ((nscript_event = script_event->called_from) != NULL)
	    {
	      for (i = 0; i < 10; i++)
		{
		  nscript_event->thing[i] = script_event->thing[i];
		  nscript_event->val[i] = script_event->val[i];
		}
	      nscript_event->runner = script_event->runner;
	      nscript_event->obj = script_event->obj;
	      nscript_event->starter = script_event->starter;
	      nscript_event->time = current_time;
	      nscript_event->calling = NULL;
	      add_script_event (nscript_event);
	      free_script_event (script_event);
	    }
	  else
	    free_script_event (script_event);
	}
    }
  return;
}


int
run_script_event (SCRIPT_EVENT *script_event)
{
  CODE *code;
  int value = 0;
  THING *runner = NULL, *runnern, *proto;
  char *csl;

  if (!script_event || !script_event->start_of_line
      || !*script_event->start_of_line)
    return 0;
  
  
  /* We keep running code until we have to stop for some reason.
     If we have more code to run, we just return 0 to the value,
     and that will mean that we need to restart a timed script, or
     we need to just end this all together. */


  if (script_event->trig && 
      script_event->trig->type == TRIG_TIMED && 
      script_event->trig->timer > 0 && 
      script_event->trig->vnum > 0 &&
      (proto = find_thing_num (script_event->trig->vnum)) != NULL)
    {
      csl = script_event->start_of_line;
      for (runner = thing_hash[proto->vnum % HASH_SIZE]; runner != NULL; runner = runnern)
	{
	  runnern = runner->next;
	  script_event->start_of_line = csl;
	  if (runner->vnum == proto->vnum && runner->in && !IS_AREA (runner->in))
	    {
	      script_event->runner = runner;
	      value = run_code (script_event);
	    }
	}
      script_event->runner = NULL;
    }
  else
    value = run_code (script_event);
  
  /* If we are at the end of the script... */

  if (value == SIG_QUIT)
    {
      /* if this is a timed script and the code still exists,
	 we make the code run again */
      if (script_event->trig && script_event->trig->timer > 0 &&
	  (code = find_code_chunk (script_event->trig->code)) != NULL)
	{
	  free_str (script_event->code);
	  script_event->code = new_str (code->code);
	  script_event->start_of_line = script_event->code;
	  script_event->time = current_time + script_event->trig->timer;
	  /* So we return the timer > 0 to get the script running again. */
	  return script_event->trig->timer;
	}
      
      /* If we don't have that repeating script, then this is the 
	 end of the script, so we return 0 and stop it. */
      
      return SIG_QUIT;
    }
  return value; 
}



int
run_code (SCRIPT_EVENT *script_event)
{
  char buf[STD_LEN];
  char *s, *t, *curr;
  int signal = SIG_CONTINUE, lines_done = 0;
  /* We copy a line of code to the buffer. We stop when we get to the
     '\n' or '\r'. */
  
  curr = script_event->start_of_line;
  if (script_event->trig && 
      IS_SET (script_event->trig->flags, TRIG_LEAVE_STOP) &&
      (!script_event->starter || !script_event->runner || 
      (script_event->starter->in != script_event->runner->in &&
       script_event->starter->in != script_event->runner &&
       script_event->starter != script_event->runner->in)))
    return SIG_QUIT;
  while (curr && *curr && signal == SIG_CONTINUE)
    {
      /* Copy one line of code... */
      buf[0] = '\0';
      s = buf;
      for (t = curr; *t && *t != '\n' && *t != '\r'; t++)
	{
	  *s++ = *t;
	}
      
      *s = '\0';
      if (*t == '\n' || *t == '\r')
	{
	  t++;
	  if (*t == '\n' || *t == '\r')
	    t++;
	}
      
      script_event->start_of_line = t;	
      signal = run_code_line (script_event, buf);      
      curr = script_event->start_of_line;
      lines_done++;
      
      if (lines_done > 100)
	{
	  char errbuf[STD_LEN];
	  sprintf (errbuf, "Possible infinite loop in script %s.\n", (script_event->trig ? script_event->trig->name : "noname"));
	  log_it (errbuf);
	  return SIG_QUIT;
	}
    }
  if (!*curr)
    signal = SIG_QUIT;
  return signal;
}

/* Now need run_code_line, replace_symbols, find_start_of_line, is_in_line
   etc... These will takea bit of doing to get right. */


int
run_code_line (SCRIPT_EVENT *script_event, char *buf)
{
  int signal = 0;
  if (!*buf)
    return SIG_QUIT;
  
  if (!str_prefix ("@", buf))
    {
      signal = math_com_line (script_event, buf);
    }
  else if (!str_prefix ("if", buf))
    {
      signal = if_com_line (script_event, buf);
    }
  else if (!str_prefix ("goto", buf))
    {
      signal = goto_com_line (script_event, buf);
    }
  else if (!str_prefix ("make", buf))
    {
      signal = make_com_line (script_event, buf);
    }
  else if (!str_prefix ("nuke", buf))
    {
      signal = nuke_com_line (script_event, buf);
    }
  else if (!str_prefix ("wait", buf))
    {
      signal = wait_com_line (script_event, buf);
    }
  else if (!str_prefix ("end", buf) || !str_prefix ("done", buf))
    signal = SIG_QUIT;
  else if (!str_prefix ("do", buf))
    {
      signal = do_com_line (script_event, buf);
    }
  else if (!str_prefix ("call", buf))
    {
      signal = call_com_line (script_event, buf);
    }
  else if (!str_prefix ("label", buf))
    signal = SIG_CONTINUE;
  else
    {
      char errbuf[STD_LEN];
      sprintf (errbuf, "Bad Line in code %s:  %s\n", 
	       (script_event->trig ? script_event->trig->code : ""), buf);
      log_it (errbuf);
      signal = SIG_CONTINUE;
    }
  return signal;
}



void
remove_from_script_events (THING *th)
{
  SCRIPT_EVENT *script_event, *script_eventn;
  int i;
  if (!th)
    return;


  for (script_event = script_event_list; script_event != NULL; script_event = script_eventn)
    {
      script_eventn = script_event->next;
      if ((script_event->runner == th || script_event->starter == th || script_event->obj == th)
	  && ( !script_event->trig || script_event->trig->type != TRIG_TIMED ||
	       script_event->trig->timer == 0))
	{
	  remove_script_event (script_event);
	  free_script_event (script_event);
	}
      for (i = 0; i < 10; i++)
	{
	  if (script_event->thing[i] == th)
	    script_event->thing[i] = NULL;
	}
    }
  return;
}
	   


/* This checks all triggers to see if they are timed, and if so,
   it checks if there is an script_event which is pointing to that trigger,
   so the trigger is constantly running. If not, we make a new script_event
   and add it to the list. */

void
setup_timed_triggers (void)
{
  TRIGGER *trig;
  SCRIPT_EVENT *script_event;
  int i;
  bool found = FALSE;
  CODE *code;

  for (i = 0; i < HASH_SIZE; i++)
    {
      for (trig = trigger_hash[i]; trig != NULL; trig = trig->next)
	{
	  if (trig->type == TRIG_TIMED && trig->timer > 0 &&
	      trig->code && *trig->code &&
	      (code = find_code_chunk (trig->code)) != NULL)
	    {
	      for (script_event = script_event_list; script_event != NULL; script_event = script_event->next)
		{
		  if (script_event->trig == trig)
		    {
		      found = TRUE;
		      break;
		    }
		}
	      if (!found)
		{
		  script_event = new_script_event ();
		  script_event->time = current_time;
		  script_event->code = new_str (code->code);
		  script_event->start_of_line = script_event->code;
		  script_event->trig = trig;
		  script_event->runner = NULL;
		  script_event->starter = NULL;
		  script_event->called_from = NULL;
		  script_event->obj = NULL;
		  add_script_event (script_event);
		}
	    }
	}
    }
  return;
}
  
