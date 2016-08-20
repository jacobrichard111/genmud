

/* This is for the history generator. The idea is to make the
   histories of a sequence of ages. Each age looks at the list of
   societies and makes them "fight" and so forth and sets up
   leaders for different societies at different points and makes
   societies come into existence and leave. It also makes up some
   ancient protector/evil races and so forth so that when the players
   are in the world, they have certain things that they must ally with
   or avoid. */

#define MAX_GODS_PER_ALIGN          10

#define DEITY_LEVEL                 500 /* What level the gods are. */
#define DEITY_SUMMON_COST        10000  /* Wps needed to summon a god. */
#define DEITY_SUMMON_HOURS          30 /* Hours summoned deity is here. */
extern char *old_races[ALIGN_MAX];
extern char *old_gods[ALIGN_MAX][MAX_GODS_PER_ALIGN];
extern char *old_gods_spheres[ALIGN_MAX][MAX_GODS_PER_ALIGN];
extern const char vowels[NUM_VOWELS+1];
/* Read and write the historygen data. */

void read_history_data (void);
void write_history_data (void);

/* Initialize the history vars. */

void init_historygen_vars (void);

/* Generates all of the history. */
void historygen (void);

/* Set up the historygen data. */
void historygen_setup (void);

/* Creates the ancient races each alignment has...*/
void historygen_generate_races (void);

/* Generate gods for each race. */
void historygen_generate_gods (THING *);

/* Generate organizations for players to use to get help. */
void historygen_generate_organizations (void);


/* This generates the past history that tells of the times of peace
   and so forth. */

char *historygen_past(void);

/* This tells of the bad thing that ended the good times. */

char *historygen_disaster(void);

/* This tells how the world is in flux now and the old powers are stirring
   and how you should go make your mark. */

char *historygen_present(void);


/* This clears some of the historygen data. Mainly the gods and their
   items. */

void historygen_clear(void);

/* This creates a deity and gives it equipment. */

void generate_deity (char *name, char *spheres, int align);

/* This lets a deity hunt and kill really fast. */

void deity_hunt (THING *th);
