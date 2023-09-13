using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Excel = Microsoft.Office.Interop.Excel;

namespace sexcel
{
    public static class SexBuilder
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
        public static void AppendTableNameToLocalStructName(StringBuilder sb, TableParser table)
        {
            bool isFirst = true;
            int currentLength = sb.Length;

            foreach (char c in table.Name)
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
        public static void AppendTableColumnTypeToSXYType(StringBuilder sb, string fieldType, TableParser table)
        {
            string sxyType;
            if (!MapXlTypeToSXYType.TryGetValue(fieldType, out sxyType))
            {
                var errBuilder = new StringBuilder(1024);
                errBuilder.AppendFormat("Table {0} -> Unknown type [{1}]. Must be one of ", GetId(table), table.Name);
                AppendAllTypesToString(errBuilder);

                throw new Exception(errBuilder.ToString());
            }

            sb.Append(sxyType);
        }
        public static void AppendTableColumnNameToSXYIdentifier(StringBuilder sb, string fieldName, TableParser table)
        {
            bool isFirst = true;
            int currentLength = sb.Length;

            foreach (char c in fieldName)
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

        public static void AppendRowDef(StringBuilder sb, TableParser table)
        {
            sb.Append("(struct ");
            AppendTableNameToLocalStructName(sb, table);
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

        /// <summary>
        /// Appens sequence of tokens separated by blank space, stripping space and raising case of characters after blank space
        /// Example 'find first cat' is appended as 'findFirstCat'
        /// </summary>
        /// <param name="sb">StringBuilder to which the characters are appended</param>
        /// <param name="entry">The string to parse</param>
        public static void AppendMakeSexyCase(StringBuilder sb, string entry)
        {
            bool shiftUp = false;

            foreach (char c in entry)
            {
                if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
                {
                    shiftUp = true;
                }
                else if (IsLowerCase(c) && shiftUp)
                {
                    shiftUp = false;
                    sb.Append((char)(c & ~32));
                }
                else
                {
                    sb.Append(c);
                }
            }
        }

        public static void AppendNamespace(StringBuilder sb)
        {
            if (!Globals.Sexel.Application.ActiveWorkbook.Path.StartsWith(Globals.Sexel.ContentFolder))
            {
                throw new Exception("T");
            }
           // Globals.Sexel.Application.ActiveWorkbook.Path
        }
        private enum NAMESPACE_PARSE_STATE
        {
            ReadingFirstSubspaceChar,
            ReadingSubspaceTrailingCharacters,
        }
        public static void AppendPathStringAsNamespace(StringBuilder sb, string filename)
        {
            // Example \models.Skeleton\bones.sxl should map to Asset.Models.Skeleton

            int lastSlash = filename.LastIndexOf('\\');
            if (lastSlash == -1)
            {
                throw new Exception(string.Format("Cannot parse {0} into a namespace. No directory slash", filename));
            }

            sb.Append("Asset.");

            NAMESPACE_PARSE_STATE state = NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar;

            int len = sb.Length;

            for (int i = 0; i < lastSlash; i++)
            {
                char c = filename[i];
                if (c == '\\' || c == '/')
                {
                    switch(state)
                    {
                        case NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar:
                            throw new Exception(string.Format("Cannot parse namespace from {0}. Unexpected slash at pos {1}", filename, i));
                        case NAMESPACE_PARSE_STATE.ReadingSubspaceTrailingCharacters:
                            sb.Append('.');
                            state = NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar;
                            break;
                    }
                }
                else if (c == '.')
                {
                    switch (state)
                    {
                        case NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar:
                            throw new Exception(string.Format("Cannot parse namespace from {0}. Unexpected '.' at pos {1}", filename, i));
                        case NAMESPACE_PARSE_STATE.ReadingSubspaceTrailingCharacters:
                            sb.Append('.');
                            state = NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar;
                            break;
                    }
                }
                else if (IsCapital(c) || IsLowerCase(c))
                {
                    switch (state)
                    {
                        case NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar:
                            sb.Append((char)(c & ~32)); // That appended a capitalized character
                            state = NAMESPACE_PARSE_STATE.ReadingSubspaceTrailingCharacters;
                            break;
                        case NAMESPACE_PARSE_STATE.ReadingSubspaceTrailingCharacters:
                            sb.Append(c);
                            break;
                    }
                }
                else
                {
                    switch (state)
                    {
                        case NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar:
                            throw new Exception(string.Format("Cannot parse namespace from {0}. Unexpected character at pos {1}", filename, i));
                        case NAMESPACE_PARSE_STATE.ReadingSubspaceTrailingCharacters:
                            if (IsDigit(c))
                            {
                                sb.Append(c);
                            }
                            else
                            {
                                throw new Exception(string.Format("Cannot parse namespace from {0}. Unexpected character at pos {1}", filename, i));
                            }
                            break;
                    }
                }
            }

            if (len == sb.Length)
            {
                throw new Exception("No useful characters were parsed to generate a namespace");
            }
        }

        public static void AppendAsTypeName(StringBuilder sb, string shortFileName)
        {
            // alien.tables_01.sxl becomes IAlienTables01

            int len = sb.Length;

            NAMESPACE_PARSE_STATE state = NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar;

            int lastDot = shortFileName.LastIndexOf('.');
            string src = lastDot >= 0 ? shortFileName.Substring(0, lastDot) : shortFileName;

            foreach(char c in src)
            {
                if (c == '.')
                {
                    switch (state)
                    {
                        case NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar:
                            throw new Exception(string.Format("Cannot map filename to interface name. Unexpected dot in {0}", shortFileName));
                        case NAMESPACE_PARSE_STATE.ReadingSubspaceTrailingCharacters:
                            state = NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar;
                            break;
                    }
                }
                else if (IsCapital(c))
                {
                    sb.Append(c);
                    state = NAMESPACE_PARSE_STATE.ReadingSubspaceTrailingCharacters;
                }
                else if (IsLowerCase(c))
                {
                    switch(state)
                    {
                        case NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar:
                            sb.Append((char)(c & ~32)); // Capitalize
                            state = NAMESPACE_PARSE_STATE.ReadingSubspaceTrailingCharacters;
                            break;
                        case NAMESPACE_PARSE_STATE.ReadingSubspaceTrailingCharacters:
                            sb.Append(c);
                            break;
                    }
                }
                else if (IsDigit(c))
                {
                    switch (state)
                    {
                        case NAMESPACE_PARSE_STATE.ReadingFirstSubspaceChar:
                            sb.Append((char)(c & ~32)); // Capitalize
                            break;
                        case NAMESPACE_PARSE_STATE.ReadingSubspaceTrailingCharacters:
                            sb.Append(c);
                            break;
                    }
                }
                else
                {
                    // All other characters are ignored
                }
            }

            if (sb.Length - len < 3)
            {
                // Sanity check, don't let artists create IMu as an interface name, it's inpolite
                throw new Exception("Need at least 4 characters in an interface name. Use a more descriptive filename");
            }
        }
    }
    public class Sexport
    {
        private StringBuilder sb = new StringBuilder(4096);
        private string filename;
        public Sexport(string filename)
        {
            this.filename = filename;
        }
        public override string ToString()
        {
            return sb.ToString();
        }
        public string Filename
        {
            get
            {
                return Globals.Sexel.Application.ActiveWorkbook.FullName;
            }
        }

        void AppendNamespace()
        {
            if (!Filename.StartsWith(Globals.Sexel.ContentFolder))
            {
                throw new Exception(string.Format("Cannot infer namespace. The file path '{0}' was not a subdirectory of the content folder '{1}'", Filename, Globals.Sexel.ContentFolder));
            }

            string subpath = Filename.Substring(Globals.Sexel.ContentFolder.Length);
            SexBuilder.AppendPathStringAsNamespace(sb, subpath);
        }

        void AppendInterfaceShortName()
        {
            sb.Append("IAsset");
            AppendFilenameAsTypeName();
        }

        void AppendFilenameAsTypeName()
        {
            int lastSlash = Filename.LastIndexOf('\\');
            if (lastSlash == -1)
            {
                throw new Exception(string.Format("Could not infer interface name from {0}. No backslash", Filename));
            }

            SexBuilder.AppendAsTypeName(sb, Filename.Substring(lastSlash + 1));
        }

        public void AppendHeaders()
        {
            sb.AppendFormat("(' generator: Sexel 1.0.0.0 from: \"");
            SexBuilder.AddEscapedText(sb, Filename);
            sb.AppendFormat("\")\n\n");
        }

        public void AppendClass()
        {
            int i = 0;
            foreach (TableParser table in Tables.Items)
            {
                SexBuilder.AppendRowDef(sb, table);
            }

            foreach (TableParser table in Tables.Items)
            {
                sb.Append("(alias ");
                SexBuilder.AppendTableNameToLocalStructName(sb, table);
                sb.Append("Row ");

                AppendNamespace();
                sb.Append('.');
                SexBuilder.AppendTableNameToLocalStructName(sb, table);
                sb.Append("Row)\n");
            }

            sb.Append("\n(class CSXL0");
            AppendFilenameAsTypeName();
            sb.Append("Asset");

            sb.Append(" (defines ");
            AppendNamespace();
            sb.Append('.');
            AppendInterfaceShortName();
            sb.Append(")\n");

            i = 1;

            foreach(TableParser table in Tables.Items)
            {
                sb.Append("\t(array<");
                SexBuilder.AppendAsTypeName(sb, table.Name);
                sb.AppendFormat("Row> table{0}", i++);
                sb.Append(")\n");
            }

            sb.Append(")\n\n");

            i = 1;

            foreach (TableParser table in Tables.Items)
            {
                sb.Append("(method CSXL0");
                AppendFilenameAsTypeName();
                sb.Append("Asset.");
                SexBuilder.AppendAsTypeName(sb, table.Name);
                sb.Append(" -> (array<");
                SexBuilder.AppendAsTypeName(sb, table.Name);
                sb.AppendFormat("Row> table):\n\t(table = this.table{0})\n)\n", i++);
            }

            sb.Append("\n(method CSXL0.");
            AppendFilenameAsTypeName();
            sb.Append("Asset.Construct :\n");

            i = 1;
            foreach (TableParser table in Tables.Items)
            {
                sb.AppendFormat("\t(ConstructFromAppendix table{0} \"table{1}\")\n", i, i++);
            }

            sb.Append(")\n\n");

            i = 1;
            foreach (TableParser table in Tables.Items)
            {
                sb.AppendFormat("(factory ");
                AppendNamespace();
                sb.Append(".New");
                AppendFilenameAsTypeName();
                sb.Append(" ");
                AppendNamespace();
                sb.Append(".IAsset");
                AppendFilenameAsTypeName();
                sb.Append(": (construct CSXL0");
                AppendFilenameAsTypeName();
                sb.Append("))\n");
            }

            sb.Append("\n");

            i = 1;
            foreach (TableParser table in Tables.Items)
            {
                sb.AppendFormat("(' table{0} ", i++);
                SexBuilder.AppendTableNameToLocalStructName(sb, table);

                sb.Append("\n");

                Excel.Range item;

                for (int j = table.FirstRowIndex; j < table.FinalRowIndex; ++j)
                {
                    sb.Append("\t(");

                    for (int k = table.FirstColumnIndex; k < table.FinalColumnIndex; ++k)
                    {
                        if (k != table.FirstColumnIndex)
                        {
                            sb.Append("\t");
                        }

                        string entry = table.GetCellText(k, j, out item);

                        Excel.Range typeEntry;
                        string type = table.GetFieldType(k, out typeEntry);
                        if (type == "string")
                        {
                            bool usesQuotes = SexBuilder.HasEscapedCharacters(entry);
                            if (usesQuotes)
                            {
                                sb.Append("\"");
                            }

                            SexBuilder.AddEscapedText(sb, entry);

                            if (usesQuotes)
                            {
                                sb.Append("\"");
                            }
                        }
                        else
                        {
                            SexBuilder.AppendMakeSexyCase(sb, entry);
                        }
                    }

                    sb.Append(")\n");
                }

                sb.AppendFormat(")\n");
            }
        }
    }
}
