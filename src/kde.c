#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serv.h"

/* This section deals with the extra info sent to the KDE client. */

void
print_update (THING *th)
{
  int flags, i;
  char buf[STD_LEN];
  if (!th || !th->fd || !IS_PC (th))
    return;
  
  flags = flagbits (th->flags, FLAG_PC2);
  
  if (IS_SET (flags, PC2_BLANK) && !USING_KDE (th))
    write_to_buffer ("\n\r", th->fd);
  
  if (!IS_SET (flags, PC2_NOPROMPT))
    write_to_buffer (string_parse (th, th->pc->prompt), th->fd);
  
  
  if (!USING_KDE (th))
    return;
  
  flags = th->pc->kde_update_flags;

  /* This part sends all the updates for stuff like hps/exp etc...
     using the kde update code. */

  /* I have decided to do the work of deciding how to format the data here
     so that the client can be more flexible if people want to use it 
     with different MUDs. */
  
  /* Always send HMV atm...*/
  
  if (IS_SET (flags, KDE_UPDATE_HMV) || TRUE)
    {
      THING *vict;
      sprintf (buf, "<HMV>%d %d %d %d %d %d ",
	       th->hp, th->max_hp, 
	       th->mv, th->max_mv, 
	       th->pc->mana, th->pc->max_mana);
      if (th->fgt && th->fgt->bashlag > 2)
	strcat (buf, "*Bashed* ");
      if (th->fgt && th->fgt->delay > 0)
	strcat (buf, "Delayed ");
      if (th->position >= POSITION_CASTING &&
	  th->position < POSITION_MAX)
	{
	  strcat (buf, capitalize (position_names[th->position])) ;
	  strcat (buf, " ");
	}
      if ((vict = FIGHTING (th)) != NULL && vict->max_hp > 0)
	{
	  if (vict->hp >= vict->max_hp)
	    strcat (buf, "[Oppnt: Excellent] ");
	  else if (vict->hp >= vict->max_hp * 2 /3)
	    strcat (buf, "[Oppnt: Good] ");
	  else if (vict->hp >= vict->max_hp /3)
	    strcat (buf, "[Oppnt: Hurt] ");
	  else if (vict->hp >= vict->max_hp /10)
	    strcat (buf, "[Oppnt: Bad] ");
	  else 
	    strcat (buf, "[Oppnt: Awful] ");
	}
      strcat (buf, " </HMV>");
      stt (buf, th);
    }
 

  

  /* Update the name/level/race/align and remorts. */
 
  if (IS_SET (flags, KDE_UPDATE_NAME))
    {
      sprintf (buf, "<NAME>%s %d %s %s %d </NAME> ", 
	       NAME (th),
	       th->level,
	       FALIGN (th)->name,
	       FRACE (th)->name,
	       th->pc->remorts);
      stt (buf, th);
    }
 
  /* Send the basic stats... */

  if (IS_SET (flags, KDE_UPDATE_STAT))
    {
      sprintf (buf, "<STAT>Stats:\n");
      for (i = 0; i < STAT_MAX; i++)
	{
	  sprintf (buf + strlen (buf), "%s: %2d ",
		   stat_short_name[i],
		   get_stat (th, i));
	  if ((i % 4) == 3)
	    strcat (buf, "\n");
	}
      strcat (buf, "</STAT>");
      stt (buf, th);
    }
  
  /* Send the guild info. */
  
  if (IS_SET (flags, KDE_UPDATE_GUILD))
    {
      sprintf (buf, "<GUILD>Guilds:     ");
      for (i = 0; i < GUILD_MAX; i++)
	{
	  sprintf (buf + strlen (buf), "%-9s:%d ",
		   capitalize(guild_info[i].name),
		   th->pc->guild[i]);
	  if ((i % 3) == 1)
	    strcat (buf, "\n");
	}
      strcat (buf, " </GUILD>");
      stt (buf, th);
    }
  
  /* Send the implant info. */

  if (IS_SET (flags, KDE_UPDATE_IMPLANT))
    {
       sprintf (buf, "<IMPLANT>Implant: ");
      for (i = 0; i < PARTS_MAX; i++)
	{
	  sprintf (buf + strlen (buf), "%-5s: %d ",
		   capitalize ((char *) parts_names[i]),
		   th->pc->implants[i]);
	  if ((i % 3) == 1)
	    strcat (buf, "\n");
	}
      strcat (buf, " </IMPLANT>");
      stt (buf, th);
    }

  /* Send upated exp info. */
  
  if (IS_SET (flags, KDE_UPDATE_EXP))
    {
      sprintf (buf, "<EXP>%d %d </EXP>",
	       th->pc->exp, exp_to_level (LEVEL(th))-th->pc->exp);
      stt (buf, th);
    }

  /* Send pkill etc..info. */
  
  if (IS_SET (flags, KDE_UPDATE_PK))
    {
      sprintf (buf, "<PK>%d %d %d </PK>",
	       th->pc->pk[PK_WPS], th->pc->pk[PK_TOT_WPS],
	       th->pc->pk[PK_PKILLS]);
      stt (buf, th);
    }

  /* Send weight/carrying info. */
  
  if (IS_SET (flags, KDE_UPDATE_WEIGHT))
    {
      int carry_num = 0;
      THING *obj;
      for (obj = th->cont; obj; obj = obj->next_cont)
	carry_num++;
	  
      sprintf (buf, "<WEIGHT>%d %d %d </WEIGHT>",
	       carry_num, 
	       th->carry_weight/WGT_MULT,
	       th->carry_weight % WGT_MULT);
      stt (buf, th);
    }

  /* Send hitroll/damroll/evasion info */

  if (IS_SET (flags, KDE_UPDATE_COMBAT))
    {
      sprintf (buf, "<COMBAT>%d %d %d </COMBAT>",
	       get_hitroll (th),
	       get_damroll (th),
	       get_evasion (th));
      stt (buf, th);
    }
  
  if (IS_SET (flags, KDE_UPDATE_MAP))
    create_map (th, th->in, SMAP_MAXX, SMAP_MAXY);
  
  th->pc->kde_update_flags = 0;
  return;
}
	 



/* This adds flags to the kde update list of needed... */

void
update_kde (THING *th, int val)
{
  if (!th || !th->fd || !USING_KDE (th))
    return;
  
  th->pc->kde_update_flags |= val;
}


