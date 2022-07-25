using System;
using System.Collections.Generic;
using System.Text;

using System.Linq;
using DocumentFormat.OpenXml;
using DocumentFormat.OpenXml.Packaging;
using DocumentFormat.OpenXml.Spreadsheet;


namespace Rococo.Carpenter
{
    public struct SemanticAttribute
    {
        public string Name { get; private set; }
        public Type Type { get; private set; }

        public SemanticAttribute(string name, Type type)
        {
            Name = name;
            Type = type;
        }

        public override string ToString()
        {
            return Type.Name + ": "  + Name;
        }
    }

    public delegate bool Delegate_OnInvoke(string[] arguments, Semantic semantic);

    public class Semantic
    {
        public string Key
        {
            get;
            private set;
        }

        public SemanticAttribute[] AttributeKeys
        {
            get;
            private set;
        }

        public Delegate_OnInvoke OnInvoke
        {
            get;
            private set;
        }

        public Semantic(string key, SemanticAttribute[] attributeKeys, Delegate_OnInvoke onInvoke)
        {
            this.Key = key;
            this.AttributeKeys = attributeKeys;
            this.OnInvoke = onInvoke;
        }

        public bool Invoke(SharedStringTable strings, List<Cell> cells)
        {
            string[] arguments = new string[cells.Count - 1];

            for (int i = 0; i < arguments.Length; i++)
            {
                arguments[i] = AsString(strings, cells[i + 1]);
            }

            return OnInvoke(arguments, this);
        }

        private string AsString(SharedStringTable strings, Cell cell)
        {
            var value = cell.InnerText;

            if (strings != null && cell.DataType == CellValues.SharedString)
            {                
                return strings.ElementAt(int.Parse(value)).InnerText;
            }
            else
            {
                return cell.InnerText;
            }
        }

        public bool TryParse(SharedStringTable strings, List<Cell> cells)
        {
            var keyCell = cells[0];

            string key = AsString(strings, keyCell);
            
            if (key != Key)
            {
                return false;
            }

            if (cells.Count() < 1 + AttributeKeys.Length)
            {
                return false;
            }

            return Invoke(strings, cells);
        }

        public bool TryMatchFloat(out float value, int index, string[] args)
        {
            value = 0;

            if (args.Length <= index)
            {
                return false;
            }

            if (AttributeKeys.Length == 0)
            {
                return false;
            }

            int attrIndex = (index >= AttributeKeys.Length) ? AttributeKeys.Length - 1 : index;

            if (AttributeKeys[attrIndex].Type != typeof(float))
            {
                return false;
            }

            return float.TryParse(args[index], out value);
        }

        public bool TryMatchString(out string value, int index, string[] args)
        {
            value = string.Empty;

            if (args.Length <= index)
            {
                return false;
            }

            if (AttributeKeys.Length == 0)
            {
                return false;
            }

            int attrIndex = (index >= AttributeKeys.Length) ? AttributeKeys.Length - 1 : index;

            if (AttributeKeys[attrIndex].Type != typeof(string))
            {
                return false;
            }

            value = args[index];
            return true;
        }

        public bool TryMatchEnum<T>(out T value, int index, string[] args) where T : struct
        {
            string sValue;
            if (!TryMatchString(out sValue, index, args))
            {
                value = default(T);
                return false;
            }

            return Enum.TryParse<T>(sValue, out value);
        }

        public override string ToString()
        {
            return "Semantic '" + Key + "'";
        }
    }
    public class RowDirectiveParser
    {
        List<Semantic> semantics = new List<Semantic>();

        public RowDirectiveParser()
        {
        }

        public void AddSemantic(Semantic semantic)
        {
            semantics.Add(semantic);
        }

        public void Parse(SharedStringTable strings, Row row)
        {
            var cells = row.Elements<Cell>().ToList();
            if (cells.Count == 0)
            {
                return;
            }

            var keyCell = cells[0];

            foreach(var semantic in semantics)
            {
                try
                {
                    if (semantic.TryParse(strings, cells))
                    {
                        return;
                    }
                }
                catch (SemanticException ex)
                {
                    string cellRef = string.Empty;

                    int cellIndex = ex.Index + 1;
                    if (cellIndex <= cells.Count)
                    {
                        cellRef = cells[cellIndex].CellReference;
                    }

                    throw new Exception("Semantic error at " + cellRef, ex);
                }
            }
        }
    }

    public abstract class ExcelSheet
    {
        SpreadsheetDocument doc;
        protected RowDirectiveParser parser = new RowDirectiveParser();

        public SheetData Data
        {
            get;
            private set;
        }

        public Sheet SheetMeta
        {
            get;
            private set;
        }

        public WorksheetPart SheetPart
        {
            get;
            private set;
        }
        public SharedStringTable Strings
        {
            get;
            private set;
        }
        public string AsString(Cell cell)
        { 
            if (Strings != null && cell.DataType != null && cell.DataType == CellValues.SharedString)
            {
                return Strings.ElementAt(int.Parse(cell.InnerText)).InnerText;
            }
            else
            {
                return cell.InnerText;
            }
        }

        public override string ToString()
        {
            return GetType().Name + ": " + SheetMeta.Name;
        }

        public ExcelSheet(SpreadsheetDocument doc, Sheet sheet, SharedStringTable strings)
        {
            this.doc = doc;
            this.SheetMeta = sheet;
            this.Strings = strings;

            WorkbookPart workbookPart = doc.WorkbookPart;
            SheetPart = ((WorksheetPart)workbookPart.GetPartById(sheet.Id));
            Worksheet worksheet = SheetPart.Worksheet;
            Data = worksheet.GetFirstChild<SheetData>();
        }

        public void PassRowsToParser()
        {
            foreach (var row in Data.Elements<Row>())
            {
                try
                {
                    parser.Parse(Strings, row);
                }
                catch(Exception ex)
                {
                    throw new ArgumentException("Semantic error in WorkSheet " + SheetMeta.Name, ex);
                }
            }
        }

        public abstract void Parse();
    }

    public class ExcelSpecSheet : ExcelSheet
    {
        public string Version { get; set; }

        private bool OnVersion(string[] args, Semantic semantic)
        {
            string version;
            if (!semantic.TryMatchString(out version, 0, args))
            {
                return false;
            }

            Version = version;

            return true;
        }

        public ExcelSpecSheet(SpreadsheetDocument doc, Sheet sheet, SharedStringTable strings) : base(doc, sheet, strings)
        {
            parser.AddSemantic(new Semantic("Version", new SemanticAttribute[] { new SemanticAttribute("VersionNumber", typeof(string)) }, OnVersion));
        }

        public override void Parse()
        {
            PassRowsToParser();
        }
    }

    public enum UnderlyingType
    {
        String,
        Float32,
        Float64,
        Int32,
        Int64,
        Bool
    }

    public struct TypeDefinition
    {
        public string NameSpace { get; set; }
        public string LocalName { get; set; }
        public UnderlyingType SysType { get; set; }

        public override string ToString()
        {
            return "Define " + NameSpace + "." + LocalName + " as " + SysType;
        }
    }

    public interface ITypes
    {
        bool TryGetType(string name, out TypeDefinition def);
    }

    public class ExcelTypesSheet : ExcelSheet, ITypes
    {
        Dictionary<string, TypeDefinition> types = new Dictionary<string, TypeDefinition>();
        private bool OnDefine(string[] args, Semantic semantic)
        {
            string ns;
            if (!semantic.TryMatchString(out ns, 0, args))
            {
                return false;
            }

            string name;
            if (!semantic.TryMatchString(out name, 1, args))
            {
                return false;
            }

            UnderlyingType sysType;
            if (!semantic.TryMatchEnum(out sysType, 2, args))
            {
                return false;
            }

            types.Add(name, new TypeDefinition { NameSpace = ns, LocalName = name, SysType = sysType });

            return true;
        }

        public ExcelTypesSheet(SpreadsheetDocument doc, Sheet sheet, SharedStringTable strings) : base(doc, sheet, strings)
        {
            parser.AddSemantic(
                new Semantic("Define", new SemanticAttribute[]
                { 
                    new SemanticAttribute("Namespace", typeof(string)),
                    new SemanticAttribute("LocalName", typeof(string)),
                    new SemanticAttribute("UnderlyingType", typeof(string))
                },
                OnDefine)
            );
        }

        public override void Parse()
        {
            PassRowsToParser();
        }

        public bool TryGetType(string name, out TypeDefinition def)
        {
            return types.TryGetValue(name, out def);
        }
    }

    public struct VariableDef<T>
    {
        public string name;
        public bool isImmutable;
        public T value;
    }

    class SemanticException: Exception
    {
        public int Index { get; private set; }
        public SemanticException(int index, string message): base(message)
        {
            Index = index;
        }
    }

    public interface IMetaData
    {
        public Dictionary<string, VariableDef<string>> StringVariables
        {
            get;
        }
    }

    public class ExcelMetaDataSheet : ExcelSheet, IMetaData
    {
        private Dictionary<string, VariableDef<string>> stringVariables = new Dictionary<string, VariableDef<string>>();

        public Dictionary<string, VariableDef<string>> StringVariables
        {
            get
            {
                return stringVariables;
            }
        }

        private bool OnURLVariable(string[] args, Semantic semantic)
        {
            string value;
            if (!semantic.TryMatchString(out value, 1, args))
            {
                return false;
            }

            if (!OnStringVariable(args, semantic))
            {
                return false;
            }

            if (!value.StartsWith("http://") && !value.StartsWith("https://"))
            {
                throw new SemanticException(1, "Expecting URL to begin with http:// or https://");
            }

            return true;
        }

        private bool OnStringVariable(string[] args, Semantic semantic)
        {
            string key;
            if (!semantic.TryMatchString(out key, 0, args))
            {
                return false;
            }

            string value;
            if (!semantic.TryMatchString(out value, 1, args))
            {
                return false;
            }

            bool isImmutable = false;

            for (int i = 2; i < args.Length; i++)
            {
                string attribute;
                if (!semantic.TryMatchString(out attribute, i, args))
                {
                    if (attribute == "Immutable")
                    {
                        isImmutable = true;
                    }
                }
            }

            VariableDef<string> variableDef = new VariableDef<string>();
            variableDef.name = key;
            variableDef.value = value;
            variableDef.isImmutable = isImmutable;

            stringVariables.Add(key, variableDef);

            return true;
        }

        public ExcelMetaDataSheet(SpreadsheetDocument doc, Sheet sheet, SharedStringTable strings) : base(doc, sheet, strings)
        {
            parser.AddSemantic(
                new Semantic("string", new SemanticAttribute[]
                {
                    new SemanticAttribute("Key", typeof(string)),
                    new SemanticAttribute("Value", typeof(string))
                },
                OnStringVariable)
            );
            parser.AddSemantic(
                new Semantic("url", new SemanticAttribute[]
                {
                    new SemanticAttribute("Key", typeof(string)),
                    new SemanticAttribute("Value", typeof(string))
                },
                OnURLVariable)
            ); 
        }

        public override void Parse()
        {
            PassRowsToParser();
        }
    }

    public struct ColumnDesc
    {
        public static string ExtractColumnRef(string cellRef)
        {
            if (cellRef == null)
            {
                throw new ArgumentException("It is not permitted for the celLRef to be null");
            }

            for (int i = 0; i < cellRef.Length; i++)
            {
                if (char.IsDigit(cellRef[i]))
                {
                    return cellRef.Substring(0, i);
                }
            }

            throw new ArgumentException("Expecting non-digit somewhere in the celLRef ref");
        }

        private string columnRef;
        public string ColumnRef
        {
            get
            {
                return columnRef;
            }
            set
            {
                columnRef = ExtractColumnRef(value);
            }
        }

        public string TypeInfo { get; set; }
        public string Title { get; set; }
    }

    public interface ITable
    {
        ColumnDesc[] Columns
        {
            get;
        }

        int NumberOfRows
        {
            get;
        }

        IItemSequence GetRow(int row);

        string TableName
        {
            get;
        }
    }

    public interface IItemSequence
    {
        int Length
        {
            get;
        }

        abstract string GetItemText(int index);
    }

    public sealed class ItemSequence: IItemSequence
    {
        private readonly Cell[] cells;
        private ExcelSheet owner;

        public int Length
        {
            get
            {
                return cells == null ? 0 : cells.Length;
            }
        }

        public ItemSequence(Cell[] cells, ExcelSheet owner)
        {
            this.cells = cells;
            this.owner = owner;
        }

        public string GetItemText(int index)
        {
            if (cells == null)
            {
                return string.Empty;
            }

            var cell = cells[index];
            if (cell == null)
            {
                return string.Empty;
            }

            return owner.AsString(cell);
        }
    }


    public class ExcelTableSheet : ExcelSheet, ITable
    {
        List<Row> xlRows = new List<Row>();

        ColumnDesc[] columns;

        public ExcelTableSheet(SpreadsheetDocument doc, Sheet sheet, SharedStringTable strings) : base(doc, sheet, strings)
        {

        }

        public override void Parse()
        {
            xlRows = Data.Elements<Row>().ToList();

            // We must have at least 2 rows for an empty table - a column-types definition and a titles definition

            // Example of column-types: 'Index          Enum-Unique   Enum-Unique   Float32     Bool    Enum        Float32             Type-ElectronVolts   Float32    Type-Kelvin     Type-Kelvin'
            // Example of titles:       'AtomicNumber	Element       Symbol	    AtomicMass	IsMetal	ElementType	Electronegativity	FirstIonization	     Density	MeltingPoint	BoilingPoint'

            if (xlRows.Count < 2)
            {
                throw new Exception("Table lacked sufficient rows for exporting. Requires at least a column-type row and a title row");
            }

            var row0 = xlRows[0];
            var rowCells0 = row0.Elements<Cell>().ToList();

            var row1 = xlRows[1];
            var rowCells1 = row1.Elements<Cell>().ToList();

            if (rowCells0.Count > rowCells1.Count)
            {
                throw new Exception("Table had " + rowCells0.Count + "column-types" + " but only " + rowCells1.Count + "titles");
            }

            columns = new ColumnDesc[rowCells0.Count];

            for (int i = 0; i < rowCells0.Count; i++)
            {
                columns[i] = new ColumnDesc { TypeInfo = AsString(rowCells0[i]), Title = AsString(rowCells1[i]), ColumnRef = rowCells0[i].CellReference  };
            }
        }

        public ColumnDesc[] Columns
        {
            get
            { 
                return columns; 
            } 
        }

        public int NumberOfRows
        {
            get
            {
                 return xlRows.Count - 2;
            }
        }

        public string TableName
        {
            get 
            {
                return base.SheetMeta.Name;
            }
        }

        // For every column, returns a cell under the column in the given row. If undefined in Excel, yields a null cell
        public Cell[] GetItemsUnderColumns(Row row)
        {
            List<Cell> nonEmptyCells = row.Elements<Cell>().ToList();

            int currentColumn = 0;

            // We have one string for every column
            Cell[] cellsUnderColumns = new Cell[columns.Length];
            for (int i = 0; i < nonEmptyCells.Count; i++)
            {
                var nonEmptyCell = nonEmptyCells[i];

                string columnRef = ColumnDesc.ExtractColumnRef(nonEmptyCell.CellReference);

                for (int j = currentColumn; j < columns.Length; j++)
                {
                    var col = columns[j];
                    if (col.ColumnRef == columnRef)
                    {
                        cellsUnderColumns[j] = nonEmptyCell;
                        currentColumn = j + 1;
                    }
                }
            }

            return cellsUnderColumns;
        }

        public IItemSequence GetRow(int row)
        {
            int xlRowNumber = row + 2;
            if (xlRowNumber >= xlRows.Count)
            {
                throw new Exception("Bad row index: " + row);
            }

            var xlRow = xlRows[xlRowNumber];

            return new ItemSequence(GetItemsUnderColumns(xlRow), this);
        }
    }

    public enum MetaDataAccessType
    {
        None,
        Composition,
        BaseInheritance,
        StandAlone
    }

    public enum TableLifetime
    {
        Static, // Object is a global run time object with no dynamic allocation
        Dynamic, // Object is created with a factory. Multiple instances can be generated
        Singleton // Object is dynamically created when desired, but only one instance is ever available. 
    }

    public interface IRules
    {
        public bool TargetCPP
        {
            get;
        }

        public bool TargetSexy
        {
            get;
        }

        public string CppRepo
        {
            get;
        }

        public string CppNamespace
        {
            get;
        }

        public string CppInterface
        {
            get;
        }

        public string CppFactory
        {
            get;
        }

        public TableLifetime Lifetime
        {
            get;
        }

        public bool IsMutable
        {
            get;
        }

        public MetaDataAccessType MetaDataAccess
        {
            get;
        }

        public string RuleSetName
        {
            get;
        }

        public IEnumerable<string> KeyNames
        {
            get;
        }
    }

    public class ExcelRulesSheet : ExcelSheet, IRules
    {
        public bool TargetCPP
        {
            get;
            private set;
        }

        public bool TargetSexy
        {
            get
            {
                return SexyHeader != null && SexyHeader.EndsWith(".sxh");
            }
        }

        public string CppHeader
        {
            get;
            private set;
        }

        public string SexyHeader
        {
            get;
            private set;
        }

        public string CppSource
        {
            get;
            private set;
        }

        public string CppRepo
        {
            get;
            private set;
        }

        public string CppNamespace
        {
            get;
            private set;
        }

        public string CppInterface
        {
            get;
            private set;
        }

        public string CppFactory
        {
            get;
            private set;
        }

        public TableLifetime Lifetime
        {
            get;
            private set;
        }

        public bool IsMutable
        {
            get;
            private set;
        }

        public MetaDataAccessType MetaDataAccess
        {
            get;
            private set;
        }

        public string RuleSetName
        {
            get
            {
                return base.SheetMeta.Name;
            }
        }

        private HashSet<string> keyNames = new HashSet<string>();

        public IEnumerable<string> KeyNames
        {
            get
            {
                return keyNames;
            }
        }

        private bool OnExport(string[] args, Semantic semantic)
        {
            string target;
            if (!semantic.TryMatchString(out target, 0, args))
            {
                return false;
            }

            if (target == "C++")
            {
                TargetCPP = true;
            }
            else
            {
                throw new SemanticException(0, "Unknown target. Must be 'C=++'");
            }

            return true;
        }

        private bool OnCPPHeader(string[] args, Semantic semantic)
        {
            string value;
            if (!semantic.TryMatchString(out value, 0, args))
            {
                return false;
            }

            CppHeader = value;

            return true;
        }

        private bool OnSexyHeader(string[] args, Semantic semantic)
        {
            string value;
            if (!semantic.TryMatchString(out value, 0, args))
            {
                return false;
            }

            if (!value.EndsWith(".sxh"))
            {
                throw new SemanticException(0, "Expecting Sexy Header to end with '.sxh'");
            }

            SexyHeader = value;

            return true;
        }

        private bool OnCPPLifetime(string[] args, Semantic semantic)
        {
            TableLifetime value;

            if (!semantic.TryMatchEnum(out value, 0, args))
            {
                return false;
            }

            Lifetime = value;

            return true;
        }
        private bool OnKeyName(string[] args, Semantic semantic)
        {
            string columnTitle = string.Empty;

            if (!semantic.TryMatchString(out columnTitle, 0, args))
            {
                return false;
            }

            keyNames.Add(columnTitle);

            return true;
        }

        private bool OnCPPSource(string[] args, Semantic semantic)
        {
            string value;
            if (!semantic.TryMatchString(out value, 0, args))
            {
                return false;
            }

            CppSource = value;

            return true;
        }

        private bool OnCPPRepo(string[] args, Semantic semantic)
        {
            string value;
            if (!semantic.TryMatchString(out value, 0, args))
            {
                return false;
            }

            CppRepo = value;

            return true;
        }

        private bool OnCPPInterface(string[] args, Semantic semantic)
        {
            string value;
            if (!semantic.TryMatchString(out value, 0, args))
            {
                return false;
            }

            CppInterface = value;

            return true;
        }

        private bool OnCPPFactory(string[] args, Semantic semantic)
        {
            string value;
            if (!semantic.TryMatchString(out value, 0, args))
            {
                return false;
            }

            CppFactory = value;

            return true;
        }

        private bool OnMetaData(string[] args, Semantic semantic)
        {
            MetaDataAccessType value;

            if (!semantic.TryMatchEnum(out value, 0, args))
            {
                return false;
            }

            MetaDataAccess = value;

            return true;
        }
        private bool OnCppNamespace(string[] args, Semantic semantic)
        {
            string value;

            if (!semantic.TryMatchString(out value, 0, args))
            {
                return false;
            }

            CppNamespace = value;

            return true;
        }

        private bool OnImmutable(string[] args, Semantic semantic)
        {
            IsMutable = false;
            return true;
        }

        private bool OnMutable(string[] args, Semantic semantic)
        {
            IsMutable = true;
            return true;
        }

        public ExcelRulesSheet(SpreadsheetDocument doc, Sheet sheet, SharedStringTable strings) : base(doc, sheet, strings)
        {
            parser.AddSemantic(
                new Semantic("Export", new SemanticAttribute[]
                {
                    new SemanticAttribute("Target", typeof(string)),
                },
                OnExport)
            );
            parser.AddSemantic(
                new Semantic("C++ Header", new SemanticAttribute[]
                {
                    new SemanticAttribute("C++HeaderFile", typeof(string))
                },
                OnCPPHeader)
            );
            parser.AddSemantic(
               new Semantic("Sexy Header", new SemanticAttribute[]
               {
                    new SemanticAttribute("SexyHeaderFile", typeof(string))
               },
               OnSexyHeader)
           );
            parser.AddSemantic(
               new Semantic("C++ Source", new SemanticAttribute[]
               {
                  new SemanticAttribute("C++SourceFile", typeof(string))
               },
               OnCPPSource)
           );
           parser.AddSemantic(
              new Semantic("C++ Repo", new SemanticAttribute[]
              {
                 new SemanticAttribute("C++Repository", typeof(string))
              },
              OnCPPRepo)
           );
           parser.AddSemantic(
             new Semantic("Interface", new SemanticAttribute[]
             {
                 new SemanticAttribute("InterfaceName", typeof(string))
             },
             OnCPPInterface)
           );
           parser.AddSemantic(
             new Semantic("Factory", new SemanticAttribute[]
             {
                new SemanticAttribute("Factory", typeof(string))
             },
             OnCPPFactory)
           );
            parser.AddSemantic(
             new Semantic("Lifetime", new SemanticAttribute[]
             {
                new SemanticAttribute("Value", typeof(string))
             },
             OnCPPLifetime));
            parser.AddSemantic(
                new Semantic("MetaData", new SemanticAttribute[]
                {
                    new SemanticAttribute("AccessMethod", typeof(string))
                },
                OnMetaData));
            parser.AddSemantic(
                new Semantic("C++ Namespace", new SemanticAttribute[]
                {
                    new SemanticAttribute("DottedNamespace", typeof(string))
                },
                OnCppNamespace));
            parser.AddSemantic(new Semantic("Immutable", new SemanticAttribute[] { }, OnImmutable));
            parser.AddSemantic(new Semantic("Mutable", new SemanticAttribute[] { }, OnMutable));
            parser.AddSemantic(new Semantic("Key", new SemanticAttribute[]
                {
                    new SemanticAttribute("KeyColumn", typeof(string))
                }, OnKeyName));
        }

        public override void Parse()
        {
            PassRowsToParser();
        }
    }

    public class ExcelDoc
    {
        ExcelSpecSheet spec;
        ExcelTypesSheet types;
        ExcelMetaDataSheet metaData;
        Dictionary<string, ExcelTableSheet> tables = new Dictionary<string, ExcelTableSheet>();
        Dictionary<string, ExcelRulesSheet> rules = new Dictionary<string, ExcelRulesSheet>();
        SpreadsheetDocument doc;

        public string Version
        {
            get;
            set;
        }

        public void ParseSheets(IMapFullTablePathToResource mapNameToResource)
        {
            if (spec == null)
            {
                throw new Exception("Missing worksheet Sexcel-Spec");
            }

            spec.Parse();

            string legalVersion = "1.0.0.0";

            if (spec.Version != legalVersion)
            {
                throw new Exception("Unhandled version. Expecting " + legalVersion);
            }

            if (types == null)
            {
                throw new Exception("Missing worksheet Types");
            }

            types.Parse();

            if (metaData == null)
            {
                throw new Exception("Missing worksheet MetaData");
            }

            metaData.Parse();

            foreach(var t in tables)
            {
                t.Value.Parse();
            }

            foreach(var r in rules)
            {
                r.Value.Parse();
            }

            Console.WriteLine("Parsed XML. Generating code");

            foreach(var r in rules)
            {
                if (r.Value.TargetCPP)
                {
                    string tableName = r.Key;
                    ExcelTableSheet table;
                    if (!tables.TryGetValue(tableName, out table))
                    {
                        throw new Exception("Cannot find a table with name " + r.Key + " Table");
                    }

                    var gen = new CPPGenerator(types, metaData, table, r.Value, mapNameToResource);
                    gen.Go();

                    if (r.Value.Lifetime != TableLifetime.Static)
                    {
                        // Table is loaded dynamically when needed
                        var binaryDataTableGenerator = new BinaryGenerator(types, metaData, table, r.Value, gen, mapNameToResource);
                        binaryDataTableGenerator.Go();
                    }

                    var genSexy = new SexyGenerator(gen, types, metaData, table, r.Value);
                    genSexy.Go();
                }
            }
        }

        public ExcelDoc(SpreadsheetDocument doc)
        {
            this.doc = doc;
            this.Version = string.Empty;

            var wb = doc.WorkbookPart.Workbook;

            var sheetsIds = wb.Sheets.Cast<Sheet>();

            SharedStringTablePart stringTable = doc.WorkbookPart.GetPartsOfType<SharedStringTablePart>().FirstOrDefault();

            SharedStringTable strings = stringTable != null ? stringTable.SharedStringTable : null;

            foreach (var sheet in sheetsIds)
            {
                string name = sheet.Name;
                if (name == "Sexcel-Spec")
                {
                    this.spec = new ExcelSpecSheet(doc, sheet, strings);
                }
                else if (name == "Types")
                {
                    this.types = new ExcelTypesSheet(doc, sheet, strings);
                }
                else if (name == "MetaData")
                {
                    this.metaData = new ExcelMetaDataSheet(doc, sheet, strings);
                }
                else if (name.EndsWith(" Table"))
                {
                    this.tables.Add(name.Substring(0, name.Length - " Table".Length), new ExcelTableSheet(doc, sheet, strings));
                }
                else if (name[0] == '#')
                {
                    this.rules.Add(name.Substring(1), new ExcelRulesSheet(doc, sheet, strings));
                }
                else
                {
                    throw new ArgumentException("Do not know how to parse sheet " + name);
                }
            }
        }
    }
}
