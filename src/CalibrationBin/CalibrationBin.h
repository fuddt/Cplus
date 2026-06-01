#ifndef CALIBRATIONBIN_CALIBRATIONBIN_H
#define CALIBRATIONBIN_CALIBRATIONBIN_H

#include <cstdint>  // uint32_t
#include <string>

namespace CalibrationBin {

// ================================================================
// キャリブレーションテーブル構造体
//
// binファイルとの対応:
//   このstructのメモリをそのままファイルに書き出したものが .bin
//   読むときも、binをそのままこのstructのメモリに流し込む
//
// メモリレイアウト（合計 24 bytes）:
//   offset  0 -  3 : width   (uint32_t, 4 bytes)
//   offset  4 -  7 : height  (uint32_t, 4 bytes)
//   offset  8 - 11 : scaleX  (float,    4 bytes)
//   offset 12 - 15 : scaleY  (float,    4 bytes)
//   offset 16 - 19 : offsetX (float,    4 bytes)
//   offset 20 - 23 : offsetY (float,    4 bytes)
// ================================================================
struct CalibrationTable
{
    uint32_t width;    // 画像の横幅（ピクセル）
    uint32_t height;   // 画像の縦幅（ピクセル）
    float    scaleX;   // X方向の倍率（座標変換で使う）
    float    scaleY;   // Y方向の倍率
    float    offsetX;  // X方向の平行移動量
    float    offsetY;  // Y方向の平行移動量
};

// ================================================================
// 2D座標
// ================================================================
struct Point
{
    float x;
    float y;
};

// binファイルを生成する（small: 1000x1000, 倍率 1.0）
void writeSmallBin(const std::string& path);

// binファイルを生成する（large: 3000x3000, 倍率 3.0）
void writeLargeBin(const std::string& path);

// binファイルを読み込む。成功なら true を返す
bool loadBin(const std::string& path, CalibrationTable& outCalib);

// CalibrationTableの内容をターミナルに出力する
void dumpCalib(const std::string& label, const CalibrationTable& calib);

// binの内容を16進ダンプ形式で表示する
void hexDump(const std::string& path);

// small.bin を読んで変換し large.bin を書き出す
void convertSmallToLarge(const std::string& srcPath, const std::string& dstPath);

// CalibrationTableのscale/offsetを使って座標を変換する
Point applyCalibration(const CalibrationTable& calib, Point p);

// デモ実行（main.cpp から呼ぶ）
void execution();

} // namespace CalibrationBin

#endif // CALIBRATIONBIN_CALIBRATIONBIN_H
