// ConsoleApplication1.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <iostream>
#include <string>



// これはただの型の別名
typedef int MYINT;
using ItemName = std::string;

// マクロ定義
// APP_NAME という名前を文字列に置き換える
#define APP_NAME "InventoryApp"

// DEBUG_MODE が定義されているかを試すためのマクロ
#define DEBUG_MODE



int main()
{

    MYINT hp = 100;
    ItemName herbName = "Green Herb";
    std::cout << hp << herbName << std::endl;
    std::cout << "app = " << APP_NAME << '\n';

#ifdef DEBUG_MODE
    // DEBUG_MODE が定義されている場合だけ、この行がコンパイル対象になる
    std::cout << "DEBUG_MODE is defined" << '\n';
#else
    // DEBUG_MODE が未定義なら、こちらが有効になる
    std::cout << "DEBUG_MODE is NOT defined" << '\n';
#endif

#ifndef RELEASE_MODE
    // RELEASE_MODE が未定義なら、この行が有効になる
    std::cout << "RELEASE_MODE is not defined" << '\n';
#endif

    return 0;


    /// 100Green Herbapp = InventoryApp
    /// DEBUG_MODE is defined
    /// RELEASE_MODE is not defined
}