
# メンバ関数を `std::function` に渡すときの落とし穴とラムダによる解決

---

## はじめに

`std::unordered_map` に `std::function` を格納して、名前から関数を呼び出す構成はよく使われます。

```cpp
using MyFunc = std::function<int(int)>;
std::unordered_map<std::string, MyFunc> functions;
```

しかしこのとき、**クラスのメンバ関数を登録しようとすると、意図どおりにいかない**ことがあります。

この記事では、どこでつまずくのか、なぜそうなるのか、そしてどう解決するかを整理します。

---

## まず前提：普通の関数とメンバ関数は別物である

次のような2つの関数を考えます。

```cpp
// ① 普通の関数（フリー関数）
int doubleValue(int a) {
    return a * 2;
}

// ② クラスのメンバ関数
class Sample {
    int doubleValue(int a) {
        return a * 2;
    }
};
```

見た目はほとんど同じです。しかし C++ の内部では、**これらは根本的に異なる**ものとして扱われます。

普通の関数のシグネチャは `int(int)` です。

一方、メンバ関数のシグネチャは実際には `int(Sample*, int)` に近い形です。
呼び出し時に「どのインスタンスに対して実行するか」を示す **`this` ポインタ** が必ず必要だからです。

---

## 間違いのコード

```cpp
class Sample {
private:
    std::unordered_map<std::string, std::function<int(int)>> functions;

    int sampleFunc1(int a) {
        return a;
    }

    int sampleFunc2(int a) {
        return a + 1;
    }

public:
    void initializeFunctions() {
        registFunction("sample1", sampleFunc1);  // ❌ コンパイルエラー
        registFunction("sample2", sampleFunc2);  // ❌ コンパイルエラー
    }
};
```

---

### 間違い：メンバ関数をそのまま `std::function` に渡せない

`registFunction("sample1", sampleFunc1)` はコンパイルエラーになります。

`std::function<int(int)>` は「引数が `int` 1つ、戻り値が `int` の呼び出し可能なもの」を格納する型です。

しかし `sampleFunc1` はメンバ関数です。これを呼び出すには `this`（インスタンス）が必要です。

```
// 内部的に必要なシグネチャ
int sampleFunc1(Sample* this, int a)
//              ^^^^^^^^^^^^^^
//              これが暗黙的に存在している
```

つまり「引数が `int` 1つ」ではなく、「`Sample*` と `int` の2つ」が必要な関数なのです。
`std::function<int(int)>` には型が合いません。

---

## 解決：ラムダで `this` を束縛する

```cpp
void initializeFunctions() {
    registFunction("sample1", [this](int a) { return sampleFunc1(a); });
    registFunction("sample2", [this](int a) { return sampleFunc2(a); });
}
```

ラムダの `[this]` は「このインスタンスを捕捉する」という指示です。

これにより、ラムダの内部では `this->sampleFunc1(a)` と同じことができます。

ラムダ自体のシグネチャは `int(int)` であり、`std::function<int(int)>` と型が合います。

---

## `[this]` キャプチャとは何か

ラムダは `[]` の中に**何を外から持ち込むか**を書きます。これをキャプチャと呼びます。

```cpp
int x = 10;

auto f = [x](int a) { return a + x; };   // x の値をコピーして持ち込む
auto g = [&x](int a) { return a + x; };  // x への参照を持ち込む
auto h = [this](int a) { return sampleFunc1(a); };  // this ポインタを持ち込む
```

メンバ関数を呼ぶためには `this` が必要です。
`[this]` と書くことで、ラムダの中からメンバ関数やメンバ変数にアクセスできるようになります。

---

## 「複雑な関数にはラムダが使えない」は誤解

「ラムダで書けるのは短い処理だけ」と思いがちですが、これは誤解です。

ラムダをメンバ関数の**薄いラッパー**として使えばよいからです。

```cpp
// sampleFunc1 の中身が何百行でも、ラムダ自体は1行
registFunction("sample1", [this](int a) { return sampleFunc1(a); });
```

複雑なロジックはメンバ関数の中に置いたままにします。
ラムダはただ「`std::function` と `this` を繋ぐ橋渡し」として機能します。

---

## 他の方法との比較

同じことを実現する方法は複数あります。

### `std::bind`

```cpp
#include <functional>

registFunction("sample1", std::bind(&Sample::sampleFunc1, this, std::placeholders::_1));
registFunction("sample2", std::bind(&Sample::sampleFunc2, this, std::placeholders::_1));
```

C++11 時代からある方法です。動作は正しいですが、`std::placeholders::_1` の記述が冗長になりがちです。
現代の C++ ではラムダの方が読みやすいとされ、`std::bind` はあまり使われなくなっています。

### メンバ関数ポインタ

```cpp
using MemberFunc = int (Sample::*)(int);
std::unordered_map<std::string, MemberFunc> functions;

functions["sample1"] = &Sample::sampleFunc1;
functions["sample2"] = &Sample::sampleFunc2;

// 呼び出し
int result = (this->*functions["sample1"])(42);
```

`std::function` のオーバーヘッドがなく最速ですが、`(this->*f)(arg)` という呼び出し構文が特殊です。
また `std::function` との互換性がなくなるため、マップの型を変える必要があります。

---

### 比較まとめ

| 方法 | 記述のシンプルさ | `std::function` との相性 | 速度 |
|---|---|---|---|
| ラムダ薄ラッパー | ◎ | ◎ | ○ |
| `std::bind` | △（`_1` が冗長） | ◎ | ○ |
| メンバ関数ポインタ | △（呼び出し構文が特殊） | ✕（型が変わる） | ◎ |

ほとんどの場面では**ラムダ薄ラッパーが最も読みやすく、実用的**です。

---

## 修正後のコード

```cpp
#include <functional>
#include <unordered_map>
#include <string>

using MyFunc = std::function<int(int)>;

class Sample {
private:
    std::unordered_map<std::string, MyFunc> functions;

    int sampleFunc1(int a) {
        return a;
    }

    int sampleFunc2(int a) {
        return a + 1;
    }

public:
    MyFunc createFunction(const std::string& name) {
        return functions.at(name);   // 未登録なら例外が出る（安全）
    }

    void registFunction(const std::string& name, MyFunc function) {
        functions[name] = function;
    }

    void initializeFunctions() {
        registFunction("sample1", [this](int a) { return sampleFunc1(a); });
        registFunction("sample2", [this](int a) { return sampleFunc2(a); });
    }
};
```

`createFunction` で `functions[name]` を使っていた部分も `functions.at(name)` に直しています。
`operator[]` は存在しないキーに対してデフォルト値を挿入してしまうため、`.at()` の方が意図が明確です。

---

## まとめ

| ポイント | 内容 |
|---|---|
| 間違い | メンバ関数は `this` が必要なため、`std::function<int(int)>` に直接渡せない |
| 解決策 | `[this]` キャプチャのラムダで薄いラッパーを作る |
| 重要な考え方 | 「複雑な処理 = ラムダが使えない」ではなく、複雑なロジックはメンバ関数に残したままでよい |

メンバ関数を `std::function` に格納したいとき、ラムダは架け橋として非常にシンプルに機能します。
