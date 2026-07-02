```

はい、それでほぼ確定です。

その buf は IYUV / YUV420 planar 用のバッファです。

org = new byte[(width << 2) * height]

これは

(width * 4) * height

なので、たぶん BGRA / 32bit画像用バッファ。

一方、

buf = new byte[
    width * height
    + (width >> 1) * height
]

これは計算すると、

width * height
+ width/2 * height
= width * height * 1.5

つまり、

YUV420 / IYUV のサイズ

です。

⸻

y / u / v の意味

y = 0
u = width * height
v = u + (width >> 1) * (height >> 1)

これはこういうメモリ配置です。

buf
+--------------------------+
| Y plane                  |  width * height
+--------------------------+
| U plane                  |  width/2 * height/2
+--------------------------+
| V plane                  |  width/2 * height/2
+--------------------------+

つまり、

Y開始位置 = 0
U開始位置 = width * height
V開始位置 = width * height + width/2 * height/2

です。

これはまさに IYUV / I420 の典型的な並びです。

⸻

つまりWriteFrameに渡すべきもの

たぶん、Python側で渡すべきはRGB/BGR画像ではなく、

IYUV形式に変換したbuf

です。

Pythonならまずこうです。

# rgb: PIL.Image から作った RGB の NumPy配列
rgb = np.array(pil_img.convert("RGB"), dtype=np.uint8)
# RGB → BGR
bgr = cv2.cvtColor(rgb, cv2.COLOR_RGB2BGR)
# BGR → IYUV / I420
buf = cv2.cvtColor(bgr, cv2.COLOR_BGR2YUV_I420)
buf = np.ascontiguousarray(buf, dtype=np.uint8)
ptr = buf.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
dll.MFVCWriteFrame(h, ptr, buf.nbytes)

buf.nbytes は、

width * height * 3 / 2

になるはずです。

⸻

C#の輝度補正はどこに入れる？

YUV のうち、輝度は Y plane です。

なので、PythonでC#側の輝度補正を再現するなら、基本は buf の先頭 width * height 部分に対して処理します。

y_size = width * height
# 1次元にして扱う
flat = buf.reshape(-1)
# Y成分だけ取り出す
y_plane = flat[:y_size]
# 例: 輝度を +20 して 0〜255 にクリップ
work = y_plane.astype(np.int16) + 20
flat[:y_size] = np.clip(work, 0, 255).astype(np.uint8)

U/Vは色差なので、輝度補正だけなら基本いじりません。

⸻

今の破損原因

かなり高確率でこれです。

DLLは IYUV / YUV420 の buf を期待している
でもPythonから RGB/BGR の img.nbytes を渡していた

サイズも中身も違うので、動画が破損扱いになるのは自然です。

次は、cv2.COLOR_BGR2YUV_I420 で作った buf を30フレーム分渡して試すのが最短です。```