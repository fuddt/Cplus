//
// Created by shirotabi on 2026/04/20.
//

#include "CommandPatternFunction.h"
#include "CommandPattern/OperationManager.h"
#include <iostream>
#include <memory>

void CommandPatternFunction::execution() {

    // 両パターンで使い回す共通の操作リスト
    json operations = json::array({
        {
            {"type", "setJockey"},
            {"name", 14},
            {"age", 32},
            {"weight", 55.5},
            {"win_rate", 0.18},
            {"equipment", {{"whipType", "standard"}, {"blinkerType", "full"}}}
        },
        {{"type", "setCourse"}, {"place", "東京"}, {"distance", 2400}},
        {{"type", "setHorse"}, {"gateNumber", 7}, {"speedRating", 112}}
    });

    auto printContext = [](const Context& ctx) {
        std::cout << "  [Course]  place=" << ctx.cource.place
                  << "  distance=" << ctx.cource.distance << "\n";
        std::cout << "  [Jockey]  id=" << ctx.jockey.name
                  << "  age=" << ctx.jockey.age
                  << "  weight=" << ctx.jockey.weight
                  << "  win_rate=" << ctx.jockey.win_rate
                  << "  whip=" << ctx.jockey.equipment.whipType
                  << "  blinker=" << ctx.jockey.equipment.blinkerType << "\n";
        std::cout << "  [Horse]   gate=" << ctx.horse.gateNumber
                  << "  speed=" << ctx.horse.speedRating << "\n";
    };

    // ── 方式1: 外部組み立て方式 ──────────────────────────────────────────────
    // OperationManager は空で作り、ハンドラを外から登録する
    // → Manager は IOperationHandler という抽象だけを知ればよい
    std::cout << "=== 方式1: 外部組み立て方式 ===\n";
    std::cout << "OperationManager は空で作成し、ハンドラを外から登録する\n\n";

    OperationManager externalManager;   // loadDefaults=false（デフォルト）
    externalManager.registerHandler("setJockey", std::make_unique<SetJockeyHandler>());
    externalManager.registerHandler("setCourse", std::make_unique<SetCourseHandler>());
    externalManager.registerHandler("setHorse",  std::make_unique<SetHorseHandler>());

    Context ctx1{};
    externalManager.applyOperations(operations, ctx1);
    printContext(ctx1);

    // ── 方式2: ハイブリッド方式 ──────────────────────────────────────────────
    // OperationManager(true) で標準ハンドラを自動登録する
    // → 使う側は manager を作るだけでよい
    std::cout << "\n=== 方式2: ハイブリッド方式 ===\n";
    std::cout << "OperationManager(true) で標準ハンドラを自動登録する\n\n";

    OperationManager hybridManager(true);   // 標準ハンドラ3つを自動登録

    Context ctx2{};
    hybridManager.applyOperations(operations, ctx2);
    printContext(ctx2);
}
