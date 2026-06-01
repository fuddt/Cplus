//
// Created by shirotabi on 2026/04/19.
//

#include <nlohmann/json.hpp>
#include "AppContext.h"
using json = nlohmann::json;

#ifndef CPLUS_OPERATIONMANAGER_H
#define CPLUS_OPERATIONMANAGER_H

class IOperationHandler {
public:
    virtual ~IOperationHandler() = default;
    //jsonのパラメータを受け取って、その情報を解析。適宜、contextに反映させる
    virtual void execute(const json& params, Context& context) = 0;
};

class SetJockeyHandler : public IOperationHandler {
    public:
        void execute(const json& params, Context& context) override;
};

class SetCourseHandler : public IOperationHandler {
    public:
    void execute(const json& params, Context& context) override;
};

class SetHorseHandler : public IOperationHandler {
    public:
    void execute(const json& params, Context& context) override;
};



class OperationManager {
    // "type文字列"と"それを処理するHandler"の連想配列
    private:
        std::unordered_map<std::string, std::unique_ptr<IOperationHandler>> handlers;
    public:
        explicit OperationManager(bool loadDefaults = false);
        void registerHandler(const std::string& name, std::unique_ptr<IOperationHandler> handler);
        void applyOperations(const json& operations, Context& context);
};


#endif //CPLUS_OPERATIONMANAGER_H