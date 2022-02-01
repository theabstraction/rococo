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
            if (args == null || args.Length == 0)
            {
                return String.Empty;
            }
            else
            {
                return args[0];
            }
        }
    }

}

public struct ComponentDefinition
{
    public string ComponentInterface;
    public string ComponentImplementation;
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

public class ComponentCodeGenerator
{
    public void GenerateCode(ComponentDefinition def)
    {
        throw new NotImplementedException();
    }
}

public class CPPMasterApplication
{
    static public void ParseComponentDefinition(XPathNavigator nodeComponent)
    {
        ComponentDefinition def;
        def.ComponentInterface = XmlAttributeParser.ToString(nodeComponent, "Interface");
        def.ComponentImplementation = XmlAttributeParser.ToString(nodeComponent, "Class");

        Console.WriteLine("Generating code for component '{0}' that implements '{1}'", def.ComponentImplementation, def.ComponentInterface);

        ComponentCodeGenerator generator = new ComponentCodeGenerator();

        generator.GenerateCode(def);
    }

    static void ParseXmlFile(StreamReader reader)
    {
        using (XmlReader xmlReader = XmlReader.Create(reader))
        { 
            XPathDocument controlDoc = new XPathDocument(xmlReader);
            XPathNavigator xpathNav = controlDoc.CreateNavigator();

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
                        ParseComponentDefinition(n);
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

    static public void MainProtected(string[] args)
    {
        ConsoleArguments commandLineArgs = new ConsoleArguments(args);

        string controlFileName = commandLineArgs.ControlFile;
        if (controlFileName.Length == 0)
        {
            Console.WriteLine("CPPMaster: No control file specified");
            return;
        }

        var textFile = System.IO.File.OpenText(controlFileName);

        try
        {
            ParseXmlFile(textFile);
        }
        catch(Exception ex)
        {
            throw new XmlException("Error parsering " + controlFileName, ex);
        }
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
