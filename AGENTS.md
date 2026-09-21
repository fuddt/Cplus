# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

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

## Goal Management & Slide Guidelines

- `docs/goal_management/slides/` の構成・役割分担・ストーリー展開は `docs/goal_management/curriculum_design_specification.md`（教材設計仕様書：全8回基準）をマスターとする。
- 「1回につき主要なメンタルモデルを原則1つ作る（1回30〜45分目安）」を遵守し、過積載を防止する。
- 作成・更新時は、必ず `docs/goal_management/slide_design_guidelines.md` の教育資料作成ルール（Whyファースト、既習事項による新概念駆動、C++顕微鏡化、意味のノイズ排除、技術的負債ゼロ）を遵守する。
- スライド末尾に「次回予告」スライドは含めない（不要）。「本日のまとめ」で完結させる。
- 作業完了時は必ず最後に `git commit` および `git push origin main` を行う。
