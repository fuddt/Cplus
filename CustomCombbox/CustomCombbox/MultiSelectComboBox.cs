using System;
using System.ComponentModel;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

namespace CustomCombbox
{
    /// <summary>
    /// 複数選択可能なComboBox風コントロール
    /// </summary>
    public class MultiSelectComboBox : UserControl
    {
        // 選択内容を表示するボタン
        private readonly Button _displayButton;

        // ドロップダウンとして表示するコンテナ
        private readonly ToolStripDropDown _dropDown;

        // 複数選択用のリスト
        private readonly CheckedListBox _checkedListBox;

        // CheckedListBoxをToolStripDropDownに載せるためのホスト
        private readonly ToolStripControlHost _controlHost;

        /// <summary>
        /// 選択状態が変わったときに発火するイベント
        /// </summary>
        public event EventHandler SelectedItemsChanged;

        public MultiSelectComboBox()
        {
            // コントロール全体の初期サイズ
            Size = new Size(200, 25);

            // 表示部分となるボタンを生成
            _displayButton = new Button
            {
                Dock = DockStyle.Fill,
                Text = "選択してください",
                TextAlign = ContentAlignment.MiddleLeft,
                FlatStyle = FlatStyle.Standard
            };

            // ボタンを押したらドロップダウンを表示する
            _displayButton.Click += DisplayButton_Click;

            // 複数選択リストを生成
            _checkedListBox = new CheckedListBox
            {
                BorderStyle = BorderStyle.None,

                // 1回のクリックでチェック状態を変更できる
                CheckOnClick = true,

                // ドロップダウン部分の初期サイズ
                Size = new Size(200, 150)
            };

            // チェック状態が変わったら表示文字列を更新する
            _checkedListBox.ItemCheck += CheckedListBox_ItemCheck;

            // CheckedListBoxをToolStripに載せるためのホスト
            _controlHost = new ToolStripControlHost(_checkedListBox)
            {
                Margin = Padding.Empty,
                Padding = Padding.Empty,
                AutoSize = false,
                Size = _checkedListBox.Size
            };

            // ドロップダウン本体
            _dropDown = new ToolStripDropDown
            {
                Padding = Padding.Empty,
                AutoClose = true
            };

            _dropDown.Items.Add(_controlHost);

            // UserControlに表示ボタンを配置
            Controls.Add(_displayButton);
        }

        /// <summary>
        /// 選択肢を追加する
        /// </summary>
        public void AddItem(object item)
        {
            _checkedListBox.Items.Add(item);
        }

        /// <summary>
        /// 複数の選択肢をまとめて追加する
        /// </summary>
        public void AddItems(object[] items)
        {
            _checkedListBox.Items.AddRange(items);
        }

        /// <summary>
        /// 選択されている項目を取得する
        /// </summary>
        [Browsable(false)]
        public CheckedListBox.CheckedItemCollection CheckedItems
        {
            get { return _checkedListBox.CheckedItems; }
        }

        /// <summary>
        /// 選択されている文字列を配列として取得する
        /// </summary>
        public string[] GetSelectedValues()
        {
            return _checkedListBox.CheckedItems
                .Cast<object>()
                .Select(item => item.ToString())
                .ToArray();
        }

        /// <summary>
        /// ボタンクリック時にドロップダウンを表示する
        /// </summary>
        private void DisplayButton_Click(object sender, EventArgs e)
        {
            // ドロップダウンの横幅を自作コントロールに合わせる
            _checkedListBox.Width = Width;
            _controlHost.Width = Width;

            // ボタンの左下を基準に表示する
            _dropDown.Show(
                _displayButton,
                new Point(0, _displayButton.Height)
            );
        }

        /// <summary>
        /// チェック状態変更時の処理
        /// </summary>
        private void CheckedListBox_ItemCheck(
            object sender,
            ItemCheckEventArgs e)
        {
            // ItemCheckの時点ではCheckedItemsに新しい状態が未反映のため、
            // BeginInvokeで更新後に表示文字列を作り直す
            BeginInvoke(new Action(() =>
            {
                UpdateDisplayText();

                SelectedItemsChanged?.Invoke(
                    this,
                    EventArgs.Empty
                );
            }));
        }

        /// <summary>
        /// 選択項目をカンマ区切りでボタンに表示する
        /// </summary>
        private void UpdateDisplayText()
        {
            string[] selectedValues = GetSelectedValues();

            if (selectedValues.Length == 0)
            {
                _displayButton.Text = "選択してください";
                return;
            }

            _displayButton.Text = string.Join(", ", selectedValues);
        }
    }
}
