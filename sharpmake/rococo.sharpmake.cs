// rococo.sharpmake.cs
using Sharpmake;
using System; 
using System.Collections.Generic;

namespace Rococo
{
    public class RococoProject: Project
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
		
		protected RococoProject(string name)
		{
            base.Name = name;
            SetSourcePath(name);

            AddTargets(new Target(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release));
		}
		
		protected void AddDefaults(Configuration conf, Target target)
		{
			conf.IncludePaths.Add(RococoRootPath + @"include\");
			conf.IncludePaths.Add(RococoSexyPath + @"Common\");
			conf.Options.Add(Sharpmake.Options.Vc.Compiler.CppLanguageStandard.CPP20);
            conf.Options.Add(Sharpmake.Options.Vc.Compiler.Exceptions.Enable);
            conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4458","4201","4324"));
            conf.IntermediatePath = RococoTmpPath + @"[target.Name]\[project.Name]\";
            conf.TargetPath = RococoBinPath + @"[target.Platform]\[conf.Name]\";
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
		
		public void StandardInit(Configuration conf, Target target)
		{
			AddDefaults(conf, target);
			
			var excludedFileSuffixes = new List<string>();
			
			if (target.Platform == Platform.win64)
			{
				excludedFileSuffixes.Add(@"mac");
			}
			
			conf.SourceFilesBuildExcludeRegex.Add(@"\.*(" + string.Join("|", excludedFileSuffixes.ToArray()) + @")\.cpp$");
            conf.TargetLibraryPath = RococoLibPath + @"[target.Platform]\[conf.Name]\";
        }
    }

    [Sharpmake.Generate]
    public class RococoUtilsProject : RococoProject
    {
        public RococoUtilsProject(): base("rococo.util")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
			conf.Output = Configuration.OutputType.Dll;
			StandardInit(conf, target);
        }
    }
	
	[Sharpmake.Generate]
    public class RococoMathsProject : RococoProject
    {
        public RococoMathsProject(): base("rococo.maths")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
			conf.Output = Configuration.OutputType.Lib;
            StandardInit(conf, target);
        }
    }
	
	[Sharpmake.Generate]
    public class RococoUtilExProject : RococoProject
    {
        public RococoUtilExProject(): base("rococo.util.ex")
        {
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
			conf.Output = Configuration.OutputType.Dll;
            conf.Defines.Add("ROCOCO_UTILS_EX_API=__declspec(dllexport)");
            conf.Defines.Add("RENDERER_API=__declspec(dllexport)");
            StandardInit(conf, target);
            conf.AddPublicDependency<RococoUtilsProject>(target);
        }
    }
	
	[Sharpmake.Generate]
    public class RococoPackagerProject : RococoProject
    {
        public RococoPackagerProject(): base("rococo.packager")
        {
            SetSourcePath(Name);
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
			conf.Output = Configuration.OutputType.Exe;
			StandardInit(conf, target);
			
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
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
            conf.Output = Configuration.OutputType.Dll;
            StandardInit(conf, target);
            conf.AddPublicDependency<RococoUtilsProject>(target);
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
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
            conf.Output = Configuration.OutputType.Dll;
            StandardInit(conf, target);
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
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
            conf.Output = Configuration.OutputType.Lib;
            StandardInit(conf, target);
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
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
            conf.Output = Configuration.OutputType.Exe;
            StandardInit(conf, target);

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
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
            conf.Output = Configuration.OutputType.Dll;
            StandardInit(conf, target);

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
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
            conf.Output = Configuration.OutputType.Lib;
            StandardInit(conf, target);
        }
    }

    /*
	msbuild $(ROCOCO)rococo.fonts\fonts.vcxproj                      $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(ROCOCO)dx11.renderer\dx11.renderer.vcxproj             $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(ROCOCO)rococo.file.browser\rococo.file.browser.vcxproj $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(DIR_GUI_RETAINED)rococo.gui.retained.vcxproj           $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
	msbuild $(ROCOCO)rococo.cpp_master\rococo.cpp_master.csproj $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	*/

    [Sharpmake.Generate]
    public class RococoSolution : Sharpmake.Solution
    {
        public RococoSolution()
        {
            Name = "rococo.sharpmake";

            AddTargets(new Target(
                    Platform.win64,
                    DevEnv.vs2022,
                    Optimization.Debug | Optimization.Release
            ));
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.SolutionFileName = "[solution.Name]_[target.DevEnv]_[target.Platform]";
            conf.SolutionPath = @"[solution.SharpmakeCsPath]\generated";
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
        }
    }

    public static class Main
    {
        [Sharpmake.Main]
        public static void SharpmakeMain(Sharpmake.Arguments arguments)
        {
            arguments.Generate<RococoUtilsProject>();
            arguments.Generate<RococoMiscUtilsProject>();
            arguments.Generate<RococoMathsProject>();
			arguments.Generate<RococoUtilExProject>();
			arguments.Generate<RococoPackagerProject>();
            arguments.Generate<RococoWindowsProject>();
            arguments.Generate<RococoSolution>();
            arguments.Generate<RococoSexyIDEProject>();
            arguments.Generate<RococoSexyCmdProject>();
            arguments.Generate<RococoSexyMathSexProject>();
            arguments.Generate<RococoFontsProject>();
        }
    }
}