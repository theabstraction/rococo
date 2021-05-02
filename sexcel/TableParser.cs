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
        private string name;
        private Excel.Worksheet sheet;
        private int finalColumnIndex;
        private int finalRowIndex;

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
            return GetCellText(i, 1, out item);
        }

        public string GetFieldName(int i, out Excel.Range item)
        {
            return GetCellText(i, 2, out item);
        }
        public static string GetName(Excel.Range item)
        {
            return item.AddressLocal[true, true, Excel.XlReferenceStyle.xlA1].Replace('$', ' ');
        }


        public string Name
        {
            get
            {
                return name;
            }
        }
        void SetFieldTypesFormat()
        {
            Excel.Range item;
            for (int i = 1; i < FinalColumnIndex; ++i)
            {
                GetCellText(i, 1, out item);
                item.Interior.Color = Excel.XlRgbColor.rgbWhiteSmoke;
                item.Borders.Color = Excel.XlRgbColor.rgbWhiteSmoke;
            }
        }

        void SetDataFormat()
        {
            Excel.Range item;
            for (int j = 3; j < finalRowIndex; ++j)
            {
                for (int i = 1; i < FinalColumnIndex; ++i)
                {
                    GetCellText(i,j, out item);
                    item.Interior.Color = (j % 2) == 1 ? Excel.XlRgbColor.rgbWhite : Excel.XlRgbColor.rgbSilver;
                    item.Font.Color = Excel.XlRgbColor.rgbBlack;
                    item.Borders.Color = Excel.XlRgbColor.rgbBlack;
                    item.Font.Name = "Consolas";
                }
            }
        }

        void SetFieldNamesFormat()
        {
            Excel.Range item;
            for (int i = 1; i < FinalColumnIndex; ++i)
            {
                GetCellText(i, 2, out item);
                item.Interior.Color = Excel.XlRgbColor.rgbDarkBlue;
                item.Font.Color = Excel.XlRgbColor.rgbWhite;
                item.Borders.Color = Excel.XlRgbColor.rgbBlack;
            }
        }

        public TableParser(string name, Excel.Worksheet sheet)
        {
            this.name = name;
            this.sheet = sheet;

            const int fieldTypeRow = 1;
            const int fieldNameRow = 2;

            Excel.Range item;
            for (int i = 1; i <= sheet.UsedRange.Columns.Count; ++i)
            {
                string typeString = GetCellText(i, fieldTypeRow, out item);
                if (typeString.Length > 0)
                {
                    finalColumnIndex = i + 1;
                    item.Font.Color = Excel.XlRgbColor.rgbGray;
                }
                else
                {
                    break;
                }
            }

            // names
            for (int i = 1; i < finalColumnIndex; ++i)
            {
                string fieldName = GetCellText(i, fieldNameRow, out item);
                if (fieldName.Length > 0)
                {
                }
                else
                {
                    throw new Exception(string.Format("Expecting field name in field at {0}", GetName(item)));
                }
            }

            for (int j = 3; j <= sheet.UsedRange.Rows.Count; ++j)
            {
                bool atLeastOneEntry = false;

                for (int i = 1; i < finalColumnIndex; ++i)
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
                    finalRowIndex = j + 1;
                }
                else
                {
                    break;
                }
            }

            SetFieldTypesFormat();
            SetDataFormat();
            SetFieldNamesFormat();
        }

        public int FirstRowIndex
        {
            get { return 3; }
        }
        public int FinalRowIndex
        {
            get { return finalRowIndex; }
        }
        public int FirstColumnIndex
        {
            get { return 1; }
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
