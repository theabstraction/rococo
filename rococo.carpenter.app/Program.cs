using System;

namespace Rococo.Carpenter
{
    internal class Program
    {
        static void Main(string[] args)
        {
            string[] filenames =
            {
            //    "tables\\periodic-table.xlsx",
            //    "tables\\localization-text-table.xlsx",
            //    "tables\\quotes-table.xlsx",
                "tables\\users.demo.xlsx"
            };

            Carpenter.GenerateTables(filenames);
        }
    }
}
