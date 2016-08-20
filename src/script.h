/* This file has the stuff related to scripts. */


/**********************************************************************/
/* These are trigger flags for scripts. Certain script actions trigger
   certain things. Do we want this as flags or as numbers 0-N? hmm */
/**********************************************************************/

#define TRIG_TIMED      0x00000000  /* Timed...not checked on action. */
#define TRIG_TAKEN      0x00000001  /* Checked if a thing gets this */
#define TRIG_MOVED      0x00000002  /* Checked if a thing moves this */
#define TRIG_MOVING     0x00000004  /* Checked when this moves self */
#define TRIG_GIVEN      0x00000008  /* Checked when this receives a thing */
#define TRIG_DROPPED    0x00000010  /* Checked when this is dropped. */
#define TRIG_TAKEN_FROM 0x00000020  /* Checked if something is taken from it */
#define TRIG_GET        0x00000040  /* Checked when this gets a thing */
#define TRIG_LEAVE      0x00000080  /* Checked if a thing leaves this */
#define TRIG_ENTER      0x00000100  /* Checked if a thing enters this */
#define TRIG_CREATION   0x00000200  /* This does something when created */
#define TRIG_DEATH      0x00000400  /* This does something when killed */
#define TRIG_SAY        0x00000800  /* This gets triggered on says */
#define TRIG_TELL       0x00001000  /* Checked when told something. */
#define TRIG_COMMAND    0x00002000  /* Command trigger */
#define TRIG_ARRIVE     0x00004000  /* Checked when this arrives someplace. */
#define TRIG_BRIBE      0x00008000  /* Bribe--given money. Keyword is amt. */
#define TRIG_MAX        0x00010000  /* This is the max trigger...useful */


#define TRIG_INTERRUPT  0x00000001 /* Can this trigger be interrupted
				      and restarted? */
#define TRIG_LEAVE_STOP 0x00000002 /* The trigger stops if someone leaves. */
#define TRIG_NUKE       0x00000004 /* Dont' save this trigger. */
#define TRIG_PC_ONLY    0x00000008 /* Only pc's trigger this. */


/* Code flags. */

#define CODE_NUKE       0x00000001   /* Don't save this code. */
#define CODE_META       0x00000002   /* Meta code, not regular code. Used
					to generate quests. */

/**********************************************************************/
/* These are the various "signals" that the script interpreter can
   return which then have to be handled by the code. */

/**********************************************************************/

#define SIG_CONTINUE                -1 /* Continue the script */
#define SIG_QUIT                     0 /* Quit the script */

/* Signals > 0 represent "wait times"... */
/**********************************************************************/
/* These structs are used with scripting */
/**********************************************************************/

/**********************************************************************/
/* This is an script_event which at a certain TIME will continue to execute CODE
   starting at line LINE. The "run" is the thing that is running the
   script, and the "trig" is the thing that triggered the script. 
   Others are other THINGS which are forced to be part of the action. */
/**********************************************************************/

struct script_event_data
{
  int time;     /* Time when it starts */
  char *code;   /* Code it exectutes */
  int val[10];  /* Variables you can use in the scripts */
  char *start_of_line; /* Start of the current line. */
  TRIGGER *trig;
  int depth;    /* How many functions deep this is. */
  SCRIPT_EVENT *called_from; /* What script_event this was called from */
  SCRIPT_EVENT *calling;   /* What script_event this one called */
  THING *runner; /* Thing running this */
  THING *starter; /* Thing which started this */
  THING *obj;    /* The object involved..if any */
  THING *thing[10]; /* Other things that the script deals with. */
  SCRIPT_EVENT *next;   /* Next script_event in the list */
};

/**********************************************************************/
/* A trigger is something that is attached to a certain thing number,
   and when some action gets done to or near that thing, it may cause
   a script to happen. */
/**********************************************************************/

struct trigger_data 
{
  char *name;  /* Name of the trigger */
  int type;    /* What sort of action will cause this to trigger? */
  int timer;   /* If this is constantly repeated, what time interval is it? */
  int flags;   /* Various flags associated with this */
  int vnum;    /* What thing number runs this */
  int pct;     /* Percentage chance it gets called */
  char *code;  /* Name of the code used with this */
  char *keywords; /* Which keywords are used to trigger this (if any) */
  TRIGGER *next;
};


/**********************************************************************/
/* Simply, a piece of CODE is a name and a list of code. Easy to work with.
   Hashed by 1st/2nd letters of code name. */
/**********************************************************************/

struct code_data
{
  char *name;   /* The name for this code. */
  char *code;   /* The actual code string. */
  CODE *next;   /* The next code in the table/list. */
  int flags;    /* Flags on this code. */
};

 
/* Globals. */


extern CODE *code_hash[CODE_HASH][CODE_HASH];
extern TRIGGER *trigger_hash[HASH_SIZE];
extern SCRIPT_EVENT *script_event_list;
extern TRIGGER *trigger_free;
extern SCRIPT_EVENT *script_event_free;
extern CODE *code_free;
extern int script_event_count;
extern int code_count;
extern int trigger_count;

/* Types of triggers */

extern const char *trigger_types[];

/* Functions. */

void remove_from_script_events (THING *);
char *script_math_parse (SCRIPT_EVENT *, char *); 
/**********************************************************************/
/* These are script related functions. This first section deals with 
   I/O and creating and freeing. */
/**********************************************************************/

SCRIPT_EVENT *new_script_event (void);
void free_script_event (SCRIPT_EVENT *);

TRIGGER *new_trigger (void);
void free_trigger (TRIGGER *);
void read_triggers (void);
void read_trigger (FILE *);
void write_triggers (void);
void write_trigger (FILE *, TRIGGER *);

CODE *new_code (void);
void free_code (CODE *);
void read_codes (void);
CODE *read_code (FILE *);
void write_codes (void);
void write_code (FILE *, CODE *);

void check_triggers (THING *initiator, THING *mover, THING *start_in, THING *end_in, int flags);
void check_trigtype (THING *, THING *, THING *, char *arg, int);
void run_script (TRIGGER *, THING *, THING *, THING *);
CODE *find_code_chunk (char *);
void add_code_to_table (CODE *);
TRIGGER *find_trigger (char *);
void show_trigger (THING *);
char* show_trigger_type (int);
void add_script_event (SCRIPT_EVENT *);
void setup_timed_triggers (void); /* Sets up timed triggers */
void remove_script_event (SCRIPT_EVENT *);
void check_script_event_list (void);
int run_script_event (SCRIPT_EVENT *);
int run_code (SCRIPT_EVENT *);
int run_code_line (SCRIPT_EVENT *, char *);
char *replace_one_value (SCRIPT_EVENT *, char *, THING *, bool, char, char *);
char *replace_values (SCRIPT_EVENT *script_event, char *txt);
int do_operation (int, char, int);
void trigedit (THING *, char *);

/**********************************************************************/
/* These are the actual kinds of lines which are allowed in scripts   */
/**********************************************************************/

int math_com_line (SCRIPT_EVENT *, char *);
int *script_thing_value_loc (THING *, char *);
int if_com_line (SCRIPT_EVENT *, char *);
int goto_com_line (SCRIPT_EVENT *, char *);
int make_com_line (SCRIPT_EVENT *, char *);
int nuke_com_line (SCRIPT_EVENT *, char *);
int wait_com_line (SCRIPT_EVENT *, char *);
int do_com_line (SCRIPT_EVENT *, char *);
int call_com_line (SCRIPT_EVENT *, char *);
