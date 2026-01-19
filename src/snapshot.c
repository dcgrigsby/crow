/* snapshot.c - game state snapshot output for ML training
 *
 * Copyright (C) 2025
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crobots.h"
#include "snapshot.h"

/* Global file pointer for snapshot output */
static FILE *snapshot_fp = NULL;

/* External damage tracker and functions */
extern s_damage_tracker damage_tracker;
void reset_damage_tracker(void);

/* Simplified state structures for buffering */
typedef struct {
    int status;
    int x, y;
    int heading;
    int speed;
    int damage;
    char name[14];
} s_snapshot_robot_state;

typedef struct {
    int stat;
    int cur_x, cur_y;
    int head;
    int rang_remaining;
} s_snapshot_missile_state;

/* State buffers - static storage for previous state */
static s_snapshot_robot_state prev_robots[MAXROBOTS];
static s_snapshot_missile_state prev_missiles[MAXROBOTS * MIS_ROBOT];
static int has_prev_state = 0;
static long prev_cycle = 0;


/**
 * output_state_robots - Output robot state in plain text format from buffer
 */
static void output_state_robots(s_snapshot_robot_state *robot_states)
{
    int r;

    for (r = 0; r < MAXROBOTS; r++) {
        if (robot_states[r].status != ACTIVE)
            continue;

        fprintf(snapshot_fp, "ROBOT %d %s %d %d %d %d %d\n",
                r + 1,
                robot_states[r].name,
                robot_states[r].x,
                robot_states[r].y,
                robot_states[r].heading,
                robot_states[r].speed,
                robot_states[r].damage);
    }
}

/**
 * output_state_missiles - Output missile state in plain text format from buffer
 */
static void output_state_missiles(s_snapshot_missile_state *missile_states)
{
    int r, m;

    for (r = 0; r < MAXROBOTS; r++) {
        for (m = 0; m < MIS_ROBOT; m++) {
            int idx = r * MIS_ROBOT + m;
            if (missile_states[idx].stat == AVAIL)
                continue;

            fprintf(snapshot_fp, "MISSILE %d.%d %s %d %d %d %d 0\n",
                    r + 1,
                    m,
                    (missile_states[idx].stat == FLYING) ? "FLYING" : "EXPLODING",
                    missile_states[idx].cur_x,
                    missile_states[idx].cur_y,
                    missile_states[idx].head,
                    missile_states[idx].rang_remaining);
        }
    }
}

/**
 * output_current_state_robots - Output current robot state in plain text format
 */
static void output_current_state_robots(void)
{
    int r;

    for (r = 0; r < MAXROBOTS; r++) {
        if (robots[r].status != ACTIVE)
            continue;

        fprintf(snapshot_fp, "ROBOT %d %s %d %d %d %d %d\n",
                r + 1,
                robots[r].name,
                robots[r].x / CLICK,
                robots[r].y / CLICK,
                robots[r].heading,
                robots[r].speed,
                robots[r].damage);
    }
}

/**
 * output_current_state_missiles - Output current missile state in plain text format
 */
static void output_current_state_missiles(void)
{
    int r, m;

    for (r = 0; r < MAXROBOTS; r++) {
        for (m = 0; m < MIS_ROBOT; m++) {
            if (missiles[r][m].stat == AVAIL)
                continue;

            fprintf(snapshot_fp, "MISSILE %d.%d %s %d %d %d %d 0\n",
                    r + 1,
                    m,
                    (missiles[r][m].stat == FLYING) ? "FLYING" : "EXPLODING",
                    missiles[r][m].cur_x / CLICK,
                    missiles[r][m].cur_y / CLICK,
                    missiles[r][m].head,
                    (missiles[r][m].rang - missiles[r][m].curr_dist) / CLICK);
        }
    }
}

/**
 * output_action_list - Output actions executed in this interval
 */
static void output_action_list(void)
{
    int r, i;
    const char *action_name;

    if (!g_config.log_actions)
        return;

    for (r = 0; r < MAXROBOTS; r++) {
        if (robots[r].status != ACTIVE)
            continue;

        for (i = 0; i < robots[r].action_buffer.count; i++) {
            switch (robots[r].action_buffer.actions[i].type) {
                case ACTION_DRIVE:  action_name = "DRIVE"; break;
                case ACTION_SCAN:   action_name = "SCAN"; break;
                case ACTION_CANNON: action_name = "CANNON"; break;
                default:            action_name = "UNKNOWN";
            }
            fprintf(snapshot_fp, "ACTION %d %s %d %d\n",
                    r + 1,
                    action_name,
                    robots[r].action_buffer.actions[i].param1,
                    robots[r].action_buffer.actions[i].param2);
        }
    }
}

/**
 * copy_current_state_to_buffer - Save current state to static buffers
 */
static void copy_current_state_to_buffer(void)
{
    int r, m;

    for (r = 0; r < MAXROBOTS; r++) {
        prev_robots[r].status = robots[r].status;
        prev_robots[r].x = robots[r].x / CLICK;
        prev_robots[r].y = robots[r].y / CLICK;
        prev_robots[r].heading = robots[r].heading;
        prev_robots[r].speed = robots[r].speed;
        prev_robots[r].damage = robots[r].damage;
        strncpy(prev_robots[r].name, robots[r].name, 13);
        prev_robots[r].name[13] = '\0';

        for (m = 0; m < MIS_ROBOT; m++) {
            int idx = r * MIS_ROBOT + m;
            prev_missiles[idx].stat = missiles[r][m].stat;
            prev_missiles[idx].cur_x = missiles[r][m].cur_x / CLICK;
            prev_missiles[idx].cur_y = missiles[r][m].cur_y / CLICK;
            prev_missiles[idx].head = missiles[r][m].head;
            prev_missiles[idx].rang_remaining =
                (missiles[r][m].rang - missiles[r][m].curr_dist) / CLICK;
        }
    }
}

/**
 * calculate_reward - Calculate per-robot reward for this tick
 * @robot_idx: Robot index
 * Returns: damage_dealt - damage_taken
 */
static int calculate_reward(int robot_idx)
{
    int damage_dealt = 0;
    int damage_taken = 0;
    int i;

    for (i = 0; i < damage_tracker.count; i++) {
        if (damage_tracker.events[i].victim == robot_idx) {
            damage_taken += damage_tracker.events[i].amount;
        }
        if (damage_tracker.events[i].attacker == robot_idx) {
            damage_dealt += damage_tracker.events[i].amount;
        }
    }

    return damage_dealt - damage_taken;
}

/**
 * clear_action_buffers - Clear action buffers for next snapshot
 */
static void clear_action_buffers(void)
{
    int r;
    for (r = 0; r < MAXROBOTS; r++) {
        robots[r].action_buffer.count = 0;
    }
}

void init_snapshot(FILE *fp)
{
  if (!fp)
    return;

  snapshot_fp = fp;

  /* Write file header */
  fprintf(snapshot_fp, "CROBOTS SNAPSHOT LOG\n");

  /* Reset state buffering on init */
  has_prev_state = 0;
  prev_cycle = 0;
}

void output_snapshot(long cycle)
{
  if (!snapshot_fp)
    return;

  /* First snapshot: just buffer state, don't output */
  if (!has_prev_state) {
    copy_current_state_to_buffer();
    prev_cycle = cycle;
    has_prev_state = 1;
    return;
  }

  /* Output interval in plain text format */
  fprintf(snapshot_fp, "INTERVAL %ld %ld\n", prev_cycle, cycle);

  /* Output initial state robots and missiles */
  output_state_robots(prev_robots);
  output_state_missiles(prev_missiles);

  /* Output actions */
  output_action_list();

  /* Output final state robots and missiles */
  output_current_state_robots();
  output_current_state_missiles();

  /* Match separator */
  fprintf(snapshot_fp, "---\n");

  /* Copy current state to buffer for next iteration */
  copy_current_state_to_buffer();
  prev_cycle = cycle;

  /* Clear buffers for next snapshot period */
  clear_action_buffers();
  reset_damage_tracker();
}

void close_snapshot(void)
{
  if (!snapshot_fp)
    return;

  /* Don't close the file here - it's managed by main.c */
  snapshot_fp = NULL;
}

/**
 * Local Variables:
 *  indent-tabs-mode: nil
 *  c-file-style: "gnu"
 * End:
 */
