# 第4章：インベントリを作ろう

---

## 4-1 インベントリとは何か

ゲームにおける「インベントリ」は、プレイヤーが持ち歩けるアイテムの袋だ。

今回実装するのはこういう動作だ。

```
インベントリ: [グリーンハーブ(30), グリーンハーブ(30), グリーンハーブ(30)]

useItem(0) を呼ぶ
→ HP が回復
→ 使ったアイテムはインベントリから消える

インベントリ: [グリーンハーブ(30), グリーンハーブ(30)]
```

これを実現するために `std::vector` を使う。

---

## 4-2 `std::vector` とは何か

`std::vector` は「サイズが可変な配列」だ。
普通の配列は宣言時にサイズを固定する必要があるが、`vector` は後から要素を追加・削除できる。

```mermaid
flowchart LR
    subgraph 普通の配列["普通の配列（固定サイズ）"]
        direction LR
        A0["[0]"] --- A1["[1]"] --- A2["[2]"]
        note1["宣言時に<br/>サイズ確定<br/>変えられない"]
    end

    subgraph vector["std::vector（可変サイズ）"]
        direction LR
        V0["[0]"] --- V1["[1]"] --- V2["[2]"] --- VA["..."]
        note2["push_back で<br/>追加できる<br/>erase で削除できる"]
    end
```

### 主な操作

| 操作 | コード例 | 意味 |
|---|---|---|
| 追加 | `v.push_back(x)` | 末尾に要素を追加 |
| アクセス | `v[i]` | i番目の要素を取得（0始まり）|
| サイズ | `v.size()` | 要素数を返す |
| 削除 | `v.erase(v.begin() + i)` | i番目の要素を削除 |
| 空チェック | `v.empty()` | 要素が0個なら `true` |

---

## 4-3 インデックスは 0 から始まる

```mermaid
flowchart LR
    subgraph inventory["std::vector&lt;Herb&gt; inventory"]
        direction LR
        I0["[0]<br/>GreenHerb<br/>heal=30"]
        I1["[1]<br/>GreenHerb<br/>heal=30"]
        I2["[2]<br/>GreenHerb<br/>heal=30"]
        I0 --- I1 --- I2
    end

    S["size() = 3"]
    inventory --- S
```

3つ入っているとき：
- 最初の要素は `inventory[0]`
- 最後の要素は `inventory[2]`（= `inventory[size()-1]`）
- `inventory[3]` は **存在しない** → アクセスすると未定義動作（クラッシュ等）

---

## 4-4 `push_back` と `erase` のイメージ

### `push_back`

```mermaid
flowchart LR
    subgraph before["push_back 前"]
        direction LR
        B0["[0] herb"] --- B1["[1] herb"]
    end

    subgraph after["push_back(herb) 後"]
        direction LR
        A0["[0] herb"] --- A1["[1] herb"] --- A2["[2] herb（新しく追加）"]
    end

    before -->|"push_back(herb)"| after
```

### `erase`（index 1 を削除する場合）

```plantuml
@startuml
title erase(begin() + 1) のイメージ

rectangle "削除前" {
    rectangle "[0] GreenHerb" as E0 #lightblue
    rectangle "[1] GreenHerb" as E1 #ffcccc
    rectangle "[2] GreenHerb" as E2 #lightblue
}

rectangle "削除後" {
    rectangle "[0] GreenHerb" as A0 #lightblue
    rectangle "[1] GreenHerb" as A1 #lightblue
    note right of A1 : [2] が [1] に\nシフトされる
}

E1 -down-> A0 : 削除対象（消える）
E2 -down-> A1 : 後ろの要素が前に詰まる

@enduml
```

`erase()` に渡すのは「インデックスの数字」ではなく **イテレータ** だ。
`inventory.begin()` が先頭を指すイテレータ。そこに `+ index` して目的の位置を指す。

```cpp
inventory.erase(inventory.begin() + 1);  // index 1 を削除
```

---

## 4-5 設計：Playerクラスに何を追加するか

```mermaid
classDiagram
    class Player {
        -int hp
        -int maxHp
        -Condition condition
        -vector~Herb~ inventory

        +heal(int amount) void
        +damage(int amount) void
        +updateCondition() void

        +addItem(Herb herb) void
        +useItem(int index) void
        +discardItem(int index) void

        +getHp() int
        +getMaxHp() int
        +getCondition() Condition
        +getInventory() vector~Herb~
    }

    class Herb {
        +int healAmount
        +use(Player player) void
    }

    Player "1" o-- "*" Herb : inventory
    Herb ..> Player : heal() を呼ぶ
```

`Player` が `Herb` を持ち、`Herb` が `Player` を使う。
この **双方向の関係** は、ファイルのインクルードで問題を起こす可能性がある。
後ほど詳しく説明する。

---

## 4-6 `useItem()` の処理フロー

```mermaid
flowchart TD
    A["useItem(int index) 呼び出し"]
    B{index が<br/>有効範囲内か？}
    C["何もしない（return）"]
    D["inventory[index].use(*this)"]
    E["inventory.erase(inventory.begin() + index)"]
    F["終了"]

    A --> B
    B -- "範囲外<br/>(index < 0 または index >= size)" --> C
    B -- "有効" --> D --> E --> F
```

### `*this` について

`use()` は `Player&` を引数に取る。
`useItem()` は `Player` クラスの中にあるので、自分自身を渡すには `*this` と書く。

```mermaid
flowchart LR
    subgraph Player クラスの内側
        A["this<br/>（Playerオブジェクトへのポインタ）"]
        B["*this<br/>（ポインタを参照に変換）"]
        A -->|"* で参照を取り出す"| B
    end

    B -->|"Player& として渡せる"| C["Herb::use(Player& player)"]
```

| 書き方 | 型 | 意味 |
|:--:|:--:|---|
| `this` | `Player*` | 自分自身へのポインタ |
| `*this` | `Player` | ポインタを参照外しした実体 |
| `Player&` への引数 | `Player&` | `*this` を渡せば自動的に参照になる |

---

## 4-7 循環依存の問題と解消

`Player.h` が `Herb.h` をインクルードし、
`Herb.h` が `Player.h` をインクルードすると循環する。

```mermaid
flowchart LR
    A["Player.h"] -->|"#include Herb.h"| B["Herb.h"]
    B -->|"#include Player.h"| A

    style A fill:#ffebee,stroke:#c62828
    style B fill:#ffebee,stroke:#c62828
```

これはコンパイルエラーになる。

**解決策：前方宣言（Forward Declaration）**

`Herb.h` で `Player` の定義が必要な部分（`use()` の実装）を `.cpp` に移動する。
ヘッダーには「`Player` というクラスが存在する」という宣言だけ書けばよい。

```mermaid
flowchart LR
    A["Player.h"] -->|"#include Herb.h"| B["Herb.h<br/>（前方宣言のみ）"]
    C["Herb.cpp"] -->|"#include Player.h"| A
    C -->|"#include Herb.h"| B

    style B fill:#e8f5e9,stroke:#2e7d32
```

`Herb.h` はもう `Player.h` をインクルードしないので、循環が消える。

---

## 4-8 実装コード

### `Herb.h`（前方宣言版）

```cpp
#pragma once

class Player;  // ← 前方宣言：「Playerというクラスがある」とだけ伝える

struct Herb {
    int healAmount = 30;
    void use(Player& player);  // 宣言だけ。実装は Herb.cpp へ
};
```

### `Herb.cpp`（実装）

```cpp
#include "Herb.h"
#include "Player.h"  // ← ここで初めて Player の全定義を取り込む

void Herb::use(Player& player) {
    player.heal(healAmount);
}
```

### `Player.h`（更新版）

```cpp
#pragma once
#include <vector>
#include "Herb.h"  // Herb の定義が必要（Herb.h は Player を前方宣言するだけなので循環しない）

enum class Condition {
    Fine,
    Middle,
    Danger,
};

class Player {
public:
    void heal(int amount);
    void damage(int amount);
    void updateCondition();

    void addItem(Herb herb);
    void useItem(int index);
    void discardItem(int index);

    int                  getHp()        const;
    int                  getMaxHp()     const;
    Condition            getCondition() const;
    const std::vector<Herb>& getInventory() const;

private:
    int                hp        = 100;
    int                maxHp     = 100;
    Condition          condition = Condition::Fine;
    std::vector<Herb>  inventory;
};
```

### `Player.cpp`（更新版）

```cpp
#include "Player.h"

void Player::heal(int amount) {
    hp += amount;
    if (hp > maxHp) hp = maxHp;
    updateCondition();
}

void Player::damage(int amount) {
    hp -= amount;
    if (hp < 0) hp = 0;
    updateCondition();
}

void Player::updateCondition() {
    float ratio = static_cast<float>(hp) / maxHp;
    if      (ratio > 0.67f) condition = Condition::Fine;
    else if (ratio > 0.33f) condition = Condition::Middle;
    else                    condition = Condition::Danger;
}

void Player::addItem(Herb herb) {
    inventory.push_back(herb);
}

void Player::useItem(int index) {
    if (index < 0 || index >= static_cast<int>(inventory.size())) {
        return;
    }
    inventory[index].use(*this);
    inventory.erase(inventory.begin() + index);
}

void Player::discardItem(int index) {
    if (index < 0 || index >= static_cast<int>(inventory.size())) {
        return;
    }
    inventory.erase(inventory.begin() + index);
}

int                       Player::getHp()        const { return hp; }
int                       Player::getMaxHp()     const { return maxHp; }
Condition                 Player::getCondition() const { return condition; }
const std::vector<Herb>&  Player::getInventory() const { return inventory; }
```

### `main.cpp`（動作確認）

```cpp
#include <iostream>
#include "Player.h"

std::string conditionName(Condition c) {
    switch (c) {
        case Condition::Fine:   return "Fine";
        case Condition::Middle: return "Middle";
        case Condition::Danger: return "Danger";
    }
    return "Unknown";
}

void printStatus(const Player& p) {
    std::cout << "HP: " << p.getHp() << "/" << p.getMaxHp()
              << "  [" << conditionName(p.getCondition()) << "]"
              << "  アイテム数: " << p.getInventory().size()
              << std::endl;
}

int main() {
    Player player;

    // アイテムを3つ追加
    Herb herb1; herb1.healAmount = 30;
    Herb herb2; herb2.healAmount = 30;
    Herb herb3; herb3.healAmount = 30;

    player.addItem(herb1);
    player.addItem(herb2);
    player.addItem(herb3);

    player.damage(75);
    printStatus(player);    // HP: 25/100  [Danger]  アイテム数: 3

    player.useItem(0);      // index 0 を使う
    printStatus(player);    // HP: 55/100  [Middle]  アイテム数: 2

    player.useItem(0);
    printStatus(player);    // HP: 85/100  [Fine]    アイテム数: 1

    player.discardItem(0);  // 捨てる
    printStatus(player);    // HP: 85/100  [Fine]    アイテム数: 0

    player.useItem(0);      // 存在しないindex → 何も起きない
    printStatus(player);    // HP: 85/100  [Fine]    アイテム数: 0

    return 0;
}
```

**期待される出力：**
```
HP: 25/100  [Danger]  アイテム数: 3
HP: 55/100  [Middle]  アイテム数: 2
HP: 85/100  [Fine]    アイテム数: 1
HP: 85/100  [Fine]    アイテム数: 0
HP: 85/100  [Fine]    アイテム数: 0
```

---

## 4-9 全体のシーケンス図

```mermaid
sequenceDiagram
    participant main
    participant Player
    participant Herb

    main  ->> Player : addItem(herb) × 3
    main  ->> Player : damage(75)
    note right of Player: hp=25, condition=Danger<br/>inventory=[herb, herb, herb]

    main  ->> Player : useItem(0)
    activate Player
    Player ->> Herb  : use(*this)
    activate Herb
    Herb  ->> Player : heal(30)
    note right of Player: hp=55, condition=Middle
    Herb -->> Player : 完了
    deactivate Herb
    Player ->> Player : erase(begin() + 0)
    note right of Player: inventory=[herb, herb]
    Player -->> main : 完了
    deactivate Player
```

---

## 4-10 ファイル構成のまとめ

```mermaid
flowchart TB
    main["main.cpp<br/>#include Player.h"]
    playerH["Player.h<br/>#include Herb.h"]
    playerCPP["Player.cpp<br/>#include Player.h"]
    herbH["Herb.h<br/>class Player; （前方宣言）"]
    herbCPP["Herb.cpp<br/>#include Herb.h<br/>#include Player.h"]

    main --> playerH
    playerH --> herbH
    playerCPP --> playerH
    herbCPP --> herbH
    herbCPP --> playerH

    style herbH fill:#fff3e0,stroke:#f57c00
```

循環が消えている。`Herb.h` は `Player.h` をインクルードしていない。

---

## 4-11 確認問題

1. `inventory[3]` を 3要素のvectorに対して呼び出すと何が起きる可能性があるか？

2. `useItem()` で `erase()` を呼ぶとき、なぜ `inventory.begin() + index` という書き方をするのか？

3. `getInventory()` の戻り値型が `const std::vector<Herb>&` になっている理由は何か。
   `std::vector<Herb>` （`const` なし・`&` なし）と何が違うか？

4. `*this` を使わず、次のように書くことはできるか？その理由は？
   ```cpp
   inventory[index].use(this);
   ```

---

## まとめと次章への橋渡し

```mermaid
mindmap
    root((第4章まとめ))
        std::vector
            可変サイズの配列
            push_back で追加
            erase で削除
            インデックスは 0 始まり
        useItem の設計
            境界チェック
            use と erase をセットで
        this と *this
            this はポインタ
            *this は実体への参照
        前方宣言
            循環依存を避ける
            宣言と実装を分離
```

ここで一度立ち止まって考えよう。

今の `Player` は `std::vector<Herb>` を持っている。
これで「ハーブを複数持てる」ようになった。

でも、**問題がある。**

> 「鍵（`Key`）のようなアイテムを追加したくなったら、どうする？」

`std::vector<Herb>` の中に `Key` は入らない。
`Herb` 専用のコンテナだからだ。

次の第5章では、この設計の「限界」に正面から向き合う。
