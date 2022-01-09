using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Controls;
using SexyDotNet.Host;
using System.Windows.Documents;
using System.Windows.Media;
using System.Globalization;
using System.Windows;

namespace SexyDotNet
{
    internal struct Segment
    {
        public int Start { get; set; }
        public int End { get; set; }
    }
    internal class AssemblerPopulator
    {
        AssemblerView viewer;
        ScrollViewer scroller;
        CultureInfo culture;

        public AssemblerPopulator(AssemblerView viewer, ScrollViewer scroller)
        {
            this.viewer = viewer;
            this.scroller = scroller;
            culture = CultureInfo.GetCultureInfo("en-us");
        }

        internal static AssemblySection GetSection(SourceModule module, string sectionName)
        {
            if (module == null) return null;

            foreach (var section in module.Sections)
            {
                if (section.FunctionName == sectionName)
                {
                    return section;
                }
            }

            return null;
        }

        private Typeface typeface;

        public static void AppendDissassembly(StringBuilder sb, out int hiStart, out int hiEnd, AssemblySection section, SourceFileSegment segment, ref int lineCount)
        {
            sb.Append("Disassembly of ");
            sb.AppendLine(segment.Source.Name);
            sb.AppendLine();
            sb.AppendLine(section.FunctionName);
            sb.AppendLine();

            hiStart = 0;
            hiEnd = 0;

            foreach (var s in section.Segments)
            {
                if (section.FunctionName == segment.Function.Name && lineCount == segment.LineNumber)
                {
                    hiStart = sb.Length;
                    hiEnd = hiStart + s.Length;
                }

                sb.AppendLine(s);

                if (section.FunctionName == segment.Function.Name)
                {
                    lineCount++;
                }
            }
        }

        public static void AppendDissassembly(StringBuilder sb, out int hiStart, out int hiEnd, AssemblySection section, SourceFileSegment segment, SourceModule module, ref int lineCount)
        {
            sb.AppendLine();
            sb.AppendLine(section.FunctionName);
            sb.AppendLine();

            hiStart = 0;
            hiEnd = 0;

            foreach (var s in section.Segments)
            {
                if (section.FunctionName == segment.Function.Name && lineCount == segment.LineNumber)
                {
                    hiStart = sb.Length;
                    hiEnd = hiStart + s.Length;
                }

                sb.AppendLine(s);

                if (section.FunctionName == segment.Function.Name)
                {
                    lineCount++;
                }
            }
        }

        public static void AppendDissassembly(StringBuilder sb, out int hiStart, out int hiEnd, SourceModule module, SourceFileSegment segment)
        {
            hiStart = 0;
            hiEnd = 0;
            int lineCount = 0;

            if (module == null) return;

            sb.Append("Disassembly of ");
            sb.AppendLine(module.Name);
            sb.AppendLine();

            foreach (var section in module.Sections)
            {
                int sectionHiStart, sectionHiEnd;
                AppendDissassembly(sb, out sectionHiStart, out sectionHiEnd, section, segment, module, ref lineCount);
                if (sectionHiEnd > sectionHiStart)
                {
                    hiStart = sectionHiStart;
                    hiEnd = sectionHiEnd;
                }
            }
        }

        IntPtr lastFunction;

        private void BringScrollerIntoView(int hiStart, int hiEnd)
        {
            var g = text.BuildHighlightGeometry(viewer.RenderPos, hiStart, hiEnd - hiStart);

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
        }

        FormattedText text;

        internal void Populate(Project project, CPUImage cpu)
        {
            if (typeface == null)
            {
                typeface = new Typeface(viewer.FontFamily, viewer.FontStyle, viewer.FontWeight, viewer.FontStretch);
            }

            SourceFileSegment segment;
            project.GetLineSegment(out segment, cpu.PC);

            AssemblySection section = GetSection(segment.Source, segment.Function.Name);
            
            if (section == null || segment.Function.Handle == IntPtr.Zero)
            {
                string s = "No assembly code at this location: " + cpu.PC;
                text = new FormattedText(s, culture, FlowDirection.LeftToRight, typeface, viewer.FontSize, Brushes.Black, 1.0);

                viewer.HiStart = 0;
                viewer.HiEnd = 0;
                viewer.Text = text;

                lastFunction = IntPtr.Zero;
            }
            else if (segment.Function.Handle != lastFunction)
            {
                int hiStart, hiEnd;
                var sb = new StringBuilder(4096);
                int lineCount = 0;
                AppendDissassembly(sb, out hiStart, out hiEnd, section, segment, ref lineCount);

                text = new FormattedText(sb.ToString(), culture, FlowDirection.LeftToRight, typeface, viewer.FontSize, Brushes.Black, 1.0);
                viewer.Width = text.Width;
                viewer.Height = text.Height;
                
                viewer.Text = text;
                viewer.HiStart = hiStart;
                viewer.HiEnd = hiEnd;

                if (hiEnd > hiStart)
                {
                    BringScrollerIntoView(hiStart, hiEnd);
                }

                lastFunction = segment.Function.Handle;
            }           
            else
            {
                int hiStart, hiEnd;
                var sb = new StringBuilder(4096);
                int lineCount = 0;
                AppendDissassembly(sb, out hiStart, out hiEnd, section, segment, ref lineCount);

                text = new FormattedText(sb.ToString(), culture, FlowDirection.LeftToRight, typeface, viewer.FontSize, Brushes.Black, 1.0);
                if (text != null && hiEnd > hiStart)
                {
                    BringScrollerIntoView(hiStart, hiEnd);
                }

                viewer.Text = text;
                viewer.HiStart = hiStart;
                viewer.HiEnd = hiEnd;
            }

            viewer.InvalidateVisual();
        }

        public void Populate(Project project, ProjectFile pf, CPUImage cpu)
        {
            if (typeface == null)
            {
                typeface = new Typeface(viewer.FontFamily, viewer.FontStyle, viewer.FontWeight, viewer.FontStretch);
            }

            SourceFileSegment segment;
            project.GetLineSegment(out segment, cpu.PC);

            int hiStart, hiEnd;
            var sb = new StringBuilder(4096);

            AppendDissassembly(sb, out hiStart, out hiEnd, pf.Module, segment);

            text = new FormattedText(sb.ToString(), culture, FlowDirection.LeftToRight, typeface, viewer.FontSize, Brushes.Black, 1.0);
            text.SetForegroundBrush(Brushes.White, hiStart, hiEnd);

            viewer.Width = text.Width;
            viewer.Height = text.Height;

            viewer.Text = text;
            viewer.HiStart = hiStart;
            viewer.HiEnd = hiEnd;

            if (text != null && hiEnd > hiStart)
            {
                BringScrollerIntoView(hiStart, hiEnd);
            }

            viewer.InvalidateVisual();
        }
    }
}
