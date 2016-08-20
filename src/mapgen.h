


/* Maximum width and height of a generated piece. */

#define MAPGEN_MAXX        141
#define MAPGEN_MAXY        141


/* Flags used in the mapgen_used struct. */

#define MAPGEN_USED           1  /* Was this used in the map? */
#define MAPGEN_SEARCHED       2  /* Was this searched, if so, it's used. */


/* Used to make the caverns of modnar. */
#define MODNAR_AREA_VNUM 120000
#define MODNAR_MAX_SIZE  900

struct mapgen_data
{
  int num_rooms;         /* Number of nontrivial rooms in this map piece. */
  bool used[MAPGEN_MAXX][MAPGEN_MAXY]; /* Matrix of which rooms are used. */
  MAPGEN *next;          /* For the free list. */

  /* These are the min and max x and y values where the map
     actually sits. */
  int min_x;          
  int max_x;         
  int min_y; 
  int max_y;
};


extern int mapgen_count;    /* Total number of mapgens created.  */
extern MAPGEN *mapgen_free;  /* The list of freed mapgens.  */



MAPGEN *new_mapgen(void);  /* Create a new mapgen, or get from the free list. */
void free_mapgen (MAPGEN *); /* Put a mapgen onto the free list. */


void show_mapgen (THING *, MAPGEN *); /* Show a mapgen to a person. */
MAPGEN* mapgen_generate (char *); /* Generate a mapgen (not the rooms). */
char *mapgen_create (MAPGEN *, int); /* Actually create a set of rooms. */
void mapgen_connection (short, short); /* dfs search for connected component. */

/* This overlays two maps and OR's them to make a new map. */

MAPGEN *mapgen_combine (MAPGEN *, MAPGEN *, int offset_x, int offset_y);


void generate_modnar(void); /* This generates the caverns of modnar on reboots. */
/* This sets up a room in the modnar map and puts a bunch of other rooms
   like it right next to it. */
void set_up_modnar_room (THING *, int, char *, int);

















