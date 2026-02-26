# 第5章：設計の限界（C#版）

## 5-1 現状を確認する

第4章の `Player` は `List<Herb>` を持っている。
これは「ハーブだけ」の世界では十分だが、ゲームとしてはすぐ限界が来る。

```mermaid
flowchart LR
    Player["Player"] --> Inv["List&lt;Herb&gt; inventory"] --> Herb["Herb"]
```

## 5-2 問い①：ハーブが複数種類になったら

`GreenHerb`, `RedHerb`, `MixedHerb` のように種類が増えるとどうなるか。

- `Herb` を汎用クラスとしてパラメータで表現する方法はある
- でも「回復アイテム以外」が入った瞬間に破綻する

```csharp
// まだ Herb 系だけなら何とかなる
var inventory = new List<Herb>();
inventory.Add(new Herb(30));
inventory.Add(new Herb(60));
```

## 5-3 問い②：Key（鍵）を追加したら

`Key` は `Herb` ではないので、`List<Herb>` に入らない。

```csharp
var inventory = new List<Herb>();
// inventory.Add(new Key("BossRoom")); // コンパイルエラー
```

```mermaid
flowchart TD
    A["List&lt;Herb&gt;"] --> B{"追加したいものは Herb ?"}
    B -- Yes --> C["入る"]
    B -- No（Key など） --> D["入らない ❌"]
```

## 5-4 ダメな解決策 A：リストを増やす

`List<Herb>` と `List<Key>` を別々に持つ案。

```mermaid
flowchart LR
    Player --> H["List&lt;Herb&gt;"]
    Player --> K["List&lt;Key&gt;"]
    Player --> M["List&lt;MixedHerb&gt;"]
```

問題点。

- コレクションが分散する
- 「インベントリの 0 番目を使う」が表現しづらい
- UI/並び順/共通操作の実装が複雑になる

## 5-5 ダメな解決策 B：`enum` と分岐で管理する

1つの `struct` / `class` に全部押し込んで、`Type` で分岐する案。

```csharp
public enum ItemType { GreenHerb, RedHerb, MixedHerb, Key }

public class ItemData
{
    public ItemType Type;
    public int HealAmount;   // Key には無意味
    public string? KeyId;    // Herb には無意味
}
```

```mermaid
flowchart LR
    AddItem["新アイテム追加"] --> Switch["switch(Type) の修正"]
    Switch --> Use["Use処理の修正"]
    Switch --> UI["表示処理の修正"]
```

問題点。

- 無意味なフィールドが増える
- 分岐が増え続ける
- 新しい型の追加に弱い

## 5-6 本質的な問題の整理

欲しいのは「型が違っても同じ操作で扱えること」。

- どのアイテムも `Use(Player)` を持つ
- インベントリは一つで管理したい
- 新しいアイテムを増やしても `switch` を増やしたくない

```mermaid
flowchart TD
    A["本質的な問題"] --> B["異なる型を1つのコレクションで管理したい"]
    B --> C["共通の型（抽象/インターフェース）が必要"]
```

## 5-7 本当に欲しいものは何か

```mermaid
flowchart LR
    Inv["inventory"] --> G["GreenHerb"]
    Inv --> R["RedHerb"]
    Inv --> K["Key"]
    Inv --> M["MixedHerb"]
```

この構造を C# で自然に表現するには、基底クラスまたはインターフェースを使う。

## 5-8 解決策の方向性

候補は2つ。

- `abstract class Item` を作る
- `interface IItem` を作る

このコースでは、共通のデータや既定動作も持ちやすいので `abstract class Item` を採用する。

```mermaid
classDiagram
    class Item {
        <<abstract>>
        +Use(Player player)*
    }
    class GreenHerb
    class RedHerb
    class Key

    Item <|-- GreenHerb
    Item <|-- RedHerb
    Item <|-- Key
```

## 5-9 3つの設計の比較まとめ

```mermaid
flowchart TB
    A["現設計<br/>List&lt;Herb&gt;"] --> A1["シンプルだが Herb 以外を入れられない"]
    B["ダメ解決策A<br/>型ごとに List"] --> B1["管理が分散"]
    C["ダメ解決策B<br/>enum + switch"] --> C1["分岐が肥大化"]
    D["次章の設計<br/>List&lt;Item&gt; + 継承"] --> D1["統一管理 + 拡張しやすい"]
```

## 5-10 確認問題

1. `List<Herb>` に `Key` を入れられない理由は何か。
2. 型ごとに別リストを持つ案の問題点を2つ挙げよ。
3. `enum + switch` 案が拡張に弱い理由は何か。

## まとめ

- `List<Herb>` の限界は「型の固定」にある
- 欲しいのは「共通操作で扱える抽象化」
- 次章で `Item` を導入し、ポリモーフィズムに進む
