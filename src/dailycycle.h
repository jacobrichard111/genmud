/* This is used to make mobs follow a daily cycle of sleeping/working
   so that players find it pretty reasonable watching them doing stuff. */

bool update_daily_cycle (THING *);

/* This finds a place nearby where this thing can get something
   to eat. */

THING *find_nearby_inn (THING *);

/* This tells if the current location is an inn or not. */

bool is_an_inn (THING *room);

/* These times are set after the start time that a person has. Most
   people will wake up at 6-8 am. */

#define CYCLE_HOUR_WAKE_UP       0    /* Get up at this time. */
#define CYCLE_HOUR_AM_WORK       1    /* Go to work at this time. */
#define CYCLE_HOUR_LUNCH         5    /* Eat lunch now. */
#define CYCLE_HOUR_PM_WORK       6    /* Go to work now. */
#define CYCLE_HOUR_DINNER       10    /* Eat dinner now. */
#define CYCLE_HOUR_HOME         12    /* Go home now. */
#define CYCLE_HOUR_SLEEP        16    /* Go to sleep now. */

#define CYCLE_HOUR_MAX           7    /* Number of cycle hours...*/
