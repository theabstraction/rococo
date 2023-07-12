using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace Rococo.Carpenter
{
    public class BinaryGenerator
    {
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

        public IMapFullTablePathToResource MapTableToResource
        {
            get;
            private set;
        }

        public EnumDef[] EnumColumns
        {
            get;
            private set;
        }

        public IParsedColumnData Columns;

        private Stream memoryStream;
        private BinaryWriter writer;

        public BinaryGenerator(ITypes types, IMetaData metaData, ITable table, IRules rules, IParsedColumnData columns, IMapFullTablePathToResource mapTableToResource)
        {
            this.Types = types;
            this.MetaData = metaData;
            this.Table = table;
            this.Rules = rules;
            this.Columns = columns;
            this.MapTableToResource = mapTableToResource;

            EnumColumns = new EnumDef[columns.ColumnHeaders.Length];

            for (int i = 0; i < columns.ColumnHeaders.Length; i++)
            {
                if (Columns.ColumnHeaders[i].EnumName != null)
                {
                    EnumColumns[i] = CPPCore.FindEnumDef(Columns.ColumnHeaders[i].EnumName);
                }
            }

            memoryStream = new MemoryStream(8192);
            writer = new BinaryWriter(memoryStream, Encoding.UTF8);
            MapTableToResource = mapTableToResource;
        }

        public void SerializeEnum(EnumDef def, int index)
        {
            writer.Write(index);
        }

        public void Serialize(UnderlyingType type, string text)
        {
            switch(type)
            {
                case UnderlyingType.String:
                    writer.Write(text);
                    break;
                case UnderlyingType.Bool:
                    writer.Write(text == "TRUE" ? (byte) 1 : (byte) 0);
                    break;
                case UnderlyingType.Int64:
                    Int64 ill;
                    Int64.TryParse(text, out ill);
                    writer.Write(ill);
                    break;
                case UnderlyingType.Int32:
                    int i;
                    int.TryParse(text, out i);
                    writer.Write(i);
                    break;
                case UnderlyingType.Float32:
                    float f;
                    float.TryParse(text, out f);
                    writer.Write(f);
                    break;
                case UnderlyingType.Float64:
                    double g;
                    double.TryParse(text, out g);
                    writer.Write(g);
                    break;
                default:
                    throw new Exception("Unknown type: " + type);
            }
        }

        public void ParseRow(IItemSequence row)
        {
            for (int i = 0; i < row.Length; i++)
            {
                try
                {
                    string text = row.GetItemText(i);

                    var enumColumn = EnumColumns[i];
                    if (enumColumn != null)
                    {
                        int index;
                        if (!enumColumn.TryGetIndex(text.Replace(" ", ""), out index))
                        {
                            throw new Exception("Expecting item '" + text + "' to be an enum of type " + enumColumn.Name);
                        }

                        SerializeEnum(enumColumn, index);
                    }
                    else
                    {
                        Serialize(Columns.ColumnHeaders[i].UnderlyingType, text);
                    }
                }
                catch (Exception ex)
                {
                    throw new Exception("Exception parsering cell #" + (char)('A' + i), ex);
                }
            }
        }

        public void Go()
        {
            writer.Write("Rococo.Carpenter.BinaryTable");
            writer.Write("1.0.0.0");
            writer.Write("[BeginMagic");
            writer.Write(0x12345678);
            writer.Write(0x12345678ABCDEF42);
            writer.Write((float)Math.PI);
            writer.Write(Math.PI);
            writer.Write("EndMagic]");
            writer.Write(String.Format("Source: {0}", CPPCore.ExcelSource));
            writer.Write(String.Format("Table: {0}", Table.TableName));
            writer.Write(Table.NumberOfRows);
            writer.Write("[BeginColumns:");
            writer.Write(Columns.ColumnHeaders.Length);
            writer.Write("Types");
            for(int j = 0; j < Columns.ColumnHeaders.Length; j++)
            {
                writer.Write(Columns.ColumnHeaders[j].UnderlyingType.ToString());
            }

            writer.Write("C++Types");
            for (int j = 0; j < Columns.ColumnHeaders.Length; j++)
            {
                writer.Write(Columns.ColumnHeaders[j].FullTypeName);
            }

            writer.Write("Titles");
            for (int j = 0; j < Columns.ColumnHeaders.Length; j++)
            {
                writer.Write(Columns.ColumnHeaders[j].Name);
            }

            writer.Write("EndColumns]");
            writer.Write("[BeginRows");

            for (int i = 0; i < Table.NumberOfRows; i++)
            {
                var row = Table.GetRow(i);

                try
                {
                    writer.Write(i);
                    ParseRow(row);
                }
                catch (Exception ex)
                {
                    throw new Exception("Exception processing row #" + (i + 3) + " of " + Table.TableName, ex);
                }
            }
            writer.Write("EndRows]");

            string pingPath = CPPCore.GetPingPathArchiveName(MapTableToResource, Table);
            Console.WriteLine("Writing binary file " + pingPath);

            string sysPath = Environment.PingPathToSysPath(pingPath);

            byte[] buffer = new byte[memoryStream.Length];
            memoryStream.Seek(0, SeekOrigin.Begin);
            memoryStream.Read(buffer, 0, buffer.Length);
            System.IO.File.WriteAllBytes(sysPath, buffer);
        }
    }
}
