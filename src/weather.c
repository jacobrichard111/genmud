#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"



void
read_time_weather (void)
{
  FILE *f;
  if (wt_info)
    {
      free_value (wt_info);
      wt_info = NULL;
    }
  if ((f = wfopen ("wtinfo.dat", "r")) == NULL)
    {
      wt_info = new_value ();
      wt_info->type = 1;
      wt_info->val[WVAL_YEAR] = nr (200, 1200);
      wt_info->val[WVAL_MONTH] = nr (0, NUM_MONTHS - 1);
      wt_info->val[WVAL_DAY] = nr (0, NUM_WEEKS*NUM_DAYS - 1);
      wt_info->val[WVAL_HOUR] = nr (0, NUM_HOURS);
      wt_info->val[WVAL_TEMP] = nr (50, 75);
      wt_info->val[WVAL_WEATHER] = 0;
      wt_info->val[WVAL_LAST_RAIN] = 0;
      return;
    }
  read_word (f);
  wt_info = read_value (f);
  fclose (f);
  return;
}

void
write_time_weather (void)
{
  FILE *f;
  
  if (!wt_info)
    return;
  
  if ((f = wfopen ("wtinfo.dat", "w")) == NULL)
    return;
  
  write_value (f, wt_info);
  fclose (f);
  return;
}

  

/* This updates the time and weather info. It's pretty simple and lame,
   but I guess it's ok for a start...and it's not like the players
   would ever notice or appreciate something more complicated,
   but I might add moons or something else like that. */

void 
update_time_weather (void)
{
  char buf[STD_LEN];
  THING *th;
  int day, old_weather, new_weather;
  bool is_night;
  if (!wt_info)
    return;
  if (++wt_info->val[WVAL_HOUR] >= NUM_HOURS)
    {
      wt_info->val[WVAL_HOUR] = 0;
      if (++wt_info->val[WVAL_DAY] >= NUM_WEEKS*NUM_DAYS)
	{
	  wt_info->val[WVAL_DAY] = 0;
	  if (++wt_info->val[WVAL_MONTH] >= NUM_MONTHS)
	    {
	      wt_info->val[WVAL_MONTH] = 0;
	      wt_info->val[WVAL_YEAR]++;	      
	      sprintf (buf, "It is the new year %d, the year of the %s!!\n\r", wt_info->val[WVAL_YEAR], year_names[wt_info->val[WVAL_YEAR] % NUM_YEARS]);
	      echo (buf);
	    }
	  sprintf (buf, "It is a new month, the month of %s!\n\r", month_names[wt_info->val[WVAL_MONTH]]);
	  echo (buf);
	}
      day = wt_info->val[WVAL_DAY] + 1;
      sprintf (buf, "It is now %s, the %d%s of %s, %d, the year of the %s.\n\r", day_names[day % NUM_DAYS],
	       day, (day > 10 && day < 19 ? "th" :
		     day % 10 == 1 ? "st" :
		     day % 10 == 2 ? "nd" :
		     day % 10 == 3 ? "rd" : "th"),
	       month_names[wt_info->val[WVAL_MONTH]],
	       wt_info->val[WVAL_YEAR],
	       year_names[wt_info->val[WVAL_YEAR] % NUM_YEARS]);
      echo (buf);
      
    }
  
  buf[0] = '\0';
  switch (wt_info->val[WVAL_HOUR])
    {
	case 2: 
	  sprintf (buf, "Some unseen creatures howl off in the distance.\n\r");
	  break;
	case 6:
	  sprintf (buf, "The first rays of the sun appear in the west.\n\r");
	  break;
	case HOUR_DAYBREAK:
	  sprintf (buf, "The sun creeps over the western horizon.\n\r");
	  break;
	case 12:
	  sprintf (buf, "The sun is almost directly overhead now.\n\r");
	  break;
	case 17:
	  sprintf (buf, "Shadows lengthen as the sun falls toward the eastern skyline.\n\r");
	  break;
	case 19:
	  sprintf (buf, "The sky begins to darken as night approaches.\n\r");
	  break;
	case HOUR_NIGHT:
	  if (wt_info->val[WVAL_WEATHER] == WEATHER_STORMY)
	    sprintf (buf, "It was a dark and stormy night.\n\r");
	  else
	    sprintf (buf, "Night is here.\n\r");
	  break;
	case 24: 
	  sprintf (buf, "The darkness is total.\n\r");
	  break;
    }
  if (buf[0])
    {
      for (th = thing_hash[PLAYER_VNUM % HASH_SIZE]; th; th = th->next)
	{
	  if (!IS_PC (th) || th->position < POSITION_SLEEPING || 
	      (th->in && IS_ROOM_SET (th->in, ROOM_INSIDE | ROOM_WATERY | ROOM_UNDERWATER | ROOM_UNDERGROUND)))
	    continue;
	  stt (buf, th);
	}
    }
  
  if (wt_info->val[WVAL_HOUR] == 6)
    {
      wt_info->val[WVAL_TEMP] = 55 + nr (1,10);
      
      /* Temperature is affected by the month. Near middle of months = + temp,
	 near edges = - temp. So, when the month is close to NUM_MONTHS/2 the
	 temp is approx 20 more. When it's close to 0 or NUM_MONTHS, it's 
	 much less since then the diff inside the abs is about 5*7 = 35 */
      
      wt_info->val[WVAL_TEMP] += (18 - ABS(5*(NUM_MONTHS/2 - wt_info->val[WVAL_MONTH])));
    }

  if (wt_info->val[WVAL_HOUR] > HOUR_NIGHT ||
      wt_info->val[WVAL_HOUR] < HOUR_DAYBREAK)
    is_night = TRUE;
  else
    is_night = FALSE;
  
  if (is_night)
    wt_info->val[WVAL_TEMP] += nr (1,3);
  else
    wt_info->val[WVAL_TEMP] -= nr (1,3);


  old_weather = wt_info->val[WVAL_WEATHER];
  new_weather = wt_info->val[WVAL_WEATHER];
  
  if (old_weather < 0 || old_weather >= WEATHER_MAX)
    old_weather = WEATHER_SUNNY;
  
  buf[0] = '\0';
 
  switch (old_weather)
    {
    case WEATHER_SUNNY:
    default:
      if (nr (1,7) == 2)
	{
	  new_weather = WEATHER_CLOUDY;
	  sprintf (buf, "Some clouds start to roll in.\n\r");	 
	}
      break;
    case WEATHER_CLOUDY:
      if (nr (1,5) == 2)
	{
	  new_weather = WEATHER_SUNNY;
	  sprintf (buf, "The clouds clear up overhead.\n\r"); 
	}      
      else if (nr (1,5) == 3)
	{
	  new_weather = WEATHER_RAINY;
	  if (wt_info->val[WVAL_TEMP] > 30)
	    sprintf (buf, "The clouds get thicker, and it starts to rain.\n\r");
	  else
	    {
	      sprintf (buf, "The clouds get thicker, and it starts to snow.\n\r");
	      if (nr (1,500) == 2)
		snow_disaster();
	      
	    }
	}
      break;
    case WEATHER_RAINY:
      if (nr (1,10) == 2)
	{
	  new_weather = WEATHER_SUNNY;
	  sprintf (buf, "The rain stops, and the clouds part, revealing a clear sky.\n\r");
	}
      else if (nr (1,8) == 2)
	{
	  new_weather = WEATHER_CLOUDY;
	  sprintf (buf, "The rain stops, but clouds still loom overhead.\n\r");
	}
      else if (nr (1,24) == 3)
	{
	  new_weather = WEATHER_FOGGY;
	  sprintf (buf, "The rain stops, but a thick fog covers the land.\n\r");
	}
      else if (nr (1,30) == 3)
	{
	  new_weather = WEATHER_STORMY;
	  if (wt_info->val[WVAL_TEMP] > 30)
	    sprintf (buf, "The storm intensifies overhead! You should seek shelter!\n\r");
	  else 
	    {
	      sprintf (buf, "The snowstorm turns into a blizzard! You should seek shelter!\n\r");
	      if (nr (1,40) == 3)
		snow_disaster();
	    }
	}
      break;
    case WEATHER_STORMY:
      if (nr (1,6) == 2)
	{
	  new_weather = WEATHER_RAINY;
	  if (wt_info->val[WVAL_TEMP] > 30)
	    sprintf (buf, "The storm abates, but it is still raining a great deal.\n\r");
	  else
	    sprintf (buf, "The storm, abates, but it is still snowing.\n\r");
	}
      break;
    case WEATHER_FOGGY:
      if (nr (1,7) != 2)
	{
	  new_weather = WEATHER_SUNNY;
	  sprintf (buf, "The fog burns off, leaving the clear sky visible above.\n\r");
	}
      break;
    }
  
  /* Now get the temp modifiers from the weather. */

  if (wt_info->val[WVAL_WEATHER] == WEATHER_SUNNY)
    wt_info->val[WVAL_TEMP] += nr (1,3);
  else if (wt_info->val[WVAL_WEATHER] == WEATHER_RAINY ||
	   wt_info->val[WVAL_WEATHER] == WEATHER_STORMY)
    wt_info->val[WVAL_TEMP] -= nr (1,3);
  
  

  if (new_weather == WEATHER_SUNNY || 
      new_weather == WEATHER_CLOUDY)
    wt_info->val[WVAL_LAST_RAIN]++;
  else
    wt_info->val[WVAL_LAST_RAIN] = 0;
  
  
  for (th = thing_hash[PLAYER_VNUM % HASH_SIZE]; th != NULL; th = th->next)
    {
      if (IS_PC (th) && !FIGHTING (th) && th->pc->no_quit > 0)
        th->pc->no_quit--;
      if (!buf[0] || !IS_PC (th) || !th->in || 
	  IS_ROOM_SET (th->in, ROOM_INSIDE | ROOM_EARTHY | ROOM_FIERY | 
            ROOM_UNDERGROUND | ROOM_UNDERWATER) ||
	  th->position < POSITION_RESTING)
	continue;
      stt (buf, th);
    }
  wt_info->val[WVAL_WEATHER] = new_weather;
  
  
  
  /* If it's cold and stormy, make areas get snowed in. Cover all
     road/field/forest/rough rooms with snow in all nondesert nonswamp
     areas. */
  

  write_time_weather ();
  return;
}


void
calculate_warmth (THING *th)
{
  THING *obj;
  VALUE *arm;

  if (!IS_PC (th)) /* Only pc's worry about this. */
    return;

  th->pc->warmth = 0;
  
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if ((arm = FNV (obj, VAL_ARMOR)) != NULL)
	th->pc->warmth += arm->val[6];
    }
  
  if (IS_HURT (th, AFF_FIRE))
    th->pc->warmth += 100;
  if (IS_HURT (th, AFF_WATER)) /* ICE */
    th->pc->warmth -= 100;
  return;
}


void
check_weather_effects (THING *th)
{
  int temp;
  
  if (!IS_PC (th) || !wt_info || !th->in || !IS_ROOM (th->in) ||
      !th->in->in || !IS_AREA (th->in->in))
    return;
  
  temp = find_room_temperature (th->in) + th->pc->warmth;
  
  
  if (temp > 89 && !IS_ROOM_SET (th->in, ROOM_SNOW | ROOM_WATERY))
    {
      stt ("You sure are HOT!\n\r", th);
      th->pc->cond[COND_THIRST] -= nr (1,3);
    }
  else if (temp < 25 && !IS_ROOM_SET (th->in, ROOM_INSIDE | ROOM_FIERY | ROOM_EARTHY))
    {
      stt ("You sure are COLD!\n\r", th);
      if (th->hp > 3)
	th->hp -= 2;
    }
  return;
}

/* This gets the temprerature of a room only. */
  
int
find_room_temperature (THING *room)
{
  int areaflags, roomflags;
  int temp;
  if (!room || !IS_ROOM (room) || !room->in || !IS_AREA (room->in) ||
      !wt_info)
    return 70;

  temp = wt_info->val[WVAL_TEMP];

  areaflags = flagbits (room->in->flags, FLAG_ROOM1);
  roomflags = flagbits (room->flags, FLAG_ROOM1);
  
  /* Flags like watery/fiery etc..taken care of by another 
     update. */

  if (IS_SET (areaflags | roomflags, ROOM_DESERT))
    temp += 20;
  if (IS_SET (areaflags | roomflags, ROOM_SWAMP))
    temp += 10;
  if (IS_SET (areaflags | roomflags, ROOM_FOREST))
    temp -= 10;
  if (IS_SET (areaflags | roomflags, ROOM_ROUGH | ROOM_MOUNTAIN))
    temp -= 20;
  if (IS_SET (areaflags | roomflags, ROOM_SNOW))
    temp -= 30;
  
  /* Underground tends towards 55 degrees slightly. */
  if (IS_SET (areaflags | roomflags, ROOM_UNDERGROUND))
    {
      if (temp < 50)
	temp += 10;
      else if (temp > 60)
	temp -= 10;
    }
  
  /* Inside tends towards 65 degrees strongly. */
  if (IS_SET (areaflags | roomflags, ROOM_INSIDE))
    {
      if (temp < 60)
	temp += (60 - temp)/2;
      else if (temp > 70)
	temp -= (temp - 60)/2;
    }

  /* At night the temp goes down. */
  if (wt_info->val[WVAL_HOUR] >= HOUR_NIGHT + 4 ||
      wt_info->val[WVAL_HOUR] < HOUR_DAYBREAK)
    {
      temp -= 20;
      if (IS_SET (areaflags, ROOM_DESERT))
	temp -= 20;
    }

  return temp;
}

  
