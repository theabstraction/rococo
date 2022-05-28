using System;

namespace Rococo.Carpenter
{
    internal class Program
    {
        static void Main(string[] args)
        {
            Config standardConfig = new Config();
            standardConfig.XCBaseFile = "tables\\tables.base.xc";
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
        }
    }
}
