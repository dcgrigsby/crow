CROW: CRObots for World models
===============================
[![License Badge][]][License] [![GitHub Status][]][GitHub]

**CROW** is CROBOTS enhanced for **machine learning and world model training**. Generate deterministic, physics-accurate game state datasets from robot battle simulations.

[![C Robots Demo][]][Demo]

CROW combines the classic CROBOTS programming game with powerful snapshot export for training Transformer-based world models. Perfect for studying emergent behavior, deterministic physics simulation, and RL/planning algorithm development.

### Quick Start

Generate training data in seconds:

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

### Use Cases

- Training world models on deterministic game physics
- Studying emergent multi-agent combat behavior
- Generating synthetic training data for RL/planning algorithms
- Analyzing game state transitions and decision-making
- Benchmarking world model accuracy on known physics

**Performance**: Headless snapshot generation runs **~33x faster** than real-time display (100 matches in ~3 seconds).


About CROBOTS
-------------

CROW is based on **CROBOTS** ("see-robots"), the original C robot programming game from 1985.

Unlike arcade games that require real-time human input, CROBOTS emphasizes **strategy through programming**. You write a C program that controls a robot's behavior—vision, movement, targeting, timing. Your program runs autonomously to seek out, track, and destroy other robots. Each robot is equally equipped, and up to four robots can compete simultaneously.

CROBOTS is best played among programmers, each refining their own robot program and testing against others.

CROBOTS consists of a C compiler, a virtual computer, and battlefield
display (text graphics only, monochrome or color).  The CROBOTS compiler
accepts a limited (but useful) subset of the C language.  The C robot
programs are aided by hardware functions to scan for opponents, start
and stop drive mechanisms, fire cannons, etc.  After the programs are
compiled and loaded into separate robots, the battle is observed.
Robots moving, missiles flying and exploding, and certain status
information are displayed on the screen, in real-time.

CROBOTS started out as DOS shareware, but is, as of Oct 23 2013, free
software under terms of the GNU General Public License, version 2.

CROBOTS has been tested and runs on Linux (GLIBC & musl libc), FreeBSD,
DragonflyBSD, macOS, and OmniOS (Illumos/OpenSolaris).  For some reason
it does not work well on NetBSD, despite many hours of debugging by the
maintainer.  Patches are most welcome!


Intended audience
-----------------

CROBOTS will most likely appeal to programmers (especially those who
think they can write the "best" programs), computer game enthusiasts,
people wishing to learn the C language, and those who are interested in
compiler design and virtual computer interpreters.


User interface
--------------

CROBOTS does not use menus, windows, pop-ups, or any other user-friendly
interface.  Since the emphasis is on designing and writing robot control
programs, CROBOTS is started as a compiler might be started, from the
command line.

![C robots action screenshot](doc/crobots.png)


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

### Output Format

Example snapshot:
```
=== CYCLE 30 ===
BATTLEFIELD (1000x1000m):
+--------------------------------------------------+
|                                        1         |
|                            2                     |
|                                                  |
+--------------------------------------------------+

ROBOTS:
[1] counter.r  | Pos: ( 500, 750) | Head: 045 | Speed: 050 | Damage: 000% | ACTIVE
[2] jedi12.r   | Pos: ( 800, 250) | Head: 270 | Speed: 075 | Damage: 015% | ACTIVE

MISSILES:
[1.0] FLYING | Pos: ( 600, 700) | Head: 315 | Range: 0450 | Dist: 0200
```

Each snapshot contains complete game state in human-readable, easily tokenizable format.


Why CROW?
---------

**CROW** (CRObots for World models) extends CROBOTS with modern machine learning capabilities:

- **Deterministic physics**: Identical inputs produce identical outcomes—perfect for training predictive models
- **Complete state capture**: Every game state is recorded at regular intervals with full robot and missile dynamics
- **High-throughput generation**: Generate millions of training samples in minutes
- **Emergent behavior**: Study how simple robot control programs create complex multi-agent dynamics
- **Benchmark data**: Use CROW as a controlled test environment for world model architectures

CROW preserves the original CROBOTS game mechanics while adding the snapshot export feature for modern ML applications. The physics engine remains deterministic and true to the original 1985 implementation.


Origin & References
-------------------

CROW builds on **CROBOTS**, the original C robot programming game by [Tom Poindexter][] from 1985. The base CROBOTS project was first ported to Linux by Pablo Algar in 2018.

**CROW** adds machine learning capabilities (game state snapshots for world model training) while preserving the original game mechanics and physics. The implementation maintains backward compatibility—all original CROBOTS programs run identically, just with optional snapshot output.

CROW is maintained as part of the broader CROBOTS ecosystem. For the base CROBOTS project with community patches, see [troglobit/crobots](https://github.com/troglobit/crobots).

[C Robots Demo]:    https://asciinema.org/a/369639.svg
[Demo]:             https://asciinema.org/a/369639
[Tom Poindexter]:   https://github.com/tpoindex/
[License]:          https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
[License Badge]:    https://img.shields.io/badge/License-GPL%20v2-blue.svg
[GitHub]:           https://github.com/troglobit/crobots/actions/workflows/build.yml/
[GitHub Status]:    https://github.com/troglobit/crobots/actions/workflows/build.yml/badge.svg
