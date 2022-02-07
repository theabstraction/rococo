using System.Text;
using System.IO;

public struct ComponentDefinition
{
    public string ComponentInterface;
    public string targetHeader;
    public string targetSource;
    public string sourceInclude;
    public string declarationsInclude;
}

public class ComponentSourceInfo
{
    public List<string> ComponentInterfaces = new List<string>();
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
    readonly string solutionPath;
    readonly string componentTemplateHeaderFilename;
    readonly string componentTemplateSourceFilename;

    StringBuilder sb = new StringBuilder(1024);

    Dictionary<string, StringBuilder> mapFilenameToFile;

    Dictionary<string, ComponentSourceInfo> mapFilenameToTargetSource = new Dictionary<string, ComponentSourceInfo>();
    Dictionary<string, ComponentSourceInfo> mapFilenameToTargetHeader = new Dictionary<string, ComponentSourceInfo>();

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
        componentTemplateHeader = File.ReadAllText(componentTemplateHeaderFilename);

        componentTemplateSourceFilename = solutionPath + "rococo.cpp_master\\component.template.cpp";
        componentTemplateSource = File.ReadAllText(componentTemplateSourceFilename);

        mapFilenameToFile = new Dictionary<string, StringBuilder>();
    }
    public void GenerateCode(ComponentDefinition def)
    {
        string fullHeaderSavePath = solutionPath + def.targetHeader;

        ComponentSourceInfo? headerInfo;

        StringBuilder? headerBuilder;
        if (!mapFilenameToFile.TryGetValue(fullHeaderSavePath, out headerBuilder))
        {
            mapFilenameToFile.Add(fullHeaderSavePath, headerBuilder = new StringBuilder(1024));
            headerInfo = new ComponentSourceInfo(headerBuilder);
            mapFilenameToTargetHeader.Add(fullHeaderSavePath, headerInfo);

            headerBuilder.AppendLine("#pragma once");
            headerBuilder.AppendLine();

            var t = System.DateTime.Now.ToUniversalTime();
            headerBuilder.AppendFormat("// Generated at: {0} UTC", t.ToString("MMM dd yyyy T"));
            headerBuilder.AppendLine();
            headerBuilder.AppendFormat("// Based on the template file: {0}", componentTemplateHeaderFilename);
            headerBuilder.AppendLine();
            headerBuilder.AppendLine();

            headerBuilder.AppendLine("#include <rococo.api.h>");
            headerBuilder.AppendLine("#include <list>");
            headerBuilder.AppendLine("#include <unordered_map>");
            headerBuilder.AppendLine("#include \"rococo.component.entities.h\"");
            headerBuilder.AppendFormat("#include \"{0}\"", def.declarationsInclude);
            headerBuilder.AppendLine();
        }
        else 
        {
            headerInfo = mapFilenameToTargetHeader[fullHeaderSavePath];
        }

        headerInfo.ComponentInterfaces.Add(def.ComponentInterface);

        headerBuilder.AppendLine();
        headerBuilder.AppendLine();
        string s0 = componentTemplateHeader.Replace("IComponentInterface", def.ComponentInterface);
        headerBuilder.Append(s0);

        string fullSourceSavePath = solutionPath + def.targetSource;

        ComponentSourceInfo? sourceInfo;

        StringBuilder? sourceBuilder;
        if (!mapFilenameToFile.TryGetValue(fullSourceSavePath, out sourceBuilder))
        {
            mapFilenameToFile.Add(fullSourceSavePath, sourceBuilder = new StringBuilder(1024));
            sourceInfo = new ComponentSourceInfo(sourceBuilder);
            mapFilenameToTargetSource.Add(fullSourceSavePath, sourceInfo);
      
            sourceBuilder.AppendFormat("#include \"{0}\"", def.sourceInclude);
            sourceBuilder.AppendLine();

            var t = System.DateTime.Now.ToUniversalTime();
            sourceBuilder.AppendFormat("// Generated at: {0} UTC", t.ToString("MMM dd yyyy T"));
            sourceBuilder.AppendLine();
            sourceBuilder.AppendFormat("// Based on the template file: {0}", componentTemplateSourceFilename);
        }
        else
        {
            sourceInfo = mapFilenameToTargetSource[fullSourceSavePath];
        }

        sourceInfo.ComponentInterfaces.Add(def.ComponentInterface);

        sourceBuilder.AppendLine();
        sourceBuilder.AppendLine();
        string s1 = componentTemplateSource.Replace("IComponentInterface", def.ComponentInterface);
        sourceBuilder.Append(s1);
    }
    public static void AppendVariableNameFromInterfaceName(StringBuilder sb, string interfaceName)
    {
        sb.Append(char.ToLower(interfaceName[1]));

        for(int i = 2; i < interfaceName.Length; i++)
        {
            sb.Append(interfaceName[i]);
        }
    }

    public static void AppendFunctionNameFromInterfaceName(StringBuilder sb, string interfaceName)
    {
        for (int i = 1; i < interfaceName.Length; i++)
        {
            sb.Append(interfaceName[i]);
        }
    }

    public void Commit()
    {
        foreach (var i in mapFilenameToTargetHeader)
        {
            var sb = i.Value.sb;
            sb.AppendLine();
            sb.AppendLine();
            sb.AppendLine("namespace Rococo::Components::Sys");
            sb.AppendLine("{");
            sb.AppendLine("\tusing namespace Rococo::Components;");
            sb.AppendLine();
            sb.AppendLine("\tstruct ComponentFactories");
            sb.AppendLine("\t{");
            foreach (var ci in i.Value.ComponentInterfaces)
            {
                sb.AppendFormat("\t\t{0}Factory& ", ci);
                AppendVariableNameFromInterfaceName(sb, ci);
                sb.AppendLine("factory;");
            }
            sb.AppendLine("\t};");
            sb.AppendLine();

            sb.AppendLine("\tstruct ActiveComponents");
            sb.AppendLine("\t{");

            for (int j = 0; j < i.Value.ComponentInterfaces.Count; j++)
            {
                sb.Append("\t\tbool has");
                AppendFunctionNameFromInterfaceName(sb, i.Value.ComponentInterfaces[j]);
                sb.AppendLine(": 1;");
            }
            sb.AppendLine("\t};");
            sb.AppendLine();
            sb.AppendLine("\tROCOCOAPI IComponentTables");
            sb.AppendLine("\t{");

            foreach (var ci in i.Value.ComponentInterfaces)
            {
                sb.AppendFormat("\t\tvirtual {0}* Add", ci);
                AppendFunctionNameFromInterfaceName(sb, ci);
                sb.AppendFormat("(EntityIndex index, ActiveComponents& ac) = 0;", ci);
                sb.AppendLine("");
            }

            sb.AppendLine("");

            sb.AppendLine("\t\tvirtual void Deprecate(EntityIndex index, const ActiveComponents& ac) = 0;");

            foreach (var ci in i.Value.ComponentInterfaces)
            {
                sb.AppendLine("");
                sb.AppendFormat("\t\tvirtual {0}Table& Get", ci);
                AppendFunctionNameFromInterfaceName(sb, ci);
                sb.AppendFormat("Table() = 0;", ci);      
            }

            sb.AppendLine();
            sb.AppendLine("\t};");
            sb.AppendLine();
            sb.AppendLine("\tROCOCOAPI IComponentTablesSupervisor: IComponentTables");
            sb.AppendLine("\t{");
            sb.AppendLine("\t\tvirtual void Free() = 0;");
            sb.AppendLine("\t};");
            sb.AppendLine();
            sb.AppendLine("\tIComponentTablesSupervisor* CreateComponentTables(ComponentFactories& factories);");
            sb.AppendLine("}");
        }

        foreach (var i in mapFilenameToTargetSource)
        {
            var sb = i.Value.sb;

            sb.AppendLine();
            sb.AppendLine();
            sb.AppendLine("namespace ANON");
            sb.AppendLine("{");
            sb.AppendLine("\tusing namespace Rococo::Components;");
            sb.AppendLine();
            sb.AppendLine("\tstruct Impl_AllComponentTables : IComponentTablesSupervisor");
            sb.AppendLine("\t{");

            foreach (var ci in i.Value.ComponentInterfaces)
            {
                sb.AppendFormat("\t\tAutoFree<{0}Table> ", ci);
                AppendVariableNameFromInterfaceName(sb, ci);
                sb.AppendFormat("Table;", ci);
                sb.AppendLine();
            }

            sb.AppendLine();
            sb.AppendLine("\t\tImpl_AllComponentTables(ComponentFactories& factories)");
            sb.AppendLine("\t\t{");

            foreach (var ci in i.Value.ComponentInterfaces)
            {
                sb.AppendFormat("\t\t\t");
                AppendVariableNameFromInterfaceName(sb, ci);
                sb.AppendFormat("Table = Factories::NewComponentInterfaceTable(factories.");
                AppendVariableNameFromInterfaceName(sb, ci);
                sb.AppendLine("factory);");
            }

            sb.AppendLine("\t\t}");
            sb.AppendLine();

            foreach (var ci in i.Value.ComponentInterfaces)
            {
                sb.AppendFormat("\t\t{0}* Add", ci);
                AppendFunctionNameFromInterfaceName(sb, ci);
                sb.AppendLine("(EntityIndex index, ActiveComponents& ac) override");
                sb.AppendLine("\t\t{");
                sb.Append("\t\t\tac.has");
                AppendFunctionNameFromInterfaceName(sb, ci);
                sb.AppendLine(" = true;");
                sb.AppendFormat("\t\t\treturn ");
                AppendVariableNameFromInterfaceName(sb, ci);
                sb.AppendLine("Table->AddNew(index);");
                sb.AppendLine("\t\t}");
                sb.AppendLine();
            }

            sb.AppendLine("\t\tvoid Deprecate(EntityIndex index, const ActiveComponents& ac) override");
            sb.AppendLine("\t\t{");

            bool isFirst = true;

            foreach (var ci in i.Value.ComponentInterfaces)
            {
                if (isFirst)
                {
                    isFirst = false;
                }
                else
                {
                    sb.AppendLine();
                }

                sb.Append("\t\t\tif (ac.has");
                AppendFunctionNameFromInterfaceName(sb, ci);
                sb.AppendLine(")");
                sb.AppendLine("\t\t\t{");
                sb.Append("\t\t\t\t");
                AppendVariableNameFromInterfaceName(sb, ci);
                sb.AppendLine("Table->Deprecate(index);");
                sb.AppendLine("\t\t\t}");
            }

            sb.AppendLine("\t\t}");
            sb.AppendLine();

            foreach (var ci in i.Value.ComponentInterfaces)
            {
                sb.AppendFormat("\t\t{0}Table& Get", ci);
                AppendFunctionNameFromInterfaceName(sb, ci);
                sb.AppendLine("Table()");
                sb.AppendLine("\t\t{");
                sb.Append("\t\t\treturn *");
                AppendVariableNameFromInterfaceName(sb, ci);
                sb.AppendLine("Table;");
                sb.AppendLine("\t\t}");
                sb.AppendLine();
            }

            sb.AppendLine("\t\tvoid Free() override");
            sb.AppendLine("\t\t{");
            sb.AppendLine("\t\t\tdelete this;");
            sb.AppendLine("\t\t}");

            sb.AppendLine("\t};");
            sb.AppendLine("}");

            sb.AppendLine("namespace Rococo::Components::Sys::Factories");
            sb.AppendLine("{");
            sb.AppendLine("\tIComponentTables* CreateComponentTables(ComponentFactories& factories)");
            sb.AppendLine("\t{");
            sb.AppendLine("\t\treturn new ANON::Impl_AllComponentTables(factories);");
            sb.AppendLine("\t}");
            sb.AppendLine("}");
        }

        foreach(var i in mapFilenameToFile)
        {
            File.WriteAllText(i.Key, i.Value.ToString());
        }
    }
}

