#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "script.h"

/* If the script_event is NULL, it checks to see if the thing is there.
   If the thing is not there, it just exits, otherwise the thing is
   assumed to be what we are setting. If the script_event IS there, it parses
   the text to find out what thing associated with the script_event we are
   using. */

char *
replace_one_value (SCRIPT_EVENT *script_event, char *arg, THING *thg, bool setting, char op, char *reparg)
{
  static char ret[STD_LEN * 2];
  THING *th = NULL;
  char *s;
  int amount, i;
  char buf[STD_LEN];
  ret[0] = '\0';
  if (!arg || !arg[0] || 
      (setting && (!is_op (op) || !reparg || !*reparg)))
    return ret;
  amount = atoi (reparg);
  
  
  if (thg)
    th = thg;
  else
    {
      if (!script_event)
	return ret;
      while (isspace (*arg))
	arg++;
      if (UC (*arg) == 'U') /* Unknown thing here */
        {
          if (IS_AREA (script_event->runner))
            thg = find_random_thing 
	      (find_random_room 
	       (script_event->runner, FALSE, 0, 0));
          else if (IS_ROOM (script_event->runner))
            thg = find_random_thing (script_event->runner);
          else if (IS_WORN(script_event->runner))
            thg = script_event->runner->in;
          else
            thg = find_random_thing (script_event->runner->in);
        }
      else if (UC (*arg) == 'R')
	{
	  th = script_event->runner;
	  arg++;
	}
      else if (UC (*arg) == 'S')
	{
	  th = script_event->starter;
	  arg++;
	}
      else if (UC (*arg) == 'O')
	{
	  th = script_event->obj;
	  arg++;
	}
      else if (UC (*arg) == 'T')
	{
	  arg++;
	  if (*arg >= '0' && *arg <= '9')
	    {
	      th = script_event->thing[*arg - '0'];
	      arg++;
	    }
	}
      else if (UC (*arg) == 'V')
	{
	  arg++;
	  if (*arg < '0' || *arg > '9')
	    return ret;
	  if (!setting)
	    sprintf (ret, "%d", script_event->val[*arg - '0']);
	  else
	    script_event->val[*arg - '0'] = 
	      do_operation (script_event->val[*arg - '0'], op, amount);
	  
	  return ret;
	}
    }
  if (!th)
    return ret;

  /* If we are setting and this is a PC, send ALL updates! */
  
  if (setting)
    update_kde (th, ~0);
  
  /* Now set stuff for specific things... */
  
  switch (UC (*arg))
    {
    case 'A':
      if (!str_cmp ("ac", arg) && !IS_AREA (th)) 
	{
	  if (!setting)  
	    sprintf (ret, "%d", th->armor);
	  else 
	    th->armor = do_operation (th->armor, op, amount);
	  th->armor = MID (0, th->armor, 5000);
	}
      if (!str_cmp ("al", arg))
	{ 
	  if (!setting) 
	    sprintf (ret, "%d", th->align);
	  else
	    {	
	      int newalign;
	      if((newalign = do_operation (th->align, op, amount)) >= 0 &&
		 newalign < ALIGN_MAX && align_info[newalign])
		th->align = newalign;
	    }
	}
      break;
    case 'B':
      if (!str_cmp ("bl", arg))
	{
	  if (th->fgt && th->fgt->bashlag > 3)
	    {
	      if (th->position == POSITION_STUNNED)
		sprintf (ret, "(Bashed %d)", th->fgt->bashlag - 2);
	      else
		sprintf (ret, "* ");
	    }
	  else
	    ret[0] = '\0';
	}
      break;
    case 'C':
      if (!str_cmp ("cs", arg))
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->cost);
	  else
	    th->cost = do_operation (th->cost, op, amount);
	  th->cost = MID(0, th->cost, 200000);
	}
      
      else if (!str_cmp ("cw", arg))
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->carry_weight);
	}
      else if (!str_cmp ("ca", arg))
	{
	  if (!setting)
	    sprintf (ret, "%d Stone%s, %d Pebble%s",
		     th->carry_weight/WGT_MULT, 
		     th->carry_weight/WGT_MULT == 1 ? "" : "s",
		     th->carry_weight % WGT_MULT,
		     th->carry_weight % WGT_MULT == 1 ? "" : "s");
	}
      if (!str_cmp ("cn", arg))
	{
	  THING *obj;
	  int count = 0;
	  for (obj = th->cont; obj; obj = obj->next_cont)
	    count++;
	  if (!setting)
	    sprintf (ret, "%d", count);
	}
      else if (!str_cmp ("cp", arg))
	{
	  int multi = (MID (0, th->carry_weight/(10*MAX (1, get_stat(th, STAT_STR))), MAX_CARRY_WEIGHT_AMT - 1));
	  strcpy (ret, carry_weight_amt[multi]);
	}
      
      break;
    case 'D':
      if (!str_cmp ("dl", arg))
	{
	  if (th->fgt && th->fgt->delay > 0)
	    {
	      if (!setting)
		sprintf (ret, "[%d]", th->fgt->delay);
	      else
		th->fgt->delay = do_operation (th->fgt->delay, op, amount);
	    }
	  else
	    ret[0] = '\0';
	}
      break; 
      case 'F':
	if (!str_prefix ("f:", arg))
	  {
	    if (setting)
	      {
		sprintf (buf, "%s %s", arg + 2, reparg);
		edit_flags (NULL, &th->flags, buf);
	      }
	    else
	      {	
		int ftype;		
		if ((ftype = find_flagtype_from_name (arg+2)) < 0 ||
		    ftype >= NUM_FLAGTYPES ||
		    flaglist[ftype].whichflag == NULL)
		  return 0;
		
		sprintf (ret, "%d", find_bit_from_name (ftype, reparg));
		
	      }
	  }
	break;
      case 'H':
      if (!str_cmp ("hc", arg))
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->hp);
	  else
	    th->hp = do_operation (th->hp, op, amount);
	  th->hp = MID(1, th->hp, 2000000000);
	}
      
      if (!str_cmp ("hm", arg))
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->max_hp);
	  else
	    th->max_hp = do_operation (th->max_hp, op, amount);
	  th->max_hp = MID(1, th->max_hp, 2000000000);
	}
      
      if (!str_cmp ("hg", arg))
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->height);
	  else
	    th->height = do_operation (th->height, op, amount);
	  th->height = MID(1, th->height, 30000);
	}
      break;
    case 'L':
      if (!str_cmp ("lv", arg))
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->level);
	  else
	    {
	      char errbuf[STD_LEN];
	      th->level = do_operation (th->level, op, amount);
	      if (IS_PC (th))		
		th->level = MID (1, th->level, MAX_LEVEL);
	      else
		th->level = MID (1, th->level, 1000);

		sprintf (errbuf, "SCRIPT %s level set to %d.\n", 
			 NAME (th), th->level);
		log_it (errbuf);
	    }
	}
      if (!str_cmp ("lg", arg))
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->light);
	  else
	    th->light = do_operation (th->light, op, amount);
	  th->light = MIN (th->light, 20000);
	}
      if (!str_cmp (arg, "ld"))
	{
	  if (!setting)
	    strcpy (ret, LONG(th));
	  else
	    {
	      free_str (th->long_desc);
	      th->long_desc = new_str (reparg);
	    }
	}
      break;
    case 'N':
      if (!str_cmp (arg, "nm"))
	{ 
	  /* Only copy first name or it gets BAD for use in scripts. */
	  if (!setting)
	    {
	      f_word (KEY(th), ret);
	    }
	  else
	    {
	      if (IS_PC (th) && !is_valid_name (th->fd, reparg))
		return "";
	      free_str (th->name);
	      th->name = new_str (reparg);
	      if (IS_PC (th))
		{
		  free_str (th->short_desc);
		  th->short_desc = new_str (capitalize (reparg));
		}
	    }
	}
      break;
    case 'O':
      if (!str_cmp ("oc", arg))
	{
	  THING *vict;
	  if ((vict = FIGHTING (th)) != NULL && vict->max_hp > 0)
	    {
	      if (vict->hp >= vict->max_hp)
		sprintf (ret, "[Oppnt: Excellent] ");
	      else if (vict->hp >= vict->max_hp * 2 /3)
		sprintf (ret, "[Oppnt: Good] ");
	      else if (vict->hp >= vict->max_hp /3)
		sprintf (ret, "[Oppnt: Hurt] ");
	      else if (vict->hp >= vict->max_hp /10)
		sprintf (ret, "[Oppnt: Bad] ");
	      else 
		sprintf (ret, "[Oppnt: Awful] ");
	    }
	  else
	    ret[0] = '\0';
	}
      break;
    case 'P':
      if (!str_cmp ("ps", arg))
	{
	  if (!setting)
	    {
	      if (th->position < POSITION_MAX)
		sprintf (ret, "%s", position_names[th->position]);
	      else
		sprintf (ret, "error_pos");
	    }
	  else
	    th->position = MID (1, do_operation (th->position, op, amount), POSITION_MAX - 1);
	}
      break;
    case 'S':
      if (!str_cmp (arg, "sd"))
	{
	  if (!setting)
	    strcpy (ret, NAME(th));
	  else
	    {
	      if (IS_PC (th) && !is_valid_name (th->fd, reparg))
		return "";
	      free_str (th->short_desc);	      
	      if (IS_PC (th))
		*reparg = UC(*reparg);
	      th->short_desc = new_str (reparg);
	      if (IS_PC (th))
		{
		  free_str (th->name);
		  th->name = new_str (reparg);
		}
	    }
	}
      if (!str_cmp ("sx", arg))
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->sex);
	  else
	    th->sex = do_operation (th->sex, op, amount);
	  th->sex = MIN (th->sex, SEX_MAX -1);
	}
      if (!str_cmp ("sz", arg)) 
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->size);
	  else
	    th->size = do_operation (th->size, op, amount);
	  th->size = MIN (th->size, 30000);
	}
      break;
    case 'T':
      if (!str_cmp (arg, "ty"))
	{
	  if (!setting)
	    strcpy (ret, th->type);
	  else
	    {
	      free_str (th->type);
	      th->type = new_str (reparg);
	    }
	}
      if (!str_cmp (arg, "ti"))
	{
	  if (!setting)
	    sprintf (ret, "%d", th->timer);
	  else
	    {
	      th->timer = do_operation (th->timer, op, amount);
	      if (th->timer < 0)
		th->timer = 0;
	    }
	}
      if (!str_cmp ("tk", arg))
	{
	  THING *tank;
	  if (FIGHTING(th) && 
	      (tank = FIGHTING (FIGHTING(th))) != NULL && tank != th &&
	      tank->max_hp > 0)
	    {
	      if (tank->hp >= tank->max_hp)
		sprintf (ret, "[Tank: Excellent] ");
	      else if (tank->hp >= tank->max_hp * 2 /3)
		sprintf (ret, "[Tank: Good] ");
	      else if (tank->hp >= tank->max_hp /3)
		sprintf (ret, "[Tank: Hurt] ");
	      else if (tank->hp >= tank->max_hp /10)
		sprintf (ret, "[Tank: Bad] ");
	      else 
		sprintf (ret, "[Tank: Awful] ");
	    }
	  else
	    ret[0] = '\0';
	}
      break;
    case 'V':
      if (!str_cmp ("vm", arg))
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->max_mv);
	  else
	    th->max_mv = do_operation (th->max_mv, op, amount);
	  th->max_mv = MID (1, th->max_mv, 30000);
	} 
      else if (!str_cmp ("vc", arg))
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->mv);
	  else
	    th->mv = do_operation (th->mv, op, amount);
	  th->mv = MID (1, th->mv, 30000);
	}
      else if (!str_prefix ("v:", arg))
	{
	  VALUE *val;
	  char word[300];
	  char *t, *s;
	  bool setword = FALSE, settimer = FALSE;
	  int num = 0, i;
	  t = arg;
	  
	  while (*t && *t != ':')
	    t++;
	  if (*t != ':')
	    {
	      ret[0] = '\0';
	      break;
	    }
	  t++;
	  word[0] = '\0';
	  s = word;
	  while (*t &&  *t != ':')
	    {
	      *s = *t;
	      s++;
	      t++;
	    }
	  *s = '\0';
	  if (*t != ':' || !word[0])
	    {
	      ret[0] = '\0';
	      break;
	    }
	  t++;
	  if (LC (*t) == 'w')
	    setword = TRUE;
	  else if (LC (*t) == 't')
	    settimer = TRUE;
	  else
	    {
	      num = *t - '0';
	      if (num < 0 || num >= NUM_VALS)
		{
		  ret[0] = '\0';
		  break;
		}
	    }
	  for (i = 0; i < VAL_MAX; i++)
	    if (!str_prefix (word, value_list[i]))
	      break;
	  if (i == VAL_MAX)
	    {
	      ret[0] = '\0';
	      break;
	    }
	  if ((val = FNV (th, i)) == NULL)
	    {
	      val = new_value ();
	      val->type = i;
	      add_value (th, val);
	    }
	  if (setword)
	    {
	      if (!setting)
		strcpy (ret, val->word);
	      else
		{
		  set_value_word (val, reparg);
		}
	    }
	  else if (settimer)
	    {
	      if (!setting)
		sprintf (ret, "%d", val->timer);
	      else
		val->timer = do_operation (val->timer, op, amount);
	      if (val->timer < 0)
		val->timer = 1;
	    }
	  else
	    {
	      
	      if (!setting) 
		sprintf (ret, "%d", val->val[num]);
	      else
		val->val[num] = do_operation (val->val[num], op, amount);
	    }
	}
      break;
    case 'W':
      if (!str_cmp ("wg", arg)) 
	{
	  if (!setting) 
	    sprintf (ret, "%d", th->weight);
	  else
	    th->weight = do_operation (th->weight, op, amount);
	  th->weight = MIN (th->weight, 30000);
	}
    }
  
  if (IS_PC (th) && !ret[0])
    {
      switch (UC (*arg))
	{
	case 'A':
	  if (!str_cmp (arg, "an"))
	    {
	      sprintf (ret, FALIGN(th)->name);
	    }
	  if (!str_cmp (arg, "ap"))
	    {
	      sprintf (ret, "%d", get_damroll (th));
	    }
	  break;
	case 'B':
	  if (!str_cmp (arg, "bk"))
	    {
	      if (!setting)
		sprintf (ret, "%d", th->pc->bank);
	      else
		th->pc->bank = do_operation (th->pc->bank, op, amount);
	      th->pc->bank = MID (0, th->pc->bank, 2000000000);
	    }
	  break;
	case 'E':
	  /* Elem level bonus, elemental power bonus. */
	  if (!str_cmp ("elb", arg) ||
	      !str_cmp ("epb", arg))
	    {
	      bool power_bonus = !str_cmp (arg, "epb");
	      sprintf (ret, "Elemental %s bonus: ",
		       (!power_bonus ? "level" : "power"));
	      for (i = 0; mana_info[i].flagval != 0; i++)
		{
		  sprintf (ret + strlen(ret), "%s: %d", 
			   mana_info[i].app, 
			   (power_bonus ?
			    th->pc->aff[FLAG_ELEM_POWER_START + i] :
			    th->pc->aff[FLAG_ELEM_LEVEL_START + i]));
		}			 		
	    }
	    
	  if (!str_cmp (arg, "ed"))
	    {
	      if (th->pc->editing && th->fd && th->fd->connected == CON_EDITING)
		sprintf (ret, "< %d >", ((THING * ) th->pc->editing)->vnum);
	      else
		ret[0] = '\0';
	    }
	  if (!str_cmp (arg, "ex"))
	    {
	      if (!setting)
		sprintf (ret, "%d", th->pc->exp);
	      else 
		th->pc->exp = do_operation (th->pc->exp, op, amount);
	      th->pc->exp = MID (0, th->pc->exp, 2000000000);
	    }
	  if (!str_cmp (arg, "el"))
	    {
		sprintf (ret, "%d", exp_to_level (LEVEL(th)) - th->pc->exp);
	    }
	  if (!str_cmp (arg, "eb"))
	    {	      
	      sprintf (ret, "%d", get_evasion (th));
	    }
	  break;
	 
	  case 'G':
	    if (!str_prefix ("g:", arg))
	      {
		s = arg + 2;
		buf[0] = '\0';
	      if (!str_cmp (s, "all"))
		{ 
		  sprintf (ret, "Guilds: ");
		  for (i = 0; i < GUILD_MAX; i++)
		    {
		      if (th->pc->guild[i] > 0)
			{
			  sprintf (buf, "\x1b[1;36m%s\x1b[0;37m tier \x1b[1;31m%d\x1b[0;37m  ", guild_info[i].name, th->pc->guild[i]);
			  buf[0] = UC (buf[0]);
			  strcat (ret, buf);
			}
		    }
		  if (buf[0] == '\0')
		    strcat (ret, "none.");
		}
	      else
		{
		  for (i = 0; i < GUILD_MAX; i++)
		    {
		      if (!str_prefix (s, guild_info[i].name))
			break;
		    }		 
		  if (i < GUILD_MAX)
		    {
		      if (setting)
			th->pc->guild[i] = do_operation (th->pc->guild[i], op, amount);
		      else
			sprintf (ret, "%d", th->pc->guild[i]);
		      th->pc->guild[i] = MID (0, th->pc->guild[i], GUILD_TIER_MAX);
		    }	 
		}
	    }
	  break;
	case 'I':
	  if (!str_prefix ("ip:", arg))
	    {
	      s = arg + 3;
	      buf[0] = '\0';
	      if (!str_cmp (s, "all"))
		{ 
		  sprintf (ret, "Implants: ");
		  for (i = 0; i < PARTS_MAX; i++)
		    {
		      if (th->pc->implants[i] > 0)
			{ 
			  sprintf (buf, "\x1b[1;36m%s\x1b[0;37m tier \x1b[1;31m%d\x1b[0;37m  ", parts_names[i], th->pc->implants[i]);
			  buf[0] = UC (buf[0]);
			  strcat (ret, buf);
			}
		    }
		  if (buf[0] == '\0')
		    strcat (ret, "none.");
		}
	      else
		{
		  for (i = 0; i < PARTS_MAX; i++)
		    {
		      if (!str_prefix (s, parts_names[i]))
			break;
		    }		 
		  if (i < PARTS_MAX)
		    {
		      if (setting)
			th->pc->implants[i] = do_operation (th->pc->implants[i], op, amount);
		      else
			sprintf (ret, "%d", th->pc->implants[i]);
		      th->pc->implants[i] = MID (0, th->pc->implants[i], IMPLANT_TIER_MAX);
		    }	 
		}
	    }
	   break;
	case 'M':
	  if (!str_cmp ("mm", arg) || !str_cmp ("mc", arg))
	    {
	      THING *gem;
	      VALUE *gm;
	      bool maxmana = (!str_cmp ("mm", arg) ? TRUE : FALSE);
	      int mana = (maxmana ? th->pc->max_mana : th->pc->mana); 
	      if (!setting)
		{
		  for (gem = th->cont; gem != NULL; gem = gem->next_cont)
		    {
		      if (!IS_WORN(gem) || 
			  gem->wear_loc > ITEM_WEAR_WIELD)
			break;
		      if (gem->wear_loc == ITEM_WEAR_WIELD &&
			  (gm = FNV (gem, VAL_GEM))!= NULL)
			mana += (maxmana ? gm->val[2] : gm->val[1]);
		    }
		  sprintf (ret, "%d", mana);
		}
	    }
	  break;
	case 'O':
	  if (!str_cmp (arg, "ob"))
	    {	      
	      sprintf (ret, "%d", get_hitroll (th));
	    }
	  if (!str_cmp (arg, "ofd"))
	    {
	      sprintf (ret, "%d", th->pc->off_def);
	    }
	case 'P':
	  if (!str_prefix ("pk:", arg))
	    {
	      s = arg + 3;
	      for (i = 0; i < PK_MAX; i++)
		{
		  if (!str_cmp (s, pk_names[i]))
		    break;
		}
	      if (i < PK_MAX)
		{
		  if (!setting)
		    sprintf (ret, "%d", th->pc->pk[i]);
		  else
		    th->pc->pk[i] = do_operation (th->pc->pk[i], op, amount);	  
		  th->pc->pk[i] = MID (0, th->pc->pk[i], 2000000000);
		}
	    }
	  if (!str_prefix ("pt:", arg))
	    {
	      s = arg + 3;
	      if (!str_cmp (s, "all"))
		{
		  sprintf (ret, "Body Parts: (1 is normal) ");
		  for (i = 0; i < PARTS_MAX; i++)
		    {
		      sprintf (buf, "\x1b[1;36m%d \x1b[0;36m%s(s)%s ",
			       th->pc->parts[i], parts_names[i], 
			       (i < PARTS_MAX - 2 ? "," :
				i == PARTS_MAX - 2 ? " and" : "."));
		      strcat (ret, buf);
		    }
		  strcat (ret, "\x1b[0;37m");
		}
	      else
		{
		  for (i = 0; i < PARTS_MAX; i++)
		    {
		      if (!str_prefix (s, parts_names[i]))
			break;
		    }
		  
		  if (i < PARTS_MAX)
		    { /* You can't set parts!!! It's just bad, trust me. :) */ 
		      if (!setting)
			sprintf (ret, "%d", th->pc->parts[i]);
		    }
		}
	    }
	  if (!str_cmp (arg, "pr"))
	    {
	      if (!setting) 
		sprintf (ret, "%d", th->pc->practices);
	      else
		th->pc->practices = do_operation (th->pc->practices, op, amount);
	      th->pc->practices = MID (0, th->pc->practices, 30000);
	    }
	  break;
	case 'Q':
	  /* These have the form qf:<name>:<num 0 - 5> If any piece
	     of this is missing, it just returns and ignores the line. */
	  if (!str_prefix ("qf:", arg))
	    {
	      VALUE *qf;
	      char word[STD_LEN];
	      char *t, *s;
	      int num;
	      int bit_num = -1;
	      
	      t = arg;
	      
	      /* Move to the first : */
	      while (*t && *t != ':')
		t++;
	      if (*t != ':')
		{
		  ret[0] = '\0';
		  break;
		}
	      t++;
	      word[0] = '\0';
	      
	      /* Now copy the word before the next colon into the 
		 word buffer. This is the name of the questflag. */
	      
	      s = word;
	      while (*t && isalnum (*t) && *t != ':')
		{
		  *s = *t;
		  s++;
		  t++;
		}
	      *s = '\0';
	      if (*t != ':' || !word[0])
		{
		  ret[0] = '\0';
		  break;
		}
	      
	      /* Now get which value we're looking at. */

	      t++;
	      num = *t - '0';
	      if (num < 0 || num >= NUM_VALS)
		{
		  ret[0] = '\0';
		  break;
		}
	      
	      t++;
	      /* Now see if there's a bit involved. This happens if
		 there's another : after the value we're looking at. */
	      
	      if (*t == ':')
		{
		  bit_num = 0;
		  t++;
		  if (isdigit (*t))
		    bit_num = *t - '0';
		  else
		    break;
		  t++;
		  if (isdigit(*t))
		    {
		      bit_num *= 10;
		      bit_num += *t - '0';
		    }
		  
		  if (bit_num < 0 || bit_num >= 32)
		    bit_num = -1;

		  /* End of possible bits. */
		}
	      for (qf = th->pc->qf; qf != NULL; qf = qf->next)
		{
		  if (!str_cmp (qf->word, word))
		    break;
		}
	      if (!qf && IS_PC (th))
		{
		  qf = new_value ();
		  qf->type = VAL_QFLAG;
		  set_value_word (qf, word);
		  qf->next = th->pc->qf;
		  th->pc->qf = qf;
		}
	      if (bit_num == -1)
		{
		  if (!setting) 
		    sprintf (ret, "%d", qf->val[num]);
		  else
		    qf->val[num] = do_operation (qf->val[num], op, amount);
		}
	      else /* Look at bits. */
		{
		  if (!setting)
		    {
		      sprintf (ret, "%d", (qf->val[num] & (1 << bit_num)));
		    }
		  else /* Have to set it on or off. */
		    {
		      if (*reparg == '1')
			qf->val[num] |= (1 << bit_num);
		      else
			qf->val[num] &= ~(1 << bit_num);		      
		    }
		}
	    }
	  if (!str_cmp ("qp", arg))
	    {
	      if (!setting)
		sprintf (ret, "%d", th->pc->quest_points);
	      else
		th->pc->quest_points = do_operation (th->pc->quest_points, op, amount);
	    }
	  break;
	case 'R':
	  if (!str_cmp ("rm", arg))
	    {
	      if (!setting) 
		sprintf (ret, "%d", th->pc->remorts);
	      else
		th->pc->remorts = do_operation (th->pc->remorts, op, amount);
	      th->pc->remorts = MID (0, th->pc->remorts, MAX_REMORTS);
	      calc_max_remort (th);
	    }
	  else if (!str_cmp ("rn", arg))
	    {
	      sprintf (ret, "%s", FRACE(th)->name);
	    }
	  else if (!str_cmp ("rc", arg))
	    {
	      if (!setting) 
		sprintf (ret, "%d", th->pc->race);
	      else
		{
		  int newvnum;
		  newvnum = do_operation (th->pc->race, op, amount);
		  if (newvnum >= 0 && newvnum < RACE_MAX &&
		      race_info[newvnum])
		    th->pc->race = newvnum;
		}
	    }
	  break;
	case 'S':
	  if (*arg && !isdigit (*(arg+1)))
	    {
	      if (!str_cmp (arg, "sab"))
		{
		  sprintf (ret, "%d", th->pc->aff[FLAG_AFF_SPELL_ATTACK]);
		}
	      else if (!str_cmp (arg, "shb"))
		{
		  sprintf (ret, "%d", th->pc->aff[FLAG_AFF_SPELL_HEAL]);
		}
	      else if (!str_cmp (arg, "srb"))
		{
		  sprintf (ret, "%d", th->pc->aff[FLAG_AFF_SPELL_RESIST]);
		}
	      else if (!str_cmp (arg, "sta"))
		{
		  sprintf (ret, "%d", th->pc->aff[FLAG_AFF_THIEF_ATTACK]);
		}
	      else if (!str_cmp (arg, "sdf"))
		{
		  sprintf (ret, "%d", th->pc->aff[FLAG_AFF_DEFENSE]);
		}
	      else if (!str_cmp (arg, "skd"))
		{
		  sprintf (ret, "%d", th->pc->aff[FLAG_AFF_KICKDAM]);
		}
	      else if (!str_cmp (arg, "sdr"))
		{
		  sprintf (ret, "%d", th->pc->aff[FLAG_AFF_DAM_RESIST]);
		}
	      else if (!str_cmp (arg, "sga"))
		{
		  sprintf (ret, "%d", th->pc->aff[FLAG_AFF_GF_ATTACK]);
		}
	      else if (!str_cmp (arg, "sgd"))
		{
		  sprintf (ret, "%d", th->pc->aff[FLAG_AFF_GF_DEFENSE]);
		}
	    }
	  else
	    {
	      int vnum;
	      vnum = atoi (arg + 1);
	      if (vnum > 0 && vnum < MAX_SPELL)
		{
		  if (!setting)
		    sprintf (ret, "%d", th->pc->prac[vnum]);
		  else
		    th->pc->prac[vnum] = do_operation (th->pc->prac[vnum], op, amount);
		  th->pc->prac[vnum] = MIN (th->pc->prac[vnum], 100);
		}
	    }
	  break;
	case 'W':
	  if (!str_cmp ("wm", arg))
	    {
	      if (!setting)
		sprintf (ret, "%d", th->pc->wimpy);
	      else
		th->pc->wimpy = do_operation (th->pc->wimpy, op, amount);
	      th->pc->wimpy = MID (0, th->pc->wimpy, 30000);
	    }
	  break;	      
	case 'Z':
	  {
	    char *s;
	    int i;
	    s = arg + 2;
	    for (i = 0; i < STAT_MAX; i++)
	      {
		if (!str_cmp (s, stat_short_name[i]))
		  break;
	      }
	    if (i < STAT_MAX)
	      {
		if (!setting)
		  {
		    if (LEVEL (th) < 15)
		      sprintf (ret, "XX");
		    else
		      sprintf (ret, "%d", th->pc->stat[i]);
		  }
		else
		  th->pc->stat[i] = do_operation (th->pc->stat[i], op, amount);
		th->pc->stat[i] = MID (MIN_STAT_VAL, th->pc->stat[i],
				       MAX_STAT_VAL);
	      }	  
	  }
	  break;
	}
    }
  return ret;
}


int
math_com_line (SCRIPT_EVENT *script_event, char *line)
{
  char being_set[STD_LEN];
  char valstring[STD_LEN];
  char *s = being_set, *t = line, op ;
 
  while (isspace (*t))
    t++;
  if (*t == '@')
    t++;
  while (!is_op (*t) && !isspace (*t) && *t)
    {
      *s = *t;
      s++;
      t++;
    }
  *s = '\0';  
  while (isspace (*t))
    t++;  
  if (is_op(*t))
    op = *t;
  else
    {
      char errbuf[STD_LEN];
      sprintf (errbuf, "Operation missing! %s\n", line);
      log_it (errbuf);
      return SIG_CONTINUE;
    }
  t++;
  sprintf (valstring, "%d", math_parse (replace_values (script_event, t), LEVEL (script_event->runner)));
  replace_one_value (script_event, being_set, NULL, TRUE, op, valstring);
  
  return SIG_CONTINUE;
}


/* This creates a new thing based on a vnum given. It then attempts
   to send it to the runner, or at least to runner->in. */


int
make_com_line (SCRIPT_EVENT *script_event, char *line)
{
  char *t;
  int vnum = 0;
  THING *th;
  char number[STD_LEN];
  /* Get past the "make" */
  
  t = line;
  while (UC(*t) != 'E' && *t)
    t++;
  t++;
  
  t = f_word (t, number);
  
  vnum = atoi (number);
  
  if ((th = create_thing (vnum)) == NULL)
    {
      char errbuf[STD_LEN];
      sprintf (errbuf, "Script error, thing %d does not exist!\n", vnum);
      log_it (errbuf);
      return SIG_CONTINUE;
    }
  
  while (isdigit (*t) || isspace (*t))
    t++;
  if (*t == '>')
    {
      t++;
      while (isspace (*t) && *t)
	t++;
      if (*t == '@' && UC(*(t + 1)) == 'T' && 
	  *(t + 2) >= '0' && *(t + 2) <= '9')
	{
	  script_event->thing[*(t + 2)-'0'] = th;
	}
    }
  
  /* Now put the thing someplace. If the runner is a room, send it there.
     Otherwise if it can be picked up, and the runner can get stuff,
     send it to the runner. Otherwise, send it to runner->in
     otherwise nuke it. */


  if (IS_ROOM (script_event->runner))
    thing_to (th, script_event->runner);
  else if (!IS_SET (th->thing_flags, TH_NO_TAKE_BY_OTHER) &&
	   !IS_SET (script_event->runner->thing_flags,TH_NO_CAN_GET| TH_NO_CONTAIN))
    thing_to (th, script_event->runner);  
  else if (script_event->runner->in)
    thing_to (th, script_event->runner->in);
  else
    free_thing (th);
  return SIG_CONTINUE;
}
  

/* This destroys a specified thing..cannot be runner or a PC. */

int
nuke_com_line (SCRIPT_EVENT *script_event, char *line)
{
  char *t;
  THING *th = NULL, *thg = NULL;
  int vnum = 0, num_digits = 0;
  t = line;

  /* Parse up to "nuke" */

  while (UC(*t) != 'E' && *t)
    t++;
  t++;
  
  while (isspace (*t) && *t)
    t++;
  
  /* See if we nuke a number or not. */

  while (isdigit (*t) && num_digits++ < 10)
    {
      vnum *= 10;
      vnum += *t - '0';
    }
  
  if (vnum == 0)
    {
      if (*t == '@')
	{
	  t++;
	  if (UC(*t) == 'O')
	  th = script_event->obj;
	  else if (UC(*t) == 'T' && isdigit (*(t + 1)))
	    th = script_event->thing[*(t + 1) - '0'];
	}
    }
  else
    {
      for (thg = script_event->runner->cont; thg != NULL; thg = thg->next_cont)
	{
	  if (thg->vnum == vnum)
	    {
	      th = thg;
	      break;
	    }
	}
      if (!th)
	{
	  if (script_event->runner->in)
	    {
	      for (thg = script_event->runner->in->cont; thg != NULL; thg = thg->next_cont)
		{
		  if (thg->vnum == vnum)
		    {
		      th = thg;
		      break;
		    }
		}
	    }
	}
    }
	
  if (th && th != script_event->runner && !IS_PC (th))
    free_thing (th);
  return SIG_CONTINUE;
}


  
/* Syntax wait <num seconds> returns a wait of that many seconds. */

int
wait_com_line (SCRIPT_EVENT *script_event, char *line)
{
  char *t;
  int time = 0;
  char arg1[STD_LEN];
  t = line;
  t = f_word (t, arg1);
  time = atoi (t);

  if (time < 1)
    time = 1;
  return time;
}
      

int
goto_com_line (SCRIPT_EVENT *script_event, char *line)
{
  char *r, *s, *t;
  char labelname[200];
  char arg1[300];
  char arg2[300];
  char buf[STD_LEN];
  bool found = FALSE;
  
  t = line;
  t = f_word (t, labelname);
  t = f_word (t, labelname);
  
  s = script_event->code;
  
  /* See if the label is found someplace after "label" and then go
     and set the current line pointer to just after it. */

  while (*s && !found)
    {

      /* Copy a line of the code into the buffer. And then get
       the first two "words" in that line and if they match the
       label in the goto statement, we have found our place and the
       script_event->start_of_line pointer gets set to the new s and we
       continue coding from there. */

      r = buf;
      while (*s && *s != '\n' && *s != '\r')
	{
	  *r = *s;
	  r++;
	  s++;
	}
      if (*s == '\r' || *s == '\n')
	s++;
      *r = '\0';

      r = buf;
      r = f_word (r, arg1);
      r = f_word (r, arg2);
      
      if (!str_cmp (arg1, "label") && !str_cmp (arg2, labelname))
	{
	  found = TRUE;
	  break;
	}
    }
  if (found)
    {
      while (isspace (*s) && *s)
	s++;
      script_event->start_of_line = s;
      return SIG_CONTINUE;
    }
  return SIG_QUIT;
}



int
do_com_line (SCRIPT_EVENT *script_event, char *line)
{
  THING *doer = NULL;
  char *t;
  char buf[STD_LEN * 2];
  t = line + 2;
  while (isspace (*t) && *t)
    t++;
  buf[0] = '\0';
  if (*t != '@')
    doer = script_event->runner;
  else
    {
      t++;
      if (LC(*t) == 'r')
	doer = script_event->runner;
      else if (LC (*t) == 't')
	{
	  t++;
	  if (*t >= '0' && *t <= '9')
	    doer = script_event->thing[*t - '0'];
	  
	}
      t++;
    }
  
  strcpy (buf, replace_values (script_event, t));
  if (!doer)
    return SIG_CONTINUE;
  interpret (doer, buf);
  return SIG_CONTINUE;
}


int
if_com_line (SCRIPT_EVENT *script_event, char *line)
{
  char *s, *t = line;
  char lhs[STD_LEN];
  char rhs[STD_LEN];
  int less = FALSE, more = FALSE, equals = FALSE, nott = FALSE, outcome = FALSE;
  
  int leftval = 0;
  int rightval = 0;
  int num_skip = 0;
  int num_parens = 0;
  int num_lines = 0;
 
  while (*t && *t != '(')
    t++;
  if (!*t)
    return SIG_CONTINUE;
  t++;
  /* Go until we reach the middle < > = ! thing. */

  s = lhs;
  while (*t && *t != '<' && *t != '!' && *t != '=' && *t != '>')
    {
      *s = *t;
      s++;
      t++;
    }
  *s = '\0';
  
  if (!*t)
    return SIG_CONTINUE;

  /* Get the kind of check. */
  
  if (*t == '!')
    nott = TRUE;
  else if (*t == '<')
    less = TRUE;
  else if (*t == '>')
    more = TRUE;
  else if (*t == '=')
    equals = TRUE;
  else
    return SIG_CONTINUE;
  t++;
  if (*t == '=')
    {
      equals = TRUE;
      t++;
    }
  
  
  /* This is a little more complicated since we need to stop at the parens. */

  s = rhs;
  num_parens = 1;
  for (; *t && num_parens > 0; t++)
    {
      *s = *t;
      if (*t == '(')
	num_parens++;
      else if (*t == ')')
	{
	  num_parens--;
	  if (num_parens < 1)
	    break;
	}
      s++;
    }
  *s = '\0';
  
  if (*t != ')')
    return SIG_CONTINUE;
  
  /* Now get the number of lines skipped at the end..if it's FALSE, we skip
     this many lines and continue execution. */

  t++;
  num_skip = atoi (t);
  if (num_skip < 1)
    num_skip = 1;

  
  /* So now we have parsed the string. We have the lhs operand,
     the rhs operand, the kind of inequality/equality we are checking,
     and the number of lines we will skip if the statement is FALSE 
     Now we need to replace any @t3wp junk with numbers and then math
     parse. */
  

  leftval = math_parse (replace_values (script_event, lhs), LEVEL (script_event->runner));
  rightval = math_parse (replace_values (script_event, rhs), LEVEL (script_event->runner));
  
  if (less)
    {
      if (equals)
	{
	  if (leftval > rightval)
	    outcome = FALSE;
	  else
	    outcome = TRUE;
	}
      else
	{
	  if (leftval >= rightval)
	    outcome = FALSE;
	  else
	    outcome = TRUE;
	}
    }
  else if (more)
    {
      if (equals)
	{
	  if (leftval < rightval)
	    outcome = FALSE;
	  else
	    outcome = TRUE;
	}
      else
	{
	  if (leftval <= rightval)
	    outcome = FALSE;
	  else
	    outcome = TRUE;
	}
    }
  else if (nott)
    {
      if (leftval != rightval)
	outcome = TRUE;
      else
	outcome = FALSE;
    }
  else if (leftval == rightval)
    outcome = TRUE;
  else outcome = FALSE;
  

  /* If it was true, we continue executing the next lines... */
  
  if (outcome)
    return SIG_CONTINUE;
  

  /* if it was false, we skip num_skip lines. */
  
  t = script_event->start_of_line;
  for (; *t; t++)
    {
      if (*t == '\n' || *t == '\r')
	{
	  t++;
	  if (*t == '\n' || *t == '\r')
	    t++;
	  if (++num_lines >= num_skip)
	    {	     
	      script_event->start_of_line = t;
	      break;
	    }
	}
    }
  if (!*t)
    {
      script_event->start_of_line = t;
      return SIG_QUIT;
    }

  return SIG_CONTINUE;
}




int
do_operation (int cval, char op, int amount)
{
  switch (op)
    {
    case '+':
      return cval + amount;
    case '-':
      return cval - amount;
    case '*':
      return cval * amount;
    case '/':
      if (amount == 0)
	return 0;
      else
	return cval/amount;
    case '=':
      return amount;
    case '_':
      return MIN (amount, cval);
    case '^':
      return MAX (amount, cval);
    case '%':
      if (amount < 2)
        return 0;
      return cval % amount;
    }
  return 0;
}


int 
call_com_line (SCRIPT_EVENT *script_event, char *line)
{
  SCRIPT_EVENT *called_script_event;
  char *t;
  CODE *newcode;
  int i;
  char arg1[STD_LEN];
  
  t = line;
  t = f_word (t, arg1);
  
  /* Check if the code we want to execute exists */
  if ((newcode = find_code_chunk (t)) == NULL)
    return SIG_CONTINUE;
  
  /* Cannot have functions called too deep */

  if (script_event->depth >= MAX_SCRIPT_DEPTH)
    return SIG_CONTINUE;

  /* Script_Event is already removed? */
  /* Set up the new script_event */
  
  
  called_script_event = new_script_event ();
  called_script_event->code = new_str (newcode->code);
  called_script_event->trig = NULL;
  called_script_event->starter = script_event->starter;
  called_script_event->runner = script_event->runner;
  called_script_event->called_from = script_event;
  called_script_event->obj = script_event->obj;
  called_script_event->depth = script_event->depth + 1;
  called_script_event->time = script_event->time;
  called_script_event->start_of_line = called_script_event->code;
  script_event->calling = called_script_event;
  for (i = 0; i < 10; i++)
    {
      called_script_event->thing[i] = script_event->thing[i];
      called_script_event->val[i] = script_event->val[i];
    }
  add_script_event (called_script_event);
  return SIG_QUIT;
}



char *
replace_values (SCRIPT_EVENT *script_event, char *txt)     
{
  static char ret[STD_LEN * 3];
  char arg[STD_LEN];
  char *r, *s, *t;
  ret[0] = '\0';
  if (!script_event || !txt || !*txt)
    return ret;
  s = ret;
  for (t = txt; *t;)
    {
      if (*t != '@')
	{
	  *s = *t;
	  s++;
	  t++;
	}
      else 
	{
	  t++;
	  if (*t == '@' || (*t >= '0' && *t <= '9'))
	    *s++ = *t++;
	  else
	    {  /* So we will replace something, so chop off ret and find
		  out what we are replacing. */
	      r = arg;
	      *r = '\0';
	      while (isalnum (*t) || *t == ':')
		*r++ = *t++;
	      *r = '\0';
	      *s = '\0';
	      strcat (ret, replace_one_value (script_event, arg, NULL, FALSE, '=', nonstr));
	      s = ret + strlen (ret);
	      while (isspace (*t))
		*s++ = *t++;
	      *s = '\0';
	    }
	}
    }
  *s = '\0';
  return ret;
}
