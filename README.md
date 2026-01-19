CROW: CRObots for World models
===============================

**CROW** is CROBOTS enhanced for **world model training**. Generate deterministic, physics-accurate game state datasets from robot battle simulations.

CROW combines the classic CROBOTS programming game with snapshot export.

### Quick Start

Generate training data:

```bash
# Headless: 100 matches of training data
./src/crobots -o training_data.txt -m 100 examples/counter.r examples/jedi12.r

# Custom battlefield (512×512m) and grid size (64×64):
./src/crobots -b 512 -g 64 -o custom.txt -m 10 examples/counter.r examples/jedi12.r
```

### What You Get

Each snapshot contains complete game state:
- **ASCII battlefield visualization** (configurable grid, default 128×128) showing game state
- **Structured robot data**: position, heading, speed, damage, status
- **Missile dynamics**: position, heading, range, distance, lifetime
- **Sampling rate**: Configurable via `-u` option (default 30 CPU cycles, ~16,666 snapshots per 500k-cycle match)
- **Format**: Human-readable, easily tokenizable for Transformer training
- **Configurable dimensions**: Battlefield size (64-16384m) and snapshot grid (16-1024) for ML training consistency


Robot Programming Guide
-----------------------

### New Built-in Functions

CROW adds two new built-in functions for runtime access to battlefield configuration:

- `batsiz()` - Returns the configured battlefield size in meters (e.g., 1024 for a 1024×1024m arena)
- `canrng()` - Returns the maximum cannon range in meters (~70% of battlefield size)

These allow robot programs to dynamically adapt to configurable battlefield sizes without hardcoding assumptions:

```c
int range_limit = canrng();  // Get actual range for current battlefield
int arena_size = batsiz();   // Get actual battlefield size
int center = arena_size / 2; // Calculate center point dynamically
```

**Example:** The provided `counter.r`, `sniper.r`, `rook.r`, `jedi12.r`, and `ksnipper.r` robots use these functions to adapt to any configured battlefield size. To create robots for ML training with varying arena sizes, your robots can now automatically adjust their strategy.

For standard CROBOTS built-in functions (drive, cannon, scan, damage, speed, loc_x, loc_y, rand, math functions), see the original [CROBOTS documentation](https://github.com/troglobit/crobots).


Origin
------

**CROW** extends [CROBOTS](https://github.com/troglobit/crobots), the classic C robot programming game created by Tom Poindexter in 1985. CROBOTS is free software under the GNU General Public License v2.


Snapshot Usage Guide
--------------------

### Configuration Options

**Battlefield and Grid Dimensions:**
- `-b SIZE` - Battlefield size (SIZE×SIZE meters, must be power of 2, range 64-16384, default 1024)
- `-g SIZE` - Snapshot grid size (SIZE×SIZE, must be power of 2, range 16-1024, default 128)

**Robot Compilation:**
- `-k SIZE` - Max instruction limit per robot (range 256-8000, default 1000). Use for complex robots

**Logging Control:**
- `-a 0|1` - Enable/disable action logging (default 1). Logs robot drive, scan, and cannon actions
- `-r 0|1` - Enable/disable reward logging (default 1). Logs damage events for reward calculation
- `-x 0|1` - Enable/disable ASCII battlefield visualization (default 0)

**Game Control:**
- `-o FILE` - Output snapshots to file (structured game state data + optional ASCII visualization)
- `-u CYCLES` - Snapshot interval in CPU cycles (range 1-1000, default 30). Lower values = more snapshots
- `-m NUM` - Run multiple matches. Combine with `-o` for headless batch generation
- `-l NUM` - Limit cycles per match (default: 500,000)

### Usage Examples

**Headless batch generation with defaults (1024×1024m, 128×128 grid):**
```bash
./src/crobots -o training.txt -m 100 examples/counter.r examples/jedi12.r
```

**Small arena for rapid iteration (512×512m, 64×64 grid):**
```bash
./src/crobots -b 512 -g 64 -o small.txt -m 100 examples/counter.r examples/jedi12.r
```

**Large arena for detailed training data (2048×2048m, 256×256 grid):**
```bash
./src/crobots -b 2048 -g 256 -o large.txt -m 50 examples/counter.r examples/jedi12.r
```

**Single match with snapshots:**
```bash
./src/crobots -o single.txt -m 1 examples/counter.r examples/jedi12.r
```

**Watch battle live + record snapshots:**
```bash
./src/crobots -o battle.txt examples/counter.r examples/jedi12.r
```

**Control cycle limit:**
```bash
./src/crobots -o data.txt -m 10 -l 100000 examples/counter.r examples/jedi12.r
```

**Complex robots with increased instruction limit:**
```bash
./src/crobots -k 2000 -o complex.txt -m 50 examples/jedi12.r examples/counter.r
```

**Higher sampling rate for dense data (snapshot every 10 cycles instead of 30):**
```bash
./src/crobots -u 10 -o dense.txt -m 100 examples/counter.r examples/jedi12.r
```

**Lower sampling rate for faster processing (snapshot every 60 cycles):**
```bash
./src/crobots -u 60 -o sparse.txt -m 100 examples/counter.r examples/jedi12.r
```

Snapshot File Format
--------------------

Output files are text-based and human-readable. Each file contains one or more matches, with snapshots recorded at intervals configurable via `-u` (default every 30 CPU cycles). The crow-visualize utility can parse and display these snapshots with ASCII visualization.

### File Structure

**File header:**
```
CROBOTS SNAPSHOT LOG
```

**Per-cycle snapshot (plain text interval format):**
```
INTERVAL 30 60
ROBOT 1 counter.r 324 675 0 0 0
ROBOT 2 counter.r 877 723 0 0 0
ACTION 1 SCAN 115 1
ACTION 1 SCAN 116 1
ACTION 2 SCAN 242 1
ACTION 2 SCAN 243 1
ROBOT 1 counter.r 324 675 0 0 0
ROBOT 2 counter.r 877 723 0 0 0
```

**Match separator:**
```
---
```

### Data Fields

**INTERVAL line:**
```
INTERVAL start_cycle end_cycle
```
- `start_cycle` - Starting CPU cycle number for this snapshot interval
- `end_cycle` - Ending CPU cycle number for this snapshot interval

**ROBOT line (one per active robot in interval):**
```
ROBOT id name x y heading speed damage
```
- `id` - Robot ID (1-4)
- `name` - Robot program name
- `x`, `y` - Position in meters (0-1024 for default battlefield)
- `heading` - Direction in degrees (0-359)
- `speed` - Robot speed value (0-100)
- `damage` - Cumulative damage taken (0-100)

**MISSILE line (one per active missile in interval):**
```
MISSILE id.missile_num status x y heading range distance
```
- `id.missile_num` - Robot ID and missile index (e.g., "1.0" for robot 1, missile 0)
- `status` - FLYING or EXPLODING
- `x`, `y` - Current position in meters
- `heading` - Direction in degrees (0-359)
- `range` - Maximum range in meters
- `distance` - Distance traveled so far in meters

**ACTION line (one per action executed, when `-a 1` is enabled):**
```
ACTION robot_id command param1 param2
```
- `robot_id` - Which robot executed the action (1-4)
- `command` - DRIVE, SCAN, or CANNON
- `param1`, `param2` - Command-specific parameters (heading/speed for DRIVE, angle/resolution for SCAN, etc.)

### ASCII Battlefield Visualization Tool

The `crow-visualize` utility converts snapshot files to ASCII visualizations. The snapshot filename must be the first argument, then options can follow in any order.

**Modes:**
- **Static dump mode** (with `-o FILE`): Writes ASCII frames to a file
- **Interactive playback mode** (default): Plays frames interactively in ncurses with configurable frame delays

```bash
# Static dump: Write first match to stdout
./src/crow-visualize data.txt

# Static dump: Save second match to file (32×32 grid)
./src/crow-visualize data.txt -m 1 -o output.txt -g 32

# Interactive playback: Watch match 0 with 200ms frame delay
./src/crow-visualize data.txt -m 0 -d 200

# Static dump: Custom grid size
./src/crow-visualize data.txt -g 64 -o frames.txt
```

**Options:**
- `-m NUM` - Match number to display (0-indexed, default 0)
- `-o FILE` - Write static frames to file (enables dump mode instead of interactive playback)
- `-g SIZE` - Grid size for ASCII output (must be power of 2, range 32-256, default 128)
- `-d MSEC` - Frame delay in milliseconds for interactive playback (default 100ms, only used without `-o`)

**Visualization output format:**
```
CYCLE 60 | Interval 30-60

[1] counter.r    dmg=0  spd=  0  hdg=  0  (324,675)
[2] counter.r    dmg=0  spd=  0  hdg=  0  (877,723)

+----+
|  1 |
|    |
|  2 |
|  * |
+----+
```

**Visualization details:**
- Robot positions: numbered `1`-`4`
- Missile positions: marked with `*`
- Coordinate system: (0,0) at top-left, (battlefield_size, battlefield_size) at bottom-right
- Grid size configurable (must be power of 2, range 32-256, default 128)
- Interactive mode displays frames sequentially with configurable delays
- Static mode writes all frames to a text file for batch processing

### Parsing Considerations

- Lines with `---` separate matches
- Dead robots don't appear in subsequent INTERVAL snapshots
- Inactive missiles don't appear in state sections
- Action logging controlled via `-a 1|0` flag during generation
- All positions and distances use meters as units
- Format is easily tokenizable for ML pipelines

### Breaking Changes

**Default battlefield size increased from 1000m to 1024m:**
- Power-of-2 constraint for ML training consistency
- Use `-b 1024` to maintain new default
- Use `-b 512` or `-b 256` for smaller arenas
- Legacy 1000m size no longer supported (not power of 2)

**Default snapshot grid increased from 50×20 to 128×128:**
- Square grids for better ML model training
- Use `-g 128` for new default
- Use `-g 64` or `-g 32` for faster iteration
- Legacy 50×20 size no longer supported

Existing robot programs should work unchanged; these are only spatial configuration changes.
