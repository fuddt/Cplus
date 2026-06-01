#include "Packed12bit.h"
#include <iostream>
#include <iomanip>

namespace Packed12bit {

void packPair(uint16_t A, uint16_t B,
              uint8_t& byte0, uint8_t& byte1, uint8_t& byte2) {
    byte0 = static_cast<uint8_t>(A >> 4);
    byte1 = static_cast<uint8_t>(((A & 0xF) << 4) | (B >> 8));
    byte2 = static_cast<uint8_t>(B & 0xFF);
}

void unpackPair(uint8_t byte0, uint8_t byte1, uint8_t byte2,
                uint16_t& A, uint16_t& B) {
    A = static_cast<uint16_t>((byte0 << 4) | (byte1 >> 4));
    B = static_cast<uint16_t>(((byte1 & 0xF) << 8) | byte2);
}

void unpackGrid2x2(const uint8_t* buf, std::size_t lineSpan,
                   uint16_t out[2][2]) {
    for (int row = 0; row < 2; row++) {
        const uint8_t* rowPtr = buf + row * lineSpan;
        unpackPair(rowPtr[0], rowPtr[1], rowPtr[2], out[row][0], out[row][1]);
    }
}

void bilinearInterp2x2to5x5(const uint16_t src[2][2], float dst[5][5]) {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            float u = static_cast<float>(i) / 4.0f;
            float v = static_cast<float>(j) / 4.0f;
            dst[i][j] = (1.0f - u) * (1.0f - v) * src[0][0]
                      + (1.0f - u) * v            * src[0][1]
                      + u          * (1.0f - v)   * src[1][0]
                      + u          * v             * src[1][1];
        }
    }
}

// -------------------------------------------------------

static void exercise1() {
    std::cout << "[演習1] 8-bit値の格納\n";
    uint8_t values[3] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        std::cout << "  values[" << i << "] = " << static_cast<int>(values[i]) << "\n";
    }
}

static void exercise2() {
    std::cout << "\n[演習2] 12-bit packed: A=1000, B=2000\n";
    uint16_t A = 1000, B = 2000;
    uint8_t byte0, byte1, byte2;
    packPair(A, B, byte0, byte1, byte2);
    std::cout << std::hex << std::uppercase;
    std::cout << "  byte0 = 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(byte0) << "\n";
    std::cout << "  byte1 = 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(byte1) << "\n";
    std::cout << "  byte2 = 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(byte2) << "\n";
    std::cout << std::dec << std::nouppercase << std::setfill(' ');
}

static void exercise3() {
    std::cout << "\n[演習3] アンパック: {0x3E, 0x87, 0xD0}\n";
    uint8_t byte0 = 0x3E, byte1 = 0x87, byte2 = 0xD0;
    uint16_t A, B;
    unpackPair(byte0, byte1, byte2, A, B);
    std::cout << "  A = " << A << "\n";
    std::cout << "  B = " << B << "\n";
}

static void exercise4() {
    std::cout << "\n[演習4] 2x2テーブルのアンパック\n";

    // 入力: 0=0, 1=100, 2=200, 3=300
    // packed: row0 = {A=0, B=100} → {0x00, 0x00, 0x64}
    //         row1 = {A=200, B=300} → {0x0C, 0x81, 0x2C}
    const std::size_t lineSpan = 3;
    uint8_t buf[2 * lineSpan] = {
        0x00, 0x00, 0x64,  // row0
        0x0C, 0x81, 0x2C   // row1
    };

    std::cout << "packed buffer (hex):\n";
    std::cout << std::hex << std::uppercase;
    for (int row = 0; row < 2; row++) {
        std::cout << "  row" << std::dec << row << ": ";
        for (std::size_t i = 0; i < lineSpan; i++) {
            std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(buf[row * lineSpan + i]) << " ";
        }
        std::cout << "\n";
    }
    std::cout << std::dec << std::nouppercase << std::setfill(' ');

    uint16_t grid[2][2];
    unpackGrid2x2(buf, lineSpan, grid);

    std::cout << "アンパック後:\n";
    for (int row = 0; row < 2; row++) {
        std::cout << "  ";
        for (int col = 0; col < 2; col++) {
            std::cout << "[" << row << "][" << col << "] = "
                      << std::setw(3) << grid[row][col] << "  ";
        }
        std::cout << "\n";
    }
}

static void exercise5() {
    std::cout << "\n[演習5] バイリニア補間 2x2 → 5x5\n";
    uint16_t src[2][2] = {{0, 100}, {200, 300}};
    float dst[5][5];
    bilinearInterp2x2to5x5(src, dst);

    std::cout << "入力 (2x2):\n";
    for (int i = 0; i < 2; i++) {
        std::cout << "  ";
        for (int j = 0; j < 2; j++) {
            std::cout << std::setw(5) << src[i][j];
        }
        std::cout << "\n";
    }

    std::cout << "補間後 (5x5):\n";
    std::cout << std::fixed << std::setprecision(1);
    for (int i = 0; i < 5; i++) {
        std::cout << "  ";
        for (int j = 0; j < 5; j++) {
            std::cout << std::setw(7) << dst[i][j];
        }
        std::cout << "\n";
    }
}

void execution() {
    std::cout << "=== 第8章：12-bit packed データ演習 ===\n\n";
    exercise1();
    exercise2();
    exercise3();
    exercise4();
    exercise5();
    std::cout << "\n=== 演習終了 ===\n";
}

} // namespace Packed12bit
