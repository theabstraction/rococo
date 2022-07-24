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
        static void BuildTablesPackage()
        {
            string batchFile = Environment.Solution + "packages" + Path.DirectorySeparatorChar + "gen.tables.package.bat";
            Console.WriteLine("Executing..." + batchFile);
            Process.Start("cmd.exe", "/C " + batchFile);
        }
        static void Main(string[] args)
        {
            Config standardConfig = new Config();
            standardConfig.XCBaseFile = "tables\\tables.base.xc";
            standardConfig.SexyHeader = "tables\\rococo.tables.test.sxh";
            standardConfig.CPP_Root = "tables\\";

            // TODO -> have Carpenter generate all boiler plate, including the .sxh file and the package generating batch files.
            standardConfig.TypeDependentHeaders = new string[] { "tables.sxh.h" };
            // standardConfig.AdditionalSourceHeaders = new string[] { "tables.sxh.inl" };

            string prefixToStrip = Environment.Solution;
            MapFullTablePathToResourceViaPrefixStripping mapFileNameToResource = new MapFullTablePathToResourceViaPrefixStripping(prefixToStrip, "");

            TableTarget[] targets =
            {
                new TableTarget { xlsxPath = "tables\\periodic-table.xlsx", config = standardConfig },
                new TableTarget { xlsxPath = "tables\\localization-text-table.xlsx", config = standardConfig },
                new TableTarget { xlsxPath =  "tables\\quotes-table.xlsx", config = standardConfig },
                new TableTarget { xlsxPath =  "tables\\users.demo.xlsx", config = standardConfig }
            };

            Environment.SetDeclarationsShortPath("tables\\declarations.h");
            Environment.SetProjectName("rococo.carpenter.test");

            Carpenter.GenerateTables(targets, mapFileNameToResource);

            BuildTablesPackage();
        }
    }
}
