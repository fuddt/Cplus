# LineSpan を考慮した左上配置コピーと座標系

## はじめに

今回やりたいことを一言で言うと、これです。

```text
小さい画像 A を、大きいバッファ B の左上にそのままコピーして置く
```

拡大もしない。バイリニア補間もしない。12bit の変換もしない。
やることは **配置・コピー・初期化** の3つだけです。

シンプルに見えますが、「バッファの仕組み」と「座標系の考え方」をちゃんと理解しないと、
バグを埋め込みやすい処理でもあります。
この教材では、そのあたりをひとつずつ丁寧に解説します。

---

## 1. バッファと LineSpan の基礎

### 画像データはメモリ上では「1本の長い配列」

プログラムの中で画像データは、こういう型で扱われます。

```cpp
const std::uint8_t* src;   // 画像Aの先頭アドレス
std::uint8_t*       dst;   // 画像Bの先頭アドレス
```

`uint8_t` は「1バイトの符号なし整数（0〜255）」です。
1ピクセルが1バイトに対応すると考えてください。

2次元の画像に見えますが、メモリの上では1本の長い配列として並んでいます。

```text
[px(0,0)][px(1,0)][px(2,0)]...[px(0,1)][px(1,1)]...
   ↑
 先頭アドレス（row0の先頭）
```

これを「折りたたんで」2次元として扱うのが、画像処理の基本です。

---

### 有効幅と LineSpan は別物

ここが最初のポイントです。

画像の「横の画素数」を `width` と呼びます。
しかし実際のメモリには、その右側に **余白（padding）** が付いていることがあります。

1行が実際にメモリ上で占めるバイト数を **LineSpan**（ラインスパン）と呼びます。
「ストライド（stride）」「ピッチ（pitch）」と呼ばれることもあります。

```text
画像A（width=100, LineSpan=300）

row 0: [有効データ 100byte][padding 200byte]
row 1: [有効データ 100byte][padding 200byte]
row 2: [有効データ 100byte][padding 200byte]
       |<-------- LineSpan = 300byte ------->|
```

padding が存在する理由は、ハードウェアやOSの都合（メモリアラインメント要件）です。
「1行が特定のバイト数の倍数でないといけない」という制約が原因のことが多いです。

**重要:** `width ≤ LineSpan` は常に成り立ちます。
`width == LineSpan` のこともあります（padding ゼロ）。

---

### 行のアドレス計算

「y行目の先頭アドレス」は、こう計算します。

```text
行yの先頭アドレス = 先頭アドレス + y × LineSpan
```

コードで書くとこうです。

```cpp
const std::uint8_t* rowPtr = src + y * srcLineSpan;
```

ここで `srcLineSpan` は `width` ではなく「1行が占める実際のバイト数」を使うことが重要です。
`width` を使ってしまうと、padding の分だけずれていってしまいます。

---

## 2. 座標系の話

ここが今回の教材のメインです。

座標系には大きく2種類あります。プログラムの世界では両方が登場するので、
それぞれの意味と変換方法を理解しておくことが大切です。

---

### 2-1. 左上原点（Top-Left Origin）

**画面座標系・画像処理の標準的な座標系**です。

```text
(0,0)──────────────→ x（右向きが正）
  │
  │
  │
  ↓
  y（下向きが正）
```

グリッドで表すと（横4×縦3の例）：

```text
     x=0   x=1   x=2   x=3
y=0  (0,0) (1,0) (2,0) (3,0)
y=1  (0,1) (1,1) (2,1) (3,1)
y=2  (0,2) (1,2) (2,2) (3,2)
```

**特徴：**
- 左上が (0, 0)
- 右に進むほど x が増える
- **下に進むほど y が増える**（数学とは逆）

**アドレス計算：**

```cpp
// ピクセル (x, y) のアドレス
std::uint8_t* pixel = ptr + y * lineSpan + x;
```

この計算式は「左上原点」を前提にしています。
`y` が増えると「下の行」に進む → `y * lineSpan` でメモリの先頭から離れていく。
これは自然に成り立ちます。

**使われる場面：**
- Windows / macOS の画面描画
- 一般的な画像ファイル（PNG, BMP, JPEG の多くの実装）
- 今回のコピー処理

---

### 2-2. 左下原点（Bottom-Left Origin）

**数学・物理・OpenGLの座標系**です。

```text
  ↑
  y（上向きが正）
  │
  │
  │
(0,0)──────────────→ x（右向きが正）
```

グリッドで表すと（横4×縦3の例）：

```text
     x=0   x=1   x=2   x=3
y=2  (0,2) (1,2) (2,2) (3,2)
y=1  (0,1) (1,1) (2,1) (3,1)
y=0  (0,0) (1,0) (2,0) (3,0)
```

**特徴：**
- 左下が (0, 0)
- 右に進むほど x が増える（左上原点と同じ）
- **上に進むほど y が増える**（数学と同じ）

**使われる場面：**
- OpenGL のテクスチャ座標
- 数学の xy 平面
- BMPファイルの内部（底から行が始まる形式）

---

### 2-3. 2つの座標系を比較する

高さ `H = 4` の画像で、同じ4点が2つの座標系でどう表されるか比べます。

```text
実際の見た目（高さ4の画像）

行0（一番上）: ████████
行1:           ████████
行2:           ████████
行3（一番下）: ████████
```

| 位置           | 左上原点 (x, y) | 左下原点 (x, y) |
|--------------|----------------|----------------|
| 左上の角       | (0, 0)         | (0, 3)         |
| 右上の角       | (W-1, 0)       | (W-1, 3)       |
| 左下の角       | (0, 3)         | (0, 0)         |
| 右下の角       | (W-1, 3)       | (W-1, 0)       |

y だけが違います。x は両方同じです。

---

### 2-4. 変換式を導く

左上原点の `y_top` と、左下原点の `y_bottom` の関係を求めます。

高さ `H` の画像で考えます（y は 0 始まり、最大 H-1）。

```text
左上原点で「一番上の行」は y_top = 0
左下原点で「一番上の行」は y_bottom = H - 1

左上原点で「一番下の行」は y_top = H - 1
左下原点で「一番下の行」は y_bottom = 0
```

2点から直線の関係式を求めると：

```text
y_top が 0 のとき → y_bottom = H - 1
y_top が H-1 のとき → y_bottom = 0
```

これを満たす関係式は：

```text
y_bottom = (H - 1) - y_top
```

整理すると：

```text
y_top    = (H - 1) - y_bottom
y_bottom = (H - 1) - y_top
```

同じ式で双方向に変換できます。

具体例（H = 4）:

| y_top | y_bottom | 計算               |
|-------|----------|--------------------|
| 0     | 3        | (4-1) - 0 = 3      |
| 1     | 2        | (4-1) - 1 = 2      |
| 2     | 1        | (4-1) - 2 = 1      |
| 3     | 0        | (4-1) - 3 = 0      |

---

### 2-5. 今回はどちらを使うか

今回の `copyTopLeft` 処理は、**左上原点で統一**します。

- コピー元A：左上原点
- コピー先B：左上原点

両方が同じ座標系なので、座標変換は不要です。
`dstX = srcX`、`dstY = srcY` がそのまま成り立ちます。

---

## 3. 左上配置コピーの仕組み

### 全体像

```text
B: 300 × 500

+------------------------------+
| A:100×200                    |
| +----------+                 |
| |          |                 |
| |          |                 |
| +----------+                 |
|              ← ここは0埋め    |
|                              |
+------------------------------+
```

AはBの左上にそのまま置きます。

対応関係は：

```text
A(0,0)    → B(0,0)
A(1,0)    → B(1,0)
A(99,0)   → B(99,0)
A(0,199)  → B(0,199)
A(99,199) → B(99,199)
```

`dstX = srcX`、`dstY = srcY` で対応が決まります。

---

### なぜ0埋めが必要か

Bのバッファには、Aをコピーしない領域が残ります。

```text
B の各行:

row y < 200:
[コピー済み100byte][未使用200byte]

row y ≥ 200:
[未使用300byte（全体）]

さらに各行のpadding:
[有効域][padding 300byte]
```

これらの領域に前回の処理の残りカスが入っていると、
「古いデータが透けて見える」ようなバグになります。

そのため、コピー前にB全体を0で初期化します。

```text
B全体を0で初期化
↓
Aの各行の先頭100byteだけを
Bの同じ行の先頭100byteへコピー
↓
Bの右側200byte、下側300行、paddingは0のまま残る
```

---

## 4. C++実装の解説

```cpp
#include <algorithm>
#include <cstdint>
#include <cstring>

void copyTopLeft(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    // [1] B全体を0で初期化
    std::memset(
        dst,
        0,
        static_cast<std::size_t>(dstHeight * dstLineSpan)
    );

    // [2] コピーできる範囲を決める
    int copyWidth  = std::min(srcWidth,  dstWidth);
    int copyHeight = std::min(srcHeight, dstHeight);

    // [3] 行ごとにコピー
    for (int y = 0; y < copyHeight; ++y)
    {
        const std::uint8_t* srcRow = src + y * srcLineSpan;
        std::uint8_t*       dstRow = dst + y * dstLineSpan;

        std::memcpy(dstRow, srcRow, static_cast<std::size_t>(copyWidth));
    }
}
```

### [1] memset でB全体を0に

```cpp
std::memset(dst, 0, static_cast<std::size_t>(dstHeight * dstLineSpan));
```

`memset` は指定したバイト数を一定の値で埋める関数です。
`dstHeight * dstLineSpan` がバッファ全体のバイト数です。

paddingも含めてすべて0になります。

---

### [2] コピー範囲の安全な計算

```cpp
int copyWidth  = std::min(srcWidth,  dstWidth);
int copyHeight = std::min(srcHeight, dstHeight);
```

`std::min` は2つの値の小さいほうを返します。

通常は `srcWidth < dstWidth`、`srcHeight < dstHeight` なので、
`copyWidth = srcWidth`、`copyHeight = srcHeight` になります。

ただし、万が一「Aの方がBより大きい」場合でも、
Bをはみ出して書き込む事故を防ぐための保険です。

---

### [3] 行ごとのコピーループ

```cpp
for (int y = 0; y < copyHeight; ++y)
{
    const std::uint8_t* srcRow = src + y * srcLineSpan;
    std::uint8_t*       dstRow = dst + y * dstLineSpan;

    std::memcpy(dstRow, srcRow, static_cast<std::size_t>(copyWidth));
}
```

- `src + y * srcLineSpan` → Aの y 行目の先頭アドレス
- `dst + y * dstLineSpan` → Bの y 行目の先頭アドレス
- `memcpy` で `copyWidth` バイトをコピー

ここで `srcLineSpan` と `dstLineSpan` を正しく使わないと、
2行目以降のアドレスがずれて壊れた画像になります。

`width` ではなく `LineSpan` を使う — これが今回の処理の核心です。

---

## 5. for文だけで書く版

`memcpy` / `memset` を使わずに `for` 文だけで書いた実装を2パターン紹介します。
「標準ライブラリ関数が使えない環境」や「1バイトずつの処理を追いたいとき」に役立ちます。

---

### 5-1. 左上原点版（copyTopLeftOnlyForLoop）

AをBの左上に置く。セクション4の `copyTopLeft` と同じ動作です。

```text
B全体

+------------------------+
| A A A A A              |
| A A A A A              |
| A A A A A              |
|                        |
|                        |
+------------------------+
```

```cpp
#include <algorithm>
#include <cstdint>

void copyTopLeftOnlyForLoop(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    // [1] B全体を0で初期化
    //
    // dstHeight 行 × dstLineSpan byte（padding含む）をすべて0に。
    for (int y = 0; y < dstHeight; ++y)
    {
        std::uint8_t* dstRow = dst + y * dstLineSpan;

        for (int x = 0; x < dstLineSpan; ++x)
        {
            dstRow[x] = 0;
        }
    }

    // [2] コピーできる範囲を決める
    int copyWidth  = std::min(srcWidth,  dstWidth);
    int copyHeight = std::min(srcHeight, dstHeight);

    // [3] 左上原点でコピー
    //
    // A(x, y) → B(x, y)  ← 行インデックス同じ
    for (int y = 0; y < copyHeight; ++y)
    {
        const std::uint8_t* srcRow = src + y * srcLineSpan;
        std::uint8_t*       dstRow = dst + y * dstLineSpan;

        for (int x = 0; x < copyWidth; ++x)
        {
            dstRow[x] = srcRow[x];
        }
    }
}
```

---

### 5-2. 左下原点版（copyBottomLeftOnlyForLoop）

AをBの左下に置く。「Aをどの行から置き始めるか」が左上版との違いです。

```text
B全体

+------------------------+
|                        |
|                        |
|                        |
| A A A A A              |
| A A A A A              |
| A A A A A              |
+------------------------+
```

#### dstStartY の計算

Aの行数を `copyHeight`、Bの行数を `dstHeight` とすると、
Aをちょうど底に揃えるには、こう計算します。

```text
dstStartY = dstHeight - copyHeight
```

具体例：

```text
dstHeight  = 500
copyHeight = 200

dstStartY = 500 - 200 = 300

src row 0   → dst row 300
src row 1   → dst row 301
src row 199 → dst row 499  ← Bの最終行
```

```cpp
#include <algorithm>
#include <cstdint>

void copyBottomLeftOnlyForLoop(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    // [1] B全体を0で初期化
    for (int y = 0; y < dstHeight; ++y)
    {
        std::uint8_t* dstRow = dst + y * dstLineSpan;

        for (int x = 0; x < dstLineSpan; ++x)
        {
            dstRow[x] = 0;
        }
    }

    // [2] コピーできる範囲を決める
    int copyWidth  = std::min(srcWidth,  dstWidth);
    int copyHeight = std::min(srcHeight, dstHeight);

    // [3] コピー先の開始行を下側に合わせる
    //
    // Aをちょうどbottomに揃えるために、
    // コピー先の0行目をずらす。
    int dstStartY = dstHeight - copyHeight;

    // [4] 左下配置でコピー
    //
    // src row y → dst row (dstStartY + y)
    for (int y = 0; y < copyHeight; ++y)
    {
        const std::uint8_t* srcRow = src + y * srcLineSpan;
        std::uint8_t*       dstRow = dst + (dstStartY + y) * dstLineSpan;

        for (int x = 0; x < copyWidth; ++x)
        {
            dstRow[x] = srcRow[x];
        }
    }
}
```

---

### 5-3. 重要：「左下に配置」と「行順反転」は別物

ここは間違えやすいポイントです。

上の `copyBottomLeftOnlyForLoop` は「Aをそのままの行順でBの下側に置く」だけです。
Aの一番上の行がBの中段付近に来て、Aの一番下がBの最終行になります。

一方、「行順を上下ひっくり返してコピーする」ときは、
`dstStartY` の計算ではなく、**コピー先の行インデックスを逆順にする** 式になります。

```cpp
int dstY = dstHeight - 1 - y;
```

| 処理             | dstY の式                     | 何が起きるか                         |
|----------------|------------------------------|-------------------------------------|
| 左上に配置        | `y`                          | AをBの左上にそのまま置く               |
| 左下に配置        | `dstStartY + y`              | AをBの左下に（行順はそのまま）置く       |
| 行順反転コピー    | `dstHeight - 1 - y`          | AをBに置きながら上下もひっくり返す       |

図で見ると違いが分かります。

```text
元のA（4行）:      左下に配置（5行B）:    行順反転（5行B）:

 row 0: aaaaa       row 0: 00000           row 0: 00000
 row 1: bbbbb       row 1: 00000           row 1: ddddd
 row 2: ccccc       row 2: aaaaa           row 2: ccccc
 row 3: ddddd       row 3: bbbbb           row 3: bbbbb
                    row 4: ccccc           row 4: aaaaa
                            ddddd
                    ↑Aの行順はそのまま      ↑Aの行順が逆になっている
```

どちらが必要かは、用途によります。

```text
確認すべき問い:
「左下原点」とは、AをBの左下に配置するという意味か？
それとも、行の向きそのものを上下反転するという意味か？
```

---

## 6. チェックリスト

この処理を実装・確認するときに見るべきポイントです。

```text
□ 1. Aの有効領域（width / height）は正しいか
□ 2. BのどこにAを置くか（今回は左上）は合っているか
□ 3. Bの余った領域は0埋めでよいか
□ 4. A/BそれぞれのLineSpanを正しく使っているか（widthと混同していないか）
□ 5. headerや件数など、value以外に更新すべき値があるか
```

特に 4. の LineSpan と width の混同は、よくあるバグの原因です。
「1行のバイト数」と「有効データのバイト数」を意識して区別しましょう。

---

## 付録: 座標系の見分け方

実際のコードやドキュメントで座標系を判断するヒントです。

| 手がかり                    | 座標系の可能性          |
|---------------------------|----------------------|
| `y=0` が画面の上           | 左上原点              |
| `y=0` が画面の下           | 左下原点              |
| Windows API, GDI, DirectX | 左上原点              |
| OpenGL テクスチャ座標       | 左下原点              |
| BMP ファイル（DIB）         | 左下原点が多い         |
| PNG / JPEG                | 左上原点              |
| 「row 0 は先頭行」          | 左上原点              |

「どちらの座標系か」が不明なときは、
`y=0` の行がどこに対応するかを実際に確認するのが一番

```

A(0,0) → B(0,0)
A(1,0) → B(1,0)
A(0,1) → B(0,1)

教材内の copyTopLeft と同じ考え方です。 

以下に 左上原点版 と 左下原点版 の両方を、memcpy / memset を使わずに for 文だけで書きます。

⸻

1. 左上原点版

AをBの 左上 に置く。

#include <algorithm>
#include <cstdint>
void copyTopLeftOnlyForLoop(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    // ------------------------------------------------------------
    // 1. コピー先Bを全体0で初期化する
    //
    // dstHeight 行ある。
    // 各行は dstLineSpan byte ある。
    // padding部分も含めて、全部0にする。
    // ------------------------------------------------------------
    for (int y = 0; y < dstHeight; ++y)
    {
        std::uint8_t* dstRow =
            dst + y * dstLineSpan;
        for (int x = 0; x < dstLineSpan; ++x)
        {
            dstRow[x] = 0;
        }
    }
    // ------------------------------------------------------------
    // 2. コピーできる範囲を決める
    //
    // srcがdstより大きい場合でも、はみ出さないようにする。
    // ------------------------------------------------------------
    int copyWidth =
        std::min(srcWidth, dstWidth);
    int copyHeight =
        std::min(srcHeight, dstHeight);
    // ------------------------------------------------------------
    // 3. 左上原点でコピーする
    //
    // srcの y 行目を、dstの y 行目へコピーする。
    //
    // A(0,0) → B(0,0)
    // A(1,0) → B(1,0)
    // A(0,1) → B(0,1)
    // ------------------------------------------------------------
    for (int y = 0; y < copyHeight; ++y)
    {
        const std::uint8_t* srcRow =
            src + y * srcLineSpan;
        std::uint8_t* dstRow =
            dst + y * dstLineSpan;
        for (int x = 0; x < copyWidth; ++x)
        {
            dstRow[x] = srcRow[x];
        }
    }
}

イメージ。

B全体
+------------------------+
| A A A                  |
| A A A                  |
| A A A                  |
|                        |
|                        |
+------------------------+

⸻

2. 左下原点版

AをBの 左下 に置く。

ポイントは、コピー先の開始Yを下側にずらすことです。

dstStartY = dstHeight - copyHeight

例えば、

srcHeight = 200
dstHeight = 500

なら、

dstStartY = 500 - 200 = 300

つまり、

Aの0行目 → Bの300行目
Aの1行目 → Bの301行目
...
Aの199行目 → Bの499行目

になります。

#include <algorithm>
#include <cstdint>
void copyBottomLeftOnlyForLoop(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    // ------------------------------------------------------------
    // 1. コピー先Bを全体0で初期化する
    //
    // 左下にAを置く場合でも、
    // 先にB全体を0にしておく。
    // ------------------------------------------------------------
    for (int y = 0; y < dstHeight; ++y)
    {
        std::uint8_t* dstRow =
            dst + y * dstLineSpan;
        for (int x = 0; x < dstLineSpan; ++x)
        {
            dstRow[x] = 0;
        }
    }
    // ------------------------------------------------------------
    // 2. コピーできる範囲を決める
    // ------------------------------------------------------------
    int copyWidth =
        std::min(srcWidth, dstWidth);
    int copyHeight =
        std::min(srcHeight, dstHeight);
    // ------------------------------------------------------------
    // 3. 左下原点で配置するため、
    //    dst側の開始行を下側にずらす。
    //
    // 例:
    // dstHeight = 500
    // copyHeight = 200
    //
    // dstStartY = 300
    //
    // src row 0   → dst row 300
    // src row 199 → dst row 499
    // ------------------------------------------------------------
    int dstStartY =
        dstHeight - copyHeight;
    // ------------------------------------------------------------
    // 4. 左下配置でコピーする
    //
    // srcの y 行目を、
    // dstの dstStartY + y 行目へコピーする。
    // ------------------------------------------------------------
    for (int y = 0; y < copyHeight; ++y)
    {
        const std::uint8_t* srcRow =
            src + y * srcLineSpan;
        std::uint8_t* dstRow =
            dst + (dstStartY + y) * dstLineSpan;
        for (int x = 0; x < copyWidth; ++x)
        {
            dstRow[x] = srcRow[x];
        }
    }
}

イメージ。

B全体
+------------------------+
|                        |
|                        |
|                        |
| A A A                  |
| A A A                  |
| A A A                  |
+------------------------+

⸻

重要な注意

上の 左下原点版 は、Aの行順を反転していません。

つまり、

A row 0 → Bの下側ブロックの先頭行
A row 1 → その次の行

です。

もし「左下原点」という意味が、行順も上下反転してコピーする という意味なら、別の式になります。

その場合はこうです。

int dstY =
    dstHeight - 1 - y;

ただし、これは「左下に配置」ではなく、上下反転コピー に近いです。

なので確認すべきことはこれです。

左下原点とは、
AをBの左下に配置するという意味か？
それとも、
行の向きそのものを上下反転するという意味か？



memcpy を使わずに、行ループ + 列ループ だけで書けます。

元の memcpy 版は、1行ぶんをまとめてコピーしていました。 

std::memcpy(dstRow, srcRow, static_cast<std::size_t>(copyWidth));

これを、1byteずつコピーする for 文に置き換えるとこうです。

#include <algorithm>
#include <cstdint>
#include <cstring>
void copyTopLeftWithoutMemcpy(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    // B側を全体0初期化する。
    // ここは memset を使っています。
    // もし memset も使いたくなければ、後ろに for 文版を書きます。
    std::memset(
        dst,
        0,
        static_cast<std::size_t>(dstHeight * dstLineSpan)
    );
    // コピーできる範囲を決める。
    // src が dst より大きい場合でも、はみ出さないようにする。
    int copyWidth = std::min(srcWidth, dstWidth);
    int copyHeight = std::min(srcHeight, dstHeight);
    // y方向、つまり行ごとに処理する。
    for (int y = 0; y < copyHeight; ++y)
    {
        // src の y 行目の先頭アドレス
        const std::uint8_t* srcRow =
            src + y * srcLineSpan;
        // dst の y 行目の先頭アドレス
        std::uint8_t* dstRow =
            dst + y * dstLineSpan;
        // x方向、つまり列ごとに1byteずつコピーする。
        for (int x = 0; x < copyWidth; ++x)
        {
            dstRow[x] = srcRow[x];
        }
    }
}

memcpy を使わない場合の本質はこれです。

for (int x = 0; x < copyWidth; ++x)
{
    dstRow[x] = srcRow[x];
}

つまり、

srcRow[0] → dstRow[0]
srcRow[1] → dstRow[1]
srcRow[2] → dstRow[2]
...

と1つずつコピーしています。

memset も使わず、完全に for 文だけで書くならこうです。

#include <algorithm>
#include <cstdint>
void copyTopLeftOnlyForLoop(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    // B側を全体0初期化する。
    // dstHeight 行、各行 dstLineSpan byte あるので、
    // 全体は dstHeight * dstLineSpan byte。
    for (int y = 0; y < dstHeight; ++y)
    {
        std::uint8_t* dstRow =
            dst + y * dstLineSpan;
        for (int x = 0; x < dstLineSpan; ++x)
        {
            dstRow[x] = 0;
        }
    }
    // コピーできる範囲を決める。
    int copyWidth = std::min(srcWidth, dstWidth);
    int copyHeight = std::min(srcHeight, dstHeight);
    // Aの有効領域をBの左上にコピーする。
    for (int y = 0; y < copyHeight; ++y)
    {
        const std::uint8_t* srcRow =
            src + y * srcLineSpan;
        std::uint8_t* dstRow =
            dst + y * dstLineSpan;
        for (int x = 0; x < copyWidth; ++x)
        {
            dstRow[x] = srcRow[x];
        }
    }
}

理解用なら for 文版の方が分かりやすいです。
本番寄りなら、1行コピーは memcpy の方が自然です。理由は、連続したメモリをまとめてコピーする用途に合っていて、速く、意図も明確だからです。


#include <algorithm>
#include <cstdint>

void copyVerticalFlipBottomLeftOnlyForLoop(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    // ------------------------------------------------------------
    // 1. コピー先Bを全体0で初期化する
    // ------------------------------------------------------------
    for (int y = 0; y < dstHeight; ++y)
    {
        std::uint8_t* dstRow =
            dst + y * dstLineSpan;

        for (int x = 0; x < dstLineSpan; ++x)
        {
            dstRow[x] = 0;
        }
    }

    // ------------------------------------------------------------
    // 2. コピーできる範囲を決める
    // ------------------------------------------------------------
    int copyWidth =
        std::min(srcWidth, dstWidth);

    int copyHeight =
        std::min(srcHeight, dstHeight);

    // ------------------------------------------------------------
    // 3. 上下反転してコピーする
    //
    // src row 0              → dst row dstHeight - 1
    // src row 1              → dst row dstHeight - 2
    // src row copyHeight - 1 → dst row dstHeight - copyHeight
    //
    // つまり、Aの上側の行がBの下側へ行く。
    // ------------------------------------------------------------
    for (int y = 0; y < copyHeight; ++y)
    {
        int dstY =
            dstHeight - 1 - y;

        const std::uint8_t* srcRow =
            src + y * srcLineSpan;

        std::uint8_t* dstRow =
            dst + dstY * dstLineSpan;

        for (int x = 0; x < copyWidth; ++x)
        {
            dstRow[x] = srcRow[x];
        }
    }
}



最終的には、やることはこれです。

uint8_t value[テーブル高さ][ラインスパン];

という 大きい入れ物 に対して、

1. 全体を0で埋める
2. コピーしたい範囲だけ、元データから値を入れる
3. LineSpanを使って「何行目か」を計算する

です。

今回の前提が、

拡大しない
12bit変換しない
補間しない

なら、処理はかなり単純化できます。

⸻

1. まず value[height][lineSpan] の意味

例えば変換先Bがこれだとします。

uint8_t value[500][600];

これは、

500行ある
1行あたり600byteある

という意味です。

ただし、有効な横幅が300なら、

1行のうち、
0〜299    : 有効データ
300〜599  : 余白・padding

です。

つまり、1行はこうです。

Bの1行:
|------ 有効データ 300byte ------|------ 余白 300byte ------|

教材でも、LineSpanは「1行が実際にメモリ上で占めるバイト数」と整理していました。 

⸻

2. 左上に置くならこう入れる

Aが、

幅 = 100
高さ = 200
LineSpan = 300

Bが、

幅 = 300
高さ = 500
LineSpan = 600

だとします。

左上に置くなら、

A(0,0)   → B(0,0)
A(1,0)   → B(1,0)
A(99,0)  → B(99,0)
A(0,199) → B(0,199)
A(99,199)→ B(99,199)

です。

つまり、

B[y][x] = A[y][x];

です。

ただし、実際のメモリ上では LineSpan を使います。

⸻

3. 一番シンプルなC++コード

uint8_t value[height][lineSpan] に対して、左上にコピーするならこうです。

#include <algorithm>
#include <cstdint>
void copyToTopLeft(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    // ------------------------------------------------------------
    // 1. コピー先Bを全体0で初期化する
    //
    // dstHeight行あり、各行はdstLineSpan byteある。
    // paddingや未使用領域も含めて全部0にする。
    // ------------------------------------------------------------
    for (int y = 0; y < dstHeight; ++y)
    {
        std::uint8_t* dstRow =
            dst + y * dstLineSpan;
        for (int x = 0; x < dstLineSpan; ++x)
        {
            dstRow[x] = 0;
        }
    }
    // ------------------------------------------------------------
    // 2. コピーする範囲を決める
    //
    // 基本はsrcWidth × srcHeightをコピーする。
    // ただし、dstよりsrcが大きい場合に備えてminを使う。
    // ------------------------------------------------------------
    int copyWidth =
        std::min(srcWidth, dstWidth);
    int copyHeight =
        std::min(srcHeight, dstHeight);
    // ------------------------------------------------------------
    // 3. Aの有効領域をBの左上にコピーする
    //
    // y行目の先頭位置は、
    // 先頭アドレス + y * LineSpan
    // で求める。
    // ------------------------------------------------------------
    for (int y = 0; y < copyHeight; ++y)
    {
        const std::uint8_t* srcRow =
            src + y * srcLineSpan;
        std::uint8_t* dstRow =
            dst + y * dstLineSpan;
        for (int x = 0; x < copyWidth; ++x)
        {
            dstRow[x] = srcRow[x];
        }
    }
}

⸻

4. 呼び出しイメージ

構造体がこうだとします。

struct TableA
{
    static constexpr int Width = 100;
    static constexpr int Height = 200;
    static constexpr int LineSpan = 300;
    std::uint8_t value[Height][LineSpan];
};
struct TableB
{
    static constexpr int Width = 300;
    static constexpr int Height = 500;
    static constexpr int LineSpan = 600;
    std::uint8_t value[Height][LineSpan];
};

呼び出しはこうです。

void convertAtoB(
    const TableA& src,
    TableB& dst
)
{
    copyToTopLeft(
        &src.value[0][0],
        TableA::Width,
        TableA::Height,
        TableA::LineSpan,
        &dst.value[0][0],
        TableB::Width,
        TableB::Height,
        TableB::LineSpan
    );
}

⸻

5. 何をしているかを図で見る

A:

A: width=100, height=200, lineSpan=300
row 0: [A有効100][padding200]
row 1: [A有効100][padding200]
...
row199:[A有効100][padding200]

B:

B: width=300, height=500, lineSpan=600
row 0: [B有効300][padding300]
row 1: [B有効300][padding300]
...
row499:[B有効300][padding300]

処理後のB:

row 0〜199:
[Aからコピーした100byte][0埋め200byte][padding 0埋め300byte]
row 200〜499:
[0埋め300byte][padding 0埋め300byte]

つまり、Bの中身はこうなります。

B全体
+----------------------------------+
| A A A A A | 0 0 0 0 0            |
| A A A A A | 0 0 0 0 0            |
| A A A A A | 0 0 0 0 0            |
|----------------------------------|
| 0 0 0 0 0 0 0 0 0 0              |
| 0 0 0 0 0 0 0 0 0 0              |
+----------------------------------+

⸻

6. 左下に置くなら変わるのは dstRow だけ

左下に置くなら、AをBの下側に配置します。

違いはここだけです。

int dstY = dstHeight - copyHeight + y;

左下配置版はこうです。

#include <algorithm>
#include <cstdint>
void copyToBottomLeft(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    // B全体を0で初期化する
    for (int y = 0; y < dstHeight; ++y)
    {
        std::uint8_t* dstRow =
            dst + y * dstLineSpan;
        for (int x = 0; x < dstLineSpan; ++x)
        {
            dstRow[x] = 0;
        }
    }
    int copyWidth =
        std::min(srcWidth, dstWidth);
    int copyHeight =
        std::min(srcHeight, dstHeight);
    // Bの下側にAを置くため、開始行を下へずらす
    int dstStartY =
        dstHeight - copyHeight;
    for (int y = 0; y < copyHeight; ++y)
    {
        const std::uint8_t* srcRow =
            src + y * srcLineSpan;
        std::uint8_t* dstRow =
            dst + (dstStartY + y) * dstLineSpan;
        for (int x = 0; x < copyWidth; ++x)
        {
            dstRow[x] = srcRow[x];
        }
    }
}

⸻

7. 今回の最終理解

最終的に uint8_t value[テーブル高さ][ラインスパン] に値を入れるとは、こういうことです。

1. value全体を0で初期化する
2. コピー元Aの各行について、
   srcRow = Aの先頭 + y * AのLineSpan
   でAのy行目を見つける
3. コピー先Bの各行について、
   dstRow = Bの先頭 + y * BのLineSpan
   でBのy行目を見つける
4. x=0からcopyWidth-1まで、
   dstRow[x] = srcRow[x]
   で1byteずつ入れる
5. Bの余った領域は0のままにする

一番大事なのはこれです。

dstRow[x] = srcRow[x];

ただし、その前に、

srcRow = src + y * srcLineSpan;
dstRow = dst + y * dstLineSpan;

で、正しい行を選ぶ必要があります。

つまり、最終的には、

LineSpanで行を選ぶ
xで列を選ぶ
値を入れる

です。




欠陥画素テーブルは、シェーディング補正テーブルと考え方が違います。

シェーディング補正は、

2次元の面データを、別の入れ物に配置する

でした。

でも欠陥画素テーブルは、おそらく、

欠陥がある画素の座標リストを、別の形式の座標リストに入れ直す

です。

つまり、dstRow を作って x 方向にコピーするような 画像バッファの行コピー ではありません。

⸻

欠陥画素テーブルのイメージ

たとえば A 側がこういうデータだとします。

struct DefectPixelA
{
    std::uint16_t x;
    std::uint16_t y;
};
struct DefectTableA
{
    DefectPixelA pixels[200];
    std::uint16_t num;
};

B 側がこう。

struct DefectPixelB
{
    std::uint16_t x;
    std::uint16_t y;
};
struct DefectTableB
{
    DefectPixelB pixels[500];
    std::uint16_t num;
};

これは画像のように、

value[height][lineSpan]

で行ごとに並んでいるわけではありません。

むしろ、

0番目の欠陥画素
1番目の欠陥画素
2番目の欠陥画素
...

という リスト です。

⸻

だからやることはこれ

欠陥画素テーブルでは、基本はこうです。

1. B側を初期化する
2. A側の有効件数 num を見る
3. Aの各欠陥画素を1件ずつ読む
4. 必要なら x, y を変換する
5. B側の同じindexに入れる
6. B側の num を設定する
7. B側の余り領域は0のままにする

つまり、行コピーではなく、

dst.pixels[i].x = 変換後のx;
dst.pixels[i].y = 変換後のy;

です。

⸻

最小コード

まずは「座標変換なしで、そのままコピーする」版です。

#include <algorithm>
#include <cstdint>
struct DefectPixelA
{
    std::uint16_t x;
    std::uint16_t y;
};
struct DefectTableA
{
    static constexpr int MaxCount = 200;
    DefectPixelA pixels[MaxCount];
    std::uint16_t num;
};
struct DefectPixelB
{
    std::uint16_t x;
    std::uint16_t y;
};
struct DefectTableB
{
    static constexpr int MaxCount = 500;
    DefectPixelB pixels[MaxCount];
    std::uint16_t num;
};
void convertDefectTableSimple(
    const DefectTableA& src,
    DefectTableB& dst
)
{
    // ------------------------------------------------------------
    // 1. B側を初期化する
    // ------------------------------------------------------------
    dst.num = 0;
    for (int i = 0; i < DefectTableB::MaxCount; ++i)
    {
        dst.pixels[i].x = 0;
        dst.pixels[i].y = 0;
    }
    // ------------------------------------------------------------
    // 2. コピーする件数を決める
    //
    // A側のnumが200を超えていたら危険なので、Aの最大件数でも制限する。
    // B側は500件まで入る。
    // ------------------------------------------------------------
    int copyCount = std::min<int>(
        src.num,
        DefectTableA::MaxCount
    );
    copyCount = std::min<int>(
        copyCount,
        DefectTableB::MaxCount
    );
    // ------------------------------------------------------------
    // 3. 欠陥画素を1件ずつコピーする
    // ------------------------------------------------------------
    for (int i = 0; i < copyCount; ++i)
    {
        dst.pixels[i].x = src.pixels[i].x;
        dst.pixels[i].y = src.pixels[i].y;
    }
    // ------------------------------------------------------------
    // 4. B側の有効件数を設定する
    // ------------------------------------------------------------
    dst.num = static_cast<std::uint16_t>(copyCount);
}

⸻

座標変換が必要な場合

A側が、

x: 11bit
y: 11bit

B側が、

x: 12bit
y: 11bit

で、かつ「Aのx座標範囲をBのx座標範囲へ広げる」必要があるなら、xだけ変換します。

たとえば、

Aのx範囲: 0〜2047
Bのx範囲: 0〜4095

なら、

newX = oldX * 4095 / 2047;

です。

std::uint16_t convertX11bitTo12bit(
    std::uint16_t oldX
)
{
    // 11bit最大値 = 2047
    // 12bit最大値 = 4095
    std::uint32_t newX =
        static_cast<std::uint32_t>(oldX)
        * 4095u
        / 2047u;
    return static_cast<std::uint16_t>(newX);
}

これを使うとこうです。

void convertDefectTableWithXScale(
    const DefectTableA& src,
    DefectTableB& dst
)
{
    // B側を初期化する
    dst.num = 0;
    for (int i = 0; i < DefectTableB::MaxCount; ++i)
    {
        dst.pixels[i].x = 0;
        dst.pixels[i].y = 0;
    }
    int copyCount = std::min<int>(
        src.num,
        DefectTableA::MaxCount
    );
    copyCount = std::min<int>(
        copyCount,
        DefectTableB::MaxCount
    );
    for (int i = 0; i < copyCount; ++i)
    {
        // xは11bit範囲から12bit範囲へ変換
        dst.pixels[i].x =
            convertX11bitTo12bit(src.pixels[i].x);
        // yは11bitのままなら、そのまま
        dst.pixels[i].y =
            src.pixels[i].y;
    }
    dst.num = static_cast<std::uint16_t>(copyCount);
}

⸻

ここで確認が必要なこと

ただし、xが11bitから12bitだからといって、必ず2倍相当にするとは限りません。

確認すべきはこれです。

Aのx=11bitは 0〜2047 の座標範囲として扱うのか？
Bのx=12bitは 0〜4095 の座標範囲として扱うのか？
xはスケール変換するのか？
それとも上位bitが増えただけで、値自体はそのまま入れるのか？
yは本当にそのままでよいのか？
numはAの有効件数をそのままBへ引き継ぐのか？
Bの余り領域は0埋めでよいのか？

ここが未確認なら、まずは 単純コピー版 と xスケール版 のどちらが仕様に合うか確認する必要があります。

⸻

シェーディング補正との違い

シェーディング補正は、こうでした。

行を選ぶ
列を選ぶ
value[y][x] に入れる

LineSpanを考える必要があるので、教材内でも src + y * srcLineSpan のように行アドレスを計算していました。 

でも欠陥画素テーブルは、こうです。

i番目の欠陥画素を読む
i番目の欠陥画素として入れる

だから基本は、

for (int i = 0; i < copyCount; ++i)
{
    dst.pixels[i].x = ...;
    dst.pixels[i].y = ...;
}

です。

⸻

一番大事な理解

欠陥画素テーブルは、

2D画像バッファではない

です。

これは、

欠陥画素の座標リスト

です。

だから、最終的には、

value[y][x] に入れる

ではなく、

pixels[i].x にx座標を入れる
pixels[i].y にy座標を入れる
num に有効件数を入れる

です。

今回の欠陥画素は、たぶんシェーディング補正より簡単です。補間やLineSpanではなく、件数・座標変換・初期化 が中心です。







#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

/*
    シェーディングテーブルをそのままbinary出力する関数

    value:
        uint8_t value[height][lineSpan] の先頭アドレス

    height:
        テーブルの高さ

    lineSpan:
        1行あたりのbyte数
        有効幅ではなく、padding込みの1行サイズ

    outputPath:
        出力先binファイル名
*/
int writeShadingTableBin(
    const uint8_t* value,
    size_t height,
    size_t lineSpan,
    const char* outputPath
)
{
    if (value == NULL)
    {
        return -1;
    }

    if (outputPath == NULL)
    {
        return -2;
    }

    FILE* fp = fopen(outputPath, "wb");

    if (fp == NULL)
    {
        return -3;
    }

    /*
        書き出す総byte数。

        value[height][lineSpan] なので、
        height × lineSpan byte をそのまま出力する。
    */
    size_t writeSize = height * lineSpan;

    size_t written = fwrite(
        value,       /* 書き出すデータの先頭 */
        1,           /* 1要素 = 1byte */
        writeSize,   /* 書き出すbyte数 */
        fp
    );

    fclose(fp);

    if (written != writeSize)
    {
        return -4;
    }

    return 0;
}



#include <algorithm>
#include <cstdint>
#include <cstring>   // std::memcpy, std::memset

void copyBottomLeftWithMemcpy(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    if (src == nullptr || dst == nullptr)
    {
        return;
    }

    if (srcWidth <= 0 || srcHeight <= 0 ||
        dstWidth <= 0 || dstHeight <= 0 ||
        srcLineSpan <= 0 || dstLineSpan <= 0)
    {
        return;
    }

    // srcWidth / dstWidth は「byte数」として扱う前提。
    // 12bit packed の場合、画素数ではなく byte幅を渡す必要がある。
    if (srcWidth > srcLineSpan || dstWidth > dstLineSpan)
    {
        return;
    }

    // ------------------------------------------------------------
    // [1] dst全体を0で初期化する
    //
    // dstは uint8_t の連続バッファなので、
    // height × lineSpan byte をまとめて0埋めできる。
    // ------------------------------------------------------------
    std::memset(
        dst,
        0,
        static_cast<std::size_t>(dstHeight) *
        static_cast<std::size_t>(dstLineSpan)
    );

    // ------------------------------------------------------------
    // [2] コピーできる範囲を決める
    //
    // ここでの copyWidth は byte数。
    // 12bit packed の場合、画素数ではない。
    // ------------------------------------------------------------
    const int copyWidth =
        std::min(srcWidth, dstWidth);

    const int copyHeight =
        std::min(srcHeight, dstHeight);

    // ------------------------------------------------------------
    // [3] 左下配置の開始Yを決める
    //
    // row 0 が上、row が増えるほど下、という前提。
    // dstHeight - copyHeight にすることで、下端が揃う。
    // ------------------------------------------------------------
    const int dstStartY =
        dstHeight - copyHeight;

    // ------------------------------------------------------------
    // [4] 1行ずつ memcpy でコピーする
    //
    // src row y → dst row dstStartY + y
    //
    // 1行の中身は連続しているので、byte単位でまとめてコピーできる。
    // ------------------------------------------------------------
    for (int y = 0; y < copyHeight; ++y)
    {
        const std::uint8_t* srcRow =
            src + static_cast<std::size_t>(y) * srcLineSpan;

        std::uint8_t* dstRow =
            dst + static_cast<std::size_t>(dstStartY + y) * dstLineSpan;

        std::memcpy(
            dstRow,
            srcRow,
            static_cast<std::size_t>(copyWidth)
        );
    }
}


#include <algorithm>
#include <cstdint>
#include <cstring>

void copyBottomLeftWithMemcpy(
    const std::uint8_t* src,
    int srcWidth,
    int srcHeight,
    int srcLineSpan,
    std::uint8_t* dst,
    int dstWidth,
    int dstHeight,
    int dstLineSpan
)
{
    if (src == nullptr || dst == nullptr)
    {
        return;
    }

    const int copyWidth =
        std::min(srcWidth, dstWidth);

    const int copyHeight =
        std::min(srcHeight, dstHeight);

    const int dstStartY =
        dstHeight - copyHeight;

    // dst全体を0で初期化する
    std::memset(
        dst,
        0,
        static_cast<std::size_t>(dstHeight) *
        static_cast<std::size_t>(dstLineSpan)
    );

    // srcは0行目から読む
    const std::uint8_t* srcRow = src;

    // dstは左下配置なので、dstStartY行目から書く
    std::uint8_t* dstRow =
        dst + static_cast<std::size_t>(dstStartY) *
              static_cast<std::size_t>(dstLineSpan);

    for (int y = 0; y < copyHeight; ++y)
    {
        // 1行ぶんをまとめてコピーする
        std::memcpy(
            dstRow,
            srcRow,
            static_cast<std::size_t>(copyWidth)
        );

        // 次の行の先頭へ進める
        srcRow += srcLineSpan;
        dstRow += dstLineSpan;
    }
}

```