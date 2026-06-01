# Commandパターンを関数で置き換えるとどうなるか

## 関数ベースで依存関係を外に出すべきか、それともハイブリッド方式にすべきか

ソフトウェア設計の話をしていると、よく出てくるのが次のような構成です。

* 何かしらの命令を表す共通インターフェースがある
* 命令ごとの具体クラスがある
* 管理役のクラスが、適切な命令を選んで実行する

これは典型的には Command パターンに近い構成です。

ただ、ここで一度立ち止まって考えたいことがあります。
**本当にクラスである必要はあるのでしょうか。**

もし各命令が

* 状態を持たない
* 外部依存を持たない
* ただ呼ばれて処理を実行するだけ

であるなら、わざわざ `ICommand` を作り、具象クラスを増やし、`execute()` を持たせるのは、少し重いかもしれません。

そういうときに有力なのが、**関数ベースで命令を扱う方式**です。

この記事では、クラス版Command構成をそのまま関数ベースに置き換えた場合、設計がどう変わるかを整理します。
特に、**依存関係を外部で組み立てる方式** と **標準機能を内部で持つハイブリッド方式** に絞って、メリット・デメリットを見ていきます。

---

## まず前提：ここでいう「関数版Command」とは何か

まずは最小限の形を置きます。

```cpp
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <stdexcept>

// 命令の共通シグネチャ
// 今回は「引数なし・戻り値なし」の関数として扱う
using Command = std::function<void()>;

// 具体的な命令A
void startCommand()
{
    std::cout << "start\n";
}

// 具体的な命令B
void stopCommand()
{
    std::cout << "stop\n";
}
```

クラス版では `ICommand` を継承した `StartCommand` や `StopCommand` を作っていましたが、関数版ではそれを**普通の関数**に置き換えています。

その上で、これらを呼び出す管理クラスを作ります。

```cpp
class CommandManager {
private:
    std::unordered_map<std::string, Command> commands;

public:
    void registerCommand(const std::string& name, Command command)
    {
        commands[name] = std::move(command);
    }

    void run(const std::string& name)
    {
        auto it = commands.find(name);
        if (it == commands.end()) {
            throw std::runtime_error("unknown command: " + name);
        }

        // 見つけた関数を呼び出す
        it->second();
    }
};
```

ここでやっていることはシンプルです。

* 文字列キーと関数を対応づける
* 実行したい名前が来たら対応する関数を探す
* 見つけた関数を呼ぶ

クラス版と違って、`it->second->execute()` ではなく、`it->second()` として**関数そのものを実行**しています。

ここで問題になるのは、**`startCommand` や `stopCommand` をどこで `registerCommand()` するか** です。

---

# 方式1：依存関係を外部で組み立てる方式

まずは最も素直な方法です。
`CommandManager` 自体は空の状態で作り、外から必要な関数を登録します。

```cpp
int main()
{
    CommandManager manager;

    manager.registerCommand("start", startCommand);
    manager.registerCommand("stop", stopCommand);

    manager.run("start");
    manager.run("stop");
}
```

この方式の特徴は明確です。

* `CommandManager` は **`Command` という関数型だけ** を知っていればよい
* 具体的な `startCommand` や `stopCommand` は **外側のコードが組み立てる**
* 管理クラスは **「何が登録されるか」を知らなくてよい**

これは設計としてかなりきれいです。

---

## 外部組み立て方式のメリット

### 1. 疎結合になる

これが最大のメリットです。

`CommandManager` は「関数を登録して、呼び出す」という責務だけを持ちます。
「どんな命令が存在するか」という具体知識を持たなくて済みます。

つまり、管理役が具体実装に引っ張られにくくなります。

---

### 2. 新しい機能を足しやすい

新しい `restartCommand` を追加したいとします。

```cpp
void restartCommand()
{
    std::cout << "restart\n";
}
```

このとき、`CommandManager` のコード自体は変更せずに済みます。

```cpp
manager.registerCommand("restart", restartCommand);
```

これだけです。

管理役を改造せずに機能追加できるので、拡張に強いです。

---

### 3. テストしやすい

これは地味ですがかなり重要です。

たとえば、`CommandManager` が正しく命令を振り分けているかだけを確認したいとします。
そのとき、本物の `startCommand` ではなく、テスト用の関数を入れられます。

```cpp
void test_run_calls_command()
{
    CommandManager manager;

    bool called = false;

    // テスト用の関数をラムダで用意する
    manager.registerCommand("test", [&called]() {
        called = true;
    });

    manager.run("test");

    // 本当に関数が呼ばれたかだけを見る
    if (!called) {
        throw std::runtime_error("test failed");
    }
}
```

このように、**管理クラスの挙動だけを切り出してテストできる**のが強いです。

クラス版で FakeCommand を作っていた部分が、関数版では**ラムダ1つ**で済むこともあります。

---

### 4. optional feature に強い

環境や条件によって、使う命令を変えたい場面があります。

たとえば

* デバッグ時だけ使う命令
* 有料版だけ有効な命令
* 特定OSだけ有効な命令

などです。

外部組み立て方式なら、登録側で分岐すれば済みます。

```cpp
void debugDumpCommand()
{
    std::cout << "debug dump\n";
}

int main()
{
    CommandManager manager;

    manager.registerCommand("start", startCommand);
    manager.registerCommand("stop", stopCommand);

    bool debugMode = true;
    if (debugMode) {
        manager.registerCommand("debug_dump", debugDumpCommand);
    }

    manager.run("start");
}
```

つまり、**「どの機能を載せるか」という判断を manager の外に置ける**のです。

---

## 外部組み立て方式のデメリット

### 1. 初期化コードが長くなりやすい

機能が増えるほど、`main()` や初期化関数に `registerCommand()` が並びます。

```cpp
manager.registerCommand("a", commandA);
manager.registerCommand("b", commandB);
manager.registerCommand("c", commandC);
manager.registerCommand("d", commandD);
```

これは見通しを悪くしやすいです。

---

### 2. 標準セットが自明でなくなる

「この manager は最低限何を持っているべきか」が、作る場所によってばらつく可能性があります。

ある場所では `start` を登録しているが、別の場所では登録し忘れている、という事故も起こりえます。

---

# 方式2：ハイブリッド方式

次にハイブリッド方式です。

これは、**よく使う標準命令は manager の内部で自動登録しつつ、必要なら外から追加や差し替えもできる**という考え方です。

```cpp
class CommandManager {
private:
    std::unordered_map<std::string, Command> commands;

public:
    // 標準セットを自動登録する
    CommandManager()
    {
        registerCommand("start", startCommand);
        registerCommand("stop", stopCommand);
    }

    void registerCommand(const std::string& name, Command command)
    {
        commands[name] = std::move(command);
    }

    void run(const std::string& name)
    {
        auto it = commands.find(name);
        if (it == commands.end()) {
            throw std::runtime_error("unknown command: " + name);
        }

        it->second();
    }
};
```

使う側はかなり楽になります。

```cpp
int main()
{
    CommandManager manager;
    manager.run("start");
}
```

これだけで動きます。

---

## ハイブリッド方式のメリット

### 1. 使う側が圧倒的に楽

最大の魅力はここです。

外部組み立て方式では毎回

```cpp
manager.registerCommand(...);
manager.registerCommand(...);
```

が必要でした。

しかしハイブリッド方式では、標準機能については manager を作るだけで使えます。

これは実務ではかなり大きいです。
理想論より、**毎回の利用が楽であること** の価値は高いです。

---

### 2. 標準セットを保証できる

「この manager には最低限この命令群が入っている」という保証が作れます。

これは利用側にとって安心です。
登録漏れが起きにくくなります。

---

### 3. 小〜中規模では非常にバランスが良い

機能数がそこまで多くないなら、ハイブリッド方式はかなり実用的です。

* 標準機能はすぐ使える
* 特殊機能だけ外から足せる

この形は現実的な落とし所になりやすいです。

---

## ハイブリッド方式のデメリット

### 1. 管理クラスが具体実装を知ることになる

ここが一番大きいです。

外部組み立て方式では、`CommandManager` は `Command` という関数型しか知りませんでした。
しかしハイブリッド方式にすると、manager 自身が `startCommand` や `stopCommand` という標準実装を内部で選び始めます。

つまり、管理役が具体機能の知識を持ち始めます。

これは責務の純度を下げます。

---

### 2. optional feature が増えると manager が太る

最初はこうです。

```cpp
CommandManager()
{
    registerCommand("start", startCommand);
    registerCommand("stop", stopCommand);
}
```

しかし実務では、すぐにこうなります。

```cpp
class CommandManager {
private:
    std::unordered_map<std::string, Command> commands;

public:
    CommandManager(bool debugMode, bool premiumMode)
    {
        registerCommand("start", startCommand);
        registerCommand("stop", stopCommand);

        if (debugMode) {
            registerCommand("debug_dump", debugDumpCommand);
        }

        if (premiumMode) {
            registerCommand("advanced", []() {
                std::cout << "advanced\n";
            });
        }
    }

    void registerCommand(const std::string& name, Command command)
    {
        commands[name] = std::move(command);
    }

    void run(const std::string& name)
    {
        auto it = commands.find(name);
        if (it == commands.end()) {
            throw std::runtime_error("unknown command: " + name);
        }

        it->second();
    }
};
```

さらに条件が増えると、manager が「単なる実行管理」ではなく、**機能選択の責務まで背負い始める**のです。

これはハイブリッド方式の典型的な重さです。

---

### 3. テスト差し替えが少し面倒になる

ハイブリッド方式では、manager を作った時点で本物の命令が入っています。

そのため、テストで偽物に差し替えたいときに工夫が必要です。

たとえば、デフォルト登録を無効化するオプションを作るなどです。

```cpp
class CommandManager {
private:
    std::unordered_map<std::string, Command> commands;

public:
    CommandManager(bool loadDefaults = true)
    {
        if (loadDefaults) {
            registerCommand("start", startCommand);
            registerCommand("stop", stopCommand);
        }
    }

    void registerCommand(const std::string& name, Command command)
    {
        commands[name] = std::move(command);
    }

    void run(const std::string& name)
    {
        auto it = commands.find(name);
        if (it == commands.end()) {
            throw std::runtime_error("unknown command: " + name);
        }

        it->second();
    }
};
```

これならテスト時だけ空で作れます。

```cpp
void test_dispatch_only()
{
    CommandManager manager(false); // 標準登録なし

    bool called = false;

    manager.registerCommand("test", [&called]() {
        called = true;
    });

    manager.run("test");

    if (!called) {
        throw std::runtime_error("test failed");
    }
}
```

つまり、**ハイブリッド方式はテストできないのではない**です。
ただし、**テストしやすくするための設計を追加で用意する必要がある**ということです。

---

# 何がトレードオフなのか

ここを一言でまとめるとこうです。

## 外部組み立て方式が得るもの

* 疎結合
* 差し替えやすさ
* optional feature への強さ
* テストしやすさ

## 外部組み立て方式が失うもの

* 初期化の簡単さ
* 標準構成のわかりやすさ
* 利用時の手軽さ

---

## ハイブリッド方式が得るもの

* 使いやすさ
* 標準セットの保証
* 初期化の簡単さ
* 小規模〜中規模での現実的な運用性

## ハイブリッド方式が失うもの

* 疎結合の純度
* 機能選択の柔軟さ
* テスト差し替えのしやすさ
* 管理クラスの責務のきれいさ

---

# クラス版と関数版は、どちらがよいのか

ここで重要なのは、**クラス版Commandと関数版Command風構成は、どちらが正義かという話ではない**ことです。

判断基準はシンプルです。

### 関数版が向いているケース

* 各命令が状態を持たない
* 各命令が短い
* 外部依存がない
* 共通シグネチャだけ揃えば十分
* 実装を軽く保ちたい

### クラス版が向いているケース

* 命令ごとに状態を持ちたい
* Logger や DB などの依存関係を持ちたい
* 初期化や後始末が必要
* テストでオブジェクト単位に差し替えたい
* 命令自体が大きく育ってきた

つまり、**状態を持たない薄い処理に対して、最初からクラスを作る必要はない**のです。

関数で十分なら、まずは関数で始めた方が軽いです。

---

# 実務的な結論

個人的には、状態を持たない処理が中心なら、**最初は関数版で始める**のがかなり合理的だと思います。

特に、

* 処理が短い
* ただ呼ばれて動くだけ
* 共有すべき内部状態がない

のであれば、クラス化の利益は薄いことが多いです。

その上で、標準機能をどこまで楽に使いたいかによって、

* より疎結合にしたいなら **外部組み立て方式**
* 利用側の手軽さを優先したいなら **ハイブリッド方式**

を選べばよいです。

そして、ハイブリッド方式を選ぶなら、クラス版と同じく
**`loadDefaults = false` のようなテスト用の逃げ道を最初から作っておく**
のが安全です。

---

# まとめ

Commandパターン周辺の設計で重要なのは、「クラスであること」そのものではありません。
本当に重要なのは、**命令をどう表現するか** と **依存関係をどこで組み立てるか** です。

* 状態を持たないなら、関数で表現してもよい
* 外部で組み立てれば、疎結合で柔軟になる
* 内部で自動登録すれば、使いやすくなる
* ハイブリッド方式は、その中間にある現実的な選択肢である

そして本質的には、これもやはり明確なトレードオフです。

**使いやすさを取るか、差し替えやすさを取るか。
標準構成の明快さを取るか、責務の純度を取るか。**

設計とは、このバランスを自分の現場に合わせて決める作業です。

