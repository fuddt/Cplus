# INIファイルからRGBカラーリストを読み込む設計パターン比較

既存の`.ini`設定ファイルに、RGB値を1行1色で書いておき、それを読み込んでアプリ側で
`List<Color>`として保持したい、という要件を考える。

制約は次の2つ。

- 新しく色専用の設定ファイルは作らない。既存の`.ini`を流用する
- 色の数は決め打ちできない。目安は13色だが、5色になることも20色になることもある

同じ結果（`List<Color>`が手に入る）を実現する方法はいくつかあるが、
「ini側の書き方」と「読み込み側の実装」の組み合わせによって、運用のしやすさや
バグの起きやすさが変わる。ここでは4パターンを比較する。

## 比較表

| パターン | 概要 | メリット | デメリット | 向いている場面 |
|---|---|---|---|---|
| 連番＋打ち切り方式（採用） | `Color01`, `Color02`... と番号を振り、キーが見つからなくなった時点で読み込みを止める | 上限を決め打ちしなくてよい／`List<T>`と同じ「歯抜けなし」という性質のまま読み込める／欠番があると即座に件数が減り、ミスに気づきやすい | 途中の1色だけ削除・挿入すると、以降の番号を振り直す必要がある | ツールやコードが機械的に書き出す設定、手編集の頻度が低いもの |
| 連番＋歯抜け許容・番号順ソート方式 | 番号は振るが、途中が欠番でも構わない。全キーを読んでから番号順に並べ替える | 途中の1色だけ削除・追加してもリネーム不要 | 実装がやや複雑（正規表現でキー抽出＋数値ソートが必要）／「意図した歯抜け」か「typoによる欠番」かを区別できない | 人間が頻繁に手で編集するファイル |
| 1行にまとめてデリミタ2階層で持つ方式 | `List=R,G,B;R,G,B;...`のように1キーに全色を詰める | キーが1つで済み、番号管理そのものが不要 | 区切り文字が2階層になりパースがやや面倒／1行が長大になり差分(diff)が見づらい | 色の数が少なく、あまり編集しないファイル |
| 固定名キー方式（参考） | `HpBarColor=...`のように意味のある名前をキーにする | 何の色か名前から分かる／読み込み側で型（プロパティ）として持てる | 色を増減するたびにプロパティ・コンストラクタ・コレクションへの追加、3箇所を直す必要がある。可変長リストには不向き | 色の数と用途が固定されている設定（例：背景色、警告色など数個限定） |

以降、各パターインのini記述例とC#コードを示す。すべて次の共通パーサを土台にする。

## 共通コード：軽量INIパーサ

セクション→キー→値の`Dictionary`を`File.ReadAllLines`から組み立てるだけの、
最小限のINI読み込みクラス。以降の全パターンはこれを経由してiniの値を取得する。

```csharp
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

public sealed class IniFile
{
    private readonly Dictionary<string, Dictionary<string, string>> _sections =
        new Dictionary<string, Dictionary<string, string>>(StringComparer.OrdinalIgnoreCase);

    public IniFile(string path)
    {
        string currentSection = string.Empty;
        foreach (string rawLine in File.ReadAllLines(path, Encoding.UTF8))
        {
            string line = rawLine.Trim();
            if (line.Length == 0 || line.StartsWith(";") || line.StartsWith("#"))
            {
                continue;
            }

            if (line.StartsWith("[") && line.EndsWith("]"))
            {
                currentSection = line.Substring(1, line.Length - 2).Trim();
                continue;
            }

            int separatorIndex = line.IndexOf('=');
            if (separatorIndex < 0)
            {
                continue;
            }

            string key = line.Substring(0, separatorIndex).Trim();
            string value = line.Substring(separatorIndex + 1).Trim();

            if (!_sections.TryGetValue(currentSection, out var keyValues))
            {
                keyValues = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                _sections[currentSection] = keyValues;
            }
            keyValues[key] = value;
        }
    }

    public bool TryGetValue(string section, string key, out string value)
    {
        value = null;
        return _sections.TryGetValue(section, out var keyValues)
            && keyValues.TryGetValue(key, out value);
    }

    public IEnumerable<string> GetKeys(string section)
    {
        return _sections.TryGetValue(section, out var keyValues)
            ? keyValues.Keys
            : Array.Empty<string>();
    }
}
```

`R,G,B`という文字列を`Color`に変換する処理は全パターン共通で使うので、
各パターンのコードでは同じ形の`TryParseColor`をそのまま再掲している。

## 1. 連番＋打ち切り方式（採用）

```ini
[Colors]
Color01=120,56,72
Color02=200,100,50
Color03=10,20,30
```

```csharp
using System.Collections.Generic;
using System.Drawing;

public sealed class SequentialStopIniColorLoader
{
    private const string SectionName = "Colors";

    public IReadOnlyList<Color> Load(IniFile ini)
    {
        var colors = new List<Color>();

        for (int index = 1; ; index++)
        {
            string key = $"Color{index:00}";
            if (!ini.TryGetValue(SectionName, key, out string rawValue))
            {
                break; // 連番が途切れたらそこで打ち止め
            }

            if (TryParseColor(rawValue, out Color color))
            {
                colors.Add(color);
            }
        }

        return colors;
    }

    private static bool TryParseColor(string rawValue, out Color color)
    {
        color = default;
        string[] parts = rawValue.Split(',');
        if (parts.Length != 3)
        {
            return false;
        }

        if (!byte.TryParse(parts[0], out byte r) ||
            !byte.TryParse(parts[1], out byte g) ||
            !byte.TryParse(parts[2], out byte b))
        {
            return false;
        }

        color = Color.FromArgb(r, g, b);
        return true;
    }
}
```

**設計が意味すること**

`List<Color>`はそもそも「歯抜け」を表現できないデータ構造（インデックスは必ず
0からN-1まで連続する）。読み込み側を「最初の欠番で止める」仕様にすることで、
ini側にも同じ「歯抜けなし」という制約を課し、ファイルと読み込み結果の対応を
一致させている。

また、`ColorCount=13`のような件数キーを別途持たなくても、キーが尽きた時点が
そのまま終端になる（自己終端）。上限を決め打ちする必要もない。

副作用として、途中の1色だけ削除・挿入すると以降の番号を振り直す必要がある。
逆に言えば、typoやリネーム忘れで1つ欠番になると、それ以降が丸ごと読めなくなり
件数が大きくずれるため、ミスに気づきやすい（fail-fast）。

## 2. 連番＋歯抜け許容・番号順ソート方式

```ini
[Colors]
Color01=120,56,72
Color03=10,20,30
Color07=200,100,50
```

```csharp
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text.RegularExpressions;

public sealed class SequentialSortIniColorLoader
{
    private const string SectionName = "Colors";
    private static readonly Regex KeyPattern = new Regex(@"^Color(\d+)$", RegexOptions.IgnoreCase);

    public IReadOnlyList<Color> Load(IniFile ini)
    {
        var numbered = new List<(int Index, Color Color)>();

        foreach (string key in ini.GetKeys(SectionName))
        {
            Match match = KeyPattern.Match(key);
            if (!match.Success)
            {
                continue;
            }

            if (!ini.TryGetValue(SectionName, key, out string rawValue) ||
                !TryParseColor(rawValue, out Color color))
            {
                continue;
            }

            numbered.Add((int.Parse(match.Groups[1].Value), color));
        }

        return numbered
            .OrderBy(entry => entry.Index)
            .Select(entry => entry.Color)
            .ToList();
    }

    private static bool TryParseColor(string rawValue, out Color color)
    {
        color = default;
        string[] parts = rawValue.Split(',');
        if (parts.Length != 3)
        {
            return false;
        }

        if (!byte.TryParse(parts[0], out byte r) ||
            !byte.TryParse(parts[1], out byte g) ||
            !byte.TryParse(parts[2], out byte b))
        {
            return false;
        }

        color = Color.FromArgb(r, g, b);
        return true;
    }
}
```

**設計が意味すること**

`IniFile.GetKeys`が返す順序は`Dictionary`の内部実装に依存し、書いた順とは
限らない。そのため、キー名に埋め込んだ番号を明示的に取り出して`OrderBy`で
並べ替えることで、格納順序をパーサの内部実装から切り離している。

歯抜けを許すぶん、途中の色を1つ削除・追加しても他のキーに触れなくてよい。
一方で、「意図した歯抜け」なのか「typoで書き間違えたキー」なのかをコード側で
区別できないため、想定より少ない件数が読み込まれても異常検知はできない。

## 3. 1行にまとめてデリミタ2階層で持つ方式

```ini
[Colors]
List=120,56,72;200,100,50;10,20,30
```

```csharp
using System.Collections.Generic;
using System.Drawing;

public sealed class DelimitedLineIniColorLoader
{
    private const string SectionName = "Colors";
    private const string KeyName = "List";

    public IReadOnlyList<Color> Load(IniFile ini)
    {
        var colors = new List<Color>();

        if (!ini.TryGetValue(SectionName, KeyName, out string rawValue))
        {
            return colors;
        }

        foreach (string entry in rawValue.Split(';'))
        {
            if (TryParseColor(entry, out Color color))
            {
                colors.Add(color);
            }
        }

        return colors;
    }

    private static bool TryParseColor(string rawValue, out Color color)
    {
        color = default;
        string[] parts = rawValue.Split(',');
        if (parts.Length != 3)
        {
            return false;
        }

        if (!byte.TryParse(parts[0], out byte r) ||
            !byte.TryParse(parts[1], out byte g) ||
            !byte.TryParse(parts[2], out byte b))
        {
            return false;
        }

        color = Color.FromArgb(r, g, b);
        return true;
    }
}
```

**設計が意味すること**

キーが1つで済むため、連番管理そのものが不要になる。ただし、`R,G,B`という
1階層目の区切り文字（カンマ）に加えて、色同士を区切る2階層目の区切り文字
（セミコロン）が必要になり、パースがその分複雑になる。また、色数が増えるほど
1行が長くなり、Gitの差分（diff）で「どの色が変わったか」が読み取りづらくなる。

## 4. 固定名キー方式（参考・可変長には非推奨）

```ini
[Colors]
HpBarColor=120,56,72
BackgroundColor=30,30,30
WarningColor=200,40,40
```

```csharp
using System;
using System.Drawing;

public sealed class NamedKeyColorSettings
{
    private const string SectionName = "Colors";

    public Color HpBarColor { get; }
    public Color BackgroundColor { get; }
    public Color WarningColor { get; }

    public NamedKeyColorSettings(IniFile ini)
    {
        HpBarColor = ReadColor(ini, "HpBarColor");
        BackgroundColor = ReadColor(ini, "BackgroundColor");
        WarningColor = ReadColor(ini, "WarningColor");
    }

    private static Color ReadColor(IniFile ini, string key)
    {
        if (!ini.TryGetValue(SectionName, key, out string rawValue) ||
            !TryParseColor(rawValue, out Color color))
        {
            throw new InvalidOperationException($"{key} の値が不正です。");
        }
        return color;
    }

    private static bool TryParseColor(string rawValue, out Color color)
    {
        color = default;
        string[] parts = rawValue.Split(',');
        if (parts.Length != 3)
        {
            return false;
        }

        if (!byte.TryParse(parts[0], out byte r) ||
            !byte.TryParse(parts[1], out byte g) ||
            !byte.TryParse(parts[2], out byte b))
        {
            return false;
        }

        color = Color.FromArgb(r, g, b);
        return true;
    }
}
```

**設計が意味すること**

キー名がそのまま用途を表すため、コードを読むだけで「何の色か」が分かる。
ただし、色を1つ増やすたびに「プロパティを増やす」「コンストラクタで読み込む」
「必要ならリストにまとめる」という3箇所を直す必要がある。今回のように
色の数が可変（5〜20色）で、用途も個別に決まっていない場合は、増減のたびに
コード修正が発生するこの方式は不向き。あくまで、色の数と用途が固定されている
設定（背景色・警告色など数個限定）向けのパターンとして参考に載せている。

## 結論

今回は「連番＋打ち切り方式」を採用した。理由は次の3点。

1. 色数の上限をコード側に決め打ちしなくてよい（自己終端）
2. 最終的に格納する`List<Color>`自体が「歯抜けを表現できない」データ構造なので、
   ini側にも同じ制約を課すことで、ファイルと読み込み結果の対応が常に一致する
3. 番号の振り直し漏れやtypoによる欠番が起きた場合、読み込み件数が大きくずれる
   ため、ミスに気づきやすい

「途中の色だけ気軽に抜き差ししたい」という手編集の頻度が高くなるようなら、
パターン2（歯抜け許容・番号順ソート）への切り替えを検討する。
