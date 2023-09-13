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

namespace Gisephone.Fonts.Generator
{
    public delegate void RenderFn(DrawingContext dc, Rect rect);
    /// <summary>
    /// Interaction logic for DeferredRenderView.xaml
    /// </summary>
    public partial class DeferredRenderView : UserControl
    {
        public DeferredRenderView()
        {
            InitializeComponent();
        }

        public RenderFn Render { get; set; }

        protected override void OnRender(DrawingContext dc)
        {
            Rect rect = new Rect(0, 0, ActualWidth, ActualHeight);            
            if (Render != null)
            {
                Render(dc, rect);
            }            
            else
            {
                dc.DrawRectangle(Background, null, rect);
            }            
        }
    }
}
