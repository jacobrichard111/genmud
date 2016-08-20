


/* These functions are used to generate caves and 3d mazes. */

#define CAVEGEN_ALLOWED   0x00000001   /* Can we use this spot to cavegen? */
#define CAVEGEN_USED      0x00000002   /* Is this a part of the cave? */
#define CAVEGEN_CONNECTED 0x00000004   /* Is this room connected? */
#define CAVEGEN_ADDED_ADJACENT 0x00000008 /* Next to something that
					     was added this time...dont'
					     add this one. */

/* Size of cavegen grid is [CG_MAX*2+1][CG_MAX*2+1][CG_MAX] Middle starts
   at GC_MAX, CG_MAX, CG_MAX/2.  We assume that we don't go to the edges
   of the map when setting the dimensions of any particular cave. */

#define CAVEGEN_MAX  21


/* When adding rooms to the cave, we don't let rooms get put in if too
   many are "near" to the current room. This number determines how far
   we go out in the +/- x and y dirs to search for near rooms.
   KEEP THIS VALUE AT LEAST 1 OR ELSE CERTAIN LOOPS IN CAVEGEN.C WON'T WORK
   CORRECTLY DUE TO OUT OF BOUNDS ARRAY ERRORS. THIS IS HERE TO MAKE THE
   CODE A BIT FASTER. */

#define CAVEGEN_NEAR_DIST 3

/* This is the max number of rooms a room can have near it before it's
   unlikely to add another room. */

#define CAVEGEN_MAX_ROOMS_NEAR (CAVEGEN_NEAR_DIST*3)

#define CAVEGEN_ADD_TRIES   50  /* Number of times you try to add rooms. */
#define CAVEGEN_FORCE_UD    15 /* The likelihood of linking U/D rooms
				   increases with this number goes from 
				   1-N so that the UD rooms get connected
				   after most rooms are connected, but still
				   fairly quickly. */


#define CAVEGEN_ADD_BEFORE_CHECK_CONNECT 3 /* Try to add N rooms before you
					      check connections
					      again. */
/* Min number of cavegen rooms: */

#define CAVEGEN_MIN_ROOMS 100

/* This clears 1 or more bits from the cave_grid matrix. */
void cavegen_clear_bit (int bit);



/* This runs the cavegen command...sets up the rooms. It only returns
   true if you actually just made a map just now. */
bool cavegen (THING *, char *);

/* This checks if all of the cavegen rooms are connected or not. */

bool cavegen_is_connected (void);


/* This is used for the DFS to see if the rooms are connected or not. */
void cavegen_check_connected (int x, int y, int z);

/* This seeds rooms into the cavegen map. */
void cavegen_seed_rooms (); 


/* This seeds a few rooms at the edge of the map. The 0 args are randomized
   within the min/max_values and the one that's nonzero is set for that
   coord. */

void cavegen_seed_room (int x, int y, int z);



/* This attempts to attach more rooms to the cave. It does this by attaching
   new rooms adjacent to existing rooms in such a way that the rooms are
   not likely to end up in clumps. */

void cavegen_add_rooms (int force_number);

/* This actually generates the cavegen rooms for use within an area.*/
bool cavegen_generate (THING *th, int start_vnum);


/* This shows the levels of the caves that were generated as a series of
   maps. It also shows up exits as green, down exits as red, and up/down
   exits as cyan. */

void cavegen_show (THING *th);

/* This returns the number of cavegen rooms made. */

int find_num_cave_rooms (void);
