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

[License]:          https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
[License Badge]:    https://img.shields.io/badge/License-GPL%20v2-blue.svg
[GitHub]:           https://github.com/troglobit/crobots/actions/workflows/build.yml/
[GitHub Status]:    https://github.com/troglobit/crobots/actions/workflows/build.yml/badge.svg
