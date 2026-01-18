The Original C Robots Programming Game
======================================
[![License Badge][]][License] [![GitHub Status][]][GitHub]

CROBOTS ("see-robots") is a game based on computer programming, now enhanced with **machine learning training data generation**.

[![C Robots Demo][]][Demo]


ML Training & World Models
---------------------------

This fork of CROBOTS adds powerful **game state snapshot export** for training Transformer-based world models. Generate deterministic, physics-accurate training data from robot battles:

```bash
# Generate 100 matches of training data (complete with physics states)
./src/crobots -o training_data.txt -m 100 examples/counter.r examples/jedi12.r
```

Each snapshot contains:
- **ASCII battlefield visualization** (50×20 grid) showing game state
- **Structured robot data**: position, heading, speed, damage, status
- **Missile dynamics**: position, heading, range, distance, lifetime
- **Sampling rate**: Every 30 CPU cycles (~16,666 snapshots per 500k-cycle match)

Perfect for:
- Training world models on deterministic game physics
- Studying emergent combat behavior
- Generating synthetic training data for RL/planning algorithms
- Analyzing game state transitions and robot decision-making

**Performance**: Headless snapshot generation runs **~33x faster** than real-time display (100 matches in ~3 seconds).


Classic CROBOTS Game
--------------------

Unlike arcade type games which require human inputs controlling some
object, all strategy in CROBOTS must be complete before the actual game
begins.  Game strategy is condensed into a C language program that you
design and write.  Your program controls a robot whose mission is to
seek out, track, and destroy other robots, each running different
programs.  Each robot is equally equipped, and up to four robots may
compete at once.  CROBOTS is best played among several people, each
refining their own robot program, then matching program against program.

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


Origin & References
-------------------

This is the original CROBOTS game, by [Tom Poindexter][] from 1985.  It
was first ported to Linux by Pablo Algar in 2018.  Please don't bother
Tom with email or patches, he will probably **not** be updating CROBOTS
any more.

Instead, send patches or GitHub pull requests to this project.  It is
the logical continuation of Tom's project and has the explicit goal of
gathering (sane) patches from all forks out there.  Some of them work
just as well, but this one has the explicit goal of retaining the true
spirit of the original, with the same limits (e.g., MAX 1000 CPU
instructions) and behavior.

[C Robots Demo]:    https://asciinema.org/a/369639.svg
[Demo]:             https://asciinema.org/a/369639
[Tom Poindexter]:   https://github.com/tpoindex/
[License]:          https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
[License Badge]:    https://img.shields.io/badge/License-GPL%20v2-blue.svg
[GitHub]:           https://github.com/troglobit/crobots/actions/workflows/build.yml/
[GitHub Status]:    https://github.com/troglobit/crobots/actions/workflows/build.yml/badge.svg
