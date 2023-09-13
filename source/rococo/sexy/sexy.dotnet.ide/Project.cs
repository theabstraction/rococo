using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

using SexyDotNet.Host;
using System.Windows;

namespace SexyDotNet
{
    public class ProjectFile
    {
        public ProjectFile(string name)
        {
            Filename = name;
        }

        public string Filename { get; private set; }
        public SourceModule Module { get; internal set; }
    }

    static class XmlBuilder
    {
        public static void AddAttribute(XmlElement e, string name, string value)
        {
            var a = e.OwnerDocument.CreateAttribute(name);
            a.Value = value;
            e.Attributes.Append(a);
        }

        public static XmlElement AddChild(XmlElement parent, string name)
        {
            var e =  parent.OwnerDocument.CreateElement(name);
            parent.AppendChild(e);
            return e;
        }

        public static string GetNonNullAttribute(XmlNode n, string name)
        {
            if (n == null) throw new NullReferenceException("Node was null");
            XmlAttributeCollection attributes = n.Attributes;
            if (attributes == null) throw new NullReferenceException("No attributes on node");
            var a = attributes[name];
            if (a == null) throw new NullReferenceException("No attribute named " + name);
            string value = a.Value;
            if (value == null || value.Length == 0) throw new NullReferenceException("The attribute value of " + name + " was blank");
            return value;
        }
    }

    public delegate bool DelegateRouteSysMessages();

    public class Project
    {
        private bool isCompiled = false;

        public Dictionary<string, ProjectFile> Files { get; private set; }

        public event Action EvDirty;
        public event Action EvCompiling;
        public event Action EvCompiled;
        private DelegateRouteSysMessages OnRouteSysMessages;

        private SexyScriptLanguage scriptLanguage = null;

        public Project(DelegateRouteSysMessages _onRouteSysMessages)
        {
            OnRouteSysMessages = _onRouteSysMessages;
            Files = new Dictionary<string, ProjectFile>();
        }

        private bool RouteSysMessages()
        {
            return OnRouteSysMessages();
        }

        public void EnumerateRegisters(Action<int, long> callback)
        {
            if (scriptLanguage != null)
            {
                for (int i = 0; i < 256; ++i)
                {
                    callback(i, scriptLanguage.GetRegisterValue(i));
                }
            }
        }

        public void LoadFromDoc(XmlDocument doc)
        {
            Files.Clear();
            XmlNodeList fileNodes = doc.DocumentElement.SelectNodes("file");
            foreach (XmlNode n in fileNodes)
            {
                string path = XmlBuilder.GetNonNullAttribute(n, "path");
                var pf = new ProjectFile(path);
                Files.Add(path, pf);
            }

            PublishDirty();
        }
        
        private void PublishDirty()
        {
            if (EvDirty != null) EvDirty();
        }
        
        public void SaveToDoc(XmlDocument doc)
        {
            doc.LoadXml(@"<?xml version=""1.0"" encoding=""UTF-8""?><sexy/>");

            foreach (string key in Files.Keys)
            {
                var e = XmlBuilder.AddChild(doc.DocumentElement, "file");
                XmlBuilder.AddAttribute(e, "path", key);
            }
        }

        public ProjectFile AddFile(string filename)
        {
            ProjectFile pf;
            if (Files.TryGetValue(filename, out pf))
            {
                throw new Exception("File is already a member of the project: " + filename);
            }

            pf = new ProjectFile(filename);
            Files.Add(filename, pf);

            PublishDirty();

            return pf;
        }

        unsafe public void Compile()
        {
            try
            {
                scriptLanguage = new SexyScriptLanguage(Logger.Log, RouteSysMessages);
                scriptLanguage.EvTerminated += OnScriptLanguageTerminated;

                foreach (string key in Files.Keys)
                {
                    Files[key].Module = scriptLanguage.AddModule(key);
                }

                if (EvCompiling != null) EvCompiling();
                scriptLanguage.Compile();
                isCompiled = true;
                if (EvCompiled != null) EvCompiled();
            }
            catch(CompileError cex)
            {
                var errBox = new ExceptionWindow();
                errBox.SetException(cex);
                errBox.ShowDialog();

                scriptLanguage = null;
            }
            catch (Exception ex)
            {
                var errBox = new ExceptionWindow();
                errBox.SetException(ex);
                errBox.ShowDialog();

                scriptLanguage = null;
            }
        }

        public void RefreshDisassembler()
        {
            if (scriptLanguage != null) scriptLanguage.RefreshDisassembler();
        }

        private void OnScriptLanguageTerminated(int exitCode)
        {
            MessageBox.Show("Program terminated with code " + exitCode);
        }

        public void Execute()
        {
            if (scriptLanguage != null && isCompiled)
            {
                scriptLanguage.Execute();
            }
        }

        public string SourceCode
        {
            get
            {
                return scriptLanguage != null && scriptLanguage.SourceCode != null ? scriptLanguage.SourceCode : string.Empty;
            }
        }

        public string Filename
        {
            get
            {
                return scriptLanguage != null && scriptLanguage.Filename != null ? scriptLanguage.Filename : string.Empty;
            }
        }

        public SourceLocation SourceStart
        {
            get
            {
                return scriptLanguage != null ? scriptLanguage.Start : new SourceLocation(-1, -1);
            }
        }

        public SourceLocation SourceEnd
        {
            get
            {
                return scriptLanguage != null ? scriptLanguage.End : new SourceLocation(-1, -1);
            }
        }

        public bool StepInto()
        {
            if (scriptLanguage != null && isCompiled)
            {
                return scriptLanguage.StepInto();                
            }

            return false;
        }

        public bool StepOver()
        {
            if (scriptLanguage != null && isCompiled)
            {
                return scriptLanguage.StepOver();
            }

            return false;
        }

        public bool StepOut()
        {
            if (scriptLanguage != null && isCompiled)
            {
                return scriptLanguage.StepOut();
            }

            return false;
        }

        public void GetLineSegment(out SourceFileSegment segment, IntPtr pc)
        {
            if (scriptLanguage == null || scriptLanguage.SourceFileSegments == null) segment = new SourceFileSegment(0,IntPtr.Zero, null, new FunctionRef());
            else scriptLanguage.SourceFileSegments.TryGetValue(pc, out segment);            
        }

        public List<VariableDesc> GetVariables(Int32 callDepth)
        {
            if (scriptLanguage == null) return new List<VariableDesc>();
            return scriptLanguage.GetVariables(callDepth);
        }

        public List<VariableDesc> GetElements(string variableName, Int32 callDepth)
        {
            if (scriptLanguage == null) return new List<VariableDesc>();
            return scriptLanguage.GetElements(variableName, callDepth);
        }

        internal IntPtr GetReturnAddress(IntPtr sf)
        {
            if (scriptLanguage == null) return IntPtr.Zero;
            return scriptLanguage.GetReturnAddress(sf);
        }

        internal IntPtr GetCallerSF(IntPtr sf)
        {
            if (scriptLanguage == null) return IntPtr.Zero;
            return scriptLanguage.GetCallerSF(sf);
        }

        internal IntPtr GetStackFrame(int callDepth)
        {
            if (scriptLanguage == null) return IntPtr.Zero;
            return scriptLanguage.GetStackFrame(callDepth);
        }

        internal IntPtr GetPCAddress(int callDepth)
        {
            if (scriptLanguage == null) return IntPtr.Zero;
            return scriptLanguage.GetPCAddress(callDepth);
        }
    }
}
