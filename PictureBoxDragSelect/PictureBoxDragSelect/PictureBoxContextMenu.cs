using System;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace PictureBoxDragSelect
{
    /// <summary>
    /// PictureBox の右クリックメニュー。項目の構成とイベント処理はすべてこのクラスに閉じる。
    /// 呼び出し側は PictureBox.ContextMenuStrip にインスタンスを渡すだけでよい。
    /// </summary>
    public class PictureBoxContextMenu : ContextMenuStrip
    {
        private readonly PictureBox _target;
        private readonly ToolStripMenuItem _openItem;
        private readonly ToolStripMenuItem _saveAsItem;
        private readonly ToolStripMenuItem _clearItem;

        public PictureBoxContextMenu(PictureBox target)
        {
            _target = target ?? throw new ArgumentNullException(nameof(target));

            _openItem = new ToolStripMenuItem("画像を開く...");
            _openItem.Click += OpenItem_Click;

            _saveAsItem = new ToolStripMenuItem("名前を付けて保存...");
            _saveAsItem.Click += SaveAsItem_Click;

            _clearItem = new ToolStripMenuItem("画像をクリア");
            _clearItem.Click += ClearItem_Click;

            Items.Add(_openItem);
            Items.Add(_saveAsItem);
            Items.Add(new ToolStripSeparator());
            Items.Add(_clearItem);

            Opening += PictureBoxContextMenu_Opening;
        }

        private void PictureBoxContextMenu_Opening(object sender, CancelEventArgs e)
        {
            bool hasImage = _target.Image != null;
            _saveAsItem.Enabled = hasImage;
            _clearItem.Enabled = hasImage;
        }

        private void OpenItem_Click(object sender, EventArgs e)
        {
            using (var dialog = new OpenFileDialog
            {
                Filter = "画像ファイル|*.jpg;*.jpeg;*.png;*.bmp;*.gif|すべてのファイル|*.*"
            })
            {
                if (dialog.ShowDialog() != DialogResult.OK)
                {
                    return;
                }

                Image previousImage = _target.Image;
                _target.Image = Image.FromFile(dialog.FileName);
                previousImage?.Dispose();
            }
        }

        private void SaveAsItem_Click(object sender, EventArgs e)
        {
            if (_target.Image == null)
            {
                return;
            }

            using (var dialog = new SaveFileDialog { Filter = "PNG画像|*.png" })
            {
                if (dialog.ShowDialog() != DialogResult.OK)
                {
                    return;
                }

                _target.Image.Save(dialog.FileName, ImageFormat.Png);
            }
        }

        private void ClearItem_Click(object sender, EventArgs e)
        {
            Image previousImage = _target.Image;
            _target.Image = null;
            previousImage?.Dispose();
        }
    }
}
