#include "sample.h"

#include <utility>

using json = nlohmann::json;

std::string Sample::getPlace() const {
    if (result_.data.empty()) {
        return "";
    }

    return result_.data.front().place;
}

int Sample::getDistance() const {
    if (result_.data.empty()) {
        return 1800;
    }

    return result_.data.front().distance;
}

std::string Sample::getName() const {
    return result_.name;
}

std::string Sample::makeMissingKeyError(const std::string& key) const {
    return "必須キー '" + key + "' がありません";
}

std::string Sample::makeTypeMismatchError(const std::string& key, const std::string& expected) const {
    return "'" + key + "' の型が不一致です (" + expected + " が必要です)";
}

bool Sample::requireKey(const json& j, const std::string& key, std::string& error) const {
    if (!j.contains(key)) {
        error = makeMissingKeyError(key);
        return false;
    }
    return true;
}

bool Sample::requireString(const json& j, const std::string& key, std::string& error) const {
    if (!requireKey(j, key, error)) {
        return false;
    }

    if (!j.at(key).is_string()) {
        error = makeTypeMismatchError(key, "string");
        return false;
    }

    return true;
}

bool Sample::requireArray(const json& j, const std::string& key, std::string& error) const {
    if (!requireKey(j, key, error)) {
        return false;
    }

    if (!j.at(key).is_array()) {
        error = makeTypeMismatchError(key, "array");
        return false;
    }

    return true;
}

bool Sample::requireNumberIfExists(const json& j, const std::string& key, std::string& error) const {
    if (j.contains(key) && !j.at(key).is_number_integer()) {
        error = makeTypeMismatchError(key, "integer");
        return false;
    }

    return true;
}

void Sample::rebuildName(std::string& name) const {
    // ファイルパスとして使うので、波線を含めて文字は置換しない。
    // json から取り出した並びをそのまま使う。
    (void)name;
}

bool Sample::validateDataItem(const json& item, DataEntry& out, std::string& error) const {
    if (!item.is_object()) {
        error = "data の要素はオブジェクトである必要があります";
        return false;
    }

    if (!requireString(item, "place", error)) {
        return false;
    }

    if (!requireNumberIfExists(item, "distance", error)) {
        return false;
    }

    out.place = item.at("place").get<std::string>();
    out.distance = item.value("distance", 1800);
    return true;
}

void Sample::loadJsonFile(const json& data, ResultData& out) {
    out.name = data.at("name").get_ref<const json::string_t&>();
    rebuildName(out.name);

    for (const auto& item : data.at("data")) {
        DataEntry entry;
        entry.place = item.at("place").get<std::string>();
        entry.distance = item.value("distance", 1800);
        out.data.push_back(std::move(entry));
    }
}

bool Sample::validateJson(const json& j, std::string& error) {
    error.clear();

    if (!j.is_object()) {
        error = "JSON全体はオブジェクトである必要があります";
        return false;
    }

    if (!requireString(j, "name", error)) {
        return false;
    }

    if (!requireArray(j, "data", error)) {
        return false;
    }

    const auto& dataArray = j.at("data");
    if (dataArray.empty()) {
        error = "'data' は1要素以上必要です";
        return false;
    }

    for (std::size_t i = 0; i < dataArray.size(); ++i) {
        DataEntry entry;
        if (!validateDataItem(dataArray.at(i), entry, error)) {
            error = "data[" + std::to_string(i) + "]: " + error;
            return false;
        }
    }

    ResultData nextResult;
    loadJsonFile(j, nextResult);
    result_ = std::move(nextResult);

    return true;
}
