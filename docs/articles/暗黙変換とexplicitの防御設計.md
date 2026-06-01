# 暗黙変換と explicit の防御設計

C++ は「賢すぎる言語」だ。
意図しないところでコンパイラが型を自動変換し、コードを通してしまう。

この記事では、その危険の正体と `explicit` による防衛を整理する。

> `インスタンス生成と初期化.md` で扱った `{}` による narrowing conversion 防止と
> 対になる知識。あわせて読むと多層防衛の全体像がつかめる。

---

## 第1章：暗黙変換の正体

### まずこのコードを見る

```cpp
class ItemBox
{
public:
    ItemBox(std::size_t capacity)
    {
        // capacity 個分の箱を作る
    }
};
```

このクラスがあるとき、次のコードはコンパイルを通る。

```cpp
ItemBox box = 10;  // これが通る
```

`10` は `std::size_t` ではなく `int` だ。
しかし `ItemBox` でもない。

なぜ通るのか。

### コンパイラが裏で変換している

コンパイラは `=` を見たとき、右辺の型が左辺と違っていても
「変換できるコンストラクタがないか？」を探す。

```
10  →（コンパイラが変換）→  ItemBox(10)
```

この自動変換が **暗黙変換（implicit conversion）** だ。

`ItemBox(std::size_t)` という単一引数コンストラクタが存在するため、
コンパイラは `10` を使ってコンストラクタを呼び出し、`ItemBox` を作る。

---

## 第2章：なぜ危険か

### 可読性が崩れる

```cpp
ItemBox box = 10;
```

これを読んだ人は「10 を代入している」と思う。
しかし実際は「`ItemBox(10)` でオブジェクトを構築している」。

**見た目と実態が乖離する。**

### 関数呼び出しにも影響する

```cpp
void open(ItemBox box);

open(10);  // これが通る
```

`open(ItemBox{10})` と書くべきところが、`open(10)` で通ってしまう。

設計意図が曖昧になり、呼び出し側の意識が散漫になる。

### 意図しない変換がバグになる

```cpp
void setSize(ItemBox box);
void setSize(bool flag);

setSize(5);  // どちらが呼ばれる？（危険な多義性）
```

暗黙変換が有効だと、オーバーロード解決が想定外の動きをする場合がある。

---

## 第3章：`explicit` で防ぐ

### 解決策はシンプル

```cpp
class ItemBox
{
public:
    explicit ItemBox(std::size_t capacity)
    //^^^^^^ これだけでいい
    {
    }
};
```

`explicit` をつけると、暗黙変換が禁止される。

```cpp
ItemBox box = 10;     // エラー: 暗黙変換は禁止
ItemBox box(10);      // OK: 明示的な構築
ItemBox box{10};      // OK: 明示的な構築（推奨）
```

---

## 第4章：`explicit` の適用ルール

### 原則：単一引数コンストラクタは基本 `explicit`

```cpp
class A
{
public:
    explicit A(int x);         // ← つける
    explicit A(std::string s); // ← つける
    A(int x, int y);           // 2引数以上は暗黙変換しないので不要
};
```

単一引数コンストラクタは「変換コンストラクタ」として機能してしまう。
これを意図的に使う場面は稀なので、基本は `explicit` をつける。

### 例外：意図的に暗黙変換を使わせたいとき

```cpp
class StringWrapper
{
public:
    StringWrapper(const char* s) : data_{s} {}
    // explicit なし → "hello" から暗黙変換を意図している
private:
    std::string data_;
};

StringWrapper w = "hello";  // 自然に読める
```

文字列リテラルからの変換など、「自明で安全な変換」であれば
`explicit` を外すことも設計上あり得る。
ただしこれは意図的な選択であり、デフォルトは `explicit` だ。

---

## 第5章：`explicit` と `{}` の多層防衛

`{}` と `explicit` は別々の問題を防いでいる。

| 防衛手段 | 防ぐもの |
| :--- | :--- |
| `{}` による初期化 | 縮小変換（`double → int` など数値の精度落ち）|
| `explicit` | コンストラクタを経由した暗黙の型変換 |

```cpp
class Score
{
public:
    explicit Score(int value) : value_{value} {}

private:
    int value_;
};

Score s1 = 100;         // エラー（explicit が防ぐ）
Score s2(3.14);         // 通る（3 に切り捨て）
Score s3{3.14};         // エラー（{} が narrowing conversion を防ぐ）
Score s4{100};          // OK（明示的、かつ縮小変換なし）
```

`explicit` と `{}` を組み合わせると、2層で変換ミスを防げる。

> **推奨：**
> - 単一引数コンストラクタには `explicit` をつける
> - 初期化には `{}` を使う
> この2つが揃えば、型変換に関するバグを大幅に排除できる。

---

## 理解度チェック

以下のコードのうち、コンパイルエラーになるものを全て選んでほしい。

```cpp
class Container
{
public:
    explicit Container(int size);
    Container(int rows, int cols);
};

// A
Container a = 5;

// B
Container b(5);

// C
Container c{5};

// D
Container d = Container(5);

// E
Container e(2, 3);

// F
Container f = {2, 3};
```

---

**正解：**

- **A → エラー**。`=` による暗黙変換は `explicit` で禁止される。
- **B → OK**。直接コンストラクタを呼んでいる。
- **C → OK**。`{}` でも直接構築は可能。
- **D → OK**。`Container(5)` を明示的に作って代入している。
- **E → OK**。2引数なので `explicit` は関係しない。
- **F → OK**。`{2, 3}` による直接初期化。2引数コンストラクタが呼ばれる。

> **重要な視点：** `explicit` は「暗黙変換」だけを禁止する。
> `Container b(5)` や `Container c{5}` のような「明示的な構築」は問題なく動く。
