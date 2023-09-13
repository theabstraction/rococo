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
using System.Windows.Forms;
using System.Drawing;

namespace Gisephone.Fonts.Generator
{
    /// <summary>
    /// Interaction logic for Window1.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private List<Font> fonts = new List<Font>();
        public IEnumerable<Font> AllFonts { get { return fonts;  } }
        public delegate void FontChanged(object sender, EventArgs args);
        public event FontChanged FontListChanged;
        public event Action Export;

        public MainWindow()
        {
            InitializeComponent();
        }

        public RenderFn Render
        {
            set
            {
                viewFonts.Render = value;
            }
        }

        private void OnFontsChanged()
        {
            fonts.Clear();
            foreach(Font f in listViewFonts.Items)
            {
                fonts.Add(f);
            }
            
            FontListChanged(this, new EventArgs());

            viewFonts.InvalidateVisual();
        }

        private void buttonAddFont_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var dlg = new FontDialog();
                dlg.AllowVectorFonts = true;
                dlg.AllowVerticalFonts = true;
                dlg.ShowEffects = true; 
                
                if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    Font selectedFont = dlg.Font;
                    listViewFonts.Items.Add(selectedFont);
                    OnFontsChanged();
                }        
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show(ex.Message, "Font Generator Error", MessageBoxButton.OK);
            }    
        }

        private void buttonRemoveFont_Click(object sender, RoutedEventArgs e)
        {
            int i = listViewFonts.SelectedIndex;
            if (i >= 0)
            {
                listViewFonts.Items.RemoveAt(i);
                OnFontsChanged();
            }            
        }

        private void buttonFontUp_Click(object sender, RoutedEventArgs e)
        {
            int i = listViewFonts.SelectedIndex;
            if (i >= 1)
            {
                var o1 = listViewFonts.Items[i];
                var o2 = listViewFonts.Items[i-1];
                listViewFonts.Items[i-1] = o1;
                listViewFonts.Items[i] = o2;
                listViewFonts.SelectedIndex = i - 1;
                OnFontsChanged();
            } 
        }

        private void buttonFontDown_Click(object sender, RoutedEventArgs e)
        {
            int i = listViewFonts.SelectedIndex;
            if (i >= 0 && listViewFonts.Items.Count > 1 && i < listViewFonts.Items.Count - 1)
            {
                var o1 = listViewFonts.Items[i];
                var o2 = listViewFonts.Items[i + 1];
                listViewFonts.Items[i + 1] = o1;
                listViewFonts.Items[i] = o2;
                listViewFonts.SelectedIndex = i + 1;
                OnFontsChanged();
            } 
        }

        private void buttonExport_Click(object sender, RoutedEventArgs e)
        {
            Export();
        }
    }
}
