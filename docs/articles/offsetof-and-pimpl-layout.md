# offsetof とレイアウト構造体を使ったバイナリpatch設計

## はじめに

バイナリバッファを扱う処理では、「バッファ内のどのフィールドが何バイト目から始まるか」を正確に把握する必要があります。

このとき、バッファのレイアウトを表す構造体があれば、`offsetof` マクロを使ってフィールドの位置を安全に取得できます。

さらに実際の現場コードでは、Pimplに近いパターンで `BufferImpl` が `PlayData` を内包しているケースがあります。この記事では、そのような構造での `offsetof` の使い方と、patch処理の設計を整理します。

---

## 1. offsetof の基本

バッファのレイアウトを表す構造体があるとします。

```cpp
#include <cstdint>
#include <cstddef>

struct A { uint8_t data[4];  };
struct B { uint8_t data[8];  };
struct C { uint8_t data[16]; };

struct BufferLayout
{
    A a;
    B b;
    C c;
};
```

このとき、`c` が `BufferLayout` の先頭から何バイト目にあるかは `offsetof` で取得できます。

```cpp
constexpr size_t cOffset = offsetof(BufferLayout, c);
// → 12  (A=4バイト + B=8バイト の後ろ)
```

`BufferLayout` が実際のバッファのレイアウトと一致していれば、この値はそのままバッファ内での `C` 領域の開始位置として使えます。

---

## 2. offsetof を使った完全なサンプル

```cpp
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iostream>

struct A { uint8_t data[4];  };
struct B { uint8_t data[8];  };
struct C { uint8_t data[16]; };

struct BufferLayout
{
    A a;
    B b;
    C c;
};

int main()
{
    uint8_t buffer[sizeof(BufferLayout)] = {};

    // buffer を BufferLayout として見る（コピーではなく、同じメモリを別の型で解釈する）
    auto* layout = reinterpret_cast<BufferLayout*>(buffer);

    constexpr size_t cOffset = offsetof(BufferLayout, c);
    std::cout << "C offset: " << cOffset << std::endl;   // 12
    std::cout << "C size:   " << sizeof(layout->c) << std::endl;  // 16

    // C 領域に差し替えるデータ
    uint8_t patchData[16] = {
        9, 9, 9, 9,
        9, 9, 9, 9,
        9, 9, 9, 9,
        9, 9, 9, 9
    };

    // 構造体フィールド経由で C を書き換える
    // 実体は buffer[cOffset] 以降が書き換わっている
    std::memcpy(&layout->c, patchData, sizeof(layout->c));

    for (size_t i = 0; i < sizeof(layout->c); ++i)
    {
        std::cout << static_cast<int>(buffer[cOffset + i]) << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

`offsetof(BufferLayout, c)` が返すのは「`BufferLayout` の先頭から見た `c` の開始位置」です。`BufferLayout` が実バッファのレイアウトと一致しているなら、それはそのまま「バッファの先頭から見た `C` 領域の開始位置」として使えます。

---

## 3. Pimpl + レイアウト構造体パターン

現場コードでは、次のような構造を持つことがあります。

```
Data クラス（外向きのインターフェース）
└─ BufferImpl（内部実装の隠蔽）
   └─ PlayData（実際のバッファレイアウト定義）
      ├─ header[4]
      ├─ owner[16]
      ├─ playMode[4]
      └─ reserved[32]
```

コードで書くとこうです。

```cpp
struct PlayData
{
    uint8_t header[4];
    uint8_t owner[16];
    uint8_t playMode[4];
    uint8_t reserved[32];
};

class Data
{
private:
    struct BufferImpl
    {
        PlayData data;
    };
    BufferImpl* impl = nullptr;
};
```

この設計では、`PlayData` が「バイナリデータのレイアウト定義」そのものです。単なるデータ構造ではなく、バッファ内の区画を名前付きフィールドとして表現しています。

---

## 4. ネストした構造体での offsetof

`PlayData` 内のフィールド位置は次のように取得します。

```cpp
constexpr size_t playModeOffset = offsetof(PlayData, playMode);
```

`BufferImpl` の先頭から `playMode` までの位置が必要な場合は、両方の offset を足します。

```cpp
constexpr size_t fieldOffsetInBufferImpl =
    offsetof(BufferImpl, data) + offsetof(PlayData, playMode);
```

`BufferImpl` に `PlayData data;` しかない場合、`offsetof(BufferImpl, data)` は `0` になるため、実質的に `offsetof(PlayData, playMode)` だけで十分です。

実際の出力で確認するとこうなります。

```cpp
#include <iostream>
#include <cstdint>
#include <cstddef>

struct PlayData
{
    uint8_t header[4];
    uint8_t owner[16];
    uint8_t playMode[4];
    uint8_t reserved[32];
};

struct BufferImpl
{
    PlayData data;
};

int main()
{
    std::cout << "offset data in BufferImpl:     "
              << offsetof(BufferImpl, data) << std::endl;
    std::cout << "offset owner in PlayData:      "
              << offsetof(PlayData, owner) << std::endl;
    std::cout << "offset playMode in PlayData:   "
              << offsetof(PlayData, playMode) << std::endl;
    std::cout << "offset playMode in BufferImpl: "
              << offsetof(BufferImpl, data) + offsetof(PlayData, playMode)
              << std::endl;
    std::cout << "sizeof(PlayData):              "
              << sizeof(PlayData) << std::endl;

    return 0;
}
```

出力イメージ：

```
offset data in BufferImpl:     0
offset owner in PlayData:      4
offset playMode in PlayData:   20
offset playMode in BufferImpl: 20
sizeof(PlayData):              56
```

---

## 5. patch処理の実装

`Data` クラス内で `playMode` フィールドを差し替えるなら、次のように書きます。

```cpp
#include <vector>
#include <cstring>

class Data
{
private:
    struct BufferImpl
    {
        PlayData data;
    };
    BufferImpl* impl = nullptr;

public:
    bool patchPlayMode(const std::vector<uint8_t>& patchData)
    {
        if (impl == nullptr)
        {
            return false;
        }
        if (patchData.size() != sizeof(impl->data.playMode))
        {
            return false;
        }
        std::memcpy(impl->data.playMode,
                    patchData.data(),
                    patchData.size());
        return true;
    }
};
```

`impl->data.playMode` と書くことで、`PlayData` 内の `playMode` 領域を直接指しています。

---

## 6. 最も重要な設計上の問いかけ：コピーか、同じメモリか

`impl->data` を書き換えたとき、それが元のバッファに反映されるかどうかは、`impl` の初期化方法によって決まります。これが設計全体を左右します。

### パターンA：BufferImpl がコピーを持っている

```cpp
BufferImpl impl;
std::memcpy(&impl.data, buffer.data(), sizeof(PlayData));
```

```
buffer  →  impl.data にコピー  →  別物
```

`impl.data` を書き換えても、元の `buffer` は変わりません。書き戻しが必要です。

```cpp
// patch後、元のbufferへ手動で書き戻す
std::memcpy(buffer.data(), &impl.data, sizeof(PlayData));
```

### パターンB：BufferImpl がバッファを直接見ている

```cpp
BufferImpl* impl = reinterpret_cast<BufferImpl*>(buffer.data());
```

```
buffer と impl は同じメモリ
```

`impl->data` を書き換えると、`buffer` も同時に変わります。書き戻しは不要です。

---

## 7. どちらのパターンかを判断するチェックリスト

| 確認項目 | 内容 |
|---|---|
| `impl` はどこで初期化されているか | `new BufferImpl` か `reinterpret_cast` か |
| `PlayData data` はどのように値が入っているか | `memcpy` コピーか、バッファを直接指しているか |
| patch後にファイルへの書き戻しが必要か | コピーパターンでは必要 |

---

## 8. padding に関する注意

`PlayData` が全て `uint8_t[]` で構成されている場合は、paddingの影響を受けにくいです。

```cpp
// 安全：全フィールドが uint8_t 配列
struct PlayData
{
    uint8_t header[4];
    uint8_t owner[16];
    uint8_t playMode[4];
};
```

しかし、次のように `int` や `double` が混在するとpaddingが入ることがあります。

```cpp
// 要注意：padding が入る可能性がある
struct PlayData
{
    uint8_t a;   // 1バイト
    int b;       // paddingが入り、offset 4 から配置されることがある
};
```

このような場合は `offsetof` で実際の位置を確認するべきです。バイナリレイアウト用の構造体は、全フィールドを `uint8_t[]` にしておくのが安全です。

---

## 9. patch前に必ず行う検証

```cpp
// impl が有効か
if (impl == nullptr)
{
    return false;
}

// buffer が構造体として扱えるサイズか
if (bufferSize < sizeof(BufferLayout))
{
    return false;
}

// patchData のサイズがフィールドサイズと一致するか
if (patchData.size() != sizeof(impl->data.playMode))
{
    return false;
}
```

`memcpy` は意味を理解しません。サイズ分だけ機械的にコピーします。意味の検証は設計側で行う必要があります。

---

## まとめ

### offsetof の役割

```
バッファのレイアウト構造体がある
↓
offsetof(LayoutStruct, フィールド名) でbuffer内offsetが分かる
↓
layout->フィールド名 経由でその領域を書き換えられる
```

### ネストした構造体での位置計算

```cpp
// PlayData 内でのフィールド位置
constexpr size_t offset = offsetof(PlayData, playMode);

// BufferImpl 先頭からの位置（BufferImpl に PlayData しかない場合は実質同じ）
constexpr size_t offset = offsetof(BufferImpl, data) + offsetof(PlayData, playMode);
```

### 設計の核心

`impl->data` を書き換えたとき、それが元バッファに反映されるかは `impl` の初期化方法次第です。

| 初期化方法 | 動作 |
|---|---|
| `new BufferImpl` + `memcpy` | コピー。元バッファへの書き戻しが必要 |
| `reinterpret_cast<BufferImpl*>(buffer.data())` | 同じメモリ。書き戻し不要 |

patch処理を実装する前に、この区別を必ず確認することが設計上の最重要ポイントです。
