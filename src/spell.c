#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"

/* This returns a spell based on name or number. */

SPELL *
find_spell (char *name, int vnum)
{
  SPELL *spl;
  if (vnum > 0)
    {
      for (spl = spell_number_hash[vnum % 256]; spl; spl = spl->next_num)
	{
	  if (spl->vnum == vnum)
	    return spl;
	}
    }
  if (name && *name)
    {
      for (spl = spell_name_hash[LC(*name)]; spl; spl = spl->next_name)
	{
	  if (!str_prefix (name, spl->name))
	    return spl;
	}
    }
  return NULL;
}


char *
prac_pct (THING *th, SPELL *spl)
{
  int pct;
  static char buf[STD_LEN];
  if (!th || !IS_PC (th) || !spl || spl->vnum < 0 || spl->vnum >= MAX_SPELL)
    return "<ERROR!>";
  
  pct = th->pc->prac[spl->vnum];
  
  if (LEVEL (th) >= BLD_LEVEL)
    {
      sprintf (buf, "(%d)", pct);
      if (pct < MAX_TRAIN_PCT)
	strcat (buf, " ");
      if (pct < 10)
	strcat (buf, " ");
      return buf;
    }
  /* THESE STRINGS BELOW HERE MUST ALL BE THE SAME LENGTH OR ELSE THE
     COLUMNS WILL LOOK LIKE CRAP WHEN YOU SLIST SINCE THE WORDS WILL
     HAVE MORE OR LESS SPACE BEFORE THE SECOND COLUMN! */
  if (pct == 0)
    return "(Unlearned)";
  else if (pct < 14)
    return "(Beginner) ";
  else if (pct < 28)
    return "(Poor)     ";
  else if (pct < 44)
    return "(Fair)     ";
  else if (pct < 57)
    return "(Good)     ";
  else if (pct < 71)
    return "(Adept)    ";
  else if (pct < 84)
    return "(Excellent)";
  else if (pct < 99)
    return "(Superb)   ";
  return   "(Godly)    ";
}
  


/* This reads all the spells into the game. */

void
read_spells (void)
{
  
  FILE_READ_SETUP ("spells.dat");
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("SPELL")
	read_spell (f);
      FKEY("END_OF_SPELLS")
	break;
      FILE_READ_ERRCHECK("spells.dat");
    }  
  fclose (f);
  setup_prereqs ();
  return;
}
  



/* This writes all the spells out to a file. */

void
write_spells (void)
{
  SPELL *spl;
  FILE *f;
  
  
  if ((f = wfopen ("spells.dat", "w")) == NULL)
    {
      perror ("spells.dat");
      return;
    }
  
  for (spl = spell_list; spl; spl = spl->next)
    {
      if (spl->level <= MAX_LEVEL)
	write_spell (f, spl);
    }
  fprintf (f, "\nEND_OF_SPELLS\n");
  fclose (f);
  setup_prereqs ();
  return;
}

/* This allocates a new chunk of memory for a spell and returns it. */

SPELL *
new_spell (void)
{
  int i;
  SPELL *newspell;
  
  newspell = (SPELL *) mallok (sizeof (*newspell));
  spell_count++;
  newspell->next = NULL;
  newspell->next_name = NULL;
  newspell->next_num = NULL;
  newspell->name = new_str ("NewSpell");
  newspell->ndam = nonstr;
  newspell->damg = nonstr;
  newspell->duration = nonstr;
  newspell->message = nonstr;
  newspell->message_repeat = nonstr;
  newspell->compname = nonstr;
  newspell->wear_off = new_str ("The spell has worn off @1n.");
  for (i = 0; i < NUM_PRE; i++)
    {
      newspell->pre_name[i] = nonstr;
      newspell->prereq[i] = NULL;
    }
  for (i = 0; i < MAX_PREREQ_OF; i++)
    newspell->prereq_of[i] = 0;
  newspell->mana_amt = 10;
  newspell->mana_type = 0;
  newspell->spell_type = 0;
  newspell->target_type = 0;
  newspell->vnum = 0;
  newspell->transport_to = 0;
  newspell->flags = NULL;
  newspell->level = 1;
  newspell->ticks = 10;
  newspell->position = POSITION_STANDING;
  newspell->creates = 0;
  return newspell;
}

void
read_spell (FILE *f)
{
  SPELL *spl;
  char word[STD_LEN];
  bool found;
  int pre_number = 0, i;
  int bad_read_count = 0;
  spl = new_spell ();
  for (;;)
    {
      found = FALSE;
      strcpy (word, read_word (f));
      switch (UC(word[0]))
	{
	    case 'C':
	      if (!str_cmp (word, "Comp"))
		{
		  spl->comp_lev = read_number (f);
		  for (i = 0; i < MAX_COMP; i++)
		    spl->comp[i] = read_number (f);
		}
	      if (!str_cmp (word, "CompName"))
		{
		  free_str (spl->compname);
		  spl->compname = new_str (read_string (f));
		}
	      break;
	    case 'D':
	      if (!str_cmp (word, "Duration"))
		{
		  spl->duration = new_str (read_string (f));
		  found = TRUE;
		}
	      if (!str_cmp (word, "Damg"))
		{
		  spl->damg = new_str (read_string (f));
		  found = TRUE;
		}
	      break;
	    case 'E':
	      if (!str_cmp (word, "END_SPELL"))
		{
		  SPELL *prev;
		  /* Add spell to the lists and hash tables */
		  if (spell_list == NULL || (spell_list->level > spl->level))
		    {
		      spl->next = spell_list;
		      spell_list = spl;
		    }
		  else 
		    {
		      for (prev = spell_list; prev; prev = prev->next)
			{
			  if (!prev->next || prev->next->level > spl->level)
			    {
			      spl->next = prev->next;
			      prev->next = spl;
			      break;
			    }
			}
		    }
		  spl->next_name = spell_name_hash[(int) LC(*spl->name)];
		  spell_name_hash[(int) LC(*spl->name)] = spl;
		  spl->next_num = spell_number_hash[spl->vnum % 256];
		  spell_number_hash[spl->vnum % 256] = spl;
		  return;
		}  
	      break;
	    case 'F':
	      if (!str_cmp (word, "Flag"))
		{
		  FLAG *newflag;
		  newflag = read_flag (f);
		  newflag->next = spl->flags;
		  spl->flags = newflag;
		}
	      break;	      
	    case 'G':
	      if (!str_cmp (word, "Gen"))
		{
		  spl->spell_type = read_number (f);
		  spl->target_type = read_number (f);
		  spl->mana_amt = read_number (f);
		  spl->mana_type = read_number (f);
		  spl->vnum = read_number (f);
		  spl->level = read_number (f);
		  spl->ticks = read_number (f);
		  spl->position = read_number (f);
		  spl->creates = read_number (f);
		  top_spell = MAX (top_spell, spl->vnum);
		  found = TRUE;
		}
	      if (!str_cmp (word, "Guilds"))
		{
		  for (i = 0; i < GUILD_MAX; i++)
		    spl->guild[i] = read_number (f);
		  found = TRUE;
		}
	      break;	      
	    case 'M':
	      if (!str_cmp (word, "Message"))
		{
		  spl->message = new_str (read_string (f));
		  found = TRUE;
		}
	      if (!str_cmp (word, "MessageRepeat"))
		{
		  spl->message_repeat = new_str (read_string(f));
		  found = TRUE;
		}
	      break;
	    case 'N':
	      if (!str_cmp (word, "Name"))
		{
		  spl->name = new_str (read_string (f));
		  found = TRUE;
		}
	      if (!str_cmp (word, "Ndam"))
		{
		  
		  spl->ndam = new_str (read_string (f));
		  found = TRUE;
		}
	      break;
	    case 'P':
	      if (!str_cmp (word, "Pre_Name"))
		{
		  if (pre_number >= NUM_PRE)
		    read_string (f);
		  spl->pre_name[pre_number] = new_str (read_string (f));
		  pre_number++;
		  found = TRUE;
		}
	    case 'R':
	      if (!str_cmp (word, "Repeat"))
		{
		  spl->repeat = read_number (f);
		  spl->delay = read_number (f);
		  spl->damage_dropoff_pct = read_number (f);
		  found = TRUE;
		}
	    case 'S':
	      if (!str_cmp (word, "Stats"))
		{
		  for (i = 0; i < STAT_MAX; i++)
		    spl->min_stat[i] = read_number (f);
		  found = TRUE;
		}
	      break;
	    case 'T':
	      if (!str_cmp (word, "Transport"))
		{
		  spl->transport_to = read_number (f);
		  found = TRUE;
		}
	      if (!str_cmp (word, "Teachers"))
		{
		  for (i = 0; i < MAX_TEACHER; i++)
		    spl->teacher[i] = read_number(f);
		}
	      break;
	    case 'W':
	      if (!str_cmp (word, "Wear_Off"))
		{
		  spl->wear_off = new_str (read_string (f));
		  found = TRUE;
		}
	      break;	
	    default:
	      bad_read_count++;
	      break;
	}
      if (bad_read_count >= 100)
	{
	  log_it ("Bad read on single spell in spells.dat!");
	  break;
	}
       
    }
  return;
}


void 
write_spell (FILE *f, SPELL *spl)
{
  int i;
  FLAG *flg;
  fprintf (f, "SPELL\n");
  if (spl->name && spl->name[0])
    write_string (f, "Name", spl->name); 
  fprintf (f, "Gen %d %d %d %d %d %d %d %d %d\n",
	   spl->spell_type, spl->target_type, spl->mana_amt, spl->mana_type,
	   spl->vnum, spl->level, spl->ticks, spl->position, spl->creates);
  if (spl->ndam && spl->ndam[0])
    write_string (f, "Ndam", spl->ndam);
  if (spl->damg && spl->damg[0])
    write_string (f, "Damg", spl->damg);
  if (spl->duration && spl->duration[0])
    write_string (f, "Duration", spl->duration);
  if (spl->wear_off && spl->wear_off[0])
    write_string (f, "Wear_Off", spl->wear_off);
  if (spl->message && spl->message[0])
    write_string (f, "Message", spl->message);
  if (spl->message_repeat && spl->message_repeat[0])
    write_string (f, "MessageRepeat", spl->message_repeat);
  if (spl->compname && *spl->compname)
    write_string (f, "CompName", spl->compname);
  fprintf (f, "Comp %d ", spl->comp_lev);
  for (i = 0; i < MAX_COMP; i++)
    fprintf (f, "%d ", spl->comp[i]);
  fprintf (f, "\n");
  fprintf (f, "Repeat %d %d %d\n",
	   spl->repeat, spl->delay, spl->damage_dropoff_pct);
  for (i = 0; i < NUM_PRE; i++)
    {
      if (spl->pre_name[i] && spl->pre_name[i] != nonstr)
	write_string (f, "Pre_Name", spl->pre_name[i]);
    }
  fprintf (f, "Guilds ");
  for (i = 0; i < GUILD_MAX; i++)
    fprintf (f, "%d ", spl->guild[i]);
  fprintf (f, "\n");
  fprintf (f, "Teachers ");
  for (i = 0; i < MAX_TEACHER; i++)
    fprintf (f, "%d ", spl->teacher[i]);
  fprintf (f, "\n");
  fprintf (f, "Stats ");
  for (i = 0; i < STAT_MAX; i++)
    fprintf (f, "%d ", spl->min_stat[i]);
  fprintf (f, "\n");
  if (spl->transport_to)
    fprintf (f, "Transport %d\n", spl->transport_to);
  flg = spl->flags;
  while (flg)
    {
      write_flag (f, flg);
      flg = flg->next;
    }
  fprintf (f, "END_SPELL\n\n");
}


/* This brings the player into the seditor. */


void 
do_sedit (THING *th, char *arg)
{
  SPELL *spl = NULL, *currspell = NULL;
  char arg1[STD_LEN];
  int curr_vnum, new_vnum = 0;  
  if (LEVEL (th) < MAX_LEVEL || !IS_PC (th) || !th->fd)
    {
      stt ("Huh?\n\r", th);
      return;
    }
  if (!*arg)
    {
      stt ("Syntax: sedit <number>, sedit <name>, or sedit create <number>.\n\r", th);
      return;
    }
  if (!str_cmp (arg, "find teacher locations"))
    {
      find_teacher_locations ();
      stt ("Ok, teacher locations set up.\n\r", th);
      return;
    }
  
  if (IS_SET (th->fd->flags, FD_EDIT_PC))
    {
      stt ("You can't edit anything else while you are editing a pc.\n\r", th);
      return;
    }
  if (!str_prefix ("create", arg))
    {
      arg = f_word (arg, arg1);
      if (!*arg || !is_number (arg))
	{ /* Allow for "sedit create" to give the next avail spell if sediting */
          if (th->fd->connected == CON_SEDITING &&
              (currspell = (SPELL *) th->pc->editing) != NULL)
            {
              curr_vnum = currspell->vnum;
              for (new_vnum = curr_vnum; new_vnum < MAX_SPELL; new_vnum++)
                if ((spl = find_spell (NULL, new_vnum)) == NULL)
                  break;
            }
          else
            {
	      stt ("Syntax sedit create <number>.\n\r", th);
	      return;
	    }
        }
      else if ((spl = find_spell (NULL, atoi(arg))) != NULL)
	{
	  stt ("That spell already exists. Editing now.\n\r", th);
	  th->pc->editing = spl;
	  if (th->fd)
	    th->fd->connected = CON_SEDITING;
	  show_spell (th, spl);
	  return;
	}
      else if ((new_vnum = atoi (arg)) >= MAX_SPELL)
	{
	  stt ("That spell number is too big.\n\r",th);
	  return;
	}
      if (new_vnum > 0 && new_vnum < MAX_SPELL)
        {
	  spl = new_spell ();
	  spl->vnum = new_vnum;
	  top_spell = MAX (top_spell, spl->vnum);
	  spl->next = spell_list;
	  spell_list = spl;
	  spl->next_name = spell_name_hash[(int) LC(*spl->name)];
	  spell_name_hash[(int) LC(*spl->name)] = spl;
	  spl->next_num = spell_number_hash[new_vnum % 256];
	  spell_number_hash[new_vnum % 256] = spl;
	  th->pc->editing = spl;
	  if (th->fd)
	    th->fd->connected = CON_SEDITING;
	  show_spell (th, spl);
	  return;
	}
    }
  else 
    {
      if (*arg == '\'')
	arg = f_word (arg, arg1); 
      else
	strcpy (arg1, arg);
      if ((spl = find_spell (arg, atoi(arg1))) != NULL)
	{
	  stt ("Ok, editing this spell.\n\r", th);
	  th->pc->editing = spl;
	  if (th->fd)
	    th->fd->connected = CON_SEDITING;
	  show_spell (th, spl);
	}
      else
	stt ("That spell doesn't exist!\n\r", th);
    }
  return;
}

void
sedit (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  SPELL *spl;
  char *oldarg = arg;
  int value, i, num;
  if (!arg)
    return;
  if (!th || !IS_PC (th) || !th->fd || !th->pc->editing ||
      th->fd->connected != CON_SEDITING || LEVEL (th) < MAX_LEVEL)
    {
      if (IS_PC (th))
	th->pc->editing = NULL;
      if (th->fd)
	th->fd->connected = CON_ONLINE;
      return;
    }
  spl = (SPELL *) th->pc->editing;
  arg = f_word (arg, arg1);
  if (!*arg1)
    {
      show_spell (th, spl);
      return;
    }
  
  
  switch (UC(arg1[0]))
    {
    case 'C':
      if (!str_cmp (arg1, "creates"))
	{
	  if (!is_number (arg))
	    {
	      stt ("Syntax: Creates <number>\n\r", th);
	      return;
	    }
	  spl->creates = atoi (arg);
	  show_spell (th, spl);
	  stt ("Ok, the thing the spell creates is changed.\n\r", th);
	  return;
	}
      if (!str_cmp (arg1, "comp") ||
	  !str_prefix (arg1, "components"))
	{
	  arg = f_word (arg, arg1);
	  if (!str_prefix (arg1, "level"))
	    spl->comp_lev = atoi (arg);
	  else if (is_number (arg1) && (num = atoi (arg1)) >= 1 &&
		   num <= MAX_COMP)
	    spl->comp[num - 1] = atoi (arg);
	  else
	    {
	      sprintf (buf, "Syntax: comp <num> or lev <value>. The <num> is between 1 and %d.\n\r", MAX_COMP);
	      stt (buf, th);
	      show_spell (th, spl);
	      return;
	    }
	  stt ("Ok, component info set.\n\r", th);
	  show_spell (th, spl);
	  return;
	}
      if (!str_cmp (arg1, "compname"))
	{
	  free_str (spl->compname);
	  spl->compname = new_str (arg);
	  stt ("Component name changed.\n\r", th);
	  show_spell (th, spl);
	  return;	  
	}
      break;
    case 'D':
      if (!str_cmp (arg1, "damg"))
	{
	  free_str (spl->damg);
	  spl->damg = new_str (arg);
	  stt ("Spell damage changed.\n\r", th);
	  show_spell (th, spl);
	  return;
	}
      else if (!str_cmp (arg1, "delay"))
	{
	  spl->delay = atoi (arg);
	  stt ("Spell delay changed.\n", th);
	  show_spell (th, spl);
	  return;
	}
      else if (!str_cmp (arg1, "dropoff"))
	{
	  spl->damage_dropoff_pct = MID (0, atoi (arg), 100);
	  stt ("Spell damage dropoff percent changed.\n", th);
	  show_spell (th, spl);
	  return;
	}
      else if (!str_prefix (arg1, "duration"))
	{
	  free_str (spl->duration);
	  spl->duration = new_str (arg);
	  stt ("Spell duration changed.\n\r", th);
	  show_spell (th, spl);
	  return;
	}
      else if (!str_cmp (arg1, "done"))
	{
	  if (th->pc && th->pc->editing)
	    th->pc->editing = NULL;
	  if (th->fd)
	    th->fd->connected = CON_ONLINE;
	  stt ("Ok, all done editing.\n\r", th);
	  setup_prereqs ();
	  return;
	}
      break;    
    case 'F':
      if (!str_cmp (arg1, "flag") || !str_cmp (arg1, "flags"))
	{
	  edit_flags (th, &spl->flags, arg);
	  show_spell (th, spl);
	  return;
	}
      break;
    case 'G':
      if (!str_cmp (arg1, "guilds") || !str_cmp (arg1, "guild"))
	{
	  int guild_num = GUILD_MAX;
	  arg = f_word (arg, arg1);	  
	  if ((guild_num = find_guild_name (arg1)) != GUILD_MAX &&
	      (value = atoi (arg)) >= 0 && value <= GUILD_TIER_MAX)
	    {
	      spl->guild[guild_num] = value;
	      stt ("Guilds changed.\n\r", th);
	      show_spell (th, spl);
	      return;
	    }
	  else
	    {
	      stt ("Guild <guildname> <Tier>\n\r", th);
	      return;
	    }
	}
      break;
    case 'L':
      if (!str_cmp (arg1, "level") || !str_cmp (arg1, "lvl") ||
	  !str_cmp (arg1, "lv") || !str_cmp (arg1, "lev"))
	{
	  SPELL *prev;
	  if (!is_number (arg))
	    {
	      stt ("Syntax: Level <number>\n\r", th);
	      return;
	    }
	  spl->level = atoi (arg);
	  /* Add spell to the lists and hash tables */
	  if (spell_list)
	    {
	      if (spell_list == spl)
		{
		  spell_list = spell_list->next;
		  spl->next = NULL;
		}
	      else
		{
		  for (prev = spell_list; prev; prev = prev->next)
		    {
		      if (prev->next == spl)
			{
			  prev->next = spl->next;
			  spl->next = NULL;
			  break;
			}
		    }
		}
	    }
	  if (spell_list == NULL || (spell_list->level > spl->level))
	    {
	      spl->next = spell_list;
	      spell_list = spl;
	    }
	  else 
	    {
	      for (prev = spell_list; prev; prev = prev->next)
		{
		  if (!prev->next || prev->next->level > spl->level)
		    {
		      spl->next = prev->next;
		      prev->next = spl;
		      break;
		    }
		}
	    }
	  show_spell (th, spl);
	  stt ("Ok, the thing the spell level is changed.\n\r", th);
	  return;
	}
      break;
    case 'M':
      if (!str_cmp (arg1, "message"))
	{
	  free_str (spl->message);
	  spl->message = new_str (arg);
	  stt ("Spell message changed.\n\r", th);
	  show_spell (th, spl);
	  return;
	}
      if (!str_cmp (arg1, "message_repeat") ||
	  !str_cmp (arg1, "mess_rep") ||
	  !str_cmp (arg1, "messrep"))
	{
	  free_str (spl->message_repeat);
	  spl->message_repeat = new_str (arg);
	  stt ("Spell message repeat changed.\n\r", th);
	  show_spell (th, spl);
	  return;
	}
      if (!str_cmp (arg1, "mana"))
	{
	  if (!is_number (arg))
	    {
	      if ((value = find_bit_from_name (FLAG_MANA, arg)) == 0)
		{
		  stt ("Syntax: Mana <amount/type>\n\r", th);
		  return;
		}
	      spl->mana_type ^= value;
	      stt ("Mana type toggled.\n\r", th);
	      show_spell (th, spl);
	      return;
	    }
	  spl->mana_amt = atoi (arg);
	  show_spell (th, spl);
	  stt ("Ok, mana amount changed.\n\r", th);
	  show_spell (th, spl);
	  return;
	}
      break;

    case 'N':
      if (!str_cmp (arg1, "ndam"))
	{
	  free_str (spl->ndam);
	  spl->ndam = new_str (arg);
	  stt ("Spell ndam changed.\n\r", th);
	  show_spell (th, spl);
	  return;
	}
      if (!str_cmp (arg1, "name"))
	{
	  SPELL *prev;
	  if (!*arg)
	    {
	      stt ("Syntax: name <name>\n\r", th);
	      return;
	    }
	  if (spl == spell_name_hash[(int) LC(*spl->name)])
	    {
	      spell_name_hash[(int) LC(*spl->name)] = spell_name_hash[(int) LC(*spl->name)]->next_name;
	      spl->next_name = NULL;
	    }
	  else
	    {
	      for (prev = spell_name_hash[LC((int) *spl->name)]; prev; prev = prev->next_name)
		{
		  if (prev->next_name == spl)
		    {
		      prev->next_name = prev->next_name->next_name;
		      spl->next_name = NULL;
		      break;
		    }
		}
	    }
	  free_str (spl->name);
	  spl->name = new_str (arg);
	  stt ("Spell name changed.\n\r", th);
	  show_spell (th, spl);
	  spl->next_name = spell_name_hash[(int) LC(*spl->name)];
	  spell_name_hash[(int) LC(*spl->name)] = spl;
	  return;
	}
    case 'P':
      if (!str_cmp ("pre", arg1))
	{
	  arg = f_word (arg, arg1);
	  if (is_number (arg1) && atoi(arg1) > 0 && atoi (arg1) <= NUM_PRE)
	    {        
	      free_str (spl->pre_name[atoi (arg1) - 1]);
              spl->pre_name[atoi(arg1) - 1] = nonstr;
              if (*arg)
  	        spl->pre_name[atoi (arg1) - 1] = new_str (arg);
	      show_spell (th, spl);
	      return;
	    }
	  stt ("Syntax: pre <number> <string>\n\r", th);
	  return;
	}
      if (!str_cmp (arg1, "pos") || !str_cmp (arg1, "position"))
	{
	  for (i = 0; i < POSITION_MAX; i++)
	    {
	      if (!str_cmp (arg, position_names[i]))
		{
		  spl->position = i;
		  stt ("Ok, spell position changed.\n\r", th);
		  show_spell (th, spl);
		  return;
		}
	    }
	  stt ("Unknown spell type.\n\r", th);
	  return;
	}
      break;
	case 'R':
	  if (!str_cmp (arg1, "repeat") ||
	      !str_cmp (arg1, "times") ||
	      !str_cmp (arg1, "repeat_times"))
	    {
	      spl->repeat = atoi(arg);
	      stt ("Spell repeat times changed.\n\r", th);
	      show_spell (th, spl);
	      return;
	    }
    case 'S':
      if (!str_cmp (arg1, "stat") || !str_cmp (arg1, "stats"))
	{ 	
	  int stat_num = STAT_MAX;
	  arg = f_word (arg, arg1);
	  for (i = 0; i < STAT_MAX; i++)
	    {
	      if (!str_prefix (arg1, stat_name[i]))
		{
		  stat_num = i;
		  break;
		}
	    }
	  if (stat_num < 0 || stat_num >= STAT_MAX || !is_number (arg) || 
	      (value = atoi (arg)) < 0 || value > MAX_STAT_VAL)
	    {
	      stt ("Stat <name> <value>\n\r", th);
	      return;
	    }
	  spl->min_stat[stat_num] = value;
	  stt ("Stat min changed.\n\r", th);
	  show_spell (th, spl);
	  return;
	}
      break;
    case 'T':
      if (!str_cmp (arg1, "time") || !str_cmp (arg1, "ticks"))
	{
	  if (!is_number (arg))
	    {
	      stt ("Syntax: Time <number>\n\r", th);
	      return;
	    }
	  spl->ticks = atoi (arg);
	  show_spell (th, spl);
	  stt ("Ok, the thing the spell time is changed.\n\r", th);
	  return;
	}
      if (!str_prefix (arg1, "transports"))
	{
	  spl->transport_to = atoi (arg);
	  stt ("Ok, spell transportation destination set. 0 = home, 1 = random, 2 = to caster, 3 = to vict, other = to that room.\n\r", th);
	  show_spell (th, spl);
	  return;
	}
      if (!str_prefix (arg1, "target"))
	{
	  for (i = 0; i < TAR_MAX; i++)
	    {
	      if (!str_cmp (arg, spell_target_types[i]))
		{
		  spl->target_type = i;
		  stt ("Ok, spell target type changed.\n\r", th);
		  show_spell (th, spl);
		  return;
		}
	    }
	  stt ("Unknown spell target type.\n\r", th);
	  return;
	}
      if (!str_cmp (arg1, "type"))
	{
	  for (i = 0; i < SPELLT_MAX; i++)
	    {
	      if (!str_prefix (arg, spell_types[i]))
		{
		  spl->spell_type = i;
		  stt ("Ok, spell type changed.\n\r", th);
		  show_spell (th, spl);
		  return;
		}
	    }
	  stt ("Unknown spell type.\n\r", th);
	  return;
	}
      break;
    case 'V':
      if (!str_cmp (arg1, "vnum"))
	{
	  SPELL *prev;
	  int new_vnum; 
	  if (!is_number (arg))
	    {
	      stt ("Syntax: Vnum <number>\n\r", th);
	      return;
	    }
	  new_vnum = atoi (arg);
	  if (find_spell (NULL, new_vnum) != NULL)
	    {
	      stt ("A spell is already using that vnum, sorry.\n\r", th);
	      return;
	    }
	  if (spl == spell_number_hash[(int) *spl->name])
	    {
	      spell_number_hash[spl->vnum % 256] = spell_number_hash[spl->vnum % 256]->next_num;
	      spl->next_num = NULL;
	    }
	   else
	    {
	      for (prev = spell_number_hash[spl->vnum % 256]; prev; prev = prev->next_num)
		{
		  if (prev->next_num == spl)
		    {
		      prev->next_num = prev->next_num->next_num;
		      spl->next_num = NULL;
		      break;
		    }
		}
	    }
	  spl->vnum = new_vnum;
	  spl->next_num = spell_number_hash[spl->vnum % 256];
	  spell_number_hash[spl->vnum % 256] = spl;
	  show_spell (th, spl);
	  stt ("Ok, the thing the spell vnum is changed.\n\r", th);
	  return;
	}
      break;
    case 'W':
      if (!str_cmp (arg1, "wear_off") || !str_cmp (arg1, "wearoff") ||
	  !str_cmp (arg1, "wearo"))
	{
	  free_str (spl->wear_off);
	  spl->wear_off = new_str (arg);
	  stt ("Ok, wear off set.\n\r", th);
	  show_spell (th, spl);
	  return;
	}
      break;
    }
 
  interpret (th, oldarg);
  return;
}

void
show_spell (THING *th, SPELL *spl)
{
  char buf[STD_LEN];
  char buf2[STD_LEN];
  int i;
  if (!th || !spl)
    return;
  SBIT (server_flags, SERVER_CHANGED_SPELLS);
  if (spl->spell_type < 0 || spl->spell_type >= SPELLT_MAX)
    spl->spell_type = 0;
  if(spl->target_type < 0 || spl->target_type >= TAR_MAX)
    spl->target_type = TAR_SELF;
  stt ("\n\r\x1b[1;31m---------------------------------------------------------------------\x1b[0;37m\n\r", th);
  sprintf (buf, "[\x1b[1;36m%3d\x1b[0;37m] %-27s  Lv \x1b[1;37m%3d\x1b[0;37m     Type: \x1b[1;32m%s\x1b[0;37m\n\r", spl->vnum,
	   spl->name, spl->level, spell_types[spl->spell_type]);
  stt (buf, th);
  buf[0] = '\0';
  for (i = 0; i < NUM_PRE; i++)
    {
      if (spl->pre_name && spl->pre_name[0])
	{
	  sprintf (buf2, "Prereq%d: \x1b[1;33m%-25s\x1b[0;37m ", i + 1, spl->pre_name[i]);
	  strcat (buf, buf2);
	}
    }
  if (buf[0])
    {
      strcat (buf, "\n\r");
      stt (buf, th);
    }
  if (spl->position < 0 || spl->position >= POSITION_MAX)
    spl->position = POSITION_FIGHTING;
  sprintf (buf, "%-34s ManaC: \x1b[1;34m%3d\x1b[0;37m  Target: \x1b[1;31m%-10s\x1b[0;37m\n\rTicks: \x1b[1;33m%3d\x1b[0;37m    Pos: \x1b[1;35m%-10s\x1b[0;37m       Create \x1b[1;32m%6d\x1b[0;37m Trans: \x1b[1;37m%d\x1b[0;37m \n\r", list_flagnames (FLAG_MANA, spl->mana_type), spl->mana_amt, spell_target_types[spl->target_type], spl->ticks, position_names[spl->position], spl->creates, spl->transport_to);
  stt (buf, th);
  sprintf (buf, "Ndam: \x1b[1;37m%-27s\x1b[0;37m  Damg: \x1b[1;37m%s\x1b[0;37m\n\rDur: \x1b[1;37m%-25s\x1b[0;37m     WearO: \x1b[1;37m%s\x1b[0;37m\n\r", 
	   spl->ndam, spl->damg, spl->duration, spl->wear_off);
  stt (buf, th);
  sprintf (buf, "Repeat  Times: \x1b[1;31m%d\x1b[0;37m   Delay: \x1b[1;34m%d\x1b[0;37m   Dropoff \x1b[1;36m%d\x1b[0;37m\n",
	   spl->repeat, spl->delay, spl->damage_dropoff_pct);
  stt (buf, th);
  if (th->pc->prac[spl->vnum] >= MIN_PRAC)
    { 
      sprintf (buf, "Components:    Lev: %d ", spl->comp_lev);
      for (i = 0; i < MAX_COMP; i++)
	{
	  sprintf (buf + strlen (buf), "%d:%d  ", i + 1, spl->comp[i]);
	}
      sprintf (buf + strlen(buf), "  CompName: %s\n\r", spl->compname);
      stt (buf, th);
    }
  sprintf (buf, "Teachers: ");
  for (i = 0; i < MAX_TEACHER; i++)
    sprintf (buf + strlen(buf), "%6d", spl->teacher[i]);
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Message: %s\n\r", spl->message);
  stt (buf, th); 
  sprintf (buf, "MessRep: %s\n\r", spl->message_repeat);
  stt (buf, th);
  sprintf (buf, "MinStats: ");
  for (i = 0; i < STAT_MAX; i++)
    {
      if (spl->min_stat[i])
	{
	  sprintf (buf2, "%s: %d  ", stat_short_name[i], spl->min_stat[i]);
	  buf2[0] = UC (buf2[0]);
	  strcat (buf, buf2);
	}
    }
  strcat (buf, "\n\r");
  stt (buf, th);
  sprintf (buf, "Guilds: ");
  buf2[0] = '\0';
  for (i = 0; i < GUILD_MAX; i++)
    {
      if (spl->guild[i])
	{
	  sprintf (buf2, "%s tier %d  ", guild_info[i].name, spl->guild[i]);
	  buf2[0] = UC (buf2[0]);
	  strcat (buf, buf2);
	}
    }
  if (buf2[0] == '\0')
    strcat (buf, "none.");
  strcat (buf, "\n\r");
  stt (buf, th);
  if (spl->flags)
    stt(show_flags (spl->flags, 0, LEVEL (th) >= BLD_LEVEL), th); 
  stt ("\x1b[1;31m---------------------------------------------------------------------\x1b[0;37m\n\r", th);
  
  return;
}

/* This first checks if you use a spell/skill correctly, (mobs have a
chance based on their level), and then if you do use it successfullly
and are a pc, you get a chance to improve at it. This also gives
you a chance to gain a "learn try" to learn better skills once you've
mastered previous skills. */

bool
check_spell (THING *th, char *name, int vnum)
{
  SPELL *spl;
  int pct;
  char buf[STD_LEN];
  if ((spl = find_spell (name, vnum)) == NULL || !th->in)
    return FALSE;
  
  if (!IS_PC (th))
    pct = MIN (60, LEVEL (th)*2/3-20);
  else if (spl->vnum < 1 || spl->vnum >=  MAX_SPELL ||
	   th->pc->prac[spl->vnum] < MIN_PRAC/2 || 
	   spl->level > LEVEL(th))
    return FALSE;
  else
    pct = th->pc->prac[spl->vnum] + get_stat (th, STAT_LUC)/2 - 8;
  
  if (align_info[th->align])
    pct += align_info[th->align]->relic_amt;
  
  /* Damage makes you worse at things. At full hps you do 100
     pct chance of success. At 0 hps, you have 33pct
     chance of success. */

  if (th->max_hp > 0)
    pct -= (th->max_hp - th->hp) * 33 / th->max_hp;
  
  if (np () > pct)
    return FALSE;
  
  if (!IS_PC (th) || 
      (pct = th->pc->prac[spl->vnum]) < MIN_PRAC/2 ||
     !IS_ROOM (th->in) || !th->in->in ||
      !DIFF_ALIGN (th->in->in->align, th->align) ||
     (th->pc->no_quit < NO_QUIT_PK && nr (1,4) != 3) || CONSID)
    return TRUE;
  
  pct = th->pc->prac[spl->vnum];


  /* Faster pracs once you're (somewhat) higher level than the spell. */
 
  if (LEVEL(th) < (spl->level+3)*15/14 &&
      nr (1,5) == 3 &&
      nr (5,20) > (LEVEL(th) - spl->level) &&
      (pct * (MAX_TRAIN_PCT - pct) < 
       nr (1, MAX_TRAIN_PCT * MAX_TRAIN_PCT)))
    return TRUE;
  
  if (pct < 100)
    {
      th->pc->prac[spl->vnum]++;
      
      sprintf (buf, "\x1b[1;37m*****[You have become better at %s!]*****\x1b[0;37m\n\r", spl->name);
      stt (buf, th);
    }
  
  /* If you're very good at this skill, try to learn another skill based
     on this one. */
  if (pct > MAX_PRAC && nr (1,3) != 2)
    try_to_learn_spell (th, spl);
      
  return TRUE;
}
    
      


void
do_slist (THING *th, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  char gldbuf[200];
  char mnabuf[100];
  char vnumbuf[100];
  char *t, *oldarg = arg;
  int spellt = SPELLT_MAX, mana = 0, guild = GUILD_MAX, i, targett = TAR_MAX;
  SPELL *spl;
  bool no_prereq = FALSE;
  
  if (str_cmp (arg, "all"))
    {
      while (*arg)
	{
	  arg = f_word (arg, arg1);
	  if (!str_cmp (arg1, "noprereq") ||
	      !str_cmp (arg1, "no_prereq") ||
	      !str_cmp (arg1, "no_prereqs"))
	    no_prereq = TRUE;
	  if (mana == 0)
	    {
	      for (i = 0; i < MANA_MAX; i++)
		{
		  if (!str_cmp (arg1, mana_info[i].name))
		    {
		      mana |= (1 << i);
		      break;
		    }
		}
	    }
	  if (targett == TAR_MAX)
	    {
	      for (i = 0; i < TAR_MAX; i++)
		{
		  if (!str_prefix (arg1, spell_target_types[i]))
		    {
		      targett = i;
		      spellt = SPELLT_SPELL;
		      break;
		    }		  
		}
	    }
	  if (guild == GUILD_MAX)
	    {
	      for (i = 0; i < GUILD_MAX; i++)
		{
		  if (!str_cmp (arg1, guild_info[i].name))
		    {
		      guild = i;
		      break;
		    }
		}
	    }
	  if (spellt == SPELLT_MAX)
	    {
	      t = arg1 + strlen (arg1);
	      if (t > arg1 + 1)
		{
		  t--;
		  if (LC (*t) == 's')
		    *t = '\0';
		}
	      for (i = 0; i < SPELLT_MAX; i++)
		{
		  if (!str_prefix (arg1, spell_types[i]))
		    {
		      spellt = i;
		      break;
		    }
		}
	    }
	}
      if (mana == 0 && spellt == SPELLT_MAX && guild == GUILD_MAX &&
	  targett == TAR_MAX)
	{
	  if ((spl = find_spell (oldarg, atoi (oldarg))) == NULL ||
	      (LEVEL (th) < BLD_LEVEL &&
	       IS_SET (flagbits (spl->flags, FLAG_SPELL), SPELL_NO_SLIST)))
	    {
	      stt ("slist [spell/skill/prof] [fire/earth/water] [warrior/thief/wizard], or combinations except only one of spell/skill etc..\n\r", th);
	      return;
	    }
	  else
	    {
	      show_spell_info (th, spl);
	      return;
	    }
	}
    }
  bigbuf[0] = '\0';
 
  add_to_bigbuf ("\n\r\x1b[1;34m-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\x1b[0;37m\n\r");

  for (spl = spell_list; spl; spl = spl->next)
    {
      if (no_prereq)
	{
	  bool has_a_prereq = FALSE;
	  for (i = 0; i < NUM_PRE; i++)
	    {
	      if (spl->prereq[i])
		{
		  has_a_prereq = TRUE;
		  break;
		}
	    }
	  if (has_a_prereq)
	    continue;
	}
      if ((spellt != SPELLT_MAX && spl->spell_type != spellt) ||
	  (targett != TAR_MAX && spl->target_type != targett) ||
	  (mana && !IS_SET (spl->mana_type, mana)) ||
	  (guild != GUILD_MAX && spl->guild[guild] == 0) ||
	  (LEVEL (th) < BLD_LEVEL && 
	   IS_SET (flagbits (spl->flags, FLAG_SPELL), SPELL_NO_SLIST)))
	continue;
      if (guild != GUILD_MAX)
	{
	  sprintf (gldbuf, "\x1b[1;31m(\x1b[0;31m%s tier \x1b[1;37m%d\x1b[1;31m)\x1b[0;37m ", guild_info[guild].name, spl->guild[guild]);
	}
      else
	gldbuf[0] = '\0';
      sprintf (mnabuf, "    ");
      if (spl->spell_type == SPELLT_SPELL)
	{
	  for (i = 0; i < MANA_MAX; i++)
	    {	      
	      if (IS_SET (spl->mana_type, mana_info[i].flagval))
		{
		  sprintf (mnabuf, "\x1b[1;30m(\x1b[1;37m%3d\x1b[0;37m-%s%c\x1b[1;30m)", spl->mana_amt, mana_info[i].help, UC (mana_info[i].name[0]));
		  break;
		}	
	    }
	}
      vnumbuf[0] = '\0';
      if (LEVEL (th) >= BLD_LEVEL)
	{
	  sprintf (vnumbuf, "(\x1b[1;37mV: %3d\x1b[1;34m) ", spl->vnum);
	}
      
      sprintf (buf, "\x1b[1;34m[\x1b[1;37m%2d\x1b[1;34m] %s\x1b[1;36m%-20s\x1b[0;37m %s%s\x1b[1;36m %-15s %s\x1b[0;37m\n\r",
	       spl->level, vnumbuf, spl->name, gldbuf, mnabuf, spl->pre_name[0],
		 spl->pre_name[1]);
      add_to_bigbuf (buf);
    }
  add_to_bigbuf ("\n\r\x1b[1;34m-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\x1b[0;37m\n\n\r");
  send_bigbuf (bigbuf, th);
  return;
}



void
show_spell_info (THING *th, SPELL *spl)
{
  char buf[STD_LEN];
  char buf2[STD_LEN];
  char vnumbuf[100];
  char mnabuf[STD_LEN];
  bool pre0, pre1;
  HELP *hlp;
  int i;
  THING *obj;
  if (!th || !spl)
    return;
  
  vnumbuf[0] = '\0';
  if (LEVEL (th) >= BLD_LEVEL)
    {
      sprintf (vnumbuf, "\x1b[1;34m(\x1b[1;37mV: %d\x1b[1;34m) ", spl->vnum);
    }
  
  sprintf (buf, "\x1b[1;34m[\x1b[1;37m%2d\x1b[1;34m] \x1b[1;36m%-s\x1b[0;37m  \x1b[0;35mis some sort of \x1b[1;37m%s\x1b[0;35m.\x1b[1;37m  %s\x1b[0;37m\n\r", spl->level, spl->name, spell_types[spl->spell_type], vnumbuf);
  stt (buf, th);
  
  if (spl->spell_type == SPELLT_SPELL)
    {
      mnabuf[0] = '\0';
      for (i = 0; i < MANA_MAX; i++)
	{
	  if (IS_SET (spl->mana_type, mana_info[i].flagval))
	    {
	      strcat (mnabuf, mana_info[i].help);
	      strcat (mnabuf, mana_info[i].name);
	      strcat (mnabuf, " ");
	    }
	}
      if (mnabuf[0] == '\0')
	sprintf (mnabuf, "none");
      
      sprintf (buf, "\x1b[0;32mIt requires \x1b[1;37m%d\x1b[1;36m %s\x1b[0;32m mana to cast and is \x1b[0;31m%s\x1b[0;32m.\x1b[0;37m\n\r", spl->mana_amt, mnabuf, spell_target_types[spl->target_type]);
      stt (buf, th);
    }
  pre0 = (spl->pre_name[0] && *spl->pre_name[0]);
  pre1 = (spl->pre_name[1] && *spl->pre_name[1]);
  
  if (pre0 || pre1)
    {
      if (pre0 != pre1)
	{
	  sprintf (buf, "This %s has one prerequisite: \x1b[1;36m%s\x1b[0;37m.\n\r", spell_types[spl->spell_type], (pre0 ? spl->pre_name[0] : spl->pre_name[1]));
	  stt (buf, th);
	}
      else
	{
	  sprintf (buf, "This %s has two prerequisites: \x1b[1;36m%s\x1b[0;37m and \x1b[1;36m%s\x1b[0;37m.\n\r", spell_types[spl->spell_type], spl->pre_name[0], spl->pre_name[1]);
	  stt (buf, th);
	}
    }
  sprintf (buf, "\x1b[1;33mMinStats: ");
  for (i = 0; i < STAT_MAX; i++)
    {
      if (spl->min_stat[i])
	{
	  sprintf (buf2, "%s: %d  ", stat_short_name[i], spl->min_stat[i]);
	  buf2[0] = UC (buf2[0]);
	  strcat (buf, buf2);
	}
    }
  strcat (buf, "\x1b[0;37m\n\r");
  stt (buf, th);
  sprintf (buf, "\x1b[1;37mGuilds: ");
   for (i = 0; i < GUILD_MAX; i++)
    {
      if (spl->guild[i])
	{
	  sprintf (buf2, "%s tier %d  ", guild_info[i].name, spl->guild[i]);
	  buf2[0] = UC (buf2[0]);
	  strcat (buf, buf2);
	}
    }
  strcat (buf, "\x1b[0;37m\n\r");
  stt (buf, th);
  
  if (spl->spell_type != SPELLT_SKILL &&
      spl->spell_type != SPELLT_PROF)
    {
      for (i = 0; i < MAX_COMP; i++)
	{
	  if (spl->comp[i] > 0 &&
	      (obj = find_thing_num (spl->comp[i])) != NULL &&
	      !IS_SET (obj->thing_flags, TH_IS_ROOM |TH_IS_AREA))
	    {
	      sprintf (buf, "This %s requires %s to use.\n\r",
		       spell_types[spl->spell_type],
		       NAME(obj));
	      stt (buf, th);
	    }
	}
    }

  if ((hlp = find_help_name (th, spl->name)) != NULL &&
      hlp->text && *hlp->text)
    {
      stt (HELP_BORDER, th);
      stt (hlp->text, th);
      stt (HELP_BORDER, th);
    }
  return;
}


void
do_sset (THING *th, char *arg)
{
  THING *vict;
  char arg1[STD_LEN];
  SPELL *spl = NULL;
  int value, i;
  bool all = FALSE;
  arg = f_word (arg, arg1);
  if ((vict = find_pc (th, arg1)) == NULL || !IS_PC (vict))
    {
      stt ("That person isn't around to sset.\n\r", th);
      stt ("Syntax: sset <name> <spellname or all> <percent>\n\r", th);
      return;
    }
  arg = f_word (arg, arg1);
  if (!str_cmp (arg1, "all"))
    {
      all = TRUE;
    }
  else if ((spl = find_spell (arg1, atoi (arg1))) == NULL)
    {
      stt ("That spell doesn't seem to exist.\n\r", th);
      stt ("Syntax: sset <name> <spellname or all> <percent>\n\r", th);
      return;
    }
  
  if (!is_number (arg) || (value = atoi (arg)) < 0 || value > MAX_TRAIN_PCT)
    {
      stt ("Please pick a percent from 0 to 100.\n\r", th);
      stt ("Syntax: sset <name> <spellname or all> <percent>\n\r", th);
      return;
    }
  if (spl && spl->vnum < MAX_SPELL)
    {
      vict->pc->prac[spl->vnum] = value;
      stt ("Spell percent set.\n\r", th);
    }
  else if (all)
    {
      for (i = 0; i < MAX_SPELL; i++)
	vict->pc->prac[i] = value;
      stt ("All spells set.\n\r", th);
    }
  else
    stt ("Syntax: sset <name> <spellname or all> <percent>\n\r", th);
  fix_pc (vict);
  return;
}

/* Simple enough, just sets up all prereqs if they exist. */  


void
setup_prereqs (void)
{
  SPELL *spl;
  int i, j;
  char buf[STD_LEN];
  bool has_a_prereq;
  
  /* Clear all "prereq_of" numbers. */
  
  for (spl = spell_list; spl; spl = spl->next)
    {
      for (i = 0; i < MAX_PREREQ_OF; i++)
	spl->prereq_of[i] = 0;
    }

  /* Sets up all prereqs, now go for total spells used etc... */

  for (spl = spell_list; spl != NULL; spl = spl->next)
    {
      for (i = 0; i < NUM_PRE; i++)
	{
	  spl->prereq[i] = find_spell (spl->pre_name[i], 0);
      /* Set spl as one of the things that spl->prereq[i] is a "prereq_of" */
	  if (spl->prereq[i])
	    {
	      for (j = 0; j < MAX_PREREQ_OF; j++)
		{
		  if (spl->prereq[i]->prereq_of[j] == 0)
		    {
		      spl->prereq[i]->prereq_of[j] = spl->vnum;
		      break;
		    }
		}
	      if (j == MAX_PREREQ_OF)
		{
		  sprintf (buf, "Spell %d is a prereq of more than %d spells!",
			   spl->prereq[i]->vnum, MAX_PREREQ_OF);
		  log_it (buf);
		}
	    }
	}
      has_a_prereq = FALSE;
      for (i = 0; i < NUM_PRE; i++)
	{
	  if (spl->prereq[i])
	    has_a_prereq = TRUE;
	}
    }
  
  /* Sets up numbers of prereqs of each type so we dont recalc on the fly. */

  for (spl = spell_list; spl != NULL; spl = spl->next)
    {
      for (i = 0; i < SPELLT_MAX; i++)
	num_prereqs[i] = 0;
      for (i = 0; i < MAX_SPELL; i++)
	used_spell[i] = 0;
      find_num_prereqs (spl);
      for (i = 0; i < SPELLT_MAX; i++)
	spl->num_prereq[i] = num_prereqs[i];
    }
	  
	  
  return;
}


void
find_num_prereqs (SPELL *spl)
{
  int i;
  
  if (!spl || used_spell[spl->vnum])
    return;
  
  used_spell[spl->vnum] = TRUE;
  num_prereqs[spl->spell_type]++;
  
  for (i = 0; i < NUM_PRE; i++)
    {
      if (spl->prereq[i])
	find_num_prereqs(spl->prereq[i]);
    }
  return;
}

void
do_prereq (THING *th, char *arg)
{
  SPELL *spl;
  char buf[STD_LEN];
  int i, slots_avail[SPELLT_MAX], slots_used[SPELLT_MAX], slots_needed[SPELLT_MAX];
  
  if (!IS_PC (th))
    return;
  
  if (!arg || !*arg)
    {
      stt ("pre <spell/skill name>\n\r", th);
      return;
    }
  
  
  if ((spl = find_spell (arg, 0)) == NULL)
    {
      stt ("Prereqs for what spell?\n\r", th);
      return;
    }

  for (i = 0; i < MAX_PRE_DEPTH; i++)
    draw_line[i] = FALSE;
  for (i = 0; i < MAX_SPELL; i++)
    used_spell[i] = FALSE;
  for (i = 0; i < SPELLT_MAX; i++)
    spells_known[i] = 0;
  bigbuf[0] = '\0';
  add_to_bigbuf("\n\r\x1b[1;35m-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\x1b[0;37m\n\r");
  show_prereqs (th, spl, 0);
  
  
  /* Now figure out how many things you know and whether you can learn this
     spell or not */
  
  add_to_bigbuf("\n\r\x1b[1;35m-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\x1b[0;37m\n\n\r");

  /* Find your total spells learned. */
  
  for (i = 0; i < SPELLT_MAX; i++)
    {
      slots_used[i] = total_spells (th, i);
      slots_avail[i] = allowed_spells (th, i) - slots_used[i];
    }
  
  /* List the types of skills */
  
  add_to_bigbuf ("Types:             ");
  for (i = 0; i < SPELLT_MAX; i++)
    {
      sprintf (buf, "%10s", spell_types[i]);
      add_to_bigbuf (buf);
    }

  /* List how many prerequisites the spell has */
  
  add_to_bigbuf("\n\rNum Prereqs:     ");
  
  for (i = 0; i < SPELLT_MAX; i++)
    {
      sprintf (buf, "  \x1b[1;32m%8d\x1b[0;37m",
	       spl->num_prereq[i]);
      add_to_bigbuf(buf);
    }
  
  /* List how many spells you have learned. */
  

  add_to_bigbuf ("\n\rYou already know:");

  for (i = 0; i < SPELLT_MAX; i++)
    {
      sprintf (buf, "  \x1b[1;37m%8d\x1b[0;37m", spells_known[i]);
      add_to_bigbuf(buf);
    }
  
  /* Now find the difference */
  
  
  add_to_bigbuf ("\n\rYou still need:  ");
  
  for (i = 0; i < SPELLT_MAX; i++)
    {
      slots_needed[i] = spl->num_prereq[i] - spells_known[i];
      sprintf (buf, "  \x1b[1;36m%8d\x1b[0;37m", slots_needed[i]);
      add_to_bigbuf(buf);
    }
  
  add_to_bigbuf ("\n\rAvailable slots: ");
  
  for (i = 0; i < SPELLT_MAX; i++)
    {
      sprintf (buf, "  \x1b[1;34m%8d\x1b[0;37m", slots_avail[i]);
      add_to_bigbuf (buf);
    }
  
  add_to_bigbuf ("\n\rDifference:        ");
  
  for (i = 0; i < SPELLT_MAX; i++)
    {
      sprintf (buf, "\x1b[1;3%dm%8d\x1b[0;37m  ", 
	       (slots_needed[i] <= slots_avail[i] ? 2 : 1), 
	       slots_avail[i] - slots_needed[i]);
      add_to_bigbuf (buf);
    }
  add_to_bigbuf ("\n\n\r");
	       
  send_bigbuf (bigbuf, th);
  return;
}

/* This shows an actual line of prereqs for a spell. Note we don't
   show prereqs more than once so this will cause highlevel spell/skills
   to have a "tree" which is incomplete...but still shows all prereqs
   at least once. It's better this way unless you NEVER have spells
   which are prereqs for more than one spell...it gets the output big
   and ugly otherwise. */

void
show_prereqs (THING *th, SPELL *spl, int depth)
{
  int i, pre_num;
  char buf[STD_LEN];
  
  if (!th || !IS_PC (th) || !spl || depth < 0 || depth >= MAX_PRE_DEPTH ||
      used_spell[spl->vnum] == TRUE)
    return;
  if (spl->spell_type >= 0 && spl->spell_type < SPELLT_MAX &&
      th->pc->prac[spl->vnum] > 0)
    spells_known[spl->spell_type]++;
  sprintf (buf, "\x1b[1;31m");
  for (i = 0; i < depth - 1; i++)
    {
      if (draw_line[i])
	strcat (buf, "|..");
      else
	strcat (buf, "...");
    }
  if (depth > 0)
    strcat (buf, "\\__");
  for (i = depth; i < MAX_PRE_DEPTH; i++)
    draw_line[i] = FALSE;
  strcat (buf, "\x1b[1;37m ");
  strcat (buf, spl->name);
  strcat (buf, "\x1b[1;31m  ");

  /* Pad on rhs */
  
  i = (strlen(spl->name) + 3*depth + 1);
  
  for (; i < 63; i++)
    strcat (buf, ".");
  strcat (buf, "\x1b[1;36m");
  strcat (buf, prac_pct (th, spl));
  strcat (buf, "\x1b[0;37m\n\r");
  add_to_bigbuf (buf);
  used_spell[spl->vnum] = TRUE;
  draw_line[depth] = TRUE;
  for (pre_num = 0; pre_num < NUM_PRE; pre_num++)
    if (!spl->prereq[pre_num] || used_spell[spl->prereq[pre_num]->vnum])
      draw_line[depth] = FALSE;
  if (draw_line[depth] && is_prereq_of (spl->prereq[1], spl->prereq[0], 0))
    draw_line[depth] = FALSE;
  if (spl->prereq[0] && !used_spell[spl->prereq[0]->vnum])
    show_prereqs (th, spl->prereq[0], depth + 1);
  
  if (spl->prereq[1] &&  !used_spell[spl->prereq[1]->vnum])
    {
      draw_line[depth] = FALSE;
      show_prereqs(th, spl->prereq[1], depth + 1);
    }
  return;
}
  




	
bool
is_prereq_of (SPELL *pre, SPELL *spl, int depth)
{
  int i;
  
  if (!spl || !pre || depth >= MAX_PRE_DEPTH ||
      used_spell[pre->vnum])
    return FALSE;
  
  if (spl == pre)
    return TRUE;
  
  for (i = 0; i < NUM_PRE; i++)
    {
      if (spl->prereq[i] && 
	  (pre == spl->prereq[i] ||
	   is_prereq_of (pre, spl->prereq[i], depth + 1)))
	return TRUE;
    }
  return FALSE;
}
  
/* This puts the locations of up to 10 teachers into the
   information for each spell. It clears all teacher info 
   Then, it goes down the list of all prototypes and it
   checks all values on those prototypes. For each teacher
   value, it goes through all vals to see if any are the 
   vnums of spells. If so, then that spell is checked to 
   see if it has any space left in its teacher list for 
   the vnum of the mob teacher. If so, it is added to the
   spell. */

void
find_teacher_locations (void)
{
  THING *th;
  VALUE *val;
  SPELL *spl;
  int i, j, k;
  
  for (spl = spell_list; spl != NULL; spl = spl->next)
    {
      for (i = 0; i < MAX_TEACHER; i++)
	spl->teacher[i] = 0;
    }
  
  for (i = 0; i < HASH_SIZE; i++)
    {
      for (th = thing_hash[i]; th != NULL; th = th->next)
	{
	  if (!IS_AREA(th) && (!th->in || !IS_AREA (th->in)))
	    break;
	  for (val = th->values; val; val = val->next)
	    {
	      if (val->type == VAL_TEACHER0 ||
	      val->type == VAL_TEACHER1 ||
		  val->type == VAL_TEACHER2)
		{
		  for (j = 0; j < NUM_VALS; j++)
		    {
		      if ((spl = find_spell (NULL, val->val[j])) != NULL)
			{
			  for (k = 0; k < MAX_TEACHER; k++)
			    {
			      if (spl->teacher[k] == 0)
				{
				  spl->teacher[k] = th->vnum;
				  break;
				}
			    }
			}
		    }
		}
	    }
	}
    }
  write_spells ();
  return;
}

/* This lets a player attempt to learn a spell. What has to happen is that
   the player checks a random skill that this is a prereq of, and if
   the player has gotten all prereqs of that skill up to MAX_PRAC+, then it
   has a chance to "learn the skill" and you get a message saying so.
   Once your learning tries are >= nr (5,15), you learn the new skill and
   get it at a low pct. Will have to watch out for race/align/remort maxes
   in terms of how many spells you can learn. This is Mog's (Justin
   Dynda's) idea. :) */

void
try_to_learn_spell (THING *th, SPELL *spl)
{
  int i, tries;
  /* Which spell choice we try to learn. */
  int num_chose = 0, count = 0, num_choices = 0;
  SPELL *learnspell = NULL;
  bool have_all_prereqs = TRUE;
  char buf[STD_LEN];
  
  if (!th || !IS_PC (th) || !spl)
    return;

/* Get the number of choices to deal with. */
  
  for (count = 0; count < 2; count++)
    {
      for (i = 0; i < MAX_PREREQ_OF; i++)
	{
	  if ((learnspell = find_spell (NULL, spl->prereq_of[i])) == NULL ||
	      learnspell->vnum < 0 || 
	      learnspell->vnum >= MAX_SPELL ||
	      th->pc->nolearn[learnspell->vnum] ||
	      th->pc->prac[learnspell->vnum] > 0)
	    continue;
	  if (count == 0)
	    num_choices++;
	  else if (--num_chose < 1)
	    break;
	}
      
      if (count == 0)
	{
	  if (num_choices < 1)
	    return;
	  num_chose = nr (1, num_choices);
	}
    }
  
  /* Little sanity check. */
  if (!learnspell || learnspell->vnum < 0 || learnspell->vnum >= MAX_SPELL ||
      th->pc->nolearn[learnspell->vnum] ||
      th->pc->prac[learnspell->vnum] > 0)
    return;
  
  /* Make sure that the player has all of the prereqs necessary for
     this skill/spell. */

  for (i = 0; i < NUM_PRE; i++)
    {
      if (!learnspell->prereq[i] ||
	  learnspell->prereq[i]->vnum < 0 ||
	  learnspell->prereq[i]->vnum >= MAX_SPELL)
	continue;
      if (th->pc->prac[learnspell->prereq[i]->vnum] < MAX_PRAC)
	have_all_prereqs = FALSE;
    }

  if (!have_all_prereqs)
    return;
  
  /* Now check guilds. */

  for (i = 0; i < GUILD_MAX; i++)
    {
      if (learnspell->guild[i] > th->pc->guild[i])
	{
	  return;
	}
    }
  
  /* Now check stats. */
  
  for (i = 0; i < STAT_MAX; i++)
    {
      if (learnspell->min_stat[i] > get_stat (th, i))
	return;
    }

  /* Check for how many spells of this type are allowed. */
  if (total_spells (th, learnspell->spell_type) >= 
      allowed_spells (th, learnspell->spell_type))
    return;
  
    

  /* So we have all of the prereqs so give a chance to improve. */
  
  tries = ++th->pc->learn_tries[learnspell->vnum];
  
  if (nr (5,10) >= tries)
    {
      sprintf (buf, "\x1b[1;36m***** [You %s understand  %s!] *****\x1b[0;37m\n\r",
	       (tries < 2 ? "barely" :
		(tries < 3 ? "begin to" :
		 (tries < 4 ? "sort of" :
		  (tries < 5 ? "mostly" : "can")))),
	       learnspell->name);
      stt (buf, th);
    }
  else
    {
      sprintf (buf, "\x1b[1;37m***** [You can now use %s!]  *****\x1b[0;37m\n\r", 
	       learnspell->name);
      stt (buf, th);
      th->pc->learn_tries[learnspell->vnum] = 0;
      th->pc->prac[learnspell->vnum] = MIN_PRAC + 1;
    }
  return;
}
