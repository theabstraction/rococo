using System;
using System.Collections.Generic;
using System.IO;
using DocumentFormat.OpenXml.Packaging;

namespace Rococo.Carpenter
{
    public static class Environment
    {
        private static string xcBase = null;
        public static string XCBase
        {
            get
            {
                if (xcBase != null)
                {
                    return xcBase;
                }

                xcBase = System.IO.File.ReadAllText(XCBaseFile);
                return xcBase;
            }
        }

        private static string xcBaseFile = null;

        public static string XCBaseFile
        {
            get
            {
                return xcBaseFile;

            }
            internal set
            {
                string candidate = Environment.Solution + value;
                if (!File.Exists(candidate))
                {
                    throw new Exception(string.Format("Cannot find XC file: " + candidate));
                }

                xcBaseFile = candidate;
            }
        }

        public static Config TargetConfig
        {
            get;set;
        }

        private static string GetSolutionDir()
        {
            string thePath = System.IO.Directory.GetCurrentDirectory();
            while (true)
            {
                string searchTarget = thePath + System.IO.Path.DirectorySeparatorChar + "rococo.sln";
                if (System.IO.File.Exists(searchTarget))
                {
                    return thePath + System.IO.Path.DirectorySeparatorChar;
                }

                var parentDir = System.IO.Directory.GetParent(thePath);
                if (parentDir == null)
                {
                    throw new DirectoryNotFoundException("Expecting to have recursed current directory and found rococo.sln");
                }

                thePath = parentDir.FullName;
            }
        }

        public static string PingPathToSysPath(string pingPath)
        {
            if (pingPath == null || pingPath.Length == 0)
            {
                throw new Exception("No ping path specified");
            }

            if (pingPath[0] != '!')
            {
                throw new Exception("Ping path did not begin with !: " + pingPath);
            }

            string content = Environment.Solution + "content\\";
            string sysPath = content + pingPath.Substring(1).Replace('/', System.IO.Path.DirectorySeparatorChar);
            return sysPath;
        }

        private static string solution = string.Empty;

        public static string Solution
        {
            get 
            { 
                if (solution == string.Empty)
                {
                    solution = GetSolutionDir();
                }
                return solution; 
            }
        }

        public static void SetDeclarationsShortPath(string shortPathName)
        {
            declarationShortPath = shortPathName;
        }

        private static string declarationShortPath = string.Empty;

        public static string DeclarationsHeader
        {
            get
            {
                return declarationShortPath.Replace("\\", "/");
            }
        }

        public static string DeclarationsFullPath
        { 
            get
            {
                if (declarationShortPath == string.Empty)
                {
                    throw new ArgumentException("Requires a call to void Environment.SetDeclarationsShortPath(string shortPathName)");
                }
                return Environment.Solution + declarationShortPath;
            }
        }
    }

    public class Config
    {
        public string XCBaseFile { get; set; }

        /// <summary>
        /// List of header files that require table types to be defined before the headers are included
        /// </summary>
        public IEnumerable<string> TypeDependentHeaders { get; set; }

        /// <summary>
        /// List of headers added to the .cpp file
        /// </summary>
        public IEnumerable<string> AdditionalSourceHeaders { get; set; }

        public string SexyHeader { get; set; }
    }

    public struct TableTarget
    {
        public string xlsxPath;
        public Config config;
    }

    public class Carpenter
    {
        static private void Publish(string tableFilename)
        {
            Console.Write("Exporting " + tableFilename + "...");

            using (Stream stream = File.Open(tableFilename, FileMode.Open))
            {
                using (var doc = SpreadsheetDocument.Open(stream, false))
                {
                    ExcelDoc xlDoc = new ExcelDoc(doc);
                    xlDoc.ParseSheets();
                }
            }
        }
        static private void PublishTable(TableTarget target)
        {
            Environment.XCBaseFile = target.config.XCBaseFile;
            Environment.TargetConfig = target.config;

            string fullPath = Environment.Solution + target.xlsxPath;

            if (!fullPath.EndsWith(".xlsx"))
            {
                throw new Exception("Expecting " + target.xlsxPath + " to end with '.xlsx'");
            }

            if (!File.Exists(fullPath))
            {
                throw new FileNotFoundException(fullPath);
            }

            CPPCore.ExcelSource = fullPath;

            try
            {
                Publish(fullPath);
            }
            catch(Exception ex)
            {
                throw new Exception("Error publishing " + fullPath, ex);
            }
        }
        static public void GenerateTables(IEnumerable<TableTarget> targets)
        {
            try
            {
                foreach (var target in targets)
                {
                    PublishTable(target);
                }

                CPPCore.Commit();
                SexyCore.Commit();
            }
            catch(Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }
    }
}
