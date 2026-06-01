#include <iostream>
#include <memory>
#include <string>
#include "BioHazard/biohazard.h"
#include "CommandPatterFunction/CommandPatternFunction.h"
#include "HexDump/HexDump.h"
#include "CalibrationBin/CalibrationBin.h"
#include "Packed12bit/Packed12bit.h"

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::cout << "引数が渡されました。合計: " << argc - 1 << " 個" << std::endl;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            // C++では文字列の比較は if 文で行います
            if (arg == "biohazard") {
                BioHazard::execution();
            } else if (arg == "commandpattern") {
                CommandPatternFunction::execution();
            } else if (arg == "hexdump") {
                HexDump::execution();
            } else if (arg == "calibration") {
                CalibrationBin::execution();
            } else if (arg == "packed12bit") {
                Packed12bit::execution();
            } else {
                // デフォルトの処理
                BioHazard::execution();
            }
            
            std::cout << "引数[" << i << "]: " << arg << std::endl;
        }
    } else {
        std::cout << "引数は渡されませんでした。デフォルトの処理を開始します。" << std::endl;
        BioHazard::execution();
    }

    return 0;
}
