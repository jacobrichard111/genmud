
/* This is how many words there are for things littering a room. */

#define ROOMDESC_LITTER_MAX   51
#define ROOMDESC_LITTER_AMOUNTS_MAX 12
#define ROOMDESC_LITTER_SUFFIX_MAX  8
/* Number of different ground temperatures. */

#define ROOMDESC_GROUND_TEMP_MAX      13
#define ROOMDESC_GROUND_HARDNESS_MAX  9 
#define ROOMDESC_GROUND_DRYNESS_MAX   10




/* This creates a room description. */

char *generate_room_description (THING *room);
