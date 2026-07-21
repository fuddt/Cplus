# WinFormsにおけるMVPアーキテクチャ設計（PathSetting・Generate分割）

WinFormsで「見た目は1枚のウィンドウだが、内部的には複数クラスに分かれている」アプリを作りたい場合を考える。

採用するアーキテクチャはMVP（Model-View-Presenter）。画面を役割ごとに以下の3区画に分ける。

- **MainForm** — ウィンドウ本体。各区画を配置するだけのコンポジションルート
- **PathSettingForm** — ファイルパス入力（FileA〜E、ResultData1）
- **GenerateForm** — 出力先・出力形式の設定とレポート生成

## 要件

PathSettingの入力欄には相関検証がある。

- FileA と FileB は「データ件数が一致」していないといけない
- FileB と FileD は「カラム名が一致」していないといけない

検証は該当する2つの入力欄が埋まったタイミングで走らせ、エラーはポップアップではなく画面内のラベルに赤文字で小さく表示する。

GenerateForm側はPathSettingForm側の入力（ファイルパスなど）を使って生成処理を行う。

## 設計方針

- PathSettingForm/GenerateForm は **UserControl** として実装し、MainForm に埋め込む（クラス名は "Form" でも実体は UserControl）
- ファイル内容の相関検証（件数一致・カラム名一致）は **専用のValidatorサービスクラス**に切り出す。UIから完全分離することでテストしやすくする
- GenerateForm が PathSettingForm の入力を使う際は、**共有Modelを両Presenterが参照**する疎結合構成にする（MainPresenterのような仲介役は置かない）

## クラス図

```mermaid
classDiagram
    class MainForm {
        -PathSettingForm _pathSettingView
        -GenerateForm _generateView
        -PathSettingModel _sharedModel
        -PathSettingPresenter _pathSettingPresenter
        -GeneratePresenter _generatePresenter
        +MainForm()
        -WireUpPresenters()
    }

    class IPathSettingView {
        <<interface>>
        +string FileAPath
        +string FileBPath
        +string FileCPath
        +string FileDPath
        +string FileEPath
        +string ResultData1Path
        +ShowError(FieldPair pair, string message)
        +ClearError(FieldPair pair)
        +event PathFieldChanged
    }

    class PathSettingForm {
        <<UserControl>>
        -TextBox txtFileA
        -TextBox txtFileB
        -TextBox txtFileC
        -TextBox txtFileD
        -TextBox txtFileE
        -TextBox txtResultData1
        -Label lblErrorAB
        -Label lblErrorBD
        +ShowError(FieldPair pair, string message)
        +ClearError(FieldPair pair)
    }
    PathSettingForm ..|> IPathSettingView

    class PathSettingPresenter {
        -IPathSettingView _view
        -PathSettingModel _model
        -IFileValidationService _validator
        +PathSettingPresenter(view, model, validator)
        -OnPathFieldChanged(FieldPair pair)
        -RunValidation(FieldPair pair)
    }
    PathSettingPresenter --> IPathSettingView : uses
    PathSettingPresenter --> PathSettingModel : updates
    PathSettingPresenter --> IFileValidationService : uses

    class PathSettingModel {
        +string FileAPath
        +string FileBPath
        +string FileCPath
        +string FileDPath
        +string FileEPath
        +string ResultData1Path
    }

    class IFileValidationService {
        <<interface>>
        +ValidationResult ValidateRecordCount(string pathA, string pathB)
        +ValidationResult ValidateColumnNames(string pathB, string pathD)
    }

    class FileValidationService {
        +ValidationResult ValidateRecordCount(string pathA, string pathB)
        +ValidationResult ValidateColumnNames(string pathB, string pathD)
    }
    FileValidationService ..|> IFileValidationService
    FileValidationService --> ValidationResult : returns

    class ValidationResult {
        +bool IsValid
        +string ErrorMessage
    }

    class IGenerateView {
        <<interface>>
        +string OutputDir
        +IEnumerable~string~ CheckedFileFormats
        +SetGenerateEnabled(bool enabled)
        +event GenerateRequested
    }

    class GenerateForm {
        <<UserControl>>
        -TextBox txtOutputDir
        -CheckedListBox clbFileFormat
        -Button btnGenerateReport
        +SetGenerateEnabled(bool enabled)
    }
    GenerateForm ..|> IGenerateView

    class GeneratePresenter {
        -IGenerateView _view
        -PathSettingModel _sharedModel
        -IReportGenerationService _reportService
        +GeneratePresenter(view, sharedModel, reportService)
        -OnGenerateRequested()
    }
    GeneratePresenter --> IGenerateView : uses
    GeneratePresenter --> PathSettingModel : reads
    GeneratePresenter --> IReportGenerationService : uses

    class IReportGenerationService {
        <<interface>>
        +Generate(PathSettingModel input, GenerateOptions options)
    }

    class ReportGenerationService {
        +Generate(PathSettingModel input, GenerateOptions options)
    }
    ReportGenerationService ..|> IReportGenerationService

    MainForm *-- PathSettingForm : embeds
    MainForm *-- GenerateForm : embeds
    MainForm --> PathSettingPresenter : creates
    MainForm --> GeneratePresenter : creates
    MainForm --> PathSettingModel : creates (shared)
```

## 設計のポイント

### View/Presenter/Model の役割分担

- `PathSettingForm`/`GenerateForm`（View）はコントロール配置とイベント発火のみを担当し、業務ロジックは持たない
- Presenterがイベントを受けて処理し、`IPathSettingView`/`IGenerateView` インターフェース越しに表示を更新する。ViewをインターフェースにしておくことでPresenterは実UIに依存せず単体テストできる
- `PathSettingModel` は単純なデータの入れ物（POCO）。PathSettingPresenterが更新し、GeneratePresenterが読み取り専用で参照する共有オブジェクト

### MainFormはコンポジションルート

MainFormは3つのUserControlと共有Modelを生成し、各Presenterへ配線するだけの役割にとどめる。MainForm自体には業務ロジックを書かない。

### 「2つの入力欄が埋まったら検証」の実現方法

- `PathSettingForm` は各TextBoxの変更時（`Leave`や`TextChanged`）に `PathFieldChanged` イベントを発火し、どのフィールドが変わったかを伝える
- `PathSettingPresenter.OnPathFieldChanged` で、変更されたフィールドに関係する検証ペア（A-B、B-D）を判定し、両方が空でなければ該当の検証のみを実行する
  - 例：FileBが変更された場合、(A,B)ペアと(B,D)ペアの両方をチェック対象にする
- 検証NGなら `View.ShowError(pair, message)` で該当ラベルに赤文字表示、OKなら `View.ClearError(pair)` でクリアする
- 検証ルールが増えてきた場合は `IPathValidationRule` のようなルールリスト構造への拡張も可能だが、現状のペア数（2つ）では条件分岐で十分にシンプル

### ファイル内容の検証をUIから分離する理由

- `FileValidationService` はファイルI/Oと業務ルール（件数比較・カラム名比較）だけを持ち、WinFormsに依存しない
- これによりファイルを用意すれば単体テストが書ける（UIを起動しなくても検証ロジックを確認できる）
- 将来ルールが増えても（例：FileC/FileEの整合性チェックなど）このクラスにメソッドを追加するだけで済む

### GenerateFormとPathSettingFormの連携

- `GeneratePresenter` はコンストラクタで共有 `PathSettingModel` の参照を受け取る
- 「生成」ボタン押下時に `_sharedModel` の各パスを読み取り、`IReportGenerationService.Generate` に渡す
- PathSettingForm側の変更を都度Generate側が監視する必要はなく、ボタン押下時に最新のModelを読むだけでよい
