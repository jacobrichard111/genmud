#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "script.h"
#include "wildalife.h"
#include "craft.h"

/* This function takes a single string inputted by a person and
   attempts to process it as a command. */


void 
interpret (THING *th, char *arg)
{
  char buf[STD_LEN];
  char word[STD_LEN];
  char *true_arg;
  CMD *com = NULL;
  CHAN *chan;
  bool found = FALSE;
  bool done = FALSE;
  int i, hurt_bits = 0;
  THING *vict, *victn;
  
  if (!arg || !arg[0] || !th || !th->in)
    return;
  strcpy (buf, arg);
  
  /* If the first letter is not an alphanumeric char, then assume that
     the first character is the whole command. (for channels mostly.) */

  if (!isalnum(*arg))
    {
      true_arg = arg+1;
      while (isspace (*true_arg) && *true_arg)
	true_arg++;
      word[0] = *arg;
      word[1] = '\0';
    }
  else
    true_arg = f_word (arg, word);

  if (IS_PC (th) && IS_PC1_SET (th, PC_FREEZE))
    {
      stt ("You are frozen and unable to do anything!\n\r", th);
      return;
    }

  if (!IS_PC (th) || LEVEL (th) <= MORT_LEVEL)
    {
      hurt_bits = flagbits (th->flags, FLAG_HURT); 
  
      if (IS_SET (hurt_bits, AFF_PARALYZE))
	{
	  stt ("YOU CAN'T MOVE!!!\n\r", th);
	  return;
	}
      
      if (IS_SET (hurt_bits, AFF_CONFUSE) && nr (1,8) == 2)
	{
	  act ("@1n seem@s a little bit confused.", th, NULL, NULL, NULL, TO_ALL);
	  return;
	}
    }
  
  remove_flagval (th, FLAG_AFF, AFF_HIDDEN);
  for (chan = channel_list; chan; chan = chan->next)
    {
      /* If the word only has one letter, we check if the interpreter
	 tries to find a channel using just that one letter..since for
	 some combos like say/south we want SOUTH to be the thing which
	 gets used. ALso check for the proper level. */
      
      if (LEVEL (th) >= chan->use_level)
	{
	  for (i = 0; i < chan->num_names; i++)
	    {
	      if (!str_cmp (word, chan->name[i]))
		{
		  do_channel (th, chan, true_arg);
		  found = TRUE;
		  done = TRUE;
		  break;
		}
	    }
	}
    }
  if (!found)
    {
      for (i = 0; i < CLAN_MAX; i++)
	{
	  if (!str_cmp (word, clantypes[i]))
	    {
	      clan (th, true_arg, i);
	      found = TRUE;
	      done = TRUE;
	      break;
	    }
	}
    }
  if (!found)
    {
      for (com = com_list[LC(word[0])]; com && !found; com = com->next)
	{
	  if (LEVEL (th) >= com->level &&
	      !str_prefix (word, com->name))
	    {
	      found = TRUE;
	      break;
	    }
	}
    }
  if (!found)
    {
      if (check_craft_command (th, arg))
	{
	  found = TRUE;
	  done = TRUE;
	}
    }
  if (!found)
    {
      if (!found_social (th, arg))
	{
	  if (nr(1,3) == 2)
	    stt ("Huh?\n\r", th);
	  else if (nr (1,2) == 2)
	    stt ("What?\n\r", th);
	  else
	    stt ("I don't understand.\n\r", th); 
	}
      done = TRUE;
    }
  if (!done)
    (*com->called_function) (th, true_arg);
  
  if (th->in)
    check_trigtype (th->in, th, NULL, word, TRIG_COMMAND);
  for (vict = th->cont; vict != NULL && th->in; vict = victn)
    {
      victn = vict->next_cont;
      if (!IS_SET (vict->thing_flags, TH_SCRIPT))
	continue;
      check_trigtype (vict, th, NULL, word, TRIG_COMMAND);
    }
  if (th->in)
    for (vict = th->in->cont; vict != NULL && th->in; vict = victn)
      {
	victn = vict->next_cont;
	if (!IS_SET (vict->thing_flags, TH_SCRIPT))
	  continue;
	check_trigtype (vict, th, NULL, word, TRIG_COMMAND);
      }
  if (IS_PC (th) &&
      (IS_SET (server_flags, SERVER_LOG_ALL) || 
       (com && com->log) ||
       IS_PC1_SET (th, PC_LOG)))
    {
      sprintf (buf, "%s: %s", NAME (th), arg);
      log_it (buf);
    }
  
       
  return;
}

/* Writes an error message to the logfile. */

void
log_it (char *txt)
{
  char buf[STD_LEN *3];
  
  if (!txt || !*txt)
    return;
  
  sprintf (buf, "%s Log: %s\n", c_time (current_time), txt);
  fprintf (stderr, buf);
  return;
}

CMD *
new_cmd (void)
{
  CMD *newcmd;
  if (cmd_free)
    {
      newcmd= cmd_free;
      cmd_free = cmd_free->next;
    }
  else
    {
      newcmd = (CMD *) mallok (sizeof (CMD));
    }
  bzero (newcmd, sizeof (CMD));
  newcmd->called_function = NULL;
  newcmd->next = NULL;
  newcmd->name = nonstr;
  newcmd->word = nonstr;
  return newcmd;
}

void
free_cmd (CMD *cmd)
{
  if (!cmd)
    return;

  cmd->called_function = NULL;
  free_str (cmd->name);
  cmd->name = nonstr;
  free_str (cmd->word);
  cmd->word = nonstr;
  cmd->next = cmd_free;
  cmd_free = cmd;
}

/* This function takes a single command and adds it to the linked lists. */


void
add_command (char *name, COMMAND_FUNCTION *funct, int level, bool log)
{
  CMD *newcmd;
  newcmd = (CMD *) new_cmd (); 
  cmd_count++;
  newcmd->called_function = funct;
  newcmd->level = level;
  newcmd->name = new_str (name);
  newcmd->next = com_list[LC(name[0])];
  com_list[LC(name[0])] = newcmd;
  newcmd->log = log;
  return;
}

/* This goes through the list of commands checking each one you have
   to put them here, or else the server will not know they exist..essentially
   running each one brings it into the system. */

void
init_command_list (void)
{
  add_command ("wizlock", do_wizlock, MAX_LEVEL, TRUE);
  add_command ("zap", do_zap, 0, FALSE);
  add_command ("raze", do_raze, 0, FALSE);
  add_command ("brandish", do_brandish, 0, FALSE);
  add_command ("bribe", do_bribe, 0, FALSE);
  add_command ("recite", do_recite, 0, FALSE);
  add_command ("skills", do_skills, 0, FALSE);
  add_command ("spells", do_spells, 0, FALSE);
  add_command ("poisons", do_poisons, 0, FALSE);
  add_command ("ascend", do_ascend, 0, FALSE);
  add_command ("traps", do_traps, 0, FALSE);
  add_command ("proficiencies", do_proficiencies, 0, FALSE);  
  add_command ("topten", do_topten, 0, FALSE);
  add_command ("rating", do_rating, 0, FALSE);
  add_command ("skim", do_skim, 0, FALSE);
  add_command ("manage", do_manage, 0, FALSE);
 
#ifdef USE_WILDERNES
  add_command ("sectors", do_sectors, BLD_LEVEL, FALSE);
#endif
  add_command ("inspire", do_inspire, 0, FALSE);
  add_command ("demoralize", do_demoralize, 0, FALSE);
  add_command ("channels", do_channels, 0, FALSE);
  add_command ("thing", do_thing, MAX_LEVEL, FALSE);
  add_command ("capture", do_capture, 0, FALSE);
  add_command ("craft", do_craft, 0, FALSE);
  add_command ("echo", do_echo, 0, FALSE);
  add_command ("exlist", do_exlist, BLD_LEVEL, FALSE);
  add_command ("peace", do_peace, 0, FALSE);
  add_command ("pfile", do_pfile, 0, FALSE);
  add_command ("description", do_description, 0, FALSE);
  add_command ("password", do_password, 0, FALSE);
  add_command ("citygen", do_citygen, MAX_LEVEL, FALSE);
  add_command ("areagen", do_areagen, MAX_LEVEL, FALSE);
  add_command ("cavegen", do_cavegen, MAX_LEVEL, FALSE);
  add_command ("play", do_play, 0, FALSE);
  add_command ("areas", do_areas, 0, FALSE);
  add_command ("log", do_log, MAX_LEVEL, TRUE);
  add_command ("snoop", do_snoop, MAX_LEVEL, FALSE);
  add_command ("sharpen", do_sharpen, 0, FALSE);
  add_command ("news", do_news, 0, FALSE);
  add_command ("return", do_return, 0, FALSE);
  add_command ("rumors", do_rumors, MAX_LEVEL, FALSE);
  add_command ("run", do_run, 0, FALSE);
  add_command ("switch", do_switch, MAX_LEVEL, FALSE);
  add_command ("swap", do_swap, 0, FALSE);
  add_command ("ally", do_alliances, 0, FALSE);
  add_command ("allies", do_alliances, 0, FALSE);
  add_command ("alliances", do_alliances, 0, FALSE);
  add_command ("scan", do_scan, 0, FALSE);
  add_command ("extinguish", do_extinguish, 0, FALSE);
  add_command ("split", do_split, 0, FALSE);
  add_command ("imbue", do_imbue, 0, FALSE);
  add_command ("graft", do_graft, 0, FALSE);
  add_command ("investigate", do_investigate, 0, FALSE);
  /*  add_command ("forge", do_forge, 0, FALSE); */
  add_command ("metalgen", do_metalgen, MAX_LEVEL, FALSE);
  add_command ("update", do_update, MAX_LEVEL, FALSE);
  add_command ("enchant", do_enchant, 0, FALSE);
  add_command ("butcher", do_butcher, 0, FALSE);
  add_command ("search", do_search, 0, FALSE);
  add_command ("transfer", do_transfer, MAX_LEVEL, FALSE);
  add_command ("trigedit", do_trigedit, MAX_LEVEL, FALSE);
  add_command ("score", do_score, 0, FALSE);
  add_command ("battleground", do_battleground, 0, FALSE);
  add_command ("users", do_users, MAX_LEVEL, FALSE);
  add_command ("wsave", do_worldsave, BLD_LEVEL, FALSE);
  add_command ("weight", do_weight, 0, FALSE);
  add_command ("fortify", do_fortify, 0, FALSE);
  add_command ("mythology", do_mythology, 0, FALSE);
  add_command ("get", do_get, 0, FALSE);
  add_command ("drop", do_drop, 0, FALSE);
  add_command ("diplomacy", do_diplomacy, 0, FALSE);
  add_command ("divine", do_divine, 0, FALSE);
  add_command ("memory", do_memory, MAX_LEVEL, FALSE);
  add_command ("enter", do_enter, 0, FALSE);
  add_command ("levels", do_levels, 0, FALSE);
  add_command ("emplace", do_emplace, 0, FALSE);
  add_command ("push", do_push, 0, FALSE);
  add_command ("brew", do_brew, 0, FALSE);
  add_command ("scribe", do_scribe, 0, FALSE);
  add_command ("mount", do_mount, 0, FALSE);
  add_command ("dismount", do_dismount, 0, FALSE);
  add_command ("pbase", do_pbase, 0, FALSE);
  add_command ("buck", do_buck, 0, FALSE);
  add_command ("examine", do_look, 0, FALSE);
  add_command ("exits", do_exits, 0, FALSE);
  add_command ("think", do_think, 0, FALSE);
  add_command ("sset", do_sset, MAX_LEVEL, FALSE);
  add_command ("put", do_put, 0, FALSE);
  add_command ("sheath", do_sheath, 0, FALSE);
  add_command ("draw", do_draw, 0, FALSE);
  add_command ("mapgen", do_mapgen, MAX_LEVEL, FALSE);
  add_command ("map", do_map, MAX_LEVEL, FALSE);
  add_command ("commands", do_commands, 0, FALSE);
  add_command ("slist", do_slist, 0, FALSE);
  add_command ("slay", do_slay, MAX_LEVEL, FALSE);
  add_command ("sla", do_sla, MAX_LEVEL, FALSE);
  add_command ("eat", do_eat, 0, FALSE);
  add_command ("fill", do_fill, 0, FALSE);
  add_command ("sneak", do_sneak, 0, FALSE);
  add_command ("hide", do_hide, 0, FALSE);
  add_command ("visible", do_visible, 0, FALSE);
  add_command ("chameleon", do_chameleon, 0, FALSE);
  add_command ("drink", do_drink, 0, FALSE);
  add_command ("make", do_make, BLD_LEVEL, FALSE);
  add_command ("pagelength", do_pagelength, 0, FALSE);
  add_command ("reboot", do_reboot, MAX_LEVEL, TRUE);
  add_command ("reboo", do_reboo, MAX_LEVEL, FALSE);
  add_command ("delete", do_delete, 0, TRUE);
  add_command ("delet", do_delet, 0, FALSE);
  add_command ("flurry", do_flurry, 0, FALSE);
  add_command ("backstab", do_backstab, 0, FALSE);
  add_command ("flee", do_flee, 0, FALSE);
  add_command ("bs", do_backstab, 0, FALSE);
  add_command ("fire", do_fire, 0, FALSE);
  add_command ("aim", do_fire, 0, FALSE);
  add_command ("load", do_load, 0, FALSE);
  add_command ("unload", do_unload, 0, FALSE);
  add_command ("goto", do_goto, BLD_LEVEL, FALSE);
  add_command ("armor", do_armor, 0, FALSE);
  add_command ("give", do_give, 0, FALSE);
  add_command ("purse", do_purse, 0, FALSE);
  add_command ("order", do_order, 0, FALSE);
  add_command ("affects", do_affects, 0, FALSE);
  add_command ("configure", do_config, 0, FALSE);
  add_command ("offensive", do_offensive, 0, FALSE);
  add_command ("consider", do_consider, 0, FALSE);
  add_command ("gconsider", do_gconsider, 0, FALSE);
  add_command ("speedup", do_speedup, MAX_LEVEL, TRUE);
  add_command ("beep", do_beep, BLD_LEVEL, FALSE);
  add_command ("disarm", do_disarm, 0, FALSE);
  add_command ("assist", do_assist, 0, FALSE);
  add_command ("take", do_get, 0, FALSE);
  add_command ("grab", do_get, 0, FALSE);
  add_command ("pkdata", do_pkdata, 0, FALSE);
  add_command ("open", do_open, 0, FALSE);
  add_command ("deposit", do_deposit, 0, FALSE);
  add_command ("alchemy", do_alchemy, 0, FALSE);
  add_command ("withdraw", do_withdraw, 0, FALSE);
  add_command ("pick", do_pick, 0, FALSE);
  add_command ("group", do_group, 0, FALSE);
  add_command ("prompt", do_prompt, 0, FALSE);
  add_command ("follow", do_follow, 0, FALSE);
  add_command ("compare", do_compare, 0, FALSE);
  add_command ("nolearn", do_nolearn, 0, FALSE);
  add_command ("ditch", do_ditch, 0, FALSE);
  add_command ("break", do_break, 0, FALSE);
  add_command ("twhere", do_twhere, BLD_LEVEL, FALSE);
  add_command ("tfind", do_tfind, BLD_LEVEL, FALSE);
  add_command ("tset", do_tset, MAX_LEVEL, FALSE);
  add_command ("attributes", do_attributes, 0, FALSE);
  add_command ("at", do_at, MAX_LEVEL, FALSE);
  add_command ("alias", do_alias, 0, FALSE);
  add_command ("unlock", do_unlock, 0, FALSE);
  add_command ("lock", do_lock, 0, FALSE);
  add_command ("trophy", do_trophy, 0, FALSE);
  add_command ("store", do_store, 0, FALSE);
  add_command ("light", do_light, 0, FALSE);
  add_command ("sleep", do_sleep, 0, FALSE);
  add_command ("stand", do_stand, 0, FALSE);
  add_command ("wake", do_wake, 0, FALSE);
  add_command ("rescue", do_rescue, 0, FALSE);
  add_command ("guard", do_guard, 0, FALSE);
  add_command ("reset", do_reset, BLD_LEVEL, FALSE);
  add_command ("disengage", do_disengage, 0, FALSE);
  add_command ("restore", do_restore, MAX_LEVEL, FALSE);
  add_command ("rest", do_rest, 0, FALSE);
  add_command ("unlearn", do_unlearn, 0, FALSE);
  add_command ("citybuild", do_citybuild, 0, FALSE);
  add_command ("meditate", do_meditate, 0, FALSE);
  add_command ("zedit", do_zedit, 0, FALSE);
  add_command ("unstore", do_unstore, 0, FALSE);
  add_command ("cls", do_cls, 0, FALSE);
  add_command ("resize", do_resize, 0, FALSE);
  add_command ("pbase", do_pbase, 0, FALSE);
  add_command ("siteban", do_siteban, MAX_LEVEL, FALSE);
  add_command ("track", do_track, 0, FALSE);
  add_command ("silence", do_silence, MAX_LEVEL, TRUE);
  add_command ("freeze", do_freeze, MAX_LEVEL, TRUE);
  add_command ("finger", do_finger, 10, TRUE);
  add_command ("deny", do_deny, MAX_LEVEL, TRUE);
  add_command ("validate", do_validate, MAX_LEVEL, FALSE);
  add_command ("auction", do_auction, 0, FALSE);
  add_command ("guild", do_guild, 0, FALSE);
  add_command ("implant", do_implant, 0, FALSE);
  add_command ("bid", do_bid, 0, FALSE);
  add_command ("wimpy", do_wimpy, 0, FALSE);
  add_command ("sedit", do_sedit, MAX_LEVEL, TRUE);
  /* Do race must come after do racechange so that do race is checked
     first...or you'll never be able to use the race command again. */
  add_command ("racechange", do_racechange, 0, FALSE);
  add_command ("race", do_race, 0, FALSE);
  add_command ("align", do_align, 0, FALSE);
  add_command ("close", do_close, 0, FALSE);
  add_command ("where", do_where, 0, FALSE);
  add_command ("who", do_who, 0, FALSE);
  add_command ("advance", do_advance, MAX_LEVEL, FALSE);
  add_command ("leave", do_leave, 0, FALSE);
  add_command ("kick", do_kick, 0, FALSE);
  add_command ("knock", do_knock, 0, FALSE);
  add_command ("kill", do_kill, 0, FALSE);
  add_command ("repair", do_repair, 0, FALSE);
  add_command ("bash", do_bash, 0, FALSE);
  add_command ("tstat", do_tstat, BLD_LEVEL, FALSE);
  add_command ("tackle", do_tackle, 0, FALSE);
  add_command ("climb", do_climb, 0, FALSE);
  add_command ("noaffect", do_noaffect, MAX_LEVEL, FALSE);
  add_command ("note", do_note, 0, FALSE);
  add_command ("worldgen", do_worldgen, MAX_LEVEL, FALSE);
  add_command ("world", do_world, 0, FALSE);
  add_command ("time", do_world, 0, FALSE);
  add_command ("info", do_world, 0, FALSE);
  add_command ("weather", do_world, 0, FALSE);
  add_command ("cast", do_cast, 0, FALSE);
  add_command ("ignore", do_ignore, 0, FALSE);
  add_command ("help", do_help, 0, FALSE);
  add_command ("save", do_save, 0, FALSE);
  add_command ("sell", do_sell, 0, FALSE);
  add_command ("buy", do_buy, 0, FALSE);
  add_command ("mana", do_mana, 0, FALSE);
  add_command ("appraise", do_appraise, 0, FALSE);
  add_command ("account", do_account, 0, FALSE);
  add_command ("list", do_list, 0, FALSE);
  add_command ("socials", do_socials, 0, FALSE);
  add_command ("title", do_title, 0, FALSE);
  add_command ("society", do_society, BLD_LEVEL, FALSE);
  add_command ("edit", do_edit, BLD_LEVEL, FALSE);
  add_command ("tedit", do_edit, BLD_LEVEL, FALSE);
  add_command ("thedit", do_edit, BLD_LEVEL, FALSE);
  add_command (">", do_rightarrow, BLD_LEVEL, FALSE);
  add_command ("<", do_leftarrow, BLD_LEVEL, FALSE);
  add_command ("prerequisites", do_prereq, 0, FALSE);
  add_command ("practice", do_practice, 0, FALSE);
  add_command ("say", do_say, 0, FALSE);
  add_command ("quit", do_quit, 0, FALSE);
  add_command ("wear", do_wear, 0, FALSE);
  add_command ("wield", do_wear, 0, FALSE);
  add_command ("hold", do_wear, 0, FALSE);
  add_command ("remort", do_remort, 0, FALSE);
  add_command ("remove", do_remove, 0, FALSE);
  add_command ("reply", do_reply, 0 , FALSE);
  add_command ("purge", do_purge, BLD_LEVEL, FALSE);
  add_command ("qui", do_qui, 0, FALSE);
  add_command ("sacrifice", do_sacrifice, 0, FALSE);
  add_command ("look", do_look, 0, FALSE);
  add_command ("inventory", do_inventory, 0, FALSE);
  add_command ("equipment", do_equipment, 0, FALSE);
  add_command ("east", do_east, 0, FALSE);
  add_command ("west", do_west, 0, FALSE);
  add_command ("south", do_south, 0, FALSE);
  add_command ("north", do_north, 0, FALSE);
  add_command ("up", do_up, 0, FALSE);
  add_command ("down", do_down, 0, FALSE);
  return;
}
  
void
do_commands (THING *th, char *arg)
{
  int count = 0, i;
  CMD *com;
  char buf[STD_LEN];
  buf[0] = '\0';
  bigbuf[0] = '\0';
  add_to_bigbuf ("The commands available within the game:\n\n\r");
  for (i = 0; i < 256; i++)
    {
      for (com = com_list[i]; com != NULL; com = com->next)
	{
	  if (com->level > LEVEL (th))
	    continue;
	  sprintf (buf, "%-18s", com->name);
	  add_to_bigbuf (buf);
	  if (++count == 4)
	    {
	      count = 0;
	      add_to_bigbuf ("\n\r");
	    }
	}
    }
  add_to_bigbuf ("\n\r");
  send_bigbuf (bigbuf, th);
  return;
}


