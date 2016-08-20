


/* Each job in the quest gets put into a job. */

typedef struct _job_data JOB;

/* The kinds of jobs we can get. */

#define JOB_NONE          0
#define JOB_GIVE_ITEM      1
#define JOB_KILL_THING    2
#define JOB_GIVE_PASSWORD 3
#define JOB_GIVE_MONEY    4
#define JOB_TYPE_MAX           5

/* The stages of a job: */

#define JOB_CANSTART     0   /* Can we start this quest? Usually the
				      job_stage_completed from prev quest
				      unless this is a middle prereq in
				      which case it's if we've started
				      the stage that needs the result of
				      this. */
#define JOB_DIDSTART     1   /* Did we start this job? */
#define JOB_FINISHED     2   /* Did we finish this job? Usually the
				      same as the CANSTART for the next
				      quest. */
#define JOB_STAGE_MAX          3

#define JOB_PREV_MAX 5   /* Max 5 jobs to complete a current job. */  


/* Types of rewards given. */
#define QUEST_REWARD_ITEM        0
#define QUEST_REWARD_MONEY       1
#define QUEST_REWARD_PASSWORD    2
#define QUEST_REWARD_EXPERIENCE  3
#define QUEST_REWARD_MAX         4



struct _job_data
{
  JOB *prev[JOB_PREV_MAX]; /* Jobs needed to complete to do this quest. */
  JOB *next;        /* What the next stage of the quest is. */
  int runner;      /* Who's running this quest. */  
  int runner_room;  /* What room the runner is in. */
  int job_type[JOB_PREV_MAX]; /* What and how much of it we need. */
  int job_num[JOB_PREV_MAX];  /* What num/how much we're looking for. */ 
  char job_password[JOB_PREV_MAX][100]; /* Password we're looking for. */
  int reward_type; /* What kind of reward...password? Object? Cash? */
  int reward_num;  /* What the number of the reward is...obj/cash, passwd? */
  char reward_password[100]; /* Password used for next stage. */
  int stage;       /* What stage of the quest this is. */
  int qf_num;      /* What qf num we use. */
  char qf_name[20]; /* What the name of the quest flag we're using is. */
};

/* This generates all quests in the world. */
void generate_quests (void);
/* This generates a single quest. */
JOB *generate_job (JOB *last_job);

/* Create and destroy jobs -- These are NOT counted as memory since you
   really shouldn't have too many of these anyway. */
JOB *new_job(void);
/* We only free all jobs at once or else we may have memory leaks. */
void free_jobs (JOB *);

/* This increments the qf bits that name each script. */
void qf_inc_bits (void);

/* This sets the qf_bits on a job to tell when it's started. */
void setup_job_start_qf (JOB *job, JOB *from);

/* This sets the job qf bit to the next one on the list. */
void set_job_qf_bit (JOB *job, int job_stage);

/* This sets up the various stages needed to complete the quest. It tells
 what job this was setup from so that you know whether or not you
 have to look at this as a prereq or the next step. */
void setup_job (JOB *job, JOB *from);

/* Read in the "templates" for the quest code -- both regular and META. */
void read_quest_chunk_types (void);

/* This returns a questgen initial string to start a quest. */
char *questgen_init_string (int type);

/* This finds an area where a quest can go. */
THING *find_quest_area (void);
