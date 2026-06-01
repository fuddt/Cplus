# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
# Configure (from repo root)
cmake -S . -B build

# Build
cmake --build build

# Run
./build/Cplus
```

No external dependencies — standard library only.

## Architecture

This is an educational C++ project implementing a Resident Evil-style inventory/item system to teach OOP concepts.

**Class hierarchy:**
```
Item (abstract base — src/Item/)
├── Herb (src/Item/Herb/) — healing items, overrides use(player)
│   ├── GreenHerb — restores 30 HP
│   └── RedHerb  — restores 60 HP
└── Key (src/Item/Key/) — door unlock items

Player (src/Player.h/.cpp)
 └── owns std::vector<std::unique_ptr<Item>> inventory
```

**Key design decisions:**
- `Item::use(Player&)` is the polymorphic entry point — items are responsible for their own effect on the Player
- Player owns inventory via `unique_ptr`; items are moved into it via `addItem(std::unique_ptr<Item>)`
- `Condition` enum (`Fine` / `Caution` / `Danger`) is recalculated on every HP change, not stored separately
- `src/` is the include root — headers are included as `Item/Herb/Herb.h`, not by full path

## Project Context

`docs/chapters/` contains the learning curriculum (chapters 0–7) that explains the rationale behind each design choice. When making changes, these docs describe the intended teaching goals.

`phase1/` is legacy Windows console code and is not part of the CMake build.
