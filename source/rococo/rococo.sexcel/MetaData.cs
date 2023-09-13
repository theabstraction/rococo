using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Excel = Microsoft.Office.Interop.Excel;

namespace sexcel
{
    public class MetaDatum
    {
        public string Category { get; set; }
        public string Type { get; set; }
        public string Name { get; set; }
        public string Value { get; set; }
        public bool IsImmutable { get; set; }
    }

    public static class MetaData
    {
        static List<MetaDatum> items = new List<MetaDatum>();

        public static void Add(string category, string type, string name, string value, bool isImmutable)
        {
            items.Add(new MetaDatum() { Category = category, Type = type, Value = value, IsImmutable = isImmutable });
        }

        public static void Clear()
        {
            items.Clear();
        }

        public static IList<MetaDatum> All
        {
            get
            {
                return items;
            }
        }

        public static Excel.Range At(Excel.Range row, int index)
        {
            return row.Cells[index];
        }

        public static string TextAt(Excel.Range row, int index)
        {
            var text = row.Cells[index].Value;
            return text is string ? text : string.Empty;
        }

        public static readonly string attribute_const = "const ";

        public static readonly string sysFontName = "Consolas";

        public static void BuildFrom(Excel.Worksheet sheet)
        {
            string category = sheet.Name.Substring("Table".Length).Trim();
            if (category.Length == 0) category = "Default";

            bool isEven = true;

            foreach(Excel.Range row in sheet.UsedRange.Rows)
            {
                isEven = !isEven;

                string type = TextAt(row, 1);
                string name = TextAt(row, 2);
                string value = TextAt(row, 3);

                if (type.Length == 0 || name.Length == 0 || value.Length == 0)
                {
                    continue;
                }

                bool isImmutable = type.StartsWith(attribute_const);
                if (isImmutable)
                {
                    type = type.Substring(attribute_const.Length);
                }

                MetaData.Add(category, type, name, value, isImmutable);

                At(row, 1).Font.Name = sysFontName;
                At(row, 2).Font.Name = sysFontName;
                At(row, 3).Font.Name = sysFontName;

                At(row, 1).Font.Color = Excel.XlRgbColor.rgbBlue;
                At(row, 2).Font.Color = Excel.XlRgbColor.rgbBlack;
                At(row, 3).Font.Color = Excel.XlRgbColor.rgbBlack;

                var backColor = isEven ? Excel.XlRgbColor.rgbLightYellow : Excel.XlRgbColor.rgbLightGoldenrodYellow;

                At(row, 1).Interior.Color = backColor;
                At(row, 2).Interior.Color = backColor;
                At(row, 3).Interior.Color = backColor;
            }
        }

        public static string GetCellText(Excel.Worksheet sheet, int i, int j, out Excel.Range item)
        {
            item = null;

            if (j > sheet.UsedRange.Rows.Count)
            {
                return String.Empty;
            }

            Excel.Range row = sheet.Rows[j];

            if (i > row.Cells.Count)
            {
                return String.Empty;
            }

            item = row.Cells[i];
            object value = item.Value;

            if (value is string)
            {
                return value as string;
            }
            else
            {
                return String.Empty;
            }
        }
    }
}
