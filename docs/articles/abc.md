

---

# サンプル① Handle生成

一番簡単なもの。

```python
import ctypes

dll = ctypes.WinDLL("MFVideoCreate.dll")

dll.MFCreateHandle.restype = ctypes.c_void_p

handle = dll.MFCreateHandle()

print(handle)
```

ここで伝えたいこと

> PythonでもC++のポインタ(IntPtr相当)を保持できる。

---

# サンプル② Handle受け渡し

```python
import ctypes

dll = ctypes.WinDLL("MFVideoCreate.dll")

dll.MFVCCreateHandle.restype = ctypes.c_void_p
dll.MFVCDestroyHandle.argtypes = [ctypes.c_void_p]

handle = dll.MFCreateHandle()

dll.MFVCDestroyHandle(handle)
```

ここで伝えたいこと

> `c_void_p` と `argtypes` を設定すれば、そのまま別関数へ渡せる。

---

# サンプル③ NumPy→DLL

これが今回一番重要。

```python
from PIL import Image
import numpy as np
import ctypes

# PIL画像取得
pil_img = Image.open("sample.png")

# NumPyへ変換
img = np.array(pil_img)

# DLLへ渡せるよう連続メモリ化
img = np.ascontiguousarray(img)

# ポインタ取得
ptr = img.ctypes.data_as(
    ctypes.POINTER(ctypes.c_ubyte)
)

# バッファサイズ
length = img.nbytes

dll.MFWriteFrame(
    handle,
    ptr,
    length
)
```

ここで伝えたいこと

```text
PIL.Image
      ↓
NumPy
      ↓
連続メモリ
      ↓
unsigned char*
      ↓
DLL
```

---

# サンプル④ 全体フロー

最後に1枚絵。

```text
             Python

    CreateHandle()
           │
           ▼
      MFSetup()

           │
           ▼

+------------------------------+

    PIL.Image

         │

         ▼

    NumPy変換

         │

         ▼

 ascontiguousarray()

         │

         ▼

 ctypes.data_as()

         │

         ▼

 MFWriteFrame()

         │

         ▼

（全画像繰り返す）

+------------------------------+

           │
           ▼

    DestroyHandle()

           │
           ▼

         MP4完成
```

---

## そして最後に

私は**比較コード**も載せます。

### C#

```csharp
Bitmap

↓

LockBits()

↓

BitmapData

↓

Marshal.Copy()

↓

GCHandle

↓

AddrOfPinnedObject()

↓

WriteFrame()
```

↓

### Python

```python
img = np.array(pil_img)

img = np.ascontiguousarray(img)

ptr = img.ctypes.data_as(...)

dll.MFVCWriteFrame(...)
```

これを並べると、

**「Python版はこんなにシンプルになるのか」**

ということが一目で伝わります。

---

### もし私がこの資料を作るなら

最後に**「本番イメージ（擬似コード）」**を1ページ載せます。

```python
handle = CreateHandle()

Setup(...)

for image in images:

    # 点描画
    pil_img = draw_point(image)

    # NumPyへ変換
    img = np.array(pil_img)

    # DLLへ渡せるようにする
    img = np.ascontiguousarray(img)

    ptr = img.ctypes.data_as(
        ctypes.POINTER(ctypes.c_ubyte)
    )

    WriteFrame(
        handle,
        ptr,
        img.nbytes
    )

DestroyHandle(handle)
```


