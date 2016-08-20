#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "script.h"
#include "track.h"
#include "rumor.h"
/* Echo stuff */

char no_echo[] = 
{
  -1, -5, 1, '\0', 
};
char yes_echo[] =
{ 
  -1, -4, 1, '\0', 
};


/* These are pieces of information about the stats in the game. */

const char *stat_name[] =
{
  "strength",
  "intelligence",
  "wisdom",
  "dexterity",
  "constitution",
  "luck",
  "charisma",
};

const char *stat_short_name[] =
{
  "str",
  "int",
  "wis",
  "dex", 
  "con",
  "luc",
  "cha",
};

const char *parts_names[PARTS_MAX] =
{
  "head",
  "body",
  "arms",
  "legs", 
};

const char *implant_descs[PARTS_MAX] = 
{
  "Head implants make your magic more powerful, and make you more resistant to magic.",
  "Body implants increase your resistance to physical attacks.",
  "Arm implants help you to attack harder, faster, and more accurately.",
  "Leg implants help you to move faster, kick harder, and maintain your footing in combat.",
};

/* name view_name, part_on, num, flagval The only problem with this
   is that each item can have at most ONE spot where it can work. Also,
   if you remove spots, you must leave blank lines in here so that there
   aren't any gaps since the code assumes that ITEM_WEAR_WHATEVER is in
   line whatever. */
const struct wear_loc_data wear_data[] =
{
  {"none"       , "<none             >  ", 0        , 0, ITEM_WEAR_NONE},
  {"floating"   , "<floating nearby  >  ", PART_HEAD, 2, ITEM_WEAR_FLOAT},
  {"head"       , "<worn on head     >  ", PART_HEAD, 1, ITEM_WEAR_HEAD},
  {"eyes"       , "<worn on eyes     >  ", PART_HEAD, 1, ITEM_WEAR_EYES},
  {"ears"       , "<worn on ear      >  ", PART_HEAD, 2, ITEM_WEAR_EARS},
  {"face"       , "<worn on face     >  ", PART_HEAD, 1, ITEM_WEAR_FACE},
  {"neck"       , "<worn around neck >  ", PART_HEAD, 2, ITEM_WEAR_NECK},
  {"body"       , "<worn on body     >  ", PART_BODY, 1, ITEM_WEAR_BODY},
  {"chest"      , "<worn on chest    >  ", PART_BODY, 4, ITEM_WEAR_CHEST},
  {"about"      , "<worn about body  >  ", PART_BODY, 1, ITEM_WEAR_ABOUT},
  {"shoulder"   , "<worn on shoulder >  ", PART_ARMS, 2, ITEM_WEAR_SHOULDER},
  {"arms"       , "<worn on arms     >  ", PART_ARMS, 1, ITEM_WEAR_ARMS},
  {"wrist"      , "<worn on wrist    >  ", PART_ARMS, 2, ITEM_WEAR_WRIST},
  {"hands"      , "<worn on hands    >  ", PART_ARMS, 1, ITEM_WEAR_HANDS},
  {"finger"     , "<worn on finger   >  ", PART_ARMS, 2, ITEM_WEAR_FINGER},
  {"shield"     , "<worn as shield   >  ", PART_ARMS, 1, ITEM_WEAR_SHIELD},
  {"wield"      , "<wielded          >  ", PART_ARMS, 2, ITEM_WEAR_WIELD},
  {"waist"      , "<worn about waist >  ", PART_BODY, 1, ITEM_WEAR_WAIST},
  {"belt"       , "<thrust thru belt >  ", PART_BODY, 0, ITEM_WEAR_BELT},
  {"legs"       , "<worn on legs     >  ", PART_LEGS, 1, ITEM_WEAR_LEGS},
  {"ankle"      , "<worn on ankle    >  ", PART_LEGS, 2, ITEM_WEAR_ANKLE},
  {"feet"       , "<worn on feet     >  ", PART_LEGS, 1, ITEM_WEAR_FEET},
  {""           , ""                   , 0        , 0, ITEM_WEAR_MAX},
};

const struct flag_data area1_flags[] =
{
  {"closed", " Closed", AREA_CLOSED, "This area is open to morts."},
  {"noquit", " Noquit", AREA_NOQUIT, "Pc's cannot quit here."},
  {"nosettle", " Nosettle", AREA_NOSETTLE, "Societies will not make new societies here."},
  {"norepop", " Norepop", AREA_NOREPOP, "Randomly placed mobs will not pop in this area."},
  {"nolist", " Nolist", AREA_NOLIST, "Area is not listed in mortal area command."},
  {"none", "none", 0, "nothing"},
};


/* These are all the kinds of values an item can have. */

const char *value_list[VAL_MAX] =
{
  "none" ,
  "n_exit", 
  "s_exit", 
  "e_exit", 
  "w_exit", 
  "u_exit", 
  "d_exit", 
  "o_exit", 
  "i_exit", 
  "nouter", 
  "souter", 
  "eouter", 
  "wouter", 
  "uouter", 
  "douter", 
  "oouter", 
  "iouter", 
  "weapon",
  "gem",
  "armor", 
  "food", 
  "drink", 
  "magic_item",
  "tool", 
  "raw" ,
  "ammo", 
  "ranged", 
  "shop" ,
  "teacher", 
  "money" , 
  "guard", 
  "cast", 
  "powershield", 
  "map", 
  "guild", 
  "feed", 
  "teach1", 
  "teach2",
  "shop2",
  "light", 
  "randpop", 
  "pet",
  "society",
  "build",
  "popul",
  "madeof",
  "replaceby",
  "qflag",
  "package",
  "dimension",
  "craft",
  "daily_cycle",
  "coord",
};


/* MAKE SURE THESE ARE IN THE SAME ORDER AS THE FLAG_AFF_*** THINGS
   IN SERV.H. IF YOU DO NOT DO THIS *BAD* THINGS WILL HAPPEN!!!!! BE VERY
   CAREFUL WITH THIS!!!! */

const struct flag_data affectlist[] = 
{
  {"str", "Str", FLAG_AFF_STR, "This thing affects strength."},
  {"int", "Int", FLAG_AFF_INT, "This thing affects intelligence."},
  {"wis", "Wis", FLAG_AFF_WIS, "This thing affects wisdom."},
  {"dex", "Dex", FLAG_AFF_DEX, "This thing affects dexterity."},
  {"con", "Con", FLAG_AFF_CON, "This thing affects constitution."},
  {"luc", "Luc", FLAG_AFF_LUC, "This thing affects luck."},  
  {"cha", "Cha", FLAG_AFF_CHA, "This thing affects charisma."},  
  {"dam", "Dam", FLAG_AFF_DAM, "This thing affects how hard something hits."},
  {"hit", "Hit", FLAG_AFF_HIT, "This affects how often something hits."},
  {"mv", "Mv", FLAG_AFF_MV, "This affects the mv of the thing."},
  {"hp", "Hp", FLAG_AFF_HP, "This affects how many hps the thing has."},
  {"sp_attack", "Spell_Attack", FLAG_AFF_SPELL_ATTACK, "This affects how well you attack other people with spells."},
  {"sp_heal", "Spell_Heal", FLAG_AFF_SPELL_HEAL, "This affects how well you heal."},
  {"sp_resist", "Spell_Resist", FLAG_AFF_SPELL_RESIST, "This affects how well you reduce damage from spell attacks."},
  {"th_attack", "Thief_Attack", FLAG_AFF_THIEF_ATTACK, "This affects how well thieves use their skills to attack."},
  {"defense", "Defense", FLAG_AFF_DEFENSE, "This affects how well pcs use their skills for defense."},
  {"kickdam", "Kickdam", FLAG_AFF_KICKDAM, "This affects how well someone kicks."},
  {"armor", "Armor", FLAG_AFF_ARMOR, "This affects how well damage is absorbed-straight numbers, not pcts."},
  {"dam_resist", "Dam_Resist", FLAG_AFF_DAM_RESIST, "This affects how well someone absorbs damage, percentage, not fixed amount."},
  {"gf_attack", "Gf_Attack", FLAG_AFF_GF_ATTACK, "This lets you hit harder in groundfights."},
  {"gf_defense", "Gf_Defense", FLAG_AFF_GF_DEFENSE, "This lets you absorb damage in groundfights."},
  {"air_power", "Air_Power", FLAG_ELEM_POWER_AIR, "This alters your air casting power."},
  {"earth_power", "Earth_Power", FLAG_ELEM_POWER_EARTH, "This alters your earth casting power."},
  {"fire_power", "Fire_Power", FLAG_ELEM_POWER_FIRE, "This alters your fire casting power."},
  {"water_power", "Water_Power", FLAG_ELEM_POWER_WATER, "This alters your water casting power."},
  {"spirit_power", "Spirit_Power", FLAG_ELEM_POWER_SPIRIT, "This alters your spirit casting power."},
   {"air_level", "Air_Level", FLAG_ELEM_LEVEL_AIR, "This alters your air casting level."},
  {"earth_level", "Earth_Level", FLAG_ELEM_LEVEL_EARTH, "This alters your earth casting level."},
  {"fire_level", "Fire_Level", FLAG_ELEM_LEVEL_FIRE, "This alters your fire casting level."},
  {"water_level", "Water_Level", FLAG_ELEM_LEVEL_WATER, "This alters your water casting level."},
  {"spirit_level", "Spirit_Level", FLAG_ELEM_LEVEL_SPIRIT, "This alters your spirit casting level."},
  
  {"none", " None", 0, "None."},
};



/* These are mana types.... the help is the "color" for the mana */

const struct flag_data mana_info[] =
{
  {"air", " Air", MANA_AIR, "\x1b[1;36m"},
  {"earth", " Earth", MANA_EARTH, "\x1b[0;32m"},
  {"fire", " Fire", MANA_FIRE, "\x1b[1;31m"},
  {"water", " Water", MANA_WATER, "\x1b[1;34m"},
  {"spirit", " Spirit", MANA_SPIRIT, "\x1b[1;37m"},
  {"none", " None", 0, ""},
};

const struct flag_data room1_flags[] =
{
  {"inside", " Inside", ROOM_INSIDE, "This room is inside."},
  {"watery", " Watery", ROOM_WATERY, "This room is watery - need swim or boat."},
  {"airy", " Airy", ROOM_AIRY, "This room is airy..need to fly"},
  {"fiery", " Fiery", ROOM_FIERY, "This room is fiery - burns you."},
  {"earthy", " Earthy", ROOM_EARTHY, "This room is earthy - underground, crushes."},
  {"noisy", " Noisy", ROOM_NOISY, "This room is noisy, communications are harder."},
  {"peaceful", " Peaceful", ROOM_PEACEFUL, "No fighting in this room!"},
  {"dark", " Dark", ROOM_DARK, "This room is dark."},
  {"extraheal", " Extraheal", ROOM_EXTRAHEAL, "This room heals faster."},
  {"extramana", " Extramana", ROOM_EXTRAMANA, "This room gives mana back faster."},
  {"manadrain", " Manadrain", ROOM_MANADRAIN, "This room takes mana away."},
  {"nomagic", " Nomagic" ,  ROOM_NOMAGIC, "This room prevents magic from working."},
  {"underground", " Underground", ROOM_UNDERGROUND, "This room is underground."},
  {"notracks", " Notracks", ROOM_NOTRACKS, "No tracks are left in this room."},
  {"noblood", " Noblood", ROOM_NOBLOOD, "No blood is made in this room."},
  {"easymove", " Easymove", ROOM_EASYMOVE, "This room is easier to move through..no shoes required."},
  {"rough", " Rough", ROOM_ROUGH, "Harder to move through."},
  {"forest", " Forest", ROOM_FOREST, "Trees...."},
  {"field", " Field", ROOM_FIELD, "Open Area"},
  {"desert", " Desert", ROOM_DESERT, "Dry Area"},
  {"swamp", " Swamp", ROOM_SWAMP, "Hot, Watery Area"},
  {"norecall", " Norecall", ROOM_NORECALL, "Cannot recall (leave) with magic."},  
  {"nosummon", " Nosummon", ROOM_NOSUMMON, "Cannot draw people here w/magic."},
  {"underwater", " Underwater", ROOM_UNDERWATER, "Need water breath."},
  {"minerals", " Minerals", ROOM_MINERALS, "Can be mined etc...wears out."},
  {"light", " Light", ROOM_LIGHT, "There is light in here."},
  {"mountain", " Mountain", ROOM_MOUNTAIN, "You need boots to climb here."},
  {"snow", " Snow", ROOM_SNOW, "Cold and tracks disappear quickly."},
  {"astral", " Astral", ROOM_ASTRAL, "Powerful things reside here."},
  {"nomap", " Nomap", ROOM_NOMAP, "Room cannot be mapped."},
  {"none", "none", 0, "nothing"},
};

const struct flag_data thing1_flags[] =
{
  {"no_take", " No_Take", TH_NO_TAKE_BY_OTHER, "This thing cannot be moved from its spot by another thing and then get placed into that thing doing the moving."},
  {"no_move_self", " No_Move_Self", TH_NO_MOVE_SELF, "This thing cannot move itself anywhere."},
  {"no_contain", " No_Contain", TH_NO_CONTAIN, "This can never have stuff put into it."},
  {"no_drag", " No_Drag", TH_NO_DRAG, "This thing cannot be dragged anywhere..."},
  {"no_given", " No_Given", TH_NO_GIVEN, "Other things cannot give stuff to this, but it can put things into itself."},
  {"no_drop", " No_Drop", TH_NO_DROP, "A thing carrying this cannot remove it from itself."},
  {"no_can_get", " No_Can_Get", TH_NO_CAN_GET, "This thing cannot pick up anything else."},
  {"no_move_by_other", " No_Move_By_Other", TH_NO_MOVE_BY_OTHER, "This thing cannot be moved by another thing under any circumstances. No Drag/Drop/get etc."},
  {"no_take_from", " No_Take_From", TH_NO_TAKE_FROM, "This thing will not let you remove anything from it. It must remove things itself."},
  {"no_move_into", " No_Move_Into", TH_NO_MOVE_INTO, "A thing cannot place itself into this thing."},
  {"no_leave", " No_Leave", TH_NO_LEAVE, "A thing cannot remove itself from this thing."},
  {"no_fight", " No_Fight", TH_NO_FIGHT, "This thing will not fight back if attacked."},
  {"no_talk", " No_Talk", TH_NO_TALK, "This thing cannot talk and will not respond to communications."},
  {"marked", " Marked", TH_MARKED, "This thing is marked temporarily for some reason."},
  {"no_remove", " No_Remove", TH_NO_REMOVE, "This thing cannot be removed!"},
  {"changed", " Changed", TH_CHANGED, "This thing has been edited and is marked as changed now."},
  {"is_room", " Is_Room", TH_IS_ROOM, "This thing is marked as a room..cannot be copied."},
  {"unseen", " Unseen", TH_UNSEEN, "This thing cannot be seen by anyone except special players (imms)."},
  {"is_area", " Is_Area", TH_IS_AREA, "This thing is a special area thing."},
  {"nuke", " Nuke", TH_NUKE, "This thing will not be saved either as a thing or as a base."},
  {"no_peek", " No_Peek", TH_NO_PEEK, "Things cannot look into this unless they know peek."},
  {"script", " Script", TH_SCRIPT, "This thing has a script or scripts associated with it."},
  {"drop_destroy", " Drop_Destroy", TH_DROP_DESTROY, "This thing is destroyed if it is dropped."},
  {"no_give", " No_Give", TH_NO_GIVE, "You cannot give this to anyone."},
  {"none", " none", 0, "nothing"},
};


const struct flag_data aff1_flags[] =
  {
  {"blind", " Blind", AFF_BLIND, "The thing cannot see."},
  {"curse", " Cursed", AFF_CURSE, "Bad things happen to this thing..."},
  {"sleep", " Sleep", AFF_SLEEP, "This must sleep"},
  {"fear", " Fear", AFF_FEAR, "This thing cannot attack!"},
  {"disease", " Disease", AFF_DISEASE, "This has a disease."},
  {"wound", " Wound", AFF_WOUND, "This is bleeding a lot."},
  {"raw" , " Raw", AFF_RAW, "Takes more damage in combat."},
  {"outline", " Outline", AFF_OUTLINE, "You take more dam when outlined."},
  {"slow", " Slow", AFF_SLOW, "Does things slower, less attacks."},
  {"numb", " Numb", AFF_NUMB, "Makes you drop stuff."},
  {"confuse", " Confuse", AFF_CONFUSE, "Some commands fail."},
  {"forget", " Forget", AFF_FORGET, "Fails to complete timed things."},
  {"befuddle", " Befuddle", AFF_BEFUDDLE, "Spells fail 3/4 of the time."},
  {"weak", " Weak", AFF_WEAK, "Does less damage in combat."},
  {"paralyze", " Paralyze", AFF_PARALYZE, "Cannot do ANYTHING!!"},
  {"air" ," Air", AFF_AIR, "Takes more damage from air spells."},
  {"earth", " Earth", AFF_EARTH, "Takes more damage from earth spells."},
  {"fire", " Fire", AFF_FIRE, "Takes more damage from fire spells."},
  {"water", " Water", AFF_WATER, "Takes more damage from water spells."},
  {"none", "none", 0, "nothing"},
};


const struct flag_data aff2_flags[] =
{
  {"invis", " Invis", AFF_INVIS, "This thing cannot be seen."},
  {"hidden", " Hidden", AFF_HIDDEN, "This thing is hidden from sight."},
  {"dark", " Dark", AFF_DARK, "This thing is dark and hard to see."},
  {"chameleon", " Chameleon", AFF_CHAMELEON, "This thing blends in with the surroundings."},
  {"sneak", " Sneak", AFF_SNEAK, "This thing moves very quietly."},
  {"foggy", " Foggy", AFF_FOGGY, "This lets you pass through rock and doors."},
  {"sanctuary", " Sanctuary", AFF_SANCT, "This thing is resistant to damage"},
  {"protect", " Protect", AFF_PROTECT, "This thing is REALLY resistant to damage"},
  {"flying", " Flying", AFF_FLYING, "This thing is able to fly."},
  {"regenerate" , " Regenerate", AFF_REGENERATE, "This thing regenerates quickly."},
  {"reflect", " Reflect", AFF_REFLECT, "Reflects spells back."},
  {"breathe_water", " Breathe_Water", AFF_WATER_BREATH, "Can breathe underwater."},
  {"haste", " Haste", AFF_HASTE, "Does things faster."},
  {"prot_align", " Prot_Align", AFF_PROT_ALIGN, "Protects from opp align attacks."}, 
  {"air" ," Air", AFF_AIR, "A shield of electricity."},
  {"earth", " Earth", AFF_EARTH, "A shield of shards."},
  {"fire", " Fire", AFF_FIRE, "A shield of fire."},
  {"water", " Water", AFF_WATER, "A shield of ice."},
  {"none", "none", 0, "nothing"},
};
const struct flag_data act1_flags[] =
{
  {"aggressive", " Aggressive", ACT_AGGRESSIVE, "This thing tends to attack things around it."},
  {"fasthunt", " Fasthunt", ACT_FASTHUNT, "This thing chases after things very quickly."},
  {"banker", " Banker", ACT_BANKER, "This thing is a banker."},
  {"kill_opp", " Kill_Opp", ACT_KILL_OPP, "This attacks opp align things. "},
  {"assistall", " Assistall", ACT_ASSISTALL, "This mob assists every other mob in the room vs pcs."},
  {"sentinel", " Sentinel", ACT_SENTINEL, "This does not move."},
  {"angry", " Angry", ACT_ANGRY, "This is aggressive, but not as often."},
  {"mountable", " Mountable", ACT_MOUNTABLE, "This can be ridden."},
  {"wimpy", " Wimpy", ACT_WIMPY, "Will flee at 1/6 hps."},
  {"carnivore", " Carnivore", ACT_CARNIVORE, "Kills things weaker than it for food."},
  {"prisoner", " Prisoner", ACT_PRISONER, "This is a prisoner."},
  {"dummy", " Dummy", ACT_DUMMY, "This won't hit back in combat."},
  {"deity", " Deity", ACT_DEITY, "Hunts and kills fast."}, 
  {"none", "none", 0, "nothing"},
};

const struct flag_data mob1_flags[] =
{
  {"ghost", " Ghost", MOB_GHOST, "Hit only by glowing weapons."},
  {"demon", " Demon", MOB_DEMON, "Hit only by blessed weapons."},
  {"undead", " Undead", MOB_UNDEAD, "Hit by humming weapons."},
  {"nomagic", " Nomagic", MOB_NOMAGIC, "Spells don't affect this."},
  {"nohit", " Nohit", MOB_NOHIT, "Cannot physically hit this."},
  {"dragon", " Dragon", MOB_DRAGON, "Need power weapons to hit this."},
  {"angel", " Angel", MOB_ANGEL, "Hit only by baleful weapons."},
  {"fire", " Fire", MOB_FIRE, "Hit only by water weapons."},
  {"water", " Water", MOB_WATER, "Hit only by fire weapons."},
  {"Earth", " Earth", MOB_EARTH, "Hit only by air weapons."},
  {"Air", " Air", MOB_AIR, "Hit only by earth weapons."},
  {"none", "none", 0, "nothing"},
};

const struct flag_data obj1_flags[] =
{
  {"glow", " Glow", OBJ_GLOW, "This can hit ghosts."},
  {"hum", " Hum", OBJ_HUM, "This thing can hit undead."},
  {"blessed", " Blessed", OBJ_BLESSED, "This thing can hit demons."},
 
  {"power", " Power", OBJ_POWER, "This can hit dragons."},
  {"cursed", " Cursed", OBJ_CURSED, "This can hit angels."},
  {"fire", " Fire", OBJ_FIRE, "This can hit water mobs."},
  {"water", " Water", OBJ_WATER, "This can hit fire mobs."},
  {"earth", " Earth", OBJ_EARTH, "This can hit earth mobs."},
  {"air", " Air", OBJ_AIR, "This can hit earth mobs."},
  {"driveable", " Driveable", OBJ_DRIVEABLE, "This can be driven by something inside of it."}, 
  {"two_handed", " Two_Handed", OBJ_TWO_HANDED, "This obj requires two hands to hold."},
  {"nostore", " Nostore", OBJ_NOSTORE, "This item cannot be stored."},
  {"noauction", " Noauction", OBJ_NOAUCTION, "This item cannot be auctioned."},
  {"magical", " Magical", OBJ_MAGICAL, "This cannot be enchanted again."},
  {"nosacrifice", " Nosacrifice", OBJ_NOSACRIFICE, "This cannot be sacrificed by mortals.\n\r"},
  {"none", "none", 0, "nothing"},
};


const struct flag_data pc1_flags[] =
{
  {"holywalk", " Holywalk", PC_HOLYWALK, "able to walk anywhere and don't get attacked."},
  {"holylight", " Holylight", PC_HOLYLIGHT, "able to see all."},
  {"holypeace", " Holypeace", PC_HOLYPEACE, "able to stop all fighting in your room."},
  {"holyspeed", " Holyspeed", PC_HOLYSPEED, "able to do things without any delay"},
  {"wizinvis", " Wizinvis", PC_WIZINVIS, "invisible to everyone of lower level."},
  {"wizquiet", " Wizquiet", PC_WIZQUIET, "listening to any channels."},
  {"zqxsile", " Silenced", PC_SILENCE, "able to talk."},
  {"zqxfroz", " Frozen", PC_FREEZE, "able to enter commands."},
  {"zqxdeni", " Denied", PC_DENY, "able to log in."},
  {"zqxlog", " Logged", PC_LOG, "being logged"},
  {"zqxpermadeath", " Permadeath", PC_PERMADEATH, "allowed only one death THIS INCLUDES ADMINS!."},
  {"unvalidated", " Unvalidated", PC_UNVALIDATED, "validated."},
  {"ascended", " Ascended", PC_ASCENDED, "ascended."},
  {"none", "none", 0, "nothing"},
};

/* These helps look strange since they're used internally for the
   config command. The idea is that you put a "you X" or "you don't X" 
   that gets shown to the player automatically. */

const struct flag_data pc2_flags[] =
{
  {"autoloot", " Autoloot", PC2_AUTOLOOT, "automatically loot all items from corpses."},
  {"autoexit", " Autoexit", PC2_AUTOEXIT, "look at exits when you enter a room."},
  {"brief", " Brief", PC2_BRIEF, "see room descriptions when you enter a room."},
  {"autogold", " Autogold", PC2_AUTOGOLD, "automatically loot gold from corpses."},
  {"autosac" ," Autosac", PC2_AUTOSAC, "automatically sacrifice corpses.."},
  {"autosplit", " Autosplit", PC2_AUTOSPLIT, "automatically split money with group members after a kill."},
  {"tackle", " Tackle", PC2_TACKLE, "want to tackle other people, and don't mind being tackled."},
  {"nospam", " Nospam", PC2_NOSPAM, "see spammy combat messages."},
  {"mapping", " Mapping" ,PC2_MAPPING, "see an overhead map."},
  {"assist", " Assist", PC2_ASSIST, "automatically assist group members who are in combat."},
  {"autoscan", " Autoscan", PC2_AUTOSCAN, "scan for enemy players when you walk into a room."},
  {"ansi", " Ansi", PC2_ANSI, "see ANSI telnet colors."},
  {"blank", " Blank", PC2_BLANK, "see a blank line after you receive a prompt."},
  {"noprompt", " Noprompt", PC2_NOPROMPT, "have a blank prompt."},
  {"asciimap", " AsciiMap", PC2_ASCIIMAP, "see an extended ASCII character map."},
  {"numberdam", " NumberDam", PC2_NUMBER_DAM, "see numerical damage messages."},
  {"none", "none", 0, "nothing"},
};


const struct flag_data spell1_flags[] =
{
  {"opp_align", " Opp_Align", SPELL_OPP_ALIGN, "This spell works only on opp align."},
  {"heals", " Heals", SPELL_HEALS, "This spell heals damage."},
  {"refresh", " Refresh", SPELL_REFRESH, "This spell adds moves."},
  {"remove_bit", " Remove_Bit", SPELL_REMOVE_BIT, "This spell removes bits instead of adding them."},
  {"pc_only", " Pc_Only", SPELL_PC_ONLY, "Target must be a pc."},
  {"npc_only", " Npc_Only", SPELL_NPC_ONLY, "Target must not be a pc."},
  {"show_location", " Show_Location", SPELL_SHOW_LOCATION, "Shows where a thing is."},
  {"transport", " Transport", SPELL_TRANSPORT, "This spell transports something someplace."},
  {"same_align", " Same_Align", SPELL_SAME_ALIGN, "This spell only works on the same align."},
  {"perma", " Perma", SPELL_PERMA, "Adds permanent flags or removes permanent affects."},
  {"no_slist", " No_Slist", SPELL_NO_SLIST, "Players cannot slist this spell."},
  {"vampiric", " Vampiric", SPELL_VAMPIRIC, "Drains hps and gives to caster."},
  {"drain_mana", " Drain_Mana", SPELL_DRAIN_MANA, "Drains mana from the victim to give to the caster."},
  {"knowledge", " Knowledge", SPELL_KNOWLEDGE, "Gives you knowledge about something. (identify)."},
  {"room_only", " Room_Only", SPELL_ROOM_ONLY, "Can only be cast on rooms."},
  {"obj_only", " Object_Only", SPELL_OBJ_ONLY, "Can only be cast on objects."},
  {"mob_only", " Mob_Only", SPELL_MOB_ONLY, "Can only be cast on mobiles."},
  {"delayed", " Delayed", SPELL_DELAYED, "Doesn't take affect right away."},
  
  {"none", "none", 0, "nothing"},
};

const struct flag_data clan1_flags[] =
{
  {"nosave", " Nosave", CLAN_NOSAVE, "This clan is not saved with the rest."},
  
  {"all_align", " All_Align", CLAN_ALL_ALIGN, "This clan accepts members from all alignments."},
  {"none" , "none", 0, "nothing"},
};

const struct flag_data act2_flags[] =
{
  {"none", "none", 0, "nothing" },
};


const struct flag_data trigger1_flags[] =
{
  {"interrupt", " Interrupt", TRIG_INTERRUPT, "This trigger will restart if someone tries to re-execute in the middle."},
  {"leave_stop", " Leave_Stop", TRIG_LEAVE_STOP, "This script will stop if the starter leaves the same place as the runner."},
  {"nuke", " Nuke", TRIG_NUKE, "Don't save this trigger."},
  {"pc_only", " PC_Only", TRIG_PC_ONLY, "Only Pc's can trigger this."},
  {"none", "none", 0, "nothing"},

};

/* DO NOT EVER LEAVE A GAP IN HERE WHERE THERE IS A NULL FLAG BECAUSE IF
   YOU DO IT SCREWS UP THE SEARCHING THAT STOPS WHEN IT REACHES A NULL
   FLAG. JUST LEAVE A DUMMY FLAG IN THERE SOMEPLACE AND REPLACE IT AT SOME
   FUTURE DATE IF YOU WANT TO PUT MORE FLAGS IN!!!! */

struct flag_pointer flaglist[NUM_FLAGTYPES] =

{
  {"", "", NULL},
  {"zzthing", "Thing flags",(FLAG_DATA *) &thing1_flags},
  {"area", "Area flags", (FLAG_DATA *) &area1_flags},
  {"room", "Room flags", (FLAG_DATA *) &room1_flags},
  {"obj", "Object Flags",  (FLAG_DATA *) &obj1_flags},
  {"hurt", "Hurt by", (FLAG_DATA *) &aff1_flags},
  {"prot", "Protected from", (FLAG_DATA *) &aff1_flags},
  {"aff", "Affected by", (FLAG_DATA *) &aff2_flags},
  {"det", "Detecting", (FLAG_DATA *) &aff2_flags},
  {"act", "Acting", (FLAG_DATA *) &act1_flags},
  {"", "", (FLAG_DATA *) &act2_flags},
  {"mob", "Mobile flags", (FLAG_DATA *) &mob1_flags},
  {"pc1",  "Wiz flags", (FLAG_DATA *) &pc1_flags},
  {"pc2",  "Configuration", (FLAG_DATA *) &pc2_flags},
  {"spell", "Spell flags", (FLAG_DATA *) &spell1_flags},
  {"zzclan", "Clan flags", (FLAG_DATA *) &clan1_flags},
  {"zzmana", "Mana types", (FLAG_DATA *) &mana_info},
  {"society", "Society flags", (FLAG_DATA *) &society1_flags},
  {"caste", "Caste flags", (FLAG_DATA *) &caste1_flags},
  {"trigger", "Trigger flags", (FLAG_DATA *) &trigger1_flags},
  {"", "", NULL},
  {"", "", NULL},
  {"", "", NULL},
  {"", "", NULL},
  {"", "", NULL},
  {"", "", NULL},
  {"", "", NULL},
  {"", "", NULL},
  {"", "", NULL},
  {"", "", NULL},
};

/* note that the guilds must go in the same order and location as the
   numbers GUILD_THIEF etc...are in serv.h */

const struct flag_data guild_info[] =
{
  {"warrior", " Warrior's Guild", STAT_STR, "Helps with offensive fighting."},
  {"wizard", " Wizard's Guild", STAT_INT, "Helps with attack spells."},
  {"healer", " Healer's Guild", STAT_WIS, "Helps with defensive/healing spells."},
  {"thief", " Thief's Guild", STAT_DEX, "Helps with sneakines."},
  {"ranger", " Ranger's Guild", STAT_CON, "Helps with the outdoors."},
  {"tinker", " Tinker's Guild", STAT_MAX, "Helps to make and repair things."},
  {"alchemist", " Alchemist's Guild", STAT_MAX, "Helps to make magical items."},
  {"knight", " Knight's Guild", STAT_MAX, "Helps with defensive fighting techniques."},
  {"mystic", " Mystic's Guild", STAT_MAX, "Helps with detection and transport magic."},
  {"diplomat", " Diplomat's Guild", STAT_CHA, "Lets you use reason to bring societies to your side." },
  {"max", " MAX_GUILD", STAT_MAX, "No guild here."},
};

char *dir_name[] =
{
  "north",
  "south",
  "east",
  "west",
  "up",
  "down",
  "out",
  "in",
};


char *dir_track[] = 
{
  "the north",
  "the south",
  "the east",
  "the west",
  "up above",
  "down below",
  "outside",
  "inside",
};

char *dir_track_in[DIR_MAX] =
{
  "the north",
  "the south",
  "the east",
  "the west",
  "above",
  "below",
  "outside",
  "inside",
};

char *dir_dist[SCAN_RANGE] = 
  {
    "",
    "close by",
    "off",
  };

char *dir_rev[DIR_MAX] = 
  {
    "south",
    "north",
    "west", 
    "east",
    "down",
    "up",
    "in",
    "out",
  };

char *dir_letter[DIR_MAX] =
{
  "n",
  "s",
  "e",
  "w",
  "u",
  "d",
  "i",
  "o",
};


char *clantypes[CLAN_MAX] =
{
  "clan",
  "sect", 
  "temple",
  "academy",
};

/* The first string is the "name" of the coin. The second string is
   the "long appearance" of the coin if you wish to have a complicated
   expression. The number is the number of coins of the PREVIOUS TYPE
   that this coin is worth...not the TOTAL value. Note that the first
   number MUST BE 1.!!!!! The last string is the color of the coin
   which can be put in front of the name for less fancy output. */


const struct flag_data money_info[] = 
{
  {"copper", "\x1b[1;31mcopper\x1b[0;37m", 1, "\x1b[1;31m"},
  {"silver", "\x1b[1;37msilver\x1b[0;37m", 10, "\x1b[1;37m"},
  {"gold", "\x1b[1;33mgold\x1b[0;37m", 100, "\x1b[1;33m"},
  {"platinum", "\x1b[1;37mplatinum\x1b[0;37m", 1000, "\x1b[1;37m"},
  {"obsidian", "\x1b[1;30mobsidian\x1b[0;37m", 10000, "\x1b[1;30m"},
  {"adamantium", "\x1b[1;35madamantium\x1b[0;37m", 100000, "\x1b[1;35m"},
  {"end", "end" , 0, ""},
};
  
    

/* These are structs associated with time and weather. */

const char *weather_names[WEATHER_MAX] = 
{
  "clear",
  "cloudy",
  "rainy", 
  "stormy",
  "foggy",
};

const char *day_names[NUM_DAYS] = 
{
  "Solday",
  "Hergday",
  "Korday",
  "Zentra",
  "Morday",
  "Lengday",
  "Ferday",
  "Panday",
};

const char *month_names[NUM_MONTHS] =
{
  "Frigarta", /* Winter */
  "Coldallo",
  "Lighthope",
  "Newstrom", /* Spring */
  "Angrill",
  "Portent",
  "Veridian", /* Summer */
  "Alcastar",
  "Shorgran",
  "Hechke", /* Fall */
  "Larator",
  "Grotach",
  "Envirta", /* Winter */
  "Mengor",
};

const char *year_names[NUM_YEARS] = 
{
  "Owl",
  "Fox",
  "Tortoise",
  "Rabbit",
  "Deer",

  "Grasshopper",
  "Cat",
  "Hawk",
  "Chicken",
  "Bear", 

  "Wolf", 
  "Ant",
  "Turkey",
  "Fish",
  "Squirrel",

  "Rat", 
  "Antelope",
  "Bumblebee",
  "Eagle",
  "Coyote",

  "Buffalo",
  "Raven",
  "Raccoon",
  "Dog",
  "One-eyed, One-horned, Flying Purple People Eater",
};



const char *hour_names[NUM_HOURS] =
{
  "High Darkness", /* 0 */
  "Post Darkness",
  "Mid Night",
  "Mid Night",
  "Pre Dawn",

  "Early Watch", /* 5 */
  "First Light",
  "Sunrise",
  "Early Morning",
  "Mid Morning",

  "Mid Morning", /* 10 */
  "Ante Watch",
  "High Watch",
  "Post Watch",
  "Mid Postwatch",

  "Mid Postwatch", /* 15 */
  "Late Postwatch",
  "Early Evening",
  "Dusk",
  "Late Watch",

  "Evening",    /* 20 */
  "Nightfall",
  "Night",
  "Night",
  "Shadow Watch",
  "Ante Darkness",
};



char *position_names[] =
{
  "dead",
  "sleeping",
  "stunned",
  "resting",
  "tackled",
  "fighting",
  "standing",
  "meditating",
  "casting",
  "backstabbing",
  "firing",
  "searching",
  "investigating",
  "gathering",
  "grafting",
  "forging",
  "max",
};



char *position_looks[] =
{
  "here doing nothing.",
  "here sleeping peacefully.",
  "lying here stunned.",
  "here taking a moment to rest.",
  "on the ground fighting %s.",
  "here, fighting %s.",
  "here.",
  "sitting here meditating.",
  "casting a spell.",
  "here.",
  "here aiming a weapon.",
  "searching for something.",
  "investigating a corpse.",
  "gathering some raw materials.",
  "grafting something onto an item.",
  "forging a piece of equipment.",
  "in an invalid position.",
};

char *spell_types[] =
{
  "spell",
  "skill",
  "prof",
  "trap",
  "poison",
};


char *spell_target_types[] =
{
  "offensive",
  "defensive",
  "self",
  "anyone",
  "inven",
};


const char *trigger_types[] =
{
  "timed",
  "taken",
  "moved",
  "moving", 
  "given",
  "dropped",
  "taken_from",
  "get",
  "leave",
  "enter", 
  "creation",
  "death",
  "say",
  "tell",
  "command",
  "bribe",
  "max",
};



/* This is information used for gathering raw materials. These
   are the currently supported commands, and what you have
   to do to use these commands. These follow the list of raw material
   types like RAW_MINERAL in serv.h If you change that, then change
   this, too. 
   
   
The format:

{ command, raw_name, level, time,
{ 4 equip nums needed },
{ 4 equip values needed },
{ 4 names (combined with equip values) like "axe" and weapon },
 {4 use_up numbers...like corpse or vial for blood. },
 put_into_object, need_minerals, need_skill, room_flags, 
vnum_start_gathering, ranks_to_gather, {stop_chances}, {fail_chances}}


I really don't want to make an editor for this, sorry. These shouldn't
be changing that often anyway. 

*/

const struct gather_raw_data gather_data[] = 
{ 
  { "", "", "", 0, 0,
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    { "", "", "", ""},
    { 0,0,0,0},
    0, FALSE, FALSE, 0,
    0, 0, 0, {0,0}, {0,0}},

  { "mine", "mineral", "minerals", 20, 30,  /* Minerals  1 */
    {190, 0, 0, 0},
    {0, 0, 0, 0}, 
    {"", "", "", ""},
    {0, 0, 0, 0},
    0, TRUE, TRUE, ROOM_UNDERGROUND | ROOM_ROUGH | ROOM_MOUNTAIN, 
      FORGED_EQ_AREA_VNUM+10, 1, MAX_MINERAL, {2,1}, {2, 1}},

  { "hew", "stone", "stone", 30, 40,  /* Stone  2 */
    {192, 0, 0, 0},
    {0, 0, 0, 0},
    {"", "", "", ""},
    {0, 0, 0, 0},
    0, FALSE, FALSE, ROOM_MOUNTAIN | ROOM_UNDERGROUND,
    900, 4, 1, {3,1}, {2,1}},

  { "chop", "wood", "wood", 25, 40,  /* Wood  3 */
    {0, 0, 0, 0},
    {VAL_WEAPON, 0, 0, 0},
    {"axe", "", "", ""},
    {0, 0, 0, 0},
    0, TRUE, TRUE, ROOM_FOREST,
    820, 6, 1, {2, 1}, {2, 1}},
  
  { "collect", "flower", "flowers", 30, 35,         /* Flowers 4 */
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    { "", "", "", ""},
    { 0,0,0,0},
    0, FALSE, FALSE, ROOM_FIELD,
    800, 4, 1, {2,1}, {3,1}},

  { "harvest", "food", "food", 30, 40,  /* Food 5 */
    {191, 0, 0, 0},
    {0, 0, 0, 0},
    {"", "", "", ""},
    {0, 0, 0, 0},
    0, FALSE, FALSE, ROOM_FIELD,
    920, 3, 1, {2,1}, {2,1}}, 

  { "find", "herb", "herbs", 35, 60,  /* Herbs 6 */
    {773, 0, 0, 0},
    {VAL_WEAPON, 0, 0, 0},
    {"sickle", 0, 0, 0},
    {0, 0, 0, 0},
    774, TRUE, TRUE, ROOM_FOREST,
    850, 7, 1, {5, 4}, {2, 1}},
  
  { "extrude", "ichor", "ichor", 50, 30, /* Ichor 7 */
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
      { "", "", "", "" },
    { CORPSE_VNUM, 708, 0, 0 },
      0, FALSE, TRUE, 0, 
	950, 5, 1, {2,1}, {4,1}},
  
  
  { "max", "", "", 0, 0,          /* Max 8 */
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    { "", "", "", ""},
    { 0,0,0,0},
    0, FALSE, FALSE, 0,
    0, 0, 0, {0,0}, {0,0}},
};

/* These are the various kinds of values you can add onto items if
   they have certain values on them already. The format is this:


   {"command/spellname", time to do it, oldval, newval, num_needed,
   {pct,pct, pct, pct, pct, pct}} <- pcts of old values brought over. 

   The blank struct at the end tells us where to stop. */


const struct graft_value_data graft_data[] =
{
  {"imbue", 100, VAL_WEAPON, VAL_GEM, 5,
   {100, 67, 67, 67, 0, 0}},
  {"graft", 100, VAL_GEM, VAL_WEAPON, 5,
   {67, 80, 100, 50, 0, 0}},
  {"emplace", 150, VAL_ARMOR, VAL_RANGED, 5, 
   {125, 75, 75, 70, 100, 0}},
  {"", 0, 0, 0, 0,
   {0, 0, 0, 0, 0, 0}},
};
   
   


/* These two are accessed by looking at th->hp * 10/th->max_hp then putting
   it between 0 and 10. */

const char *obj_short_condition[CONDITIONS] = 
{
  "(tatters)",   /* 0 */
  "(badly damaged)",
  "(damaged)",
  "(poor)",
  "(poor)",
  "(fair)",  /* 5 */
  "(fair)",
  "(good)",
  "(good)",
  "(brand new)",
  "(brand new)", /* 10 */
};

const char *obj_condition[CONDITIONS] =
{
  "is in tatters.",
  "is badly damaged.",
  "is damaged.",
  "is in poor condition.",
  "is in poor condition.",
  "is in fair condition.",
  "is in fair condition.",
  "is in good condition.",
  "is in good condition.",
  "is brand new.",
  "is brand new.",
};
const char *mob_condition[CONDITIONS] = 
{
  "is at death's door.", /* 0 */
  "is almost dead.",
  "is very badly hurt.",
  "is badly hurt.",
  "is hurt.",
  "is hurt.",  /* 5 */
  "is hurt.",
  "is in good condition.",
  "is in good condition.",
  "is in good condition.", 
  "is in excellent condition.", /* 10 */
};

  
const char *carry_weight_amt[MAX_CARRY_WEIGHT_AMT] =
{
  "You are unburdened.",
  "No problem.",
  "A tad heavy.",
  "Somewhat heavy.",
  "Very Heavy.",
  "REALLY HEAVY!",
  "OMG YOU *NEED* TO PUT SOME OF THIS STUFF DOWN!",
};


/* Default strings you see when you use a weapon. */

const char *weapon_damage_names[] =
{
  "slash",
  "pierce",
  "pound",
  "whip"
};

/* Default types of weapons. */

const char *weapon_damage_types[] =
{
  "slashing",
  "piercing",
  "concussion",
  "whipping",
  "max",
};

const char *sex_names[] =
{
  "neuter",
  "female",
  "male",
};

/* Potion colors with ansi strings */


const char *potion_colors[POTION_COLORS] =
{
  "\x1b[1;30mblack\x1b[0;37m",
  "\x1b[0;31mred\x1b[0;37m",
  "\x1b[0;32mgreen\x1b[0;37m",
  "\x1b[0;33mbrown\x1b[0;37m",

  "\x1b[0;34mblue\x1b[0;37m",
  "\x1b[0;35mmagenta\x1b[0;37m",
  "\x1b[0;36mcyan\x1b[0;37m",
  "\x1b[0;37mgray\x1b[0;37m",

  "\x1b[0;37mclear\x1b[0;37m",
  "\x1b[1;31mred\x1b[0;37m",
  "\x1b[1;32mgreen\x1b[0;37m",
  "\x1b[1;33myellow\x1b[0;37m",

  "\x1b[1;34mblue\x1b[0;37m",
  "\x1b[1;35mopurple\x1b[0;37m",
  "\x1b[1;36mcyan\x1b[0;37m",
  "\x1b[1;37mwhite\x1b[0;37m",
};

/* This is the potion colors without the ansi strings */

const char *potion_color_names[POTION_COLORS] =
{
  "black",
  "red",
  "green",
  "brown",

  "blue",
  "magenta",
  "cyan",
  "gray",

  "clear",
  "red",
  "green",
  "yellow",

  "blue",
  "purple",
  "cyan",
  "white",
};

const char *potion_types[] =
{
  "bubbling ",
  "",
  "glowing ",
  "thick ",
  "viscous ", /* 5 */
  "sparkling ",
  "translucent ", /* These three descriptions rule being dumb. :) */
  "transparent ",
  "opaque ",
  "lumpy ", /* 10 */
  "fizzing ",
  "luminous ",
  "spotted ",
  "gaseous ", /* 14 */
};
  

const char *mineral_names[MAX_MINERAL] =
{
  "brass",
  "copper",
  "iron",
  "steel",
  "silver",
  "gold",
  "platinum",
  "obsidian",
  "mithril",
  "adamantium",
};

const char *mineral_colornames[MAX_MINERAL] =
{
  "\x1b[1;33mbrass\x1b[0;37m",
  "\x1b[1;31mcopper\x1b[0;37m",
  "\x1b[1;30miron\x1b[0;37m",
  "\x1b[1;37msteel\x1b[0;37m",
  "\x1b[1;37msilver\x1b[0;37m",
  "\x1b[1;33mgold\x1b[0;37m",
  "\x1b[1;37mplatinum\x1b[0;37m",
  "\x1b[1;30mobsidian\x1b[0;37m",
  "\x1b[1;36mmithril\x1b[0;37m",
  "\x1b[1;35madamantium\x1b[0;37m",
};


const struct _forge_data forge_data[] =
{
  {"chunk", ITEM_WEAR_WIELD, VAL_RAW, STAT_MAX, 0,RAW_MINERAL, 2, 200},
  {"anvil", ITEM_WEAR_WIELD, VAL_TOOL, STAT_MAX, 0, TOOL_ANVIL, 15, 
10000},
  {"hammer", ITEM_WEAR_WIELD, VAL_TOOL, STAT_MAX, 0, TOOL_HAMMER, 4, 
5000},
  {"helmet", ITEM_WEAR_HEAD, VAL_ARMOR, STAT_WIS, 0, 0, 4, 3000},
  {"mask", ITEM_WEAR_FACE, VAL_ARMOR, STAT_INT, 0, 0, 2, 2500},
  {"plate", ITEM_WEAR_BODY, VAL_ARMOR, STAT_STR, 0, 0, 7, 3500},
  {"vambraces", ITEM_WEAR_ARMS, VAL_ARMOR, STAT_STR, 0, 0, 5, 4000},
  {"gauntlets", ITEM_WEAR_HANDS, VAL_ARMOR, STAT_DEX, 0, 0, 2, 3500},
  {"shield", ITEM_WEAR_SHIELD, VAL_ARMOR, STAT_DEX, 0, 0, 5, 3000},
  {"sword", ITEM_WEAR_WIELD, VAL_WEAPON, STAT_MAX, 2, WPN_DAM_SLASH, 6, 3000},
  {"mace", ITEM_WEAR_WIELD, VAL_WEAPON, STAT_WIS, 2, WPN_DAM_CONCUSSION, 5, 4000},
  {"dagger", ITEM_WEAR_WIELD, VAL_WEAPON, STAT_DEX, 2, WPN_DAM_PIERCE, 2, 6000},
  {"belt", ITEM_WEAR_WAIST, VAL_ARMOR, STAT_STR, 0, 0, 2, 6000},
  {"greaves", ITEM_WEAR_LEGS, VAL_ARMOR, STAT_MAX, 0, 0, 5, 4000},
  {"boots", ITEM_WEAR_FEET, VAL_ARMOR, STAT_MAX, 0, 0, 4, 5000},
  {"", 0, 0, 0, 0, 0, 0, 0},
};


/* This is a list of names of all the types of pkdata for use with
   replace_one_value */

const char *pk_names[PK_MAX]=
  {
    "wps",
    "twps",
    "kils",
    "lev",
    "hlp",
    "pks",
    "kpts",
    "solo",
    "rat",
    "bgpkw",
    "kild",
    "pked",
    "brigack",
    
  };

    
    /* This is a list of all the items newbies get. up to 20 should be
   sufficient... */

int newbie_items[NUM_NEWBIE_ITEMS] =
{
  642, 740, 740, 740, 740, 
  741, 650, 300, 300, 300, 
  600,   0,   0,   0,   0,
  0,   0,   0,   0,  0};


/* ALWAYS MAKE THE FIRST RAW MATERIAL NUMBER 0 IN THIS STRUCT!! */

const struct _build_data build_data[BUILD_MAX] =
{
  {"member", BUILD_MEMBER, {5, 5, 5, 5, 5, 10, 10, 10}},
  {"maxpop", BUILD_MAXPOP, {5, 10, 10, 10, 5, 10, 10, 10}},
  {"quality", BUILD_QUALITY, {50, 50, 30, 20, 50, 40, 45, 50}},
  {"tier", BUILD_TIER, {200, 30, 30, 30, 20, 15, 10, 10}},
  {"warrior", BUILD_WARRIOR, {2, 2, 2, 2, 2, 2, 2, 2}},
  {"worker", BUILD_WORKER, {2, 2, 2, 2, 2, 2, 2, 2}},  
};


/* List of damage types/names for groundfighting. */

char *groundfight_damage_types[NUM_GROUNDFIGHT_DAMAGE_TYPES] =
{
  "punch",
  "pummel",
  "kick",
  "knee",
  "scrape",
  "rake",
  "pound",
  "smash",
  "twist",
  "slam",
  "smack",
};


/* List of the names of differnet kinds of tools. */

const char *tool_names[TOOL_MAX] =
  {
    "nothing",
    "lockpicks",
    "anvil",
    "hammer",
    "needle",
    "saw",
    "knife",
    "plane",
    "plate",
    "bowl",
    "jar",
    "pipe",
  };

const char *disease_symptoms[MAX_DISEASE_SYMPTOMS] =
  { 
    "cough",
    "wheeze",
    "sneeze",
    "shiver",
    "look ill", /* 5 */
    
    "look sick",
    "look dizzy",
    "moan in pain",
    "get a glazed look",
    "hack",  /* 10 */
    
    "groan",
    "groan in agony",
    "whimper",
    "cough",
    "cough", /* 15 */
   
    "cough",
    "sneeze",
    "sneeze",
    "sniffle",
    "snuffle",  /* 20 */

    
  };
