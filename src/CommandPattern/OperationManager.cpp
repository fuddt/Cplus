//
// Created by shirotabi on 2026/04/19.
//
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "OperationManager.h"

// ── Constructor ──────────────────────────────────────────────────────────────
// loadDefaults=true のとき標準ハンドラを自動登録する（ハイブリッド方式）
// loadDefaults=false（デフォルト）のときは空のまま（外部組み立て方式）
OperationManager::OperationManager(bool loadDefaults) {
    if (loadDefaults) {
        registerHandler("setJockey", std::make_unique<SetJockeyHandler>());
        registerHandler("setCourse", std::make_unique<SetCourseHandler>());
        registerHandler("setHorse",  std::make_unique<SetHorseHandler>());
    }
}

// ── registerHandler ──────────────────────────────────────────────────────────
void OperationManager::registerHandler(const std::string& name,
                                        std::unique_ptr<IOperationHandler> handler) {
    handlers[name] = std::move(handler);
}

// ── applyOperations ──────────────────────────────────────────────────────────
// operations: JSON配列 [ {"type": "setJockey", ...}, {"type": "setCourse", ...}, ... ]
// 各要素の "type" でハンドラを引き、execute() に委譲する
void OperationManager::applyOperations(const json& operations, Context& context) {
    for (const auto& op : operations) {
        const std::string type = op.at("type").get<std::string>();
        auto it = handlers.find(type);
        if (it == handlers.end()) {
            throw std::runtime_error("unknown operation type: " + type);
        }
        it->second->execute(op, context);
    }
}

// ── SetJockeyHandler ─────────────────────────────────────────────────────────
void SetJockeyHandler::execute(const json& params, Context& context) {
    context.jockey.name     = params.at("name").get<int>();       // Jockey::name は int（騎手ID）
    context.jockey.age      = params.at("age").get<int>();
    context.jockey.weight   = params.at("weight").get<float>();
    context.jockey.win_rate = params.at("win_rate").get<float>();
    context.jockey.equipment.whipType    = params.at("equipment").at("whipType").get<std::string>();
    context.jockey.equipment.blinkerType = params.at("equipment").at("blinkerType").get<std::string>();
}

// ── SetCourseHandler ─────────────────────────────────────────────────────────
void SetCourseHandler::execute(const json& params, Context& context) {
    context.cource.place    = params.at("place").get<std::string>(); // "cource" は AppContext.h の綴り
    context.cource.distance = params.at("distance").get<int>();
}

// ── SetHorseHandler ──────────────────────────────────────────────────────────
void SetHorseHandler::execute(const json& params, Context& context) {
    context.horse.gateNumber  = params.at("gateNumber").get<int>();
    context.horse.speedRating = params.at("speedRating").get<int>();
}
