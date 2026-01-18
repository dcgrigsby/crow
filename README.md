CROW: CRObots for World models
===============================
[![License Badge][]][License] [![GitHub Status][]][GitHub]

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
- **Sampling rate**: Every 30 CPU cycles (~16,666 snapshots per 500k-cycle match)
- **Format**: Human-readable, easily tokenizable for Transformer training
- **Configurable dimensions**: Battlefield size (64-16384m) and snapshot grid (16-1024) for ML training consistency


Robot Programming Guide
-----------------------

### New Built-in Functions (CROW v2.1+)

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

**Game Control:**
- `-o FILE` - Output snapshots to file (ASCII battlefield + structured data every 30 cycles)
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

Snapshot File Format
--------------------

Output files are text-based and human-readable. Each file contains one or more matches, with snapshots recorded every 30 CPU cycles.

### File Structure

**File header:**
```
CROBOTS GAME STATE SNAPSHOT LOG
================================
```

**Per-cycle snapshot:**
```
=== CYCLE 30 ===
BATTLEFIELD (1024x1024m):
+------------------------------------------+
|            1                             |
|                                          |
|            2                             |
|                                          |
|            *                             |
+------------------------------------------+

ROBOTS:
[1] counter          | Pos: ( 512, 491) | Head: 270 | Speed: 050 | Damage:   0% | ACTIVE
[2] jedi12           | Pos: ( 513, 512) | Head:  90 | Speed: 050 | Damage:   0% | ACTIVE

MISSILES:
[1.0] FLYING        | Pos: ( 520, 491) | Head: 270 | Range: 1024 | Dist: 0008

```

**Match separator:**
```
---
```

### Data Fields

**Battlefield:**
- Configurable ASCII grid (default 128×128) representing the battlefield
- Grid size specified by `-g SIZE` parameter
- Robot positions: numbered `1`-`4` (or blank if dead)
- Missile positions: marked with `*`
- Origin: top-left corner is (0, battlefield_size), bottom-right is (battlefield_size, 0)
- Battlefield size in header reflects `-b SIZE` parameter (default 1024×1024m)

**Robot table fields:**
- `[N]` - Robot index (1-4)
- `Name` - Robot program name
- `Pos` - Position in meters (x, y)
- `Head` - Heading in degrees (0-359)
- `Speed` - Speed value (0-100)
- `Damage` - Damage percentage (0-100%)
- `Status` - ACTIVE or DEAD

**Missile table fields:**
- `[N.M]` - Robot index and missile index
- `Status` - FLYING or EXPLODING
- `Pos` - Position in meters (x, y)
- `Head` - Heading in degrees (0-359)
- `Range` - Maximum range in meters
- `Dist` - Current distance traveled in meters

### Parsing Considerations

- Lines starting with `===` mark cycle boundaries
- Lines with `---` separate matches
- Empty/dead robots don't appear in ROBOTS table
- Inactive missiles don't appear in MISSILES table
- All positions and distances use meters as units
- Human-readable format is easily tokenizable for ML pipelines

### Breaking Changes (v2.0)

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

[License]:          https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
[License Badge]:    https://img.shields.io/badge/License-GPL%20v2-blue.svg
[GitHub]:           https://github.com/troglobit/crobots/actions/workflows/build.yml/
[GitHub Status]:    https://github.com/troglobit/crobots/actions/workflows/build.yml/badge.svg
