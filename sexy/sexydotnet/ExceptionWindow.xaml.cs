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
    /// Interaction logic for ExceptionWindow.xaml
    /// </summary>
    public partial class ExceptionWindow : Window
    {
        public ExceptionWindow()
        {
            InitializeComponent();
        }

        public void SetException(Exception ex)
        {
            textMessage.Text = ex.Message;
            textType.Text = ex.GetType().FullName;
            textStackTrace.Text = Exceptions.Format(ex);
        }

        private void buttonCopy_Click(object sender, RoutedEventArgs e)
        {
            Clipboard.SetText(textStackTrace.Text);
        }
    }

    public static class Exceptions
    {
        public static string Format(Exception ex)
        {
            var sb = new StringBuilder(256);
            var elist = new Stack<Exception>();
            for (Exception i = ex; i != null; i = i.InnerException)
            {
                elist.Push(i);
            }

            foreach (Exception j in elist)
            {
                sb.AppendLine(ex.Message);
                sb.AppendLine("\r\n");
                sb.AppendLine(ex.StackTrace);
                sb.AppendLine("\r\n");
            }

            return sb.ToString();
        }
    }
}
