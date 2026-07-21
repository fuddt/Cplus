using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace CustomCombbox
{
    public partial class Form1 : Form
    {
        private readonly MultiSelectComboBox _multiSelectComboBox;
        private readonly Button _confirmButton;

        public Form1()
        {
            InitializeComponent();

            _multiSelectComboBox = new MultiSelectComboBox
            {
                Left = 20,
                Top = 20,
                Width = 250
            };

            // 選択候補を追加する
            _multiSelectComboBox.AddItems(new object[]
            {
                "東京",
                "大阪",
                "福岡",
                "北海道",
                "沖縄"
            });

            // コンボボックスの隣に確定ボタンを配置する
            _confirmButton = new Button
            {
                Left = _multiSelectComboBox.Right + 10,
                Top = _multiSelectComboBox.Top,
                Width = 80,
                Height = _multiSelectComboBox.Height,
                Text = "確定"
            };

            // 確定ボタンを押した時点で選択内容を確定させる
            _confirmButton.Click += ConfirmButton_Click;

            Controls.Add(_multiSelectComboBox);
            Controls.Add(_confirmButton);
        }

        private void ConfirmButton_Click(object sender, EventArgs e)
        {
            string[] selectedValues =
                _multiSelectComboBox.GetSelectedValues();

            // 確認用にフォームタイトルへ表示する
            Text = selectedValues.Length == 0
                ? "未選択"
                : string.Join(" / ", selectedValues);
        }
    }
}
