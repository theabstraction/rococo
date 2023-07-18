// rococo.sharpmake.cs
using Sharpmake;
using System;
using System.IO;
using System.Collections.Generic;

namespace Rococo
{
    public static class Roots
    {
        private static string rococoRootPath = null;
        public static string RococoRootPath
        {
            get
            {
                if (rococoRootPath == null)
                {
                    rococoRootPath = Directory.GetCurrentDirectory();
                }

                string rococoPath = Directory.GetCurrentDirectory();
                string rococoGuidFile = Path.Combine(rococoPath, "rococo.guid.txt");
                string guid = File.ReadAllText(rococoGuidFile);
                string officialGuid = "EAF1A643-486F-4019-A3D4-6712BFC9568D";
                if (guid != officialGuid)
                {
                    throw new Exception("Working directory for this script must be the rococo root directory, that contains the rococo guid in rococo.guid.txt " + officialGuid);
                }

                return rococoRootPath;
            }
        }

        public static string RococoProjectPath
        {
            get
            {
                return Path.Combine(Roots.RococoRootPath, @"build\projects\");
            }
        }

        public static string GetRelativeToProject(string pathName)
        {
            return Util.PathGetRelative(Roots.RococoProjectPath, pathName);
        }

        public static string RococoContentPath
        {
            get
            {
                return Path.Combine(Roots.RococoRootPath, @"content\");
            }
        }

        public static string RococoSourcePath
        {
            get
            {
                return Path.Combine(Roots.RococoRootPath, @"source\rococo\");
            }
        }

        public static string RococoToolsPath
        {
            get
            {
                return Path.Combine(Roots.RococoRootPath, @"build\tools\");
            }
        }

        public static string RococoTmpPath
        {
            get
            {
                return Path.Combine(Roots.RococoRootPath, @"gen\temp\");
            }
        }

        public static string RococoLibPath
        {
            get
            {
                return Path.Combine(Roots.RococoRootPath, @"gen\lib\");
            }
        }


        public static string RococoBinPath
        {
            get
            {
                return Path.Combine(Roots.RococoRootPath, @"gen\bin\");
            }
        }

        public static string RococoSexyPath
        {
            get
            {
                return Path.Combine(RococoSourcePath, @"sexy\");
            }
        }

        public static string RococoIncludePath
        {
            get
            {
                return Path.Combine(RococoSourcePath, @"include\");
            }
        }

        public static string ThirdPartyPath
        {
            get
            {
                return Path.Combine(Roots.RococoRootPath, @"source\3rd-Party\");
            }
        }
    }

    public static class Local
    {
        public static void SetOptimizations(Project.Configuration config, Sharpmake.Target target)
        {
            if (target.Optimization == Optimization.Debug)
            {
                config.Options.Add(Sharpmake.Options.Vc.Compiler.Inline.OnlyInline);
                //config.Options.Add(Sharpmake.Options.Vc.Compiler.Inline.OnlyInline);
            }
            else
            {
                config.Options.Add(Sharpmake.Options.Vc.Compiler.Inline.AnySuitable);
            }
        }
    }

    public class RococoBaseProject : Project
    {
        public void AddDefaultLibraries(Configuration config)
        {
            config.TargetLibraryPath = Path.Combine(Roots.RococoLibPath, @"[target.Platform]\[conf.Name]\");
        }

        public void Exclude(params string[] names)
        {
            foreach (var name in names)
            {
                SourceFilesExclude.Add(name);
            }
        }

        public void AddSXHFileBuildStep(Configuration conf, string sxh, string xc, bool asPreBuildStep)
        {
            if (!sxh.EndsWith(".sxh"))
            {
                throw new Exception("Expecting file " + sxh + " to end with .sxh");
            }

            if (!xc.EndsWith(".xc"))
            {
                throw new Exception("Expecting config file " + xc + " to end with .xc");
            }

            if (asPreBuildStep)
            {
                string makeFilePath = Path.Combine(Roots.RococoToolsPath, @"rococo.sxh.mak");

                string makeRelative = Roots.GetRelativeToProject(makeFilePath);
                string contentRelative = Roots.GetRelativeToProject(Roots.RococoContentPath);
                string srcRelative = Roots.GetRelativeToProject(SourceRootPath);
                string binRelative = Roots.GetRelativeToProject(conf.TargetPath);

                string makeFile = string.Format("NMAKE /NOLOGO /F \"{0}\" \"BENNY_HILL={1}\\sexy.bennyhill.exe\" all SOURCE_ROOT={2}\\ CONTENT_ROOT={3}\\ SXH_FILE={4} XC_FILE={5}", makeRelative, binRelative, srcRelative, contentRelative, sxh, xc);
                conf.EventPreBuild.Add(makeFile);
            }
            else
            {
                string srcRelative = Roots.GetRelativeToProject(SourceRootPath);
                string contentRelative = Roots.GetRelativeToProject(Roots.RococoContentPath);

                string[] args = new[] { srcRelative, contentRelative, sxh };

                var customStep = new Configuration.CustomFileBuildStep
                {
                    KeyInput = sxh,
                    Output = sxh + ".inl",
                    Description = "Generates C++ and SXY code from the " + sxh + " file",
                    Executable = @"$(OutDir)sexy.bennyhill.exe",
                    ExecutableArguments = string.Format("{0} {1} {2}", srcRelative, contentRelative, sxh)
                };

                conf.CustomFileBuildSteps.Add(customStep);
            }
        }
    }

    public class RococoProject : RococoBaseProject
    {
        // Note, default to CPP 17 rather than CPP 20, because on VC I had serious issues were internal compiler errors compiling sexy.script when C+ 20 is set
        public void StandardInit(Configuration conf, Target target, Configuration.OutputType type, int CCPVersion = 17)
        {
            conf.Output = type;
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = Roots.RococoProjectPath;
            Local.SetOptimizations(conf, target);

            AddDefaults(conf, target, CCPVersion);

            var excludedFileSuffixes = new List<string>();

            if (target.Platform == Platform.win64)
            {
                excludedFileSuffixes.Add(@"mac");
            }

            conf.SourceFilesBuildExcludeRegex.Add(@"\.*(" + string.Join("|", excludedFileSuffixes.ToArray()) + @")\.cpp$");
            conf.TargetLibraryPath = Path.Combine(Roots.RococoLibPath, @"[target.Platform]\[conf.Name]\");
        }

        protected RococoProject(string name, string subdir, Platform platform = Platform.win64)
        {
            base.Name = name;
            SetSourcePath(subdir);

            AddTargets(new Target(
                platform,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Dll,
                Blob.NoBlob,
                BuildSystem.MSBuild,
                DotNetFramework.v4_8)
            );
        }

        protected RococoProject(string name): this(name, name)
        {
        }

        protected void AddDefaults(Configuration conf, Target target, int CPPVersion = 17)
        {
            conf.IncludePaths.Add(Roots.RococoIncludePath);
            conf.IncludePaths.Add(Path.Combine(Roots.RococoSexyPath, @"Common\"));

            switch(CPPVersion)
            {
                case 17:
                    conf.Options.Add(Sharpmake.Options.Vc.Compiler.CppLanguageStandard.CPP17);
                    break;
                default:
                    conf.Options.Add(Sharpmake.Options.Vc.Compiler.CppLanguageStandard.CPP20);
                    break;
            } 

            conf.Options.Add(Options.Vc.Compiler.Exceptions.Enable);
            conf.Options.Add(Options.Vc.Compiler.Inline.OnlyInline);

            if (target.Optimization == Optimization.Debug)
            {
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebugDLL);
            }
            else
            {
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);
            }

            conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4458", "4201", "4324", "4250"));
            conf.IntermediatePath = Path.Combine(Roots.RococoTmpPath, @"[target.Name]\[project.Name]\");
            conf.TargetPath = Path.Combine(Roots.RococoBinPath, @"[target.Platform]\[conf.Name]\");
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

            SourceRootPath = Path.Combine(Roots.RococoSourcePath, subdir);
        }
    }

    public class ThirdPartyProject : RococoBaseProject
    {
        // Note, default to CPP 17 rather than CPP 20, because on VC I had serious issues were internal compiler errors compiling sexy.script when C+ 20 is set
        public void StandardInit(Configuration conf, Target target, Configuration.OutputType type, int CCPVersion = 17)
        {
            conf.Output = type;
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = Roots.RococoProjectPath;

            AddDefaults(conf, target, CCPVersion);

            var excludedFileSuffixes = new List<string>();

            if (target.Platform == Platform.win64)
            {
                excludedFileSuffixes.Add(@"mac");
            }

            conf.SourceFilesBuildExcludeRegex.Add(@"\.*(" + string.Join("|", excludedFileSuffixes.ToArray()) + @")\.cpp$");
            conf.TargetLibraryPath = Path.Combine(Roots.RococoLibPath, @"[target.Platform]\[conf.Name]\");

            Local.SetOptimizations(conf, target);
        }

        protected ThirdPartyProject(string name, string subdir)
        {
            base.Name = name;
            SetSourcePath(subdir);

            AddTargets(new Target(
                Platform.win64,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Dll,
                Blob.NoBlob,
                BuildSystem.MSBuild,
                DotNetFramework.v4_8)
            );
        }

        protected void AddDefaults(Configuration conf, Target target, int CPPVersion = 17)
        {
            switch (CPPVersion)
            {
                case 17:
                    conf.Options.Add(Sharpmake.Options.Vc.Compiler.CppLanguageStandard.CPP17);
                    break;
                default:
                    conf.Options.Add(Sharpmake.Options.Vc.Compiler.CppLanguageStandard.CPP20);
                    break;
            }
            conf.Options.Add(Sharpmake.Options.Vc.Compiler.Exceptions.Enable);
            conf.IntermediatePath = Path.Combine(Roots.RococoTmpPath, @"[target.Name]\[project.Name]\");
            conf.TargetPath = Path.Combine(Roots.RococoBinPath, @"[target.Platform]\[conf.Name]\");
            conf.SolutionFolder = " - Third-Party Projects";
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

            SourceRootPath = Path.Combine(Roots.ThirdPartyPath, subdir);
        }
    }

    public class SexyProject : RococoProject
    {
        protected SexyProject(string name, string subdir, Platform platform = Platform.win64) : base(name, subdir, platform)
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

            SourceRootPath = System.IO.Path.Combine(Roots.RococoSexyPath, subdir);
        }
    }

    public class RococoCSharpProject : CSharpProject
    {
        private void SetSourcePath(string subdir)
        {
            if (subdir == null || subdir[0] == 0)
            {
                throw new Exception("Blank subdir");
            }

            if (subdir[0] == '/' || subdir[0] == '\\')
            {
                throw new Exception("The subdirectory must not begin with a directory slash");
            }

            SourceRootPath = Path.Combine(Roots.RococoSourcePath, subdir);
        }

        protected RococoCSharpProject(string name, string subdir)
        {
            base.Name = name;
            SetSourcePath(subdir);

            RootPath = Roots.RococoRootPath;

			AddTargets(new Target(
                Platform.anycpu,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Dll,
                Blob.NoBlob,
                BuildSystem.MSBuild,
                DotNetFramework.v4_8)
            );

            SourceRootPath = Path.Combine(Roots.RococoSourcePath, Name);
        }

        public virtual void ConfigureAll(Configuration conf, Target target)
        {
            conf.ProjectFileName = "[project.Name]";
            conf.ProjectPath = Roots.RococoProjectPath;
            conf.TargetLibraryPath = Path.Combine(Roots.RococoLibPath, @"[target.Platform]\[conf.Name]\");
            conf.IntermediatePath = Path.Combine(Roots.RococoTmpPath, @"[target.Name]\[project.Name]\");
            conf.TargetPath = Path.Combine(Roots.RococoBinPath, @"[target.Platform]\[conf.Name]\");
        }
    }

    public class SexyCSharpProject : CSharpProject
    {
        private void SetSourcePath(string subdir)
        {
            if (subdir == null || subdir[0] == 0)
            {
                throw new Exception("Blank subdir");
            }

            if (subdir[0] == '/' || subdir[0] == '\\')
            {
                throw new Exception("The subdirectory must not begin with a directory slash");
            }

            SourceRootPath = Path.Combine(Roots.RococoSexyPath, subdir);
        }

        protected SexyCSharpProject(string name, string subdir, Platform platform = Platform.win64)
        {
            base.Name = name;
            SetSourcePath(subdir);

            AddTargets(new Target(
                platform,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Dll,
                Blob.NoBlob,
                BuildSystem.MSBuild,
                DotNetFramework.v4_8)
            );
        }

        public virtual void ConfigureAll(Configuration conf, Target target)
        {
            conf.ProjectFileName = "[project.Name]";
            conf.ProjectPath = Roots.RococoProjectPath;
            conf.TargetLibraryPath = Path.Combine(Roots.RococoLibPath, @"[target.Platform]\[conf.Name]\");
            conf.IntermediatePath = Path.Combine(Roots.RococoTmpPath, @"[target.Name]\[project.Name]\");
            conf.TargetPath = Path.Combine(Roots.RococoBinPath, @"[target.Platform]\[conf.Name]\");
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
    public class RococoECSProject : RococoProject
    {
        public RococoECSProject() : base("rococo.ecs")
        {
            SourceFiles.Add(@"..\include\rococo.ecs.h");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.Defines.Add("ROCOCO_ECS_API=__declspec(dllexport)");
            conf.SolutionFolder = "ECS";
        }
    }

    [Sharpmake.Generate]
    public class RococoECSTestProject : RococoProject
    {
        public RococoECSTestProject() : base("rococo.ecs.test")
        {
            SourceFiles.Add(@"..\include\rococo.ecs.h");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Exe);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoComponentsAnimationProject>(target);
            conf.AddPublicDependency<RococoComponentsBodyProject>(target);
            conf.AddPublicDependency<RococoComponentsConfigurationProject>(target);
            conf.AddPublicDependency<RococoComponentsSkeletonProject>(target);
            conf.Defines.Add("ROCOCO_ECS_API=__declspec(dllexport)");
            conf.SolutionFolder = "ECS";
        }
    }

    [Sharpmake.Generate]
    public class RococoComponentsConfigurationProject : RococoProject
    {
        public RococoComponentsConfigurationProject() : base("rococo.component.configuration", @"components\configuration")
        {
            SourceFiles.Add(@"..\..\include\rococo.ecs.h");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoECSProject>(target);
            conf.SolutionFolder = "ECS";
        }
    }

    [Sharpmake.Generate]
    public class RococoComponentsAnimationProject : RococoProject
    {
        public RococoComponentsAnimationProject() : base("rococo.component.animation", @"components\animation")
        {
            SourceFiles.Add(@"..\..\include\rococo.ecs.h");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoECSProject>(target);
            conf.AddPublicDependency<RococoMathsProject>(target);
            conf.SolutionFolder = "ECS";
        }
    }

    [Sharpmake.Generate]
    public class RococoComponentsBodyProject : RococoProject
    {
        public RococoComponentsBodyProject() : base("rococo.component.body", @"components\body")
        {
            SourceFiles.Add(@"..\..\include\rococo.ecs.h");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoECSProject>(target);
            conf.AddPublicDependency<RococoMathsProject>(target);
            conf.SolutionFolder = "ECS";
        }
    }

    [Sharpmake.Generate]
    public class RococoComponentsSkeletonProject : RococoProject
    {
        public RococoComponentsSkeletonProject() : base("rococo.component.skeleton", @"components\skeleton")
        {
            SourceFiles.Add(@"..\..\include\rococo.ecs.h");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoECSProject>(target);
            conf.AddPublicDependency<RococoMathsProject>(target);
            conf.SolutionFolder = "ECS";
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
            conf.AddPublicDependency<LibJPegProject>(target);
            conf.AddPublicDependency<LibTiffProject>(target);
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
            conf.AddPublicDependency<SexyScriptProject>(target);
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
            conf.IncludePaths.Add(Path.Combine(Roots.RococoSexyPath, @"STC\stccore\"));
            conf.AddPublicDependency<RococoSexyIDEProject>(target); 
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoWindowsProject>(target);
            conf.AddPublicDependency<SexyScriptProject>(target);      
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
            conf.AddPublicDependency<SexyBennyHillProject>(target);
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
        public RococoCPPMasterProject() : base("rococo.cpp_master", "rococo.cpp_master")
        {
        }

        [Configure()]
        public override void ConfigureAll(Configuration conf, Target target)
        {
            conf.Output = Configuration.OutputType.DotNetConsoleApp;
            base.ConfigureAll(conf, target);
            conf.TargetLibraryPath = Path.Combine(Roots.RococoLibPath, @"[target.Platform]\[conf.Name]\");
        }
    }

    [Sharpmake.Generate]
    public class RococoMPlatProject : RococoProject
    {
        public RococoMPlatProject() : base("rococo.mplat")
        {
            SourceFiles.Add("mplat.sxh", "config.xc", @"..\include\rococo.mplat.h");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Lib);

            conf.SourceFilesBuildExcludeRegex.Add(@"mplat.component.template.cpp");
            conf.SourceFilesBuildExcludeRegex.Add(@"mplat.component.template.h");
            conf.SourceFilesBuildExcludeRegex.Add(@"mplat.test.app.cpp");
            conf.AddPublicDependency<SexyBennyHillProject>(target);
            
            string makeFilePath = Path.Combine(Roots.RococoToolsPath, "make-components.C++.bat");
            string makeRelative = Roots.GetRelativeToProject(makeFilePath);

            conf.EventPreBuild.Add(makeRelative);

            AddSXHFileBuildStep(conf, "mplat.sxh", "config.xc", true);
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
            conf.AddPublicDependency<RococoFontsProject>(target);
            conf.AddPublicDependency<RococoMathsProject>(target);
            conf.AddPublicDependency<RococoMiscUtilsProject>(target);
            conf.AddPublicDependency<RococoSexyIDEProject>(target);
            conf.AddPublicDependency<RococoWindowsProject>(target);
            conf.AddPublicDependency<RococoFileBrowserProject>(target);
            conf.AddPublicDependency<RococoGuiRetainedProject>(target);
            conf.AddPublicDependency<RococoMPlatProject>(target);
            conf.AddPublicDependency<RococoUtilExProject>(target);
            conf.AddPublicDependency<RococoAudioProject>(target);
            conf.AddPublicDependency<RococoDX11RendererProject>(target);
            conf.AddPublicDependency<RococoComponentsAnimationProject>(target);
            conf.AddPublicDependency<RococoComponentsConfigurationProject>(target);
            conf.AddPublicDependency<RococoComponentsBodyProject>(target);
            conf.AddPublicDependency<RococoComponentsSkeletonProject>(target);
        }
    }

    [Sharpmake.Generate]
    public class RococoMHostProject : RococoProject
    {
        public RococoMHostProject() : base("rococo.mhost")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Exe);
            conf.AddPublicDependency<RococoWindowsProject>(target);
            conf.AddPublicDependency<RococoUtilExProject>(target);
            conf.AddPublicDependency<RococoMathsProject>(target);
            conf.AddPublicDependency<SexyBennyHillProject>(target);
            conf.Options.Add(Options.Vc.Linker.SubSystem.Windows);
            /*
            string makeFilePath = Path.Combine(Roots.RococoToolsPath, "package.mak");
            string makeRelative = Roots.GetRelativeToProject(makeFilePath);
            string contentRelative = Roots.GetRelativeToProject(Roots.RococoContentPath);
            string rococoRelative = Roots.GetRelativeToProject(Roots.RococoRootPath);
            string makeCmd = string.Format("NMAKE /NOLOGO /F \"{0}\" all ROCOCO={1}\\ CONTENT={2}\\", makeRelative, rococoRelative, contentRelative);
            
            conf.EventPostBuild.Add(makeCmd);
            */
        }
    }

    [Sharpmake.Generate]
    public class RococoSexyStudioProject : RococoProject
    {
        public RococoSexyStudioProject() : base("sexystudio")
        {
            SourceFiles.Add("sexystudio.rc");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoMiscUtilsProject>(target);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoWindowsProject>(target);
            conf.AddPublicDependency<SexyScriptProject>(target);
            conf.AddPublicDependency<SexySParserProject>(target);
            conf.AddPublicDependency<SexyUtilProject>(target);
            conf.SolutionFolder = " - SexyStudio";
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
            conf.Options.Add(Sharpmake.Options.Vc.Linker.SubSystem.Windows);
            conf.SolutionFolder = " - SexyStudio";
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
            conf.AddPublicDependency<RococoMiscUtilsProject>(target);
            conf.SolutionFolder = " - SexyStudio";
        }
    }

    [Sharpmake.Generate]
    public class RococoSexyStudio4NPPProject : RococoProject
    {
        public RococoSexyStudio4NPPProject() : base("sexystudio.4.NPP")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.Defines.Add("UNICODE");
            conf.Defines.Add("_ITERATOR_DEBUG_LEVEL=0");
            conf.SolutionFolder = " - SexyStudio";
        }
    }

    [Sharpmake.Generate]
    public class RococoAudioProject : RococoProject
    {
        public RococoAudioProject() : base("rococo.audio")
        {
            base.SourceFiles.Add("rococo.audio.sxh");
            base.SourceFiles.Add("config.xc");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.Defines.Add("ROCOCO_AUDIO_API=__declspec(dllexport)");
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<RococoMathsProject>(target);
            conf.AddPublicDependency<SexyBennyHillProject>(target);

            AddSXHFileBuildStep(conf, "rococo.audio.sxh", "config.xc", true);
        }
    }

    [Sharpmake.Compile]
    public class RococoBuildFinalProject : Project
    {	
        public RococoBuildFinalProject()
        {
            AddTargets(new Target(
               Platform.win64,
               DevEnv.vs2022,
               Optimization.Debug | Optimization.Release,
               OutputType.Dll,
               Blob.NoBlob,
               BuildSystem.MSBuild,
               DotNetFramework.v4_8)
           );
		   
		   Name = "rococo.build.final";
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.ProjectPath = @"..\..\build\hand-coded-projects\";
            conf.ProjectFileName = "rococo.build.final";

            conf.TargetPath = Path.Combine(Roots.RococoRootPath, @"..\..\build\projects\");
            conf.TargetFileName = "rococo.build.final.vcxproj";

            conf.Output = Configuration.OutputType.None; // Required to create dependencies in a compiled/not-generated object

            conf.AddPrivateDependency<RococoPackagerProject>(target, DependencySetting.OnlyBuildOrder);
            conf.AddPrivateDependency<RococoMHostProject>(target, DependencySetting.OnlyBuildOrder);
            conf.AddPrivateDependency<SexyBennyHillProject>(target, DependencySetting.OnlyBuildOrder);
			conf.AddPrivateDependency<RococoSexyStudioTestProject>(target, DependencySetting.OnlyBuildOrder);
            conf.AddPrivateDependency<RococoSexyStudio4NPPProject>(target, DependencySetting.OnlyBuildOrder);
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
            conf.AddPublicDependency<RococoAudioProject>(target);
            conf.AddPublicDependency<RococoUtilsProject>(target);           
            conf.AddPublicDependency<RococoWindowsProject>(target);
            conf.Options.Add(Sharpmake.Options.Vc.Linker.SubSystem.Windows);
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
            conf.SolutionFolder = " - Sexy";
        }
    }

    [Sharpmake.Generate]
    public class SexyDotNetCLIProject : SexyProject
    {
        public SexyDotNetCLIProject() : base("sexy.dotnet.cli", "sexy.dotnet.cli")
        {
 
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.DotNetClassLibrary);
            conf.SolutionFolder = " - Sexy";
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<SexyUtilProject>(target);
            conf.AddPublicDependency<SexyVMProject>(target);
            conf.AddPublicDependency<SexyCompilerProject>(target);
            conf.AddPublicDependency<SexyScriptProject>(target);
            conf.AddPublicDependency<SexySParserProject>(target);
            conf.Options.Add(Options.Vc.General.CommonLanguageRuntimeSupport.ClrSupport);
            conf.Options.Add(Options.Vc.Compiler.Exceptions.EnableWithSEH);
            conf.Options.Add(Options.Vc.Compiler.RTTI.Enable);
        }
    }

    [Sharpmake.Generate]
    public class SexyDotNetIDEProject : SexyCSharpProject
    {
        public SexyDotNetIDEProject() : base("sexy.dotnet.ide", "sexy.dotnet.ide", Platform.win64)
        {
            StartupObject = "SexyDotNet.Program";
        }

        [Configure()]
        public override void ConfigureAll(Configuration conf, Target target)
        {
            conf.SolutionFolder = " - Sexy";
            conf.AddPublicDependency<SexyDotNetCLIProject>(target);
            conf.Output = Configuration.OutputType.DotNetWindowsApp;
            conf.Options.Add(Options.CSharp.AllowUnsafeBlocks.Enabled);
            base.ConfigureAll(conf, target);
            conf.TargetLibraryPath = Path.Combine(Roots.RococoLibPath, @"[target.Platform]\[conf.Name]\");
            if (target.Optimization == Optimization.Debug)
            {
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebugDLL);
            }
            else
            {
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);
            }

            conf.ReferencesByName.Add(
                "System",
                "System.Xaml",
                "System.Xml",
                "WindowsBase",
                "PresentationCore",
                "PresentationFramework"
            );

        }
    }

    [Sharpmake.Generate]
    public class SexyBennyHillProject : SexyProject
    {
        public SexyBennyHillProject() : base("sexy.bennyhill", "sexy.bennyhill")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Exe);
            conf.Defines.Add("SEXYUTIL_API=__declspec(dllexport)");
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<SexySParserProject>(target);
            conf.AddPublicDependency<SexyUtilProject>(target);
            conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4100", "4189"));
            conf.SolutionFolder = " - Sexy";
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
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.SolutionFolder = " - Sexy";
        }
    }

    [Sharpmake.Generate]
    public class SexyCompilerProject : SexyProject
    {
        public SexyCompilerProject() : base("sexy.compiler", "STC/stccore")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Lib);
            conf.SolutionFolder = " - Sexy";
        }
    }

    [Sharpmake.Generate]
    public class SexySParserProject : SexyProject
    {
        public SexySParserProject() : base("sexy.s-parser", "SP/sexy.s-parser")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Lib);
            conf.SolutionFolder = " - Sexy";
        }
    }

    [Sharpmake.Generate]
    public class SexyScriptTestProject : SexyProject
    {
        public SexyScriptTestProject() : base("sexy.script.test", "SS/sexy.script.test")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Exe);
            conf.AddPublicDependency<SexyScriptProject>(target);
            conf.AddPublicDependency<SexyCompilerProject>(target);
            conf.AddPublicDependency<SexyCoroutinesProject>(target);
            conf.AddPublicDependency<SexyReflectionProject>(target);
            conf.SolutionFolder = " - Sexy";
        }
    }

    [Sharpmake.Generate]
    public class SexyCoroutinesProject : SexyProject
    {
        public SexyCoroutinesProject() : base("sexy.nativelib.coroutines", "SS/sexy.nativelib.coroutines")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<SexyUtilProject>(target);
            conf.SolutionFolder = " - Sexy";
        }
    }

    [Sharpmake.Generate]
    public class SexyReflectionProject : SexyProject
    {
        public SexyReflectionProject() : base("sexy.nativelib.reflection", "SS/sexy.nativelib.reflection")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<SexyUtilProject>(target);
            conf.SolutionFolder = " - Sexy";
        }
    }

    [Sharpmake.Generate]
    public class SexyMathsProject : SexyProject
    {
        public SexyMathsProject() : base("sexy.nativelib.maths", "SS/sexy.nativelib.maths")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<SexyUtilProject>(target);
            conf.SolutionFolder = " - Sexy";
        }
    }

    [Sharpmake.Generate]
    public class SexyScriptProject : SexyProject
    {
        public SexyScriptProject() : base("sexy.script", "SS/sexy.script")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<SexyUtilProject>(target);
            conf.AddPublicDependency<SexyCompilerProject>(target);
            conf.AddPublicDependency<SexyVMProject>(target);
            conf.AddPublicDependency<SexySParserProject>(target);
            conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4100", "4189", "4244"));
            conf.Defines.Add("SCRIPTEXPORT_API=__declspec(dllexport)");
            conf.SolutionFolder = " - Sexy";
        }
    }

    [Sharpmake.Generate]
    public class LibTiffProject : ThirdPartyProject
    {
        public LibTiffProject() : base("lib-tiff", @"libtiff\libtiff\")
        {
            Exclude("mkg3states.c", "tif_acorn.c", "tif_apple.c", "tif_msdos.c", "tif_unix.c", "tif_win32.c", "tif_win3.c", "tif_atari.c");
            SourceFiles.Add(Path.Combine(SourceRootPath, @"..\bloke.tiff.cpp"));
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.AddPublicDependency<RococoUtilsProject>(target);
            conf.AddPublicDependency<LibJPegProject>(target);
            conf.AddPublicDependency<LibZipProject>(target);
            conf.IncludePaths.Add(Path.Combine(Roots.ThirdPartyPath, @"libjpg\jpeg-6b\"));
            conf.IncludePaths.Add(Path.Combine(Roots.ThirdPartyPath, @"zlib\"));
            conf.IncludePaths.Add(Roots.RococoIncludePath);
            conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4100", "4244", "4267", "4996", "4456", "4334", "4706", "4133", "4457", "4311", "4324"));
            AddDefaultLibraries(conf);
        }
    }

    [Sharpmake.Generate]
    public class LibZipProject : ThirdPartyProject
    {
        public LibZipProject() : base("lib-zip", @"zlib")
        {
            //Exclude();
            SourceFilesExcludeRegex.Add(@".*contrib\.*");
            SourceFilesExcludeRegex.Add(@".*examples\.*");
            SourceFilesExcludeRegex.Add(@".*test\.*");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.IncludePaths.Add(Roots.RococoIncludePath);
            conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4131", "4324", "4244", "4127", "4996"));
            AddDefaultLibraries(conf);
            conf.Defines.Add("ZLIB_DLL");
        }
    }

    [Sharpmake.Generate]
    public class LibJPegProject : ThirdPartyProject
    {
        public LibJPegProject() : base("lib-jpg", @"libjpg\jpeg-6b\")
        {
            Exclude("ansi2knr.c", "example.c", "ckConfig.c", "cjpeg.c", "djpeg.c", "jmemmac.c", "jmemdos.c", "jmemname.c");
            Exclude("rdjpgcom.c", "wrjpgcom.c", "jpegtran.c", "wrjpgcom.c", "jmemansi.c", "jpegtran.c");
            SourceFiles.Add(Path.Combine(SourceRootPath, @"..\readimage.cpp"));
            SourceFiles.Add(Path.Combine(SourceRootPath, @"..\writeimage.cpp"));
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            StandardInit(conf, target, Configuration.OutputType.Dll);
            conf.IncludePaths.Add(Path.Combine(Roots.ThirdPartyPath, @"libjpg\jpeg-6b\"));
            conf.IncludePaths.Add(Roots.RococoIncludePath);
            conf.IncludePaths.Add(Path.Combine(Roots.ThirdPartyPath, @"zlib"));

            conf.Defines.Add("ROCOCO_JPEG_API=__declspec(dllexport)");

            conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4996", "4100", "4324", "4146", "4244", "4267", "4127", "4702", "4611"));
        }
    }

    [Sharpmake.Generate]
    public class RococoCSharpSolution : /* CSharpSolution */ Solution
    {
		
        public RococoCSharpSolution()
        {
            Name = "rococo.sharpmake";

            AddTargets(new Target(
                Platform.anycpu,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Dll,
                Blob.NoBlob,
                BuildSystem.MSBuild,
                DotNetFramework.v4_8)
            );		
	
			AddTargets(new Target(
                Platform.win64,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Dll,
                Blob.NoBlob,
                BuildSystem.MSBuild,
                DotNetFramework.v4_8)
            );
			
			MergePlatformConfiguration = true;
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.SolutionFileName = "rococo.all";
            conf.SolutionPath = Path.Combine(base.SharpmakeCsPath, @"..\");

            if (conf.Platform == Platform.anycpu)
            {
                // conf.AddProject<RococoCPPMasterProject>(target);
            }
            else
            {
                conf.AddProject<RococoUtilsProject>(target);
                conf.AddProject<SexyUtilProject>(target);
                conf.AddProject<SexyVMProject>(target);
                conf.AddProject<SexyCompilerProject>(target);
                conf.AddProject<SexyScriptProject>(target);
                conf.AddProject<SexySParserProject>(target);
                conf.AddProject<SexyScriptTestProject>(target);
                conf.AddProject<SexyCoroutinesProject>(target);
                conf.AddProject<SexyReflectionProject>(target);
                conf.AddProject<SexyMathsProject>(target);
                conf.AddProject<SexyBennyHillProject>(target);

                conf.AddProject<LibJPegProject>(target);
                conf.AddProject<LibTiffProject>(target);
                conf.AddProject<LibZipProject>(target);

                conf.AddProject<RococoUtilsProject>(target);
                conf.AddProject<SexyUtilProject>(target);
                conf.AddProject<SexyVMProject>(target);
                conf.AddProject<SexyCompilerProject>(target);
                conf.AddProject<SexyScriptProject>(target);
                conf.AddProject<SexySParserProject>(target);
                conf.AddProject<SexyScriptTestProject>(target);
                conf.AddProject<SexyCoroutinesProject>(target);
                conf.AddProject<SexyReflectionProject>(target);
                conf.AddProject<SexyMathsProject>(target);
                conf.AddProject<SexyBennyHillProject>(target);
                conf.AddProject<SexyDotNetCLIProject>(target);

                conf.AddProject<LibJPegProject>(target);
                conf.AddProject<LibTiffProject>(target);
                conf.AddProject<LibZipProject>(target);

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
                conf.AddProject<RococoSexyStudio4NPPProject>(target);
                conf.AddProject<RococoAudioProject>(target);
                conf.AddProject<RococoAudioTestProject>(target);
                conf.AddProject<RococoBuildFinalProject>(target);
                conf.AddProject<RococoECSProject>(target);
                conf.AddProject<RococoComponentsConfigurationProject>(target);
                conf.AddProject<RococoComponentsAnimationProject>(target);
                conf.AddProject<RococoComponentsBodyProject>(target);
                conf.AddProject<RococoComponentsSkeletonProject>(target);
                conf.AddProject<RococoECSTestProject>(target);
                conf.AddProject<SexyDotNetIDEProject>(target);
            }
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
                DotNetFramework.v4_8)
            );
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.SolutionFileName = "sexy.all";
            conf.SolutionPath = Path.Combine(base.SharpmakeCsPath, @"..\");

            conf.AddProject<RococoUtilsProject>(target);
            conf.AddProject<SexyUtilProject>(target);
            conf.AddProject<SexyVMProject>(target);
            conf.AddProject<SexyCompilerProject>(target);
            conf.AddProject<SexyScriptProject>(target);
            conf.AddProject<SexySParserProject>(target);
            conf.AddProject<SexyScriptTestProject>(target);
            conf.AddProject<SexyCoroutinesProject>(target);
            conf.AddProject<SexyReflectionProject>(target);
            conf.AddProject<SexyMathsProject>(target);
            conf.AddProject<SexyBennyHillProject>(target);
            conf.AddProject<SexyDotNetCLIProject>(target);
            conf.AddProject<SexyDotNetIDEProject>(target);
        }
    }

    public static class Main
    {
        [Sharpmake.Main]
        public static void SharpmakeMain(Sharpmake.Arguments arguments)
        {
            arguments.Generate<RococoCSharpSolution>();
            arguments.Generate<SexyCSharpSolution>();

            arguments.Generate<RococoUtilsProject>();
            arguments.Generate<SexyUtilProject>();
			
            arguments.Generate<SexyVMProject>();
            arguments.Generate<SexyCompilerProject>();
            arguments.Generate<SexyScriptProject>();
            arguments.Generate<SexySParserProject>();
            arguments.Generate<SexyScriptTestProject>();
            arguments.Generate<SexyCoroutinesProject>();
            arguments.Generate<SexyReflectionProject>();
            arguments.Generate<SexyMathsProject>();
            arguments.Generate<SexyBennyHillProject>();

            arguments.Generate<LibJPegProject>();
            arguments.Generate<LibTiffProject>();
            arguments.Generate<LibZipProject>();
  
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
            // arguments.Generate<RococoCPPMasterProject>();
            arguments.Generate<RococoMPlatProject>();
            arguments.Generate<RococoMPlatDynamicProject>();
            arguments.Generate<RococoMHostProject>();
            arguments.Generate<RococoSexyStudioProject>();
            arguments.Generate<RococoSexyStudioAppProject>();
            arguments.Generate<RococoSexyStudioTestProject>();
            arguments.Generate<RococoAudioProject>();
            arguments.Generate<RococoAudioTestProject>();
            arguments.Generate<RococoBuildFinalProject>();
            arguments.Generate<RococoSexyStudio4NPPProject>();

            arguments.Generate<RococoECSProject>();
            arguments.Generate<RococoComponentsAnimationProject>();
            arguments.Generate<RococoComponentsConfigurationProject>();
            arguments.Generate<RococoComponentsBodyProject>();
            arguments.Generate<RococoComponentsSkeletonProject>();
            arguments.Generate<RococoECSTestProject>();
            arguments.Generate<SexyDotNetCLIProject>();
            arguments.Generate<SexyDotNetIDEProject>();
        }
    }
}