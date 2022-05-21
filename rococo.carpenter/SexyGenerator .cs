using System;
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
                foreach(char c in Table.TableName)
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

                sb.Append("Row");

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
            sb.AppendFormat("(as.cpp {0} \"tables\")", SexyInterfaceName);
            sb.AppendLine();

            AppendTab(sb);
            sb.AppendFormat("(context factory {0})", SexyInterfaceName);
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
            sb.AppendFormat("(struct {0} {1} {2})", TableRowName, FQTableRowName, FQTableRowName);
            sb.AppendLine();
        }
    }
}
