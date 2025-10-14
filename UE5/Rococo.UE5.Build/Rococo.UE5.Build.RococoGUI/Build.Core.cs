using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Security.Cryptography;
using System.Text;

namespace Rococo
{
    internal class RococoBuilder
    {
        protected readonly string rococoIncludeDirectory;
        protected readonly string rococoSexyIncludeDirectory;
        protected readonly string rococoHomeDirectory;
        protected readonly string rococoSourceDirectory;
        protected readonly string pluginDirectory;
        protected readonly string rococoConfigPath;
        protected readonly string thirdPartyPath;

        public RococoBuilder()
        {
            string userLocalApps = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            rococoConfigPath = Path.Combine(userLocalApps, "19th-Century-Software", "Rococo.cfg");

            if (File.Exists(rococoConfigPath))
            {
                rococoHomeDirectory = File.ReadAllText(rococoConfigPath);
                rococoHomeDirectory = rococoHomeDirectory.Trim();
                if (!Directory.Exists(rococoHomeDirectory))
                {
                    throw new Exception(rococoConfigPath + " exists, but the directory name within did not match a directory: " + rococoHomeDirectory);
                }

                Console.WriteLine("Found rococoConfigPath {0} from config: {1}", rococoHomeDirectory, rococoConfigPath);
            }
            else
            {
                rococoHomeDirectory = FindHomeFromCurrentDirectory();
            }

            rococoIncludeDirectory = Path.Join(rococoHomeDirectory, "source", "rococo", "include");
            rococoSexyIncludeDirectory = Path.Join(rococoHomeDirectory, "source", "rococo", "sexy", "Common");
            rococoSourceDirectory = Path.Join(rococoHomeDirectory, "source");

            if (!Directory.Exists(rococoSourceDirectory))
            {
                throw new System.Exception("Could not find source directory" + rococoSourceDirectory);
            }

            if (!Directory.Exists(rococoSexyIncludeDirectory))
            {
                throw new System.Exception("Could not find sexy include directory" + rococoSexyIncludeDirectory);
            }

            if (!Directory.Exists(rococoIncludeDirectory))
            {
                throw new System.Exception("Could not find rococo include directory" + rococoIncludeDirectory);
            }

            pluginDirectory = Path.Join(rococoHomeDirectory, "UE5", "RococoTestFPS", "Plugins");

            if (!Directory.Exists(pluginDirectory))
            {
                throw new System.Exception("Could not find plugin directory" + pluginDirectory);
            }

            thirdPartyPath = Path.Join(rococoHomeDirectory, "Source", "3rd-Party");
        }

        private static string FindHomeFromCurrentDirectory()
        {
            string startingDir = Directory.GetCurrentDirectory();
            string fullPath = startingDir;
            string lastFullPath;

            do
            {
                lastFullPath = fullPath;

                fullPath = Path.GetFullPath(Path.Combine(fullPath, ".."));

                string candidateIncludeDirectory = Path.Join(fullPath, "source", "rococo", "include");
                if (Directory.Exists(candidateIncludeDirectory))
                {
                    return fullPath;
                }

            } while (lastFullPath != fullPath);

            throw new System.Exception("Could not find rococo directory enumerating ancestors of " + startingDir);
        }

        protected void CreateBundleDirect(string pluginSourceDirectory, string bundleName, string? headerFile, string? prelude, string? postlude, string sourceDirectory, List<string> sourceNames)
        {
            StringBuilder sb = new StringBuilder();

            sourceDirectory = sourceDirectory.Replace('/', Path.DirectorySeparatorChar);

            AppendBanner(sb);

            if (headerFile != null)
            {
                sb.AppendLine("#include \"" + headerFile + "\"");
            }

            if (prelude != null)
            {
                sb.AppendLine(File.ReadAllText(Path.Combine(pluginSourceDirectory, prelude)));
            }

            foreach (var sourceName in sourceNames)
            {
                string fullPath = Path.Combine(rococoSourceDirectory, sourceDirectory, sourceName);
                fullPath = fullPath.Replace("\\", "/");
                if (!File.Exists(fullPath))
                {
                    throw new System.Exception("Could not find bundle file " + fullPath);
                }

                AppendFile(sb, fullPath);
            }

            if (postlude != null)
            {
                sb.AppendLine(File.ReadAllText(Path.Combine(pluginSourceDirectory, postlude)));
            }

            string fullBundlePath = Path.Combine(pluginSourceDirectory, bundleName);
           
            if (!fullBundlePath.EndsWith(".cpp"))
            {
                throw new System.Exception("Expecting bundle file to end with cpp");
            }

            WriteUpdated(sb, fullBundlePath);
        }

        protected void CreateBundleByMatch(string pluginSourceDirectory, string bundleName, string prelude, string postlude, string sourceDirectory, List<string> matchPatterns)
        {
            string fullSrcPath = Path.Combine(rococoSourceDirectory, sourceDirectory);

            var files = new System.Collections.Generic.HashSet<string>();

            foreach (var mp in matchPatterns)
            {
                foreach (var file in Directory.EnumerateFiles(fullSrcPath, mp))
                {
                    files.Add(file);
                }
            }

            List<string> fileList = new List<string>();

            foreach (var file in files)
            {
                fileList.Add(file);
            }

            CreateBundleDirect(pluginSourceDirectory, bundleName, null, prelude, postlude, sourceDirectory, fileList);
        }

        private bool appendFileByLink = true;

        protected bool AppendFileByLink
        {
            get { return appendFileByLink; }
            set { appendFileByLink = value; }
        }

        protected void AppendFile(StringBuilder sb, string fullpath)
        {
            if (appendFileByLink)
            {
                sb.AppendFormat("#include \"{0}\"", fullpath);
                sb.AppendLine();
            }
            else
            {
                sb.AppendFormat("// Origin: {0}", fullpath);
                sb.AppendLine();
                sb.AppendLine(File.ReadAllText(fullpath));
            }
        }

        protected void CreateSeparateFilesDirect(string pluginSourceDirectory, string root, string headerFile, string prelude, string postlude, string sourceDirectory, List<string> sourceNames)
        {
            foreach (var sourceName in sourceNames)
            {
                StringBuilder sb = new StringBuilder();

                AppendBanner(sb);

                if (headerFile != null)
                {
                    sb.AppendLine("#include \"" + headerFile + "\"");
                }

                if (prelude != null)
                {
                    sb.AppendLine(File.ReadAllText(Path.Combine(pluginSourceDirectory, prelude)));
                }

                string fullPath = Path.Combine(rococoSourceDirectory, sourceDirectory, sourceName);
                fullPath = fullPath.Replace("\\", "/");
                if (!File.Exists(fullPath))
                {
                    throw new System.Exception("Could not find bundle file " + fullPath);
                }

                AppendFile(sb, fullPath);

                if (postlude != null)
                {
                    sb.AppendLine(File.ReadAllText(Path.Combine(pluginSourceDirectory, postlude)));
                }

                string wrappedPath = Path.Combine(pluginSourceDirectory, root + sourceName);

                WriteUpdated(sb, wrappedPath);
            }
        }

        protected string MakePluginSourceFolder(string pluginName)
        {
            string dir = Path.Join(pluginDirectory, pluginName, "Source", pluginName, "Private");

            if (!Directory.Exists(dir))
            {
                throw new Exception("Expecting directory to exist: " + dir);
            }

            return dir;
        }

        private void WriteUpdated(StringBuilder sb, string filePath)
        {
            string textToWrite = sb.ToString();

            if (File.Exists(filePath))
            {
                string existingText = File.ReadAllText(filePath);
                if (textToWrite != existingText)
                {
                    Console.WriteLine("Overwrite: {0}", filePath);
                    File.WriteAllText(filePath, textToWrite);
                }
            }
            else
            {
                Console.WriteLine("Add:       {0}", filePath);
                File.WriteAllText(filePath, textToWrite);
            }
        }

        private void WrapHeader(string pluginSourceDirectory, string headerRoot, string srcHeader)
        {
            StringBuilder sb = new StringBuilder();

            AppendBanner(sb);
            sb.AppendLine("#pragma once");
            sb.AppendLine();

            string fullHeaderPath = Path.Join(headerRoot, srcHeader);

            AppendFile(sb, fullHeaderPath);

            string wrappedPath = Path.Join(pluginSourceDirectory, srcHeader);

            WriteUpdated(sb, wrappedPath);
        }
        protected void WrapHeaders(string pluginSourceDirectory, string? headerRoot, List<string> srcHeaderPath)
        {
            string includeRoot;

            if (headerRoot == null)
            {
                includeRoot = rococoIncludeDirectory;
            }
            else
            {
                includeRoot = headerRoot;
            }

            foreach (string srcHeader in srcHeaderPath)
            {
                WrapHeader(pluginSourceDirectory, includeRoot, srcHeader);
            }
        }

        private void AppendBanner(StringBuilder sb)
        {
            sb.AppendLine("// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com.");
            sb.AppendLine("// Bundle generated by Build.Rococo.GUI.cs");
            sb.AppendLine("// Created: " + DateTime.UtcNow.ToString("MMM yyyy") + " UTC");
        }

        protected void CopyOtherPluginFileToHeader(string sourceDirectory, string pluginName, string filename)
        {
            string targetFile = Path.Join(sourceDirectory, filename);
            string sourceFile = Path.Combine(pluginDirectory, pluginName, "Source", pluginName, "Public", filename);

            StringBuilder sb = new StringBuilder();
            AppendBanner(sb);
            AppendFile(sb, sourceFile);

            WriteUpdated(sb, targetFile);
        }

        protected void CopyOtherPluginFileToSource(string sourceDirectory, string pluginName, string filename)
        {
            string targetFile = Path.Join(sourceDirectory, filename);
            string sourceFile = Path.Combine(pluginDirectory, pluginName, "Source", pluginName, "Private", filename);

            StringBuilder sb = new StringBuilder();
            AppendBanner(sb);
            AppendFile(sb, sourceFile);

            WriteUpdated(sb, targetFile);
        }

        protected void CopyFileToSource(string sourceDirectory, string absDirectory, string filename)
        {
            string targetFile = Path.Join(sourceDirectory, filename);
            string sourceFile = Path.Join(absDirectory, filename);

            StringBuilder sb = new StringBuilder();
            AppendBanner(sb);
            AppendFile(sb, sourceFile);

            WriteUpdated(sb, targetFile);
        }

        protected void CopyFilesToSource(string sourceDirectory, string absDirectory, List<string> filenames)
        {
            foreach (string f in filenames)
            {
                CopyFileToSource(sourceDirectory, absDirectory, f);
            }
        }
    }
}