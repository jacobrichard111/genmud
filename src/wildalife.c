#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>
#include "serv.h"
#include "wildalife.h"
#include "wilderness.h"
#include "society.h"


/* Externs for the wilderness alife simulation. */

int sector_max_res[SECTOR_MAX][SECTOR_MAX][RAW_MAX];
int sector_curr_res[SECTOR_MAX][SECTOR_MAX][RAW_MAX];
int sector_used_res[SECTOR_MAX][SECTOR_MAX][RAW_MAX];
char dominant_terrain[SECTOR_MAX][SECTOR_MAX];
POPUL *sector_populs[SECTOR_MAX][SECTOR_MAX];
int sector_pulse;
POPUL *popul_free;
int popul_count;
int popul_made;
/* These are the various kinds of raw materials that different room
   types provide for the map. */

/* None Min stone wood flower food herb ichor */
const int room_resources[PIXMAP_MAX][RAW_MAX] =
  {/* N  M  S  W FL FO He Ic */
    { 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0}, /* default */   
    { 0, 0, 0, 0, 0, 3, 2, 0}, /* Water */
    { 0, 1, 1, 3, 4, 2, 6, 2}, /* Swamp */ 
    { 0, 1, 0, 2, 4, 6, 3, 3}, /* Field */
    { 0, 0, 1, 6, 1, 2, 4, 2}, /* Forest */   
    { 0, 6, 6, 2, 0, 0, 1, 1}, /* Mountain */
    { 0, 1, 0, 2, 2, 3, 1, 2}, /* Snow */
    { 0, 4, 4, 0, 0, 1, 3, 5}, /* Desert */ 
    { 0, 1, 1, 1, 1, 1, 1, 1}, /* road */
    { 0, 0, 0, 0, 0, 0, 0, 0}, /* Air */
    { 0, 4, 0, 0, 0, 0, 0, 0}, /* Fire */
    { 0, 0, 0, 0, 0, 0, 0, 0}, /* Inside */
    { 0, 4, 4, 0, 0, 2, 0, 0}, /* Underground */
  };
    

/* This initializes the variables for the populations. */

void
init_popul_vars (void)
{
  int x, y, raw;
  
  sector_pulse = nr (1, SECTOR_MAX);
  popul_free = NULL;
  popul_count = 0;
  popul_made = 0;
  /* Make sure the wilderness_size/sector_max is an exact int. */

  if (WILDERNESS_SIZE % SECTOR_MAX)
    {
      log_it ( "WILDERNESS_SIZE/SECTOR_MAX is not an exact int.\n");
      exit (1);
    }
  for (x = 0; x < SECTOR_MAX; x++)
    {
      for (y = 0; y < SECTOR_MAX; y++)
	{
	  sector_populs[x][y] = NULL;
	  
	  for (raw = 0; raw < RAW_MAX; raw++)
	    {
	      sector_max_res[x][y][raw] = 0;
	      sector_curr_res[x][y][raw] = 0;
	      sector_used_res[x][y][raw] = 0;
	     
	    }
	  dominant_terrain[x][y] = 0;
	  calculate_sector_raws (x, y);
	}
    }
  
 
  return;
}

/* This clears population data. */

void
clear_popul_data (POPUL *popul)
{
  if (!popul)
    return;
  
  popul->society = 0;
  popul->next = NULL;
  popul->power = 0;
  popul->alert = 0;
  popul->x = SECTOR_MAX;
  popul->y = SECTOR_MAX;
  popul->align = 0;
  
}
/* This creates a new population data structure and initializes it. */

POPUL *
new_popul (void)
{
  POPUL *popul;
  
  popul_made++;
  if (popul_free)
    {
      popul = popul_free;
      popul_free = popul_free->next;
    }
  else
    {
      popul = (POPUL *) mallok (sizeof (POPUL));
      popul_count++;
    }
  
  clear_popul_data(popul);
  return popul;
}

/* This frees a population. */

void
free_popul (POPUL *popul)
{
  
  
  if (!popul)
    return;
  
  popul_from_sector(popul);
  clear_popul_data(popul);
  popul->next = popul_free;
  popul_free = popul;
  return;
}


/* This adds a population to a sector list. */

bool
popul_to_sector (POPUL *popul, int x, int y)
{
  POPUL *pop2;
  
  if (!popul || x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX) 
    return FALSE;
  
  popul_from_sector (popul);
  
  popul->x = x;
  popul->y = y;
  if ((pop2 = find_popul_sector (x, y, popul->society)) != NULL)
    {
      add_population_power (x, y, popul->society, popul->power);
      free_popul (popul);
      return TRUE;
    }
  popul->next = sector_populs[x][y];
  sector_populs[x][y] = popul;
  return TRUE;
}


/* This removes a population from a sector. */

void
popul_from_sector (POPUL *popul)
{
  POPUL *prev;
  if (!popul )
    return;
  
  /* Population not in a sector. */
  
  if (popul->x < 0 || popul->y < 0 || popul->x >= SECTOR_MAX ||
      popul->y >= SECTOR_MAX)
    {
      popul->x = SECTOR_MAX;
      popul->y = SECTOR_MAX;
      return;
    }

  if (popul == sector_populs[popul->x][popul->y])
    {
      sector_populs[popul->x][popul->y] = popul->next;
      popul->next = NULL;
    }
  else
    {
      for (prev = sector_populs[popul->x][popul->y]; prev; prev = prev->next)
	{
	  if (prev->next == popul)
	    {
	      prev->next = popul->next;
	      popul->next = NULL;
	      break;
	    }
	}
    }
  popul->x = SECTOR_MAX;
  popul->y = SECTOR_MAX;
  return;
}
	
  
/* This will read all of the populations in from population.dat */

void
read_wildalife (void)
{
  FILE *f;
  char word[STD_LEN];
  int badcount = 0;
  POPUL *popul = NULL;
  init_popul_vars();
  if ((f = wfopen ("population.dat", "r")) == NULL)
    {
      perror ("population.dat");
    }
  else
    {
      while (TRUE)
	{
	  strcpy (word, read_word (f));
	  if (!str_cmp (word, "POPULATION"))
	    {
	      popul = read_popul (f);
	      /* Try to add population to sector. */
	      if (!popul_to_sector (popul, popul->x, popul->y))
		free_popul (popul);
	      
	    }
	  else if (!str_cmp (word, "END_OF_POPULATIONS"))
	    {
	      fclose (f);
	      break;
	    }
	  else 
	    {
	      char errbuf[STD_LEN];
	      sprintf (errbuf, "Bad read on population.dat: %s\n", word);
	      
	      if (++badcount > 100)
		{
		  log_it ("Too many bad reads on population.dat!\n");
		  fclose (f);
		  break;
		}
	    }
	} 
    }
  
  if ((f = wfopen ("sector_resources.dat", "r")) == NULL)
    {
      perror ("sector_resources.dat");
    }
  else
    {
      while (TRUE)
	{
	  strcpy (word, read_word (f));
	  if (!str_cmp (word, "SECTOR_RES"))
	    {
	      read_sector_resources(f);
	    }
	  else if (!str_cmp (word, "END_OF_SECTOR_RESOURCES"))
	    {
	      fclose (f);
	      break;
	    }
	  else 
	    {
	      char errbuf[STD_LEN];
	      sprintf (errbuf, "Bad read on sector_resources.dat: %s\n", word);
	      log_it (errbuf);
	      if (++badcount > 100)
		{
		  log_it ("Too many bad reads on sector_resources.dat!\n");
		  fclose (f);
		  break;
		}
	    }
	}
    }
  seed_populations();
  return;
}

/* This writes the populations out to the file. */

void
write_wildalife (void)
{
  POPUL *popul;
  FILE *f;
  int x, y;
  if ((f = wfopen ("population.dat", "w")) == NULL)
    {
      perror ("population.dat");
      return;
    }

  for (x = 0; x < SECTOR_MAX; x++)
    {
      for (y = 0; y < SECTOR_MAX; y++)
	{
	  for (popul = sector_populs[x][y]; popul; popul = popul->next)
	    {
	      write_popul (f, popul);
	    }
	}
    }
  fprintf (f, "\nEND_OF_POPULATIONS\n\n\r");
  fclose (f);
  
   /* Now write sector raws data. */
  
   if ((f = wfopen ("sector_resources.dat", "w")) == NULL)
    {
      perror ("sector_resources.dat");
      return;
    }

  for (x = 0; x < SECTOR_MAX; x++)
    {
      for (y = 0; y < SECTOR_MAX; y++)
	{
	  write_sector_resources (f, x, y);
	}
    }
  fprintf (f, "\nEND_OF_SECTOR_RESOURCES\n\n\r");
  fclose (f); 
  return;
}

/* This writes the current sector resources out to a file. */

void
write_sector_resources (FILE *f, int x, int y)
{
  int raw;
  if (!f || x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX)
    return;  
  
  fprintf (f, "SECTOR_RES %d %d %d", x, y, sector_used_res[x][y][1]);
  for (raw = 0; raw < RAW_MAX; raw++)
    fprintf (f, " %d", sector_curr_res[x][y][raw]);
  fprintf (f, "\n");
}


/* This writes a population out to a file. */

void
write_popul (FILE * f, POPUL *popul)
{
  if (!f || !popul)
    return;

  fprintf (f, "POPULATION %d %d %d %d %d %d\n",
	   popul->society, popul->power, popul->alert,
	   popul->x, popul->y, popul->align);
  return;
}


/* This reads a population in from a file. */

POPUL *
read_popul (FILE *f)
{
  POPUL *popul;
  
  if (!f)
    return NULL;

  popul = new_popul();
  
  popul->society = read_number (f);
  popul->power = read_number (f);
  popul->alert = read_number (f);
  popul->x = read_number (f);
  popul->y = read_number (f);
  popul->align = read_number (f);
  return popul;
}

/* This reads a piece of the sector resources in from the file. */

void
read_sector_resources (FILE *f)
{
  int x,y, raw, curr_used;
  if (!f)
    return;

  x = read_number (f);
  y = read_number (f);
  
  if (x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX)
    {
      for (raw = 0; raw < RAW_MAX; raw++)
	read_number (f);
      return;
    }
  
  curr_used = read_number (f);
  for (raw = 0; raw < RAW_MAX; raw++)
    {
      sector_curr_res[x][y][raw] = read_number (f);
      sector_used_res[x][y][raw] = curr_used;
    }
  return;
}



/* This calculates the raw materials found in a sector based on the
   kinds of rooms found there. */

void
calculate_sector_raws (int x, int y)
{
  int min_x, max_x, min_y, max_y;
  int raw, roomtype, pix;
  int curr_x, curr_y;
  int sector_count[PIXMAP_MAX]; /* Used to find the dominant terrain here. */
  int dominant_sector = 0, dominant_sector_count = 0;

  int count = 0 ;

  if (x < 0 || x >= SECTOR_MAX || y < 0 || y >= SECTOR_MAX)
    return;
  
  
  /* Get the min and max values of the rooms to be checked from the 
     wilderness map. */
  
  min_x = x*SECTOR_SIZE;
  max_x = min_x + SECTOR_SIZE;
  min_y = y*SECTOR_SIZE;
  max_y = min_y + SECTOR_SIZE;
  
  for (pix = 0; pix < PIXMAP_MAX; pix++)
    sector_count[pix] = 0;

  for (raw = 0; raw < RAW_MAX; raw++)
    sector_max_res[x][y][raw] = 0;
  
  /* Go through all of the rooms in the sector and add their raw materials
     to the totals for the sector. */

  for (curr_x = min_x; curr_x < max_x; curr_x++)
    {
      for (curr_y = min_y; curr_y < max_y; curr_y++)
	{
	  if ((roomtype = wilderness[curr_x][curr_y]) > 0 &&
	      roomtype < PIXMAP_MAX)
	    {
	      sector_count[roomtype]++;
	      for (raw = 0; raw < RAW_MAX; raw++)
		sector_max_res[x][y][raw] += room_resources[roomtype][raw];
	    }
	  count++;
	}
  
    }
  
  for (raw = 0; raw < RAW_MAX; raw++)
    sector_curr_res[x][y][raw] = sector_max_res[x][y][raw];

  /* Now find the dominant sector type. */

  for (pix = 0; pix < PIXMAP_MAX; pix++)
    {
      if (sector_count[pix] > dominant_sector_count)
	{
	  dominant_sector_count = sector_count[pix];
	  dominant_sector = pix;
	}
    }  
  dominant_terrain[x][y] = dominant_sector;

  return;
}
		
/* This just calls update wilderness sectors with a false arg. */

void
update_wilderness_sectors_event (void)
{
  update_wilderness_sectors (FALSE);
  return;
}

/* This updates the sectors in the game. It updates 1/SECTOR_MAX of
   the world each time through the loop unless it's set to update all
   at once, then it propogates through all of the updates at once. */

void
update_wilderness_sectors (bool all_at_once)
{
  int x, y;

  
  if (++sector_pulse >= SECTOR_MAX)
    {
      sector_pulse = 0;
      update_population_homelands();
    }
  /* Update all of the sector rows at least once in the y direction. */
  
  if (!all_at_once)
    {
      for (x = 0; x < SECTOR_MAX; x++)
	update_sector (x, sector_pulse);
      return;
    }
  
  for (y = 0; y < SECTOR_MAX; y++)
    {
      for (x = 0; x < SECTOR_MAX; x++)
	{
	  update_sector (x, y);
	}
    }
  return;
}

/* This function actually updates the data in a sector. */

void
update_sector (int x, int y)
{
  int resource_shortfall_percent = 0;
  
  if (x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX)
    return;
  
  /* Calculate the resources used and if any shortfall exists. */
  
  resource_shortfall_percent = population_resource_usage (x, y);
  

  /* Update the population growth. */
  
  population_growth (x, y, resource_shortfall_percent);


  
  /* Now find enemies to attack. */  
  
  population_sector_attack (x, y, !!resource_shortfall_percent);


  /* Now do "alignment attacks". Basically, pick a nonzero align, and
     all of the populations in that align attack one member of another
     alignment. It allows several smaller populations to fight vs a big 
     one. */
  
  population_alignment_attack (x, y);
  
  /* Now do settling. */
  
  population_settle (x, y, !!resource_shortfall_percent);
  
  /* Now assist adjacent populations that need help. */

  population_assist (x, y);
  
  /* Now get rid of all "Dead" populations. */

  population_sector_cleanup (x, y);
  
  return;
}



/* This calculates the resources used by the populations in the sector
   and whether or not there's some kind of shortfall. */

int
population_resource_usage (int x, int y)
{
   POPUL *pop;
  int popul_power_sum = 0; /* Sum of all population powers in this sector. */
  int popul_res_used = 0; /* Resources used by populations in this sector. */
  int raw;
  /* Find the min and max resources available to represent some fungibility
     between resources. */
  
  int min_resource_avail = 100000000;
  int max_resource_avail = 0;
  

  /* The percentage which the resources are falling short by due to
     overpopulation. */
  
  int resource_shortfall_percent = 0;

  /* These are the total number of resources available for use
     within this sector. */

  int total_resources = 0;



  if (x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX)
    return 0;

/* This updates the resources in the sector and tells what the shortfall
   is like (if any). */

  /* Add up all population powers. */
  
  for (pop = sector_populs[x][y]; pop; pop = pop->next)
    popul_power_sum += pop->power;
      
  /* Figure out how many resources the populations use. */
  
  popul_res_used = MAX(1, popul_power_sum/POPUL_POWER_PER_RESOURCE);
  
  /* Now compare this to the resources available in the sector. */
  
  for (raw = 1; raw < RAW_MAX; raw++)
    {
      if (sector_curr_res[x][y][raw] < min_resource_avail)
	min_resource_avail = sector_curr_res[x][y][raw];
      if (sector_curr_res[x][y][raw] > max_resource_avail)
	max_resource_avail = sector_curr_res[x][y][raw];
      total_resources += sector_curr_res[x][y][raw];

      /* If the resources being used are more than half the 
	 available amount in any particular area, decrement 
	 that area. Go down to 10pct of max. */
      
      if (sector_curr_res[x][y][raw] < 2*popul_res_used)
	{
	  sector_curr_res[x][y][raw]--;
	  if (sector_curr_res[x][y][raw] < sector_max_res[x][y][raw]/10)
	    sector_curr_res[x][y][raw] = sector_max_res[x][y][raw]/10;
	}

      /* Make the sector used res equal to the popul_res_used.*/
      
      sector_used_res[x][y][raw] = popul_res_used;
      
    }

  /* Allow for some exchange of resources. Let the smallest resource
     available be the max of itself and 1/5 of the average amt of
     resources available. (Don't allow this anymore.) */

  /* min_resource_avail = MAX (min_resource_avail, 
			    (total_resources/(MAX(1,RAW_MAX-1)*5)));
  */


  
  /* Calculate the resource shortfall percent. 
     If m == r, this is 100 - 100 = 0. If m = r/2, this is 50. */

  
  resource_shortfall_percent = MAX(0, 100-((min_resource_avail*100)/popul_res_used));

return resource_shortfall_percent;
}

/* This makes the populations in the sector grow (or shrink) based
   on how over or underpopulated the sector is. */

void
population_growth (int x, int y, int resource_shortfall_percent)
{
  POPUL *pop;
  SOCIETY *soc;
  /* This ranges from -7 to +3 pct based on resource availability. */
  int pop_inc_pct = 30 - resource_shortfall_percent;

  if (x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX)
    return;
  /* Now increase or decrease population based on the shortfall. */
  
  for (pop = sector_populs[x][y]; pop; pop = pop->next)
    {
      if (pop->power < 100 && pop->power > 0)
	{
	  if (resource_shortfall_percent > 0)
	    pop->power /= 2;
	  else
	    pop->power  += MAX(1, pop->power/100);
	}
      if ((soc = find_society_num (pop->society)) == NULL)
	continue;
      
      pop->power += ((pop_inc_pct+soc->alife_growth_bonus)*pop->power)/1000;   
    }
}  


/* This lets populations attack individual enemies. Generally the
   populations don't attack unless there's a resource shortfall,
   and they usually attack something weak. */

void
population_sector_attack (int x, int y, bool shortfall_exists)
{
  POPUL *pop, *weakest_enemy, *pop2;
  
  if (x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX)
    return;
  

  for (pop = sector_populs[x][y]; pop; pop = pop->next)
    {
      weakest_enemy = NULL;
      
      if (pop->alert > 0)
	pop->alert--;
      
      /* If it's a neutral society, it tends to leave things alone
	 unless resources are scarce. */
      
      if (pop->align == 0)
	{
	  if (!shortfall_exists)
	    continue;
	  
	  /* Even if resources are scarce, it doesn't attack a lot. */
	  if (nr (1,7) != 3)
	    continue;
	  
	  for (pop2 = sector_populs[x][y]; pop2; pop2 = pop2->next)
	    {
	      /* Don't attack self or a dead population or a 
		 population that's too buffed. */
	      
	      if (pop2 == pop || pop2->power == 0 ||
		  pop2->power > pop->power*3/2)
		continue;
	      
	      if (!weakest_enemy || pop2->power < weakest_enemy->power)
		weakest_enemy = pop2;
	      
	    }
	  
	  if (weakest_enemy)
	    population_attack (pop, weakest_enemy);
	}
      else
	{
	  
	  /* It only attacks sometimes. */
	  
	  if (nr (1,8) != 3 && !shortfall_exists)
	    continue;
	  
	  for (pop2 = sector_populs[x][y]; pop2; pop2 = pop2->next)
	    {
	      /* Don't attack self or a dead population or a population
		 based on the same align, or a population that's too
		 buffed. */
	      
	      if (pop2->power == 0 ||
		  !DIFF_ALIGN (pop->align, pop2->align) ||
		  pop2->power > pop->power*3/2)
		continue;
	      
	      if (!weakest_enemy || pop2->power < weakest_enemy->power)
		weakest_enemy = pop2;
	      
	    }
	  
	  if (weakest_enemy)
	    population_attack (pop, weakest_enemy);
	}
    } 
  return;
}

/* This lets all members of a (nonzero) alignment decide to gang up on
   one population of a different alignment. */

void
population_alignment_attack (int x, int y)
{
  RACE *align;
  int i, max_align_num = 0;
  int att_power = 0, def_power = 0, num_attackers = 0;
  int num_choices = 0, num_chose = 0, choice = 0;
  POPUL *pop;
  SOCIETY *soc;

  if (x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX)
    return; 
  
  if (nr (1,14) != 3)
    return;

  
  for (i = 0; i < ALIGN_MAX; i++)
    if ((align = find_align (NULL, i)) != NULL)
      max_align_num = i;
  
  /* Find the 2 aligns. */
  align = find_align (NULL, nr (1, max_align_num));
  
  /* Check if they're ok. */
  if (!align)
    return;
  
  for (pop = sector_populs[x][y]; pop; pop = pop->next)
    {
      if (!DIFF_ALIGN (pop->align, align->vnum))
	{
	  att_power += pop->power;
	  if ((soc = find_society_num (pop->society)) != NULL)
	    att_power += pop->power*soc->alife_combat_bonus/100;
	  num_attackers++;
	}
      else /* Number of choices to attack. */
	num_choices++;
    }

  if (num_choices < 1 || num_attackers < 1)
    return;

  num_chose = nr (1, num_choices);
  
  for (pop = sector_populs[x][y]; pop; pop = pop->next)
    {
      if (DIFF_ALIGN (pop->align, align->vnum) &&
	  ++choice >= num_chose)
	break;
    }
  
  if (!pop)
    return;
  
  def_power = pop->power;
  if ((soc = find_society_num (pop->society)) != NULL)
    pop->power += pop->power*soc->alife_combat_bonus/100;
  
  if (nr (1, att_power + def_power) <= att_power)
    {
      pop->power -= MAX (att_power/10, def_power/10);
    }
  else
    {
      for (pop = sector_populs[x][y]; pop; pop = pop->next)
	{
	  pop->power -= (MAX(att_power, def_power))/(10*num_attackers);
	}
    }
  return;
}


/* This lets a population send some of its population to an adjacent 
   sector. It does this once in a while, but is more likely to do it if
   there's a resource shortfall. */

void
population_settle (int x, int y, bool shortfall_exists)
{
  POPUL *pop;
  int settle_direction, new_x, new_y;
  
  if (x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX)
    return;
  
  for (pop = sector_populs[x][y]; pop; pop = pop->next)
    {
      /* Only settle once in a while. */	  
      if (nr (1,32) != 27 &&
	  (!shortfall_exists || nr (1,10) != 4))
	continue;
      
      
      /* Pick a direction to settle and find the new x and y. */
      
      /* Dirs look like:
	 
      6 7 8
      3 4 5
      0 1 2
      
      4 maps to nowhere. */
      
      settle_direction = nr (0, 8);
      new_x = x + (settle_direction % 3) - 1;
      new_y = y + (settle_direction / 3) - 1;
      
      /* Don't settle in same square. */
      if (new_x == x && new_y == y)
	continue;
      
      /* Make sure the new coords are legit. */
      
      if (new_x < 0 || new_x >= SECTOR_MAX || new_y < 0 || new_y >= SECTOR_MAX)
	continue;
      
      
      /* This adds population to a certain sector based on the society
	 number. */
      
      add_population_power ( new_x, new_y, pop->society, MAX(1,pop->power/3));
      pop->power -= pop->power/3;      
    }
  return;
}


/* This lets populations assist populations of the same society in 
   adjacent sectors if they need it. */

void
population_assist (int x, int y)
{
  POPUL *pop, *pop2;
  int adjacent_need_help = 0;
  int count, dir;
  int new_x, new_y;
  
  
  if (x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX)
    return;
  
 for (pop = sector_populs[x][y]; pop; pop = pop->next)
   {
      /* Find how many populations need help, then split 10pct of power
	 among all of those that need help. */
     
      adjacent_need_help = 0;
      
      /* Small populations or ones on alert don't assist. */
      if (pop->power < 200 || pop->alert > 0 || nr (1,78) != 3)
	continue;
      
      /* Check all adjacent directions. */
      
      
      for (count = 0; count < 2; count++)
	{	     
	  if (count == 1 && adjacent_need_help < 1)
	    continue;
	  
	  for (dir = 0; dir < 9; dir++)
	    {
	      new_x = x + (dir % 3) - 1;
	      new_y = y + (dir / 3) - 1;
	      
	      if ((new_x == x && new_y == y) ||
		  new_x < 0 || new_y < 0 ||
		  new_x >= SECTOR_MAX || new_y >= SECTOR_MAX)
		continue;

	      /* Give help to anything that needs it. */

	      for (pop2 = sector_populs[new_x][new_y]; pop2; pop2 = pop2->next)
		{
		  if (pop2->society == pop->society && 
		      pop2->alert > 2)
		    {
		      if (count == 0)
			adjacent_need_help++;
		      else 
			{
			  pop2->power += pop->power/(10*adjacent_need_help);
			}
		    }
		}
	    }
	  /* Remove the power you just gave out. */
	  if (count == 1 && adjacent_need_help > 0)
	    pop->power -= pop->power/10;
	}
    }  
 return;
}

/* This cleans up "dead" populations in a sector. */

void 
population_sector_cleanup (int x, int y)
{
  POPUL *pop, *popn;

  if (x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX)
    return;
  
  for (pop = sector_populs[x][y]; pop; pop = popn)
    {
      popn = pop->next;
      if (pop->power <= 0)
	free_popul (pop);
    }
  return;
}
  

/* This function adds population to all of the "homeland" sectors for
   each society so that they don't die off. */

void
update_population_homelands (void)
{
  SOCIETY *soc;
  POPUL *pop;
  int curr_power;
  int x, y, min_x, max_x, min_y, max_y, curr_x, curr_y;
  
  for (soc = society_list; soc; soc = soc->next)
    {
      if (IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE))
	continue;

      /* Do this VERY infrequently. */
      if (nr (1,35) != 3)
	continue;
      
      if ((x = soc->alife_home_x) < 0 || x >= SECTOR_MAX ||
	  (y = soc->alife_home_y) < 0 || y >= SECTOR_MAX)
	continue;
      
      /* The society has a valid homeland location at this point. */
      
      /* Find all of the coordinates around the central sector. */

      min_x = MAX (0, x - HOMELAND_RADIUS);
      max_x = MIN (SECTOR_MAX -1, x + HOMELAND_RADIUS);
      min_y = MAX (0, y - HOMELAND_RADIUS);
      max_y = MIN (SECTOR_MAX - 1, y + HOMELAND_RADIUS);
      
      /* Loop through all of the valid sectors and make the population 
	 grow. */
      
      for (curr_x = min_x; curr_x <= max_x; curr_x++)
	{
	  for (curr_y = min_y; curr_y <= max_y; curr_y++)
	    {
	      if ((pop = find_popul_sector (curr_x, curr_y, soc->vnum)) != NULL)
		curr_power = pop->power;
	      else
		curr_power = 0;
		add_population_power (curr_x, curr_y, soc->vnum, 
				      MAX (1000, curr_power/5));
	    }
	}
    }
  return;
}


/* This lets a population attack another. */
void
population_attack (POPUL *attacker, POPUL *defender)
{
  int total_power, power_lost;
  int apow, dpow; /* Attacker and defender power. */
  SOCIETY *soc;
  /* Don't fight if nothing's there to do. */

  if (!attacker || !defender || attacker->power < 1 || defender->power < 1)
    return;
  
  
  apow = attacker->power;
  if ((soc = find_society_num (attacker->society)) != NULL)
    apow += apow*soc->alife_combat_bonus/100;
  dpow = defender->power;
  if ((soc = find_society_num (defender->society)) != NULL)
    dpow += dpow*soc->alife_combat_bonus/100;
  
  total_power = apow + dpow;
  
  power_lost = MAX (apow, dpow)/10;
  
  if (nr (1,total_power) <= apow)
    {
      add_population_power (defender->x, defender->y, defender->society, -power_lost);
      defender->alert++;
    }
  else
    {
      add_population_power (attacker->x, attacker->y, attacker->society, -power_lost);
      attacker->alert++;
    }
  
  return;
}
  

/* This finds a population of a certain society number in a sector. */

POPUL *
find_popul_sector (int x, int y, int society)
{
  POPUL *pop;
  
  if (x < 0 || y < 0 || x >= SECTOR_MAX || y >= SECTOR_MAX ||
      society < 1)
    return NULL;

  for (pop = sector_populs[x][y]; pop; pop = pop->next)
    if (pop->society == society)
      return pop;
  return NULL;
}


/* This adds (or subtracts) population from a certain society in a certain 
   sector. It does not remove "Dead" populations. */

void
add_population_power (int x, int y, int society_num, int amount)
{
  POPUL *pop;
  SOCIETY *society;
  if (x < 0 || y < 0 || y >= SECTOR_MAX || x >= SECTOR_MAX ||
      amount == 0 || (society = find_society_num (society_num)) == NULL)
    return;
  
  if ((pop = find_popul_sector (x, y, society->vnum)) == NULL)
    {
      if (amount > 0)
	{
	  pop = new_popul();
	  pop->society = society_num;
	  pop->align = society->align;
	  pop->x = x;
	  pop->y = y;
	  popul_to_sector (pop, x, y);
	}
      else
	return;
    }
  pop->power += amount;
  if (pop->power > 500000)
    pop->power = 500000;
  if (pop->power <= 0)
    free_popul (pop);
  return;
}

/* This shows the sector map to the player. It's a general map that
   has broad info about how the world looks. */

void
do_sectors (THING *th, char *arg)
{
  int x = 0, y = 0, i; /* Various counters. */
  char mapbuf[STD_LEN*5]; 
  int curr_color= 7, color = 7;
  char *t;  /* Used for creating the strings to send. */
  int sector;
  int highest_bit_set, curr_power;
  SOCIETY *soc = NULL;
  int total_power;
  int maxpow;
  POPUL *pop;
  RACE *align = NULL;
  bool all_aligns = FALSE;
  int popul_power[ALIGN_MAX];
  int sole_align_with_pop;
  int num_aligns_with_pop;
  
  if (!th || LEVEL(th) < BLD_LEVEL)
    return;
  
  /* If we pass a name to the function, find the society it belongs to,
     then give us a map of how powerful that society is and how spread
     out it is in the overall map. */
  
  if (!arg || !*arg)
    {
      for (y = SECTOR_MAX-1; y >= 0; y--)
	{
	  t = mapbuf;
	  for (x = 0; x < SECTOR_MAX; x++)
	    {
	      sector = dominant_terrain[x][y];
	      if (sector < 0 || sector >= PIXMAP_MAX)
		sector = 0;
	      /* Get the color of this sector. If it different than the
		 current color, change the color. */
	      color = pixmap_symbols[sector][0]*8 + 
		pixmap_symbols[sector][1];
	      if (curr_color != color)
		{
		  *t++ = '\x1b'; 
		  *t++ = '[';
		  *t++ = pixmap_symbols[sector][0];
		  *t++ = ';';
		  *t++ = '3';
		  *t++ = pixmap_symbols[sector][1];;
		  *t++ = 'm';
		  curr_color = color;
		}
	      *t++ = pixmap_symbols[sector][2];		  
	    }
	  *t++ = '\n';
	  *t++ = '\r';
	  *t = '\0';      
	  stt (mapbuf, th);
	}
      stt ("\x1b[0;37m", th);
      return;
    }

  if (!str_cmp (arg, "clear") || !str_cmp (arg, "reset"))
    {
      clear_all_sectors();
      seed_populations();
      stt ("Ok, all populations cleared from all sectors.\n\r", th);
      return;
    }
  
  if (!str_prefix (arg, "alignments"))
    all_aligns = TRUE;
  else
    {	  	  
      align = find_align (arg, -1);
      
      if (!align)
	{
	  for (soc = society_list; soc; soc = soc->next)
	    {
	      if (!str_prefix (arg, soc->name) &&
		  !IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE))
		break;
	    }
	  if (!soc)
	    {
	      stt ("There are no societies or alignments with that name!\n\r", th);
	      return;
	    }
	}
    }
  
  for (y = SECTOR_MAX-1; y >= 0; y--)
    {
      t = mapbuf;
      for (x = 0; x < SECTOR_MAX; x++)
	{
	  total_power = 0;
	  if (all_aligns)
	    {
	      for (i = 0; i < ALIGN_MAX; i++)
		popul_power[i] = 0;
	      
	      for (pop = sector_populs[x][y]; pop; pop = pop->next)
		{
		  if (pop->align >= 0 && pop->align < ALIGN_MAX)
		    {
		      popul_power[pop->align] += pop->power;
		      total_power += pop->power;
		    }
		}
	      if (total_power == 0)
		*t++ = ' ';
	      else
		{
		  num_aligns_with_pop =  0;
		  sole_align_with_pop = -1;
		  
		  for (i = 0; i < ALIGN_MAX; i++)
		    {
		      if (popul_power[i] > 0)
			{
			  num_aligns_with_pop++;
			  if (num_aligns_with_pop == 1)
			    sole_align_with_pop = i;
			  else
			    sole_align_with_pop = -1;
			}
		    }
		  
		  highest_bit_set = 0;
		  curr_power = total_power/1000;
		  for (maxpow = 0; maxpow < 20; maxpow++)
		    {
		      if (IS_SET (curr_power, (1<<maxpow)))
			highest_bit_set = MIN(9,maxpow);
		    }
		  
		  if (sole_align_with_pop == -1)
		    color = 6;	
		  else 
		    color = sole_align_with_pop % 8;
		  
		  if (curr_color != color)
		    {
		      sprintf (t, "\x1b[1;3%dm", color % 8);
		      t += 7;
		      curr_color = color;
		    }
		  *t++ = highest_bit_set + '0';
		}
	    }	      
	  else
	    {
	      if (align)
		{
		  for (pop = sector_populs[x][y]; pop; pop = pop->next)
		    {
		      if (!DIFF_ALIGN(pop->align,align->vnum))
			total_power += pop->power;
		    }
		}
	      else
		{
		  pop = find_popul_sector (x, y, soc->vnum);
		  if (pop)
		    total_power = pop->power;
		}
	      if (total_power < 1)
		*t++ = ' ';
	      else
		{
		  highest_bit_set = 0;
		  curr_power = total_power/1000;
		  for (maxpow = 0; maxpow < 20; maxpow++)
		    {
		      if (IS_SET (curr_power, (1<<maxpow)))
			highest_bit_set = maxpow;
		    }
		  *t++ = MID (0, highest_bit_set, 9) + '0';
		}
	    }
	}
      sprintf (t, "\n\r");
      stt (mapbuf, th);
    }
  stt ("\x1b[0;37m", th);
  return;
}

/* This clears all of the populations from all of the sectors. */

void
clear_all_sectors (void)
{
  int x, y;
  POPUL *pop, *popn;
  
  for (x = 0; x < SECTOR_MAX; x++)
    {
      for (y = 0; y < SECTOR_MAX; y++)
	{
	  for (pop = sector_populs[x][y]; pop; pop = popn)
	    {
	      popn = pop->next;
	      free_popul (pop);
	    }
	}
    }
  return;
}

/* This will seed populations into the giant map if there aren't any
   right now. The idea is to find the number of societies that are
   indestructable and put a few of them into the world in various
   places if they don't exist already. */


void
seed_populations (void)
{
  SOCIETY *soc; /* Used for going down list of societies. */
  POPUL *pop;   /* Used for going down list of populations. */
  int x, y;     /* Counters used for sectors. */
  int new_x, new_y; /* Where a new population will get put. */
  int pop_count[SOCIETY_HASH]; /* Only check first N societies. */
  int i, times; /* General counters */
  
  for (i = 0; i < SOCIETY_HASH; i++)
    {      
      pop_count[i] = 0;
    }
  

  /* Now make a count of how many populations are out there in the first
     place. */


  for (x = 0; x < SECTOR_MAX; x++)
    {
      for (y = 0; y < SECTOR_MAX; y++)
	{
	  for (pop = sector_populs[x][y]; pop; pop = pop->next)
	    {
	      if ((soc = find_society_num (pop->society)) != NULL &&
		  pop->society > 0 && pop->society < SOCIETY_HASH &&
		  !IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE))
		{
		  pop_count[pop->society]++;
		}
	    }
	}
    }
  
  
  /* Seed more populations if there aren't enough of certain kinds. */
  
  for (i = 0; i < SOCIETY_HASH; i++)
    {
      /* If there aren't enough populations out there, but the society
	 exists, add some of them. */
      if (pop_count[i] < POPUL_SEED_COUNT &&
	  (soc = find_society_num (i)) != NULL &&
	  !IS_SET (soc->society_flags, SOCIETY_DESTRUCTIBLE))
	{
	  for (times = 0; times < (POPUL_SEED_COUNT-pop_count[i]); times++)
	    {
	      new_x = nr (0, (SECTOR_MAX-1));
	      new_y = nr (0, (SECTOR_MAX-1));
	      add_population_power (new_x, new_y, i, nr (1000, 2000));
	    }
	}
    }

  return;


}
 
/* This adds or removes the level of the thing to or from the
   population of the sector that it's in. Useful for making/destroying
   population mobs within sectors. */

void 
update_sector_population_thing (THING *th, bool add)
{
  int sect_x, sect_y;
  VALUE *popul;
  
  
  if (!th || !th->in || !IS_WILDERNESS_ROOM (th->in))
    return;
  
  if ((popul = FNV (th, VAL_POPUL)) == NULL)
    return;
  
  /* Find the x and y coords for this room. */
  
  
  sect_x = ((th->in->vnum - WILDERNESS_MIN_VNUM) % WILDERNESS_SIZE)/SECTOR_SIZE;
  sect_y = ((th->in->vnum - WILDERNESS_MIN_VNUM) / WILDERNESS_SIZE)/SECTOR_SIZE;
  
  
  if (sect_x < 0 || sect_y < 0 || sect_x >= SECTOR_MAX || sect_y >= SECTOR_MAX)
    return;
  
  
  add_population_power (sect_x, sect_y, popul->val[0], LEVEL(th)*(add?1:-1));
  
}


  /* This creates an encounter for a player in the wilderness. There are
   2 types of encounters in the broad sense. There is the random encounter
   with wilderness creatures, then there are encounters with populations
   of organized mobs. The first step is to pick the kind of encounter that
   will happen, then the next is to set up the encounters (in separate
   functions */

void
create_encounter (THING *th)
{
  int x, y;
  int sect_x, sect_y; /* What x and y sectors we end up in. */
  int animal_encounter_pct = 99;
  int raw;
  if (!th || !IS_PC (th) || !IS_WILDERNESS_ROOM(th->in))
    return;
  
  /* These values should be ok because of the previous check, but
     I will check anyway. */
  
  x = (th->in->vnum - WILDERNESS_MIN_VNUM) % WILDERNESS_SIZE;
  y = (th->in->vnum - WILDERNESS_MIN_VNUM) / WILDERNESS_SIZE;
  
  if (x < 0 || y < 0 || x >= WILDERNESS_SIZE || y >= WILDERNESS_SIZE)
    return;
  
  sect_x = x/SECTOR_SIZE;
  sect_y = y/SECTOR_SIZE;
  
  if (sect_x >= SECTOR_MAX || sect_y >= SECTOR_MAX)
    return;
  
  /* Now compare the resources available in the sector to the amount
     used up by the populations. The more resources that are being
     used up, the smaller the chance of encountering a random monster
     instead of some population members. */

  for (raw = 0; raw < RAW_MAX; raw++)
    {
      if (sector_used_res[x][y][raw] >= sector_curr_res[x][y][raw]/2 &&
	  nr (1,5) == 3)
	animal_encounter_pct--;
    }
  
  if (np() <= animal_encounter_pct && 0)
    create_animal_encounter (th);
  else
    create_society_encounter (th);
}

/* This will create an animal encounter for the player to see. It will place
   a few mobs nearby the character in the wildernes area. */

void
create_animal_encounter (THING *th)
{
  int x, y;
  int sect_x, sect_y;
  int num_animals = nr (1,ENCOUNTER_MAX_SIZE);
  int animal_count;  
  int min_x, max_x, min_y, max_y;
  int curr_x, curr_y;
  int count, mob_vnum, room_vnum;
  int roomflags_needed;
  int num_choices, num_chose, choice;
  THING *proto, *mob, *randpop_item, *room = NULL;
  VALUE *randpop;
  bool found_room = FALSE;
   if (!th || !IS_PC (th) || !IS_WILDERNESS_ROOM(th->in))
    return;
   
   /* See if the mob randpop item exists. */
   
   if ((randpop_item = find_thing_num (MOB_RANDPOP_VNUM)) == NULL ||
       (randpop = FNV (randpop_item, VAL_RANDPOP)) == NULL)
     {
       char errbuf[STD_LEN];
       sprintf (errbuf, "Warning, randpop item %d doesn't exist or isn't a randpop item, so wilderness encounters cannot be made!\n", MOB_RANDPOP_VNUM);
       log_it (errbuf);
       return;
     }
   
   /* Should be ok because of the previous check, but
     I will check anyway. */
   
  x = (th->in->vnum - WILDERNESS_MIN_VNUM) % WILDERNESS_SIZE;
  y = (th->in->vnum - WILDERNESS_MIN_VNUM) / WILDERNESS_SIZE;
  
  if (x < 0 || y < 0 || x >= WILDERNESS_SIZE || y >= WILDERNESS_SIZE)
    return;
  
  sect_x = x/SECTOR_SIZE;
  sect_y = y/SECTOR_SIZE;
  
  if (sect_x >= SECTOR_MAX || sect_y >= SECTOR_MAX)
    return;
  
  /* Find the min and max coordinates where the new creatures will
     show up. */
  
  min_x = MAX (0, x-ENCOUNTER_RADIUS);
  max_x = MIN (WILDERNESS_SIZE-1, x+ENCOUNTER_RADIUS);
  min_y = MAX (0, y-ENCOUNTER_RADIUS);
  max_y = MIN (WILDERNESS_SIZE-1, y+ENCOUNTER_RADIUS);
  
  /* For each animal created... */

  for (animal_count = 0; animal_count < num_animals; animal_count++)
    {
      
      /* Figure out which creature gets made (if any). */
      
      mob_vnum = calculate_randpop_vnum (randpop, LEVEL(th));
      if ((proto = find_thing_num (mob_vnum)) == NULL)
	continue;
      
      /* Find out what special roomflags are needed for this mob. */
  
      roomflags_needed = flagbits (proto->flags, FLAG_ROOM1) & ROOM_SECTOR_FLAGS;
      
      /* Now find the room where the mob will go (if any exists) by
	 checking how many rooms nearby have the proper flags 
	 if they're needed. */
      
      num_choices = 0;
      num_chose = 0;
      choice = 0;
      found_room = FALSE;
      for (count = 0; count < 2; count++)
	{
	  for (curr_x = min_x; curr_x <= max_x && !found_room; curr_x++)
	    {
	      for (curr_y = min_y; curr_y <= max_y; curr_y++)
		{
		  if (curr_x == x && curr_y == y)
		    continue;
		  
		  room_vnum = curr_x + WILDERNESS_SIZE*curr_y + WILDERNESS_MIN_VNUM;
		  if ((room = find_thing_num (room_vnum)) == NULL)
		    continue;
		  if (!roomflags_needed ||
		      IS_ROOM_SET (room, roomflags_needed))
		    {
		      if (count == 0)
			num_choices++;
		      else if (++choice >= num_chose)
			{
			  found_room = TRUE;
			  break;
			}		      
		    }
		}
	    }
	  
	  /* Now find which room we will use. If none are available, just
	     bail out of the loop. */
	  if (count == 0)
	    {
	      if (num_choices < 1)
		count++;
	      else
		num_chose = nr (1, num_choices);
	    }
	  else if (room && choice >= num_chose)
	    {
	      if ((mob = create_thing (mob_vnum)) != NULL)
		thing_to (mob, room);
	    }
	}
    }
  return;
}


/* This will create an encounter of society mobs near the player. 
   It does this by picking one of the populations to use (chance 
   proportional to population size), then it loads some mobs based 
   on the overall power of the population. You encounter more if 
   there are more mobs of those populations out there.*/

void
create_society_encounter (THING *th)
{
  THING *mob, *proto, *room;
  int caste_num, caste_tier;
  int min_x, max_x, min_y, max_y;
  int new_x, new_y, x, y, i;
  int sect_x, sect_y;
  int total_power = 0, power_chose = 0, current_power = 0;
  int encounter_power, power_used = 0;
  int check_counter = 0;
  POPUL *pop;
  SOCIETY *soc;
  VALUE *popval;
  
  x = (th->in->vnum - WILDERNESS_MIN_VNUM) % WILDERNESS_SIZE;
  y = (th->in->vnum - WILDERNESS_MIN_VNUM) / WILDERNESS_SIZE;
  
  if (x < 0 || y < 0 || x >= WILDERNESS_SIZE || y >= WILDERNESS_SIZE)
    return;
  
  sect_x = x/SECTOR_SIZE;
  sect_y = y/SECTOR_SIZE;
  
  if (sect_x >= SECTOR_MAX || sect_y >= SECTOR_MAX)
    return;
  
  /* Find the min and max coordinates where the new creatures will
     show up. */
  
  min_x = MAX (0, x-ENCOUNTER_RADIUS);
  max_x = MIN (WILDERNESS_SIZE-1, x+ENCOUNTER_RADIUS);
  min_y = MAX (0, y-ENCOUNTER_RADIUS);
  max_y = MIN (WILDERNESS_SIZE-1, y+ENCOUNTER_RADIUS);
  
  /* First find population power in this sector and pick a population. */
  
  for (pop = sector_populs[sect_x][sect_y]; pop; pop = pop->next)
    total_power += pop->power;
  power_chose = nr (1, total_power);
  for (pop = sector_populs[sect_x][sect_y]; pop; pop = pop->next)
    {
      current_power += pop->power;
      if (current_power >= power_chose)
	break;
    }
  
  if (!pop)
    return;
  
  if ((soc = find_society_num (pop->society)) == NULL)
    return;
  
  /* The power you encounter is a percent of the total power of the 
     population, NOT of how powerful you the character are! */
  
  encounter_power = MAX(500, pop->power*nr(5,10)/100);
  if (encounter_power > pop->power)
    encounter_power = pop->power;
  
  /* Keep creating mobs till you use up all of the available power. */
  
  while (power_used < encounter_power && ++check_counter < 100)
    {
      /* Find where this mob will go. */
      new_x = nr (min_x, max_x);
      new_y = nr (min_y, max_y);      
      if ((new_x == x && new_y == y) ||
	  (room = find_thing_num (new_x + WILDERNESS_SIZE*new_y +
	   WILDERNESS_MIN_VNUM)) == NULL ||
	  IS_ROOM_SET (room, BADROOM_BITS))
	continue;
      
      /* Find the caste and the tier within that caste to load up. */
      if ((caste_num = find_caste_from_percent (soc, TRUE, FALSE)) < 0 ||
	  caste_num >= CASTE_MAX)
	continue;
      caste_tier = 0;
      for (i = 0; i < soc->max_tier[caste_num]; i++)
	{
	  if (nr(1,4) == 3)
	    caste_tier++;
	  else
	    break;
	}
      
      /* Create the mob. */
      if ((proto = find_thing_num (soc->start[caste_num]+caste_tier)) == NULL)
	continue;
      
      if ((mob = create_thing (proto->vnum)) == NULL)
	continue;
      
      if ((popval = FNV (mob, VAL_POPUL)) == NULL)
	{
	  popval = new_value();
	  popval->type = VAL_POPUL;
	  add_value (mob, popval);
	}
      popval->val[0] = soc->vnum;
      mob->align = soc->align;
      add_flagval (mob, FLAG_ACT1, ACT_KILL_OPP);
      update_sector_population_thing (mob, FALSE);
      thing_to (mob, room);
      power_used += LEVEL (mob);
    }
  pop->power -= power_used;
  return;
}
  
