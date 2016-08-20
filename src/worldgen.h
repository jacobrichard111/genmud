



/* The world is maxxed out at NxN areas. */
/* If you change this number it must be congruent to 1 (mod 4). In
   other words, it must be 4k+1 for some positive integer k. */
#define WORLDGEN_MAX     33

/* The size of each of the worldgen areas. */
#define WORLDGEN_AREA_SIZE  500

/* This how far away in x and y dirs cities on the map must be from
   each other. */

#define WORLDGEN_CITY_DISTANCE 2



/* The start vnums for the upper and lower worldgen areas. */
/* Note that this may overlap with the wilderness area if the numbers
   are too big. Deal with it by making the vnum size a bit smaller. */
#define WORLDGEN_START_VNUM             200000
#define WORLDGEN_VNUM_SIZE              1000000
#define WORLDGEN_UNDERWORLD_START_VNUM  (WORLDGEN_START_VNUM+WORLDGEN_VNUM_SIZE)
#define WORLDGEN_END_VNUM (WORLDGEN_UNDERWORLD_START_VNUM+WORLDGEN_VNUM_SIZE)

/* All areas below this level get all aggros removed from them, and they're
   nosettle so societies don't go there as much. Should make it easier
   on newbies. */
#define WORLDGEN_AGGRO_AREA_MIN_LEVEL  40

/* Level of the areas outside of the outposts. */

#define WORLDGEN_OUTPOST_GATE_AREA_LEVEL  10

/* This generates a world. */
void do_worldgen (THING *, char *);
void worldgen (THING *, char *);

/* This checks if there are any areas within the worldgen 
   range. */

bool worldgen_check_vnums (void);

/* This removes an object if it's an object from a worldgen area. */

bool worldgen_remove_object (THING *obj);

/* This removes all worldgen objects from all players online or on disk. */

void worldgen_clear_player_items (void);

/* This nukes all areas within the worldgen range so a new worldgen can
   be made next reboot. */

void worldgen_clear (void);

/* This sets up the worldgen sectors by trying to match adjacent 
   sectors to what they want to be next to. */

void worldgen_add_sector (int x, int y);

/* This checks if the worldgen sectors are attached or not. */

bool worldgen_sectors_attached (void);

/* Check if an individual sector is attached. */

void worldgen_check_sector_attachment(int x, int y);

/* This sets the levels of the worldgen areas. */

void worldgen_generate_area_levels (void);


/* This shows the sectors to a player. */

void worldgen_show_sectors (THING *th);

/* This generates the actual areas to be used in the worldgen. */


void worldgen_generate_areas (int area_size);

/* This links the pieces of the map together. */

void worldgen_link_areas (void);

/* This links up 2 individual areas. */


void worldgen_add_exit (THING *area, int dir_to, int x, int y,  bool underwld);

/* This puts a link between the area in the top world and the
   area in the underworld below it. */

void worldgen_link_above_to_below (THING *top_area, THING *bottom_area);

/* This seeds the world with societies that have room flags as part of
   their definition and which aren't destructible. */

void worldgen_society_seed(void);

/* This finds areas on the edge of the worldgen map (just as it's being
   created) so that we can use these to link the alignment outposts to
   the world. */

THING *find_area_on_edge_of_world (int dir);

/* This links the alignment outposts to the world. */

void worldgen_link_align_outposts (void);

/* This adds catwalks between areas. */

void worldgen_add_catwalks_between_areas (void);

/* Create and clear quests */
void worldgen_clear_quests (THING *);
void worldgen_generate_quests (void);

/* This checks if the thing needs to have worldgen objects reset on it. */

void worldgen_check_resets (THING *);

/* This clears all questflags a player has (used on remort) so that
   the player can redo the quests. */

void clear_player_worldgen_quests (THING *);

/* This checks if the world will be auto worldgenned on reboot or not. */

void worldgen_check_autogen (void);

/* This gives the number of worldgen grid areas (TOP level so mult by 2 for
   below level..*/

int worldgen_num_areas (void);

/* This generates demons and demon areas. */

void worldgen_generate_demons (int curr_underworld_vnum, int area_size);

/* This removes aggros from newbie areas so players have a half a 
   chance. */

void setup_newbie_areas (void);


/* This creates and places guildmasters around the world. Newbie
   guildmasters go near homelands, midlevel ones go near midlevel 
   areas and highlevel guildmasters go near highlevel areas. */

void worldgen_place_guildmasters (void);


/* This adds a guildmaster of the proper guild and rank to the 
   given area. */

void add_guildmaster_to_area (THING *area, int guild, int rank);


/* This links special rooms to the world like implant/remort rooms. */

void worldgen_link_special_room (int special_room_vnum);


/* This finds the number of generator words there are in the world. */

int find_num_gen_words (THING *);

