


/* This code makes cities. It starts with a NxNxH grid of empty rooms
   and sets up a base city shape by adding a block of rooms to the
   city grid.

   Then, it goes through and starts to add roads and alleyways 
   and gates inside the grid. It also sets up "walls" outside the
   city by adding a path around the city a few rooms deep, but only
   connects to the gates. 

   Then, it starts adding big blocks to the city like marketplace and
   governmental centers and so forth. It adds things like houses
   and towers and other things piece by piece.

*/

/* Cities are max CITY_SIZE across and CITY_HEIGHT tall. (You shouldn't use anywhere
   NEAR this number of rooms for a given city, however. */

#define CITY_SIZE     100
#define CITY_HEIGHT   10
#define CITYGEN_STREET_LEVEL  2 /* Street level is height 2...(allows for
				 sewers and such below the city). */

#define CITYGEN_UL_NAMES  30 /* A section can connect to at most
			      30 diff names of things one depth below
			      it. */


#define CITYGEN_FIELD_DEPTH  -1  /* Fields outside the city are set
				    to have a depth of -1. */

#define CITYGEN_GUARD_GATE  0
#define CITYGEN_GUARD_CITY  1
#define CITYGEN_GUARD_MAX   2 /* 2 types of guards...gateguards and
				 buffed guards. */

/* Min and max numbers of rooms in the city to generate. */
#define CITYGEN_MIN_SIZE  100
#define CITYGEN_MAX_SIZE  2000



extern THING *city_grid[CITY_SIZE][CITY_SIZE][CITY_HEIGHT];


extern char city_name[STD_LEN];
extern char city_full_name[STD_LEN];

/* This generates a city area. */

void citygen (THING *th, char *arg);

/* This clears the city grid...the variable tells if we nuke the things
   in it or not. */
void clear_city_grid (bool nuke_things);

/* This checks if a given coord is within the city grid limits. */
bool city_coord_is_ok (int x, int y, int z);

/* This generates a base city grid of rooms or a base block of rooms for
   a building at the given vnum and puts it into the area
   the start_vnum is in. */

void generate_base_city_grid (THING *start_obj, int start_vnum);


/* This adds a detail to a city. This is normally first called in
   citygen() by going down the list of words in the citygen area
   and looking for the rooms corresponding to those words. */

void citygen_add_detail (RESET *start_rst, THING *area_to, THING *to, VALUE *search_dims, 
			 int depth);

/* This finds the start location where the new detail will go. The 
   start_DIR variables are where the returned values are stored. */

void citygen_find_detail_start_location (RESET *rst, THING *area_to, THING *to, VALUE *search_dims, int depth, int *start_x, int *start_y, int *start_z);




/* This adds something like a road to an area. */

void citygen_add_stringy_detail (RESET *start_rst, THING *area, THING *to, VALUE *search_dims, int depth);

/* This shows the city map to a player (street level only) */

void show_city_map (THING *th);



/* This connects blocks of rooms inside of the city. */

void citygen_connect_same_level (int depth, VALUE *dims);

/* This links details at a certain level to things one level up. */

void citygen_connect_next_level_up (int depth, VALUE *dims);

/* Thinks links the base grid of the city (depth 0 things) so we
   get the whole city linked up. */

void citygen_link_base_grid (void);


/* This adds guards and gatehouses to the city. */

void citygen_add_guards (THING *area);

/* This adds fields around the city. */

void citygen_add_fields (THING *area);

/* This connects disconnected regions within the city after everything
   has been built. */

void citygen_connect_regions (THING *area, int max_depth_jump);


/* This exports some of the map as a simple plasm setup. */
void citygen_plasm_export (void);


