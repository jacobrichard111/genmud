#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "serv.h"
#include "script.h"
#include "worldgen.h"
#include "objectgen.h"
#include "questgen.h"
#include "questgenold.h"
#include "areagen.h"

static JOB *job_free = NULL;  /* Free list of jobs. */
static char qf_name[STD_LEN]; /* The quest flag name being used. */
static int qf_val = 0;        /* What value it is in the quest. */
static int qf_stage = 1;
CODE *quest_code_list = NULL;
static char quest_name[STD_LEN];

/* This generates all of the quests. */

void
generate_quests (void)
{
  JOB *quest_start;
  strcpy (qf_name, "aa");
  strcpy (quest_name, "aaaa");
  qf_stage = 1;
  qf_inc_bits ();
  quest_start = generate_job (NULL);
}

/* Increment the current flag being used. */

void
qf_inc_bits (void)
{
  qf_val++;
  if (qf_val < 0 || qf_val >= NUM_VALS)
    {
      qf_val = 0;
      questgen_name_iterate (qf_name);
    }
  return;
}

/* This creates a new job. */

JOB *
new_job (void)
{
  JOB *job;
  int i;
  if (job_free)
    {
      job = job_free;
      job_free = job_free->next;
    }
  else
    {
      job = (JOB *) mallok (sizeof (JOB));
    }
  
  job->next = NULL;
  job->runner = 0;
  job->runner_room = 0;
  job->reward_type = 0;
  job->reward_num = 0;
  job->reward_password[0] = '\0';
  
  for (i = 0; i < JOB_PREV_MAX; i++)
    {
      job->job_type[i] = JOB_NONE;
      job->job_num[i] = 0;
      job->job_password[i][0] = '\0';
    }
  
  job->qf_num = -1;
  job->qf_name[0] = '\0';
  return job;
}


/* This frees all jobs from the quest tree. */

void
free_job (JOB *job)
{
  int i;

  if (!job)
    return;
  
  if (job->next)
    {
      for (i = 0; i < JOB_PREV_MAX; i++)
	{
	  if (job->next->prev[i] == job)
	    job->next->prev[i] = NULL;
	}
      free_job (job->next);
      job->next = NULL;
    }
  
  for (i = 0; i < JOB_PREV_MAX; i++)
    {
      if (job->prev[i])
	{
	  if (job->prev[i]->next == job)
	    job->prev[i]->next = NULL;
	  free_job (job->prev[i]);
	  job->prev[i] = NULL;
	}
    }
  job->next = job_free;
  job_free = job;
  return;
}

/* This reads the quest code chunks in from the data file. */

void
read_quest_chunk_types (void)
{
  CODE *code;
  FILE_READ_SETUP ("questcode.dat");
  
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("CODE")
	{
	  if ((code = read_code(f)) != NULL)
	    {
	      code->next = quest_code_list;
	      quest_code_list = code;
	    }
	}
      FKEY ("END_OF_CODES")
	break;
      FILE_READ_ERRCHECK ("questcode.dat");
    }
  fclose (f);
  return;
}


CODE *
find_questgen_code (char *name)
{
  CODE *code;
  if (!name || !*name)
    return NULL;
  
  for (code = quest_code_list; code; code = code->next)
    {
      if (!str_cmp (code->name, name))
	return code;
    }
  return NULL;
}
	  
	  
void
setup_quest_trigger (char *name, int vnum, int type, char *keywords)
{
  TRIGGER *trig;
  THING *mob;
  
  if (!name || !*name || vnum == 0 || (mob = find_thing_num (vnum)) == NULL ||
      type < 0 || type >= TRIG_MAX)
    return;
  

  trig = new_trigger();
  trig->vnum = vnum;
  trig->next = trigger_hash[trig->vnum % HASH_SIZE];
  trigger_hash[trig->vnum % HASH_SIZE] = trig;
  trig->name = new_str (name);
  trig->code = new_str (name);
  trig->type = type;
  trig->flags = TRIG_LEAVE_STOP | TRIG_INTERRUPT | TRIG_PC_ONLY;
  return;
}


char *
questgen_init_string (int type)
{
  switch (type)
    {
      case JOB_GIVE_ITEM:
	return "#give_item_quest\n";
	break;
      case JOB_GIVE_MONEY:
	return "#give_money_quest\n";
	break;
      case JOB_GIVE_PASSWORD:
	return "#give_password_quest\n";
	break;
      case JOB_KILL_THING:
	return "#give_kill_quest\n";
	break;
      default:
	log_it ("Bad questgen type given!\n");
	return "";
    }
  return "";
}

/* This sets up a quest. */

JOB *
generate_job (JOB *next_job)
{
  int num_prev_jobs = 0, i, jobnum, jobtype;
  JOB *job;
  int count, times, trigtype;
  /* Things used in the quest. */  

  /* Things from prerequisites. */
  THING *mob[JOB_PREV_MAX];
  THING *room[JOB_PREV_MAX];
  THING *area[JOB_PREV_MAX];
  THING *obj[JOB_PREV_MAX];
  char password[JOB_PREV_MAX][100];
  
  THING *person = NULL, *this_area = NULL;
  
  CODE *code = NULL;
  TRIGGER *trig = NULL;
  /* Code buffers. */
  char tempbuf[STD_LEN*10];
  char currbuf[STD_LEN*10];
  char word[STD_LEN];
  
  /* Did we change code or not. */
  bool changed_code = FALSE;

  /* If we don't find good objects for previous steps, we just bail 
     out. */
  
  bool bad_things_found = FALSE;
  /* Need a place where we'll do this quest. */
  
  
  /* Find the area where this quest takes place. */


  if ((this_area = find_quest_area()) == NULL)
    return NULL;
  
  
  /* Need a person who's the focus of this quest. */
  
  if ((person = find_quest_thing (this_area, NULL, QUEST_THING_PERSON)) == NULL)
    return NULL;
  
  for (i = 0; i < JOB_PREV_MAX; i++)
    {
      obj[i] = NULL;
      mob[i] = NULL;
      area[i] = NULL;
      room[i] = NULL;
    }
  
  
  if ((job = new_job()) == NULL)
    return NULL;
  
  job->runner = person->vnum;

  if (next_job)
    job->next = next_job;
  
  /* Now add previous jobs to this. */
  
  if (nr (1,7) != 3)
    num_prev_jobs = 0;
  else if (nr (1,6) != 6)
    num_prev_jobs = 1;
  else if (nr (1,5) != 3)
    num_prev_jobs = 2;
  else
    num_prev_jobs = 3;
  

  /* Create those jobs and set them up, too. */
  
  for (i = 0; i < num_prev_jobs; i++)
    job->prev[i] = generate_job (job);
  
  
  /* At this point, all prev jobs should be set up. */

 
  /* Get the job nums/pwds based on the prev jobs. */

 
  for (i = 0; i < JOB_PREV_MAX; i++)
    {
      if (!job->prev[i])
	continue;
      
      switch (job->prev[i]->reward_type)
	{
	  case QUEST_REWARD_ITEM:
	    job->job_type[i] = JOB_GIVE_ITEM;
	    job->job_num[i] = job->prev[i]->reward_num;
	    break;
	  case QUEST_REWARD_MONEY:
	    job->job_type[i] = JOB_GIVE_MONEY;
	    job->job_num[i] = job->prev[i]->reward_num;
	    break;
	  case QUEST_REWARD_PASSWORD:
	    job->job_type[i] = JOB_GIVE_PASSWORD;
	    strcpy (job->job_password[i], job->prev[i]->reward_password);
	    break;
	  default:
	    log_it ("Bad reward given to generate job!\n");
	    break;
	}
    }

  
  /* If there's no previous job, pick a random quest to do. */
  
  if (!job->prev[0])
    {
      if (nr (1,12) != 3)
	job->job_type[0] = JOB_GIVE_ITEM;
      else if (nr (1,7) != 5)
	job->job_type[0] = JOB_GIVE_MONEY;
      else
	job->job_type[0] = JOB_KILL_THING;      
    }
  
  /* Now get all of the setup that we need so that we can generate
     these scripts. */
  
  for (i = 0; i < JOB_PREV_MAX; i++)
    {
      if (!job->prev[i])
	{
	  if (i == 0)
	    {
	      area[i] = find_quest_area();
	      mob[i] = find_quest_thing (area[i], person, QUEST_THING_MOBGEN);
	      room[i] = find_random_room (area[i], FALSE, 0, BADROOM_BITS);
	      if (job->job_type[0] == JOB_GIVE_ITEM)
		{
		  obj[i] = find_quest_thing (area[i], mob[i], QUEST_THING_ITEM);
		  if (!obj[i])
		    bad_things_found = TRUE;
		}
	      if (!area[i] || !mob[i] || !room[i])
		bad_things_found = TRUE;
	    }
	}
      else
	{
	  mob[i] = find_thing_num (job->prev[i]->runner);
	  room[i] = find_thing_num (job->prev[i]->runner_room);
	  area[i] = find_thing_num (room[i]->in->vnum);	  
	  if (job->job_type[i] == JOB_GIVE_ITEM)
	    {
	      obj[i] = find_thing_num (job->prev[i]->reward_num);
	      if (!obj[i])
		bad_things_found = TRUE;
	    }
	  if (!mob[i] || !area[i] || !room[i])
	    bad_things_found = TRUE;	  
	}           
    }
  
  if (bad_things_found)
    return NULL;
  
  /* At this point we have the quest to do and we can set up the code. 
     We also hopefully have the correct things set up for each stage. */
  
  for (jobnum = 0; jobnum < JOB_PREV_MAX; jobnum++)
    {
      if ((jobtype = job->job_type[jobnum]) <= JOB_NONE ||
	  jobtype >= JOB_TYPE_MAX)
	continue;
      
      if (jobnum > 0 && !job->prev[jobnum])
	continue;
      
      /* We know we have the objects needed for the previous
	 setup, so hopefully we this will work. */
				      
      for (count = 0; count < 2; count++)
	{
	  /* First time through gen the setup script if this is the first
	     script. */
	  tempbuf[0] = '\0';
	  currbuf[0] = '\0';
	  
	  /* First write setup script. The first one gets an 
	     entering script. */
	  
	  if (jobnum == 0)
	    {
	      questgen_name_iterate (quest_name);
	      setup_quest_trigger (quest_name, person->vnum, TRIG_ENTER, NULL);
	    }
	  if (count == 0)
	    {
	      if (jobnum > 0 || job->prev[jobnum])
		continue;
	      
	      /* So this is a base job with no previous things. */
	      
	      sprintf (tempbuf, "#ask_intro\n");
	      strcat (tempbuf, questgen_init_string (job->job_type[0]));
	      
	    }
	  else /* Count == 1...this is the 
		  thanks, then next_part/reward quest. */
	    {
	      int trig_type;
	      switch (jobtype)
		{
		  case JOB_GIVE_ITEM:
		    trigtype = TRIG_GIVEN;
		    break;
		  case JOB_GIVE_MONEY:
		    trigtype = TRIG_BRIBE;
		    break;
		  case JOB_GIVE_PASSWORD:
		    trigtype = TRIG_SAY;
		    break;
		  case JOB_KILL_THING:
		    trigtype = TRIG_ENTER;
		    break;
		  default:
		    log_it ("Bad questgen job type in trigger setup!\n");
		    free_job (job);
		    return NULL;
		}
	      
	      questgen_name_iterate (quest_name);
	      setup_quest_trigger 
		(quest_name, person->vnum, trigtype,
		 (jobtype == TRIG_SAY  &&
		  job->prev[i] ? job->prev[i]->reward_password : NULL));
	      
	      
	      
       	    }
	  
	  
	  
	  
	}
    }
  return NULL;
}

THING *
find_quest_area (void)
{
  return NULL;
}
