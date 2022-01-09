using System;
using System.Collections.Generic;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Documents;

using SexyDotNet.Host;
using System.Windows;

namespace SexyDotNet
{
    public class SourceViewPopulator // This is too slow writing to a richtextbox, we need to rewrite using custom draw routines that allow dirty updates
    {
        private SexView view;
        private ScrollViewer scroller;

        public SourceViewPopulator(SexView view, ScrollViewer scroller)
        {
            this.view = view;
            this.scroller = scroller;
        }

        private static int ReadUntil(SourceLocation pos, string src)
        {
            int X = 0, Y = 0;

            int i;
            for (i = 0; i < src.Length; ++i)
            {
                if (Y > pos.Line) break;
                else if (pos.Line == Y && X == pos.Pos)
                {
                    break;
                }
                char c = src[i];
                switch (c)
                {
                    case '\r':
                        break;
                    case '\n':
                        Y++;
                        X = 0;
                        break;
                    default:
                        X++;
                        break;
                }
            }

            return i;
        }

        public void Populate(string src, string filename, CompileError emphasis, SourceLocation start, SourceLocation end)
        {
            view.SourceCode = src;

            Geometry g;

            if (emphasis != null)
            {
                int startChar = ReadUntil(emphasis.Start, src);
                int endChar = ReadUntil(emphasis.End, src);
                g = view.SetEmphasis(startChar, endChar, true);
            }
            else
            {
                int startChar = ReadUntil(start, src);
                int endChar = ReadUntil(end, src);
                g = view.SetEmphasis(startChar, endChar, false);
            }

            double yMin = scroller.VerticalOffset;
            double yMax = yMin + scroller.ViewportHeight;

            double xMin = scroller.HorizontalOffset;
            double xMax = xMin + scroller.ViewportWidth;

            if (g != null && g.Bounds.Top < yMin)
            {
                scroller.ScrollToVerticalOffset(g.Bounds.Top);
            }
            else if (g != null && g.Bounds.Bottom > yMax)
            {
                if (g.Bounds.Bottom > yMax + g.Bounds.Height)
                {
                    scroller.ScrollToVerticalOffset(g.Bounds.Top);
                }
                else
                {
                    scroller.ScrollToVerticalOffset(yMin + g.Bounds.Height);
                }                
            }
 
                               
            view.InvalidateVisual();
        }
    }
}
