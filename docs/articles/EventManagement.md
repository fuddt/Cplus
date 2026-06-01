# 毎回if文で探すな。『イベント会場の案内板』で考える高速検索設計

プログラムを書いていると、最初はついこういうコードを書きたくなる。

```cpp
if (currentMinute < 60)
{
    // パレード
}
else if (currentMinute < 150)
{
    // マジックショー
}
else if (currentMinute < 240)
{
    // キャラクターイベント
}
```

最初はこれで動く。

でもイベントが3個ではなく300個あったらどうだろう。

しかも毎秒大量アクセスされる。

例えば巨大テーマパークで、来場者が常にこう聞いてくるとする。

> 今10:47だけど何のイベントやってる？

1回なら問題ない。

でも数万回、数十万回呼ばれるなら話は別になる。

ここで考え方を変える。

「毎回探す」のをやめる。

先に答え表を作る。

## 発想転換

例えばイベントスケジュールがこうだったとする。

```text
09:00〜10:00 パレード
10:00〜11:30 マジックショー
11:30〜13:00 キャラクターイベント
```

これを毎回ifで探すのではなく、先に変換表を作る。

```text
時間:

0
1
2
3
4
5
6
7
8
9

↓

イベント番号:

0
0
0
1
1
1
2
2
2
2
```

すると検索はこうなる。

```cpp
uint32_t eventIndex =
    timeToEvent[currentMinute];
```

終わり。

比較もループも不要。

配列1回。

これが今回の核心。

## 最小実装（MVP）

まずイベント情報を表す構造体を作る。

```cpp
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

struct EventBlock
{
    std::string eventName;

    // タイムライン上の開始位置
    uint64_t timelineStart=0;

    // イベント開始時間
    uint64_t eventStartMinute=0;

    // イベント終了時間
    uint64_t eventEndMinute=0;
};

struct CurrentEvent
{
    std::string eventName;

    // イベント開始から何分経過したか
    uint64_t elapsedMinute=0;
};
```

次にスケジュール表を作る。

```cpp
struct ScheduleTable
{
    std::vector<EventBlock> items;

    // 添字:現在時間
    // 値:イベント番号
    std::vector<uint32_t> timeToEvent;

    bool addEvent(
        const std::string& name,
        uint64_t startMinute,
        uint64_t endMinute)
    {
        EventBlock item;

        item.eventName=name;
        item.eventStartMinute=startMinute;
        item.eventEndMinute=endMinute;

        item.timelineStart=
            timeToEvent.size();

        items.push_back(item);

        uint32_t eventIndex=
            static_cast<uint32_t>(items.size()-1);

        uint64_t length=
            endMinute-startMinute+1;

        for(uint64_t i=0;i<length;++i)
        {
            timeToEvent.push_back(eventIndex);
        }

        return true;
    }

    bool findCurrentEvent(
        uint64_t currentMinute,
        CurrentEvent& outEvent) const
    {
        if(currentMinute>=timeToEvent.size())
        {
            return false;
        }

        uint32_t eventIndex=
            timeToEvent[currentMinute];

        const EventBlock& event=
            items[eventIndex];

        outEvent.eventName=
            event.eventName;

        outEvent.elapsedMinute=
            currentMinute-
            event.timelineStart+
            event.eventStartMinute;

        return true;
    }
};
```

使う側。

```cpp
int main()
{
    ScheduleTable table;

    table.addEvent(
        "Parade",
        0,
        59);

    table.addEvent(
        "MagicShow",
        0,
        89);

    table.addEvent(
        "CharacterMeet",
        0,
        119);

    CurrentEvent result;

    if(table.findCurrentEvent(100,result))
    {
        std::cout
            << result.eventName
            << " elapsed="
            << result.elapsedMinute
            << std::endl;
    }
}
```

出力:

```text
MagicShow elapsed=40
```

## これ何が嬉しいの？

一見すると、わざわざ配列を作って遠回りしているように見える。

でも実際は逆。

最初に少しだけ準備して、後の検索コストをほぼゼロにしている。

処理回数が増えるほど効いてくる。

この考え方はかなり色々な場所で使われている。

* AI特徴量前計算
* 競馬AIの事前集計

共通点は同じ。

「毎回探すな」

「答えを先に作れ」

地味だけど、実務ではかなり強い考え方だったりする。

