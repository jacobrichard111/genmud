

/* Types of quests you can generate. */


#define QUEST_NONE                 0
#define QUEST_GET_ITEM_RANDMOB     1


#define QUEST_MAX                  2

#define QUEST_THING_PERSON        0     /* These things can give quests. */
#define QUEST_THING_MOBGEN        1     /* These can be targets of quests. */
#define QUEST_THING_ITEM          2     /* Items generated based on the
					   thing that they pop into. */
#define QUEST_THING_ROOM          3     /* Rooms used for quests. */

#define QUEST_THING_MAX           4



/* This clears all of the quests set on mobs within the worldgen
   areas. */


void worldgen_clear_quests (THING *th);


/* This generates all of the worlds' quests. */

void worldgen_generate_quests (void);

/* These generate single-area fedex quests. */

void questgen_fedex_single_areas (void);
void questgen_fedex_single_area (THING *area);

/* This generates quests between areas. */


void questgen_fedex_multi_areas (void);

/* This generates a quest between two (possibly same) areas of a 
   certain type. */

void questgen_generate (THING *start_area, THING *end_area, int quest_type);


/* This finds a quest mob in a thing of a certain type. */

THING *find_quest_thing (THING *in, THING *for_whom, int type);


/* These generate the actual scripts used in the quests. */

void generate_setup_script (THING *quest_giver, THING *quest_mob, THING *quest_room, THING *quest_item);
void generate_given_script (THING *quest_giver, THING *quest_item);

/* This finds a word based on the kind of word given to the function. It
   works a lot like objectgen_find_word without the colors and special
   wielded names. */

char *questgen_find_word (char *type);

/* This is a counter used to iterate the questgen names. It was just
   easier to do it recursively. :P */

void questgen_name_iterate (char *txt);
