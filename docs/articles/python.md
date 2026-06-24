```
Pythonでライブラリのバージョンによって処理を分岐する方法

Pythonで開発をしていると、

* 開発環境では NumPy 1.x
* 本番環境では NumPy 2.x
* お客様環境では pandas の古いバージョン

といった状況に遭遇することがあります。

そのような場合、ライブラリのバージョンによって処理を切り替えたいことがあります。

本記事では、Pythonでライブラリのバージョンを取得し、実行時に処理を分岐する方法を紹介します。

⸻

なぜ必要なのか

例えば NumPy 2.0 では、一部のAPIが変更・削除されています。

以下のようなコードがあった場合、

import numpy as np
result = np.some_function()

環境によっては、

AttributeError: module 'numpy' has no attribute 'some_function'

が発生する可能性があります。

このようなケースでは、

* NumPy 1.x
* NumPy 2.x

で処理を分岐する必要があります。

⸻

方法1：ライブラリの version を利用する

多くのライブラリは __version__ 属性を持っています。

import numpy as np
print(np.__version__)

実行結果

2.3.0

取得したバージョンで分岐できます。

import numpy as np
if np.__version__.startswith("1."):
    print("NumPy 1系")
else:
    print("NumPy 2系")

ただし、この方法には問題があります。

* 全ライブラリが __version__ を持っているわけではない
* 文字列比較は危険

という点です。

⸻

方法2：importlib.metadata を利用する

Python 3.8以降では標準ライブラリでバージョン取得が可能です。

from importlib.metadata import version
numpy_version = version("numpy")
print(numpy_version)

実行結果

2.3.0

ライブラリを import しなくても取得できます。

⸻

NG例：文字列比較

一見問題なさそうですが、

if version("numpy") >= "2.0":
    ...

は危険です。

例えば、

print("10.0" < "2.0")

実行結果

True

になります。

これは文字列比較だからです。

⸻

方法3：packaging.version を使う（推奨）

バージョン比較は専用ライブラリを使います。

from importlib.metadata import version
from packaging.version import Version
numpy_ver = Version(version("numpy"))
if numpy_ver >= Version("2.0.0"):
    print("NumPy 2系以上")
else:
    print("NumPy 1系")

これが最も安全です。

⸻

実践例

NumPy 2.0以上で新APIを使用する例です。

from importlib.metadata import version
from packaging.version import Version
NP_VER = Version(version("numpy"))
def calculate():
    if NP_VER >= Version("2.0.0"):
        return execute_new_api()
    else:
        return execute_old_api()

こうすることで、

* 開発環境
* テスト環境
* 本番環境

でライブラリのバージョンが異なっていても対応できます。

⸻

さらに実務的な方法：機能の有無で判定する

実は、実務ではバージョン番号を見るよりも、

「その機能が存在するか」

を確認する方が安全なことがあります。

例えば、

import numpy as np
if hasattr(np, "new_function"):
    np.new_function()
else:
    np.old_function()

という書き方です。

⸻

なぜこちらが強いのか

例えば、

NumPy 2.0
NumPy 2.1
NumPy 2.2

で機能が追加・削除される可能性があります。

バージョン番号だけに依存すると、

if version >= 2.0:

では対応できないケースがあります。

一方、

hasattr()

なら実際に機能が存在するかどうかを確認できます。

⸻

実務でのおすすめ

優先順位としては次の順番です。

① 機能有無で判定（推奨）

if hasattr(module, "new_function"):
    ...

② 例外処理で吸収

try:
    module.new_function()
except AttributeError:
    module.old_function()

③ バージョン判定

if Version(version("numpy")) >= Version("2.0.0"):
    ...

⸻

まとめ

Pythonではライブラリのバージョンを取得して処理を分岐できます。

代表的な方法は以下の3つです。

方法	推奨度	特徴
__version__	★★☆☆☆	簡単だが非推奨
importlib.metadata.version()	★★★★☆	標準ライブラリで取得可能
packaging.version.Version()	★★★★★	正確なバージョン比較が可能

ただし実務では、バージョン番号ではなく、

* hasattr()
* try-except

による機能存在チェックの方が保守性が高いケースも多くあります。

「バージョンで判断するべきか、機能で判断するべきか」

を状況に応じて使い分けることが重要です。

```