using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml.Linq;
using Excel = Microsoft.Office.Interop.Excel;
using Office = Microsoft.Office.Core;
using Microsoft.Office.Tools.Excel;

namespace sexcel
{
    public partial class Sexel
    {
        public static void ShowMessage(string msg)
        {
            System.Windows.Forms.MessageBox.Show(msg, "Sexel - Sexy Excel");
        }

        public static void ShowMessage(Exception ex)
        {
            string msg = "Sexel threw an exception\r\n";

            Exception i = ex;
            while (i != null)
            {
                msg += "\r\nStackTrace:" + i.StackTrace + "\r\nMessage: " + i.Message;
                i = i.InnerException;
            }

            ShowMessage(msg);
        }

        public static bool IsSexyScript()
        {
            Excel.Worksheet sheet = Globals.Sexel.Application.ActiveSheet;
            if (sheet == null)
            {
                return false;
            }

            Excel.Range range = Globals.Sexel.Application.get_Range("A1", "Z20");
            Excel.Range sexyIndicator = range.Find("#sexel");

            return (sexyIndicator != null);
        }

        private static void ParseCommandString(int rowIndex, string text, Excel.Range cell)
        {
            Excel.Worksheet sheet = Globals.Sexel.Application.ActiveSheet;

            switch (text)
            {
                case "string":
                    cell.Font.Bold = true;
                    cell.Font.Color = Excel.XlRgbColor.rgbBlue;
                    break;
                case "#table":
                    try
                    {
                        var table = new TableParser(rowIndex, sheet);
                        cell.Font.Color = Excel.XlRgbColor.rgbGray;
                        Tables.Add(table);
                    }
                    catch (Exception ex)
                    {
                        Sexel.ShowMessage(ex.Message);
                    }
                    break;
            }
        }

        public static void SexUpWorksheet()
        {
            try
            {
                Tables.Clear();
                Excel.Worksheet sheet = Globals.Sexel.Application.ActiveSheet;
                if (sheet == null)
                {
                    return;
                }

                int nRows = sheet.UsedRange.Rows.Count;
                int nColumns = sheet.UsedRange.Rows.Count;

                if (nColumns < 2)
                {
                    return;
                }

                for (int i = 1; i <= nRows; ++i)
                {
                    Excel.Range command = sheet.Cells[i, 2];

                    object value = command.Value;
                    if (value is String)
                    {
                        ParseCommandString(i, value as string, command);
                    }
                }
            }
            catch (Exception ex)
            {
                ShowMessage(ex);
            }
        }

        private void Sexel_Startup(object sender, System.EventArgs e)
        {
        }

        private void Sexel_Shutdown(object sender, System.EventArgs e)
        {
        }

        private static string GetId(TableParser table)
        {
            Excel.Range item;
            table.GetCellText(3, table.FirstRowIndex, out item);
            return item.AddressLocal[true, true].Replace('$', ' ');
        }

        private static bool IsCapital(char c)
        {
            return (c >= 'A' && c <= 'Z');
        }

        private static bool IsLowerCase(char c)
        {
            return (c >= 'a' && c <= 'z');
        }

        private static bool IsDigit(char c)
        {
            return (c >= '0' && c <= '9');
        }

        private static bool IsAlphaNum(char c)
        {
            return IsCapital(c) || IsLowerCase(c) || IsDigit(c);
        }

        public static void AppendTableNameToLocalStructName(StringBuilder sb, string name, TableParser table)
        {
            bool isFirst = true;
            int currentLength = sb.Length;

            foreach (char c in name)
            {
                if (isFirst)
                {
                    if (IsCapital(c))
                    {
                        throw new Exception(string.Format("Table {0} -> the first character of the table name must be [A-Z]", GetId(table)));
                    }

                    sb.Append(c);
                    isFirst = false;
                }

                if (c == ' ')
                {
                    continue;
                }

                if (!IsAlphaNum(c))
                {
                    throw new Exception(string.Format("Table {0} -> illegal character in table name. All characters must be alphanumeric", GetId(table)));
                }

                sb.Append(c);
            }

            if (currentLength == sb.Length)
            {
                throw new Exception(string.Format("Table {0} -> blank table name. All characters must be alphanumeric", GetId(table)));
            }
        }

        private static readonly Dictionary<string, string> mapXlTypeToSXYType = new Dictionary<string, string>()
        {
            { "string",  "IString"  },
            { "int",     "Int32"    },
            { "long",    "Int64"    },
            { "float",   "Float32"  },
            { "double",  "Float64"  },
            { "boolean", "Bool"     }
        };

        public static Dictionary<string,string> MapXlTypeToSXYType
        {
            get
            {
                return mapXlTypeToSXYType;
            }
        }

        public static string AllTypes
        {
            get
            {
                return mapXlTypeToSXYType.Keys.ToString();
            }
        }

        public static void AppendTableColumnTypeToSXYType(StringBuilder sb, string name, TableParser table)
        {
            string sxyType;
            if (!MapXlTypeToSXYType.TryGetValue(name, out sxyType))
            {
                throw new Exception(string.Format("Table {0} -> Unknown type [{1}]. Must be one of [{2}]", GetId(table), name, AllTypes));
            }

            sb.Append(sxyType);
        }
        public static void AppendTableColumnNameToSXYIdentifier(StringBuilder sb, string name, TableParser table)
        {
            bool isFirst = true;
            int currentLength = sb.Length;

            foreach (char c in name)
            {
                if (isFirst)
                {
                    if (IsLowerCase(c))
                    {
                        throw new Exception(string.Format("Table {0} -> the first character of the table name must be a longer case letter [a-z]", GetId(table)));
                    }

                    sb.Append(c);
                    isFirst = false;
                }

                if (!IsAlphaNum(c))
                {
                    throw new Exception(string.Format("Table {0} -> illegal character in table name. All characters must be alphanumeric", GetId(table)));
                }

                sb.Append(c);
            }

            if (currentLength == sb.Length)
            {
                throw new Exception(string.Format("Table {0} -> blank table name. All characters must be alphanumeric", GetId(table)));
            }
        }

        public static void Append_SXY_Struct_To_SXY(StringBuilder sb, TableParser table)
        {
            sb.Append("(struct \n");
            AppendTableNameToLocalStructName(sb, table.Name, table);

            int typeRowIndex = table.FirstRowIndex + 1;
            int nameRowIndex = table.FirstRowIndex + 2;

            Excel.Range item;

            for (int i = table.FirstColumnIndex; i < table.FinalColumnIndex; ++i)
            {
                string fieldName = table.GetCellText(i, nameRowIndex, out item);
                string fieldType = table.GetCellText(i, typeRowIndex, out item);

                sb.Append("    (");
                AppendTableColumnTypeToSXYType(sb, fieldType, table);

                sb.Append(" ");
                AppendTableColumnNameToSXYIdentifier(sb, fieldName, table);
                sb.Append(")\n");
        }

            sb.AppendFormat(")\n");
        }
        public static void ExportToSXY()
        {
            if (Tables.Items.Count == 0)
            {
                Sexel.ShowMessage("There were no tables to export. Sex-up the worksheet and try again.");
            }

            StringBuilder sb = new StringBuilder(4096);

            foreach (TableParser table in Tables.Items)
            {
                Append_SXY_Struct_To_SXY(sb, table);
            }
        }

        #region VSTO generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InternalStartup()
        {
            this.Startup += new System.EventHandler(Sexel_Startup);
            this.Shutdown += new System.EventHandler(Sexel_Shutdown);
        }
        
        #endregion
    }
}
