// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnrealBuildTool;

public class RococoTiffLib : ModuleRules
{
    private string rococoIncludeDirectory;
    private string rococoSexyIncludeDirectory;
    private string rococoHomeDirectory;
    private string rococoSourceDirectory;
    private string thisSourceDirectory;

    private static string RococoConfigPath
    {
        get
        {
            string userLocalApps = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            string rococoConfig = Path.Combine(userLocalApps, "19th-Century-Software", "Rococo.cfg");
            return rococoConfig;
        }
    }

    private static string ReadRococoHomeFromConfig()
    {
        if (File.Exists(RococoConfigPath))
        {
            string rococoPath = File.ReadAllText(RococoConfigPath);
            rococoPath = rococoPath.Trim();
            if (!Directory.Exists(rococoPath))
            {
                throw new Exception(RococoConfigPath + " exists, but the directory name within did not match a directory: " + rococoPath);
            }

            return rococoPath;
        }

        return null;
    }

    private void InitPathFromRococoHome(string _rococoHomeDirectory)
    {
        rococoHomeDirectory = _rococoHomeDirectory;
        rococoIncludeDirectory = Path.Combine(rococoHomeDirectory, "source/rococo/include/").Replace('/', Path.DirectorySeparatorChar);
        rococoSexyIncludeDirectory = Path.Combine(rococoHomeDirectory, "source/rococo/sexy/Common/").Replace('/', Path.DirectorySeparatorChar);
        rococoSourceDirectory = Path.Combine(_rococoHomeDirectory, "source").Replace('/', Path.DirectorySeparatorChar);
    }

    private string PrivateSourceRelPath
    {
        get
        {
            return "Source/RococoTiffLib/Private".Replace('/', Path.DirectorySeparatorChar);
        }
    }

    private void PrepRococoDirectories()
    {
        if (rococoIncludeDirectory == null)
        {
            string dir = PluginDirectory;
            string fullPath = dir;
            string lastFullPath;

            thisSourceDirectory = Path.Combine(PluginDirectory, PrivateSourceRelPath);
            if (!Directory.Exists(thisSourceDirectory))
            {
                throw new System.Exception("Expecting directory to exist: " + thisSourceDirectory);
            }

            string rococoHomeFromConfig = ReadRococoHomeFromConfig();

            if (rococoHomeFromConfig != null)
            {
                InitPathFromRococoHome(rococoHomeFromConfig);
                return;
            }


            do
            {
                lastFullPath = fullPath;

                fullPath = Path.GetFullPath(Path.Combine(fullPath, ".."));

                string candidateIncludeDirectory = Path.Combine(fullPath, "source/rococo/include/").Replace('/', Path.DirectorySeparatorChar);
                if (Directory.Exists(candidateIncludeDirectory))
                {
                    InitPathFromRococoHome(fullPath);
                    return;
                }

            } while (lastFullPath != fullPath);

            throw new System.Exception("Could not find rococo directory from either " + RococoConfigPath + " or  enumerating ancestors of " + dir);
        }
    }
    private void CreateSeparateFilesDirect(string root, string headerFile, string prelude, string postlude, string sourceDirectory, List<string> sourceNames)
    {
        foreach (var sourceName in sourceNames)
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendLine("// Bundle generated by Rococo.TiffLib.Build.cs");
            sb.AppendLine("// Created: " + DateTime.UtcNow.ToString("d MMM yyyy HH:mm:ss") + " UTC");
            sb.AppendLine();

            if (headerFile != null)
            {
                sb.AppendLine("#include \"" + headerFile + "\"");
            }

            if (prelude != null)
            {
                sb.AppendLine(File.ReadAllText(Path.Combine(thisSourceDirectory, prelude)));
            }

            string fullPath = Path.Combine(rococoSourceDirectory, sourceDirectory, sourceName);
            fullPath = fullPath.Replace("\\", "/");
            if (!File.Exists(fullPath))
            {
                throw new System.Exception("Could not find bundle file " + fullPath);
            }

            sb.AppendFormat("#include \"{0}\"", fullPath);
            sb.AppendLine();

            if (postlude != null)
            {
                sb.AppendLine(File.ReadAllText(Path.Combine(thisSourceDirectory, postlude)));
            }

            string wrappedPath = Path.Combine(thisSourceDirectory, root + sourceName);

            if (File.Exists(wrappedPath))
            {
                string existingText = File.ReadAllText(wrappedPath);
                if (sb.ToString() != existingText)
                {
                    File.WriteAllText(wrappedPath, sb.ToString());
                }
            }
            else
            {
                File.WriteAllText(wrappedPath, sb.ToString());
            }
        }
    }
    private void CreateBundles()
    {
        CreateSeparateFilesDirect("wrap.", "rococo.tiff.UE5.h", "rococo.tiff.prelude.h", "rococo.tiff.postlude.h", "3rd-Party/libtiff/libtiff",
            new List<string>()
            {
                "tif_aux.c",
                "tif_close.c",
                "tif_codec.c",
                "tif_color.c",
                "tif_compress.c",
                "tif_dir.c",
                "tif_dirinfo.c",
                "tif_dirread.c",
                "tif_dirwrite.c",
                "tif_dumpmode.c",
                "tif_error.c",
                "tif_extension.c",
                "tif_fax3.c",
                "tif_fax3sm.c",
                "tif_flush.c",
                "tif_getimage.c",
                "tif_hash_set.c",
                "tif_jbig.c",
                "tif_jpeg.c",
                "tif_jpeg_12.c",
                "tif_lerc.c",
                "tif_luv.c",
                "tif_lzma.c",
                "tif_lzw.c",
                "tif_next.c",
                "tif_ojpeg.c",
                "tif_open.c",
                "tif_packbits.c",
                "tif_pixarlog.c",
                "tif_predict.c",
                "tif_print.c",
                "tif_read.c",
                "tif_strip.c",
                "tif_swab.c",
                "tif_thunder.c",
                "tif_tile.c",
                "tif_version.c",
                "tif_warning.c",
                "tif_webp.c",
                "tif_write.c",
                "tif_zip.c",
                "tif_zstd.c"
            }
        );

        CreateSeparateFilesDirect("wrap.", "rococo.tiff.UE5.h", "rococo.tiff.prelude.decl.h", "rococo.tiff.postlude.h", "3rd-Party/libtiff/",
            new List<string>()
            {
                "bloke.tiff.cpp"
            }
        );
    }

    public RococoTiffLib(ReadOnlyTargetRules Target) : base(Target)
	{
        PrepRococoDirectories();
        CreateBundles();

        bEnableExceptions = true;

        if (Target.LinkType == TargetLinkType.Monolithic)
        {
            PublicDefinitions.Add("ROCOCO_BUILD_IS_MONOLITHIC");
        }

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
		
		PublicIncludePaths.AddRange(
			    new string[] 
                {
                     rococoIncludeDirectory
                }
			);
				
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "RococoJPEGLib",
                "RococoZLib"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
