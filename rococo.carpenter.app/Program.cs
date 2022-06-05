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

            // TODO -> have Carpenter generate all boiler plate, including the .sxh file and the package generating batch files.
            standardConfig.TypeDependentHeaders = new string[] { "tables.sxh.h" };
            standardConfig.AdditionalSourceHeaders = new string[] { "tables.sxh.inl" };

            TableTarget[] targets =
            {
                new TableTarget { xlsxPath = "tables\\periodic-table.xlsx", config = standardConfig },
            //   "tables\\localization-text-table.xlsx",
            //   "tables\\quotes-table.xlsx",
            //   "tables\\users.demo.xlsx"
            };

            Carpenter.GenerateTables(targets);

            BuildTablesPackage();
        }
    }
}
