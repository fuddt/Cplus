
```cpp
// 小さい方
uint8_t value[4][1024];

// 大きい方
uint8_t value[16][1024];
```

```text
小さい方:
ゲイン0用のLUTが1024個
ゲイン1用のLUTが1024個
ゲイン2用のLUTが1024個
ゲイン3用のLUTが1024個

大きい方:
ゲイン0用のLUTが1024個
...
ゲイン15用のLUTが1024個
```

つまり問題は、

```text
LUTの横幅1024は同じ。
でも、ゲイン段数が 4 → 16 に増えている。
```

ということ。

---

# memcpyで何をコピーするのか

MatrixLUTの場合、`memcpy` でコピーしやすい単位は **LUT 1本分** です。

1本分はこれ。

```cpp
uint8_t value[1024];
```

つまり、

```text
1024 byte の連続データ
```

なので、これはそのまま `memcpy` できます。

```cpp
std::memcpy(dst[dstGain], src[srcGain], 1024);
```

意味は、

```text
srcのあるゲイン段のLUT 1024個を、
dstのあるゲイン段へ丸ごとコピーする
```

です。

---

# 一番自然な移植方針

小さい方は4段、大きい方は16段。

なので、まず一番素直なのは、

```text
小さい方の1段を、大きい方の4段に複製する
```

です。

対応表はこう。

```text
dst[0]  = src[0]
dst[1]  = src[0]
dst[2]  = src[0]
dst[3]  = src[0]

dst[4]  = src[1]
dst[5]  = src[1]
dst[6]  = src[1]
dst[7]  = src[1]

dst[8]  = src[2]
dst[9]  = src[2]
dst[10] = src[2]
dst[11] = src[2]

dst[12] = src[3]
dst[13] = src[3]
dst[14] = src[3]
dst[15] = src[3]
```

式で書くと、

```cpp
srcGain = dstGain / 4;
```

です。

---

# コード例

```cpp
#include <cstdint>
#include <cstring>

/*
    MatrixLUTを 4ゲイン段 → 16ゲイン段へ移植する。

    小さい方:
        src[4][1024]

    大きい方:
        dst[16][1024]

    方針:
        src[0] を dst[0]〜dst[3] へコピー
        src[1] を dst[4]〜dst[7] へコピー
        src[2] を dst[8]〜dst[11] へコピー
        src[3] を dst[12]〜dst[15] へコピー

    ポイント:
        LUT 1本は uint8_t 1024個なので、
        1本ずつ memcpy で丸ごとコピーできる。
*/
void copyMatrixLut4To16(
    const std::uint8_t src[4][1024],
    std::uint8_t dst[16][1024]
)
{
    for (int dstGain = 0; dstGain < 16; ++dstGain)
    {
        // dstGain 0〜3   -> srcGain 0
        // dstGain 4〜7   -> srcGain 1
        // dstGain 8〜11  -> srcGain 2
        // dstGain 12〜15 -> srcGain 3
        const int srcGain = dstGain / 4;

        // LUT 1本分、1024 byteをコピーする
        std::memcpy(
            dst[dstGain],
            src[srcGain],
            1024 * sizeof(std::uint8_t)
        );
    }
}
```

`sizeof(std::uint8_t)` は1なので、実質これは1024 byteコピーです。

---

# なぜmemsetは必須ではないか

このコードでは、`dst[0]` から `dst[15]` まで全部 `memcpy` で埋めています。

だから、基本的には先に `memset` しなくてもよいです。

```cpp
for (int dstGain = 0; dstGain < 16; ++dstGain)
{
    std::memcpy(...);
}
```

で16段すべて埋まるからです。

ただし、現場で「念のため初期化しろ」と言われるなら、こう書いてもいい。

```cpp
std::memset(dst, 0, 16 * 1024 * sizeof(std::uint8_t));
```

でもMatrixLUTで `0` 初期化は少し注意。
もしコピー漏れがあった場合、残った `0` は「補正なし」ではなく、「全部0に変換するLUT」になる可能性があります。

なので、**16段すべてを確実にmemcpyで埋める**のが大事。

---

# ポインタで受け取る場合

現場コードでは、関数引数がこうなっているかもしれません。

```cpp
const uint8_t* src
uint8_t* dst
```

その場合は、2次元配列ではなく、1次元の連続メモリとして扱います。

```cpp
#include <cstdint>
#include <cstring>

/*
    MatrixLUTをポインタで受け取る版。

    src:
        小さいMatrixLUTの先頭
        実体は value[4][1024] 相当

    dst:
        大きいMatrixLUTの先頭
        実体は value[16][1024] 相当
*/
void copyMatrixLut4To16Pointer(
    const std::uint8_t* src,
    std::uint8_t* dst
)
{
    if (src == nullptr || dst == nullptr)
    {
        return;
    }

    constexpr int SRC_GAIN_LEVELS = 4;
    constexpr int DST_GAIN_LEVELS = 16;
    constexpr int LUT_SIZE = 1024;

    for (int dstGain = 0; dstGain < DST_GAIN_LEVELS; ++dstGain)
    {
        const int srcGain = dstGain / 4;

        // srcのsrcGain段目の先頭アドレス
        const std::uint8_t* srcLut =
            src + srcGain * LUT_SIZE;

        // dstのdstGain段目の先頭アドレス
        std::uint8_t* dstLut =
            dst + dstGain * LUT_SIZE;

        // LUT 1本分をコピー
        std::memcpy(
            dstLut,
            srcLut,
            LUT_SIZE * sizeof(std::uint8_t)
        );
    }
}
```

このポインタ計算はこういう意味。

```text
src + srcGain * 1024
```

これは、

```text
srcのsrcGain段目のLUT先頭へ移動する
```

ということ。

---

# 逆にやってはいけない可能性が高いもの

## 先頭4段だけコピー

```cpp
std::memcpy(dst, src, 4 * 1024);
```

これは一見よさそうだけど、大きい方の `dst[4]〜dst[15]` が埋まりません。

つまり、

```text
16段あるうち、後ろ12段が未設定
```

になります。

大きいツール側がゲイン4〜15を使うなら危険です。

---

## 16段全部にsrc[0]だけコピー

```text
dst[0]〜dst[15] = src[0]
```

これもたぶん雑すぎる。
src[1]〜src[3] の情報を捨ててしまうから。

---

# まとめ

MatrixLUTで考えることはシンプル。

```text
LUT_SIZE 1024 は同じ
→ LUT 1本はそのまま memcpy できる

違うのはゲイン段数
→ 4段を16段へどう割り当てるか考える
```

仮実装として一番自然なのはこれ。

```cpp
for (int dstGain = 0; dstGain < 16; ++dstGain)
{
    int srcGain = dstGain / 4;
    memcpy(dst[dstGain], src[srcGain], 1024);
}
```

つまり、

```text
小さい1段を、大きい4段に複製する
```

という移植。

---


うん、それなら話が変わる。
**リーダーに仕様確認して終わり**ではなく、こちらで **既存コードから仕様を逆算する** 仕事になる。

MatrixLUTの場合、調査の本丸はこれです。

```text
value[16][1024] の「16」が、実際の処理でどう選ばれているか
```

つまり、

```cpp
value[gainLevel][inputValue]
```

みたいに使われているはずなので、
この `gainLevel` がどこから来て、どんな範囲を取るのかを追えばいい。

---

# まず結論

今すぐ勝手に、

```cpp
dst[0..3]   = src[0]
dst[4..7]   = src[1]
dst[8..11]  = src[2]
dst[12..15] = src[3]
```

と決め打ちするのは、まだ早い。

やるべき調査はこれ。

```text
1. MatrixLUT が実際に参照されている箇所を探す
2. value[?][?] の1次元目が何で決まるか調べる
3. 小さいツールでは 0〜3 のどれが使われているか調べる
4. 大きいツールでは 0〜15 のどれが使われるか調べる
5. 4段 → 16段の対応関係を推定する
```

---

# 見るべきコード

まず検索するキーワードはこれ。

```text
MatrixLUT
value[
LUTSIZE
1024
gain
Gain
gainLevel
gain_level
level
Matrix
```

特に見たいのはこういうコード。

```cpp
out = matrixLut.value[gainLevel][input];
```

または、

```cpp
pLut = matrixLut.value[gainLevel];
```

または、

```cpp
value[level][index]
```

みたいな箇所。

---

# 一番重要なのは `value[?][?]` の `?`

MatrixLUTがこうなら、

```cpp
uint8_t value[16][1024];
```

使う側はたぶんこうなっている。

```cpp
uint8_t corrected = matrixLut.value[gainLevel][inputValue];
```

このとき重要なのは、

```cpp
gainLevel
```

がどう計算されているか。

例えばこういう可能性がある。

---

## パターンA：ゲイン値から段階を直接選んでいる

```cpp
int gainLevel = currentGain;
uint8_t corrected = lut.value[gainLevel][inputValue];
```

この場合、大きい側では本当に `0〜15` が使われる可能性が高い。

---

## パターンB：ゲイン値を4段に丸めている

```cpp
int gainLevel = currentGain / 4;
uint8_t corrected = lut.value[gainLevel][inputValue];
```

この場合、小さい側の `value[4][1024]` に対応している。

---

## パターンC：16段あるが、実際には一部しか使っていない

```cpp
int gainLevel = getGainLevel();

if (gainLevel > 3)
{
    gainLevel = 3;
}
```

この場合、大きい側が16段あっても、実際には0〜3しか使っていない可能性がある。

---

# 調査の考え方

MatrixLUTの移植方針は、**構造体定義からは決まりません**。

```cpp
uint8_t value[4][1024];
uint8_t value[16][1024];
```

これだけ見ても、

```text
4段を16段へ複製するのか
先頭4段だけ使うのか
補間するのか
16段のうち特定位置に置くのか
```

は決められない。

答えは、**使っている側のコード**にあります。

---

# 調査で見るべきポイント

## 1. MatrixLUTの読み込み処理

まず、ファイルからMatrixLUTを読み込んでいるところを見る。

探すもの。

```cpp
fread
memcpy
MatrixLUT
sizeof(MatrixLUT)
value
```

例えばこういうコード。

```cpp
fread(&matrixLut, sizeof(MatrixLUT), 1, fp);
```

または、

```cpp
memcpy(&matrixLut.value, buffer + offset, sizeof(matrixLut.value));
```

ここでは、

```text
ファイル上のMatrixLUTが何byteある想定か
```

が分かる。

小さい側なら、

```text
4 × 1024 × 1 = 4096 byte
```

大きい側なら、

```text
16 × 1024 × 1 = 16384 byte
```

---

## 2. MatrixLUTの適用処理

こっちが本命。

探すコードはこういうもの。

```cpp
value[gainLevel][inputValue]
```

または、

```cpp
lut[gain][pixel]
```

または、

```cpp
matrix[index]
```

実際には名前が違うかもしれない。

見たいのは、

```text
1次元目のindexがどこから来るか
2次元目のindexがどこから来るか
```

です。

---

## 3. gainLevelの範囲

コード中にこういう制限があるか見る。

```cpp
if (gainLevel >= 16)
{
    gainLevel = 15;
}
```

または、

```cpp
gainLevel &= 0x0F;
```

または、

```cpp
gainLevel = gainLevel >> 2;
```

ここが重要。

特に `>> 2` が出てきたら、

```text
16段を4段に圧縮している
```

可能性がある。

逆に `/ 4` や `>> 2` ではなく、そのまま0〜15を使っているなら、16段は意味がある。

---

# C++で調査ログを入れるなら

適用処理の直前にログを入れられるなら、これが一番強いです。

```cpp
#include <cstdio>
#include <cstdint>

/*
    MatrixLUTがどのゲイン段を使っているか確認するためのログ。
    何フレームか流して、gainLevelが0〜15のどこを取るか見る。
*/
void debugPrintMatrixLutAccess(
    int gainLevel,
    int inputValue
)
{
    static int count = 0;

    // ログが大量に出すぎないように、最初の100回だけ出す
    if (count < 100)
    {
        std::printf(
            "MatrixLUT access: gainLevel=%d, inputValue=%d\n",
            gainLevel,
            inputValue
        );
        ++count;
    }
}
```

使う側でこう呼ぶ。

```cpp
debugPrintMatrixLutAccess(gainLevel, inputValue);

uint8_t corrected =
    matrixLut.value[gainLevel][inputValue];
```

これで、

```text
実際に gainLevel が 0〜3 しか出ないのか
0〜15 まで出るのか
特定の値だけ出るのか
```

が分かる。

---

# さらに強い調査：16段を全部違う値にして反応を見る

これは実験としてかなり有効。

大きい側の `value[16][1024]` に、ゲイン段ごとに明確に違うLUTを入れる。

例えば、

```cpp
dst[0] は全部 10
dst[1] は全部 20
dst[2] は全部 30
...
dst[15] は全部 160
```

すると、出力画像を見ることで、

```text
今どのgainLevelのLUTが使われているか
```

が分かる。

サンプルコード。

```cpp
#include <cstdint>

/*
    MatrixLUTのどのゲイン段が使われているか確認するため、
    各ゲイン段に明確に違う値を入れる。

    注意:
        これは調査用。
        正しい補正値ではない。
        本番コードには残さない。
*/
void fillMatrixLutForDebug(
    std::uint8_t value[16][1024]
)
{
    for (int gain = 0; gain < 16; ++gain)
    {
        // gainごとに違う値にする
        // 例: gain0=10, gain1=20, ..., gain15=160
        const std::uint8_t debugValue =
            static_cast<std::uint8_t>((gain + 1) * 10);

        for (int i = 0; i < 1024; ++i)
        {
            value[gain][i] = debugValue;
        }
    }
}
```

もし画像出力が全体的に `40` 相当になったら、

```text
dst[3] が使われている
```

と推定できる。

ただし、後段処理でスケーリングされるかもしれないので、出力値そのものより、**差が出るか**を見る。

---

# もっと安全なデバッグLUT

全部同じ値にすると画像が壊れすぎる場合は、LUTの一部だけ変える。

例えば、通常はほぼ恒等変換にして、ゲイン段ごとに少しだけ差をつける。

```cpp
#include <cstdint>
#include <algorithm>

/*
    入力値に応じたLUT形状は保ちつつ、
    gainごとに少しだけ差をつけるデバッグLUT。
*/
void fillMatrixLutForDebugRamp(
    std::uint8_t value[16][1024]
)
{
    for (int gain = 0; gain < 16; ++gain)
    {
        for (int i = 0; i < 1024; ++i)
        {
            // 10bit入力 0〜1023 を 8bit 0〜255 にする仮の変換
            int base = i >> 2;

            // gainごとに少し差をつける
            int debug = base + gain;

            if (debug > 255)
            {
                debug = 255;
            }

            value[gain][i] =
                static_cast<std::uint8_t>(debug);
        }
    }
}
```

これなら画像が極端に壊れにくい。

---

# バイナリから調べる方法

もし小さいMatrixLUTと大きいMatrixLUTのキャリブレーションバイナリがあるなら、Pythonで比較できます。

## 小さい側の4段LUTを読む

```python
import numpy as np

small = np.fromfile("small_matrix_lut.bin", dtype=np.uint8)
small = small.reshape(4, 1024)

print(small.shape)
print(small[0, :10])
print(small[1, :10])
print(small[2, :10])
print(small[3, :10])
```

## 大きい側の16段LUTを読む

```python
import numpy as np

large = np.fromfile("large_matrix_lut.bin", dtype=np.uint8)
large = large.reshape(16, 1024)

print(large.shape)

for g in range(16):
    print(g, large[g, :10])
```

---

# 4段→16段の既存関係を比較する

もし、正しい大きい側の既存ファイルがあるなら、これが最強。

```python
import numpy as np

small = np.fromfile("small_matrix_lut.bin", dtype=np.uint8).reshape(4, 1024)
large = np.fromfile("large_matrix_lut.bin", dtype=np.uint8).reshape(16, 1024)

for dst_gain in range(16):
    diffs = []

    for src_gain in range(4):
        diff = np.mean(np.abs(
            large[dst_gain].astype(np.int16) -
            small[src_gain].astype(np.int16)
        ))
        diffs.append(diff)

    best_src = int(np.argmin(diffs))

    print(
        "dst_gain",
        dst_gain,
        "best_src",
        best_src,
        "diffs",
        diffs
    )
```

これで、例えば結果がこうなら、

```text
dst 0 best src 0
dst 1 best src 0
dst 2 best src 0
dst 3 best src 0
dst 4 best src 1
...
```

かなり高確率で、

```text
4段を16段へ4回ずつ複製
```

だと分かる。

逆に、

```text
dst 0 best src 0
dst 1 best src 0
dst 2 best src 1
dst 3 best src 1
...
```

みたいになれば、別の対応関係がある。

---

# 調査の順番

実務的には、この順番がいい。

```text
1. 構造体定義を見る
   value[4][1024] / value[16][1024] を確認

2. 読み込み箇所を見る
   何byte読み込んでいるか確認

3. 適用箇所を見る
   value[gainLevel][input] の gainLevel の由来を追う

4. gainLevelのログを出す
   実際に0〜15のどこが使われるか確認

5. デバッグLUTを入れる
   どの段が出力に効くか確認

6. 既存の小/大MatrixLUTがあるならPythonで類似度比較

7. その結果から移植方針を決める
```

---

# 移植方針を決める条件

## もし実行時にgainLevelが0〜3しか出ないなら

この場合は、

```text
先頭4段だけコピーでも動く可能性あり
```

ただし、大きい構造体の残り12段は未使用と判断できる。

---

## もしgainLevelが0〜15まで出るなら

この場合は、先頭4段コピーだけではダメ。

候補は、

```text
4段を16段へ複製展開
または
4段から16段へ補間
```

になる。

---

## もし既存の大きい側LUTが4段複製に近いなら

この場合は、

```text
src[0] -> dst[0..3]
src[1] -> dst[4..7]
src[2] -> dst[8..11]
src[3] -> dst[12..15]
```

でよい可能性が高い。

---

## もし既存の大きい側LUTがなめらかに変化しているなら

この場合は、補間の可能性がある。

例えば、`dst[0]` から `dst[15]` までだんだん値が変わるなら、単純複製ではなく、gain方向補間かもしれない。

---

# 最終的な仮実装はこれ

調査しても完全には分からず、でも実装しないといけないなら、私はこれを仮実装にします。

```cpp
#include <cstdint>
#include <cstring>

/*
    調査結果として、
    4段LUTを16段へ粗く展開する必要があると判断した場合の実装。

    dst[0..3]   = src[0]
    dst[4..7]   = src[1]
    dst[8..11]  = src[2]
    dst[12..15] = src[3]
*/
void copyMatrixLut4To16ByReplication(
    const std::uint8_t src[4][1024],
    std::uint8_t dst[16][1024]
)
{
    for (int dstGain = 0; dstGain < 16; ++dstGain)
    {
        const int srcGain = dstGain / 4;

        std::memcpy(
            dst[dstGain],
            src[srcGain],
            1024 * sizeof(std::uint8_t)
        );
    }
}
```

でも、コードにはコメントでこう残す。

```cpp
/*
    4段 -> 16段の展開仕様が明文化されていないため、
    既存実装のgainLevel参照範囲とLUT比較結果に基づき、
    1段を4段へ複製する方針としている。
*/
```

これが大事。
「なんとなく」ではなく、**調査結果に基づく仮説**として残す。

---

# まとめ

今回やるべきことは、いきなりコードを書くことではなく、

```text
大きいMatrixLUTの16段がどう使われているかを調べる
```

こと。

見るべき核心はこれ。

```cpp
value[gainLevel][inputValue]
```

この `gainLevel` が、

```text
0〜3しか使われないのか
0〜15まで使われるのか
どうやって計算されるのか
```

を追う。

その結果で、移植方針が決まる。

```text
0〜3しか使わない
→ 先頭4段コピーでもよい可能性

0〜15まで使う
→ 4段を16段へ展開する必要あり

既存大LUTと比較して4回複製に近い
→ dstGain / 4 方式が有力

なめらかに変化している
→ 補間方式の可能性
```

なので、次の作業はこれです。

```text
MatrixLUTの適用箇所を探して、
value[?][?] の1個目のindexが何から来ているか調べる
```

ここが見つかれば、かなり前進します。







うん、**かなり分かります。**
ただし、分かることと分からないことを分けた方がいいです。

結論から言うと、

```text
MatrixLUT適用前後の画像をbin出力してCSV比較する
```

これは **MatrixLUTが効いているか** を確認するには有効です。

でも、それだけで必ず、

```text
4段を16段へどう割り当てるべきか
```

まで完全に分かるとは限りません。

---

# 何が分かるか

MatrixLUTはたぶんこういう処理です。

```cpp
after = MatrixLUT[gainLevel][before];
```

つまり、

```text
入力画素値 before
↓
LUTを通す
↓
出力画素値 after
```

です。

なので、適用直前と直後を出せば、

```text
before の値が after でどう変わったか
```

が分かります。

例えばCSVで、

```text
before = 100
after  = 25
```

なら、

```text
MatrixLUT[どこかのgainLevel][100] = 25
```

が使われた可能性が高いです。

---

# かなり有効な調査方法

やるなら、こうです。

```text
1. MatrixLUT適用直前の画像をbin出力
2. MatrixLUT適用直後の画像をbin出力
3. 同じ座標の before / after を比較
4. before値をindexとして、MatrixLUTのどのgain段に一致するか調べる
```

ここがポイントです。

単に画像を見比べるだけではなく、

```text
before値をLUTのindexとして使う
```

のが重要です。

---

# 具体例

例えば、ある座標でこうだったとします。

```text
before[y][x] = 312
after[y][x]  = 78
```

MatrixLUTが `value[16][1024]` なら、Pythonでこう調べます。

```python
for gain in range(16):
    if matrix_lut[gain][312] == 78:
        print("この座標では gain", gain, "が使われた可能性あり")
```

もし `gain=4,5,6,7` のどれかが一致するなら、そのあたりのLUT段が使われている可能性があります。

---

# Pythonで調べるイメージ

MatrixLUTを `matrix_lut.bin` として取り出せている前提です。

```python
import numpy as np

WIDTH = 3840
HEIGHT = 1860

before = np.fromfile("before_matrix_lut.bin", dtype=np.uint16).reshape(HEIGHT, WIDTH)
after  = np.fromfile("after_matrix_lut.bin", dtype=np.uint8).reshape(HEIGHT, WIDTH)

matrix_lut = np.fromfile("matrix_lut.bin", dtype=np.uint8).reshape(16, 1024)

# 調べたい座標
x = 1000
y = 1000

input_value = int(before[y, x])
output_value = int(after[y, x])

print("before =", input_value)
print("after  =", output_value)

for gain in range(16):
    if matrix_lut[gain, input_value] == output_value:
        print("matched gain =", gain)
```

これで、その画素に対してどのgain段のLUTが使われたか推定できます。

---

# ただし注意：beforeが12bitならindex調整があるかも

ここが重要です。

MatrixLUTのサイズが1024なら、indexは `0〜1023` です。

でも画像が12bitなら、画素値は普通、

```text
0〜4095
```

です。

その場合、LUTへ入れる前に、

```cpp
index = before >> 2;
```

みたいに10bit化している可能性があります。

つまり本当は、

```cpp
after = matrixLut[gainLevel][before >> 2];
```

かもしれません。

だからPythonでも両方試した方がいいです。

```python
input_raw = int(before[y, x])

candidate_indexes = [
    input_raw,
    input_raw >> 2,
    input_raw >> 4,
]

for idx in candidate_indexes:
    if 0 <= idx < 1024:
        print("try index =", idx)

        for gain in range(16):
            if matrix_lut[gain, idx] == output_value:
                print("matched gain =", gain, "index =", idx)
```

---

# ただし1画素だけでは弱い

1画素だけだと偶然一致します。

`uint8_t` の出力は `0〜255` なので、複数のLUT段が同じ値を返すことがあります。

だから、複数画素で調べた方がいいです。

```text
100点くらいサンプリングする
↓
各点で一致するgain候補を出す
↓
一番多く一致するgainを見る
```

---

# 複数画素で推定するPython例

```python
import numpy as np
from collections import Counter

WIDTH = 3840
HEIGHT = 1860

before = np.fromfile("before_matrix_lut.bin", dtype=np.uint16).reshape(HEIGHT, WIDTH)
after  = np.fromfile("after_matrix_lut.bin", dtype=np.uint8).reshape(HEIGHT, WIDTH)

matrix_lut = np.fromfile("matrix_lut.bin", dtype=np.uint8).reshape(16, 1024)

counter = Counter()

# 画像の中央付近を粗くサンプリング
for y in range(200, HEIGHT - 200, 100):
    for x in range(200, WIDTH - 200, 100):
        input_raw = int(before[y, x])
        output_value = int(after[y, x])

        # 12bit -> 10bit の可能性も見る
        candidate_indexes = [
            input_raw,
            input_raw >> 2,
            input_raw >> 4,
        ]

        for idx in candidate_indexes:
            if not (0 <= idx < 1024):
                continue

            for gain in range(16):
                if int(matrix_lut[gain, idx]) == output_value:
                    counter[(gain, "idx=" + str(idx))] += 1

print(counter.most_common(20))
```

ただ、このままだとindex値ごとに分かれてしまうので、実際には `gain` と `shift` で集計する方がよいです。

```python
from collections import Counter

counter = Counter()

shifts = [0, 2, 4]

for y in range(200, HEIGHT - 200, 100):
    for x in range(200, WIDTH - 200, 100):
        input_raw = int(before[y, x])
        output_value = int(after[y, x])

        for shift in shifts:
            idx = input_raw >> shift

            if not (0 <= idx < 1024):
                continue

            for gain in range(16):
                if int(matrix_lut[gain, idx]) == output_value:
                    counter[(gain, shift)] += 1

print(counter.most_common(20))
```

これで、

```text
(gain=7, shift=2) が大量に一致
```

みたいな結果が出たら、

```cpp
after = matrixLut.value[7][before >> 2];
```

の可能性が高いです。

---

# 何が分からないか

この方法で分かるのは主に、

```text
実行時にどのgain段が使われているか
before値がどうindex化されているか
MatrixLUTが本当に適用されているか
```

です。

でも、これだけでは、

```text
小さい4段を大きい16段へどう移植すべきか
```

が直接決まるとは限りません。

なぜなら、実行時にたまたま `gain=3` しか使われていなかったら、他のgain段の使われ方は観測できないからです。

---

# より強く調べる方法

もしゲイン条件を変えられるなら、かなり強いです。

```text
低ゲインのフレーム
中ゲインのフレーム
高ゲインのフレーム
```

をそれぞれ流して、MatrixLUT適用前後を出します。

そして、それぞれで使われたgain段を推定します。

```text
低ゲイン → gain 0〜3
中ゲイン → gain 4〜7
高ゲイン → gain 12〜15
```

みたいに見えれば、16段がちゃんと使われていることが分かります。

---

# 実験としてはかなり良い

あなたの案はこういう意味で良いです。

```text
MatrixLUT適用前後の画像を出す
↓
before/after の対応を見る
↓
どのLUT段を通ったか逆算する
```

これはかなり実務的です。

特に、コードを読むだけで分からない場合、

```text
入出力から逆算する
```

という調査方法として有効です。

---

# 注意点

## 1. beforeとafterの型が違う可能性

MatrixLUT前は `uint16_t`、後は `uint8_t` かもしれません。

```text
before: 12bit相当なので uint16
after : LUT結果なので uint8
```

ここを間違えてPythonで読むとズレます。

---

## 2. LineSpan / padding に注意

出力binが単純な `width * height` でない可能性があります。

行ごとにpaddingがあるなら、reshape前にLineSpanを考慮する必要があります。

---

## 3. LUT前に別補正が入っている可能性

本当に「MatrixLUT直前」「MatrixLUT直後」で出せるなら強いです。

でも少し離れた場所で出すと、

```text
MatrixLUT以外の補正も混ざる
```

可能性があります。

だから出力位置はできるだけ、

```cpp
// MatrixLUT適用直前
dump(before);

// MatrixLUT適用

// MatrixLUT適用直後
dump(after);
```

に近い方がいいです。

---

# 結論

はい、分かります。

ただし、見るべきなのは画像そのものではなく、

```text
before画素値
after画素値
MatrixLUT[gain][index]
```

の対応です。

この3つを突き合わせると、

```text
どのgain段が使われたか
12bit値をどう1024 indexにしているか
MatrixLUTが効いているか
```

がかなり分かります。

実験としては、

```text
MatrixLUT適用直前bin
MatrixLUT適用直後bin
MatrixLUT本体bin
```

この3つがあるとかなり強いです。
