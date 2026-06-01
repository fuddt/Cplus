# 第2章：設計を考える

---

## 2-1 問いかけ

第1章で `Player` クラスを作った。`heal()` という関数があり、HP変更はそこを通じて行う。

ここで一度立ち止まって考えよう。

---

> **「なぜ `heal()` を経由しないといけないのか？**
> **`player.hp += 30` と直接書いてはダメなのか？」**

---

答えだけ先に言うとこうだ。

> **「ダメではない。でも、設計として壊れやすい。」**

なぜ壊れやすいのか。具体的に見ていく。

---

## 2-2 ダメな設計：ロジックが散らばる

「直接変える」設計でコードを書いたとする。

```cpp
// グリーンハーブを使うとき
player.hp += 30;

// スーパーハーブを使うとき
player.hp += 100;

// スキルで回復するとき
player.hp += 20;
```

これの何が問題か？

```mermaid
flowchart TD
    A["Herb"] -->|"player.hp += 30"| P1["Player.hp 直接変更"]
    B["SuperHerb"] -->|"player.hp += 100"| P2["Player.hp 直接変更"]
    C["Skill"] -->|"player.hp += 20"| P3["Player.hp 直接変更"]

    P1 --> X["❌ 状態更新（updateCondition）忘れ"]
    P2 --> Y["❌ maxHp 超過"]
    P3 --> Z["❌ 将来の毒判定漏れ"]

    style X fill:#ffebee,stroke:#e53935
    style Y fill:#ffebee,stroke:#e53935
    style Z fill:#ffebee,stroke:#e53935
```

3種類の「回復する処理」が、バラバラな場所に書かれている。

- `updateCondition()` を呼び忘れると、HPが変わっても表示状態が変わらない
- `maxHp` のチェックを忘れると、HP が 130/100 になる
- 将来「毒状態なら回復半減」という仕様が追加されたとき、**3か所全部を書き直す**必要がある

「今は動く。でも、後で必ず壊れる。」

---

## 2-3 王道設計：Playerに集約する

正しい設計はこうだ。

```mermaid
flowchart TD
    A["Herb"]      -->|"player.heal(30)"| B["Player.heal()"]
    C["SuperHerb"] -->|"player.heal(100)"| B
    D["Skill"]     -->|"player.heal(20)"| B

    B --> E["hp を加算"]
    E --> F{hp > maxHp ?}
    F -- "Yes" --> G["hp = maxHp"]
    F -- "No"  --> H["そのまま"]
    G --> I["updateCondition()"]
    H --> I
    I --> J["Condition 確定"]

    style A fill:#e1f5fe,stroke:#0288d1
    style C fill:#e1f5fe,stroke:#0288d1
    style D fill:#e1f5fe,stroke:#0288d1
    style B fill:#fff3e0,stroke:#f57c00
```

**全ての回復処理が `heal()` という1つの窓口を通る。**

- 上限チェック：`heal()` の中で **1回だけ** 書けばいい
- 状態更新：`heal()` が **必ず** 呼んでくれる
- 仕様変更：`heal()` を **1か所だけ** 直せばいい

---

## 2-4 設計の原則

このような考え方をひとことで表すとこうなる。

---

> ### 「状態を持つクラスが、その状態変更の責任を持つ」

---

`hp` を持っているのは `Player` だ。
だから HP を変えるロジックも `Player` が持つべきだ。

`Herb` は「回復量」というデータを持っている。
でも「HPをどう変えるか」は知らなくていい。
知っているのは `Player` だけでいい。

---

## 2-5 シーケンス図で比較する

### ダメな設計：Herb が Player の内部に直接触る

```mermaid
sequenceDiagram
    participant ゲーム
    participant Herb
    participant Player

    ゲーム  ->> Herb   : use(player) を呼ぶ
    Herb    ->> Player : hp += 30（直接変更）

    note over Player: ❌ updateCondition() が呼ばれない
    note over Player: ❌ maxHp チェックがない
    note over Player: ❌ 毒補正が入らない
```

### 王道設計：Herb は heal() を呼ぶだけ

```mermaid
sequenceDiagram
    participant ゲーム
    participant Herb
    participant Player

    ゲーム  ->> Herb   : use(player) を呼ぶ
    activate Herb
    Herb    ->> Player : heal(30)
    activate Player

    note right of Player: ✅ hp += 30<br/>✅ maxHp チェック<br/>✅ updateCondition()

    Player -->> Herb   : 完了
    deactivate Player
    deactivate Herb
```

`Herb` が知っているのは「`player.heal(30)` と呼ぶ」という事実だけ。
`Player` の内部でどんな処理が起きているかは、`Herb` は知らなくていい。

これを **情報隠蔽（カプセル化）** と呼ぶ。

---

## 2-6 仕様変更に強い設計

ここが設計のもっとも重要なポイントだ。

ゲーム開発では、仕様は**必ず変わる**。

「毒状態なら回復量が半減する」という仕様が追加されたとする。

### ダメな設計の場合

```mermaid
flowchart LR
    A["Herb の use()"] -->|"修正が必要"| X["❌"]
    B["SuperHerb の use()"] -->|"修正が必要"| X
    C["Skill の use()"] -->|"修正が必要"| X
    D["将来追加するアイテム..."] -->|"修正が必要"| X

    style X fill:#ffebee,stroke:#e53935
```

**回復するものが増えるほど、修正箇所が増える。**

### 王道設計の場合

```mermaid
flowchart LR
    A["Herb の use()"]        --> B["Player.heal()"]
    C["SuperHerb の use()"]   --> B
    D["Skill の use()"]       --> B
    E["将来追加するアイテム"] --> B

    B -->|"ここだけ修正すればいい"| OK["✅ 1か所だけ直す"]

    style OK fill:#e8f5e9,stroke:#388e3c
```

`Player::heal()` を1か所修正するだけで、**全ての回復処理に自動的に適用される。**

---

## 2-7 仕様変更後の `heal()` の姿

「毒状態なら回復半減」「バフがあれば1.5倍」が追加された場合、`heal()` はこうなる。

```mermaid
flowchart TD
    A["heal(int baseAmount) 呼び出し"]
    B["amount = baseAmount"]
    C{毒状態か？}
    D["amount = amount / 2"]
    E{バフ中か？}
    F["amount = amount × 1.5"]
    G["hp += amount"]
    H{hp > maxHp?}
    I["hp = maxHp"]
    J["そのまま"]
    K["updateCondition()"]
    L["終了"]

    A --> B --> C
    C -- "Yes" --> D --> E
    C -- "No"  --> E
    E -- "Yes" --> F --> G
    E -- "No"  --> G
    G --> H
    H -- "Yes" --> I --> K
    H -- "No"  --> J --> K
    K --> L
```

この複雑なロジックが `heal()` の中に**1か所だけ**ある。

`Herb` も `SuperHerb` も `Skill` も、**全く変更しなくていい。**

---

## 2-8 責務の整理

「誰が何の責任を持つか」を表にまとめる。

```mermaid
classDiagram
    class Herb {
        -int healAmount = 30
        +use(Player player)
    }

    class Player {
        -int hp
        -int maxHp
        -Condition condition
        +heal(int amount)
        +damage(int amount)
        +updateCondition()
    }

    note for Herb "責務：<br/>回復量を決める<br/>Playerに heal() を依頼する"
    note for Player "責務：<br/>hp を正しく計算する<br/>上限・下限を守る<br/>状態を更新する<br/>将来の補正を処理する"

    Herb --> Player : heal() を呼ぶ
```

| クラス | 知っていること | 知らなくていいこと |
|---|---|---|
| `Herb` | 回復量（30点）| HP の上限チェック、状態更新の仕組み |
| `Player` | HPを安全に変える方法 | どのアイテムが使われたか |

これが **責務の分離** だ。

---

## 2-9 「設計とは何か」

ここまでで、設計について重要なことが見えてきた。

---

> ### 設計とは「未来の変更コストを下げる行為」だ。

---

「今動く」だけなら、`player.hp += 30` でいい。

でも設計を考えるということは、

- 「3ヶ月後に毒状態が追加されたとき、どこを直すか」
- 「アイテムが100種類になったとき、コードは崩壊しないか」

を先に想像して、コードを組む。

プログラムの品質は「今の動作」だけでは測れない。

---

## 2-10 まとめ：3つの問いの答え

第2章の冒頭で立てた問いに答える。

```mermaid
flowchart TB
    subgraph Q1["問い①：HPを直接変えていいのか？"]
        A1["⚠️ 動くがロジックが漏れる<br/>将来の仕様変更で破綻しやすい"]
    end

    subgraph Q2["問い②：どこに回復ロジックを書くべきか？"]
        A2["✅ Player側に集約する<br/>「状態を持つ者が管理する」"]
    end

    subgraph Q3["問い③：状態更新はどこで実行するのが最適か？"]
        A3["✅ heal() / damage() の内部<br/>HP変更が必ず状態更新とセットになる"]
    end

    Q1 --> Q2 --> Q3
```

---

## 2-11 確認問題

1. `player.hp -= 20;` と直接書いた場合、何が問題になる可能性があるか？
   具体的に2つ挙げよ。

2. 「毒状態では `damage()` を毎秒自動で呼ぶ」という仕様が追加された場合、
   今の設計ではどこを変更すればよいか？

3. `Herb` が `player.heal()` を呼ぶとき、`Player` の内部実装を知る必要はあるか？
   その理由は？

4. 以下の2つのコードを比較して、「設計として優れているのはどちらか」とその理由を説明せよ。

   **A案：**
   ```cpp
   class Herb {
       void use(Player& p) {
           p.hp += 30;
           if (p.hp > p.maxHp) p.hp = p.maxHp;
           p.updateCondition();
       }
   };
   ```

   **B案：**
   ```cpp
   class Herb {
       void use(Player& p) {
           p.heal(30);
       }
   };
   ```

---

## 次の章へ

設計の考え方が整理できた。

次の第3章では、実際に `Herb` クラスを実装する。
「回復量を持ち、`Player::heal()` を呼ぶだけ」というシンプルな設計だ。

その中で **参照渡し（`&`）** という C++ の重要な仕組みを学ぶ。
