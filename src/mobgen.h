

/* Mobs >= this level are strong, otherwise weak. */

#define STRONG_RANDPOP_MOB_MINLEVEL   70


/* The various parts of a mobgen name. */
#define MOBGEN_NAME_A_AN         0
#define MOBGEN_NAME_PROPER       1
#define MOBGEN_NAME_ADJECTIVE    2
#define MOBGEN_NAME_SOCIETY      3
#define MOBGEN_NAME_JOB          4
#define MOBGEN_NAME_MAX          5


/* Used in find_randpop_mob_name () */

#define RANDMOB_NAME_A_AN        0x00000001 /* Return an a/an in name. */ 
#define RANDMOB_NAME_FULL        0x00000002 /* Use full name */
#define RANDMOB_NAME_STRONG      0x00000004 /* Use strong mobs. */
#define RANDMOB_NAME_TWO_WORDS   0x00000008 /* Use two words in the name. */

/* These are for generating regular mobs within an area. */

void areagen_generate_persons (THING *area);

/* This generates a person in an area and gives it a level bonus. */

THING *areagen_generate_person (THING *area, int level_bonus);

/* Find a society name to give a mob. */

SOCIETY *persongen_find_society (void);

/* Find a person type to give to the person...it adds some stuff like flags
   and values and alters levels and things of that nature. */

THING *find_persongen_type (void);

/* This creates some objects and adds their resets to this person. */

void add_objects_to_person (THING *person, char *name);



/* These are used to generate base/background animal/creature type
   mobs within areas. */


void clear_base_randpop_mobs (THING *);

void generate_randpop_mobs (THING *);

/* This generates a single randpop mob for an area based on the names
   given. */

THING *generate_randpop_mob (THING *area, THING *proto, int curr_vnum);


/* This sets up the mob randpop item. */

void setup_mob_randpop_item (int start_vnum, int tier_size);


/* This tells if an area has a free mobject vnum in it or not. */

int find_free_mobject_vnum (THING *area);


/* this finds a random mobgen name. */

char *find_randpop_mob_name (int flags);
