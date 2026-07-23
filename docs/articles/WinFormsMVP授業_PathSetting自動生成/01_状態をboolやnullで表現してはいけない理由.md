# Part1. 状態をboolやnullで表現してはいけない理由

## お題

FolderA〜Eという5つのフォルダパス入力欄がある。ここから内部的にファイルパスを自動生成したい。ただし条件がある。

- FolderAが入力されていないと、B〜Eに紐づくファイルパスは生成できない
- FolderA〜Eは好きな順番で入力してよい

まず、生成できたかどうかを判定するコードを書いてみる。多くの人は最初にこう書きたくなる。

```csharp
public bool TryGenerateFilePath(FolderKey key, out string filePath)
{
    // 生成できたら true、できなければ false
}
```

一見自然に見える。ここで質問。

## 質問1

次の5つの状態は、それぞれ意味が違う。

```text
① FolderB自体がまだ空欄
② FolderBは入力済みだが、FolderAが空欄なので生成できない
③ FolderBもFolderAも入力済みで、正常に生成できた
④ FolderBの入力値に使えない文字が含まれていて、そもそも不正
⑤ 生成処理の途中で例外が起きた（想定外のエラー）
```

`TryGenerateFilePath`が`false`を返したとき、呼び出し側はこの①②④⑤のうちどれが起きたのか区別できるだろうか？

少し考えてみてほしい。

---

答えは「区別できない」。`bool`は`true`/`false`の2値しか持たないので、「失敗した」という事実は伝わっても「なぜ失敗したか」は伝わらない。

これは`null`で表現しても同じことが起きる。

```csharp
public string? TryGenerateFilePath(FolderKey key)
{
    // 生成できなければ null
}
```

`null`が返ってきたとき、それが「まだ入力されていないだけ」なのか「Aを待っている」のか「壊れている」のか、呼び出し側には判別できない。

## なぜ区別が必要なのか

区別できないと何が困るのか、具体的に考えてみよう。今回の要件には「エラーはラベルに赤文字で表示する」という仕様がある。

- ①（未入力）なら、何も表示しない（ユーザーはまだ入力中なので、責める必要がない）
- ②（A待ち）なら、赤字ではなく「FolderAを入力すると生成されます」のような案内を薄い色で出す（ユーザーのミスではない）
- ④（入力不正）なら、赤字で「使用できない文字が含まれています」と出す（ユーザーの入力ミス）
- ⑤（生成失敗）なら、赤字で「生成に失敗しました。管理者に連絡してください」と出す（システム側の異常）

`bool`や`null`だけを使う設計では、このUIの出し分けを実現しようとした瞬間に、呼び出し側（Presenter）が独自にフラグを増やし始める。

```csharp
bool isNotEntered = ...;
bool isWaitingForA = ...;
bool isInvalid = ...;
bool isFailed = ...;
```

これは「状態を表す情報」が、本来1つの変数に収まるはずなのに、あちこちに散らばってしまっている状態。しかもこれらのフラグは本来「同時に2つ以上trueにならない」という制約があるのに、bool変数を並べただけではその制約をコード上で保証できない。

## 解決策：列挙型で状態そのものを1つの値にする

```csharp
public enum PathGenerationStatus
{
    NotEntered,   // 対象フォルダが未入力
    WaitingForA,  // Aが未入力/無効なので待機中
    Success,      // 生成成功
    InvalidInput, // 入力値が不正
    Failed        // 生成処理で失敗
}
```

列挙型にすると、ある瞬間に取りうる状態は必ずこの5つのうちどれか1つだけになる。「未入力かつA待ち」のような矛盾した組み合わせはそもそも表現できない。これはbool変数を並べる設計にはない強みである。

そして、この状態・入力値・生成結果・メッセージを1つのオブジェクトにまとめる。

```csharp
public class PathGenerationItem
{
    public string FolderPath { get; set; } = string.Empty;
    public string GeneratedFilePath { get; private set; } = string.Empty;
    public PathGenerationStatus Status { get; private set; } = PathGenerationStatus.NotEntered;
    public string Message { get; private set; } = string.Empty;
}
```

こうすることで、FolderB用の`PathGenerationItem`を1つ見るだけで「今どういう状態で、なぜその状態なのか」が分かるようになる。呼び出し側（Presenter/View）は`Status`を見て分岐するだけでよく、独自にフラグを増やす必要がなくなる。

## この回のまとめ

- `bool`や`null`は「成功したかどうか」しか表現できず、「なぜ失敗したか」を表現できない
- 状態の種類が3つ以上あり、それぞれ扱い（UI表示など）が異なるなら、列挙型で状態そのものを表現するべき
- 状態・データ・理由（メッセージ）は、バラバラの変数にせず1つのオブジェクトにまとめると、呼び出し側の分岐が単純になる

## 次回予告

状態の表現方法は決まった。では「いつ、どのタイミングで、どの範囲を再計算するか」を次に考える。FolderA〜Eが自由な順番で入力される、という要件が、ここで効いてくる。
