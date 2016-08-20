


/* Number of parts to an objgen name. */

/* The type part of the name. */
#define OBJECTGEN_NAME_OWNER     0
#define OBJECTGEN_NAME_A_AN      1
#define OBJECTGEN_NAME_PREFIX    2
#define OBJECTGEN_NAME_COLOR     3
#define OBJECTGEN_NAME_GEM       4
#define OBJECTGEN_NAME_METAL     5
#define OBJECTGEN_NAME_NONMETAL  6
#define OBJECTGEN_NAME_ANIMAL    7
#define OBJECTGEN_NAME_SOCIETY   8
#define OBJECTGEN_NAME_TYPE      9
#define OBJECTGEN_NAME_SUFFIX   10
#define OBJECTGEN_NAME_CALLED   11
#define OBJECTGEN_NAME_MAX      12


/* Min level a mobgen proto must be before it will be used to generate
   object suffixes. */

#define OBJECTGEN_ANIMAL_SUFFIX_MINLEVEL  40


/* Only N affects per rank in the object. */
#define MAX_AFF_PER_RANK 2

/* Power rank and power likelihood. */
#define AFF_POWER_RANK    0
#define AFF_POWER_CHANCE  1
#define AFF_POWER_TYPES   2

/* The ranks of power that things have. Also, the likelihood of each
   type of affect showing up. */

extern const int aff_power_ranks[AFF_MAX-AFF_START][AFF_POWER_TYPES];

/* This is the function that generates an object. It attempts to 
   make an object of a certain level of a certain type (that can be
   specified or chosen randomly) and in a certain area. 
   You can also specify the vnum you want to fill right away and
   you can specify a society name or an owner name if you want. */

THING *objectgen (THING *area, int wear_loc, int level, int curr_vnum,
		  char *names);

/* This calculates a random wear location: 20pct wield (weapon),
   70pct single (large) armor, 20pct jewelry (small) armor. 
   The size is based on the number of wear slots you have: 1 or
   more than 1. */


int objectgen_find_wear_location (void);

/* This returns an item name based on the type requested. */

char *objectgen_find_typename (int wear_loc, int weapon_type);

/* This generates all of the names that will go into the object. */

void objectgen_generate_names (char name[OBJECTGEN_NAME_MAX][STD_LEN], 
			       char color[OBJECTGEN_NAME_MAX][STD_LEN],
			       int wear_loc, int weapon_type, int level,
			       char *names);
/* This sets up the various names on the object that we're actually making. */

void objectgen_setup_names (THING *obj, char name[OBJECTGEN_NAME_MAX][STD_LEN],
			    char color[OBJECTGEN_NAME_MAX][STD_LEN],
			    char *preset_names);

/* This sets up the stats and bonuses for the object. */

void objectgen_setup_stats (THING *object, int weapon_type);

/* This returns a random weapon type...weighted toward slashing. */

int generate_weapon_type (void);

/* This generates some fake metal names. */

void generate_metal_names (void);

/* This generates provisions like food, drinks, bags and tools. */

void generate_provisions (THING *);

/* This clears the provisions load area. */

void clear_provisions (THING *);

/* This generates an animal name like "lizard skin" "wolf tooth" or something
   like that. */

char *objectgen_animal_part_name (int wear_loc, int weapon_type);

/* This generates an animal name based on the level of the animal...
   only animals with base levels >= 40 are checked. Like
   sword of the lizard and so forth. */

char *objectgen_animal_suffix_name (void);


/* This finds an objectgen part name given a string. */

int find_objectgen_part_name (char *);

/* This finds an affect type of a certain rank. */
int find_aff_with_rank (int rank);


/* This calculates the reset percent for an object based on its
   level. */

int objectgen_reset_percent (int level);
