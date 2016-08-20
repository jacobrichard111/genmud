/* This part of the program is used to generate (hopefully) interesting details
   within areas. It uses things to store the data in area 104000 and
   uses that data to generate different kinds of locations. */

/* Max number of details per area is N. */



#define DETAIL_MAX 30


/* Max depth you can go to when trying to add details is N. */
#define DETAIL_MAX_DEPTH 10 



/* This is the detail generator for an area. It works using the area's
   size and main sector type. */

void detailgen (THING *area);


/* This finds a valid detail start room for use within an area. */

THING *find_detail_type (THING *area, int used[DETAIL_MAX]);


/* This starts a detail by generating its name and then by altering
   some rooms nearby. */

void add_detail (THING *area, THING *to, THING *detail_thing, int depth);

/* This starts the main detail location then adds it to the area. */

void start_main_detail_rooms (THING *area, THING *detail_thing);
void add_main_detail_room (THING *room, int depth_left, int room_bits);

/* These start and add secondary details to the detail that's been
   premade. It is assumed that the detail portion of the area is
   the one with the marked rooms. 

   It returns one of the rooms in this detail so that it's possible
   to put resets in there. */

THING *start_other_detail_rooms (THING *area, THING *detail_thing);

/* This creates a mobject from the detail area and resets it into to
   (or resets it into a room if to is an area. */

THING *generate_detail_mobject (THING *area, THING *to, THING *detail_thing);


/* This adds the reset details to an area which has already been set up
   with the base detail. */

void add_detail_resets (THING *area, THING *to, THING *detail_thing, int depth);


/* This creates a detail name based on the info given in the thing. It puts
   the names onto the target and uses the given society name and 
   proper name if needed. */

void generate_detail_name (THING *proto, THING *target);

/* This appends the word detail to a room name. */

void append_detail_to_room_name (THING *room);
