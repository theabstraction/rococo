using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Excel = Microsoft.Office.Interop.Excel;

namespace sexcel
{
    public static class Sexport
    {
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
                    if (!IsCapital(c))
                    {
                        throw new Exception(string.Format("Table {0} -> the first character of the table name must be [A-Z]", GetId(table)));
                    }

                    sb.Append(c);
                    isFirst = false;
                    continue;
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
            { "string",         "IString"       },
            { "int",            "Int32"         },
            { "long",           "Int64"         },
            { "float",          "Float32"       },
            { "double",         "Float64"       },
            { "boolean",        "Bool"          },
            { "pingpath",       "IPingPath"     },
            { "mass",           "Kilograms"     },
            { "duration",       "Seconds"       },
            { "enum-material",  "MaterialIndex" }
        };
        public static Dictionary<string, string> MapXlTypeToSXYType
        {
            get
            {
                return mapXlTypeToSXYType;
            }
        }
        public static void AppendAllTypesToString(StringBuilder sb)
        {
            bool isFirst = true;
            foreach (var key in mapXlTypeToSXYType.Keys)
            {
                if (isFirst)
                {
                    isFirst = false;
                }
                else
                {
                    sb.Append(", ");
                }

                sb.Append(key);
            }
        }
        public static void AppendTableColumnTypeToSXYType(StringBuilder sb, string name, TableParser table)
        {
            string sxyType;
            if (!MapXlTypeToSXYType.TryGetValue(name, out sxyType))
            {
                var errBuilder = new StringBuilder(1024);
                errBuilder.AppendFormat("Table {0} -> Unknown type [{1}]. Must be one of ", GetId(table), name);
                AppendAllTypesToString(errBuilder);

                throw new Exception(errBuilder.ToString());
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
                    if (!IsLowerCase(c))
                    {
                        throw new Exception(string.Format("Table {0} -> the first character of the table name must be a lower case letter [a-z]", GetId(table)));
                    }

                    sb.Append(c);
                    isFirst = false;
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
        public static void Append_SXY_Struct_To_SXY(StringBuilder sb, TableParser table)
        {
            sb.Append("(struct ");
            AppendTableNameToLocalStructName(sb, table.Name, table);
            sb.Append("Row\n");

            int typeRowIndex = table.FirstRowIndex - 2;
            int nameRowIndex = table.FirstRowIndex - 1;

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
        public static bool HasEscapedCharacters(string s)
        {
            foreach (char c in s)
            {
                if (c <= 32)
                {
                    return true;
                }
                else if (c == '(')
                {
                    return true;
                }
                else if (c == ')')
                {
                    return true;
                }
                else
                {
                    // Everything is ok (in the most ok of all possible worlds)
                }
            }

            return false;
        }
        public static void AddEscapedText(StringBuilder sb, string text)
        {
            foreach (char c in text)
            {
                if (c == '\r')
                {
                    sb.Append("&r");
                }
                else if (c == '\n')
                {
                    sb.Append("&n");
                }
                else if (c <= 127)
                {
                    sb.Append(c);
                }
                else
                {
                    sb.Append('?');
                }
            }
        }

        /// <summary>
        /// Appens sequence of tokens separated by blank space, stripping space and raising case of characters after blank space
        /// Example 'find first cat' is appended as 'findFirstCat'
        /// </summary>
        /// <param name="sb">StringBuilder to which the characters are appended</param>
        /// <param name="entry">The string to parse</param>
        public static void AppendMakeSexyCase(StringBuilder sb, string entry)
        {
            bool shiftUp = false;

            foreach(char c in entry)
            {
                if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
                {
                    shiftUp = true;
                }
                else if (IsLowerCase(c) && shiftUp)
                {
                    shiftUp = false;
                    sb.Append((char) (c & ~32));
                }
                else
                {
                    sb.Append(c);
                }
            }
        }

        public static void Append_SXY_Table_To_SXY(StringBuilder sb, TableParser table)
        {
            sb.Append("(table ");
            AppendTableNameToLocalStructName(sb, table.Name, table);

            sb.Append("\n");

            Excel.Range item;

            for (int j = table.FirstRowIndex; j < table.FinalRowIndex; ++j)
            {
                sb.Append("\t(");

                for (int i = table.FirstColumnIndex; i < table.FinalColumnIndex; ++i)
                {
                    if (i != table.FirstColumnIndex)
                    {
                        sb.Append("\t");
                    }

                    string entry = table.GetCellText(i, j, out item);

                    Excel.Range typeEntry;
                    string type = table.GetFieldType(i, out typeEntry);
                    if (type == "string")
                    {
                        bool usesQuotes = HasEscapedCharacters(entry);
                        if (usesQuotes)
                        {
                            sb.Append("\"");
                        }

                        AddEscapedText(sb, entry);

                        if (usesQuotes)
                        {
                            sb.Append("\"");
                        }
                    }
                    else
                    {
                        AppendMakeSexyCase(sb, entry);
                    }
                }

                sb.Append(")\n");
            }

            sb.AppendFormat(")\n");
        }
    }
}
