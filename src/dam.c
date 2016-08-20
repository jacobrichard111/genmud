#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"


DAMAGE *
new_damage (void)
{
  DAMAGE *dam;
  dam = (DAMAGE *) mallok (sizeof (DAMAGE));
  return dam;
}

void
read_damage (void)
{
  DAMAGE *dam, *dam_last;
  FILE_READ_SETUP ("dam.dat");
  
  dam_list = NULL;
  dam_last = NULL;
  
  FILE_READ_LOOP
    {
      FKEY_START;
      FKEY("DAM_MSG")
	{
	  dam = new_damage ();
	  dam->low = read_number (f);
	  dam->high = read_number (f);
	  dam->message = new_str (read_string (f));
	  if (dam_last)
	    {
	      dam_last->next = dam;
	      dam->next = NULL;
	      dam_last = dam;
	    }
	  else
	    {
	      dam_list = dam;
	      dam_last = dam;
	      dam->next = NULL;
	    }
	}
      FKEY ("END_DAMAGES")
	break;
      FILE_READ_ERRCHECK ("dam.dat");
    }
  fclose (f);
  return;
}


