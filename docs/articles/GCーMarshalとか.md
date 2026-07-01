ここは**「何を実現したいのか」**から考えると、一気に理解できます。

実は、C#もPythonも最終目的は全く同じです。

---

# 最終目的

DLLはこれが欲しいだけです。

```cpp
const unsigned char* buffer
```

つまり

> **「画像データが並んでいるメモリの先頭アドレスをください。」**

これだけなんです。

DLLからすると

```text
255
123
54
・・・

というデータがどこにあるか
```

しか興味がありません。

---

# C#ではどうやってそこまで行く？

ここを一段ずつ見てみます。

## Step1 Bitmap

最初は

```text
Bitmap
```

があります。

でもBitmapは

> 「画像ですよ」

というオブジェクトです。

DLLはBitmapなんて知りません。

---

## Step2 LockBits()

だから

```text
Bitmap

↓

LockBits()

↓

BitmapData
```

に変えます。

BitmapDataになると

```text
画像の幅

高さ

Stride

Scan0
```

など

**画像の生データ**

へアクセスできます。

---

## Step3 Scan0

ここで

```text
Scan0
```

が

```text
画像データの先頭アドレス
```

になります。

例えば

```text
0x12345678
```

です。

でもDLLは

```cpp
unsigned char*
```

が欲しい。

---

## Step4 Marshal.Copy()

BitmapDataは

内部的にはBitmap専用の構造です。

そこで

```text
BitmapData

↓

byte[]
```

へコピーします。

つまり

```text
255

120

80

・・・

```

だけの配列を作っています。

---

## Step5 GCHandle.Alloc()

ここが一番重要。

C#の

```text
byte[]
```

は

GCが管理しています。

GCは

```text
メモリ整理するね！

ここへ移動！
```

を勝手にやります。

すると

DLLは

```text
0x12345678
```

を覚えているのに

実際には

```text
0x87654321
```

へ移動してしまいます。

だから

```csharp
GCHandle.Alloc(...Pinned)
```

で

> 「この配列だけは絶対動かさないで！」

とお願いしています。

---

## Step6 AddrOfPinnedObject()

そして最後に

```text
固定されたbyte[]
```

の

先頭アドレス

を取得します。

それが

```text
buf_ptr
```

です。

そして

```text
WriteFrame(
    h,
    buf_ptr,
    buf.Length
)
```

になります。

---

# Pythonでは？

ここからが面白いところです。

Python(OpenCV)

あるいは

PIL→NumPy

まで来ると

```python
img = np.array(...)
```

この時点で

```text
NumPy配列
```

になります。

---

## NumPy配列とは？

NumPyは

見た目は

```python
[[1,2,3],
 [4,5,6]]
```

ですが

内部では

```text
1 2 3 4 5 6
```

という

**C言語が扱いやすい連続メモリ**

になっています。

つまり

DLLが欲しい形にかなり近い。

---

# ascontiguousarray()

ここだけ保険です。

NumPyは

場合によって

```text
1

2

3

・・・

```

が

飛び飛びの場所に置かれることがあります。

DLLは

```text
255

120

80

・・・

```

が

きれいに並んでいることを期待します。

だから

```python
img = np.ascontiguousarray(img)
```

で

> 必ず連続したメモリ

にします。

---

# ctypes.data_as()

ここで

```python
ptr = img.ctypes.data_as(...)
```

を呼ぶと

NumPyは

```text
私の先頭アドレスは

0x12345678

です
```

と教えてくれます。

つまり

C#の

```text
AddrOfPinnedObject()
```

と

役割はほぼ同じです。

---

# nbytes

DLLは

```cpp
buffer
```

だけでは困ります。

どこまで読めばいいか分からない。

だから

```python
img.nbytes
```

で

```text
全部で6220800Byteあります
```

と教えます。

これは

```text
buf.Length
```

そのものです。

---

# 一番大事なこと

ここまでを図にすると

## C#

```text
Bitmap
      │
      ▼
LockBits()
      │
      ▼
BitmapData
      │
      ▼
Marshal.Copy()
      │
      ▼
byte[]
      │
      ▼
GCHandle(Pinned)
      │
      ▼
AddrOfPinnedObject()
      │
      ▼
WriteFrame()
```

---

## Python

```text
PIL.Image
      │
      ▼
NumPy配列
      │
      ▼
ascontiguousarray()
      │
      ▼
ctypes.data_as()
      │
      ▼
WriteFrame()
```

---

# なぜPythonはこんなに短いの？

答えは

**NumPyがDLLと相性抜群だからです。**

C#は

```text
Bitmap
```

という

GUI用のオブジェクト

から

DLL用の

```text
unsigned char*
```

へ

一生懸命変換しています。

一方NumPyは

最初から

```text
C言語向けの配列
```

なんです。

だから

```text
Marshal.Copy

GCHandle

AddrOfPinnedObject
```

みたいな準備がほとんど不要になります。

---

## 私が今回一番伝えたいこと

最初は

> 「Pythonだから簡単なんだ」

と思っていたかもしれません。

でも、本質は違います。

**簡単なのはPythonだからではありません。**

**NumPyが「C/C++とデータをやり取りするための配列」として非常によく設計されているから**です。

だから今回のDLLとの相性が非常によく、C#で十数行かかっていた「画像バッファをDLLへ渡す準備」が、Pythonでは数行で済むわけです。

これが今回の調査で一番重要な理解だと思います。
