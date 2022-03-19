using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace Rococo.Carpenter
{
    public class EnumObject
    {
        private List<string> values = new List<string>();
        public void Add(string fieldName)
        {
            foreach(string s in values)
            {
                if (s == fieldName)
                {
                    return;
                }
            }

            values.Add(fieldName);
        }
    }

    public static class CPPCore
    {
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

        public static Dictionary<string, EnumObject> enums = new Dictionary<string, EnumObject>();

        public static EnumObject Find(string enumObjectName)
        {
            EnumObject obj;
            if (!enums.TryGetValue(enumObjectName, out obj))
            {
                return null;
            }

            return obj;
        }

        public static void Add(string enumObjectName, string fieldName)
        {
            EnumObject obj;
            if (!enums.TryGetValue(enumObjectName, out obj))
            {
                obj = new EnumObject();
                enums.Add(enumObjectName, obj);
                obj.Add(fieldName);
            }

            obj.Add(fieldName);
        }

        public static string ExcelSource
        {
            get;set;
        }

        public static string GetCppType(UnderlyingType type, bool isMutable)
        {
            switch(type)
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
                    return "int64;";
                case UnderlyingType.Bool:
                    return "bool";
                default:
                    throw new Exception("CPPCore.GetCppType(...): do not know how to handle " + type);
            }
        }

        public static void Commit()
        {
            foreach(var i in openFiles)
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
        private List<string> keys = new List<string> ();
        private HashSet<string> uniqueKeys = new HashSet<string> ();
        public void AddKey(string item)
        {
            if (uniqueKeys.Contains (item))
            {
                return;
            }

            uniqueKeys.Add(item);
            keys.Add(item);

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
    }
    public class CPPGenerator
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

        public UnderlyingType[] FieldTypes
        {
            get;private set;
        }

        public string[] FieldNames
        {
            get; private set;
        }

        public string[] EnumNames
        {
            get; private set;
        }

        public string[] FullTypeNames
        {
            get; private set;
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
            RowStructName = interfaceName.Substring(1) + "_Row";
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

                List<string> fieldNames = new List<string>();
                List<UnderlyingType> underlyingTypes = new List<UnderlyingType>();
                List<string> enumNames = new List<string>();
                List<string> fullTypes = new List<string>();

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

                    fieldNames.Add(String.Format("{0}{1}", char.ToLower(q), c.Title.Substring(1)));

                    UnderlyingType type;
                    if (Enum.TryParse(c.TypeInfo, out type))
                    {
                        underlyingTypes.Add(type);
                        enumNames.Add(null);
                        fullTypes.Add(CPPCore.GetCppType(type, Rules.IsMutable));
                        continue;
                    }

                    if (c.TypeInfo == "Index")
                    {
                        underlyingTypes.Add(UnderlyingType.Int32);
                        enumNames.Add(null);
                        fullTypes.Add(c.TypeInfo);
                        continue;
                    }

                    string enumType;
                    if (TryParseAsEnumType(c.TypeInfo, out enumType))
                    {
                        underlyingTypes.Add(UnderlyingType.Int32);
                        enumNames.Add(enumType);
                        fullTypes.Add(enumType);
                        continue;
                    }

                    if (!TryParseAsAliasedType(c.TypeInfo, out type))
                    {
                        throw new Exception("Unknown type in column '" + c.TypeInfo + "'");
                    }

                    string aliasPrefix = "Type-";

                    underlyingTypes.Add(type);
                    enumNames.Add(null);
                    fullTypes.Add(c.TypeInfo.Substring(aliasPrefix.Length));
                }

                FieldNames = fieldNames.ToArray();
                FieldTypes = underlyingTypes.ToArray();
                EnumNames = enumNames.ToArray();
                FullTypeNames = fullTypes.ToArray();


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
            sb.AppendLine();
        }

        private readonly string tab = "    ";

        public void AppendTab(StringBuilder sb)
        {
            sb.Append(tab);
        }

        public void AppendRowField(int fieldIndex, StringBuilder sb)
        {
            sb.AppendFormat("{0} {1}", FullTypeNames[fieldIndex], FieldNames[fieldIndex]);
        }

        Dictionary<string, EnumDef> enumDefs = new Dictionary<string, EnumDef>();

        public void AppendEnumDef(StringBuilder sb, string enumName)
        {
            EnumDef def;
            if (enumDefs.TryGetValue(enumName, out def))
            {
                return;
            }

            def = new EnumDef();

            enumDefs.Add(enumName, def);

            for(int i = 0; i < EnumNames.Length; i++)
            {
                if (EnumNames[i] == enumName)
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

            sb.AppendFormat("enum class {0}: {1}", enumName, typeDef.SysType.ToString());
            sb.Append(enumName);

            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("{");

            bool first = true;

            foreach(var item in def.Keys)
            {
                if (first)
                {
                    first = false;
                }
                else 
                {
                    sb.AppendLine(", ");
                }

                AppendTab(sb);
                AppendTab(sb);


                sb.Append(item.Replace(" ", ""));
            }

            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("};");
            sb.AppendLine();
        }

        public void AppendInterface()
        {
            var sb = HeaderBuilder;

            sb.Append("namespace ");
            sb.AppendLine(CppNamespace);
            sb.AppendLine("{");

            for(int i = 0; i < FieldTypes.Length; ++i)
            {
                string enumName = EnumNames[i];
                if (enumName != null)
                {
                    AppendEnumDef(sb, enumName);
                }                
            }

            AppendTab(sb);
            sb.AppendFormat("struct {0}", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("{");

            var columns = Table.Columns;
            for(int i = 0; i < FieldTypes.Length; ++i)
            {
                AppendTab(sb);
                AppendTab(sb);
                AppendRowField(i, sb);
                sb.AppendLine(";");
            }

            AppendTab(sb);
            sb.AppendLine("};");
            sb.AppendLine();

            AppendTab(sb);
            sb.AppendFormat("ROCOCOAPI {0}", MetaDataInterfaceName);
            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("{");

            foreach(var ms in MetaData.StringVariables)
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
            sb.AppendLine(Rules.CppInterface);
            AppendTab(sb);
            sb.AppendLine("{");
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("virtual const {0}& GetRow(int32 index) const = 0;", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("virtual const int32 NumberOfRows() const = 0;", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("virtual const {0}* begin() const = 0;", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("virtual const {0}* end() const = 0;", RowStructName);
            sb.AppendLine();

            if (Rules.MetaDataAccess == MetaDataAccessType.Composition)
            {
                AppendTab(sb);
                AppendTab(sb);
                sb.AppendFormat("virtual const {0}& Meta() const = 0;", MetaDataInterfaceName);
                sb.AppendLine();
                AppendTab(sb);
            }

            sb.AppendLine("};");

            sb.AppendLine();

            if (Rules.Lifetime == TableLifetime.Dynamic)
            {
                sb.AppendLine();
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
            else // Static or singleton
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
                default:
                    sb.Append(item);
                    break;
            }
        }

        public void AppendTableImplementation()
        {
            var sb = SourceBuilder;

            sb.AppendFormat("static {0}{1} defaultRows[{2}] = ", Rules.IsMutable ? "" : "const ", RowStructName, Table.NumberOfRows);
            sb.AppendLine();
            sb.AppendLine("{");

            for(int i = 0; i < Table.NumberOfRows; i++)
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

                    if (EnumNames[j] != null)
                    {
                        sb.AppendFormat("{0}::{1}", EnumNames[j], item.Replace(" ", ""));
                    }
                    else
                    {
                        AppendSanitizedItem(sb, item, FieldTypes[j]);
                    }
                }

                sb.AppendLine();

                AppendTab(sb);
                sb.Append("}");
            }

            sb.AppendLine();
            sb.AppendLine("};");
            sb.AppendLine();

            sb.AppendLine("namespace ANON");
            sb.AppendLine("{");
            AppendTab(sb);
            sb.AppendFormat("using namespace {0};", CppNamespace);
            sb.AppendLine();
            sb.AppendLine();
            AppendTab(sb);
            sb.AppendFormat("struct {0}_Implementation: {1}", CppTableName, Rules.CppInterface);

            if (Rules.MetaDataAccess == MetaDataAccessType.Composition)
            {
                sb.AppendFormat(", {0}", MetaDataInterfaceName);
            }

            sb.AppendLine();
            AppendTab(sb);
            sb.AppendLine("{");

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("const {0}& GetRow(int32 index) const override", RowStructName);
            sb.AppendLine();
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("{");

            AppendTab(sb);
            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("return defaultRows[index];");
            sb.AppendLine();

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
            sb.AppendFormat("return {0};", Table.NumberOfRows);
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendLine("}");
            sb.AppendLine();

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
                sb.AppendFormat("return \"{0}\"", ms.Value.value);
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

                sb.AppendFormat("{0}Supervisor* {1}();", Rules.CppInterface, Rules.CppFactory);
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
            else // Static or singleton
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
