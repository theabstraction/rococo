// rococo.sharpmake.cs
using Sharpmake;
using System; 
using System.Collections.Generic;

namespace Rococo
{
    public class RococoProject : Project
    {
        public void StandardInit(Configuration conf, Target target, Configuration.OutputType type)
        {
            conf.Output = type;
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";

            AddDefaults(conf, target);

            var excludedFileSuffixes = new List<string>();

            if (target.Platform == Platform.win64)
            {
                excludedFileSuffixes.Add(@"mac");
            }

            conf.SourceFilesBuildExcludeRegex.Add(@"\.*(" + string.Join("|", excludedFileSuffixes.ToArray()) + @")\.cpp$");
            conf.TargetLibraryPath = RococoLibPath + @"[target.Platform]\[conf.Name]\";
        }

        public string RococoRootPath
        {
            get
            {
                return @"[project.SharpmakeCsPath]\..\";
            }
        }

        public string RococoTmpPath
        {
            get
            {
                return @"[project.SharpmakeCsPath]\..\temp\sharpmake\";
            }
        }

        public string RococoLibPath
        {
            get
            {
                return @"[project.SharpmakeCsPath]\..\lib\";
            }
        }


        public string RococoBinPath
        {
            get
            {
                return @"[project.SharpmakeCsPath]\..\bin\";
            }
        }

        public string RococoSexyPath
        {
            get
            {
                return RococoRootPath + @"sexy\";
            }
        }

        protected RococoProject(string name)
        {
            base.Name = name;
            SetSourcePath(name);

            AddTargets(new Target(
                Platform.win64,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Dll,
                Blob.NoBlob,
                BuildSystem.MSBuild,
                DotNetFramework.net6_0)
            );
        }

        protected void AddDefaults(Configuration conf, Target target)
        {
            conf.IncludePaths.Add(RococoRootPath + @"include\");
            conf.IncludePaths.Add(RococoSexyPath + @"Common\");
            conf.Options.Add(Sharpmake.Options.Vc.Compiler.CppLanguageStandard.CPP20);
            conf.Options.Add(Sharpmake.Options.Vc.Compiler.Exceptions.Enable);
            conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4458", "4201", "4324", "4250"));
            conf.IntermediatePath = RococoTmpPath + @"[target.Name]\[project.Name]\";
            conf.TargetPath = RococoBinPath + @"[target.Platform]\[conf.Name]\";
        }

        public virtual void SetSourcePath(string subdir)
        {
            if (subdir == null || subdir[0] == 0)
            {
                throw new Exception("Blank subdir");
            }

            if (subdir[0] == '/' || subdir[0] == '\\')
            {
                throw new Exception("The subdirectory must not begin with a directory slash");
            }

            SourceRootPath = RococoRootPath + subdir;
        }
    }

    public class SexyProject : RococoProject
    {
        protected SexyProject(string name, string subdir) : base(subdir)
        {
            base.Name = name;
        }

        public override void SetSourcePath(string subdir)
        {
            if (subdir == null || subdir[0] == 0)
            {
                throw new Exception("Blank subdir");
            }

            if (subdir[0] == '/' || subdir[0] == '\\')
            {
                throw new Exception("The subdirectory must not begin with a directory slash");
            }

            SourceRootPath = RococoSexyPath + subdir;
        }
    }

    public class RococoCSharpProject : CSharpProject
    {
        public string RococoRootPath
        {
            get
            {
                return @"[project.SharpmakeCsPath]\..\";
            }
        }

        public string RococoTmpPath
        {
            get
            {
                return @"[project.SharpmakeCsPath]\..\temp\sharpmake\";
            }
        }

        public string RococoLibPath
        {
            get
            {
                return @"[project.SharpmakeCsPath]\..\lib\";
            }
        }


        public string RococoBinPath
        {
            get
            {
                return @"[project.SharpmakeCsPath]\..\bin\";
            }
        }

        public string RococoSexyPath
        {
            get
            {
                return RococoRootPath + @"sexy\";
            }
        }

        public void SetSourcePath(string subdir)
        {
            if (subdir == null || subdir[0] == 0)
            {
                throw new Exception("Blank subdir");
            }

            if (subdir[0] == '/' || subdir[0] == '\\')
            {
                throw new Exception("The subdirectory must not begin with a directory slash");
            }

            SourceRootPath = RococoRootPath + subdir;
        }

        protected RococoCSharpProject(string name)
        {
            base.Name = name;
            SetSourcePath(name);

            RootPath = RococoRootPath;

            AddTargets(new Target(
                Platform.win64,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Dll,
                Blob.NoBlob,
                BuildSystem.MSBuild,
                DotNetFramework.net6_0)
            );

            // This Path will be used to get all SourceFiles in this Folder and all subFolders
            SourceRootPath = @"[project.RootPath]\[project.Name]\";
        }

        public virtual void ConfigureAll(Configuration conf, Target target)
        {
            conf.ProjectFileName = "[project.Name].[target.DevEnv].[target.Framework]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated\";
            conf.TargetLibraryPath = RococoLibPath + @"[target.Platform]\[conf.Name]\";
            conf.IntermediatePath = RococoTmpPath + @"[target.Name]\[project.Name]\";
            conf.TargetPath = RococoBinPath + @"[target.Platform]\[conf.Name]\";
        }
    }

    [Sharpmake.Generate]
    public class RococoUtilsProject : RococoProject
    {
        public RococoUtilsProject() : base("rococo.util")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
        }
    }

    [Sharpmake.Generate]
    public class RococoMathsProject : RococoProject
    {
        public RococoMathsProject() : base("rococo.maths")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Lib);
        }
    }

    [Sharpmake.Generate]
    public class RococoUtilExProject : RococoProject
    {
        public RococoUtilExProject() : base("rococo.util.ex")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.Defines.Add("ROCOCO_UTILS_EX_API=__declspec(dllexport)");
            conf.Defines.Add("RENDERER_API=__declspec(dllexport)");
            conf.AddPublicDependency<RococoUtilsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoPackagerProject : RococoProject
    {
        public RococoPackagerProject() : base("rococo.packager")
        {
            SetSourcePath(Name);
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Exe);
            conf.AddPublicDependency<RococoUtilsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoMiscUtilsProject : RococoProject
    {
        public RococoMiscUtilsProject() : base("rococo.misc.utils")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoWindowsProject>(target);
            conf.Defines.Add("ROCOCO_MISC_UTILS_API=__declspec(dllexport)");
        }
    }

    [Sharpmake.Generate]
    public class RococoWindowsProject : RococoProject
    {
        public RococoWindowsProject() : base("rococo.windows")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.Defines.Add("ROCOCO_WINDOWS_API=__declspec(dllexport)");
        }
    }

    [Sharpmake.Generate]
    public class RococoSexyIDEProject : RococoProject
    {
        public RococoSexyIDEProject() : base("rococo.sexy.ide")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Lib);
        }
    }

    [Sharpmake.Generate]
    public class RococoSexyCmdProject : RococoProject
    {
        public RococoSexyCmdProject() : base("rococo.sexy.cmd")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Exe);
            conf.IncludePaths.Add(RococoSexyPath + @"STC\stccore\");
            conf.AddPublicDependency<RococoUtilsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoSexyMathSexProject : RococoProject
    {
        public RococoSexyMathSexProject() : base("rococo.sexy.mathsex")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoMathsProject>(target);
            conf.AddPublicDependency<RococoUtilsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoFontsProject : RococoProject
    {
        public RococoFontsProject() : base("rococo.fonts")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Lib);
        }
    }

    [Sharpmake.Generate]
    public class RococoDX11RendererProject : RococoProject
    {
        public RococoDX11RendererProject() : base("dx11.renderer")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Lib);
        }
    }

    [Sharpmake.Generate]
    public class RococoFileBrowserProject : RococoProject
    {
        public RococoFileBrowserProject() : base("rococo.file.browser")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Lib);
        }
    }

    [Sharpmake.Generate]
    public class RococoGuiRetainedProject : RococoProject
    {
        public RococoGuiRetainedProject() : base("rococo.gui.retained")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.Defines.Add("ROCOCO_GUI_RETAINED_API=__declspec(dllexport)");
            conf.AddPublicDependency<RococoUtilsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoCPPMasterProject : RococoCSharpProject
    {
        public RococoCPPMasterProject() : base("rococo.cpp_master")
        {
        }

        [Configure()]
        public override void ConfigureAll(Configuration conf, Target target)
        {
            conf.Output = Configuration.OutputType.DotNetConsoleApp;
            base.ConfigureAll(conf, target);
            conf.TargetLibraryPath = RococoLibPath + @"[target.Platform]\[conf.Name]\";
        }
    }

    [Sharpmake.Generate]
    public class RococoMPlatProject : RococoProject
    {
        public RococoMPlatProject() : base("rococo.mplat")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Lib);

            conf.SourceFilesBuildExcludeRegex.Add(@"mplat.component.template.cpp");
            conf.SourceFilesBuildExcludeRegex.Add(@"mplat.test.app.cpp");
        }
    }

    [Sharpmake.Generate]
    public class RococoMPlatDynamicProject : RococoProject
    {
        public RococoMPlatDynamicProject() : base("rococo.mplat.dynamic")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoWindowsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoMHostProject : RococoProject
    {
        public RococoMHostProject() : base("mhost")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Exe);
        }
    }

    [Sharpmake.Generate]
    public class RococoSexyStudioProject : RococoProject
    {
        public RococoSexyStudioProject() : base("sexystudio")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoWindowsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoSexyStudioAppProject : RococoProject
    {
        public RococoSexyStudioAppProject() : base("sexystudio.app")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Exe);
            conf.AddPublicDependency<RococoSexyStudioProject>(target);
            conf.AddPublicDependency<RococoWindowsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoSexyStudioTestProject : RococoProject
    {
        public RococoSexyStudioTestProject() : base("sexystudio.test")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Exe);
            conf.AddPublicDependency<RococoSexyStudioProject>(target);
            conf.AddPublicDependency<RococoWindowsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoAudioProject : RococoProject
    {
        public RococoAudioProject() : base("rococo.audio")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.Defines.Add("ROCOCO_AUDIO_API=__declspec(dllexport)");
            conf.AddPublicDependency<RococoUtilsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoAudioTestProject : RococoProject
    {
        public RococoAudioTestProject() : base("rococo.audio.test")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Exe);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoAudioProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoCSharpSolution : CSharpSolution
    {
        public RococoCSharpSolution()
        {
            Name = "rococo.sharpmake";

            AddTargets(new Target(
                Platform.win64,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Dll,
                Blob.NoBlob,
                BuildSystem.MSBuild,
                DotNetFramework.net6_0)
            );
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.SolutionFileName = "rococo.all";
            conf.SolutionPath = @"[solution.SharpmakeCsPath]\generated";
            conf.AddProject<RococoCPPMasterProject>(target);
            conf.AddProject<RococoUtilsProject>(target);
            conf.AddProject<RococoMiscUtilsProject>(target);
            conf.AddProject<RococoMathsProject>(target);
            conf.AddProject<RococoUtilExProject>(target);
            conf.AddProject<RococoPackagerProject>(target);
            conf.AddProject<RococoWindowsProject>(target);
            conf.AddProject<RococoSexyIDEProject>(target);
            conf.AddProject<RococoSexyCmdProject>(target);
            conf.AddProject<RococoSexyMathSexProject>(target);
            conf.AddProject<RococoFontsProject>(target);
            conf.AddProject<RococoFileBrowserProject>(target);
            conf.AddProject<RococoGuiRetainedProject>(target);
            conf.AddProject<RococoMPlatProject>(target);
            conf.AddProject<RococoMPlatDynamicProject>(target);
            conf.AddProject<RococoMHostProject>(target);
            conf.AddProject<RococoSexyStudioProject>(target);
            conf.AddProject<RococoSexyStudioAppProject>(target);
            conf.AddProject<RococoSexyStudioTestProject>(target);
            conf.AddProject<RococoAudioProject>(target);
            conf.AddProject<RococoAudioTestProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class SexyUtilProject : SexyProject
    {
        public SexyUtilProject() : base("sexy.util", "Utilities")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.Defines.Add("SEXYUTIL_API=__declspec(dllexport)");
            conf.AddPublicDependency<RococoUtilsProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class SexyVMProject : SexyProject
    {
        public SexyVMProject() : base("sexy.vm", "SVM/svmcore")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Lib);
        }
    }

    [Sharpmake.Generate]
    public class SexyCSharpSolution : CSharpSolution
    {
        public SexyCSharpSolution()
        {
            Name = "sexy.sharpmake";

            AddTargets(new Target(
                Platform.win64,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Dll,
                Blob.NoBlob,
                BuildSystem.MSBuild,
                DotNetFramework.net6_0)
            );
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.SolutionFileName = "sexy.all";
            conf.SolutionPath = @"[solution.SharpmakeCsPath]\generated";
            conf.AddProject<SexyUtilProject>(target);
            conf.AddProject<SexyVMProject>(target);
        }
    }

    public static class Main
    {
        [Sharpmake.Main]
        public static void SharpmakeMain(Sharpmake.Arguments arguments)
        {
            arguments.Generate<RococoCSharpSolution>();
            arguments.Generate<RococoUtilsProject>();
            arguments.Generate<RococoMiscUtilsProject>();
            arguments.Generate<RococoMathsProject>();
            arguments.Generate<RococoUtilExProject>();
            arguments.Generate<RococoPackagerProject>();
            arguments.Generate<RococoWindowsProject>();
            arguments.Generate<RococoSexyIDEProject>();
            arguments.Generate<RococoSexyCmdProject>();
            arguments.Generate<RococoSexyMathSexProject>();
            arguments.Generate<RococoFontsProject>();
            arguments.Generate<RococoDX11RendererProject>();
            arguments.Generate<RococoFileBrowserProject>();
            arguments.Generate<RococoGuiRetainedProject>();
            arguments.Generate<RococoCPPMasterProject>();
            arguments.Generate<RococoMPlatProject>();
            arguments.Generate<RococoMPlatDynamicProject>();
            arguments.Generate<RococoMHostProject>();
            arguments.Generate<RococoSexyStudioProject>();
            arguments.Generate<RococoSexyStudioAppProject>();
            arguments.Generate<RococoSexyStudioTestProject>();
            arguments.Generate<RococoAudioProject>();
            arguments.Generate<RococoAudioTestProject>();

            arguments.Generate<SexyCSharpSolution>();
            arguments.Generate<SexyUtilProject>();
            arguments.Generate<SexyVMProject>();
        }
    }
}