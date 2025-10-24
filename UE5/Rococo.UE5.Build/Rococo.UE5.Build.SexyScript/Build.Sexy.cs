using System.IO;

namespace Rococo.UE5.Build.SexyScript
{
    internal class Program
    {
        static void Main(string[] args)
        {
            RococoSexyBuilder builder = new RococoSexyBuilder();
            builder.Build();
        }
    }

    internal class RococoSexyBuilder : RococoBuilder
    {
        public RococoSexyBuilder() : base(Path.Join("..", "Rococo.Reflect", "RococoUE5SexportTest", "Plugins"))
        {
            AppendFileByLink = true;       
            reflectProjectDirectory = Path.Join(rococoHomeDirectory, "..", "Rococo.Reflect");
            sexyDirectory = Path.Join(rococoSourceDirectory, "rococo", "sexy");

            if (!Directory.Exists(reflectProjectDirectory))
            {
                throw new Exception("Cannot find directory " + reflectProjectDirectory);
            }

            if (!Directory.Exists(sexyDirectory))
            {
                throw new Exception("Cannot find directory " + sexyDirectory);
            }

            sexyPluginDirectory = Path.Join(rococoHomeDirectory, "..", "Rococo.Reflect", "RococoUE5SexportTest", "Plugins", "RococoScript");
        }

        private readonly string reflectProjectDirectory;
        private readonly string sexyDirectory;
        private readonly string sexyPluginDirectory;

        public string CodeGenPath(string subpath)
        {
            return Path.Join("..", "..", "..", "..", "..", "RococoScript.CodeGen", subpath);
        }

        public void Build()
        {
            string sexyPluginPrivateSrcCode = Path.Join(sexyPluginDirectory, "Source", "RococoScript", "Private");
            string sexyPluginPublicSrcCode = Path.Join(sexyPluginDirectory, "Source", "RococoScript", "Public");

            string sexyPluginSSSrcCode = Path.Join(sexyPluginPrivateSrcCode, "SS");

            string ssDirectory = Path.Join(sexyDirectory, "SS" , "sexy.script");
            base.CopyFilesToSourceMatching(sexyPluginSSSrcCode, ssDirectory, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSSSrcCode, ssDirectory, "*.inl");

            string sexyPluginHeaders = Path.Join(sexyPluginPrivateSrcCode, "SexyAPI");
            base.CopyFilesToSourceMatching(sexyPluginHeaders, rococoSexyIncludeDirectory, "*.h");

            string sexyPluginSPSrcCode = Path.Join(sexyPluginPrivateSrcCode, "SP");
            string rococoSexySParserPath = Path.Join(sexyDirectory, "SP", "sexy.s-parser");
            base.CopyFilesToSourceMatching(sexyPluginSPSrcCode, rococoSexySParserPath, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSPSrcCode, rococoSexySParserPath, "*.inl");

            string rococoSexyCoroutinesPath = Path.Join(sexyDirectory, "SS", "sexy.nativelib.coroutines");
            base.CopyFilesToSourceMatching(sexyPluginSSSrcCode, rococoSexyCoroutinesPath, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSSSrcCode, rococoSexyCoroutinesPath, "*.inl");

            string rococoSexyMathsPath = Path.Join(sexyDirectory, "SS", "sexy.nativelib.maths");
            base.CopyFilesToSourceMatching(sexyPluginSSSrcCode, rococoSexyMathsPath, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSSSrcCode, rococoSexyMathsPath, "*.inl");

            string rococoSexyReflectionPath = Path.Join(sexyDirectory, "SS", "sexy.nativelib.reflection");
            base.CopyFilesToSourceMatching(sexyPluginSSSrcCode, rococoSexyReflectionPath, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSSSrcCode, rococoSexyReflectionPath, "*.inl");

            string sexyPluginSTCSrcCode = Path.Join(sexyPluginPrivateSrcCode, "STC");
            string rococoSexySTCPath = Path.Join(sexyDirectory, "STC", "stccore");
            base.CopyFilesToSourceMatching(sexyPluginSTCSrcCode, rococoSexySTCPath, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSTCSrcCode, rococoSexySTCPath, "*.inl");
            base.CopyFilesToSourceMatching(sexyPluginHeaders, rococoSexySTCPath, "*.h");

            string sexyPluginSVMSrcCode = Path.Join(sexyPluginPrivateSrcCode, "SVM");
            string rococoSexySVMPath = Path.Join(sexyDirectory, "SVM", "svmcore");
            base.CopyFilesToSourceMatching(sexyPluginSVMSrcCode, rococoSexySVMPath, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSVMSrcCode, rococoSexySVMPath, "*.inl");

            string rococoPluginHeaders = Path.Join(sexyPluginPrivateSrcCode, "RococoAPI");
            base.CopyFilesToSourceMatching(rococoPluginHeaders, rococoIncludeDirectory, "*.h");

            string rococoPluginBase = Path.Join(sexyPluginPrivateSrcCode, "RococoBase");

            CreateBundleDirect(rococoPluginBase, "wrap.s-utils.cpp", "rococo.UE5.h", null, null, "rococo/sexy/Utilities",
                new List<string>()
                {
                    "sexy.util.cpp"
                }
            );

            CreateBundleDirect(rococoPluginBase, "wrap.rococo_util.cpp", "rococo.os.UE5.h", CodeGenPath("rococo.os.UE5.prelude.h"), CodeGenPath("rococo.os.UE5.postlude.h"), "rococo/rococo.util",
                new List<string>()
                {
                    "rococo.strings.cpp",
                    "rococo.base.cpp",
                    "rococo.heap.string.cpp",
                    "rococo.allocators.cpp",
                    "rococo.throw.cr_sex.cpp"
                }
            );

            string guiPath = Path.Join(rococoHomeDirectory, "UE5", "Plugins", "RococoGuiUltra", "Source", "RococoGui");

            CopyFilesToSource(sexyPluginPrivateSrcCode, Path.Join(guiPath, "Private"), new List<string>
                {
                    "rococo.os.UE5.cpp"
                }
            );

            CopyFilesToSource(sexyPluginPublicSrcCode, Path.Join(guiPath, "Public"), new List<string>
                {
                    "rococo.UE5.h",
                    "rococo.OS.UE5.h"
                }
            );
        }
    }
}
