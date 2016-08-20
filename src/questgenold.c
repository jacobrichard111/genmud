#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "serv.h"
#include "script.h"
#include "worldgen.h"
#include "objectgen.h"
#include "questgenold.h"
#include "questgen.h"
#include "areagen.h"

/* This clears all of the quests given to worldgen mobs. It doesn't
   actually remove the triggers and quests, it just nukes
   all of the triggers and quests associated with the worldgen areas. */



/* The names of the quests get iterated in this string. */
static char questgen_name[STD_LEN];

/* The names of the questflags that are used to tell who's done what
   quests are iterated in this string. */
static char questgen_qflag_name[STD_LEN];

/* This is the current number of the flag we're supposed to use
   to tell which flag to set in the questflag values. */

int questgen_qflag_num = -1;


void
worldgen_clear_quests(THING *th)
{
  TRIGGER *trig;
  CODE *code;
  int i;
  
  for (i = 0; i < HASH_SIZE; i++)
    {
      for (trig = trigger_hash[i]; trig; trig = trig->next)
	{
	  if (trig->vnum < WORLDGEN_START_VNUM ||
	      trig->vnum > WORLDGEN_UNDERWORLD_START_VNUM + WORLDGEN_VNUM_SIZE)
	    continue;
	  
	  trig->flags |= TRIG_NUKE;
	  if ((code = find_code_chunk (trig->code)) != NULL)
	    {
	      code->flags |= CODE_NUKE;
	    }
	}
    }

  write_codes ();
  write_triggers(); 
  stt ("All worldgen triggers and scripts cleared.\n\r", th);
  return;
}


/* This is how all of the quests will be generated. */

void
worldgen_generate_quests (void)
{
  strcpy (questgen_name, "aa");
  strcpy (questgen_qflag_name, "aaaaqzx");
  
  /* These are simple "Get foo from bar for me" quests. Have to
     start somewhere. :) */
  
  questgen_fedex_single_areas ();
  questgen_fedex_multi_areas();
  
  return;
}

/* This attempts to do fedex quests in each single area. */

void 
questgen_fedex_single_areas (void)
{
  THING *area;
  
  for (area = the_world->cont; area; area = area->next_cont)
    {
      questgen_fedex_single_area (area);
    }
  return;
}

/* This lets you do questgens that span multiple areas. They just
   pick random areas for each quest and then they get sent there. */


void
questgen_fedex_multi_areas (void)
{
  THING *area = NULL, *area2 = NULL;
  int num_worldgen_areas = 0;
  int num_quests;
  
  int times, area_tries;
  /* First get the number of worldgen areas. */

  for (area = the_world->cont; area; area = area->next_cont)
    {
      if (area->vnum >= WORLDGEN_START_VNUM &&
	  area->vnum < WORLDGEN_END_VNUM)
	num_worldgen_areas++;
    }
  
  unmark_areas();
  /* Figure out the number of quests. */

  num_quests = nr (num_worldgen_areas/10, num_worldgen_areas/5);
  
  
  for (times = 0; times < num_quests; times++)
    {
      for (area_tries = 0; area_tries < 30; area_tries++)
	{
	  if ((area = find_random_area 
(AREA_NOLIST|AREA_NOSETTLE|AREA_NOREPOP)) == NULL ||
	      area->vnum < WORLDGEN_START_VNUM ||
	      area->vnum >= WORLDGEN_END_VNUM ||
	      IS_MARKED (area) ||
	      (area2 = find_random_area 
(AREA_NOLIST|AREA_NOSETTLE|AREA_NOREPOP)) == NULL ||
	      area2 == area  ||
	      area->vnum < WORLDGEN_START_VNUM || 
	      area->vnum > WORLDGEN_END_VNUM)
	    {
	      area = NULL;
	      area2 = NULL;	      
	      continue;
	    }
	  break;
	}
      
      if (area && area2)
	{
	  SBIT(area->thing_flags, TH_MARKED);
	  SBIT(area2->thing_flags, TH_MARKED);
	  questgen_generate (area, area2, QUEST_GET_ITEM_RANDMOB);      
	}
    }
  unmark_areas();
  return;
}
      
  


/* This generates some simple fedex quests within the area stated.
   It does this by repeatedly finding a talking nonscript mob and
   then it makes an item if possible and loads the item on a
   random randpop mob that gets reset in some room. Then if all
   of that works, the talking mob gets 2 scripts. 

   1. an enter script where it tells its story, then

   2. a script quest where it receives and item and gives
      a reward.

*/

void
questgen_fedex_single_area (THING *area)
{
  int num_quests, i;
  int num_mobs = 0;
  THING *mob;
  if (!area || !IS_AREA (area) || area->vnum < WORLDGEN_START_VNUM ||
      area->vnum > WORLDGEN_END_VNUM)
    return;
  for (mob = area->cont; mob; mob = mob->next_cont)
    {
      if (CAN_FIGHT(mob) && CAN_TALK(mob) && !IS_ROOM(mob) &&
	  !IS_SET (mob->thing_flags, TH_SCRIPT) && CAN_MOVE(mob))
	num_mobs++;
    }
  
  num_quests = nr (0, (7+ num_mobs)/15);
  
  for (i = 0; i < num_quests; i++)
    questgen_generate (area, area, QUEST_GET_ITEM_RANDMOB);
  
  return;
}

/* This generates a simple quest between two areas (or possibly
   within one area). It consists of killing some randomly generated
   mob that pops some piece of eq somewhere in some area. */

void
questgen_generate (THING *start_area, THING *end_area, int quest_type)
{
  THING *proto = NULL;
  int open_vnum;
  bool found_questmob_slot = FALSE;
  /* Thing that hands out the quest. */
  THING *quest_giver = NULL;
  /* The mob that will pop the item that the quest giver wants. */
  THING *quest_mob = NULL;
  /* The place where the quest mob will reset. */
  THING *quest_room = NULL;
  /* The item that you're supposed to return to the quest giver. */
  THING *quest_item = NULL;
  
  
  /* Make sure the areas exist and they're in the acceptable range and
     that the type of quest is ok. */

  if (!start_area || !IS_AREA (start_area) || 
      start_area->vnum < WORLDGEN_START_VNUM || 
      start_area->vnum > WORLDGEN_END_VNUM ||
      !end_area || !IS_AREA (end_area) ||
      end_area->vnum < WORLDGEN_START_VNUM || 
      end_area->vnum > WORLDGEN_END_VNUM)
    return;
  
  if (quest_type <= QUEST_NONE || quest_type >= QUEST_MAX)
    return;
  
  /* Increment the quest name and the flagname. */
  
  questgen_name_iterate (questgen_name);

  questgen_qflag_num++;
  
  /* Store 30*num_vals questflags in one value to save space. */
  
  if (questgen_qflag_num >= 30*NUM_VALS)
    {
      questgen_qflag_num = 0;
      questgen_name_iterate (questgen_qflag_name);
    }
  
  if (quest_type == QUEST_GET_ITEM_RANDMOB)
    {
      if ((quest_giver = find_quest_thing (start_area, NULL, QUEST_THING_PERSON)) == NULL ||
	  (quest_mob = find_quest_thing (NULL, start_area, QUEST_THING_MOBGEN)) == NULL ||
	  (quest_room = find_quest_thing (end_area, NULL, QUEST_THING_ROOM)) == NULL ||
	  (quest_item = find_quest_thing (end_area, quest_mob, QUEST_THING_ITEM)) == NULL)
	return;
      
      /* Copy quest mob to a new number. */
      for (open_vnum = start_area->vnum + start_area->mv + 1; 
	   open_vnum < start_area->vnum + start_area->max_mv; open_vnum++)
	{
	  if ((proto = find_thing_num (open_vnum)) == NULL)
	    {
	      if ((proto = new_thing ()) == NULL)
		break;
	      found_questmob_slot = TRUE;
	      copy_thing (quest_mob, proto);
	      proto->vnum = open_vnum;
	      proto->max_mv = 1;
	      proto->thing_flags = MOB_SETUP_FLAGS | TH_NO_TALK;
	      proto->name = new_str (quest_mob->name);
	      proto->short_desc = new_str (quest_mob->short_desc);
	      proto->long_desc = new_str (quest_mob->long_desc);
	      proto->desc = new_str (quest_mob->desc);
	      proto->type = new_str (quest_mob->type);
	      copy_resets (quest_mob, proto);	   
	      proto->in = NULL;
	      proto->next_cont = NULL;
	      proto->prev_cont = NULL;
	      proto->cont = NULL;
	      thing_to (proto, start_area);
	      add_thing_to_list (proto);
	      add_reset (quest_room, proto->vnum, 100, 1, 1);
	      add_reset (proto, quest_item->vnum, 10, 1, 1);
	      break;
	    }
	}
      if (!found_questmob_slot)
	return;
      generate_setup_script (quest_giver, proto, quest_room, quest_item);
      generate_given_script (quest_giver, quest_item);
      return;
    }
  
  return;
}

/* This finds a thing of a certain type within a certain area. */

THING *
find_quest_thing (THING *in, THING *for_whom, int type)
{
  THING *thg;
  int num_choices = 0, num_chose = 0, count;
  VALUE *guild = NULL;
  if (type < 0 || type >= QUEST_THING_MAX)
    return NULL;
  if (in == NULL && type == QUEST_THING_MOBGEN)
    in = find_thing_num (MOBGEN_LOAD_AREA_VNUM);
  if (!in || !IS_AREA (in))
    return NULL;

  if (type == QUEST_THING_ITEM && for_whom)
    { 
      /* Find the last name for the for whom item which should
	 be the name of the owner. */
      
      thg = objectgen (in, ITEM_WEAR_NONE, for_whom->level/4, 0, NULL);
      if (thg)
	thg->cost *=10;
      return thg;
    }

  
  for (count = 0; count < 2; count++)
    {
      for (thg = in->cont; thg; thg = thg->next_cont)
	{
	  if (type == QUEST_THING_ROOM &&
	      ((!IS_ROOM (thg) || IS_ROOM_SET (thg, BADROOM_BITS)) ||
	       thg->resets))
	    continue;
	  
	  if (type == QUEST_THING_PERSON || type == QUEST_THING_MOBGEN)
	    {
	      
	      /* A person or mob must be a mob and it can't be scripted */
	      if (!CAN_MOVE (thg) || !CAN_FIGHT (thg) ||
		  IS_SET (thg->thing_flags, TH_SCRIPT) ||
		  (guild = FNV (thg, VAL_GUILD)) != NULL)
		continue;
	      
	      /* If the thing is for someone else, make sure it's
		 close to the same level. */
	      if (for_whom && 
		  (thg->level < for_whom->level/2 ||
		   thg->level > for_whom->level*2))
		continue;
	      
		if (type == QUEST_THING_PERSON && 
		    (!CAN_TALK (thg) || 
		     IS_ACT1_SET (thg, ACT_ANGRY | ACT_AGGRESSIVE)))
		  continue;
		
		if (type == QUEST_THING_MOBGEN && CAN_TALK(thg))
		  continue;
		
	    }
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      if (count == 0)
	{
	  if (num_choices < 1)
	    return NULL;
	  num_chose = nr (1, num_choices);
	}
    }
  if (type == QUEST_THING_PERSON)
    thg->thing_flags |= TH_SCRIPT;
  return thg;
}


/* This sets up a quest for when people enter the room. The thing
   asks for help to get something from something. */


void
generate_setup_script (THING *quest_giver, THING *quest_mob, 
		       THING *quest_room, THING *quest_item)
{
  TRIGGER *trig;
  CODE *code;
  char name[STD_LEN];
  char txt[STD_LEN*10];
  char line[STD_LEN];
  
  if (!quest_giver || !quest_mob || !quest_item || !quest_room ||
      !quest_room->in || !IS_AREA (quest_room->in))
    return;
  /* Name is 2 random letters followed by the thing's vnum. */
  sprintf (name, "%sS", questgen_name);
  
  /* Set up the trigger. */
  trig = new_trigger();
  trig->vnum = quest_giver->vnum;
  trig->next = trigger_hash[trig->vnum % HASH_SIZE];
  trigger_hash[trig->vnum % HASH_SIZE] = trig;
  trig->name = new_str (name);
  trig->code = new_str (name);
  trig->type = TRIG_ENTER;
  trig->flags = TRIG_LEAVE_STOP | TRIG_INTERRUPT | TRIG_PC_ONLY;
  
  /* Now set up the code. */
  code = new_code();
  code->name = new_str (name);
  add_code_to_table (code);
  txt[0] = '\0';
  
  strcat (txt, "wait @r(2,4)\n");
 
  sprintf (line, "do say ");
  if (nr (1,3) == 2)
    strcat (line, "O ");
 
  
  /* Give the first line of text. */
  strcat (line, find_gen_word (QUESTGEN_AREA_VNUM, "compliment", NULL));
  strcat (line, " ");
  strcat (line, find_gen_word (QUESTGEN_AREA_VNUM, "adventurer", NULL));
  strcat (line, " ");
  strcat (line, find_gen_word (QUESTGEN_AREA_VNUM, "ask_for", NULL));
  strcat (line, " ");  
  strcat (line, find_gen_word (QUESTGEN_AREA_VNUM, "help_me", NULL));
  strcat (line, " ");
  strcat (line, find_gen_word (QUESTGEN_AREA_VNUM, "short_time", NULL));
  strcat (line, "?\n");
  strcat (txt, line);
  
  
  strcat (txt, "wait @r(5,12)\n");
  
  sprintf (line, "do say %s$F near %s$F in %s$F has %s$F that I need.\n",
	   NAME(quest_mob), NAME(quest_room), 
	   NAME (quest_room->in), NAME(quest_item));
  strcat (txt, line);
  strcat (txt, "wait @r(5,12)\n");
  sprintf (line, "do say %s bring it to me?\n",
	   find_gen_word (QUESTGEN_AREA_VNUM, "ask_for", NULL));
  strcat (txt, line);
  code->code = new_str (txt);
  return;
}


/* This generates a given script so that when the quest mob is
   given this item, it gives the giver a monetary reward. */

void    
generate_given_script (THING *quest_giver, THING *quest_item)
{
  TRIGGER *trig;
  CODE *code;
  char name[STD_LEN];
  char txt[STD_LEN*10];
  char buf[STD_LEN];
  char qfbuf[STD_LEN];  /* Buffer for the questflag data. */
  int valnum = 0;
  int bitnum = 0;
  int reward_type;
  
  valnum = questgen_qflag_num/30;
  bitnum = questgen_qflag_num % 30;
  
  if (valnum < 0 || valnum >= NUM_VALS)
    return;

  if (!quest_giver || !quest_item)
    return;
  
  /* Name is 2 random letters followed by the thing's vnum. */
  sprintf (name, "%sG", questgen_name);
  
  /* Set up the trigger. */
  trig = new_trigger();
  trig->vnum = quest_giver->vnum;
  trig->type = TRIG_GIVEN;
  trig->next = trigger_hash[trig->vnum % HASH_SIZE];
  trigger_hash[trig->vnum % HASH_SIZE] = trig;
  trig->name = new_str (name);
  trig->code = new_str (name);
  trig->flags = TRIG_LEAVE_STOP | TRIG_PC_ONLY;
  
  /* Now set up the code. */
  code = new_code();
  code->name = new_str (name);
  sprintf (name, "%d", quest_item->vnum);
  trig->keywords = new_str (name);
  add_code_to_table (code);
  txt[0] = '\0';
  
  /* Get the reward type. */
  if (nr (1,8) != 2)
    reward_type = QUEST_REWARD_MONEY;
  else
    reward_type = QUEST_REWARD_EXPERIENCE;

  /* First check if we have the questflag already set. */

  sprintf (qfbuf, "@sqf:%s:%d:%d", questgen_qflag_name, valnum, bitnum);
  
  sprintf (buf, "if (%s != 0) 3\n", qfbuf);
  strcat (txt, buf);
  strcat (txt, "do say You've already completed this quest!\n");
  strcat (txt, "nuke @o\n");
  strcat (txt, "done\n");
  sprintf (buf, "%s = 1\n", qfbuf);
  strcat (txt, buf);
  strcat (txt, "do say Thank you for finding @osd$F!\n");
  if (reward_type == QUEST_REWARD_MONEY)
    {
      strcat (txt, "@v0 = @ocs\n");
      strcat (txt, "@v0 * 5\n");
      strcat (txt, "@rv:money:0 + @v0\n");
      strcat (txt , "do say Here's your reward!\n");
      strcat (txt , "do give @v0 copper @snm\n");
    }
  else if (reward_type == QUEST_REWARD_EXPERIENCE)
    {
      sprintf (buf, "@sex + %d\n", 10*exp_to_level(LEVEL(quest_item)));
      strcat (txt, buf);
    }
  
  strcat (txt, "nuke @o\n");
  strcat (txt,  "done\n");
  code->code = new_str (txt);
  return;
}


/* This iterates the questgen namer so that we end up hashing
   the names pretty well and keeping them short, too */

void
questgen_name_iterate(char *txt)
{
 
  if (!txt)
    return;
  
  if (*txt < 'a')
    {
      *txt = 'a';
      return;
    }
  
  *txt = (*txt) + 1;
  if (*txt > 'z')
    {
      *txt -= 'z' - 'a';
      questgen_name_iterate (txt + 1);
      return;
    }
  return;
}


  
