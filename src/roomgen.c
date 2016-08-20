#include<stdio.h>
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#include "serv.h"
#include "areagen.h"


/* This lets you generate a room name out of whole cloth. */

void
roomgen (THING *th, THING *room, char *arg)
{
  THING *area;
  int sector_type = 0, patch_type;
  bool underground = FALSE;

  if (!th || !room || !IS_ROOM (room))
    {
      stt ("You can only roomgen rooms.\n\r", th);
      return;
    }
  if (!arg || !*arg)
    {
      stt ("roomgen <sector_type>\n\r", th);
      return;
    }
  
  if ((patch_type = atoi (arg)) == 0)
    patch_type = find_bit_from_name (FLAG_ROOM1, arg);
  if (!IS_SET (patch_type, ROOM_SECTOR_FLAGS))
    {
      stt ("You can only setup rooms with room sector flags as a base!\n\r", th);
      return;
    }
  
  if ((area = room->in) != NULL && IS_AREA (area))
    sector_type = flagbits (area->flags, FLAG_ROOM1) & ROOM_SECTOR_FLAGS;
  
  if (IS_SET (sector_type, ROOM_UNDERGROUND))
    underground = TRUE;
  
  remove_flagval (room, FLAG_ROOM1, ROOM_SECTOR_FLAGS);
  if (underground)
    add_flagval (room, FLAG_ROOM1, ROOM_UNDERGROUND);
  add_flagval (room, FLAG_ROOM1, patch_type);
  free_str (room->short_desc);
  room->short_desc = new_str (generate_patch_name (sector_type, patch_type));
  SBIT (area->thing_flags, TH_CHANGED);
  stt ("Ok, room changed.\n\r", th);
  return;
}

void
room_add_exit (THING *room, int dir, int to, int exit_flags)
{
  THING *nroom;
  VALUE *exit;
  
  if (!room || !to ||
      (nroom = find_thing_num (to)) == NULL ||
      !IS_ROOM (nroom))
    return;

  if ((exit = FNV (room, dir + 1)) == NULL)
    {
      exit = new_value();
      exit->type = dir + 1;
      add_value (room, exit);
      if (room->in && IS_AREA (room->in))
	room->in->thing_flags |= TH_CHANGED;
    }  
  exit->val[0] = to;
  exit->val[1] = exit_flags;
  exit->val[2] = exit_flags;
  return;
}
