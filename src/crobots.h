/* crobots.h - shared constants and data types
 *
 * Copyright (C) 1985-2013 Tom Poindexter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef CROBOTS_H_
#define CROBOTS_H_

#include "config.h"

#define ILEN           8	/* length of identifiers, also in lexanal.l */
#define MAXROBOTS      4	/* maximum number of robots */
#define CODESPACE      INSTRMAX	/* maximum number of machine instructions (1000) */
#define DATASPACE      DATAMAX	/* maximum number of data stack entries (500) */
#define UPDATE_CYCLES  30	/* number of cycles before screen update (30) */
#define MOTION_CYCLES  15 	/* number of cycles before motion update (15) */
#define CYCLE_DELAY    200	/* microseconds of sleep to slow down things when display is on*/
#define CYCLE_LIMIT    500000L	/* limit of background cycles */
#define ROBOT_SPEED    7	/* multiplicative speed factor */
#define TURN_SPEED     50	/* maximum speed for direction change */
#define ACCEL          10	/* acceleration factor per motion cycle */

#define ACTIVE         1	/* robot active & running */
#define DEAD           0	/* dead robot */



typedef struct instr {		/* robot machine instruction */
  char ins_type;		/* instruction type */
  union {
    long k;			/* constant value */
    short int var1;		/* variable offset, function offset, operator */
    struct instr *br;		/* false branch */
    struct {
      short int var2;		/* assignment variable offset */
      short int a_op;		/* assignment operator */
    } a;
  } u;
} s_instr;

typedef struct func {		/* function header */
  struct func *nextfunc;	/* next function header in chain */
  char func_name[ILEN];		/* function name */
  s_instr *first;		/* first instruction pointer */
  int var_count;		/* number of pool variables needed */
  int par_count;		/* number of parameters expected */
} s_func;

/* Action logging structures */
typedef struct action_log {
    int type;           /* ACTION_DRIVE, ACTION_SCAN, ACTION_CANNON */
    int param1;         /* heading for all actions */
    int param2;         /* speed (DRIVE), width (SCAN), range (CANNON) */
} s_action_log;

#define MAX_ACTIONS_PER_SNAPSHOT 100

typedef struct robot_actions {
    s_action_log actions[MAX_ACTIONS_PER_SNAPSHOT];
    int count;
} s_robot_actions;

typedef struct robot {		/* robot context */
  int status;			/* status of robot, active or dead */
  char name[14];		/* name of robot */
  int x;			/* current x location * 100 */
  int y;			/* current y location * 100 */
  int org_x;			/* orgin x location * 100 */
  int org_y;			/* orgin y location * 100 */
  int range;			/* distance traveled on this heading * 100 */
  int last_x;			/* last plotted physical x */
  int last_y;			/* last plotted physical y */
  int speed;			/* current speed, 0 - 100 */
  int last_speed;		/* last speed, 0 - 100 */
  int accel;			/* linear acceleration / de-acceleration lag */
  int d_speed;			/* desired speed */
  int heading;			/* current heading, 0 - 359 */
  int last_heading;		/* last heading, 0 - 359 */
  int d_heading;		/* desired heading, 0 - 359 */
  int damage;			/* damage sustained, percent */
  int last_damage;		/* last damage */
  int scan;			/* current scan direction */
  int last_scan;		/* last scan direction */
  int reload;			/* number of cycles between reloading */
  int ext_count;		/* size of external pool needed */
  long *external;		/* external variable pool  (Lower MEM address) ?? */
  long *local;			/* current local variables on stack */
  long *stackbase;		/* base of local & expression stack */
  long *stackend;		/* end of stack (Higher MEM address) ?? */
  long *stackptr;		/* current stack pointer, grows up */
  long *retptr;			/* return frame pointers, grow down */
  char *funcs;			/* table of function names by offset */
  s_func *code_list;		/* list of function headers */
  s_instr *code;		/* machine instructions, actually instr */
  s_instr *ip; 			/* instruction pointer */
  s_robot_actions action_buffer;	/* Action logging buffer */
} s_robot;


/* missile constants */
#define MIS_SPEED 500		/* how far in one motion cycle (in clicks) (originally 500)*/
#define MIS_ROBOT 2		/* number of active missiles per robot */
#define AVAIL  0		/* missile available for use */
#define FLYING 1		/* missile in air */
#define EXPLODING 2		/* missile exploding on ground */
#define RELOAD 15		/* motion cycles before reload */
#define EXP_COUNT 5 		/* motion cycles for exploding missile */

typedef struct missile {	/* active missiles */
  int stat;			/* missile status */
  int beg_x;			/* beginning x * 100 */
  int beg_y;			/* beginning y * 100 */
  int cur_x;			/* current x * 100 */
  int cur_y;			/* current y * 100 */
  int last_xx;			/* last plotted x */
  int last_yy;			/* last plotted y */
  int head;			/* heading, 0 - 359 */
  int count;			/* cycle count for exploding missiles */
  int rang;			/* range of missile */
  int curr_dist;		/* current distance from orgin * 100 */
} s_missile;

extern s_missile missiles[MAXROBOTS][MIS_ROBOT];

extern
s_robot *cur_robot,		/* current robot */
        robots[MAXROBOTS];	/* all robots */

extern
int r_debug,			/* debug switch */
    r_flag;			/* global flag for push/pop errors */

/* instruction types */
#define NOP    0		/* end of code marker */
#define FETCH  1		/* push(varpool(offset)) */
#define STORE  2		/* push(pop op pop) --> varpool(offset) */
#define CONST  3		/* push(constant) */
#define BINOP  4		/* pop -->y, pop -->x, push(x op y) */
#define FCALL  5		/* pop --> parmn..parm1, save ip, call */
#define RETSUB 6		/* push(returnval), restore ip */
#define BRANCH 7		/* if (pop == 0) branch --> ip*/
#define CHOP   8		/* pop --> bit bucket */
#define FRAME  9		/* frame stack pointer for call */

/* external variable flag (or'ed in or and'ed out) , also in grammar.y */
#define EXTERNAL 0x8000

/* size of battlefield, see MIS_RANGE also. note - x and y coordinates */
/* of robots and missiles are measured in CLICKS * MAX_? */
#define CLICK 10		/* 10 clicks per meter */

/* Global configuration structure for runtime-configurable parameters */
typedef struct {
    int battlefield_size;   /* CLI value (e.g., 1024) */
    int snapshot_grid_size; /* CLI value (e.g., 128) */
    int max_x;             /* battlefield_size (replaces MAX_X) */
    int max_y;             /* battlefield_size (replaces MAX_Y) */
    int mis_range;         /* 70% of battlefield_size */
    int max_instr;         /* Maximum robot instruction limit (default 1000) */
    int snapshot_interval;  /* Cycles between snapshots (default 30) */
    int log_actions;        /* -a flag: log actions (default 1) */
    int log_rewards;        /* -r flag: log rewards (default 1) */
} config_t;

extern config_t g_config;

/* Macro aliases for seamless transition from compile-time constants */
#define MAX_X g_config.max_x
#define MAX_Y g_config.max_y
#define MIS_RANGE g_config.mis_range

/* damage factors, percent */
#define DIRECT_HIT 10
#define NEAR_HIT   5
#define FAR_HIT    3
#define COLLISION  2
/* distance of damage factors */
#define DIRECT_RANGE 5
#define NEAR_RANGE   20
#define FAR_RANGE    40

/* Action types for logging */
#define ACTION_DRIVE  1
#define ACTION_SCAN   2
#define ACTION_CANNON 3

/* Damage tracking structures */
typedef struct damage_event {
    int victim;         /* Robot index that took damage */
    int attacker;       /* Robot index that dealt damage (-1 for collision/wall) */
    int amount;         /* Damage amount in percent */
} s_damage_event;

#define MAX_DAMAGE_EVENTS 50

typedef struct damage_tracker {
    s_damage_event events[MAX_DAMAGE_EVENTS];
    int count;
} s_damage_tracker;

/* motion functions */

#endif /* CROBOTS_H_ */

/**
 * Local Variables:
 *  indent-tabs-mode: nil
 *  c-file-style: "gnu"
 * End:
 */
