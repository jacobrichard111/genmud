

/* This all deals with societies. */



#define SOCIETY_GATHER_COUNT   8      /* Number of items a society member 
					 gathers before dropping them off. */

#define SOCIETY_RAW_POWER      3      /* Get N "credits" for each raw
                                         material gathered. */	

#define SOCIETY_NEW_MEMBER_TIMES 15    /* The number of tries a society
	                                 gets each time it tries to build
                                         a new member. */


#define NUM_GUARD_POSTS       10       /* Max num of posts a society has. */

#define RAW_GATHER_DEPTH     50     /* Things only gather up to N
					 spaces from present location. */

#define RAW_GATHER_ROOM_CHOICES 5     /* Pick up to N rooms to use to look
					for raw materials. */

#define GUARD_POST_CHOICES    100     /* Maximum number of rooms that are
					 possibiltiies in the code
					 to see which ones should be made
					 into guard posts. */

#define POP_BEFORE_SETTLE     300   /* Soc needs N members before it
					attempts to make a new society. */

#define SOCIETY_SETTLE_CHANCE 230   /* 1 out of N chance to settle
				       per hour when population is big. */

#define POP_LEVEL_MULTIPLIER  10    /* Max pop for a society is based on
				       POP_BEFORE_SETTLE if it's creation
				       member (soc->start[0]) member is
				       level POP_LEVEL_MULTIPLIER. The max
				       pop is altered depending on the
				       different levels for the baby
				       member. */

#define SOCIETY_SETTLE_HOURS 1000    /* Number of hours before the society
					can attempt to settle again. */

/* Min and max percents of warriors that a society or align tries
   to have. */


#define WARRIOR_PERCENT_MIN   30 
#define WARRIOR_PERCENT_MAX   70

#define SOCIETY_MAX_POP_INCREASE 3   /* Amount max pop is increased by
					each time the society pays for it. */

#define SOCIETY_MAXPOP_INCREASE_TRIES 10 /* number of maxpop increase tries... 
					    increased when called for. */

# define RAID_SETUP_HOURS    10    /* Hours for raiders to make it to setup room. */

#define RAID_HOURS          100    /* Raiders have close to hours to raid, then
				      they must return home. */
#define SOCIETY_RAID_HOURS  250    /* Society must wait this long between
				      raids. */

#define SOCIETY_RAID_CHANCE  400    /* 1 out of this chance of raiding each
				      hour that is has the resources. */

#define SOCIETY_RAID_ADJACENT_CHANCE 60 /* Will raid adjacent societies
					   except 1 out of this many times. */

#define RAID_HASH            100   /* Size of raid hash table. */

#define RAID_MAX_VNUM       5000   /* Maximum number of raids possible at 
				      once. You should *never* get anywhere
				      near this number. */

#define ALERT_HOURS           20   /* Number of hours the society is on
				      alert from raiders. */

#define ALERT_SETUP_HOURS      5   /* Number of hours the society has to set 
				      up if people raid it. */

#define RAW_TAX_AMOUNT      10000  /* Amounts above this are tithed to the
				     alignment, and amounts below this
				     are left alone. If the alignment
				     gets 100000 or more of a type, it
				     stops collecting that type for the
				     time being. It also taxes 20pct of
				     all raws from 1k to 4k. */

#define RAW_ALIGN_RESERVE  100000  /* If the alignment gets this much of
				      a raw material, it stops collecting. */

#define RAW_WANT_MULT       5     /* If a society needs extra raw materials,
				     the amount it asks for is multiplied
				     by this so it will have enough
				     for a few turns. */

#define BATTLE_CASTE_ADVANCE_COST 10 /* Mineral base cost to advance in the 
					battle caste tiers. */

#define MAX_WAYPOINTS          15    /* Maximum number of waypoints a
					patrolling society mob will go
					to before it returns home. */

#define SOCIETY_COST_MULTIPLIER 150   /* The society upgrade costs go
					 up by base_cost every time the
					 costs pass this number. It's
					 proportional to the current
					 population. */

#define SOCIETY_HASH          500   /* Size of the society hash table. */


#define SOCIETY_BUILD_TIERS     5   /* Each room in the society built
				       city has a rank from 0-5 tiers. */

#define SOCIETY_BUILD_REPEAT    10   /* Each city block location must be
				       "built" 10x before it goes up a
				       level. */

#define BURN_MESSAGE_MAX    6 /* How many ranks of burn messages a
				 room can have. */

#define CAPTURE_CHANCE      300      /* A raider has a 1 out of this chance
					of taking a prisoner. */

#define RELIC_CHANCE         10      /* An overlord leader has a 1 out of 
					this chance of loading a relic. */

#define SOCIETY_GEN_CASTE_TIER_MAX 40 /* Max N tiers of names to choose
					 from when generating a society. */

#define NUM_VOWELS              10 /* This is because the vowels are 
				      weighted..e = 3x, aio = 2x, u = 1x. */


#define MAX_MORALE           200 /* Max that morale can be (min is neg
				     of this. */

#define REWARD_EXP_MULT    1000 /* This many exp rewarded per 1 
				   reward unit. */

#define REWARD_AMOUNT_MAX  100000 /* Max amount a reward can be. */

#define REWARD_QPS_DIVISOR 50     /* 1/50 qps per reward pt. */

/* These are the vnums used to make the "ancient races" and "organizations"
   created in the historygen code. It's so that they have special vnums
   that get their creatures and objects made differently. */

#define ANCIENT_RACE_SOCIGEN_VNUM       (WORLD_VNUM+1)
#define ORGANIZATION_SOCIGEN_VNUM       (WORLD_VNUM+2)
#define DEMON_SOCIGEN_VNUM              (WORLD_VNUM+3)

/* Stages of decay for an area after a society gets defeated. */

#define SOCIETY_BUILD_ABANDONED  (SOCIETY_BUILD_TIERS+1)
#define SOCIETY_BUILD_RUINS      (SOCIETY_BUILD_ABANDONED+1)
#define SOCIETY_BUILD_OVERGROWN  (SOCIETY_BUILD_RUINS+1)


/* These are flags for when a society engages in a large scale
   activity. */

#define SOCIACT_OVERRIDE_HUNT 0x00000001 /* Members stop hunting to do
						this activity. */
#define SOCIACT_HOME_AREA 0x00000002 /* Only things in start area do it. */
#define SOCIACT_NOT_HOME_AREA 0x00000004 /* Things not in start do it. */

#define SOCIETY_ATTR_CHANGE_MAX     10 /* Max number of things you can
					  change in a society across
					  the board at once. */

/* These are used when a player tries to bribe a society to convert or
   to stay. */

#define BRIBE_POWER_MULTIPLIER 100 /* It costs soc->power*this to make a 
				      society change sides (approx). */

#define BRIBE_SAME_ALIGN_STAY 10    /* Cost to convert over this number
				      makes a society more likely to stay
				      on your side. */

/* A society gets an array of 100 ints to store rumors. */
#define SOCIETY_RUMOR_ARRAY_SIZE    100

/* Each society rumor array is an array of ints so it has 32*size bits. */
#define RUMOR_COUNT_MAX    (32*SOCIETY_RUMOR_ARRAY_SIZE)

/* Number of hours it takes for an area to decay another level. */
#define SOCIETY_BUILD_DECAY_HOURS  200

/* How long things must go between updating a certain need. N seconds. */

#define NEED_UPDATE_TIMER  100

/* How long a mob will wait before giving a request to another person. 
   (seconds) */

#define NEED_REQUEST_TIMER   300 


#define MAX_AFFINITY 95 /* Maximum affinity percent a society can have. */

/* When a thing needs something, it either needs to get it, or it needs
   to make it or it needs to give it to someone */


#define NEED_GET          0
#define NEED_MAKE         1
#define NEED_GIVE         2
#define NEED_MAX          3
/* Check up to 5 mobs here for things that satisfy needs. */
#define NEED_MOBHERE_MAX  5

/* These are the various processes that crafters can carry out to make
   items. */

#define PROCESS_FORGE   0
#define PROCESS_SMELT   1
#define PROCESS_CARVE   2
#define PROCESS_BAKE    3
#define PROCESS_TAN     4
#define PROCESS_CRAFT   5
#define PROCESS_WEAVE   6
#define PROCESS_SPIN    7
#define PROCESS_SHAPE   8
#define PROCESS_MILL    9
#define PROCESS_BUTCHER 10
#define PROCESS_BLOW    11
#define PROCESS_MAX     12




/**********************************************************************/
/* Society Flags */
/**********************************************************************/

#define SOCIETY_AGGRESSIVE      0x00000001 /* Society attacks when big. */
#define SOCIETY_SETTLER         0x00000002 /* Settles in new places. */
#define SOCIETY_FIXED_ALIGN     0x00000004 /* Cannot switch align. */
#define SOCIETY_XENOPHOBIC      0x00000008 /* Attacks outsiders. */
#define SOCIETY_NUKE            0x00000010 /* Not saved. */
#define SOCIETY_DESTRUCTIBLE    0x00000020 /* When population goes to 0, 
                                              this society stops breeding.  */
#define SOCIETY_NORESOURCES     0x00000040 /* Society does not need resources
					      to improve...Kills instead? */
#define SOCIETY_NOCTURNAL       0x00000080 /* Sleeps at night. */
#define SOCIETY_NOSLEEP         0x00000100 /* This society doesn't have
					      a sleep cycle. */
#define SOCIETY_NONAMES         0x00000200 /* These society members 
					      don't get names. */
#define SOCIETY_NECRO_GENERATE  0x00000400 /* The mages for this society
					      raise the dead to become
					      new members. */
#define SOCIETY_MARKED          0x00000800 /* This society is marked. */
#define SOCIETY_FASTGROW        0x00001000 /* More powerful...does
					      things bigger and faster
					      than other societies. */
#define SOCIETY_OVERLORD        0x00002000 /* This society has an 
					      overlord -- makes them
					      work better and not switch
					      alignment. */

/* These are the base society flags given to societies when they're
   generated. */

#define SOCIETY_BASE_FLAGS  (SOCIETY_AGGRESSIVE|SOCIETY_SETTLER|SOCIETY_XENOPHOBIC|SOCIETY_DESTRUCTIBLE)

/**********************************************************************/
/* Society Build Types --- things societies can spend resources on.  */
/**********************************************************************/

#define BUILD_MEMBER    0x00000001 /* Build a new member. */
#define BUILD_MAXPOP    0x00000002 /* Increase the maxpop */
#define BUILD_QUALITY   0x00000004 /* Improve the quality of the members. */
#define BUILD_TIER      0x00000008 /* Increase a curr tier in a caste. */
#define BUILD_WARRIOR   0x00000010 /* Increase fighter caste chances. */
#define BUILD_WORKER    0x00000020 /* Increase nonfighter caste chances. */

#define BUILD_MAX             6    /* 6 kinds of build data. */



/* These are the kinds of processes you can engage in to make items for
   the society needs code. The general setup is that you take a process
   from this list and several items and have them listed in a VAL_MADEOF
   and then in order to make this item you must have all of the listed
   items and perform the process. */


/**********************************************************************/
/* Caste Flags Incidentally - the total number of these cannot be more
   than CASTE_MAX since when the game tries to make cities, it checks
   for different kinds of buildings and is restricted by CASTE_MAX. */
/**********************************************************************/

#define CASTE_WARRIOR           0x00000001  /* Warriors defend home. */
#define CASTE_LEADER            0x00000002  /* Leaders..defended by rest */
#define CASTE_WORKER            0x00000004  /* These things find resources */
#define CASTE_BUILDER           0x00000008  /* These use resources to
					       make stuff/improve stuff. */
#define CASTE_HEALER            0x00000010  /* Healer caste. */
#define CASTE_WIZARD            0x00000020  /* Wizards... */
#define CASTE_SHOPKEEPER        0x00000040  /* Shopkeepers...*/
#define CASTE_FARMER            0x00000080  /* Farmers/herders */
#define CASTE_CRAFTER           0x00000100  /* Craftsmen who make items. */
#define CASTE_CHILDREN          0x00000200  /* Childrens' caste. */
#define CASTE_THIEF             0x00000400  /* Thief/spy caste. */
#define BATTLE_CASTES (CASTE_WARRIOR | CASTE_HEALER | CASTE_WIZARD | CASTE_LEADER | CASTE_THIEF)
/**********************************************************************/
/* Warrior Caste(s) status Flags */
/**********************************************************************/

#define WARRIOR_NONE           0  /* Not set yet. */
#define WARRIOR_SENTRY         1  /* This is a sentry..does not move or
				     help out with invaders. */
#define WARRIOR_GUARD          2  /* Defensive warrior */
#define WARRIOR_PATROL         3  /* Defensive and offensive-- does not track
				     too far out of the area. */
#define WARRIOR_HUNTER         4  /* Offensive warrior. */
#define WARRIOR_MAX            5  /* Max number of warrior types. */

/* These are different kinds of shops a society will have. */

#define SOCIETY_SHOP_ARMOR             0
#define SOCIETY_SHOP_WEAPONS           1      
#define SOCIETY_SHOP_PROVISIONS        2
#define SOCIETY_SHOP_GEMS_MAGIC        3
#define SOCIETY_SHOP_FOOD              4
#define SOCIETY_SHOP_MAX               5

/* A shop can sell up to three kinds of items. */

#define SOCIETY_SHOP_VALS_MAX         3


/* The number of resets a society shopkeeper can get. */

#define SOCIETY_SHOP_RESETS_MAX      2
/**********************************************************************/
/* This is a society. It is a population of monsters that grows in
 number and power until it reaches a maximal size and either expands 
 or it attacks something, or maybe it just sits there. These things
 can also each pop equipment when the new members are made...and the
 old creatures are replaced with new when necessary. */

/* This has been updated because of the RTS factors. */

/**********************************************************************/

struct society_data
{
  SOCIETY *next;
  SOCIETY *next_hash;         /* Used in hash table for fast lookups. */
  char *name;                 /* Name of this society. */
  char *pname;                /* Plural name of this society. */
  char *aname;                /* Name of this society as an adjective */
  char *adj;                  /* Descriptive word about the society. */
  FLAG *flags;                /* Flags added to all things of this type. */
  int vnum;                   /* Number of the caste */
  int room_start;             /* Room they start popping in. */
  int room_end;               /* Room the end popping in. */
  int object_start;           /* Start of objects they might load. */
  int object_end;             /* End of objects they might load. */
  int society_flags;          /* Flags to describe the society behavior. */
  int population;             /* What is the population of this society? */
  int recent_maxpop;          /* Recent max population of the society. (Used
				 for finding city size.) */
  int power;                  /* This the power (sum level squared) of soc. */
  int warriors;               /* Number of warriors in the society. */
  int quality;                /* How much they've improved over the base. */
  int raw_want[RAW_MAX];      /* What number of resources we want. */
  int raw_curr[RAW_MAX];      /* Current resources. */
  int goals;                  /* What do we need done atm? -- bits for now */
  int level_bonus;            /* Extra levels the mobs get when made. */
  /* Castes have changed a lot. They used to be their own structs, but I
     decided to simplify this for the purposes of the RTS stuff. */
  
  int settle_hours;           /* Hours left in settling. */
  int raid_hours;             /* Hours left before we can raid again. */
  int start[CASTE_MAX];       /* Which mob starts this caste. */
  int curr_pop[CASTE_MAX];    /* Current pop of this caste. */
  int max_pop[CASTE_MAX];     /* Max pop of this caste. */
  int max_tier[CASTE_MAX];    /* Max num of tiers possible for this caste. */
  int curr_tier[CASTE_MAX];   /* Current max tier for this caste. */
  int chance[CASTE_MAX];      /* Chance of joining this caste. */
  int base_chance[CASTE_MAX]; /* Core/base chance of joining this caste. */
  int cflags[CASTE_MAX];      /* Flags associated with this caste. */
  int alert;                  /* Alert level... */
  int last_alert;             /* Most recent time alerted. */
  int alert_rally_point;      /* Rally point for responding to threats. */
  int alert_hours;            /* Hours to get to alert rally point. */
  THING *last_killer;         /* Last killer of a society member. */
  
  int guard_post[NUM_GUARD_POSTS]; /* Guard posts for sentries. */
  int align;                  /* Alignment */
  int warrior_percent;        /* The percentage of warriors this society desires. */
  int assist_hours;           /* Hours since this was last assisted. This
				 is to keep an alignment from spam assisting
				 one alignment. */
  int alife_combat_bonus;     /* Pct bonus to alife attack. */
  int alife_growth_bonus;     /* Pct bonus to growth this has. */
  /* Coordinates of the society homeland..will always regenerate. */
  int alife_home_x;           /* X coord of this society's homeland. */
  int alife_home_y;           /* Y coord of this society's homeland. */
  int kills[ALIGN_MAX];       /* How many opp align kills this soc has. */
  int killed_by[ALIGN_MAX];   /* What aligns have killed this thing...*/
  int align_affinity[ALIGN_MAX]; /* How much you like an alignment. */
  int crafters_needed;        /* What kinds of craftsmen the society needs. */
  int shops_needed;           /* What kinds of shopkeepers the society needs. */
  /* Array of bits representing rumors known to society members. */
  int rumors_known[SOCIETY_RUMOR_ARRAY_SIZE]; 
  int delete_timer;           /* Timer for when this gets deleted when 
				 it dies off. */
  int generated_from;         /* What was this society generated from? */
  int population_cap;         /* The max population that this society
				 is allowed to attain. It is based on
				 the level of the society starting member. */
  int level;                  /* Level used internally to say where the
				 society can be put when the world regens. */
  int relic_raid_hours;       /* Hours until relic raid goes. */
  int relic_raid_gather_point; /* Place where raiders gather. */
  int relic_raid_target;      /* Place raiders will go. */
  int abandon_hours;          /* Hours until we can abandon again. */
  int morale;                 /* Morale of society--goes up with 
				 winning battles, down with losing. 
				 Affected by diplomats. Tends to 0 -- normal */
  int lifespan;               /* Average lifespan for members of this race. */
};

/**********************************************************************/
/* Build data -- types of things for societies to build and the costs. */
/**********************************************************************/

struct _build_data
{
  char *name;            /* What this type of building is called. */
  int val;               /* What build flag we are using. */
  int cost[RAW_MAX];     /* What the cost for each of these kinds of
			    building are in terms of raw materials. */
};



/**********************************************************************/
/* Need data -- info about a need this society currently has.   */
/**********************************************************************/

struct _need_data
{
  NEED *prev;  /* Prev need this thing has. */
  NEED *next;  /* Next need this thing has. */
  THING *th;   /* What thing has this need. */
  int last_updated; /* Last time this was changed to keep it from 
		       getting spammed by requests. */
  int num;    /* Vnum of the object this need represents 
		 If the number is < VAL_MAX it's assumed to be something
		 with this value. */
  int times;   /* How many times this has been needed. */
  short type;    /* What kind of need is this? To make or to get? 
		  0 = get, 1 = make */
  short helper_sent; /* When a helper was sent to perform the task
			embodied in the need. They get 
			NEED_REQUEST_TIMER seconds to do it before
			the mob looks for more help. */
  
};


/**********************************************************************/

/* These are various processes that crafters can carry out to make items.
   They */
/**********************************************************************/

struct _process_data 
{
  char *name;         /* Name of the process. */
  char *worker_name;  /* Name of the worker that does this. */
  int tools_in_hand;  /* Tools the person needs in hand to do this process. */
  
};
/**********************************************************************/
/* Society shop data -- what kinds of shops a society needs.   */
/**********************************************************************/

struct _society_shop_data
{
  char *name;
  /* What values this shop sells (used to determine the kinds of items
     that are valid for this shop: see is_valid_shop_item() */
  int values[SOCIETY_SHOP_VALS_MAX];
  /* Each shop gets certain resets of items. They are all at 50pct,
     and the first number is the reset number and the second is the
     number of times they get called. This will be modified by 
     society-specific items later on. They may also get replaced if I
     decide to make a list of all the food/drink/armor/wpns etc...that 
     aren't too good and then pick from those lists. */
  int resets[SOCIETY_SHOP_RESETS_MAX][2];
};

  /**********************************************************************/
/* Raid data -- info about a raid a society carries out vs another.   */
/**********************************************************************/


struct _raid_data 
{
  RAID *next;  /* Next raid in the raid table. */
  int vnum;    /* Vnum of the raid. */
  int raider_society;  /* The society number doing the raiding. */
  int victim_society;  /* The victim being raided. */
  int raider_power_lost; /* Power lost by raider in combat. */
  int victim_power_lost; /* Power lost by victim to raiders. */
  int victim_start_power; /* Victim starting power. */
  int raider_start_power; /* Power sent by raider to start with. */
  int post[NUM_GUARD_POSTS]; /* Actual guard posts attacked. */
  int hours;    /* Number of hours left in the raid. */
};




/* Externs for societies. */

extern int top_society;
extern int society_ticks; /* Where in the update cycle we are for
				    societies. This number goes from
				    0 to SOCIETY_HASH then loops back
				    to 0. */

extern int society_count;
SOCIETY *society_free;
extern SOCIETY *society_list;
extern SOCIETY *society_hash[SOCIETY_HASH];
extern const struct _build_data build_data[BUILD_MAX]; 
extern const struct _process_data process_data[PROCESS_MAX];
extern const struct _society_shop_data society_shop_data[SOCIETY_SHOP_MAX];


/* Externs for raids. */

extern RAID *raid_hash[RAID_HASH]; /* Table for tracking raids. */
extern RAID *raid_free;             /* List of free raids. */
extern int raid_count;              /* Number of raids made. */

/* Externs for society needs. */

extern NEED *need_free;
extern int need_count;
extern const char *need_names[NEED_MAX]; /* Types of needs...get give make. */



/**********************************************************************/
/* These are functions used with societies. */
/**********************************************************************/


SOCIETY *new_society (void);
/* This creates a new society and adds it to the lists. */
SOCIETY *create_new_society (void);
/* Clear all society cities and clear out all society mobs and 
   remove all societies that are destructible. */
void clear_societies (void);

/* This unmarks all societies. */
void unmark_all_societies (void);

/* Clear all recent_maxpops for this area (or all areas if area == NULL)
   This is so that if you purge an area or purge all, you don't 
   make other societies assist/reinforce this society. */
void society_clear_recent_maxpop (THING *area);
void free_society (SOCIETY *);
void clear_society_data (SOCIETY *);
void society_from_list (SOCIETY *);
void society_from_table (SOCIETY *);
void read_societies (void);
void write_societies (void);
SOCIETY *read_society (FILE *);
void write_society (FILE *, SOCIETY *);
SOCIETY *find_society_num (int);
bool in_same_society (THING *, THING *);
bool is_in_society (THING *);
void show_society (THING *, SOCIETY *);
void change_society_align (SOCIETY *, int); /* Changes the society align
					       to this number. */
void update_society_list (bool); /* Updates societies and AI eventually */
void update_society (SOCIETY *, bool); /* Updates a single society...*/
void society_edit (THING *, char *); /* Society editor */ 
/* Makes a new society out of moved mobs */
void settle_new_society_now (THING *); 
void society_gather (THING *); /* Workers gather materials for the society. */
void find_gather_location (THING *); /* Find room w/proper flags */
void find_dropoff_location (THING *); /* Finds builder in the society. */
void find_new_gather_type (THING *); /* Finds a type of thing to gather */
void society_activity (THING *); /* Make this do some society activity */
void society_worker_activity (THING *); /* What workers do. */
void society_shopkeeper_activity (THING *); /* What shopkeepers do. */
void society_crafter_activity (THING *);

/* This sets up a shopkeeper if it isn't set up already. */

void society_setup_shopkeeper (THING *);

void increase_max_pop (SOCIETY *); /* Increases the max pop of a society. */

/* Picks a caste based on the start chances for the caste. It adds up all
   possible chances and picks from one of them. */
int find_caste_from_percent (SOCIETY *, bool no_children, bool only_warriors);

void check_society_change_align (SOCIETY *); /* Checks if a society wants
						to change alignment due to
						being beat up too bad. */
int find_caste_in (THING *);
void set_up_guard_posts (SOCIETY *soc); /* Find where guard posts should 
					   go. */

void start_guarding_post (THING *); /* This makes a thing start guarding
				       a post. */

/* This finds a society member based on the type of caste the
   victim is in and also how far out it will search. */
THING *find_society_member_nearby (THING *, int type, int max_depth); 
SOCIETY *find_society_in (THING *);
void copy_society (SOCIETY *, SOCIETY *);
void update_societies (bool rapid); /* Updates societies */
void update_societies_event (void); /* Calls upd_soci (FALSE); */
/* This finds the population of a certain tier in a certain caste in a soc. */
int find_pop_caste_tier (int socnum, int castenum, int tiernum); 

/* Used for the RTS aspect of the game. */

bool make_new_member (SOCIETY *);  /* Makes a new member... */
void set_new_member_stats (THING *newmem, SOCIETY *soc);


void soc_to_list (SOCIETY *); /* Adds a society to the list based on vnum. */
int society_curr_pop (SOCIETY *); /* Current population of a society. */
int society_max_pop (SOCIETY *); /* Max pop of a society. */
bool has_resources (SOCIETY *, int); /* Does the society have enough
					resources to do this upgrade? */
bool subtract_build_cost (SOCIETY *, int); /* Subtracts the building cost
					      for a society upgrade. */

void update_raws_wanted (SOCIETY *); /* Figure out what raw materials 
					the society needs to get. */
void carry_out_upgrades (SOCIETY *); /* Upgrade the society. */
void update_society_goals (SOCIETY *); /* This updates what goals
					  the society has. */

void update_caste_chances (SOCIETY *); /* Update the chances that certain
					  castes are picked relative to
					  the base chances. */
void find_upgrades_wanted (SOCIETY *); /* Find what upgrades
					  the society wants. */

void weaken_society (SOCIETY *); /* See if the society needs to be
				    weakened. */
int raw_needed_by_society (SOCIETY *); /* Tells what kind of raw material
					  the society needs.. RAW_MAX means
					  don't need anything. */

bool increase_tier (SOCIETY *); /* Increases the curr tier in a society. */

/* This is used to increase the chance that either warriors appear or
   nonwarriors appear...the boolean tells which kind we are looking for. */

void increase_caste_chances(SOCIETY *, bool warrior); 
/* This is used to increase the chance that either warriors appear or
   nonwarriors appear...the boolean tells which kind we are looking for. */

void stop_raiding (SOCIETY *); /* This makes society members stop raiding and
				  go home. */
int find_num_members (SOCIETY *, int); /* Finds how many members this
					  society has with these kinds
					  of flags (if the flags are 0, the
					  total pop is returned. */


void check_society_alert (SOCIETY *);
void society_alert (SOCIETY *); /* This puts the society on alert and
				   brings all the members to a central spot. */
void society_fight (SOCIETY *); /* Makes the society lash out and fight
				   as a mass once it's gathered. */
void society_calm (SOCIETY *);  /* Makes society calm down and go back to
				   where they were. */
 
/* Raid another society. If an int is sent, the society
   must raid that society. This is used for raids from
   the alignment forcing societies to attack something. */
void update_raiding (SOCIETY *, int);

/* Attempt to raid a relic room instead of a society. Small chance of
   occuring. _start lets them gather in their homeland. _actual sends
   everyone in the gather room on the raid. */

bool relic_raid_start (SOCIETY *); 
bool relic_raid_actual (SOCIETY *);
/* When a player asks for news from a society member, the society
   member will sometimes check if there's someone/something kidnapped
   or taken and if so, it will tell the player who has the person/item. */

bool send_kidnapped_message (THING *player, THING *mob);

/* This checks if the mob is kidnapped, and if so, it updates its
   prisoner state. */

bool update_prisoner_status (THING *th);


void settle_new_society (SOCIETY *soc); /* Settle a new society. */

/* Used with alliances/alignments. */

void show_alliances (THING *); /* Shows who is aligned with whom. */

/* This name the look of a "built up" room to a player. */

char *show_build_name (THING *target); 

/* This gives the "Sector" type seen by players when they enter the
   room. */

char *show_build_sector ( THING *target);

/* This shows the build description for the player. */

char * show_build_desc (THING *target);

/* Make society members "grow" in power, */

void update_society_members (SOCIETY *soc); 

/* Make a single society member grow in power. */

THING *upgrade_society_member (THING *);


/* This checks other societies to see if they need extra members to help
   them grow. */

void reinforce_other_societies(SOCIETY *soc);


/* Checks if you can get from a room to another area avoiding rooms owned by a certain society. */
int dir_to_other_area (THING *start_room, THING *room, int lvnum, int uvnum);

void update_patrols (SOCIETY *soc); /* Small chance of sending out a patrol. */


int find_build_type (int); /* Finds the build type based on the flag given. */

/* This finds the cost of raw_type raw materials to build an enhancement
   of type build_type. */
int find_build_cost (SOCIETY *soc, int build_type, int raw_type);


/* This moves a raw material or wpn/armor from a thing to a society member
   and frees the raw and updates the society data. If it's a weapon/armor
   the item isn't freed and the player gets a small reward. */

void society_item_move (THING *th, THING *mover, THING *builder);

/* This deals with updates when a society member kills or is killed. */

void society_get_killed (THING *killer, THING *vict);


/* This returns the name of the society. */

char *society_name (SOCIETY *soc);

char *society_pname (SOCIETY *soc);



THING *find_weakest_room (SOCIETY *soc); /* Finds the room in the society
					    rooms with the most powerful
					    enemy concentration. */

/* This returns TRUE only if the area in question exists and if either
   the area is neutral or its of the same align as the society and
   there are only a couple of societies on the same align as the society
   in the area and there are no clones of the society of the same align
   in the area already. */

bool society_can_settle_in_area (SOCIETY *soc, THING *ar);

/* This updates the populations of ALL society members. */
void update_society_population (SOCIETY *);

/* This modifies the population when a certain member is added or 
   removed from the society. */
void update_society_population_thing (THING *, bool);

/* This gives a society member a defensive bonus if it's inside of its
   homeland. */

int society_defensive_damage_reduction (THING *, int);

/* This makes a society attack a certain name. */

void society_name_attack (SOCIETY *, char *);

/* These are functions used with raids. */
RAID *new_raid(void);   /* Make a new raid. */
void free_raid (RAID *); /* Remove a raid from the table and set it aside for
			   reuse. */
void add_raid_to_list (RAID *); /* Adds a raid to the table of raids. */
RAID *find_raid_num (int num); /* Find a raid based on the number given. */
void update_raids(void);       /* Update all of the raids on the raid list. */
int find_open_raid_vnum (void); /* Finds the first unused raid vnum. 
				  up to 50000...but there should never
				  be 50000 raids anyway. :) */
void write_raids (void);    /* Write the raids out to the file. */
void read_raids (void);     /* Read the raids in from the file. */
RAID *read_raid (FILE *);    /* Reads a raid in from a file. */


bool sleeping_time (THING *); /* This makes a thing go sleep someplace. */


void find_sleeping_room (THING *); /* This finds a room where this thing
				      can sleep. It scans all rooms with
				      VAL_BUILDS the same as for this
				      society, and with BUILDING_HOUSE
				      set on them. */

/* These functions deal with finding areas adjacent to a particular area
   so that raids/settlers can settle nearby instead of going across the
   whole world. */

/* This marks all areas adjacent to the given area. i.e. There is an 
   exit from the area to the area being tested. And the area being
   tested cannot be the start_area. */

void mark_adjacent_areas (THING *ar);


/* This sets up each room in the society area as a "City" room so that
   the builders will build it up. */

void setup_city (SOCIETY *soc);

/* This sets up a block of rooms to be a caste house within a society. */

void setup_caste_house (SOCIETY *, int);

/* This is a little dfs-type function that marks all unused rooms near
   a starting point with their proper caste affiliations. */

void setup_caste_house_rooms (THING *, SOCIETY *, int flags, int dir_from, int depth_left);

/* This gives the room name of a city based on the thing 
   population that lives there and whether or not that room 
   is a special one for castes. */

char *find_city_room_name (SOCIETY *, int flags);

/* This updates a built room to start getting overgrown if the city
   that built it goes away. */

void update_built_room (THING *room);

/* This makes a bunch of society members start to engage in some 
   activity. It is explained in socirts.c */

THING * society_do_activity (SOCIETY *soc, int percent, char *arg);


/* This creates a society name for a society member and stores the name 
   in the society member's socval word. */

char *create_society_name (VALUE *socval);


/* This sets the society member's name and short desc */

void setup_society_name (THING *th);

/* This sets up a society member's name if they are a crafter or shopkeeper. */

void setup_society_maker_name (THING *th);

/* These find if the society needs crafters or shopkeepers. */

int find_crafter_needed (SOCIETY *);
int find_shop_needed (SOCIETY *);


/* These are for the society needs. */

NEED *read_need (FILE *);
void write_need (FILE *, NEED *);

NEED *new_need(void);
void free_need (NEED *);

void need_to_thing (THING *, NEED *);
void need_from_thing (NEED *);
/* Copy needs from old to new...*/
void copy_needs (THING *told, THING *tnew);

/* This updates a society member's needs. */

/* If a society has needs and this thing might be able to 
   handle the needs, then have it handle those needs. */

bool create_society_member_need (THING *);

/* This tells if a society member successfully created an item or not. */

bool made_needed_item (THING *, int vnum);

/* This finds a shop that can satisfy the needs of a particular thing. */
/* The type can be a VAL_ type (if type < VAL_MAX) or a vnum if > VAL_MAX. */
THING *find_shop_to_satisfy_need (SOCIETY *soc, int type);

/* This finds a crafter that can make a particular thing. If the
   type is < PROCESS_MAX we look for a crafter that can do that
   process, otherwise we check if the object has a VAL_MADEOF and if so
   we see if the crafter can carry out the process in the VAL_MADEOF or not. */

THING *find_crafter_to_satisfy_need (SOCIETY *soc, int type);

/* This makes a society member try to buy what he/she wants. */

void society_satisfy_needs (THING *);

/* This gives a society thing a need of this type. */

void generate_need (THING *th, int vnum, int type);

/* This shows the needs this thing has to another thing. */

void see_needs (THING *th, THING *targ);

/* This makes a worker find some kind of need to satisfy. */

bool worker_generate_need (THING *);

/* This checks if a worker is getting raw materials to help somebody
   satisfy their needs. */
 
bool dropoff_raws_for_needs (THING *th);

/* This makes a thing hunt a dropoff mob for this need. */

bool start_hunting_dropoff_mob (THING *, NEED *);


/* These let you generate societies just using names and simple flags. 
   The place where you put the prototypes for the societies is in
   SOCIETYGEN_AREA_VNUM...all of the nonrooms there attempt to generate
   societies. */

void generate_societies (THING *);

SOCIETY * generate_society (THING *);

/* Clears all societies and nukes "generated" societies. */
void society_clearall (THING *);

/* This finds the tiernames available to a society in a certain caste. */

int num_caste_tiernames (char tiernames[SOCIETY_GEN_CASTE_TIER_MAX][STD_LEN],
			 int caste_flags, THING *proto);
   

/* Generates some objects for the society. */

int generate_society_objects (SOCIETY *soc, int level, int curr_vnum);


/* This adds society objects to a society member that's being upgraded. */

void add_society_objects (THING *th);


/* This generates a random society name. The name type is pname aname or
   anything else to generate the various types of society names. The
   only_generated option decides if it's taking the name from all 
   societies or only the generated ones. */

char *find_random_society_name (char name_type, int generated_from);

/* This finds a random society (out of the indestructible ones!) */

SOCIETY *find_random_society (bool only_generated);


/* This lets a society member raise the dead. */

bool society_necro_generate (THING *th);

/* Find a random generated society. */
SOCIETY *find_random_generated_society (int align);

/* This generates a society. */

void society_generate (THING *proto);


/* This finds a random name of a caste tier in one of the given
   castes. */

char *find_random_caste_name_and_flags (int caste_flags, int *return_flags);

/* This makes a society under assault run away and try to find a new
   home. */

void society_run_away (SOCIETY *soc);

/* This causes a society to get a disease. */

void society_get_disease (void);

/* This lets a society member reward a player for helping out. It gives
   money, exp and quest points. */

bool society_give_reward (THING *giver, THING *receiver, int type, int reward);

/* This makes somebody in the room give a reward to the player. */

bool society_somebody_give_reward (THING *player, int type, int reward);


/* This adds a reward to a player. */

void add_society_reward (THING *th, THING *to, int type, int amount);

/* This updates the rewards that are "owed" to a player. In one case, this
   decrements the rewards they've earned. In another, it pays them for 
   doing something good. */

void update_society_rewards (THING *th, bool payout_now);

/* This lets you add or subtract morale from a society so that the
   amounts stay within the proper ranges. */

void add_morale (SOCIETY *soc, int amount);

/* This checks if a society can add an overlord. */

bool society_can_add_overlord (SOCIETY *);

/* This checks if a society already has an overlord and returns it if it
   does. */

THING *find_society_overlord (SOCIETY *soc);


/* This makes a society attack a package holder to keep him/her from
   delivering it. */

void attack_package_holder (THING *obj);
