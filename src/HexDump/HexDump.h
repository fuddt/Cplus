#ifndef HEXDUMP_HEXDUMP_H
#define HEXDUMP_HEXDUMP_H

#include <string>

namespace HexDump {

// 1バイトずつインデックス付きで16進数表示する
void dumpBytes(const std::string& text, const std::string& label);

// 全バイトを1行で16進数表示する
void dumpBytesOneLine(const std::string& text, const std::string& label);

// 2つの文字列のバイト列を並べて比較表示する
void compareBytes(const std::string& a, const std::string& b,
                  const std::string& labelA, const std::string& labelB);

// デモ実行
void execution();

} // namespace HexDump

#endif // HEXDUMP_HEXDUMP_H
