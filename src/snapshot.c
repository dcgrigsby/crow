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
#include <string.h>
#include "crobots.h"
#include "snapshot.h"

/* Global file pointer for snapshot output */
static FILE *snapshot_fp = NULL;

/* Battlefield dimensions for ASCII grid */
#define GRID_WIDTH  50
#define GRID_HEIGHT 20

/**
 * convert_to_grid - Convert game coordinate to grid position
 * @pos: Position in clicks * 100 (centimeter precision)
 * @max_pos: Maximum position (MAX_X or MAX_Y in meters * CLICK)
 * @grid_size: Grid dimension (GRID_WIDTH or GRID_HEIGHT)
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
  char grid[GRID_HEIGHT][GRID_WIDTH];
  int i, j, r, m;
  int gx, gy;

  /* Initialize grid with spaces */
  for (i = 0; i < GRID_HEIGHT; i++) {
    for (j = 0; j < GRID_WIDTH; j++) {
      grid[i][j] = ' ';
    }
  }

  /* Place robots on grid */
  for (r = 0; r < MAXROBOTS; r++) {
    if (robots[r].status != ACTIVE)
      continue;

    gx = convert_to_grid(robots[r].x, MAX_X * CLICK, GRID_WIDTH);
    gy = convert_to_grid(robots[r].y, MAX_Y * CLICK, GRID_HEIGHT);

    if (gx >= 0 && gy >= 0) {
      /* Invert Y-axis: game Y increases upward, but grid rows increase downward */
      gy = GRID_HEIGHT - 1 - gy;
      grid[gy][gx] = '1' + r;  /* Robot numbers 1-4 */
    }
  }

  /* Place missiles on grid */
  for (r = 0; r < MAXROBOTS; r++) {
    for (m = 0; m < MIS_ROBOT; m++) {
      if (missiles[r][m].stat != FLYING && missiles[r][m].stat != EXPLODING)
        continue;

      gx = convert_to_grid(missiles[r][m].cur_x, MAX_X * CLICK, GRID_WIDTH);
      gy = convert_to_grid(missiles[r][m].cur_y, MAX_Y * CLICK, GRID_HEIGHT);

      if (gx >= 0 && gy >= 0) {
        /* Invert Y-axis: game Y increases upward, but grid rows increase downward */
        gy = GRID_HEIGHT - 1 - gy;
        /* Only overwrite spaces, don't overwrite robots */
        if (grid[gy][gx] == ' ')
          grid[gy][gx] = '*';
      }
    }
  }

  /* Print battlefield header */
  fprintf(snapshot_fp, "=== CYCLE %ld ===\n", cycle);
  fprintf(snapshot_fp, "BATTLEFIELD (1000x1000m):\n");

  /* Print top border */
  fprintf(snapshot_fp, "+");
  for (j = 0; j < GRID_WIDTH; j++)
    fprintf(snapshot_fp, "-");
  fprintf(snapshot_fp, "+\n");

  /* Print grid rows */
  for (i = 0; i < GRID_HEIGHT; i++) {
    fprintf(snapshot_fp, "|");
    for (j = 0; j < GRID_WIDTH; j++) {
      fprintf(snapshot_fp, "%c", grid[i][j]);
    }
    fprintf(snapshot_fp, "|\n");
  }

  /* Print bottom border */
  fprintf(snapshot_fp, "+");
  for (j = 0; j < GRID_WIDTH; j++)
    fprintf(snapshot_fp, "-");
  fprintf(snapshot_fp, "+\n");
  fprintf(snapshot_fp, "\n");
}

/**
 * output_robot_table - Output robot data table
 */
static void output_robot_table(void)
{
  int r;
  int x_meters, y_meters;
  const char *status_str;

  fprintf(snapshot_fp, "ROBOTS:\n");

  for (r = 0; r < MAXROBOTS; r++) {
    if (robots[r].status != ACTIVE)
      continue;

    /* Convert positions from clicks*100 to meters */
    x_meters = robots[r].x / CLICK;
    y_meters = robots[r].y / CLICK;

    status_str = (robots[r].damage >= 100) ? "DEAD" : "ACTIVE";

    fprintf(snapshot_fp,
            "[%d] %-16s | Pos: (%4d,%4d) | Head: %03d | Speed: %03d | Damage: %3d%% | %s\n",
            r + 1,
            robots[r].name,
            x_meters,
            y_meters,
            robots[r].heading,
            robots[r].speed,
            robots[r].damage,
            status_str);
  }

  fprintf(snapshot_fp, "\n");
}

/**
 * output_missile_table - Output missile data table
 */
static void output_missile_table(void)
{
  int r, m;
  int x_meters, y_meters;
  const char *stat_str;

  fprintf(snapshot_fp, "MISSILES:\n");

  for (r = 0; r < MAXROBOTS; r++) {
    for (m = 0; m < MIS_ROBOT; m++) {
      if (missiles[r][m].stat == AVAIL)
        continue;

      /* Convert positions and distances from clicks*100 to display units */
      x_meters = missiles[r][m].cur_x / CLICK;
      y_meters = missiles[r][m].cur_y / CLICK;

      /* Determine status string */
      switch (missiles[r][m].stat) {
      case FLYING:
        stat_str = "FLYING";
        break;
      case EXPLODING:
        stat_str = "EXPLODING";
        break;
      default:
        stat_str = "UNKNOWN";
      }

      fprintf(snapshot_fp,
              "[%d.%d] %-10s | Pos: (%4d,%4d) | Head: %03d | Range: %04d | Dist: %04d\n",
              r + 1, m,
              stat_str,
              x_meters,
              y_meters,
              missiles[r][m].head,
              missiles[r][m].rang,
              missiles[r][m].curr_dist / CLICK);
    }
  }

  fprintf(snapshot_fp, "\n");
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
}

void output_snapshot(long cycle)
{
  if (!snapshot_fp)
    return;

  /* Draw ASCII battlefield visualization */
  draw_battlefield(cycle);

  /* Output robot data table */
  output_robot_table();

  /* Output missile data table */
  output_missile_table();
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
