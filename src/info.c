#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "note.h"
#include "society.h"
#include "script.h"
#include "track.h"
#include "mapgen.h"
#include "rumor.h"
#include "event.h"
#include "worldgen.h"
#ifdef USE_WILDERNESS
#include "wildalife.h"
#endif
void
do_world (THING *th, char *arg)
{
  char buf[STD_LEN];
  int days, hours, minutes, seconds, time_up;
  
  int day = wt_info->val[WVAL_DAY] + 1;
  time_up = current_time - boot_time;
  
  seconds = time_up % 60;
  time_up /= 60;
  minutes = time_up % 60;
  time_up /= 60;
  hours = time_up % 24;
  time_up /= 24;
  days = time_up;
  sprintf (buf, "\x1b[1;30m--------\x1b[0;37m-----------\x1b[1;37m+++++++\x1b[1;31m %s \x1b[1;37m+++++++\x1b[0;37m-----------\x1b[1;30m--------\x1b[0;37m\n\n\r", game_name_string);
  stt (buf, th);
  if (LEVEL (th) >= BLD_LEVEL && IS_SET (server_flags, SERVER_WIZLOCK))
    {
      int i;
      for (i = 0; i < 10; i++)
	stt ("\x1b[1;31m          THE GAME IS WIZLOCKED!!!\n\n\r\x1b[0;37m", th);
    }
  sprintf (buf, "It is %d O'clock game time.\n\r", wt_info->val[WVAL_HOUR] + 1);
  stt (buf, th);
  sprintf (buf, "It is now %s, the %d%s of %s, %d, the year of the %s.\n\n\r", day_names[day % NUM_DAYS],
	   day, (day > 10 && day < 19 ? "th" :
		 day % 10 == 1 ? "st" :
		 day % 10 == 2 ? "nd" :
		 day % 10 == 3 ? "rd" : "th"),
	   month_names[wt_info->val[WVAL_MONTH]],
	   wt_info->val[WVAL_YEAR],
	   year_names[wt_info->val[WVAL_YEAR] % NUM_YEARS]);
  stt (buf, th);
  if (th->in && IS_ROOM (th->in) && !IS_ROOM_SET (th->in, ROOM_INSIDE | 
ROOM_EARTHY | ROOM_UNDERGROUND | ROOM_FIERY))
    {
      int weat = wt_info->val[WVAL_WEATHER];
      sprintf (buf, "It is %d degrees and %s right now.\n\r", 
wt_info->val[WVAL_TEMP], (weat >= 0 && weat < WEATHER_MAX ? 
weather_names[weat] : "sunny"));
       stt (buf, th);
    }
  stt ("\n\r", th);

  sprintf (buf, "The real time is: %s\n\r", c_time (current_time));
  stt (buf, th);
  sprintf (buf, "The server has been up %d day%s, %d hour%s, %d minute%s, and %d second%s.\n\r", days, (days != 1 ? "s" : ""),
	   hours, (hours != 1 ? "s" : ""),
	   minutes, (minutes != 1 ? "s" :""),
	   seconds, (seconds != 1 ? "s" :""));
  stt (buf, th);
  
  /* Added this for the admins to see effectively how long the
     simulation has been up. */
  
  if (LEVEL (th) == MAX_LEVEL)
    {
      time_up = times_through_loop/UPD_PER_SECOND;
      seconds = time_up % 60;
      time_up /= 60;
      minutes = time_up % 60;
      time_up /= 60;
      hours = time_up % 24;
      time_up /= 24;
      days = time_up;
      sprintf (buf, "Server equivalent uptime: %d day%s, %d hour%s, %d minute%s, and %d second%s.\n\r", days, (days != 1 ? "s" : ""),
	       hours, (hours != 1 ? "s" : ""),
	       minutes, (minutes != 1 ? "s" :""),
	       seconds, (seconds != 1 ? "s" :""));
      stt (buf, th);
      if (current_time > boot_time)
	{
	  sprintf (buf, "Current equiv_uptime/uptime ratio: %d\n\r",
		   times_through_loop/
		   (UPD_PER_SECOND*(current_time-boot_time)));
	  stt (buf, th);
	}
          sprintf (buf, "Average track size: %d\n\r", bfs_tracks_count/MAX(1, bfs_times_count));
	      stt (buf, th); 
    }
  
  
  sprintf (buf, "There are currently %d things in the world.\n\r", thing_count);
  stt (buf, th);
  if (LEVEL (th) == MAX_LEVEL)
    {
      sprintf (buf, "A total of %d things have been made this reboot.\n\r", thing_made_count);
      stt (buf, th);
      do_society (th, "count");
      do_areas (th, "count");
      sprintf (buf, "\x1b[1;36m%d\x1b[0;37m words and phrases are used to generate the world.\n\r", find_num_gen_words(the_world));
      stt (buf, th);
    }
  sprintf (buf, "There have been %d players max online since the last reboot.\n\r", max_players);
  stt (buf, th);
  
  return;
}

/* This lists the estimated memory used for the various dynamically
   allocated chunks. It takes the count of memory used times the size
   of each type of memory, and it even adds in the 8 bytes of
   padding you find when you malloc something. */

void
do_memory (THING *th, char *arg)
{
  char buf[STD_LEN];
  
  sprintf (buf, "[%5d] Things    @ %4d B: %-7d[%5d] Values    @ %4d B: %-7d\n\r",
	   thing_count, sizeof (THING), thing_count * sizeof (THING),
	   value_count, sizeof (VALUE), value_count * sizeof (VALUE));
  stt (buf, th);
  
  
  
  sprintf (buf, "[%5d] Flags     @ %4d B: %-7d[%5d] Fights    @ %4d B: %-7d\n\r", 
	   flag_count, sizeof (FLAG), flag_count * sizeof (FLAG),
	   fight_count, sizeof (FIGHT), fight_count * sizeof (FIGHT));
  stt (buf, th);
  
  
  
  sprintf (buf, "[%5d] Resets    @ %4d B: %-7d[%5d] Pcs       @ %4d B: %-7d\n\r", 
	   reset_count, sizeof (RESET), reset_count * sizeof (RESET),
	   pc_count, sizeof (PCDATA), pc_count * sizeof (PCDATA));
  stt (buf, th);  
  

  sprintf (buf, "[%5d] Clans     @ %4d B: %-7d[%5d] Notes     @ %4d B: %-7d\n\r", 
	   clan_count, sizeof (CLAN), clan_count * sizeof (CLAN),
	   note_count, sizeof (NOTE), note_count * sizeof (NOTE));
  stt (buf, th);  



  sprintf (buf, "[%5d] Races     @ %4d B: %-7d[%5d] Spells    @ %4d B: %-7d\n\r", 
	   race_count, sizeof (RACE), race_count * sizeof (RACE),
	   spell_count, sizeof (SPELL), spell_count * sizeof (SPELL));

  stt (buf, th);  


  sprintf (buf, "[%5d] Helps     @ %4d B: %-7d[%5d] BFSs      @ %4d B: %-7d\n\r", 
	   help_count, sizeof (HELP), help_count * sizeof (HELP),
	   bfs_count, sizeof (BFS), bfs_count * sizeof (BFS));
  stt (buf, th);  


  
  sprintf (buf, "[%5d] Cmds      @ %4d B: %-7d[%5d] HelpKeys  @ %4d B: %-7d\n\r", 
	   cmd_count, sizeof (CMD), cmd_count * sizeof (CMD),
	   help_key_count, sizeof (HELP_KEY), help_key_count * sizeof (HELP_KEY));
  stt (buf, th);  
  
  
  sprintf (buf, "[%5d] Auctions  @ %4d B: %-7d[%5d] Script_Ev @ %4d B: %-7d\n\r", 
	   auction_count, sizeof (AUCTION), auction_count * sizeof (AUCTION),
	   script_event_count, sizeof (SCRIPT_EVENT), script_event_count * sizeof (SCRIPT_EVENT));
  stt (buf, th);  
  
  sprintf (buf, "[%5d] Codes     @ %4d B: %-7d[%5d] Triggers  @ %4d B: %-7d\n\r",
	   code_count, sizeof (CODE), code_count * sizeof (CODE),
	   trigger_count, sizeof (TRIGGER), trigger_count * sizeof (TRIGGER));
  stt (buf, th);  
  

  
  sprintf (buf, "[%5d] Chans     @ %4d B: %-7d[%5d] Tracks    @ %4d B: %-7d\n\r", 
	   channel_count, sizeof (CHAN), channel_count * sizeof (CHAN),
	   track_count, sizeof (TRACK), track_count * sizeof (TRACK));
  stt (buf, th); 
 
  sprintf (buf, "[%5d] Societies @ %4d B: %-7d[%5d] Mapgens   @ %4d B: %-7d\n\r",  
	   society_count, sizeof (SOCIETY), society_count * sizeof (SOCIETY),
	   mapgen_count, sizeof (MAPGEN), mapgen_count * sizeof (MAPGEN));
  stt (buf, th); 



  sprintf (buf, "[%5d] Pbases    @ %4d B: %-7d[%5d] Sitebans  @ %4d B: %-4d\n\r",
	  pbase_count, sizeof (PBASE), pbase_count * sizeof (PBASE), 
	  siteban_count, sizeof (SITEBAN), siteban_count * sizeof(SITEBAN));
 stt (buf, th); 
 
  
  sprintf (buf, "[%5d] FDs       @%5d B: %-6d [%5d] Scrollbak @ %4d B: %-6d\n\r",
	   file_desc_count, sizeof (FILE_DESC), file_desc_count * sizeof (FILE_DESC),
	   scrollback_count, sizeof (SCROLLBACK), scrollback_count * sizeof (SCROLLBACK));
  stt (buf, th); 
  
  sprintf (buf, "[%5d] Rumors    @ %4d B: %-7d[%5d] Raids     @ %4d B: %-4d\n\r",
	   rumor_count, sizeof (RUMOR), rumor_count * sizeof (RUMOR), 
	   raid_count, sizeof (RAID), raid_count * sizeof(RAID));
 stt (buf, th); 
 
#ifdef USE_WILDERNESS
  sprintf (buf, "[%5d] Events    @ %4d B: %-7d[%5d] Populs    @ %4d B: %-4d\n\r",
	   event_count, sizeof (EVENT), event_count * sizeof (EVENT),
	   popul_count, sizeof(POPUL), popul_count * sizeof(POPUL));
  stt (buf, th); 
#else
  sprintf (buf, "[%5d] Events    @ %4d B: %-7d [%5d] Edescs    @ %4d B: %-7d\n\r",
	   event_count, sizeof (EVENT), event_count * sizeof (EVENT),
	   edesc_count, sizeof (EDESC), edesc_count * sizeof (EDESC)
	   );
  stt (buf, th); 
#endif
  
  
  sprintf (buf, "[%5d] Needs     @ %4d B: %-7d[%5d] Socials   @ %4d B: %-4d\n\r",
	   need_count, sizeof (NEED), need_count * sizeof (NEED),
	   social_count, sizeof (SOCIAL), social_count*sizeof (SOCIAL));
  stt (buf, th); 
  
  sprintf (buf, "\n\n\rTotal string size used: %d\n\r", string_count);
  stt (buf, th);
  
  
  total_memory_used = find_total_memory_used ();
  sprintf (buf, "\n\n\rTotal estimated memory usage is: %d.\n\r",
	   total_memory_used);
  
  
  stt (buf, th);
  return;
}

/* This calculates the total memory used (approx) within the game. */

int 
find_total_memory_used (void)
{
  int total = 0, mallocs = 0;

  
  total += thing_count * sizeof (THING) + value_count * sizeof (VALUE);
  mallocs += thing_count + value_count;
  total += flag_count * sizeof (FLAG) + fight_count * sizeof (FIGHT);
  mallocs += flag_count + fight_count;
  total += clan_count * sizeof (CLAN) + note_count * sizeof (NOTE);
  mallocs += clan_count + note_count;
  total += race_count * sizeof (RACE) + spell_count * sizeof (SPELL);    
  mallocs += race_count + spell_count;

  total += help_count * sizeof (HELP) + bfs_count * sizeof (BFS);
  mallocs += help_count + bfs_count;
  total += cmd_count * sizeof (CMD) + help_key_count * sizeof (HELP_KEY);
  mallocs += cmd_count + help_key_count;
  
  total += code_count * sizeof (CODE) + trigger_count * sizeof (TRIGGER);
  mallocs += code_count + trigger_count;
  
  total += auction_count * sizeof (AUCTION) + script_event_count * sizeof (SCRIPT_EVENT);
  mallocs += auction_count + script_event_count;
  
  total += reset_count * sizeof (RESET) + pc_count * sizeof (PCDATA);
  mallocs += reset_count + pc_count;
  total += chan_count * sizeof (CHAN) + track_count * sizeof (TRACK);
  mallocs += chan_count + track_count;


  total += + society_count * sizeof (SOCIETY) + mapgen_count * sizeof (MAPGEN); 
  mallocs += society_count + mapgen_count;
  total += pbase_count * sizeof (PBASE) + siteban_count * sizeof (SITEBAN); 
  mallocs += pbase_count + siteban_count;

  total += file_desc_count * sizeof (FILE_DESC) + scrollback_count * sizeof (SCROLLBACK);
  mallocs += file_desc_count + scrollback_count;
  
  total += rumor_count * sizeof (RUMOR) + raid_count * sizeof (RAID); 
  mallocs += rumor_count + raid_count;
  total += event_count * sizeof (EVENT);
  mallocs += event_count;
  total += edesc_count * sizeof(EDESC);
  mallocs += edesc_count;
#ifdef USE_WILDERNESS
  total += + popul_count * sizeof (POPUL);
  mallocs += popul_count;
#endif
  total += need_count * sizeof (NEED) + social_count * sizeof (SOCIAL); 
  mallocs += need_count + social_count;
  
  total += string_count;
  /* Close...but not quite. */
  total += 8*mallocs;
  total_memory_used = total;
  return total; 
}
  
void 
do_users (THING *th, char *arg)
{
  char buf[STD_LEN];
  FILE_DESC *fd;
  bigbuf[0] = '\0';
  stt("\n\n\rThis is a list of everyone who is connected at the moment:\n\n\r-------------------------------------------------------------\n\n\r", th);
  
  for (fd = fd_list; fd; fd = fd->next)
    {
      sprintf (buf, "---> %-15s                 %-15s    %s\n\r", 
	       (fd->th && fd->th->name ? fd->th->name : "no_name"), fd->num_address, "");
      stt (buf, th);
    }
  return;
}


void 
do_who (THING *th, char *arg)
{
  char buf[STD_LEN];
  FILE_DESC *fd;
  int pass, type = -2, i, clantype = CLAN_MAX;
  RACE *align;
  char alignbuf[STD_LEN];
  bool found = FALSE, name = FALSE;
  THING *vict;
  CLAN *clan = NULL;
  
  if (!IS_PC (th) || !th->fd)
    return;
  
  /* First we check if the person is max level or not. If they are,
     we can then choose a single alignment to look at, or we can 
     choose a clan or sect or something to look at, or we can choose
     a specific name. */
  
  /* One of the things that has also interested me about the who function
     is the idea of maybe having short who lists...like where you just
     sit there and print out the name so you can see everyone on one
     screen. I was thinking about this going back to when I used to play
     on some of the other old old oldschool muds like some of the real
     old Dikus and Mercs, where you didn't see all the flash you have
     today, well you know sometimes I get frustrated with this overbearing
     flashiness and I just want to do_emote ("%@#^*/HJ/*$&2#*!"); to get
     all of my frustrations out over how overblown things get sometimes. Ah 
     well Ce'st la vie I guess. :) */ 
						      
						      
   if (LEVEL (th) == MAX_LEVEL &&
       (align = find_align (arg, (isdigit (*arg) ? atoi(arg) : -1))) != NULL)
     {
       found = TRUE;
       type = align->vnum;
     }
  if (!found)
    {
      for (i = 0; i < CLAN_MAX; i++)
	{
	  if (!str_cmp (arg, clantypes[i]))
	    {
	      clantype = i;
	      if ((clan = find_clan_in (th, clantype, FALSE)) == NULL)
		{
		  sprintf (buf, "You aren't a member of any %s.\n\r", clantypes[clantype]);
		  stt (buf, th);
		  return;
		}
	      found = TRUE;
	      type = -2;
	      break;
	    }
	}
    }
  if (!found && !str_cmp (arg, "staff"))
    {
      type = -1;
      found = TRUE;
    }
  else if (LEVEL (th) < BLD_LEVEL)
    type  = -2;
  
  if (!found && *arg)
    {
      found = TRUE;
      name = TRUE;
    }
  
  if (LEVEL (th) > MORT_LEVEL && LEVEL (th) < MAX_LEVEL)
    type = -1;
  for (pass = -1; pass <= top_align; pass++)
    {
      if (pass == -1)
	{
	  bigbuf[0] = '\0';
	  add_to_bigbuf ("\n\r\x1b[1;30m[\x1b[1;37m+\x1b[1;30m]----------\x1b[0;37m----------\x1b[1;37m-----------------------\x1b[0;37m----------\x1b[1;30m----------[\x1b[1;37m+\x1b[1;30m]\x1b[0;37m\n\r");
	  add_to_bigbuf ("                         ");
	  sprintf (buf, "%s\n\r", game_name_string);
	  add_to_bigbuf (buf);
	  add_to_bigbuf ("\x1b[1;30m[\x1b[1;37m+\x1b[1;30m]----------\x1b[0;37m----------\x1b[1;37m-----------------------\x1b[0;37m----------\x1b[1;30m----------[\x1b[1;37m+\x1b[1;30m]\x1b[0;37m\n\r");
	}
      if (pass == 0)
	{
	  add_to_bigbuf ("\x1b[1;30m[\x1b[1;37m+\x1b[1;30m]----------\x1b[0;37m----------\x1b[1;37m-----------------------\x1b[0;37m----------\x1b[1;30m----------[\x1b[1;37m+\x1b[1;30m]\x1b[0;37m\n\r");
	}
      for (fd = fd_list; fd != NULL; fd = fd->next)
	{
	  if (fd->connected > CON_ONLINE ||
	      (vict = fd->th) == NULL ||
	      !IS_PC (vict))
	    continue;
	  if ((pass == -1) && LEVEL (vict) < BLD_LEVEL)
	    continue;
	  if ((pass != -1) && LEVEL (vict) >= BLD_LEVEL)
	    continue;
	  if (pass >= 0 && vict->align != pass)
	    continue;
	  if ((type != -2 && pass != type) ||
	      !can_see (th, vict) ||
	      (LEVEL (th) < BLD_LEVEL && 
	       DIFF_ALIGN (vict->align, th->align) &&
	       !DETECT_OPP && LEVEL (vict) < MAX_LEVEL) ||
	      (clan && clan != find_clan_in (vict, clantype, FALSE)) ||
	      (name && str_prefix (arg, NAME (vict))))
	    continue;
	  if (LEVEL (th) == MAX_LEVEL)
	    sprintf (alignbuf, "\x1b[1;31m%2d %d %d", vict->pc->remorts, vict->align, vict->pc->race);
	  else
	    alignbuf[0] = '\0';
	  sprintf (buf, "\x1b[1;34m[\x1b[1;36m%-3d %s\x1b[1;34m]\x1b[0;37m %s %s\n\r", 
		   LEVEL (vict), alignbuf, NAME (vict), vict->pc->title);
	  add_to_bigbuf (buf);
	}
    }
  add_to_bigbuf ("\x1b[1;30m[\x1b[1;37m+\x1b[1;30m]----------\x1b[0;37m----------\x1b[1;37m-----------------------\x1b[0;37m----------\x1b[1;30m----------[\x1b[1;37m+\x1b[1;30m]\x1b[0;37m\n\r");
  send_bigbuf (bigbuf, th);
  return;
}

void
read_score (void)
{
  FILE *f;
  if (score_string)
   free_str (score_string);
  score_string = nonstr;
  if ((f = wfopen ("score.dat", "r")) == NULL)
    {
      if ((f = wfopen ("score.dat", "w")) != NULL)
	{
	  fprintf (f, "Name: @nm@    Level: @lv@\n\n");
	  fclose (f);
	}
      score_string = new_str ("There is no score string. Tell an admin.\n\r");
      return;
    }
  score_string = new_str (read_string (f));
  fclose (f);
  return;
}

void
do_score (THING *th, char *arg)
{
  if (!IS_PC (th))
    {
      stt ("Players only!\n\r", th);
      return;
    }
  if (LEVEL (th) == MAX_LEVEL && !str_cmp (arg, "reload"))
    {
      read_score ();
      stt ("Ok, score string reloaded.\n\r", th);
      return;
    }
  stt (string_parse (th, score_string), th);
  return;
}


void
do_title (THING *th, char *arg)
{
  char *t;
  int len = 0;
  char buf[STD_LEN * 3];

  if (!IS_PC (th))
    return;
  
  if (!arg || !*arg)
    {
      stt ("Ok clearing title.\n\r", th);
      free_str (th->pc->title);
      th->pc->title = nonstr;
      return;
    }
  
  strcpy (buf, add_color (arg));
  
  if (strlen (buf) > 200)
    {
      stt ("Your title cannot be more than 200 characters long when the color codes are decompressed.\n\r", th);
      return;
    }
  
  for (t = buf; *t; t++)
    {
      if (*t == '\x1b')
	{
	  while (LC (*t) != 'm')
	    t++;
	}
      else
	len++;
    }
  
  if (len > 55)
    {
      stt ("You may have no more than 55 viewable characters in your title.\n\r", th);
      return;
    }
  
  
  free_str (th->pc->title);
  th->pc->title = new_str (buf);
  stt ("Title set.\n\r", th);
  return;
}


void
do_where (THING *th, char *arg)
{
  THING *room, *area, *thg, *oroom, *oarea, *area2;
  char buf[STD_LEN];
  bool found = FALSE, all = FALSE;
  SOCIETY *soc;
  
  if ((room = th->in) == NULL ||  !IS_PC (th))
    return;
  
  if (!IS_ROOM (room))
    {
      sprintf (buf, "\n\rYou are in %s.\n\r", show_build_name (room));
      stt (buf, th);
      return;
    }

  if ((area = room->in) == NULL || !IS_AREA (area))
    {
      stt ("Some kind of error. Your room is not in an area.\n\r", th);
      return;
    }
  
  if (LEVEL (th) == MAX_LEVEL && !str_cmp (arg, "all"))
    all = TRUE;
  
  
 


      
 sprintf (buf, "You are in %s, created by %s\n\n\rThe following players are near you:\n\r", NAME (area), area->type);
  stt (buf, th);

  
  /* NOTE THAT YOU CAN SEE LINKDEAD CHARS!!!!! */

  for (thg = thing_hash[PLAYER_VNUM % HASH_SIZE]; thg != NULL; thg = thg->next)
    {
      if (!IS_PC (thg) ||  thg == th || !can_see (th, thg) ||
	  (DIFF_ALIGN (th->align, thg->align) && LEVEL (th) < MAX_LEVEL) ||
	  (oroom = thg->in) == NULL ||
	  !IS_ROOM (oroom) ||
	  (oarea = oroom->in) == NULL ||
	  !IS_AREA (oarea) ||
	  (!all && oarea != area))
	continue;
      found = TRUE;
      if (LEVEL(th) == MAX_LEVEL)
        sprintf (buf, "[%6d] %15s ---- %s\n\r", oroom->vnum, NAME(thg), 
		 show_build_name (oroom));
      else       
        sprintf (buf, "%-15s ---- %s\n\r", NAME (thg), 
		 show_build_name (oroom));
      stt (buf, th);
    }
  if (!found)
    {
      stt ("Noone.\n\r", th);
    }  /* List all enemy/friend societies nearby. */
  
  stt ("\n\n\r", th);
  for (soc = society_list; soc; soc = soc->next)
    {
      if ((area2 = find_area_in (soc->room_start)) != area)
	continue;
      sprintf (buf, "There are some ");
      
      if (soc->align > 0 && !DIFF_ALIGN (soc->align, th->align))
	strcat (buf, "\x1b[1;32mFriendly ");
      else
	strcat (buf, "\x1b[1;31mHostile ");
      strcat (buf, "\x1b[1;36m");
      if (soc->adj && *soc->adj)
	{
	  strcat (buf, soc->adj);
	  strcat (buf, " ");
	}
      if (soc->pname && *soc->pname)
	strcat (buf, soc->pname);
      else
	strcat (buf, "Creatures");
      strcat (buf, " \x1b[0;37mnearby.\n\r");
      stt (buf, th);
    }
  return;
}

void
do_prompt (THING *th, char *arg)
{
  char buf[STD_LEN * 5];
  if (!IS_PC (th))
    {
      stt ("Only pc's have prompts.\n\r", th);
      return;
    }

  if (!*arg)
    {
      stt ("prompt <string>\n\r", th);
      return;
    }

  if (!str_cmp (arg, "default"))
    {
      free_str (th->pc->prompt);
      th->pc->prompt = new_str (DEFAULT_PROMPT);
      stt ("Ok, default prompt set.\n\r", th);
      return;
    }
  if (strlen (arg) > 150)
    {
      stt ("Your prompt string can be no more than 150 characters long.\n\r", th);
      return;
    }
  strcpy (buf, string_parse (th, arg));
  if (strlen (buf) > 400)
    {
      stt ("Your total prompt length may be no longer than 400 characters.\n\r", th);
      return;
    }
  free_str (th->pc->prompt);
  if (isspace(*arg))
    arg++;
  th->pc->prompt = new_str (arg);
  stt ("Ok, prompt set.\n\r", th);
  return;
}

void
do_attributes (THING *th, char *arg)
{
  int i, statval;
  char buf[STD_LEN];
  char buf2[100];
  if (!th || !IS_PC (th))
    return;

  
  stt ("\x1b[1;34m-------------------------------------------\x1b[0;37m\n\n\r", th);
  for (i = 0; i < STAT_MAX; i++)
    {
      statval  = get_stat (th, i);
      if (LEVEL (th) >= NUMERIC_STATS_SEEN)
	sprintf (buf2, "%d", statval);
      else if (statval < 6)
	sprintf (buf2, "Awful");
      else if (statval < 9)
	sprintf (buf2, "Poor");
      else if (statval < 12)
	sprintf (buf2, "Fair");
      else if (statval < 15)
	sprintf (buf2, "Moderate");
      else if (statval < 18)
	sprintf (buf2, "Good");
      else if (statval < 21)
	sprintf (buf2, "Very Good");
      else if (statval < 24)
	sprintf (buf2, "Excellent");
      else if (statval < 27)
	sprintf (buf2, "Superb");
      else 
	sprintf (buf2, "Amazing");
      
      sprintf (buf, "\x1b[1;34m[\x1b[1;37m%-20s\x1b[1;34m]\x1b[0;37m:   \x1b[1;36m%s\x1b[0;37m\n\r", stat_name[i], buf2);
      stt (buf, th);
    }
  
  stt ("\n\r\x1b[1;34m-------------------------------------------\x1b[0;37m\n\r", th);
  return;
}


void
do_pagelength (THING *th, char *arg)
{
  int len;
 
  len = atoi (arg);
  
  if (!IS_PC (th))
    return;
  
  if (len < 10 || len > 80)
    {
      stt ("Your pagelength must be between 10 and 80.\n\r", th);
      return;
    }
  
  th->pc->pagelen = len;
  stt ("Pagelength set.\n\r", th);	
  return;
}
