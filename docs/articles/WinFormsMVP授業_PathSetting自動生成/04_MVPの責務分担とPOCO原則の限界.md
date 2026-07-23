# Part4. MVPの責務分担とPOCO原則の限界

これまで3回で、状態の持ち方（列挙型）、再評価のタイミング（全体再評価）、責務の分け方（パス生成と依存判定を分離）を決めてきた。最後に、これらをMVPというアーキテクチャ全体の中にどう位置づけるか整理する。

## おさらい：MVPの役割分担

このシリーズの最初の設計では、次のように役割を分けた。

```text
View（PathSettingForm）
    コントロールの配置とイベント発火だけ。業務ロジックは持たない

Presenter（PathSettingPresenter）
    Viewのイベントを受け取り、業務ロジックを実行し、Viewへ表示を反映する

Model（PathSettingModel）
    単純なデータの入れ物
```

このとき、「Modelは単純なデータの入れ物（POCO）であるべき」という方針を立てた。POCOとは「Plain Old CLR Object」の略で、特別な基底クラスや振る舞いを持たない、データだけのオブジェクトを指す。

## 質問1

Part1で作った`PathGenerationItem`を思い出してほしい。

```csharp
public class PathGenerationItem
{
    public string FolderPath { get; set; } = string.Empty;
    public string GeneratedFilePath { get; private set; } = string.Empty;
    public PathGenerationStatus Status { get; private set; } = PathGenerationStatus.NotEntered;
    public string Message { get; private set; } = string.Empty;

    public void SetNotEntered() { ... }
    public void SetWaitingForA(string message) { ... }
    public void Apply(PathGenerationResult result) { ... }
}
```

このクラスには`SetWaitingForA`のようなメソッドがある。これは「単純なデータの入れ物」と言えるだろうか？

---

厳密には言えない。このクラスは「自分の状態をどう変更してよいか」を自分自身で制御している。もし本当に「単純なデータの入れ物」に徹するなら、こう書くはずだ。

```csharp
public class PathGenerationItem
{
    public string FolderPath { get; set; }
    public string GeneratedFilePath { get; set; }
    public PathGenerationStatus Status { get; set; }
    public string Message { get; set; }
}
```

全部public setterにして、外部から自由に書き換えられるようにする。これが「純粋なPOCO」である。

## 質問2

この「全部public setter」版のPathGenerationItemを使うと、どんな間違いが起こりうるだろうか？

---

例えばこんなコードが書けてしまう。

```csharp
item.Status = PathGenerationStatus.Success;
item.GeneratedFilePath = string.Empty;  // 生成成功なのにパスが空、という矛盾
```

あるいは、

```csharp
item.Status = PathGenerationStatus.WaitingForA;
item.GeneratedFilePath = "C:\\old\\path.csv";  // A待ちのはずなのに、古い生成結果が残っている
```

こういう矛盾した状態は、「Status」と「GeneratedFilePath」を別々に、いつでも誰でも書き換えられるようにしているから起きる。この矛盾は、コンパイラは教えてくれない。実行時にどこかで表示がおかしくなって、初めて気づく。

## 「ModelはPOCOであるべき」の再検討

ここまで見てきたように、「Modelは単純なデータの入れ物であるべき」というルールを機械的に適用すると、かえって危険な設計になる場面がある。

- 状態と関連データを別々に書き換えられるようにすると、矛盾状態を作れてしまう
- 「この状態のときはこう遷移してよい」というルールを、呼び出し側（Presenter）が毎回正しく守らなければならなくなる
- 結果として、本来オブジェクト自身が守るべき整合性のチェックが、Presenterのあちこちに散らばる

これは、オブジェクト指向で古くから言われる「**Tell, Don't Ask（尋ねるな、命じよ）**」という原則に関係する。外部から中身を覗いて（Ask）勝手に書き換えるのではなく、オブジェクトに「こうなってほしい」と命じ（Tell）、オブジェクト自身が正しい状態遷移を保証する、という考え方である。

`SetWaitingForA()`のようなメソッドは、「A待ちの状態にして」という命令であり、その内部で`GeneratedFilePath`を空にする、`Message`をセットする、といった**整合性の維持**まで含めて面倒を見ている。これは「業務ロジックをModelに書いている」というより、「オブジェクト自身の状態管理をオブジェクト自身に任せている」という方が近い。

## では何でもModelに書いていいのか？　→ そうではない

ここで注意したいのは、「ModelはPOCOにすべき」という考え方を撤回したからといって、「何でもModelに書いていい」という話にはならない、という点である。Part3で見た「A〜Eの依存関係を判定する」というロジックを、もし`PathGenerationItem`自身に持たせたらどうなるか考えてみてほしい。

```csharp
public class PathGenerationItem
{
    public void EvaluateAgainst(PathGenerationItem folderA, IFilePathGenerationService service)
    {
        // Aの状態を見て、自分がB〜Eのどれであるかを判断して…
    }
}
```

これはおかしい。`PathGenerationItem`は「自分1人の状態」しか知らないはずなのに、「他のアイテム（A）の状態」まで知る必要が出てきてしまう。これはPart3で分けた「依存関係の判定」という責務を、また1つのクラスに戻してしまっている。

つまり線引きはこうなる。

```text
PathGenerationItem（Modelの一部）
    → 自分自身の状態の整合性だけを守る
      （Status, GeneratedFilePath, Messageがバラバラにならないようにする）

PathSettingEvaluator
    → 複数のPathGenerationItem間の関係（AとB〜Eの依存関係）を判断する
```

「Modelに振る舞いを持たせてよいか」の判断基準は、**その振る舞いが「自分自身の整合性を守るだけ」か、それとも「他のオブジェクトとの関係を判断している」かで決まる。** 前者はModelに置いてよい。後者はEvaluatorのような別のクラスに置くべきである。

## この回のまとめ

- 「ModelはPOCOであるべき」は絶対原則ではなく、チームでよく使われる指針の1つに過ぎない
- 全部public setterのPOCOは、矛盾した状態を外部から自由に作れてしまう危険がある
- `SetWaitingForA()`のような、自分自身の整合性を守るメソッドをModelに持たせるのは問題ない（Tell, Don't Ask）
- 一方、「他のオブジェクトとの関係を判断するロジック」（今回で言えば依存関係の判定）はModelに持たせるべきではなく、別クラス（Evaluator）に分離する
- 判断基準は「自分の面倒だけを見ているか」「他人の面倒まで見ようとしていないか」

## シリーズ全体のまとめ

```text
Part1: 状態は列挙型で表現する（bool/nullでは意味を区別できない）
Part2: 差分更新ではなく全体再評価にする（依存関係の伝播漏れを防ぐ）
Part3: 「パス生成」と「依存関係の判定」は別クラスに分ける（名前と責務を一致させる）
Part4: Modelに振る舞いを持たせてよいかは「自分の整合性を守るだけか」で判断する
```

いずれも、「動けばいい」から一歩進んで「なぜこの形にするのか」を言語化する練習である。設計に唯一の正解はないが、**選んだ理由を説明できること**が、後で自分や他の人がコードを読んだときの助けになる。
