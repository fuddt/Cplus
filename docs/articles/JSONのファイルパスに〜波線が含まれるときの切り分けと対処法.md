# JSONのファイルパスに `〜` 波線が含まれるときの切り分けと対処法

---

## はじめに

JSON に書かれたファイルパスを C++ で読み取り、そのまま `std::ifstream` で開く処理を書いていた。

ところがファイル名に `〜` が入ると、どうにも挙動が怪しい。

たとえばこういうファイル名だ。

```text
テスト〜２０２０（最新版）.txt
```

私はこの件で 8 時間以上はまった。

最初は「`〜` という文字そのものが悪いのでは」と思っていたが、実際に切り分けてみると、問題はもっと整理して考える必要があった。

この記事では、

- `〜` を含むファイルパスは本当に扱えないのか
- `get<std::string>()` と `get_ref<const json::string_t&>()` の違いは何か
- どこでエラーになるのか

を、実験ベースで整理する。

---

## 結論

先に結論を書く。

1. **`〜` を含むファイルパス自体は、UTF-8 として正しい JSON なら普通に扱える。**
2. **`get_ref<const json::string_t&>()` は便利だが、万能の特効薬ではない。**
3. **壊れるなら、まず疑うべきは `〜` そのものではなく、文字コード・文字の不一致・途中の変換処理である。**

つまり、

```cpp
j.at("name").get<std::string>()
```

でも通るケースは普通にある。

`get_ref` は、

```cpp
j.at("name").get_ref<const json::string_t&>()
```

のように書くことで、**「JSON が内部に保持している文字列を、そのまま参照する」意図を明確にできる**書き方だ。

---

## まず誤解しやすい点

### `〜` と `～` は別文字

見た目は似ているが、これは別文字だ。

```text
〜  波ダッシュ
～  全角チルダ
```

ファイルパスとしては、1 文字違えば別ファイル名である。

つまり、

```text
テスト〜２０２０.txt
テスト～２０２０.txt
```

は別名だ。

表示上は似ていても、片方で作ったファイルをもう片方の文字で開こうとすれば失敗する。

---

### `get_ref` は「パース前の生バイト」を拾う機能ではない

ここも誤解しやすい。

`get_ref<const json::string_t&>()` が返すのは、

**JSON を `parse()` したあとに、nlohmann::json が内部保持している文字列**

への参照である。

つまり流れはこうだ。

```text
JSONファイルを読む
    ↓
json::parse() が JSON 文字列として解釈する
    ↓
json の内部に string_t として保存される
    ↓
get_ref<const json::string_t&>() で参照する
```

したがって、**JSON 自体が不正 UTF-8 なら `get_ref` に到達する前に `parse()` で落ちる**。

---

## `get<std::string>()` と `get_ref<const json::string_t&>()` の違い

比較するとこうなる。

```cpp
std::string a = j.at("name").get<std::string>();
const std::string& b = j.at("name").get_ref<const json::string_t&>();
```

違いは主に次の2点だ。

| 書き方 | 意味 |
|---|---|
| `get<std::string>()` | `std::string` を新しく受け取る |
| `get_ref<const json::string_t&>()` | JSON 内部の文字列を参照する |

重要なのは、**正常な UTF-8 の JSON に対しては、この2つで結果の文字列が変わるとは限らない**ことだ。

今回の再現実験でも、両者は同じ結果だった。

---

## 実験1: `〜` を含むファイルパスは実際に開けるのか

まず、一時ディレクトリに次のようなファイルを実際に作った。

```text
/tmp/.../テスト〜２０２０（最新版）.txt
```

そして、その絶対パスを JSON に入れた。

```json
{
  "name": "/tmp/テスト〜２０２０（最新版）.txt",
  "data": [
    {
      "place": "東京",
      "distance": 2400
    }
  ]
}
```

そのうえで `Sample::validateJson()` に渡し、`getName()` と `data[0]` を返す `getPlace()` / `getDistance()` を確認したあと、`getName()` で取り出した文字列を `std::ifstream` に渡して実際に開いた。

テストコードの骨格はこうだ。

```cpp
nlohmann::json j = nlohmann::json::parse(jsonInput);

Sample sample;
std::string error;
if (!sample.validateJson(j, error)) {
    std::cerr << "validate failed: " << error << "\n";
    return 1;
}

std::ifstream target(sample.getName(), std::ios::binary);
if (!target) {
    std::cerr << "target open failed: " << sample.getName() << "\n";
    return 1;
}
```

実行結果はこうだった。

```text
name=/tmp/cplus-sample-test.dJjrIv/テスト〜２０２０（最新版）.txt
place=東京
distance=2400
body=sample-body
```

つまりこのケースでは、`〜` を含むファイルパスは **JSON -> Sample -> std::ifstream** の流れで普通に扱えた。

---

## 実験2: `get` と `get_ref` で差は出るのか

次に、同じ JSON 値から `get<std::string>()` と `get_ref<const json::string_t&>()` を比較した。

コードはこうだ。

```cpp
const std::string path = "/tmp/テスト〜２０２０（最新版）.txt";
json j = {
    {"name", path},
    {"place", "東京"},
    {"distance", 2400}
};

std::string via_get = j.at("name").get<std::string>();
const std::string& via_ref = j.at("name").get_ref<const json::string_t&>();

std::cout << "get=" << via_get << "\n";
std::cout << "ref=" << via_ref << "\n";
std::cout << "equal=" << (via_get == via_ref) << "\n";
```

結果はこうだった。

```text
get=/tmp/テスト〜２０２０（最新版）.txt
ref=/tmp/テスト〜２０２０（最新版）.txt
equal=1
```

この結果から分かるのは、

**有効な UTF-8 の JSON に対しては、`get<std::string>()` を使っただけで即エラーになるわけではない**

ということだ。

ここはかなり大事だ。

私は最初、`get_ref` を知らなかったことが原因の中心だと思っていた。
しかし実験してみると、`get_ref` を使わないこと自体が必ずしも真因ではなかった。

---

## 実験3: 本当にエラーになるのはどこか

次に、**無効な UTF-8 バイトを含む JSON** を用意して `parse()` させた。

例として、文字列中に不正な `0x80` を混ぜた JSON を作る。

```json
{"name":"abc\x80def"}
```

もちろんこれは概念的な説明で、実際にはバイナリとして不正 UTF-8 を埋め込んでいる。

その結果はこうだった。

```text
exception=[json.exception.parse_error.101] parse error at line 1, column 13: syntax error while parsing value - invalid string: ill-formed UTF-8 byte; last read: '"abc�'
```

つまり、**このケースは `get_ref` を使う・使わない以前に `json::parse()` で失敗する。**

ここを切り分けないまま、

- `〜` が悪い
- `std::string` 変換が悪い
- `get_ref` を使えば全部直る

と考えてしまうと、原因を見誤る。

---

## 今回の `Sample` 実装でやったこと

今回の `Sample` では、`name` を「表示用の文字列」ではなく **ファイルパス** として扱うようにした。

そのため、文字を勝手に置換しない方針にしている。

```cpp
void Sample::rebuildName(std::string& name) const {
    // ファイルパスとして使うので、波線を含めて文字は置換しない。
    // json から取り出した並びをそのまま使う。
    (void)name;
}
```

そして `loadJsonFile()` では、`name` をこう取り出している。

```cpp
out.name = data.at("name").get_ref<const json::string_t&>();
rebuildName(out.name);
```

ここでの意図は、

**「JSON が内部保持している文字列を、そのままファイルパスとして扱う」**

ことを明示することにある。

`〜` を `～` に直したり、逆に `～` を `〜` に寄せたりすると、ファイルパスとしては別名になってしまう可能性がある。

ファイルパスを扱うなら、見た目を整えるための正規化は危険だ。

---

## 実務での切り分け手順

この手の問題では、次の順番で疑うとよい。

### 1. JSON ファイル自体が UTF-8 として正しいか

まずここを疑う。

`nlohmann/json` は UTF-8 前提で文字列を扱うため、不正なバイト列を含む JSON は `parse()` で落ちる。

---

### 2. `〜` と `～` を取り違えていないか

これも非常に多い。

見た目が似ているため、JSON に書いた文字と、実際に存在するファイル名の文字がズレていることがある。

---

### 3. 途中で文字列を変換・正規化していないか

たとえば、

- 独自の UTF-8 変換
- `wstring` との相互変換
- 波線の置換
- 表示用の整形処理

などが入ると、そこでファイルパスが壊れる可能性がある。

---

### 4. 必要なら `get_ref` で「ここでは再解釈していない」と明示する

`get_ref<const json::string_t&>()` は、問題を魔法のように解決するわけではない。

ただし、

**「JSON 内部の文字列を、そのまま使う」**

という意図をコード上ではっきり示せる。

この意味で、ファイルパスのように勝手な補正をしたくない値には相性がよい。

---

## まとめ

私は今回、`get_ref` を知らずに 8 時間以上苦しんだ。

ただ、実験してみて分かったのは、

**本質は `get_ref` という API 名を知っているかどうかだけではなく、どこで壊れているのかを切り分けること**

だった。

今回の学びを一言でまとめるとこうなる。

1. `〜` を含むファイルパス自体は扱える
2. `get_ref` は便利だが万能解ではない
3. 真っ先に疑うべきは UTF-8 不正、文字の不一致、途中の変換処理
4. ファイルパスは見た目のために勝手に置換しない

もし JSON から読んだファイルパスで問題が起きたら、まずは

- JSON が UTF-8 として正しいか
- `〜` と `～` が一致しているか
- 途中で文字列をいじっていないか

を確認してほしい。

そのうえで、「ここでは JSON 内部の文字列をそのまま扱う」と意図をはっきり書きたいなら、`get_ref<const json::string_t&>()` はとても良い選択肢になる。
