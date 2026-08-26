using System;
using System.Drawing;

namespace PictureBoxDragSelect
{
    public sealed class RegionSelectedEventArgs : EventArgs
    {
        public Rectangle Region { get; }

        public RegionSelectedEventArgs(Rectangle region)
        {
            Region = region;
        }
    }
}
