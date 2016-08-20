

#define RANDOBJ_COUNT_PER_TIER  500 /* N items per randobj tier. */
#define RANDOBJ_TIERS  5   /* N tiers of randobj items. */
#define RANDOBJ_TIER_LEVEL_JUMP  40 /* Each tier of randobj eq jumps
				       this many levels. */

/* Do this from within the worldgen code. Generate objects then seed 
   the randobj repop on mobs within the game. */

void worldgen_randobj_generate (void);

/* Clean it all up. */

void clear_randobj_items (THING *);



