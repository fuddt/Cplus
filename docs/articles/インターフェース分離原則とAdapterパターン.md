# インターフェース分離原則と Adapter パターン

> **前提：** この記事は C++ の純粋仮想関数・継承・ポリモーフィズムを
> 理解している人向けに、**設計レベルの問題** を扱う。
> コードの「動き方」ではなく「構造として正しいか」の話。

---

## 第1章：その違和感は正しい

こういう状況に陥ったことはないか。

```cpp
class BigInterface
{
public:
    virtual void load()      = 0;
    virtual void A()         = 0;
    virtual void B()         = 0;
    virtual void C()         = 0;
    virtual void D()         = 0;
    // ... さらに続く
};

class MyClass : public BigInterface
{
public:
    void load() override { /* 実際の処理 */ }
    void A()    override { /* 実際の処理 */ }
    void B()    override { /* 実際の処理 */ }
    void C()    override {}  // 使わない。でも実装しないとコンパイルエラー
    void D()    override {}  // 使わない。でも実装しないとコンパイルエラー
};
```

「使わない関数を空で実装するのは何か変だ」

この違和感は正しい。
これは設計原則レベルの問題だ。

---

## 第2章：ISP（インターフェース分離の原則）

SOLID の **I** にあたる原則。

> **Interface Segregation Principle（ISP）**
> クライアントは、使わないメソッドに依存させてはいけない。

「太ったインターフェース（God Interface）」の典型例：

```cpp
// 悪い設計：1つに詰め込みすぎ
class PrinterScannerFax
{
public:
    virtual void print()   = 0;
    virtual void scan()    = 0;
    virtual void fax()     = 0;
    virtual void staple()  = 0;
};
```

FAX 機能のない安いプリンタに `fax()` を実装させられる。

```cpp
// 良い設計：役割で分割
class IPrinter { virtual void print() = 0; };
class IScanner  { virtual void scan()  = 0; };
class IFax      { virtual void fax()   = 0; };
```

必要なものだけ継承できる。

### God Interface が生まれる典型的な経緯

```
昔：「共通処理をまとめよう」
→ 途中：「あれも追加しよう」
→ さらに：「これも入れよう」
→ 結果：誰も全部使わない巨大インターフェース
```

未来予測で「将来使うかもしれない」と追加し続けた結果だ。
この予測はほぼ外れる。

---

## 第3章：「本物のインターフェース」の定義

インターフェースが正しく設計されているときの条件：

> **全派生クラスが、全メソッドを意味的に持つ**

正しい例：

```cpp
class Shape
{
public:
    virtual void draw()  = 0;
    virtual double area() = 0;
};

class Circle    : public Shape { /* draw と area を実装 */ };
class Rectangle : public Shape { /* draw と area を実装 */ };
class Triangle  : public Shape { /* draw と area を実装 */ };
```

`Circle` に `draw()` は意味的にある。`area()` も意味的にある。
これは本物のインターフェース。

---

## 第4章：God Interface の診断

3つの問いで診断できる。

| 問い | YES なら |
| :--- | :--- |
| 使わない関数がある？ | 危険シグナル |
| 空実装を書いている？ | ほぼアウト |
| インターフェースの責務が複数ある？ | 分割対象 |

### 実際の診断例

10個の純粋仮想関数を持つ BigInterface と4つの派生クラスがある。
全クラスで使われているのは `load()` 1個のみ。
残りは以下のようにバラバラだ：

```
Class1 → load + A + B
Class2 → load + B + C
Class3 → load + C + D
Class4 → load + A + D
```

この状況の診断結果：

> **BigInterface は「インターフェースのふりをした機能の寄せ集め」**

唯一の本物の契約は `load()` だけだ。

```cpp
// これだけが本物のインターフェース
class ILoader
{
public:
    virtual void load() = 0;
};
```

`A` / `B` / `C` / `D` はクラスごとに異なる組み合わせで使われており、
共通の契約にはなっていない。

---

## 第5章：4つの解決策（優先順）

### 解決策1：インターフェースを分割する（最優先）

自分のコードなら迷わずこれを選ぶ。

```cpp
// 唯一の共通契約
class ILoader { public: virtual void load() = 0; };

// 機能単位で分割
class IA { public: virtual void A() = 0; };
class IB { public: virtual void B() = 0; };
class IC { public: virtual void C() = 0; };
class ID { public: virtual void D() = 0; };

// 各クラスは必要なものだけ継承する
class Class1 : public ILoader, public IA, public IB {};
class Class2 : public ILoader, public IB, public IC {};
class Class3 : public ILoader, public IC, public ID {};
class Class4 : public ILoader, public IA, public ID {};
```

空実装が消える。各クラスの責務が型で明確になる。

---

### 解決策2：Adapter を挟む（変更できない場合）

外部ライブラリ・レガシーコード・社内 DLL など、
**インターフェースを変更できない場合** に使う。

---

### 解決策3：純粋仮想をデフォルト実装に変える

```cpp
class Base
{
public:
    virtual void draw() {}  // デフォルトは何もしない
};
```

必要なものだけ override する。
ただしこれは「インターフェース」ではなく「基底クラス」になる。

---

### 解決策4：継承をやめて Composition にする

```cpp
// 継承する代わりに持つ
class Object
{
    Renderer renderer;
    Loader   loader;
};
```

責務を合成で表現する。これが一番強い解決策だが、既存コードの変更量が大きい。

---

## 第6章：Adapter パターンの詳細

「Adapter でインターフェースから必要な部分だけ切り出せるか？」

答えは **Yes。ただし発想が重要だ。**

### Adapter は「削る」のではなく「隠す」

```
誤解：BigInterface → (削る) → SmallInterface
正しい：BigInterface を Adapter で包んで SmallInterface を新たに作る
```

構造はこうなる：

```
変更前:
  Client → BigInterface

変更後:
  Client → ISmallA → Adapter → BigInterface
```

Client から BigInterface が見えなくなる。
BigInterface は Adapter の内側に隠れる。

### 実装

```cpp
// 1. 必要な部分だけの小さいインターフェースを定義する
class ISmallA
{
public:
    virtual void A() = 0;
};

// 2. Adapter を作る
class AdapterA : public ISmallA
{
private:
    BigInterface* impl_;

public:
    explicit AdapterA(BigInterface* impl) : impl_{impl} {}

    void A() override
    {
        impl_->A();  // 必要なものだけ委譲する
    }
    // B / C / D は露出しない
};

// 3. 呼び出し側は BigInterface を知らなくていい
void process(ISmallA* obj)
{
    obj->A();
}
```

### Adapter が有効な3つの場面

| 場面 | 理由 |
| :--- | :--- |
| 外部ライブラリ（DirectX / OpenCV 等） | 変更できないため |
| レガシーコード・社内 DLL | 変更コストが高いため |
| 段階的なリファクタリング | 既存コードを壊さずに移行できるため |

### Adapter が得るもの

```cpp
// テストが書きやすくなる
class MockA : public ISmallA
{
public:
    void A() override {}  // テスト用のダミー実装
};

// 依存が減る
// Client は BigInterface の存在を知らなくていい
// BigInterface を差し替えても Client は変更不要
```

---

## 第7章：安全な段階移行戦略

既存コードをいきなり壊すのは危険だ。
以下の順番で段階的に移行する。

```
Step 1：新しい小さいインターフェースを作る

    class ILoader { public: virtual void load() = 0; };


Step 2：既存の派生クラスに追加継承させる

    class Class1 : public BigInterface, public ILoader {};
    // BigInterface はまだ生きている。壊れない。


Step 3：新しいコードは ILoader を使う

    void process(ILoader* loader);  // BigInterface は使わない


Step 4：BigInterface への依存を徐々に消す

    // 既存コードを順番に process(ILoader*) に切り替える


Step 5：全部切り替わったら BigInterface を削除する
```

この戦略の利点：各 Step でビルドが通る。途中で止めても壊れない。

---

## 第8章：判断フロー

```
インターフェースが太い
│
├── 自分のコードか？
│   └── Yes → 分割する（最優先）
│
└── 変更できないコードか？
    ├── 外部ライブラリ・レガシー → Adapter 一択
    │
    └── 自分のコードだが変更コストが高い
        → 段階移行戦略（新 interface + 追加継承 → 徐々に移行）
```

---

## 理解度チェック

以下の設計について「どの問題があるか」と「どう直すか」を答えてほしい。

```cpp
class IGameObject
{
public:
    virtual void update()   = 0;
    virtual void draw()     = 0;
    virtual void onHit()    = 0;
    virtual void save()     = 0;
    virtual void load()     = 0;
    virtual void playSound() = 0;
};

class BackgroundImage : public IGameObject
{
public:
    void update()    override {}         // 動かない
    void draw()      override { /* ... */ }
    void onHit()     override {}         // 当たり判定なし
    void save()      override {}         // 保存しない
    void load()      override { /* ... */ }
    void playSound() override {}         // 音なし
};
```

---

**正解：**

**問題：**
- `BackgroundImage` は6個中4個が空実装になっている
- `IGameObject` が「ゲームオブジェクトが持ちうる全機能」を詰め込んでいる
- ISP 違反。`BackgroundImage` に必要ない責務を押し付けている

**修正方針：**

```cpp
// 役割で分割する
class IUpdatable  { public: virtual void update()    = 0; };
class IDrawable   { public: virtual void draw()      = 0; };
class ICollidable { public: virtual void onHit()     = 0; };
class ISaveable   { public: virtual void save() = 0; virtual void load() = 0; };
class IAudible    { public: virtual void playSound() = 0; };

// BackgroundImage は持つ責務だけを宣言する
class BackgroundImage : public IDrawable, public ISaveable
{
public:
    void draw() override { /* ... */ }
    void save() override {}
    void load() override { /* ... */ }
    // update / onHit / playSound の空実装は消える
};

// 動く敵キャラはこう
class Enemy : public IUpdatable, public IDrawable, public ICollidable, public IAudible
{
    // 全部に意味のある実装が書ける
};
```

> **重要な視点：**
> インターフェースは「このクラスが持つ全機能のリスト」ではない。
> インターフェースは「この役割を持つという契約」だ。
> 空実装が出た瞬間、「インターフェースが役割ではなくリストになっている」と疑う。
