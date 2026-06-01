# nlohmann/json 実践ガイド

> **前提：** `nlohmann/json` を使う環境が整っている前提で書く。
> vcpkg / CMake 経由で `#include <nlohmann/json.hpp>` が通る状態を想定。

まず最初にこれだけ書く。以降 `json` と書けば `nlohmann::json` を指す。

```cpp
#include <nlohmann/json.hpp>

using json = nlohmann::json;
```

---

## JSON を扱う6つの基本操作

JSON の仕事はほぼこの6つだ。

| # | 操作 | 使う API |
| :--- | :--- | :--- |
| 1 | ファイルから読む | `json::parse(ifs)` |
| 2 | 値を取り出す | `at()` / `value()` / `contains()` |
| 3 | 配列を回す | `for (const auto& x : j.at("arr"))` |
| 4 | 値を追加・更新する | `j["key"] = value` / `push_back()` |
| 5 | ファイルへ書き戻す | `j.dump(4)` → `ofstream` |
| 6 | 構造体へ変換する | `from_json` / `to_json` / `get<T>()` |

この記事はこの順番で進む。

---

## 第1章：ファイルを読み込む

### 使うJSONファイル `config.json`

```json
{
  "app_name": "ToolA",
  "version": 3,
  "window": {
    "width": 1280,
    "height": 720,
    "fullscreen": false
  },
  "recent_files": ["a.txt", "b.txt", "c.txt"]
}
```

### 読み込みの基本形

```cpp
#include <fstream>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main()
{
    try
    {
        // ファイルを開く
        std::ifstream ifs("config.json");
        if (!ifs)
        {
            std::cerr << "config.json を開けませんでした\n";
            return 1;
        }

        // パース
        json j = json::parse(ifs);

        // 必須項目を読む
        std::string appName = j.at("app_name").get<std::string>();
        int version         = j.at("version").get<int>();

        // ネストした値を読む
        int width  = j.at("window").at("width").get<int>();
        int height = j.at("window").at("height").get<int>();

        std::cout << appName << " v" << version << "\n";
        std::cout << width << " x " << height << "\n";
    }
    catch (const json::exception& e)
    {
        // パース失敗・キー欠落・型違いはここへ来る
        std::cerr << "JSONエラー: " << e.what() << "\n";
        return 1;
    }
}
```

### 読み方の流れ

```
ifstream でファイルを開く
    ↓
json::parse(ifs) でパースする
    ↓
at("key") でキーを指定する
    ↓
get<T>() で C++ の型に変換する
```

`at()` はキーが存在しなければ `json::exception` を投げる。
これが「必須項目向き」な理由だ。

---

## 第2章：`at()` / `value()` / `contains()` の使い分け

ここが実務で最も重要な判断だ。

| メソッド | 動作 | 使う場面 |
| :--- | :--- | :--- |
| `at("key")` | なければ例外 | **必須項目**（ないとアプリが動かない） |
| `value("key", default)` | なければデフォルト値を返す | **任意項目**（なくても動く） |
| `contains("key")` | 存在確認（bool） | 読む前に確認したいとき |
| `j["key"]` | なければキーを作る（！） | 書き込み用途のみ |

### `[]` を読み取りに使ってはいけない理由

```cpp
json j = json::parse(ifs);

// 非 const の json に対して [] で読もうとすると
// キーが存在しない場合、null として新しいキーを作ってしまう
int width = j["window"]["width"];  // 危険

// 正しく書くならこう
int width = j.at("window").at("width").get<int>();       // 必須
int width = j["window"].value("width", 800);              // 任意
```

### 実務でよく使うパターン

```cpp
json j = json::parse(ifs);

// 必須項目：なければ例外で止める
std::string appName = j.at("app_name").get<std::string>();

// 任意項目：なければ 60 を使う
int fps = j.value("fps", 60);

// 存在確認してから読む
if (j.contains("window"))
{
    int width  = j["window"].value("width",  800);
    int height = j["window"].value("height", 600);
    std::cout << width << " x " << height << "\n";
}
```

### 判断基準

```
このキーがなければアプリが壊れる？
    Yes → at()
    No  → value("key", デフォルト値)

読む前に存在を確認したい？
    → contains()

値を書き込む？
    → j["key"] = value
```

---

## 第3章：配列を回す

### 文字列の配列

```cpp
// "recent_files": ["a.txt", "b.txt", "c.txt"]

const json& files = j.at("recent_files");

for (const auto& file : files)
{
    std::cout << file.get<std::string>() << "\n";
}
// a.txt
// b.txt
// c.txt
```

### オブジェクトの配列

```json
{
  "items": [
    { "id": 1, "name": "ポーション",    "count": 3 },
    { "id": 2, "name": "グリーンハーブ", "count": 5 }
  ]
}
```

```cpp
for (const auto& item : j.at("items"))
{
    int         id    = item.at("id").get<int>();
    std::string name  = item.at("name").get<std::string>();
    int         count = item.at("count").get<int>();

    std::cout << "[" << id << "] " << name
              << " x" << count << "\n";
}
// [1] ポーション x3
// [2] グリーンハーブ x5
```

`const auto&` で受けると余計なコピーが発生しない。

---

## 第4章：値を更新して保存する

```cpp
json j = json::parse(ifs);

// 既存の値を更新
j["window"]["width"]  = 1920;
j["window"]["height"] = 1080;

// 新しいキーを追加
j["theme"] = "dark";

// 配列に要素を追加
j["recent_files"].push_back("d.txt");

// dump(4) でインデント4スペースの整形出力
std::ofstream ofs("config_out.json");
if (!ofs)
{
    std::cerr << "書き込めませんでした\n";
    return 1;
}

ofs << j.dump(4) << '\n';
```

`dump(0)` で改行なし、`dump(4)` で整形ありになる。
ファイル保存は `dump(4)` が可読性が高い。

---

## 第5章：設計の本質 ── JSON は境界に閉じ込める

### やってはいけない設計

```cpp
// json オブジェクトをアプリ全体に持ち回る
class Game
{
public:
    json config_;  // ← これが問題

    void update()
    {
        // ロジック層に json の文字列キーが漏れ出す
        int width = config_["window"]["width"].get<int>();
        // ...
    }
};
```

これは第2章で扱った「責務散乱」の JSON 版だ。

- `"window"` `"width"` という文字列リテラルがロジック全体に散らかる
- キー名を変えたとき、修正箇所が全コードに広がる
- JSON の構造が変わると、触っている全クラスが壊れる

### 正しい設計

```
JSON ファイル
    ↓ （境界：ここだけで json に触る）
読み込み関数（parse → 構造体へ変換）
    ↓
アプリ内部は struct / class で扱う
    ↓ （境界：ここだけで json に触る）
書き込み関数（構造体 → json → dump）
    ↓
JSON ファイル
```

**JSON は入出力形式。アプリ内部は C++ の型で扱う。**

これは `Player` が自分の HP 変更責務を持つのと同じ考え方だ。
「JSON という外部形式を知っているのは I/O 層だけ」と役割を閉じる。

---

## 第6章：構造体との相互変換

`from_json` / `to_json` を定義すると、`json::parse` で読んだ直後に構造体へ変換でき、
ロジック層に JSON を露出させずに済む。

### 構造体の定義

```cpp
struct WindowConfig
{
    int  width;
    int  height;
    bool fullscreen;
};

struct AppConfig
{
    std::string              app_name;
    int                      version;
    WindowConfig             window;
    std::vector<std::string> recent_files;
};
```

### JSON → 構造体（from_json）

```cpp
// nlohmann/json はこの名前の関数を自動で探して使う
void from_json(const json& j, WindowConfig& w)
{
    j.at("width").get_to(w.width);        // get_to() は変数へ直接書き込む
    j.at("height").get_to(w.height);
    j.at("fullscreen").get_to(w.fullscreen);
}

void from_json(const json& j, AppConfig& c)
{
    j.at("app_name").get_to(c.app_name);
    j.at("version").get_to(c.version);
    j.at("window").get_to(c.window);           // WindowConfig も自動で変換される
    j.at("recent_files").get_to(c.recent_files);
}
```

### 構造体 → JSON（to_json）

```cpp
void to_json(json& j, const WindowConfig& w)
{
    j = json{
        {"width",      w.width},
        {"height",     w.height},
        {"fullscreen", w.fullscreen}
    };
}

void to_json(json& j, const AppConfig& c)
{
    j = json{
        {"app_name",     c.app_name},
        {"version",      c.version},
        {"window",       c.window},        // WindowConfig の to_json が自動で呼ばれる
        {"recent_files", c.recent_files}
    };
}
```

### 全体サンプル（読み込み → 更新 → 保存）

```cpp
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct WindowConfig { int width; int height; bool fullscreen; };
struct AppConfig
{
    std::string              app_name;
    int                      version;
    WindowConfig             window;
    std::vector<std::string> recent_files;
};

void from_json(const json& j, WindowConfig& w)
{
    j.at("width").get_to(w.width);
    j.at("height").get_to(w.height);
    j.at("fullscreen").get_to(w.fullscreen);
}
void from_json(const json& j, AppConfig& c)
{
    j.at("app_name").get_to(c.app_name);
    j.at("version").get_to(c.version);
    j.at("window").get_to(c.window);
    j.at("recent_files").get_to(c.recent_files);
}

void to_json(json& j, const WindowConfig& w)
{
    j = json{{"width", w.width}, {"height", w.height}, {"fullscreen", w.fullscreen}};
}
void to_json(json& j, const AppConfig& c)
{
    j = json{
        {"app_name", c.app_name}, {"version", c.version},
        {"window", c.window}, {"recent_files", c.recent_files}
    };
}

int main()
{
    try
    {
        // 1. 読み込み → 構造体へ変換
        std::ifstream ifs("config.json");
        if (!ifs) { std::cerr << "読み込み失敗\n"; return 1; }

        AppConfig config = json::parse(ifs).get<AppConfig>();

        // 2. ロジック層では構造体だけ触る（json を知らない）
        config.version += 1;
        config.window.width  = 1920;
        config.window.height = 1080;
        config.recent_files.push_back("new.txt");

        // 3. 構造体 → JSON → 保存
        std::ofstream ofs("config_saved.json");
        if (!ofs) { std::cerr << "保存失敗\n"; return 1; }

        json out = config;   // to_json が自動で呼ばれる
        ofs << out.dump(4) << '\n';

        std::cout << "保存完了\n";
    }
    catch (const json::exception& e)
    {
        std::cerr << "JSONエラー: " << e.what() << "\n";
        return 1;
    }
}
```

`from_json` / `to_json` は名前と引数の形さえ合っていれば、
`get<AppConfig>()` や `json out = config;` のタイミングで自動的に呼ばれる。

---

## 第7章：エラー処理の方針

### 4つの典型的な失敗

| 失敗の種類 | 原因 | 対処 |
| :--- | :--- | :--- |
| ファイルが開けない | パス間違い・権限なし | `ifstream` の直後でチェック |
| JSON の文法が壊れている | カンマ抜け・括弧不一致 | `json::parse()` が例外 |
| キーが存在しない | 設定ファイルの旧バージョン | `at()` が例外 |
| 型が違う | 文字列のはずが数値など | `get<T>()` が例外 |

全部 `json::exception` のサブクラスとして飛んでくるため、
`catch (const json::exception& e)` 1つで受けられる。

### 設計とエラー処理は連動する

```
必須項目に at() を使う
    → 欠落した瞬間に例外で止まる（サイレント失敗しない）

任意項目に value() を使う
    → 欠落しても動く（デフォルトで補完）
```

`at()` か `value()` かの選択が、そのままエラー処理の設計になる。

---

## 理解度チェック

以下のコードの問題点を2つ指摘してほしい。

```cpp
class PlayerManager
{
public:
    void loadFromJson(const std::string& path)
    {
        std::ifstream ifs(path);
        config_ = json::parse(ifs);
    }

    std::string getPlayerName() const
    {
        return config_["player"]["name"].get<std::string>();
    }

    int getPlayerHp() const
    {
        return config_["player"]["hp"].get<int>();
    }

    void setPlayerHp(int hp)
    {
        config_["player"]["hp"] = hp;
    }

private:
    json config_;  // json をメンバとして持ち回っている
};
```

---

**正解：**

**問題1：json オブジェクトをクラス内部で持ち回っている**

`config_` が `json` 型なので、アプリ内部のロジックが `"player"` `"hp"` という
JSON の文字列キーに依存する。
JSON のキー名や構造が変わったとき、`PlayerManager` のメソッド全体を修正しなければならない。

```cpp
// 正しい設計
struct PlayerData { std::string name; int hp; };

class PlayerManager
{
public:
    void loadFromJson(const std::string& path)
    {
        std::ifstream ifs(path);
        player_ = json::parse(ifs).at("player").get<PlayerData>();
    }

    std::string getPlayerName() const { return player_.name; }
    int         getPlayerHp()   const { return player_.hp;   }
    void        setPlayerHp(int hp)   { player_.hp = hp;      }

private:
    PlayerData player_;  // 内部は構造体
};
```

**問題2：ファイルが開けないときのチェックがない**

`ifstream ifs(path)` の直後で `if (!ifs)` をチェックしていない。
ファイルが存在しない場合、`json::parse(ifs)` がエラーになるが、
「ファイルが開けなかった」という根本原因が分かりにくくなる。

```cpp
std::ifstream ifs(path);
if (!ifs)
{
    throw std::runtime_error("ファイルを開けませんでした: " + path);
}
```

> **まとめ：**
> JSON を読んだら早めに構造体へ変換し、`json` 型はその場で捨てる。
> これだけで、ロジック層が JSON の構造変化に引っ張られなくなる。
