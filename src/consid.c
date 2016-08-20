#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"



void
do_consider (THING *th, char *arg)
{
  THING *vict;
  int vround, tround, diff;
  char buf[STD_LEN];
  

  /* Set special consider flag so the server knows that this is
     fake combat. */
  SBIT (server_flags, SERVER_CONSIDERING);
  if ((vict = find_thing_in_combat (th, arg, TRUE)) == NULL)
    {
      RBIT (server_flags, SERVER_CONSIDERING);
      return;
    }
  
  cons_hp = th->hp;

  /* Do 50 rounds of combat you vs vict to see who takes longer
     to die. */
  for (vround = 0; vround < 50 && cons_hp > 0; vround++)
    multi_hit (vict, th);
  cons_hp = vict->hp;
  for (tround = 0; tround < 50 && cons_hp > 0; tround++)
    multi_hit (th, vict);
  RBIT (server_flags, SERVER_CONSIDERING);

  /* Difference in time to kill, vict - you. If this is big,
     then the vict took longer to kill you than you did to kill
     it. If it's small, the victim killed you fast, and you killed
     it slow. */
  
  diff = (vround - tround)/2;
  
  if (diff < -9)
    sprintf (buf, "YOU WILL DIE!!!\n\r");
  else if (diff == -8)
    sprintf (buf, "It will take a miracle for you to survive this!\n\r");
  else if (diff == -7)
    sprintf (buf, "%s will toss you around like a ragdoll.\n\r", NAME (vict));
  else if (diff == -6)
    sprintf (buf, "You do not stand much of a chance vs %s.\n\r", NAME (vict));
  else if (diff == -5)
    sprintf (buf, "You will need a lot of help for this one.\n\r");
  else if (diff == -4)
    sprintf (buf, "You better get some help before you attack %s.\n\r", NAME (vict));
  else if (diff == -3)
    sprintf (buf, "%s will probably defeat you.\n\r", NAME (vict));
  else if (diff == -2)
    sprintf (buf, "%s is a somewhat stronger than you.\n\r", NAME (vict));
  else if (diff > -2 && diff < 2)
    sprintf (buf, "It will be a close battle.\n\r");
  else if (diff == 2)
    sprintf (buf, "You are a little bit stronger than %s.\n\r", NAME (vict));
  else if (diff == 3)
    sprintf (buf, "You can probably defeat %s.\n\r", NAME (vict));
  else if (diff == 4)
    sprintf (buf, "You should be able to take %s pretty easily.\n\r", NAME (vict));
  else if (diff == 5)
    sprintf (buf, "%s looks fairly weak.\n\r", NAME (vict));
  else if (diff == 6)
    sprintf (buf, "%s doesn't stand much of a chance against you.\n\r", NAME (vict));
  else if (diff == 7)
    sprintf (buf, "You will beat %s down like a rented mule.\n\r", NAME (vict));
  else if (diff == 8)
    sprintf (buf, "Don't you feel bad picking on something this weak?\n\r");
  else 
    sprintf (buf, "YOU R00000L 0V3R %s!!!!!\n\r", NAME (vict));
  stt (buf, th);
  return;
}
  
/* Group consider */

void
do_gconsider (THING *th, char *arg)
{
  THING *vict, *thg;
  int vround, tround, diff;
  char buf[STD_LEN];

  
  SBIT (server_flags, SERVER_CONSIDERING);
  if ((vict = find_thing_in_combat (th, arg, TRUE)) == NULL)
    {
      RBIT (server_flags, SERVER_CONSIDERING);
      return;
    }
  cons_hp = th->hp;
  for (vround = 0; vround < 50 && cons_hp > 0; vround++)
    multi_hit (vict, th);
  cons_hp = vict->hp;
  for (tround = 0; tround < 50 && cons_hp > 0; tround++)
    {
      for (thg = th->in->cont; thg && cons_hp > 0; thg = thg->next_cont)
	{
	  if (CAN_FIGHT (thg) && in_same_group (thg, th))
	    multi_hit (thg, vict);
	}
    }
  RBIT (server_flags, SERVER_CONSIDERING);
  diff = (vround - tround)/2;
  
   if (diff < -9)
    sprintf (buf, "YOUR GROUP WILL DIE!!!\n\r");
  else if (diff == -8)
    sprintf (buf, "It will take a miracle for your group to survive this!\n\r");
  else if (diff == -7)
    sprintf (buf, "%s will toss your group around like a ragdoll.\n\r", NAME (vict));
  else if (diff == -6)
    sprintf (buf, "Your group does not stand much of a chance vs %s.\n\r", NAME (vict));
  else if (diff == -5)
    sprintf (buf, "Your group will need a lot of help for this one.\n\r");
  else if (diff == -4)
    sprintf (buf, "Your group had better get some help before you attack %s.\n\r", NAME (vict));
  else if (diff == -3)
    sprintf (buf, "%s will probably defeat your group.\n\r", NAME (vict));
  else if (diff == -2)
    sprintf (buf, "%s is a somewhat stronger than your group.\n\r", NAME (vict));
  else if (diff > -2 && diff < 2)
    sprintf (buf, "It will be a close battle.\n\r");
  else if (diff == 2)
    sprintf (buf, "Your group is a little bit stronger than %s.\n\r", NAME (vict));
  else if (diff == 3)
    sprintf (buf, "Your group can probably defeat %s.\n\r", NAME (vict));
  else if (diff == 4)
    sprintf (buf, "Your group should be able to take %s pretty easily.\n\r", NAME (vict));
  else if (diff == 5)
    sprintf (buf, "%s looks fairly weak compared to your group.\n\r", NAME (vict));
  else if (diff == 6)
    sprintf (buf, "%s doesn't stand much of a chance against your group.\n\r", NAME (vict));
  else if (diff == 7)
    sprintf (buf, "Your group will beat %s down like a rented mule.\n\r", NAME (vict));
  else if (diff == 8)
    sprintf (buf, "Doesn't your group feel bad picking on something this weak?\n\r");
  else 
    sprintf (buf, "YOUR GR0000P R00000LZ 0V3R %s!!!!!\n\r", NAME (vict));
  stt (buf, th);
  return;
}
