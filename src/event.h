

/* Two types of functions we can call. */
typedef void EVENT_FUNCTION ();
/* This is the beginnings of writing an event driver for general MUD
   activity outside of the little script engine I have. */


/* The events will be kept in a hash table indexed wrt the
   number of pulses since startup. Some events will be linked
   to things and will cause the thing to do something like updating
   combat or making it track, or making it heal or something.
   
   Some will just be global events. 

   The engine will be limited for now because I am learning how to
   do this, so the events have to have specific types as opposed to
   just calling arbitrary functions on arbitrary things.

   Hopefully all updates will eventually be put into this code.
   It will also make things act in "clumps" a lot less since they
   can all have their own individual updates. :) */


struct _event_data 
{
  EVENT *th_prev;   /* Previous event on this thing. */
  EVENT *th_next;   /* Next event on this thing. */
  EVENT *next;      /* Next event in the queue. */
  EVENT *prev;      /* Previous event in the queue. */
  THING *th;        /* Who owns or runs this event. */
  THING *attacker;  /* Who is attacking this thing. */
  char *arg;        /* The argument sent to the command (if any). */
  int time;         /* When the event will occur. */
  int delay;        /* Pulses until the next time it's called. */
  short times_left; /* Number of times to repeat this event. */
  int flags;        /* Flags used to set up this event. */
  EVENT_FUNCTION *callback; /* What gets called when the event is triggered. */
};

/* Constants associated with events. */

#define EVENT_HASH 3000

/* Event flags. */

#define EVENT_REPEAT     0x00000001  /* Event repeats forever. */
#define EVENT_DAMAGE     0x00000002  /* Special damage event. */
#define EVENT_COMMAND    0x00000004  /* Special command event. */
#define EVENT_TRACK      0x00000008  /* Special track events. */
#define EVENT_DEAD       0x00000010  /* Dead event--don't execute. */


/* Variables associated with events. */

extern int event_count;    /* Number of events created. */
extern EVENT *event_free;  /* List of free events available for reuse. */
extern EVENT *event_hash[EVENT_HASH]; /* Table of events. */ 

/* Functions associated with events. */

EVENT *new_event(void); /* Returns a new event for use. */
void free_event (EVENT *); /* Removes an event from the world. */

/* This creates a repeating event based on these arguments. Other arguments 
   can be set by hand...be aware that this sets EVENT_REPEAT flag. */

EVENT *create_repeating_event (THING *, int delay, EVENT_FUNCTION *callback);


void event_to_thing (EVENT *, THING *); /* Adds an event to a thing. */
void event_from_thing (EVENT *); /* Removes an event from a thing's list. */

void event_from_list (EVENT *); /* Removes an event from the big list. */
void event_to_list (EVENT *);   /* Adds an event to the big list. */

/* This adds the "Standard" events to a new thing. */
void add_events_to_thing (THING *);

/* This removes all of the events from a thing. It should be called 
   only when the thing is freed. */
void remove_events_from_thing (THING *);


/* This sets up the global events at bootup. */
void set_up_global_events (void);


/* Run all of the current events. */

void update_events (void); 
void exec_event (EVENT *); /* This runs an event. */

/* Add a track event to a player. */

void add_track_event (THING *th);

/* Lets you add a command event that delays a certain command from
   taking place for a certain number of ticks. */

void add_command_event (THING *th, char *arg, int ticks);


/* This handles a few special cases of events. */

void special_event_handler (EVENT *);

/* This removes events of a certain type. Command events get a little extra
   code so that the player who stops using the command knows what 
   happened. */


void remove_typed_events (THING *th, int type);


/* Run a command event. */

void do_command_event (EVENT *);


/* This carries out a damage event. */

void do_damage_event (EVENT *);


/* This adds a damage event. */


void add_damage_event (THING *th, THING *vict, SPELL *spl);


/* This adds a free thing event. It is done to prevent the problem of 
   freeing a thing that's being saved at that moment by a different
   thread. */

void add_free_thing_event (THING *th);






