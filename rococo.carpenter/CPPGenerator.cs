using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace Rococo.Carpenter
{
    public static class CPPCore
    {
        public static string GetPingPathArchiveName(IRules rules, ITable table)
        {      
            string spacelessTableName = table.TableName.Replace(" ", "_");
            string fullname = rules.CppSource.Replace(".cpp", ".") + spacelessTableName + ".bin";
            return "!" + fullname.Replace("\\", "/");
        }

        static Dictionary<string, StringBuilder> openFiles = new Dictionary<string, StringBuilder>();
        public static StringBuilder OpenFile(string filename)
        {
            StringBuilder sb;
            if (!openFiles.TryGetValue(filename, out sb))
            {
                openFiles.Add(filename, sb = new StringBuilder());
            }

            return sb;
        }

        private static Dictionary<string, EnumDef> enums = new Dictionary<string, EnumDef>();

        public static EnumDef FindEnumDef(string name)
        {
            EnumDef result = null;
            return enums.TryGetValue(name, out result) ? result : null;
        }

        public static EnumDef FindElseCreateEnumDef(string name, string source)
        {
            EnumDef result = null;
            if (enums.TryGetValue(name, out result))
            {
                return result;
            }

            result = new EnumDef(name, source);
            enums.Add(name, result);
            return result;
        }

        public static bool HasEnumDefs(string source)
        {
            foreach(var e in enums)
            {
                if (e.Value.PrimarySourceFile == source)
                {
                    return true;
                }               
            }

            return false;
        }

        public static IDictionary<string, EnumDef> EnumDefs
        {
            get { return enums; }
        }

        public static string ExcelSource
        {
            get; set;
        }

        public static string GetCppType(UnderlyingType type, bool isMutable)
        {
            switch (type)
            {
                case UnderlyingType.String:
                    return isMutable ? "HString" : "fstring";
                case UnderlyingType.Float32:
                    return "float";
                case UnderlyingType.Float64:
                    return "double";
                case UnderlyingType.Int32:
                    return "int32";
                case UnderlyingType.Int64:
                    return "int64";
                case UnderlyingType.Bool:
                    return "bool";
                default:
                    throw new Exception("CPPCore.GetCppType(...): do not know how to handle " + type);
            }
        }

        public static void Commit()
        {
            foreach (var i in openFiles)
            {
                File.WriteAllText(i.Key, i.Value.ToString());
            }
        }
    }

    internal enum NS_PARSE_STATE
    {
        EXPECTING_FIRST_CHAR,
        INSIDE_SUBSPACE
    }

    public static class SexyValidators
    {
        public static void ValidateLegalSexyFunctionName(string dottedNamespace, string hint)
        {
            NS_PARSE_STATE state = NS_PARSE_STATE.EXPECTING_FIRST_CHAR;

            int i = 0;

            foreach (char c in dottedNamespace)
            {
                switch(state)
                {
                    case NS_PARSE_STATE.EXPECTING_FIRST_CHAR:
                        if (!Char.IsUpper(c))
                        {
                            throw new Exception(hint + ": Expecting capital letter at the start of each subspace token. Error at position " + i);
                        }
                        state = NS_PARSE_STATE.INSIDE_SUBSPACE;
                        break;
                    case NS_PARSE_STATE.INSIDE_SUBSPACE:
                        if (c == '.')
                        {
                            state = NS_PARSE_STATE.EXPECTING_FIRST_CHAR;
                        }
                        else if (c == ':')
                        {
                            throw new Exception(hint + ": Expecting '.' to separate subspace elements, not ':'" + i);
                        }
                        else if (!Char.IsUpper(c) && !Char.IsLower(c) && !Char.IsNumber(c))
                        {
                            throw new Exception(hint + ": Expecting alpha numeric: A-Z, a-z or 0-9 following the first character in the subspace token. Error at position " + i);
                        }
                        break;
                }


                i++;
            }

            if (state == NS_PARSE_STATE.EXPECTING_FIRST_CHAR)
            {
                throw new Exception("Expecting namespace A.B.C.D");
            }
        }

        public static void ValidateLegalSexyLocalFunctionName(string localName, string hint)
        {
            if (localName == null || localName.Length == 0)
            {
                throw new Exception("Function name was blank");
            }

            NS_PARSE_STATE state = NS_PARSE_STATE.EXPECTING_FIRST_CHAR;

            int i = 0;

            foreach (char c in localName)
            {
                switch (state)
                {
                    case NS_PARSE_STATE.EXPECTING_FIRST_CHAR:
                        if (!Char.IsUpper(c))
                        {
                            throw new Exception(hint + ": Expecting capital letter at the start of each subspace token. Error at position " + i);
                        }
                        state = NS_PARSE_STATE.INSIDE_SUBSPACE;
                        break;
                    case NS_PARSE_STATE.INSIDE_SUBSPACE:
                        if (!Char.IsUpper(c) && !Char.IsLower(c) && !Char.IsNumber(c))
                        {
                            throw new Exception(hint + ": Expecting alpha numeric: A-Z, a-z or 0-9 following the first character in the subspace token. Error at position " + i);
                        }
                        break;
                }

                i++;
            }
        }
    }

    public class EnumDef
    {
        public string Name
        {
            get;
            private set;
        }

        public string PrimarySourceFile
        {
            get;
            private set;
        }

        public EnumDef(string name, string primarySourceFile)
        {
            this.Name = name;
            this.PrimarySourceFile = primarySourceFile;
        }

        public bool TryGetIndex(string key, out int index)
        {
            for(int i = 0; i < this.keys.Count; i++)
            {
                if (this.keys[i] == key)
                {
                    index = i;
                    return true;
                }
            }

            index = -1;
            return false;
        }

        private List<string> keys = new List<string> ();
        private HashSet<string> uniqueKeys = new HashSet<string> ();
        public void AddKey(string item)
        {
            string shortenedItem = item.Replace(" ", "");
            if (uniqueKeys.Contains (shortenedItem))
            {
                return;
            }

            uniqueKeys.Add(shortenedItem);
            keys.Add(shortenedItem);

            keyArray = null;
        }

        private string[] keyArray = null;

        public string[] Keys
        {
            get
            {
                if (keyArray == null)
                {
                    keyArray = keys.ToArray();
                }

                return keyArray;
            }
        }

        public override string ToString()
        {
            return string.Format("{0}: with {1} values", Name, Keys.Length);
        }
    }

    public interface IParsedColumnData
    {
        public ColumnHeader[] ColumnHeaders
        {
            get;
        }
    }

    public struct ColumnHeader
    {
        public UnderlyingType UnderlyingType;
        public string Name;
        public string EnumName;
        public string FullTypeName;
        public bool IsAliasType;
    }

    public class CPPGenerator : IParsedColumnData
    {
        public string Repository
        {
            get; private set;
        }

        public string FullHeaderPath
        {
            get; private set;
        }
        public string FullSourcePath
        {
            get; private set;
        }

        public string CppNamespace
        {
            get; private set;
        }

        public StringBuilder HeaderBuilder
        {
            get; private set;
        }
        public StringBuilder SourceBuilder
        {
            get; private set;
        }

        public string RowStructName
        {
            get; private set;
        }
        public string MetaDataInterfaceName
        {
            get
            {
                return Rules.CppInterface + "_MetaData";
            }
        }

        public ITypes Types
        {
            get;
            private set;
        }

        public IMetaData MetaData
        {
            get;
            private set;
        }

        public ITable Table
        {
            get;
            private set;
        }

        public IRules Rules
        {
            get;
            private set;
        }

        public ColumnHeader[] ColumnHeaders
        {
            get;
            private set;
        }

        void SetCppNamespace(string dottedNamespace)
        {
            SexyValidators.ValidateLegalSexyFunctionName(dottedNamespace, "C++ Namespace");
            CppNamespace = dottedNamespace.Replace(".", "::");
        }

        public void ValidateRules()
        {
            string i = Rules.CppInterface;
            if (i == null || !i.StartsWith("I"))
            {
                throw new Exception("Expecting 'C++ Interface' to begin with capital I");
            }

            if (i.EndsWith("Supervisor"))
            {
                throw new Exception("'C++ Interface' must not end with 'Supervisor'");
            }

            SexyValidators.ValidateLegalSexyLocalFunctionName(i, "C++ Interface");

            SexyValidators.ValidateLegalSexyLocalFunctionName(Rules.CppFactory, "C++ Factory");
        }

        public void SetStructName(string interfaceName)
        {
            RowStructName = interfaceName.Substring(1) + "Row";
        }

        public CPPGenerator(ITypes types, IMetaData metaData, ITable table, IRules rules)
        {
            Types = types;
            MetaData = metaData;
            Table = table;
            Rules = rules;

            try
            {
                SetRepository(Rules.CppRepo);
                SetFullHeaderPath(Rules.CppHeader);
                SetFullSourcePath(Rules.CppSource);
                SetCppNamespace(Rules.CppNamespace);
                ValidateRules();
                SetStructName(Rules.CppInterface);

                List<ColumnHeader> columnHeaders = new List<ColumnHeader>();

                foreach (var c in Table.Columns)
                {
                    if (c.TypeInfo == null || c.TypeInfo.Length == 0 || c.Title == null || c.Title.Length == 0)
                    {
                        continue;
                    }

                    char q = c.Title[0];

                    if (!Char.IsUpper(q))
                    {
                        throw new Exception("Expecting column title to begin with capital letter: " + c.Title);
                    }

                    string fieldName = String.Format("{0}{1}", char.ToLower(q), c.Title.Substring(1));

                    UnderlyingType type;
                    if (Enum.TryParse(c.TypeInfo, out type))
                    {
                        columnHeaders.Add(new ColumnHeader { EnumName = null, UnderlyingType = type, FullTypeName = CPPCore.GetCppType(type, Rules.IsMutable), Name = fieldName, IsAliasType = false });
                        continue;
                    }

                    if (c.TypeInfo == "Index")
                    {
                        columnHeaders.Add(new ColumnHeader { EnumName = null, UnderlyingType = UnderlyingType.Int32, FullTypeName = c.TypeInfo, Name = fieldName, IsAliasType = false });
                        continue;
                    }

                    string enumType;
                    if (TryParseAsEnumType(c.TypeInfo, out enumType))
                    {
                        columnHeaders.Add(new ColumnHeader { EnumName = enumType, UnderlyingType = UnderlyingType.Int32, FullTypeName = enumType, Name = fieldName, IsAliasType = false });
                        continue;
                    }

                    if (!TryParseAsAliasedType(c.TypeInfo, out type))
                    {
                        throw new Exception("Unknown type in column '" + c.TypeInfo + "'");
                    }

                    string aliasPrefix = "Type-";

                    columnHeaders.Add(new ColumnHeader { EnumName = null, UnderlyingType = type, FullTypeName = c.TypeInfo.Substring(aliasPrefix.Length), Name = fieldName, IsAliasType = true });
                }

                ColumnHeaders = columnHeaders.ToArray();
            }
            catch (Exception ex)
            {
                throw new Exception("Worksheet '" + Table.TableName + "' with rules '" + Rules.RuleSetName + "' raised an exception", ex);
            }
        }

        public bool TryParseAsEnumType(string candidate, out string enumType)
        {
            enumType = null;

            string enumTypePrefix = "Enum-";

            if (!candidate.StartsWith(enumTypePrefix))
            {
                return false;
            }

            enumType = candidate.Substring(enumTypePrefix.Length);

            return enumType.Length > 0;
        }

        public bool TryParseAsAliasedType(string candidate, out UnderlyingType type)
        {
            type = UnderlyingType.String;

            string customTypePrefix = "Type-";

            if (!candidate.StartsWith(customTypePrefix))
            {
                return false;
            }

            TypeDefinition typeDef;
            if (!Types.TryGetType(candidate.Substring(customTypePrefix.Length), out typeDef))
            {
                return false;
            }

            type = typeDef.SysType;
            return true;
        }

        public void SetFullHeaderPath(string newHeaderPath)
        {
            if (newHeaderPath == null || newHeaderPath.Length == 0)
            {
                throw new Exception("Bad rule: missing C++ Header");
            }

            if (!newHeaderPath.EndsWith(".h") && !newHeaderPath.EndsWith(".hpp"))
            {
                throw new Exception("Bad rule: 'C++ Header' must specify a file with either .h or .hpp extension");
            }

            FullHeaderPath = Repository + newHeaderPath;
        }

        public void SetFullSourcePath(string newSourcePath)
        {
            if (newSourcePath == null || newSourcePath.Length == 0)
            {
                throw new Exception("Bad rule: missing C++ Source");
            }

            if (!newSourcePath.EndsWith(".cpp") && !newSourcePath.EndsWith(".inl"))
            {
                throw new Exception("Bad rule: 'C++ Source' must specify a file with either .cpp or .inl extension");
            }

            FullSourcePath = Repository + newSourcePath;
        }

        public void SetRepository(string newRepo)
        {
            if (newRepo == null || newRepo.Length == 0)
            {
                throw new Exception("Bad rule: missing C++ Repo");
            }

            Repository = newRepo.Replace("$(ROCOCO)", Environment.Solution);

            if (!Repository.EndsWith(Path.DirectorySeparatorChar))
            {
                Repository += Path.DirectorySeparatorChar;
            }

            if (!Directory.Exists(Repository))
            {
                throw new Exception("Bad rule: CppRepo '" + Repository + "' cannot determine directory.");
            }
        }

        public void AppendHeaderIncludes()
        {
            var sb = HeaderBuilder;

            sb.AppendLine("#pragma once");
            sb.Append("// Generated by rococo.carpenter. Timestamp: ");
            sb.AppendLine(System.DateTime.Now.ToString());
            sb.Append("// Excel Source: ");
            sb.AppendLine(CPPCore.ExcelSource);
            sb.AppendLine();

            sb.AppendLine("#include <rococo.types.h>");
            sb.AppendLine();
        }

        public void AppendSourceIncludes()
        {
            var sb = SourceBuilder;

            sb.Append("// Generated by rococo.carpenter. Timestamp: ");
            sb.AppendLine(System.DateTime.Now.ToString());
            sb.Append("// Excel Source: ");
            sb.AppendLine(CPPCore.ExcelSource);
            sb.AppendLine();

            sb.AppendFormat("#include \"{0}\"", Rules.CppHeader);
            sb.AppendLine();
            if (Rules.KeyNames.GetEnumerator().MoveNext())
            {
                sb.AppendLine("#include <rococo.hashtable.h>");
            }
            sb.AppendLine();
        }

        private readonly string tab = "    ";

        public void AppendTab(StringBuilder sb)
        {
            sb.Append(tab);
        }

        public void AppendRowField(int fieldIndex, StringBuilder sb)
        {
            sb.AppendFormat("{0} {1}", ColumnHeaders[fieldIndex].FullTypeName, ColumnHeaders[fieldIndex].Name);
        }

        public void AppendEnumDef(StringBuilder sb, string enumName)
        {
            EnumDef def = CPPCore.FindEnumDef(enumName);

            if (def != null)
            {
                return;
            }

            def = CPPCore.FindElseCreateEnumDef(enumName, this.Rules.CppSource);

            for (int i = 0; i < ColumnHeaders.Length; i++)
            {
                if (ColumnHeaders[i].EnumName == enumName)
                {
                    // We found a column at column index i populated by enum names, so add them all to the enum type

                    for (int j = 0; j < Table.NumberOfRows; ++j)
                    {
                        IItemSequence rowItems = Table.GetRow(j);
                        var item = rowItems.GetItemText(i);

                        if (item != null && item.Length > 0)
                        {
                            def.AddKey(item);
                        }
                    }
                }
            }

            AppendTab(sb);

            TypeDefinition typeDef;
            Types.TryGetType(enumName, out typeDef);

            sb.AppendFormat("enum class {0}: int", enumName);

            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("{");

            bool first = true;

            int count = 1;

            foreach (var item in def.Keys)
            {
                if (first)
                {
                    first = false;

                    AppendTab(sb);
                    AppendTab(sb); ;
                }
                else
                {
                    if ((count++ % 12) == 0)
                    {
                        sb.AppendLine(",");
                        AppendTab(sb);
                        AppendTab(sb);
                    }
                    else
                    {
                        sb.Append(", ");
                    }
                }

                sb.Append(item);
            }

            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("};");
            sb.AppendLine();

            AppendTab(sb);
            sb.AppendFormat("bool TryParse(const fstring& text, {0}& result);", enumName);
            sb.AppendLine();
            AppendTab(sb);
            sb.AppendFormat("fstring ToString({0} value);", enumName);
            sb.AppendLine();
            sb.AppendLine();
        }

        HashSet<string> requiredNamespaces = new HashSet<string>();

        public void AppendTypedefs()
        {
            var sb = HeaderBuilder;

            HashSet<string> aliasTypes = new HashSet<string>();

            for (int i = 0; i < ColumnHeaders.Length; ++i)
            {
                var h = ColumnHeaders[i];
                if (h.IsAliasType)
                {
                    if (aliasTypes.Add(h.FullTypeName))
                    {
                        TypeDefinition typedef;
                        if (Types.TryGetType(h.FullTypeName, out typedef))
                        {
                            string ns = typedef.NameSpace.Replace(".", "::");

                            sb.Append("namespace ");
                            sb.Append(ns);
                            sb.AppendLine();
                            sb.AppendLine("{");

                            AppendTab(sb);
                            sb.AppendFormat("typedef {0} {1};", CPPCore.GetCppType(h.UnderlyingType, false), typedef.LocalName);
                            sb.AppendLine();

                            sb.AppendLine("}");

                            sb.AppendLine();

                            requiredNamespaces.Add(ns);
                        }
                        else
                        {
                            throw new Exception("No typedef for " + h.FullTypeName + ". Add a definition in the Types worksheet or correct spelling");
                        }
                    }
                }
            }
        }

        public void AppendUsingDirectives(StringBuilder sb)
        {
            foreach (var ns in requiredNamespaces)
            {
                AppendTab(sb);
                sb.AppendFormat("using namespace {0};", ns);
                sb.AppendLine();
            }
        }

        public bool HasInterfaceForRow
        {
            get
            {
                if (Rules.Lifetime != TableLifetime.Static)
                {
                    foreach (var column in ColumnHeaders)
                    {
                        if (column.UnderlyingType == UnderlyingType.String)
                        {
                            return true;
                        }
                    }
                }

                return false;
            }
        }

        public string GetFunctionNameForHeader(ColumnHeader header)
        {
            StringBuilder sb = new StringBuilder();
            string name = header.Name;
            sb.Append(char.ToUpper(name[0]));
            sb.Append(name, 1, name.Length - 1);
            return sb.ToString();
        }

        public string GetFunctionNameForHeader(int index)
        {
            ColumnHeader header = ColumnHeaders[index];
            return GetFunctionNameForHeader(header);
        }

        public void AppendInterface()
        {
            var sb = HeaderBuilder;

            AppendTypedefs();

            sb.Append("namespace ");
            sb.AppendLine(CppNamespace);
            sb.AppendLine("{");

            AppendUsingDirectives(sb);

            for (int i = 0; i < ColumnHeaders.Length; ++i)
            {
                var h = ColumnHeaders[i];
                if (h.EnumName != null)
                {
                    sb.AppendLine();
                    AppendEnumDef(sb, h.EnumName);
                }
            }

            AppendTab(sb);

            sb.AppendFormat("{0} {1}{2}", HasInterfaceForRow ? "ROCOCOAPI" : "struct", HasInterfaceForRow ? "I" : "", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("{");

            var columns = Table.Columns;
            for (int i = 0; i < ColumnHeaders.Length; ++i)
            {
                AppendTab(sb);
                AppendTab(sb);

                if (HasInterfaceForRow)
                {
                    sb.Append("virtual ");
                    sb.AppendFormat("{0} Get{1}() const = 0;", ColumnHeaders[i].FullTypeName, GetFunctionNameForHeader(i));
                    sb.AppendLine();
                }
                else
                {
                    AppendRowField(i, sb);
                    sb.AppendLine(";");
                }
            }

            AppendTab(sb);
            sb.AppendLine("};");
            sb.AppendLine("}");
            sb.AppendLine();

            foreach (var commonHeader in Environment.TargetConfig.TypeDependentHeaders)
            {
                sb.AppendFormat("#include \"{0}\"", commonHeader);
                sb.AppendLine();
            }

            sb.AppendLine();

            sb.Append("namespace ");
            sb.AppendLine(CppNamespace);
            sb.AppendLine("{");


            AppendTab(sb);
            sb.AppendFormat("ROCOCOAPI {0}", MetaDataInterfaceName);
            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("{");

            foreach (var ms in MetaData.StringVariables)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("virtual fstring Get{0}() const = 0;", ms.Key);
                sb.AppendLine();
            }

            AppendTab(sb);
            sb.AppendLine("};");
            sb.AppendLine();

            AppendTab(sb);
            sb.Append("ROCOCOAPI ");
            sb.AppendFormat("{0}: protected {0}_Sexy", Rules.CppInterface);
            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("{");
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("virtual const {0}{1}& GetRow(int32 index) const = 0;", HasInterfaceForRow ? "I" : "", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("virtual const int32 NumberOfRows() const = 0;", RowStructName);
            sb.AppendLine();

            if (Rules.Lifetime == TableLifetime.Static)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("virtual const {0}* begin() const = 0;", RowStructName);
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("virtual const {0}* end() const = 0;", RowStructName);
                sb.AppendLine();
            }

            if (Rules.Lifetime == TableLifetime.Dynamic)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("// load a table from a binary archive. If [tablePingPath] is null, defaults to {0}", CPPCore.GetPingPathArchiveName(Rules, Table));
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("virtual void Load(const IInstallation& installation, cstr tablePingPath = nullptr) = 0;");
                sb.AppendLine();
            }

            if (Rules.MetaDataAccess == MetaDataAccessType.Composition)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("virtual const {0}& Meta() const = 0;", MetaDataInterfaceName);
                sb.AppendLine();
            }

            foreach(string key in Rules.KeyNames)
            {
                var header = FindColumnByTitle(key);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("virtual const {0}{1}* FindRowBy{2}({3} {4}, int32& index) const = 0;", HasInterfaceForRow ? "I" : "", RowStructName, key, header.FullTypeName, HeaderNameToVariableName(key));
                sb.AppendLine();
            }

            AppendTab(sb);
            sb.AppendLine("};");

            sb.AppendLine();

            if (Rules.Lifetime == TableLifetime.Dynamic)
            {
                AppendTab(sb);
                sb.AppendFormat("ROCOCOAPI {0}Supervisor: {0}", Rules.CppInterface);
                sb.AppendLine();
                AppendTab(sb);
                sb.AppendLine("{");
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("virtual void Free() = 0;");
                AppendTab(sb);
                sb.AppendLine("};");

                sb.AppendLine();
                AppendTab(sb);

                sb.AppendFormat("{0}Supervisor* {1}();", Rules.CppInterface, Rules.CppFactory);
                sb.AppendLine();
            }
            else if (Rules.Lifetime == TableLifetime.Singleton)
            {
                AppendTab(sb);
                sb.AppendFormat("ROCOCOAPI {0}Supervisor: {0}", Rules.CppInterface);
                sb.AppendLine();
                AppendTab(sb);
                sb.AppendLine("{");
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("virtual void Free() = 0;");
                AppendTab(sb);
                sb.AppendLine("};");

                sb.AppendLine();
                AppendTab(sb);

                sb.AppendFormat("{0}Supervisor* {1}(IInstallation& installation);", Rules.CppInterface, Rules.CppFactory);
                sb.AppendLine();
            }
            else // Static
            {
                AppendTab(sb);
                sb.AppendFormat("{0}& {1}();", Rules.CppInterface, Rules.CppFactory);
                sb.AppendLine();
            }

            sb.AppendLine("}");
        }

        public string CppTableName
        {
            get
            {
                return Table.TableName.Replace(" ", "_");
            }
        }

        public static string MakeNonEmptyString(string item, string defaultValue)
        {
            if (item == null || item.Length == 0) return defaultValue;
            return item;
        }

        public static bool RequiresEscapement(string text)
        {
            foreach (char c in text)
            {
                if (c < 32 || c > 127 || c == '"' || c == '\\')
                {
                    return true;
                }
            }

            return false;
        }

        public static void AppendCPPString(StringBuilder sb, string item)
        {
            if (!RequiresEscapement(item))
            {
                sb.Append('"');
                sb.Append(item);
                sb.Append('"');
                return;
            }

            string raw = item.Contains("[RAW]") ? "#RAW#" : "[RAW]";
            sb.AppendFormat("u8R\"{0}({1}){0}\"", raw, item);
        }

        public static void AppendSanitizedItem(StringBuilder sb, string item, UnderlyingType type)
        {
            switch (type)
            {
                case UnderlyingType.Bool:
                    sb.Append(item == "0" ? "false" : "true");
                    break;
                case UnderlyingType.Int32:
                    sb.Append(Int32.Parse(MakeNonEmptyString(item, "0")));
                    break;
                case UnderlyingType.Int64:
                    sb.Append(Int64.Parse(MakeNonEmptyString(item, "0")));
                    break;
                case UnderlyingType.Float32:
                    sb.Append(float.Parse(MakeNonEmptyString(item, "0")));
                    break;
                case UnderlyingType.Float64:
                    sb.Append(double.Parse(MakeNonEmptyString(item, "0")));
                    break;
                case UnderlyingType.String:
                    sb.Append("to_fstring((cstr)");
                    AppendCPPString(sb, item);
                    sb.Append(")");
                    break;
                default:
                    throw new Exception("Cannot handle type " + type);
            }
        }

        public void AppendTableDeclaration()
        {
            var sb = SourceBuilder;

            if (Rules.Lifetime == TableLifetime.Singleton)
            {
                sb.Append("static ");
            }

            sb.AppendFormat("std::vector<{0}> rows;", RowStructName);

            sb.AppendLine();
        }

        public string HeaderNameToVariableName(string header)
        {
            return string.Format("{0}{1}", char.ToLower(header[0]), header.Substring(1));
        }


        public void AppendLoadRows()
        {
            var sb = SourceBuilder;

            sb.AppendFormat("static void Append{0}s(", RowStructName);

            foreach (var key in Rules.KeyNames)
            {
                var header = FindColumnByTitle(key);
                if (header.Name == String.Empty)
                {
                    throw new Exception(String.Format("Key '{0}' in rules does not match a column title", key));
                }

                if (header.UnderlyingType == UnderlyingType.String)
                {
                    sb.AppendFormat("stringmap<size_t>& {0}_to_index, ", header.Name);
                }
                else
                {
                    sb.AppendFormat("std::unordered_map<{0}, size_t>& {1}_to_index, ", header.FullTypeName, HeaderNameToVariableName(header.Name));
                }
            }

            sb.AppendFormat("std::vector<{0}>& rows, const IInstallation& installation, const char* source)", RowStructName);
            sb.AppendLine("");
            sb.AppendLine("{");
            AppendTab(sb);
            sb.AppendFormat("if (!source || !*source) source = \"{0}\";", CPPCore.GetPingPathArchiveName(Rules, Table));
            sb.AppendLine();
            sb.AppendLine();

            AppendTab(sb);
            sb.AppendLine("struct ANON : ITableRowBuilder");
            AppendTab(sb);
            sb.AppendLine("{");

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("const char* source = nullptr;");
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("std::vector<{0}>* rows = nullptr;", RowStructName);
            sb.AppendLine();
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("void OnColumns(int numberOfColumns, const ColumnHeader* headers) override");
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("{");


            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("if (numberOfColumns != {0}) Throw(0, \"%s: Found %d columns. Expecting {0} columns in %s\", __FUNCTION__, numberOfColumns, source);", ColumnHeaders.Length);
            sb.AppendLine();

            for(int i = 0; i < ColumnHeaders.Length; i++)
            {
                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);

                sb.AppendFormat("ValidateHeader(headers[{0}], ColumnType::UnderlyingType", i);

                switch (ColumnHeaders[i].UnderlyingType)
                {
                    case UnderlyingType.Int32:
                        sb.Append("Int32");
                        break;
                    case UnderlyingType.Int64:
                        sb.Append("Int64");
                        break;
                    case UnderlyingType.Float32:
                        sb.Append("Float32");
                        break;
                    case UnderlyingType.Float64:
                        sb.Append("Float64");
                        break;
                    case UnderlyingType.Bool:
                        sb.Append("Bool");
                        break;
                    case UnderlyingType.String:
                        sb.Append("UTF8");
                        break;
                    default:
                        throw new Exception("Unknown type" + ColumnHeaders[i].UnderlyingType);
                            
                }

                sb.AppendLine(", source);");
            }

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("}");
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("void OnHeaders(const TableRowHeaders& headers) override");

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("{");

            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("rows->reserve(headers.NumberOfRows);");

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("}");
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("void OnRow(ITableRowData& archiveData)");

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("{");

            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("{0} row;", RowStructName);
            sb.AppendLine();

            foreach(var column in ColumnHeaders)
            {
                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);

                if (column.IsAliasType)
                {
                    TypeDefinition def;
                    Types.TryGetType(column.FullTypeName, out def);
                    sb.AppendFormat("row.{0} = ({1}::{2}) archiveData.Next", column.Name, def.NameSpace.Replace(".", "::"), def.LocalName);
                }
                else
                {
                    sb.AppendFormat("row.{0} = ({1}) archiveData.Next", column.Name, column.FullTypeName);
                }

                switch(column.UnderlyingType)
                {
                    case UnderlyingType.Int32:
                        sb.Append("Int32");
                        break;
                    case UnderlyingType.Int64:
                        sb.Append("Int64");
                        break;
                    case UnderlyingType.Float32:
                        sb.Append("Float32");
                        break;
                    case UnderlyingType.Float64:
                        sb.Append("Float64");
                        break;
                    case UnderlyingType.Bool:
                        sb.Append("Bool");
                        break;
                    case UnderlyingType.String:
                        sb.Append("TempString");
                        break;
                }

                sb.AppendLine("();");
            }

            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("rows->push_back(row);");

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("}");

            AppendTab(sb);
            sb.AppendLine("} builder;");
            sb.AppendLine();

            AppendTab(sb);
            sb.AppendLine("builder.source = source;");
            AppendTab(sb);
            sb.AppendLine("builder.rows = &rows;");
            sb.AppendLine();

            AppendTab(sb);
            sb.AppendLine("ParseTableRows(installation, source, builder);");

            foreach(var key in Rules.KeyNames)
            {
                sb.AppendLine();
                AppendTab(sb);
                sb.AppendLine("for (size_t i = 0; i < rows.size(); ++i)");
                AppendTab(sb);
                sb.AppendLine("{");
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("auto & r = rows[i];");
                AppendTab(sb);
                AppendTab(sb);

                var header = FindColumnByTitle(key);
                if (header.UnderlyingType == UnderlyingType.String)
                {
                    sb.AppendFormat("auto j = {0}_to_index.insert(r.ownerId, i);", HeaderNameToVariableName(key));
                }
                else
                {
                    sb.AppendFormat("auto j = {0}_to_index.insert(std::make_pair(r.{0}, i));", HeaderNameToVariableName(key));
                }

                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("if (!j.second)");
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("{");
                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("Throw(0, \" %s: Duplicate '{0}' at row %llu. First seen at row %llu\", source, i, j.first->second);", key);
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("}");
                AppendTab(sb);
                sb.AppendLine("}");
            }

            sb.AppendLine("}");
            sb.AppendLine();
        }

        public ColumnHeader FindColumnByTitle(string title)
        {
            foreach(var header in ColumnHeaders)
            {
                if (string.Compare(header.Name, title, true) == 0)
                {
                    return header;
                }
            }

            ColumnHeader nullHeader = new ColumnHeader() { Name = String.Empty };
            return nullHeader;
        }

        public void AppendFQSexyInterfaceName(StringBuilder sb)
        {
            sb.Append(CppNamespace);
            sb.Append("::");
            sb.Append(Rules.CppInterface);
            sb.Append("_Sexy");
        }


        public void AppendFactoryConstruct(StringBuilder sb)
        {
            AppendFQSexyInterfaceName(sb);
            sb.Append("* ");
            sb.Append("FactoryConstruct");
            sb.Append(CppNamespace.Replace("::", ""));
            sb.Append(Rules.CppFactory);
            sb.Append("(");
            AppendFQSexyInterfaceName(sb);
            sb.Append("* nullContext)");
        }

        public void AppendTableImplementation()
        {
            var sb = SourceBuilder;

            sb.AppendLine("#include <rococo.api.h>");

            if (Rules.Lifetime != TableLifetime.Static)
            {
                sb.AppendLine("#include <rococo.io.h>");
                sb.AppendLine();
                sb.AppendLine("#include <vector>");
                sb.AppendLine();
            }

            if (HasInterfaceForRow)
            {
                sb.AppendLine("#include <rococo.strings.h>");
            }

            if (Rules.Lifetime == TableLifetime.Singleton)
            {
                sb.AppendLine("#include <atomic>");
            }

            sb.AppendLine("using namespace Rococo;");

            if (Rules.Lifetime != TableLifetime.Static)
            {
                sb.AppendLine("using namespace Rococo::IO;");
            }

            sb.Append("using namespace ");
            sb.Append(CppNamespace);
            sb.AppendLine(";");
            sb.AppendLine();

            if (HasInterfaceForRow)
            {
                sb.Append("namespace ");
                sb.Append(CppNamespace);
                sb.AppendLine();
                sb.AppendLine("{");
                AppendTab(sb);
                sb.AppendFormat("struct {0}: I{0}", RowStructName);
                sb.AppendLine("");
                AppendTab(sb);
                sb.AppendLine("{");

                foreach(var header in ColumnHeaders)
                {
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendFormat("{0} {1};", header.UnderlyingType == UnderlyingType.String ? "HString" : header.FullTypeName, header.Name);
                    sb.AppendLine();
                }

                sb.AppendLine();

                foreach (var header in ColumnHeaders)
                {
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendFormat("{0} Get{1}() const override {{ return {2}; }}", header.FullTypeName, GetFunctionNameForHeader(header), header.Name);
                   
                    sb.AppendLine();
                }

                AppendTab(sb);
                sb.AppendLine("};");
                sb.AppendLine("}");
                sb.AppendLine();
            }

            if (Rules.Lifetime == TableLifetime.Static)
            {
                sb.AppendFormat("static {0}{1} defaultRows[{2}] = ", Rules.IsMutable ? "" : "const ", RowStructName, Table.NumberOfRows);
                sb.AppendLine();
                sb.AppendLine("{");

                for (int i = 0; i < Table.NumberOfRows; i++)
                {
                    if (i > 0)
                    {
                        sb.AppendLine(",");
                    }
                    AppendTab(sb);
                    var row = Table.GetRow(i);

                    sb.AppendFormat("{{ // #{0}", i);
                    sb.AppendLine();
                    AppendTab(sb);
                    AppendTab(sb);

                    for (int j = 0; j < row.Length; ++j)
                    {
                        if (j != 0)
                        {
                            sb.Append(", ");
                        }

                        string item = row.GetItemText(j);

                        if (ColumnHeaders[j].EnumName != null)
                        {
                            sb.AppendFormat("{0}::{1}", ColumnHeaders[j].EnumName, item.Replace(" ", ""));
                        }
                        else
                        {
                            AppendSanitizedItem(sb, item, ColumnHeaders[j].UnderlyingType);
                        }
                    }

                    sb.AppendLine();

                    AppendTab(sb);
                    sb.Append("}");
                }

                sb.AppendLine();
                sb.AppendLine("};");
                sb.AppendLine();
            }
            else if (Rules.Lifetime == TableLifetime.Singleton)
            {
                AppendTableDeclaration();
                sb.AppendLine();
            }

            foreach(var key in Rules.KeyNames)
            {
                var header = FindColumnByTitle(key);
                if (header.Name == String.Empty)
                {
                    throw new Exception(String.Format("Key '{0}' in rules does not match a column title", key));
                }

                if (header.UnderlyingType == UnderlyingType.String)
                {
                    sb.AppendFormat("static stringmap<size_t> {0}_to_index;", header.Name);
                    sb.AppendLine();
                }
                else
                {
                    sb.AppendFormat("static std::unordered_map<{0}, size_t> {1}_to_index;", header.FullTypeName, HeaderNameToVariableName(header.Name));
                    sb.AppendLine();
                }
            }

            if (Rules.KeyNames.GetEnumerator().MoveNext())
            {
                sb.AppendLine();
            }

            if (Rules.Lifetime != TableLifetime.Static)
            {
                AppendLoadRows();
            }

            sb.AppendLine("namespace ANON");
            sb.AppendLine("{");
            AppendTab(sb);
            sb.AppendFormat("using namespace {0};", CppNamespace);
            sb.AppendLine();
            sb.AppendLine();
            AppendTab(sb);
            sb.AppendFormat("struct {0}_Implementation: {1}{2}", CppTableName, Rules.CppInterface, Rules.Lifetime == TableLifetime.Static ? "" : "Supervisor");

            if (Rules.MetaDataAccess == MetaDataAccessType.Composition)
            {
                sb.AppendFormat(", {0}", MetaDataInterfaceName);
            }

            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("{");

            AppendTab(sb);
            AppendTab(sb);
            AppendFQSexyInterfaceName(sb);
            sb.AppendFormat("& GetSexyInterface()");
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.Append("{");
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("return *this;");

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("}");

            sb.AppendLine();

            if (Rules.Lifetime == TableLifetime.Singleton)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("std::atomic<int32> referenceCount = 0;");
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("{0}_Implementation(IInstallation& installation)", CppTableName);
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("{");

                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);

                sb.AppendFormat("if (referenceCount.fetch_add(1) == 0) Append{0}s(", RowStructName);

                foreach(var key in Rules.KeyNames)
                {
                    sb.AppendFormat("{0}_to_index, ", HeaderNameToVariableName(key));
                }

                sb.Append("rows, installation, nullptr);");
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("}");
                sb.AppendLine();
            }

            if (Rules.Lifetime == TableLifetime.Dynamic)
            {
                AppendTab(sb);
                AppendTab(sb);
                AppendTableDeclaration();
                sb.AppendLine();
            }

            if (Rules.Lifetime != TableLifetime.Static)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("void Free() override");

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("{");

                if (Rules.Lifetime == TableLifetime.Singleton)
                {
                    AppendTab(sb);
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("if (referenceCount.fetch_sub(1) == 1) { rows.clear(); rows.shrink_to_fit(); }");
                }

                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("delete this;");

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("}");
                sb.AppendLine("");
            }

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("const {0}{1}& GetRow(int32 index) const override", HasInterfaceForRow ? "I" : "", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("{");

            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);

            if (Rules.Lifetime == TableLifetime.Static)
            {
                sb.AppendFormat("return defaultRows[index];");
            }
            else
            {
                sb.AppendFormat("return rows[index];");
            }

            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("}");
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("// Sexy Interface Method");

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("void GetRow(int32 index, {0}{1}& row) override", HasInterfaceForRow ? "I" : "", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("{");
            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("if (index > NumberOfRows()) Throw(0, \"%s: [index] out of range.\", __FUNCTION__);");
            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("row = GetRow(index);");
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("}");
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("const int32 NumberOfRows() const override", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("{");

            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);

            if (Rules.Lifetime == TableLifetime.Static)
            {
                sb.AppendFormat("return {0};", Table.NumberOfRows);
            }
            else
            {
                sb.AppendFormat("return (int32) rows.size();");
            }

            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("}");
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("// Sexy Interface Method");


            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("int32 NumberOfRows() override", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("{");

            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);

            if (Rules.Lifetime == TableLifetime.Static)
            {
                sb.AppendFormat("return {0};", Table.NumberOfRows);
            }
            else
            {
                sb.AppendFormat("return (int32) rows.size();");
            }

            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("}");

            sb.AppendLine();

            if (Rules.Lifetime == TableLifetime.Static)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("const {0}* begin() const override", RowStructName);
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("{");

                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("return defaultRows;");
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("}");
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("const {0}* end() const override", RowStructName);
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("{");
                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("return defaultRows + {0};", Table.NumberOfRows);
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("}");
                sb.AppendLine();
            }

            if (Rules.Lifetime == TableLifetime.Dynamic)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("void Load(const IInstallation& installation, cstr tablePingPath) override");
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("{");

                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("rows.clear();");

                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("Append{0}s(rows, installation, tablePingPath);", RowStructName);
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("}");
                sb.AppendLine();
            }

            if (Rules.MetaDataAccess == MetaDataAccessType.Composition)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("const {0}& Meta() const override", MetaDataInterfaceName);
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("{");

                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("return *this;");
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("}");
            }

            foreach(string key in Rules.KeyNames)
            {
                var header = FindColumnByTitle(key);
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("const {0}{1}* FindRowBy{2}({3} {4}, int32& index) const override", HasInterfaceForRow ? "I" : "", RowStructName, key, header.FullTypeName, HeaderNameToVariableName(key));
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("{");
                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("auto i = {0}_to_index.find({0});", HeaderNameToVariableName(key));
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("if (i != {0}_to_index.end()) {{ index = (int32) i->second; return &rows[index]; }}", HeaderNameToVariableName(key));
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.Append("else { index = -1; return nullptr; }");
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("}");
            }
            
            foreach (var ms in MetaData.StringVariables)
            {
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("fstring Get{0}() const override", ms.Key);
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("{");

                AppendTab(sb);
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("return \"{0}\"_fstring;", ms.Value.value);
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("}");
            }

            AppendTab(sb);
            sb.AppendLine("};");

            sb.AppendLine("}");

            sb.AppendLine();

            sb.AppendFormat("namespace {0}", CppNamespace);
            sb.AppendLine();
            sb.AppendLine("{");

            if (Rules.Lifetime == TableLifetime.Dynamic)
            {
                AppendTab(sb);

                sb.AppendFormat("{0}Supervisor* {1}()", Rules.CppInterface, Rules.CppFactory);
                sb.AppendLine();

                AppendTab(sb);
                sb.AppendLine("{");

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("return new ANON::{0}_Implementation();", CppTableName);
                sb.AppendLine();

                AppendTab(sb);
                sb.AppendLine("}");
            }
            else if (Rules.Lifetime == TableLifetime.Singleton)
            {
                AppendTab(sb);

                sb.AppendFormat("{0}Supervisor* {1}(IInstallation& installation)", Rules.CppInterface, Rules.CppFactory);
                sb.AppendLine();

                AppendTab(sb);
                sb.AppendLine("{");

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("return new ANON::{0}_Implementation(installation);", CppTableName);
                sb.AppendLine();

                AppendTab(sb);
                sb.AppendLine("}");
            }
            else // Static
            {
                AppendTab(sb);
                sb.AppendFormat("{0}& {1}()", Rules.CppInterface, Rules.CppFactory);
                sb.AppendLine();
                AppendTab(sb);
                sb.AppendLine("{");

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("static ANON::{0}_Implementation globalInstance;", CppTableName);
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("return globalInstance;");

                AppendTab(sb);
                sb.AppendLine("}");
            }

            sb.AppendLine("}");

            AppendFactoryConstruct(sb);

            sb.AppendLine();

            sb.AppendLine("{");
            if (Rules.Lifetime == TableLifetime.Dynamic)
            {
                AppendTab(sb);
                sb.AppendFormat("auto* instance = new ANON::{0}_Implementation();", CppTableName);
                sb.AppendLine();

                AppendTab(sb);
                sb.AppendFormat("return &instance->GetSexyInterface();", CppTableName);
            }
            else if (Rules.Lifetime == TableLifetime.Singleton)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("IInstallation& installation = GetInstallationFromCallContext();");
                AppendTab(sb);
                sb.AppendFormat("{0}Supervisor* instance = {1}(installation)", Rules.CppInterface, Rules.CppFactory);
                sb.AppendLine();
                AppendTab(sb);
                sb.AppendLine("return instance;");
            }
            else // Static
            {
                AppendTab(sb);
                sb.AppendFormat("return &{0}();", Rules.CppFactory);
            }
            sb.AppendLine();
            sb.AppendLine("}");

            bool firstHeader = true;

            foreach (string header in Environment.TargetConfig.AdditionalSourceHeaders)
            {
                if (firstHeader)
                {
                    firstHeader = false;
                    sb.AppendLine();
                    sb.AppendLine("#include <sexy.types.h>"); 
                    sb.AppendLine("#include <sexy.vm.h>");
                    sb.AppendLine("#include <sexy.vm.cpu.h>");
                    sb.AppendLine("#include <sexy.script.h>");
                    sb.AppendLine();
                }
                sb.AppendFormat("#include \"{0}\"", header);
                sb.AppendLine();
            }
        }

        public void AppendTypeFunctions()
        {
            var sb = SourceBuilder;

            if (!CPPCore.HasEnumDefs(Rules.CppSource)) return;

            const int MIN_ELEMENTS_FOR_MAP = 8;

            sb.AppendLine();

            bool includedHeaders = false;

            foreach (var def in CPPCore.EnumDefs)
            {
                if (def.Value.PrimarySourceFile == Rules.CppSource && def.Value.Keys.Length >= MIN_ELEMENTS_FOR_MAP)
                {
                    sb.AppendLine("#include <string>");

                    sb.AppendLine("#include <unordered_map>");
                    sb.AppendLine();
                    includedHeaders = true;
                    break;
                }
            }

            if (!includedHeaders && !HasInterfaceForRow)
            {
                sb.Append("#include <string.h>"); // for strcmp
                sb.AppendLine();
            }

            sb.AppendFormat("namespace {0}", CppNamespace);
            sb.AppendLine();
            sb.AppendLine("{");

            bool firstDef = true;

            foreach(var def in CPPCore.EnumDefs)
            {
                if (def.Value.PrimarySourceFile != Rules.CppSource)
                {
                    continue;
                }

                if (firstDef)
                {
                    firstDef = false;
                }
                else
                {
                    sb.AppendLine();
                }

                AppendTab(sb);
                sb.AppendFormat("fstring ToString({0} value)", def.Key);
                sb.AppendLine();
                AppendTab(sb);
                sb.AppendLine("{");

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("using enum {0};", def.Key);
                sb.AppendLine();
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("switch(value)");
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("{");

                foreach (var item in def.Value.Keys)
                {
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendFormat("case {0}: return \"{0}\"_fstring;", item);
                    sb.AppendLine();
                }

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("default: return {{nullptr,0}};");
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("}");

                AppendTab(sb);
                sb.AppendLine("}");

                sb.AppendLine();

                AppendTab(sb);
                sb.AppendFormat("bool TryParse(const fstring& text, {0}& result)", def.Key);
                sb.AppendLine();
                AppendTab(sb);
                sb.AppendLine("{");

                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("using enum {0};", def.Key);
                sb.AppendLine();

                AppendTab(sb);
                AppendTab(sb);

                if (def.Value.Keys.Length >= MIN_ELEMENTS_FOR_MAP)
                {
                    sb.AppendFormat("static std::unordered_map<std::string, {0}> bindings = {{", def.Key);
                    sb.AppendLine();
                }
                else
                {
                    sb.AppendFormat("struct Binding {{ cstr key; {0} value; }}; ", def.Key);
                    sb.AppendLine();

                    AppendTab(sb);
                    AppendTab(sb);

                    sb.Append("static Binding bindings[] = {");
                    sb.AppendLine();
                }


                bool first = true;

                int count = 1;
                bool startingNewLine = true;

                foreach(var value in def.Value.Keys)
                {
                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        sb.Append(",");

                        startingNewLine = (count++ % 4) == 0;

                        if (startingNewLine)
                        {
                            sb.AppendLine();
                        }
                    }

                    if (startingNewLine)
                    {
                        AppendTab(sb);
                        AppendTab(sb);
                        AppendTab(sb);
                    }

                    sb.Append("{");
                    sb.AppendFormat("\"{0}\", {0}", value);
                    sb.Append("}");
                }

                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("};");

                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);

                if (def.Value.Keys.Length >= MIN_ELEMENTS_FOR_MAP)
                {
                    sb.AppendLine("auto i = bindings.find(text.buffer);");

                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("if (i != bindings.end())");
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("{");
                    AppendTab(sb);
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("result = i->second;");
                    AppendTab(sb);
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("return true;");
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("}");
                }
                else
                {
                    sb.AppendLine("for(auto& b: bindings)");
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("{");
                    AppendTab(sb);
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("if (strcmp(b.key, text) == 0)");
                    AppendTab(sb);
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("{");
                    AppendTab(sb);
                    AppendTab(sb);
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("result = b.value;");
                    AppendTab(sb);
                    AppendTab(sb);
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("return true;");
                    AppendTab(sb);
                    AppendTab(sb);
                    AppendTab(sb);

                    sb.AppendLine("}");
                    AppendTab(sb);
                    AppendTab(sb);
                    sb.AppendLine("}");

                }
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("result = {0}::{1};", def.Key, def.Value.Keys[0]);
                sb.AppendLine();
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendLine("return false;");
                AppendTab(sb);
                sb.AppendLine("}");
            }

            sb.AppendLine("}");
        }

        public void GoProtected()
        {
            SourceBuilder = CPPCore.OpenFile(FullSourcePath);
            HeaderBuilder = CPPCore.OpenFile(FullHeaderPath);

            if (HeaderBuilder.Length == 0)
            {
                // This is the first time this file has been opened, so we need to append the include statements
                AppendHeaderIncludes();
            }

            AppendInterface();

            if (SourceBuilder.Length == 0)
            {
                // This is the first time this file has been opened, so we need to append the include statements
                AppendSourceIncludes();
            }

            AppendTableImplementation();

            AppendTypeFunctions();
        }

        public void Go()
        {
            try
            {
                GoProtected();
            }
            catch (Exception ex)
            {
                throw new Exception("Worksheet '" + Table.TableName + "' with rules '" + Rules.RuleSetName + "' raised an exception", ex);
            }
        }
    }
}
