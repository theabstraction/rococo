using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Controls;
using SexyDotNet;
using SexyDotNet.Host;
using System.Windows.Media;
using System.Windows;

namespace SexyDotNet
{
    internal class SFItem
    {
        public string Name { get; set; }
        public string SFOffset { get; set; }
        public string Type { get; set; }
        public string Value { get; set; }
        public VariableKind Kind { get; set; }
        public IntPtr SF { get; set; }
        public IntPtr PC { get; set; }
        public Int32 CallDepth { get; set; }
    }

    public static class Strings
    {
        public static bool SplitHead(string s, out string head, out string tail)
        {
            for (int i = 1; i < s.Length; ++i)
            {
                if (s[i] == '.')
                {
                    head = s.Substring(0, i);
                    tail = s.Substring(i + 1);
                    return true;
                }
            }

            head = string.Empty;
            tail = string.Empty;

            return false;
        }

        public static bool SplitTail(string s, out string head, out string tail)
        {
            for (int i = s.Length-1; i >= 1; --i)
            {
                if (s[i] == '.')
                {
                    head = s.Substring(0, i);
                    tail = s.Substring(i + 1);
                    return true;
                }
            }

            head = string.Empty;
            tail = string.Empty;

            return false;
        }
    }

    internal static class StackFrameViewPopulator
    {
        static StackFrameViewPopulator()
        {

        }

        public static TreeViewItem GenerateItem(TreeView view, TreeViewItem parent, string name, string type, string value, IntPtr sf, IntPtr pc)
        {
            TreeViewItem x = new TreeViewItem();
            x.FontFamily = view.FontFamily;
            x.FontSize = view.FontSize;
           
            string head, tail;
            if (!Strings.SplitHead(name, out head, out tail))
            {
                parent.Items.Add(x);
                x.Tag = name;
                return x;
            }
            else
            {
                foreach (TreeViewItem y in parent.Items)
                {
                    if (string.Equals(y.Tag as string, head))
                    {
                        return GenerateItem(view, y, tail, type, value, sf, pc);
                    }
                }

                throw new Exception("Expected parent to contain the head: " + head);               
            }
        }

        public static void Add(TreeView view, TreeViewItem parent, string name, string type, string value, string address, IntPtr sf, IntPtr pc)
        {
            TreeViewItem i = GenerateItem(view, parent, name, type, value, sf, pc);

            string head, tail;
            if (!Strings.SplitTail(name, out head, out tail)) tail = name;

            UpdateItem(i, tail, type, value, address, sf, pc);    
        }

        private static void SetDeltaText(TextBox tb, string text)
        {
            if (string.Equals(tb.Text, text))
            {
                tb.Foreground = Brushes.Black;
            }
            else
            {
                tb.Foreground = Brushes.Red;
                tb.Text = text;
            }
        }

        private static void UpdatePanel(WrapPanel panel, string name, string type, string value, string address, IntPtr sf, IntPtr pc)
        {
            var a = panel.Children[0] as TextBox;
            var n = panel.Children[1] as TextBox;
            var t = panel.Children[2] as TextBox;
            var v = panel.Children[3] as TextBox;

            SetDeltaText(a, address);
            SetDeltaText(n, name);
            SetDeltaText(t, type);
            SetDeltaText(v, value);
        }

        private static void UpdateItem(TreeViewItem item, string name, string type, string value, string address, IntPtr sf, IntPtr pc)
        {
            if (item.Header == null)
            {
                item.Header = new WrapPanel();
                item.IsExpanded = true;              
            }

            WrapPanel panel = item.Header as WrapPanel;
            if (panel.Children.Count == 0)
            {
                panel.Children.Add(new TextBox { Width = 110, IsReadOnly = true });
                panel.Children.Add(new TextBox { Width = 150, IsReadOnly = true });
                panel.Children.Add(new TextBox { Width = 150, IsReadOnly = true });
                panel.Children.Add(new TextBox { Width = 240, TextAlignment = TextAlignment.Right, IsReadOnly = true });
            }

            UpdatePanel(panel, name, type, value, address, sf, pc);
        }

        private static bool IsForVariable(VariableDesc v, TreeViewItem i)
        {
            return string.Equals(v.Name, i.Tag as string);
        }

        private static bool UpdateSubitems(TreeView view, TreeViewItem item, string name, VariableDesc v, IntPtr sf, IntPtr pc)
        {
            string category = item.Tag as string;

            string head, tail;
            if (Strings.SplitHead(name, out head, out tail))
            {
                if (!string.Equals(category, head))
                {
                    return false;
                }
                else
                {
                    foreach (TreeViewItem subItem in item.Items)
                    {
                        if (UpdateSubitems(view, subItem, tail, v, sf, pc))
                        {
                            return true;
                        }
                    }

                    Add(view, item, tail, v.Type, v.Value, v.Address.ToString("X"), sf, pc);

                    return true;
                }
            }
            else
            {
                if (string.Equals(category, name))
                {
                    UpdateItem(item, name, v.Type, v.Value, v.Address.ToString("X"), sf, pc);
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        private static bool UpdateItemViewer(TreeView itemViewer, VariableDesc v, IntPtr sf, IntPtr pc)
        {
            foreach (TreeViewItem item in itemViewer.Items)
            {
                if (UpdateSubitems(itemViewer, item, v.Name, v, sf, pc))
                {
                    return true;
                }
            }

            return false;
        }

        public static void Add(ListView view, string sfOffset, string name, string type, string value, VariableKind kind, IntPtr sf, IntPtr pc, int callDepth)
        {
            var s = new SFItem { Name = name, SFOffset = sfOffset, Type = type, Value = value, Kind = kind, SF = sf, PC = pc, CallDepth = callDepth };
            var item = new ListViewItem { Content = s };

            switch (kind)
            {
                case VariableKind.Global:
                    item.Foreground = Brushes.DimGray;
                    break;
                case VariableKind.Input:
                    item.Foreground = Brushes.Black;
                    break;
                case VariableKind.Output:
                    item.Foreground = Brushes.Black;
                    break;
                case VariableKind.Local:
                    item.Foreground = Brushes.Black;
                    break;
                case VariableKind.Pseudo:
                    item.Foreground = Brushes.Red;
                    break;
            }

            view.Items.Add(item);
        }

        public static void OnItemSelected(Project project, ListView listView)
        {
            var item = listView.SelectedItem as ListViewItem;
            if (item != null)
            {
                var sfItem = item.Content as SFItem;
                if (sfItem != null && sfItem.Kind != VariableKind.Pseudo)
                {
                    OnItemSelected(project, sfItem, listView);
                }
            }
        }

        public static void OnItemSelected(Project project, ListView srcView, TreeView trgtTreeView)
        {
            var item = srcView.SelectedItem as ListViewItem;
            if (item != null)
            {
                var sfItem = item.Content as SFItem;
                if (sfItem != null && sfItem.Kind != VariableKind.Pseudo)
                {
                    OnItemSelected(project, sfItem, trgtTreeView);
                }
            }
        }

        private static SFItem FindItem(VariableDesc v, ListView view)
        {
            foreach (ListViewItem item in view.Items)
            {
                var sfItem = item.Content as SFItem;
                if (sfItem != null && sfItem.Kind != VariableKind.Pseudo)
                {
                    if (v.Name == sfItem.Name)
                    {
                        return sfItem;
                    }
                }
            }

            return null;
        }

        private static void OnItemSelected(Project project, SFItem item, ListView view)
        {
            if (item.SF != IntPtr.Zero)
            {
                var variables = project.GetElements(item.Name, item.CallDepth);
                foreach (var v in variables)
                {
                    if (FindItem(v, view) == null)
                    {
                        Add(view, v.SFOffset.ToString("X"), v.Name, v.Type, v.Value, v.Direction, item.SF, item.PC, item.CallDepth);
                    }
                }
            }            
        }

        unsafe private static IntPtr AddPointer(IntPtr x, int y)
        {
            return new IntPtr((void*)(((byte*)x.ToPointer()) + y)); 
        }

        private static void OnItemSelected(Project project, SFItem item, TreeView targetView)
        {
            string sfToItem;

            int result;
            if (int.TryParse(item.SFOffset, out result))
            {
                IntPtr ipResult = new IntPtr(result);
                sfToItem = AddPointer(item.SF, result).ToString("X");
            }
            else
            {
                sfToItem = string.Empty;
            }

            if (item.SF != IntPtr.Zero)
            {
                targetView.Items.Clear();

                var variables = project.GetElements(item.Name, item.CallDepth);

                TreeViewItem topLevel = new TreeViewItem();
                topLevel.Tag = item.Name;
                UpdateItem(topLevel, item.Name, "", "", sfToItem, item.SF, item.PC);
                    
                targetView.Items.Add(topLevel);

                foreach (var v in variables)
                {
                    string head, tail;
                    if (!Strings.SplitHead(v.Name, out head, out tail))
                    {
                        tail = v.Name;
                    }

                    Add(targetView, topLevel, tail, v.Type, v.Value, v.Address.ToString("X"), item.SF, item.PC);
                }
            }
        }

        private static void Populate(ListView view, TreeView itemView, Project project, Int32 callDepth)
        {
            view.Items.Clear();

            IntPtr SF = project.GetStackFrame(callDepth);
            IntPtr PC = project.GetPCAddress(callDepth);

            if (SF != IntPtr.Zero)
            {
                Add(view, "0", "SF", "Register", SF.ToString("X"), VariableKind.Global, SF, PC, callDepth);

                // StackFrame = [Output1]...[OutputN][Input1]...[InputN][OldSF.Ptr][ReturnAddress.Ptr]. REGISTER_SF points to the return address.

                int returnOffset = -IntPtr.Size;
                Add(view, returnOffset.ToString(), "Return Address", "PC address", project.GetReturnAddress(SF).ToString("X"), VariableKind.Global, IntPtr.Zero, IntPtr.Zero, callDepth);

                IntPtr callerSFAddress = project.GetCallerSF(SF);

                int oldSFOffset = -2*IntPtr.Size;
                Add(view, oldSFOffset.ToString(), "Caller's SF", "Stack address", callerSFAddress == IntPtr.Zero ? "0 (Execution Stub)" : callerSFAddress.ToString("X"), VariableKind.Global, IntPtr.Zero, IntPtr.Zero, callDepth);

                var variables = project.GetVariables(callDepth);
                foreach (var v in variables)
                {
                    Add(view, v.SFOffset.ToString(), v.Name, v.Type, v.Value, v.Direction, SF, PC, callDepth);
                    if (UpdateItemViewer(itemView, v, SF, PC))
                    {
                        var members = project.GetElements(v.Name, callDepth);
                        foreach(var member in members)
                        {
                            UpdateItemViewer(itemView, member, SF, PC);
                        }
                    }
                }
            }
            else
            {
                Add(view, string.Empty, string.Empty, "Register", "Execution Stub", VariableKind.Local, SF, PC, callDepth);
            }
        }

        public static void Populate(ListView callStackView, TreeView itemView, Project project, CPUImage cpu)
        {
            Populate(callStackView, itemView, project, 0);  
        }

        public static void Populate(ListView callStackView, TreeView itemView, Project project, CallStackViewItem item)
        {
            Populate(callStackView, itemView, project, item.CallDepth);  
        }
    }
}
