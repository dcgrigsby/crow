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
 * convert_to_grid - Convert game coordinate to grid position
 * @pos: Position in clicks * 100 (centimeter precision)
 * @max_pos: Maximum position (MAX_X or MAX_Y in meters * CLICK)
 * @grid_size: Grid dimension (configurable)
 *
 * Returns: Grid coordinate (0 to grid_size-1), or -1 if out of bounds
 */
static int convert_to_grid(int pos, int max_pos, int grid_size)
{
  int meters = pos / CLICK;
  int grid_pos = (meters * grid_size) / max_pos;

  if (grid_pos < 0 || grid_pos >= grid_size)
    return -1;
  return grid_pos;
}

/**
 * draw_battlefield - Draw ASCII battlefield with robot and missile positions
 * @cycle: Current cycle number (for display only)
 */
static void draw_battlefield(long cycle)
{
  int grid_width = g_config.snapshot_grid_size;
  int grid_height = g_config.snapshot_grid_size;
  char *grid;
  int i, j, r, m;
  int gx, gy;

  /* Allocate dynamic grid */
  grid = calloc(grid_height * grid_width, sizeof(char));
  if (!grid) {
    fprintf(stderr, "Failed to allocate grid memory\n");
    return;
  }

  /* Initialize grid with spaces */
  for (i = 0; i < grid_height * grid_width; i++) {
    grid[i] = ' ';
  }

  /* Place robots on grid */
  for (r = 0; r < MAXROBOTS; r++) {
    if (robots[r].status != ACTIVE)
      continue;

    gx = convert_to_grid(robots[r].x, MAX_X * CLICK, grid_width);
    gy = convert_to_grid(robots[r].y, MAX_Y * CLICK, grid_height);

    if (gx >= 0 && gy >= 0) {
      /* Invert Y-axis: game Y increases upward, but grid rows increase downward */
      gy = grid_height - 1 - gy;
      grid[gy * grid_width + gx] = '1' + r;  /* Robot numbers 1-4 */
    }
  }

  /* Place missiles on grid */
  for (r = 0; r < MAXROBOTS; r++) {
    for (m = 0; m < MIS_ROBOT; m++) {
      if (missiles[r][m].stat != FLYING && missiles[r][m].stat != EXPLODING)
        continue;

      gx = convert_to_grid(missiles[r][m].cur_x, MAX_X * CLICK, grid_width);
      gy = convert_to_grid(missiles[r][m].cur_y, MAX_Y * CLICK, grid_height);

      if (gx >= 0 && gy >= 0) {
        /* Invert Y-axis: game Y increases upward, but grid rows increase downward */
        gy = grid_height - 1 - gy;
        /* Only overwrite spaces, don't overwrite robots */
        if (grid[gy * grid_width + gx] == ' ')
          grid[gy * grid_width + gx] = '*';
      }
    }
  }

  /* Print battlefield header */
  fprintf(snapshot_fp, "=== CYCLE %ld ===\n", cycle);
  fprintf(snapshot_fp, "BATTLEFIELD (%dx%dm):\n", g_config.battlefield_size, g_config.battlefield_size);

  /* Print top border */
  fprintf(snapshot_fp, "+");
  for (j = 0; j < grid_width; j++)
    fprintf(snapshot_fp, "-");
  fprintf(snapshot_fp, "+\n");

  /* Print grid rows */
  for (i = 0; i < grid_height; i++) {
    fprintf(snapshot_fp, "|");
    for (j = 0; j < grid_width; j++) {
      fprintf(snapshot_fp, "%c", grid[i * grid_width + j]);
    }
    fprintf(snapshot_fp, "|\n");
  }

  /* Print bottom border */
  fprintf(snapshot_fp, "+");
  for (j = 0; j < grid_width; j++)
    fprintf(snapshot_fp, "-");
  fprintf(snapshot_fp, "+\n");
  fprintf(snapshot_fp, "\n");

  /* Free allocated memory */
  free(grid);
}

/**
 * output_state_robots - Output robot state in structured format from buffer
 */
static void output_state_robots(s_snapshot_robot_state *robot_states, long cycle)
{
    int r;

    fprintf(snapshot_fp, "ROBOTS:\n");

    for (r = 0; r < MAXROBOTS; r++) {
        if (robot_states[r].status != ACTIVE)
            continue;

        fprintf(snapshot_fp, "[%d] %s | pos:(%d,%d) | heading:%03d | speed:%03d | damage:%d\n",
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
 * output_state_missiles - Output missile state in structured format from buffer
 */
static void output_state_missiles(s_snapshot_missile_state *missile_states)
{
    int r, m;
    int count = 0;

    for (r = 0; r < MAXROBOTS; r++) {
        for (m = 0; m < MIS_ROBOT; m++) {
            int idx = r * MIS_ROBOT + m;
            if (missile_states[idx].stat == AVAIL)
                continue;

            if (count == 0)
                fprintf(snapshot_fp, "MISSILES:\n");

            fprintf(snapshot_fp, "[%d] %s | pos:(%d,%d) | heading:%03d | range:%d\n",
                    r + 1,
                    (missile_states[idx].stat == FLYING) ? "FLYING" : "EXPLODING",
                    missile_states[idx].cur_x,
                    missile_states[idx].cur_y,
                    missile_states[idx].head,
                    missile_states[idx].rang_remaining);
            count++;
        }
    }
}

/**
 * output_current_state_robots - Output current robot state in structured format
 */
static void output_current_state_robots(void)
{
    int r;

    fprintf(snapshot_fp, "ROBOTS:\n");

    for (r = 0; r < MAXROBOTS; r++) {
        if (robots[r].status != ACTIVE)
            continue;

        fprintf(snapshot_fp, "[%d] %s | pos:(%d,%d) | heading:%03d | speed:%03d | damage:%d\n",
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
 * output_current_state_missiles - Output current missile state in structured format
 */
static void output_current_state_missiles(void)
{
    int r, m;
    int count = 0;

    for (r = 0; r < MAXROBOTS; r++) {
        for (m = 0; m < MIS_ROBOT; m++) {
            if (missiles[r][m].stat == AVAIL)
                continue;

            if (count == 0)
                fprintf(snapshot_fp, "MISSILES:\n");

            fprintf(snapshot_fp, "[%d] %s | pos:(%d,%d) | heading:%03d | range:%d\n",
                    r + 1,
                    (missiles[r][m].stat == FLYING) ? "FLYING" : "EXPLODING",
                    missiles[r][m].cur_x / CLICK,
                    missiles[r][m].cur_y / CLICK,
                    missiles[r][m].head,
                    (missiles[r][m].rang - missiles[r][m].curr_dist) / CLICK);
            count++;
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

        fprintf(snapshot_fp, "[%d] ", r + 1);

        if (robots[r].action_buffer.count == 0) {
            fprintf(snapshot_fp, "(none)\n");
            continue;
        }

        for (i = 0; i < robots[r].action_buffer.count; i++) {
            switch (robots[r].action_buffer.actions[i].type) {
                case ACTION_DRIVE:  action_name = "DRIVE"; break;
                case ACTION_SCAN:   action_name = "SCAN"; break;
                case ACTION_CANNON: action_name = "CANNON"; break;
                default:            action_name = "UNKNOWN";
            }
            fprintf(snapshot_fp, "%s(%d,%d) ",
                    action_name,
                    robots[r].action_buffer.actions[i].param1,
                    robots[r].action_buffer.actions[i].param2);
        }
        fprintf(snapshot_fp, "\n");
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
  fprintf(snapshot_fp, "CROBOTS GAME STATE SNAPSHOT LOG\n");
  fprintf(snapshot_fp, "================================\n");
  fprintf(snapshot_fp, "\n");

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

  /* Output interval with INITIAL_STATE, ACTIONS, EVENTS, FINAL_STATE */
  fprintf(snapshot_fp, "<INTERVAL cycle_start=%ld cycle_end=%ld>\n\n", prev_cycle, cycle);

  fprintf(snapshot_fp, "<INITIAL_STATE>\n");
  output_state_robots(prev_robots, prev_cycle);
  output_state_missiles(prev_missiles);
  fprintf(snapshot_fp, "</INITIAL_STATE>\n\n");

  fprintf(snapshot_fp, "<ACTIONS>\n");
  output_action_list();
  fprintf(snapshot_fp, "</ACTIONS>\n\n");

  fprintf(snapshot_fp, "<EVENTS>\n");
  fprintf(snapshot_fp, "</EVENTS>\n\n");

  fprintf(snapshot_fp, "<FINAL_STATE>\n");
  output_current_state_robots();
  output_current_state_missiles();
  fprintf(snapshot_fp, "</FINAL_STATE>\n\n");

  /* Optional ASCII visualization */
  if (g_config.show_ascii) {
    fprintf(snapshot_fp, "DEBUG_VISUALIZATION:\n");
    draw_battlefield(cycle);
    fprintf(snapshot_fp, "\n");
  }

  fprintf(snapshot_fp, "</INTERVAL>\n\n");

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

  /* Write match separator */
  fprintf(snapshot_fp, "---\n\n");

  /* Don't close the file here - it's managed by main.c */
  snapshot_fp = NULL;
}

/**
 * Local Variables:
 *  indent-tabs-mode: nil
 *  c-file-style: "gnu"
 * End:
 */
