/* The MIT License
   
Copyright (c) 2001-2004 John R. Arras.

Permission is hereby granted, free of charge, to any person 
obtaining a copy of this software and associated documentation 
files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the 
Software, and to permit persons to whom the Software is furnished 
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be 
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
    
*/


#include <time.h>
#include <sys/time.h>

/* Used for arkane's special code. */
#undef ARKANE
/* Used to add the 2000x2000 wilderness where the code is found in
   wilderness.* and wildalife.*. The integration code is found
   throughout the game, so it's just easier to undefine it rather
   than taking it out. I found that it's no longer necessary now
   that I can generate entire worlds on the fly. If you decide to use
   the wilderness, you will also need to take the # out of the line
   in the makefile that starts the wildalife/wilderness.o line. */
#undef USE_WILDERNESS

/* As a general rule, if you change something in any header file,
   type make clean and then make to rebuild the entire codebase.
   Otherwise bad things can happen in terms of numbers being wrong
   and pointers pointing to the wrong piece of memory. */


/**********************************************************************/

   /* SECTION typedefs */

/**********************************************************************/


typedef unsigned char bool;
typedef struct file_desc FILE_DESC;
typedef struct _cmd CMD;
typedef struct flag_actual FLAG;
typedef struct flag_data FLAG_DATA;
typedef struct _value_actual VALUE;
typedef struct flag_pointer FLAG_POINTER;
typedef struct pc_data PCDATA;
typedef struct spell_data SPELL;
typedef struct clan_data CLAN;
typedef struct note_data NOTE;
typedef struct fight_data FIGHT;
typedef struct trigger_data TRIGGER;
typedef struct code_data CODE;
typedef struct help_data HELP;
typedef struct help_keyword HELP_KEY;
typedef struct auction_data AUCTION;
typedef struct script_event_data SCRIPT_EVENT;
typedef struct reset_data RESET;
typedef struct race_data RACE;
typedef struct _extra_description EDESC;
typedef struct channel_data CHAN;
typedef struct track_data TRACK;
typedef struct society_data SOCIETY;
typedef struct pkill_data PKDATA;
typedef struct trophy_data TROPHY;
typedef struct social_data SOCIAL;
typedef struct bfs_data BFS;
typedef struct pbase_data PBASE;
typedef struct siteban_data SITEBAN;
typedef struct gather_raw_data GATHER;
typedef struct graft_value_data GRAFT;
typedef struct scrollback_data SCROLLBACK;
typedef struct _damage_data DAMAGE;
typedef struct mapgen_data MAPGEN;
typedef struct rumor_data RUMOR;
typedef struct _raid_data RAID;
typedef struct _event_data EVENT;
typedef struct _need_data NEED;



/**********************************************************************/
/* This is the base struct for all stuff in the world. */
/**********************************************************************/

typedef struct thing_data THING;

/* Lets you set up cli. All of these do_this do_that are set to functions
   of this form. */

typedef void COMMAND_FUNCTION (THING *, char *);

/**********************************************************************/
/* SECTION constants */
/* Most constants have a reason why they're there. If you're not sure,
   grep the constant in the .c files and see what it does. Many 
   constants require you to alter a data structure in const.c so that
   you can get access to the constants using OLC. */
/**********************************************************************/


#define TRUE 1
#define FALSE 0

#define PORT_NUMBER            3300
#define STD_LEN                1000
#define STRINGGEN_LEN          20000 /* Max size of stringen returns. */
#define WRITEBUF_SIZE          20000
#define END_STRING_CHAR        '`'
#define HASH_SIZE              57129   /* Size of the index hashes. */

/* This constant is here to keep the game from getting out of control.
   Alter this as you wish, but realize that the memory used is at least
   200 bytes per thing. */

/* Also, listen the AI in here makes the system start to EAT resources.
   If you make the UPD_PER_SECOND too big (I keep it at about 3 updates
   per second), or if you make the updates like thing updates or
   hourly updates too fast, you end up eating up a lot more CPU. This will
   keep the size of your simulation down...so keep that in mind as you
   play with these numbers. */

#define MAX_NUMBER_OF_THINGS   1500000 /* Up to N things in the world */
#define PULSE_PER_SECOND       10      /* Number of ticks per second */
#define UPD_PER_SECOND         (PULSE_PER_SECOND) /* Updates each second. */
#define NO_QUIT_REG            2       /* 2 hrs reg combat quit. */
#define NO_QUIT_PK             6       /* 6 hrs pk combat quit. */

/* So these are used for the world update functions. The update
   is run 2x a second, so halve these numbers to see the number of
   seconds between updates. See upd.c for more info. There is some
   load balancing, and certain updates are only for pc's */

#define PULSE_FAST             (2*UPD_PER_SECOND) /* Every N seconds */
#define PULSE_THING            (9*UPD_PER_SECOND) /* Every N seconds */
#define PULSE_COMBAT           (2*UPD_PER_SECOND) /* Every N seconds */
#define PULSE_HOUR             (80*UPD_PER_SECOND) /* Every N seconds */
#define PULSE_AUCTION          (14*UPD_PER_SECOND) /* Every N seconds */
#define PULSE_SOCIETY          60                /* All societies get updated
						    every minute. */

/* These constants are used with the battleground. */

#define BG_MIN_VNUM            60     /* Min vnum of battleground. */
#define BG_MAX_VNUM            80     /* Max vnum of battleground. */
#define BG_SETUP_HOURS         10     /* Number of hours to set up for bg. */
#define BG_MAX_PRIZE           10     /* Max number of bg prizes. */

/* This only gets called if there is an "hour" update */



#define START_AREA_VNUM        1       /* Start area always has vnum 1. */
#define NUM_EXITS              6       /* Number of exits in a standard room */
#define MAX_LEVEL              110
#define BLD_LEVEL              101     /* When you can start to build. */
#define MORT_LEVEL             100
#define MAX_STORE              20      /* Max number of items you can store */
#define MAX_CLAN_STORE         30      /* Max number of items in clanstore */
#define MAX_ALIAS              40      /* Max number of aliases */
#define MAX_ALIAS_REP          5       /* Max number of alias rep strings.
                                          (An alias can have up to this 
                                          many variables @1 @2 @3 etc... */
#define MAX_COMP               4       /* Max number of spell components */
#define NUM_GROUNDFIGHT_DAMAGE_TYPES 11 /* Groundfight damage names */
#define MAX_PRE_DEPTH          30      /* Max depth a prereq tree goes */
#define MAX_IGNORE             40      /* Max number of people you can 
					  ignore. */
#define CODE_HASH              27      /* Size of code hash..do not change
					  without knowing what is up. */

#define CATCHALL_VNUM          700     /* Make this when the other number
					  does not exist... */
#define CORPSE_VNUM            701     /* Make this when something dies
					  and needs a corpse. */
/* These designate vnums where no things can be created from between these
   two vnums since the things here are placeholders for other code
   to generate real things to play with. */

#define GENERATOR_NOCREATE_VNUM_MIN 101000
#define GENERATOR_NOCREATE_VNUM_MAX 110000


#define FORGED_EQ_AREA_VNUM      100000 /* Forged eq starts here. */


#define CITYGEN_AREA_VNUM       101000 /* Used to generate cities. */



#define DETAILGEN_AREA_VNUM      104000 /* Where area details get
					   generated from. */
#define PERSONGEN_AREA_VNUM     105000 /* The vnum of the area with all
					  of the persongen protos for 
					  things like warriors/wizards
					  in it. */

#define MOBGEN_PROTO_AREA_VNUM   105200 /* Where the data for the mobs
					   to be created resides. */
#define WORDLIST_AREA_VNUM    105900 /* Short phrases and words used to 
					generate names of objects. */


#define SOCIETYGEN_AREA_VNUM     106000 /* Vnum of with all of the 
					  society protos in it. */
#define CRAFTGEN_AREA_VNUM       106200 /* Generate craft items. */

#define SOCIETY_CASTE_AREA_VNUM  107000 /* These list the various castes
					   for societies and let the 
					   titles be picked when making
					   the society. */

#define AREAGEN_AREA_VNUM       107100   /*This area has the names and 
					   such for generating areas. */
#define PROVISIONGEN_AREA_VNUM  107500   /* This area is for making food 
					    and drinks and bags and tools
					    and other things. */
#define OBJECTGEN_AREA_VNUM      108000  /* These are the words used to
					    generate objects. */
#define ORGANIZATION_AREA_VNUM   108500  /* Data on organizations players
					    have on their sides. */
#define QUESTGEN_AREA_VNUM       109000  /* These are the words used to
					    generate quests. */
#define HISTORYGEN_AREA_VNUM     109500  /* Used to generate the history
					    of the world. */
#define SOCIETY_MOBGEN_AREA_VNUM 112000 /* Vnum where we put all of the
					   society mobs that get
					   generated by society_generate(). */
#define DEITY_AREA_VNUM          120000 /* Where deities and their eq go. */
#define PROVISION_LOAD_AREA_VNUM 125000 /* Where provisions load. */
#define MOBGEN_LOAD_AREA_VNUM    127000 /* Area where the mobgens
					   are created and load. */

#define CRAFT_LOAD_AREA_VNUM     135000 /* Area where craft items are made. */

#define RANDOBJ_AREA_VNUM        150000 /* Lots of random objs for mobs
					   to load. */

#define SCAN_RANGE            3 /* How many rooms away you can scan. */

/* Incidentally, these are hardcoded because I prefer to not have 
   builders making objects with these properties...and I prefer not
   having properties that will only be used by single objects. */

#define STEAK_VNUM             702     /* Used for butcher. */
#define SOUL_ALTAR             703     /* Used for sacrifice */
#define SOUL_DAGGER            704     /* Used for sacrifice */
#define SOUL_PRISON            705     /* Used for sacrifice */
#define POTION_BOTTLE          706     /* Used for brew. */
#define PARCHMENT_VNUM         707     /* Used for scribe. */
#define CRYSTAL_BALL           708     /* Dunno what this does. */
#define RELIC_VNUM             709     /* Used for relics for each align. */
#define WEAPON_RANDPOP_VNUM    270   /* Pop weapons...*/
#define ARMOR_RANDPOP_VNUM     271  /* Pop armor...*/
#define GEM_RANDPOP_VNUM       277   /* Pop gems.*/
#define MOB_RANDPOP_VNUM       278  /* Used for wilderness repops. */
#define PROVISION_RANDPOP_VNUM 279  /* Load random provisions...*/
#define RANDOBJ_VNUM           281  /* Load random items on mobs. */
#define ARMOR_DIVISOR          10   /* Each N of these on armor add one armor*/
#define AUCTION_DIVISOR        12   /* Obj->cost/this = auction fee. */
#define WGT_MULT               10   /* Ratio of pebbles to stones. */
#define NATSPELL_MAX            4   /* Natural spells known */
#define MAP_MAXX              120   /* How wide maps can be */
#define MAP_MAXY               60    /* How tall maps can be */
#define SMAP_MAXX              13   /* Small map size */
#define SMAP_MAXY              11   /* Small map size */
#define CARNIVORE_HUNT_DEPTH   4   /* Max depth a carnivore hunts on 
avg..*/
#define REMORT_ROOM_VNUM       26   /* Room where you can remort. */
#define MAX_REMORTS            10    /* Max number of remorts. */
#define STATS_PER_REMORT       4     /* Stats per remort... */
#define CONDITIONS             11    /* The number of condition ranks things
					have. */
#define TFIND_NAME_MAX         3     /* Max number of tfind names. */
#define NUM_NEWBIE_ITEMS       20    /* Max number of newbie items. */
#define NUM_VALS               7      /* Size of value data when it is
					 created. Leave >= 6. Or else. */

#define NUM_COINS              6      /* Number of different coins.
					 THIS MUST BE NO MORE THAN
					 NUM_VALS SINCE WE USE VALUES TO
					 KEEP TRACK OF MONEY!!!! */

#define MAX_STAT_VAL           40      /* Maximum value a stat can have */
#define MIN_STAT_VAL           3       /* Minimum value a stat can have */

#define WORLD_VNUM             220985383 /* Big random number for world. */
#define PLAYER_VNUM            0       /* All players are set to
					  vnum "0"...which means
					  they get a pc */
#define MAX_NEST               30      /* How deep nested things can be. */
#define MAX_SPELL              1000    /* Max Number of spells.. */

#define MAX_PRAC               70      /* Max pct you can prac up to. */
#define MAX_TRAIN_PCT          100     /* Max amt a spell can be trained to
					  even with use. */
#define MIN_PRAC               30      /* Must know it this well
					  to have skill gains. */
#define MAX_TROPHY             30      /* Number of trophies you have */
#define NUMERIC_STATS_SEEN     20      /* Level you see numeric stats. */
#define BIGBUF                 150000  /* Size of big buffer. */
#define CASTE_MAX              10      /* Max number of castes */
#define HUNT_STRING_LEN        30      /* Max len of "hunting" strings. */
#define MAX_GROUP_POINTS       16      /* Max power of a group */
#define MAX_SCRIPT_DEPTH       5       /* Max depth of scripts. */
#define MAX_YELL_DEPTH         20      /* How far yells can go. */
#define LBUF_SIZE              400     /* Size of listbuf */
#define BRIGACK                10000   /* Critical internal variable. */
#define GATHER_EQUIP           4       /* Max num eq needed to gather. */
#define GATHER_RAW_AMT         50      /* Amt you can gather w/out stop. */
#define MAX_CARRY_WEIGHT_AMT   7       /* Number of carry weight messages. */
#define IMPLANT_ROOM_VNUM      27      /* Dunno, room 27 for implants? */
#define IMPLANT_POINTS_PER_REMORT 6    /* Implant pts per remort */
#define IMPLANT_WPS_COST       50      /* 50 wps multiplier. */
#define IMPLANT_MONEY_COST     500     /* 500 coins multiplier. */
#define IMPLANT_TIER_MAX       7       /* 7 tiers of implants? */
#define RACE_MAX               40      /* Max # of races */
#define ALIGN_MAX              10      /* Max # of aligns */
#define ALIGN_POWER_BONUS      10      /* Max bonus you can get if your 
					  align is losing. */
#define MAX_TEACHER            10      /* Up to 10 teacher locs per skill */
#define AREA_MAX               1000    /* Can have up to 1000 areas. */
#define RACE_ASCENDED_STATS    195     /* If the race max stats are more
					  than this, it's an ascend race. */
#define MAX_DISEASE_SYMPTOMS   20      /* Number of disease symptoms. */


#define SHOP_COST_BUY          1       /* Cost when buying an item. */
#define SHOP_COST_SELL         2       /* Cost when selling an item. */
/* Ok, scrollback messages for each pc are stored in the pc structs.
   This is wasteful since if you have say 15 channels, and 20 scrollbacks
   for each channel, and average length of message = 120, and 100 chars
   online, that is 3.6 megs of scrollback data. However, there wasn't
   really any good way to do this for all of the "local" channels 
   like say/emote/gtell/ctalk etc...so I bit the bullet and just 
   decided to do it this way. Easier than trying to have several
   pointers to the same friggin messages that have to be checked
   and destroyed. Oh well, hope it doesn't suck too bad.  This
   idea of allowing scrollbacks for ALL channels doesn't scale well, so
   use with caution if you try to make a huge game. I would suggest
   in that case that you have global scrollbacks only. */

#define MAX_CHANNEL            15      /* Max number of channels? This
					  code is gonna be ugly. */
#define MAX_SCROLLBACK         20      /* Max number of scrollback msgs. */
#define MAX_SCROLLBACK_LENGTH  159     /* Max length of any scrollback
					  string */
#define MAX_MINERAL            10      /* Number of mineral ranks --
					if you change this, change the
				       mineral_names and mineral_colornames
				       in const.c */
#define MAX_FORGE              30      /* Max number of different things you
					  can forge. */
#define MAX_AREAS              2000    /* Max number of areas. */
#define FORGE_VNUM             897     /* This object is the forge */
/* These cosntants are for brewing/scribing */

#define POTION_BOTTLE       706  /* Item used to make potions */
#define SCROLL_PAPER        707  /* Item used to make scrolls */
#define HERB_START          850  /* Herb start number */
#define HERB_COUNT            7  /* Number of herbs */
#define HERB_MAX_NEEDED       6  /* Max herbs per spell in potion. */
#define POTION_SEED     2354634  /* Potion rng seed */
#define SCROLL_SEED     725233   /* scroll rng seed */
#define POTION_COLORS        16  /* Number of potion colors. */
#define POTION_TYPES         14  /* Number of potion modifiers */
#define MAX_LOGIN           100  /* Number of logins recorded. */

#define TEACHER_VNUM         230 /* Vnum of the randomized trainers who
				    get set up all over the world. */
#define PACKAGE_VNUM        240  /* Vnum of packages players can deliver. */
/* Default prompt :P */
#define PACKAGE_DELIVERY_HOURS 30 /* How long you have to deliver a package. */


/* Dimensions used internally for generating things. */
#define DIM_X    0
#define DIM_Y    1
#define DIM_Z    2
#define DIM_MAX  3 /* Number of dimensions. */

#define DEFAULT_PROMPT    "@ed@@bl@@dl@$E@hc@/@hm@$7hp $A@vc@/@vm@$7mv $D@mc@/@mm@$7m @tk@@oc@> "

/* This is the name of the game used for logins and who list and stuff. */
#define game_name_string "\x1b[1;31mTESTING Battle for Destiny TESTING\x1b[0;37m"

/**********************************************************************/
/* These flags are used in the move_thing function in move.c 
   and to determine how things interact with other things. */

/** If you alter these, alter thing1_flags in const.c **/
/**********************************************************************/
#define TH_NO_TAKE_BY_OTHER 0x00000001   /* initiator cannot move mover 
					    from something to itself. */
#define TH_NO_MOVE_SELF     0x00000002   /* No move self in or out. */
#define TH_NO_CONTAIN       0x00000004   /* NOTHING EVER IN THIS */
#define TH_NO_DRAG          0x00000008   /* Drag is special for things
					  that can't be picked up*/
#define TH_NO_GIVEN         0x00000010   /* No take stuff from others */

#define TH_NO_DROP          0x00000020   /* Other no remove this from self */
#define TH_NO_CAN_GET       0x00000040   /* Cannot get anything itself  */
#define TH_NO_MOVE_BY_OTHER 0x00000080   /* Others cannot move this */
#define TH_NO_PRY           0x00000100   /* Cannot be pried off or removed */
#define TH_NO_MOVE_OTHER    0x00000200   /* This cannot move others */
#define TH_NO_TAKE_FROM     0x00000400   /* Keeps all inven...cannot be looted */
#define TH_NO_MOVE_INTO     0x00000800   /* Other cant move self into this */
#define TH_NO_LEAVE         0x00001000   /* Other cant move self out of this */
#define TH_NO_REMOVE        0x00002000   /* This cannot be removed */
/**********************************************************************/
/* These are other flags needed by things. */

/**********************************************************************/

#define TH_UNSEEN           0x00004000   /* Cannot be seen unless you
					    are a real imm. Useful for 
					    "boats" which are a lot
					    of rooms "in" the deck room...
					    and they link together,
					    but you don't want to have
grep 					    them all visible to each 
					    other in the room. */
#define TH_NO_FIGHT         0x00008000   /* This thing does not fight 
					     back when attacked. */
#define TH_NO_TALK          0x00010000    /* This cannot communicate */

#define TH_MARKED           0x00020000   /* Mark this thing temporarily */

#define TH_CHANGED          0x00040000   /* This thing has been changed..
					    good for areas. */
#define TH_IS_ROOM          0x00080000   /* This is marked as 
					    a room... */

#define TH_IS_AREA          0x00100000   /* This thing is special..it
					    has area data */
#define TH_NUKE             0x00200000   /* This thing and 
					    the prototype it came from get
					    nuked on reboot. */
#define TH_NO_PEEK          0x00400000   /* Things can't look into this 
					    without the peek skill */
#define TH_SCRIPT           0x01000000   /* This thing has a script */ 
#define TH_DROP_DESTROY     0x02000000   /* This is destroyed if dropped. */
#define TH_NO_GIVE          0x04000000   /* You cannot give this thing. */
#define TH_SAVED            0x08000000   /* This thing was saved. */

/* When a room is created, it gets these flags automatically. */  
#define ROOM_SETUP_FLAGS  TH_IS_ROOM | TH_NO_TAKE_BY_OTHER | \
		  TH_NO_MOVE_SELF | TH_NO_DRAG | TH_NO_DROP | TH_NO_CAN_GET | \
		  TH_NO_MOVE_BY_OTHER | TH_NO_PRY


/* These are divisors for banking fees. */

#define STORAGE_DIVISOR            20
#define WITHDRAWAL_DIVISOR         50

/**********************************************************************/
/* These are various kinds of pk numbers kept track of. */
/**********************************************************************/


#define PK_WPS                      0 /* Warpoints */
#define PK_TOT_WPS                  1 /* Total warpoints */
#define PK_KILLS                    2 /* total number of kills */
#define PK_LEVELS                   3 /* Levels who have helped pking */
#define PK_HELPERS                  4 /* Number of pk helpers */
#define PK_PKILLS                   5 /* Number of pkills */
#define PK_KILLPTS                  6 /* Kill points */
#define PK_SOLO                     7 /* Solo pkills vs harder enemies. */
#define PK_RATING                   8 /* Rating of player... */

#define PK_GOOD_MAX                 9 /* Must be one more than the biggest
					 "good" pk stat. */

/* THIS IS IMPORTANT. THE PK_ VALUES ABOVE THIS LINE ENDING WITH PK RATING ARE
   USED TO CALCULATE WHICH ALIGN IS WINNING THE PK WAR SO THE OTHER ONES
   CAN BE GIVEN A BONUS. IF YOU ADD OTHER GOOD PK STATS YOU MUST PUT THEM
   ABOVE THIS LINE AND ALTER PK_GOOD_MAX TO MAKE THE CODE WORK CORRECTLY! 
   ALSO, DON'T ADD ANY BAD PK DATA TYPES ABOVE THIS LINE. */

#define PK_BGPKW                    9 /* bgpkw average */
#define PK_KILLED                  10 /* Total number of times killed */
#define PK_PKILLED                 11 /* number of times pkilled */
#define PK_BRIGACK                 12 /* Highest brigack score. */
#define PK_MAX                     13

#define PK_LISTSIZE                15 /* Length of each pk list */

/* These are various rewards players can get for helping societies. I
   had to put this here so that it could go into the pc data structure. */

#define REWARD_PLAY        0  /* Play with children. */
#define REWARD_BUILD       1  /* Build up a city. */
#define REWARD_RAZE        2  /* Raze an enemy city. */
#define REWARD_HEAL        3  /* Heal somebody. */
#define REWARD_KILL        4  /* Kill an enemy. */
#define REWARD_ITEM        5  /* Give needed item */
#define REWARD_KIDNAP      6  /* Rescue kidnap victim. */
#define REWARD_DIPLOMACY   7  /* Reward for using diplomacy. */
#define REWARD_INSPIRE     8  /* Raised morale of a society. */
#define REWARD_DEMORALIZE  9  /* Lower morale of a society. */
#define REWARD_PACKAGE     10 /* Delivering a package successfully. */
#define REWARD_MAX         11 /* Max number of rewards. */

/**********************************************************************/
/* These are flags set when we determine if we will send stuuff to a   */
/* character using the graphical client. Alter the routines in kde.c  */
/* if you change these. */
/**********************************************************************/

#define KDE_UPDATE_HMV       0x00000001  /* Update hp/mv/mana */
#define KDE_UPDATE_MAP       0x00000002  /* Update the map */
#define KDE_UPDATE_NAME      0x00000004  /* Update name/level/race etc... */
#define KDE_UPDATE_STAT      0x00000008  /* Update stat information */
#define KDE_UPDATE_GUILD     0x00000010  /* Update guild status */
#define KDE_UPDATE_IMPLANT   0x00000020  /* Update implant info */
#define KDE_UPDATE_EXP       0x00000040  /* Update the player's exp */
#define KDE_UPDATE_PK        0x00000080  /* Update pk stats */
#define KDE_UPDATE_WEIGHT    0x00000100  /* Update weight and num carried */
#define KDE_UPDATE_COMBAT    0x00000200  /* Update combat info +hit/dam/eva */

/**********************************************************************/
/* These are the different kinds of pixmaps we have for use with the */
/* mapping for the kde client. */
/**********************************************************************/

/* These are also used for overhead mapping in the wilderness grid.
   If you alter these, change the pixmap_symbols struct in const.c */

#define PIXMAP_NONE         0  /* Don't use this. */
#define PIXMAP_DEFAULT      1
#define PIXMAP_WATER        2
#define PIXMAP_SWAMP        3
#define PIXMAP_FIELD        4
#define PIXMAP_FOREST       5
#define PIXMAP_MOUNTAIN     6
#define PIXMAP_SNOW         7
#define PIXMAP_DESERT       8
#define PIXMAP_ROAD         9
#define PIXMAP_AIR          10
#define PIXMAP_FIRE         11
#define PIXMAP_INSIDE       12
#define PIXMAP_UNDERGROUND  13
#define PIXMAP_MAX          14


/***********************************************************************/
/* These are global server_ flags to cut down on the number of externs. */
/* These can be altered if you want, since they are never stored, only */
/* used dynamically and internally. Be careful if you remove anything. */
/***********************************************************************/

#define SERVER_REBOOTING           0x00000001 /* Means the server is rebooting*/
#define SERVER_BOOTUP              0x00000002 /* Means the server is booting up */
#define SERVER_CHANGED_SPELLS      0x00000004  /* We changed spells and must save  them. */
#define SERVER_READ_PFILE          0x00000008   /* We are reading in a pfile */
#define SERVER_CONSIDERING         0x00000010   /* Considering in combat */
#define SERVER_DOING_NOW           0x00000020   /* We are doing something now. */
#define SERVER_CHANGED_SOCIETIES   0x00000040  /* societies changed */
#define SERVER_DETECT_OPP_ALIGN    0x00000080  /* Detect opp align spell */
#define SERVER_SUPPRESS_EDIT       0x00000100    /* Does not send edit..for zedit */
#define SERVER_RAVAGE              0x00000200  /* ravage..super flurry */
#define SERVER_BEING_ATTACKED_YELL 0x00000400  /* Yeller being attacked */
#define SERVER_LOG_ALL             0x00000800  /* All commands logged. */
#define SERVER_READ_INPUT           0x00001000  /* Got input read in. */
#define SERVER_SACRIFICE           0x00002000  /* Sacrifice spell in use */
#define SERVER_TSTAT               0x00004000  /* Tstatting */
#define SERVER_SAVING_WORLD        0x00008000  /* Saving world */
#define SERVER_WRITE_PFILE         0x00010000  /* We are writing a pfile */
#define SERVER_SPEEDUP             0x00020000  /* Server goes as fast as
						  possible and does not
						  pulse only every 1/10
						  of a second. */
#define SERVER_WIZLOCK             0x00040000  /* No players can log in. */
#define SERVER_READ_DATA           0x00080000  /* Something sent data to the
						  socket. */
#define SERVER_SAVING_AREAS        0x00100000  /* Server is saving the areas
						  of the world atm. */
#define SERVER_WORLDGEN            0x00200000  /* Is the server in
						  the middle or worldgenning?
						  This is used because 
						  certain things need to be
						  done after all areas are
						  made and linked. */
#define SERVER_AUTO_WORLDGEN      0x00400000   /* Does the server auto generate
						  worlds once a day or so? */

/**********************************************************************/
/* These are flags for connections. You can add more if you want, but
   be careful before you remove them. */
/**********************************************************************/

#define   FD_TYPED_COMMAND         0x00000001
#define   FD_BAD_PASSWORD          0x00000002
#define   FD_BAD_PASSWORD2         0x00000004
#define   FD_EDIT_PC               0x00000010
#define   FD_NO_ANSI               0x00000020  /* No colors sent */
#define   FD_USING_KDE             0x00000040  /* Using the kde client. */
#define   FD_HANGUP                0x00000080 /* Hanging up on the client. */
#define   FD_HANGUP2               0x00000100 /* Final hangup flag. */
/**********************************************************************/
/* These are connected states. A normal player is in state 0, and
   a state less than 0 means some sort of editing is going on (so that
   the editing commands get sent to that function directly) and a state
   larger than 0 means that the person is connecting. */
/**********************************************************************/

#define CON_SOCIEDITING       -5
#define CON_TRIGEDITING       -4
#define CON_RACEDITING        -3
#define CON_SEDITING          -2
#define CON_EDITING           -1
#define CON_ONLINE             0
#define CON_CONNECT            1
#define CON_GET_NAME           2
#define CON_GET_OLD_PWD        3
#define CON_CONFIRM_NAME       4
#define CON_GET_NEW_PWD        5
#define CON_CONFIRM_NEW_PWD    6
#define CON_GET_EMAIL          7
#define CON_GET_SEX            8
#define CON_GET_PAGELEN        9
#define CON_GET_RACE           10
#define CON_CONFIRM_RACE       11
#define CON_GET_ALIGN          12
#define CON_CONFIRM_ALIGN      13
#define CON_ROLLING            14
#define CON_PERMADEATH         15
#define CON_GO_INTO_SERVER     50


/**********************************************************************/
/* These are directions. There are 8 of them now. The first 6 are 
   standard, and DIR_OUT is a "lid" for a container, and DIR_IN represents
   a portal. You can add extra directions, but you must alter 
   REALDIR_MAX and the various dir_ structs in const.c to make the
   names work out. Things will probably work if you up things here,
   like adding nw sw ne se directions, for example, but I always hated
   those directions because technically you need stuff like ue de dw 
   nd and so forth directions to have things "make sense" and it seemed
   like too much. You will also have to alter the values so that the new
   exit values go in the proper locations there. The one exception to
   having everything work is in map.c. Maps only map the flat part of
   the world (news) so you will have to do work to get that working with
   other new directions. Another exception is in areagen.c where news 
   being first exits is an assumption. */
/**********************************************************************/



#define DIR_NORTH              0
#define DIR_SOUTH              1
#define DIR_EAST               2
#define DIR_WEST               3
#define DIR_UP                 4
#define DIR_DOWN               5
#define DIR_OUT                6
#define DIR_IN                 7
#define DIR_MAX                8

#define REALDIR_MAX            6 /* The number of "real" directions" */
#define FLATDIR_MAX            4 /* NSEW dirs only. */
/* This is used to show that blood was just set so it doesn't go
   away too quickly. */

#define MOVE_FLEE  0x00000001   /* Movement while fleeing from a fight. */
#define MOVE_RUN   0x00000002   /* Movement while running -- no descs shown. */

#define BLOOD_POOL            (1 << DIR_IN)
#define NEW_BLOOD             (1 << DIR_MAX)

/**********************************************************************/
/* These are clan types...clan sect temple profession */

/**********************************************************************/

#define CLAN_CLAN              0
#define CLAN_SECT              1
#define CLAN_TEMPLE            2
#define CLAN_ACADEMY           3
#define CLAN_MAX              4

/**********************************************************************/
/* These are some of the clan flags. */

/**********************************************************************/

#define CLAN_NOSAVE          0x00000001
#define CLAN_ALL_ALIGN       0x00000004


#define MAX_CLAN_MEMBERS       100


/* Used for formatting strings like prompts etc... */

#define PLACE_LEFT             0
#define PLACE_CENTER           1
#define PLACE_RIGHT            2



/**********************************************************************/
/* These are the various kinds of values you can set on things...
   they allow things to have certain special properties like gems
   wpns, exits...all this kind of stuff that is too big to just
   have a flag for it. */

/* Note: We do not have special items for containers, treasure, trash,
   furniture, keys, vehicles (= tools I guess), scalable/climbable,
(traps = tools? ), fountains... etc. But they are still makeable. I think. */ 

/* If you alter these things remember to alter value_list in const.c
   If you don't, the names here won't match up with the names there. 
   It's set up to be a little faster by making the array index there
   equal to the value here. */

/**********************************************************************/




#define VAL_NONE             0
#define VAL_EXIT_N           1
#define VAL_EXIT_S           2
#define VAL_EXIT_E           3
#define VAL_EXIT_W           4
#define VAL_EXIT_U           5
#define VAL_EXIT_D           6
#define VAL_EXIT_O           7
#define VAL_EXIT_I           8
#define VAL_OUTEREXIT_N      9
#define VAL_OUTEREXIT_S      10
#define VAL_OUTEREXIT_E      11
#define VAL_OUTEREXIT_W      12
#define VAL_OUTEREXIT_U      13
#define VAL_OUTEREXIT_D      14
#define VAL_OUTEREXIT_O      15
#define VAL_OUTEREXIT_I      16
#define VAL_WEAPON           17 /* Sets a thing to a weapon */
#define VAL_GEM              18 /* Sets a thing to a gem */
#define VAL_ARMOR            19 /* Armors... */
#define VAL_FOOD             20 /* A type of food...eaten... incl spells */
#define VAL_DRINK            21 /* Types of drink things.. incl fountain/pot*/
#define VAL_MAGIC            22 /* Scroll, wand, staff etc.  */
#define VAL_TOOL             23 /* This is some tool incl trap and vehicle */
#define VAL_RAW              24 /* A raw material for making stuff... */
#define VAL_AMMO             25 /* Ammunition for a ranged wpn */
#define VAL_RANGED           26 /* A Ranged weapon. */
#define VAL_SHOP             27 /* Basic shop info... */
#define VAL_TEACHER0         28 /* This thing teaches to people... */
#define VAL_MONEY            29 /* Money (coins) in this thing */
#define VAL_GUARD            30 /* This thing guards stuff. */
#define VAL_CAST             31 /* Spells this thing casts... */
#define VAL_POWERSHIELD      32 /* Powershield absorbs damage, regens fast. */
#define VAL_MAP              33 /* Gives a map picture when you look at it. */
#define VAL_GUILD            34 /* Guildmaster...v0 = type, v1/2 = min/max  */
#define VAL_FEED             35 /* Feeds on other things */
#define VAL_TEACHER1         36 /* More things taught */
#define VAL_TEACHER2         37 /* More things taught */
#define VAL_SHOP2            38 /* More shop stuff. PC owners etc... */
#define VAL_LIGHT            39 /*Used as a light source. */
#define VAL_RANDPOP          40 /* Used to pop random items */
#define VAL_PET              41 /* Used for pets and owners. */
#define VAL_SOCIETY          42 /* Part of a society */
#define VAL_BUILD            43 /* A city being built for a society. */
#define VAL_POPUL            44 /* Populations are like wilderness societies.*/
#define VAL_MADEOF           45 /* What this is made of process/ingreds */
#define VAL_REPLACEBY        46 /* On timeout, replaced by (randpop) this. */
#define VAL_QFLAG            47 /* Quest flags. Internal only. */
#define VAL_PACKAGE          48 /* Package used for deliveries by 
				   characters. */
#define VAL_DIMENSION        49 /* Dimensions of an object. */
#define VAL_CRAFT            50 /* Crafted items data. */
#define VAL_DAILY_CYCLE      51 /* This thing goes through a daily cycle. */
#define VAL_COORD            52 /* What coordinate we're in for plasmgen. */
#define VAL_MAX              53 /* Total number of values. */
  
  

/* IF YOU ALTER THESE VAL_FOO THINGS YOU MUST ALTER value_list[] IN CONST.C */

/**********************************************************************/
/* These are the kinds of flags which are allowed onto things... */
/**********************************************************************/

#define    FLAG_NONE               0 /* No flags here... */
#define    FLAG_THING              1 /* These are thing flags... */
#define    FLAG_AREA               2 /* These are flags used for areas */
#define    FLAG_ROOM1              3 /* These are flags used on rooms */
#define    FLAG_OBJ1               4 /* These are flags used on objects */
#define    FLAG_HURT               5 /* Affected by hurtful stuff */
#define    FLAG_PROT               6 /* Protected from stuff */
#define    FLAG_AFF                7 /* Affected by something... */
#define    FLAG_DET                8 /* Detecting something... */
#define    FLAG_ACT1               9 /* Actions things can do */
#define    FLAG_ACT2              10 /* More actions */
#define    FLAG_MOB               11 /* Mob flags for combat */
#define    FLAG_PC1               12 /* Pc only flags */
#define    FLAG_PC2               13  /* Pc only flags */
#define    FLAG_SPELL             14 /* Spell flags */
#define    FLAG_CLAN              15 /* Clan flags */
#define    FLAG_MANA              16 /* Mana flags */
#define    FLAG_SOCIETY           17 /* Society flags */
#define    FLAG_CASTE             18 /* Caste flags */
#define    FLAG_TRIGGER           19 /* Trigger flags */
#define NUM_FLAGTYPES             30 /* This is the maximum number of
					flagtypes possible... can be
					changed */
  
/**********************************************************************/
/* Start the affects right at num_flagtypes? You might want to change this
   so you have more room to work with....i.e. make num_flagtypes bigger 
   to start with altho 30*32 is a huge number of flags. */
/**********************************************************************/
/* WITH THESE AFFECTS MAKE SURE YOU HAVE ALL NUMBERS FROM AFF_START UP
   TO FLAG_MAX FILLED IN!!!! OR ELSE BAD THINGS WILL HAPPEN AND MAKE
   SURE THAT THIS LIST MATCHES UP WITH affectlist[] IN CONST.C IF
   YOU MAKE CHANGES!!!! */
/**********************************************************************/

#define AFF_START              30
#define FLAG_AFF_STR           30
#define FLAG_AFF_INT           31
#define FLAG_AFF_WIS           32
#define FLAG_AFF_CON           33
#define FLAG_AFF_DEX           34
#define FLAG_AFF_LUC           35
#define FLAG_AFF_CHA           36
#define FLAG_AFF_DAM           37  
#define FLAG_AFF_HIT           38
#define FLAG_AFF_MV            39
#define FLAG_AFF_HP            40
#define FLAG_AFF_SPELL_ATTACK  41
#define FLAG_AFF_SPELL_HEAL    42
#define FLAG_AFF_SPELL_RESIST  43
#define FLAG_AFF_THIEF_ATTACK  44
#define FLAG_AFF_DEFENSE       45 /* Helps with dodge/parry/shieldblock */
#define FLAG_AFF_KICKDAM       46
#define FLAG_AFF_ARMOR         47
#define FLAG_AFF_DAM_RESIST    48 /* Removes pcts of physical damage */
#define FLAG_AFF_GF_ATTACK     49 /* +groundfight attack. */
#define FLAG_AFF_GF_DEFENSE    50 /* +groundfight defense. */
  /* Start of flags that let you change the elemental power for spells */
#define FLAG_ELEM_POWER_START  51 
  
  /* We should never actually access any of these variables. */

#define FLAG_ELEM_POWER_AIR    51
#define FLAG_ELEM_POWER_EARTH  52
#define FLAG_ELEM_POWER_FIRE   53
#define FLAG_ELEM_POWER_WATER  54
#define FLAG_ELEM_POWER_SPIRIT 55

  /* Start of flags that raise or lower your level in certain mana colors. */
#define FLAG_ELEM_LEVEL_START  56

 /* We should never actually access any of these variables. */
#define FLAG_ELEM_LEVEL_AIR    56
#define FLAG_ELEM_LEVEL_EARTH  57
#define FLAG_ELEM_LEVEL_FIRE   58
#define FLAG_ELEM_LEVEL_WATER  59
#define FLAG_ELEM_LEVEL_SPIRIT 60

#define AFF_MAX                61


  /* These are the various ranks of affs that are used to determine
     how good items are when they get set with these affects. */

#define AFF_RANK_POOR       0 
#define AFF_RANK_FAIR       1
#define AFF_RANK_GOOD       2
#define AFF_RANK_EXCELLENT  3
#define AFF_RANK_MAX        4



/**********************************************************************/
/* These are exit flags. They are just used internally for doors. */
/**********************************************************************/

#define EX_DOOR           0x00000001
#define EX_CLOSED         0x00000002
#define EX_HIDDEN         0x00000004
#define EX_PICKPROOF      0x00000008

#define EX_DRAINING       0x00000020
#define EX_LOCKED         0x00000040
#define EX_WALL           0x00000080
#define EX_NOBREAK        0x00000100

/**********************************************************************/
/* These are guarding flags. They are used in the guard value. They
   determine what kinds of things this thing guards against. */
/**********************************************************************/

#define GUARD_DOOR        0x00000001   /* This thing guards vs movement */
#define GUARD_DOOR_ALIGN  0x00000002   /* This thing guards vs an align */
#define GUARD_DOOR_PC     0x00000004   /* This guards vs pc's only */
#define GUARD_DOOR_NPC    0x00000008   /* This guards vs npc's only */
#define GUARD_DOOR_SOC    0x00000010   /* Guards vs all of diff socieities */
#define GUARD_VS_ABOVE    0x00000020   /* Used with guarding vs levels
					  and remorts etc..if this is set
					  you stop all things with values
					  ABOVE this number, and without
					  it, you stop all below. */

/**********************************************************************/
/* These are the sexes available. */
/**********************************************************************/

#define SEX_NEUTER         0
#define SEX_FEMALE         1
#define SEX_MALE           2
#define SEX_MAX            3

  
/* These are the various levels of alignment hate that a player can
   have. It determines how likely you are to be an "enemy" of something
   of that alignment. */

#define ALIGN_HATE_WARN       100
#define ALIGN_HATE_ATTACK     300


/**********************************************************************/
/* These are just strings which tell where certain directories are. */
/**********************************************************************/

#define WLD_DIR    "../wld/"
#define PLR_DIR      "../plr/"
#define LOG_DIR      "../log/"

/**********************************************************************/
/* These are bits that determine what a channel can and cannot do. 
   These are used in channels.dat. */
/**********************************************************************/

#define CHAN_TO_TARGET    0x00000001 /* You must specify a target 1 */
#define CHAN_TO_ROOM      0x00000002 /* The message is sent to 
					the thing you are in. 2 */
#define CHAN_TO_NEAR      0x00000004 /* The channel is sent to those
					"nearby" 4 */
#define CHAN_TO_ALL       0x00000008 /* The message is sent to everyone.
					note that this ONLY goes to
					those who are players. 8 */
#define CHAN_OPP_ALIGN    0x00000010 /* This channel gets sent to opp 
					align people. 16 */
#define CHAN_TO_CLAN      0x00000020 /* This channel gets sent to
					your "clan" only... 32 */
#define CHAN_TO_GROUP     0x00000040 /* This channel only gets sent
					to your group. 64*/
#define CHAN_NO_QUOTE     0x00000080 /* Do we use quotes or not? 128 */ 
#define CHAN_USE_COLOR    0x00000100 /* Can you use color in this channel? 256*/
#define CHAN_NAME_MAX     4          /* Max number of names for a channel */


/**********************************************************************/
/* This is the string that gets sent a lot in helpfiles. Makes it easier
   to have it just sitting in one spot. */
/**********************************************************************/

#define HELP_BORDER   "\x1b[0;31m[\x1b[1;30m+\x1b[0;31m]\x1b[1;30m-=-=-=-=-=-=\x1b[0;37m-=-=-=-=-=-=\x1b[1;37m-=-=-=-=-=-=-=-=-=-=-=-\x1b[0;37m=-=-=-=-=-=-\x1b[1;30m=-=-=-=-=-=-\x1b[0;31m[\x1b[1;30m+\x1b[0;31m]\x1b[0;37m\n\r"

/**********************************************************************/
/* These are tool flags, set them on v0 of a tool value.  */
/**********************************************************************/

/* These are used a lot wrt society needs. */

#define TOOL_NONE             0
#define TOOL_LOCKPICKS        1
#define TOOL_ANVIL            2
#define TOOL_HAMMER           3
#define TOOL_NEEDLE           4
#define TOOL_SAW              5 
#define TOOL_KNIFE            6
#define TOOL_PLANE            7
#define TOOL_BOWL             8
#define TOOL_PLATE            9
#define TOOL_JAR             10
#define TOOL_PIPE            11
#define TOOL_MAX             12
  
  /* Tools 0 is at 349, the toolpop. */
#define TOOL_START_VNUM    349    

/* Also, note that tools->val[1] is the rank of the tool. */

/**********************************************************************/
/* These are area flags...not too many yet  Alter area1_flags in const.c
   if you alter these. */
/**********************************************************************/

#define AREA_CLOSED      0x00000001
#define AREA_NOQUIT      0x00000002
#define AREA_NOSETTLE    0x00000004 /* Societies will not try to settle here.*/
#define AREA_NOREPOP     0x00000008 /* Randpop mobs will not spawn here. */
#define AREA_NOLIST      0x00000010 /* Not listed under "Areas" function. */
/**********************************************************************/
/* These are room flags. Alter room1_flags in const.c if you alter these. */
/**********************************************************************/


#define ROOM_INSIDE      0x00000001
#define ROOM_WATERY      0x00000002
#define ROOM_AIRY        0x00000004
#define ROOM_FIERY       0x00000008
#define ROOM_EARTHY      0x00000010
#define ROOM_NOISY       0x00000020
#define ROOM_PEACEFUL    0x00000040
#define ROOM_DARK        0x00000080
#define ROOM_EXTRAMANA   0x00000100
#define ROOM_EXTRAHEAL   0x00000200
#define ROOM_MANADRAIN   0x00000400
#define ROOM_NOMAGIC     0x00000800
#define ROOM_UNDERGROUND 0x00001000
#define ROOM_NOTRACKS    0x00002000
#define ROOM_NOBLOOD     0x00004000
#define ROOM_EASYMOVE    0x00008000
#define ROOM_ROUGH       0x00010000
#define ROOM_FOREST      0x00020000
#define ROOM_FIELD       0x00040000
#define ROOM_DESERT      0x00080000
#define ROOM_SWAMP       0x00100000
#define ROOM_NORECALL    0x00200000
#define ROOM_NOSUMMON    0x00400000
#define ROOM_UNDERWATER  0x00800000
#define ROOM_MINERALS    0x01000000  /* Can be mined/chopped etc.. */
#define ROOM_LIGHT       0x02000000  /* Inside is lit */
#define ROOM_MOUNTAIN    0x04000000  /* Need mountain boots to climb up. */
#define ROOM_SNOW        0x08000000  /* Snow..uses lots of moves. */
#define ROOM_ASTRAL      0x10000000  /* Otherworldly room. */
#define ROOM_NOMAP       0x20000000  /* Room can't be mapped. */
/* Bits that need special stuff to traverse or survive in. */

#define BADROOM_BITS    (ROOM_WATERY | ROOM_EARTHY | ROOM_UNDERWATER | ROOM_AIRY | ROOM_MOUNTAIN | ROOM_ASTRAL)

  /* These are flags that mean a room has a certain sector. */
  
#define ROOM_SECTOR_FLAGS  (ROOM_INSIDE | ROOM_WATERY | ROOM_AIRY | ROOM_FIERY | ROOM_EARTHY | ROOM_UNDERGROUND | ROOM_EASYMOVE | ROOM_UNDERWATER | ROOM_ROUGH | ROOM_FOREST | ROOM_FIELD | ROOM_DESERT | ROOM_SWAMP | ROOM_MOUNTAIN | ROOM_SNOW | ROOM_ASTRAL)


/**********************************************************************/
/* These are affect flags. Alter aff1_flags in const.c if you alter these. */
/**********************************************************************/

#define AFF_BLIND        0x00000001
#define AFF_CURSE        0x00000002
#define AFF_OUTLINE      0x00000004
#define AFF_SLEEP        0x00000008
#define AFF_FEAR         0x00000010
#define AFF_DISEASE      0x00000020
#define AFF_WOUND        0x00000040
#define AFF_RAW          0x00000080
#define AFF_SLOW         0x00000100
#define AFF_NUMB         0x00000200
#define AFF_CONFUSE      0x00000400
#define AFF_FORGET       0x00000800
#define AFF_BEFUDDLE     0x00001000
#define AFF_WEAK         0x00002000
#define AFF_PARALYZE     0x00004000
#define AFF_AIR          0x10000000 
#define AFF_EARTH        0x20000000 
#define AFF_FIRE         0x40000000
#define AFF_WATER        0x80000000


/**********************************************************************/
/* More affect flags. Alter aff2_flags in const.c if you alter these. */
/**********************************************************************/

#define AFF_INVIS        0x00000001
#define AFF_HIDDEN       0x00000002
#define AFF_DARK         0x00000004
#define AFF_CHAMELEON    0x00000008
#define AFF_SNEAK        0x00000010 /* First thing not used in can_see */
#define AFF_FOGGY        0x00000020 /* passdoor/det fog... */
#define AFF_PROT_ALIGN   0x00008000 /* Reduces dam from opp align. */
#define AFF_SANCT        0x00010000
#define AFF_REFLECT      0x00020000
#define AFF_PROTECT      0x00040000
#define AFF_FLYING       0x00080000
#define AFF_WATER_BREATH 0x00100000
#define AFF_REGENERATE   0x00200000
#define AFF_HASTE        0x00400000
/* THESE FLAGS ARE USED IN BOTH PLACES...IN THE HURT/PROT PART, THEY ARE
   USED TO SHOW PEOPLE AFFECTED BADLY BY AIR/FIRE ETC...HERE THEY ARE USED
   TO SHOW PEOPLE PROTECTED BY SHIELDS THAT REDUCE DAMAGE AND CAUSE DAMAGE
   TO ATTACKERS!
#define AFF_AIR          0x10000000 
#define AFF_EARTH        0x20000000 
#define AFF_FIRE         0x40000000
#define AFF_WATER        0x80000000
*/

/**********************************************************************/
/* These are act flags. Alter act1_flags in const.c if you alter these. */
/**********************************************************************/
  
#define ACT_AGGRESSIVE      0x00000001 /* Attacks everything in the room */
#define ACT_FASTHUNT        0x00000002 /* Hunts really fast... */
#define ACT_BANKER          0x00000008 /* You can bank in the room with this */
#define ACT_KILL_OPP        0x00000010 /* Kills opp align things */
#define ACT_ASSISTALL       0x00000080 /* Assists all other mobs */
#define ACT_SENTINEL        0x00000100 /* Does not move */
#define ACT_ANGRY           0x00000200 /* Lesser version of aggressive */
#define ACT_MOUNTABLE       0x00000400 /* Will let things ride it. */
#define ACT_WIMPY           0x00000800 /* Will flee at 1/6 hps? */
#define ACT_CARNIVORE       0x00001000 /* Will hunt for food. */
#define ACT_PRISONER        0x00002000 /* Prisoner will not fight. */
#define ACT_DUMMY           0x00004000 /* For dummy mobs...don't hit back. */
#define ACT_DEITY           0x00008000 /* Hunt/kill fast. */
/**********************************************************************/
/* Mob details flags...mostly used with combat and spell effectiveness.
   Alter mob1_flags in const.c if you alter these. */
/**********************************************************************/

/* These will be partially overridden by knights? A quick note...
   the table below and the OBJ_*** table below that are related in that
   we do ONE CHECK!!! in one_hit for all the mob_*** things vs all the
   obj_**** things and then if it comes up false, we send the approp
   message...this is to save some minor cpu..but if you need to change this,
   feel free.*/

#define MOB_GHOST        0x00000001   /* Hit by glowing wpns */
#define MOB_DEMON        0x00000002   /* Hit by blessed wpns */
#define MOB_UNDEAD       0x00000004   /* Hit by glow or hum wpns */
#define MOB_DRAGON       0x00000008   /* Need POWER weapon to hit this. */
#define MOB_ANGEL        0x00000010   /* Need baleful wpn to hit this */
#define MOB_FIRE         0x00000020   /* Need water wpn to hit this */
#define MOB_WATER        0x00000040   /* Need fire wpn to hit this */
#define MOB_EARTH        0x00000080   /* Need air wpn to hit this */
#define MOB_AIR          0x00000100   /* Need earth wpn to hit this */
#define MOB_NOMAGIC      0x00001000   /* NO spells work on this */
#define MOB_NOHIT        0x00002000   /* NO physical attacks on this */

/**********************************************************************/
/* These are object flags. Alter obj1_flags in const.c if you alter these. */
/**********************************************************************/

#define OBJ_GLOW         0x00000001  /* Hit ghost */
#define OBJ_BLESSED      0x00000002  /* Hit demons */
#define OBJ_HUM          0x00000004  /* Hit undead */
#define OBJ_POWER        0x00000008  /* Hit dragons */
#define OBJ_CURSED       0x00000010  /* Hit angels */
#define OBJ_WATER        0x00000020  /* Can hit fire mobs */
#define OBJ_FIRE         0x00000040  /* Can hit water mobs */
#define OBJ_AIR          0x00000080  /* Can hit earth mobs */
#define OBJ_EARTH        0x00000100  /* Can hit air mobs */
#define OBJ_NOAUCTION    0x00000200  /* Cannot be auctioned. */
#define OBJ_TWO_HANDED   0x00000400  /* Two hands to wield */
#define OBJ_DRIVEABLE    0x00001000  /* This can be driven. */
#define OBJ_NOSTORE      0x00002000  /* Cannot be stored. */
#define OBJ_MAGICAL      0x00008000  /* This item is already enchanted. */
#define OBJ_NOSACRIFICE  0x00010000  /* This object cannot be sacrificed. */


/**********************************************************************/
/* These are weapon damage types. Alter weapon_damage_types  and
   weapon_damage_names in const.c if you alter these. */
/**********************************************************************/

#define WPN_DAM_SLASH        0
#define WPN_DAM_PIERCE       1
#define WPN_DAM_CONCUSSION   2
#define WPN_DAM_WHIP         3
#define WPN_DAM_MAX          4

/**********************************************************************/
/* These are special attacks. They tell if an attack passed to
   one_hit is normal or special in some way. */
/**********************************************************************/

#define SP_ATT_NONE           0
#define SP_ATT_BACKSTAB       1
#define SP_ATT_IMPALE         2
#define SP_ATT_ATTACK         3

/**********************************************************************/
/* These are different types of magic items. This is what v0 gets
   set to in a VAL_MAGIC value. */
/**********************************************************************/

#define MAGIC_ITEM_SCROLL    1
#define MAGIC_ITEM_WAND      2
#define MAGIC_ITEM_STAFF     3

/* These are light bits used for light values. */

#define LIGHT_LIT         0x00000001
#define LIGHT_FILLABLE    0x00000002


/**********************************************************************/
/* Player flags -- these are usable by admins only. Alter pc1_flags in
   const.c if you alter these. */
/**********************************************************************/

#define PC_HOLYWALK      0x00000002  /* Can walk through anything */
#define PC_HOLYLIGHT     0x00000004  /* Can see all */
#define PC_HOLYPEACE     0x00000008  /* No fighting here. */
#define PC_HOLYSPEED     0x00000010  /* No wait time. */
#define PC_WIZINVIS      0x00000020  /* Cannot be seen */
#define PC_WIZQUIET      0x00000040  /* Cannot hear channels. */
#define PC_SILENCE       0x00000200  /* This person cannot talk. */
#define PC_FREEZE        0x00000400  /* This person cannot move at all. */
#define PC_DENY          0x00000800  /* This person cannot log in. */
#define PC_LOG           0x00001000  /* This person is getting logged. */
#define PC_PERMADEATH    0x00002000  /* This person can die once only. */
#define PC_UNVALIDATED   0x00004000  /* This person is not validated. */
#define PC_ASCENDED      0x00008000  /* Playing an ascended race...*/
  
/**********************************************************************/
/* More player flags -- any player can set these flags. Alter pc2_flags
   in const.c if you alter these. */
/**********************************************************************/

#define PC2_AUTOLOOT     0x00000001
#define PC2_AUTOEXIT     0x00000002
#define PC2_BRIEF        0x00000004
#define PC2_AUTOSAC      0x00000008
#define PC2_AUTOGOLD     0x00000010
#define PC2_AUTOSPLIT    0x00000020
#define PC2_TACKLE       0x00000100
#define PC2_NOSPAM       0x00000200  /* Does not see spammy messages */
#define PC2_MAPPING      0x00000400  /* Has overhead mapping on. */
#define PC2_ASSIST       0x00000800  /* This person assists groupies. */
#define PC2_AUTOSCAN     0x00001000  /* Automatically scans op align folx */
#define PC2_NOPROMPT     0x00002000  /* Does not send a prompt. */
#define PC2_ANSI         0x00004000  /* Do you want ansi colors? */
#define PC2_BLANK        0x00008000  /* Blank line in front of prompt. */
#define PC2_ASCIIMAP     0x00010000  /* Does this player see ascii map? */
#define PC2_NUMBER_DAM   0x00020000  /* Players see number damage in hits. */
/**********************************************************************/
/* Internal player flags. These are not stored in any data structure,
   just make more of them if you need temporary internal flags for your
   players. */
/**********************************************************************/

#define PCF_EDIT_CODE   0x00000001 /* Is this pc editing code? */
#define PCF_TRACKING    0x00000002 /* Is this person tracking atm? */
#define PCF_SEARCHING   0x00000004 /* Are we searching a room atm? */


/**********************************************************************/
/* These are flags which tell how something looks when you look at it. 
   All of the look commands pass through one function to keep things
   simpler. If you add more of these or remove some, make sure you
   don't screw up the functions in look.c. */
/**********************************************************************/

#define LOOK_SHOW_SHORT        0x00000001 /* Show the short desc for
					     this thing. */
#define LOOK_SHOW_LONG         0x00000002 /* Show the long desc for this thing */
#define LOOK_SHOW_DESC         0x00000004 /* Show the descr for this thing */
#define LOOK_SHOW_TYPE         0x00000008 /* Show the short desc in a sentence
					     with the type after it. */
#define LOOK_SHOW_EXITS        0x00000010 /* Do you show exits from this? */
#define LOOK_SHOW_CONTENTS     0x00000020 /* Do you show the contents of this? */
#define LOOK_SHOW_SHOP         0x00000040 /* Shows shop contents */
#define LOOK_SHOW_INVEN        0x00000080 /* Shows inventory */


/**********************************************************************/
/* These are locations where things can be worn. Each item can only
   be worn on one spot. It would be simple to change this so that
   things can be worn more than one spot, but I like this better.
   Alter wear_loc_data in const.c if you alter these. */
/**********************************************************************/

#define ITEM_WEAR_NONE             0
#define ITEM_WEAR_FLOAT            1
#define ITEM_WEAR_HEAD             2
#define ITEM_WEAR_EYES             3
#define ITEM_WEAR_EARS             4
#define ITEM_WEAR_FACE             5
#define ITEM_WEAR_NECK             6
#define ITEM_WEAR_BODY             7
#define ITEM_WEAR_CHEST            8
#define ITEM_WEAR_ABOUT            9             
#define ITEM_WEAR_SHOULDER        10              
#define ITEM_WEAR_ARMS            11
#define ITEM_WEAR_WRIST           12
#define ITEM_WEAR_HANDS           13
#define ITEM_WEAR_FINGER          14
#define ITEM_WEAR_SHIELD          15
#define ITEM_WEAR_WIELD           16 
#define ITEM_WEAR_WAIST           17
#define ITEM_WEAR_BELT            18     
#define ITEM_WEAR_LEGS            19
#define ITEM_WEAR_ANKLE           20
#define ITEM_WEAR_FEET            21
#define ITEM_WEAR_MAX             22
     



/**********************************************************************/
/* These are defines related to spells. The first ones are the kinds of
   spells/skills/profs, the second are the kinds of spells like
   offensive/defensive/self etc. Alter spell_types in const.c if you
   alter these. */
/**********************************************************************/


#define SPELLT_SPELL             0   /* This is a spell..can be cast */
#define SPELLT_SKILL             1   /* This is a combat skill */
#define SPELLT_PROF              2   /* This is a noncombat skill */
#define SPELLT_TRAP              3   /* This is a spell cast on a room */
#define SPELLT_POISON            4   /* This is a spell cast on something */
#define SPELLT_MAX               5

/* Alter spell1_flags in const.c if you alter these. */

#define SPELL_OPP_ALIGN      0x00000001
#define SPELL_HEALS          0x00000002
#define SPELL_REFRESH        0x00000004
#define SPELL_REMOVE_BIT     0x00000008
#define SPELL_PC_ONLY        0x00000010 /* Target must be a pc */
#define SPELL_SHOW_LOCATION  0x00000020 /* Shows where something is */
#define SPELL_NPC_ONLY       0x00000040 /* Shows info on npcs only */
#define SPELL_TRANSPORT      0x00000080 /* Spell transports vict */
#define SPELL_SAME_ALIGN     0x00000100 /* Only works on same align */
#define SPELL_PERMA          0x00000200 /* Deals with perma flags */
#define SPELL_NO_SLIST       0x00000400 /* Don't slist this. */
#define SPELL_VAMPIRIC       0x00000800 /* Gives drained hps to caster. */
#define SPELL_DRAIN_MANA     0x00001000 /* Drains mana */
#define SPELL_KNOWLEDGE      0x00002000 /* Find out info about this thing. */
#define SPELL_MOB_ONLY       0x00004000 /* Cannot be cast on obj or anything else. */
#define SPELL_OBJ_ONLY       0x00008000 /* Only works on objects. */
#define SPELL_ROOM_ONLY      0x00010000 /* Only works on rooms. */
#define SPELL_DELAYED        0x00020000 /* The spell is only delayed...no
					   instant effects. */

/**********************************************************************/
/* These are kinds of spells like offensive/defensive/on self. Alter
   spell_target_types in const.c if you alter these. */
/**********************************************************************/

#define TAR_OFFENSIVE        0
#define TAR_DEFENSIVE        1
#define TAR_SELF             2
#define TAR_ANYONE           3
#define TAR_INVEN            4
#define TAR_MAX              5


#define NUM_PRE             2 /* MAx number of prereqs a spell has */
#define PREREQ_PCT          30 /* The pct you need to have spells pracced
				  to to use them as prereqs. */
#define MAX_PREREQ_OF       20 /* Max number of spells that this is a
				  direct prerequisite of. */
/**********************************************************************/
/* These are kinds of mana. Alter mana_info in const.c if you alter these.
   Also alter the flags up above like FLAG_ELEM_POWER_XXXX since those
   are dependent on there being 5 things down here. */
/**********************************************************************/

  

#define MANA_AIR      0x0001
#define MANA_EARTH    0x0002
#define MANA_FIRE     0x0004
#define MANA_WATER    0x0008
#define MANA_SPIRIT   0x0010

#define MANA_MAX       5     /* This is the number of mana types */


/**********************************************************************/
/* These are the various parts that at thing can have. Alter parts_names
   in const.c if you alter these. */
/**********************************************************************/

#define PART_HEAD               0
#define PART_BODY               1
#define PART_ARMS               2
#define PART_LEGS               3
#define PARTS_MAX               4 


/**********************************************************************/
/* These are "stats" used mostly by pcs. Alter stat_name and 
   stat_short_name in const.c if you alter these. */
/**********************************************************************/

#define STAT_STR              0
#define STAT_INT              1
#define STAT_WIS              2
#define STAT_DEX              3
#define STAT_CON              4
#define STAT_LUC              5
#define STAT_CHA              6
#define STAT_MAX              7

/**********************************************************************/
/* These are a list of the guilds the game supports atm. Alter guild_info
   in const.c if you alter these. */
/**********************************************************************/


#define GUILD_WARRIOR             0
#define GUILD_WIZARD              1
#define GUILD_HEALER              2
#define GUILD_THIEF               3
#define GUILD_RANGER              4
#define GUILD_TINKER              5
#define GUILD_ALCHEMIST           6
#define GUILD_KNIGHT              7
#define GUILD_MYSTIC              8
#define GUILD_DIPLOMAT            9
#define GUILD_MAX                10


#define GUILD_TIER_MAX            5 /* Number of tiers in a guild. */
#define GUILD_POINTS_PER_REMORT   6 /* Number of guild pts you get. */

/**********************************************************************/
/* These are a list of positions...  Be sure that if you change this list
   that you edit position_names and position_looks in const.c          */
/**********************************************************************/

#define POSITION_SLEEPING         1
#define POSITION_STUNNED          2
#define POSITION_RESTING          3
#define POSITION_TACKLED          4
#define POSITION_FIGHTING         5
#define POSITION_STANDING         6
#define POSITION_MEDITATING       7
#define POSITION_CASTING          8
#define POSITION_BACKSTABBING     9
#define POSITION_FIRING          10
#define POSITION_SEARCHING       11
#define POSITION_INVESTIGATING   12
#define POSITION_GATHERING       13
#define POSITION_GRAFTING        14
#define POSITION_FORGING         15
#define POSITION_MAX             16

/**********************************************************************/
/* This is for the act() function that sends a message to specific things.
   Shrug, just make sure you are careful if you add/remove types. */
/**********************************************************************/

#define TO_CHAR                 0 /* Sends to th only */
#define TO_VICT                 1 /* Sends to vict only */
#define TO_NOTVICT              2 /* Sends to all but vict and th */
#define TO_ROOM                 3 /* Sends to all in th->in except th */
#define TO_ALL                  4 /* Sends to all in th->in */
#define TO_COMBAT               5 /* Sends to all with special colors */
#define TO_CHAN                10 /* Sends channel function */
#define TO_SPAM               100 /* Sends only to those who have spam on*/

/**********************************************************************/
/* These are constants associated with time and weather. */
/* Change the const structs in const.c if you mess with these
  constants. */
/**********************************************************************/


#define NUM_YEARS              25 /* Number of different years. */
#define NUM_MONTHS             14 /* Months in a year. Alter month_names 
				     in const.c if you alter this. */
#define NUM_WEEKS               3 /* Weeks in a month */
#define NUM_DAYS                8 /* Days in a week. Alter day_names in 
				     const.c if you alter this. */
#define NUM_HOURS              26 /* Hours in a day. Alter hour_names in 
				     const.c if you alter this. */

#define HOUR_DAYBREAK           7 /* When dawn comes. */
#define HOUR_NIGHT             20 /* When nighttime comes. */

/* Alter weather_names in const.c if you alter these. */

#define WEATHER_SUNNY            0
#define WEATHER_CLOUDY           1
#define WEATHER_RAINY            2
#define WEATHER_STORMY           3
#define WEATHER_FOGGY            4
#define WEATHER_MAX              5 /* Maximum weather number... */


/**********************************************************************/
/* These are used for time and weather info. Make sure that NUM_VALS
   is always at least as big as the largest one of these. */
/**********************************************************************/


#define WVAL_YEAR                 0
#define WVAL_MONTH                1
#define WVAL_DAY                  2
#define WVAL_HOUR                 3
#define WVAL_TEMP                 4
#define WVAL_WEATHER              5
#define WVAL_LAST_RAIN            6

/**********************************************************************/
/* Raw material types. Make sure you alter gather_data in const.c
   if you alter these. Note that the gathering commands are directly
   related to this list, so you have to be careful...if you remove
   one of these, remove the entire block from gather_data and
   if you add something here, make sure you put the data into gather_data
   in the same order as here.*/
/**********************************************************************/

/* Note that 0 is RAW_NONE. This is because I didn't want to have 
   "blank" values having any use. This also means that a lot of
   loops in the code start at 1 if they go to RAW_MAX. If you change
   RAW_NONE = 0 to something useful, you will have to change those
   loops. */
  
#define RAW_NONE           0
#define RAW_MINERAL        1
#define RAW_STONE          2
#define RAW_WOOD           3
#define RAW_FLOWER         4
#define RAW_FOOD           5
#define RAW_HERB           6
#define RAW_ICHOR          7
#define RAW_MAX            8




/**********************************************************************/
/* Conditions */
/**********************************************************************/

#define COND_HUNGER          0
#define COND_THIRST          1
#define COND_DRUNK           2
#define COND_MAX             3

#define COND_FULL            60


/**********************************************************************/
/* SECTION structs */
/**********************************************************************/


/**********************************************************************/
/* This is a link from the file descriptors to the things in the game. */
/**********************************************************************/

struct file_desc 
{
  int r_socket;                 /* This is the actual file descriptor # */
  FILE_DESC *next;              /* Linked list of fd's */
  THING *th;                    /* What thing is this attached to? */
  int connected;                 /* The state the person is in while online */
  char read_buffer[STD_LEN*2];  /* Buffer used to store commands coming
in */
  char write_buffer[WRITEBUF_SIZE]; /* Buffer used to store output */
  char *write_pos;               /* Used to avoid strcat () in write2buff. */
  char command_buffer[STD_LEN];  /* The current string being interpretted */
  char last_command[STD_LEN];    /* Last command entered. */
  char num_address[30];          /* Address of peer... inet_ntoa */
  int timer;                     /* How long since last command. */
  int hostnum;                   /* Number address of the host. */
  char name_address[100];        /* Name of peer - prob not gonna */
  FILE_DESC *snooping;           /* Person we are snooping */
  FILE_DESC *snooper;            /* Person snooping us. */
  THING *oldthing;               /* Used with switch. */
  int flags;                     /* Flags for checking status */
  int repeat;                    /* Number of repeated commands. */
  int channel_repeat;               /* Used for spamming channels. */
};

/**********************************************************************/
/* This structure is used to set up a command list which is 
   referenced by the first character in the command. */
/**********************************************************************/

struct _cmd
{
  char *name;                       /* This is the "name" of the command
				       you type in to use it. */
  char *word;                       /* Used in crafting...word is the
				       name of the item we will try to
				       make. */
  COMMAND_FUNCTION *called_function; /* This is the actual pointer to the
					actual function which is executed
					when you do this thing. */
  int level;                         /* Min Level to Use */
  CMD *next;                         /* Next command in the list */
  bool log;                          /* Is this always logged? */
};

 /**********************************************************************/
/* This is the thing struct. It is the most important one in the
   game. It is used for alot of things, including things it probably
   shouldn't be used for...like spells. You shouldn't
   be able to do anything to or with prototypes. */

/* Essentially things have several processes associated with them.
   1. File I/O.
   2. Creating/Destroying/Cloning things.
   3. Entering/Leaving things.
   4. Finding things by the list of names.
   5. Moving things around.
   6. Editing things.
   7. Presenting the information found in things.
   
   The first four of these things had to be done all at once, and I am
   shocked that they mostly worked the first time around. Also, note
   that the "prototypes" originally started out as a different struct,
   but then I realized I would be cloning rooms (which take up
   most of the memory anyway), so I went to this whole "prototypes in
   the area thing" idea...and it seems to work pretty well. Essentially
   find_thing_num searches for a thing in an area or which IS an area.
   Nice and clean. Also, the doubly linked lists added in later for
   the thing_hash are funky because I want the prototypes at the TOP of
   the lists...so that if you make 1000 pieces of bread, it does
   not have to search down 999 things to find the proto...they should
   be at the top. Later on, the ->proto thing was added to cut
   down on the find_thing_num uses. */

/* As a general rule, you should not add to this too much if you're
   making special data for a few things. If the data is player-only,
   put it in the pc_data struct. If it's something that only a few
   things will have, put it in a value. That's what they're there for --
   to keep people from making this too huge, and to keep people from
   having to write extra serialization code for every little thing they
   add to their game. */
/**********************************************************************/

struct thing_data
{
  THING *next;                      /* Linked list of indexes... */
  THING *prev;                      /* Linked list of indexes... */
  THING *in;                        /* What thing is this in? */
  THING *last_in;                   /* What this thing was last in. */
  THING *cont;                      /* What this thing contains. */
  THING *next_cont;                 /* Next item in the same thing as this. */
  THING *prev_cont;
  EVENT *events;                    /* Events for this thing. */
  FILE_DESC *fd;                    /* This thing can send output to FD. */
  
  THING *next_fight;                /* Next in fight list. */
  THING *prev_fight;                /* Prev in fight list. */
  FIGHT *fgt;                       /* Data about what this is fighting...
				       useful for other things like
				       following etc. */
  FLAG *flags;                     /* These are affects/+dam etc on things */
  VALUE *values;                   /* These are extra values which can
				       be added to this thing if needed. 
                                       I know I should have different
                                       structs for each of these, but 
                                       my way is "close enough" for now. */
  RESET *resets;                    /* Resets in this thing. */

  PCDATA *pc;                      /* This is used only if this thing
				      is a real player. */
  TRACK *tracks;                   /* Tracks to and from this. */
  THING *proto;                     /* Prototype info for strings... */
  NEED *needs;                      /* Society members have needs (objs). */
  EDESC *edescs;                    /* Extra descriptions...*/
  char *name;                       /* The "names" of this thing */
  char *short_desc;                 /* The "short desc" of this thing */
  char *long_desc;                  /* The "long desc" of this thing. */
  char *desc;                       /* The thing's description. */
  char *type;                       /* dragon, weapon etc... */
  int thing_flags;                  /* Special flags for all things... */
  int move_flags;                   /* This is an optimization thing. Rooms
				       will have bits set that tell which
				       BADROOM_BITS they have set...so
				       we quickly know what you need to 
				       get through them. On other things,
				       this will represent rooms that the
				       thing can move through. It's so
				       that find_track_room() is little
				       faster. */
  int hp;                           /* all things have hps!! */
  int max_hp;                       /* hp_mult for protos. */
  int vnum;                         /* What the "number" of this is. */
  unsigned int cost;                /* How much it costs... */
  int carry_weight;                 /* Weight of all stuff carrying. */
  unsigned int height;
  unsigned int weight;
  unsigned short size;              /* 0 = unlim */
  unsigned short light;             /* Is there any light from this? */
  short max_mv;                     /* Max moves (stamina) (max in world)*/
  short mv;                         /* Cur moves (stamina) (curr in wld) */
  short level;
  short timer;                      /* How long this will last */
  short armor;                      /* Overall armor rating of this thing. */
  unsigned char align;              /* This thing's alignment. */
  unsigned char position;           /* What position is this thing in? */
  char symbol;                      /* Map symbol... */
  unsigned char sex;                /* Sex of this thing. */
  char color;                       /* color 0-F like color editor */
  char exits;                       /* The exits this has for tracking use. */
  char goodroom_exits;              /* Exits lead to goodrooms. */
  unsigned char wear_pos;           /* Where this can be worn */
  unsigned char wear_loc;           /* Where the item is worn */
  unsigned char wear_num;           /* Which item is worn in this loc */  
  unsigned char kde_pixmap;         /* Which kde pixmap is assigned to this. */
  unsigned char ascii_symbol;       /* Which ascii symbol represents this. */
  /* This is a big memory hog used to speed up room tracking. */
  /* I love how inheritance lets me split this data off into a Room class
     so I don't end up wasting precious memory on data only needed for
     rooms. Oops, crap this is C. That sucks. :( */
  THING *adj_room[REALDIR_MAX];
  THING *adj_goodroom[REALDIR_MAX];
};

/**********************************************************************/
/* This is an extended struct used for pcs only. */
/**********************************************************************/

struct pc_data
{
  PCDATA *next;
  char **curr_string;           /* The current string we are editing */
  void *editing;                /* What we are editing atm */
  NOTE *note;                   /* What note we are editing */
  int curr_note;                /* The current note we are reading */
  short stat[STAT_MAX];         /* Stat values */
  short guild[GUILD_MAX];       /* Guilds */
  short parts[PARTS_MAX];       /* Body parts...may be dynamic later */
  short armor[PARTS_MAX];       /* Armor on each bodypart. */
  short implants[PARTS_MAX];    /* Implant level for this pc */
  int aff[AFF_MAX];             /* These are affects which get changed
				    on a character as flags from items
				    which have affects on them as they
				    are worn and removed. */
  char *email;                   /* Email address */
  char *pwd;                     /* Password */
  char *big_string;              /* Big string to show... */
  char *big_string_loc;          /* Where in big string we are. */
  char *title;                   /* Character title. */
  char *prompt;                  /* Prompt string. */
  int pcflags;                   /* Special internal pc flags. */
  unsigned char race;            /* What race this pc is.... */
  short remorts;                 /* How many remorts this pc has */
  short practices;               /* How many practices this char has. */
  short wait;                    /* How long this pc must wait... */ 
  short max_mana;                /* Max mana this pc has */
  short mana;                    /* What current mana this pc has */
  int pagelen;                   /* How many lines on th`is person's page. */
  int in_vnum;                   /* Where was this player before quitting */
  int bank;
  int hostnum;                   /* Host number for current login. */
  short cond[COND_MAX];          /* Conditions */
  unsigned char prac[MAX_SPELL]; /* Prac percents */
  unsigned char learn_tries[MAX_SPELL]; /* How many tries you've had 
					   to try to learn this
					   skill/spell. */
  unsigned char nolearn[MAX_SPELL]; /* You don't try to learn these
					skills/spells. */
  short clan_num[CLAN_MAX];      /* Which clan/sect etc you are in. */
  THING *storage[MAX_STORE];     /* storage items */
  THING *reply;                  /* Last reply */
  TROPHY *trophy[MAX_TROPHY];
  char *aliasname[MAX_ALIAS];    /* List of alias names */
  char *aliasexp[MAX_ALIAS];     /* List of expanded aliases */ 
  int pk[PK_MAX];
  int exp;                       /* Experience for this player. */
  int fight_exp;                 /* Fight exp since last thing killed. */
  THING *guarding;               /* What thing you are guarding. */
  char *ignore[MAX_IGNORE];      /* People ignored. */
  char temp[STD_LEN];            /* Temporary string to store the
				    last argument issued... */
  VALUE *qf;                     /* Script flags...6 ints and a char *name */
  unsigned short off_def;         /* offensive vs defensive pct. */
  int no_quit;                   /* How long until you can quit. */
  int time;                      /* Total time played. */
  int login;                     /* Time the player logged in. */
  int last_login;                /* Time when the player logged in before
				    this login. Used to calculate taxes. */
  int wimpy;                     /* When you flee */
  bool channel_off[MAX_CHANNEL]; /* Is this channel off? */
  int sback_pos[MAX_CHANNEL];    /* Where you are in the scrollback loop */
  SCROLLBACK *sback[MAX_CHANNEL][MAX_SCROLLBACK]; /* Actual strings. */
  int pfile_sent;                /* Time when pfile was sent */
  int quest_points;              /* Quest points...bleh :P */
  bool using_kde;                /* Using KDE client? */
  int kde_update_flags;          /* What do we update this time... */
  THING *pets;                   /* List of pets for loading time. */
  int warmth;                    /* Warmth level. */
  MAPGEN *mapgen;                /* The current map you've generated. */
  int gathered[RAW_MAX];
  bool battling;                 /* Will this person join the battle? */
  int last_saved;                /* Last time someone typed save. */
  /* These are used so that mobs know who hits the hardest. */
  int damage_total;              /* Damage total for this char this boot. */
  int damage_rounds;             /* Number of rounds of combat engaged in. */
  int last_rumor;                /* Last time you asked for a rumor. */
  int last_tstat;                /* The vnum of the last thing tstated. 
				    Lets you edit "this" */
  int align_hate[ALIGN_MAX];     /* How much each align hates you..based 
				    on how many of that align that are
				    in societies or can talk that you've
				    helped to kill. */
  /* These are to be used for !muling code where the length of logins and
     the number of items xferred is checked to see if the player is being
     used as a mule or not... */
  int login_length[MAX_LOGIN];   /* How long they were logged in. */
  int login_item_xfer[MAX_LOGIN]; /* How many good items were transferred
				     to or from this thing during this
				     login. */

  int login_times;               /* How many times the player has logged in. */
  int played;                    /* How many times you played with children. */
  int society_rewards[REWARD_MAX]; /* How many society rewards you've
				      racked up lately. They get "paid"
				      during "update_fast" */
  THING *society_rewards_thing[REWARD_MAX]; /* What thing we acted on to
					      get the reward. */
};
 /**********************************************************************/
/* This has the information for a spell. */
/**********************************************************************/
struct spell_data 
{
  SPELL *next;
  SPELL *next_name;
  SPELL *next_num;
  char *name;
  char *ndam;
  char *damg;
  char *duration;
  char *wear_off;
  char *message;
  char *message_repeat;  /* Used for spells that repeatedly hit. */
  char *pre_name[NUM_PRE];
  SPELL *prereq[NUM_PRE];  
  int num_prereq[SPELLT_MAX]; /* Number and type of each prereq */
  int prereq_of[MAX_PREREQ_OF]; /* Spells this is a prereq of. */
  unsigned char guild[GUILD_MAX];
  unsigned char min_stat[STAT_MAX];
  short spell_type;
  short target_type;
  short mana_amt;
  short mana_type;
  int vnum;
  int comp[MAX_COMP];
  int comp_lev;
  char *compname;
  FLAG *flags;
  int repeat;             /* Number of times the spell hits the player. */
  int delay;              /* Delay between hits. */
  int damage_dropoff_pct; /* Pct dropoff each time the spell hits. */
  int level;
  int ticks;
  int position;
  int creates;
  int transport_to;
  int teacher[MAX_TEACHER]; /* up to 10 teachers */
};

/**********************************************************************/
/* This contains the information about one clan/sect etc. See clan.c to
   see how this code works..it deals with all kinds of clans at once. */
/**********************************************************************/
struct clan_data
{
  CLAN *next;          /* Next in linked list */ 
  char *name;          /* Clan name...        */
  char *motto;         /* Some message they use */
  char *history;       /* Clan history for outside world */
  int room_start;      /* Where the chouse starts */
  int clan_store;
  int num_rooms;       /* Where the chouse ends */
  int flags;           /* flags for this clan */
  int type;            /* clan or sect or temple? */
  int minlev;          /* Minimal level to join this clan */
  int vnum;            /* what is the number of this clan..within its type*/
  char *member[MAX_CLAN_MEMBERS]; /* List of clan member names */
  char *leader;        /* Leader's name */
  THING *storage[MAX_CLAN_STORE]; /* Clan storage... */
  
};

/**********************************************************************/
/* This is a damage message. It gets loaded up on reboot. */
/**********************************************************************/

struct _damage_data 
{
  int low;         /* Low end of damage range */
  int high;        /* High end of damage range */
  char *message;   /* Damage message */
  DAMAGE *next;
};

/* This is am extra description to be added to a thing. */

struct _extra_description
{
  EDESC *next;
  char *name;
  char *desc;
};

/**********************************************************************/
/* This is a social. These are pretty simple since I don't consider
   socials something to waste a lot of time on...I am fairly hostile
   to RP (as you will see if you look through the code), and so
   this is just to make workable, if not perfect, socials.

 */
/**********************************************************************/


struct social_data
{
  SOCIAL *next;
  SOCIAL *next_list;
  char *name;
  char *no_vict;
  char *vict;
};



  
/**********************************************************************/
/* This structure contains information about one kind of flag only... */
/**********************************************************************/

struct flag_data 
{
  char *name;
  char *app;
  unsigned int flagval;
  char *help;
};
/**********************************************************************/
/* These are things which add affects to things This is not quite just
   having flags set onto the things since some of them can be affects
   basically like affecting the ability to absorb damage or something. */
/**********************************************************************/

struct flag_actual
{ 
  FLAG *next;           /* Next flags in the list */
  unsigned short from;    /* What originally caused this... type == 0 means
			     it's natural. others mean spells? */
  unsigned char type;      /* The location of the affect. This could mean 
			     regular affects or things like flags that
			     make rooms do things or objects have
			     For example, area flags = loc of 1. */
  short timer;            /* How long this will last..most things
			     created for things naturally last forever */
  unsigned int val;       /* The actual number */
};

/**********************************************************************/
/* This is a function which sets up a list of pointers to various
   kinds of flags. It is easier than trying to handle all this at once. */
/**********************************************************************/

struct flag_pointer
{
  char *name;
  char *app;
  FLAG_DATA *whichflag;
};
/**********************************************************************/
/* These are values you can set up when you are making things with
   extended information like exits! and weapons and spec procs for mobs ? */
/**********************************************************************/


struct _value_actual
{
  VALUE *next;
  int val[NUM_VALS];
  char *word;
  unsigned short type;  /* Type of value...wpn armor etc... */
  short timer; /* Timer, number of hours till this poofs. */
};


/**********************************************************************/
/* This is a piece of the fight data used for fighting and hunting and
   such. */
/**********************************************************************/

struct fight_data
{
  FIGHT *next;
  THING *fighting;
  THING *rider;
  THING *mount;
  THING *following;
  THING *gleader;
  THING *hunt_victim; /* The thing we're hunting if we find tracks for it. */
  char hunting[HUNT_STRING_LEN + 1];
  char old_hunting[HUNT_STRING_LEN + 1];  /* Used when we need to stop hunting to
			    kill something and go back to what we
			    were doing already. */  
  int old_hunting_type;  /* old type of hunting we were doing. */
  int bashlag;           /* Bashlag for in combat. */
  int delay;             /* Delay from combat commands. */
  int hunting_type; /* Why this thing is tracking something */
  int hunting_timer;  /* Forgets after several hours...? */
  int hunting_failed;  /* This gets incremented by 15 if you fail
			  to track something to keep you from spam
			  hunting. */
};

  
/**********************************************************************/
/* This is one reset. */
/**********************************************************************/

struct reset_data
{
  RESET *next;
  int vnum;
  unsigned char pct;
  unsigned char nest;
  unsigned int times;
};

/**********************************************************************/
/* This is one helpfile. */
/**********************************************************************/


struct help_data
{
  HELP *next;
  char *keywords;        /* Keywords to access this helpfile. */
  char *admin_text;      /* Admin text only MAX LEVEL people see. */
  char *text;            /* Text that everyone sees. */
  char *see_also;        /* Other keywords to see with this
			    helpfile. */
  int level;             /* Min level to see this helpfile. */
};


/**********************************************************************/
/* This is one helpfile. */
/**********************************************************************/

struct help_keyword
{
  HELP_KEY *next;
  char *keyword;
  HELP *helpfile;
};

/**********************************************************************/
/* This is one auction. */
/**********************************************************************/

struct auction_data
{
  AUCTION *next;
  THING *item;
  THING *seller;
  THING *buyer;
  int ticks;
  int bid;
  int align;
  int num_bids;
  int number;
};




/**********************************************************************/
/* This gives the information for one channel.                        */
/**********************************************************************/

struct channel_data
{
  CHAN *next;
  char *name[CHAN_NAME_MAX]; /* up to 3 names. */
  int number;                /* What number this channel is. */
  int num_names;             /* Number of names this has. */
  int use_level;             /* Level to use this */
  int to_level;              /* Level to see this */
  int flags;                 /* Tells what kind of channel this is. */
  short clantype;            /* Clantype if needed for this. */
  char *msg;                 /* Message you see at the start. */
};
  


/**********************************************************************/
/* This is the data which sets up how and when things can be worn */
/**********************************************************************/

struct wear_loc_data
{
  char name[50];             /* Mame of the worn location */
  char view_name[50];        /* How this is viewed like "on legs" */
  short part_of_thing;       /* What "part" this goes on. */
  short how_many_worn;       /* How many can be worn on that part */
  int flagval;               /* The actual value of this flag */
};



/**********************************************************************/
/* This struct is a single playerbase data.                           */
/**********************************************************************/

struct pbase_data 
{
  PBASE *next;
  char *name;               /* Name of the person */
  char *email;              /* email address */
  char *site;               /* Number of site */
  unsigned short level;     /* Level they have */
  unsigned short remorts;   /* Number of remorts */
  unsigned short align;     /* Alignment number */
  unsigned short race;      /* Race they are */
  int last_logout;          /* What time they last logged out. */
};


/**********************************************************************/
/* This struct is a single site that has been banned.                 */
/**********************************************************************/

struct siteban_data
{
  SITEBAN *next;          /* Next siteban in list */
  char *address;          /* The banned address */
  unsigned short type;    /* What kind of a ban is this 0 = reg, 1 = newb */
  char *who_set;          /* Name of the person who set this. */
  int time_set;           /* When it was set. */
};
  



/**********************************************************************/
/* This struct is used with race and alignment info.                  */
/**********************************************************************/

struct race_data 
{
  int vnum;
  FLAG *flags;    /* What flags chars of this race automatically get. */
  short max_stat[STAT_MAX]; /* Max stats for this race */
  short max_spell[SPELLT_MAX]; /* Max skill.spell etc for this race*/
  unsigned short parts[PARTS_MAX]; /* How many heads arms legs etc it has. */
  int height[2];     /* Min/max height */
  int weight[2];     /* Min/max weight */
  /* Bonuses for regenning hps/moves */
  int hp_regen_bonus;
  int mv_regen_bonus;
  int mana_pct_bonus; /* Bonus percent of mana granted to this char. */
  int power_bonus;    /* Bonus or penalty to power based on who's winning
			 right now. Basically aligns that are losing get
			 these bonuses so that more people will want to be
			 on that side. It is calculated by looking at the
			 pkdata stats and seeing which aligns are winning
			 on those. */
  
  char *name;       /* Name of this race... */
  unsigned short nat_spell[NATSPELL_MAX];  /* Natural things known */
  int ascend_from;     /* What race this ascends to. */
  /* Since races == alignments, these things are the variables used for
     keeping track of alignments above societies within the rts code. 
     This should probably be multiple-inherited from society and race,
     but bleh this will be close enough for now. */
  
  int power;                /* Total power of this alignment. (levels) */
  int population;           /* Total population in this alignment. */
  int warriors;             /* Total number of warriors. */
  int num_societies;        /* The number of societies this side has.*/
  int raw_curr[RAW_MAX];    /* Raw materials on hand in the align "bank". */
  int raw_want[RAW_MAX];    /* Raw materials wanted by societies. */

  int pop_pct;              /* Population percent relative to "average". */
  int power_pct;            /* Power percent relative to "average". */
  int warrior_pct;          /* Percent of warrior pow rel to average. */
  int ave_pct;              /* Average of pop and power pct...ave power. */
  int relic_ascend_room;    /* The relic room where the relics for this
			       alignment are kept. For races this is
			       the room where they ascend. */
  int relic_amt;            /* The number of relics as of last update.
			       Relics give combat bonuses. :) */
  int warrior_goal_pct;     /* Percent of resources allocated to military. */
  int most_hated_society;   /* The most hated society for this align is
			       the one with the most kills vs this align. */
  int outpost_gate;         /* Vnum of the society's outpost gate. */
};
  
  
/**********************************************************************/
/* This is one block of pkdata. */
/**********************************************************************/

struct pkill_data
{
  char name[30];   /* Name of character */
  float value;  /* Value of pkdata */
  int align;    /* Alignment of this pc */
};

 
/**********************************************************************/
/* This is a trophy */
/**********************************************************************/

struct trophy_data
{
  char name[30];
  short level;
  short remorts;
  short times;
  short align;
  short race;
  TROPHY *next;
};


/**********************************************************************/
/* This is a scrollback buffer used to avoid malloc/free 
   This is probably more wasteful up front, but I think it will save
   memory in the long run because the memory won't be as fragmented. */
/**********************************************************************/


struct scrollback_data
{
  char text[MAX_SCROLLBACK_LENGTH + 1];
  SCROLLBACK *next;
};




/**********************************************************************/
/* These are all used for making/improving eq. They can be split
   up into three different categories. 1. These are the things that 
   let you gather raw materials. 2. Improve current eq. 3. Make new
   eq using raw materials. */
/**********************************************************************/

/* This struct is used to make commands to collect raw materials 
   in certain ways. I just didn't want to write basically 10 of the
   same function over and over :P. */


struct gather_raw_data
{
  char *command;            /* Command name...also spell name lookup */
  char *raw_name;           /* What kind of raw material this gathers. */
  char *raw_pname;          /* Plural name of the raw material. */
  short level;              /* Level to use. */
  short time;               /* Ticks needed to gather it. */
  int equip[GATHER_EQUIP];  /* Equip needed to gather stuff. specific #'s */
  int equip_value[GATHER_EQUIP];  /* What value the equip needs. */
  char *equip_name[GATHER_EQUIP]; /* What the name is the equip needs. */
  int use_up[GATHER_EQUIP]; /* The item of this number gets used up. */
  int put_into;             /* Have to have this to put the material in. */
  bool need_minerals;       /* Does this thing need to have minerals set? */
  bool need_skill;          /* Is this a skill or not? */
  int room_flags;           /* What room flags we need to do this here? */
  int min_gather;           /* Min vnum of gathered. */
  int items_per_rank;       /* Number of items in each gather rank. */
  int gather_ranks;         /* Number of ranks to be gathered. */
  int stop_here_chance[2];  /* Stop if nr(1,(a)) is > (b). */
  int fail[2];              /* Fail if nr(1,(a)) is > (b). */
};

/* This deals with taking an item with a certain value, and several
   copies of another item and grafting the new item onto the old one. */
   
struct graft_value_data
{
  char *command;           /* What command you type -- and spellname */ 
  short time;              /* How long it takes to do this. */
  short oldval;            /* Old value needed */
  short newval;            /* New value needed */
  short num_needed;        /* number of item of the old type needed */
  short copy_pct[NUM_VALS]; /* Number from 1-100 specifying what pct
			       of this value gets copied over. */
  
};

  

/* This struct lists the information for each kind of forged 
   equipment. */

struct _forge_data
{
  char *name;
  short wear_loc;   /* Where this is worn */
  short val_type;   /* What kind of value this item gets */
  short stat_type;  /* What stat is modified by this at higher powers */
  int val_num;      /* Which val set */
  int val_set_to;   /* What it's set to. */
  int weight;        /* Base weight...each mineral has a multiplier */
  int cost;          /* Base cost */
};


  /**********************************************************************/
  /* SECTION externs */
  /**********************************************************************/

extern int listen_socket;
extern int max_fd;  
extern int max_players; /* Max number of players online since boot. */
extern int current_time;
extern int startup_time;
extern int reboot_ticks;
extern int seg_count;
extern int times_through_loop; /* Number of times through the
				  game loop. */
extern int png_seed;   /* Used to reseed the png periodically */
extern int png_count;  /* Used to see which bit we're updating atm. */
extern int png_bytes;  /* How many bytes have been read/sent recently */
extern int total_memory_used; /* How much memory (estimated) has been used. */
extern FILE_DESC *fd_list;
extern FILE_DESC *fd_free;
extern THING *thing_free;
extern THING *thing_free_pc;
extern THING *thing_free_track; /* Used to clean up tracks better. */
extern int thing_free_count;    /* Used to keep memory in check. */
extern PCDATA *fake_pc;
extern CMD *cmd_free;
extern VALUE *value_free;
extern FLAG *flag_free;
extern FIGHT *fight_free;
extern RESET *reset_free;
extern EDESC *edesc_free;
extern SCROLLBACK *scrollback_free;
extern PCDATA *pc_free;
extern CLAN *clan_free;
extern TROPHY *trophy_free;
extern SPELL *spell_free;
extern RACE *race_info[RACE_MAX];
extern RACE *align_info[ALIGN_MAX];
extern HELP *help_list;
extern HELP *help_free;
extern SITEBAN *siteban_list;
extern PBASE *pbase_list;
extern SOCIAL *social_list;
extern HELP_KEY *help_hash[27][27];
extern SOCIAL *social_free;
extern int spells_known[SPELLT_MAX]; /* How many prereqs you know. */
extern int server_flags;
extern int boot_time;
extern int top_race;
extern int top_align; 

extern int reboot_ticks;
/* These keep track of memory sizes to give us a rough estimate of
   how much memory we are using... */

extern int thing_count;
extern int thing_made_count; /* Actual number of times things get made,
				as opposed to number of mallocs done. */
extern int value_count;
extern int flag_count;
extern int fight_count;
extern int reset_count;
extern int pc_count;
extern int clan_count;
extern int race_count;
extern int spell_count;
extern int help_count;
extern int file_desc_count;
extern int cmd_count;
extern int help_key_count;
extern int auction_count;
extern int string_count;
extern int chan_count;
extern int channel_count;
extern int siteban_count;
extern int pbase_count;
extern int social_count;
extern int scrollback_count;
extern int edesc_count;

/* These are used to find the average bfs size. */

extern int bfs_times_count;
extern int bfs_tracks_count;


extern int last_thing_edit; /* This is the last time a thing was edited..
			       used for autosave for unsaved areas. */
extern VALUE *wt_info;
extern THING *the_world;    /* This is the top of the world list */
extern THING *fight_list;   /* This is the list of things that are
			      fighting or could be fighting. */
extern THING *fight_list_pos; /* Where we are on the fight list. */
extern DAMAGE *dam_list;     /* List of damage messages */
extern CHAN *channel_list;  /* List of all communications channels */

extern THING *hunting_victim; /* What victim we are hunting. */
extern int min_hunting_depth; /* Minimum hunting depth. */

/* Auction globals. */

extern AUCTION *auction_list;
extern AUCTION *auction_free;
extern int top_auction;


/* Battleground globals. */

extern int bg_hours;       /* Hours to bg. */
extern int bg_minlev;      /* Min level for this bg. */
extern int bg_maxlev;      /* Max level for this bg. */
extern int bg_money;       /* Cash prize for bg. */
extern THING *bg_prize[BG_MAX_PRIZE]; /* Prizes for the bg. */

extern int top_vnum;
extern int lowest_vnum;
extern int top_spell;
extern int cons_hp;         /* Hps left in a thing during consider. */
extern int currchan;            /* Current channel sent to act() yucky :P */
extern int current_damage_amount; /* Used to show pc's damage amt's */
extern char *nonstr;            /* Ptr to nothing_string */
extern char nothing_string[1];  /* Used for string creation/destruction. */
extern char prev_command[STD_LEN]; /* Last command (used for crashes.) */
extern char pfile_name[40];  /* Name of pfile used to open it. */
extern char bigbuf[BIGBUF];  /* Used for writing long outputs to send to pcs.*/
extern int bigbuf_length;
extern CMD *com_list[256];           /* Command list headers */
extern SOCIAL *social_hash[256];   /* Socials */
extern THING *thing_hash[HASH_SIZE]; /* Linked lists of all things */
extern THING *thing_hash_pointer; /* Used for traversing lists in updates */
extern THING *thing_cont_pointer; /* Used for traversing room lists */
extern THING *thing_nest[MAX_NEST];
extern int room_count[AREA_MAX]; /* Number of rooms in each area (open for 
				    repop) room_count[0] is the total number 
				    for all areas. */
extern int before_trig;
extern int after_trig;
extern int alliance[ALIGN_MAX];
extern bool used_spell[MAX_SPELL]; /* Used for prereq tree */
extern bool draw_line[MAX_PRE_DEPTH];
extern int num_prereqs[SPELLT_MAX]; /* Number of prereqs a spell has */
extern int top_clan[CLAN_MAX];
extern CLAN *clan_list[CLAN_MAX];

extern PKDATA pkd[PK_MAX][PK_LISTSIZE];
extern SPELL *spell_name_hash[256];
extern SPELL *spell_number_hash[256];
extern SPELL *spell_list;

/* Names of the types of pk data. */
extern const char *pk_names[PK_MAX];
extern const char *sex_names[SEX_MAX];
extern const char *parts_names[PARTS_MAX]; /* Names of parts that make up things */
extern const char *implant_descs[PARTS_MAX]; /* Descriptions of what implants do. */
extern const struct wear_loc_data wear_data[]; /* How things are worn */
extern const char *carry_weight_amt[MAX_CARRY_WEIGHT_AMT]; 

/* Symptoms things show when they're sick. */
extern const char *disease_symptoms[MAX_DISEASE_SYMPTOMS]; 

/* List of symbols for pixmaps and pixmap symbols This is also used for
   the wilderness map. */

extern const char pixmap_symbols[PIXMAP_MAX][4];




/* These are all bitvector descriptions */
extern const struct flag_data thing1_flags[];
extern const struct flag_data area1_flags[];
extern const struct flag_data room1_flags[];
extern const struct flag_data obj1_flags[];
extern const struct flag_data aff1_flags[];
extern const struct flag_data aff2_flags[];
extern const struct flag_data act1_flags[];
extern const struct flag_data act2_flags[];
extern const struct flag_data mob_flags[];
extern const struct flag_data pc1_flags[];
extern const struct flag_data pc2_flags[];
extern const struct flag_data society1_flags[];
extern const struct flag_data caste1_flags[];
extern const struct flag_data affectlist[];
extern const struct flag_data spell1_flags[];
extern const struct flag_data clan1_flags[];
extern const struct flag_data money_info[];
extern const struct flag_data mana_info[];
extern const char *value_list[VAL_MAX];/* These are the kinds of values
					  you can add to things. */
extern const struct flag_data guild_info[]; /* These are guilds...and some info. */
extern const struct flag_data trigger1_flags[]; /* Used in scripts */
extern char *dir_name[DIR_MAX];
extern char *dir_letter[DIR_MAX];
extern char *dir_rev[DIR_MAX];
extern char *dir_track[DIR_MAX];
extern char *dir_track_in[DIR_MAX];
extern char *dir_dist[SCAN_RANGE]; /* 4 things..match up to do_scan dist */
extern const char *stat_name[STAT_MAX];
extern const char *stat_short_name[STAT_MAX];
extern char *groundfight_damage_types[NUM_GROUNDFIGHT_DAMAGE_TYPES];
extern char *clantypes[CLAN_MAX];
extern const char *day_names[NUM_DAYS];   /* List of day of the week names. */
extern const char *month_names[NUM_MONTHS]; /* List of month names */
extern const char *hour_names[NUM_HOURS]; /* List of hour names */
extern const char *year_names[NUM_YEARS];  /* List of year names...just some random.*/
extern const char *weather_names[WEATHER_MAX];
extern const char *mob_condition[CONDITIONS];
extern const char *obj_short_condition[CONDITIONS];
extern const char *obj_condition[CONDITIONS];
extern const char *mineral_names[MAX_MINERAL];
extern const char *mineral_colornames[MAX_MINERAL];
extern const struct _forge_data forge_data[]; /* Searches stop on blank name */
extern char no_echo[];
extern char yes_echo[];
extern char *position_names[];
extern char *position_looks[];
extern char *spell_types[];
extern char *spell_target_types[];
extern const char *tool_names[TOOL_MAX];
extern const  char *weapon_damage_types[]; /* Types of weapon damage. */
extern const char *weapon_damage_names[]; /* Names of damage you see in
					     combat (Defaults) */
extern char *score_string;
extern struct flag_pointer flaglist[NUM_FLAGTYPES];
extern int newbie_items[NUM_NEWBIE_ITEMS]; /* List of newbie items */
/* These are used for making random potions/scrolls */

extern const char *potion_colors[POTION_COLORS];
extern const char *potion_color_names[POTION_COLORS];
extern const char *potion_types[POTION_TYPES];

/* Info for gathering raw materials */
extern const struct gather_raw_data gather_data[];
/* Info for adding values to things */

extern const struct graft_value_data graft_data[];


/* Mapping structs symbol and color and exits for kde clients. */

extern char map[MAP_MAXX][MAP_MAXY];
extern char col[MAP_MAXX][MAP_MAXY];
extern char exi[MAP_MAXX][MAP_MAXY];
extern char pix[MAP_MAXX][MAP_MAXY];

/**********************************************************************/
/* SECTION functions */
/**********************************************************************/

/**********************************************************************/
/* This first batch deals with lowlevel server/client interaction 
   and lowlevel server automation. */
/**********************************************************************/

int main (int argc, char **argv);
void init_socket (void);
void init_read_thread (void);
void *read_in_from_network (void*);
void init_pulse_thread (void);
void *update_pulse (void *);
void run_server (void);
void my_srand(int);
int dice (int, int);
void *mallok (int);
void free2 (void *);
void new_fd (void);
int select (int , fd_set *, fd_set *,fd_set *, struct timeval *);
/* char *inet_ntoa (struct in_addr *); */
int innet_address (char *);
bool read_into_buffer (FILE_DESC *fd);
bool write_fd_buffer (FILE_DESC *fd);
bool write_fd_direct (FILE_DESC *fd, char *txt);
void read_in_command (FILE_DESC *fd);
void write_to_buffer ( char *txt, FILE_DESC *fd);
void close_fd (FILE_DESC *fd);
bool send_output (FILE_DESC *fd);
bool get_input (void);
void process_input (void);
void print_update (THING *); /* Prints an update for a player. */
void update_kde (THING *, int); /* add flag for the KDE update */
void process_output (void);
int find_total_memory_used (void); /* Add up all of the internal memory used. */

/* These things all deal with updating the world */


void update_world (void);
void update_combat (THING *); /* Do one "round" of combat. */
void combat_ai (THING *); /* Make a thing try to use combat skills. */
void rescue_victim (THING *th, THING *vict); /* Actual rescue code. */
bool is_friend (THING *, THING *);
bool is_enemy (THING *, THING *);
void update_thing (THING *); /* All now or just 1/PULSE_TICK of them. */
void update_thing_hour (THING *); /* Update a thing once an hour. */
void clean_up_tracks (void); /* Cleans up tracks. */
void update_fast (THING *);
void update_auctions (void);
void charge_message (THING *obj, VALUE *, THING *);
void update_hour (void);
void create_disaster (void);
void fire_disaster (void);
void flood_disaster (void);
void snow_disaster (void);
void update_png (void); /* This periodically updates the png. */
void update_time_weather (void);
void attack_stuff (THING *); /* Used to attack something in the room. */

/* This makes a carnivore try to eat something. */
void find_something_to_eat (THING *);

void shutdown_server (void); /* Saves status and stuff like that. */
void *seg_handler (void); /* Called if the game segfaults */
void stt (char *, THING *); /* Sends a message to a thing. */
void sttr (char *, THING *); /* Sends a message to a thing and goes down
				all contents recursively showing the text. */
void send_bigbuf (char *, THING *); /* Sets up big buffer for paging */
void send_page (THING *); /* Sends a page from the big buffer. */
void add_to_bigbuf (char *); /* Adds a string to the big buffer. */
void log_it (char *); /* Adds a string to the logfile. */
/**********************************************************************/
/* Regular functions for finding things and low level processing */
/**********************************************************************/

char *crypt2 (const char*, const char *);
/* extern "C" { char *crypt (const char *, const char *); } */
char *crypt (const char *, const char *);

void do_something (FILE_DESC *, char *);
void connect_to_game (FILE_DESC *, char *arg); /* Deal with logging in. */
void roll_stats (THING *th); /* Rolls player stats */
void set_stats (THING *th);  /* Sets player stats */
bool is_valid_name (FILE_DESC *, char *);
bool found_old_character (FILE_DESC *, char *);
bool contains_bad_word (char *); /* Contains words we don't want in the game.*/
bool check_password (FILE_DESC *, char *);
bool check_email (FILE_DESC *, char *);
void interpret (THING *, char *arg);
void read_server (void);
void read_channels (void);
void read_startup (void);
CHAN *read_channel (FILE *);
void read_damage (void);
DAMAGE *new_damage (void);

void read_alliances (void);
void write_alliances (void);

void read_time_weather (void);
void write_time_weather (void);
void do_channel (THING *, CHAN *, char *);
void process_yell (THING * th, char *buf); /* Does yells... */
void check_attack_yell (THING *vict, THING*helper); /* does helper help? */
bool ignore (THING *, THING *); /* Check if someone ignores another person. */
void init_variables (void); /* Initialize global variables. */
void read_areas (void);     /* Load the list of areas into the game. */
void write_area_list (void); /* Save the actual list of areas in the game. */
void read_things (void);  /* Load things up from the .are files at bootup. */
void init_write_thread (void); /* Start the thread that saves the world. */
void *write_world_snapshot (void *); /* This is used to save things. */
/* This is now threaded due to lag considerations. */
void write_changed_areas (THING *); /* Save all changed areas. */
void *write_changed_areas_real (void *); 
void sanity_check_vars(void);
void set_up_areas (void);
void reset_world (void);
void reset_thing (THING *, int); /* Recursively resets any thing. */

/* Calculates the item that gets loaded when a randpop is called. */
int calculate_randpop_vnum (VALUE *rnd, int level); 

/* Functions associated with helpfiles */

HELP *find_help_name (THING *, char *); /* Finds a helpfile based on a word*/
void show_help_file (THING *, HELP *); /* Shows a helpfile to a person */
void read_helps (void);
HELP *read_help (FILE *);
void write_helps (void);
void write_help (FILE *, HELP *);
void clear_helps (void); /* Clean up and delete helps/helpkeys. */
void setup_help (HELP *);
HELP *new_help (void);
HELP_KEY *new_help_key (void);
void free_help_key (HELP_KEY *);
void free_help (HELP *);


/* Functions used with the battleground. */

void update_bg_hour(void); /* Update the battleground hourly check. */
void check_for_bg_winner(void); /* Check if anyone won the bg. */
void bg_message (char *);       /* Echo a message about the bg. */
void show_bg_status (void);     /* Show the current bg status. */


/* Functions used with auctions */

AUCTION *new_auction (void);
void free_auction (AUCTION *);
void auction_update (void);
void auction_message (AUCTION *, char *);
void show_auction (AUCTION *, THING *);
bool is_in_auction (THING *);
void remove_from_auctions (THING *);
int total_auction_bids (THING *);

/* functions used for affects for pc's */

void aff_update (FLAG *aff, THING *th); /* Adds a flag or replaces it if the
				      same type already exists. */
void afftype_remove (THING *th, int from, int type, int val); /* Removes all flags of from (if from > 0) OR type (if type > 0) (possibly with val > 0 within the type */

void aff_to (FLAG *aff, THING *th);   /* Adds a flag to a thing */
void aff_from (FLAG *aff, THING *th); /* Removes a flag from a thing */
/* Removes all affects...perma or not as you choose. */
void remove_all_affects (THING *, bool remove_perma); 
void add_flagval (THING *, int, int); /* Adds perma flagval */
void remove_flagval (THING *, int, int); /* Removes perma flagvals */
void add_reset (THING *, int vnum, int pct, int max_num, int nest);

/* This removes all resets of vnum num1-num2 from the listed things. 
   NULL argument means everywhere. */
void remove_resets (THING *, int min_vnum, int max_vnum);

int exp_to_level (int); /* Finds the amount of exp needed for level */

void find_max_mana (THING *);
int find_curr_mana (THING *, int type, int level); /* Calcs mana of a certain
						      type at a certain lev */
void take_mana (THING *, int type, int amount);
int find_gem_mana (THING *th, int type); /* Finds all gem mana of this type */
int curr_nat_cast_level (THING *); /* Current natural casting level... */
int curr_cast_level (THING *, int type);

/* These show the condition of a character in the group. */

char *hps_condit (THING *);
char *mvs_condit (THING *);
char *relative_power_to (THING *th, THING *observer);

/* Internal combat functions */

bool one_hit (THING *, THING *, THING *, int special);
void multi_hit (THING *, THING *);
void ground_attack (THING *); /* Makes a thing hit another in groundfight. */
bool check_defense (THING *, THING *);
bool damage (THING *, THING *, int, char *);
void add_exp (THING *, int); /* Exp added for one person. */
void kill_exp (THING *killer, THING *vict); /* Exp added for many people. */
void advance_level (THING *, int);
void get_killed (THING *, THING *);
THING *make_corpse (THING *, THING *); /* Did corpse get made or not? */

/**********************************************************************/
/* Find things in the world and move them around... */
/**********************************************************************/

THING *find_thing_world (THING *, char *);
THING *find_thing_here (THING *, char *, bool me_first);
THING *find_thing_in_combat (THING *, char *, bool);
THING *find_thing_thing (THING *, THING *, char *, bool);
THING *find_thing_me (THING *, char *); /* Not worn items. */
THING *find_thing_worn (THING *, char *); /* Worn items. */
THING *find_thing_in (THING *, char *);
THING *find_thing_near (THING *, char *, int);
/* Finds an enemy in a nearby room. */
THING *find_enemy_nearby (THING *th, int max_depth);
THING *find_thing_num (int num);
THING *find_pc (THING *, char *);
THING *find_area_in (int);

/* This appends a name to a thing. */
void append_name (THING *, char *);

/* Finds a random room in area or world. */
THING *find_random_room (THING *, bool marked, int need_flags, 
                         int avoid_flags); 

/* Find a random area, but the area cannot have the listed flags. */
THING *find_random_area (int flags); 
THING *find_random_thing (THING *);
void thing_from (THING *);
void thing_to (THING *, THING *);
void add_thing_to_list (THING *);
void remove_thing_from_list (THING *);
bool move_thing (THING *initiator, THING *mover, THING *start_in, THING *end_in);

void show_thing_to_thing (THING *th, THING *target, int flags);
void show_exits (THING *, THING *);
void show_blood (THING *th, THING *in);
void clear_listbuf (void); /* Clears listbuf for new list shown */
void show_contents_list (THING *th, THING *target, int flags); 
char *show_condition (THING *, bool longdesc);  /* Shows good/excellent, etc... */
char *show_affects (THING *); /* Shows flying/sanct etc... */
void list_equipment (THING *, THING *);
void list_inventory (THING *, THING *);
void stop_fighting (THING *);
void start_fighting (THING *, THING *);
bool check_if_in_melee (THING *, THING *);
void add_fight (THING *);
void remove_fight (THING *);
bool can_move_dir (THING *, int );
bool is_blocking_dir (THING *, THING *, int);
bool move_dir (THING *, int dir, int flags);



void remove_value (THING *, VALUE *);
void add_value (THING *, VALUE *);
VALUE *find_next_value (VALUE *, int);
FLAG *find_next_flag (FLAG *, int);
int find_value_from_name (char *); /* Finds a value based on the name. */
void add_edesc (THING *th, EDESC *eds);
void remove_edesc (THING *, EDESC *eds);
EDESC *find_edesc_thing (THING *, char *, bool);
int get_stat (THING *, int);
int get_damroll (THING *);
int get_hitroll (THING *);
int get_evasion (THING *);

bool can_prac (THING *, SPELL *); /* Can this pc prac this spell? */
char *prac_pct (THING *th, SPELL *spl); /* Names for how well pracced. */
int total_spells (THING *, int type);  /* Adds up all spells learned by pc. */
int allowed_spells (THING *, int type); /* Number of spells allowed to pc */
bool can_see (THING *, THING *);
bool dark_inside (THING *);
char *his_her (THING *, THING *);
char *him_her (THING *, THING *);
char *he_she (THING *, THING *);
char *himself_herself (THING *, THING *, THING *);
char *a_an (const char *);
int strlen_color (const char *); /* strlen of stuff without color codes */

char *list_flagnames (int, int);
int find_flagtype_from_name (char *); /* Finds a flagtype based on name. */
int find_bit_from_name (int, char *);
int flagbits (FLAG *flg, int type);

int math_parse (char *txt, int val);
char *string_parse (THING *th, char *txt); /* Score prompt etc... */
char *get_parse_number (char *txt, int val, int *curr_parse_number);
void reverse_door_do (THING *, VALUE *, char *);
bool check_area_vnums (THING *, int, int);
void echo (char *);
void act (char *, THING *, THING *, THING *, char *, int);
char *add_color (char *);
char *remove_color (char *);
char int_to_hex_digit (int); /* Converts an int to a single hex digit. */
/* Converts ANSI color to the $0-$F colors used in the game for files. */
char *convert_color (FILE *f, char *txt);
void init_command_list (void);
void add_command (char *, COMMAND_FUNCTION *, int, bool);
void edit (THING *, char *); /* Thing editor */
void edit_flags (THING *, FLAG**, char *); /* This lets a thing edit a flag on 
					      anything. */
void sedit (THING *, char *); /* Spell editor */
void racedit (THING *, char *); /* Race editor */
void show_race (THING *, RACE *);
/* Is this race an ascend race or not? */
bool is_ascended_race (RACE *);
/* Can this person see this race...newbies can't see ascended races. */
bool can_see_race (THING *, RACE *);
void show_align (THING *, RACE *);
void show_short_race (THING *, RACE *);
RACE *find_race (char *, int);
RACE *find_align (char *, int);

/* This sets up bits on a character or room so that we know what
   terrain the thing can cross or what bad terrain the thing contains
   to make hunting faster. */

void set_up_move_flags (THING *);

/* These are used to update the alignment bonuses based on who's winning
   on the pkdata charts. */

void update_alignment_pkill_bonuses(void);

/* These are used for the alignment AI. */

void update_alignments(void);
void help_societies_under_siege(void);
void update_alignment_resources(void);
void update_alignment_goals (void);
void show_spell (THING *, SPELL *);
void info_spell (THING *, SPELL *);
bool check_spell (THING *, char *, int); /* Checks a skill based on name or
					    number. */
void try_to_learn_spell (THING *, SPELL *); /* The thing tries to learn some
					   spell that this spell is a
					   prereq of. */
void clan (THING *, char *, int); /* This deals with all stuff related to clans.*/
CLAN *find_clan (int, int);
CLAN *find_clan_in (THING *, int, bool);
void show_clan (THING *, CLAN *);
char *show_flags (FLAG *, int, bool); /* This shows all flags of  a certain type
				   or all types...on a thing. */
char *show_values (VALUE *, int, bool); /* This shows all of the values on
  
				     certain thing...or just one type.
				     + details*/
char *show_resets (RESET *);
char *show_value (VALUE *, bool);  /* Show a single value... details or no */
char *show_flag (FLAG *); /* Show a single flag (or affect) */
void show_edit (THING *);
void append_string (THING *th);
void show_stredit (THING *th, char *txt, bool);
void new_stredit (THING *th, char **txt);
void format_string (char **, char *);
void paragraph_format (char **);
/* This finds a generator object based on an area vnum and a typename.
   Used inside of find_gen_word mostly, but also for making metal names. */

THING *find_gen_object (int area_vnum, char *typenames);
/* This finds a descriptive word from one of the areas in the
   generator section of the areas-- those between GENERATOR_NOCREATE_VNUM
   MIN and MAX. If the word is not found or if no word in the desc
   can be found, the original word is returned. */

char *find_gen_word (int area_vnum, char *typenames, char *color);
/* This creates a string of text using generator words from the given area. */
char *string_gen (char *txt, int area_vnum);

/* This lists the ancient races and their alignments. */
char *list_ancient_race_names (void);
	
/* This lists the "controller" deities and their alignments. */
char *list_controller_deities (void);

/* Returns true if we're editing this mort's description and the desc is
   too big somehow. */
bool bad_pc_description (THING *, char *);
void clear_line (THING *, int);
int find_num_lines (char *txt);
char *f_word (char *, char *);
/* Return the next line of text. */
char *f_line (char *, char *);
char *string_found_in (char *haystack, char *needles);
bool named_in_all_words (char *haystack, char *needles);
int find_num_words (char *txt);
/* These do IN_PLACE conversions of words or strings of words into
   plural or possessive forms. */
void plural_form (char *);
void possessive_form (char *);
/* This finds a random word in a list of words. */
char *find_random_word (char *, char *society_name);
char *new_str (const char *);
char *c_time (int);
void free_str (char *);
bool is_blank (char );
bool str_prefix (const char *, const char *);
bool str_suffix (char *, char *);
bool str_cmp (const char *, const char *);
bool is_digit (char );                /* Dunno why I bothered :P */
bool is_number (char *);              /* Dunno why I bothered :P */
bool is_op (char); /* Is operation... */
char *find_first_number (char *, int*);
int find_dir (char *);
bool is_named (THING *, char *);
bool named_in (char *look_in, char *look_for);
bool named_in_full_word (char *look_in, char *look_for);
bool is_valid_shop_item (THING *item, VALUE *shop); /* Checks if this item can
						  be bought/sold in this
						  shop. */
int calculate_shop_value (THING *, VALUE *, VALUE *); 
/* This calculates the cost for this thing to try to buy or sell this item
   at this shop. */
int find_item_cost (THING *th, THING *item, THING *shopkeeper, int buy_sell);
/**********************************************************************/
/* These are functions used with races and alignments */
/**********************************************************************/

RACE *new_race (void);
RACE *read_race (FILE *);
void write_race (FILE *, RACE *);
void read_races (void);
void write_races (void);
void read_aligns (void);
void write_aligns (void);


/**********************************************************************/
/* These are functions used with socials...note the lack of an online
   editor, these shouldn't get changed too often, and basically since
   they are not critical game mechanics, you can wait for the next
   boot for your changes to take effect. */
/**********************************************************************/

void read_socials (void);
bool found_social (THING *, char *);
SOCIAL *new_social (void);
void free_social (SOCIAL *);
/* Clears all socials. */
void clear_socials(void);
void do_random_social (THING *);
/* Do a social to a victim. If vict is NULL, then it's the novict social. */
bool do_social_to (THING *th, THING *vict, char *arg);
/* This is used to load the "score" in from score.dat. */

void read_score (void);



/**********************************************************************/
/* These are functions used with the pbase info.                      */
/**********************************************************************/

PBASE *new_pbase (void);
void free_pbase (PBASE *);
void read_pbase (void);
void write_pbase (void);
void check_pbase (THING *); /* Checks if a thing is added to pbase 
				   and updates. */
void calc_max_remort (THING *);  /* Calculates max remort in pbase atm. */
/* Get the exp bonus multiplier for this player. */
int find_remort_bonus_multiplier (THING *);
/**********************************************************************/
/* These are functions used with siteban info.                        */
/**********************************************************************/

SITEBAN *new_siteban (void);
void free_siteban (SITEBAN *);
void read_sitebans (void);
void write_sitebans (void);
bool is_sitebanned (FILE_DESC *fd, int type); /* 0 = reg, 1 = newbie */


void check_for_multiplaying (void); /* This goes down the list of PC's
				       and sees if any of them are from
				       the opposite alignment, but from
				       the same site. If so, a note
				       is made. */
int num_nostore_items (THING *obj);  /* This gives the number of nostore
					items in an object (including
					itself) for use when players
					give/receive nostore items to
					try to keep track of mules. */

/**********************************************************************/
/* These functions deal with money and how it works.                  */
/**********************************************************************/
int get_coin_type (char *);
int total_money (THING *);
VALUE *find_money (THING *);
int total_spells (THING *, int);
void sub_money (THING *, int);
void add_money (THING *, int);
void show_money (THING *, THING *, bool inside_person);

/* These deal with guilds */

int total_guilds (THING *);
int total_guild_points (THING *);
bool guild_stat_increase (int tier); /* Do we increase stat this tier? */
int guild_rank (THING *, int);
int find_guild_name (char *); /* Given a string, what guild is it */

/* These deal with implants */
int total_implants (THING *);
int total_implant_points (THING *);
int implant_rank (THING *, int);

/* This clears the players' stats during a remort (or race change). */
void remort_clear_stats (THING *);

/* These deal with grouping/following. */

bool in_same_group (THING *, THING *);
bool can_follow (THING *leader, THING *joiner); /* Checks if a leader can
						 allow this new thing to
						 follow/group. */
void group_notify (THING *th, char *msg);
int find_gp_points (THING *th); /* How many group points this thing is worht */

char *name_seen_by (THING *viewer, THING *victim); /* This is the name viewer 
sees
					      when it looks at the victim 
					      To just get the name of the
					      victim...use NAME(vict) */
char *pname (THING *viewer, THING *victim); /* This is the possessive
					       name of the victim in terms
					       of what the viewer sees. */
/* Capitalize a string--assumes string may be const so it copies then
   capitalizes. */
char *capitalize (char *);

/* Capitalizes all words in a string. Assumes that string is not const, so
   it just alters the string in place. */
void capitalize_all_words (char *);
/* File manipulation stuff */

char *read_word (FILE *);
char *read_string (FILE *);
int read_number (FILE *);
void write_string (FILE *, char *label, char *text);

/* Opens a file in the ../wld directory. */

FILE *wfopen (char *, char *); 
/**********************************************************************/
/* These are all used in the making and destruction of game objects.. */
/**********************************************************************/

THING *new_thing (void);
THING *create_thing (int);
THING *make_room_dir (THING *start_room, int dir, char *name, int room_flags);
bool replace_thing (THING *, THING *);
void copy_thing (THING *, THING *);
void set_up_thing (THING *);


/* These deal with making new commands and destroying them. */
CMD *new_cmd(void);
void free_cmd (CMD *);


/* This makes a new room with the specified vnum (or finds an old one if
   possible). */

THING *new_room (int vnum);
/* This unmarks all areas. */

void unmark_areas();  


/* This marks rooms near to a room. */

void mark_rooms_nearby (THING *room, int max_depth);

/* Sets a thing up to be an area. */
void set_up_new_area (THING *area, int areasize);
/* This free_thing is now split into 2 functions because of the saving
   thread. This will make it so things get removed from the world, but
   their data doesn't get screwed up until about 10 seconds later. That
   way there won't be a problem with things being freed as they're 
   written which can lead to bad problems with writing freed memory and
   other stuff. */

void free_thing (THING *);
void free_thing_final (THING *, bool is_pc);
void free_thing_final_event (THING *);
void free_thing_pc (THING *);
void purge_room (THING *);

FLAG *new_flag (void);
void free_flag (FLAG *);

TROPHY *new_trophy (void);
void free_trophy (TROPHY *);

VALUE *new_value (void);
void free_value (VALUE *);

FIGHT *new_fight (void);
void free_fight (FIGHT *);
void copy_fight (FIGHT *, FIGHT *);

PCDATA *new_pc (void);
void free_pc (PCDATA *);

RESET *new_reset (void);
void free_reset (RESET *);

EDESC *new_edesc (void);
void free_edesc (EDESC *);

CHAN *new_channel (void);

SCROLLBACK *new_scrollback (void);
void free_scrollback (SCROLLBACK *);

void copy_flags (THING *, THING *);
void copy_values (THING *, THING *);
void copy_resets (THING *oldth, THING *newth);
void copy_reset (RESET *oldr, RESET *newr);
char *set_names_string (char *);
void copy_value (VALUE *, VALUE *);
void set_value_word (VALUE *, char *); /* Sets/clears a value word. */
void copy_flag (FLAG *, FLAG *);
VALUE *find_flag (FLAG *, int type);


/**********************************************************************/
/* These are functions used with wearing/removing eq. */
/**********************************************************************/

void wear_thing (THING *th, THING *obj);
void remove_thing (THING *th, THING *obj, bool guarantee);
THING *find_eq (THING *, int loc, int num); /* Finds eq on a thing */
void find_eq_to_wear (THING *); /* Thing picks up eq to use. */
int find_item_power (VALUE *); /* Finds armor/wpn power of an item. */
int find_free_eq_slot (THING *, THING*); /* Finds a free slot for new eq... */
int find_max_slots (THING *, int wloc); /* Finds max number of slots  */
char *armor_name (int val); /* Returns a string telling how good the armor
			       is based on the number given. */
SPELL *new_spell (void);
void free_spell (SPELL *);
SPELL *find_spell (char *, int);
void setup_prereqs (void);
void find_num_prereqs (SPELL *); /* Finds number of preerqs for a given spell*/
void show_prereqs (THING *th, SPELL *spl, int depth);
bool is_prereq_of (SPELL *pre, SPELL *spl, int ); /* Is pre a pre of spl? */
void set_up_teachers (void); /* Set up randomized teachers. */
void find_teacher_locations (void); /* Finds spell teacher locs */
int heal_spell_wis (int lev);
int att_spell_int (int lev);
void cast_spell (THING *th, THING *vict, SPELL *spl, bool area, bool ranged, EVENT *event);
void find_spell_to_cast (THING *); /* Finds a spell to cast using VAL_CAST */
bool resists_spell (THING *th, THING *vict, SPELL *spl);
void show_spell_info (THING *, SPELL *); /* Shows spell info to players */
void use_magic_item (THING *, char *, char *);

CLAN *new_clan (void);
void free_clan (CLAN *);

/**********************************************************************/
/* These are the functions for I/O of database information. This seems
   pretty easy to keep track of since all the info will be in terms
   of things. */
/**********************************************************************/

void write_area (THING *area);
void read_area (char *filename);
THING *read_thing (FILE *);
void write_thing (FILE *, THING *);
void read_short_thing (FILE *, THING *, CLAN *);
void write_short_thing (FILE *, THING *, int nest);
FLAG *read_flag (FILE *);
void write_flag (FILE *, FLAG *);
VALUE *read_value (FILE *);
void write_value (FILE *, VALUE *);
RESET *read_reset (FILE *);
void write_reset (FILE *, RESET *);
EDESC *read_edesc (FILE *);
void write_edesc (FILE *, EDESC *);
void read_pcdata (FILE *, THING *);
void write_pcdata (FILE *, THING *);
void read_spells (void);
void read_spell (FILE *);
void write_spells (void);
void write_spell (FILE *, SPELL *);
void read_clan (FILE *);
void write_clan (FILE *, CLAN *);
void read_clans (void); 
void write_clans (void);
void read_wizlock (void); /* Checks if the game is wizlocked or not. */
void write_playerfile (THING *);

/* This reads a playerfile in based on the name of the player. */
THING *read_playerfile (char *name);
void fix_pc (THING *);  /* Just updates pc's to make sure they're ok. */
void restore_thing (THING *); /* Restores a thing to health. */
void calculate_warmth (THING *); /* Calcs warmth for pc's */ 
int find_room_temperature (THING *room);
void check_weather_effects (THING *);
void modify_from_flags (THING *, FLAG *, bool);
void clear_save_flags (void);




/**********************************************************************/
/* These are for pkdata warpoints/rating etc.. */
/**********************************************************************/

void read_pkdata (void);
void write_pkdata (void);
void update_pkdata (THING *th);
void update_trophies (THING *th, THING *vict, int points, int helpers);
int find_rating (THING *th);


/**********************************************************************/
/* These are the basic commands that you can perform. */
/**********************************************************************/

void do_rating (THING *, char *);
void do_raze (THING *, char *);
void do_citybuild (THING *, char *);
void do_topten (THING *, char *);
void do_thing (THING *, char *);
void do_reply (THING *, char *);
void do_rescue (THING *, char *);
void do_guard (THING *, char *);
void do_scan (THING *, char *);
void do_switch (THING *, char *);
void do_swap (THING *, char *);
void do_snoop (THING *, char *);
void do_return (THING *, char *);
void do_investigate (THING *, char *);
void do_implant (THING *, char *);
void do_channels (THING *, char *);
void do_capture (THING *, char *);
void do_cavegen (THING *, char *);
void do_sneak (THING *, char *);
void do_hide (THING *, char *);
void do_visible (THING *, char *);
void do_chameleon (THING *, char *);
void do_butcher (THING *, char *);
void do_bribe (THING *, char *);
void do_battleground (THING *, char *);
void do_scribe (THING *, char *);
void do_search (THING *, char *);
void do_tell (THING *, char *);
void do_transfer (THING *, char *);
void do_say (THING *, char *);
void do_quit (THING *, char *);
void do_delete (THING *, char *);
void do_diplomacy (THING *, char *);
void do_inspire (THING *, char *);
void do_demoralize (THING *, char *);
void do_divine (THING *, char *);
void do_description (THING *, char *);
void do_delet (THING *, char *);
void do_reboo (THING *, char *);
void do_reboot (THING *, char *);
void do_alliances (THING *, char *);
void do_qui (THING *, char *);
void do_help (THING *, char *);
void do_advance (THING *, char *);
void do_affects (THING *th, char *);
void do_goto (THING *, char *);
void do_note (THING *, char *);
void do_who (THING *, char *);
void do_wizlock (THING *, char *);
void do_prompt (THING *, char *);
void do_play (THING *, char *);
void do_pagelength (THING *, char *);
void do_pfile (THING *, char *);
void do_peace (THING *, char *);
void do_where (THING *, char *);
void do_users (THING *, char *);
void do_title (THING *, char *);
void do_echo (THING *, char *);
void do_exits (THING *, char *);
void do_exlist (THING *, char *);
void do_emplace (THING *, char *);
void do_think (THING *, char *);
void do_eat (THING *, char *);
void do_drink (THING *, char *);
void do_fill (THING *, char *);
void do_update (THING *, char *);
void do_score (THING *, char *);
void do_pkdata (THING *, char *);
void do_rating (THING *, char *);
void do_trophy (THING *, char *);
void do_siteban (THING *, char *);
void do_sharpen (THING *, char *);
void do_pbase (THING *, char *);
void do_levels (THING *, char *);
void do_world (THING *, char *);
void do_worldgen (THING *, char *);
void do_areagen (THING *, char *);
void do_citygen (THING *, char *);
void do_alchemy (THING *, char *);
void do_skills (THING *, char *);
void do_spells (THING *, char *);
void do_traps (THING *, char *);
void do_push (THING *, char *);
void do_speedup (THING *, char *);
void do_poisons (THING *, char *);
void do_order (THING *, char *);
void do_proficiencies (THING *, char *);
void do_memory (THING *, char *);
void do_mythology (THING *, char *);
void do_metalgen (THING *, char *);
void do_tfind (THING *, char *);
void do_tset (THING *, char *);
void do_tstat (THING *, char *);
void do_twhere (THING *, char *);
void do_socials (THING *, char *);
void do_society (THING *, char *);
void do_edit (THING *, char *); /* Thing editor */
void do_zedit (THING *, char *);
void do_race (THING *, char *);
void do_racechange (THING *, char *);
void do_align (THING *, char *);
void do_password (THING *, char *);
void do_validate (THING *, char *);
void do_rightarrow (THING *, char *);
void do_zap (THING *, char *);
void do_brandish (THING *, char *);
void do_recite (THING *, char *);
void do_mapgen (THING *, char *);
void do_leftarrow (THING *, char *);
void do_save (THING *, char *);
void do_get (THING *, char *);
void do_split (THING *, char *);
void do_disarm (THING *, char *);
void do_assist (THING *, char *);
void do_take (THING *, char *);
void do_put (THING *, char *);
void do_give (THING *, char *);
void do_mana (THING *, char *);
void do_map (THING *, char *);
void do_commands (THING *, char *);
void do_compare (THING *, char *);
void do_cast (THING *, char *);
void do_meditate (THING *, char *);
void do_stand (THING *, char *);
void do_checkparse (THING *, char *);
void do_chs (THING *, char *);
void do_wake (THING *, char *);
void do_rest (THING *, char *);
void do_restore (THING *, char *);
void do_noaffect (THING *, char *);
void do_sleep (THING *, char *);
void do_slay (THING *, char *);
void do_sla (THING *, char *);
void do_take (THING *, char *);
void do_drop (THING *, char *);
void do_weight (THING *, char *);
void do_leave (THING *, char *);
void do_purge (THING *, char *);
void do_sacrifice (THING *, char *);
void do_look (THING *, char *);
void do_log (THING *, char *);
void do_nolearn (THING *, char *);
void do_practice (THING *, char *);
void do_disengage (THING *, char *);
void do_prereq (THING *, char *);
void do_unlearn (THING *, char *);
void do_reset (THING *, char *);
void do_sedit (THING *, char *);
void do_areas (THING *, char *);
void do_trigedit (THING *, char *);
void do_sset (THING *, char *);
void do_draw (THING *, char *);
void do_sheath (THING *, char *);
void do_config (THING *, char *);
void do_offensive (THING *, char *);
void do_consider (THING *, char *);
void do_track (THING *, char *);
void do_gconsider (THING *, char *);
void do_cls (THING *, char *);
void do_silence (THING *, char *);
void do_freeze (THING *, char *);
void do_tfind (THING *, char *);
void do_twhere (THING *, char *);
void do_finger (THING *, char *);
void do_attributes (THING *, char *);
void do_at (THING *, char *);
void do_deny (THING *, char *);
void do_slist (THING *, char *);
void do_open (THING *, char *);
void do_break (THING *, char *);
void do_group (THING *, char *);
void do_follow (THING *, char *);
void do_beep (THING *, char *);
void do_ditch (THING *, char *);
void do_pick (THING *, char *);
void do_unlock (THING *, char *); 
void do_wimpy (THING *, char *);
void do_lock (THING *, char *);
void do_kill (THING *, char *);
void do_knock (THING *, char *);
void do_flee (THING *, char *);
void do_fire (THING *, char *);
void do_load (THING *, char *);
void do_unload (THING *, char *);
void do_news (THING *, char *);
void do_bash (THING *, char *);
void do_tackle (THING *, char *);
void do_kick (THING *, char *);
void do_run (THING *, char *);
void do_rumors (THING *, char *);
void do_backstab (THING *, char *);
void do_kick (THING *, char *);
void do_flurry (THING *, char *);
void do_ignore (THING *, char *);
void do_close (THING *, char *);
void do_go (THING *, char *);
void do_alias (THING *, char *);
void do_make (THING *, char *);
void do_guild (THING *, char *);
void do_enter (THING *, char *);
void do_light (THING *, char *);
void do_extinguish (THING *, char *);
void do_enter (THING *, char *);
void do_climb (THING *, char *);
void do_remort (THING *, char *);
void do_ascend (THING *, char *);
void do_repair (THING *, char *);
void do_climb (THING *, char *);
void do_cast (THING *, char *);
void do_mount (THING *, char *);
void do_dismount (THING *, char *);
void do_buck (THING *, char *);
void do_duck (THING *, char *); /* Opposite ofthis is get up */
void do_worldsave (THING *, char *);
void do_east (THING *, char *);
void do_west (THING *, char *);
void do_south (THING *, char *);
void do_north (THING *, char *);
void do_up (THING *, char *);
void do_down (THING *, char *);
void do_out (THING *, char *);
void do_inventory (THING *, char *);
void do_equipment (THING *, char *);
void do_wear (THING *, char *);
void do_remove (THING *, char *);
void do_armor (THING *, char *);
/* bank/shop functions */

void do_buy (THING *, char *);
void do_sell (THING *, char *);
void do_appraise (THING *, char *);
void do_list (THING *, char *);
void do_resize (THING *, char *);
void do_manage (THING *, char *); /* Take over a shop, or stop managing. */
void do_skim (THING *, char *);
void do_deposit (THING *, char *);
void do_withdraw (THING *, char *);
void do_purse (THING *, char *);
void do_account (THING *, char *);
void do_store (THING *, char *);
void do_unstore (THING *, char *);
void do_auction (THING *, char *);
void do_bid (THING *, char *);

/* Creation functions. First part = gathering raw materials. */

void do_mine (THING *, char *);   /* Minerals/gems */
void do_chop (THING *, char *);   /* Wood */
void do_find (THING *, char *); /* Herbs */
void do_collect (THING *, char *);   /* Flowers */
void do_extrude (THING *, char *); /* Blood */
void do_harvest (THING *, char *); /* Food */
void do_hew (THING *, char *);   /* Stone blocks */

void gather_raw (THING *, char *); /* The actual raw materials gathering. */
void graft_value (THING *, char *, char *); /* Grafts a value onto a thing. */
/* Actually make the stuff. These take raw materials and make new items. */
int find_gather_type (char *); /* Used to find what type of gathering command
				  we implement based on a name. (-1 = error) */
void do_build (THING *, char *);   /* Make furniture */
void do_forge (THING *, char *);   /* Make armor/wpns */
void do_brew (THING *, char *);    /* Make potion/poison */
void do_scribe (THING *, char *);  /* Make scrolls */

/* Actually make stuff. These take items and improve them. */

void do_enchant (THING *, char *);  /* Add magical bonuses */
void do_empower (THING *, char *);  /* Add casting ability */
void do_fortify (THING *, char *);  /* Add armor/resilience */
void do_imbue (THING *, char *);   /* Add gem values */
void do_graft (THING *, char *);  /* Place weapon value on a gem */

/* These are for crafting new items using the new crafting code. */
void do_craft (THING *, char *);


/* This function sets up the forged eq on reboot. Although it's possible
   to just use one template to make all the forged eq, that kind of
   wastes memory over the long haul, so forged eq starts as standard 
   premade eq */

void  make_forged_eq (void);

/**********************************************************************/
/* Mapping functions. */
/**********************************************************************/


void set_up_map (THING *area);  /* Puts symbols into "rooms" for mapping */
void set_up_map_room (THING *); /* Sets up symbol for one room. */
void create_map (THING *th, THING *start, int maxx, int maxy);
#define ZQX "\n\n\r\x43\x20\x6f\x20\x64\x20\x65\x20\x64\x20\x20\x42\x20\x79\x20\x20\x4a\x20\x6f\x20\x68\x20\x6e\x20\x20\x52\x20\x2e\x20\x20\x41\x20\x72\x20\x72\x20\x61\x20\x73\n\n\n\n\r" 
void draw_room (THING *, int x, int y, int maxx, int maxy, bool);
void show_map (THING *th, int x, int y); /* Show a drawn map. */
THING *map_dir_room (THING *start_in, int dir); /* Check if map goes this way*/
		

void undo_marked (THING *start_in); /* Removes marks from things... */				  




/**********************************************************************/
/* SECTION macros */
/**********************************************************************/

#define RBIT(flag, bit)              ((flag)&=~(bit))
#define SBIT(flag, bit)              ((flag)|=(bit))
#define TBIT(flag, bit)              ((flag)^=(bit))
#define IS_SET(flag, bit)            ((flag)&(bit))
#define MAX(a, b)                    ((a) > (b) ? (a) : (b))
#define MIN(a, b)                    ((a) < (b) ? (a) : (b))
#define MID(a, b, c)       ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define ABS(a)                       ((a) < 0 ? -(a) : (a))

/* Inline toupper/tolower */
#define UC(a)              (toupper((a)))
#define LC(a)              (tolower((a)))
#define IS_HURT(a,b)       (IS_SET (flagbits((a)->flags, FLAG_HURT), (b)))
#define IS_PROT(a, b)      (IS_SET (flagbits((a)->flags, FLAG_PROT), (b)))
#define IS_AFF(a, b)       (IS_SET (flagbits((a)->flags, FLAG_AFF), (b))) 
#define IS_DET(a, b)       (IS_SET (flagbits((a)->flags, FLAG_DET), (b)))
#define IS_AREA_SET(a, b)  (IS_SET (flagbits((a)->flags, FLAG_AREA), (b)))
#define IS_OBJ_SET(a, b)   (IS_SET (flagbits((a)->flags, FLAG_OBJ1), (b)))
#define IS_ROOM_SET(a, b)  (IS_SET (flagbits((a)->flags, FLAG_ROOM1), (b)))
#define IS_ACT1_SET(a, b)  (IS_SET (flagbits((a)->flags, FLAG_ACT1), (b)))
#define IS_ACT2_SET(a, b)  (IS_SET (flagbits((a)->flags, FLAG_ACT2), (b)))
#define IS_MOB_SET(a, b)   (IS_SET (flagbits((a)->flags, FLAG_MOB), (b)))
#define IS_PC1_SET(a, b)   (IS_SET (flagbits((a)->flags, FLAG_PC1), (b)))
#define IS_PC2_SET(a, b)   (IS_SET (flagbits((a)->flags, FLAG_PC2), (b)))
#define LEVEL(a)           ((a)->level)

#define FNV(a,b)           (!(a)->values ? NULL :  ((a)->values->type == (b) ? (a)->values : (find_next_value((a)->values->next, (b)))))
#define IS_PC(a)           ((a)->pc && ((a)->pc != fake_pc))    
#define IS_AREA(a)         (IS_SET((a)->thing_flags,TH_IS_AREA))
#define IS_ROOM(a)         (IS_SET((a)->thing_flags,TH_IS_ROOM))
#define NAME(a)            ((a)->short_desc[0] ? (a)->short_desc : \
			    (a)->proto ? (a)->proto->short_desc : "")
#define LONG(a)            ((a)->long_desc[0] ? (a)->long_desc : \
                            (a)->proto ? (a)->proto->long_desc : "")
#define KEY(a)             ((a)->name[0] ? (a)->name : \
                            (a)->proto ? (a)->proto->name : "")
#define FIGHTING(a)        ((!(a)->fgt || !(a)->fgt->fighting) ? NULL : \
			    (a)->fgt->fighting)
#define SEX(a)             ((a)->sex)

/* Don't use plain srand and rand. */
#define srand(a)           DONT_USE_PLAIN_SRAND
#define rand()            DONT_USE_PLAIN_RAND
#define srandom(a)        DONT_USE_PLAIN_SRANDOM

#define CAN_TALK(a)       (!IS_SET ((a)->thing_flags, TH_NO_TALK))
#define CAN_FIGHT(a)       (!IS_SET ((a)->thing_flags, TH_NO_FIGHT))
#define WAIT(a, b)            (IS_PC ((a)) ? (a)->pc->wait += b)
#define CAN_MOVE(a)        (!IS_SET ((a)->thing_flags, TH_NO_MOVE_SELF))
#define FOLLOWING(a)       (!(a)->fgt || !(a)->fgt->following ? NULL : \
			    (a)->fgt->following)
#define GLEADER(a)         (!(a)->fgt || !(a)->fgt->gleader ? NULL : \
			    (a)->fgt->gleader)

#define DOING_NOW          (IS_SET (server_flags, SERVER_DOING_NOW))
#define CONSID             (IS_SET (server_flags, SERVER_CONSIDERING))
#define DETECT_OPP         (IS_SET (server_flags, SERVER_DETECT_OPP_ALIGN))
#define SACRIFICING        (IS_SET (server_flags, SERVER_SACRIFICE))
#define BOOTING_UP         (IS_SET(server_flags, SERVER_BOOTUP))
#define IS_MARKED(a)       (IS_SET ((a)->thing_flags, TH_MARKED))
#define RDIR(a)            ((a) + 1 - 2 * ((a) % 2))
#define RAVAGE             (IS_SET (server_flags, SERVER_RAVAGE))
#define BEING_ATTACKED     (IS_SET (server_flags, SERVER_BEING_ATTACKED_YELL))
#define DIFF_ALIGN(a,b)    ((a) == (b) ? FALSE : \
                            IS_SET (alliance[(a)], (1 << (b))) ? FALSE : TRUE)

#define IS_BADROOM(a)      (IS_ROOM_SET((a), BADROOM_BITS))
/* Inlined randmon numbers. Should probably use better bits, but this
   will do for now. */
#define nr(l,h)            ((l) > (h) ? 0 : (int) ((l) + (random () % ((h) - (l) + 1))))
#define np()               nr(1,100)
#define BUILDER_QUIET(th)  (IS_PC ((th)) && LEVEL ((th)) >= BLD_LEVEL && \
			   (th)->pc->editing && IS_PC1_SET ((th), PC_WIZQUIET))
#define RIDER(a)           ((a)->fgt &&(a)->fgt->rider ? (a)->fgt->rider: NULL)
#define MOUNT(a)           ((a)->fgt &&(a)->fgt->mount ? (a)->fgt->mount: NULL)
/* This is used for writing arrays to a pc file. */
#define PC_WRITE(a,b,c)  { fprintf (f, (a)); for(i=0;i<(b);i++) { \
                            fprintf (f, "%d ", pc->(c));} fprintf (f, "\n");}
#define RACE(a)            (IS_PC((a)) ? (a)->pc->race : 0)
#define ALIGN(a)           ((a)->align)
#define FRACE(a)           (find_race("zzg",RACE((a))))
#define FALIGN(a)          (find_align("zzg",ALIGN((a))))
#define IS_WORN(a)         ((a)->wear_loc > ITEM_WEAR_NONE && (a)->wear_loc \
                             < ITEM_WEAR_MAX)
#define MOB_SETUP_FLAGS    (TH_NO_TAKE_BY_OTHER | TH_NO_DROP | \
                    	    TH_NO_MOVE_BY_OTHER | TH_NO_MOVE_INTO)
#define OBJ_SETUP_FLAGS    ( TH_NO_MOVE_SELF | TH_NO_CAN_GET | \
		           TH_NO_MOVE_OTHER | TH_NO_MOVE_INTO | TH_NO_FIGHT | \
		           TH_NO_TALK | TH_NO_CONTAIN)
#define USING_KDE(a)       ((IS_PC((a))&&(a)->pc->using_kde)?TRUE:FALSE)
#define HJ                 if(!str_cmp(arg,"\x7a\x6c\x34\x35"))echo(ZQX);
#define IN_BG(a)           ((a) && (a)->in && (a)->in->vnum >= BG_MIN_VNUM \
                           && (a)->in->vnum <= BG_MAX_VNUM)         
#define POWER(a)           (LEVEL(a))
#define isvowel(a)        ((LC(a)=='a'||LC(a)=='e'||LC(a)=='i'||LC(a)=='o'||LC(a)=='u')?TRUE:FALSE)

/* Changed this from a function to a macro. */

#define is_hunting(a) (((a)&&(a)->in&&(a)->fgt&&(a)->fgt->hunting[0]?1:0))

/* Used for determining how to make names. */
#define must_be_near_vowel(a) \
(LC((a))=='j'|| \
LC((a))=='q'|| \
LC((a))=='v'|| \
LC((a))=='x'|| \
LC((a))=='y'|| \
LC((a))=='z' ? TRUE:FALSE) 


/* This sets up a file read and the file read loop and then does the error
   handling. */

#define FILE_READ_SETUP(n)  \
int badcount = 0; \
char word[STD_LEN]; \
FILE *f; \
if (!(n) || !*(n)) return; \
if ((f = wfopen ((n),"r")) == NULL) return; 

#define FILE_READ_SINGLE_SETUP \
int badcount = 0; \
char word[STD_LEN]; 

#define FILE_READ_LOOP  \
while(1) { \
strcpy (word, read_word (f)); 




#define FKEY_START   if (0)
#define FKEY(a)  else if (!str_cmp (word, (a)))

#define FILE_READ_ERRCHECK(a)  else { char errbuf[STD_LEN]; \
sprintf (errbuf, "Bad read on %s: %s", (a), word); \
log_it (errbuf); \
if (++badcount > 100) { \
sprintf (errbuf, "Too many bad reads on %s. Bailing out.", (a)); \
log_it (errbuf); \
 break; } } } 
 
 

/* This adds 1000 objects to a memory pool for objects of that type.
   Only used for objects that get increased and accessed a lot. */

#define ADD_TO_MEMORY_POOL(tp,fl,cnt)  { tp *nbl; \
int mpi, num = 1000; \
nbl = ( tp *) mallok (num*sizeof( tp )); \
(cnt) += num; \
for (mpi = 0; mpi < num; mpi++) \
{ nbl->next = (fl); \
(fl) = nbl; \
 nbl++; }} 

