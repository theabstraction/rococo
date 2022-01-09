using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace SexyDotNet
{
    /// <summary>
    /// Interaction logic for AssemblerView.xaml
    /// </summary>
    public partial class AssemblerView : UserControl
    {
        public AssemblerView()
        {
            InitializeComponent();
        }

        private FormattedText text;
        private int hiStart;
        private int hiEnd;

        public int HiStart
        {
            get { return hiStart;  }
            set { hiStart = value; }
        }
        public int HiEnd
        {
            get { return hiEnd;   } 
            set { hiEnd = value;  }
        }

        public Point RenderPos { get { return new Point(10, 0); } }

        public FormattedText Text
        {
            get
            {
                return text;
            }
            set
            {
                text = value;
            }
        }

        protected override void OnRender(DrawingContext dc)
        {
            if (text != null)
            {
                text.SetForegroundBrush(Brushes.Black);

                if (hiEnd > hiStart)
                {
                    Geometry g = text.BuildHighlightGeometry(RenderPos, hiStart, hiEnd - hiStart);
                    text.SetForegroundBrush(Brushes.White, hiStart, hiEnd - hiStart);
                    dc.DrawGeometry(Brushes.Blue, null, g);
                }

                dc.DrawText(text, RenderPos);                
            }
        }
    }
}
