

/**********************************************************************/
/* Reasons for hunting. Things hunt (move to a new location) for
   different reasons. Pretty much hunting to kill or get healed take
   precedence, and one hunting type is stored in a ministack so that
   the thing can go back to its original goal once the temporary danger
   is over. */
/**********************************************************************/

#define HUNT_NONE           0    /* Not hunting anything */
#define HUNT_KILL           1    /* Hunting to kill something */
#define HUNT_FOLLOW         2    /* Hunting to follow something */
#define HUNT_GATHER         3    /* Hunting to gather something */
#define HUNT_DROPOFF        4    /* Hunting to drop something off */
#define HUNT_PATROL         5    /* Hunting just to patrol for people. */
#define HUNT_SETTLE         6    /* Society moves to a new location */
#define HUNT_GUARDPOST      7    /* Society member moving to a guard post. */
#define HUNT_HEALING        8    /* Trying to find a healer. */
#define HUNT_SLEEP          9    /* Looking for a place to sleep. */
#define HUNT_RAID          10    /* Moving to raid a society. */
#define HUNT_NEED          11    /* Hunt because you need something. */
#define HUNT_FAR_PATROL    12    /* Go to another area to patrol. */
#define HUNT_MAX           13    /* Maximum kind of hunting. */

#define MAX_HUNT_DEPTH         3000    /* Max depth hunting works... */
#define MAX_SHORT_HUNT_DEPTH   100     /* Max depth you go on "short" hunts */
#define SOCIETY_HUNT_DEPTH     6       /* Max depth society members hunt. 
*/
#define TRACK_TICKS     (2*UPD_PER_SECOND) /* Takes 2 seconds to track. */

/* This macro encapsulates the idea of checking if an adjacent room is a
   goodroom or not and using it or else checking for other things using
   find_track_room. */

#define FTR(r,d,b)  ((!(b) && (r)->adj_goodroom[(d)]) ? \
		     (r)->adj_goodroom[(d)] : \
		     ((r)->adj_room[(d)] && \
		     is_track_room ((r)->adj_room[(d)], (b))) ? \
		     (r)->adj_room[(d)] : NULL)



#define is_track_room(a,b) ((!IS_MARKED(a)&&!IS_SET((a)->move_flags,~(b)))?TRUE:FALSE)
			    

/**********************************************************************/
/* This struct represents a single track.                             */
/**********************************************************************/

struct track_data
{
  TRACK *next;
  THING *who;
  unsigned char dir_from;
  unsigned char dir_to;
  short timer;
  bool move_set; /* Players only see tracks set by movement. */
};

/**********************************************************************/
/* This struct represents a single piece of the bfs list for tracks.  */
/**********************************************************************/


struct bfs_data
{
  BFS *next;
  THING *room;
  BFS *from;
  unsigned short dir;
  unsigned short depth;
};


/* Global variables. I use a global bfs queue so that I can keep track
   of the end of the list easily. Since they get destroyed right
   after they're used, it works out ok. */

extern TRACK *track_free;
extern int track_count;
extern BFS *bfs_free;
extern BFS *bfs_list;
extern BFS *bfs_last;
extern BFS *bfs_curr;

extern int bfs_count;


/* These are associated with the hunting types above. Change this
   if you change them. */

extern const char *hunting_type_names[HUNT_MAX];

/**********************************************************************/
/* These are functions used with tracking/hunting. Went from dfs->bfs */
/**********************************************************************/

TRACK *new_track (void);
void free_track (TRACK *);
void write_tracks (FILE *, TRACK *);
void write_track (FILE *, TRACK *);
TRACK *read_track (FILE *);

BFS *new_bfs (void);
void add_bfs (BFS *from, THING *room, int dir);
void clear_bfs_list (void); /* Frees the whole bfs list. */
TRACK *place_track (THING *who, THING *where, int dir_from, int dir_to, bool move_set);
void show_tracks (THING *th, THING *room);
void place_backtracks (THING *, int); /* This places a sequence of backtracks
				    using a bfs queue that was created to
				    hunt something down. */
bool hunt_thing (THING *, int maxdepth); /* Hunts a room/obj/mobile */
void find_new_patrol_location (THING *th); /* Sets the mob to go to a
					      random location within
					      their "sphere of influence" */
void track_victim (THING *); /* Player track...*/
void place_tracks (THING *th, int rdir); /* Place tracks to victim */
bool place_one_track (THING *th, THING *croom, int dir_from); 
/* Finds if a room exists in this direction for tracking. If the
   thing doing the tracking has the ability to move through certain 
   kinds of environments, the tracking will move through those 
   environments also. */

THING *find_track_room (THING *, int dir, int flags);


/* Set this thing to hunt something. */
void start_hunting (THING *, char *, int type); 

/* This lets you start hunting a room based on its vnum. */

void start_hunting_room (THING *, int vnum, int type);

/* Make this thing stop hunting...maybe making it go back to an old 
   hunt type. If the all is set to true, it stops hunting everything. 
   Otherwise it just stops current hunt. */

void stop_hunting (THING *th, bool all);

/* Makes a thing that's trying to hunt something move through a portal
   or climbable object. If the second argument is NULL, then it moves
   through any old object, but if it's not NULL, then the other object
   must have tracks or exist on the other side. */

void move_through_portal (THING *, THING *);


/* This marks all trackable rooms adjacent to the start room. If
   the area is specified then all subsequent rooms must be in
   the same area as that. Otherwise the rooms can be anywhere. */

void mark_track_room (THING *room, THING *area, int goodroom_bits);



