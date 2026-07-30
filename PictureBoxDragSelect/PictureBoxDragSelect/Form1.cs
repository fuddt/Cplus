using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace PictureBoxDragSelect
{
    public partial class Form1 : Form
    {
        private readonly Button _openButton;
        private readonly PictureBox _pictureBox;

        // ドラッグ中かどうか、開始位置、現在の選択範囲を保持する
        private bool _isDragging;
        private Point _dragStart;
        private Rectangle _dragRectangle;

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

            Controls.Add(_pictureBox);
            Controls.Add(_openButton);
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

            // マウスアップのタイミングで選択枠を消す
            _isDragging = false;
            _dragRectangle = Rectangle.Empty;
            _pictureBox.Invalidate();
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
