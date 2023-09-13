using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Controls;
using SexyDotNet.Host;

namespace SexyDotNet
{
    public class CallStackViewItem
    {
        public string Module   { get; set; }
        public string Function { get; set; }
        public string Comments { get; set; }
        public Int32 CallDepth { get; set; }
        public IntPtr PC { get; set; }
    };

    public static class CallStackViewPopulator
    {
        static CallStackViewPopulator() { }

        private static void AppendStackFrame(Project project, ListView view, Int32 callDepth, IntPtr pc)
        {
            SourceFileSegment segment;
            project.GetLineSegment(out segment, pc);

            string module, function, comments;

            if (segment.Function.Name == null)
            {
                module = "<Unknown module>";
                function = "<Unknown function>";
                comments = "";
            }
            else
            {
                module = segment.Source.Name;
                function = segment.Function.Name;
                comments = "";
            }

            var s = new CallStackViewItem { Module = module, Function = function, Comments = comments, CallDepth = callDepth, PC = pc };
            var item = new ListViewItem { Content = s};            
            view.Items.Add(item);
        }

        public static void StackWalkToView(Project project, ListView view, CPUImage cpu)
        {
            view.Items.Clear();
            int callDepth = 0;
            for (IntPtr sf = cpu.SF, pc = cpu.PC; sf != IntPtr.Zero; pc = project.GetReturnAddress(sf), sf = project.GetCallerSF(sf))
            {
                AppendStackFrame(project, view, callDepth++, pc);
            }
        }
    }
}
