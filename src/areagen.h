


/* This data is used to generate areas. */

/* These are the kinds of "base" areas that you're allowed to make. */

#define AREAGEN_SECTOR_FLAGS (ROOM_ROUGH | ROOM_FOREST | ROOM_FIELD | ROOM_DESERT | ROOM_SWAMP | ROOM_SNOW | ROOM_UNDERGROUND | ROOM_WATERY)

/* This tells how many of each kind of detail get placed into an area. */
/* Generally when deciding how many of a detail to put into an area
   the number is nr (1, 1+area->armor/DETAIL_DENSITY) */

#define DETAIL_DENSITY 65

/* This is the max number of map parts that can be put into an area. */

#define AREAGEN_MAP_PART_MAX   5

/* These are the min and max levels that areas can have when you're
   making mobs for them. */

#define AREA_LEVEL_MIN 10
#define AREA_LEVEL_MAX 400


/* This is the max number of trees an area can have. Or at least the
   max number that will have catwalks strung between them. */

#define AREAGEN_TREE_MAX  20

/* These are used to generate ridges. */

#define MAX_ELEVATION_RADIUS 9 /* Max number of rooms a ridge can go in
			      each direction from the center. */

#define MIN_ELEVATION_ROOMS  1  /* Need at least this many rooms next to
			       each other to make an elevation. */

/* These are the number of different kinds of sectors that we might add
   to an area. */

#define SECTOR_PATCHTYPE_MAX   8


/* Makes a grid for determining most northerly/southerly/easterly/westerly
   rooms in the area for making roads. */

#define AREAGEN_MAX 200




/* These are the names like woods forest hills used to generate the area
   names They should end with a "" so that we can search them and pick
   random things out of them. */

extern const int sector_patchtypes[SECTOR_PATCHTYPE_MAX];

/* This is a word used to generate objects along with a color for that
   particular word. */



/* Generate an area. */
void do_areagen (THING *, char *);
void areagen (THING *th, char *arg);



/* Create a name for an area by taking for example Bob and forest and
   making Bob woods or The Bob woods or The Forest of Bob. */

char *generate_area_name (char *name, int sector_type);

/* This generates a grid of rooms that maps out the relative locations
   of the rooms in the area. */

void generate_room_grid (THING *room, int grid[AREAGEN_MAX][AREAGEN_MAX], int extremes[REALDIR_MAX], int x, int y);

/* This finds a room on an edge of an area. Useful for linking and
   making roads and such. */

THING *find_room_on_area_edge (THING *area, int dir, int room_flags_wanted);


/* This finds the number of rooms adjacent to the given room (including 
   itself) that are accessible. It does not unmark rooms so you must
   do this! */

int find_num_adjacent_rooms (THING *room, int room_flags);

/* This creates a simple road name. */

char *generate_road_name(int sector_type);

/* This generates the roads in an area. */

void add_roads (THING *area, int go_dirs);

/* This adds a road to an area. If the end_edge is REALDIR_MAX, it makes a
   half road that stops at the first road it meets. */

void add_road (THING *area, int start_edge, int end_edge);

/* This adds some extra curve or turn names to roads in an area. */

void add_road_turns (THING *area);


/* This adds water like small lakes and streams to the area. */

void add_water (THING *area);

/* Used to add lakes to an area. */

void add_lakes (THING *area);

/* Used to add a lake to an area. */

void add_lake (THING *room, char *name, 
	       int sector_type, int curr_depth, int max_depth);

/* This adds streams to an area. */

void add_streams (THING *area);

/* This adds a stream into an area. There's no max depth, just a chance
   of stopping after a certain point. The dir_from is used to keep
   the course straight more often than not. */

void add_stream (THING *room, char *name, int sector, int depth, int dir);

/* This creates a stream name. Useful for splitting streams. */

char *generate_stream_name (int sector_type);

/* This adds a few forest and/or field rooms next to an oasis. */

void add_adjacent_oasis_rooms (THING *room, int sector_type);

/* These add underpasses to bridges and things that go over top of
   water. */

void add_underpasses (THING *area);

/* This finds a room below an overpass. */
THING *find_below_room (THING *room);

/* Adds details like patches of ground...stage 1? */

void add_sector_details (THING *area);

/* This adds a single patch to an area of a certain type. */

void start_sector_patch (THING *area, int patch_type);

/* This actually adds the patch into the game. */

void add_sector_patch (THING *room, char *name, int sector_type,
		       int patch_type, int curr_depth, int max_depth);


/* This generates the name for a patch of earth given a sector name. */

char *generate_patch_name (int sector_type, int patch_type);

/* This generates room descriptions for the area. */

void add_room_descriptions (THING *area);

/* This creates a description for this particular room. */

char *generate_room_description (THING *room);

/* This tries to reconnect parts of an area that were disconnected by
   water or some other badroom type. */

void fix_disconnected_sections (THING *area);

/* This returns the number of rooms in a disconnected region of an area. 
   It can also specify certain badroom bits allowed.  */

int find_connected_rooms_size (THING *room, int badroom_bits_allowed);


/* This attempts to find a path across badroom sectors. It searches
   to a max depth then stops so we know we get the shortest bridge
   possible. */

bool found_path_through_badrooms (THING *room, int max_depth);



/* This adds some trees to the area. Since they use up rooms and are
   vertical, they are done last. */

void add_trees (THING *area);
void add_tree (THING *base_room);

/* This adds catwalks between trees. */

void add_catwalks (THING *area);

void make_catwalk (THING *branch_from, THING *branch_to, int dx, int dy);

/* This finds a tree in the area. It either looks for a tree in an
   extreme direction (Set up when catwalks were set up) or it
   searches for a random tree in the area if the dir isn't between
   0 and 3 inclusive. */

THING *find_tree_area (THING *area, int dir);

/* This finds a catwalk branch in a certain direction based on the
   dx and dy. It randomly picks from all permissible branches. */

THING *find_tree_branch (THING *tree_base, int dir);

/* This adds some elevation to the area that make the area less flat. */

void add_elevations (THING *area);

/* This tells if a room is ok to be part of a elevation. */

bool ok_elevation_room (THING *room, bool link_down);


/* This reports if there are any loop exits in the area. */

void check_loop_exits (THING *area, int num);


/* This places some secret doors in the area. */

void add_secret_doors (THING *area);

/* This places a secret door between two rooms. */

void place_secret_door (THING *room, int dir, char *name);

/* This creates and returns a secret door name. */

char *generate_secret_door_name (void);

/* This adds a guarded mob to a room behind a secret door. */

void add_guarded_mob (THING *room);

/* This marks the area edges by making their room names of the form
   dir_edge. */

void mark_area_edges (THING *area);


/* This adds shoreline room names near water in areas. */

void add_shorelines (THING *area);


/* This lets you generate a new desc and sector type for a room. */

void roomgen (THING *th, THING *room, char *arg);

/* This adds an exit from a room to another vnummed room if the target
   exists. The exit flag tells what exits to add here. */

void room_add_exit (THING *room, int dir, int vnum_to, int exit_flags);
