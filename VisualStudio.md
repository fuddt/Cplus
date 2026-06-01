以下、Visual Studio で cpp / hpp を移動したあとに include NotFound を避けるための、画像付き手順書としてまとめる。
先に前提だけ言うと、Visual Studio の Solution Explorer の見た目と、実際のファイル配置と、コンパイラの #include 探索経路は別物だ。さらに C++ プロジェクトの Source Files / Header Files は .vcxproj.filters による論理フォルダであって、コンパイラの探索場所そのものではない。Microsoft Learn と Microsoft Q&A の内容でもその点は明確にされている。  ￼

画像の見方

1枚目と2枚目は、Solution Explorer でプロジェクトを右クリックして Add > Existing Item... に入る流れの参考画像。3枚目は Solution Explorer 上でどこが project node かの参考。4枚目は プロジェクトの Properties から Include Directories を確認する画面の参考。UI は Visual Studio の版やプロジェクト種別で多少違うが、操作の考え方は同じだ。Microsoft Learn でも、C++ プロジェクトへの追加は project を右クリックして Add > Existing、設定変更は project を右クリックして Properties が基本導線になっている。  ￼

⸻

手順書: include NotFound を避ける安全な移動方法

0. まず理解すべきこと

Visual Studio で新しいコードや既存ファイルを扱う基本導線は、Solution Explorer の project node から Add > New または Add > Existing だ。C++ のビルド設定は project node の Properties から行う。VC++ Directories は project 単位の設定で、solution ではない。  ￼

また、#include "xxx.hpp" はまず親ソースファイルのあるディレクトリから探し、それで見つからなければ Additional Include Directories (/I) に進む。Visual Studio の開発環境では、INCLUDE 環境変数ではなく project properties で指定した include directories が使われる。だから、見た目上 Header Files に入っていても、それだけでは #include は解決しない。  ￼

⸻

1. やってはいけない操作

Visual Studio を開いたまま、エクスプローラでファイルを移動する。
これが一番危ない。vcxproj 自体が文字通り破損するとは限らないが、project が保持している参照パスと実ファイルの場所がズレる。結果として「ファイルはあるのに見つからない」「include が通らない」「後で LNK2019 になる」という壊れ方をしやすい。Solution Explorer の filters は表示整理用であって、コンパイラ探索とは別だから、見た目だけ正しくても信用できない。  ￼

⸻

2. 正しい移動手順

手順1: いったん Visual Studio を閉じる

開いたまま物理移動すると参照ズレの原因になるので、まず閉じる。これは Microsoft の仕様文そのものではなく、project 参照と物理ファイルをズラさないための実務上の安全策だが、根拠は上で述べた Add > Existing ベースの project 管理と、filters が物理配置や include 探索と別である点にある。  ￼

手順2: エクスプローラで実ファイルを移動する

ここで実際に .cpp と .hpp を意図したフォルダに移す。
おすすめは次のような構成だ。

ProjectRoot/
  include/
    MyClass.hpp
  src/
    MyClass.cpp
    main.cpp

この構成にすると、ヘッダの置き場と実装の置き場が分かれ、後で include path を通しやすい。これは Microsoft 公式の「こうしろ」という固定構成ではないが、C++ の include 探索ルールと Visual Studio の project properties の使い方に合っている。  ￼

手順3: Visual Studio を開き直す

開き直した時点で、移動したファイルが未登録になったり、古いパスを指していることがある。ここで Solution Explorer の見た目だけで判断しない。3枚目のように project node を基準に操作する。  ￼

手順4: Add > Existing Item... で再登録する

画像1枚目・2枚目の流れで、project を右クリック → Add → Existing Item... を選び、移動後の .cpp / .hpp を project に再追加する。Microsoft Learn の C++ project 管理でも、既存ファイルは Add > Existing で追加するのが正式な導線だ。  ￼

手順5: 必要なら include path を設定する

画像4枚目の系統の画面で、project を右クリック → Properties → Configuration Properties → VC++ Directories を開き、Include Directories を確認する。Microsoft Learn では、このプロパティページは build 時に使う各種ディレクトリを project 単位で指定する場所として説明されている。  ￼

たとえば include フォルダを使うなら、Include Directories に

$(ProjectDir)include

を追加する。

すると、src/main.cpp などから

#include "MyClass.hpp"

で解決しやすくなる。これは #include の探索順が、親ソースのディレクトリの次に Additional Include Directories に進む、という Microsoft の #include 説明に沿っている。  ￼

⸻

3. 画像と対応する実操作

画像1・2: Add > Existing Item...

ここでやることは、移動後のファイルを project に登録し直すこと。
「ファイルを作る」のではなく、「すでにあるファイルを project に含める」が目的。C++ の project 管理の公式説明でも、追加は Add > Existing が基本になっている。  ￼

画像3: Project node を選ぶ

設定変更は solution ではなく C++ project node に対して行う。Microsoft Learn でも、VC++ Directories が見えない場合は top-level solution ではなく C++ project node を選べと明記されている。  ￼

画像4: Include Directories

ここで Include Directories を見る。
ただし Microsoft Learn では、このページは「現在値を見る」用途としては有効だが、複数 project で再利用するなら property sheet の利用がより良いとしている。今回のような個別 project の復旧なら、この画面の確認で十分だ。  ￼

⸻

4. エラー別の切り分け

cannot open include file / include NotFound

これは ヘッダ探索の問題。
主に次のどれかだ。
	•	実ファイルを移動したが再登録していない
	•	#include の相対位置が合っていない
	•	Include Directories にヘッダ置き場を足していない
	•	Header Files の見た目だけを信じている

根拠は、#include の探索順が親ソースのディレクトリ → Additional Include Directoriesであり、filters はそれに影響しない、という公式説明にある。  ￼

LNK2019

これは 宣言は見えているが実装がリンクできない問題。
つまり .hpp は見えているが .cpp が project に入っていない、または定義シグネチャが一致していないケースが多い。これは今回の include NotFound とは別系統。Microsoft Learn の C++ project 管理でも、追加対象として source files も project に含める前提になっている。  ￼

⸻

5. 今後の運用ルール

今後はこれで固定した方がいい。
	1.	Visual Studio を開いたままエクスプローラで移動しない。
	2.	物理移動したら、Add > Existing Item... で再登録する。
	3.	Header Files / Source Files は論理フォルダであって、物理配置や include 探索場所ではないと理解する。
	4.	#include が怪しい時は、project node の Properties → VC++ Directories を確認する。
	5.	C++ の設定変更は solution ではなく project node でやる。  ￼

⸻

最短チェックリスト
	•	Visual Studio を閉じたか
	•	実ファイルは本当に移動先にあるか
	•	Add > Existing Item... で再登録したか
	•	#include の書き方は移動後の構成に合っているか
	•	Properties > VC++ Directories > Include Directories を見たか
	•	Header Files の見た目だけで安心していないか  ￼