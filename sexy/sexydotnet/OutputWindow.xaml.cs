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

namespace SexyDotNet
{
    /// <summary>
    /// Interaction logic for Window1.xaml
    /// </summary>
    public partial class OutputWindow : Window
    {
        public OutputWindow()
        {
            InitializeComponent();
        }

        public void AppendText(string text)
        {
            viewOutput.AppendText(text);
            viewOutput.AppendText("\r\n");
        }

        public void ClearText()
        {
            viewOutput.Document.Blocks.Clear();
        }
    }
}
