/* This code creates a fake wilderness part of the MUD. The way 
   it works is that as players/mobs move around, rooms get created
   then those rooms get destroyed as the rooms are emptied. */

#define WILDERNESS_AREA_VNUM  999999  /* Number of wilderness area. */
#define WILDERNESS_SIZE       2000    /* The width/height of the wilderness. */

/* The min vnum is the area vnum + 1. The max vnum is the min vnum plus
   WILDERNESS_SIZE squared. */


 /* Min vnum for wilderness rooms. */
#define WILDERNESS_MIN_VNUM   (WILDERNESS_AREA_VNUM+1)

 /* Max vnum for the wilderness. */
#define WILDERNESS_MAX_VNUM   (WILDERNESS_MIN_VNUM+(WILDERNESS_SIZE*WILDERNESS_SIZE))
#define WILDERNESS_FIELD_PROTO  40   /* Room where wilderness proto's
					get set to...*/

#define IS_WILDERNESS_ROOM(a)  ((a) && (a)->vnum >= WILDERNESS_MIN_VNUM&&\
                                (a)->vnum < WILDERNESS_MAX_VNUM)


/* Externs related to maps. */

extern THING *wilderness_area; /* The pointer to the wilderness area.
				  Should be set up at bootup. */

extern char wilderness[WILDERNESS_SIZE][WILDERNESS_SIZE];


/* Functions to create these rooms...*/
THING *make_wilderness_room (int num); 

/* This updates a wilderness room to see if it needs to be destroyed
   or a timer set. */

bool update_wilderness_room (THING *room);


/* This adds the base data for the wilderness area if needed. */

void setup_wilderness_area (void);

/* This writes out the wilderness data. */

void write_wilderness (void);

/* This draws a wilderness map instead of a regular map. */

void create_wilderness_map (THING *th, THING *in, int maxx, int maxy);
