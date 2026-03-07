# 第0章：ゴールを確認しよう

このコースでは、コンソール RPG を題材にして次の設計を段階的に作る。

- `Player` が HP と状態（Fine / Caution / Danger）を管理する
- `Herb` や `Key` などのアイテムを使える
- インベントリを `List<T>` で管理する
- 継承 / ポリモーフィズムで拡張しやすくする
- メモリ管理（GC）とリソース解放（IDisposable）を正しく理解する

## 何を作るのか

最終的に作るイメージは次の通り。

- プレイヤーは HP を持つ
- ハーブを使うと HP が回復する
- HP に応じて状態（Condition）が変わる
- アイテムボックスとインベントリ間でアイテムをやり取りできる

## システム全体の設計図

```mermaid
flowchart LR
    Game["Game / Program"] --> Player["Player<br/>HP・状態を管理"]
    Game --> Inv["Inventory<br/>List&lt;Item&gt;（後半）"]
    Inv --> Herb["GreenHerb / RedHerb"]
    Inv --> Key["Key"]
    Herb --> Player
```

## HP状態（Condition）の遷移

```mermaid
stateDiagram-v2
    [*] --> Fine : 開始（HP 100%）

    Fine --> Caution : HP 比率 <= 0.67
    Caution --> Fine : HP 比率 > 0.67

    Caution --> Danger : HP 比率 <= 0.33
    Danger --> Caution : HP 比率 > 0.33

    Danger --> [*] : HP 0（ゲームオーバー）
```

`Condition` は `Player` 自身が管理する。外部のクラスが `hp` を直接いじる設計にすると、状態更新の漏れが起きやすい。

## ハーブを使ったときの処理フロー

```mermaid
sequenceDiagram
    participant Game as Program
    participant Herb
    participant Player

    Game->>Herb: Use(player)
    Herb->>Player: Heal(30)
    Player->>Player: HP更新 + 状態更新
    Player-->>Herb: 完了
    Herb-->>Game: 完了
```

## 最終プログラムの全体シーケンス

```mermaid
sequenceDiagram
    participant P as Program
    participant Box as ItemBox
    participant Player
    participant item as Item（GreenHerb / RedHerb）

    P->>Box: Store(new GreenHerb())
    P->>Box: Store(new RedHerb())
    P->>Box: Store(new Key("ボスルームの鍵"))
    Note over Box: 3アイテム保管中

    P->>Box: Retrieve(0)
    Box-->>P: GreenHerb
    P->>Player: AddItem(GreenHerb)

    P->>Box: Retrieve(0)
    Box-->>P: RedHerb
    P->>Player: AddItem(RedHerb)
    Note over Box: Key のみ残る（1個）

    P->>Player: Damage(80)
    Player->>Player: HP: 100 → 20 / UpdateCondition()
    Note over Player: HP: 20/100 [Danger]

    P->>Player: UseItem(0)
    activate Player
    Player->>item: Use(player)
    item->>Player: Heal(30)
    Player->>Player: HP: 20 → 50 / UpdateCondition()
    deactivate Player
    Note over Player: HP: 50/100 [Caution]

    P->>Player: UseItem(0)
    activate Player
    Player->>item: Use(player)
    item->>Player: Heal(50)
    Player->>Player: HP: 50 → 100 / UpdateCondition()
    deactivate Player
    Note over Player: HP: 100/100 [Fine]
```

## 最終プログラムのアクティビティ図

```mermaid
flowchart TD
    S([開始]) --> Init["Player・ItemBox を生成"]
    Init --> Store["ItemBox にアイテムを保管<br/>GreenHerb / RedHerb / Key"]
    Store --> Retrieve["GreenHerb・RedHerb を<br/>インベントリへ移す"]
    Retrieve --> PrintBox["ボックス残数を表示<br/>（Key のみ残る）"]
    PrintBox --> Dmg["Damage(80) → HP: 20"]
    Dmg --> C1{HP 比率 ≤ 0.33？}
    C1 -->|はい| D1["Condition: Danger"]
    D1 --> U1["UseItem(0)<br/>GreenHerb.Use(player)"]
    U1 --> H1["Heal(30) → HP: 50"]
    H1 --> C2{HP 比率 > 0.67？}
    C2 -->|いいえ| D2["Condition: Caution"]
    D2 --> U2["UseItem(0)<br/>RedHerb.Use(player)"]
    U2 --> H2["Heal(50) → HP: 100"]
    H2 --> C3{HP 比率 > 0.67？}
    C3 -->|はい| D3["Condition: Fine ✅"]
    D3 --> End([終了])

    style D1 fill:#ffebee,stroke:#c62828
    style D2 fill:#fff8e1,stroke:#f57f17
    style D3 fill:#e8f5e9,stroke:#2e7d32
```

## 学習ロードマップ

```mermaid
flowchart LR
    C0["第0章<br/>ゴール確認"] -->
    C1["第1章<br/>Playerクラス<br/>最小構成"] -->
    C2["第2章<br/>設計を考える<br/>責務分離"] -->
    C3["第3章<br/>Herbクラス<br/>参照型の理解"] -->
    C4["第4章<br/>インベントリ<br/>List"] -->
    C5["第5章<br/>設計の限界<br/>問題提起"] -->
    C6["第6章<br/>ポリモーフィズム<br/>継承/抽象"] -->
    C7["第7章<br/>参照管理<br/>GC / IDisposable"]
```

## 本コースで扱う主要な C# の機能

| 機能 | 内容 |
|---|---|
| `List<T>` | 動的な配列（リスト） |
| `enum` | 列挙型。状態の定義に使用 |
| `.cs` ファイル | 通常はクラス単位でファイルを構成 |
| 参照型（class） | オブジェクトを「参照」として扱う仕組み |
| GC / IDisposable | メモリの自動回収とリソースの明示的解放 |

## まず動かしてみよう

```csharp
using System;

int hp = 100;
int maxHp = 100;

Console.WriteLine($"HP: {hp}/{maxHp}");

hp -= 70; // ダメージ
Console.WriteLine($"HP: {hp}/{maxHp}");

hp += 30; // 回復
if (hp > maxHp) hp = maxHp;
Console.WriteLine($"HP: {hp}/{maxHp}");
```

この段階ではまだクラスを使っていない。第1章で `Player` クラスを作って、HPと状態管理をまとめる。
