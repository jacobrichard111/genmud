

/* This handles stuff dealing with notes. */


/**********************************************************************/
/* This is one note. */
/**********************************************************************/

struct note_data 
{
  NOTE *next;
    char *sender;      /* Name of sender. */
    char *to;          /* List of recipients. */
    char *subject;     /* What the note is about. */
    char *message;     /* The message the note contains. */
  char *adminresp;   /* Admin response...neat idea taken from ???? */
    int time;          /* When it was posted. */
};

extern NOTE *note_free;
extern NOTE *note_list;    /* This is the list of notes... */
extern int note_count;
extern int top_note;

/**********************************************************************/
/* These are functions used with note writing. */
/**********************************************************************/

bool can_see_note (THING *, NOTE *);
void show_note_info (THING *, NOTE *);
void show_note (THING *, NOTE *);
NOTE *new_note (void);
void get_new_note (THING *);
void free_note (NOTE *);
void read_notes (void);
void write_notes (void);
NOTE *read_note (FILE *);
