using System.Text;
using System.IO;

public struct ComponentDefinition
{
    public string ComponentInterface;
}

public class ComponentSourceInfo
{
    public StringBuilder sb;

    public ComponentSourceInfo(StringBuilder sb)
    {
        this.sb = sb;
    }
}

public class ComponentCodeGenerator
{
    readonly string componentTemplateHeader;
    readonly string componentTemplateSource;
    readonly string componentTemplateHeader_globals;
    readonly string componentTemplateSource_globals;
    readonly string solutionPath;
    readonly string componentTemplateHeaderFilename;
    readonly string componentTemplateSourceFilename;

    StringBuilder sb = new StringBuilder(1024);

    Dictionary<string, StringBuilder> mapFilenameToFile;

    Dictionary<string, ComponentSourceInfo> mapFilenameToTargetSource = new Dictionary<string, ComponentSourceInfo>();
    Dictionary<string, ComponentSourceInfo> mapFilenameToTargetHeader = new Dictionary<string, ComponentSourceInfo>();

    List<string> headerInstances = new List<string>();
    List<string> sourceInstances = new List<string>();

    public ComponentCodeGenerator(string solutionPath, string templateHeader, string templateSource)
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

        componentTemplateHeaderFilename = solutionPath + templateHeader;
        componentTemplateHeader = File.ReadAllText(componentTemplateHeaderFilename);

        componentTemplateSourceFilename = solutionPath + templateSource;
        componentTemplateSource = File.ReadAllText(componentTemplateSourceFilename);

        mapFilenameToFile = new Dictionary<string, StringBuilder>();

        componentTemplateHeader_globals = ParseInstances(componentTemplateHeader, headerInstances);
        componentTemplateSource_globals = ParseInstances(componentTemplateSource, sourceInstances);
    }

    List<ComponentDefinition> defs = new List<ComponentDefinition>();
    
    string fullHeaderSavePath = String.Empty;
    string fullSourceSavePath = String.Empty;
    string declarationIncludePath = String.Empty;

    StringBuilder headerBuilder = new StringBuilder(32768);
    StringBuilder sourceBuilder = new StringBuilder(32768);

    public void Prepare(string targetHeader, string targetSource, string declarationIncludePath)
    {
        fullHeaderSavePath = solutionPath + targetHeader;
        fullSourceSavePath = solutionPath + targetSource;

        headerBuilder.AppendLine("#pragma once");
        headerBuilder.AppendLine();

        var t = System.DateTime.Now.ToUniversalTime();
        headerBuilder.AppendFormat("// Generated at: {0} {1} UTC", t.ToString("MMM dd yyyy"), t.ToString("t"));
        headerBuilder.AppendLine();
        headerBuilder.AppendFormat("// Based on the template file: {0}", componentTemplateHeaderFilename);
        headerBuilder.AppendLine();

        sourceBuilder.AppendFormat("// Generated at: {0} UTC", t.ToString("MMM dd yyyy t"));
        sourceBuilder.AppendLine();
        sourceBuilder.AppendFormat("// Based on the template file: {0}", componentTemplateSourceFilename);
        sourceBuilder.AppendLine();

        this.declarationIncludePath = declarationIncludePath;
    }

    /// <summary>
    /// Extracts source code sections // #BEGIN_INSTANCED# and // #END_INSTANCED#, and puts them in the list.
    /// </summary>
    /// <param name="sourceCode">The source code, .h or .cpp</param>
    /// <param name="instancesBuilder">The list to build</param>
    /// <returns>The source code with instance sections replaced with // #INSTANCE_XXX, where XXX is the index of the instance section, starting from zero </returns>
    public static string ParseInstances(string sourceCode, List<string> instancesBuilder)
    {
        if (sourceCode.Length == 0) return String.Empty;

        int substituteIndex = 0;

        StringBuilder sb = new StringBuilder(sourceCode.Length);

        string startSentinel = "// #BEGIN_INSTANCED#\r\n";
        string endSentinel = "// #END_INSTANCED#\r\n";

        int i = 0;
        while(i < sourceCode.Length)
        {
            int startIndex = sourceCode.IndexOf(startSentinel, i);
            if (startIndex == -1)
            {
                sb.Append(sourceCode, i, sourceCode.Length - i);
                return sb.ToString();
            }
            
            int startOfInstanceCode = startIndex + startSentinel.Length;

            int endIndex = sourceCode.IndexOf(endSentinel, startOfInstanceCode);
            if (endIndex == -1)
            {
                string msg = string.Format("Missing {0} at position {1}", endSentinel, endIndex);
                throw new ArgumentException(msg, "sourceCode");
            }

            instancesBuilder.Add(sourceCode.Substring(startOfInstanceCode, endIndex - startOfInstanceCode));
            
            sb.Append(sourceCode, i, startIndex - i);
            sb.AppendFormat("// #INSTANCE_{0}", substituteIndex++);
            sb.AppendLine();

            i = endIndex + endSentinel.Length;
        }

        return sb.ToString();
    }

    public void GenerateCode(ComponentDefinition def)
    {
        defs.Add(def);
    }

    public static string ToVariableNameLowerPrefix(ComponentDefinition def)
    {
        StringBuilder sb = new StringBuilder(def.ComponentInterface.Length);

        int firstIndex = 0;
        if (def.ComponentInterface[0] == 'I')
        {
            firstIndex = 1;
        }

        sb.Append(char.ToLower(def.ComponentInterface[firstIndex]));
        sb.Append(def.ComponentInterface, 1 + firstIndex, def.ComponentInterface.Length - 1 - firstIndex);
        return sb.ToString();
    }

    public static string ToVariableName(ComponentDefinition def)
    {
        StringBuilder sb = new StringBuilder(def.ComponentInterface.Length);

        int firstIndex = 0;
        if (def.ComponentInterface[0] == 'I')
        {
            firstIndex = 1;
        }

        sb.Append(def.ComponentInterface, firstIndex, def.ComponentInterface.Length - firstIndex);
        return sb.ToString();
    }

    public void Commit()
    {
        string modifiedHeader = componentTemplateHeader_globals.Replace("DeclarationsInclude", declarationIncludePath);

        int index = 0;
        foreach(string s in headerInstances)
        {
            StringBuilder sb = new StringBuilder(s.Length * defs.Count);
            foreach (var def in defs)
            {
                string variableName = ToVariableName(def);
                string variableNameLower = ToVariableNameLowerPrefix(def);
                string s0 = s.Replace("IComponentInterface", def.ComponentInterface);
                string s1 = s0.Replace("componentVariable", variableNameLower);
                string s2 = s1.Replace("ComponentVariable", variableName);
                sb.Append(s2);
            }

            string key = string.Format("// #INSTANCE_{0}\r\n", index++);
            modifiedHeader = modifiedHeader.Replace(key, sb.ToString());     
        }

        headerBuilder.AppendLine(modifiedHeader);

        string modifiedSource = componentTemplateSource_globals;

        index = 0;
        foreach (string s in sourceInstances)
        {
            StringBuilder sb = new StringBuilder(s.Length * defs.Count);
            foreach (var def in defs)
            {
                string variableName = ToVariableName(def);
                string variableNameLower = ToVariableNameLowerPrefix(def);
                string s0 = s.Replace("IComponentInterface", def.ComponentInterface);
                string s1 = s0.Replace("componentVariable", variableNameLower);
                string s2 = s1.Replace("ComponentVariable", variableName);
                sb.Append(s2);
            }

            string key = string.Format("// #INSTANCE_{0}\r\n", index++);
            modifiedSource = modifiedSource.Replace(key, sb.ToString());
        }

        sourceBuilder.AppendLine(modifiedSource);

        File.WriteAllText(fullSourceSavePath, sourceBuilder.ToString());
        File.WriteAllText(fullHeaderSavePath, headerBuilder.ToString());
    }
}

