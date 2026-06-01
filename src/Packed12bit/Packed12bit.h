#pragma once
#include <cstdint>
#include <cstddef>

namespace Packed12bit {

// 2つの12-bit値を3byteにパックする
void packPair(uint16_t A, uint16_t B,
              uint8_t& byte0, uint8_t& byte1, uint8_t& byte2);

// 3byteを2つの12-bit値にアンパックする
void unpackPair(uint8_t byte0, uint8_t byte1, uint8_t byte2,
                uint16_t& A, uint16_t& B);

// 2×2の12-bit値テーブルをアンパックする（lineSpan: padding込みの1行byte数）
void unpackGrid2x2(const uint8_t* buf, std::size_t lineSpan,
                   uint16_t out[2][2]);

// 2×2のuint16_t値をバイリニア補間で5×5に拡大する
void bilinearInterp2x2to5x5(const uint16_t src[2][2], float dst[5][5]);

// 演習1〜5を実行する
void execution();

} // namespace Packed12bit
