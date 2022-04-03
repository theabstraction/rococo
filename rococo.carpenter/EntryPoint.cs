using System;
using System.IO;
using DocumentFormat.OpenXml.Packaging;

namespace Rococo.Carpenter
{
    public static class Environment
    {
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
        static private void PublishEachTable(string[] filenames)
        {
            foreach (var filename in filenames)
            {
                string fullPath = Environment.Solution + filename;

                if (!fullPath.EndsWith(".xlsx"))
                {
                    throw new Exception("Expecting " + filename + " to end with '.xlsx'");
                }

                if (!System.IO.File.Exists(fullPath))
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
                    throw new Exception("Error publishing " + filename, ex);
                }
            }
        }
        static public void GenerateTables(string[] filenames)
        {
            try
            {
                PublishEachTable(filenames);
                CPPCore.Commit();
            }
            catch(Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }
    }
}
