







/* Max 50 items can be used for crafting. */
#define CRAFT_MAX_USE_ITEMS    50
#define CRAFT_MAX_MAKE_ITEMS   100


/* These are flags used when checking the items needed for crafting. */

#define CRAFT_DO_PROCESS       0x00000001  /* Do the process. */
#define CRAFT_SHOW_MATERIALS   0x00000002  /* Just show the materials needed.*/

/* This generates the items used in crafting. */

void generate_craft_items (THING *);

/* This adds crafting commands for the craft processes. */

void add_craft_commands (void);

/* When a player types a command, it calls this function that attempts
   to make the player craft something. */

bool check_craft_command (THING *th, char *arg);

/* This clears the items used in crafting. */

void clear_craft_items (THING *);


/* This finds the base craft prototype for an object. */

THING *find_base_craft_proto (THING *);


/* This lets a player craft or gather an item. */

void craft_item (THING *th, char *itemname, char *typename, bool show_materials);

/* This checks if a player actually has the crafte materials, and if so,
   it deletes the resources used and makes the new items if 
   do_the_process is true. Otherwise, it returns messages saying
   what's wrong. */

bool check_craft_materials (THING *th, THING *item, THING *proto, EDESC *process, int flags);

/* This shows a list of crafting commands to a player. */

void show_crafting_commands (THING *);
