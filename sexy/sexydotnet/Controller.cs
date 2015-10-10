using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Win32;
using System.Xml;
using System.Windows.Controls;
using System.IO;
using System.Windows.Input;
using System.Windows;
using SexyDotNet.Properties;
using System.Diagnostics;
using System.Windows.Media;
using SexyDotNet.Host;
using System.Windows.Documents;
using System.Runtime.InteropServices;

namespace SexyDotNet
{
    public class Controller
    {
        private GuiViews views;
        private Project project = null;
        private ContextMenu contextMenu = null;
        private CPUImage cpu = new CPUImage();
        SourceViewPopulator sourceViewPopulator;
        AssemblerPopulator assemblyPopulator;
        DelegateRouteSysMessages routeSysMessage;

        public Controller(IGuiEvents events, GuiViews _views, DelegateRouteSysMessages _routeSysMessages)
        {
            routeSysMessage = _routeSysMessages;
            views = _views;
            events.EvFileNew += OnFileNew;
            events.EvFileExit += OnFileExit;
            events.EvFileClose += OnFileClose;
            events.EvFileSave += OnFileSave;
            events.EvFileOpen += OnFileOpen;
            events.EvProjectAddFile += OnProjectAddFile;
            events.EvProjectAddNewFile += OnProjectAddNewFile;
            events.EvDebugExecute += OnDebugExecute;
            events.EvDebugStepInto += OnStepInto;
            events.EvDebugStepOut += OnStepOut;
            events.EvDebugStepOver += OnStepOver;
            events.EvProjectCompile += () =>
            {
                if (project != null)
                {
                    SetEmphasis(null);
                    project.Compile();                    
                }
            };

            views.TreeProject.SelectedItemChanged += OnTreeView_SelectedItemChanged;
            views.StackFrameView.SelectionChanged += OnStackView_SelectedItemChanged;
            views.CallStackView.SelectionChanged += OnCallStackView_SelectionChanged;
            views.StackItemView.SelectedItemChanged += OnStackItemView_SelectionChanged;

            cpu.EvRegisterChanged += OnCPURegisterChanged;

            contextMenu = new ContextMenu();

            sourceViewPopulator = new SourceViewPopulator(views.SourceView, views.SourceScroller);
            assemblyPopulator = new AssemblerPopulator(views.EditorAssembly, views.AssemblyScroller);
        }

        void OnCallStackView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = views.CallStackView.SelectedItem as ListViewItem;
            if (item != null)
            {
                var s = item.Content as CallStackViewItem;
                if (s != null)
                {
                    StackFrameViewPopulator.Populate(views.StackFrameView, views.StackItemView, project, s);                   
                }
            }
        }

        public void UpdateDisassembly()
        {
            sourceViewPopulator.Populate(project.SourceCode, project.Filename, emphasis, project.SourceStart, project.SourceEnd);
        }

        public void UpdateRegisters()
        {
            project.EnumerateRegisters(cpu.UpdateRegister);            
        }

        void OnCPURegisterChanged(int registerIndex)
        {
            var item = views.RegisterList.Items[registerIndex] as ListViewItem;
            item.Content = new RegisterBinding
            {
                Name = cpu.GetRegisterName(registerIndex),
                Value = string.Format("0x{0:X}", cpu.Register(registerIndex).int64Value)
            };
        }

        void OnStackView_SelectedItemChanged(object sender, SelectionChangedEventArgs e)
        {
            StackFrameViewPopulator.OnItemSelected(project, views.StackFrameView, views.StackItemView);
        }

        void OnStackItemView_SelectionChanged(object sender,  RoutedPropertyChangedEventArgs<object> e)
        {
            var tv = sender as TreeView;
            if (tv != null && tv.SelectedItem != null)
            {
                var i = tv.SelectedItem as TreeViewItem;
                string name = i.Tag as string;
                if (name != null)
                {
                    
                }
            }
        }

        void OnTreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
 	        var tv = sender as TreeView;
            if (tv != null && tv.SelectedItem != null)
            {
                var i = tv.SelectedItem as TreeViewItem;
                ProjectFile pf = i != null ? i.Tag as ProjectFile : null;
                if (pf != null)
                {
                    string src = null;
                    try
                    {
                        src = File.ReadAllText(pf.Filename);
                    }
                    catch(Exception ex)
                    {
                        MessageBox.Show("Error opening: " + pf.Filename + "\n" + ex.Message);
                        return;
                    }
                    
                    sourceViewPopulator.Populate(src, pf.Filename, emphasis, new SourceLocation(-1, -1), new SourceLocation(-1, -1));
                    assemblyPopulator.Populate(project, pf, cpu);
                }
            }
        }

        static void OnTreeView_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            TreeViewItem i = sender as TreeViewItem;
            if (i != null)
            {
                var pf = i.Tag as ProjectFile;
                if (pf != null)
                {
                    var w = new CompileWindow();
                    w.SetText(pf.Filename);
                    w.ShowDialog();
                }
            }
        }

        string GetTitleRoot()
        {
            return Debugger.IsAttached ? "SexyDotNet IDE (debug mode)" : "SexyDotNet IDE";
        }

        bool TryFileClose()
        {
            if (project != null)
            {
                project.EvDirty -= OnProjectDirty;
                project = null;
            }

            views.Main.EnableProjectMenus(false);
            views.Main.Title = GetTitleRoot();
            views.TreeProject.Items.Clear();

            return true;
        }

        void OnProjectDirty()
        {
            ProjectViewPopulator.Populate(project, views.TreeProject, contextMenu, emphasis);
        }

        void OnUpdatedPC()
        {
            assemblyPopulator.Populate(project, cpu);
            StackFrameViewPopulator.Populate(views.StackFrameView, views.StackItemView, project, cpu);
            CallStackViewPopulator.StackWalkToView(project, views.CallStackView, cpu);
        }

        void OnStepInto()
        {
            if (project.StepInto())
            {
                UpdateDisassembly();                
            }

            UpdateRegisters();
            OnUpdatedPC();
        }

        void OnStepOver()
        {
            if (project.StepOver())
            {
                UpdateDisassembly();
            }

            UpdateRegisters();
            OnUpdatedPC();
        }

        void OnStepOut()
        {
            if (project.StepOut())
            {
                UpdateDisassembly();
            }

            UpdateRegisters();
            OnUpdatedPC();
        }

        bool TryFileSave(XmlDocument projectDoc)
        {
            if (project == null) return false;
            project.SaveToDoc(projectDoc);
            return true;
        }

        bool TryFileNew()
        {
            if (!TryFileClose()) return false;

            project = new Project(routeSysMessage);
            project.EvDirty += OnProjectDirty;
            project.EvCompiling += () => views.Main.EnableExecution(false);
            project.EvCompiled += () =>
               {
                   views.Main.EnableExecution(true);

                   project.RefreshDisassembler();
                   UpdateDisassembly();
               };
            
            views.Main.EnableProjectMenus(true);            

            return true;
        }

        void OnFileNew()
        {
            if (!TryFileNew()) return;
            views.Main.Title = GetTitleRoot() + " - New Project";
        }

        void OnFileOpen()
        {
            if (!TryFileClose()) return;

            var fd = new OpenFileDialog
            {
                AddExtension = true,
                CheckFileExists = true,
                CheckPathExists = true,
                DefaultExt = "xxx",
                Filter = "Sexy Projects|*.xxx"
            };

            if (fd.ShowDialog(views.Main) ?? false == true)
            {
                var projectDoc = new XmlDocument();
                projectDoc.Load(fd.FileName);

                if (!TryFileNew()) return;

                project.LoadFromDoc(projectDoc);

                views.Main.Title = GetTitleRoot() + " - " + fd.FileName;
            }
        }

        void OnFileSave()
        {
            var projectDoc = new XmlDocument();

            if (!TryFileSave(projectDoc))
            {
                return;
            }

            var fd = new SaveFileDialog
            {
                AddExtension = true,
                CheckPathExists = true,
                DefaultExt = "xxx",
                Filter = "Sexy Projects|*.xxx"
            };

            if (fd.ShowDialog(views.Main) ?? false == true)
            {                
                projectDoc.Save(fd.FileName);

                views.Main.Title = GetTitleRoot() + " - " + fd.SafeFileName;
            }
        }

        void OnFileClose()
        {
            TryFileClose();
        }

        void OnFileExit()
        {
            if (TryFileClose())
            {
                views.Main.Close();
            }
        }

        void OnProjectAddFile()
        {
            var fd = new OpenFileDialog
            {
                AddExtension = true,
                CheckFileExists = true,
                CheckPathExists = true,
                DefaultExt = "sxy",
                Filter = "Sexy Source|*.sxy",
                Title = "Add existing file to the Sexy project"
            };

            if (fd.ShowDialog(views.Main) ?? false == true)
            {
                project.AddFile(fd.FileName);
            }
        }

        void OnProjectAddNewFile()
        {
            var fd = new OpenFileDialog
            {
                AddExtension = true,   
                CheckFileExists = false,
                CheckPathExists = true,
                DefaultExt = "sxy",
                Filter = "Sexy Source|*.sxy",
                Title = "Create a new file add add to the Sexy project"
            };

            if (fd.ShowDialog(views.Main) ?? false == true)
            {
                if (File.Exists(fd.FileName))
                {
                    throw new Exception("A file already exists with this name: " + fd.FileName);
                }

                if (!fd.FileName.EndsWith("sxy"))
                {
                    throw new Exception("Filename does not end with sxy: " + fd.FileName);
                }

                DateTime now = DateTime.Now;
                string datestamp = now.ToString("HH:mm:ss - dd MMM yyyy");
                string src = string.Format("//Created {0}\r\n(using Sys.Types)\r\n\r\n", datestamp);

                File.WriteAllText(fd.FileName, src, Encoding.UTF7);

                project.AddFile(fd.FileName);
            }
        }       

        public void OnDebugExecute()
        {
            if (project != null)
            {
                project.Execute();
                UpdateRegisters();
            }
        }

        public void SetEmphasis(CompileError emphasis)
        {
            this.emphasis = emphasis;            
            OnProjectDirty();
        }

        private CompileError emphasis;
    }
}
