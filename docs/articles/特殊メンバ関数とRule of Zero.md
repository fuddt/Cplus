# 特殊メンバ関数と Rule of Zero

C++ で違和感が出やすい理由は2つある。

1. **書いていないのに存在する関数がある**
2. **1つ書いた瞬間に、他の自動生成ルールが変わる**

この記事ではこの2点を整理する。

---

## 第1章：6つの特殊メンバ関数

C++ にはコンパイラが自動生成できる特別な関数が6つある。

```cpp
class Sample {
public:
    Sample();                            // ① デフォルトコンストラクタ
    Sample(const Sample& other);         // ② コピーコンストラクタ
    Sample& operator=(const Sample&);    // ③ コピー代入演算子
    Sample(Sample&& other);              // ④ ムーブコンストラクタ
    Sample& operator=(Sample&&);         // ⑤ ムーブ代入演算子
    ~Sample();                           // ⑥ デストラクタ
};
```

これらを**特殊メンバ関数**と呼ぶ。

**条件が整えば、自分で書かなくてもコンパイラが生成する。**

```cpp
class User {
public:
    std::string name;
    int age = 0;
};

User a;            // ① デフォルトコンストラクタ（自動生成）
User b = a;        // ② コピーコンストラクタ（自動生成）
User c; c = a;     // ③ コピー代入（自動生成）
User d = std::move(a); // ④ ムーブコンストラクタ（自動生成）
```

`User` には何も書いていないが、これが全部動く。
理由は次章で説明する。

---

## 第2章：Rule of Zero ── 書かないことが正解

### なぜ何も書かなくていいのか

`User` のメンバは `std::string` と `int` だ。

- `std::string` は自力でコピーもムーブも破棄もできる
- `int` はプリミティブなので当然できる

**メンバが全部ちゃんと後始末できるなら、クラス全体の特殊メンバ関数も自動生成される。**

```cpp
// これで十分。特殊メンバ関数は書かない
class Player {
private:
    std::string name_;
    int hp_ = 100;
    std::vector<std::string> items_;  // vector も自前で後始末できる
};
```

`std::string` / `std::vector` / `std::unique_ptr` などはすべて
自力で後始末できるように設計されている。
これらをメンバに使えば、**自分ではほぼ何も書かなくてよい。**

これを **Rule of Zero** と呼ぶ。

> Rule of Zero：
> コピー・ムーブ・破棄のどれも自前制御が不要なら、
> 6つの特殊メンバ関数を1つも書かない。

---

## 第3章：`= default` と `= delete` の意味

### `= default` とは

「自前実装ではなく、コンパイラの標準動作で生成してくれ」という明示だ。

```cpp
class Item {
public:
    Item() = default;       // コンパイラに標準生成を依頼
    ~Item() = default;      // コンパイラに標準生成を依頼
};
```

**`= default` は「特別な処理を書く」のではなく「普通の自動生成を明示する」だけ。**

ただし「書かない」と `= default` は完全に同じではない。

| 書き方 | 意味 |
| :--- | :--- |
| 何も書かない | コンパイラが条件次第で暗黙生成（生成されないこともある） |
| `= default` | 「必ず生成してくれ」という明示的な依頼 |

特に `virtual ~Base() = default;` のように、
**「virtual にしたい」** という意図は書かないと伝わらないため、
この形は意味がある。

### `= delete` とは

「この関数を存在させない」という禁止宣言だ。

```cpp
class Logger {
public:
    Logger(const Logger&) = delete;            // コピー禁止
    Logger& operator=(const Logger&) = delete; // コピー代入禁止
};

Logger a;
Logger b = a;  // コンパイルエラー：コピー禁止
```

`= delete` をつけることで、**「このクラスはコピーできない」** を型レベルで強制できる。

---

## 第4章：書いた瞬間にルールが変わる落とし穴

ここが C++ で最も非自明な挙動だ。

### 空のデストラクタを書いただけで何かが変わる

```cpp
class A {
public:
    ~A() {
        // 何もしない
    }
};
```

これは一見 harmless に見える。
しかし **デストラクタを自分で定義すると、ムーブの自動生成が抑制される。**

```
デストラクタを自分で書く
    → ムーブコンストラクタが自動生成されない
    → ムーブ代入演算子が自動生成されない
    → ムーブの代わりにコピーが使われる（パフォーマンス劣化）
```

これが「書いた瞬間にルールが変わる」の正体だ。

### 安全な書き方

何も処理がないなら、書かない方が自然だ。
もし明示したいなら `= default` を使う。

```cpp
// これはNG：ムーブが自動生成されなくなる
class A {
public:
    ~A() {}   // 何もしてないのに書いてしまった
};

// これはOK：ムーブの自動生成は維持される
class A {
public:
    ~A() = default;
};

// これが一番いい：書かない
class A {
    // デストラクタを書かない
};
```

### 自動生成の抑制ルール（概要）

| 何を定義したか | ムーブが自動生成されるか |
| :--- | :--- |
| 何も書かない | される |
| デストラクタを書いた | **されない**（deprecated な挙動） |
| コピーコンストラクタを書いた | **されない** |
| コピー代入を書いた | **されない** |

> **結論：** 意味のないデストラクタ・コピーを「なんとなく書く」のは有害だ。
> 書くなら意図を持って書く。意図がないなら書かない。

---

## 第5章：4つの実務パターン

### パターン1：普通のクラス ── 何も書かない

```cpp
// string / vector / map などをメンバに持つ一般的なクラス
class Config {
private:
    std::string path_;
    std::vector<int> values_;
    std::map<std::string, int> settings_;
};
// → デストラクタもコピーもムーブも書かない
//   メンバが全部後始末してくれる
```

---

### パターン2：抽象基底クラス ── virtual デストラクタだけ書く

```cpp
class IItem {
public:
    virtual ~IItem() = default;  // これだけ書く
    virtual void use()    = 0;
    virtual std::string name() const = 0;
};

class Herb : public IItem {
public:
    void use() override { /* ... */ }
    std::string name() const override { return "Herb"; }
    // デストラクタは書かない
};
```

なぜ基底クラスだけ `virtual ~IItem()` が必要か。

```cpp
IItem* item = new Herb();
delete item;   // IItem* 経由で delete する
```

`IItem*` 経由で `delete` すると、`IItem` のデストラクタが呼ばれる。
`virtual` がないと `Herb` のデストラクタが呼ばれず、**リソースリークする。**

```cpp
// virtual なし → 危険
class IItem {
public:
    ~IItem() {}   // 非 virtual
};

// virtual あり → 安全
class IItem {
public:
    virtual ~IItem() = default;   // Herb のデストラクタまで呼ばれる
};
```

---

### パターン3：コピー禁止、ムーブ許可

`std::unique_ptr` を持つクラスはコピーできない。
これを明示する。

```cpp
class AssetManager {
public:
    AssetManager() = default;

    // コピー禁止（unique_ptr はコピーできないので明示）
    AssetManager(const AssetManager&)            = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // ムーブは許可
    AssetManager(AssetManager&&)            = default;
    AssetManager& operator=(AssetManager&&) = default;

private:
    std::unique_ptr<int> resource_;
};
```

`unique_ptr` を持つクラスは、このパターンが基本形になる。

---

### パターン4：生の `new / delete` を使っている ── 設計を疑う

```cpp
// 避けたい形
class RawBuffer {
public:
    RawBuffer() { data_ = new int[100]; }
    ~RawBuffer() { delete[] data_; }   // 自前で書かないといけない
private:
    int* data_ = nullptr;
};
```

このクラスはコピーコンストラクタ・コピー代入も自前で書かないと
二重解放（double free）が起きる危険がある。
（Rule of Five：デストラクタを書いたら、コピーとムーブも5つ全部書け）

しかし本当は、設計を見直してスマートポインタに置き換えるのが正しい。

```cpp
// 正しい形：スマートポインタを使えば特殊メンバ関数は不要
class SafeBuffer {
private:
    std::unique_ptr<int[]> data_ = std::make_unique<int[]>(100);
};
// → 特殊メンバ関数は何も書かなくていい（Rule of Zero に戻る）
```

---

## 第6章：判断フロー

```
このクラスは「特別な後始末」が必要か？
│
├── No（string / vector / unique_ptr などで管理できる）
│   └── Rule of Zero：何も書かない
│
└── Yes
    ├── 基底クラスとして多態的に使う？
    │   └── Yes → virtual ~Base() = default; だけ書く
    │
    ├── コピーさせたくない？
    │   └── Yes → コピー系を = delete
    │
    ├── ムーブは許可したい？
    │   └── Yes → ムーブ系を = default
    │
    └── 生の new/delete を自前管理している
        └── → まず設計を見直す（unique_ptr で置き換えられないか）
```

---

## 理解度チェック

以下の4つのクラスについて「問題点があるか」「あるなら何が問題か」を答えてほしい。

```cpp
// クラス A
class A {
public:
    ~A() {}
private:
    std::vector<std::string> data_;
};

// クラス B
class IBase {
public:
    virtual void execute() = 0;
};

// クラス C
class C {
public:
    C() = default;
private:
    std::unique_ptr<int> ptr_;
};

// クラス D
class D {
public:
    D()  { buf_ = new char[256]; }
    ~D() { delete[] buf_; }
private:
    char* buf_ = nullptr;
};
```

---

**正解：**

**クラス A：問題あり**

`~A() {}` を書いたことで、ムーブコンストラクタとムーブ代入演算子の
自動生成が抑制される。
`std::vector` は本来ムーブ効率的だが、このクラスではコピーが使われてしまう。

```cpp
// 直し方
class A {
    // ~A() を書かない（メンバ vector が後始末してくれる）
private:
    std::vector<std::string> data_;
};
```

---

**クラス B：問題あり**

基底クラスなのに `virtual ~IBase()` が書かれていない。
`IBase*` 経由で派生クラスを `delete` すると、派生クラスのデストラクタが呼ばれず
リソースリークする危険がある。

```cpp
// 直し方
class IBase {
public:
    virtual ~IBase() = default;
    virtual void execute() = 0;
};
```

---

**クラス C：問題あり**

`std::unique_ptr` を持つクラスは暗黙のコピーができない。
しかし `= delete` で明示していないため、
コードを読む人が「コピーできないのか」を意図から読み取れない。

```cpp
// 直し方（コピー不可を明示する）
class C {
public:
    C() = default;
    C(const C&)            = delete;
    C& operator=(const C&) = delete;
    C(C&&)                 = default;
    C& operator=(C&&)      = default;
private:
    std::unique_ptr<int> ptr_;
};
```

---

**クラス D：問題あり（設計レベル）**

`new / delete` を自前管理しており、コピーコンストラクタ・コピー代入を書いていない。
デフォルトのコピーは `buf_` のアドレスをそのままコピーするため、
2つの `D` が同じメモリを `delete` する**二重解放**が起きる。

Rule of Five：デストラクタを自前で書いたなら、
コピー2つ・ムーブ2つも全部書かなければならない。

しかしより根本的には、設計を見直して `unique_ptr` に置き換えるのが正しい。

```cpp
// 根本的な直し方
class D {
private:
    std::unique_ptr<char[]> buf_ = std::make_unique<char[]>(256);
    // 特殊メンバ関数は何も書かなくていい（Rule of Zero）
};
```

> **まとめ：**
> 「とりあえず書く」は有害。意図のない特殊メンバ関数は
> 自動生成ルールを壊すか、設計の意図を隠す。
> Rule of Zero を基本とし、必要なときだけ明示的に書く。
