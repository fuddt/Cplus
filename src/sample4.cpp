//
// Created by shirotabi on 2026/04/20.
//


#include <functional>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
using MyFunc = std::function<int(int)>;
class Sample {
private:
    std::unordered_map<std::string, std::function<int(int)>> functions;

    int sampleFunc1(int a) {
        return a;
    }

    int sampleFunc2(int a) {
        return a + 1;
    }

public:
    MyFunc createFunction(const std::string& name) {
        return functions[name];
    }

    void registFunction(std::string name, MyFunc function) {

        functions[name] = function;

    }

    void initializeFunctions() {
        registFunction("sample1", [this](int a) { return sampleFunc1(a); });
        registFunction("sample2", [this](int a) { return sampleFunc2(a); });

        MyFunc myFunc = createFunction("sample1");
    }
const uint64_t INVALID_FRAME =
    std::numeric_limits<uint64_t>::max();

const std::string INVALID_TEXT =
    "__INVALID_JSON_FIELD__";

const nlohmann::json INVALID_JSON_VALUE = nullptr;

};