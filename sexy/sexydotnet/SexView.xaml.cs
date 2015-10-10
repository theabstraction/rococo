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
using System.Globalization;

namespace SexyDotNet
{
    /// <summary>
    /// Interaction logic for SexView.xaml
    /// </summary>
    public partial class SexView : UserControl
    {
        public SexView()
        {
            InitializeComponent();
            culture = CultureInfo.GetCultureInfo("en-us");
            ClipToBounds = true;
        }

        private CultureInfo culture;
        private Typeface typeface;
        private FormattedText text;

        private string rawText;
        private int rawStart;
        private int rawEnd;
        private bool rawError;

        private Point renderPos = new Point(20, 0);

        public string SourceCode
        {
            get
            {
                return rawText;
            }
            set
            {
                if (!object.ReferenceEquals(rawText, value))
                {
                    rawText = value;
                    rawStart = rawEnd = 0;

                    if (typeface == null)
                    {
                        typeface = new Typeface(this.FontFamily, this.FontStyle, this.FontWeight, this.FontStretch);
                    }

                    text = new FormattedText
                    (
                        value,
                        culture,
                        FlowDirection.LeftToRight,
                        typeface,
                        this.FontSize,
                        Brushes.Black
                    );

                    text.MaxTextWidth = 1920;
                    text.Trimming = TextTrimming.WordEllipsis;
                    text.MaxTextHeight = 10000000;

                    this.Height = text.Height + renderPos.Y;
                    this.Width = text.Width + renderPos.X;                    
                }
            }
        }

        protected override void OnRender(DrawingContext dc)
        {
            if (text != null)
            {
                if (rawStart < rawEnd)
                {
                    Geometry g = text.BuildHighlightGeometry(renderPos, rawStart, rawEnd - rawStart);
                    dc.DrawGeometry(rawError ? Brushes.Red : Brushes.Blue, null, g);
                }
                dc.DrawText(text, renderPos);
            }
        }

        public Geometry SetEmphasis(int startChar, int endChar, bool isError)
        {
            if (rawEnd > rawStart)
            {
                text.SetForegroundBrush(Brushes.Black, rawStart, rawEnd-rawStart);
            }

            if (isError)
            {
                text.SetForegroundBrush(Brushes.White, startChar, endChar - startChar);
            }
            else
            {
                text.SetForegroundBrush(Brushes.White, startChar, endChar - startChar);
            }          

            rawStart = startChar;
            rawEnd = endChar;
            rawError = isError;

            if (rawStart < rawEnd)
            {
                Geometry g = text.BuildHighlightGeometry(renderPos, rawStart, rawEnd - rawStart);
                return g;
            }
            else
            {
                return null;
            }
        }
    }
}
