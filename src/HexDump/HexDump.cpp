#include "HexDump/HexDump.h"

#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>

namespace HexDump {

void dumpBytes(const std::string& text, const std::string& label)
{
    std::cout << label << " (size = " << text.size() << ")\n";
    for (size_t i = 0; i < text.size(); ++i)
    {
        // char は signed になる処理系があるので unsigned char 経由で int 化する
        unsigned char byte = static_cast<unsigned char>(text[i]);
        std::cout
            << "[" << i << "] "
            << "0x"
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(byte)
            << std::dec
            << "\n";
    }
}

void dumpBytesOneLine(const std::string& text, const std::string& label)
{
    std::cout << label << " (size = " << text.size() << "): ";
    for (unsigned char byte : text)
    {
        std::cout
            << "0x"
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(byte)
            << " ";
    }
    std::cout << std::dec << "\n";
}

void compareBytes(const std::string& a, const std::string& b,
                  const std::string& labelA, const std::string& labelB)
{
    dumpBytesOneLine(a, labelA);
    dumpBytesOneLine(b, labelB);

    if (a == b)
        std::cout << "=> バイト列は一致しています\n";
    else
        std::cout << "=> バイト列は一致していません\n";
}

void execution()
{
    using json = nlohmann::json;

    std::cout << "=== HexDump デモ ===\n\n";

    // JSON 文字列からバイト列を検査する例
    json j = {
        {"name", "サンプル〜ファイルパス〜.txt"},
        {"ascii", "hello.txt"}
    };

    std::string name  = j.at("name").get<std::string>();
    std::string ascii = j.at("ascii").get<std::string>();

    std::cout << "-- dumpBytes --\n";
    dumpBytes(name, "name");

    std::cout << "\n-- dumpBytesOneLine --\n";
    dumpBytesOneLine(name, "name");
    dumpBytesOneLine(ascii, "ascii");

    // get<> と get_ref<> で取得した文字列のバイト列を比較する例
    std::cout << "\n-- compareBytes: get vs get_ref --\n";
    std::string        viaGet = j.at("name").get<std::string>();
    const std::string& viaRef = j.at("name").get_ref<const json::string_t&>();
    compareBytes(viaGet, viaRef, "viaGet", "viaRef");
}

} // namespace HexDump
