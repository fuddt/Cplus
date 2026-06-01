#include "CalibrationBin/CalibrationBin.h"

#include <fstream>
#include <iomanip>
#include <iostream>

namespace CalibrationBin {

// ================================================================
// small.bin を生成する
// 解像度: 1000x1000, 倍率: 1.0, オフセット: 0
// ================================================================
void writeSmallBin(const std::string& path)
{
    // structに値をセット
    CalibrationTable calib;
    calib.width   = 1000;
    calib.height  = 1000;
    calib.scaleX  = 1.0f;
    calib.scaleY  = 1.0f;
    calib.offsetX = 0.0f;
    calib.offsetY = 0.0f;

    // std::ios::binary が重要:
    //   テキストモードで開くと改行コード('\n')が変換されてバイト数がズレる
    std::ofstream ofs(path, std::ios::binary);

    if (!ofs)
    {
        std::cerr << "[ERROR] 書き出し失敗: " << path << "\n";
        return;
    }

    // structのメモリをそのままファイルに書き出す
    //
    //   reinterpret_cast<char*>(&calib)
    //     → structのアドレスを「バイト配列の先頭」として再解釈する
    //     → ofs.write() は char* しか受け取れないため必要
    //
    //   sizeof(CalibrationTable)
    //     → 何バイト書くか（= 24）
    ofs.write(
        reinterpret_cast<char*>(&calib),
        sizeof(CalibrationTable)
    );

    std::cout << "[OK] " << path << " を書き出しました"
              << " (" << sizeof(CalibrationTable) << " bytes)\n";
}

// ================================================================
// large.bin を直接生成する（比較検証用）
// 解像度: 3000x3000, 倍率: 3.0, オフセット: 0
// ================================================================
void writeLargeBin(const std::string& path)
{
    CalibrationTable calib;
    calib.width   = 3000;
    calib.height  = 3000;
    calib.scaleX  = 3.0f;
    calib.scaleY  = 3.0f;
    calib.offsetX = 0.0f;
    calib.offsetY = 0.0f;

    std::ofstream ofs(path, std::ios::binary);

    if (!ofs)
    {
        std::cerr << "[ERROR] 書き出し失敗: " << path << "\n";
        return;
    }

    ofs.write(
        reinterpret_cast<char*>(&calib),
        sizeof(CalibrationTable)
    );

    std::cout << "[OK] " << path << " を書き出しました"
              << " (" << sizeof(CalibrationTable) << " bytes)\n";
}

// ================================================================
// binファイルを読み込む
// 成功なら true、失敗なら false を返す
// ================================================================
bool loadBin(const std::string& path, CalibrationTable& outCalib)
{
    std::ifstream ifs(path, std::ios::binary);

    if (!ifs)
    {
        std::cerr << "[ERROR] 読み込み失敗: " << path << "\n";
        return false;
    }

    // ファイルのデータをstructのメモリに流し込む
    // write の逆: ファイル → struct
    ifs.read(
        reinterpret_cast<char*>(&outCalib),
        sizeof(CalibrationTable)
    );

    // 実際に読み込めたバイト数を確認する
    // ファイルが壊れている・別フォーマットの場合にここで気づける
    if (static_cast<std::size_t>(ifs.gcount()) != sizeof(CalibrationTable))
    {
        std::cerr << "[ERROR] サイズ不一致: "
                  << ifs.gcount() << " bytes 読み込み, 期待値 "
                  << sizeof(CalibrationTable) << " bytes\n";
        return false;
    }

    return true;
}

// ================================================================
// CalibrationTableの内容をターミナルに表示する
// ================================================================
void dumpCalib(const std::string& label, const CalibrationTable& calib)
{
    std::cout << "=== " << label << " ===\n";
    std::cout << "  width   : " << calib.width   << "\n";
    std::cout << "  height  : " << calib.height  << "\n";
    std::cout << "  scaleX  : " << calib.scaleX  << "\n";
    std::cout << "  scaleY  : " << calib.scaleY  << "\n";
    std::cout << "  offsetX : " << calib.offsetX << "\n";
    std::cout << "  offsetY : " << calib.offsetY << "\n";
}

// ================================================================
// binの内容をHxDライクな16進ダンプ形式で表示する
//
// 出力例:
//   offset | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
//   -------+---------------------------------------------------
//   000000 | E8 03 00 00 E8 03 00 00 00 00 80 3F 00 00 80 3F
//   000010 | 00 00 00 00 00 00 00 00
// ================================================================
void hexDump(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);

    if (!ifs)
    {
        std::cerr << "[ERROR] 読み込み失敗: " << path << "\n";
        return;
    }

    // ファイル全体をバイト列として読み込む
    std::string bytes(
        (std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>()
    );

    std::cout << "=== hexDump: " << path
              << " (" << bytes.size() << " bytes) ===\n";

    // ヘッダー行
    std::cout << "offset | ";
    for (int col = 0; col < 16; ++col)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << col << " ";
    }
    std::cout << "\n";
    std::cout << "-------+--------------------------------------------------\n";

    // バイト列を16バイトずつ表示
    for (std::size_t i = 0; i < bytes.size(); i += 16)
    {
        // オフセット列
        std::cout << std::hex << std::setw(6) << std::setfill('0') << i << " | ";

        // 16進数列
        for (std::size_t col = 0; col < 16; ++col)
        {
            if (i + col < bytes.size())
            {
                unsigned char byte = static_cast<unsigned char>(bytes[i + col]);
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(byte) << " ";
            }
            else
            {
                std::cout << "   ";  // 端数を空白で揃える
            }
        }

        std::cout << "\n";
    }

    // 元の出力フォーマットを戻す
    std::cout << std::dec;
}

// ================================================================
// small.bin を読んで large.bin へ変換して書き出す
//
// 変換ルール（仮: 実際の現場では仕様書に従うこと）:
//   width  *= 3
//   height *= 3
//   scaleX  = 3.0f
//   scaleY  = 3.0f
//   offsetX, offsetY は変換しない
// ================================================================
void convertSmallToLarge(const std::string& srcPath, const std::string& dstPath)
{
    // Step1: 変換元を読む
    CalibrationTable calib;

    if (!loadBin(srcPath, calib))
    {
        std::cerr << "[ERROR] 変換中止\n";
        return;
    }

    std::cout << "\n";
    dumpCalib("変換前: " + srcPath, calib);

    // Step2: 変換する
    // ここが「何を書き換えるか」の本質部分
    // 実際の現場では仕様書なしに触らないこと
    calib.width  = calib.width  * 3;   // 1000 → 3000
    calib.height = calib.height * 3;   // 1000 → 3000
    calib.scaleX = 3.0f;
    calib.scaleY = 3.0f;
    // offsetX, offsetY は変換しない（今回は 0 のまま）

    std::cout << "\n";
    dumpCalib("変換後: " + dstPath, calib);

    // Step3: 変換後を書き出す
    std::ofstream ofs(dstPath, std::ios::binary);

    if (!ofs)
    {
        std::cerr << "[ERROR] 書き出し失敗: " << dstPath << "\n";
        return;
    }

    ofs.write(
        reinterpret_cast<char*>(&calib),
        sizeof(CalibrationTable)
    );

    std::cout << "\n[OK] " << dstPath << " を書き出しました\n";
}

// ================================================================
// CalibrationTableのscale/offsetを使って座標を変換する
//
// 変換式（Affine変換の一部）:
//   result.x = p.x * scaleX + offsetX
//   result.y = p.y * scaleY + offsetY
// ================================================================
Point applyCalibration(const CalibrationTable& calib, Point p)
{
    Point result;

    result.x = p.x * calib.scaleX + calib.offsetX;
    result.y = p.y * calib.scaleY + calib.offsetY;

    return result;
}

// ================================================================
// デモ実行
// main.cpp の "calibration" 引数から呼ばれる
// ================================================================
void execution()
{
    std::cout << "=== CalibrationBin デモ ===\n\n";

    const std::string smallPath     = "small_calib.bin";
    const std::string largePath     = "large_calib.bin";
    const std::string convertedPath = "converted_calib.bin";

    // --------------------------------------------------
    // Step1: small.bin と large.bin を生成する
    // --------------------------------------------------
    std::cout << "--- Step1: binファイル生成 ---\n";
    writeSmallBin(smallPath);
    writeLargeBin(largePath);

    // --------------------------------------------------
    // Step2: 生成したファイルを読んで内容を確認する
    // --------------------------------------------------
    std::cout << "\n--- Step2: 読み込み確認 ---\n";
    {
        CalibrationTable smallCalib;
        CalibrationTable largeCalib;

        if (loadBin(smallPath, smallCalib))
            dumpCalib(smallPath, smallCalib);

        if (loadBin(largePath, largeCalib))
            dumpCalib(largePath, largeCalib);
    }

    // --------------------------------------------------
    // Step3: 16進ダンプ（HxDで見るのと同じ情報）
    // --------------------------------------------------
    std::cout << "\n--- Step3: 16進ダンプ ---\n";
    hexDump(smallPath);
    std::cout << "\n";
    hexDump(largePath);

    // --------------------------------------------------
    // Step4: small → converted 変換
    // --------------------------------------------------
    std::cout << "\n--- Step4: small → converted 変換 ---\n";
    convertSmallToLarge(smallPath, convertedPath);

    // --------------------------------------------------
    // Step5: 変換結果と直接生成した large を比較する
    // --------------------------------------------------
    std::cout << "\n--- Step5: converted vs large 比較 ---\n";
    {
        CalibrationTable convertedCalib;
        CalibrationTable largeCalib;

        const bool convertedOk = loadBin(convertedPath, convertedCalib);
        const bool largeOk     = loadBin(largePath, largeCalib);

        if (convertedOk && largeOk)
        {
            dumpCalib(convertedPath, convertedCalib);
            dumpCalib(largePath, largeCalib);

            // フィールドごとに一致を検証する
            const bool widthMatch   = (convertedCalib.width   == largeCalib.width);
            const bool heightMatch  = (convertedCalib.height  == largeCalib.height);
            const bool scaleXMatch  = (convertedCalib.scaleX  == largeCalib.scaleX);
            const bool scaleYMatch  = (convertedCalib.scaleY  == largeCalib.scaleY);
            const bool offsetXMatch = (convertedCalib.offsetX == largeCalib.offsetX);
            const bool offsetYMatch = (convertedCalib.offsetY == largeCalib.offsetY);

            std::cout << "\n  width  : " << (widthMatch   ? "OK" : "NG") << "\n";
            std::cout << "  height : " << (heightMatch  ? "OK" : "NG") << "\n";
            std::cout << "  scaleX : " << (scaleXMatch  ? "OK" : "NG") << "\n";
            std::cout << "  scaleY : " << (scaleYMatch  ? "OK" : "NG") << "\n";
            std::cout << "  offsetX: " << (offsetXMatch ? "OK" : "NG") << "\n";
            std::cout << "  offsetY: " << (offsetYMatch ? "OK" : "NG") << "\n";
        }
    }

    // --------------------------------------------------
    // Step6: Affine変換の例
    //   small座標系の点(100, 200) を large座標系へ変換する
    // --------------------------------------------------
    std::cout << "\n--- Step6: 座標変換 (small → large) ---\n";
    {
        CalibrationTable largeCalib;

        if (loadBin(largePath, largeCalib))
        {
            Point smallPoint;
            smallPoint.x = 100.0f;
            smallPoint.y = 200.0f;

            Point largePoint = applyCalibration(largeCalib, smallPoint);

            std::cout << "  small座標: (" << smallPoint.x << ", " << smallPoint.y << ")\n";
            std::cout << "  large座標: (" << largePoint.x << ", " << largePoint.y << ")\n";
            // 期待値: (300.0, 600.0)  ← scaleX=3.0, scaleY=3.0
        }
    }

    std::cout << "\n=== CalibrationBin デモ 完了 ===\n";
}

} // namespace CalibrationBin
