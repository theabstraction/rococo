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

    internal static class TableFilenames
    {
        internal static string[] filenames = 
        { 
            "tables\\periodic-table.xlsx" 
        };
    }

    internal class Program
    {
        static void Publish(string tableFilename)
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
        static void PublishEachTable(string[] filenames)
        {
            foreach (var filename in TableFilenames.filenames)
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
        static void Main(string[] args)
        {
            try
            {
                PublishEachTable(TableFilenames.filenames);
            }
            catch(Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }
    }
}
