/* snapshot.h - game state snapshot output for ML training
 *
 * Copyright (C) 2025
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SNAPSHOT_H_
#define SNAPSHOT_H_

#include <stdio.h>

/**
 * init_snapshot - Initialize snapshot output
 * @fp: Open file pointer for snapshot output
 *
 * Call at start of each match to write header and initialize state.
 */
void init_snapshot(FILE *fp);

/**
 * output_snapshot - Write current game state to snapshot file
 * @cycle: Current CPU cycle number
 *
 * Outputs ASCII battlefield visualization and robot/missile data tables.
 * Should be called approximately every UPDATE_CYCLES (30 cycles).
 */
void output_snapshot(long cycle);

/**
 * close_snapshot - Finalize snapshot output
 *
 * Call at end of each match to flush and close snapshot output.
 * Writes footer separators between matches.
 */
void close_snapshot(void);

/**
 * reset_damage_tracker - Reset the damage event tracker
 *
 * Clears the damage tracker for the next snapshot period.
 */
void reset_damage_tracker(void);

#endif /* SNAPSHOT_H_ */

/**
 * Local Variables:
 *  indent-tabs-mode: nil
 *  c-file-style: "gnu"
 * End:
 */
