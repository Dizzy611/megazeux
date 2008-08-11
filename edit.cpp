/* $Id$
 * MegaZeux
 *
 * Copyright (C) 1996 Greg Janson
 * Copyright (C) 2004 Gilead Kutnick - exophase@adelphia.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "helpsys.h"
#include "scrdisp.h"
#include "sfx.h"
#include "sfx_edit.h"
#include "counter.h"
#include "game.h"
#include "fill.h"
#include "block.h"
#include "pal_ed.h"
#include "char_ed.h"
#include "edit_di.h"
#include "intake.h"
#include "param.h"
#include "error.h"
#include "idarray.h"
#include "edit.h"
#include "data.h"
#include "const.h"
#include "graphics.h"
#include "window.h"
#include "idput.h"
#include "hexchar.h"
#include "counter.h"
#include "mzm.h"
#include "world.h"
#include "event.h"
#include "audio.h"

/* Edit menu- (w/box ends on sides) Current menu name is highlighted. The
  bottom section zooms to show a list of options for the current menu,
  although all keys are available at all times. PGUP/PGDN changes menu.
  The menus are shown in listed order. [draw] can be [text]. (in which case
  there is the words "Type to place text" after the ->) [draw] can also be
  [place] or [block]. The options DRAW, TEXT, DEBUG, BLOCK, MOD, and DEF.
  COLORS are highlighted white (instead of green) when active. Debug mode
  pops up a small window in the lower left corner, which rushes to the right
  side and back when the cursor reaches 3/4 of the way towards it,
  horizontally. The menu itself is 15 lines wide and 7 lines high. The
  board display, w/o the debug menu, is 19 lines high and 80 lines wide.
  Note: SAVE is highlighted white if the world has changed since last load
  or save.

  The commands with ! instead of : have not been implemented yet.

  The menu line turns to "Overlay editing-" and the lower portion shows
  overlay commands on overlay mode.
컴컴컴컴컴컴커
X/Y:  nnn/nnn�
Board:    nnn�
(module)     �
컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
 WORLD  BOARD  THING  CURSOR  SHOW  MISC  Drawing:_!_ (_#_ PushableRobot p00)
컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
 L:Load     S:Save  G:Global Info    Alt+R:Restart  Alt+T:Test  *:Protection
 Alt+S:Status Info  Alt+C:Char Edit  Alt+E:Palette  Alt+F:Sound Effects

 Alt+Z:Clear   X:Exits       Alt+P:Size/Pos  I:Info  A:Add  D:Delete  V:View
 Alt+I:Import  Alt+X:Export  B:Select Board  Alt+O:Edit Overlay

 F3:Terrain  F4:Item      F5:Creature  F6:Puzzle  F7:Transport  F8:Element
 F9:Misc     F10:Objects  P:Parameter  C:Color

 :Move  Space:Place  Enter:Modify+Grab  Alt+M:Modify  Ins:Grab  Del:Delete
 F:Fill   Tab:Draw     F2:Text            Alt+B:Block   Alt+:Move 10

 Shift+F1:Show InvisWalls  Shift+F2:Show Robots  Shift+F3:Show Fakes
 Shift+F4:Show Spaces

 F1!Help    R:Redraw Screen  Alt+A:Select Char Set  Alt+D:Default Colors
 ESC:Exit Alt+L:Test SAM   Alt+Y:Debug Mode       Alt+N:Music
컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴횾gup/Pgdn:Menu�

컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
 OVERLAY EDITING- (Alt+O to end)          Current:_!_ (_#_ Character p00)
컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
 :Move  Space:Place  Ins:Grab  Enter:Character  Del:Delete        F:Fill
 C:Color  Alt+B:Block  Tab:Draw  Alt+:Move 10   Alt+S:Show level  F2:Text
컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

Undocumented keys:

End:      LR corner
Home:     UL corner
ESC:      Cancel mode/overlay mode if active (otherwise exit)
Backspace:  Delete (move left one in text)
*/

#define synchronize_board_values()          \
  src_board = mzx_world->current_board;     \
  board_width = src_board->board_width;     \
  board_height = src_board->board_height;   \
  level_id = src_board->level_id;           \
  level_param = src_board->level_param;     \
  level_color = src_board->level_color;     \
  overlay = src_board->overlay;             \
  overlay_color = src_board->overlay_color; \
  clear_screen_no_update(177, 1)            \

#define fix_scroll()                        \
  if(cursor_board_x >= board_width)         \
  {                                         \
    cursor_board_x = (board_width - 1);     \
    scroll_x = (board_width - 80);          \
                                            \
    if(scroll_x < 0)                        \
      scroll_x = 0;                         \
  }                                         \
                                            \
  if(cursor_board_y >= board_height)        \
  {                                         \
    cursor_board_y = (board_height - 1);    \
      scroll_y =                            \
      (board_height - edit_screen_height);  \
                                            \
    if(scroll_y < 0)                        \
      scroll_y = 0;                         \
  }                                         \

#define fix_board(new_board)                \
  mzx_world->current_board_id = new_board;  \
  mzx_world->current_board =                \
   mzx_world->board_list[new_board];        \

#define fix_mod()                           \
  load_mod(src_board->mod_playing);         \
  strcpy(mzx_world->real_mod_playing,       \
   src_board->mod_playing);                 \


#define NUM_MENUS 6
char menu_names[NUM_MENUS][9] =
{
  " WORLD ", " BOARD ", " THING ", " CURSOR ", " SHOW ", " MISC "
};

char menu_positions[] = "11111112222222333333344444444555555666666";

char draw_names[7][10] =
{
  " Current:", " Drawing:", "    Text:", "   Block:", "   Block:", " Import:"
};

char *menu_lines[NUM_MENUS][2]=
{
  {
    " L:Load\tS:Save  G:Global Info    Alt+R:Restart  Alt+T:Test",
    " Alt+S:Status Info  Alt+C:Char Edit  Alt+E:Palette  Alt+F:Sound Effects"
  },
  {
    " Alt+Z:Clear   X:Exits\t  Alt+P:Size/Pos  I:Info  A:Add  D:Delete  V:View",
    " Alt+I:Import  Alt+X:Export  B:Select Board  Alt+O:Edit Overlay"
  },
  {
    " F3:Terrain  F4:Item\tF5:Creature  F6:Puzzle  F7:Transport  F8:Element",
    " F9:Misc\tF10:Objects  P:Parameter  C:Color"
  },
  {
    " :Move  Space:Place  Enter:Modify+Grab  Alt+M:Modify  Ins:Grab  Del:Delete",
    " F:Fill   Tab:Draw\tF2:Text\t\t  Alt+B:Block   Alt+:Move 10"
  },
  {
    " Shift+F1:Show InvisWalls  Shift+F2:Show Robots  Shift+F3:Show Fakes",
    " Shift+F4:Show Spaces"
  },
  {
    " F1:Help    Home\\End:Corner  Alt+A:Select Char Set  Alt+D:Default Colors",
    " ESC:Exit   Alt+L:Test SAM   Alt+Y:Debug Mode\t  Alt+N:Music  Alt+8:Mod *"
  }
};

char *overlay_menu_lines[4] =
{
  " OVERLAY EDITING- (Alt+O to end)",
  " :Move  Space:Place  Ins:Grab  Enter:Character  Del:Delete\t   F:Fill",
  " C:Color  Alt+B:Block  Tab:Draw  Alt+:Move 10   Alt+S:Show level  F2:Text",
  "Character"
};

// Arrays for 'thing' menus
char tmenu_num_choices[8] = { 17, 14, 18, 8, 6, 11, 12, 10 };
char *tmenu_titles[8] =
{
  "Terrains", "Items", "Creatures", "Puzzle Pieces",
  "Transport", "Elements", "Miscellaneous", "Objects"
};

// Each 'item' is 20 char long, including '\0'.
char *thing_menus[8] =
{
  // Terrain (F3)
  "Space           ~1 \0"
  "Normal          ~E�\0"
  "Solid           ~D�\0"
  "Tree            ~A\0"
  "Line            ~B�\0"
  "Custom Block    ~F?\0"
  "Breakaway       ~C�\0"
  "Custom Break    ~F?\0"
  "Fake            ~9�\0"
  "Carpet          ~4�\0"
  "Floor           ~6�\0"
  "Tiles           ~0�\0"
  "Custom Floor    ~F?\0"
  "Web             ~7�\0"
  "Thick Web       ~7�\0"
  "Forest          ~2�\0"
  "Invis. Wall     ~1 ",

  // Item (F4)
  "Gem             ~A\0"
  "Magic Gem       ~E\0"
  "Health          ~C\0"
  "Ring            ~E\x9\0"
  "Potion          ~B�\0"
  "Energizer       ~D\0"
  "Ammo            ~3�\0"
  "Bomb            ~8\0"
  "Key             ~F\0"
  "Lock            ~F\xA\0"
  "Coin            ~E\0"
  "Life            ~B�\0"
  "Pouch           ~7�\0"
  "Chest           ~6�",

  // Creature (F5)
  "Snake           ~2�\0"
  "Eye             ~F�\0"
  "Thief           ~C\0"
  "Slime Blob      ~A*\0"
  "Runner          ~4\0"
  "Ghost           ~7�\0"
  "Dragon          ~4\0"
  "Fish            ~E�\0"
  "Shark           ~7\0"
  "Spider          ~7�\0"
  "Goblin          ~D\0"
  "Spitting Tiger  ~B�\0"
  "Bear            ~6�\0"
  "Bear Cub        ~6�\0"
  "Lazer Gun       ~4�\0"
  "Bullet Gun      ~F\0"
  "Spinning Gun    ~F\0"
  "Missile Gun     ~8",

  // Puzzle (F6)
  "Boulder         ~7�\0"
  "Crate           ~6�\0"
  "Custom Push     ~F?\0"
  "Box             ~E�\0"
  "Custom Box      ~F?\0"
  "Pusher          ~D\0"
  "Slider NS       ~D\0"
  "Slider EW       ~D",

  // Tranport (F7)
  "Stairs          ~A�\0"
  "Cave            ~6�\0"
  "Transport       ~E<\0"
  "Whirlpool       ~B�\0"
  "CWRotate        ~9/\0"
  "CCWRotate       ~9\\",

  // Element (F8)
  "Still Water     ~9�\0"
  "N Water         ~9\x18\0"
  "S Water         ~9\x19\0"
  "E Water         ~9\x1A\0"
  "W Water         ~9\x1B\0"
  "Ice             ~B�\0"
  "Lava            ~C�\0"
  "Fire            ~E�\0"
  "Goop            ~8�\0"
  "Lit Bomb        ~8�\0"
  "Explosion       ~E�",

  // Misc (F9)
  "Door            ~6�\0"
  "Gate            ~8\0"
  "Ricochet Panel  ~9/\0"
  "Ricochet        ~A*\0"
  "Mine            ~C�\0"
  "Spike           ~8\0"
  "Custom Hurt     ~F?\0"
  "Text            ~F?\0"
  "N Moving Wall   ~F?\0"
  "S Moving Wall   ~F?\0"
  "E Moving Wall   ~F?\0"
  "W Moving Wall   ~F?",

  // Objects (F10)
  "Robot           ~F?\0"
  "Pushable Robot  ~F?\0"
  "Player          ~B\0"
  "Scroll          ~F�\0"
  "Sign            ~6�\0"
  "Sensor          ~F?\0"
  "Bullet          ~F�\0"
  "Missile         ~8\0"
  "Seeker          ~A/\0"
  "Shooting Fire   ~E"
};

char tmenu_thing_ids[8][18] =
{
  //Terrain (F3)
  { 0, 1, 2, 3, 4, 5, 6, 7, 13, 14, 15, 16, 17, 18, 19, 65, 71},
  // Item (F4)
  { 28, 29, 30, 31, 32, 33, 35, 36, 39, 40, 50, 66, 55, 27 },
  // Creature (F5)
  { 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 94, 95, 60,
    92, 93, 97 },
  // Puzzle (F6)
  { 8, 9, 10, 11, 12, 56, 57, 58 },
  // Tranport (F7)
  { 43, 44, 49, 67, 45, 46 },
  // Element (F8)
  { 20, 21, 22, 23, 24, 25, 26, 63, 34, 37, 38 },
  // Misc (F9)
  { 41, 47, 72, 73, 74, 75, 76, 77, 51, 52, 53, 54 },
  // Objects (F10)
  { 124, 123, 127, 126, 125, 122, 61, 62, 79, 78}
};

// Default colors
char def_colors[128] =
{
  7 , 0 , 0 , 10, 0 , 0 , 0 , 0 , 7 , 6 , 0 , 0 , 0 , 0 , 0 , 0 ,   // 0x00-0x0F
  0 , 0 , 7 , 7 , 25, 25, 25, 25, 25, 59, 76, 6 , 0 , 0 , 12, 14,   // 0x10-0x1F
  11, 15, 24, 3 , 8 , 8 , 239,0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 8 ,   // 0x20-0x2F
  8 , 0 , 14, 0 , 0 , 0 , 0 , 7 , 0 , 0 , 0 , 0 , 4 , 15, 8 , 12,   // 0x30-0x3F
  0 , 2 , 11, 31, 31, 31, 31, 0 , 9 , 10, 12, 8 , 0 , 0 , 14, 10,   // 0x40-0x4F
  2 , 15, 12, 10, 4 , 7 , 4 , 14, 7 , 7 , 2 , 11, 15, 15, 6 , 6 ,   // 0x50-0x5F
  0 , 8 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,   // 0x60-0x6F
  0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 6 , 15, 27    // 0x70-0x7F
};

char *mod_ext[] =
{
  ".xm", ".s3m", ".mod", ".med", ".mtm", ".stm", ".it",
  ".669", ".ult", ".wav", ".dsm", ".far", ".ams", ".mdl", ".okt" ".dmf",
  ".ptm", ".dbm", ".amf", ".mt2", ".psm", ".j2b", ".umx", NULL
};

char drawmode_help[5][32] =
{
  "Type to place text",
  "Press ENTER on other corner",
  "Press ENTER to place block",
  "Press ENTER to place MZM"
};

// It's important that after changing the param the thing is placed (using
// place_current_at_xy) so that scrolls/sensors/robots get copied.

int change_param(World *mzx_world, int id, int param, Robot *copy_robot,
 Scroll *copy_scroll, Sensor *copy_sensor)
{
  if(id == 122)
  {
    return edit_sensor(mzx_world, copy_sensor);
  }
  else

  if((id == 123) || (id == 124))
  {
    return edit_robot(mzx_world, copy_robot);
  }
  else

  if((id == 125) || (id == 126))
  {
    return edit_scroll(mzx_world, copy_scroll);
  }
  else
  {
    return edit_param(mzx_world, id, param);
  }
}

int place_current_at_xy(World *mzx_world, int id, int color, int param,
 int x, int y, Robot *copy_robot, Scroll *copy_scroll, Sensor *copy_sensor,
 int overlay_edit)
{
  Board *src_board = mzx_world->current_board;
  int offset = x + (y * src_board->board_width);
  char *level_id = src_board->level_id;
  int old_id = level_id[offset];

  if(!overlay_edit)
  {
    if(id == 127)
    {
      id_remove_top(mzx_world, mzx_world->player_x, mzx_world->player_y);
      mzx_world->player_x = x;
      mzx_world->player_y = y;
    }
    else

    if((id == 123) || (id == 124))
    {
      if((old_id == 123) || (old_id == 124))
      {
        int old_param = src_board->level_param[offset];
        replace_robot(src_board, copy_robot, old_param);
        return old_param;
      }

      param = duplicate_robot(src_board, copy_robot, x, y);
      if(param != -1)
      {
        (src_board->robot_list[param])->xpos = x;
        (src_board->robot_list[param])->ypos = y;
        (src_board->robot_list[param])->used = 1;
      }
    }
    else

    if((id == 125) || (id == 126))
    {
      if((old_id == 125) || (old_id == 126))
      {
        int old_param = src_board->level_param[offset];
        replace_scroll(src_board, copy_scroll, old_param);
        return old_param;
      }

      param = duplicate_scroll(src_board, copy_scroll);
      if(param != -1)
        (src_board->scroll_list[param])->used = 1;
    }
    else

    if(id == 122)
    {
      if((old_id == 123) || (old_id == 124))
      {
        int old_param = src_board->level_param[offset];
        replace_sensor(src_board, copy_sensor, old_param);
        return old_param;
      }

      param = duplicate_sensor(src_board, copy_sensor);
      if(param != -1)
        (src_board->sensor_list[param])->used = 1;
    }

    if(param != -1)
    {
      place_at_xy(mzx_world, id, color, param, x, y);
    }
  }
  else
  {
    int offset = x + (y * src_board->board_width);
    src_board->overlay[offset] = param;
    src_board->overlay_color[offset] = color;
  }

  return param;
}

void grab_at_xy(World *mzx_world, int *new_id, int *new_color,
 int *new_param, Robot *copy_robot, Scroll *copy_scroll,
 Sensor *copy_sensor, int x, int y, int overlay_edit)
{
  Board *src_board = mzx_world->current_board;
  int board_width = src_board->board_width;
  int offset = x + (y * board_width);
  int old_id = *new_id;
  int old_param = *new_param;

  if(!overlay_edit)
  {
    int grab_id = src_board->level_id[offset];
    int grab_param = src_board->level_param[offset];
    *new_color = get_id_color(src_board, offset);

    // First, clear the existing copies, unless the new id AND param
    // matches, because in that case it's the same thing

    if(((old_id == 123) || (old_id == 124)) &&
     ((old_id != grab_id) || (old_param != grab_param)))
    {
      clear_robot_contents(copy_robot);
    }

    if(((old_id == 125) || (old_id == 126)) &&
     ((old_id != grab_id) || (old_param != grab_param)))
    {
      clear_scroll_contents(copy_scroll);
    }

    if((grab_id == 123) || (grab_id == 124))
    {
      Robot *src_robot = src_board->robot_list[grab_param];
      duplicate_robot_direct(src_robot, copy_robot, 0, 0);
    }
    else

    if((grab_id == 125) || (grab_id == 126))
    {
      Scroll *src_scroll = src_board->scroll_list[grab_param];
      duplicate_scroll_direct(src_scroll, copy_scroll);
    }
    else

    if(grab_id == 122)
    {
      Sensor *src_sensor = src_board->sensor_list[grab_param];
      duplicate_sensor_direct(src_sensor, copy_sensor);
    }

    *new_id = grab_id;
    *new_param = grab_param;
  }
  else
  {
    *new_param = src_board->overlay[offset];
    *new_color = src_board->overlay_color[offset];
  }
}

void thing_menu(World *mzx_world, int menu_number, int *new_id,
 int *new_color, int *new_param, Robot *copy_robot, Scroll *copy_scroll,
 Sensor *copy_sensor, int x, int y)
{
  int id, color, param;
  int chosen;
  int old_id = *new_id;

  cursor_off();
  chosen =
   list_menu(thing_menus[menu_number], 20, tmenu_titles[menu_number], 0,
   tmenu_num_choices[menu_number], 27);

  if(chosen >= 0)
  {
    id = tmenu_thing_ids[menu_number][chosen];
    if(def_colors[id])
    {
      color = def_colors[id];
    }
    else
    {
      color = *new_color;
    }

    // Perhaps put a new blank scroll, robot, or sensor in one of the copies
    if(id == 122)
    {
      create_blank_sensor_direct(copy_sensor);
    }
    else

    if((id == 123) || (id == 124))
    {
      if((old_id == 123) || (old_id == 124))
        clear_robot_contents(copy_robot);

      create_blank_robot_direct(copy_robot, 0, 0);
    }

    if((id == 125) || (id == 126))
    {
      if((old_id == 125) || (old_id == 126))
      {
        clear_scroll_contents(copy_scroll);
      }

      create_blank_scroll_direct(copy_scroll);
    }

    param = *new_param;
    param =
     change_param(mzx_world, id, -1, copy_robot, copy_scroll, copy_sensor);

    param = place_current_at_xy(mzx_world, id, color, param,
     x, y, copy_robot, copy_scroll, copy_sensor, 0);

    *new_id = id;
    *new_param = param;
    *new_color = color;
  }
  cursor_solid();
}

void flash_thing(World *mzx_world, int start, int end, int flash_one,
 int flash_two, int scroll_x, int scroll_y, int edit_screen_height)
{
  Board *src_board = mzx_world->current_board;
  int backup[32];
  int i, i2;

  cursor_off();

  for(i = 0, i2 = start; i2 < end + 1; i++, i2++)
  {
    backup[i] = id_chars[i2];
  }

  src_board->overlay_mode |= 0x80;

  do
  {
    for(i = start; i < end + 1; i++)
    {
      id_chars[i] = flash_one;
    }
    draw_edit_window(src_board, scroll_x, scroll_y, edit_screen_height);
    update_screen();

    for(i = start; i < end + 1; i++)
    {
      id_chars[i] = flash_two;
    }
    draw_edit_window(src_board, scroll_x, scroll_y, edit_screen_height);
    update_screen();

    update_event_status_delay();
  } while(!get_key(keycode_SDL));

  update_event_status_delay();

  src_board->overlay_mode &= 0x7F;

  for(i = 0, i2 = start; i2 < end + 1; i++, i2++)
  {
    id_chars[i2] = backup[i];
  }

  cursor_solid();
}

void edit_world(World *mzx_world)
{
  Board *src_board;
  Robot copy_robot;
  Scroll copy_scroll;
  Sensor copy_sensor;

  int i;
  int cursor_board_x = 0, cursor_board_y = 0;
  int cursor_x = 0, cursor_y = 0;
  int scroll_x = 0, scroll_y = 0;
  int current_id = 0;
  int current_color = 7;
  int current_param = 0;
  int board_width, board_height;
  int key;
  int draw_mode = 0;
  int overlay_edit = 0;
  int current_menu = 0;
  int show_level = 1;
  int display_next_pos;
  int block_x, block_y;
  int block_dest_x, block_dest_y;
  int block_command;
  int text_place;
  int text_start_x;
  int modified = 0;
  int debug_x = 60;
  int saved_overlay_mode;
  int edit_screen_height = 19;
  int copy_repeat_width = -1;
  int copy_repeat_height = -1;
  char *level_id;
  char *level_param;
  char *level_color;
  char *overlay;
  char *overlay_color;
  char current_world[128];
  char mzm_name_buffer[128];

  current_world[0] = 0;

  copy_robot.used = 0;
  copy_sensor.used = 0;
  copy_scroll.used = 0;

  mzx_world->version = VERSION;

  if(mzx_world->active)
  {
    clear_world(mzx_world);
    clear_global_data(mzx_world);
  }
  mzx_world->active = 1;

  create_blank_world(mzx_world);
  set_update_done(mzx_world);
  cursor_solid();
  m_show();

  end_mod();

  find_player(mzx_world);

  synchronize_board_values();
  update_screen();

  insta_fadein();

  do
  {
    cursor_x = cursor_board_x - scroll_x;
    cursor_y = cursor_board_y - scroll_y;

    if(current_param == -1)
    {
      current_id = 0;
      current_param = 0;
      current_color = 7;
    }

    move_cursor(cursor_x, cursor_y);

    saved_overlay_mode = src_board->overlay_mode;

    if(!overlay_edit)
    {
      src_board->overlay_mode = 0;
      draw_edit_window(src_board, scroll_x, scroll_y, edit_screen_height);
    }
    else
    {
      src_board->overlay_mode = 1;
      if(!show_level)
      {
        src_board->overlay_mode |= 0x40;
        draw_edit_window(src_board, scroll_x, scroll_y, edit_screen_height);
        src_board->overlay_mode ^= 0x40;
      }
      else
      {
        draw_edit_window(src_board, scroll_x, scroll_y, edit_screen_height);
      }
    }
    src_board->overlay_mode = saved_overlay_mode;

    if(edit_screen_height == 19)
    {
      draw_window_box(0, 19, 79, 24, EC_MAIN_BOX, EC_MAIN_BOX_DARK,
       EC_MAIN_BOX_CORNER, 0, 1);
      draw_window_box(0, 21, 79, 24, EC_MAIN_BOX, EC_MAIN_BOX_DARK,
       EC_MAIN_BOX_CORNER, 0, 1);

      if(!overlay_edit)
      {
        int i, write_color, x;
        x = 1; // X position

        for(i = 0; i < NUM_MENUS; i++)
        {
          if(i == current_menu)
            write_color = EC_CURR_MENU_NAME;
          else
            write_color = EC_MENU_NAME; // Pick the color

          // Write it
          write_string(menu_names[i], x, 20, write_color, 0);
          // Add to x
          x += strlen(menu_names[i]);
        }

        write_string(menu_lines[current_menu][0], 1, 22, EC_OPTION, 1);
        write_string(menu_lines[current_menu][1], 1, 23, EC_OPTION, 1);
      }
      else
      {
        write_string(overlay_menu_lines[0], 1, 20, EC_MENU_NAME, 1);
        write_string(overlay_menu_lines[1], 1, 22, EC_OPTION, 1);
        write_string(overlay_menu_lines[2], 1, 23, EC_OPTION, 1);
      }

      write_string(draw_names[draw_mode], 42, 20, EC_MODE_STR, 0);
      display_next_pos = strlen(draw_names[draw_mode]) + 42;

      if(draw_mode > 1)
      {
        write_string(drawmode_help[draw_mode - 2], display_next_pos, 20,
         EC_MODE_HELP, 0);
      }
      else
      {
        int display_char, display_color = current_color;

        if(!overlay_edit)
        {
          if(current_id == 122)
          {
            display_char = copy_sensor.sensor_char;
          }
          else

          if((current_id == 123) || (current_id == 124))
          {
            display_char = copy_robot.robot_char;
          }
          else
          {
            int temp_char = level_id[0];
            int temp_param = level_param[0];
            level_id[0] = current_id;
            level_param[0] = current_param;

            display_char = get_id_char(src_board, 0);

            level_id[0] = temp_char;
            level_param[0] = temp_param;
          }
        }
        else
        {
          display_char = current_param;
        }

        draw_char(' ', 7, display_next_pos, 20);
        draw_char(display_char, display_color, display_next_pos + 1, 20);
        draw_char(' ', 7, display_next_pos + 2, 20);

        display_next_pos += 4;
        draw_char('(', EC_CURR_THING, display_next_pos, 20);
        draw_color_box(display_color, 0, display_next_pos + 1, 20);
        display_next_pos += 5;

        if(overlay_edit)
        {
          write_string("Character", display_next_pos, 20,
           EC_CURR_THING, 0);
          display_next_pos += 9;
          write_hex_byte(current_param, EC_CURR_PARAM, display_next_pos + 1, 20);
          display_next_pos += 3;
        }
        else
        {
          write_string(thing_names[current_id], display_next_pos, 20,
           EC_CURR_THING, 0);
          display_next_pos += strlen(thing_names[current_id]);
          draw_char('p', EC_CURR_PARAM, display_next_pos + 1, 20);
          write_hex_byte(current_param, EC_CURR_PARAM, display_next_pos + 2, 20);
          display_next_pos += 4;
        }

        draw_char(')', EC_CURR_THING, display_next_pos, 20);
      }

      draw_char(196, EC_MAIN_BOX_CORNER, 78, 21);
      draw_char(217, EC_MAIN_BOX_DARK, 79, 21);
    }

    if(debug_mode)
    {
      draw_debug_box(mzx_world, debug_x, edit_screen_height - 6,
       cursor_board_x, cursor_board_y);
    }

    // Highlight block for draw mode 3
    if(draw_mode == 3)
    {
      int block_screen_x = block_x - scroll_x;
      int block_screen_y = block_y - scroll_y;
      int start_x, start_y;
      int block_width, block_height;

      if(block_screen_x < 0)
        block_screen_x = 0;

      if(block_screen_y < 0)
        block_screen_y = 0;

      if(block_screen_x >= 80)
        block_screen_x = 79;

      if(block_screen_y >= edit_screen_height)
        block_screen_y = edit_screen_height - 1;

      if(block_screen_x < cursor_x)
      {
        start_x = block_screen_x;
        block_width = cursor_x - block_screen_x + 1;
      }
      else
      {
        start_x = cursor_x;
        block_width = block_screen_x - cursor_x + 1;
      }

      if(block_screen_y < cursor_y)
      {
        start_y = block_screen_y;
        block_height = cursor_y - block_screen_y + 1;
      }
      else
      {
        start_y = cursor_y;
        block_height = block_screen_y - cursor_y + 1;
      }

      for(i = 0; i < block_height; i++)
      {
        color_line(block_width, start_x, start_y + i, 0x9F);
      }
    }

    text_place = 0;

    update_screen();

    update_event_status_delay();
    key = get_key(keycode_SDL);

    if(get_mouse_press())
    {
      int mouse_x, mouse_y;
      get_mouse_position(&mouse_x, &mouse_y);

      if(mouse_y == 20)
      {
        if((mouse_x >= 1) && (mouse_x <= 41))
        {
          current_menu = menu_positions[mouse_x - 1] - '1';
        }
        else

        if((mouse_x >= 56) && (mouse_x <= 58))
        {
          int new_color;
          cursor_off();
          new_color = color_selection(current_color, 0);
          if(new_color >= 0)
            current_color = new_color;

        }
      }
      else

      if((mouse_y < edit_screen_height) && (edit_screen_height == 19))
      {
        cursor_board_x = mouse_x + scroll_x;
        cursor_board_y = mouse_y + scroll_y;

        if(get_mouse_status() == SDL_BUTTON(3))
        {
          grab_at_xy(mzx_world, &current_id, &current_color,
           &current_param, &copy_robot, &copy_scroll, &copy_sensor,
           cursor_board_x, cursor_board_y, overlay_edit);
        }
        else
        {
          current_param = place_current_at_xy(mzx_world, current_id,
           current_color, current_param, cursor_board_x, cursor_board_y,
           &copy_robot, &copy_scroll, &copy_sensor, overlay_edit);
        }
      }
    }

    switch(key)
    {
      case SDLK_UP:
      {
        int i, move_amount = 1;

        if(get_alt_status(keycode_SDL))
          move_amount = 10;

        if(get_ctrl_status(keycode_SDL) && (copy_repeat_height >= 0))
          move_amount = copy_repeat_height;

        for(i = 0; i < move_amount; i++)
        {
          if(!cursor_board_y)
            break;

          cursor_board_y--;

          if(draw_mode == 1)
          {
            current_param = place_current_at_xy(mzx_world, current_id,
             current_color, current_param, cursor_board_x, cursor_board_y,
             &copy_robot, &copy_scroll, &copy_sensor, overlay_edit);
            modified = 1;
          }

          if(((cursor_board_y - scroll_y) < 3) && scroll_y)
            scroll_y--;
        }
        break;
      }

      case SDLK_DOWN:
      {
        int i, move_amount = 1;

        if(get_alt_status(keycode_SDL))
          move_amount = 10;

        if(get_ctrl_status(keycode_SDL) && (copy_repeat_height >= 0))
          move_amount = copy_repeat_height;

        for(i = 0; i < move_amount; i++)
        {
          if(cursor_board_y == (board_height - 1))
            break;

          cursor_board_y++;

          if(draw_mode == 1)
          {
            current_param = place_current_at_xy(mzx_world, current_id,
             current_color, current_param, cursor_board_x, cursor_board_y,
             &copy_robot, &copy_scroll, &copy_sensor, overlay_edit);
            modified = 1;
          }

          // Scroll board position if there's room to
          if(((cursor_board_y - scroll_y) > (edit_screen_height - 5)) &&
           (scroll_y < (board_height - edit_screen_height)))
            scroll_y++;
        }
        break;
      }

      case SDLK_LEFT:
      {
        int i, move_amount = 1;

        if(get_alt_status(keycode_SDL))
          move_amount = 10;

        if(get_ctrl_status(keycode_SDL) && (copy_repeat_width >= 0))
          move_amount = copy_repeat_width;

        for(i = 0; i < move_amount; i++)
        {
          if(!cursor_board_x)
            break;

          cursor_board_x--;

          if(draw_mode == 1)
          {
            current_param = place_current_at_xy(mzx_world, current_id,
             current_color, current_param, cursor_board_x, cursor_board_y,
             &copy_robot, &copy_scroll, &copy_sensor, overlay_edit);
            modified = 1;
          }

          if(((cursor_board_x - scroll_x) < 5) && scroll_x)
            scroll_x--;
        }

        if((cursor_board_x - scroll_x) < (debug_x + 25))
          debug_x = 60;

        break;
      }

      case SDLK_RIGHT:
      {
        int i, move_amount = 1;

        if(get_alt_status(keycode_SDL))
          move_amount = 10;

        if(get_ctrl_status(keycode_SDL) && (copy_repeat_width >= 0))
          move_amount = copy_repeat_width;

        for(i = 0; i < move_amount; i++)
        {
          if(cursor_board_x == (board_width - 1))
            break;

          cursor_board_x++;

          if(draw_mode == 1)
          {
            current_param = place_current_at_xy(mzx_world, current_id,
             current_color, current_param, cursor_board_x, cursor_board_y,
             &copy_robot, &copy_scroll, &copy_sensor, overlay_edit);
            modified = 1;
          }

          if(((cursor_board_x - scroll_x) > 74) &&
           (scroll_x < (board_width - 80)))
            scroll_x++;
        }

        if((cursor_board_x - scroll_x) > (debug_x - 5))
          debug_x = 0;

        break;
      }

      case SDLK_SPACE:
      {
        if(draw_mode == 2)
        {
          place_current_at_xy(mzx_world, 77, current_color,
           ' ', cursor_board_x, cursor_board_y, &copy_robot,
           &copy_scroll, &copy_sensor, overlay_edit);

          if(cursor_board_x < (board_width - 1))
            cursor_board_x++;

          if(((cursor_board_x - scroll_x) > 74) &&
           (scroll_x < (board_width - 80)))
            scroll_x++;
        }
        else
        {
          int offset = cursor_board_x + (cursor_board_y * board_width);
          int place_id = current_id;
          int place_param = current_param;
          int place_color = current_color;

          if((!overlay_edit) && (place_id == level_id[offset]) &&
           (place_color == level_color[offset]))
          {
            place_id = 0;
            place_color = 7;
          }

          current_param = place_current_at_xy(mzx_world, place_id,
           place_color, place_param, cursor_board_x, cursor_board_y,
           &copy_robot, &copy_scroll, &copy_sensor, overlay_edit);
        }
        modified = 1;
        break;
      }

      case SDLK_BACKSPACE:
      {
        if(draw_mode == 2)
        {
          if(cursor_board_x)
            cursor_board_x--;

          if(((cursor_board_x - scroll_x) < 5) && scroll_x)
            scroll_x--;

          place_current_at_xy(mzx_world, 77, current_color,
           ' ', cursor_board_x, cursor_board_y, &copy_robot,
           &copy_scroll, &copy_sensor, overlay_edit);
          modified = 1;
        }
        break;
      }

      case SDLK_INSERT:
      {
        grab_at_xy(mzx_world, &current_id, &current_color, &current_param,
         &copy_robot, &copy_scroll, &copy_sensor, cursor_board_x,
         cursor_board_y, overlay_edit);
        break;
      }

      case SDLK_TAB:
      {
        if(draw_mode)
        {
          draw_mode = 0;
        }
        else
        {
          draw_mode = 1;
          current_param = place_current_at_xy(mzx_world, current_id,
           current_color, current_param, cursor_board_x, cursor_board_y,
           &copy_robot, &copy_scroll, &copy_sensor, overlay_edit);
          modified = 1;
        }

        break;
      }

      case SDLK_DELETE:
      {
        if(draw_mode == 2)
        {
          place_current_at_xy(mzx_world, 77, current_color, ' ',
           cursor_board_x, cursor_board_y, &copy_robot,
           &copy_scroll, &copy_sensor, overlay_edit);
        }
        else
        {
          place_current_at_xy(mzx_world, 0, 7, 0, cursor_board_x,
           cursor_board_y, &copy_robot, &copy_scroll, &copy_sensor,
           overlay_edit);
        }
        modified = 1;
        break;
      }

      // Show invisible walls
      case SDLK_F1:
      {
        if(get_shift_status(keycode_SDL))
        {
          if(!overlay_edit)
          {
            flash_thing(mzx_world, 71, 71, 178, 176, scroll_x, scroll_y,
             edit_screen_height);
          }
        }
        else
        {
          m_show();
          help_system(mzx_world);
          break;
        }

        break;
      }

      // Show robots
      case SDLK_F2:
      {
        if(get_shift_status(keycode_SDL))
        {
          if(!overlay_edit)
          {
            flash_thing(mzx_world, 123, 124, '!', 0, scroll_x, scroll_y,
             edit_screen_height);
          }
        }
        else
        {
          draw_mode = 2;
          text_start_x = cursor_board_x;
        }
        break;
      }

      // Terrain
      case SDLK_F3:
      {
        if(!overlay_edit)
        {
          if(get_shift_status(keycode_SDL))
          {
            // Show fakes
            flash_thing(mzx_world, 13, 19, '#', 177, scroll_x, scroll_y,
             edit_screen_height);
          }
          else
          {
            thing_menu(mzx_world, 0, &current_id, &current_color,
             &current_param, &copy_robot, &copy_scroll, &copy_sensor,
             cursor_board_x, cursor_board_y);
            modified = 1;
          }
        }
        break;
      }

      // Item
      case SDLK_F4:
      {
        if(!overlay_edit)
        {
          if(get_shift_status(keycode_SDL))
          {
            // Show spaces
            flash_thing(mzx_world, 0, 0, 'O', '*', scroll_x, scroll_y,
             edit_screen_height);
          }
          else
          {
            thing_menu(mzx_world, 1, &current_id, &current_color,
             &current_param, &copy_robot, &copy_scroll, &copy_sensor,
             cursor_board_x, cursor_board_y);
            modified = 1;
          }
        }
        break;
      }

      // Creature
      case SDLK_F5:
      {
        if(!overlay_edit)
        {
          thing_menu(mzx_world, 2, &current_id, &current_color, &current_param,
           &copy_robot, &copy_scroll, &copy_sensor, cursor_board_x, cursor_board_y);
          modified = 1;
        }
        break;
      }

      // Puzzle
      case SDLK_F6:
      {
        if(!overlay_edit)
        {
          thing_menu(mzx_world, 3, &current_id, &current_color, &current_param,
           &copy_robot, &copy_scroll, &copy_sensor, cursor_board_x, cursor_board_y);
          modified = 1;
        }
        break;
      }

      // Transport
      case SDLK_F7:
      {
        if(!overlay_edit)
        {
          thing_menu(mzx_world, 4, &current_id, &current_color, &current_param,
           &copy_robot, &copy_scroll, &copy_sensor, cursor_board_x, cursor_board_y);
          modified = 1;
        }
        break;
      }

      // Element
      case SDLK_F8:
      {
        if(!overlay_edit)
        {
          thing_menu(mzx_world, 5, &current_id, &current_color, &current_param,
           &copy_robot, &copy_scroll, &copy_sensor, cursor_board_x, cursor_board_y);
          modified = 1;
        }
        break;
      }

      // Misc
      case SDLK_F9:
      {
        if(!overlay_edit)
        {
          thing_menu(mzx_world, 6, &current_id, &current_color, &current_param,
           &copy_robot, &copy_scroll, &copy_sensor, cursor_board_x, cursor_board_y);
          modified = 1;
        }
        break;
      }

      // Object
      case SDLK_F10:
      {
        if(!overlay_edit)
        {
          thing_menu(mzx_world, 7, &current_id, &current_color, &current_param,
           &copy_robot, &copy_scroll, &copy_sensor, cursor_board_x, cursor_board_y);
          modified = 1;
        }
        break;
      }

      case SDLK_c:
      {
        if(draw_mode != 2)
        {
          cursor_off();
          if(get_alt_status(keycode_SDL))
          {
            if(!get_screen_mode())
              char_editor(mzx_world);
            else
              smzx_char_editor(mzx_world);

            modified = 1;
          }
          else
          {
            int new_color = color_selection(current_color, 0);
            if(new_color >= 0)
              current_color = new_color;
          }
          cursor_solid();
        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_o:
      {
        if(draw_mode != 2)
        {
          if(get_alt_status(keycode_SDL))
          {
            if(!overlay_edit)
            {
              if(!src_board->overlay_mode)
              {
                error("Overlay mode is not on (see Board Info)", 0, 24,
                 0x1103);
              }
              else
              {
                overlay_edit = 1;
                current_param = 32;
                current_color = 7;
              }
            }
            else
            {
              overlay_edit = 0;
              current_id = 0;
              current_param = 0;
              current_color = 7;
            }
          }
        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_s:
      {
        if(draw_mode != 2)
        {
          if(get_alt_status(keycode_SDL))
          {
            if(overlay_edit)
            {
              show_level ^= 1;
            }
            else
            {
              status_counter_info(mzx_world);
              modified = 1;
              cursor_solid();
            }
          }
          else
          {
            if(!save_world_dialog(mzx_world, current_world))
            {
              struct stat file_info;

              // Name in current world name
              if(current_world[0] == 0)
                break;

              add_ext(current_world, ".mzx");
              // Check for an overwrite
              if(!stat(current_world, &file_info))
              {
                if(confirm(mzx_world, "File exists - Overwrite?"))
                  break;
              }

              // Save entire game
              save_world(mzx_world, current_world, 0, 1);

              modified = 0;
            }
            cursor_solid();
          }
        }
        else
        {
          text_place = 1;
        }

        break;
      }

      case SDLK_HOME:
      {
        cursor_board_x = 0;
        cursor_board_y = 0;
        scroll_x = 0;
        scroll_y = 0;
        break;
      }

      case SDLK_END:
      {
        cursor_board_x = board_width - 1;
        cursor_board_y = board_height - 1;
        scroll_x = board_width - 80;
        scroll_y = board_height - edit_screen_height;

        if(scroll_x < 0)
          scroll_x = 0;

        if(scroll_y < 0)
          scroll_y = 0;

        break;
      }

      case SDLK_b:
      {
        if(draw_mode != 2)
        {
          if(get_alt_status(keycode_SDL))
          {
            block_x = cursor_board_x;
            block_y = cursor_board_y;
            draw_mode = 3;
          }
          else
          {
            int new_board =
             choose_board(mzx_world, mzx_world->current_board_id,
             "Select current board:", 0);
            cursor_solid();

            if(new_board >= 0)
            {
              fix_board(new_board);
              synchronize_board_values();
              if(strcmp(src_board->mod_playing, "*") &&
               strcasecmp(src_board->mod_playing,
               mzx_world->real_mod_playing))
              {
                fix_mod();
              }

              fix_scroll();
              modified = 1;
            }
          }
        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_l:
      {
        if(get_alt_status(keycode_SDL))
        {
          char test_wav[128];
          char *wav_ext[] = { ".WAV", ".SAM", NULL };

          if(!choose_file(wav_ext, test_wav, "Choose a wav file", 0))
          {
            play_sample(428, test_wav);
          }
          cursor_solid();
        }
        else

        if(draw_mode != 2)
        {
          if(!modified || !confirm(mzx_world,
           "Load: World has not been saved, are you sure?"))
          {
            if(!choose_file(world_ext, current_world, "Load World", 1))
            {
              int fade;
              // Load world curr_file
              end_mod();
              reload_world(mzx_world, current_world, &fade);
              insta_fadein();
              synchronize_board_values();
              fix_mod();
              fix_scroll();
              modified = 0;
            }
            cursor_solid();
          }
        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_i:
      {
        if(get_alt_status(keycode_SDL))
        {
          int import_number = import_type(mzx_world);
          if(import_number >= 0)
          {
            char import_name[128];

            switch(import_number)
            {
              case 0:
              {
                char *mzb_ext[] = { ".MZB", NULL };
                if(!choose_file(mzb_ext, import_name, "Choose board to import", 0))
                {
                  replace_current_board(mzx_world, import_name);
                  synchronize_board_values();
                  if(strcmp(src_board->mod_playing, "*") &&
                   strcasecmp(src_board->mod_playing, mzx_world->real_mod_playing))
                  {
                    fix_mod();
                  }
                  fix_scroll();
                  modified = 1;
                }
                break;
              }

              case 1:
              {
                // Character set
                char *chr_ext[] = { ".CHR", NULL };
                if(!choose_file(chr_ext, import_name,
                 "Choose character set to import", 0))
                {
                  ec_load_set(import_name);
                }
                modified = 1;
                break;
              }

              case 2:
              {
                // World file
                if(!choose_file(world_ext, import_name,
                 "Choose world to import", 0))
                {
                  append_world(mzx_world, import_name);
                }
                modified = 1;
                break;
              }

              case 3:
              {
                // Palette
                // Character set
                char *pal_ext[] = { ".PAL", NULL };
                if(!choose_file(pal_ext, import_name,
                 "Choose palette to import", 0))
                {
                  load_palette(import_name);
                  update_palette();
                  modified = 1;
                }
                break;
              }

              case 4:
              {
                // Sound effects
                char *sfx_ext[] = { ".SFX", NULL };
                if(!choose_file(sfx_ext, import_name,
                 "Choose palette to import", 0))
                {
                  FILE *sfx_file;

                  sfx_file = fopen(import_name, "rb");
                  fread(mzx_world->custom_sfx, 69, 50, sfx_file);
                  mzx_world->custom_sfx_on = 1;
                  fclose(sfx_file);
                  modified = 1;
                }
                break;
              }

              case 5:
              {
                // MZM file
                char *mzm_ext[] = { ".MZM", NULL };
                if(!choose_file(mzm_ext, mzm_name_buffer,
                 "Choose image file to import", 0))
                {
                  draw_mode = 5;
                  block_command = 9;
                }

                break;
              }
            }
            cursor_solid();
          }
        }
        else

        if(draw_mode != 2)
        {
          board_info(mzx_world);
          // If this is the first board, patch the title into the world name
          if(mzx_world->current_board_id == 0)
            strcpy(mzx_world->name, src_board->board_name);

          modified = 1;
          cursor_solid();
        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_g:
      {
        if(draw_mode != 2)
        {
          global_info(mzx_world);
          modified = 1;
          cursor_solid();
        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_p:
      {
        if(draw_mode != 2)
        {
          if(get_alt_status(keycode_SDL))
          {
            size_pos(mzx_world);
            set_update_done(mzx_world);
            synchronize_board_values();
            fix_scroll();

            // Uh oh, we might need a new player
            if((mzx_world->player_x >= board_width) ||
             ((mzx_world->player_y) >= board_height))
              replace_player(mzx_world);

            modified = 1;
            cursor_solid();
          }
          else

          if(!overlay_edit)
          {
            int offset = cursor_board_x + (cursor_board_y * board_width);
            int d_id = level_id[offset];
            if(d_id < 122)
            {
              int d_param = level_param[offset];
              d_param = change_param(mzx_world, d_id, d_param, NULL, NULL, NULL);
              place_current_at_xy(mzx_world, current_id, current_color,
               d_param, cursor_board_x, cursor_board_y, NULL, NULL, NULL, 0);
              modified = 1;
            }
            cursor_solid();
          }
        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_x:
      {
        if(get_alt_status(keycode_SDL))
        {
          int export_number = export_type(mzx_world);
          if(export_number >= 0)
          {
            char export_name[128];
            export_name[0] = 0;

            switch(export_number)
            {
              case 0:
              {
                // Board file
                if(!save_file_dialog(mzx_world, "Export", "Save as: ", export_name))
                {
                  add_ext(export_name, ".mzb");
                  save_board_file(src_board, export_name);
                }
                break;
              }

              case 1:
              {
                // Character set
                int char_offset = 0;
                int char_size = 256;

                if(!save_char_dialog(mzx_world, export_name, &char_offset,
                 &char_size))
                {
                  add_ext(export_name, ".chr");
                  ec_save_set_var(export_name, char_offset, char_size);
                }

                break;
              }

              case 2:
              {
                // Text file

                if(!save_file_dialog(mzx_world, "Export", "Save as: ", export_name))
                {
                  FILE *txt_file;
                  int x, y, offset = scroll_x + (scroll_y * board_width);
                  int width = 80, height = 25;
                  int skip;

                  if(board_width < (scroll_x + 80))
                    width = board_width - scroll_x;

                  if(board_height < (scroll_y + 25))
                    height = board_height - scroll_y;

                  skip = board_width - width;

                  add_ext(export_name, ".txt");
                  txt_file = fopen(export_name, "wb");

                  for(y = 0; y < height; y++)
                  {
                    for(x = 0; x < width; x++, offset++)
                    {
                      fputc(get_id_char(src_board, offset), txt_file);
                    }
                    fputc('\n', txt_file);
                    offset += skip;
                  }
                  fclose(txt_file);
                }
                break;
              }

              case 3:
              {
                // Palette
                if(!save_file_dialog(mzx_world, "Export", "Save as: ", export_name))
                {
                  add_ext(export_name, ".pal");
                  save_palette(export_name);
                }

                break;
              }

              case 4:
              {
                // Sound effects
                if(!save_file_dialog(mzx_world, "Export", "Save as: ", export_name))
                {
                  FILE *sfx_file;

                  add_ext(export_name, ".sfx");
                  sfx_file = fopen(export_name, "wb");
                  if(mzx_world->custom_sfx_on)
                    fwrite(mzx_world->custom_sfx, 69, 50, sfx_file);
                  else
                    fwrite(sfx_strs, 69, 50, sfx_file);

                  fclose(sfx_file);
                }
                break;
              }
            }
          }
          cursor_solid();
        }
        else

        if(draw_mode != 2)
        {
          board_exits(mzx_world);
          modified = 1;
          cursor_solid();
        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_n:
      {
        if(draw_mode != 2)
        {
          if(get_alt_status(keycode_SDL))
          {
            if(!src_board->mod_playing[0])
            {
              char new_mod[128];

              if(!choose_file(mod_ext, new_mod, "Choose a module file", 0))
              {
                load_mod(new_mod);
                strcpy(src_board->mod_playing, new_mod);
                strcpy(mzx_world->real_mod_playing, new_mod);
              }
              cursor_solid();
            }
            else
            {
              end_mod();
              src_board->mod_playing[0] = 0;
              mzx_world->real_mod_playing[0] = 0;
            }
            modified = 1;
          }
        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_RETURN:
      {
        if(draw_mode == 3)
        {
          int start_x = block_x;
          int start_y = block_y;
          int block_width, block_height;

          if(start_x > cursor_board_x)
          {
            start_x = cursor_board_x;
            block_width = block_x - cursor_board_x + 1;
          }
          else
          {
            block_width = cursor_board_x - block_x + 1;
          }

          if(start_y > cursor_board_y)
          {
            start_y = cursor_board_y;
            block_height = block_y - cursor_board_y + 1;
          }
          else
          {
            block_height = cursor_board_y - block_y + 1;
          }

          // Select block command
          block_command = block_cmd(mzx_world);
          block_dest_x = cursor_board_x;
          block_dest_y = cursor_board_y;
          cursor_solid();

          // Some block commands are done automatically
          switch(block_command)
          {
            case -1:
            {
              draw_mode = 0;
              break;
            }

            case 0:
            case 1:
            case 2:
            {
              draw_mode = 4;
              break;
            }

            case 3:
            {
              // Clear block
              if(overlay_edit)
              {
                clear_layer_block(start_x, start_y, block_width,
                 block_height, overlay, overlay_color, board_width);
              }
              else
              {
                clear_board_block(src_board, start_x, start_y,
                 block_width, block_height);
              }

              draw_mode = 0;
              modified = 1;
              break;
            }

            case 4:
            {
              // Flip block
              char *temp_buffer = (char *)malloc(board_width);
              int start_offset = start_x + (start_y * board_width);
              int end_offset = start_x + ((start_y + block_height - 1) * board_width);

              if(overlay_edit)
              {
                for(i = 0; i < (block_height / 2); i++, start_offset += board_width,
                 end_offset -= board_width)
                {
                  memcpy(temp_buffer, overlay + start_offset, block_width);
                  memcpy(overlay + start_offset, overlay + end_offset, block_width);
                  memcpy(overlay + end_offset, temp_buffer, block_width);
                  memcpy(temp_buffer, overlay_color + start_offset, block_width);
                  memcpy(overlay_color + start_offset, overlay_color + end_offset,
                   block_width);
                  memcpy(overlay_color + end_offset, temp_buffer, block_width);
                }
              }
              else
              {
                char *level_under_id = src_board->level_under_id;
                char *level_under_color = src_board->level_under_color;
                char *level_under_param = src_board->level_under_param;

                for(i = 0; i < (block_height / 2); i++, start_offset += board_width,
                 end_offset -= board_width)
                {
                  memcpy(temp_buffer, level_id + start_offset, block_width);
                  memcpy(level_id + start_offset, level_id + end_offset, block_width);
                  memcpy(level_id + end_offset, temp_buffer, block_width);
                  memcpy(temp_buffer, level_color + start_offset, block_width);
                  memcpy(level_color + start_offset, level_color + end_offset,
                   block_width);
                  memcpy(level_color + end_offset, temp_buffer, block_width);
                  memcpy(temp_buffer, level_param + start_offset, block_width);
                  memcpy(level_param + start_offset, level_param + end_offset,
                   block_width);
                  memcpy(level_param + end_offset, temp_buffer, block_width);
                  memcpy(temp_buffer, level_under_id + start_offset, block_width);
                  memcpy(level_under_id + start_offset, level_under_id + end_offset,
                   block_width);
                  memcpy(level_under_id + end_offset, temp_buffer, block_width);
                  memcpy(temp_buffer, level_under_color + start_offset, block_width);
                  memcpy(level_under_color + start_offset,
                   level_under_color + end_offset, block_width);
                  memcpy(level_under_color + end_offset, temp_buffer, block_width);
                  memcpy(temp_buffer, level_under_param + start_offset, block_width);
                  memcpy(level_under_param + start_offset,
                   level_under_param + end_offset, block_width);
                  memcpy(level_under_param + end_offset, temp_buffer, block_width);
                }
              }

              free(temp_buffer);
              draw_mode = 0;
              modified = 1;
              break;
            }

            case 5:
            {
              // Mirror block
              int temp, i2;
              int start_offset = start_x + (start_y * board_width);
              int previous_offset;
              int end_offset;

              if(overlay_edit)
              {
                for(i = 0; i < block_height; i++)
                {
                  previous_offset = start_offset;
                  end_offset = start_offset + block_width - 1;
                  for(i2 = 0; i2 < (block_width / 2);
                   i2++, start_offset++, end_offset--)
                  {
                    temp = overlay[start_offset];
                    overlay[start_offset] = overlay[end_offset];
                    overlay[end_offset] = temp;
                    temp = overlay_color[start_offset];
                    overlay_color[start_offset] = overlay_color[end_offset];
                    overlay_color[end_offset] = temp;
                  }
                  start_offset = previous_offset + board_width;
                }
              }
              else
              {
                char *level_under_id = src_board->level_under_id;
                char *level_under_color = src_board->level_under_color;
                char *level_under_param = src_board->level_under_param;

                for(i = 0; i < block_height; i++)
                {
                  previous_offset = start_offset;
                  end_offset = start_offset + block_width - 1;
                  for(i2 = 0; i2 < (block_width / 2);
                   i2++, start_offset++, end_offset--)
                  {
                    temp = level_id[start_offset];
                    level_id[start_offset] = level_id[end_offset];
                    level_id[end_offset] = temp;
                    temp = level_color[start_offset];
                    level_color[start_offset] = level_color[end_offset];
                    level_color[end_offset] = temp;
                    temp = level_param[start_offset];
                    level_param[start_offset] = level_param[end_offset];
                    level_param[end_offset] = temp;
                    temp = level_under_id[start_offset];
                    level_under_id[start_offset] = level_under_id[end_offset];
                    level_under_id[end_offset] = temp;
                    temp = level_under_color[start_offset];
                    level_under_color[start_offset] = level_under_color[end_offset];
                    level_under_color[end_offset] = temp;
                    temp = level_under_param[start_offset];
                    level_under_param[start_offset] = level_under_param[end_offset];
                    level_under_param[end_offset] = temp;
                  }
                  start_offset = previous_offset + board_width;
                }
              }

              draw_mode = 0;
              modified = 1;
              break;
            }

            case 6:
            {
              // Paint
              int i2;
              int skip = board_width - block_width;
              char *dest_offset;

              if(overlay_edit)
              {
                dest_offset =
                 overlay_color + start_x + (start_y * board_width);
              }
              else
              {
                dest_offset =
                 level_color + start_x + (start_y * board_width);
              }

              for(i = 0; i < block_height; i++, dest_offset += skip)
              {
                for(i2 = 0; i2 < block_width; i2++, dest_offset++)
                {
                  *dest_offset = current_color;
                }
              }
              draw_mode = 0;
              modified = 1;
              break;
            }

            case 7:
            {
              if(!overlay_edit)
              {
                if(!src_board->overlay_mode)
                {
                  error("Overlay mode is not on (see Board Info)", 0, 24,
                   0x1103);
                  draw_mode = 0;
                }
                else
                {
                  draw_mode = 4;
                  overlay_edit = 1;
                }
              }
              else
              {
                overlay_edit = 0;
                draw_mode = 4;
              }
              break;
            }

            case 8:
            {
              // Save as MZM
              char *mzm_ext[] = { ".MZM", NULL };

              mzm_name_buffer[0] = 0;

              if(!save_file_dialog(mzx_world, "Export", "Save as: ", mzm_name_buffer))
              {
                if(overlay_edit)
                {
                  save_mzm(mzx_world, mzm_name_buffer, start_x, start_y,
                   block_width, block_height, 1, 1);
                }
                else
                {
                  save_mzm(mzx_world, mzm_name_buffer, start_x, start_y,
                   block_width, block_height, 0, 0);
                }
              }
              modified = 1;
              cursor_solid();
              draw_mode = 0;
              break;
            }
          }
        }
        else

        if(draw_mode == 2)
        {
          // Go to next line
          int dif_x = cursor_board_x - text_start_x;
          if(dif_x > 0)
          {
            for(i = 0; i < dif_x; i++)
            {
              if(!cursor_board_x)
                break;

              cursor_board_x--;

              if(((cursor_board_x - scroll_x) < 5) && scroll_x)
                scroll_x--;
            }
          }
          else

          if(dif_x < 0)
          {
            for(i = 0; i < -dif_x; i++)
            {
              if(cursor_board_x >= board_width)
                break;

              cursor_board_x++;

              if(((cursor_board_x - scroll_x) > 74) &&
               (scroll_x < (board_width - 80)))
                scroll_x++;
            }
          }

          if(cursor_board_y < board_height)
          {
            cursor_board_y++;

            // Scroll board position if there's room to
            if(((cursor_board_y - scroll_y) > (edit_screen_height - 5)) &&
             (scroll_y < (board_height - edit_screen_height)))
              scroll_y++;
          }
        }
        else

        if(overlay_edit)
        {
          if(draw_mode > 3)
          {
            int dest_x = cursor_board_x;
            int dest_y = cursor_board_y;
            int start_x = block_x;
            int start_y = block_y;
            int block_width, block_height;

            if(start_x > block_dest_x)
            {
              start_x = block_dest_x;
              block_width = block_x - block_dest_x + 1;
            }
            else
            {
              block_width = block_dest_x - block_x + 1;
            }

            if(start_y > block_dest_y)
            {
              start_y = block_dest_y;
              block_height = block_y - block_dest_y + 1;
            }
            else
            {
              block_height = block_dest_y - block_y + 1;
            }

            draw_mode = 0;

            switch(block_command)
            {
              case 0:
              case 1:
              {
                // Copy block
                char *char_buffer = (char *)malloc(block_width * block_height);
                char *color_buffer = (char *)malloc(block_width * block_height);
                copy_layer_to_buffer(start_x, start_y, block_width,
                 block_height, overlay, overlay_color, char_buffer,
                 color_buffer, board_width);
                copy_buffer_to_layer(dest_x, dest_y, block_width,
                 block_height, char_buffer, color_buffer, overlay,
                 overlay_color, board_width);
                free(char_buffer);
                free(color_buffer);

                // 1 is repeat copy
                if(block_command == 1)
                {
                  draw_mode = 4;
                  copy_repeat_width = block_width;
                  copy_repeat_height = block_height;
                }
                modified = 1;
                break;
              }

              case 2:
              {
                // Move block
                char *char_buffer = (char *)malloc(block_width * block_height);
                char *color_buffer = (char *)malloc(block_width * block_height);
                copy_layer_to_buffer(start_x, start_y, block_width,
                 block_height, overlay, overlay_color, char_buffer,
                 color_buffer, board_width);
                clear_layer_block(start_x, start_y, block_width,
                 block_height, overlay, overlay_color, board_width);
                copy_buffer_to_layer(dest_x, dest_y, block_width,
                 block_height, char_buffer, color_buffer, overlay,
                 overlay_color, board_width);
                free(char_buffer);
                free(color_buffer);
                modified = 1;
                break;
              }

              case 7:
              {
                // Copy from overlay
                int overlay_offset = dest_x + (dest_y * board_width);
                copy_board_to_layer(src_board, start_x, start_y, block_width,
                 block_height, overlay + overlay_offset,
                 overlay_color + overlay_offset, board_width);
                modified = 1;
                break;
              }

              case 9:
              {
                // Load MZM
                load_mzm(mzx_world, mzm_name_buffer, dest_x, dest_y, 1);
                modified = 1;
                break;
              }
            }
          }
          else
          {
            int new_param = char_selection(current_param);
            if(new_param >= 0)
            {
              current_param = new_param;
              modified = 1;
            }
            cursor_solid();
          }
        }
        else
        {
          if(draw_mode > 2)
          {
            int dest_x = cursor_board_x;
            int dest_y = cursor_board_y;
            int start_x = block_x;
            int start_y = block_y;
            int block_width, block_height;

            if(start_x > block_dest_x)
            {
              start_x = block_dest_x;
              block_width = block_x - block_dest_x + 1;
            }
            else
            {
              block_width = block_dest_x - block_x + 1;
            }

            if(start_y > block_dest_y)
            {
              start_y = block_dest_y;
              block_height = block_y - block_dest_y + 1;
            }
            else
            {
              block_height = block_dest_y - block_y + 1;
            }

            draw_mode = 0;

            switch(block_command)
            {
              case 0:
              case 1:
              {
                // Copy block
                int block_size = block_width * block_height;
                char *id_buffer = (char *)malloc(block_size);
                char *param_buffer = (char *)malloc(block_size);
                char *color_buffer = (char *)malloc(block_size);
                char *under_id_buffer = (char *)malloc(block_size);
                char *under_param_buffer = (char *)malloc(block_size);
                char *under_color_buffer = (char *)malloc(block_size);
                copy_board_to_board_buffer(src_board, start_x, start_y,
                 block_width, block_height, id_buffer, param_buffer,
                 color_buffer, under_id_buffer, under_param_buffer,
                 under_color_buffer);
                copy_board_buffer_to_board(src_board, dest_x, dest_y,
                 block_width, block_height, id_buffer, param_buffer,
                 color_buffer, under_id_buffer, under_param_buffer,
                 under_color_buffer);
                free(id_buffer);
                free(param_buffer);
                free(color_buffer);
                free(under_id_buffer);
                free(under_param_buffer);
                free(under_color_buffer);

                // 1 is repeat copy
                if(block_command == 1)
                {
                  draw_mode = 4;
                  copy_repeat_width = block_width;
                  copy_repeat_height = block_height;
                }

                modified = 1;
                break;
              }

              case 2:
              {
                // Move block
                int block_size = block_width * block_height;
                char *id_buffer = (char *)malloc(block_size);
                char *param_buffer = (char *)malloc(block_size);
                char *color_buffer = (char *)malloc(block_size);
                char *under_id_buffer = (char *)malloc(block_size);
                char *under_param_buffer = (char *)malloc(block_size);
                char *under_color_buffer = (char *)malloc(block_size);
                copy_board_to_board_buffer(src_board, start_x, start_y,
                 block_width, block_height, id_buffer, param_buffer,
                 color_buffer, under_id_buffer, under_param_buffer,
                 under_color_buffer);
                clear_board_block(src_board, start_x, start_y,
                 block_width, block_height);
                copy_board_buffer_to_board(src_board, dest_x, dest_y,
                 block_width, block_height, id_buffer, param_buffer,
                 color_buffer, under_id_buffer, under_param_buffer,
                 under_color_buffer);
                free(id_buffer);
                free(param_buffer);
                free(color_buffer);
                free(under_id_buffer);
                free(under_param_buffer);
                free(under_color_buffer);

                modified = 1;
                break;
              }

              case 7:
              {
                // Copy from board
                int layer_convert = rtoo_obj_type(mzx_world);

                switch(layer_convert)
                {
                  case 0:
                  {
                    // Custom Block
                    layer_convert = 5;
                    break;
                  }

                  case 1:
                  {
                    // Custom Floor
                    layer_convert = 17;
                    break;
                  }

                  case 2:
                  {
                    // Text
                    layer_convert = 77;
                    break;
                  }
                }

                if(layer_convert != -1)
                {
                  int overlay_offset = start_x + (start_y * board_width);
                  copy_layer_to_board(src_board, dest_x, dest_y, block_width,
                   block_height, overlay + overlay_offset,
                   overlay_color + overlay_offset, board_width, layer_convert);

                  modified = 1;
                }
                break;
              }

              case 9:
              {
                // Load MZM
                load_mzm(mzx_world, mzm_name_buffer, dest_x, dest_y, 0);
                modified = 1;
                break;
              }
            }
          }
          else
          {
            grab_at_xy(mzx_world, &current_id, &current_color, &current_param,
             &copy_robot, &copy_scroll, &copy_sensor, cursor_board_x,
             cursor_board_y, overlay_edit);

            current_param =
             change_param(mzx_world, current_id, current_param, &copy_robot,
             &copy_scroll, &copy_sensor);

            current_param = place_current_at_xy(mzx_world, current_id, current_color,
             current_param, cursor_board_x, cursor_board_y, &copy_robot,
             &copy_scroll, &copy_sensor, 0);

            modified = 1;
          }
          cursor_solid();
        }

        break;
      }

      case SDLK_F11:
      {
        // SMZX Mode
        set_screen_mode(get_screen_mode() + 1);
        break;
      }

      case SDLK_e:
      {
        if(draw_mode != 2)
        {
          if(get_alt_status(keycode_SDL))
          {
            palette_editor(mzx_world);
            modified = 1;
            cursor_solid();
          }
        }
        else
        {
          text_place = 1;
        }

        break;
      }

      case SDLK_f:
      {
        if(draw_mode != 2)
        {
          if(get_alt_status(keycode_SDL))
          {
            sfx_edit(mzx_world);
            modified = 1;
            cursor_solid();
          }
          else
          {
            fill_area(mzx_world, current_id, current_color,
             current_param, cursor_board_x, cursor_board_y,
             &copy_robot, &copy_scroll, &copy_sensor, overlay_edit);
            modified = 1;
          }
        }
        else
        {
          text_place = 1;
        }

        break;
      }

      case SDLK_ESCAPE:
      {
        if(draw_mode)
        {
          draw_mode = 0;
          copy_repeat_width = -1;
          copy_repeat_height = -1;
          key = 0;
        }
        else

        if(overlay_edit)
        {
          overlay_edit = 0;
          current_id = 0;
          current_param = 0;
          current_color = 7;
          key = 0;
        }
        else

        if(modified &&
         confirm(mzx_world, "Exit: World has not been saved, are you sure?"))
        {
          key = 0;
          cursor_solid();
        }
        break;
      }

      case SDLK_v:
      {
        if(draw_mode != 2)
        {
          // View mode
          int viewport_width = src_board->viewport_width;
          int viewport_height = src_board->viewport_height;
          int v_scroll_x = 0, v_scroll_y = 0;
          int v_key;

          cursor_off();

          do
          {
            draw_viewport(mzx_world);
            draw_game_window(src_board, v_scroll_x, v_scroll_y);
            update_screen();

            update_event_status_delay();
            v_key = get_key(keycode_SDL);

            switch(v_key)
            {
              case SDLK_LEFT:
              {
                if(v_scroll_x)
                  v_scroll_x--;
                break;
              }

              case SDLK_RIGHT:
              {
                if(v_scroll_x < (board_width - viewport_width))
                  v_scroll_x++;
                break;
              }

              case SDLK_UP:
              {
                if(v_scroll_y)
                  v_scroll_y--;
                break;
              }

              case SDLK_DOWN:
              {
                if(v_scroll_y < (board_height - viewport_height))
                  v_scroll_y++;
                break;
              }
            }
          } while(v_key != SDLK_ESCAPE);

          clear_screen_no_update(177, 1);
          cursor_solid();
        }
        else
        {
          text_place = 1;
        }
      }

      case SDLK_t:
      {
        if(get_alt_status(keycode_SDL))
        {
          int fade;
          int current_board_id = mzx_world->current_board_id;
          char current_name[128];
          char tempfile_name[128];

          tmpnam(tempfile_name);
          strcpy(current_name, current_world);

          save_world(mzx_world, tempfile_name, 0, 0);

          vquick_fadeout();
          cursor_off();
          src_board = mzx_world->board_list[current_board_id];
          fix_board(current_board_id);
          set_counter(mzx_world, "TIME", src_board->time_limit, 0);
          send_robot_def(mzx_world, 0, 11);
          send_robot_def(mzx_world, 0, 10);
          find_player(mzx_world);
          mzx_world->player_restart_x = mzx_world->player_x;
          mzx_world->player_restart_y = mzx_world->player_y;
          strcpy(mzx_world->real_mod_playing, src_board->mod_playing);
          play_game(mzx_world, 1);

          reload_world(mzx_world, tempfile_name, &fade);
          scroll_color = 15;
          find_player(mzx_world);
          mzx_world->current_board_id = current_board_id;
          mzx_world->current_board = mzx_world->board_list[current_board_id];
          synchronize_board_values();
          insta_fadein();
          fix_mod();
          unlink(tempfile_name);

          strcpy(current_world, current_name);
          cursor_solid();
        }
        else

        if(draw_mode == 2)
        {
          text_place = 1;
        }

        break;
      }

      case SDLK_a:
      {
        if(get_alt_status(keycode_SDL))
        {
          int charset_load = choose_char_set(mzx_world);

          switch(charset_load)
          {
            case 0:
            {
              ec_load_mzx();
              break;
            }

            case 1:
            {
              ec_load_ascii();
              break;
            }

            case 2:
            {
              ec_load_smzx();
              break;
            }

            case 3:
            {
              ec_load_blank();
              break;
            }
          }

          modified = 1;
          cursor_solid();
        }
        else

        if(draw_mode != 2)
        {
          // Add board, find first free
          for(i = 0; i < mzx_world->num_boards; i++)
          {
            if(mzx_world->board_list[i] == NULL)
              break;
          }
          if(add_board(mzx_world, i) >= 0)
          {
            fix_board(i);
            synchronize_board_values();
            fix_mod();
            fix_scroll();

            modified = 1;
          }
          cursor_solid();

        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_d:
      {
        if(draw_mode != 2)
        {
          if(get_alt_status(keycode_SDL))
          {
            default_palette();
            update_palette();
          }
          else
          {
            int del_board =
             choose_board(mzx_world, mzx_world->current_board_id,
             "Delete board:", 0);

            if((del_board > 0) &&
             !confirm(mzx_world, "DELETE BOARD - Are you sure?"))
            {
              int current_board_id = mzx_world->current_board_id;
              clear_board(mzx_world->board_list[del_board]);
              mzx_world->board_list[del_board] = NULL;

              // Remove null board from list
              optimize_null_boards(mzx_world);

              if(del_board == current_board_id)
              {
                fix_board(0);
                synchronize_board_values();
                if(strcmp(src_board->mod_playing, "*") &&
                 strcasecmp(src_board->mod_playing,
                 mzx_world->real_mod_playing))
                {
                  fix_mod();
                }
                fix_scroll();
              }
              else
              {
                synchronize_board_values();
              }
            }
          }

          modified = 1;
          cursor_solid();
        }
        else
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_z:
      {
        if(get_alt_status(keycode_SDL))
        {
          // Clear board
          if(!confirm(mzx_world, "Clear board - Are you sure?"))
          {
            int current_board_id = mzx_world->current_board_id;
            clear_board(src_board);
            src_board = create_blank_board();
            mzx_world->board_list[mzx_world->current_board_id] =
             src_board;
            mzx_world->current_board = src_board;
            synchronize_board_values();
            fix_scroll();
            end_mod();
            src_board->mod_playing[0] = 0;
            strcpy(mzx_world->real_mod_playing,
             src_board->mod_playing);
          }

          modified = 1;
          cursor_solid();
        }
        else

        if(draw_mode == 2)
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_r:
      {
        if(get_alt_status(keycode_SDL))
        {
          // Clear world
          if(!confirm(mzx_world, "Clear ALL - Are you sure?"))
          {
            clear_world(mzx_world);
            create_blank_world(mzx_world);
            synchronize_board_values();
            fix_scroll();
            end_mod();
          }

          modified = 1;
          cursor_solid();
        }
        else

        if(draw_mode == 2)
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_PAGEDOWN:
      {
        current_menu++;

        if(current_menu == 6)
          current_menu = 0;

        break;
      }

      case SDLK_PAGEUP:
      {
        current_menu--;

        if(current_menu == -1)
          current_menu = 5;

        break;
      }

      case SDLK_8:
      case SDLK_KP_MULTIPLY:
      {
        if(get_alt_status(keycode_SDL))
        {
          if(src_board->mod_playing[0])
            end_mod();
          src_board->mod_playing[0] = '*';
          src_board->mod_playing[1] = 0;

          modified = 1;
        }
        else

        if(draw_mode == 2)
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_m:
      {
        if(get_alt_status(keycode_SDL))
        {
          int offset = cursor_board_x + (cursor_board_y * board_width);
          int d_id = level_id[offset];
          int d_param = level_param[offset];

          if(d_id == 122)
          {
            edit_sensor(mzx_world, src_board->sensor_list[d_param]);
            modified = 1;
          }
          else

          if((d_id == 123) || (d_id == 124))
          {
            edit_robot(mzx_world, src_board->robot_list[d_param]);
            modified = 1;
          }
          else

          if((d_id == 125) || (d_id == 126))
          {
            edit_scroll(mzx_world, src_board->scroll_list[d_param]);
            modified = 1;
          }

          cursor_solid();
        }
        else

        if(draw_mode == 2)
        {
          text_place = 1;
        }
        break;
      }

      case SDLK_y:
      {
        if(get_alt_status(keycode_SDL))
        {
          debug_mode ^= 1;
        }
        else

        if(draw_mode == 2)
        {
          text_place = 1;
        }

        break;
      }

      case SDLK_h:
      {
        if(get_alt_status(keycode_SDL))
        {
          if(edit_screen_height == 19)
          {
            edit_screen_height = 25;
            if((scroll_y + 25) > board_height)
              scroll_y = board_height - 25;
          }
          else
          {
            edit_screen_height = 19;
            if((cursor_board_y - scroll_y) > 18)
              scroll_y += cursor_board_y - scroll_y - 18;
          }
        }
        else

        if(draw_mode == 2)
        {
          text_place = 1;
        }

        break;
      }


      case 0:
      {
        break;
      }

      default:
      {
        if(draw_mode == 2)
          text_place = 1;
      }
    }

    if(text_place)
    {
      key = get_key(keycode_unicode);
      if(key != 0)
      {
        place_current_at_xy(mzx_world, 77, current_color,
         key, cursor_board_x, cursor_board_y, &copy_robot,
         &copy_scroll, &copy_sensor, overlay_edit);

        if(cursor_board_x < (board_width - 1))
          cursor_board_x++;

        if((cursor_board_x > 74) && (cursor_board_x < (board_width - 5)) &&
         (scroll_x < (board_width - 80)))
          scroll_x++;

        modified = 1;
      }
    }
  } while(key != SDLK_ESCAPE);

  update_event_status();
  clear_world(mzx_world);
  clear_global_data(mzx_world);
  cursor_off();
  end_mod();
  m_hide();
  clear_screen(32, 7);
  insta_fadeout();
  strcpy(curr_file, current_world);
}

