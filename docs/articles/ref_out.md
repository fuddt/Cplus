```
結果だけを見れば、どの方法でも「パスを生成し、成功したかを判断する」ことはできます。

しかし、設計が表している意味は同じではありません。

違いは主に次の4点です。

* 成功・失敗を誰が持つのか
* メソッドが状態を持つのか
* 呼び出し側に何を強制するのか
* 失敗を通常扱いするのか、異常扱いするのか

ここを基準に判断すると、実装パターンを感覚ではなく説明できます。

⸻

まず全体像

設計	設計が意味すること	向いている状況	主な問題
戻り値でパス、失敗時は例外	失敗は通常起きない異常事態	入力が保証されている内部処理	通常失敗まで例外にすると重い
Try～ + out	失敗は想定内の分岐	ユーザー入力、検索、変換	返せる情報が少ない
結果クラスを返す	処理結果そのものがデータ	エラー理由や警告も必要	単純処理には大げさ
共通IsSuccessを持つ	オブジェクトが直前の処理状態を記憶する	状態を持つこと自体に意味がある処理	上書き、依存、並列実行に弱い
ref bool	呼び出し側の変数をメソッドが変更する	入出力双方に意味がある特殊ケース	責務が不明瞭、呼び出しが分かりにくい

⸻

1. 戻り値でパスを返し、失敗時は例外

using System;
using System.IO;
public class FilePathGenerator
{
    public string GenerateCsvPath(string basePath)
    {
        if (string.IsNullOrWhiteSpace(basePath))
        {
            throw new ArgumentException(
                "基準フォルダが指定されていません。",
                nameof(basePath));
        }
        return Path.Combine(basePath, "Data", "data.csv");
    }
}

この設計が意味すること

このメソッドは、次の契約を表しています。

正しい引数を渡せば、ファイルパスを返す。
正しくない引数が渡された場合は、通常の処理を継続できない。

つまり、失敗は「処理結果の一種」ではなく、「契約違反」や「異常」として扱っています。

呼び出し側も、基本的には成功を前提に書けます。

string path = generator.GenerateCsvPath(@"C:\Work");
Console.WriteLine(path);

適切なケース

例えば、呼び出し前に入力チェック済みである場合です。

if (string.IsNullOrWhiteSpace(textBoxFolder.Text))
{
    MessageBox.Show("フォルダを入力してください。");
    return;
}
// ここまで来た時点で、入力済みであることを前提にできる
string path = generator.GenerateCsvPath(textBoxFolder.Text);

この場合、空文字が入ってきたら、呼び出し側の実装ミスとも考えられます。

不適切なケース

失敗が頻繁に起きる通常シナリオなのに、例外を使う場合です。

例えば、ユーザーが未入力のままボタンを押すことは、十分起こり得ます。

try
{
    string path = generator.GenerateCsvPath(textBoxFolder.Text);
}
catch (ArgumentException)
{
    MessageBox.Show("入力してください。");
}

これは動きますが、「未入力」という普通の分岐を例外で表現しています。

この設計に対する判断はこうです。

未入力が仕様上想定される通常ケースなら、例外で制御するのは不適切。

反対に、

このメソッドに空文字が渡ること自体がプログラム上の異常なら、例外は適切。

です。

⸻

2. Try～とoutを使う

using System.IO;
public class FilePathGenerator
{
    public bool TryGenerateCsvPath(
        string basePath,
        out string filePath)
    {
        filePath = string.Empty;
        if (string.IsNullOrWhiteSpace(basePath))
        {
            return false;
        }
        filePath = Path.Combine(
            basePath,
            "Data",
            "data.csv");
        return true;
    }
}

呼び出し側です。

if (generator.TryGenerateCsvPath(
    textBoxFolder.Text,
    out string filePath))
{
    Console.WriteLine(filePath);
}
else
{
    MessageBox.Show("ファイルパスを生成できませんでした。");
}

この設計が意味すること

このメソッドは、次の契約を表しています。

この処理は失敗する可能性がある。
失敗は異常ではなく、呼び出し側が普通に判断する結果である。

成功か失敗かをboolで返し、成功時だけoutに値を入れます。

int.TryParseと同じ思想です。

if (int.TryParse("123", out int value))
{
    Console.WriteLine(value);
}

文字列が数値でないことは、プログラムの障害ではなく、普通にあり得る入力だからです。

適切なケース

* ユーザー入力
* ファイルやディレクトリの検索
* 文字列変換
* 対象が存在しない可能性のある取得処理
* 成功・失敗だけ分かれば十分な処理

例えば、

bool success =
    generator.TryGenerateCsvPath(input, out string path);

この1回の呼び出しだけで、

* 成功したか
* 成功した場合の値

が結び付いています。

不適切なケース

失敗理由が複数あり、呼び出し側が区別する必要がある場合です。

if (!generator.TryGenerateCsvPath(input, out string path))
{
    // なぜ失敗したのか分からない
}

失敗理由として、次のような種類があるかもしれません。

* 未入力
* 不正な文字を含む
* 対象フォルダが存在しない
* アクセス権限がない
* 設定が未完了

boolだけでは、すべてfalseになります。

この設計への判断はこうです。

成否だけで十分ならTryパターンが適切。
失敗理由を呼び出し側が扱う必要があるなら、情報不足なので不適切。

⸻

3. 結果クラスを返す

public class FilePathGenerationResult
{
    public bool IsSuccess { get; }
    public string FilePath { get; }
    public string ErrorMessage { get; }
    private FilePathGenerationResult(
        bool isSuccess,
        string filePath,
        string errorMessage)
    {
        IsSuccess = isSuccess;
        FilePath = filePath;
        ErrorMessage = errorMessage;
    }
    public static FilePathGenerationResult Success(
        string filePath)
    {
        return new FilePathGenerationResult(
            true,
            filePath,
            string.Empty);
    }
    public static FilePathGenerationResult Failure(
        string errorMessage)
    {
        return new FilePathGenerationResult(
            false,
            string.Empty,
            errorMessage);
    }
}
using System.IO;
public class FilePathGenerator
{
    public FilePathGenerationResult GenerateCsvPath(
        string basePath)
    {
        if (string.IsNullOrWhiteSpace(basePath))
        {
            return FilePathGenerationResult.Failure(
                "基準フォルダが入力されていません。");
        }
        string filePath = Path.Combine(
            basePath,
            "Data",
            "data.csv");
        return FilePathGenerationResult.Success(filePath);
    }
}

呼び出し側です。

FilePathGenerationResult result =
    generator.GenerateCsvPath(textBoxFolder.Text);
if (result.IsSuccess)
{
    Console.WriteLine(result.FilePath);
}
else
{
    MessageBox.Show(result.ErrorMessage);
}

この設計が意味すること

この設計では、処理結果を1つのデータとして扱います。

処理結果
├─ 成功したか
├─ 生成されたパス
└─ 失敗理由

つまり、

パス生成という処理は、単なる文字列ではなく、成功状態やエラー情報を含む結果を返す。

という契約です。

適切なケース

複数の情報を返す必要がある場合です。

例えば結果を拡張できます。

public class FilePathGenerationResult
{
    public bool IsSuccess { get; }
    public string FilePath { get; }
    public string ErrorCode { get; }
    public string ErrorMessage { get; }
    public bool DirectoryExists { get; }
}

また、失敗理由を列挙型にすることもできます。

public enum FilePathGenerationError
{
    None,
    EmptyInput,
    InvalidPath,
    DirectoryNotFound
}
public class FilePathGenerationResult
{
    public bool IsSuccess { get; }
    public string FilePath { get; }
    public FilePathGenerationError Error { get; }
}

呼び出し側で理由ごとに処理できます。

FilePathGenerationResult result =
    generator.GenerateCsvPath(input);
if (!result.IsSuccess)
{
    switch (result.Error)
    {
        case FilePathGenerationError.EmptyInput:
            MessageBox.Show("フォルダを入力してください。");
            break;
        case FilePathGenerationError.InvalidPath:
            MessageBox.Show("パスの形式が不正です。");
            break;
        case FilePathGenerationError.DirectoryNotFound:
            MessageBox.Show("フォルダが存在しません。");
            break;
    }
}

不適切なケース

返したいものが単純な文字列だけで、失敗理由も必要ない場合です。

FilePathGenerationResult result =
    generator.GenerateCsvPath(@"C:\Work");

単に次を返せば済むのに、

string path =
    generator.GenerateCsvPath(@"C:\Work");

結果クラスを作ると、設計が過剰になります。

判断はこうです。

成功値以外にも、エラー理由、警告、補足情報を一体として返すなら適切。
単純な値だけで済むなら、抽象化が過剰で不適切。

⸻

4. 共通のIsSuccessインスタンス変数を使う

public class FilePathGenerator
{
    public bool IsSuccess { get; private set; }
    public string GenerateCsvPath(string basePath)
    {
        if (string.IsNullOrWhiteSpace(basePath))
        {
            IsSuccess = false;
            return string.Empty;
        }
        IsSuccess = true;
        return Path.Combine(basePath, "data.csv");
    }
    public string GenerateLogPath(string basePath)
    {
        if (string.IsNullOrWhiteSpace(basePath))
        {
            IsSuccess = false;
            return string.Empty;
        }
        IsSuccess = true;
        return Path.Combine(basePath, "app.log");
    }
}

この設計が意味すること

このオブジェクトは、単なる処理提供者ではありません。

自分が最後に行った処理の状態を記憶するオブジェクト

になります。

つまり、状態を持つクラスです。

FilePathGenerator
├─ パスを生成する
└─ 最後の成功状態を記憶する

ここが重要です。

IsSuccessを持たせると、クラスの責務が変わります。

ただのパス生成クラスではなく、

パス生成と、直前の実行状態の管理をするクラス

になります。

問題1：呼び出し順に依存する

string csvPath =
    generator.GenerateCsvPath(@"C:\Work");
string logPath =
    generator.GenerateLogPath(string.Empty);
bool success = generator.IsSuccess;

このsuccessは、CSVではなくログパス生成の結果です。

つまり結果がメソッド呼び出し順に依存します。

問題2：値と成否が分離する

string csvPath =
    generator.GenerateCsvPath(@"C:\Work");
// ここで確認し忘れる可能性がある
bool success = generator.IsSuccess;

csvPathとIsSuccessは別々に存在しています。

この2つが同じ処理結果だという保証を、呼び出し側の書き方に依存させています。

問題3：並列処理に弱い

Task<string> csvTask = Task.Run(
    () => generator.GenerateCsvPath(csvBasePath));
Task<string> logTask = Task.Run(
    () => generator.GenerateLogPath(logBasePath));

同じインスタンスを複数処理が使うと、IsSuccessを互いに上書きします。

最終的にどちらの状態が残るかは、実行順次第です。

適切になり得るケース

状態を持つこと自体に意味がある場合です。

例えば接続オブジェクトです。

public class ServerConnection
{
    public bool IsConnected { get; private set; }
    public void Connect()
    {
        // 接続処理
        IsConnected = true;
    }
    public void Disconnect()
    {
        // 切断処理
        IsConnected = false;
    }
}

このIsConnectedは、「直前のメソッドが成功したか」ではありません。

現在、接続中か

というオブジェクトの現在状態です。

したがって、インスタンス変数として意味があります。

一方、IsSuccessは通常、

直前の一回の処理結果

であり、オブジェクトの継続的な状態ではありません。

判断はこうです。

値がオブジェクトの現在状態を表すなら、インスタンス変数が適切。
単なる一回のメソッド結果なら、インスタンス変数にするのは不適切。

⸻

5. ref boolを渡す

public string GenerateCsvPath(
    string basePath,
    ref bool isSuccess)
{
    isSuccess = false;
    if (string.IsNullOrWhiteSpace(basePath))
    {
        return string.Empty;
    }
    isSuccess = true;
    return Path.Combine(basePath, "data.csv");
}

呼び出し側です。

bool isSuccess = false;
string path = generator.GenerateCsvPath(
    @"C:\Work",
    ref isSuccess);

この設計が意味すること

refは、本来次の意味です。

呼び出し側が持っている値をメソッドに渡し、メソッドがそれを読み書きする。

つまり、入力と出力の両方です。

public void AddOne(ref int value)
{
    // 渡された現在値を読む
    value = value + 1;
    // 更新結果を呼び出し側へ返す
}
int count = 10;
AddOne(ref count);
// countは11

これはrefに意味があります。元の値を利用して変更するからです。

しかし成功フラグの場合、元の値を使っていません。

bool isSuccess = true;
// trueという入力値を渡しているが、
// メソッド内ですぐfalseに上書きする
GenerateCsvPath(path, ref isSuccess);

この場合、refが持つ「入力」の意味がありません。

outとの違い

public string GenerateCsvPath(
    string basePath,
    out bool isSuccess)

outは、

呼び出し前の値は不要で、メソッドから値を返すための引数

です。

したがって、ref boolよりはout boolの方が意図に合います。

ただし、それでも次の形です。

string path =
    generator.GenerateCsvPath(input, out bool isSuccess);

通常は逆にした方が自然です。

bool isSuccess =
    generator.TryGenerateCsvPath(input, out string path);

なぜなら、そのメソッドで最初に判断したいのは、成功したかだからです。

ref boolが不適切な理由

ref boolを使うと、呼び出し側の変数をメソッドが副作用で変更します。

bool result = false;
generator.GenerateCsvPath(path, ref result);

コードを読む側は、

* resultが入力なのか
* 初期値に意味があるのか
* メソッド後にどう変化するのか

をメソッド内部まで確認しなければなりません。

判断はこうです。

呼び出し前の値を利用して更新するならref。
単に結果を返すだけならrefは意味と実装が一致していないため不適切。

⸻

同じ結果でも設計上の意味が違う具体例

すべて次のような結果を得られるとします。

入力：C:\Work
結果：C:\Work\Data\data.csv
成功：true

例外方式

string path = generator.GenerateCsvPath(input);

意味：

正常なら必ず値が返る。返らなければ異常。

⸻

Try方式

bool success =
    generator.TryGenerateCsvPath(input, out string path);

意味：

成功と失敗は、どちらも通常の結果。

⸻

結果クラス方式

FilePathGenerationResult result =
    generator.GenerateCsvPath(input);

意味：

処理結果には、値以外にも意味のある情報が含まれる。

⸻

共通インスタンス変数方式

string path = generator.GenerateCsvPath(input);
bool success = generator.IsSuccess;

意味：

オブジェクトが直前の処理状態を記憶している。

⸻

ref bool方式

bool success = false;
string path =
    generator.GenerateCsvPath(input, ref success);

意味：

呼び出し側が持つ変数を、メソッドが外部から変更する。

⸻

判断基準

実務では、次の順序で考えると判断しやすいです。

1. 失敗は通常起こるか

通常起こらないなら、例外を検討します。

string path = GeneratePath(input);

通常起こるなら、Tryか結果クラスです。

bool success = TryGeneratePath(input, out string path);

⸻

2. 失敗理由が必要か

成否だけでよいならTryです。

bool success =
    TryGeneratePath(input, out string path);

失敗理由が必要なら結果クラスです。

GenerationResult result =
    GeneratePath(input);

⸻

3. 成功状態はオブジェクトの状態か

例えば、

* 接続中か
* 初期化済みか
* ログイン中か
* 処理実行中か

なら、インスタンス変数に意味があります。

public bool IsConnected { get; private set; }

しかし、

* 直前のパス生成が成功したか
* 直前の変換が成功したか
* 直前の検索が成功したか

は、一回限りのメソッド結果です。

public bool IsSuccess { get; private set; }

この場合、インスタンス変数に持たせる理由は弱いです。

⸻

4. 呼び出し前の値を使うか

使うならrefです。

public void Increment(ref int count)
{
    count++;
}

使わないならrefではありません。

public void Generate(out bool success)

ただし、成功値なら普通は戻り値にした方が明確です。

public bool TryGenerate(out string path)

⸻

今回のケースに対する判断

今回の処理は、

* 入力文字列を受け取る
* 文字列を追加してパスを生成する
* 生成できたかを判断する
* 同じようなメソッドが複数ある

というものです。

この場合、共通のIsSuccessは不適切です。

理由は明確です。

成功状態はクラス全体の継続状態ではなく、各メソッド呼び出しごとの結果だから。

また、ref boolも不適切です。

呼び出し前のbool値を利用しておらず、単に出力先として使っているだけだから。

成否だけでよいなら、次のTry方式が最も意味と実装が一致します。

using System.IO;
public class FilePathGenerator
{
    public bool TryGenerateCsvPath(
        string basePath,
        out string filePath)
    {
        filePath = string.Empty;
        if (string.IsNullOrWhiteSpace(basePath))
        {
            return false;
        }
        filePath = Path.Combine(
            basePath,
            "Data",
            "data.csv");
        return true;
    }
    public bool TryGenerateImagePath(
        string basePath,
        out string filePath)
    {
        filePath = string.Empty;
        if (string.IsNullOrWhiteSpace(basePath))
        {
            return false;
        }
        filePath = Path.Combine(
            basePath,
            "Images",
            "image.png");
        return true;
    }
    public bool TryGenerateLogPath(
        string basePath,
        out string filePath)
    {
        filePath = string.Empty;
        if (string.IsNullOrWhiteSpace(basePath))
        {
            return false;
        }
        filePath = Path.Combine(
            basePath,
            "Logs",
            "app.log");
        return true;
    }
}

最終的な意思決定文にするなら、こう表現できます。

共通のIsSuccessを持つ設計では、各メソッドの成否がインスタンスの共有状態として保存されるため、後続メソッドによる上書きや呼び出し順への依存が発生する。今回の成功状態はオブジェクト自身の継続的状態ではなく、各メソッド呼び出し単位の結果である。そのため、成否を戻り値、生成結果をoutで返すTryパターンの方が、責務とデータの所属が一致している。

これが、「同じ結果になるが、なぜこの設計を選ぶのか」の説明になります。

```