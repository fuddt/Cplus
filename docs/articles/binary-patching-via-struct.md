# バッファを構造体として扱い、フィールド経由でバイナリパッチを当てる設計

## はじめに

バイナリデータを扱う処理では、ファイルや通信データを読み込んだ結果が、まず `uint8_t*` や `void*` のような「生のバッファ」として渡ってくることがあります。

この時点のバッファは、ただの連続したバイト列です。

```cpp
uint8_t buffer[12] = {
    20, 24, 4, 25,
    1, 2, 3, 4,
    80, 81, 82, 83
};
```

このままでは、どこからどこまでが何のデータなのか分かりません。

しかし、データのレイアウトが決まっている場合は、構造体を使って次のように解釈できます。

```cpp
struct MetaData
{
    uint8_t birthday[4];
    uint8_t owner[4];
    uint8_t jockey[4];
};
```

この構造体は、バッファを以下のように見るための設計図です。

```
offset 0〜3   birthday
offset 4〜7   owner
offset 8〜11  jockey
```

つまり、構造体を使うことで、生のバイト列に対して「ここは birthday」「ここは owner」といった名前付きフィールドとしてアクセスできるようになります。

この記事では、この考え方を使って、バッファを構造体として解釈し、構造体フィールド経由で一部データを差し替える方法を整理します。

---

## 1. バッファは最初、ただのバイト列である

たとえば、次のようなバッファがあるとします。

```cpp
uint8_t buffer[12] = {
    20, 24, 4, 25,
    1, 2, 3, 4,
    80, 81, 82, 83
};
```

メモリ上では、次のように並んでいます。

```
index 0   20
index 1   24
index 2   4
index 3   25
index 4   1
index 5   2
index 6   3
index 7   4
index 8   80
index 9   81
index 10  82
index 11  83
```

この状態では、C++にとっては単なる12バイトの配列です。

人間が、

```
0〜3 は birthday
4〜7 は owner
8〜11 は jockey
```

と知っているだけで、C++側にはその意味はありません。

---

## 2. 構造体は「バッファに意味を与えるレイアウト」

ここで、次のような構造体を定義します。

```cpp
#include <cstdint>
struct MetaData
{
    uint8_t birthday[4];  // offset 0〜3
    uint8_t owner[4];     // offset 4〜7
    uint8_t jockey[4];    // offset 8〜11
};
```

この構造体は、バッファの中身を次のように区切るためのレイアウトです。

```
MetaData
birthday[4]  → 先頭4バイト
owner[4]     → 次の4バイト
jockey[4]    → 次の4バイト
```

つまり、構造体は「メモリに名前を付ける」ものに近いです。

ただし、より正確には、

> 連続したメモリ領域を、名前付きの区画として解釈するための設計図

です。

---

## 3. `void*` や `uint8_t*` を構造体として見る

現場コードでは、バッファが `void*` として渡ってくることがあります。

```cpp
void process(void* rawBuffer)
{
    // rawBuffer を MetaData として扱う
}
```

`void*` は「型情報を持たない生アドレス」です。

この `void*` を `MetaData*` として扱うには、次のように `static_cast` します。

```cpp
MetaData* meta = static_cast<MetaData*>(rawBuffer);
```

これは、`rawBuffer` の中身を別の場所にコピーしているわけではありません。

正しくは、

> `rawBuffer` が指している同じメモリ領域を、`MetaData` 型の構造体として見る

という意味です。

---

## 4. 「変換」ではなく「構造体ビュー」と考える

ここは非常に重要です。

```cpp
MetaData* meta = static_cast<MetaData*>(rawBuffer);
```

この処理は、バッファを構造体に変換しているわけではありません。

実体は同じメモリです。

```
buffer実体:
[20][24][4][25][1][2][3][4][80][81][82][83]
  ↑
  rawBuffer が指している
  ↑
  meta も同じ場所を MetaData として見ている
```

そのため、

```cpp
meta->owner[0] = 9;
```

と書くと、実際には `buffer[4]` が書き換わります。

つまり、構造体はコピー先ではなく、同じバッファを見ているビューです。

---

## 5. 構造体フィールド経由でパッチを当てる

たとえば、`owner` フィールドだけを差し替えたいとします。

```cpp
#include <iostream>
#include <cstdint>
#include <cstring>
#include <vector>

struct MetaData
{
    uint8_t birthday[4];
    uint8_t owner[4];
    uint8_t jockey[4];
};

bool patchOwner(void* rawBuffer,
                size_t bufferSize,
                const std::vector<uint8_t>& patchData)
{
    if (rawBuffer == nullptr)
    {
        return false;
    }
    if (bufferSize < sizeof(MetaData))
    {
        return false;
    }
    MetaData* meta = static_cast<MetaData*>(rawBuffer);
    if (patchData.size() != sizeof(meta->owner))
    {
        return false;
    }
    std::memcpy(meta->owner,
                patchData.data(),
                sizeof(meta->owner));
    return true;
}

int main()
{
    uint8_t buffer[12] = {
        20, 24, 4, 25,
        1, 2, 3, 4,
        80, 81, 82, 83
    };
    std::vector<uint8_t> patchData = {
        9, 9, 9, 9
    };
    bool ok = patchOwner(buffer,
                         sizeof(buffer),
                         patchData);
    std::cout << "patch result: " << ok << std::endl;
    for (uint8_t b : buffer)
    {
        std::cout << static_cast<int>(b) << " ";
    }
    std::cout << std::endl;
    return 0;
}
```

出力はこうなります。

```
patch result: 1
20 24 4 25 9 9 9 9 80 81 82 83
```

`owner` に相当する `buffer[4]`〜`buffer[7]` だけが書き換わりました。

---

## 6. offset方式との違い

バッファを直接操作する場合は、次のように書きます。

```cpp
std::memcpy(buffer + 4, patchData.data(), 4);
```

これは、「buffer の 4 バイト目から 4 バイト分を書き換える」という意味です。

一方、構造体経由ならこう書けます。

```cpp
std::memcpy(meta->owner,
            patchData.data(),
            sizeof(meta->owner));
```

こちらは、「`owner` フィールドを書き換える」という意味がコード上に現れます。

つまり、構造体経由のメリットは、offsetの数字ではなく、フィールド名で操作できることです。

---

## 7. 構造体経由のメリット

### 7-1. コードの意味が読みやすい

```cpp
std::memcpy(meta->owner,
            patchData.data(),
            sizeof(meta->owner));
```

このコードを見れば、`owner` を書き換えていることが分かります。

一方、offset方式では、

```cpp
std::memcpy(buffer + 4,
            patchData.data(),
            4);
```

となるため、`4` が何を意味するのかは、別途レイアウト表を見ないと分かりません。

### 7-2. `sizeof(field)` を使える

構造体経由では、対象フィールドのサイズを次のように取得できます。

```cpp
sizeof(meta->owner)
```

これにより、`4` や `16` のようなマジックナンバーを避けられます。

### 7-3. 既存コードに乗りやすい

現場で既に次のような処理がある場合、

```cpp
MetaData* meta = static_cast<MetaData*>(rawBuffer);
```

後続処理も `meta->xxx` で進んでいるなら、パッチ処理も構造体フィールド経由で書くのが自然です。既存設計の世界観に沿えるため、無理に別のlayout table方式を追加しなくて済みます。

---

## 8. 構造体経由のデメリット

### 8-1. レイアウト一致が前提

構造体経由の最大の前提は、構造体の並びと実際のバッファの並びが完全に一致していることです。

構造体を間違えて定義すると、

```cpp
struct WrongMetaData
{
    uint8_t birthday[4];
    uint8_t jockey[4];  // ownerとjockeyが逆
    uint8_t owner[4];
};
```

`owner` の位置がズレ、本来の `owner` ではなく別の領域を書き換えてしまいます。

### 8-2. padding / alignment の罠がある

```cpp
struct BadLayout
{
    uint8_t a;
    int b;
};
```

見た目では合計5バイトに見えますが、`int` を4バイト境界に配置するためpaddingが入ります。

```
offset 0    a
offset 1〜3 padding
offset 4〜7 b
sizeof(BadLayout) = 8
```

バイナリレイアウト用の構造体では、`uint8_t[]` で固定バイト領域として定義する方が安全です。

```cpp
struct SafeLayout
{
    uint8_t magic[4];
    uint8_t version[4];
    uint8_t owner[16];
    uint8_t reserved[8];
};
```

### 8-3. `static_cast` はサイズ確認をしてくれない

```cpp
MetaData* meta = static_cast<MetaData*>(rawBuffer);
```

この時点でC++は以下を確認してくれません。

- `rawBuffer` が本当に `MetaData` なのか
- `rawBuffer` のサイズが `sizeof(MetaData)` 以上あるのか
- `rawBuffer` が正しい位置を指しているのか

そのため、`static_cast` 前後で明示的な検証が必要です。

---

## 9. ヘッダーを見て適切な構造体を選ぶ

複数フォーマットがある場合は、バッファのヘッダー情報を見て、対応する構造体を選ぶ設計にします。

```cpp
struct MetaA
{
    uint8_t magic[4];      // "MTA1"
    uint8_t version[4];
    uint8_t owner[16];     // offset 8
    uint8_t reserved[8];
};

struct MetaB
{
    uint8_t magic[4];      // "MTB1"
    uint8_t version[4];
    uint8_t timestamp[8];
    uint8_t owner[16];     // offset 16
    uint8_t reserved[8];
};
```

`MetaA` と `MetaB` では `owner` の位置が違うため、`magic` を見て判定します。

```cpp
enum class FormatType
{
    Unknown,
    MetaA,
    MetaB
};

FormatType detectFormat(const uint8_t* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize < 4)
    {
        return FormatType::Unknown;
    }
    if (std::memcmp(buffer, "MTA1", 4) == 0)
    {
        return FormatType::MetaA;
    }
    if (std::memcmp(buffer, "MTB1", 4) == 0)
    {
        return FormatType::MetaB;
    }
    return FormatType::Unknown;
}
```

---

## 10. フォーマット別に構造体経由でpatchする

```cpp
bool patchOwnerForMetaA(void* rawBuffer,
                        size_t bufferSize,
                        const std::vector<uint8_t>& patchData)
{
    if (rawBuffer == nullptr) return false;
    if (bufferSize < sizeof(MetaA)) return false;
    MetaA* meta = static_cast<MetaA*>(rawBuffer);
    if (patchData.size() != sizeof(meta->owner)) return false;
    std::memcpy(meta->owner, patchData.data(), sizeof(meta->owner));
    return true;
}

bool patchOwnerForMetaB(void* rawBuffer,
                        size_t bufferSize,
                        const std::vector<uint8_t>& patchData)
{
    if (rawBuffer == nullptr) return false;
    if (bufferSize < sizeof(MetaB)) return false;
    MetaB* meta = static_cast<MetaB*>(rawBuffer);
    if (patchData.size() != sizeof(meta->owner)) return false;
    std::memcpy(meta->owner, patchData.data(), sizeof(meta->owner));
    return true;
}

bool patchOwner(void* rawBuffer,
                size_t bufferSize,
                const std::vector<uint8_t>& patchData)
{
    if (rawBuffer == nullptr) return false;
    uint8_t* bytes = static_cast<uint8_t*>(rawBuffer);
    FormatType format = detectFormat(bytes, bufferSize);
    switch (format)
    {
    case FormatType::MetaA:
        return patchOwnerForMetaA(rawBuffer, bufferSize, patchData);
    case FormatType::MetaB:
        return patchOwnerForMetaB(rawBuffer, bufferSize, patchData);
    default:
        return false;
    }
}
```

この流れにすると、

```
ヘッダーを見る
↓
フォーマットを判定する
↓
対応する構造体として扱う
↓
構造体フィールド経由でpatchする
```

という設計になります。

---

## 11. この方式の本質

この方式は言い換えると、**構造体そのものを layout table として使う方式**です。

offset方式では、

```cpp
LayoutTable layoutA = {
    { FieldID::Owner, {8, 16} }
};
```

と書くところを、構造体方式では offset/size を構造体定義で表現します。

```cpp
struct MetaA
{
    uint8_t magic[4];      // offset 0
    uint8_t version[4];    // offset 4
    uint8_t owner[16];     // offset 8
};
```

つまり、

- `MetaA` = Format A 用のレイアウト表
- `MetaB` = Format B 用のレイアウト表

です。

---

## 12. 構造体経由が向いているケース

- 既に `buffer → 構造体ビュー化` の処理がある
- フォーマット判定が既にある
- 対象フィールドが構造体に定義されている
- 後続処理もその構造体を使っている
- レイアウトが固定されている
- コードの読みやすさを重視したい

この場合は、構造体フィールド経由が第一候補になります。

---

## 13. offset / size方式が向いているケース

- フォーマット数が多い
- 外部設定でpatch対象を変えたい
- 同じpatchエンジンを完全に共通化したい
- 構造体を増やしたくない
- レイアウトが動的に変わる

この場合は、以下のような方式が有効です。

```cpp
struct FieldDescriptor
{
    size_t offset;
    size_t size;
};

std::memcpy(buffer + field.offset,
            patchData.data(),
            field.size);
```

---

## 14. 実装時に必ず確認すべきこと

1. `rawBuffer` が `nullptr` ではないか
2. `bufferSize` が `sizeof(対象構造体)` 以上あるか
3. ヘッダー情報から正しいフォーマットを判定できているか
4. 構造体のレイアウトが実データと一致しているか
5. padding / alignment の問題がないか
6. `patchData.size()` が対象フィールドのサイズと一致しているか
7. 書き換え後に checksum / length / index などの更新が必要ないか

特に重要なのは、

```cpp
if (patchData.size() != sizeof(meta->owner))
{
    return false;
}
```

のように、差し替えデータと対象フィールドのサイズを必ず比較することです。

`memcpy` は意味を見ません。サイズ分だけ機械的にコピーします。だから、意味の検証は設計側で行う必要があります。

---

## まとめ

バイナリデータを扱うとき、構造体は単なるデータ入れではありません。

> 構造体は、バッファ内の連続したバイト列を、名前付きフィールドとして解釈するためのレイアウト定義です。

既存処理がすでに

```
void* rawBuffer
↓
static_cast<MetaData*>(rawBuffer)
↓
meta->field
```

という流れで作られているなら、パッチ処理も構造体フィールド経由で行うのが自然です。

ただし、これは次の前提を満たす場合に限ります。

- 正しいフォーマット判定ができる
- 構造体サイズ分のバッファがある
- 構造体レイアウトが実データと一致している
- `patchData` のサイズが対象フィールドと一致している

### 最終判断

| ケース | 方式 |
|---|---|
| 固定フォーマット・既存構造体あり | 構造体フィールド経由でpatch |
| 可変フォーマット・汎用patchエンジン化 | offset / size方式でpatch |

どちらが常に正しいというより、既存設計と要件に合わせて選ぶのが重要です。
