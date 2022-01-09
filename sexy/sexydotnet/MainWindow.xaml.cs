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

namespace SexyDotNet
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window, IGuiEvents
    {
        public MainWindow()
        {
            InitializeComponent();

            guiViews = new GuiViews 
            {
                TreeProject = treeViewProject,
                EditorAssembly = assemblerView,
                SourceView = sourceView,
                RegisterList = listViewRegisters,
                StackFrameView = listViewStackFrame,
                CallStackView = listViewCallStack,
                StackItemView = treeViewStackItem,
                SourceScroller = sourceScroller,
                AssemblyScroller = assemblyScroller,
                Main = this
            };

            for (int i = 0; i < 256; ++i)
            {
                var item = new ListViewItem();
                item.Content = new RegisterBinding { Name = "D" + i, Value = "<undefined>" };
                listViewRegisters.Items.Add(item);
            }     
        }

        private GuiViews guiViews;

        public IGuiEvents Events { get { return this; } }
        public GuiViews Views { get { return guiViews; } }

        public event Action EvFileNew;
        public event Action EvFileOpen;
        public event Action EvFileClose;
        public event Action EvFileSave;
        public event Action EvFileExit;
        public event Action EvDebugExecute;
        public event Action EvDebugContinue;
        public event Action EvDebugStepInto;
        public event Action EvDebugStepOver;
        public event Action EvDebugStepOut;
        public event Action EvProjectAddFile;
        public event Action EvProjectAddNewFile;
        public event Action EvProjectCompile;

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (e.SystemKey == Key.F10)
            {
                if (EvDebugStepInto != null) EvDebugStepInto();
                e.Handled = true;
            }
        }

        private void MenuItem_FileOpen_Click(object sender, RoutedEventArgs e)
        {
            if (EvFileOpen != null) EvFileOpen();
        }

        private void MenuItem_FileNew_Click(object sender, RoutedEventArgs e)
        {
            if (EvFileNew != null) EvFileNew();
        }

        private void MenuItem_FileSave_Click(object sender, RoutedEventArgs e)
        {
            if (EvFileSave != null) EvFileSave();
        }

        private void MenuItem_FileClose_Click(object sender, RoutedEventArgs e)
        {
            if (EvFileClose != null) EvFileClose();
        }

        private void MenuItem_FileExit_Click(object sender, RoutedEventArgs e)
        {
            if (EvFileExit != null) EvFileExit();
        }

        private void MenuItem_DebugExecute_Click(object sender, RoutedEventArgs e)
        {
            if (EvDebugExecute != null) EvDebugExecute();
        }

        private void MenuItem_DebugContinue_Click(object sender, RoutedEventArgs e)
        {
            if (EvDebugContinue != null) EvDebugContinue();
        }

        private void OnStepOver(object sender, RoutedEventArgs e)
        {
            if (EvDebugStepOver != null) EvDebugStepOver();
        }

        private void OnStepInto(object sender, RoutedEventArgs e)
        {
            if (EvDebugStepInto != null) EvDebugStepInto();
        }

        private void OnStepOut(object sender, RoutedEventArgs e)
        {
            if (EvDebugStepOut != null) EvDebugStepOut();
        }

        private void MenuItem_ProjectAddFile_Click(object sender, RoutedEventArgs e)
        {
            if (EvProjectAddFile != null) EvProjectAddFile();
        }

        private void MenuItem_ProjectAddNewFile_Click(object sender, RoutedEventArgs e)
        {
            if (EvProjectAddNewFile != null) EvProjectAddNewFile();
        }

        private void MenuItem_ProjectCompile_Click(object sender, RoutedEventArgs e)
        {
            if (EvProjectCompile != null) EvProjectCompile();
        }

        private void SetEnableFlagsForMenusAndChildrenContainingSubstring(bool isEnabled, string substring, ItemCollection items)
        {
            if (items == null) return;

            foreach (MenuItem i in items)
            {
                if (i.Name.Contains(substring))
                {
                    i.IsEnabled = isEnabled;
                }

                SetEnableFlagsForMenusAndChildrenContainingSubstring(isEnabled, substring, i.Items);
            }
        }

        public void EnableProjectMenus(bool isEnabled)
        {
            string needsProject = "_NP";
            SetEnableFlagsForMenusAndChildrenContainingSubstring(isEnabled, needsProject, mainMenu.Items);
        }

        public void EnableExecution(bool isEnabled)
        {
            string needsExecution = "_NE";
            SetEnableFlagsForMenusAndChildrenContainingSubstring(isEnabled, needsExecution, mainMenu.Items);
        }
    }

    public class GuiViews
    {
        public MainWindow Main { get; internal set; }
        public SexView SourceView { get; internal set; }
        public AssemblerView EditorAssembly { get; internal set; }
        public TreeView TreeProject { get; internal set; }
        public ListView RegisterList { get; internal set; }
        public ListView StackFrameView { get; internal set; }
        public ListView CallStackView { get; internal set;  }
        public TreeView StackItemView { get; internal set; }
        public ScrollViewer SourceScroller { get; internal set; }
        public ScrollViewer AssemblyScroller { get; internal set; }
    }

    public interface IGuiEvents
    {
        event Action EvFileNew;
        event Action EvFileOpen;
        event Action EvFileClose;
        event Action EvFileSave;
        event Action EvFileExit;
        event Action EvDebugExecute;
        event Action EvDebugContinue;
        event Action EvDebugStepInto;
        event Action EvDebugStepOver;
        event Action EvDebugStepOut;
        event Action EvProjectAddFile;
        event Action EvProjectAddNewFile;
        event Action EvProjectCompile;
    }

    internal class RegisterBinding
    {
        public string Name { get; set; }
        public string Value { get; set; }
    }

    public class SexyCommands
    {
        public static readonly RoutedCommand StepInto = new RoutedCommand();
        public static readonly RoutedCommand StepOver = new RoutedCommand();
        public static readonly RoutedCommand StepOut = new RoutedCommand();
    }
}

