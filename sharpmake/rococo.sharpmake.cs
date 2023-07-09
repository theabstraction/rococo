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
		
		protected RococoProject()
		{
			AddTargets(new Target(
                    Platform.win64,
                    DevEnv.vs2022,
                    Optimization.Debug | Optimization.Release
            ));
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
        public RococoUtilsProject()
        {
            Name = "rococo.util";
            SetSourcePath("rococo.util");
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
        public RococoMathsProject()
        {
            Name = "rococo.maths";
            SetSourcePath("rococo.maths");
        }

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\generated";
			conf.Output = Configuration.OutputType.Dll;
            StandardInit(conf, target);
            conf.AddPublicDependency<RococoUtilsProject>(target);
        }
    }
	
	[Sharpmake.Generate]
    public class RococoUtilExProject : RococoProject
    {
        public RococoUtilExProject()
        {
            Name = "rococo.util.ex";
            SetSourcePath("rococo.util.ex");
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
        public RococoPackagerProject()
        {
            Name = "rococo.packager";
            SetSourcePath("rococo.packager");
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
	
	/*
	msbuild $(ROCOCO)rococo.misc.utils\rococo.misc.utils.vcxproj  $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.windows\rococo.windows.vcxproj        $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.sexy.ide\rococo.sexy.ide.vcxproj      $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.sexy.cmd\rococo.sexy.cmd.vcxproj      $(MSBUILD_TERSE) $(MSBUILD_PARALLEL)
	msbuild $(ROCOCO)rococo.sexy.mathsex\rococo.sexy.mathsex.vcxproj $(MSBUILD_TERSE) $(MSBUILD_PARALLEL) $(WITH_SOLUTION)
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
			conf.AddProject<RococoMathsProject>(target);
			conf.AddProject<RococoUtilExProject>(target);
			conf.AddProject<RococoPackagerProject>(target);
        }
    }

    public static class Main
    {
        [Sharpmake.Main]
        public static void SharpmakeMain(Sharpmake.Arguments arguments)
        {
            arguments.Generate<RococoUtilsProject>();
			arguments.Generate<RococoMathsProject>();
			arguments.Generate<RococoUtilExProject>();
			arguments.Generate<RococoPackagerProject>();
            arguments.Generate<RococoSolution>();
        }
    }
}