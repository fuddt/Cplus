#ifndef SAMPLE_SAMPLE_H
#define SAMPLE_SAMPLE_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class Sample {
public:
    // data 配列の先頭要素を返す
    std::string getPlace() const;
    // data 配列の先頭要素を返す
    int getDistance() const;
    std::string getName() const;

    // JSON全体を検証して、問題なければ内部の結果データを更新する
    bool validateJson(const nlohmann::json& j, std::string& error);

private:
    struct DataEntry {
        std::string place;
        int distance = 1800;
    };

    struct ResultData {
        std::string name;
        std::vector<DataEntry> data;
    };

    void loadJsonFile(const nlohmann::json& data, ResultData& out);

    // name はファイルパスとして使うため、文字を置換せず元の並びを保つ
    void rebuildName(std::string& name) const;
    bool requireKey(const nlohmann::json& j, const std::string& key, std::string& error) const;
    bool requireString(const nlohmann::json& j, const std::string& key, std::string& error) const;
    bool requireArray(const nlohmann::json& j, const std::string& key, std::string& error) const;
    bool requireNumberIfExists(const nlohmann::json& j, const std::string& key, std::string& error) const;
    bool validateDataItem(const nlohmann::json& item, DataEntry& out, std::string& error) const;

    std::string makeMissingKeyError(const std::string& key) const;
    std::string makeTypeMismatchError(const std::string& key, const std::string& expected) const;

    ResultData result_;
};

#endif // SAMPLE_SAMPLE_H
