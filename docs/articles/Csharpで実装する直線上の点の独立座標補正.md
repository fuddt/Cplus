# C# で実装する直線上の点の独立座標補正

このページは、[直線上の点を独立に座標補正する実装設計](直線上の点を独立に座標補正する実装設計.md) の C# 実装版である。対象は .NET Framework 4.7.2 の WinForms プロジェクトでも使える C# とし、`record`、nullable reference types、`Math.Hypot` など新しいランタイムを前提にしない。

既存の C# プロジェクト（`PictureBoxDragSelect` / `CustomCombbox`）には、今回の直線・距離・交点を扱う共通クラスは存在しない。そのため、既存のフォームに直接書き足さず、下記の `Geometry` 名前空間のクラスとして分離して導入する。

## 使い方

各点について `CorrectOnePoint` を **個別に** 呼ぶ。点のリストを一括で最適化する API にはしていない。

```csharp
using Geometry;

PointD original = new PointD(0.0, 5.0);
Line originalBase = new Line(0.0, 1.0, 0.0);   // y = 0
Line movedBase = new Line(0.0, 1.0, -10.0);    // y = 10
RouteLine route = new RouteLine(original, new PointD(0.0, 1.0));

CorrectionResult result = CoordinateCorrector.CorrectOnePoint(
    original, originalBase, movedBase, route);

if (result.Status == CorrectionStatus.InvalidInput)
{
    // 入力値が壊れている。result.Point は利用しない。
    return;
}

// result.Point はルート上にあり、L1 からの距離誤差が最小。
// この例では (0, 5) と (0, 15) の両方が距離 5 だが、
// 元の点から近い (0, 5) が選ばれる。
```

`RouteLine` の `Origin` は元の点 `original` にする契約である。これにより、平行で完全一致候補がないときでも `original` を返せば、必ずルート上にあり、移動距離も最小になる。

## 完全な実装

以下を、たとえば `Geometry/CoordinateCorrector.cs` として追加できる。直線は `Ax + By + C = 0` の一般形で表すため、垂直線も安全に扱える。

```csharp
using System;
using System.Collections.Generic;

namespace Geometry
{
    public struct PointD
    {
        public PointD(double x, double y)
        {
            X = x;
            Y = y;
        }

        public double X { get; private set; }
        public double Y { get; private set; }
    }

    // Ax + By + C = 0
    public struct Line
    {
        public Line(double a, double b, double c)
        {
            A = a;
            B = b;
            C = c;
        }

        public double A { get; private set; }
        public double B { get; private set; }
        public double C { get; private set; }
    }

    // Q(t) = Origin + t * Direction, -∞ < t < +∞
    public struct RouteLine
    {
        public RouteLine(PointD origin, PointD direction)
        {
            Origin = origin;
            Direction = direction;
        }

        public PointD Origin { get; private set; }
        public PointD Direction { get; private set; }
    }

    public enum CorrectionStatus
    {
        // L1 から距離 d0 の交点を選んだ。
        Exact,

        // ルートと L1 が平行。ルート上の距離は一定なので Origin を返した。
        RouteParallel,

        // 直線係数、方向、座標のどれかが不正。
        InvalidInput
    }

    public struct CorrectionResult
    {
        public CorrectionResult(PointD point, double error, CorrectionStatus status)
        {
            Point = point;
            Error = error;
            Status = status;
        }

        public PointD Point { get; private set; }
        public double Error { get; private set; }
        public CorrectionStatus Status { get; private set; }
    }

    public static class CoordinateCorrector
    {
        // 座標の単位がピクセル程度であることを想定した開始値。
        // 座標スケールが大きい場合は、呼び出し側の単位をそろえるか相対判定へ拡張する。
        public const double Epsilon = 1e-9;

        public static CorrectionResult CorrectOnePoint(
            PointD original,
            Line originalBase,
            Line movedBase,
            RouteLine route)
        {
            if (!IsValidPoint(original) || !IsValidLine(originalBase) ||
                !IsValidLine(movedBase) || !IsValidPoint(route.Origin) ||
                !IsValidPoint(route.Direction) ||
                SquaredLength(route.Direction) <= Epsilon * Epsilon)
            {
                return new CorrectionResult(
                    new PointD(double.NaN, double.NaN),
                    double.NaN,
                    CorrectionStatus.InvalidInput);
            }

            // 呼び出し契約: route.Origin は original と同じ点。
            // 必要ならここで SquaredDistance(original, route.Origin) を検査してもよい。
            double d0 = DistanceToLine(original, originalBase);
            double movedNorm = LineNorm(movedBase);

            // L1 から距離 d0 の 2 本の平行線。
            Line plusOffset = new Line(
                movedBase.A, movedBase.B, movedBase.C - d0 * movedNorm);
            Line minusOffset = new Line(
                movedBase.A, movedBase.B, movedBase.C + d0 * movedNorm);

            var candidates = new List<PointD>();
            AddIntersectionIfAny(candidates, route, plusOffset);
            AddIntersectionIfAny(candidates, route, minusOffset);

            if (candidates.Count > 0)
            {
                // ここにある候補はすべて理論上 error=0。
                // 同率の第2優先として、元の点から最も近い候補を選ぶ。
                PointD best = candidates[0];
                for (int i = 1; i < candidates.Count; i++)
                {
                    if (SquaredDistance(candidates[i], original) <
                        SquaredDistance(best, original))
                    {
                        best = candidates[i];
                    }
                }

                double error = Math.Abs(DistanceToLine(best, movedBase) - d0);
                return new CorrectionResult(best, error, CorrectionStatus.Exact);
            }

            // 交点がない無限直線ルートは L1 と平行である。
            // その場合、L1 への距離はルート上のどこでも同じ。
            // route.Origin (= original) は移動距離 0 なので第2優先でも最良。
            double parallelError = Math.Abs(
                DistanceToLine(route.Origin, movedBase) - d0);
            return new CorrectionResult(
                route.Origin, parallelError, CorrectionStatus.RouteParallel);
        }

        public static double DistanceToLine(PointD point, Line line)
        {
            double numerator = line.A * point.X + line.B * point.Y + line.C;
            return Math.Abs(numerator) / LineNorm(line);
        }

        private static void AddIntersectionIfAny(
            List<PointD> candidates, RouteLine route, Line line)
        {
            PointD intersection;
            if (!TryIntersect(route, line, out intersection))
            {
                return;
            }

            // d0=0 のとき plusOffset と minusOffset は同じ直線になる。
            // 同じ点を 2 回候補にしない。
            foreach (PointD candidate in candidates)
            {
                if (SquaredDistance(candidate, intersection) <= Epsilon * Epsilon)
                {
                    return;
                }
            }

            candidates.Add(intersection);
        }

        private static bool TryIntersect(RouteLine route, Line line, out PointD intersection)
        {
            double denominator = line.A * route.Direction.X +
                                 line.B * route.Direction.Y;

            // 小さすぎる分母で割ると、ほぼ平行な場合に巨大座標になる。
            if (Math.Abs(denominator) <= Epsilon)
            {
                intersection = new PointD();
                return false;
            }

            double numerator = line.A * route.Origin.X +
                               line.B * route.Origin.Y + line.C;
            double t = -numerator / denominator;

            double x = route.Origin.X + t * route.Direction.X;
            double y = route.Origin.Y + t * route.Direction.Y;
            if (!IsFinite(x) || !IsFinite(y))
            {
                intersection = new PointD();
                return false;
            }

            intersection = new PointD(x, y);
            return true;
        }

        private static double LineNorm(Line line)
        {
            return Math.Sqrt(line.A * line.A + line.B * line.B);
        }

        private static double SquaredDistance(PointD left, PointD right)
        {
            double dx = left.X - right.X;
            double dy = left.Y - right.Y;
            return dx * dx + dy * dy;
        }

        private static double SquaredLength(PointD vector)
        {
            return vector.X * vector.X + vector.Y * vector.Y;
        }

        private static bool IsValidLine(Line line)
        {
            return IsFinite(line.A) && IsFinite(line.B) && IsFinite(line.C) &&
                   line.A * line.A + line.B * line.B > Epsilon * Epsilon;
        }

        private static bool IsValidPoint(PointD point)
        {
            return IsFinite(point.X) && IsFinite(point.Y);
        }

        // double.IsFinite は .NET Framework 4.7.2 にないため、自前で判定する。
        private static bool IsFinite(double value)
        {
            return !double.IsNaN(value) && !double.IsInfinity(value);
        }
    }
}
```

## テスト例

既存リポジトリには C# 用テストプロジェクトがないため、ここでは任意のテストフレームワークに移せる最小の `Debug.Assert` 例を示す。正式導入時は NUnit、xUnit、MSTest のいずれかのテストプロジェクトへ移す。

```csharp
using System;
using System.Diagnostics;
using Geometry;

public static class CoordinateCorrectorExamples
{
    private const double Epsilon = 1e-8;

    private static void AssertNear(double actual, double expected)
    {
        Debug.Assert(Math.Abs(actual - expected) <= Epsilon,
            "actual=" + actual + ", expected=" + expected);
    }

    public static void Run()
    {
        // ケース1: d0=5 を完全に保てる。
        PointD p1 = new PointD(0, 5);
        CorrectionResult case1 = CoordinateCorrector.CorrectOnePoint(
            p1,
            new Line(0, 1, 0),       // L0: y=0
            new Line(0, 1, -10),     // L1: y=10
            new RouteLine(p1, new PointD(0, 1)));
        AssertNear(case1.Error, 0);
        AssertNear(CoordinateCorrector.DistanceToLine(case1.Point,
            new Line(0, 1, -10)), 5);

        // ケース2: y=+5 と y=-5 の両方が候補。元の P=(0,5) を選ぶ。
        PointD p2 = new PointD(0, 5);
        CorrectionResult case2 = CoordinateCorrector.CorrectOnePoint(
            p2,
            new Line(0, 1, 0),       // L0: y=0
            new Line(0, 1, 0),       // L1: y=0
            new RouteLine(p2, new PointD(0, 1)));
        AssertNear(case2.Point.X, 0);
        AssertNear(case2.Point.Y, 5);
        AssertNear(case2.Error, 0);

        // ケース3: d0=0。ルートと新しい基準線の交点 (4,0) を選ぶ。
        PointD p3 = new PointD(2, 0);
        CorrectionResult case3 = CoordinateCorrector.CorrectOnePoint(
            p3,
            new Line(0, 1, 0),       // L0: y=0
            new Line(1, 0, -4),      // L1: x=4（垂直線）
            new RouteLine(p3, new PointD(1, 0)));
        AssertNear(case3.Point.X, 4);
        AssertNear(case3.Point.Y, 0);
        AssertNear(case3.Error, 0);

        // ケース4: ルートが L1 と平行。距離はどこでも同じなので P を返す。
        PointD p4 = new PointD(3, 5);
        CorrectionResult case4 = CoordinateCorrector.CorrectOnePoint(
            p4,
            new Line(0, 1, 0),       // L0: y=0、d0=5
            new Line(0, 1, -20),     // L1: y=20、距離は 15
            new RouteLine(p4, new PointD(1, 0)));
        Debug.Assert(case4.Status == CorrectionStatus.RouteParallel);
        AssertNear(case4.Point.X, 3);
        AssertNear(case4.Point.Y, 5);
        AssertNear(case4.Error, 10);
    }
}
```

## 線分・半直線にする場合

上の `RouteLine` は無限直線である。画面上の有限レールに制限するなら、`t` の許可範囲を持たせる。

```csharp
// 線分: Q(t) = Start + t * (End - Start), 0 <= t <= 1
// 半直線: Q(t) = Origin + t * Direction, t >= 0
```

`TryIntersect` で計算した `t` を返すようにし、範囲外の交点は候補から除く。その後に完全一致候補がなければ、端点と範囲内の交点候補を比較すればよい。一定刻みで線分を走査する必要はない。

## WinForms へ組み込むときの注意

`System.Drawing.Point` は `int` 座標であり、補正途中の小数を失う。計算中はこの実装の `PointD` を使い、描画直前にだけ丸める。

```csharp
Point screenPoint = new Point(
    (int)Math.Round(result.Point.X),
    (int)Math.Round(result.Point.Y));
```

丸める前の `PointD` をモデル値として保存しておけば、補正を何度繰り返しても整数丸めの誤差を積み上げにくい。
