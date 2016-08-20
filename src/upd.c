#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "serv.h"
#include "society.h"
#include "track.h"
#include "rumor.h"
#ifdef USE_WILDERNESS
#include "wilderness.h"
#endif

/* This command lets an admin force updates to happen. */

void
do_update (THING *th, char *arg)
{
  if (!th || !IS_PC (th) || LEVEL(th) != MAX_LEVEL)
    return;
  
  stt ("This does nothing for right now.\n\r", th);	
  return;
}
/* This sends a charge message to a thing if it's wearing an item
   that has to charge up (gem or powershield) */


void
charge_message (THING *obj, VALUE *val, THING *th)
{
    int tenth;
    if (!obj || !val || !th || val->val[2] < 1)
	return;
    
    tenth = (val->val[1] * 10)/ val->val[2];
    
    switch (tenth)
    {
	default:
	case 0:
	case 1:
	    break;
	case 2:
	case 3:
	    act ("@3n is beginning to glow dimly.", th, NULL, obj, NULL, TO_CHAR);
	    break;
	case 4:
	case 5:
	    act ("@3n is beginning to warm up.", th, NULL, obj, NULL, TO_CHAR);
      break;
	case 6:
	case 7:
	  act ("@3n is filled with a warm glow.", th, NULL, obj, NULL, TO_CHAR);
	  break;
	case 8:
	case 9:
	  act ("@3n is glowing brightly!", th, NULL, obj, NULL, TO_CHAR);
	  break;
	case 10:
	  act ("@3n is fully charged.", th, NULL, obj, NULL, TO_CHAR);
	  break;
    }
    return;
}

/* This updates the png once in a while. What it does is count
   the number of bytes read in by the game for a minute or so. If
   that number is sufficiently large, and if enough people are
   online, the number of bytes entered is taken mod 2 and that 
   becomes a bit used for the next update cycle. */

void
update_png (void)
{
  int fd_count = 0, i;
  FILE_DESC *fd;
  
  /* If error, try to bail out as gracefully as possible...*/

  if (png_count < 0 || png_count > 31)
    {
      png_count = 0;
      png_seed = 0;
      for (i = 0; i < 32; i++)
	png_seed |= (nr(0,1) << i);
      my_srand (png_seed);
      png_seed = 0;
      png_bytes = 0;
      return;
    }
  
  for (fd = fd_list; fd; fd = fd->next)
    fd_count++;
  
  /* If there isn't enough data, try to fake it. */

  if (fd_count < 3 || png_bytes < 1000)
    png_seed |= (nr(0,1) << png_count);
      
  /* If all the bits are filled, reseed and reset. */

  if (++png_count == 32)
    {
      my_srand(png_seed);
      png_seed = 0;
      png_count = 0;
      png_bytes = 0;
    }
  return;
}

/* Happens every 8-9 seconds or so...really long, sorry about that. 
   This is the fundamental update routine for things. It deals
   with healing/wandering/casting and attacking and stuff like that.
   Tracks now get checked for "dead" things every 8 seconds.
   Then, the list of things in thing_free_track gets moved to 
   the thing_free list for reuse.  while the update at 2-2.5 seconds
   is only for things in the same location as a PC. */

/* Since load balancing is added, if all_now is set, all things
   get updated in the table all at once, else the update only
   does a small part of the list. */


void
update_thing (THING *th)
{
  int mana_rate, hurt_by, hp_rate = 0, mv_rate = 0, actbits, dir;
  THING *obj, *new_item, *room;
  VALUE *gem, *shop, *shop2 = NULL, *pet, *soc;
  RACE *race = NULL, *align = NULL;
  int regen_pct = 100;
  /* Ignore areas and protos which are not rooms. */
  
  
  if (!th || IS_AREA (th) || 
      !th->in ||
      (IS_AREA (th->in) && !IS_ROOM (th)))
    {
      return;
    }
  
  
  
  /* Only check mobs and pc's online now. */
  
  if (IS_ROOM (th) || (!CAN_MOVE (th) && !CAN_FIGHT (th)) ||
      (IS_PC (th) && th->fd && th->fd->connected > CON_ONLINE))
    return;
  
 
  

  if (IS_PC (th))
      { 
	/* Check for regen pct in astral areas. */
	if (th->align < ALIGN_MAX &&
	    th->in && th->in->in && IS_AREA (th->in->in) &&
	    IS_ROOM_SET (th->in->in, ROOM_ASTRAL) &&
	    align_info[th->align])
	  regen_pct = 2*align_info[th->align]->power_pct-100;	
	hp_rate = 4 + LEVEL (th)/10;
      }
  else
    hp_rate = 1 + LEVEL (th)/5;
  
  mv_rate = 3 + LEVEL (th)/20;
  
  if (th->position <= POSITION_RESTING)
    {
      hp_rate += 2;
      hp_rate *= 2;
      mv_rate += 2;
      mv_rate *= 2;
      if (th->position == POSITION_SLEEPING)
	{
	  hp_rate *= 2;
	  mv_rate *= 2;
	}
    }
  
  if (IS_PC (th))
    {
      if ((race = FRACE(th)) != NULL && 
	  (align = FALIGN (th)) != NULL)
	{
	  hp_rate = (hp_rate*(100+race->hp_regen_bonus+align->hp_regen_bonus)/100);
	  mv_rate = (hp_rate*(100+race->mv_regen_bonus+align->mv_regen_bonus)/100);
	  if (hp_rate < 0)
	    hp_rate = 0;
	  if (mv_rate < 0)
	    mv_rate = 0;
	}
    }
  if ((hurt_by = flagbits (th->flags, FLAG_HURT)) != 0)
    {
      if (IS_SET (hurt_by, AFF_DISEASE))
	{
	  if (IS_PC (th))
	    {
	      if (damage (th, th, MAX (3, th->hp/20), "plague"))
		return;
	      hp_rate /=2;
	      mv_rate /=2;
	    }
	  else
	    {
	      hp_rate = 0;
	      mv_rate = 0;
	      if (damage (th, th, MAX (3, th->hp/10), "plague"))
		return;
	    }
	  th->mv -= MAX (3, th->mv/20);
	  if (nr (1,5) == 2)
	    act ("@1n starts to @t.", th, NULL, NULL,
		 (char *) disease_symptoms[nr (0, MAX_DISEASE_SYMPTOMS-1)], TO_ALL);	    
	  if (nr (1,37) == 23 && th->in)
	    {
	      THING *other, *othern;
	      for (other = th->in->cont; other; other = other = othern)
		{
		  othern = other->next_cont;
		  if (nr (1,19) == 7 && CAN_MOVE (other) && 
		      !IS_PROT (other, AFF_DISEASE) &&
		      other != th && !IS_HURT (other, AFF_DISEASE))
		    {
		      FLAG *diseased;
		      for (diseased = th->flags; diseased; diseased = diseased->next)
			if (diseased->type == FLAG_HURT &&
			    IS_SET (diseased->val, AFF_DISEASE) &&
			    diseased->from == 0 &&
			    diseased->timer > 0)
			  break;
		      if (!diseased)
			{
			  diseased = new_flag ();
			  diseased->type = FLAG_HURT;
			  diseased->val = AFF_DISEASE;
			  aff_update (diseased, other);
			}
		      diseased->type = FLAG_HURT;
		      diseased->val = AFF_DISEASE;
		      diseased->timer = nr (3,6);
		      diseased->from = 0;
		      act ("$9@1n just got the plague!$7", other, NULL, NULL, NULL, TO_ALL);
		    }
		}
	    }
	}
      
      if (IS_SET (hurt_by, AFF_WOUND))
	{
	  hp_rate /= 2;
	  mv_rate /= 2;
	  th->mv--;
	  if (nr (1,20) == 3)
	    act ("$9@1n bleed@s profusely from wounds that will not close.$7", th, NULL, NULL, NULL, TO_ALL);	    
	  if (th->in && IS_ROOM (th->in)) 
	    th->in->hp |= BLOOD_POOL;
	  if (damage (th, th, 8, "wound"))
	    return;
	}
    }
  
  
  th->hp += MIN (hp_rate, th->max_hp - th->hp)*regen_pct/100;
  th->mv += MIN (mv_rate, th->max_mv - th->mv)*regen_pct/100;
  
  if (IS_PC (th)) /* Pc update mana etc... */
    {
      mana_rate = (get_stat (th, STAT_INT) + get_stat (th, STAT_WIS)) / 16  + 1;
      mana_rate = mana_rate*regen_pct/100;
      if (th->position == POSITION_SLEEPING)
	mana_rate += 3;
      else if (th->position == POSITION_RESTING)
	mana_rate ++;
      else if (th->position == POSITION_MEDITATING)
	{
	  mana_rate += 3;
	  mana_rate *= 2;
	}
      
      th->pc->mana += MIN (mana_rate, th->pc->max_mana - th->pc->mana);
      
      for (obj = th->cont; obj; obj = obj->next_cont)
	{
	  if (obj->wear_loc != ITEM_WEAR_WIELD)
	    {
	      if (!IS_WORN(obj) ||
		  obj->wear_loc > ITEM_WEAR_WIELD)
		break;
	    }
	  
	  if ((gem = FNV (obj, VAL_GEM)) == NULL)
	    continue;
	  if (gem->val[1] < gem->val[2])
	    {
	      gem->val[1] += MIN (mana_rate, gem->val[2] - gem->val[1]);
	      if (nr (1,10) == 2 || gem->val[1] == gem->val[2])
		charge_message (obj, gem, th);
	    }
	}
      if (th->fd)
	th->pc->hostnum = th->fd->hostnum;
      
      if (nr (1,3) == 2)
	check_weather_effects (th);
    }
  else
    { /* Make new stuff at shops... */
      int itemcount = 0;
      if ((shop = FNV (th, VAL_SHOP)) != NULL)
	{
	  

	  if (shop->val[3] > 0 && /* Hours */
	      shop->val[4] > 0 && /* Item number */
	      wt_info->val[WVAL_HOUR] % shop->val[3] == 0 && 
	      nr (1,10) == 2 &&
	      (new_item = find_thing_num (shop->val[4])) != NULL &&
	      !IS_SET (new_item->thing_flags, TH_IS_ROOM | TH_IS_AREA) &&
	      !CAN_MOVE (new_item) && !CAN_FIGHT (new_item))
	    {
	      for (obj = th->cont; obj; obj = obj->next_cont)
		{
		  if (obj->vnum == shop->val[4])
		    itemcount++;
		}
	      if (itemcount < 10 &&
		  (new_item = create_thing (shop->val[4])) != NULL)
		{
		  thing_to (new_item, th);
		  act ("@1n just finished making @3n.", th, NULL, new_item, NULL, TO_ALL);
		}
	    } 
	  /* Add some money to the shops...if it is below 10x their 
	     level. This may be a bit high, so feel free to change
	     it if you feel there is too much money out there. :)
	     This seems necessary since there are no more "reboots"
	     to pop money on the shopkeepers. */
	  
	  if ((shop2 = FNV (th, VAL_SHOP2)) != NULL && nr (1,10) == 2)
	    calculate_shop_value (th, shop, shop2);
	  
	  
	  if (total_money (th) < LEVEL (th) * 10 && nr (1,20) == 2)
	    add_money (th, MAX (10, (LEVEL (th) * 10 - total_money (th))/10));

	  /* If the shop is opening, then reset the mob again. */
	  if (wt_info && wt_info->val[WVAL_HOUR] == shop->val[0])
	    reset_thing (th, 0);
	}
      
      /* Sanity check fighting. */
      if (th->position == POSITION_FIGHTING &&
	  (!FIGHTING(th) || FIGHTING(th)->in != th->in))
	th->position = POSITION_STANDING;
      
      actbits = flagbits (th->flags, FLAG_ACT1);
      
      /* Make this thing do a society activity...*/
      if ((soc = FNV (th, VAL_SOCIETY)) != NULL &&
	  nr (1,6) != 2)
	{
	  society_activity (th);
	}
      /* Make this cast a spell. */
      else if (nr (1,170) == 3 && !is_hunting (th))
	{ /* Patrol */
	  find_new_patrol_location (th);
	}
      else if ((pet = FNV (th, VAL_PET)) == NULL || !pet->word ||
	       !*pet->word)
	{ /* Otherwise, wander. Make sure the thing isn't
	     fighting or hunting or anything and that
	     it's in a room and that it's not a sentinel
	     warrior for a caste and that either the new
	     room is in the same area where th was created or
	     the random number comes up. */
	  if (nr (1,12) == 3 && 
	      CAN_MOVE (th) &&  
	      !is_hunting (th) &&
	      !FIGHTING (th) && 
	      !IS_ROOM (th) && 
	      th->position >=
	      POSITION_STANDING && 
	      !IS_SET (actbits, ACT_SENTINEL | ACT_PRISONER) &&
	      !RIDER(th) && th->in && 
	      IS_ROOM (th->in) && th->proto &&
	      (!soc || !IS_SET (soc->val[2], CASTE_WARRIOR) ||
	       soc->val[3] != WARRIOR_SENTRY))
	    { 
	      dir = nr (0, REALDIR_MAX-1);
	      if (((room = FTR (th->in, dir, th->move_flags)) != NULL)
		  && (th->proto->in == room->in || nr (1,15) == 3))
		move_dir (th, dir, 0);
	      else if (nr (1,3) == 2)
		attack_stuff (th);
	      
	    } 
	}
      if (nr (1,70) == 2)
	find_eq_to_wear (th);
    }
  if (th->hp < 1 && CAN_FIGHT (th))
    get_killed (th, NULL);
  
  
  return;
}



/* This updates the world every game hour (one minute rl). It
   updates things like tracks, weather, flags and other things. */



void
update_hour (void)
{
  char buf[STD_LEN];
  if (!wt_info)
    {
      wt_info = new_value ();
      wt_info->val[WVAL_YEAR] = nr (200, 1200);
      wt_info->val[WVAL_MONTH] = nr (0, NUM_MONTHS - 1);
      wt_info->val[WVAL_DAY] = nr (0, NUM_WEEKS*NUM_DAYS - 1);
      wt_info->val[WVAL_HOUR] = nr (0, NUM_HOURS);
      wt_info->val[WVAL_TEMP] = nr (50, 75);
      wt_info->val[WVAL_WEATHER] = 0;
    }
  
  /* Check if we're autogenning the world...if so, and if the game has
     been up for a while, set the reboot ticks to reboot. */
  
  if (IS_SET (server_flags, SERVER_AUTO_WORLDGEN) &&
      (times_through_loop/UPD_PER_SECOND) >= (3600 * nr (12,16)) &&
      reboot_ticks == 0)
    {
      reboot_ticks = 15;
    }
  
  if (reboot_ticks > 0)
    {
      if (--reboot_ticks == 0)
	{
	  SBIT (server_flags, SERVER_REBOOTING);
	  return;
	}
      else
	{
	  if (IS_SET (server_flags, SERVER_AUTO_WORLDGEN))
	    {
	      echo ("\x1b[1;31mRebooting and rebuilding the world soon!\n\r");
	      echo ("\x1b[1;31mRebooting and rebuilding the world soon!\n\r");
	      echo ("\x1b[1;31mRebooting and rebuilding the world soon!\n\r");
	    }
	  sprintf (buf, "\x1b[1;36m<->\x1b[1;32m%d hour%s until mud save and reboot!\x1b[1;36m<->\x1b[0;37m\n\r", reboot_ticks, reboot_ticks > 1 ? "s" : "");
	  echo (buf);
	}
    }

  /* Update the world "clocks"...this is really simple, feel free to
     make this as complex as you want :) */
  
  update_time_weather ();

  create_disaster();
  
  if (nr (1,3) == 2)
    check_for_multiplaying ();

  /* At 3am and if the server is not sped up or if the random chance
     is met, we save the world snapshot and the unsaved areas. */
  
  if (wt_info->val[WVAL_HOUR] == 3 &&
      (!IS_SET (server_flags, SERVER_SPEEDUP) ||
       nr (1,20) == 4))
    {
      init_write_thread();
      write_area_list();
      write_changed_areas(NULL);
    }

  
  return;
}

/* This is the hourly update for a "thing". Their spells and whatnot 
   no longer wear off exactly on the hour. */



void
update_thing_hour (THING *th)
{
  char buf[STD_LEN]; 
  THING *proto, *th_in;
  FLAG *flag, *flagn;
  VALUE *feed, *light, *pet, *socval;
  VALUE *val, *valn;
  TRACK *trkn, *trk, *prev;
  int  i, j, feed_choices, feed_chose, condval;
  bool killed = FALSE;
  
  if (!th || !th->in || (IS_AREA (th->in)  && !IS_ROOM (th) &&
		  !IS_AREA (th)))
    return;
  
  RBIT (th->thing_flags, TH_MARKED);
  
  if (IS_AREA (th))
    {
      if (--th->timer < 1)
	{
	  if (!th->in || !IS_AREA (th->in) ||
	      !IS_AREA_SET (th->in, AREA_NOREPOP))
	    reset_thing (th, 0);
	  /* Send repop message to all players in the area. */
	  if (th->long_desc && th->long_desc[0])
	    {
	      THING *rth;
	      char popbuf[STD_LEN];
	      sprintf (popbuf, "%s\n\r", th->long_desc);
	      for (rth = thing_hash[PLAYER_VNUM % HASH_SIZE]; rth; rth = rth->next)
		{
		  if (rth->in && IS_ROOM (rth->in) && rth->in->in == th)
		    stt (popbuf, rth);
		}
	    }
	  th->timer = nr (10,20);
	}
      return;
    } 
  /* Removed timed objects and stuff. */
  
  if (th->timer > 0)
    {
      if (th->vnum == PACKAGE_VNUM)
	attack_package_holder (th);
      if (th->in &&
	  !IS_AREA (th->in) && 
	  (!IS_PC (th) || LEVEL (th) < MAX_LEVEL)
	  && --th->timer < 1)
	{
	  /* Don't let fighting things just poof...*/
	  if (FIGHTING (th))
	    {
	      th->timer = 2;
	    }
	  else if (IS_PC (th))
	    {
	      write_playerfile (th);
	      if (th->pc->no_quit < 1)
		free_thing (th);
	    } /* Society members die. */
	  else if ((socval = FNV (th, VAL_SOCIETY)) != NULL)
	    {
	      act ("$9@1n die@s of old age.$7", th, NULL, NULL, NULL, TO_ALL);
	      get_killed (th, NULL);
	      return;
	    }
	  else	      
	    { /* Replaceby is a simple way to make an ecology by making
		 things transform into other things when they time out. */
	      THING *replacer;
	      int replacer_vnum;
	      VALUE *replace;
	      
	      if (th->vnum == CORPSE_VNUM && th->in)
		{
		  THING *obj, *objn;
		  act ("@1n rot@s.", th, NULL, NULL, NULL, TO_ALL);
#ifndef ARKANE
		  for (obj = th->cont; obj; obj = objn)
		    {
		      objn = obj->next_cont;
		      thing_to (obj, th->in);
		    }
#endif
		}
	      if ((replace = FNV (th, VAL_REPLACEBY)) != NULL &&
		  (replacer_vnum = calculate_randpop_vnum (replace, LEVEL(th))) &&
		  replacer_vnum != th->vnum &&
		  (replacer = create_thing (replacer_vnum)) != NULL)
		replace_thing (th, replacer);
	      else
		free_thing (th);
	    }
	  return;
	}
    }
  
  if (th->fgt && th->fgt->hunting_timer > 0 && !IS_PC (th))
    {
      if (--th->fgt->hunting_timer == 0)
	stop_hunting (th, FALSE);
    }
  
  if (!th->in)
    return; 
  
  /* Remove blood...I use room->hp for blood. So sue me :P */

  if (IS_ROOM (th))
    {
      if (th->hp && nr(1,2) == 2)
	{
	  if (IS_SET (th->hp, NEW_BLOOD))
	    RBIT (th->hp, NEW_BLOOD);
	  else
	    {
	      for (j = 0; j < DIR_MAX; j++)
		{
		  if (IS_SET (th->hp, (1 << j)) && nr (1,3) == 2)
		    {
		      RBIT (th->hp, (1 << j));
		      if (th->cont)
			act ("$1A $9blood trail $1dries up.$7", th->cont, NULL, NULL, NULL, TO_ALL);
		    }
		}
	    }
	}
      
      /* Get rid of old tracks... one pass should be fast. */
      
      if ((trkn = th->tracks) != NULL)
	{
	  prev = NULL;
	  while ((trk = trkn))
	    {
	      trkn = trk->next;
	      if (--trk->timer < 1 || !trk->who || !trk->who->in ||
		  (!IS_PC (trk->who) && !trk->who->proto && !IS_ROOM (trk->who)))
		{
		  if (!prev)
		    th->tracks = trk->next;
		  else
		    prev->next = trk->next;
		  free_track (trk);
		}
	      else
		prev = trk;
	    }
	}	
      
#ifdef USE_WILDERNESS
      /* Update the wilderness room to disappear if necessary. */
      if (update_wilderness_room (th))
	return;
#endif
      
      /* Update this thing's map. */
      if (nr (1,10) == 2)
      set_up_map_room (th);

      /* Update city status. If the society that built a city here is gone,
	 start to let the city decay. */
      update_built_room (th);

      /* Now check for fire spreading. */
      
      if (nr (1,12) == 7 &&
	  IS_ROOM_SET (th, ROOM_FIERY))
	{
	  VALUE *exit, *build;
	  THING *room2;
	  FLAG *flg;
	  if ((exit = FNV (th, nr (1, REALDIR_MAX))) != NULL &&
	      (room2 = find_thing_num (exit->val[0])) != NULL &&
	      IS_ROOM (room2) &&
	      ((build = FNV (room2, VAL_BUILD)) != NULL ||
	       IS_ROOM_SET (room2, ROOM_FIELD | ROOM_FOREST | ROOM_ROUGH | ROOM_EASYMOVE)))
	    {
	      flg = new_flag();
	      flg->type = FLAG_ROOM1;
	      flg->from = 2000;
	      flg->val = ROOM_FIERY;
	      flg->timer = nr (10,20);
	      aff_update (flg, room2);
	      set_up_map_room (room2);
	    }
	}      
    }
  
  /* Remove flags that wear off. */
  
  for (flag = th->flags; flag; flag = flagn)
    {
      flagn = flag->next;
      if (flag->timer == 0)
	continue;
      if (--flag->timer < 1)
	{
	  SPELL *spl;      
	  if ((spl = find_spell (NULL, flag->from)) != NULL)
	    {
	      if (spl->wear_off && spl->wear_off[0])
		act (spl->wear_off, th, NULL, NULL, NULL,TO_CHAR);
	      else
		stt ("The spell has worn off.\n\r", th);
	    }
	  aff_from (flag, th);                
	}
    }

  /* Remove values that wear off. */

  for (val = th->values; val; val = valn)
    {
      valn = val->next;
      if (val->timer == 0)
	continue;
      if (--val->timer < 1)
	{
	  remove_value (th, val);
	}
    }
  
  if (IS_PC (th))
    {
      for (i = 0; i < ALIGN_MAX; i++)
	{
	  th->pc->align_hate[i]--;
	  if (th->pc->align_hate[i] < 0)
	    th->pc->align_hate[i] = 0;
	}
      update_society_rewards (th, FALSE);
      if (LEVEL(th) < BLD_LEVEL)
	{
	  killed = FALSE;
	  for (j = 0; j < COND_MAX && !killed; j++)
	    {
	      /* Enough remorts make it so you don't need to eat/drink. */
	      if (th->pc->remorts >= 5 &&
		  j == COND_THIRST)
		continue;
	      if (th->pc->remorts >= 8 &&
		  j == COND_HUNGER)
		continue;	      
	      if (nr(1,6) == 2)
		{
		  th->pc->cond[j]--;
		  if (j == COND_HUNGER && (condval = th->pc->cond[j]) < 10)
		    {
		      if (condval == 9)
			stt ("You are beginning to get hungry.\n\r", th);
		      else if (condval == 6)
			stt ("You are getting REALLY hungry.\n\r", th);
		      else if (condval == 3)
			stt ("You are starving. Get some food.\n\r", th);
		      else if (condval < 2)
			stt ("YOU ARE STARVING! GET SOME FOOD NOW!\n\r", th);
		      if (condval < 0)
			killed = damage (th, th, nr (-condval, 5*-condval), "hunger");	
		    }
		  else if (j == COND_THIRST && (condval = th->pc->cond[j]) < 10)
		    {
		      if (condval == 9)
			stt ("You are beginning to get thirsty.\n\r", th);
		      else if (condval == 6)
			stt ("You are getting REALLY thirsty.\n\r", th);
		      else if (condval == 3)
			stt ("You are dried up. Get some water.\n\r", th);
		      else if (condval < 2)
			stt ("YOU ARE DEHYDRATED! GET SOME WATER NOW!\n\r", th);
		      if (condval < 0)
		    killed = damage (th, th, nr (-condval, 5*-condval), "thirst");
		    }
		  else if (j == COND_DRUNK && th->pc->cond[j] == 0)
		    {
		      stt ("You sober up.\n\r", th);
		    }
		}
	    }
	}
      return;
    }
  
  
  /* If this is a light, make it flicker and go out. */

  if ((light = FNV (th, VAL_LIGHT)) != NULL &&
      IS_SET (light->val[2], LIGHT_LIT) &&
      light->val[1] > 0 && light->val[0] > 0)
    {
      light->val[0]--;
      if (light->val[0] == 0)
	{
	  act ("@2n carried by @1n just went out.", th->in, th, NULL, NULL, TO_ALL);
	  th_in = th;
	  while ((th_in = th_in->in) != NULL && !IS_AREA (th_in))
	    th_in->light = MAX (0, th_in->light - 1);
	  RBIT (light->val[2], LIGHT_LIT);
	}
      if (light->val[0] < 10 && nr (1,3) == 2)
	{
	  act ("@2n carried by @1n sputters and sparks.", th->in, th, NULL, NULL, TO_ALL);
	}
    }
  
  /* This should replace the VAL_FEED code. */
  
  if (nr (1,57) == 25)
    find_something_to_eat (th);
  
  
  /* If this thing can feed, make it do so. */

  if (FALSE && nr (1,6) == 2 && 
      (feed = FNV (th, VAL_FEED)) != NULL && 
      (!is_hunting (th) || 
       (th->fgt && th->fgt->hunting_type != HUNT_KILL)))
    { 
      if (++feed->val[5] < 20) /* Not hungry enough. */
	return;
      if (feed->val[5] > 50)
	act ("@1n look@s really hungry.", th, NULL, NULL, NULL, TO_ROOM);
      if (feed->val[5] > 60)
	{
	  th->max_hp = th->max_hp *8/10;
	  if (th->hp > th->max_hp)
	    th->hp = th->max_hp;
	  if (th->max_hp < LEVEL (th))
	    {
	      act ("@1n has died from hunger. :(", th, NULL, NULL, NULL, TO_ROOM);
	      free_thing (th);
	      return;
	    }
	}
      
      /* Now pick something to hunt for, and a certain depth. */
      
      feed_choices = 0;
      for (j = 0; j < 4; j++)
	if (feed->val[j] > 0)
	  feed_choices++;
      if (feed_choices < 1)
	return;
      feed_chose = nr (1, feed_choices);
      feed_choices = 0;
      for (j = 0; j < 4; j++)
	if (feed->val[j] > 0 && ++feed_choices == feed_chose)
	  break;
      
      /* Hunt for it...only a certain depth. Hunger makes
	 you go farther. */
      if ((proto = find_thing_num (feed->val[j])) != NULL &&
	  !IS_SET (proto->thing_flags, TH_IS_AREA | TH_IS_ROOM))
	{ 
	  f_word (proto->name, buf); /* Get a name and hunt it */
	  start_hunting (th, buf, HUNT_KILL);
	  if (!hunt_thing (th, feed->val[4] + feed->val[5]/4))
	    stop_hunting (th, FALSE);
	  
	}
    }
	
  /* If this is a pet, see if it leaves its master. */
  
  if ((pet = FNV (th, VAL_PET)) != NULL && pet->word[0])
    {
      if (pet->val[0] > 0)
	{
	  /* Remove pet value */
	  if (--pet->val[0] < 1)
	    {
	      if (th->fgt)
		{
		  if (th->fgt->following &&
		      is_named (th->fgt->following, pet->word))
		    th->fgt->following = NULL;
		  if (th->fgt->gleader && 
		      is_named (th->fgt->gleader, pet->word))
		    th->fgt->gleader = NULL;
		}
	      remove_value (th, pet);
	    }
	}
    }  
  return;
}

	
  /* This now moves the temporary list of free things to the main
     list of free things. */
        
void
clean_up_tracks (void)
{
  TRACK *trk, *trkn, *prev;
  THING *room, *area, *th;
  
  /* First, clean up the tracks in all rooms. */
  
  for (area = the_world->cont; area; area = area->next_cont)
    {
      for (room = area->cont; room; room = room->next_cont)
	{
	  if ((trkn = room->tracks) != NULL)
	    {
	      prev = NULL;
	      while ((trk = trkn))
		{
		  trkn = trk->next;
		  if (--trk->timer < 1 || !trk->who || !trk->who->in ||
		      (!IS_PC (trk->who) && !trk->who->proto && !IS_ROOM(trk->who)))
		    {
		      if (!prev)
			room->tracks = trk->next;
		      else
			prev->next = trk->next;
		      free_track (trk);
		    }
		  else
		    prev = trk;
		}
	    }	
	}
    }
    
  while (thing_free_track)
    {
      th = thing_free_track->next;
      thing_free_track->next = thing_free;
      thing_free = thing_free_track;
      thing_free_track = th;
    }
  thing_free_count = 0;
  return;
}


/* This is for aggressive monsters and hunting. It only takes place
   where players are! */

void
update_fast (THING *th)
{
  THING *room, *obj, *mob, *warnmob = NULL;
  int roomflags, thflags;
  int charge;
  VALUE *gem, *power;
  int regen_pct = 100; /* Amount that a player regens == 
			  2*soc_power_pct-100 in astral areas...
			  it does nothing until you have 50pct and only 50pct 
			  regen when you have 75 pct of the cities. */
  
  /* Only do this with players.  */
  
  if (!IS_PC (th) || (room = th->in) == NULL || 
      IS_AREA (room) || !room->in || !IS_AREA(room->in))
    return;
  roomflags = flagbits (room->flags, FLAG_ROOM1);
  
  if (th->align < ALIGN_MAX &&
      align_info[th->align] && IS_ROOM_SET (room->in, ROOM_ASTRAL))
    regen_pct = MAX (0, 2*align_info[th->align]->power_pct-100);
  
  /* Update society rewards by rewarding the player. */
  if (nr (1,3) == 2)
    update_society_rewards (th, TRUE);


  /* Charge powershields. */
  for (obj = th->cont; obj; obj = obj->next_cont)
    {
      if (obj->wear_loc != ITEM_WEAR_FLOAT)
	break;
      if ((power = FNV (obj, VAL_POWERSHIELD)) != NULL)
	{
	  if (power->val[1] < power->val[2])
	    {
	      charge = MIN (MIN (power->val[2] - power->val[1], power->val[2] / 15 + 2),  power->val[1]/10 + 2);
	      charge = charge*regen_pct/100;
	      power->val[1] += charge;
	      if (nr (1,10) == 2 || power->val[1] == power->val[2])
		charge_message (obj, power, th);
	    }
	}
    }
  
  thflags = flagbits (th->flags, FLAG_AFF);
  if (IS_SET (thflags, AFF_REGENERATE))
    {
      if (IS_PC (th))
	{
	  if (th->hp < th->max_hp)
	    
	    th->hp += MIN (th->max_hp - th->hp, nr (3, 6)*regen_pct/100);
	  if (th->mv < th->max_mv && regen_pct == 100)
	    th->mv++;
	}
      else
	{
	  if (th->hp < th->max_hp)
	    th->hp += MIN (th->max_hp - th->hp, nr (0, th->max_hp/20)*regen_pct/100);
	  if (th->mv < th->max_mv)
	    th->mv += MIN (th->max_mv - th->mv, nr (0, LEVEL (th)/20)*regen_pct/100);
	}
    }
  
  if (roomflags)
    {
      if (IS_SET (roomflags, ROOM_EXTRAHEAL))
	{
	  if (th->hp < th->max_hp)
	    th->hp += MIN (th->max_hp - th->hp, 2*regen_pct/100);
	  if (th->mv < th->max_mv && regen_pct == 100)
	    th->mv++;
	}
      if (IS_SET (roomflags, ROOM_MANADRAIN))
	{
	  th->pc->mana -= MIN (2, th->pc->mana);
	  for (obj = th->cont; obj; obj = obj->next_cont)
	    {
	      if (obj->wear_loc > ITEM_WEAR_WIELD ||
		  !IS_WORN(obj))
		break;
	      if (obj->wear_loc == ITEM_WEAR_WIELD &&
		  (gem = FNV (obj, VAL_GEM)) != NULL)
		gem->val[1] -= MIN (2, gem->val[1]);
	    }
	}
      else if (IS_SET (roomflags, ROOM_EXTRAMANA))
	{
	  if (th->pc->mana < th->pc->max_mana)
	    th->pc->mana += MIN (th->pc->max_mana - th->pc->mana, 2*regen_pct/100);
	  for (obj = th->cont; obj; obj = obj->next_cont)
	    {
	      if (obj->wear_loc > ITEM_WEAR_WIELD ||
		  !IS_WORN(obj))
		break;
	      if (obj->wear_loc == ITEM_WEAR_WIELD &&
		  (gem = FNV (obj, VAL_GEM)) != NULL)
		{
		  if (gem->val[1] < gem->val[2])
		    {
		      gem->val[1]+= MIN (gem->val[2] - gem->val[1], 2);
		      if (nr (1,10) == 5 || gem->val[1] == gem->val[2])
			charge_message (obj, gem, th);
		    }
		}
	    }
	}
      
      if (nr (1,5) == 2)
	{
	  if (!IS_PC1_SET (th, PC_HOLYWALK))
	    {
	      if (IS_SET (roomflags, ROOM_WATERY | ROOM_UNDERWATER) &&
		  !IS_SET (thflags, AFF_WATER_BREATH))
		{
		  th->mv -= MIN (5, th->mv);
		  if (IS_SET (roomflags, ROOM_UNDERWATER) ||
		      (!IS_AFF (th, AFF_FLYING) &&
		       (!check_spell (th, NULL, 303 /* Swim */) ||
			th->mv < 1)))
		    {
		      THING *downroom = NULL;
		      VALUE *downexit;
		      stt ("\x1b[1;34mYou are DROWNING!\x1b[0;37m\n\r", th);
		      act ("$C@1n is drowning!$7", th, NULL, NULL, NULL, TO_ROOM);
		      if (damage (room, th, nr (5, 10), "drown"))
			return;
		      
		      if ((downexit = FNV (th->in, VAL_EXIT_D)) != NULL &&
			  (downroom = find_thing_num (downexit->val[0])) != NULL &&
			  IS_ROOM (downroom) && nr (1,3) == 2)
			{
			  act ("@1n slip@s farther down...", th, NULL, NULL, NULL, TO_ALL);
			  move_thing (th->in, th, th->in, downroom);
			}
		    }
		}
	      
	      /* When you fall in an air room, it happens all
		 at once, and it sucks. */
	      
	      if (IS_SET (roomflags, ROOM_AIRY) &&
		  !IS_AFF (th, AFF_FLYING))
		{
		  THING *downroom = NULL, *curr_room;
		  VALUE *downexit;
		  int count = 0;
		  curr_room = th->in;
		  
		  while (curr_room && count < 100)
		    {
		      if ((downexit = FNV (curr_room, VAL_EXIT_D)) != NULL &&
			  (downroom = find_thing_num (downexit->val[0])) != NULL &&
			  IS_ROOM (downroom))
			{
			  act ("@1n is falling!", th, NULL, NULL, NULL, TO_ALL);
			  move_thing (th->in, th, curr_room, downroom);
			}
		      curr_room = downroom;
		      count++;
		    }
		  
		  if (count > 0)
		    {
		      if (damage (th->in, th, nr(count/2, count)*nr(count/2, count), "squish"))
			return;
		    }
		}
	      if (IS_SET (roomflags, ROOM_ASTRAL) &&
		  !IS_AFF (th, AFF_PROTECT))
		{
		  act ("$9The $FAstral Winds $9batter @1n to pieces!$7", th, NULL, NULL, NULL, TO_ALL);
		  
		  if (damage (room, th, nr (10,30), "dissolve"))
		    return;
		}
	      
	      if (IS_SET (roomflags, ROOM_FIERY) &&
		  !IS_PROT (th, AFF_FIRE))
		{
		  
		  act ("$9@1n get@s burned to a crisp!$7", th, NULL, NULL, NULL, TO_ALL);
		  if (damage (room, th, nr (3, 10), "flame"))
		    return;
		}
	      if (IS_SET (roomflags, ROOM_EARTHY) &&
		  !IS_SET (thflags, AFF_FOGGY))
		{
		  act ("$3The earth crushes @1n to a pulp!$7", th, NULL, NULL, NULL, TO_ALL);
		  if (damage (room, th, nr (5,15), "crush"))
		    return;
		}	   
	    }
	}
    }
  
  if (th->in && th->in == room)
    {
      bool did_random_social = FALSE;
      bool mob_attacked = FALSE;
      bool can_attack = FALSE;
      warnmob = NULL;
      if (nr (1,2) == 2)
	can_attack = TRUE;
      for (mob = th->in->cont; mob; mob = mob->next_cont)
	{
	  if (can_attack && CAN_FIGHT (mob) && nr (1,3) == 2 &&
	      (!mob_attacked || nr (1,5) == 3))
	    {
	      mob_attacked = TRUE;
	      attack_stuff (mob);
	    }
	  else if (nr (1,235) == 133 && CAN_TALK (mob) &&
		   !did_random_social && mob->position > POSITION_SLEEPING)
	    {
	      do_random_social (mob);
	      did_random_social = TRUE;
	    }
	  if (nr (1,20) == 17 && mob->in == th->in &&
	      CAN_TALK (mob) && !DIFF_ALIGN (mob->align, th->align))
	    warnmob = mob;
	}
      if (warnmob && nr (1,40) == 23 &&
	  warnmob->align < ALIGN_MAX &&
	  th->pc->align_hate[warnmob->align] >= ALIGN_HATE_WARN &&
	  th->pc->align_hate[warnmob->align] < ALIGN_MAX)
	{
	  char buf[STD_LEN];
	  sprintf (buf, "%s, you had better be careful. The %ss will not tolerate your murderous actions forever!\n\r", NAME(th), (align_info[warnmob->align] ? align_info[warnmob->align]->name : "Allie" ));
	  do_say (warnmob, buf);
	}
    }  
  return;
}

/* This makes a thing go out and try to hunt something. */

void
find_something_to_eat (THING *th)
{
  THING *vict = NULL, *victn;
  THING *room, *nroom;
  int i, loops = 0;
  bool found_intelligent_here;
  int bonus_depth = 0;
  

  /* They tend to only pick on stupid things, but if forced into it,
     they can go after intelligent things if really hungry. */
  
  if (!th || !th->in || !IS_ROOM (th->in) || 
      CAN_TALK (th) || is_hunting (th) ||
      !IS_ACT1_SET (th, ACT_CARNIVORE) ||
      LEVEL (th) < 15)
    return;

  /* Once in a while, a carnivore moves out of its local hunting grounds
     and goes after something farther away. */
  if (nr (1,30) == 3)
    bonus_depth = 10;
  
  undo_marked(th->in);
  clear_bfs_list();
  add_bfs (NULL, th->in, REALDIR_MAX);
  
  while (bfs_curr)
    {
      loops++;
      if ((room = bfs_curr->room) != NULL && IS_ROOM (room))
	{
	  found_intelligent_here = FALSE;
	  for (vict = room->cont; vict != NULL; vict = victn)
	    {
	      victn = vict->next_cont;
	      if (!CAN_FIGHT(vict) ||
		  vict->vnum == th->vnum ||
		  LEVEL(vict) > LEVEL(th)/2)
		continue;
	      

	      /* If victim can't talk and it's in close enough range,
		 attack it. */
	      if (!CAN_TALK (vict))
		{
		  if (LEVEL (vict) >= LEVEL(th)/6 &&
		      nr (1,32) == 17)
		    break;
		}
	      else /* If victim can talk, usually don't attack, 
		      instead avoid this thing. */
		{
		  if (nr (1,53) == 24)
		    {
		      if (LEVEL (vict) < LEVEL(th)/5)
			break;
		    }
		  else
		    {/* We found something intelligent here, don't
			go this way. */
		      found_intelligent_here = TRUE;
		      vict = NULL;
		      break;
		    }
		}
	      
	      /* If nothing that can talk (intelligent things) were
		 found here, keep searching. Otherwise, let this
		 node of the search tree die. */
	      
	      if (!found_intelligent_here)
		{	      
		  for (i = 0; i < REALDIR_MAX; i++)
		    {
		      if ((nroom = FTR (room, i, th->move_flags)) != NULL &&
			  bfs_curr->depth  < CARNIVORE_HUNT_DEPTH+bonus_depth)
			add_bfs (bfs_curr, nroom, i);	     
		    }	  
		}
	    }
	  if (vict)
	    {
	      place_backtracks (vict, 5);
	      break;
	    }
	}
      bfs_curr = bfs_curr->next;
    }
  clear_bfs_list();
  
  if (vict)
    {
      start_hunting (th, KEY(vict), HUNT_KILL);
      if (th->fgt)
	th->fgt->hunt_victim = vict;
      
    }
  return;
}


