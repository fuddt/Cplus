# 順序を保ったままINIファイルを読み込むIniReaderの設計

INIファイルの値を、ファイルに書かれた順番のまま上から読みたい、という要件を考える。
表示順に依存する設定、優先順位のあるリストなど、「何番目に書かれているか」自体に
意味があるケースでは、読み込んだ結果もその順序を保っていなければならない。

## なぜDictionaryでは不十分か

INIをパースするとき、素直に書くと「セクション名→キー→値」を`Dictionary`で
持たせたくなる。しかしこれには2つの問題がある。

1. `Dictionary<TKey, TValue>`は列挙順序を仕様として保証していない。現在の
   .NETランタイムの実装ではたまたま挿入順に見えることが多いが、それは
   ドキュメント化された契約ではなく実装の都合にすぎない。将来のバージョンや
   別のランタイムで変わらない保証はどこにもない
2. 同じキー名を複数回書いた場合、`Dictionary`では後から書いた値で上書きされる。
   「同じキーを複数回書きたい」というケースにそもそも対応できない

つまり`Dictionary`ベースの実装は、「順序を保証したい」という要件に対して
構造的に相性が悪い。

## 設計方針

そこで、セクションもキー=値も、すべて`List<T>`で保持する構造にする。

```
List<IniSection>
└─ IniSection
    ├─ Name
    └─ List<IniItem>
        └─ IniItem (Key, Value)
```

`List<T>`はAPIの仕様として「追加した順序を保持する」ことが明記されている。
`Dictionary`のような「たまたまそう見えるだけ」の非公式な挙動ではなく、
言語仕様として順序が保証された構造を選ぶことで、次の2点が得られる。

- ファイルに書かれた物理的な順序が、そのまま読み込み結果の順序になる
- 同じキー名が複数回登場しても、上書きされずすべて保持される

## コード

```csharp
using System;
using System.Collections.Generic;
using System.IO;

public class IniSection
{
    // [General] などのセクション名
    public string Name { get; set; }

    // セクション内のキー=値を上から順番に保持する
    public List<IniItem> Items { get; set; } = new List<IniItem>();
}

public class IniItem
{
    public string Key { get; set; }
    public string Value { get; set; }
}

public class IniReader
{
    public List<IniSection> Read(string filePath)
    {
        var sections = new List<IniSection>();

        IniSection currentSection = null;

        // File.ReadLinesを使うことで、
        // INIファイルを上から1行ずつ順番に読む
        foreach (string rawLine in File.ReadLines(filePath))
        {
            // 前後の空白を除去
            string line = rawLine.Trim();

            // 空行は無視
            if (string.IsNullOrWhiteSpace(line))
            {
                continue;
            }

            // コメント行は無視
            if (line.StartsWith(";") || line.StartsWith("#"))
            {
                continue;
            }

            // -----------------------------
            // セクション判定
            // 例:
            // [General]
            // -----------------------------
            if (line.StartsWith("[") && line.EndsWith("]"))
            {
                string sectionName = line.Substring(
                    1,
                    line.Length - 2
                ).Trim();

                currentSection = new IniSection
                {
                    Name = sectionName
                };

                // Listなのでファイルに現れた順番で追加される
                sections.Add(currentSection);

                continue;
            }

            // -----------------------------
            // キー=値の判定
            // -----------------------------
            int equalIndex = line.IndexOf('=');

            if (equalIndex >= 0)
            {
                // 最初の = より左側をキーとする
                string key = line.Substring(0, equalIndex).Trim();

                // 最初の = より右側を値とする
                string value = line.Substring(equalIndex + 1).Trim();

                // セクションがまだ存在しない場合はエラー
                if (currentSection == null)
                {
                    throw new FormatException(
                        $"セクションより前に設定値があります: {line}"
                    );
                }

                currentSection.Items.Add(new IniItem
                {
                    Key = key,
                    Value = value
                });
            }
        }

        return sections;
    }
}
```

## 使い方の例

```ini
[General]
Title=サンプルアプリ
Version=1.0.0

[Colors]
Color01=120,56,72
Color02=200,100,50
Color02=10,20,30
```

```csharp
var reader = new IniReader();
List<IniSection> sections = reader.Read("app.ini");

foreach (IniSection section in sections)
{
    Console.WriteLine($"[{section.Name}]");
    foreach (IniItem item in section.Items)
    {
        Console.WriteLine($"  {item.Key} = {item.Value}");
    }
}
```

`[General]`が`[Colors]`より先に出力され、`Colors`セクション内の項目も
`Color01`→`Color02`→`Color02`の順（3行目も上書きされずそのまま）で
出力される。ファイルの見た目通りの順序と内容が、そのまま結果に反映される。

## 設計判断の解説

**セクション未定義でのエラー**

キー=値の行がどのセクションにも属さない状態で出てきた場合、`FormatException`を
投げる。これは、壊れたINIを「とりあえず読めるところまで読む」のではなく、
不正な入力である時点で早期に失敗させる（fail-fast）という判断。原因不明のまま
値が欠けたり、意図しないセクションに紐づいたりする事故を防ぐ。

**重複キー許容というトレードオフ**

`Items`が`List<IniItem>`である以上、同じキー名を複数回書いても両方とも
保持される。これは「同じキーを繰り返し書きたい」ケースには有利だが、
裏を返すと「あるキーの値を1つだけ取り出したい」という一般的な使い方には
向いていない。`Dictionary`ならキー名から値をO(1)で引けるが、この構造では
`Items`を`Key`で線形探索（`FirstOrDefault`など）する必要があり、
セクション内の項目数が多いとその分コストがかかる。

「順序を保つこと」と「キーで高速に検索できること」は、単純な実装では
両立しない。どちらを優先するかは、そのINIファイルの使われ方（表示順が
大事なのか、特定のキーをピンポイントで引きたいのが多いのか）で決める。

## 関連記事

以前まとめた
[INIファイルからRGBカラーリストを読み込む設計パターン比較](./INIファイルからRGBカラーリストを読み込む設計パターン比較.md)
の「連番＋歯抜け許容・番号順ソート方式」では、`Dictionary`の列挙順が信頼できない
という問題に対して、キー名に埋め込んだ番号を明示的に`OrderBy`で並べ替えることで
対処していた。今回の`IniReader`は、そもそも順序が狂わない`List`ベースの構造を
選ぶことで、同じ問題を構造的に回避する別解にあたる。
