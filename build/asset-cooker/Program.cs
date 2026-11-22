using System.Diagnostics;
using System.Text.RegularExpressions;

namespace AssetCooker;

static class Program
{
    const string DefaultDirectXTex = @"D:\work\DirectXTex";

    static int Main(string[] args)
    {
        try
        {
            if (!Options.TryParse(args, out var options, out var error))
            {
                if (error is null)
                {
                    Options.PrintHelp();
                    return 0;
                }

                Console.Error.WriteLine(error);
                Console.Error.WriteLine("Use --help for usage.");
                return 1;
            }

            return Run(options);
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Fatal: {ex.Message}");
            return 1;
        }
    }

    static int Run(Options options)
    {
        if (!Directory.Exists(options.AssetsRoot))
        {
            Console.Error.WriteLine($"Assets directory not found: {options.AssetsRoot}");
            return 1;
        }

        string texconv;
        try
        {
            texconv = TexconvLocator.Resolve(options.TexconvPath, DefaultDirectXTex);
        }
        catch (FileNotFoundException ex)
        {
            Console.Error.WriteLine(ex.Message);
            return 1;
        }

        Console.WriteLine($"Assets:  {options.AssetsRoot}");
        Console.WriteLine($"Output:  {options.OutputRoot}");
        Console.WriteLine($"texconv: {texconv}");
        if (options.DryRun) Console.WriteLine("Mode:    dry-run (no files will be written)");
        Console.WriteLine();

        var inventory = Inventory.Scan(options.AssetsRoot);

        if (options.Verbose)
        {
            if (inventory.Unmatched.Count == 0)
            {
                Console.WriteLine("Unmatched TIFFs: none");
            }
            else
            {
                Console.WriteLine($"Unmatched TIFFs ({inventory.Unmatched.Count}):");
                foreach (var path in inventory.Unmatched)
                    Console.WriteLine($"  {path}");
            }
            Console.WriteLine();
        }

        if (inventory.ColourWithoutNormal.Count > 0)
        {
            Console.WriteLine($"Colour maps with no normal ({inventory.ColourWithoutNormal.Count}):");
            foreach (var map in inventory.ColourWithoutNormal)
                Console.WriteLine($"  {map.Path}  (expected {map.DirectoryName}{map.Index}_n.tif(f) in the same folder)");
            Console.WriteLine();
        }

        if (inventory.OrphanNormals.Count > 0)
        {
            Console.WriteLine($"Orphan normal maps ({inventory.OrphanNormals.Count}):");
            foreach (var path in inventory.OrphanNormals)
                Console.WriteLine($"  {path}");
            Console.WriteLine();
        }

        var jobs = inventory.BuildJobs(options);
        var stats = Cooker.Run(texconv, options, jobs);

        Console.WriteLine("Summary");
        Console.WriteLine($"  Colour maps found:     {inventory.ColourMaps.Count}");
        Console.WriteLine($"  With normals:          {inventory.ColourMaps.Count - inventory.ColourWithoutNormal.Count}");
        Console.WriteLine($"  Colour, no normal:     {inventory.ColourWithoutNormal.Count}");
        Console.WriteLine($"  Orphan normals:        {inventory.OrphanNormals.Count}");
        Console.WriteLine($"  Unmatched TIFFs:       {inventory.Unmatched.Count}");
        Console.WriteLine($"  Cooked:                {stats.Cooked}");
        Console.WriteLine($"  Skipped (up to date):  {stats.Skipped}");
        Console.WriteLine($"  Failed:                {stats.Failed}");
        if (options.DryRun) Console.WriteLine("  (dry-run: nothing written)");

        return stats.Failed > 0 ? 1 : 0;
    }
}

sealed class Options
{
    public string AssetsRoot { get; init; } = DefaultAssets;
    public string OutputRoot { get; init; } = DefaultOut;
    public string? TexconvPath { get; init; }
    public bool Force { get; init; }
    public bool DryRun { get; init; }
    public bool Verbose { get; init; }
    public string ColourFormat { get; init; } = DefaultColourFormat;
    public string NormalFormat { get; init; } = DefaultNormalFormat;
    public int Parallel { get; init; } = Math.Max(1, Environment.ProcessorCount);
    public bool SepAlpha { get; init; }
    public bool InvertY { get; init; }
    public bool NoGpu { get; init; }
    public int? GpuIndex { get; init; }
    public string? BcFlags { get; init; }

    const string DefaultAssets = @"D:\work\assets";
    const string DefaultOut = @"D:\work\assets-cooked";
    const string DefaultColourFormat = "BC7_UNORM_SRGB";
    const string DefaultNormalFormat = "BC5_UNORM";

    public static bool TryParse(string[] args, out Options options, out string? error)
    {
        options = null!;
        error = null;

        var assets = DefaultAssets;
        var output = DefaultOut;
        string? texconv = null;
        var force = false;
        var dryRun = false;
        var verbose = false;
        var colourFormat = DefaultColourFormat;
        var normalFormat = DefaultNormalFormat;
        var parallel = Math.Max(1, Environment.ProcessorCount);
        var sepAlpha = false;
        var invertY = false;
        var noGpu = false;
        int? gpuIndex = null;
        string? bcFlags = null;

        for (int i = 0; i < args.Length; i++)
        {
            var arg = args[i];
            switch (arg)
            {
                case "-h":
                case "--help":
                case "/?":
                    return false;

                case "--assets":
                    if (!RequireValue(args, ref i, arg, out assets, out error)) return false;
                    break;
                case "--out":
                    if (!RequireValue(args, ref i, arg, out output, out error)) return false;
                    break;
                case "--texconv":
                    if (!RequireValue(args, ref i, arg, out var tex, out error)) return false;
                    texconv = tex;
                    break;
                case "--force":
                    force = true;
                    break;
                case "--dry-run":
                    dryRun = true;
                    break;
                case "--verbose":
                    verbose = true;
                    break;
                case "--colour-format":
                case "--color-format":
                    if (!RequireValue(args, ref i, arg, out colourFormat, out error)) return false;
                    break;
                case "--normal-format":
                    if (!RequireValue(args, ref i, arg, out normalFormat, out error)) return false;
                    break;
                case "--parallel":
                    if (!RequireValue(args, ref i, arg, out var parallelText, out error)) return false;
                    if (!int.TryParse(parallelText, out parallel) || parallel < 1)
                    {
                        error = "--parallel must be an integer >= 1.";
                        return false;
                    }
                    break;
                case "--sep-alpha":
                    sepAlpha = true;
                    break;
                case "--invert-y":
                    invertY = true;
                    break;
                case "--nogpu":
                    noGpu = true;
                    break;
                case "--gpu":
                    if (!RequireValue(args, ref i, arg, out var gpuText, out error)) return false;
                    if (!int.TryParse(gpuText, out var gpu) || gpu < 0)
                    {
                        error = "--gpu must be an integer >= 0.";
                        return false;
                    }
                    gpuIndex = gpu;
                    break;
                case "--bc":
                    if (!RequireValue(args, ref i, arg, out bcFlags, out error)) return false;
                    break;
                default:
                    error = $"Unknown argument: {arg}";
                    return false;
            }
        }

        if (noGpu && gpuIndex is not null)
        {
            error = "Use only one of --nogpu and --gpu.";
            return false;
        }

        options = new Options
        {
            AssetsRoot = Path.GetFullPath(assets),
            OutputRoot = Path.GetFullPath(output),
            TexconvPath = texconv is null ? null : Path.GetFullPath(texconv),
            Force = force,
            DryRun = dryRun,
            Verbose = verbose,
            ColourFormat = colourFormat,
            NormalFormat = normalFormat,
            Parallel = parallel,
            SepAlpha = sepAlpha,
            InvertY = invertY,
            NoGpu = noGpu,
            GpuIndex = gpuIndex,
            BcFlags = bcFlags
        };
        return true;
    }

    static bool RequireValue(string[] args, ref int i, string name, out string value, out string? error)
    {
        if (i + 1 >= args.Length || args[i + 1].StartsWith("--", StringComparison.Ordinal))
        {
            value = "";
            error = $"{name} requires a value.";
            return false;
        }

        value = args[++i];
        error = null;
        return true;
    }

    public static void PrintHelp()
    {
        Console.WriteLine(
@"AssetCooker — convert matching albedo/normal TIFFs to compressed DDS via DirectXTex texconv.

Usage:
  dotnet run --project AssetCooker.csproj -- [options]
  AssetCooker.exe [options]

A colour (albedo) file matches when its stem equals its immediate parent
directory name, followed by an integer index:

    <dir>/<dir><index>.tif(f)     e.g.  brick/brick01.tif
    <dir>/<dir><index>_n.tif(f)   optional normal in the same folder

Cooked files are written to the same relative path under the output root:

    <out>/<relative-dir>/<dir><index>.dds
    <out>/<relative-dir>/<dir><index>_n.dds

Source TIFFs are never deleted or moved.

Options:
  --assets <dir>            Source asset root (default: D:\work\assets)
  --out <dir>               Cooked output root (default: D:\work\assets-cooked)
  --texconv <path>          Path to texconv.exe (otherwise searched under D:\work\DirectXTex)
  --force                   Recook even if the .dds is newer than the source TIFF
  --dry-run                 Print planned work; do not write files or create directories
  --verbose                 List TIFFs that do not match the naming convention
  --colour-format <fmt>     DXGI format for colour maps (default: BC7_UNORM_SRGB)
  --normal-format <fmt>     DXGI format for normal maps (default: BC5_UNORM)
  --parallel <n>            Max concurrent texconv processes (default: processor count)
  --sep-alpha               Pass -sepalpha on colour maps (alpha is not transparency)
  --invert-y                Pass -inverty on normal maps (OpenGL-style green channel)
  --nogpu                   Pass -nogpu (force CPU BC6H/BC7 compression)
  --gpu <index>             Pass -gpu <index> (DirectCompute adapter)
  --bc <flags>              Pass -bc <flags> (u, d, q, x — see texconv docs)
  --help                    Show this help

Colour texconv line (defaults):
  texconv -nologo -y -f BC7_UNORM_SRGB -m 0 -srgb -o <outDir> <file>.tif

Normal texconv line (defaults):
  texconv -nologo -y -f BC5_UNORM -m 0 -o <outDir> <file>_n.tif

An existing .dds is skipped when it is newer than or equal to its source TIFF,
unless --force is set. The process exits with code 1 if any conversion fails.");
    }
}

static class TexconvLocator
{
    public static string Resolve(string? explicitPath, string directXTexRoot)
    {
        if (!string.IsNullOrWhiteSpace(explicitPath))
        {
            if (!File.Exists(explicitPath))
                throw new FileNotFoundException($"texconv not found at --texconv path: {explicitPath}");
            return explicitPath;
        }

        foreach (var candidate in Candidates(directXTexRoot))
        {
            if (File.Exists(candidate))
                return candidate;
        }

        var fromPath = FindOnPath();
        if (fromPath is not null)
            return fromPath;

        throw new FileNotFoundException(
            "texconv.exe not found. Pass --texconv <path>, add it to PATH, or build DirectXTex so it lands in one of:\n" +
            string.Join('\n', Candidates(directXTexRoot).Select(p => "  " + p)));
    }

    public static IEnumerable<string> Candidates(string root)
    {
        string[] ide = ["Desktop_2022", "Desktop_2022_Win10", "Desktop_2022_Win11", "Desktop_2019"];
        string[] platform = ["x64", "Win32", "x86", "ARM64"];
        string[] config = ["Release", "Debug"];

        foreach (var c in config)
        {
            foreach (var vs in ide)
            {
                foreach (var p in platform)
                    yield return Path.Combine(root, "Texconv", "Bin", vs, p, c, "texconv.exe");
            }

            yield return Path.Combine(root, "Texconv", "x64", c, "texconv.exe");
            yield return Path.Combine(root, "Texconv", c, "texconv.exe");
            yield return Path.Combine(root, "bin", "x64", c, "texconv.exe");
        }
    }

    static string? FindOnPath()
    {
        var path = Environment.GetEnvironmentVariable("PATH") ?? "";
        var names = OperatingSystem.IsWindows()
            ? new[] { "texconv.exe", "texconv" }
            : new[] { "texconv", "texconv.exe" };

        foreach (var dir in path.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries))
        {
            foreach (var name in names)
            {
                var candidate = Path.Combine(dir, name);
                if (File.Exists(candidate))
                    return candidate;
            }
        }

        return null;
    }
}

enum MapKind { Colour, Normal }

sealed class ColourMap
{
    public required string Path { get; init; }
    public required string DirectoryName { get; init; }
    public required string Index { get; init; }
    public string? NormalPath { get; init; }
}

sealed class Inventory
{
    public List<ColourMap> ColourMaps { get; } = [];
    public List<ColourMap> ColourWithoutNormal { get; } = [];
    public List<string> OrphanNormals { get; } = [];
    public List<string> Unmatched { get; } = [];

    public static Inventory Scan(string assetsRoot)
    {
        var result = new Inventory();
        var tiffs = EnumerateTiffs(assetsRoot).OrderBy(p => p, StringComparer.OrdinalIgnoreCase).ToList();

        // Key: (parent-dir full path, index string as written in the filename)
        var colourByKey = new Dictionary<(string Dir, string Index), ColourMap>(new DirIndexComparer());
        var normalsByKey = new Dictionary<(string Dir, string Index), string>(new DirIndexComparer());

        foreach (var file in tiffs)
        {
            var dir = Path.GetDirectoryName(file) ?? "";
            var dirName = Path.GetFileName(dir);
            var stem = Path.GetFileNameWithoutExtension(file);
            if (string.IsNullOrEmpty(dirName))
            {
                result.Unmatched.Add(file);
                continue;
            }

            var colourPattern = new Regex("^" + Regex.Escape(dirName) + @"(\d+)$", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
            var normalPattern = new Regex("^" + Regex.Escape(dirName) + @"(\d+)_n$", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);

            var colourMatch = colourPattern.Match(stem);
            if (colourMatch.Success)
            {
                var map = new ColourMap
                {
                    Path = file,
                    DirectoryName = dirName,
                    Index = colourMatch.Groups[1].Value
                };
                var key = (dir, map.Index);
                if (colourByKey.TryGetValue(key, out var existing))
                {
                    Console.Error.WriteLine($"Warning: duplicate colour map for {dirName}{map.Index}: {existing.Path} and {file}");
                }
                else
                {
                    colourByKey[key] = map;
                }
                continue;
            }

            var normalMatch = normalPattern.Match(stem);
            if (normalMatch.Success)
            {
                var index = normalMatch.Groups[1].Value;
                var key = (dir, index);
                if (normalsByKey.TryGetValue(key, out var existing))
                    Console.Error.WriteLine($"Warning: duplicate normal map for {dirName}{index}_n: {existing} and {file}");
                else
                    normalsByKey[key] = file;
                continue;
            }

            result.Unmatched.Add(file);
        }

        foreach (var (key, colour) in colourByKey)
        {
            if (normalsByKey.TryGetValue(key, out var normalPath))
            {
                var withNormal = new ColourMap
                {
                    Path = colour.Path,
                    DirectoryName = colour.DirectoryName,
                    Index = colour.Index,
                    NormalPath = normalPath
                };
                result.ColourMaps.Add(withNormal);
            }
            else
            {
                result.ColourMaps.Add(colour);
                result.ColourWithoutNormal.Add(colour);
            }
        }

        result.ColourMaps.Sort((a, b) => string.Compare(a.Path, b.Path, StringComparison.OrdinalIgnoreCase));
        result.ColourWithoutNormal.Sort((a, b) => string.Compare(a.Path, b.Path, StringComparison.OrdinalIgnoreCase));

        foreach (var (key, path) in normalsByKey)
        {
            if (!colourByKey.ContainsKey(key))
                result.OrphanNormals.Add(path);
        }

        result.OrphanNormals.Sort(StringComparer.OrdinalIgnoreCase);
        return result;
    }

    public List<CookJob> BuildJobs(Options options)
    {
        var jobs = new List<CookJob>();
        foreach (var map in ColourMaps)
        {
            var relativeDir = RelativeOutputDir(options.AssetsRoot, map.Path);
            var outDir = string.IsNullOrEmpty(relativeDir)
                ? options.OutputRoot
                : Path.Combine(options.OutputRoot, relativeDir);

            jobs.Add(new CookJob
            {
                Kind = MapKind.Colour,
                InputPath = map.Path,
                OutputDir = outDir,
                OutputPath = Path.Combine(outDir, Path.GetFileNameWithoutExtension(map.Path) + ".dds")
            });

            if (map.NormalPath is not null)
            {
                jobs.Add(new CookJob
                {
                    Kind = MapKind.Normal,
                    InputPath = map.NormalPath,
                    OutputDir = outDir,
                    OutputPath = Path.Combine(outDir, Path.GetFileNameWithoutExtension(map.NormalPath) + ".dds")
                });
            }
        }

        return jobs;
    }

    static string RelativeOutputDir(string assetsRoot, string filePath)
    {
        var parent = Path.GetDirectoryName(filePath) ?? assetsRoot;
        var relative = Path.GetRelativePath(assetsRoot, parent);
        if (string.IsNullOrEmpty(relative) || relative == ".")
            return "";
        return relative;
    }

    static IEnumerable<string> EnumerateTiffs(string root)
    {
        foreach (var file in Directory.EnumerateFiles(root, "*.*", SearchOption.AllDirectories))
        {
            var ext = Path.GetExtension(file);
            if (TiffExtensions.Any(e => ext.Equals(e, StringComparison.OrdinalIgnoreCase)))
                yield return file;
        }
    }

    static readonly string[] TiffExtensions = [".tif", ".tiff"];

    sealed class DirIndexComparer : IEqualityComparer<(string Dir, string Index)>
    {
        public bool Equals((string Dir, string Index) x, (string Dir, string Index) y) =>
            string.Equals(x.Dir, y.Dir, StringComparison.OrdinalIgnoreCase) &&
            string.Equals(x.Index, y.Index, StringComparison.Ordinal);

        public int GetHashCode((string Dir, string Index) obj) =>
            HashCode.Combine(StringComparer.OrdinalIgnoreCase.GetHashCode(obj.Dir), obj.Index);
    }
}

sealed class CookJob
{
    public required MapKind Kind { get; init; }
    public required string InputPath { get; init; }
    public required string OutputDir { get; init; }
    public required string OutputPath { get; init; }
}

sealed class CookStats
{
    public int Cooked;
    public int Skipped;
    public int Failed;
}

static class Cooker
{
    public static CookStats Run(string texconv, Options options, List<CookJob> jobs)
    {
        var stats = new CookStats();
        var gate = new object();

        var parallelOptions = new ParallelOptions { MaxDegreeOfParallelism = options.Parallel };
        Parallel.ForEach(jobs, parallelOptions, job =>
        {
            var args = BuildArgs(job, options);
            var commandLine = FormatCommandLine(texconv, args);

            if (!options.Force && File.Exists(job.OutputPath) &&
                File.GetLastWriteTimeUtc(job.OutputPath) >= File.GetLastWriteTimeUtc(job.InputPath))
            {
                lock (gate)
                {
                    stats.Skipped++;
                    Console.WriteLine($"skip  {job.Kind,-6}  {job.OutputPath}  (up to date)");
                }
                return;
            }

            if (options.DryRun)
            {
                lock (gate)
                {
                    stats.Cooked++;
                    Console.WriteLine($"cook  {job.Kind,-6}  {commandLine}");
                }
                return;
            }

            Directory.CreateDirectory(job.OutputDir);

            var result = Invoke(texconv, args, job.OutputDir);
            lock (gate)
            {
                if (result.ExitCode == 0 && File.Exists(job.OutputPath))
                {
                    stats.Cooked++;
                    Console.WriteLine($"cook  {job.Kind,-6}  {job.OutputPath}");
                }
                else
                {
                    stats.Failed++;
                    Console.Error.WriteLine($"FAIL  {job.Kind,-6}  {job.InputPath}");
                    Console.Error.WriteLine($"      command: {commandLine}");
                    Console.Error.WriteLine($"      exit: {result.ExitCode}");
                    if (!string.IsNullOrWhiteSpace(result.StdOut))
                        Console.Error.WriteLine(Indent("stdout", result.StdOut));
                    if (!string.IsNullOrWhiteSpace(result.StdErr))
                        Console.Error.WriteLine(Indent("stderr", result.StdErr));
                    if (result.ExitCode == 0 && !File.Exists(job.OutputPath))
                        Console.Error.WriteLine($"      texconv exited 0 but did not write {job.OutputPath}");
                }
            }
        });

        return stats;
    }

    public static List<string> BuildArgs(CookJob job, Options options)
    {
        var args = new List<string>
        {
            "-nologo",
            "-y",
            "-f", job.Kind == MapKind.Colour ? options.ColourFormat : options.NormalFormat,
            "-m", "0",
        };

        if (job.Kind == MapKind.Colour)
        {
            args.Add("-srgb");
            if (options.SepAlpha)
                args.Add("-sepalpha");
        }
        else if (options.InvertY)
        {
            args.Add("-inverty");
        }

        if (options.NoGpu)
            args.Add("-nogpu");
        else if (options.GpuIndex is int gpu)
        {
            args.Add("-gpu");
            args.Add(gpu.ToString());
        }

        if (!string.IsNullOrWhiteSpace(options.BcFlags))
        {
            args.Add("-bc");
            args.Add(options.BcFlags);
        }

        args.Add("-o");
        args.Add(job.OutputDir);
        args.Add(job.InputPath);
        return args;
    }

    static (int ExitCode, string StdOut, string StdErr) Invoke(string texconv, List<string> args, string workingDir)
    {
        var start = new ProcessStartInfo
        {
            FileName = texconv,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true,
            WorkingDirectory = workingDir
        };
        foreach (var arg in args)
            start.ArgumentList.Add(arg);

        using var process = Process.Start(start);
        if (process is null)
            return (1, "", $"Failed to start {texconv}");

        var stdout = process.StandardOutput.ReadToEnd();
        var stderr = process.StandardError.ReadToEnd();
        process.WaitForExit();
        return (process.ExitCode, stdout, stderr);
    }

    static string FormatCommandLine(string texconv, List<string> args)
    {
        static string Q(string s) =>
            s.IndexOfAny([' ', '\t', '"']) >= 0 ? "\"" + s.Replace("\"", "\\\"") + "\"" : s;

        return Q(texconv) + " " + string.Join(' ', args.Select(Q));
    }

    static string Indent(string label, string text)
    {
        var lines = text.Replace("\r\n", "\n").TrimEnd().Split('\n');
        return string.Join('\n', lines.Select((l, i) => i == 0 ? $"      {label}: {l}" : $"      {l}"));
    }
}
