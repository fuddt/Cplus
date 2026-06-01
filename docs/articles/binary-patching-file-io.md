# バイナリファイルを読み込み、構造体ビューでpatchして書き戻す

## 概要

この記事では、以下の流れを最小サンプルで示します。

1. バイナリファイルを `std::vector<uint8_t>` に読み込む
2. `vector` の内部メモリを構造体ビューとして見る
3. 構造体フィールド経由でpatchする
4. patch後のデータをファイルへ書き戻す

---

## レイアウト前提

ここでは仮に、バイナリファイルの先頭12バイトがこういうレイアウトだとします。

```
offset 0〜3   birthday
offset 4〜7   owner
offset 8〜11  jockey
```

構造体ではこう表します。

```cpp
struct MetaData
{
    uint8_t birthday[4];
    uint8_t owner[4];
    uint8_t jockey[4];
};
```

---

## 完成コード

```cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <stdexcept>

// バイナリファイル上のレイアウトを表す構造体
// すべて uint8_t 配列にしておくことで、padding の影響を受けにくくしている
struct MetaData
{
    uint8_t birthday[4];  // offset 0〜3
    uint8_t owner[4];     // offset 4〜7
    uint8_t jockey[4];    // offset 8〜11
};

// バイナリファイルを std::vector<uint8_t> に読み込む関数
std::vector<uint8_t> readBinaryFile(const std::string& filePath)
{
    // std::ios::binary を付けて、バイナリモードで開く
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("failed to open file: " + filePath);
    }

    // ファイル末尾へ移動して、ファイルサイズを取得する
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    if (fileSize < 0)
    {
        throw std::runtime_error("failed to get file size: " + filePath);
    }

    // 読み取り位置をファイル先頭へ戻す
    file.seekg(0, std::ios::beg);

    // ファイルサイズ分のバッファを用意する
    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));

    // ファイルサイズが0なら、そのまま空のvectorを返す
    if (buffer.empty())
    {
        return buffer;
    }

    // ifstream::read は char* を要求するため、
    // uint8_t* を char* に変換して渡す
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
    {
        throw std::runtime_error("failed to read file: " + filePath);
    }

    return buffer;
}

// バイナリデータをファイルへ書き戻す関数
void writeBinaryFile(const std::string& filePath,
                     const std::vector<uint8_t>& buffer)
{
    std::ofstream file(filePath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("failed to open output file: " + filePath);
    }

    if (!buffer.empty())
    {
        file.write(reinterpret_cast<const char*>(buffer.data()),
                   static_cast<std::streamsize>(buffer.size()));
    }

    if (!file)
    {
        throw std::runtime_error("failed to write file: " + filePath);
    }
}

// バイト列を表示する補助関数
void printBytes(const std::vector<uint8_t>& buffer)
{
    for (uint8_t b : buffer)
    {
        std::cout << static_cast<int>(b) << " ";
    }
    std::cout << std::endl;
}

// owner フィールドだけを書き換える関数
bool patchOwner(std::vector<uint8_t>& buffer,
                const std::vector<uint8_t>& patchData)
{
    // buffer が MetaData 構造体として扱えるだけのサイズを持つか確認する
    if (buffer.size() < sizeof(MetaData))
    {
        std::cerr << "buffer size is too small" << std::endl;
        return false;
    }

    // patchData のサイズが owner フィールドのサイズと一致するか確認する
    if (patchData.size() != sizeof(MetaData::owner))
    {
        std::cerr << "patch data size mismatch" << std::endl;
        return false;
    }

    // vector内部のメモリを MetaData 構造体として見る
    // ここではコピーしていない
    // buffer.data() が指す同じメモリを MetaData* として解釈している
    MetaData* meta =
        reinterpret_cast<MetaData*>(buffer.data());

    // owner フィールドだけを書き換える
    // これは実質的には buffer[4]〜buffer[7] を書き換えている
    std::memcpy(meta->owner,
                patchData.data(),
                patchData.size());

    return true;
}

int main()
{
    try
    {
        const std::string inputPath  = "input.bin";
        const std::string outputPath = "output.bin";

        // 1. バイナリファイルを vector<uint8_t> に読み込む
        std::vector<uint8_t> buffer = readBinaryFile(inputPath);

        std::cout << "before patch:" << std::endl;
        printBytes(buffer);

        // 2. 差し替えデータを用意する（owner[4] を 9,9,9,9 に差し替える）
        std::vector<uint8_t> patchData = { 9, 9, 9, 9 };

        // 3. 構造体フィールド経由で owner をpatchする
        bool ok = patchOwner(buffer, patchData);
        if (!ok)
        {
            std::cerr << "patch failed" << std::endl;
            return 1;
        }

        std::cout << "after patch:" << std::endl;
        printBytes(buffer);

        // 4. patch後のbufferを別ファイルへ書き出す
        writeBinaryFile(outputPath, buffer);

        std::cout << "patch success. written to: "
                  << outputPath
                  << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

---

## 動作確認用に input.bin を作るコード

上のコードを試すには、まず `input.bin` が必要です。教材用に作るなら、次のコードで作れます。

```cpp
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>

void writeBinaryFile(const std::string& filePath,
                     const std::vector<uint8_t>& buffer)
{
    std::ofstream file(filePath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("failed to open file");
    }
    file.write(reinterpret_cast<const char*>(buffer.data()),
               static_cast<std::streamsize>(buffer.size()));
}

int main()
{
    // birthday = 20,24,4,25
    // owner    = 1,2,3,4
    // jockey   = 80,81,82,83
    std::vector<uint8_t> data = {
        20, 24, 4, 25,
        1, 2, 3, 4,
        80, 81, 82, 83
    };
    writeBinaryFile("input.bin", data);
    return 0;
}
```

---

## 実行結果イメージ

`input.bin` の中身がこれなら、

```
20 24 4 25 1 2 3 4 80 81 82 83
```

patch後はこうなります。

```
20 24 4 25 9 9 9 9 80 81 82 83
```

`owner` に相当する4バイトだけが差し替わります。

---

## ここで起きていること

重要なのはここです。

```cpp
MetaData* meta =
    reinterpret_cast<MetaData*>(buffer.data());
```

これは、`vector` の中身を構造体へコピーしているわけではありません。

```
std::vector<uint8_t> の内部メモリ
↓
同じメモリを MetaData として見る
```

だから、

```cpp
std::memcpy(meta->owner,
            patchData.data(),
            patchData.size());
```

と書くと、`buffer` の中身そのものが変わります。

---

## 注意点

この方式を使うなら、最低限これを守る必要があります。

1. `buffer.size() >= sizeof(MetaData)` であること
2. `patchData.size() == sizeof(meta->owner)` であること
3. `MetaData` の構造体レイアウトが `input.bin` の実際の並びと一致していること
4. `MetaData` に `std::string` や `std::vector` などを入れないこと
5. `reinterpret_cast` 後に `buffer.resize()` や `push_back()` をしないこと

特に最後は重要です。

```cpp
MetaData* meta =
    reinterpret_cast<MetaData*>(buffer.data());
```

このあとに、

```cpp
buffer.push_back(123);
buffer.resize(1000);
```

のようなことをすると、`vector` の内部メモリが再確保されて、`meta` が無効になる可能性があります。

構造体ビューを作った後は、`buffer` のサイズ変更は避けるのが安全です。

---

## まとめ

| ステップ | 処理 |
|---|---|
| 1 | `readBinaryFile` でバイナリファイルを `vector<uint8_t>` へ読み込む |
| 2 | `reinterpret_cast<MetaData*>(buffer.data())` で構造体ビューを作る |
| 3 | `memcpy(meta->owner, ...)` で対象フィールドだけ書き換える |
| 4 | `writeBinaryFile` でpatch済みのbufferをファイルへ書き戻す |

構造体ビューはコピーではなく、同じメモリを別の型として見ているだけです。そのため、ビューを作った後は `vector` のサイズ変更をしないことが安全の前提になります。
