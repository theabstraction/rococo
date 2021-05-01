using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Excel = Microsoft.Office.Interop.Excel;

namespace sexcel
{
    public class TableParser
    {
        private int rowIndex;
        private string name;
        private Excel.Worksheet sheet;
        private int finalColumnIndex;
        private int finalRowIndex;

        private void SetBorderToBlack(Excel.Range item)
        {
            item.Borders.Color = Excel.XlRgbColor.rgbBlack;
        }

        public string GetCellText(int i, int j, out Excel.Range item)
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
        public string GetFieldType(int i, out Excel.Range item)
        {
            return GetCellText(i, rowIndex + 1, out item);
        }

        public string GetFieldName(int i, out Excel.Range item)
        {
            return GetCellText(i, rowIndex + 2, out item);
        }
        public static string GetName(Excel.Range item)
        {
            return item.AddressLocal[true, true, Excel.XlReferenceStyle.xlA1].Replace('$', ' ');
        }

        public TableParser(int rowIndex, Excel.Worksheet sheet)
        {
            this.sheet = sheet;
            this.rowIndex = rowIndex;

            Excel.Range item;
            name = GetCellText(3, rowIndex, out item);

            if (name.Length == 0) throw new Exception(string.Format("Expecting a valid table name string at {0}", GetName(item)));

            item.Font.Bold = true;
            item.Font.Size = 16;

            Excel.Range row = sheet.Rows[rowIndex];
            row.RowHeight = 24;
            row.VerticalAlignment = Excel.XlVAlign.xlVAlignCenter;

            string typeIndex = GetCellText(2, rowIndex + 1, out item);
            if (typeIndex == "#types")
            {
                item.Font.Color = Excel.XlRgbColor.rgbGray;
            }
            else
            {
                throw new Exception(string.Format("Expecting [#types] at {0}", item.AddressLocal));
            }

            for (int i = 3; i < sheet.Columns.Count; ++i)
            {
                string typeString = GetCellText(i, rowIndex + 1, out item);
                if (typeString.Length > 0)
                {
                    finalColumnIndex = i;
                    item.Font.Color = Excel.XlRgbColor.rgbGray;
                }
                else
                {
                    break;
                }
            }

            for (int i = 2; i <= finalColumnIndex + 1; ++i)
            {
                GetCellText(i, rowIndex + 1, out item);
                item.Interior.Color = Excel.XlRgbColor.rgbLightYellow;
                item.Borders.Color = Excel.XlRgbColor.rgbLightYellow;

                GetCellText(i, rowIndex, out item);
                item.Interior.Color = Excel.XlRgbColor.rgbLightSteelBlue;
                item.Borders.Color = Excel.XlRgbColor.rgbLightSteelBlue;
            }

            string columnFields = GetCellText(2, rowIndex + 2, out item);
            if (columnFields == "#fields")
            {
                item.Font.Color = Excel.XlRgbColor.rgbGray;
            }
            else
            {
                throw new Exception(string.Format("Expecting [#fields] at {0}", GetName(item)));
            }

            // names
            for (int i = 3; i <= finalColumnIndex; ++i)
            {
                string fieldString = GetCellText(i, rowIndex + 2, out item);
                if (fieldString.Length > 0)
                {
                    item.Font.Color = Excel.XlRgbColor.rgbWhite;
                    item.Font.Size = 14;
                    item.Font.Bold = true;
                    item.Interior.Color = Excel.XlRgbColor.rgbBlue;
                }
                else
                {
                    throw new Exception(string.Format("Expecting data in field at {0}", GetName(item)));
                }
            }

            for (int j = rowIndex + 3; j <= sheet.UsedRange.Rows.Count; ++j)
            {
                bool atLeastOneEntry = false;

                for (int i = 3; i <= finalColumnIndex; ++i)
                {
                    string elementString = GetCellText(i, j, out item);
                    if (elementString.Length > 0)
                    {
                        atLeastOneEntry = true;
                        break;
                    }
                }

                if (atLeastOneEntry)
                {
                    finalRowIndex = j;

                    for (int i = 3; i <= finalColumnIndex; ++i)
                    {
                        string elementString = GetCellText(i, j, out item);
                        item.Font.Color = Excel.XlRgbColor.rgbBlack;
                        item.Font.Size = 11;
                        item.Interior.Color = Excel.XlRgbColor.rgbSilver;
                        item.Borders.Color = Excel.XlRgbColor.rgbBlack;
                    }
                }
                else
                {
                    break;
                }
            }

            for (int i = rowIndex + 1; i <= finalRowIndex; ++i)
            {
                GetCellText(2, i, out item);
                item.Interior.Color = Excel.XlRgbColor.rgbLightYellow;
                item.Borders.Color = Excel.XlRgbColor.rgbLightYellow;

                GetCellText(FinalColumnIndex + 1, i, out item);
                item.Interior.Color = Excel.XlRgbColor.rgbLightYellow;
                item.Borders.Color = Excel.XlRgbColor.rgbLightYellow;
            }

            for (int i = FirstColumnIndex - 1; i <= FinalColumnIndex + 1; ++i)
            {
                GetCellText(i, FinalRowIndex + 1, out item);
                item.Interior.Color = Excel.XlRgbColor.rgbLightYellow;
                item.Borders.Color = Excel.XlRgbColor.rgbLightYellow;
            }
        }

        public string Name
        {
            get { return name; }
        }
        public int FirstRowIndex
        {
            get { return rowIndex + 2; }
        }
        public int FinalRowIndex
        {
            get { return finalRowIndex; }
        }
        public int FirstColumnIndex
        {
            get { return 3; }
        }
        public int FinalColumnIndex
        {
            get { return finalColumnIndex; }
        }
    }
    public static class Tables
    {
        private static List<TableParser> tables = new List<TableParser>();

        public static void Add(TableParser table)
        {
            tables.Add(table);
        }
        public static void Clear()
        {
            tables.Clear();
        }

        public static IList<TableParser> Items
        {
            get
            {
                return tables;
            }
        }
    }
}
