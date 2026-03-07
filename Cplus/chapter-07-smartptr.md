# 第7章：アイテムボックスと所有権

---

## 7-1 前章（6.5章）の振り返りと残った問い

第6.5章で確認したこと。

- `vector<Item>` → オブジェクトスライシングで使えない
- `vector<Item*>` → ポリモーフィズムは動くが、所有権が不明確・メモリリーク・二重解放のリスクがある

残った問いはこれだ。

> 「ポインタを使えば動く。
> でも、**誰がオブジェクトを delete する責任を持つか** を明確にできないか？」

```mermaid
flowchart LR
    V2["vector&lt;Item*&gt;<br/>（生ポインタ）"]
    V3["vector&lt;unique_ptr&lt;Item&gt;&gt;<br/>（スマートポインタ）"]

    P2["⚠️ ポリモーフィズムは動く<br/>所有権が不明確・リーク危険"]
    P3["✅ ポリモーフィズムも動く<br/>所有権が明確・自動解放"]

    V2 --- P2
    V3 --- P3

    V2 -->|"この章で解決"| V3

    style P2 fill:#fff8e1,stroke:#f57f17
    style P3 fill:#e8f5e9,stroke:#2e7d32
```

これを解決するのが **`std::unique_ptr`**（スマートポインタ）だ。

---

## 7-2 `std::unique_ptr`：所有権を明確にするスマートポインタ

`std::unique_ptr` は「オブジェクトを唯一所有するポインタ」だ。

```mermaid
flowchart LR
    subgraph unique_ptr["std::unique_ptr&lt;GreenHerb&gt; herb"]
        P["ポインタ<br/>（所有権あり）"]
    end

    subgraph heap["ヒープ"]
        O["GreenHerb オブジェクト"]
    end

    P -->|"所有"| O

    note1["unique_ptr が破棄されると<br/>オブジェクトも自動で delete される"]
```

**スマートポインタの3つの特性：**

| 特性 | 内容 |
|---|---|
| 自動解放 | スコープを抜けると自動で `delete`（RAII）|
| 唯一の所有者 | コピー不可。所有者は常に1つ |
| 所有権の移譲 | `std::move` で所有権を別の `unique_ptr` に渡せる |

---

## 7-3 所有権の状態遷移

```mermaid
stateDiagram-v2
    direction LR

    [*] --> 所有中 : make_unique() で作成
    所有中 --> 移譲済み : std::move() で手放す
    移譲済み --> 所有中 : 受け取り側が所有
    所有中 --> [*] : スコープ終了 or reset() → 自動 delete
    移譲済み --> [*] : nullptr のまま破棄（delete なし）
```

`std::move` は「所有権の引き渡し」だ。
引き渡した後の `unique_ptr` は `nullptr` になる。**コピーではない。**

```mermaid
flowchart LR
    A["herb<br/>GreenHerbを所有"]
    B["std::move(herb)"]
    C["inventory<br/>GreenHerbを所有"]
    D["herb<br/>nullptr（何も持っていない）"]

    A -->|"移譲"| B --> C
    A -.->|"移譲後"| D

    style D fill:#f5f5f5,stroke:#9e9e9e
```

---

## 7-4 `make_unique` と基本操作

### オブジェクトの生成

```cpp
// new を使う書き方（非推奨）
Item* raw = new GreenHerb();   // 手動 delete が必要

// make_unique を使う書き方（推奨）
auto herb = std::make_unique<GreenHerb>();  // 自動管理
```

`std::make_unique<T>(args...)` はヒープに `T` 型オブジェクトを作り、
それを所有する `unique_ptr<T>` を返す。

### 主な操作

```cpp
auto herb = std::make_unique<GreenHerb>();

// メンバにアクセス（-> を使う）
herb->use(player);

// 生ポインタを借りる（所有権は移らない）
Item* raw = herb.get();

// 所有権を手放す
auto other = std::move(herb);  // herb は nullptr に
```

---

## 7-5 アイテムボックスの設計

「預ける・引き出す」操作を通じて所有権の移譲を体験する。

```mermaid
flowchart LR
    subgraph Inventory["インベントリ（Player が所有）"]
        I0["unique_ptr&lt;Item&gt;"]
        I1["unique_ptr&lt;Item&gt;"]
    end

    subgraph Box["アイテムボックス（Box が所有）"]
        B0["unique_ptr&lt;Item&gt;"]
        B1["unique_ptr&lt;Item&gt;"]
    end

    I0 -->|"store: 所有権を Box へ移譲"| B0
    B1 -->|"retrieve: 所有権を Inventory へ移譲"| I1
```

所有者は常に1つ。インベントリが持っているとき、ボックスは持っていない。
ボックスへ移したとき、インベントリからは消える。

### クラス図

```mermaid
classDiagram
    class Player {
        -vector~unique_ptr~Item~~ inventory
        +addItem(unique_ptr~Item~ item)
        +removeItem(int index) unique_ptr~Item~
        +useItem(int index)
        +heal(int amount)
        +damage(int amount)
    }

    class ItemBox {
        -vector~unique_ptr~Item~~ items
        +store(unique_ptr~Item~ item)
        +retrieve(int index) unique_ptr~Item~
        +getCount() int
    }

    class Item {
        <<abstract>>
        +use(Player player)*
        +~Item()
    }

    Player "1" o-- "*" Item : inventory（所有）
    ItemBox "1" o-- "*" Item : items（所有）
```

---

## 7-6 実装コード

### `Player.h`（最終版）

```cpp
#pragma once
#include <vector>
#include <memory>
#include "Item/Item.h"

enum class Condition { Fine, Caution, Danger };

class Player {
public:
    void heal(int amount);
    void damage(int amount);
    void updateCondition();

    void                  addItem(std::unique_ptr<Item> item);
    std::unique_ptr<Item> removeItem(int index);
    void                  useItem(int index);

    int       getHp()        const;
    int       getMaxHp()     const;
    Condition getCondition() const;
    int       itemCount()    const;

private:
    int                                hp        = 100;
    int                                maxHp     = 100;
    Condition                          condition = Condition::Fine;
    std::vector<std::unique_ptr<Item>> inventory;
};
```

### `Player.cpp`（最終版・抜粋）

```cpp
#include "Player.h"

// ... heal / damage / updateCondition は第1章から変わらない ...

void Player::addItem(std::unique_ptr<Item> item) {
    inventory.push_back(std::move(item));  // 所有権を vector へ
}

std::unique_ptr<Item> Player::removeItem(int index) {
    if (index < 0 || index >= static_cast<int>(inventory.size())) {
        return nullptr;
    }
    auto item = std::move(inventory[index]);  // 所有権を取り出す
    inventory.erase(inventory.begin() + index);
    return item;                              // 呼び出し元へ所有権を渡す
}

void Player::useItem(int index) {
    if (index < 0 || index >= static_cast<int>(inventory.size())) return;
    inventory[index]->use(*this);            // -> でアクセス
    inventory.erase(inventory.begin() + index);  // unique_ptr が破棄 → 自動 delete
}

int Player::itemCount() const {
    return static_cast<int>(inventory.size());
}
```

### `ItemBox.h`

```cpp
#pragma once
#include <vector>
#include <memory>
#include "Item/Item.h"

class ItemBox {
public:
    void                  store(std::unique_ptr<Item> item);
    std::unique_ptr<Item> retrieve(int index);
    int                   getCount() const;

private:
    std::vector<std::unique_ptr<Item>> items;
};
```

### `ItemBox.cpp`

```cpp
#include "ItemBox.h"

void ItemBox::store(std::unique_ptr<Item> item) {
    items.push_back(std::move(item));
}

std::unique_ptr<Item> ItemBox::retrieve(int index) {
    if (index < 0 || index >= static_cast<int>(items.size())) {
        return nullptr;
    }
    auto item = std::move(items[index]);
    items.erase(items.begin() + index);
    return item;
}

int ItemBox::getCount() const {
    return static_cast<int>(items.size());
}
```

### `main.cpp`（アイテムボックス動作確認）

```cpp
#include <iostream>
#include <string>
#include "Player.h"
#include "ItemBox.h"
#include "Item/Herb/Herb.h"
#include "Item/Key/Key.h"

std::string conditionName(Condition c) {
    switch (c) {
        case Condition::Fine:   return "Fine";
        case Condition::Caution: return "Caution";
        case Condition::Danger: return "Danger";
    }
    return "Unknown";
}

void printStatus(const Player& p) {
    std::cout << "HP: " << p.getHp() << "/" << p.getMaxHp()
              << "  [" << conditionName(p.getCondition()) << "]"
              << "  所持アイテム数: " << p.itemCount()
              << std::endl;
}

int main() {
    Player  player;
    ItemBox box;

    // ボックスにアイテムを預ける
    box.store(std::make_unique<GreenHerb>());
    box.store(std::make_unique<RedHerb>());
    box.store(std::make_unique<Key>("ボスルームの鍵"));

    std::cout << "--- ボックス: " << box.getCount() << "個" << std::endl;

    // ボックスから引き出してインベントリへ
    player.addItem(box.retrieve(0));  // GreenHerb を取り出す
    player.addItem(box.retrieve(0));  // RedHerb を取り出す（0番に詰まる）

    std::cout << "--- ボックス: " << box.getCount() << "個（鍵だけ残る）" << std::endl;

    // 戦闘
    player.damage(80);
    printStatus(player);  // HP: 20/100  [Danger]  所持: 2

    // アイテムを使う
    player.useItem(0);    // GreenHerb 使用
    printStatus(player);  // HP: 50/100  [Caution]  所持: 1

    player.useItem(0);    // RedHerb 使用
    printStatus(player);  // HP: 100/100  [Fine]   所持: 0

    // インベントリから1つボックスへ戻す（今は空なので試せないがコードは動く）
    // auto item = player.removeItem(0);
    // box.store(std::move(item));

    return 0;
}
```

**期待される出力：**
```
--- ボックス: 3個
--- ボックス: 1個（鍵だけ残る）
HP: 20/100  [Danger]  所持アイテム数: 2
HP: 50/100  [Caution]  所持アイテム数: 1
HP: 100/100  [Fine]   所持アイテム数: 0
```

---

## 7-7 全体の処理シーケンス

```mermaid
sequenceDiagram
    participant main
    participant itemBox as ItemBox
    participant player as Player
    participant herb as GreenHerb（ヒープ）

    main  ->> itemBox    : store(make_unique&lt;GreenHerb&gt;())
    note right of itemBox : itemBox が所有

    main  ->> itemBox    : retrieve(0)
    itemBox   -->> main  : unique_ptr（所有権を返す）
    main  ->> player : addItem(std::move(item))
    note right of player : player が所有

    main  ->> player : useItem(0)
    activate player
    player ->> herb  : use(*this) → heal(30)
    player ->> player: erase → unique_ptr 破棄
    note right of herb : 自動 delete ✅
    deactivate player
```

---

## 7-8 `unique_ptr` をコピーしようとすると

```cpp
auto a = std::make_unique<GreenHerb>();
auto b = a;              // ❌ コンパイルエラー：コピー不可
auto b = std::move(a);  // ✅ 移譲は可能。a は nullptr に
```

```mermaid
flowchart LR
    subgraph コピー試み["auto b = a（❌ コンパイルエラー）"]
        CA["a → GreenHerb"]
        CB["b → GreenHerb（コピー）"]
        CA -. "2人が同じものを所有？<br/>どちらが delete する？<br/>→ 禁止" .-> CB
    end

    subgraph 移譲["auto b = std::move(a)（✅）"]
        MA["a → nullptr"]
        MB["b → GreenHerb（所有）"]
        MA -->|"所有権を渡す"| MB
    end
```

「2人が同じものを所有する」状態は、二重 delete を引き起こすので禁止されている。
所有者は常に1人。これが `unique_ptr` の根本的な設計だ。

---

## 7-9 設計の全体像（最終完成形）

```mermaid
classDiagram
    class Player {
        -int hp
        -int maxHp
        -Condition condition
        -vector~unique_ptr~Item~~ inventory
        +heal(int amount)
        +damage(int amount)
        +addItem(unique_ptr~Item~)
        +removeItem(int) unique_ptr~Item~
        +useItem(int)
    }

    class ItemBox {
        -vector~unique_ptr~Item~~ items
        +store(unique_ptr~Item~)
        +retrieve(int) unique_ptr~Item~
    }

    class Item {
        <<abstract>>
        +use(Player player)*
        +~Item()
    }

    class Herb {
        -int healAmount
        +use(Player player)
    }

    class GreenHerb
    class RedHerb

    class Key {
        -string keyId
        +use(Player player)
    }

    Item <|-- Herb
    Herb <|-- GreenHerb
    Herb <|-- RedHerb
    Item <|-- Key
    Player "1" o-- "*" Item
    ItemBox "1" o-- "*" Item
```

---

## 7-10 確認問題

1. `unique_ptr` が「コピー不可」に設計されている理由を説明せよ。
   「二重 delete」「所有者は1人」という言葉を使うこと。

2. 次のコードはメモリリークするか。理由も答えよ。
   ```cpp
   {
       auto herb = std::make_unique<GreenHerb>();
   }  // ← ここで何が起きる？
   ```

3. 次のコードはコンパイルできるか。できない場合、どう直せばよいか。
   ```cpp
   auto a = std::make_unique<GreenHerb>();
   player.addItem(a);
   ```

4. `removeItem()` の実装で `std::move` を使っている。
   使わずに `return inventory[index];` と書いた場合、何が起きるか？

5. `ItemBox` と `Player` が同じ `GreenHerb` を同時に所有することはできるか。
   なぜそういう設計になっているのか説明せよ。

---

## まとめ

```mermaid
mindmap
    root((第7章まとめ))
        unique_ptr
            唯一の所有者
            スコープ終了で自動 delete
            RAII の実践
            コピー不可・移譲のみ
        make_unique
            new の代わり
            例外安全
        std::move
            コピーではなく移譲
            移譲後は nullptr
        所有権設計
            誰が持つか明確にする
            アイテムボックスで体験
            Player ↔ ItemBox 間の移譲
```

---

## コース全体の振り返り

```mermaid
flowchart LR
    C0["第0章<br/>ゴール確認"]
    C1["第1章<br/>Playerクラス<br/>カプセル化"]
    C2["第2章<br/>責務分離<br/>設計の原則"]
    C3["第3章<br/>Herbクラス<br/>参照渡し"]
    C4["第4章<br/>インベントリ<br/>vector"]
    C5["第5章<br/>設計の限界<br/>問題発見"]
    C6["第6章<br/>ポリモーフィズム<br/>仮想関数"]
    C65["第6.5章<br/>抽象クラスと<br/>inventory設計"]
    C7["第7章<br/>所有権<br/>unique_ptr"]

    C0 --> C1 --> C2 --> C3 --> C4 --> C5 --> C6 --> C65 --> C7

    style C0 fill:#4CAF50,color:#fff
    style C65 fill:#FF9800,color:#fff
    style C7 fill:#9C27B0,color:#fff
```

| 章 | 習得した概念 |
|:--:|---|
| 第1章 | クラス、カプセル化、enum class |
| 第2章 | 責務分離、「状態を持つ者が管理する」|
| 第3章 | 参照渡し（`&`）、循環依存、前方宣言 |
| 第4章 | `std::vector`、イテレータ、`*this`|
| 第5章 | 設計の限界を問いで発見する力 |
| 第6章 | 継承、仮想関数、ポリモーフィズム、vtable |
| 第6.5章 | 抽象クラス、オブジェクトスライシング、生ポインタの問題 |
| 第7章 | スマートポインタ、所有権、`std::move`|

このコースで実装した「バイオ風ハーブ回復システム」は、
現代 C++ のエッセンス ── **カプセル化・責務分離・多態性・所有権** ── を全て含んでいる。

同じ設計の考え方は、ゲーム以外のソフトウェアでも変わらず通用する。
