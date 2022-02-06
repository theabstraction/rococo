using System.Text;

public struct ComponentDefinition
{
    public string ComponentInterface;
    public string ComponentSavePath;
    public string componentDeclarationsFilename;
}

public class ComponentCodeGenerator
{
    readonly string componentTemplateHeader;
    readonly string solutionPath;
    readonly string componentTemplateHeaderFilename;

    StringBuilder sb = new StringBuilder(1024);

    public ComponentCodeGenerator(string solutionPath)
    {
        if (solutionPath.IndexOf('/') != -1)
        {
            throw new ArgumentException("solutionPath was Unix style, rather than Windows style");
        }

        if (!solutionPath.EndsWith("\\"))
        {
            throw new ArgumentException("solutionPath must end with trailling slash");
        }

        if (solutionPath.Length > 1 && solutionPath[solutionPath.Length - 2] == '\\')
        {
            throw new ArgumentException("solutionPath must not end with double trailling slashes");
        }

        this.solutionPath = solutionPath;

        componentTemplateHeaderFilename = solutionPath + "rococo.cpp_master\\component.template.h";
        componentTemplateHeader = System.IO.File.ReadAllText(componentTemplateHeaderFilename);
    }
    public void GenerateCode(ComponentDefinition def)
    {
        string s0 = componentTemplateHeader.Replace("IComponentInterface", def.ComponentInterface);
        
        StringBuilder metaBuilder = new StringBuilder(1024);
        metaBuilder.AppendLine("// Meta data:");

        var t = System.DateTime.Now.ToUniversalTime();
        metaBuilder.AppendFormat("// Generated at: {0} {1} {2} {3}.{4}.{5} UTC", t.ToString("MMM"), t.Day, t.Year, t.Hour, t.Minute, t.Second);
        metaBuilder.AppendLine();
        metaBuilder.AppendFormat("// Based on the template file: {0}", componentTemplateHeaderFilename);
        metaBuilder.AppendLine();

        string s1 = s0.Replace("// Template-MetaData", metaBuilder.ToString());

        string s2 = s1.Replace("$declarations.h", def.componentDeclarationsFilename);

        string fullSavePath = solutionPath + def.ComponentSavePath;

        System.IO.File.WriteAllText(fullSavePath, s2);
    }
}

