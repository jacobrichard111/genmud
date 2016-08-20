#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <unistd.h>
#include "serv.h"



/* This function deals with the process of connecting to the game by
   getting the name of the character and all that stuff. */

void 
connect_to_game(FILE_DESC *fd, char *arg)
{
  char arg1[STD_LEN];
  char buf[STD_LEN];
  THING *th, *room;
  char *oldarg = arg;
  int i;
  arg = f_word (arg, arg1);
  switch (fd->connected)
    {
      default:
      case CON_CONNECT:
	{ /* Arkane wanted a greeting screen so this is the way
	     it was done in Emlen's code and it's simple so here
	     you go. :) */
#ifdef ARKANE 
	  HELP *greeting = NULL;
	  if ((greeting = find_help_name (NULL, "greeting")) != NULL)
	    write_to_buffer  (greeting->text, fd);
	  else
#endif
	    {
	      write_to_buffer("\n\n\n\n\n\n\n\n\r                            ", fd);
	      write_to_buffer (game_name_string, fd);
	      write_to_buffer ("\n\n\n\n\r", fd);
	    }
	  write_to_buffer ("\n\n\r(Name), Create (Name), Quit\n\r", fd);
	  fd->connected = CON_GET_NAME;
	}
	break;	  
      case CON_GET_NAME:
	if (!str_cmp (arg1, "quit"))
	  {
	    close_fd (fd);
	    return;
	  }
	else if (!str_cmp (arg1, "create"))
	  {
	    arg = f_word (arg, arg1);
	    if (IS_SET (server_flags, SERVER_WIZLOCK))
	      {
		write_to_buffer ("Sorry, the game is currently wizlocked.\n\r", fd);
		close_fd (fd);
		return;
	      }
		 
	    /* If you're ascending, you get a new character. */
	    if (!is_valid_name (fd, arg1))
	      {
		write_to_buffer ("\n\n\r(Name), Create (Name), Quit\n\r", fd);
		break;
	      }
	    else
	      {
		if (!fd->th)
		  fd->th = new_thing ();		  
		if (!fd->th)
		  {
		    close_fd (fd);
		    return;
		  }
		fd->th->vnum = PLAYER_VNUM;
		fd->th->hp = 20;
		fd->th->max_hp = 20;
		fd->th->mv = 100;
		fd->th->max_mv = 100;
		fd->th->level = 1;
		fd->th->pc = new_pc ();
		fd->th->fd = fd;
		fd->th->thing_flags = MOB_SETUP_FLAGS;
		fd->th->name = new_str (arg1);
		arg1[0] = UC (arg1[0]);
		fd->th->short_desc = new_str (arg1);
		fd->th->pc->prompt = new_str (DEFAULT_PROMPT);
		/* If even one person deletes because their "brigack"
		   score is not high enough, then the 30 seconds I
		   spent coding this has been worth it. */
		fd->th->pc->pk[PK_BRIGACK] = nr (BRIGACK/2, BRIGACK);
		sprintf (buf, "\n\rIs %s the name you wish to use? [Y/N] ", arg1);
		write_to_buffer (buf, fd);
		fd->connected = CON_CONFIRM_NAME;
		break;
	      }
	  }
	else if (found_old_character (fd, oldarg))
	  {

	    if (IS_SET (server_flags, SERVER_WIZLOCK) &&
		(!fd->th || LEVEL(fd->th) < BLD_LEVEL))
	      {
		write_to_buffer ("Sorry, the game is currently wizlocked.\n\r", fd);
		close_fd (fd);
		return;
	      }
	    if (fd->connected != CON_ONLINE)
	      {	
		write_to_buffer ("\n\n\rEnter your password: ", fd);
		write_to_buffer (no_echo, fd);
		fd->connected = CON_GET_OLD_PWD;
		RBIT (fd->flags, FD_BAD_PASSWORD | FD_BAD_PASSWORD2);
		return;
	      }
	  }
	else
	  {
	    if (fd->r_socket > 0)
	      write_to_buffer ("\n\n\r(Name), Create (Name), Quit\n\r", fd);
	    return;
	  }
	break;
      case CON_CONFIRM_NAME:
	if (!str_cmp (arg1, "y") || !str_cmp (arg1, "yes"))
	  {
	    write_to_buffer ("\n\n\rPlease enter the password you wish to use: ", fd);
	    write_to_buffer (no_echo, fd);
	    fd->connected = CON_GET_NEW_PWD;
	  }
	else
	  {
	    write_to_buffer ("\n\n\rOk, going back to the first login state.\n\r", fd);
	    write_to_buffer ("\n\n\r(Name), Create (Name), Quit\n\r", fd);
	    fd->connected = CON_GET_NAME;
	  }
	break;
      case CON_GET_NEW_PWD:
	if (!check_password (fd, arg1))
	  {
	    write_to_buffer ("\n\n\rThat password is not valid.\n\n\r", fd);
	    write_to_buffer ("Please enter the password you wish to use: ", fd);
	    return;
	    
	  }
	else
	  {
	    free_str (fd->th->pc->pwd);
	    fd->th->pc->pwd = new_str (crypt2 (arg1, fd->th->name));
	    fd->connected = CON_CONFIRM_NEW_PWD;
	    
	    write_to_buffer ("\n\rPlease re-enter the password you wish to use: ", fd);
	  }
	break;
      case CON_CONFIRM_NEW_PWD:
	if (str_cmp (fd->th->pc->pwd, crypt2 (arg1, fd->th->name)))
	  {
	    write_to_buffer ("\n\rYour passwords do not match!!!\n\r", fd);
	    write_to_buffer ("\n\rPlease enter the password you wish to use: ", fd);
	    fd->connected = CON_GET_NEW_PWD;
	    return;
	  }
	else
	  {
	    write_to_buffer ("\n\rPlease enter your email address. This address may be checked.\n\r", fd);
	    write_to_buffer ("If it is not valid, you may be deleted without warning. \n\r", fd);
	    write_to_buffer ("If the email address given here does not match the email address of your\n\r", fd);
	    write_to_buffer ("character for purposes of validation, you will not be validated.\n\r", fd);
	    write_to_buffer ("Do not use free emails like hotmail or yahoo:  ", fd);
	    fd->connected = CON_GET_EMAIL;
	    write_to_buffer (yes_echo, fd);
	  }
	break;
      case CON_GET_EMAIL:
	{
	  PBASE *pb;
	  if (!check_email (fd, arg1))
	    {
	      write_to_buffer ("\n\rPlease enter your email address:  ", fd);
	      break;
	    }
	  free_str (fd->th->pc->email);
	  fd->th->pc->email = new_str (arg1);
	  
	    /* Check email matches */
	  
	  for (pb = pbase_list; pb != NULL; pb = pb->next)
	    {
	      if (!str_cmp (pb->email, fd->th->pc->email))
		{
		  char errbuf[STD_LEN];
		  sprintf (buf, "WARNING: Your email matches that of %s.\n\r", pb->name);
		  write_to_buffer (buf, fd);
		  sprintf (errbuf, "WARNING: %s email matches that of %s.\n",
			   fd->th->name, pb->name);
		  log_it (errbuf);
		}
	    }
	  write_to_buffer ("Please pick your sex: [M]ale, [F]emale, [N]euter.\n\r", fd);
	  fd->connected = CON_GET_SEX;
	}
	break;
      case CON_GET_SEX:
	if (LC (arg1[0]) == 'n')
	  fd->th->sex = 0;
	else if (LC (arg1[0]) == 'f')
	  fd->th->sex = 1;
	else if (LC (arg1[0]) == 'm')
	  fd->th->sex = 2;
	else
	  {
	    write_to_buffer ("Please pick your sex: [M]ale, [F]emale, [N]euter.\n\r", fd);
	    fd->connected = CON_GET_SEX;
	    break; 
	  }
	for (i = 100; i > 2; i--)
	  {
	    sprintf (buf, "%d\n\r", i);
	    write_to_buffer (buf, fd);
	  }
	write_to_buffer ("Please enter the number at the top of your screen, which is your pagelength.\n\r", fd);
	fd->connected = CON_GET_PAGELEN;
	break;
      case CON_GET_PAGELEN:
	if (!is_number (arg1) || atoi (arg1) < 10 || atoi (arg1) > 99)
	  {
	    write_to_buffer ("Pagelength not entered properly. Defaulting to 24 lines.\n\r", fd);
	    fd->th->pc->pagelen = 24;
	  }
	else
	  {
	    sprintf (buf, "Ok, pagelen set to %d lines.\n\r", atoi (arg1));
	    write_to_buffer (buf, fd);
	    fd->th->pc->pagelen = atoi (arg1);
	    }
	
	write_to_buffer ("You will now pick a race from the following list:\n\r", fd);  
	do_race (fd->th, "list");
	write_to_buffer ("Choice: ", fd);
	fd->connected = CON_GET_RACE;
	break;
      case CON_GET_RACE:
	{
	  RACE *race;
	  if (!is_number (arg1) || 
	      (race = find_race (NULL, atoi(arg1))) == NULL ||
	      is_ascended_race(race))
	    {
	      write_to_buffer ("Please pick a race from the following list:\n", fd);
	      do_race (fd->th, "list");
	      write_to_buffer ("Choice: ", fd);
	      return;
	    }	    
	  show_race (fd->th, race);
	  do_help (fd->th, race->name);
	  write_to_buffer ("Is this the race you want? ", fd);
	  fd->th->pc->race = race->vnum;
	  fd->connected = CON_CONFIRM_RACE;
	}
	break;
      case CON_CONFIRM_RACE:
	if (LC (arg1[0]) == 'y')
	  {
	    RACE *race;
	    fd->connected = CON_GET_ALIGN;
	    write_to_buffer ("You will now choose either Whitie or Darkie for your alignment.\n\n\r Darkie is made harder to play.\n\r", fd);
	    do_align (fd->th, "list");
	    race = FRACE(fd->th);
	    fd->th->height = nr (race->height[0],
				 race->height[1]);
	    fd->th->weight = nr (race->weight[0],
				 race->weight[1]);
	    
	    return;
	  }
	write_to_buffer ("Please pick a race from the following list: ", fd);
	do_race(fd->th, "list");
	write_to_buffer ("Choice: ", fd);
	fd->connected = CON_GET_RACE;
	break;
      case CON_GET_ALIGN:
	{
	  RACE *align;
	  if (!is_number (arg1) ||
	      (align = find_align (NULL, atoi(arg1))) == NULL 
	      || align->vnum < 1)
	    {
	      write_to_buffer ("Please pick alignment 1 or 2...0 is for the other stuff.\n\r", fd);
	      do_align (fd->th, "list");
	      write_to_buffer ("Choice: ", fd);
	      return;
	    }	    
	  show_align (fd->th, align);
	  do_help (fd->th, align->name);
	  write_to_buffer ("Is this the align you want? ", fd);
	  fd->th->align = align->vnum;
	  fd->th->pc->in_vnum = 100 + align->vnum;
	  fd->connected = CON_CONFIRM_ALIGN;
	  
	}
	break;
      case CON_CONFIRM_ALIGN:
	if (LC (arg1[0]) == 'y')
	  {
	    /*	      write_to_buffer ("\n\rExcellent! You will now roll your stats.\n\r", fd); */
	    write_to_buffer ("\n\rExcellent! You will now see your stats set.\n\r", fd); 
	    
	    fd->connected = CON_ROLLING;
	      /* roll_stats (fd->th); */
	    set_stats (fd->th);
	    /*	      write_to_buffer ("\n\r[K]eep this char, [C]hoose a new race, any other key to reroll.\n\r", fd); */
	    write_to_buffer ("\n\r[K]eep this char, [C]hoose a new race.\n\r", fd);
	    return;
	  }
	write_to_buffer ("Please pick an alignment besides neutral from the following list: ", fd);
	do_align (fd->th, "list");
	fd->connected = CON_GET_ALIGN;
	break;
      case CON_ROLLING:
	if (LC (*arg1) == 'k')
	  {
	    write_to_buffer ("\n\r-----------------------------------------------------\n\n\r",fd);
	    write_to_buffer ("Excellent: Now, do you want to be a permadeath character?\n", fd);
	    write_to_buffer ("If you choose permadeath, you will only have one life,\n", fd);
	    write_to_buffer ("However, you will do double damage with every attack and\n", fd);
	    write_to_buffer ("take half damage from every attack. If you die for any reason,\n", fd);
	    write_to_buffer ("including lame things like cheating or bugs, you will not be\n", fd);
	    write_to_buffer ("reimbursed. So choose carefully.\n", fd);
	    write_to_buffer ("\n\r-----------------------------------------------------\n\n\r",fd);
	    
	    write_to_buffer ("Permadeath Y or N: ", fd);
	    fd->connected = CON_PERMADEATH;
	    return;
	  }
	if (LC (*arg1) == 'c')
	  {
	    write_to_buffer ("Ok, you will rechoose your race.\n\r", fd);
	    write_to_buffer ("Please pick a race from the following list: ", fd);
	    do_race (fd->th, "list");
	    fd->connected = CON_GET_RACE;
	    return;
	  }
	write_to_buffer ("\n\r[K]eep this char, [C]hoose a new race.\n\r", fd);
	/*
	  fd->connected = CON_ROLLING;
	  roll_stats (fd->th);
	  write_to_buffer ("\n\r[K]eep this char, [C]hoose a new race, any other key to reroll.\n\r", fd); */
	break; 
      case CON_PERMADEATH:
	if (!fd->th || !fd->th->pc)
	  {
	    close_fd (fd);
	    return;
	  }
	if (LC(*arg1) != 'n' && LC (*arg1) != 'y')
	  {  
	    write_to_buffer ("\n\r-----------------------------------------------------\n\n\r",fd);
	      write_to_buffer ("Excellent: Now, do you want to be a permadeath character?\n", fd);
	      write_to_buffer ("If you choose permadeath, you will only have one life,\n", fd);
	      write_to_buffer ("However, you will do double damage with every attack and\n", fd);
	      write_to_buffer ("take half damage from every attack. If you die for any reason,\n", fd);
	      write_to_buffer ("including lame things like cheating or bugs, you will not be\n", fd);
	      write_to_buffer ("reimbursed. So choose carefully.\n", fd);
	      write_to_buffer ("\n\r-----------------------------------------------------\n\n\r",fd);
	      write_to_buffer ("Permadeath Y or N :", fd);
	      return;
	  }
	if (LC (*arg1) != 'y')
	  {
	    write_to_buffer ("Very well, you will not suffer permadeath.\n\r", fd);
	    fd->connected = CON_GO_INTO_SERVER;
	    fd->th->pc->title = new_str ("The Newbie Adventurer!");
	  }
	else
	  {
	    add_flagval (fd->th, FLAG_PC1, PC_PERMADEATH);
	    
	    write_to_buffer ("Very well, you will suffer permadeath.\n\r", fd);
	    fd->connected = CON_GO_INTO_SERVER;
	    fd->th->pc->title = new_str ("The Newbie Adventurer!");
	  }
	/* Check newbie ban */
	buf[0] = '\0';
	if (is_sitebanned (fd, 1))
	  {
	    bool matching_email = FALSE;
	    PBASE *pb;
	    FILE *f;
	    for (pb = pbase_list; pb != NULL; pb = pb->next)
	      {
		if (!str_cmp (pb->email, fd->th->pc->email))
		  {
		    if (!matching_email)
		      {
			sprintf (buf, "EMAIL MATCH: %s with ", fd->th->name);
			matching_email = TRUE;
		      }
		    sprintf (buf + strlen (buf), " %s", pb->name);
		  }
	      }
	    if ((f = wfopen ("valid.txt", "w")) == NULL)
	      {
		write_fd_direct (fd, "Sorry, validation failed, email the admins with your player name for help.\n");
		write_playerfile (fd->th);
		fd->th->fd = NULL;
		free_thing (fd->th);
		fd->th = NULL;
		close_fd (fd);
		return;
	      }
	    
	    if (buf[0])
	      {
		fprintf (f, "Your email matches that of other players, so you will have to send this\n");
		fprintf (f, "message back to our email to get your character validated.\n");
		fprintf (f, "Be sure to explain why your email matches someone else's.\n");
		fprintf (f, buf);
		add_flagval (fd->th, FLAG_PC1, PC_UNVALIDATED);
	      }
	    else
	      { /* Send them a randomized password */
		char pwdbuf[20];
		char *t;
		int i;
		t = pwdbuf;
		for (i = 0; i < 8; i++)
		  {
		    *t = ('a' + nr (0, 25));
		    t++;
		  }
		*t = '\0';
		fd->th->pc->pwd = new_str (crypt2 (pwdbuf, fd->th->name));
		fprintf (f, "Your password for %s is now %s  You can change it once you log in.\n", NAME (fd->th), pwdbuf);
	      }
	    fclose (f);
	    sprintf (buf, "mail %s -s 'Your password' < valid.txt &", fd->th->pc->email);
	    system (buf);
	    write_to_buffer ("Check your email for validation information.\n", fd);
	    write_playerfile (fd->th);
	    fd->th->fd = NULL;
	    free_thing (fd->th);
	    fd->th = NULL;
	    close_fd (fd);
	    return;
	  }
	break;
      case CON_GET_OLD_PWD:
	if (!fd->th || !IS_PC (fd->th) ||
	    !fd->th->pc->pwd)
	  {
	    close_fd (fd);
	    return;
	  }
	
	if (str_cmp (fd->th->pc->pwd, crypt2 (arg1, fd->th->name)))
	  {
	    write_to_buffer ("\n\n\rThat was not correct!\n\r", fd);
	    write_to_buffer ("\n\n\rEnter your password: ", fd);
	    if (IS_SET (fd->flags, FD_BAD_PASSWORD2))
	      {
		write_to_buffer ("\n\rToo many failed password attempts!\n\r", fd);              
		write_to_buffer (yes_echo, fd);
		close_fd (fd);
		return;
	      }
	    else if (IS_SET (fd->flags, FD_BAD_PASSWORD))
	      fd->flags |= FD_BAD_PASSWORD2;
	    else
	      fd->flags |= FD_BAD_PASSWORD;
	    break;
	  }
	sprintf (buf, "\n\n\rWelcome back %s!\n\n\r", fd->th->name);
	write_to_buffer (buf, fd);
	write_to_buffer (yes_echo, fd);
	fd->connected = CON_GO_INTO_SERVER;
	return;
      case CON_GO_INTO_SERVER:
	{
	  char errbuf[STD_LEN];
	  if ((th = fd->th) == NULL
	      || !IS_PC (th) || IS_PC1_SET (th, PC_UNVALIDATED))
	    {
	      if (IS_PC1_SET (th, PC_UNVALIDATED))
		write_to_buffer ("You have not been validated.\n", fd);
	      th->fd = NULL;
	      free_thing (th);
	      fd->th = NULL;
	      close_fd (fd);
	      return;
	    }
	  fd->th->pc->using_kde = IS_SET (fd->flags, FD_USING_KDE);
	  update_kde (fd->th, ~0);
	  sprintf (errbuf, "%s enters the world\n", NAME (fd->th));    
	  log_it (errbuf);
	  add_thing_to_list (th);
	  check_pbase (th);
	  fd->read_buffer[0] = '\0';
	  if (!th->in)
	    {
	      if ((room = find_thing_num (th->pc->in_vnum)) == NULL ||
		  !IS_ROOM (room))
		th->pc->in_vnum = 100 + th->align;
	      if (find_thing_num (th->pc->in_vnum))
		thing_to (th, find_thing_num (th->pc->in_vnum));
	      else
		thing_to (th, find_thing_num (2));
	    }
	  if (th->in)
	    {
	      THING *pet, *petn;
	      for (pet = th->pc->pets; pet; pet = petn)
		{
		  petn = pet->next_cont;
		  pet->next_cont = NULL;
		  pet->prev_cont = NULL;
		  pet->in = NULL;
		  thing_to (pet, th->in);
		  do_follow (pet, NAME (th));
		}
	      th->pc->pets = NULL;
	    }
	  act ("@1n return@s to the world.", th, NULL, NULL, NULL, TO_ALL);
	  update_pkdata (th);
	  if (LEVEL (th) <= 1 && th->pc->remorts == 0 &&
	      !th->cont)
	    {
	      THING *starter;
	      add_flagval (fd->th, FLAG_PC2, PC2_BRIEF | PC2_AUTOEXIT | 
			   PC2_AUTOLOOT | PC2_AUTOSPLIT | PC2_AUTOSAC | 
			   PC2_AUTOGOLD | PC2_TACKLE |
			   PC2_ASSIST | PC2_ANSI);
	      
	      if (IS_PC (fd->th))
		{
		  fd->th->pc->cond[COND_HUNGER] = COND_FULL;
		  fd->th->pc->cond[COND_THIRST] = COND_FULL;
		}
	      
	      for (i = 0; i < NUM_NEWBIE_ITEMS; i++)
		{
		  if ((starter = create_thing (newbie_items[i])) != NULL)
		    thing_to (starter, fd->th);
		}
	    }
	  /* This taxes all money in the player's bank account above
	     10000 coins to give to the player's alignment. */
	  /* The amount is 1 percent of all coins over 10000 per rl day,
	     capped to a max of 7 days. */
	  if (LEVEL (th) < BLD_LEVEL &&
	      th->pc->bank > 10000 && th->align > 0 && 
	      th->align < ALIGN_MAX && align_info[th->align] != NULL)
	    {
	      int tax_amount = 
		(((th->pc->bank-10000)/100)*
		 (MID(0,  current_time - th->pc->last_login, 7*3600*24)))
		/(3600*24);
	      th->pc->bank -= tax_amount;	
	      align_info[th->align]->raw_curr[nr (RAW_NONE + 1, RAW_MAX - 1)] += tax_amount/100;
	      if (nr (1,100) < tax_amount % 100)
		align_info[th->align]->raw_curr[nr(RAW_NONE+1,RAW_MAX-1)]++;
	    }
	  
	  if (!IS_SET (flagbits (th->flags, FLAG_PC2), PC2_ANSI))
	    SBIT (fd->flags, FD_NO_ANSI);
	  fix_pc (th);
	  fd->connected = CON_ONLINE;
	  th->position = POSITION_STANDING;
	  break;
	}
    }
  return;
}
	
/* This sets player stats. */

void
set_stats (THING *th)
{
  int i;
  RACE *race, *align;
  
  if (!th || !IS_PC (th) || 
      (race = FRACE (th)) == NULL || 
      (align = FALIGN(th)) == NULL)
    {
      if (th->fd)
	close_fd (th->fd);
      return;
    }
  
  for (i = 0; i < STAT_MAX; i++)
    {
      th->pc->stat[i] = 18;
      th->pc->stat[i] +=
       (race->max_stat[i]-25+ align->max_stat[i]);
    }
  
  do_attributes (th, "");
}
/* This rolls player stats. */


void
roll_stats (THING *th)
{
  int i;
  RACE *race, *align;
  
  if (!th || !IS_PC (th) || 
      (race = FRACE (th)) == NULL || 
      (align = FALIGN(th)) == NULL)
    {
      if (th->fd)
	close_fd (th->fd);
      return;
    }
  
  for (i = 0; i < STAT_MAX; i++)
    {
      th->pc->stat[i] = MAX(nr (9,20), nr (9,20));
      if (race->max_stat[i] > 25)
	th->pc->stat[i] += nr (0, race->max_stat[i] - 25);
      else if (race->max_stat[i] < 25)
	th->pc->stat[i] -= nr (0, 25 - race->max_stat[i]);
      if (align->max_stat[i] > 0)
	th->pc->stat[i] += nr (0, align->max_stat[i]);
      else if (align->max_stat[i] < 0)
	th->pc->stat[i] -= nr (0, -align->max_stat[i]);
    }
  
  do_attributes (th, "");
      
  return;

}

bool
is_valid_name (FILE_DESC *fd, char *arg)
{
  char *t;
  THING *thg, *ar;
  FILE *f;
  FILE_DESC *fd2;
  int i;
  char filename[STD_LEN];

  if (!arg  || !*arg  || strlen (arg) < 3 || strlen (arg) > 17)
    {
      write_to_buffer ("\n\n\rYour name must be between 3 and 17 characters.\n\r", fd);
      return FALSE;
    }
  
  sprintf (filename, "%s%s", PLR_DIR, capitalize (arg));
  if ((f = fopen (filename, "r")) != NULL)
    {
      write_to_buffer ("Someone is already using that name.\n\r", fd);
      fclose (f);
      return FALSE;
    }
  
  for (fd2 = fd_list; fd2; fd2 = fd2->next)
    {
      if (fd2->th && fd2 != fd && !str_cmp (fd2->th->name, arg))
	{
	  write_to_buffer ("Soemone is already using that name.\n\r", fd);
	  return FALSE;
	}
    }
  
  for (i = 0; i < ALIGN_MAX; i++)
    {
      if (align_info[i] && named_in (arg, align_info[i]->name))
	{
	  write_to_buffer ("You can't use an alignment name.\n\r", fd);
	  return FALSE;
	}
    }
  for (i = 0; i < RACE_MAX; i++)
    {
      if (race_info[i] && named_in (arg, race_info[i]->name))
	{
	  write_to_buffer ("You can't use a race name.\n\r", fd);
	  return FALSE;
	}
    }
  
  
  for (t = arg; *t; t++)
    {
      if (!isalpha (*t))
	{
	  write_to_buffer ("\n\rYour name must be all letters.\n\r", fd);
	  return FALSE;
	}
      *t = LC (*t);
    }
  /* You can't be called the following: */

  if (!str_cmp (arg, "someone") ||
      !str_cmp (arg, "you") ||
      !str_cmp (arg, "me") ||
      !str_cmp (arg, "your") ||
      !str_cmp (arg, "yours") ||
      !str_cmp (arg, "he") ||
      !str_cmp (arg, "she") ||
      !str_cmp (arg, "it") ||
      !str_cmp (arg, "something") ||
      !str_cmp (arg, "him") ||
      !str_cmp (arg, "her") ||
      !str_cmp (arg, "self") ||
      !str_cmp (arg, "here") ||
      !str_cmp (arg, "this") ||
      !str_cmp (arg, "imm") ||
      !str_cmp (arg, "immortal") ||
      !str_cmp (arg, "admin") ||
      !str_cmp (arg, "admins") ||
      !str_cmp (arg, "builder") ||
      !str_cmp (arg, "builders") ||
      !str_cmp (arg, "enemy"))
    {
      write_to_buffer ("You can't use that name.\n\r", fd);
      return FALSE;
    }
  
  /* If you like any of these words, feel free to let players have names
     with these strings in them. */
  if (contains_bad_word (arg))
    {
      write_to_buffer ("\n\n\rDo you think you can pick a name without using language like that? *I knew you could* *smile* :P\n\r", fd);
      return FALSE;
    }
  
  for (thg = thing_hash[PLAYER_VNUM % HASH_SIZE]; thg; thg = thg->next)
    {
      if (IS_PC (thg) && !str_cmp (NAME (thg), arg))
	{
	  write_to_buffer ("Someone else is using that name already.\n\r", fd);
	  return FALSE;
	}
    }
  
  for (ar = the_world->cont; ar != NULL; ar = ar->next_cont)
    {
      for (thg = ar->cont; thg != NULL; thg = thg->next_cont)
	{
	  if (IS_ROOM (thg))
	    continue;
	  
	  if (is_named (thg, arg))
	    {
	      write_to_buffer ("\n\rThat name is being used by something in the game already!\n\n\r", fd);
	      return FALSE;
	    }
	}
    }
  return TRUE;
}

bool
contains_bad_word (char *arg)
{
  if (!arg || !*arg)
    return FALSE;
  
  if (strstr (arg, "fuc") ||
      strstr (arg, "fuk") || 
      strstr (arg, "fuq") ||       
      strstr (arg, "shit") ||
      strstr (arg, "damn") ||
      strstr (arg, "cock") ||
      strstr (arg, "peni") ||
      strstr (arg, "vagin") ||
      strstr (arg, "pussy") ||
      strstr (arg, "cunt") ||
      strstr (arg, "jew") ||
      strstr (arg, "jesu") ||
      strstr (arg, "alla") ||
      strstr (arg, "god") ||
      strstr (arg, "nig") ||
      strstr (arg, "kike") ||
      strstr (arg, "spic") ||
      strstr (arg, "chink") ||
      strstr (arg, "fag") ||
      strstr (arg, "lesb") ||
      strstr (arg, "dyke") ||
      strstr (arg, "gay") ||
      strstr (arg, "bitch") ||
      strstr (arg, "biat") ||
      strstr (arg, "asshole") ||
      strstr (arg, "moron") ||
      strstr (arg, "stupid") ||
      strstr (arg, "xus") ||
      strstr (arg, "suck") ||
      strstr (arg, "wtf") ||
      strstr (arg, "jr"))
    return TRUE;
  return FALSE;
}


bool
found_old_character (FILE_DESC *fd, char *arg)
{
  char arg1[STD_LEN];
  char word[STD_LEN];
  char *t;
  THING *th;
  int i;
  CLAN *cln;

  if (!fd)
    return FALSE;
  
  if (fd->th)
    return TRUE;
  
  arg = f_word (arg, arg1);
  
  /* Check to make sure the "name" is legitimate...at least in terms
     of having proper length and being made up only out of characters. */
  
  if (strlen (arg1) < 3 || strlen (arg1) > 17)
    return FALSE;
  for (t = arg1; *t; t++)
    if (!isalpha (*t))
      return FALSE;
  
  /* First, check to see if a file descriptor with that name attached
     to its thing exists. If so, the old fd gets booted off, and the
     new fd takes over. */  
  
  for (th = thing_hash[PLAYER_VNUM % HASH_SIZE]; th; th = th->next)
    {
      if (!IS_PC (th) || str_cmp (arg1, th->name))
	continue; 
      if (!th->fd)
        {
          th->fd = fd;
          th->fd->th = th;
          return TRUE;
        }
      if (!str_cmp (th->pc->pwd, crypt2 (arg, th->name)))
	{
	  fd->th = th;
	  if (th->fd)
	    {
	      th->fd->th = NULL;
	      close_fd (th->fd);
	    }
	  th->fd = fd;
	  stt ("Reconnecting!\n\r", fd->th);
	  fd->connected = CON_ONLINE;
	  act ("@1n has reconnected!", fd->th, NULL, NULL, NULL, TO_ROOM);
	  update_kde (th, ~0);
	  return TRUE;
	}
      else
	{
	  write_fd_direct (fd, "They are already online. Type the name and password all on one line to break in.\n\r");
	  sprintf (word, "ONLINE %s tried to log in %s.", fd->num_address, arg1);
	  log_it (word);
	  close_fd (fd);
	  return FALSE;
	}
      break;
    }
  if (fd->th)
    return TRUE;
  
  /* Now we know that they are not online. So, check if a character
     with this name even exists in the player files. */
  
  if ((fd->th = read_playerfile (capitalize(arg1))) == NULL)
    return FALSE;
  
  
  /* Now check what clans the player is in. */
  if (IS_PC (fd->th))
    {
      for (i = 0; i < CLAN_MAX; i++)
	if ((cln = find_clan_in (fd->th, i, FALSE)) != NULL)
	  fd->th->pc->clan_num[i] = cln->vnum;
    }										  
  fd->th->fd = fd;
  find_max_mana (fd->th);

  return TRUE;
  
}

/* This just opens a pfile and writes into it. */
void
write_playerfile (THING *th)
{
  char filename[100];
  FILE *f;
  if (!th->name || !th->name[0] || !IS_PC (th) || 
      (th->fd && (th->fd->connected > CON_ONLINE && 
		  th->fd->connected < CON_PERMADEATH)) || 
      !th->in || !IS_ROOM (th->in))
    return; 
  
  sprintf (filename, "%s%s", PLR_DIR, capitalize (th->name));
  if ((f = fopen (filename, "w")) == NULL)
    {
      perror (filename);
      return;
    }
  SBIT (server_flags, SERVER_WRITE_PFILE);
  strcpy (pfile_name, NAME (th));
  write_thing (f, th); 
  pfile_name[0] = '\0';  
  RBIT (server_flags, SERVER_WRITE_PFILE); 
  fclose (f);  
  return;
}
    

bool 
check_password (FILE_DESC *fd, char *txt)
{
  char *t;
  bool digit = FALSE;
  bool symbol = FALSE;
  bool cap_letter = FALSE;
  bool lower_letter = FALSE;
  bool bad_symbol = FALSE;
  int length = 0;
  
  for (t = txt; *t; t++)
    {
      length++;
      if (*t >= '0' && *t <= '9')
	digit = TRUE;
      if (*t >= 'A' && *t <= 'Z')
	cap_letter = TRUE;
      if (*t >= 'a' && *t <= 'z')
	lower_letter = TRUE;
      if (!isalnum (*t))
	symbol = TRUE;
      if (*t == END_STRING_CHAR)
	bad_symbol = TRUE;
      if (isspace (*t))
	break;
      if (length > 15)
	break;
    }
  
  if (length < 6 || length > 10)
    {
      write_to_buffer ("Your password must be between 6 and 10 characters.\n\r", fd);
      return FALSE;
    }
  if ((!digit && !lower_letter && !symbol && !cap_letter) || bad_symbol)
    {
      write_to_buffer ("Your password must contain something besides all lowercase letters.\n\r", fd);
      return FALSE;
    }
  return TRUE;
}



bool
check_email (FILE_DESC *fd, char *txt)
{
  char *t;
  bool has_at = FALSE;
  if (strstr ("hotmail", txt) || strstr ("yahoo", txt) ||
      strstr ("deja", txt))
    {
      write_to_buffer ("You cannot use a free email.\n\r", fd);
      return FALSE;
    }
  if (strlen (txt) < 8 || strlen (txt) > 35)
    {
      write_to_buffer ("Your email must be between 8 and 35 characters.\n\r", fd);
      return FALSE;
    }
  for (t = txt; *t; t++)
    {
      if (*t == END_STRING_CHAR)
	{
	  char buf[400];
	  sprintf (buf, "You cannot use the %c character in your email address.\n\r", END_STRING_CHAR);
	  write_to_buffer (buf, fd);
	  return FALSE;
	}
      if (*t == '@')
        {
          if (t - txt < 2)
            {
              write_to_buffer ("I doubt your username is less than 2 letters long. If so, email me.\n\r", fd);
              return FALSE;
            }
          if ((txt + strlen(txt) - t) < 5)
            {
              write_to_buffer ("I doubt your domain name has less than 5 characters total. If so email me instead.\n\r", fd);
              return FALSE;
            }
          if (has_at)
            { 
              write_to_buffer ("Too many @'s\n\r", fd);
              return FALSE;
            }
          has_at = TRUE;
        }
    }
if (!has_at)
  {
    write_to_buffer ("You don't have an '@' in your email.\n\r", fd);
    return FALSE;
  }
return TRUE;
}

/* This saves players to disk. Typing save all will save every
   player in the game if the person doing it is an admin. */

void
do_save (THING *th, char *arg)
{
  THING *thg;

  if (IS_PC (th) && LEVEL (th) == MAX_LEVEL && !str_cmp (arg, "all"))
    {
      for (thg = thing_hash[PLAYER_VNUM % HASH_SIZE]; thg; thg = thg->next)
	if (IS_PC (thg))
	  write_playerfile (thg);
      stt ("Ok, all players saved.\n\r", th);
      return;
    }
  /* You can only save once every 3 minutes. */
  if (IS_PC (th) && 
      (current_time - th->pc->last_saved > 180 ||
       LEVEL(th) >= BLD_LEVEL))
    {
      th->pc->last_saved = current_time;
      write_playerfile (th);
    }
  return;
}

void
fix_pc (THING *th)
{
  int i, fightflags;
  THING *equip;
  VALUE *armor;
  
  
  if (!IS_PC (th))
    return;
  
  /* Fix damage_rounds and damage_total so they don't roll over too
     badly. This is an approximate record of your player's power...
     not exact. */
  
  find_max_mana (th);
  
  if (th->pc->damage_total > 1000000 && th->pc->damage_rounds >= 10)
    {
      th->pc->damage_total /= 10;
      th->pc->damage_rounds /= 10;
    }
  
  for (i = 0; i < AFF_MAX; i++)
    th->pc->aff[i] = 0;
  for (i = 0; i < PARTS_MAX; i++)
    {
      th->pc->armor[i] = 0;
      th->pc->parts[i] = FRACE(th)->parts[i] + FALIGN(th)->parts[i];
    }
  th->armor = 0;
  
  for (i = 0; i < NATSPELL_MAX; i++)
    {
      th->pc->prac[FRACE(th)->nat_spell[i]] = 100;
      th->pc->prac[FALIGN(th)->nat_spell[i]] = 100;      
    }
  
  modify_from_flags (th, FRACE(th)->flags, TRUE);
  modify_from_flags (th, FALIGN (th)->flags, TRUE);
  modify_from_flags (th, th->flags, FALSE);
  for (equip = th->cont; equip != NULL; equip = equip->next_cont)
    {
      if (!IS_WORN (equip) &&
	equip->wear_loc != ITEM_WEAR_BELT)
        continue;
      if ((armor = FNV (equip, VAL_ARMOR)) != NULL)
	{
	  for (i = 0; i < PARTS_MAX; i++)
            /* Add armor to pc's taking damage into account. */
	    th->pc->armor[i] += (armor->val[i]*
            (equip->max_hp > 0 ? 100*equip->hp/equip->max_hp : 100))/100;
	}
      modify_from_flags (th, equip->flags, FALSE);
    }
  

  fightflags = flagbits (th->flags, FLAG_PC2);
    
  find_max_mana (th);
  calculate_warmth (th); 
  set_up_move_flags (th);
  update_kde (th, ~0);
  return;
}


/* This modifies a thing based on a list of flags. For now, you can
   only get regular "Affects" from these flags, and you can also
   get stuff like stat modifiers etc...from them..but the perma affects
   are different. */


void
modify_from_flags (THING *th, FLAG *startflag, bool add_perm_affects)
{
  FLAG *flg, *pcflag;
  
  if (!IS_PC (th))
    return;
  
  
  for (flg = startflag; flg != NULL; flg = flg->next)
    {
      if (flg->type >= AFF_MAX)
	continue;
      
      /* These are where affects are added like flying, invis, dinvis etc. */
      /* This only works from races and aligns atm. */
      
      if (add_perm_affects &&
	  flg->type >= FLAG_HURT && flg->type <= FLAG_DET)
	  {
	    for (pcflag = th->flags; pcflag != NULL; pcflag = pcflag->next)
	      {
		if (pcflag->timer == 0 && pcflag->type == flg->type)
		  {
		    pcflag->val |= flg->val;
		    break;
		  }
	      } /* If we don't have a permanent flag of that kind, then
		 make one. */
	    if (!pcflag)
	      {
		pcflag = new_flag ();
		pcflag->next = th->flags;
		th->flags = pcflag;
		pcflag->type = flg->type;
		pcflag->val = flg->val;
		pcflag->timer = 0;
	      }
	  }
      /* When we add permanent flags from race/align stuff, we don't
	 add hps/moves. */
      if (flg->type >= AFF_START && flg->type < AFF_MAX &&
	  (!add_perm_affects || 
	   (flg->type != FLAG_AFF_HP &&
	    flg->type != FLAG_AFF_MV)))
	th->pc->aff[flg->type] += flg->val;
      if (flg->type == FLAG_AFF_ARMOR)
        th->armor += flg->val;      
    }
  update_kde (th, ~0);
  return;
}

void
do_noaffect (THING *th, char *arg)
{
  THING *vict;
  

  if (!IS_PC (th))
    return;

  if (!*arg)
    vict = th;  
  else if ((vict = find_pc (th, arg)) == NULL)
    {
      stt ("Noaffect who?\n\r", th);
      return;
    }
  
  remove_all_affects (vict, FALSE);
  return;
}
  
      
void
do_restore (THING *th, char *arg)
{
  THING *vict;
  
  if (!IS_PC (th))
    return;

  if (!*arg)
    vict = th;
  
  if (!str_cmp (arg, "all"))
    {
      for (vict = thing_hash[PLAYER_VNUM % HASH_SIZE]; vict; vict = vict->next)
	{
	  if (IS_PC (vict))
	    {
	      restore_thing (vict);
	    }
	}
      stt ("Ok, everyone restored!\n\r", th);
      return;
    }
  
  if ((vict = find_thing_world (th, arg)) == NULL)
    {
      stt ("Restore who?\n\r", th);
      return;
    }
  
  restore_thing (vict);
  stt ("Ok, victim restored.\n\r", th);
  return;
}

/* This function restores a thing to full health. */

void
restore_thing (THING *th)
{
  FLAG *flg, *flgn;
  SPELL *spl;
  
  th->hp = th->max_hp;
  th->mv = th->max_mv;
  
  if (IS_PC (th))
    {
      th->pc->mana = th->pc->max_mana;
      th->pc->cond[COND_DRUNK] = 0;
      th->pc->cond[COND_HUNGER] = COND_FULL;
      th->pc->cond[COND_THIRST] = COND_FULL;
    }

  
  for (flg = th->flags; flg; flg = flgn)
    {
      flgn = flg->next;
      if ((spl = find_spell (NULL, flg->from)) != NULL &&
	  spl->target_type == TAR_OFFENSIVE)
	aff_from (flg, th);
    }
  
}



void
do_password (THING *th, char *arg)
{
  char arg1[STD_LEN];
  
  
  if (!IS_PC (th) || !th->fd)
    return;
  
  arg = f_word (arg, arg1);
  
  if (str_cmp (crypt2 (arg1, th->name), th->pc->pwd))
    {
      stt ("Syntax: password <old password> <new password>\n", th);
      return;
    }
  
  if (!check_password (th->fd, arg))
    {
      stt ("Your new password is not valid.\n\r", th);
      return;
    }
  
  th->pc->pwd = new_str (crypt2 (arg, th->name));
  stt ("Ok, password changed.\n\r", th);
  write_playerfile (th);
  return;
}
  

void
do_validate (THING *th, char *arg)
{
  THING *vict;
  FLAG *flg;
  
  if (!IS_PC (th) || LEVEL (th) < MAX_LEVEL)
    return;
  
  if ((vict = read_playerfile (capitalize (arg))) == NULL)
    {
      stt ("Victim doesn't exist.\n\r", th);
      return;
    }
  
  for (flg = vict->flags; flg != NULL; flg = flg->next)
    {
      if (flg->type == FLAG_PC1)
	RBIT (flg->val, PC_UNVALIDATED);
    }
  
  write_playerfile (vict);
  free_thing (vict);
  return;
}

void
do_pfile (THING *th, char *arg)
{
  char buf[STD_LEN];

  if (!IS_PC (th))
    return;

  if (LEVEL (th) < 40 && th->pc->remorts == 0)
    {
      stt ("You must be at least level 40 to get your pfile.\n\r", th);
      return;
    }

  if (str_cmp (arg, "sendnow"))
    {
      stt ("You must type pfile sendnow\n\r", th);
      return;
    }

  if (current_time - th->pc->pfile_sent < 3600*24*2)
    {
      stt ("You can only get one pfile every 48 hours.\n\r", th);
      return;
    }

  if (!th->short_desc || !*th->short_desc)
    {
      stt ("Your name is messed up.\n\r", th);
      return;
    }
  th->pc->pfile_sent = current_time;
  write_playerfile (th);
  sprintf (buf, "mail %s -s 'Your Pfile: Save as a file named %s' < %s%s &",
           th->pc->email, th->short_desc, PLR_DIR, th->short_desc);
  system (buf);
  stt ("Pfile sent.\n\r", th);
  return;
}

/* This reads in a playerfile object based on a filename. */

THING *
read_playerfile (char *name)
{
  THING *thg;
  char fname[STD_LEN];
  FILE *f;
  FILE_READ_SINGLE_SETUP;  
  if (!name || !*name || !isalpha (name[0]))
    return NULL;
  badcount = 0;
  fname[0] = '\0';
  sprintf (fname, "%s%s", PLR_DIR, name);
  strcpy (pfile_name, name);
  
  if ((f = wfopen (fname, "r")) == NULL)
    return NULL;
  
  strcpy (word, read_word (f));
  
  /* Read in main thing/pc info. */
  
  SBIT (server_flags, SERVER_READ_PFILE);
  thg = read_thing (f);
  RBIT (server_flags, SERVER_READ_PFILE);
  if (thg)
    calc_max_remort (thg);
  fclose (f);
  return thg;
}
