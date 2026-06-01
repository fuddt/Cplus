
# Commandパターンにおける依存関係の組み立ては外に出すべきか

## それともハイブリッド方式にすべきか

ソフトウェア設計の話をしていると、よく出てくるのが次のような構成です。

* 何かしらの命令を表すインターフェースがある
* 命令ごとの具体クラスがある
* 管理役のクラスが、適切な命令を選んで実行する

これは典型的には Command パターンに近い構成です。

しかし、実務で必ずぶつかるのが次の問題です。

**「その具体クラスたちは、どこで生成して、どこで登録すべきなのか？」**

ここには大きく3つの考え方があります。

1. 依存関係を外部で組み立てる方式
2. 管理クラスが内部で全部生成する方式
3. 標準セットは内部で持ちつつ、追加や差し替えは外部でもできるハイブリッド方式

この記事では、特に **外部で依存関係を組み立てる方式** と **ハイブリッド方式** に絞って、メリット・デメリットを整理します。

---

## まず前提：ここでいう「Commandパターン」とは何か

話を単純化するために、まず最小限の形を置きます。

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <stdexcept>

// 命令の共通インターフェース
class ICommand {
public:
    virtual ~ICommand() = default;

    // 具体的な命令を実行する
    virtual void execute() = 0;
};

// 具体的な命令A
class StartCommand : public ICommand {
public:
    void execute() override {
        std::cout << "start\n";
    }
};

// 具体的な命令B
class StopCommand : public ICommand {
public:
    void execute() override {
        std::cout << "stop\n";
    }
};
```

このように、`ICommand` を共通の型として、`StartCommand` や `StopCommand` のような具体クラスを扱う構成は、非常に典型的です。

その上で、これらを呼び出す管理クラスを作るとします。

```cpp
class CommandManager {
private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> commands;

public:
    void registerCommand(const std::string& name, std::unique_ptr<ICommand> command) {
        commands[name] = std::move(command);
    }

    void run(const std::string& name) {
        auto it = commands.find(name);
        if (it == commands.end()) {
            throw std::runtime_error("unknown command: " + name);
        }
        it->second->execute();
    }
};
```

ここで問題になるのは、**`StartCommand` や `StopCommand` をどこで `registerCommand()` するか** です。

---

# 方式1：依存関係を外部で組み立てる方式

まずは最も素直な方法です。
`CommandManager` 自体は空の状態で作り、外から必要なものを登録します。

```cpp
int main() {
    CommandManager manager;

    manager.registerCommand("start", std::make_unique<StartCommand>());
    manager.registerCommand("stop", std::make_unique<StopCommand>());

    manager.run("start");
    manager.run("stop");
}
```

この方式の特徴は明確です。

* `CommandManager` は **ICommand という抽象だけ** を知っていればよい
* 具体的な `StartCommand` や `StopCommand` は **外側のコードが組み立てる**
* 管理クラスは **「何が登録されるか」を知らなくてよい**

これは設計としてかなりきれいです。

---

## 外部組み立て方式のメリット

### 1. 疎結合になる

これが最大のメリットです。

`CommandManager` は「コマンドを登録して、呼び出す」という責務だけを持ちます。
「どんなコマンドが存在するか」という知識を持たなくて済みます。

つまり、管理役が具体クラスに引っ張られにくくなります。

---

### 2. 新しい機能を足しやすい

新しい `RestartCommand` を追加したいとします。

```cpp
class RestartCommand : public ICommand {
public:
    void execute() override {
        std::cout << "restart\n";
    }
};
```

このとき、`CommandManager` のコード自体は変更せずに済みます。

```cpp
manager.registerCommand("restart", std::make_unique<RestartCommand>());
```

これだけです。

管理役を改造せずに機能追加できるので、拡張に強いです。

---

### 3. テストしやすい

これは地味ですがかなり重要です。

たとえば、`CommandManager` が正しく命令を振り分けているかだけを確認したいとします。
そのとき、本物の `StartCommand` ではなく、テスト用の偽物を入れられます。

```cpp
class FakeCommand : public ICommand {
public:
    bool called = false;

    void execute() override {
        // 本物の重い処理はしない
        called = true;
    }
};
```

```cpp
void test_run_calls_command() {
    CommandManager manager;

    auto fake = std::make_unique<FakeCommand>();
    FakeCommand* raw = fake.get();

    manager.registerCommand("test", std::move(fake));
    manager.run("test");

    // 本当に execute が呼ばれたかだけを見る
    if (!raw->called) {
        throw std::runtime_error("test failed");
    }
}
```

このように、**管理クラスの挙動だけを切り出して পরীক্ষাできる**のが強いです。

---

### 4. optional feature に強い

環境や条件によって、使うコマンドを変えたい場面があります。

たとえば

* デバッグ時だけ使うコマンド
* 有料版だけ有効なコマンド
* 特定OSだけ有効なコマンド

などです。

外部組み立て方式なら、登録側で分岐すれば済みます。

```cpp
int main() {
    CommandManager manager;

    manager.registerCommand("start", std::make_unique<StartCommand>());
    manager.registerCommand("stop", std::make_unique<StopCommand>());

    bool debugMode = true;
    if (debugMode) {
        manager.registerCommand("debug_dump", std::make_unique<DebugDumpCommand>());
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
manager.registerCommand("a", ...);
manager.registerCommand("b", ...);
manager.registerCommand("c", ...);
manager.registerCommand("d", ...);
```

これは見通しを悪くしやすいです。

---

### 2. 標準セットが自明でなくなる

「この manager は最低限何を持っているべきか」が、作る場所によってばらつく可能性があります。

ある場所では `start` を登録しているが、別の場所では登録し忘れている、という事故も起こりえます。

---

# 方式2：ハイブリッド方式

次にハイブリッド方式です。

これは、**よく使う標準コマンドは manager の内部で自動登録しつつ、必要なら外から追加や差し替えもできる**という考え方です。

```cpp
class CommandManager {
private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> commands;

public:
    // 標準セットを自動登録する
    CommandManager() {
        registerCommand("start", std::make_unique<StartCommand>());
        registerCommand("stop", std::make_unique<StopCommand>());
    }

    void registerCommand(const std::string& name, std::unique_ptr<ICommand> command) {
        commands[name] = std::move(command);
    }

    void run(const std::string& name) {
        auto it = commands.find(name);
        if (it == commands.end()) {
            throw std::runtime_error("unknown command: " + name);
        }
        it->second->execute();
    }
};
```

使う側はかなり楽になります。

```cpp
int main() {
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

「この manager には最低限このコマンド群が入っている」という保証が作れます。

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

### 1. 管理クラスが具体クラスを知ることになる

ここが一番大きいです。

外部組み立て方式では、`CommandManager` は `ICommand` しか知りませんでした。
しかしハイブリッド方式にすると、manager 自身が `StartCommand` や `StopCommand` を生成します。

つまり、管理役が具体実装の知識を持ち始めます。

これは責務の純度を下げます。

---

### 2. optional feature が増えると manager が太る

最初はこうです。

```cpp
CommandManager() {
    registerCommand("start", std::make_unique<StartCommand>());
    registerCommand("stop", std::make_unique<StopCommand>());
}
```

しかし実務では、すぐにこうなります。

```cpp
CommandManager(bool debugMode, bool premiumMode) {
    registerCommand("start", std::make_unique<StartCommand>());
    registerCommand("stop", std::make_unique<StopCommand>());

    if (debugMode) {
        registerCommand("debug_dump", std::make_unique<DebugDumpCommand>());
    }

    if (premiumMode) {
        registerCommand("advanced", std::make_unique<AdvancedCommand>());
    }
}
```

さらに条件が増えると、manager が「単なる実行管理」ではなく、**機能選択の責務まで背負い始める**のです。

これはハイブリッド方式の典型的な重さです。

---

### 3. テスト差し替えが少し面倒になる

ハイブリッド方式では、manager を作った時点で本物のコマンドが入っています。

そのため、テストで偽物に差し替えたいときに工夫が必要です。

たとえば、デフォルト登録を無効化するオプションを作るなどです。

```cpp
class CommandManager {
private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> commands;

public:
    CommandManager(bool loadDefaults = true) {
        if (loadDefaults) {
            registerCommand("start", std::make_unique<StartCommand>());
            registerCommand("stop", std::make_unique<StopCommand>());
        }
    }

    void registerCommand(const std::string& name, std::unique_ptr<ICommand> command) {
        commands[name] = std::move(command);
    }

    void run(const std::string& name) {
        auto it = commands.find(name);
        if (it == commands.end()) {
            throw std::runtime_error("unknown command: " + name);
        }
        it->second->execute();
    }
};
```

これならテスト時だけ空で作れます。

```cpp
void test_dispatch_only() {
    CommandManager manager(false); // 標準登録なし

    auto fake = std::make_unique<FakeCommand>();
    FakeCommand* raw = fake.get();

    manager.registerCommand("test", std::move(fake));
    manager.run("test");

    if (!raw->called) {
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

# どちらが正しいのか

これは「どちらが正義か」という話ではありません。
本当にトレードオフです。

---

## 外部組み立て方式が向いているケース

* 機能追加が頻繁にある
* コマンドの種類が多い
* 将来的に feature の出し分けが増えそう
* テストの差し替えを重視したい
* 管理クラスを汎用的な部品として保ちたい

---

## ハイブリッド方式が向いているケース

* 標準機能がある程度固まっている
* 利用側の手軽さを優先したい
* 規模が小〜中程度
* manager をそのアプリ専用部品として使う
* 厳密な疎結合より、実装と運用のわかりやすさを優先したい

---

# 実務的な結論

個人的には、実務では次の考え方がかなり強いと思います。

**最初はハイブリッド方式で始めてよい。
ただし、テスト用の逃げ道だけは最初から作っておく。**

たとえば

```cpp
CommandManager(bool loadDefaults = true)
```

のような形です。

これなら

* 普段は楽に使える
* テストでは空の manager に差し替えられる
* 将来きつくなったら外部組み立て方式に寄せやすい

というバランスが取れます。

逆に、最初から理想的な外部組み立てだけを目指すと、
小規模プロジェクトでは「設計はきれいだが、使いづらい」状態になりがちです。

---

# まとめ

Commandパターン周辺の設計で重要なのは、
「パターン名」そのものよりも、**依存関係をどこで組み立てるか** です。

* 外部で組み立てれば、疎結合で柔軟になる
* 内部で自動登録すれば、使いやすくなる
* ハイブリッド方式は、その中間にある現実的な選択肢である

そして本質的には、これはきれいごとの話ではなく、明確なトレードオフです。

**使いやすさを取るか、差し替えやすさを取るか。
標準構成の明快さを取るか、責務の純度を取るか。**

設計とは、このバランスを自分の現場に合わせて決める作業です。


