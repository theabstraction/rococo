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
            string sexyThirdPartyRococoSrcCode = Path.Join(sexyPluginDirectory, "Source", "ThirdParty", "Rococo");

            string sexyPluginSSSrcCode = Path.Join(sexyThirdPartyRococoSrcCode, "SS");

            string ssDirectory = Path.Join(sexyDirectory, "SS" , "sexy.script");
            base.CopyFilesToSourceMatching(sexyPluginSSSrcCode, ssDirectory, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSSSrcCode, ssDirectory, "*.inl");

            string sexyPluginHeaders = Path.Join(sexyThirdPartyRococoSrcCode, "SexyAPI");
            base.CopyFilesToSourceMatching(sexyPluginHeaders, rococoSexyIncludeDirectory, "*.h");

            string sexyPluginSPSrcCode = Path.Join(sexyThirdPartyRococoSrcCode, "SP");
            string rococoSexySParserPath = Path.Join(sexyDirectory, "SP", "sexy.s-parser");
            base.CopyFilesToSourceMatching(sexyPluginSPSrcCode, rococoSexySParserPath, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSPSrcCode, rococoSexySParserPath, "*.inl");

            string rococoSexyCoroutinesPath = Path.Join(sexyDirectory, "SS", "sexy.nativelib.coroutines");
            base.CopyFilesToSourceMatching(Path.Join(sexyThirdPartyRococoSrcCode, "SexyNativeLib_Coroutines"), rococoSexyCoroutinesPath, "*.cpp");
            base.CopyFilesToSourceMatching(Path.Join(sexyThirdPartyRococoSrcCode, "SexyNativeLib_Coroutines"), rococoSexyCoroutinesPath, "*.inl");

            string rococoSexyMathsPath = Path.Join(sexyDirectory, "SS", "sexy.nativelib.maths");
            base.CopyFilesToSourceMatching(Path.Join(sexyThirdPartyRococoSrcCode, "SexyNativeLib_Maths"), rococoSexyMathsPath, "*.cpp");
            base.CopyFilesToSourceMatching(Path.Join(sexyThirdPartyRococoSrcCode, "SexyNativeLib_Maths"), rococoSexyMathsPath, "*.inl");

            string rococoSexyReflectionPath = Path.Join(sexyDirectory, "SS", "sexy.nativelib.reflection");
            base.CopyFilesToSourceMatching(Path.Join(sexyThirdPartyRococoSrcCode, "SexyNativeLib_Reflection"), rococoSexyReflectionPath, "*.cpp");
            base.CopyFilesToSourceMatching(Path.Join(sexyThirdPartyRococoSrcCode, "SexyNativeLib_Reflection"), rococoSexyReflectionPath, "*.inl");

            string sexyPluginSTCSrcCode = Path.Join(sexyThirdPartyRococoSrcCode, "STC");
            string rococoSexySTCPath = Path.Join(sexyDirectory, "STC", "stccore");
            base.CopyFilesToSourceMatching(sexyPluginSTCSrcCode, rococoSexySTCPath, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSTCSrcCode, rococoSexySTCPath, "*.inl");
            base.CopyFilesToSourceMatching(sexyPluginHeaders, rococoSexySTCPath, "*.h");

            string sexyPluginSVMSrcCode = Path.Join(sexyThirdPartyRococoSrcCode, "SVM");
            string rococoSexySVMPath = Path.Join(sexyDirectory, "SVM", "svmcore");
            base.CopyFilesToSourceMatching(sexyPluginSVMSrcCode, rococoSexySVMPath, "*.cpp");
            base.CopyFilesToSourceMatching(sexyPluginSVMSrcCode, rococoSexySVMPath, "*.inl");

            string rococoPluginHeaders = Path.Join(sexyThirdPartyRococoSrcCode, "RococoAPI");
            base.CopyFilesToSourceMatching(rococoPluginHeaders, rococoIncludeDirectory, "*.h");

            string rococoAllocationPluginHeaders = Path.Join(sexyThirdPartyRococoSrcCode, "RococoAPI", "allocators");
            base.CopyFilesToSourceMatching(rococoAllocationPluginHeaders, Path.Join(rococoIncludeDirectory, "allocators"), "*.h");
            base.CopyFilesToSourceMatching(rococoAllocationPluginHeaders, Path.Join(rococoIncludeDirectory, "allocators"), "*.inl");

            string rococoPluginOS = Path.Join(sexyThirdPartyRococoSrcCode, "RococoOS");

            CopyFilesToSourceMatching(Path.Join(sexyThirdPartyRococoSrcCode, "SexyUtils"), Path.Join(sexyDirectory, "Utilities"), "*.cpp");

            CreateBundleDirect(rococoPluginOS, "wrap.rococo_util.cpp", null, CodeGenPath("rococo.os.UE5.prelude.h"), CodeGenPath("rococo.os.UE5.postlude.h"), "rococo/rococo.util",
                new List<string>()
                {
                    "rococo.strings.cpp",
                    "rococo.base.cpp",
                    "rococo.heap.string.cpp",
                    "rococo.allocators.cpp",
                    "rococo.throw.cr_sex.cpp",
                    "rococo.os.cpp"
                }
            );

            CopyFileToSource(rococoPluginOS, "rococo/rococo.util", "rococo.os.win32.inl");

            string guiPath = Path.Join(rococoHomeDirectory, "UE5", "Plugins", "RococoGuiUltra", "Source", "RococoGui");

            CopyFilesToSource(sexyPluginPrivateSrcCode, Path.Join(guiPath, "Private"), new List<string>
                {
                    "rococo.os.UE5.cpp"
                }
            );
        }
    }
}
