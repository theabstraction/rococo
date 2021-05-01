using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Excel = Microsoft.Office.Interop.Excel;

namespace sexcel
{
    class TableParser
    {
        private int rowIndex;
        private string name;
        private Excel.Worksheet sheet;
        private int finalColumnIndex;

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

        public TableParser(int rowIndex, Excel.Worksheet sheet)
        {
            this.sheet = sheet;
            this.rowIndex = rowIndex;

            Excel.Range item;
            name = GetCellText(3, rowIndex, out item);

            if (name.Length == 0) return;

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
                return;
            }

            for(int i = 3; i < sheet.Columns.Count; ++i)
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

            string columnNames = GetCellText(2, rowIndex + 2, out item);
            if (typeIndex == "#types")
            {
                item.Font.Color = Excel.XlRgbColor.rgbGray;
            }
            else
            {
                return;
            }

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
            }

            for (int j = rowIndex + 3; j < sheet.UsedRange.Rows.Count; ++j)
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
                    for (int i = 3; i <= finalColumnIndex; ++i)
                    {
                        string elementString = GetCellText(i, j, out item);
                        item.Font.Color = Excel.XlRgbColor.rgbBlack;
                        item.Font.Size = 11;
                        item.Interior.Color = Excel.XlRgbColor.rgbLightGrey;
                        item.Borders.Color = Excel.XlRgbColor.rgbBlack;
                    }
                }
                else
                {
                    break;
                }
            }
        }

        public int RowIndex
        {
            get { return rowIndex; }
        }


        public string Name
        {
            get { return name; }
        }
    }
}
