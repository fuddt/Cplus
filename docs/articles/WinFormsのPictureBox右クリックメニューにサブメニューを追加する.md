# PictureBoxの右クリックメニューにサブメニューを追加する

WinFormsで「右クリックすると出るメニュー」は`ContextMenuStrip`で作る。そのメニュー項目自身にさらに項目をぶら下げると、項目にカーソルを合わせたときに横へ展開するサブメニュー（カスケードメニュー）になる。

今回のゴールは、`PictureBox`を右クリックすると「画像を差し替え」「表示モード」「クリア」の3項目が出て、「表示モード」にカーソルを合わせると`Normal` / `StretchImage` / `Zoom` / `CenterImage`のサブメニューが展開し、選択中のモードにチェックが付く、という実用的な例。

## キー概念

- `ContextMenuStrip`はメニューそのものの入れ物。`PictureBox.ContextMenuStrip`プロパティに割り当てると、そのコントロール上での右クリックで自動的に開く。
- メニュー項目は`ToolStripMenuItem`。`ContextMenuStrip.Items.Add(...)`でトップレベルの項目を追加する。
- ある項目`ToolStripMenuItem`が自分の`DropDownItems`に別の`ToolStripMenuItem`を持つと、それだけでサブメニュー（入れ子のカスケードメニュー）になる。デザイナー上の特別な設定は不要で、階層を作るだけでよい。
- 「排他選択（ラジオボタン的な挙動）」はWinForms標準に専用コントロールがないため、Clickイベント内で兄弟項目の`Checked`を手動でfalseにし、選ばれた項目だけ`true`にする。

## 完全なコードサンプル

デザイナーを使わず、コードだけで完結する形にしてある。既存のフォームに`PictureBox`を1つ配置し、コンストラクタで`BuildContextMenu()`を呼べばそのまま動く。

```csharp
using System;
using System.Windows.Forms;

public partial class Form1 : Form
{
    private readonly PictureBox pictureBox1;

    public Form1()
    {
        InitializeComponent();

        pictureBox1 = new PictureBox
        {
            Dock = DockStyle.Fill,
            BorderStyle = BorderStyle.FixedSingle,
            SizeMode = PictureBoxSizeMode.Zoom
        };
        Controls.Add(pictureBox1);

        pictureBox1.ContextMenuStrip = BuildContextMenu();
    }

    private ContextMenuStrip BuildContextMenu()
    {
        var menu = new ContextMenuStrip();

        var replaceItem = new ToolStripMenuItem("画像を差し替え");
        replaceItem.Click += OnReplaceImage;

        var displayModeItem = new ToolStripMenuItem("表示モード");
        displayModeItem.DropDownItems.AddRange(BuildDisplayModeSubItems(displayModeItem));

        var clearItem = new ToolStripMenuItem("クリア");
        clearItem.Click += OnClearImage;

        menu.Items.Add(replaceItem);
        menu.Items.Add(displayModeItem);
        menu.Items.Add(clearItem);

        return menu;
    }

    private ToolStripItem[] BuildDisplayModeSubItems(ToolStripMenuItem parent)
    {
        var modes = new[]
        {
            PictureBoxSizeMode.Normal,
            PictureBoxSizeMode.StretchImage,
            PictureBoxSizeMode.Zoom,
            PictureBoxSizeMode.CenterImage
        };

        var items = new ToolStripItem[modes.Length];

        for (int i = 0; i < modes.Length; i++)
        {
            var mode = modes[i];
            var item = new ToolStripMenuItem(mode.ToString())
            {
                Checked = mode == pictureBox1.SizeMode
            };

            item.Click += (sender, e) =>
            {
                foreach (ToolStripMenuItem sibling in parent.DropDownItems)
                {
                    sibling.Checked = false;
                }

                item.Checked = true;
                pictureBox1.SizeMode = mode;
            };

            items[i] = item;
        }

        return items;
    }

    private void OnReplaceImage(object sender, EventArgs e)
    {
        using var dialog = new OpenFileDialog
        {
            Filter = "画像ファイル|*.png;*.jpg;*.jpeg;*.bmp;*.gif"
        };

        if (dialog.ShowDialog() == DialogResult.OK)
        {
            pictureBox1.Image = System.Drawing.Image.FromFile(dialog.FileName);
        }
    }

    private void OnClearImage(object sender, EventArgs e)
    {
        pictureBox1.Image = null;
    }
}
```

ポイントは`BuildDisplayModeSubItems`の中で、各サブメニュー項目のClickハンドラが「自分以外の兄弟項目のCheckedをfalseにしてから自分だけtrueにする」処理を持っている点。`parent.DropDownItems`を毎回ループしているので、モードが増えても項目を追加するだけで排他選択が壊れない。

## 補足: MVP構成に組み込む場合

このシリーズ（`WinFormsMVP授業_PathSetting自動生成`）の文脈で言うと、上のコードはViewの責務に閉じたUI組み立てであり、これ自体はPresenterに切り出す必要はない。ただし「表示モードが変更された」という事実をアプリケーションのロジックに反映したい場合（例えば設定を永続化する等）は、Click内で直接処理せず、Viewが`DisplayModeChanged`のようなイベントを発火し、Presenterがそれを購読して判断する形にすると、これまでの授業と同じ責務分離が保てる。
