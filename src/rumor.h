


/* Rumors are a "history" of the various things that societies
   do like raiding, settling and patrolling. It will serve a dual
   purpose both for letting players listen to rumors from npc's,
   and to give societies a little help with their long-range
   planning. */


#define RUMOR_PATROL     0  /* A society sends out a patrol. */
#define RUMOR_SETTLE     1  /* A society settles at a new location. */
#define RUMOR_RAID       2  /* A society raids another society. */
#define RUMOR_SWITCH     3  /* A society switches sides. */
#define RUMOR_DEFEAT     4  /* A society was defeated and destroyed. */
#define RUMOR_ASSIST     5  /* A society assists another. */
#define RUMOR_REINFORCE  6  /* A society sends reinforcements to another. */
#define RUMOR_RELIC_RAID 7  /* A society tries to raid a relic room of
			       a different alignment. */
#define RUMOR_ABANDON    8  /* Run away and settle in a new location. */
#define RUMOR_PLAGUE     9  /* This society got the plague. */
#define RUMOR_TYPE_MAX   10

#define RUMOR_HOURS_UNTIL_DELETE 300 /* How many hours rumors stay in
				    existence before they start
				    to get deleted. This is not
				    an exact hour, it's just when
				    they start to get deleted. */




struct rumor_data
{
  int who;          /* What society caused the event to happen. */
  int to;            /* What the society acted upon. */
  RUMOR *next;
  RUMOR *prev;
  short type;  /* What type of rumor/event this is. */
  short hours;  /* Hours since the rumor was first created. */
  int vnum;      /* What rumor number this is. */
};


extern RUMOR *rumor_list;
extern RUMOR *rumor_free;
extern int rumor_count;   /* Total count of rumor data chunks. */
extern int curr_rumors;   /* Number of rumors currently queued. */
extern const char *rumor_names[RUMOR_TYPE_MAX]; /* Names of types of rumors. */


RUMOR *new_rumor (void);   /* Create a new rumor. */
void free_rumor (RUMOR *); /* Remove a rumor from the list of rumors. */ 
/* Adds a rumor of a specific type, from, to, hours and vnum 
   to the rumor list. */
void add_rumor (int type , int from , int to , int hours , int vnum);

/* These deal with rumors and fileio. */

void read_rumors (void);   /* Read the rumors from rumors.dat. */
void write_rumors (void);  /* Write the rumors to rumors.dat. */

int find_rumor_from_name (char *); /* Tells what kind of rumor you have
				      based on the name you give. */

void update_rumors (void); /* Update the rumor list and get rid of old ones. */

char *show_rumor(RUMOR*, bool); /* How a rumor looks to someone. The bool
				   is true if the looker is an admin... */

char *show_rumor_time (RUMOR *, bool); /* Shows how long ago a rumor happened
					  both for admins (real numbers) and
					  players who get approx times. */

void add_to_rumor_history (char *); /* Adds a string to the history 
				       of all rumors file. */

char *no_rumor_message(void); /* The message that gets sent when a mob
				 doesn't know a rumor, or if there's an
				 error. */

char *show_random_rumor_message(SOCIETY *); /* Pick a rumor from the list
					  and show it. */

char *show_random_quest_message(int); /* Pick a quest (resource) needed
					 by an align/society and tell
					 the player about it. */

/* Returns a message saying that the society/align named name needs a
   rawtype of type rawtype. */
char *show_raw_need_message (char *name, int rawtype);
/* This finds if a mort gets a rumor and sends the message to the mort. */

void send_mortal_rumor (THING *); 

/* This lets a thing share rumors with other things in other societies. */

void share_rumors (THING *);


/* This adds a rumor to a society's known rumors. */

void add_rumor_to_society (SOCIETY *, int vnum);


/* This clears the rumor history file. */

void clear_rumor_history (void);

/* This checks for a society overlord rumor. */

bool send_overlord_message (THING *th, THING *mob);


/* This gives a player a "delivery" quest. */

bool get_delivery_quest (THING *th);








