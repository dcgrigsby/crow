/* visualize.c - Snapshot visualization utility for CROBOTS
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
#include <unistd.h>
#include <getopt.h>
#include <err.h>

/* Minimal constants from crobots.h */
#define MAXROBOTS 4
#define MIS_ROBOT 2
#define CLICK 10        /* 10 clicks per meter */
#define BATTLEFIELD_SIZE 1024
#define MAX_X (BATTLEFIELD_SIZE)
#define MAX_Y (BATTLEFIELD_SIZE)

/* Limit grid size to reasonable range */
#define MIN_GRID_SIZE 32
#define MAX_GRID_SIZE 256

/* Parsed state for current interval */
typedef struct {
    int id;
    char name[14];
    int x, y;
    int heading;
    int speed;
    int damage;
} interval_robot;

typedef struct {
    int id;
    char status[16];  /* FLYING or EXPLODING */
    int x, y;
    int heading;
    int range;
    int distance;
} interval_missile;

typedef struct {
    int robot_id;
    char action[16];
    int param1;
    int param2;
} interval_action;

typedef struct {
    long start_cycle, end_cycle;
    interval_robot robots[MAXROBOTS];
    int robot_count;
    interval_missile missiles[MAXROBOTS * 8];
    int missile_count;
    interval_action actions[100];
    int action_count;
} interval_data;

/* Global parameters */
static int grid_size = 128;
static int match_num = 0;
static int delay_ms = 100;
static FILE *output_file = NULL;
static const char *output_filename = NULL;

/**
 * convert_to_grid - Convert game coordinate to grid position
 * @pos: Position in clicks (meters * 100 converted to meters)
 * @max_pos: Maximum position (MAX_X or MAX_Y in meters)
 * @grid_size: Grid dimension (configurable)
 *
 * Returns: Grid coordinate (0 to grid_size-1), or -1 if out of bounds
 */
static int convert_to_grid(int pos, int max_pos, int grid_size)
{
    int grid_pos = (pos * grid_size) / max_pos;

    if (grid_pos < 0 || grid_pos >= grid_size)
        return -1;
    return grid_pos;
}

/**
 * draw_battlefield - Draw ASCII battlefield with robot and missile positions
 * @interval: Parsed interval data
 * @fp: Output file pointer (stdout or file)
 */
static void draw_battlefield(interval_data *interval, FILE *fp)
{
    int grid_width = grid_size;
    int grid_height = grid_size;
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

    /* Place robots on grid (robot IDs are 1-4) */
    for (r = 0; r < interval->robot_count; r++) {
        int robot_id = interval->robots[r].id;
        gx = convert_to_grid(interval->robots[r].x, MAX_X, grid_width);
        gy = convert_to_grid(interval->robots[r].y, MAX_Y, grid_height);

        if (gx >= 0 && gy >= 0) {
            /* Invert Y-axis: game Y increases upward, but grid rows increase downward */
            gy = grid_height - 1 - gy;
            grid[gy * grid_width + gx] = '0' + robot_id;
        }
    }

    /* Place missiles on grid */
    for (m = 0; m < interval->missile_count; m++) {
        gx = convert_to_grid(interval->missiles[m].x, MAX_X, grid_width);
        gy = convert_to_grid(interval->missiles[m].y, MAX_Y, grid_height);

        if (gx >= 0 && gy >= 0) {
            /* Invert Y-axis */
            gy = grid_height - 1 - gy;
            /* Only overwrite spaces, don't overwrite robots */
            if (grid[gy * grid_width + gx] == ' ')
                grid[gy * grid_width + gx] = '*';
        }
    }

    /* Print top border */
    fprintf(fp, "+");
    for (j = 0; j < grid_width; j++)
        fprintf(fp, "-");
    fprintf(fp, "+\n");

    /* Print grid rows */
    for (i = 0; i < grid_height; i++) {
        fprintf(fp, "|");
        for (j = 0; j < grid_width; j++) {
            fprintf(fp, "%c", grid[i * grid_width + j]);
        }
        fprintf(fp, "|\n");
    }

    /* Print bottom border */
    fprintf(fp, "+");
    for (j = 0; j < grid_width; j++)
        fprintf(fp, "-");
    fprintf(fp, "+\n");

    /* Free allocated memory */
    free(grid);
}

/**
 * output_interval_summary - Output interval summary with robot/missile status and battlefield
 */
static void output_interval_summary(interval_data *interval, FILE *fp)
{
    int i;

    fprintf(fp, "CYCLE %ld | Interval %ld-%ld\n\n",
            interval->end_cycle, interval->start_cycle, interval->end_cycle);

    /* Output robot status */
    for (i = 0; i < interval->robot_count; i++) {
        fprintf(fp, "[%d] %-12s dmg=%d  spd=%3d  hdg=%3d  (%d,%d)\n",
                interval->robots[i].id,
                interval->robots[i].name,
                interval->robots[i].damage,
                interval->robots[i].speed,
                interval->robots[i].heading,
                interval->robots[i].x,
                interval->robots[i].y);
    }

    /* Output actions if any */
    if (interval->action_count > 0) {
        fprintf(fp, "\nActions: ");
        for (i = 0; i < interval->action_count; i++) {
            fprintf(fp, "[%d] %s(%d,%d)  ",
                    interval->actions[i].robot_id,
                    interval->actions[i].action,
                    interval->actions[i].param1,
                    interval->actions[i].param2);
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "\n");

    /* Draw ASCII battlefield */
    draw_battlefield(interval, fp);
    fprintf(fp, "\n");
}

/* Persistent line buffer for lookahead (necessary because we can't ungetc full lines) */
static char pending_line[256] = "";
static int has_pending = 0;

/**
 * get_next_line - Get next line from file or pending buffer
 * Returns line, or NULL on EOF
 */
static char *get_next_line(FILE *fp, char *buffer, size_t size)
{
    if (has_pending) {
        strncpy(buffer, pending_line, size - 1);
        buffer[size - 1] = '\0';
        has_pending = 0;
        return buffer;
    }
    return fgets(buffer, size, fp);
}

/**
 * put_back_line - Put a line back for next read
 */
static void put_back_line(const char *line)
{
    strncpy(pending_line, line, sizeof(pending_line) - 1);
    pending_line[sizeof(pending_line) - 1] = '\0';
    has_pending = 1;
}

/**
 * parse_interval - Parse an interval from file
 * @fp: Input file pointer
 * @interval: Output interval data structure
 * @returns: 1 on success, 0 on EOF, -1 on error
 */
static int parse_interval(FILE *fp, interval_data *interval)
{
    char line[256];

    memset(interval, 0, sizeof(*interval));

    /* Read lines until we get an INTERVAL line or EOF/match separator */
    while (get_next_line(fp, line, sizeof(line))) {
        /* Skip empty lines */
        if (line[0] == '\n' || line[0] == '\r')
            continue;

        /* End of file or match separator */
        if (strcmp(line, "---\n") == 0)
            return 0;  /* Match separator - end of match */

        /* Parse INTERVAL line */
        if (sscanf(line, "INTERVAL %ld %ld", &interval->start_cycle, &interval->end_cycle) == 2) {
            /* Now read state lines until next INTERVAL or --- */
            while (get_next_line(fp, line, sizeof(line))) {
                if (line[0] == '\n' || line[0] == '\r')
                    continue;

                /* Next interval or separator - put line back and return */
                if (strncmp(line, "INTERVAL", 8) == 0 || strcmp(line, "---\n") == 0) {
                    put_back_line(line);
                    return 1;
                }

                /* Parse ROBOT line: ROBOT <id> <name> <x> <y> <heading> <speed> <damage> */
                if (sscanf(line, "ROBOT %d %13s %d %d %d %d %d",
                          &interval->robots[interval->robot_count].id,
                          interval->robots[interval->robot_count].name,
                          &interval->robots[interval->robot_count].x,
                          &interval->robots[interval->robot_count].y,
                          &interval->robots[interval->robot_count].heading,
                          &interval->robots[interval->robot_count].speed,
                          &interval->robots[interval->robot_count].damage) == 7) {
                    interval->robot_count++;
                    continue;
                }

                /* Parse MISSILE line: MISSILE <id> <status> <x> <y> <heading> <range> <dist> */
                if (sscanf(line, "MISSILE %*d.%*d %15s %d %d %d %d %d",
                          interval->missiles[interval->missile_count].status,
                          &interval->missiles[interval->missile_count].x,
                          &interval->missiles[interval->missile_count].y,
                          &interval->missiles[interval->missile_count].heading,
                          &interval->missiles[interval->missile_count].range,
                          &interval->missiles[interval->missile_count].distance) == 6) {
                    interval->missile_count++;
                    continue;
                }

                /* Parse ACTION line: ACTION <robot_id> <cmd> <param1> <param2> */
                if (sscanf(line, "ACTION %d %15s %d %d",
                          &interval->actions[interval->action_count].robot_id,
                          interval->actions[interval->action_count].action,
                          &interval->actions[interval->action_count].param1,
                          &interval->actions[interval->action_count].param2) == 4) {
                    interval->action_count++;
                    continue;
                }
            }
            return 1;
        }
    }

    /* EOF reached without finding INTERVAL */
    return 0;
}

/**
 * skip_to_match - Skip file to reach target match number
 * @fp: File pointer
 * @target_match: Match number to find (0-indexed)
 * @returns: 0 on success, -1 if match not found
 */
static int skip_to_match(FILE *fp, int target_match)
{
    int current_match = 0;
    char line[256];

    /* Always skip header line first */
    if (!fgets(line, sizeof(line), fp))
        return -1;

    if (target_match == 0)
        return 0;  /* Already positioned for first match */

    /* Count match separators to find target match */
    while (fgets(line, sizeof(line), fp)) {
        if (strcmp(line, "---\n") == 0) {
            current_match++;
            if (current_match == target_match)
                return 0;
        }
    }

    return -1;  /* Match not found */
}

/**
 * process_match_to_file - Process a match and dump frames to file
 * @fp: Input file pointer
 * @outfp: Output file pointer
 */
static void process_match_to_file(FILE *fp, FILE *outfp)
{
    interval_data interval;

    fprintf(outfp, "Match playback\n");
    fprintf(outfp, "\n");

    while (parse_interval(fp, &interval) > 0) {
        output_interval_summary(&interval, outfp);
        fprintf(outfp, "\f");  /* Form feed between frames */
    }
}

/**
 * show_help - Display usage information
 */
static void show_help(const char *prog)
{
    printf("Usage: %s <filename> [OPTIONS]\n\n", prog);
    printf("Options:\n");
    printf("  -m NUM      Match number to replay (default: 0, first match)\n");
    printf("  -o FILE     Dump frames to file instead of ncurses playback\n");
    printf("  -g SIZE     Grid size for ASCII output (default: 128, must be power of 2)\n");
    printf("  -d MSEC     Delay between frames in ncurses mode (default: 100ms)\n");
    printf("  -h          Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s data.txt                    # Dump first match to stdout\n", prog);
    printf("  %s data.txt -m 2               # Dump match 2\n", prog);
    printf("  %s data.txt -o frames.txt      # Dump to file\n", prog);
    printf("  %s data.txt -m 0 -g 64         # Use 64x64 grid\n", prog);
}

/**
 * main - Main entry point
 */
int main(int argc, char *argv[])
{
    FILE *fp;
    int c;
    const char *filename = NULL;
    int i;

    /* Find the input filename (first non-option argument) */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            filename = argv[i];
            break;
        }
        /* Skip option argument */
        if (i + 1 < argc && (argv[i][1] != '\0' &&
            (strchr("mogd", argv[i][1]) != NULL))) {
            i++;
        }
    }

    /* Parse options - use custom parsing to allow options anywhere */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == '-') {
                /* Long options not supported */
                show_help(argv[0]);
                return 1;
            }
            /* Short option */
            if (argv[i][1] == '\0') {
                show_help(argv[0]);
                return 1;
            }

            switch (argv[i][1]) {
                case 'm':
                    if (i + 1 >= argc) {
                        errx(1, "Option -m requires an argument");
                    }
                    match_num = atoi(argv[++i]);
                    if (match_num < 0)
                        errx(1, "Match number must be non-negative");
                    break;
                case 'o':
                    if (i + 1 >= argc) {
                        errx(1, "Option -o requires an argument");
                    }
                    output_filename = argv[++i];
                    break;
                case 'g':
                    if (i + 1 >= argc) {
                        errx(1, "Option -g requires an argument");
                    }
                    grid_size = atoi(argv[++i]);
                    if (grid_size < MIN_GRID_SIZE || grid_size > MAX_GRID_SIZE)
                        errx(1, "Grid size must be between %d and %d", MIN_GRID_SIZE, MAX_GRID_SIZE);
                    /* Check if power of 2 */
                    if ((grid_size & (grid_size - 1)) != 0)
                        errx(1, "Grid size must be a power of 2");
                    break;
                case 'd':
                    if (i + 1 >= argc) {
                        errx(1, "Option -d requires an argument");
                    }
                    delay_ms = atoi(argv[++i]);
                    if (delay_ms < 0 || delay_ms > 10000)
                        errx(1, "Delay must be between 0 and 10000 milliseconds");
                    break;
                case 'h':
                    show_help(argv[0]);
                    return 0;
                default:
                    show_help(argv[0]);
                    return 1;
            }
        }
    }

    /* Verify we got the input filename */
    if (!filename) {
        show_help(argv[0]);
        return 1;
    }

    /* Open input file */
    fp = fopen(filename, "r");
    if (!fp)
        err(1, "Cannot open file: %s", filename);

    /* Skip header and match separators to reach target match */
    if (skip_to_match(fp, match_num) != 0)
        errx(1, "Match %d not found in file", match_num);

    /* Open output file if specified */
    if (output_filename) {
        output_file = fopen(output_filename, "w");
        if (!output_file)
            err(1, "Cannot open output file: %s", output_filename);
    } else {
        output_file = stdout;
    }

    /* Process match and output frames */
    process_match_to_file(fp, output_file);

    /* Cleanup */
    fclose(fp);
    if (output_filename)
        fclose(output_file);

    return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: nil
 *  c-file-style: "gnu"
 * End:
 */
