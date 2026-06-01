# Phase 1 実行手順書: Windows方言の基礎

## 0. この文書の位置づけ

この文書は、`Windowsデスクトップアプリ開発 学習カリキュラム` の **Phase 1: Windows方言の基礎** を実行するための詳細手順書である。  
学習の順序、サンプルコード、確認項目、完了条件をここに固定する。

本Phaseの目的は、Windowsデスクトップアプリのコードに頻出する **型名・マクロ・文字列記法** を見ても、完全に未知の記号列に見えない状態になることだ。

---

## 1. このPhaseでやること

このPhaseでは、次の4つだけに集中する。

1. `typedef` と `using` の違いと役割を理解する
2. `#define`, `#ifdef`, `#ifndef` の役割を理解する
3. Windows特有の型名を「何の分類か」で読めるようにする
4. `UNICODE`, `TEXT()`, `L"文字列"` の関係を理解する

ここではまだGUIは作らない。  
ボタンもダイアログも後でいい。まずは **Windowsコードに出てくる単語の意味をほどく**。

---

## 2. このPhaseのゴール

このPhase完了時点で、最低限次を言えることを目指す。

- `DWORD` は Windows流儀の型別名
- `HWND` はウィンドウを識別するためのハンドル型
- `WPARAM` と `LPARAM` はメッセージ追加情報
- `LRESULT` はメッセージ処理関数の戻り値型
- `VOID` は `void` のこと
- `#define` はコンパイル前の置換
- `#ifdef` は「定義されていたらこのコードを使う」という条件分岐
- `UNICODE` により、Windows API がワイド文字版へ切り替わることがある

重要なのは、厳密な内部実装を全部覚えることではなく、**見た瞬間に分類できること** だ。

---

## 3. 事前準備

## 3.1 Visual Studio 2022 の確認

Visual Studio Installer で以下が入っていることを確認する。

- C++ によるデスクトップ開発
- Windows 10 または Windows 11 SDK
- MSVC v143 ツールセット

## 3.2 作業用フォルダ

このPhaseでは、次のようなフォルダ構成を推奨する。

```text
windows-desktop-cpp-study/
  phase1/
    01_alias_macro/
    02_windows_types/
    03_unicode_macro/
    notes/
```

1テーマ1プロジェクトに分ける。  
理由は、後から見返したときに「どのコードが何の練習だったか」が崩れにくいから。

---

## 4. 学習観点

Windowsコードを読むときは、出てきた記号を次の4分類で見る。

| 分類 | 例 | どう見るか |
|---|---|---|
| 型 | `DWORD`, `UINT`, `BOOL` | 値を入れるための型名 |
| ハンドル | `HWND`, `HINSTANCE`, `HMENU` | OS側の対象を指す識別子 |
| メッセージ関連 | `WPARAM`, `LPARAM`, `LRESULT` | Windowsメッセージ処理に使う情報 |
| マクロ | `#define`, `UNICODE`, `TEXT()` | コンパイル前に効く仕組み |

この4分類を意識するだけで、案件コードの読みやすさがかなり変わる。

---

## 5. Step 1: `typedef` と `using` に慣れる

## 5.1 目的

Windowsコードは古い `typedef` が大量に出る。  
まずは「これは特別な魔法ではなく、型の別名だ」と理解する。

## 5.2 Visual Studio 2022 での操作

1. Visual Studio 2022 を起動する
2. **新しいプロジェクトの作成** を押す
3. **コンソール アプリ** を選ぶ
4. プロジェクト名を `phase1_alias_macro` にする
5. 作成する
6. `main.cpp` を開いて、既存コードをすべて消す
7. 下のコードに置き換える

## 5.3 サンプルコード

```cpp
#include <iostream>
#include <string>

// 旧来の書き方: typedef
// int 型に MYINT という別名をつける
typedef int MYINT;

// 現代的な書き方: using
// std::string 型に ItemName という別名をつける
using ItemName = std::string;

int main()
{
    // MYINT は int の別名として使える
    MYINT hp = 100;

    // ItemName は std::string の別名として使える
    ItemName herbName = "Green Herb";

    std::cout << "hp = " << hp << '\n';
    std::cout << "item = " << herbName << '\n';

    return 0;
}
```

## 5.4 このコードで理解すべきこと

- `MYINT` は新しい型ではない。`int` の別名
- `ItemName` も同じく `std::string` の別名
- Windowsコードの `DWORD`, `UINT`, `BOOL` も、まずはこの延長で考えてよい

## 5.5 確認ポイント

- `typedef int MYINT;` を `typedef double MYINT;` に変えると、`hp` の型も変わるか
- `using ItemName = std::string;` を `using ItemName = const char*;` に変えると何が変わるか

---

## 6. Step 2: `#define`, `#ifdef`, `#ifndef` に慣れる

## 6.1 目的

案件コードで `#ifdef UNICODE` や `#ifndef` が出てきても、処理の流れを止めないようにする。

## 6.2 サンプルコード

`main.cpp` を次の内容に置き換える。

```cpp
#include <iostream>

// マクロ定義
// APP_NAME という名前を文字列に置き換える
#define APP_NAME "InventoryApp"

// DEBUG_MODE が定義されているかを試すためのマクロ
#define DEBUG_MODE

int main()
{
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
}
```

## 6.3 見るべきポイント

- `#define` は実行時ではなく、**コンパイル前** に効く
- `#ifdef` は「その名前が定義されているか」で分岐する
- `#ifndef` は「その名前が定義されていないか」で分岐する
- 普通の `if` 文とは別物

## 6.4 試すこと

1. `#define DEBUG_MODE` をコメントアウトする
2. 実行結果がどう変わるか確認する
3. `#define RELEASE_MODE` を追加して、`#ifndef RELEASE_MODE` の出力が消えることを確認する

## 6.5 この時点での理解

Windows案件で `#ifdef UNICODE` を見たら、

- これは実行時条件ではない
- コンパイル時に片方のコードしか残らない

と判断できれば十分。

---

## 7. Step 3: Windows型を実際に見てみる

## 7.1 目的

`windows.h` を実際にインクルードして、よく出る型名が「存在している」「値として扱える」ことを確認する。

## 7.2 サンプルコード

新しいコンソールアプリを `phase1_windows_types` という名前で作り、`main.cpp` を次に置き換える。

```cpp
#include <windows.h>
#include <iostream>
#include <type_traits>

int main()
{
    // Windows流儀の基本型
    DWORD flags = 123;
    UINT count = 10;
    BOOL success = TRUE;
    BYTE smallValue = 255;
    WORD shortValue = 65535;

    // ハンドル型
    // 今は本物のウィンドウを指していないので nullptr を入れている
    HWND hwnd = nullptr;
    HINSTANCE hInstance = nullptr;
    HMENU hMenu = nullptr;

    // メッセージ関連型
    WPARAM wParam = 0;
    LPARAM lParam = 0;
    LRESULT result = 0;

    std::cout << "DWORD flags = " << flags << '\n';
    std::cout << "UINT count = " << count << '\n';
    std::cout << "BOOL success = " << success << '\n';
    std::cout << "BYTE smallValue = " << static_cast<int>(smallValue) << '\n';
    std::cout << "WORD shortValue = " << shortValue << '\n';

    std::cout << "hwnd      = " << hwnd << '\n';
    std::cout << "hInstance = " << hInstance << '\n';
    std::cout << "hMenu     = " << hMenu << '\n';

    std::cout << "wParam = " << wParam << '\n';
    std::cout << "lParam = " << lParam << '\n';
    std::cout << "result = " << result << '\n';

    return 0;
}
```

## 7.3 ここで覚えるべき分類

### 値型

- `DWORD`
- `UINT`
- `BOOL`
- `BYTE`
- `WORD`

### ハンドル型

- `HWND`
- `HINSTANCE`
- `HMENU`

### メッセージ処理用

- `WPARAM`
- `LPARAM`
- `LRESULT`

## 7.4 覚え方

- `HWND` を見たら「ウィンドウへの識別子」
- `HINSTANCE` を見たら「アプリやモジュール側の識別子」
- `WPARAM`, `LPARAM` を見たら「メッセージに付いてくる追加情報」
- `LRESULT` を見たら「メッセージ処理結果を返す型」

この段階では、ビット幅や正確な内部定義を丸暗記する必要はない。

---

## 8. Step 4: `UNICODE`, `TEXT()`, `L"文字列"` を理解する

## 8.1 目的

Windowsコードに多い文字列の書き方に慣れる。  
これを知らないと、`MessageBox` 周りや文字列APIで止まりやすい。

## 8.2 前提

Windows API には ANSI 版とワイド文字版がある。  
現代のWindowsコードでは、基本的にワイド文字版を前提にしてよい。

## 8.3 サンプルコード

新しいコンソールアプリを `phase1_unicode_macro` という名前で作り、`main.cpp` を次に置き換える。

```cpp
#include <windows.h>
#include <iostream>

int main()
{
    // 通常の narrow 文字列
    const char* narrowText = "Hello";

    // ワイド文字列（L を付ける）
    const wchar_t* wideText = L"Hello";

    // TEXT マクロは、UNICODE 設定に応じて "Hello" または L"Hello" に展開される
    LPCTSTR genericText = TEXT("Hello");

    std::cout << "narrowText = " << narrowText << '\n';
    std::wcout << L"wideText   = " << wideText << L'\n';
    std::wcout << L"genericText = " << genericText << L'\n';

#ifdef UNICODE
    std::wcout << L"UNICODE is defined" << L'\n';
#else
    std::cout << "UNICODE is NOT defined" << '\n';
#endif

    return 0;
}
```

## 8.4 このコードで見ること

- `"Hello"` は通常文字列
- `L"Hello"` はワイド文字列
- `TEXT("Hello")` は設定に応じて切り替わる書き方
- `UNICODE` が定義されていると、Windows API はワイド文字寄りになることが多い

## 8.5 実務上の見方

案件コードで次を見たら、こう読む。

| 記法 | 読み方 |
|---|---|
| `L"abc"` | ワイド文字列 |
| `TEXT("abc")` | UNICODE対応のためのマクロ文字列 |
| `MessageBoxW` | ワイド文字版API |
| `MessageBoxA` | ANSI版API |

---

## 9. Step 5: `VOID`, `TRUE`, `FALSE`, `NULL`, `nullptr` を整理する

## 9.1 目的

古めのWindowsコードと現代C++の差分で混乱しないようにする。

## 9.2 整理表

| 記号 | 位置づけ | 今の見方 |
|---|---|---|
| `VOID` | `void` のWindows流儀表記 | `void` と読んでよい |
| `TRUE` | Windows流儀の真 | `true` 相当だが型は別文化 |
| `FALSE` | Windows流儀の偽 | `false` 相当だが型は別文化 |
| `NULL` | 古いヌル表現 | 今は `nullptr` を優先 |
| `nullptr` | 現代C++のヌルポインタ | 基本これを使う |

## 9.3 比較コード

```cpp
#include <windows.h>
#include <iostream>

int main()
{
    BOOL oldStyleFlag = TRUE;
    void* oldNull = NULL;
    void* modernNull = nullptr;

    std::cout << "oldStyleFlag = " << oldStyleFlag << '\n';
    std::cout << "oldNull      = " << oldNull << '\n';
    std::cout << "modernNull   = " << modernNull << '\n';

    return 0;
}
```

## 9.4 覚え方

- 他人のWindowsコードでは `TRUE`, `FALSE`, `NULL`, `VOID` は普通に出る
- 新しく自分で書くなら、まず `true`, `false`, `nullptr`, `void` を優先する
- ただし案件コードを読むには旧記法も読めないと困る

---

## 10. Phase 1 の確認課題

以下の問いに、自分の言葉で答えられるか確認する。

1. `typedef` と `using` は何をするものか
2. `#define` と `const` は何が違うか
3. `#ifdef UNICODE` は実行時分岐か、コンパイル時分岐か
4. `HWND` は数値そのものなのか、それともウィンドウ識別用の型か
5. `WPARAM` と `LPARAM` は何のためにあるか
6. `LRESULT` はどんな関数の戻り値に出やすいか
7. `TEXT("abc")` と `L"abc"` は何が関係しているか
8. `NULL` と `nullptr` の違いは何か

---

## 11. Phase 1 の成果物

このPhaseが終わったら、最低限次の成果物を残す。

### 11.1 コード

- `01_alias_macro/main.cpp`
- `02_windows_types/main.cpp`
- `03_unicode_macro/main.cpp`

### 11.2 ノート

`notes/windows_terms_phase1.md` を作り、次を表で整理する。

| 用語 | 分類 | 一言説明 |
|---|---|---|
| `DWORD` | 型 | Windows流儀の整数型名 |
| `HWND` | ハンドル | ウィンドウ識別子 |
| `WPARAM` | メッセージ情報 | 追加情報その1 |
| `#ifdef` | マクロ | 定義有無で分岐 |

最低10語は埋める。

---

## 12. Phase 1 の完了条件

次を満たしたら完了。

- 3つのサンプルプロジェクトがビルドできる
- Windows型を見て、分類できる
- `#ifdef` を見て意味がわかる
- `UNICODE`, `TEXT()`, `L""` の関係を説明できる
- `windows_terms_phase1.md` を自分の言葉で埋めた

---

## 13. このPhaseでやってはいけないこと

- `windows.h` 内部実装を全部追いかける
- `DWORD` のビット幅暗記に時間を使いすぎる
- GUIの見た目づくりに脱線する
- MFCや複雑なメッセージ処理に先走る

このPhaseは **単語の霧を晴らす段階** だ。  
深追いしすぎると本筋を外す。

---

## 14. 次のPhaseへの接続

Phase 1 が終わったら、次は **Phase 2: Win32の骨格** に進む。  
そこで初めて、次を扱う。

- `WinMain`
- メッセージループ
- `WndProc`
- `WM_COMMAND`
- ボタン1個の最小GUI

つまり、Phase 1 は **単語と記号の理解**、Phase 2 は **Windows GUIの流れの理解** という関係になる。
