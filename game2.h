/* $Id$
 * MegaZeux
 *
 * Copyright (C) 1996 Greg Janson
 * Copyright (C) 1998 Matthew D. Williams - dbwilli@scsn.net
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

// Include file for GAME2.ASM

#ifndef __GAME2_H
#define __GAME2_H

void hurt_player_id(World *mzx_world, int id);
int find_seek(World *mzx_world, int x, int y);
int inc_param(int param, int max);
int xy2array2(Board *src_board, int x, int y);
int arraydir2(Board *src_board, int x, int y, int *ret_x, int *ret_y,
 int direction);
void update_board(World *mzx_world);
void shoot_lazer(World *mzx_world, int x, int y, int dir, int length,
 int color);
int try_transport(World *mzx_world, int x, int y, int push_status,
 int can_push, int id, int dir);
int transport(World *mzx_world, int x, int y, int dir, int id, int param,
 int color, int can_push);
int push(World *mzx_world, int x, int y, int dir, int checking);
void shoot(World *mzx_world, int x, int y, int dir, int type);
void shoot_fire(World *mzx_world, int x, int y, int dir);
void shoot_seeker(World *mzx_world, int x, int y, int dir);
void shoot_missile(World *mzx_world, int x, int y, int dir);
int move(World *mzx_world, int x, int y, int dir, int move_flags);
int parsedir(World *mzx_world, int old_dir, int x, int y, int flow_dir,
 int bln, int bls, int ble, int blw);

#endif
