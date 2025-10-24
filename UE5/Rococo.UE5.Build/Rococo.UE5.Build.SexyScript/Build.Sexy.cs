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

        public void Build()
        {
            string ssDirectory = Path.Join(sexyDirectory, "SS" , "sexy.script");
            base.CopyFilesToSourceMatching(ssDirectory, sexyPluginDirectory, "*.cpp");
            base.CopyFilesToSourceMatching(ssDirectory, sexyPluginDirectory, "*.inl");
        }
    }
}
