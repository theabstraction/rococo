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
    public static string ToString(XPathNavigator node, string attributeKey, bool validateNonEmpty = true)
    {
        if (!node.HasAttributes)
        {
            throw new XmlException("No attributes for node");
        }

        string value = node.GetAttribute(attributeKey, "");
        if (value == null || (validateNonEmpty && value.Length == 0))
        {
            throw new XmlException("No attribute defined: " + attributeKey);
        }

        return value;
    }
}

internal class CPPMasterContext
{
    private readonly string solutionPath;
    private ConsoleArguments commandLineArgs;
    public CPPMasterContext(ConsoleArguments commandLineArgs)
    {
        this.solutionPath = commandLineArgs.SolutionPath;
        this.commandLineArgs = commandLineArgs;
    }
    public void Generate()
    {
        using (var textFile = System.IO.File.OpenText(commandLineArgs.ControlFile))
        {
            try
            {
                ComponentCodeGenerator generator = ParseXmlFile(textFile);
                generator.Commit();
            }
            catch (Exception ex)
            {
                throw new XmlException("Error parsering " + commandLineArgs.ControlFile, ex);
            }
        }
    }

    private ComponentCodeGenerator ParseXmlFile(StreamReader reader)
    {
        // N.B we use XPath rather than XMLDocument because we want line info for error reporting
        using (XmlReader xmlReader = XmlReader.Create(reader))
        {
            XPathDocument controlDoc = new XPathDocument(xmlReader);
            XPathNavigator xpathNav = controlDoc.CreateNavigator();

            string targetHeader = string.Empty;
            string targetSource = string.Empty;
            string declarationIncludePath = string.Empty;
            string templateHeader = string.Empty;
            string templateSource = string.Empty;
       
            var cppNode = xpathNav.SelectSingleNode("/CPP");
            if (cppNode != null)
            {
                templateHeader = XmlAttributeParser.ToString(cppNode, "TemplateHeader");
                templateSource = XmlAttributeParser.ToString(cppNode, "TemplateSource");
                targetHeader = XmlAttributeParser.ToString(cppNode, "TargetHeader");
                targetSource = XmlAttributeParser.ToString(cppNode, "TargetSource");
                declarationIncludePath = XmlAttributeParser.ToString(cppNode, "DeclarationsInclude");
            }
            else 
            {
                throw new XmlException("Expecting section /CPP");
            }

            string xpathQuery = "/CPP/Component";

            XPathExpression xpathExpr = xpathNav.Compile(xpathQuery);
            XPathNodeIterator componentIterator = xpathNav.Select(xpathExpr);

            var generator = new ComponentCodeGenerator(solutionPath, templateHeader, templateSource);
            generator.Prepare(targetHeader, targetSource, declarationIncludePath);

            while (componentIterator.MoveNext())
            {
                var n = componentIterator.Current;
                if (n != null)
                {
                    try
                    {
                        ComponentDefinition def;
                        def.ComponentInterface = XmlAttributeParser.ToString(n, "Interface");

                        Console.WriteLine("Generating code for component '{0}' in '{1}' and '{2}'", def.ComponentInterface, targetHeader, targetSource);

                        generator.GenerateCode(def);
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

            return generator;
        }
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
