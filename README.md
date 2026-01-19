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

Output files are text-based and human-readable. Each file contains one or more matches, with snapshots recorded at intervals configurable via `-u` (default every 30 CPU cycles). Optional ASCII battlefield visualization can be enabled with `-x 1`.

### File Structure

**File header:**
```
CROBOTS GAME STATE SNAPSHOT LOG
================================
```

**Per-cycle snapshot (structured interval format):**
```
<INTERVAL cycle_start=0 cycle_end=30>

<INITIAL_STATE>
ROBOTS:
[1] counter | pos:(512,491) | heading:270 | speed:50 | damage:0
[2] jedi12 | pos:(513,512) | heading:90 | speed:50 | damage:0

MISSILES:
[1.0] FLYING | pos:(520,491) | heading:270 | range:1024 | dist:0

</INITIAL_STATE>

<ACTIONS>
[1] DRIVE(270,50) SCAN(90,0)
[2] CANNON(45,500)
</ACTIONS>

<EVENTS>
</EVENTS>

<FINAL_STATE>
ROBOTS:
[1] counter | pos:(513,491) | heading:270 | speed:50 | damage:0
[2] jedi12 | pos:(514,512) | heading:90 | speed:50 | damage:0

MISSILES:
[1.0] FLYING | pos:(525,491) | heading:270 | range:1024 | dist:5

</FINAL_STATE>

</INTERVAL>
```

**Optional ASCII battlefield visualization (when `-x 1` is enabled):**
```
BATTLEFIELD (1024x1024m, 128x128 grid):
+------------------------------------------+
|            1                             |
|                                          |
|            2                             |
|                                          |
|            *                             |
+------------------------------------------+
```

**Match separator:**
```
---
```

### Data Fields

**INITIAL_STATE / FINAL_STATE sections:**
Capture robot and missile state at the start and end of each snapshot interval.

**Robot fields:**
- `[N]` - Robot index (1-4)
- `name` - Robot program name
- `pos:(x,y)` - Position in meters
- `heading` - Direction in degrees (0-359)
- `speed` - Speed value (0-100)
- `damage` - Cumulative damage taken (0-100)

**Missile fields:**
- `[N.M]` - Robot index (N) and missile index (M)
- Status - FLYING or EXPLODING
- `pos:(x,y)` - Position in meters
- `heading` - Direction in degrees (0-359)
- `range` - Maximum range in meters
- `dist` - Distance traveled so far in meters

**ACTIONS section:**
Contains actions executed during the interval (when `-a 1` is enabled).
- `DRIVE(heading,speed)` - Robot movement command
- `SCAN(degree,resolution)` - Robot scanning command
- `CANNON(degree,distance)` - Robot firing command

**EVENTS section:**
Tracks game events during the interval (when `-r 1` is enabled).
- Damage events and other significant state changes

**ASCII Battlefield Visualization:**
- Enabled with `-x 1` flag
- Configurable grid size with `-g SIZE` parameter (default 128×128)
- Robot positions: numbered `1`-`4`
- Missile positions: marked with `*`
- Coordinate system: (0,0) at top-left, (battlefield_size, battlefield_size) at bottom-right

### Parsing Considerations

- `<INTERVAL>` tags mark cycle boundaries with `cycle_start` and `cycle_end` attributes
- Lines with `---` separate matches
- Dead robots don't appear in FINAL_STATE
- Inactive missiles don't appear in state sections
- Action and damage logging can be independently controlled via `-a` and `-r` flags
- All positions and distances use meters as units
- Human-readable format is easily tokenizable for ML pipelines

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
