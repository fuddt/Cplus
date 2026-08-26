using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace PictureBoxDragSelect
{
    public partial class Form1 : Form
    {
        // これ以下の移動量ならドラッグではなくクリックとみなす
        private const int ClickThreshold = 4;

        // クリック時に判定対象とする矩形の一辺の長さ
        private const int ClickRegionSize = 4;

        private readonly Button _openButton;
        private readonly CheckBox _enableContextMenuCheckBox;
        private readonly PictureBox _pictureBox;
        private readonly PictureBoxContextMenu _contextMenu;

        // ドラッグ中かどうか、開始位置、現在の選択範囲を保持する
        private bool _isDragging;
        private Point _dragStart;
        private Rectangle _dragRectangle;

        // ドラッグ選択・クリック選択のどちらでも、確定した領域をこのイベントで通知する
        public event EventHandler<RegionSelectedEventArgs> RegionSelected;

        public Form1()
        {
            InitializeComponent();

            _openButton = new Button
            {
                Left = 12,
                Top = 12,
                Width = 120,
                Text = "画像を開く..."
            };
            _openButton.Click += OpenButton_Click;

            _enableContextMenuCheckBox = new CheckBox
            {
                Left = _openButton.Right + 12,
                Top = 16,
                Width = 200,
                Text = "右クリックメニューを有効にする",
                Checked = true
            };

            _pictureBox = new PictureBox
            {
                Left = 12,
                Top = _openButton.Bottom + 8,
                Width = ClientSize.Width - 24,
                Height = ClientSize.Height - _openButton.Bottom - 20,
                Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Bottom,
                BorderStyle = BorderStyle.FixedSingle,
                SizeMode = PictureBoxSizeMode.Zoom,
                BackColor = Color.Black
            };
            _pictureBox.MouseDown += PictureBox_MouseDown;
            _pictureBox.MouseMove += PictureBox_MouseMove;
            _pictureBox.MouseUp += PictureBox_MouseUp;
            _pictureBox.Paint += PictureBox_Paint;

            // 右クリックメニューの中身は PictureBoxContextMenu に任せる。ここではインスタンスを持ち、
            // チェックボックスの状態を判定用デリゲートとして渡すだけで、メニューの中身は関知しない。
            _contextMenu = new PictureBoxContextMenu(_pictureBox, () => _enableContextMenuCheckBox.Checked);
            _pictureBox.ContextMenuStrip = _contextMenu;

            Controls.Add(_pictureBox);
            Controls.Add(_openButton);
            Controls.Add(_enableContextMenuCheckBox);

            // 動作確認用に、確定した選択範囲をウィンドウタイトルに表示する
            RegionSelected += (sender, args) => Text = $"選択範囲: {args.Region}";
        }

        private void OpenButton_Click(object sender, EventArgs e)
        {
            using (var dialog = new OpenFileDialog
            {
                Filter = "画像ファイル|*.jpg;*.jpeg;*.png;*.bmp;*.gif|すべてのファイル|*.*"
            })
            {
                if (dialog.ShowDialog(this) != DialogResult.OK)
                {
                    return;
                }

                Image previousImage = _pictureBox.Image;
                _pictureBox.Image = Image.FromFile(dialog.FileName);
                previousImage?.Dispose();
            }
        }

        private void PictureBox_MouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left)
            {
                return;
            }

            _isDragging = true;
            _dragStart = e.Location;
            _dragRectangle = Rectangle.Empty;
        }

        private void PictureBox_MouseMove(object sender, MouseEventArgs e)
        {
            if (!_isDragging)
            {
                return;
            }

            _dragRectangle = NormalizeRectangle(_dragStart, e.Location);
            _pictureBox.Invalidate();
        }

        private void PictureBox_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left)
            {
                return;
            }

            Rectangle selectedRegion = IsClick(_dragStart, e.Location)
                ? BuildClickRegion(e.Location)
                : NormalizeRectangle(_dragStart, e.Location);

            RegionSelected?.Invoke(this, new RegionSelectedEventArgs(selectedRegion));

            // マウスアップのタイミングで選択枠を消す
            _isDragging = false;
            _dragRectangle = Rectangle.Empty;
            _pictureBox.Invalidate();
        }

        private static bool IsClick(Point start, Point end)
        {
            return Math.Abs(end.X - start.X) <= ClickThreshold
                && Math.Abs(end.Y - start.Y) <= ClickThreshold;
        }

        private static Rectangle BuildClickRegion(Point clickPoint)
        {
            return new Rectangle(
                clickPoint.X - ClickRegionSize / 2,
                clickPoint.Y - ClickRegionSize / 2,
                ClickRegionSize,
                ClickRegionSize);
        }

        private void PictureBox_Paint(object sender, PaintEventArgs e)
        {
            if (!_isDragging || _dragRectangle.Width == 0 || _dragRectangle.Height == 0)
            {
                return;
            }

            using (var fillBrush = new SolidBrush(Color.FromArgb(60, Color.DodgerBlue)))
            using (var borderPen = new Pen(Color.DodgerBlue) { DashStyle = DashStyle.Dash })
            {
                e.Graphics.FillRectangle(fillBrush, _dragRectangle);
                e.Graphics.DrawRectangle(borderPen, _dragRectangle);
            }
        }

        private static Rectangle NormalizeRectangle(Point start, Point end)
        {
            int x = Math.Min(start.X, end.X);
            int y = Math.Min(start.Y, end.Y);
            int width = Math.Abs(start.X - end.X);
            int height = Math.Abs(start.Y - end.Y);
            return new Rectangle(x, y, width, height);
        }
    }
}
