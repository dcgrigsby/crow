CROW: CRObots for World models
===============================
[![License Badge][]][License] [![GitHub Status][]][GitHub]

**CROW** is CROBOTS enhanced for **world model training**. Generate deterministic, physics-accurate game state datasets from robot battle simulations.

CROW combines the classic CROBOTS programming game with snapshot export.

### Quick Start

Generate training data:

```bash
# Headless: 100 matches of training data (~3 seconds)
./src/crobots -o training_data.txt -m 100 examples/counter.r examples/jedi12.r
```

### What You Get

Each snapshot contains complete game state:
- **ASCII battlefield visualization** (50×20 grid) showing game state
- **Structured robot data**: position, heading, speed, damage, status
- **Missile dynamics**: position, heading, range, distance, lifetime
- **Sampling rate**: Every 30 CPU cycles (~16,666 snapshots per 500k-cycle match)
- **Format**: Human-readable, easily tokenizable for Transformer training


Origin
------

**CROW** extends [CROBOTS](https://github.com/troglobit/crobots), the classic C robot programming game created by Tom Poindexter in 1985. CROBOTS is free software under the GNU General Public License v2. 


Snapshot Usage Guide
--------------------

### Command-line Options

- `-o FILE` - Output snapshots to file (ASCII battlefield + structured data every 30 cycles)
- `-m NUM` - Run multiple matches. Combine with `-o` for headless batch generation
- `-l NUM` - Limit cycles per match (default: 500,000)

### Usage Examples

**Headless batch generation (fastest):**
```bash
./src/crobots -o training.txt -m 100 examples/counter.r examples/jedi12.r
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
BATTLEFIELD (1000x1000m):
+---------+
|    1    |
|         |
|    2    |
|         |
|    *    |
+---------+

ROBOTS:
[1] counter          | Pos: ( 502, 491) | Head: 270 | Speed: 050 | Damage:   0% | ACTIVE
[2] jedi12           | Pos: ( 503, 512) | Head:  90 | Speed: 050 | Damage:   0% | ACTIVE

MISSILES:
[1.0] FLYING        | Pos: ( 510, 491) | Head: 270 | Range: 1000 | Dist: 0008

```

**Match separator:**
```
---
```

### Data Fields

**Battlefield:**
- 50×20 ASCII character grid representing the 1000×1000m battlefield
- Robot positions: numbered `1`-`4` (or blank if dead)
- Missile positions: marked with `*`
- Origin: top-left corner is (0, 1000m), bottom-right is (1000m, 0)

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

[License]:          https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
[License Badge]:    https://img.shields.io/badge/License-GPL%20v2-blue.svg
[GitHub]:           https://github.com/troglobit/crobots/actions/workflows/build.yml/
[GitHub Status]:    https://github.com/troglobit/crobots/actions/workflows/build.yml/badge.svg
