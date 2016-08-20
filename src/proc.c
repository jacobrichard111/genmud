#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include "serv.h"
#include "society.h"




/* This does stuff like 3d6 7d2 etc...not really used much since damage
   for wpns is calculated more simply. */

int 
dice (int first, int second)
{
  int i, num = 0;
  if (first < 1 || first > 100 || second < 1 || second > 1000)
    return 1;
  for (i = 0; i < first; i++)
    num += nr(1,second);
  return num;
}



/* This takes a list of words and returns a random one. */

char *
find_random_word (char *txt, char *society_name)
{
  static char arg1[STD_LEN*5];
  char *arg;
  int count, num_choices = 0, num_chose = 0;
  arg1[0] = '\0';
  
  if (!txt || !*txt)
    return arg1;
  
  
  for (count = 0; count < 2; count++)
    { 
      arg = txt; 
      do
	{
	  arg = f_word (arg, arg1);
	  
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;	  
	}
      while (*arg); 
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    {
	      *arg1 = '\0';
	      break;
	    }
	  else
	    num_chose = nr (1, num_choices);
	}
    }
  
  /* "society_name" name means return random society from generated ones
     society_any means ANY society. */
  
  
  if (!str_cmp (arg1, "society_name"))
    {
      if (!society_name)
	return find_random_society_name ('n', 1);
      else if (*society_name)
	return society_name;    
      return find_random_society_name ('n', 1);
    }
  else if (!str_cmp (arg1, "society_any"))
    {
      if (!society_name)
	return find_random_society_name ('n', 0);
      else if (*society_name)
	return society_name;
      return find_random_society_name ('n', 0);
    }
  else if (!str_cmp (arg1, "proper_name"))
    {
      return create_society_name (NULL);
    }
  return arg1;
}
  
  

/* This searches a string for something of the form 2.xxx or 34.xgfh.
   It moves up the value of the original pointer, and it also returns
   an int value. Errors basically make the return value 0. */


char *
find_first_number (char *arg, int *num)
{
  int num_digits = 0;
  char *oldarg = arg;
  *num = 0;
  if (!arg || !*arg || !isdigit (*arg))
    return oldarg;
  do
    {
      if (++num_digits >= 9)
	{
	  *num = 0;	  
	  return oldarg;
	}
      *num *= 10;
      *num += (*arg - '0');
      arg++;
    }
  while (isdigit (*arg));
  if (*arg != '.')
    {
      *num = 0;
      return oldarg;
    }
  /* now move past the '.' */
  return ++arg;
}
  

/* This is my strlen function for ignoring color codes. */

int
strlen_color (const char *t)
{
  int len = 0;
  if (!t || !*t)
    return 0;
  do
    {
      if (*t == '\x1b')
	while (*t && *t != 'm')
	  t++;
      else
	len++;
    }
  while (*t++);
  return len;
}

/* This returns "a" or "an" depending on what the string given
   starts with. Sucky. Will have to add exceptions as time goes
   on since English sucks. :P */

char *
a_an (const char *t)
{
  char word[STD_LEN];
  char c;
  if (!t || !*t)
    return "a";
  
  f_word ((char *) t, word);
  
  if (!str_prefix ("use", word))
    return "a";
  
  c = word[0];
  
  if (c == 'e' || c == 'a' || c == 'o' || c == 'i' || c == 'u')
    return "an";

  if (!str_prefix ("honest", word) ||
      !str_prefix ("honor", word))
    return "an";
  
  return "a";
}

	   
/* This makes a new string in mem...but it segfaults if we cannot 
   allocate memory. Yes it sucks since it will probably leave
   holes, but the memory usage will hopefully be in huge blocks
   at bootup which won't be free'd much anyway. */

char *
new_str (const char *old)
{
  char *newstr;
  if (!old || !*old)
    return nonstr;
  newstr = strdup (old);
  if (!newstr)
    {
      log_it ("Could not allocate a new string!\n");
      exit (5);
    }
  string_count += ((strlen(old)/4 + 3)*4);
  return newstr;
}

/* Unallocate a string. I don't honestly know how well mmap handles all
   this creation/destruction, but at least most things don't get strings
   created along with them, unless they are "unique" in some way. */

void
free_str (char *old)
{
  if (old == nonstr || !old || !*old)
    return;
  string_count -= ((strlen(old)/4 + 3)*4);
  free2(old);
  return;
}

/* Malloc that segfaults if we run out of memory :P */

void *
mallok (int size)
{
  void *ret; 
  
  ret = malloc (size);
  if (ret)
    bzero (ret, size);
  else
    {
      log_it ("MALLOC FAILED!!!!\n");
      exit (5);
    }
  return ret;
}

void 
free2 (void *p)
{
  if (p)
    free (p);
  return;
}

/* This is a wrapper for ctime so I don't have to deal with
casting and the \n\r. */

char *
c_time (int tm)
{
  static char ret[STD_LEN];
  char *t;  
  

  sprintf (ret, "%s", ctime((const time_t *) &tm));
  t = ret;
  while (*t && *t != '\n' && *t != '\r')
    t++;
  *t = '\0';
  return ret;  

}

/* Checks if something is a number or not. Dunno why I did this. */

bool 
is_number (char *s)
{  
  if (*s == '-')
    s++;
  for (; *s; s++)
    if (*s < '0' || *s > '9')
      return FALSE;
  return TRUE;
}

/* Tells if a certain character is an operation or not. */

bool
is_op (char t)
{
  if (t == '+' || t == '-' || t == '/' || t == '*' || t == '='||
      t == '%' || t == '^' || t == '_')
    return TRUE;
  return FALSE;
}

/* Get stats. Shrug, might want to adjust for "evil daylight" stuff. Dunno. */

int 
get_stat (THING *th, int num)
{
  if (!th || !IS_PC (th))
    return MAX_STAT_VAL;
  if (num < 0 || num > STAT_MAX - 1)
    return MIN_STAT_VAL;  
  return MID (MIN_STAT_VAL, th->pc->stat[num] + MIN (2, th->pc->aff[num + AFF_START]), MAX_STAT_VAL);
}

/* You type in a flagname, and this sees if it's really attached to a flag. */

int
find_bit_from_name (int type, char *buf)
{
  FLAG_DATA *flg;
  if (!buf || !*buf || type < 0 || type >= NUM_FLAGTYPES ||
      flaglist[type].whichflag == NULL)
    return 0;  
  for (flg = flaglist[type].whichflag; flg->flagval != 0; flg++)
    if (!str_cmp (buf, flg->name))
      return flg->flagval;
  return 0;
}

/* This returns a flagtype based on the name given. If there is an error,
   it returns 0. */

int 
find_flagtype_from_name (char *name)
{
  int i;
  if (!name || !*name)
    return AFF_MAX;
  
  for (i = 1;  flaglist[i].whichflag != NULL && i < NUM_FLAGTYPES; i++)
    {
      if (!str_prefix (name, flaglist[i].name))
	{
	  return i;
	}
    }
  for (i = 0; affectlist[i].flagval != 0; i++)
    {
      if (!str_prefix (name, affectlist[i].name))
	{
	  return i + AFF_START;
	}
    }
  return AFF_MAX;
}

/* This shows a list of all the names of the bits set in a certain
   bitvector. Note: This is usually accesed with show_flags, or
   show_values. */

char *
list_flagnames (int type, int flags)
{
  static char buf[STD_LEN];
  FLAG_DATA *flg;  
  int len = 0;
  buf[0] = '\0';
  if (type < 0 || type >= NUM_FLAGTYPES || flags == 0 ||
      flaglist[type].whichflag == NULL)
    return buf;
  
  sprintf (buf, "%s", flaglist[type].app);
  len = strlen(buf);
  for (flg = flaglist[type].whichflag; flg->flagval != 0; flg++)
    if (IS_SET (flags, flg->flagval))
      {
	sprintf (buf + len, flg->app);
	len += strlen(flg->app);
      }
  return buf;
}


/* This adds up all the flags for a certain thing  and then
   prints out the final message based on whether we want to see all
   flags, or just some of them. */


char *
show_flags (FLAG *startflag, int type, bool builder)
{
  static char ret[STD_LEN * 2];
  char buf[STD_LEN];
  char durat[STD_LEN];
  char sname[STD_LEN];
  char buf2[STD_LEN];
  FLAG *flg;
  SPELL *spl;
  ret[0] = '\0';
  if (type < 0 || type >= NUM_FLAGTYPES || !startflag)
    return ret;
  
  for (flg = startflag; flg; flg = flg->next)
    {
      if ((type > 0 && type != flg->type) ||
	  flg->type < 1 || flg->type >= AFF_MAX ||
	  (flg->type < NUM_FLAGTYPES && 
	   flaglist[flg->type].whichflag == NULL))
	continue;
      
      if (!builder && flg->type == FLAG_PC1)
	continue;
      
      if ((spl = find_spell (NULL, flg->from)) != NULL &&
	  spl->name && spl->name[0])
	sprintf (sname, "'%s'", spl->name);
      else
	*sname = '\0';
      
      if (flg->timer == 0)
	sprintf (durat, "permanently.");
      else if (builder)
	sprintf (durat, "will wear off in %d hour%s.", flg->timer, (flg->timer == 1 ? "" : "s"));
      else if (flg->timer== 1)
	sprintf (durat, "will wear off at any moment.");
      else if (flg->timer < 4)
	sprintf (durat, "will wear off in a few hours.");
      else if (flg->timer < 7)
	sprintf (durat, "will wear off in about a quarter day.");
      else if (flg->timer < 14)
	sprintf (durat, "will wear off in about a half day.");
      else if (flg->timer < 35)
	sprintf (durat, "will wear off in about a day.");
      else
	sprintf (durat, "will not wear off for many days!");
      
      /* Spell affects */

      if (sname[0])
	{
	  if (!builder)
	    sprintf (buf, "%s, which %s\n\r", sname , durat);
	  else
	    {
	      if (flg->type < NUM_FLAGTYPES)
		{
		  sprintf (buf, "%s sets bits %s %s%s\n\r", sname,
			   list_flagnames (flg->type, flg->val), 
			   (flg->timer != 0 ? "and " : "" ), durat);
		}
	      else
		sprintf (buf, "%s affects %s by %d %s%s\n\r",
			 sname, 
			 affectlist[flg->type - AFF_START].app,
			 flg->val, (flg->timer != 0 ? "and " : ""),
			 durat);
	    }
	}
      else 
	{
	  if (flg->type < NUM_FLAGTYPES)
	    sprintf (buf, "%s %s\n\r", list_flagnames(flg->type, flg->val),
		     durat);
	  else
	    sprintf (buf, "%s affected by %d %s\n\r",
		     affectlist[flg->type - AFF_START].app,
		     flg->val, durat); 
	}
      sprintf (buf2, "\x1b[1;3%dm%s\x1b[0;37m",
	       (flg->type == FLAG_HURT ||
		(flg->type >= AFF_START &&
		 (int) flg->val < 0) ? 1 : 7),
	       buf);
      strcat (ret, buf2);
    }  
  return ret;
}
	
    

/* This shows all of the values in a list of values. It then returns
   a list of all the values involved. May want to clean this up with
   special strings for each value. Real pain tho. */

char *
show_values (VALUE *startval, int type, bool details)
{
  static char ret[STD_LEN *10];
  char *retpos = ret, *valpos /* Location of the return buffer. */;
  VALUE *val;  
  ret[0] = '\0';
  if (type < 0 || type >= VAL_MAX || !startval)
    return ret;
  for (val = startval; val; val = val->next)
    {
      if (type > 0 && type != val->type)
	continue;
      valpos = show_value (val, details);
      sprintf (retpos, valpos);
      retpos += strlen (valpos);
      if (retpos - ret > STD_LEN * 8)
	break;
    }
  return ret;
}

/* This finds a value type from a given name. */

int 
find_value_from_name (char *name)
{
  int valtype;

  if (!name || !*name)
    return VAL_MAX;
  
  for (valtype = 1; valtype < VAL_MAX; valtype++)
    {
      if (!str_prefix (name, value_list[valtype]))
	break;
    }
  return valtype;
}

/* Show one value, using details or not. */

char *
show_value (VALUE *val, bool details)
{
  static char ret[STD_LEN*2];
  int i;
  SPELL *spl;
  THING *obj;
  ret[0] = '\0';
  if (!val || val->type < 1 || val->type >= VAL_MAX)
    return ret;
  if (!details)
    {
      sprintf (ret, "Val %s: (%s),", value_list[val->type], val->word);
      for (i = 0; i < NUM_VALS; i++)
	sprintf (ret + strlen (ret), " %d", val->val[i]);
     
    }  
  else if (val->type <= VAL_EXIT_I)
    {
      spl = find_spell (NULL, val->val[4]);
      if (spl && spl->spell_type != SPELLT_TRAP)
	spl = NULL;	  
      

      sprintf (ret, "Exit: %s  To: %d  FLags: %d/%d  Str: %d/%d  Trap: %s Key: %d", dir_name[val->type - 1], val->val[0], val->val[1], val->val[2], 
	       val->val[3]%1000, val->val[3]/1000, 
	       (spl ? spl->name : "none"), val->val[5]);
    }
  else if (val->type <= VAL_OUTEREXIT_I)
    {
      sprintf (ret, "Climbable %s  To: %d  Flags: %d/%d", 
	       dir_name[val->type - VAL_OUTEREXIT_N], val->val[0],
	       val->val[1], val->val[2]);
    }
  else
    {
      switch (val->type)
	{
	  case VAL_WEAPON:
	    spl = find_spell(NULL, val->val[5]);
	    if (spl && spl->spell_type != SPELLT_POISON)
	      spl = NULL;
	    sprintf (ret, "Weapon: Dam: %d-%d pts  Type: %s  Speed: %d  Poison Shots: %d  Type: %s  Damage Name: %s", val->val[0], val->val[1], 
		     (val->val[2] >= WPN_DAM_SLASH && val->val[2] < WPN_DAM_MAX ? weapon_damage_types[val->val[2]] : "slashing"),
		     val->val[3], val->val[4], (spl ? spl->name : "none"),
		     (val->word && *val->word ? val->word : "regular"));
	    break;
	  case VAL_GEM:
	    sprintf (ret, "Gem: Colors: %s  Max Mana: %d  Level: %d",
		     list_flagnames (FLAG_MANA, val->val[0]), val->val[2], val->val[3]);
	    break;
	  case VAL_ARMOR:
	    sprintf (ret, "Armor: Head: %d  Body: %d  Arms: %d  Legs: %d", val->val[0], val->val[1], val->val[2], val->val[3]);
	    break;
	  case VAL_FOOD:
	    sprintf (ret, "Food: Amount: %d  Spells: ",
		     val->val[0]);
	    for (i = 3; i < NUM_VALS; i++)
	      if ((spl = find_spell (NULL, val->val[i])) != NULL &&
		  spl->spell_type == SPELLT_SPELL)
		{
		  strcat (ret, spl->name);
		  strcat (ret, " ");
		}
	    break;
	  case VAL_DRINK:
	    sprintf (ret, "Drink: Curr/Max Drinks %d/%d Spells: ",
		     val->val[0], val->val[1]);
	    for (i = 3; i < NUM_VALS; i++)
	      if ((spl = find_spell (NULL, val->val[i])) != NULL && 
		  spl->spell_type == SPELLT_SPELL)
		{
		  strcat (ret, spl->name);
		  strcat (ret, " ");
		}
	    break;
	  case VAL_PACKAGE:
	    {
	      SOCIETY *soc;
	      THING *area;
	      if ((soc = find_society_num (val->val[0])) != NULL &&
		  (area = find_area_in (soc->room_start)) != NULL)
		{
		  sprintf (ret, "Must be taken to the %s of %s as quickly as possible.", society_pname(soc), NAME(area));
		}
	    }
	    break;
	  case VAL_MAGIC:
	    sprintf (ret, "Magic Item: Flags: %d  Curr/Max Charges: %d/%d ",
		     val->val[0], val->val[1], val->val[2]);
	    for (i = 3; i < NUM_VALS; i++)
	    if ((spl = find_spell (NULL, val->val[i])) != NULL && 
		spl->spell_type == SPELLT_SPELL)
	      {
		strcat (ret, spl->name);
		strcat (ret, " ");
	      }
	    break;
	  case VAL_TOOL:
	    if (val->val[0] > 0 && val->val[0] < TOOL_MAX)
	      sprintf (ret, "This tool can be used as %s %s",
		       a_an(tool_names[val->val[0]]),
		       tool_names[val->val[0]]);
	    else
	      *ret = '\0';
	    break;
	case VAL_RAW:
	  sprintf (ret, "This is a raw material.");
	  break;
	  case VAL_AMMO:
	    sprintf (ret, "Ammo: Dam: %d-%d pts  Curr Shots: %d  Max Shots: %d  Type: %d", val->val[0], val->val[1], val->val[2], val->val[3], val->val[4]);
	    break;
	  case VAL_RANGED:
	    sprintf (ret, "Ranged Weapon: Speed: %d  Shots/Aim %d  Range: %d  Multiplier: %d.%dx Type: %d", val->val[0], val->val[1], val->val[2], val->val[3]/10, val->val[3] % 10, val->val[4]);
	    break;
	  case VAL_SHOP:
	    obj = find_thing_num (val->val[4]);
	    sprintf (ret, "Shop: Hours Open: %d to %d  Profit Pct: %d  Make Hrs: %d  Makes: %d  (%s) Num To Decrease: %d  Sells: %s",
		     (val->val[0] < val->val[1] ? val->val[0] : val->val[1]),
		     (val->val[0] < val->val[1] ? val->val[1] : val->val[0]),
		     val->val[2], val->val[3], val->val[4], 
		     (obj ? obj->short_desc : "nothing"), val->val[5],
		     (val->word ? val->word : "nothing"));
	    break;
	  case VAL_TEACHER0:
	  case VAL_TEACHER1:
	  case VAL_TEACHER2:
	  case VAL_CAST:
	    if (val->type == VAL_CAST)
	      sprintf (ret, "Casts: ");
	    else
	      sprintf (ret, "Teacher: ");
	    for (i = 0; i < NUM_VALS; i++)
	      {
		if ((spl = find_spell (NULL, val->val[i])) != NULL)
		  {
		    strcat (ret, spl->name);
		  strcat (ret, " ");
		  }
	      }
	    break;
	  case VAL_PET:
	    sprintf (ret, "Pet Owner: %s", val->word);
	    break;
	  case VAL_GUARD:
	    sprintf (ret, "Guards: Flags: %d  Things: %d %d %d  Clan/Sect Num: %d %d  Directions: %s", val->val[0], val->val[1], val->val[2], val->val[3], val->val[4], val->val[5], (val->word && *val->word ? val->word : "none"));
	    break;
	  case VAL_POWERSHIELD:
	    sprintf (ret, "Powershield: Max Power: %d", val->val[2]);
	  break;
	  case VAL_MAP:
	  obj = find_thing_num (val->val[0]);
	  sprintf (ret, "This map shows the area near %s.", 
		   (obj ? obj->short_desc : "nowhere"));
	  break;
	  case VAL_GUILD:
	    if (val->val[0] < 0 || val->val[0] >= GUILD_MAX)
	      break;
	    sprintf (ret, "%s guildmaster for tiers %d to %d", 
		     guild_info[val->val[0]].app,
		     (val->val[2] > 0 ? val->val[1] : 0),
		     (val->val[2] > 0 ? val->val[2] : GUILD_TIER_MAX));
	    break;
	  case VAL_MONEY:
	    sprintf (ret, "This thing has money on it.");
	    break;
	  case VAL_FEED:
	  case VAL_SOCIETY:
	  case VAL_BUILD:
	  case VAL_POPUL:
	    sprintf (ret, "%s: %d %d %d %d %d %d '%s'",
		     value_list[val->type], val->val[0], val->val[1],
		     val->val[2], val->val[3], val->val[4], val->val[5], 
		     val->word);
	    
	  default:
	    break;
	    break;
	  case VAL_LIGHT:
	    sprintf (ret, "Light: %d out of %d hours   Flags: %d",
		     val->val[0], val->val[1], val->val[2]);
	    break;
	  case VAL_RANDPOP:
	    sprintf (ret, "Randpop: Start: %d Num/Tier: %d  Num Tiers: %d  Level Advance: %d  Rand Advance: nr(1,%d) < %d",
		     val->val[0], val->val[1], val->val[2], val->val[3], val->val[4], val->val[5]);
	    break;
	}
    } 
  if (*ret)
    {
      if (val->timer > 0)
	sprintf (ret + strlen (ret),  " for %d hours.", val->timer);
      strcat (ret, "\n\r");
    }
  return ret;
}







char *
show_resets (RESET *start)
{
  RESET *res;
  THING *thg;
  int num = 1;
  static char ret[STD_LEN * 10];
  char *retpos = ret;
  char buf[STD_LEN*2];
  ret[0] = '\0';
  if (!start)
    return "";
  for (res = start; res; res = res->next)
    {
      thg = find_thing_num (res->vnum);
      sprintf (buf, "Reset \x1b[1;34m%d\x1b[0;37m: (\x1b[1;36m%d\x1b[0;37m - %s) (\x1b[1;37m%d\x1b[0;37m pct) (\x1b[1;32m%d\x1b[0;37m max) (\x1b[1;31m%d\x1b[0;37m nest)\n\r", 
	       num, res->vnum, 
	       (thg ? 
		(*NAME(thg) ? NAME(thg) : KEY(thg)) : "nothing"), 
	       res->pct, res->times, res->nest);
      
      strcat (retpos, buf);
      retpos += strlen(buf);
      if (retpos - ret > STD_LEN * 8)
	{
	  sprintf (retpos, "The list is too long.\n\r");	  
	  break;
	}
      num++;
    }
  return ret;
}

bool 
dark_inside (THING *th)
{
  int flags = flagbits (th->flags, FLAG_ROOM1);
  if (!th || th->light > 0 || !IS_ROOM (th) || IS_SET(flags, ROOM_INSIDE | ROOM_LIGHT))
    return FALSE;
  if (wt_info->val[WVAL_HOUR] < 6 ||
      wt_info->val[WVAL_HOUR] > 19 ||
      IS_SET(flags, ROOM_DARK | ROOM_UNDERGROUND | ROOM_EARTHY | 
ROOM_UNDERWATER))
    return TRUE;    

  return FALSE;
}


bool 
can_see (THING *viewer, THING *target)
{
  int v_det, tar_aff, v_holy;
  if (!viewer || !target || !viewer->in || !target->in)
    return FALSE;
  if (viewer->in == target || viewer == target)
    return TRUE;
  v_holy = flagbits (viewer->flags, FLAG_PC1);
  
  if (IS_PC1_SET (target, PC_WIZINVIS) &&
      (LEVEL (target) > LEVEL (viewer) ||
       !IS_SET (v_holy, PC_HOLYLIGHT)))
    return FALSE;
  
  if (IS_SET (v_holy, PC_HOLYLIGHT))
    return TRUE;
  
  v_det = flagbits (viewer->flags, FLAG_DET);
  tar_aff = flagbits (target->flags, FLAG_AFF) % AFF_SNEAK;
  
  /* This checks if ALL of the target aff_bits which obscure him
     are on the viewer...if not we return false. */
  
  if ((tar_aff && (tar_aff & ~v_det)) ||
      IS_SET (target->thing_flags, TH_UNSEEN) ||
      (dark_inside (target->in) && !IS_SET (v_det, AFF_DARK)))
    return FALSE;
  
  /* Can't see through fog too easily. */

  if (wt_info->val[WVAL_WEATHER] == WEATHER_FOGGY &&
      IS_ROOM (target->in) && nr (1,4) != 2 &&
      !IS_SET (v_det, AFF_FOGGY) &&
      IS_ROOM_SET (target->in, ROOM_FOREST | ROOM_MOUNTAIN |
		   ROOM_SNOW | ROOM_FIELD | ROOM_SWAMP | 
		   ROOM_AIRY | ROOM_DESERT | ROOM_WATERY |
		   ROOM_ROUGH))
    return FALSE;
     
					   

  return TRUE;
}
      
/* This gives the bits of a certain type that are set in a list
   of flags after the start flag given. */

int
flagbits (FLAG *flg, int type)
{
  int bits = 0;
  if (type >= NUM_FLAGTYPES)
    return 0;
  for (; flg; flg = flg->next)
    if (type == flg->type)
      bits |= flg->val;
  return bits;
}



  
/* This tells whether a thing is named as something or not. */

bool
is_named (THING *th, char *arg)
{
  if (!th || !arg || !*arg)
    return FALSE;
  return (named_in ((char *) KEY(th), arg));
}


/* This capitalizes a word and returns it in another string. We assume
   that the original string may be const! */
    
char *
capitalize (char *arg)
{
  static char ret[STD_LEN*20];
  char *t;
  ret[0] = '\0';
  
  if (!arg || !*arg)
    return ret;
  
  strncpy (ret, arg, STD_LEN*20-1);
  for (t = ret; *t; t++)
    {
      if (*t == '\x1b')
	{
	  while (!isalpha(*t))
	    t++;
	  continue;
	}
      else if (!isalpha (*t))
	{
	  while (!isalpha(*t))
	    t++;
	  if (t > ret)
	    t--;
	  continue;
	}
      *t = UC(*t);
      break;
    }
  return ret;
}


/* This capitalizes all words in a string. We assume that the string
   is not const! This also skips color codes at the start of words. */

void
capitalize_all_words (char *txt)
{
  char *t = txt;
  bool color_code = FALSE;
  if (!txt || !*txt)
    return;

  *t = UC (*t);
  t++;
  for (; *t; t++)
    {
      if (*t == '\x1b')
	{
	  color_code = TRUE;
	  while (*t && *t != 'm' && *t != ' ')
	    t++;
	}
      else
	color_code = FALSE;
      
      if (*(t-1) == ' ' || color_code)
	*t = UC (*t);
    }
  return;
}

int
get_hitroll (THING *th)
{
  int hit = 0;
  THING *weapon;
  VALUE *wpn;

  if (!IS_PC (th))
    return (20 + LEVEL (th));
  
  hit = get_stat (th, STAT_DEX) + LEVEL (th)/3 + 
    3 * implant_rank (th, PART_ARMS) + (th->pc->off_def/2 - 25) +
    th->pc->prac[575 /* Accuracy */]/6 + 
    th->pc->aff[FLAG_AFF_HIT];

  
  for (weapon = th->cont; weapon; weapon = weapon->next_cont)
    {
      if (!IS_WORN(weapon))
	break;
      if (weapon->wear_loc == ITEM_WEAR_WIELD &&
	  (wpn = FNV (weapon, VAL_WEAPON)) != NULL &&
	  wpn->val[2] >= 0 && wpn->val[2] < 4)
	{
	  hit += th->pc->prac[579 + wpn->val[2] /* Wpn skill */]/3;
	  break;
	}
    }

  return hit;
}

int
get_damroll (THING *th)
{
  if (!IS_PC (th))
    return 0;
  
  return 2 + get_stat (th, STAT_STR)/4 + th->pc->aff[FLAG_AFF_DAM] +
    (implant_rank (th, PART_ARMS) + guild_rank (th, GUILD_WARRIOR))/2 +
    (th->pc->off_def - 50)/10;
}


int
get_evasion (THING *th)
{
  if (!IS_PC (th))
    return LEVEL (th)/2 + 20;
  
  return get_stat (th, STAT_DEX) + 2 * guild_rank (th, GUILD_THIEF) +
    (50 - th->pc->off_def)/4 + 
    th->pc->prac[576 /* Parry */]/10 +
    th->pc->prac[577 /* Dodge */]/10 +
    th->pc->prac[587 /* Balance */]/20 +
    th->pc->prac[578 /* Shield block */]/10 +
    th->pc->prac[583 /* Evade */]/20 +
    th->pc->prac[589 /* Steadiness */]/20 +
    th->pc->prac[585 /* Riposte */]/20 +
    th->pc->prac[586 /* Shield bash */]/20 +
    th->pc->prac[590 /* Immovable */]/25 +
    th->pc->prac[588 /* Block */]/25 +
    th->pc->prac[590 /* Blink */]/10 +
    th->pc->prac[591 /* Phase */]/20 +
    th->pc->aff[FLAG_AFF_DEFENSE];
  
}
