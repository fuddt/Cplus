# コンポジションと this ポインタの読み方

> **前提：** 参照・ポインタ・初期化子リストの基本は理解している前提で書く。
> この記事が扱うのは「複数のクラスが絡み合ったコードをどう読むか」だ。

---

## この記事が解決する問い

C++ を読んでいると、こういう場面に当たる。

```cpp
class Player {
private:
    Bag bag_;          // Bag が Player の中に？
};

class Bag {
private:
    std::vector<Apple> apples_;  // Apple の群れが Bag の中に？
};
```

そして関数の中でこういうコードが出てくる。

```cpp
void Player::eatApple(std::size_t index) {
    bag_.useApple(index, *this);  // *this ← これは何？
}
```

「クラスの中にクラスがある」「`*this` を渡している」この2点で詰まった場合、
この記事を読む。

---

## 第1章：C++ クラス読解の手順

まず最初に覚えるべきことはこれだ。

> **関数の処理から読むな。構造から読め。**

いきなり `.cpp` の関数本体に入ると、
`this`・参照・コピー・メンバの寿命が一気に混ざって見えて詰まる。

### 5ステップの読み方

```
Step 1: ヘッダを開き、メンバ変数だけを見る
            ↓
Step 2: コンストラクタの初期化子リストを見る
            ↓
Step 3: 関数シグネチャの型（T / T& / const T& / T*）を読む
            ↓
Step 4: .cpp を開き、誰に処理を委譲しているかを見る
            ↓
Step 5: コピー境界（値渡し・値返し・push_back）を確認する
```

---

## 第2章：コンポジション（has-a 関係）を読む

コンポジションとは「クラスが別のクラスをメンバとして持つ」ことだ。

```cpp
class Player {
private:
    std::string name_;
    int hp_;
    int maxHp_;
    Bag bag_;   // ← Player は Bag を持つ
};
```

これを見た瞬間に読むのは**「誰が何を持っているか」**だ。

```
Player  has-a  Bag
Bag     has-many  Apple（std::vector<Apple>）
Shop    has-a  Apple（おすすめ商品）
```

継承（is-a）との違いはここだ。

| 関係 | 読み方 | 例 |
| :--- | :--- | :--- |
| **継承（is-a）** | Circle は Shape だ | `class Circle : public Shape` |
| **コンポジション（has-a）** | Player は Bag を持つ | `Bag bag_;` |

コンポジションはクラスの「部品」として別クラスを使う設計だ。
「Bag は Player の一部」であり、Player が生きている間は Bag も生きている。

---

## 第3章：実体で持つか、ポインタで持つか

Step 1 でメンバ変数を見るとき、必ずこの問いを立てる。

> **これは実体か、ポインタか？**

### 実体で持つ（直接持つ）

```cpp
class Player {
private:
    Bag bag_;   // Bag の実体を直接持つ
};
```

- Player が作られると同時に `bag_` も作られる
- Player が消えると `bag_` も消える
- 寿命が親（Player）と完全に連動する

### ポインタで持つ

```cpp
class Player {
private:
    Bag* bag_;   // Bag の場所だけ持つ
};
```

- まだ何も指していないかもしれない（`nullptr`）
- 別の場所の Bag を指しているだけかもしれない
- 誰が所有しているかが即断しづらい

**読む順序として:** まず実体メンバ（`Bag bag_`）を理解してから、
ポインタメンバ・スマートポインタへ進む方が崩れにくい。

### `unique_ptr` はその中間

```cpp
auto rareApple = std::make_unique<Apple>("Rare Apple", 35);
```

- ヒープ上に Apple を作る
- その Apple の所有権を `unique_ptr` が管理する
- スコープを抜けると自動で解放される
- 使うときは `*rareApple` で Apple の実体にアクセスする

```cpp
hero.pickApple(*rareApple);   // unique_ptr を参照外しして Apple& として渡す
```

---

## 第4章：this と *this

これが多クラス設計で最初に詰まる場所だ。

```cpp
void Player::eatApple(std::size_t index) {
    bag_.useApple(index, *this);   // ← *this とは何か
}
```

### `this` とは

メンバ関数の中では `this` が使える。
型は**「そのクラスへのポインタ」**だ。

```cpp
// Player のメンバ関数の中では
this   →   Player*   （自分自身へのポインタ）
```

### `*this` とは

`this` を参照外ししたものだ。

```cpp
*this  →   Player&   （自分自身の参照として渡せる形）
```

### なぜ `*this` を渡すのか

`Bag::useApple` の宣言を確認する。

```cpp
void useApple(std::size_t index, Player& player);
//                                ^^^^^^^^
//                          Player の参照を受け取る
```

引数が `Player&`（参照）なので、`this`（ポインタ）のままでは型が合わない。

```
this    → Player*  ← ポインタ。Player& には渡せない
*this   → Player&  ← 参照。そのまま渡せる
```

だから `*this` を渡す。

### 処理の流れで追う

```
Player::eatApple(0)
    ↓ bag_.useApple(0, *this)   // 自分自身を Bag に渡す
Bag::useApple(0, player)
    ↓ apples_[0].use(player)    // Apple に Player を渡す
Apple::use(player)
    ↓ player.heal(healAmount_)  // Player に回復を依頼する
Player::heal(amount)
    → hp_ を更新する            // ここで状態が変わる
```

Apple が直接 HP を書き換えるのではなく、
**Player に「回復して」と依頼している**のが重要だ。
HP の更新ロジックを Player に集約する設計（責務分離）と一致している。

---

## 第5章：コピーがどこで起きるか

`const T&` で受け渡しをしているのに、どこかでコピーが必ず起きる。
その場所を正確に把握することが重要だ。

### 流れを追う

```cpp
hero.pickApple(normalApple);
```

```cpp
// Player.cpp
void Player::pickApple(const Apple& apple) {
    bag_.add(apple);
}

// Bag.cpp
void Bag::add(const Apple& apple) {
    apples_.push_back(apple);   // ← ここでコピーが起きる
}
```

```
normalApple（main 上の実体）
    ↓ const Apple& で受け取る  → コピーなし
Player::pickApple
    ↓ const Apple& で渡す      → コピーなし
Bag::add
    ↓ push_back                → ここで vector に Apple をコピーして保存
```

### コピー境界の一覧

| 操作 | コピーが起きるか |
| :--- | :--- |
| `const T&` で引数を受け取る | **起きない** |
| `T&` で引数を受け取る | **起きない** |
| `T`（値）で引数を受け取る | **起きる**（関数に入る時点でコピー） |
| `vector::push_back(x)` | **起きる**（vector 内部にコピーして保存） |
| 値で return する | **起きる**（呼び出し側に別オブジェクトが渡る） |
| `const T&` で return する | **起きない**（既存オブジェクトの参照を返す） |

### `shop.buyRecommendedApple()` の場合

```cpp
Apple Shop::buyRecommendedApple() const {
    return recommendedApple_;   // 値で return → コピーが起きる
}

hero.pickApple(shop.buyRecommendedApple());
```

1. `buyRecommendedApple()` が Apple を値で返す → コピー発生
2. 返ってきた Apple を `const Apple&` で `pickApple` が受ける → コピーなし
3. `push_back` で Bag 内に保存 → コピー発生

**値で return する関数は「別のオブジェクトを渡す境界」**と読む。

---

## 第6章：前方宣言と循環依存

複数クラスが絡むとき、ヘッダのインクルード設計が問題になる。

### 循環依存が起きる悪い形

```cpp
// Apple.h
#include "Player.h"   // Player を使うためにインクルード
class Apple { void use(Player& player) const; };

// Player.h
#include "Apple.h"    // Apple を使うためにインクルード
class Player { Bag bag_; };
```

`Apple.h` が `Player.h` を読み、
`Player.h` が `Apple.h` を読む → **循環して解決不能**。

### 前方宣言で解決する

```cpp
// Apple.h
class Player;   // 「Player という名前のクラスが存在する」とだけ伝える

class Apple {
public:
    void use(Player& player) const;  // Player& は宣言だけで使える
};
```

前方宣言は「名前だけ知らせる」だ。中身はまだ知らない。

### どちらを使うかの判断基準

| メンバの持ち方 | 必要なもの | 理由 |
| :--- | :--- | :--- |
| `Player player_`（実体） | `#include "Player.h"` | サイズと構造が必要 |
| `Player& player_`（参照） | 前方宣言で足りる | サイズ不要 |
| `Player* player_`（ポインタ） | 前方宣言で足りる | サイズ不要 |
| 関数引数 `Player& p` | 前方宣言で足りる | サイズ不要 |
| `player.heal(...)` を呼ぶ | `#include "Player.h"` | メンバ関数の定義が必要 |

```
ヘッダで「名前だけ使う」  → 前方宣言
.cpp で「中身を触る」     → #include が必要
```

### 具体例で確認

```cpp
// Apple.h（前方宣言で足りる）
class Player;

class Apple {
public:
    void use(Player& player) const;   // Player の名前だけ使う
};

// Apple.cpp（中身を触るので #include が必要）
#include "Apple.h"
#include "Player.h"   // player.heal() を呼ぶので完全定義が必要

void Apple::use(Player& player) const {
    player.heal(healAmount_);   // Player のメンバ関数を呼ぶ
}
```

---

## 第7章：3つのインスタンス生成パターン

```cpp
// 1. スタック上に直接生成（基本形）
Apple normalApple("Green Apple", 20);

// 2. 波かっこ初期化（同じくスタック、書き方の違い）
Apple anotherApple{"Red Apple", 30};

// 3. ヒープ上に生成（unique_ptr で所有権管理）
auto rareApple = std::make_unique<Apple>("Rare Apple", 35);

// unique_ptr の Apple にアクセスするときは参照外し
hero.pickApple(*rareApple);   // *rareApple → Apple&
```

| パターン | 生成場所 | 寿命管理 | 使い方 |
| :--- | :--- | :--- | :--- |
| `Apple a(...)` | スタック | 自動（スコープ終了で破棄） | 基本 |
| `Apple a{...}` | スタック | 自動（スコープ終了で破棄） | 基本（推奨） |
| `make_unique<Apple>(...)` | ヒープ | `unique_ptr` が管理 | 所有権を明示したいとき |

---

## 理解度チェック

以下のクラス構造について3問答えてほしい。

```cpp
class Engine {
public:
    Engine(int horsepower);
    void start();
    int horsepower() const;
private:
    int horsepower_;
};

class Car {
public:
    Car(std::string model, int hp);
    void drive(Driver& driver);
private:
    std::string model_;
    Engine engine_;        // ← A
};

class Driver {
public:
    void reportSpeed(int speed);
private:
    Car* currentCar_;     // ← B
};
```

**問1：** A の `Engine engine_` と B の `Car* currentCar_` の違いを「実体かポインタか」の観点で説明してほしい。

**問2：** `Car.h` に `Engine` をインクルードする必要があるか、前方宣言で足りるか。同様に `Driver.h` に `Car` のインクルードが必要か、前方宣言で足りるか。

**問3：** `Car::drive(Driver& driver)` の中で `driver.reportSpeed(100)` を呼ぶとき、`Car.h` と `Car.cpp` それぞれに何が必要か。

---

**正解：**

**問1：**

`Engine engine_`（実体）：
- `Car` が作られると同時に `engine_` も作られる
- `Car` が消えると `engine_` も消える
- `Car` の一部として `Engine` を所有している（has-a）

`Car* currentCar_`（ポインタ）：
- `Driver` は `Car` の場所だけ知っている
- `nullptr` かもしれない（車を運転していない状態）
- `Car` の所有権は `Driver` にない。別のところが管理している

---

**問2：**

`Car.h` に `Engine engine_`（実体メンバ）があるので：
→ **`#include "Engine.h"` が必要**。
コンパイラが `Engine` のサイズと構造を知る必要があるため。

`Driver.h` に `Car* currentCar_`（ポインタメンバ）があるので：
→ **前方宣言 `class Car;` で足りる**。
ポインタはサイズが固定なので `Car` の中身を知らなくていい。

---

**問3：**

`Car.h`：
→ **前方宣言 `class Driver;` で足りる**。
`drive(Driver& driver)` の宣言は `Driver` の名前だけ使っているため。

`Car.cpp`：
→ **`#include "Driver.h"` が必要**。
`driver.reportSpeed(100)` を呼ぶには `Driver` のメンバ関数の定義が必要なため。

> **重要な視点：**
> ヘッダは「宣言の場」、cpp は「実装の場」。
> ヘッダは前方宣言で依存を最小化し、
> 実際に中身を触る cpp でのみフルインクルードする。
> これが循環依存を防ぐ基本戦略だ。
