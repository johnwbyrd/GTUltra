# AI Assistant Knowledge Base - GTUltra Project

## Overview

This document contains critical information for AI assistants working on the GTUltra project. **READ THIS FIRST** before making assumptions about tools, build processes, or project structure.

---

## Build System

### ❌ WRONG: Don't use `make` directly
The project uses llvm-mos toolchain which may not be installed locally.

### ✅ CORRECT: Use GitHub Actions via `gh act`
```bash
# Build the player C implementation
gh act -j build-and-test -W .github/workflows/player-ci.yml

# Run in a specific directory
cd src/player && gh act -j build-and-test
```

**Why?** The `gh act` tool runs GitHub Actions workflows locally in Docker containers that have all the required dependencies (llvm-mos-sdk, etc.) pre-installed.

**Documentation:** See [src/player/CI_TESTING.md](../src/player/CI_TESTING.md) for detailed usage.

---

## Project Structure

### Music Player Implementations

The project has **multiple player implementations** for different channel counts:

- `src/player.s` - Mono/stereo player (original assembly, 3-channel/single SID)
- `src/player3.s` - Same as player.s (3-channel variant name)
- `src/player9.s` - 9-channel player (3 SID chips)
- `src/player12.s` - 12-channel player (4 SID chips)
- `src/altplayer.s` - Alternative player variant

**The C implementation:**
- Located in `src/player/` directory
- Targets the 3-channel (single SID) implementation
- Named "player" (not "player3") - we renamed everything to remove the "3" suffix

### Directory Layout

```
GTUltra/
├── src/
│   ├── player/              # C implementation of music player
│   │   ├── include/         # Header files (player.h, player_types.h, sid.h)
│   │   ├── src/             # C source files (player.c, effects.c, etc.)
│   │   ├── test/            # Unit tests
│   │   └── Makefile         # Build file (use via gh act, not directly)
│   ├── player.s             # Assembly reference implementation
│   ├── player3.s            # Same as player.s (kept for compatibility)
│   ├── player9.s, player12.s, altplayer.s  # Other variants
│   ├── gt2reloc.c           # Relocator tool (converts .sng to playable format)
│   ├── greloc.c             # Relocator core engine
│   ├── gsong.c              # Song file I/O (loads/saves .sng files)
│   └── *.c                  # Editor source code
├── docs/
│   ├── PLAYER_C_DESIGN.md          # C implementation design
│   ├── PLAYER_IMPLEMENTATION_PLAN.md  # Phase-by-phase plan
│   ├── PLAYER_TESTING_PLAN.md      # Testing strategy
│   └── PLAYER_ANALYSIS.md          # Assembly architecture analysis
├── examples/                # Real songs for testing (6 subdirectories)
│   ├── Jammer/
│   ├── JasonPage/
│   ├── Linus/
│   ├── LMan/
│   ├── Mibri/
│   └── Shogoon/
└── .github/workflows/
    └── player-ci.yml        # CI workflow for player build
```

---

## Music Data Flow (CRITICAL - READ THIS!)

### How Music Gets From Composition to Player

**DON'T ask "how do I create test songs?" - here's the answer:**

```
1. COMPOSITION (Editor)
   GTUltra editor → User composes music → Save as .sng file

2. .SNG FILE FORMAT
   Binary file containing:
   - Song orderlists (which patterns play in sequence)
   - Patterns (note/effect/instrument data)
   - Instruments (ADSR, wavetable refs, etc.)
   - Wavetables, pulse tables, filter tables
   - Speed/tempo data

3. RELOCATOR (gt2reloc tool)
   .sng file → gt2reloc → Assembly output with:
   - Player code (player.s embedded)
   - Frequency tables (mt_freqtbllo/hi)
   - Song/pattern data (mt_songtbllo/hi, mt_patttbllo/hi)
   - Instrument data (mt_insad, mt_inssr, etc.)
   - All tables (wave, pulse, filter, speed)

4. COMPILATION
   Assembly → Assembler → Object file → Linker → PRG/BIN/SID

5. PLAYER CONSUMPTION
   MusicData struct points to compiled data tables
   Player reads via pointers (no runtime file loading)
```

### Key Insights About Test Data

**❌ WRONG:** "Let's create a simple test song"
- You can't easily create .sng files without the GTUltra editor
- The .sng format is complex binary data
- Hand-crafting it is extremely tedious

**✅ CORRECT Approaches:**

1. **Use existing example songs** - `examples/` folder has real .sng files
   - 24+ songs covering all player features
   - Already validated by the community
   - Just use the relocator to convert them

2. **Create minimal test data by hand** (for unit tests only)
   - See `src/player/test/test_data.h` - manually constructed tables
   - Good for testing specific features in isolation
   - NOT suitable for integration testing

3. **Use the relocator** - `gt2reloc` converts .sng to player data
   - Located in `src/gt2reloc.c` and `src/greloc.c`
   - This is how the editor exports music
   - Can be adapted to generate C test data

### MusicData Structure

**Where the data comes from:**
- MusicData is a struct full of `const` pointers
- Points to **compiled/linked data** (NOT runtime-loaded files)
- Data lives in ROM (flash memory on C64)
- Generated by relocator from .sng files
- All pointers set at compile/link time

**Structure of Arrays (SoA) pattern:**
- Instrument data stored as parallel arrays (cache-friendly on 6502)
- Example: `instr_ad[64]`, `instr_sr[64]`, etc.
- NOT: `Instrument[64]` with embedded fields

---

## Common Mistakes to Avoid

### 1. ❌ Trying to run `make` directly
**Problem:** llvm-mos-sdk is not installed locally
**Solution:** Use `gh act -j build-and-test`

### 2. ❌ Confusing player variants
**Problem:** Thinking "player3" is different from "player"
**Solution:** They're the same (3-channel). We use "player" in our C implementation.

### 3. ❌ Not reading existing documentation before asking questions
**Problem:** Information is already documented
**Solution:** Check these files first:
- `src/player/CI_TESTING.md` - Build and testing info
- `src/player/README.md` - Player overview
- `docs/PLAYER_*.md` - Design, implementation, testing plans
- **THIS FILE (AI.md)** - Critical knowledge about data flow

### 4. ❌ Assuming standard build tools
**Problem:** This is a C64 cross-compilation project
**Solution:** Everything goes through llvm-mos toolchain in Docker via `gh act`

### 5. ❌ Asking "how do I create test songs?"
**Problem:** You can't easily create .sng files without the editor
**Solution:** Use existing songs in `examples/` or hand-craft minimal test data

### 6. ❌ Thinking .sng files are loaded at runtime
**Problem:** The player doesn't load files from disk
**Solution:** Relocator converts .sng → assembly → compiled data tables

---

## Naming Conventions

### File Naming
- **Header files:** `player.h`, `player_types.h`, `sid.h` (lowercase, underscores)
- **Source files:** `player.c`, `effects.c`, `sequencer.c` (lowercase, underscores)
- **Documentation:** `PLAYER_C_DESIGN.md` (UPPERCASE for visibility)

### Code Naming
- **Types:** `PascalCase` (e.g., `Channel`, `Player`, `MusicData`)
- **Functions:** `snake_case` (e.g., `player_init`, `player_play`)
- **Constants:** `UPPER_CASE` (e.g., `NUM_CHANNELS`, `FX_PORTAUP`)
- **Variables:** `snake_case` (e.g., `song_num`, `frequency`)

### Include Guards
- Format: `MODULENAME_H` (e.g., `PLAYER_H`, `PLAYER_TYPES_H`, `PLAYER_SID_H`)
- ❌ NOT: `PLAYER3_H` (we removed all "3" references)

---

## Git Workflow

### Commits
- Keep commits focused on one change
- Use descriptive commit messages

### File Operations
- **Always use `git mv`** for renaming files to preserve history
- Don't delete and recreate - rename instead

---

## Testing Strategy

### Local Testing with gh act
```bash
# Test player build
cd src/player
gh act -j build-and-test

# View available workflows
gh act -l
```

### CI Pipeline
- GitHub Actions runs on every push
- Workflow: `.github/workflows/player-ci.yml`
- Tests: Unit tests in `src/player/test/`

### Test Data Sources
1. **Unit tests:** Hand-crafted minimal data in `test/test_data.h`
2. **Integration tests:** Real songs from `examples/` folder
3. **Comparison tests:** Use relocator output for both assembly and C

---

## Key Technical Details

### Target Platform
- **Hardware:** Commodore 64 (6502/6510 CPU)
- **Compiler:** llvm-mos (LLVM-based 6502 toolchain)
- **Sound Chip:** MOS 6581/8580 SID
- **Memory:** Limited (~64KB total, player must be small)

### SID Chip
- 3 voices per chip
- 25 registers ($D400-$D418)
- Memory-mapped I/O

### Player Features
- Pattern-based music sequencing
- 16 effects (portamento, vibrato, etc.)
- Wavetable synthesis
- Pulse width modulation
- Programmable filter
- Sound effects with priority

---

## Documentation Reading Order

When starting work on the player:

1. **First:** This file (AI.md) - Critical knowledge about data flow and testing
2. **Second:** `src/player/README.md` - High-level overview
3. **Third:** `docs/PLAYER_C_DESIGN.md` - Architecture and design
4. **Fourth:** `docs/PLAYER_IMPLEMENTATION_PLAN.md` - Current status and next steps
5. **As needed:** `docs/PLAYER_TESTING_PLAN.md` - Testing approach
6. **Reference:** `docs/PLAYER_ANALYSIS.md` - Assembly implementation details

---

## Key Files for Understanding Data Flow

When working with music data, read these files:

1. **`src/gsong.c`** - Loads/saves .sng files (binary format I/O)
2. **`src/greloc.c`** - Relocator core (converts .sng to assembly)
3. **`src/gt2reloc.c`** - Relocator CLI tool
4. **`src/player/include/player_types.h`** - MusicData structure definition
5. **`src/player/test/test_data.h`** - Example of hand-crafted test data

---

## Quick Reference Commands

```bash
# Build player (in src/player directory)
gh act -j build-and-test

# Check git status
git status

# View recent commits
git log --oneline -15

# Search for code
grep -r "pattern" src/player/

# View file
cat src/player/include/player.h
```

---

## Questions to Ask Before Acting

1. **Build/compile questions:** Have I checked `src/player/CI_TESTING.md`?
2. **Architecture questions:** Have I read `docs/PLAYER_C_DESIGN.md`?
3. **File naming questions:** Have I checked this knowledge base?
4. **Testing questions:** Have I read `docs/PLAYER_TESTING_PLAN.md`?
5. **"How do I build?"** → Use `gh act`, not `make` directly
6. **"How do I create test songs?"** → Use `examples/` or hand-craft test data
7. **"Where does MusicData come from?"** → Relocator converts .sng to compiled data

---

## Updates

**Last Updated:** 2025-11-25

**When to update this file:**
- When discovering new critical information
- When making project-wide naming/structure changes
- When build process changes
- When common mistakes are identified
- **When you catch yourself asking the same question twice**

---

**Remember:** When in doubt, read the documentation first. It's faster than guessing!
