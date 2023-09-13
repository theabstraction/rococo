using System;
using System.Diagnostics;
using System.IO;

namespace Rococo.Carpenter
{
    /// <summary>
    /// This is an example table builder program using Carpenter as the engine.
    /// </summary>
    internal class Program
    {
        static void Main(string[] args)
        {
            string projectDir = "tables" + Path.DirectorySeparatorChar; // This will lead to files located in $(SolutionDir)tables\
            Environment.ProjectName = "rococo.carpenter.test";

            Environment.SetDeclarationsShortPath(projectDir + Environment.ProjectName + ".declarations.h");

            Config standardConfig = new Config(projectDir);

            string xlDir = projectDir + "XL" + Path.DirectorySeparatorChar;

            TableTarget[] targets =
            {
                new TableTarget { config = standardConfig, xlsxPath = xlDir + "periodic-table.xlsx", },
                new TableTarget { config = standardConfig, xlsxPath = xlDir + "localization-text-table.xlsx" },
                new TableTarget { config = standardConfig, xlsxPath = xlDir + "quotes-table.xlsx" },
                new TableTarget { config = standardConfig, xlsxPath = xlDir + "users.demo.xlsx" }
            };

            MapFullTablePathToResourceViaPrefixStripping mapFileNameToResource = new MapFullTablePathToResourceViaPrefixStripping(Environment.Solution + xlDir, projectDir);
            Carpenter.GenerateTables(targets, mapFileNameToResource);

            BuildTablesPackage();
        }
        static void BuildTablesPackage()
        {
            string batchFile = Environment.Solution + "packages" + Path.DirectorySeparatorChar + "gen.tables.package.bat";
            Console.WriteLine("Executing..." + batchFile);
            Process.Start("cmd.exe", "/C " + batchFile);
        }
    }
}
