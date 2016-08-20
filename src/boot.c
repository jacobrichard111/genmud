#include <sys/types.h> 
#include <signal.h>
#include <ctype.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include "serv.h"
#include "note.h"
#include "society.h"
#include "script.h"
#include "track.h"
#include "mapgen.h"
#include "rumor.h"
#include "event.h"
#ifdef USE_WILDERNESS
#include "wilderness.h"
#include "wildalife.h"
#endif
#include "worldgen.h"
#include "historygen.h"
#include "citygen.h"
#include "craft.h"

int listen_socket = 0;
int max_fd = 0;
int max_players = 0;
int server_flags = 0;
int startup_time = 0;
int boot_time = 0;
int current_time = 0;
int total_memory_used = 0;
int current_damage_amount = 0;
int png_seed = 0;
int png_count = 0;
SOCIETY *society_list = NULL;
int top_society = 0;
int society_count = 0;
SOCIETY *society_hash[SOCIETY_HASH];
RAID *raid_hash[RAID_HASH];
int png_bytes = 0;
int last_thing_edit;
int curr_parse_number = 0;
int spells_known[SPELLT_MAX];
FILE_DESC *fd_list = NULL;
FILE_DESC *fd_free = NULL;
CMD *com_list[256];
THING *thing_hash[HASH_SIZE];
THING *thing_hash_pointer = NULL;
THING *thing_cont_pointer = NULL;
CMD *cmd_free = NULL;
char prev_command[STD_LEN];
char bigbuf[BIGBUF];
int bigbuf_length = 0;
int room_count[AREA_MAX];
THING *thing_lock = NULL;
HELP_KEY *help_hash[27][27];
SOCIAL *social_hash[256];
SPELL *spell_name_hash[256];
SPELL *spell_number_hash[256];
CLAN *clan_list[CLAN_MAX];
THING *hunting_victim = NULL;
char *score_string =NULL;
int min_hunting_depth = MAX_HUNT_DEPTH;
bool used_spell[MAX_SPELL];
int num_prereqs[SPELLT_MAX];
bool draw_line[MAX_PRE_DEPTH];
THING *thing_free = NULL;
THING *thing_free_track = NULL;
int thing_free_count = 0;
VALUE *value_free = NULL;
VALUE *wt_info = NULL;
PCDATA *fake_pc;
FLAG *flag_free = NULL;
AUCTION *auction_free = NULL;
FIGHT *fight_free = NULL;
RESET *reset_free = NULL;
PCDATA *pc_free = NULL;
CLAN *clan_free = NULL;
EDESC *edesc_free = NULL;
TROPHY *trophy_free = NULL;
SPELL *spell_free = NULL;
SOCIAL *social_list = NULL;
SOCIAL *social_free = NULL;
SCROLLBACK *scrollback_free = NULL;
RAID *raid_free = NULL;
THING *the_world = NULL;
THING *fight_list = NULL;
THING *fight_list_pos = NULL; /* Needed to traverse fight list...*/
THING *thing_free_pc = NULL;
HELP *help_list = NULL;
HELP *help_free = NULL;
int event_count = 0;
EVENT *event_free = NULL;
EVENT *event_hash[EVENT_HASH];
DAMAGE *dam_list = NULL;
CHAN *channel_list = NULL;
RACE *race_info[RACE_MAX];
RACE *align_info[ALIGN_MAX];
SPELL *spell_list = NULL;
AUCTION *auction_list = NULL;
PBASE *pbase_list = NULL;
SITEBAN *siteban_list = NULL;
int cons_hp = 0;
bool consid = FALSE;
char pfile_name[40];
int currchan = 0;
int top_vnum = 0;
int top_race = 0;
int top_align = 0;
int top_auction = 0;
int thing_count = 0;
int thing_made_count = 0;
int value_count = 0;
int flag_count = 0;
int fight_count = 0;
int reset_count = 0;
int pc_count = 0;
int clan_count = 0;
int race_count = 0;
int spell_count = 0;
int help_count = 0;
int file_desc_count = 0;
int cmd_count = 0;
int help_key_count = 0;
int edesc_count = 0;
int auction_count = 0;
int chan_count = 0;
int channel_count = 0;
int string_count = 0;
int siteban_count = 0;
int pbase_count = 0;
int social_count = 0;
int scrollback_count = 0;
int reboot_ticks = 0;
int seg_count = 0;
int raid_count = 0;
int lowest_vnum = 200000000;
int top_clan[CLAN_MAX];
int alliance[ALIGN_MAX];
int top_spell = 0;
int bfs_times_count = 1;
int bfs_tracks_count = 0;
int before_trig = TRIG_MOVING | TRIG_MOVED | TRIG_TAKEN_FROM | TRIG_LEAVE | TRIG_TAKEN;
int after_trig = TRIG_GIVEN | TRIG_DROPPED | TRIG_GET | TRIG_ENTER | TRIG_ARRIVE;
char *nonstr;
char nothing_string[1];
THING *thing_nest[MAX_NEST];
char map[MAP_MAXX][MAP_MAXY];
char col[MAP_MAXX][MAP_MAXY];
char exi[MAP_MAXX][MAP_MAXY];
char pix[MAP_MAXX][MAP_MAXY];
PKDATA pkd[PK_MAX][PK_LISTSIZE];
int bg_hours = 0;       /* Hours to bg. */
int bg_minlev = 0;      /* Min level for this bg. */
int bg_maxlev = 0;      /* Max level for this bg. */
int bg_money = 0;       /* Cash prize for bg. */
THING *bg_prize[BG_MAX_PRIZE]; /* Prizes for the bg. */


void
init_variables (void)
{
  int i, j;
  nothing_string[0] = '\0';
  nonstr = &nothing_string[0];
  for (i = 0; i < 256; i++)
    {
      com_list[i] = NULL;
      spell_name_hash[i] = NULL;
      spell_number_hash[i] = NULL;
      social_hash[i] = NULL;
    }
  for (i = 0; i < HASH_SIZE; i++)
    {
      thing_hash[i] = NULL;
      trigger_hash[i] = NULL;
    }
  for (i = 0; i < CODE_HASH; i++)
    {
      for (j = 0; j < CODE_HASH; j++)
	{
	  code_hash[i][j] = NULL;
	}
    }
  for (i = 0; i < 27; i++)
    {
      for (j = 0; j < 27; j++)
	{
	  help_hash[i][j] = NULL;
	}
    }
  for (i = 0; i < SOCIETY_HASH; i++)
    society_hash[i] = NULL;
  for (i = 0; i < RAID_HASH; i++)
    raid_hash[i] = NULL;
  for (i = 0; i < EVENT_HASH; i++)
    event_hash[i] = NULL;
  for (i = 0; i < SPELLT_MAX; i++)
    num_prereqs[i] = 0;
  for (i = 0; i < MAX_PRE_DEPTH; i++)
    draw_line[i] = FALSE;
    alliance[i] = (1 << i);
  for (i = 0; i < MAX_SPELL; i++)
    used_spell[i] = FALSE;
  for (i = 0; i < MAX_NEST; i++)
    thing_nest[i] = NULL;
  for (i = 0; i < CLAN_MAX; i++)
    clan_list[i] = NULL;
  for (i = 0; i < RACE_MAX; i++)
    race_info[i] = NULL;
  for (i = 0; i < ALIGN_MAX; i++)
    align_info[i] = NULL;
  for (i = 0; i < BG_MAX_PRIZE; i++)
    bg_prize[i] = NULL;
  for (i = 0; i < AREA_MAX; i++)
    room_count[i] = 0;

  for (i = 0; i < PK_MAX; i++)
    {
      for (j = 0; j < PK_LISTSIZE; j++)
	{
	  strcpy (pkd[i][j].name, "<Free-Slot>");
	  pkd[i][j].value = 0.0;
	  pkd[i][j].align = 0;
	}
    }
  
  prev_command[0] = '\0';
  init_historygen_vars();
  for (i = 0; i < CLAN_MAX; i++)
    top_clan[i] = 0;
  last_thing_edit = current_time;
  the_world = new_thing (); 
  the_world->vnum = WORLD_VNUM;
  the_world->thing_flags |= TH_IS_AREA;
  the_world->name = new_str ("The World");
  the_world->short_desc = new_str ("This is the world!!!");
  the_world->cont = NULL;
  the_world->max_mv = 1;
  the_world->mv = 1;  
  fake_pc = new_pc ();
  if (!the_world)
    {
      log_it ("ACK! Could not set up the_world!\n");
      exit (1);
    }

  /* Some sanity checks */

  if (sizeof(int) < 4)
    {
      log_it ("ints must be at least 4 bytes.\n");
      exit(1);
    }
  if (sizeof (short) < 2)
    {
      log_it ("shorts must be at least 2 bytes.\n");
      exit(1);
    }

  if (ALIGN_MAX > 32)
    {
      log_it ("ALIGN_MAX cannot be more than 32 or else relic raids won't work.\n");
      exit (1);
    }

  if (caste1_flags[0].flagval != CASTE_CHILDREN)
    {
      log_it ("CASTE_CHILDREN must be the first flag in the caste1_flags[] array.\n\r");
      exit (1);
    }

  if (STD_LEN < 1000)
    {
      log_it ("Warning. This code uses fixed string buffers.\n\r");
      log_it ("I have tried to make sure that there won't be overruns\n\r");
      log_it ("But if you make this STD_LEN constant too small, it\n\r");
      log_it ("will make it much more likely that there will be\n\r");
      log_it ("crashes and other bugs associated with the shortened\n\r");
      log_it ("buffer sizes. So, the server is getting shut down.\n\r");
      exit(1);
    }
  
  if (money_info[0].flagval != 1)
    {
      log_it ("Coin screwup, money_info first flagval is not one.\n");
      exit (1);
    }
  for (i = 0; i < NUM_COINS; i++)
    {
      if (money_info[i].flagval == 0)
	{
	  log_it ("Money multiplier in money_info is 0!\n");
	  exit (1);
	}
      if (i > 1 && money_info[i].flagval % money_info[i - 1].flagval != 0)
	{
	  log_it ("Money multipliers in money_info are not exact multiples of each other!\n");
	  exit (1);
	}
    }

  /* This section checks to see if the value_list and flag_list .app's 
     don't have whitespace or END_STRING_CHAR's or else it screws
     up the file reading and writing. */
  
  {
    bool bad_word = FALSE;
    char *t;
    for (i = 0; i < VAL_MAX; i++)
      {
	for (t = (char *) value_list[i]; *t; t++)
	  {
	    if (isspace(*t) || *t == END_STRING_CHAR)
	      {
		char errbuf[STD_LEN];
		sprintf (errbuf, "Value List number %d App '%s' has space or %c in it.\n\r", i, value_list[i], END_STRING_CHAR);
		log_it (errbuf);
		bad_word = TRUE;
	      }
	  }
      }
    
    for (i = 0; i < NUM_FLAGTYPES; i++)
      {
	for (t = flaglist[i].name; *t; t++)
	  {
	    if (isspace(*t) || *t == END_STRING_CHAR)
	      {	
		char errbuf[STD_LEN];
		sprintf (errbuf, "Flaglist number %d Name '%s' has space or %c in it.\n\r", i, flaglist[i].name, END_STRING_CHAR);
		log_it (errbuf);
		bad_word = TRUE;
	      }
	  }
      }
    
    for (i = 0; affectlist[i].flagval != 0; i++)
      {
	for (t = affectlist[i].name; *t; t++)
	  {
	    if (isspace(*t) || *t == END_STRING_CHAR)
	      {
		char errbuf[STD_LEN];
		sprintf (errbuf, "Affectlist number %d Name '%s' has space or %c in it.\n\r", i, affectlist[i].name, END_STRING_CHAR);
		log_it (errbuf);
		bad_word = TRUE;
	      }
	  }
      }
    
    if (bad_word)
      {
	log_it ("YOU HAVE BAD WORDS IN YOUR AFFECT, FLAG, OR VALUE STRUCTS AND MUST FIX THEM!!!\n\r");
	exit(1);
      }
    clear_city_grid (FALSE);
    return;
  }
  
  
  
  return;
}

void
read_server (void)
{
  log_it ("Bootup");
  server_flags = 0;
  SBIT (server_flags, SERVER_BOOTUP);
  init_variables ();
  read_wizlock();
  init_command_list ();
  read_sitebans (); 
  read_history_data ();
  read_time_weather(); 
  read_socials (); 
  read_score (); 
  read_notes ();
  read_damage (); 
  read_helps ();
  read_alliances (); 
  read_channels (); 
  read_spells ();
  read_pbase (); 
  read_races (); 
  read_aligns ();
  read_areas (); 
  /*  generate_modnar (); */
  set_up_areas (); 
  set_up_map (NULL);
  read_codes (); 
  read_triggers ();
  read_societies ();
  read_pkdata ();
  read_things ();
  read_clans ();
  read_rumors();
  read_raids();
  reset_world ();
  add_craft_commands();
#ifdef USE_WILDERNESS
  read_wildalife();
#endif
  setup_timed_triggers ();
  make_forged_eq ();
  set_up_global_events();
  set_up_teachers();
  sanity_check_vars();
  worldgen_check_autogen();
  calc_max_remort(NULL);
  RBIT (server_flags, SERVER_BOOTUP);
  return;
}

void
read_areas (void)
{
  char word[STD_LEN];
  FILE *f;
  int area_count = 0;
  THING *base_area, *base_room, *catchall;
  if ((f = wfopen ("areas.dat", "r")) == NULL)
    {
      /* Make up a fake vnum 1 and vnum 2 and vnum 770 
	 to bootstrap the game. */
      base_area = new_thing ();
      base_area->vnum = 1;
      base_area->thing_flags = TH_IS_AREA | TH_CHANGED;
      base_area->name = new_str ("start.are");
      base_area->short_desc = new_str ("The start area.");
      base_area->long_desc = new_str ("The start area resets.");
      base_area->mv = 100;
      base_area->max_mv = 999;
      add_thing_to_list (base_area);
      thing_to (base_area, the_world);
      base_room = new_thing();
      base_room->vnum = 2;
      base_room->short_desc = new_str ("The God Room");
      base_room->thing_flags = ROOM_SETUP_FLAGS;	
      thing_to (base_room, base_area);
      add_thing_to_list (base_room);
      catchall = new_thing();
      catchall->vnum = CATCHALL_VNUM;
      catchall->name = new_str ("catchall item");
      catchall->short_desc = new_str ("the catchall item");
      catchall->long_desc = new_str ("The catchall item is here.");
      thing_to (catchall, base_area);
      add_thing_to_list (catchall);
      add_events_to_thing(base_area);
#ifdef USE_WILDERNESS
      setup_wilderness_area(); 
#endif
      write_changed_areas(NULL);
      return;
    }
  
  while (TRUE)
    {
      strcpy (word, read_word(f));
      if (!str_cmp (word, "END_AREA_LIST") ||
	  ++area_count > MAX_AREAS)
	{
	  fclose (f);
	  break;
	}
      if (word[0])
	read_area (word);      
    }
#ifdef USE_WILDERNESS
  setup_wilderness_area();
#endif
  return;
}


void 
read_area (char *filename)
{
  THING *new_area = NULL;
  THING *newthing;
  FILE_READ_SETUP (filename);
  
  strcpy (word, read_word (f));
  if (!str_cmp (word, "THING"))    
    new_area = read_thing (f);
  if (!new_area)
    {
      log_it ("Could not find the area in the area file!\n");
      exit (1);
    }
  /* Put areas in in order of their vnums. */
  thing_to (new_area, the_world);
  new_area->name = new_str (filename);
  add_thing_to_list (new_area);
  add_events_to_thing (new_area);
  new_area->timer = nr (10, 20);
  /* Now make a new area thing referencing it and put the area into the world*/
  
  FILE_READ_LOOP
    {
      newthing = NULL; 
      FKEY_START;
      FKEY ("THING")
	{
	  if ((newthing = read_thing (f)) == NULL)
	    continue;
	  /* This thing_to must come before the add_thing_to_list
	     because events are added in add_thing_to_list,
	     and they check if a nonroom is inside of an area
	     (i.e. if the thing is a prototype or not...so that
	     those things don't get events. */
	  
	  thing_to (newthing, new_area);
	  add_thing_to_list (newthing);
	}
      FKEY ("END_OF_AREA")
	break;
      FILE_READ_ERRCHECK (filename);
    }
  fclose (f);
  return;
}

/* This saves a list of the filenames attached to the areas in the game. */

void
write_area_list (void)
{
  THING *ar;
  FILE *f;
  if (!the_world)
    return;
  if ((f = wfopen ("areas.dat", "w")) == NULL)
    return;
  for (ar = the_world->cont; ar != NULL; ar = ar->next_cont)
    {
      if (IS_PC (ar) || !IS_AREA (ar) ||
	  (ar != the_world->cont && 
	   IS_SET (ar->thing_flags, TH_NUKE)))
	continue;
      if (!ar->name || !ar->name[0])
	{
	  char filename[STD_LEN];
	  sprintf (filename, "area%d.are", ar->vnum);
	  ar->name = new_str (filename);
	}
      fprintf (f, "%s\n", ar->name);
    }
  fprintf (f, "\nEND_AREA_LIST\n");
  fclose (f);
  return;
}

/* This function saves all of the changed areas (and the area list). */

/* This could lead to a problem now that it's threaded, but it lags
   too bad so I don't know what to do. */
void
write_changed_areas (THING *th)
{
  pthread_t changed_thread;
  
  if (IS_SET (server_flags, SERVER_SAVING_AREAS))
    {
      stt ("Someone is already saving the area list.\n\r", th);
      return;
    }
  SBIT (server_flags, SERVER_SAVING_AREAS);  
  pthread_create (&changed_thread, NULL, write_changed_areas_real, (void *)th);
  pthread_detach (changed_thread);
  return;
}

void*
write_changed_areas_real (void *th)
{
  THING *ar;
   char buf[STD_LEN];
   write_area_list ();
   
   buf[0] = '\0';
   for (ar = the_world->cont; ar; ar = ar->next_cont)
     {
       if (IS_SET (ar->thing_flags, TH_CHANGED) &&
	   !IS_SET (ar->thing_flags, TH_NUKE))
	 {
	   write_area (ar);
	   set_up_map (ar);
	   /*	   if (th)
	     {
	       sprintf (buf, "%s saved\n\r", ar->name);
	       stt (buf, th);
	       } */
	 }
       RBIT (ar->thing_flags, TH_CHANGED);
     }
   RBIT (server_flags, SERVER_SAVING_AREAS);
   return NULL;
}
   
void 
write_area (THING *ar)
{
  FILE *f;
  char filename[STD_LEN];
  if (!ar->name || !*ar->name)
    {
      sprintf (filename, "area%d.are", ar->vnum);
      ar->name = new_str (filename);
    }
  if ((f = wfopen (ar->name, "w")) == NULL)
    exit (1);
  
  RBIT (ar->thing_flags, TH_CHANGED);
  write_thing (f, ar);
  fprintf (f, "END_OF_AREA\n\n");
  fclose (f);
}

void
set_up_areas (void)
{
  THING *ar, *room;
  int lvnum, uvnum;
  FLAG *rflags;
  
  /* Go through each area */
  for (ar = the_world->cont; ar != NULL; ar = ar->next_cont)
    {
      if (IS_PC (ar))
	continue;
      SBIT (ar->thing_flags, TH_IS_AREA);
      lvnum = ar->vnum;
      uvnum = ar->vnum + ar->mv;
      if (uvnum <= lvnum)
	continue;
      
      /* Go through each room */
      
      for (room = ar->cont; room; room = room->next_cont)
	{
	  if (room->vnum >= lvnum && room->vnum <= uvnum)
	    {
	      room->max_mv = 1;
	      room->mv = 1;
	      if (!IS_SET (room->thing_flags, TH_IS_ROOM))
		{
		  SBIT (room->thing_flags, TH_IS_ROOM);
		  SBIT (ar->thing_flags, TH_CHANGED);
		} /* Minerals in special rooms */
	      for (rflags = room->flags; rflags; rflags = rflags->next)
		if (rflags->type == FLAG_ROOM1 && rflags->timer == 0)
		  break;
	      if (rflags && IS_SET (rflags->val, ROOM_UNDERGROUND | ROOM_FOREST | ROOM_DESERT | ROOM_SWAMP | ROOM_ROUGH | ROOM_MOUNTAIN))
		rflags->val |= ROOM_MINERALS;
	    }
	}
    }
  return;
}

/* The next two functions save and load the world status..where everything
   happens to be. */

void 
read_things (void)
{
  VALUE *shop, *shop2;
  THING *keeper, *obj, *objn;
  int i;
  FILE_READ_SETUP ("worldinfo.dat");
  
  log_it ("Loading world status\n");
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY ("TH")
	read_short_thing (f, NULL, NULL);
      FKEY ("END_SHORT_THINGS")
	break;
      FILE_READ_ERRCHECK ("worldinfo.dat");
    }
  fclose (f);
  
  
  /* Now deal with shops resetting. */

  for (i = 0; i < HASH_SIZE; i++)
    {
      for (keeper = thing_hash[i]; keeper; keeper = keeper->next)
	{
	  
	  /* Ignore anything not in anything, anything in
	     an area, anything that is not a shop, or anything that
	     is a shop and is owned by a player. */

	  if (!keeper->in || IS_AREA (keeper->in) ||
	      (shop = FNV (keeper, VAL_SHOP)) == NULL ||
	      ((shop2 = FNV(keeper, VAL_SHOP2)) != NULL &&
	       shop2->word && shop2->word[0]))
	    {
	      worldgen_check_resets (keeper);
	      continue;
	    }
	  
	  for (obj = keeper->cont; obj; obj = objn)
	    {
	      objn = obj->next_cont;
	      
	      free_thing (obj);
	    }
	  
	  sub_money (keeper, total_money(keeper));
	  
	  reset_thing (keeper, 0);
	  
	  if (keeper->proto)
	    add_money (keeper, total_money (keeper->proto));
	      
	}
    }
  return;
}


/* This starts the thread' that is used to periodically save things. */

void
init_write_thread (void)
{
  pthread_t save_thread;
  
  if (IS_SET (server_flags, SERVER_SAVING_WORLD))
    return;
  SBIT (server_flags, SERVER_SAVING_WORLD);
  
  
  pthread_create (&save_thread, NULL, write_world_snapshot, NULL);
  pthread_detach (save_thread);
  return;
}

/* This loops forever and is used to save things within the game
   periodically. */


void *
write_world_snapshot (void *arg)
{ 
  FILE *f;
  THING *ar, *room;
  char buf[STD_LEN];
  
  if ((f = wfopen ("worldinfo.bak", "w")) == NULL)
    return NULL;

  /* Do this to try to avoid dupe bugs...if the players crash it and
     there's a problem in the world, this shouldn't work so they
     shouldn't get to have their dupe bug working, either. */
  
  sprintf (buf, "\\rm %sworldinfo.dat", WLD_DIR);
  system (buf);
  
  /* Loop through all areas and all rooms in each area and save
     each room/contents. */

  clear_save_flags();
  for (ar = the_world->cont; ar; ar = ar->next_cont)
    {
      SBIT (ar->thing_flags, TH_SAVED);
      for (room = ar->cont; room; room = room->next_cont)	
	{
	  if (IS_ROOM (room) && room->cont)
	    write_short_thing (f, room, 0);
	}
    }
  fprintf (f, "\nEND_SHORT_THINGS\n");
  fclose (f);
  clear_save_flags();
  
  /* Now copy the new backup over the old backup..and hope this
     doesn't fail!!! */
  sprintf (buf, "mv %sworldinfo.bak %sworldinfo.dat",
	   WLD_DIR, WLD_DIR);
  system (buf);
#ifdef USE_WILDERNESS
  write_wildalife();
#endif
  RBIT (server_flags, SERVER_SAVING_WORLD);
  return NULL;
}

/* This clears all of the flags off things so they can be saved 
   into the world snapshot. */

void
clear_save_flags(void)
{
  int i;
  THING *th;
  for (i = 0; i < HASH_SIZE; i++)
    for (th = thing_hash[i]; th; th = th->next)
      RBIT (th->thing_flags, TH_SAVED);
}


/* This sanity checks to make sure that if there are missing .dat files
   that the world can go on. */

void
sanity_check_vars(void)
{
  if (!score_string)
    score_string = new_str ("Name: @nm@\n\n");
  if (!race_info[0])
    {
      race_info[0] = new_race();
      race_info[0]->name = new_str ("Human");
    }
  if (!align_info[0])
    {
      align_info[0] = new_race();
      align_info[0]->name = new_str ("Neutral"); 
    }
  if (ALIGN_MAX > 1 && !align_info[1])
    {
      align_info[1] = new_race();
      align_info[1]->name = new_str ("Evil");
    }
  if (ALIGN_MAX > 2 && !align_info[2])
    {
      align_info[2] = new_race();
      align_info[2]->name = new_str ("Good");
    }

  
  if (!dam_list)
    {
      dam_list = new_damage();
      dam_list->low = 0;
      dam_list->high = 10000000;
      dam_list->message = new_str ("@1n hit@s @3f.");
    }
  if (!channel_list)
    {
      channel_list = new_channel();
      channel_list->name[0] = new_str ("say");
      channel_list->msg = new_str ("@1n say@s ");
      channel_list->flags = CHAN_TO_ROOM;
    }
  

}
void
do_reboo (THING *th, char *arg)
{
  if (!IS_PC (th))
    {
      stt ("Huh?\n\r", th);
      return;
    }
  stt ("You must type out reboot FULLY!! to get the server to reboot.\n\r", th);
  return;
}



void
do_reboot (THING *th, char *arg)
{
  if (!IS_PC (th))
    {
      stt ("Huh?\n\r", th);
      return;
    }
  
  if (reboot_ticks > 0)
    {
      if (!*arg || (is_number (arg) && atoi (arg) == 0))
	{
	  reboot_ticks = 0;
	  stt ("Reboot halted.\n\r", th);
	  return;
	}
      stt ("Reboot <hours> for timed reboot, Reboot for instant reboot, Reboot or Reboot 0 resets the timer to stop rebooting.\n\r", th);
      return;
    }
  
  if (!*arg)
    {
      SBIT (server_flags, SERVER_REBOOTING);
      RBIT (server_flags, SERVER_SPEEDUP);
    }
  else if (is_number (arg) && atoi (arg) > 0)
    {
      reboot_ticks = atoi (arg);
      stt ("Reboot hours set.\n\r", th);
      return;
    }
  return;
}
		     
void*
seg_handler (void)
{
  char errbuf[STD_LEN];
  int count = 0;
  if (++seg_count > 4)
    exit(SIGSEGV);
  sprintf (errbuf, "Ack! Segmentation fault number %d! Attempting to save everything!\n", seg_count);
  log_it (errbuf);
  log_it (prev_command);
  if (IS_SET (server_flags, SERVER_SAVING_WORLD))
    log_it ("Saving world.");
  if (IS_SET (server_flags, SERVER_SAVING_AREAS))
    log_it ("Saving areas.");
  shutdown_server ();
  /* Wait for world state save. */
  do
    sleep(1);
  while (IS_SET (server_flags, SERVER_SAVING_WORLD | SERVER_SAVING_AREAS) &&
	 ++count < 20);
  
  kill (getpid(), SIGSEGV);
  exit(SIGSEGV);
}


void
shutdown_server (void)
{
  FILE_DESC *fd, *fd_next;
  int count = 0;
  /* Save players */
  
  if (IS_SET (server_flags, SERVER_AUTO_WORLDGEN))
    {
      worldgen (NULL, "clear yes");
    }
  if (seg_count < 2)
    {
      log_it ("Shutdown\n");
      for (fd = fd_list; fd != NULL; fd = fd_next)
	{
	  fd_next = fd->next;
	  if (fd->th && fd->connected <= CON_ONLINE && fd->th->pc)
	    write_playerfile (fd->th);
	  do_cls (fd->th, "");
	  stt ("Shutting down...\n\r", fd->th);
	  fd->th = NULL;
	  close_fd (fd);
	}
      init_write_thread();
      write_societies ();
      write_changed_areas(NULL);
      write_codes ();
      write_triggers ();
      write_spells();
      write_clans ();
      write_races ();
      write_aligns ();
      write_notes ();
    }
  do
    sleep(1);
  while (IS_SET (server_flags, SERVER_SAVING_WORLD | SERVER_SAVING_AREAS) &&
	 ++count < 20);
  SBIT (server_flags, SERVER_REBOOTING);
  return;
}

   

/* Reads orig startup time for the game. */
void
read_startup (void)
{
  FILE *f;
  if ((f = wfopen ("startup.dat", "r")) == NULL)
    {
      startup_time = current_time;
      if ((f = wfopen ("startup.dat", "w")) == NULL)
	return;
      fprintf (f, "%d\n", startup_time);
      fclose (f);
      return;
    }
  startup_time = read_number (f);
  fclose (f);
  return;
}

/* This checks if the wizlock.dat file exists or not, and if so,
   the game is wizlocked. */

void
read_wizlock (void)
{
  FILE *f;
  
  if ((f = wfopen ("wizlock.dat", "r")) != NULL)
    {
      SBIT (server_flags, SERVER_WIZLOCK);
      fclose (f);
    }
  return;
}
    

