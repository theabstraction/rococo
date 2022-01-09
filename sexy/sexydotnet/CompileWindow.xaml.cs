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
using System.Windows.Shapes;
using SexyDotNet.Host;

namespace SexyDotNet
{
    /// <summary>
    /// Interaction logic for CompileWindow.xaml
    /// </summary>
    public partial class CompileWindow : Window
    {
        SourceLocation start;
        SourceLocation end;

        public CompileWindow()
        {
            InitializeComponent();
        }

        public void SetText(string text)
        {
            textBlock.Text = text;
        }

        public void SetSource(string sourceFile)
        {
            textSourceFilename.Text = sourceFile;
        }

        public void SetSourceLocation(SourceLocation start, SourceLocation end)
        {
            this.start = start;
            this.end = end;

            if (start.Line != 0 || start.Pos != 0)
            {
                textSourceSpecimen.Text = string.Format("From line {0} position {1} to line {2} position {3}", start.Line, start.Pos, end.Line, end.Pos);
            }
        }
    }
}
