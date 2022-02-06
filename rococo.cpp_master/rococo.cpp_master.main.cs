using System.Xml;
using System.Xml.XPath;

class CPPMaster
{

}

public class ConsoleArguments
{
    private string[] args;

    public ConsoleArguments(string[] args)
    {
        this.args = args;
    }

    public override string ToString()
    {
        var sb = new System.Text.StringBuilder();
        foreach(var arg in args)
        {
            sb.Append(arg.ToString());
            sb.Append("\t");
        }
        return sb.ToString();
    }

    public string ControlFile
    {
        get
        {
            if (args == null || args.Length < 2)
            {
                return String.Empty;
            }
            else
            {
                return args[0];
            }
        }
    }

    public string SolutionPath
    {
        get
        {
            if (args == null || args.Length < 2)
            {
                return String.Empty;
            }
            else
            {
                return args[1];
            }
        }
    }
}

public static class XmlAttributeParser
{
    public static string ToString(XPathNavigator node, string attributeKey)
    {
        if (!node.HasAttributes)
        {
            throw new XmlException("No attributes for node");
        }

        string value = node.GetAttribute(attributeKey, "");
        if (value == null || value.Length == 0)
        {
            throw new XmlException("No attribute defined: " + attributeKey);
        }

        return value;
    }
}

internal class CPPMasterContext
{
    private ConsoleArguments commandLineArgs;
    private ComponentCodeGenerator componentGenerator;
    public CPPMasterContext(ConsoleArguments commandLineArgs)
    {
        this.commandLineArgs = commandLineArgs;
        componentGenerator = new ComponentCodeGenerator(commandLineArgs.SolutionPath);
    }
    public void Generate()
    {
        using (var textFile = System.IO.File.OpenText(commandLineArgs.ControlFile))
        {
            try
            {
                ParseXmlFile(textFile);
            }
            catch (Exception ex)
            {
                throw new XmlException("Error parsering " + commandLineArgs.ControlFile, ex);
            }
        }
    }

    private void ParseXmlFile(StreamReader reader)
    {
        // N.B we use XPath rather than XMLDocument because we want line info for error reporting
        using (XmlReader xmlReader = XmlReader.Create(reader))
        {
            XPathDocument controlDoc = new XPathDocument(xmlReader);
            XPathNavigator xpathNav = controlDoc.CreateNavigator();

            string savePath;
            string declarationPath;
       
            var cppNode = xpathNav.SelectSingleNode("/CPP");
            if (cppNode != null)
            {
                savePath = XmlAttributeParser.ToString(cppNode, "ComponentHeader");
                declarationPath = XmlAttributeParser.ToString(cppNode, "Declarations");
            }
            else 
            {
                savePath = commandLineArgs.SolutionPath + "intermediate\\components.h";
                declarationPath = commandLineArgs.SolutionPath + "intermediate\\component.declarations.h";
            }

            string xpathQuery = "/CPP/Component";

            XPathExpression xpathExpr = xpathNav.Compile(xpathQuery);
            XPathNodeIterator componentIterator = xpathNav.Select(xpathExpr);
            
            while (componentIterator.MoveNext())
            {
                var n = componentIterator.Current;
                if (n != null)
                {
                    try
                    {
                        ParseComponentDefinition(n, savePath, declarationPath);
                    }
                    catch (Exception ex)
                    {
                        IXmlLineInfo lineInfo = (IXmlLineInfo)n;
                        if (lineInfo != null)
                        {
                            throw new XmlException("Error building component", ex, lineInfo.LineNumber, lineInfo.LinePosition);
                        }
                        else
                        {
                            throw new XmlException("Error building component", ex);
                        }
                    }
                }
            }
        }
    }

    private void ParseComponentDefinition(XPathNavigator nodeComponent, string savePath, string declarationPath)
    {
        ComponentDefinition def;
        def.ComponentInterface = XmlAttributeParser.ToString(nodeComponent, "Interface");
        def.ComponentSavePath = savePath;
        def.componentDeclarationsFilename = declarationPath;

        Console.WriteLine("Generating code for component '{0}' in '{1}'", def.ComponentInterface, savePath);

        componentGenerator.GenerateCode(def);
    }
}

public class CPPMasterApplication
{
    static public void MainProtected(string[] args)
    {
        ConsoleArguments commandLineArgs = new ConsoleArguments(args);

        // Usage: cpp_master.exe <control-file> <solution-path>

        string controlFileName = commandLineArgs.ControlFile;
        if (controlFileName.Length == 0)
        {
            Console.WriteLine("CPPMaster: No control file specified");
            return;
        }

        string solutionPath = commandLineArgs.SolutionPath;
        if (solutionPath.Length == 0)
        {
            Console.WriteLine("CPPMaster: No solution specified");
            return;
        }

        CPPMasterContext context = new CPPMasterContext(commandLineArgs);
        context.Generate();
    }

    static public void Main(string[] args)
    {
        try
        {
            MainProtected(args);
        }
        catch(Exception ex)
        {
            Console.Error.WriteLine(ex);
        }
    }
}
