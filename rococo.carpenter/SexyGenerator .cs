﻿using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace Rococo.Carpenter
{
    public static class SexyCore
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

        public static void Commit()
        {
            foreach (var i in openFiles)
            {
                File.WriteAllText(i.Key, i.Value.ToString());
            }
        }

        public static void AppendSexyVariableNameChar(StringBuilder sb, char c)
        {
            if (char.IsNumber(c))
            {
                sb.Append(c);
            }
            else if (c >= 'A' && c <= 'Z')
            {
                sb.Append(c);
            }
            else if (c >= 'a' && c <= 'z')
            {
                sb.Append(c);
            }
        }


        public static string ToSexyVariableName(string columnName)
        {
            StringBuilder sb = new StringBuilder(16);
           
            if (columnName == null || columnName.Length == 0)
            {
                throw new Exception("No column name");
            }

            char firstChar = columnName[0];
            if (!char.IsUpper(firstChar))
            {
                sb.Append("the");
            }
            else
            {
                AppendSexyVariableNameChar(sb, char.ToLower(firstChar));
            }

            for(int i = 1; i < columnName.Length; i++)
            {
                AppendSexyVariableNameChar(sb, columnName[i]);
            }

            return sb.ToString();
        }

        public static string ColumnTypeInfoToSexyVariableType(ColumnDesc desc)
        {
            StringBuilder sb = new StringBuilder();

            foreach(char c in desc.TypeInfo)
            {
                if (c != '-')
                {
                    sb.Append(c);
                }
            }

            return sb.ToString();
        }
    }


    public class SexyGenerator
    {
        public string Repository
        {
            get; private set;
        }

        public string FullSXHPath
        {
            get; private set;
        }

        public string FullXCPath
        {
            get
            {
                string value = FullSXHPath;
                int sxhPos = value.LastIndexOf(".sxh");
                if (sxhPos == -1)
                {
                    return value + ".xc";
                }
                else
                {
                    return value.Substring(0, sxhPos) + ".xc";
                }
            }
        }

        public string ProjectRelativeXCPath
        {
            get
            {
                string value = FullXCPath;
                int lastSlash = value.LastIndexOf(Path.DirectorySeparatorChar);
                if (lastSlash == - 1)
                {
                    return value;
                }

                return value.Substring(lastSlash + 1);
            }
        }

        public StringBuilder SXHBuilder
        {
            get; private set;
        }

        public StringBuilder XCBuilder
        {
            get; private set;
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

        public void SetFullSXHPath(string newSXHPath)
        {
            if (newSXHPath == null || newSXHPath.Length == 0)
            {
                throw new Exception("Bad rule: missing Sexy Source");
            }

            if (!newSXHPath.EndsWith(".sxh"))
            {
                throw new Exception("Bad rule: 'Sexy Header' must specify a file with either .sxh extension");
            }

            FullSXHPath = Repository + newSXHPath;
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

        private CPPGenerator CPP
        {
            get;
            set;
        }


        public SexyGenerator(CPPGenerator cpp, ITypes types, IMetaData metaData, ITable table, IRules rules)
        {
            Types = types;
            MetaData = metaData;
            Table = table;
            Rules = rules;
            CPP = cpp;

            SetRepository(rules.CppRepo);
            SetFullSXHPath(rules.SexyHeader);
        }

        private void GoProtected()
        {
            SXHBuilder = SexyCore.OpenFile(FullSXHPath);

            if (SXHBuilder.Length == 0)
            {
                // This is the first time this file has been opened, so we need to append the introduction statements
                AppendIntroduction(SXHBuilder);
            }

            AppendInterface(SXHBuilder);

            XCBuilder = SexyCore.OpenFile(FullXCPath);

            if (XCBuilder.Length == 0)
            {
                AppendBasis(XCBuilder);
            }

            AppendTypes(XCBuilder);
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

        private void AppendBasis(StringBuilder sb)
        {
            sb.AppendLine(Environment.XCBase);
        }

        private void AppendIntroduction(StringBuilder sb)
        {
            sb.AppendLine("// Generated by rococo.carpenter on " + DateTime.Now.ToString("HH:mm:ss d MMM yyyy"));
            sb.AppendFormat("(config ${0})", ProjectRelativeXCPath);
            sb.AppendLine();
        }

        private void AppendTab(StringBuilder sb)
        {
            sb.Append("\t");
        }

        private string SexyInterfaceName
        {
            get
            {
                return string.Format("{0}.{1}", Rules.CppNamespace, Rules.CppInterface);
            }
        }

        private string TableRowName
        {
            get
            {
                bool isFirst = false;

                StringBuilder sb = new StringBuilder(Table.TableName.Length + 4);
                foreach(char c in CPP.RowStructName)
                {
                    if (c >= 'A' && c <= 'Z')
                    {
                        sb.Append(c);
                        isFirst = false;
                    }
                    else if (c >= 'a' && c <= 'z')
                    {
                        if (isFirst)
                        {
                            sb.Append(Char.ToUpper(c));
                            isFirst = false;
                        }

                        sb.Append(c);
                    }
                    else if (c >= '0' && c <= '9')
                    {
                        if (isFirst)
                        {
                            // If a table's first character is a number we still allow it, just prefix with TableId or some other string
                            sb.Append("TableId");
                            isFirst = false;
                        }
                        sb.Append(c);
                    }
                }

              //  sb.Append("Row");

                return sb.ToString();
            }
        }

        private void AppendInterface(StringBuilder sb)
        {
            sb.AppendLine("(interface");

            AppendTab(sb);
            sb.AppendFormat("(as.sxy {0} \"../packages/tables/Tables/table\")", SexyInterfaceName);
            sb.AppendLine();

            AppendTab(sb);
            sb.AppendFormat("(as.cpp {0} \"tables\")", SexyInterfaceName + "_Sexy");
            sb.AppendLine();

            AppendTab(sb);
            sb.AppendFormat("(context factory {0}_Sexy)", SexyInterfaceName);
            sb.AppendLine();

            AppendTab(sb);
            sb.AppendFormat("(methods");
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);

            sb.AppendFormat("(GetRow (Int32 index)({0} row))", TableRowName);
            sb.AppendLine();

            AppendTab(sb);
            AppendTab(sb);
            sb.AppendFormat("(NumberOfRows -> (Int32 numberOfRows))");
            sb.AppendLine();

            AppendTab(sb);
            sb.AppendLine(")");

            AppendTab(sb);
            sb.AppendFormat("(factory {0}.Get{1})", CPP.CppNamespace.Replace("::", "."), CPP.CppTableName.Replace("_", ""));
            sb.AppendLine();


            sb.AppendLine(")");
        }
        public string FQTableRowName
        {
            get
            {
                return CPP.CppNamespace.Replace("::", ".") + "." + TableRowName;
            }                
        }

        void AppendTypes(StringBuilder sb)
        {
            HashSet<string> primitives = new HashSet<string>();

            bool hasString = false;

            foreach (var column in Table.Columns)
            {
                string typePrefix = "Type-";
                string enumPrefix = "Enum-";
                if (column.TypeInfo.StartsWith(typePrefix))
                {
                    if (primitives.Add(column.TypeInfo))
                    {
                        TypeDefinition def;
                        if (!Types.TryGetType(column.TypeInfo.Substring(typePrefix.Length), out def))
                        {
                            throw new Exception("Unknown type: " + column.TypeInfo);
                        }

                        string sexyType = def.NameSpace + "." + def.LocalName;
                        string cppType = def.NameSpace + "." + def.LocalName;

                        sb.AppendFormat("(primitive {0} {1} {2})", SexyCore.ColumnTypeInfoToSexyVariableType(column), sexyType, cppType);
                        sb.AppendLine();
                    }
                }
                else if (column.TypeInfo.StartsWith(enumPrefix))
                {
                    EnumDef enumDef;
                    if (CPPCore.EnumDefs.TryGetValue(column.TypeInfo.Substring(enumPrefix.Length), out enumDef))
                    {
                        if (primitives.Add(enumDef.Name))
                        {
                            string ns = Rules.CppNamespace;
                            string sexyType = ns + "." + enumDef.Name;
                            string cppType = ns + "." + enumDef.Name;

                            sb.AppendFormat("(primitive {0} {1} {2})", SexyCore.ColumnTypeInfoToSexyVariableType(column), sexyType, cppType);
                            sb.AppendLine();
                        }
                    }
                }
            }

            if (primitives.Count > 0)
            {
                sb.AppendLine();
            }

            sb.AppendFormat("(defstruct {0} {1} {2}", TableRowName, FQTableRowName, FQTableRowName);
            sb.AppendLine();

            foreach(var column in  Table.Columns)
            {
                AppendTab(sb);
                sb.AppendFormat("({0} {1})", SexyCore.ColumnTypeInfoToSexyVariableType(column), SexyCore.ToSexyVariableName(column.Title));
                sb.AppendLine();
            }

            sb.AppendLine(")");
        }
    }
}