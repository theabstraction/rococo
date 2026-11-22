# AssetCooker

Cooks albedo and normal TIFFs under `D:\work\assets` into compressed DDS files under `D:\work\assets-cooked`, preserving the source relative directories. Source TIFFs are never moved or deleted.

It shells out to DirectXTex's `texconv.exe`. DirectXTK is not used.

## Naming convention

A colour (albedo) file matches when its stem equals its immediate parent directory name (case-insensitive) followed by an integer index:

    assets\brick\brick01.tif          ->  assets-cooked\brick\brick01.dds
    assets\brick\brick01_n.tif        ->  assets-cooked\brick\brick01_n.dds
    assets\materials\wood\wood2.tiff  ->  assets-cooked\materials\wood\wood2.dds

Both `.tif` and `.tiff` are accepted. The optional normal map must sit in the same folder and be named `<dir><index>_n.tif(f)`.

TIFFs that do not match this pattern are left alone. They are listed when you pass `--verbose`. Colour maps with no normal, and `_n` files with no matching colour map, are always reported.

## Build and run

Requires the .NET 8 SDK.

    cd asset-cooker
    dotnet run --project AssetCooker.csproj -- [options]

On Windows you can also publish a single exe:

    dotnet publish -c Release -r win-x64 --self-contained false -p:PublishSingleFile=true -o .\publish
    .\publish\AssetCooker.exe [options]

## Usage

    AssetCooker [options]

    --assets <dir>            Source asset root (default: D:\work\assets)
    --out <dir>               Cooked output root (default: D:\work\assets-cooked)
    --texconv <path>          Path to texconv.exe
    --force                   Recook even if the .dds is newer than the source TIFF
    --dry-run                 Print planned work; write nothing
    --verbose                 List TIFFs that do not match the naming convention
    --colour-format <fmt>     DXGI format for colour maps (default: BC7_UNORM_SRGB)
    --normal-format <fmt>     DXGI format for normal maps (default: BC5_UNORM)
    --parallel <n>            Max concurrent texconv processes (default: processor count)
    --sep-alpha               Pass -sepalpha on colour maps (alpha is not transparency)
    --invert-y                Pass -inverty on normal maps (OpenGL-style Y)
    --nogpu                   Force CPU BC6H/BC7 compression
    --gpu <index>             DirectCompute adapter index
    --bc <flags>              texconv -bc flags (u, d, q, x)
    --help                    Show help

An existing `.dds` is skipped when it is newer than or equal to its source TIFF, unless `--force` is set. The process exits with code 1 if any conversion fails, or if `texconv` cannot be found.

## texconv command lines

Colour maps, defaults (sRGB in and out, all mip levels, overwrite):

    texconv -nologo -y -f BC7_UNORM_SRGB -m 0 -srgb -o <outDir> <file>.tif

Normal maps, defaults (linear, BC5 XY, all mip levels, overwrite):

    texconv -nologo -y -f BC5_UNORM -m 0 -o <outDir> <file>_n.tif

`-m 0` is texconv's "generate a full mip chain" setting. texconv names the output after the input stem, so `-o <outDir>` already produces `<dir><index>.dds` / `<dir><index>_n.dds`.

`--sep-alpha` appends `-sepalpha` to the colour line when the alpha channel is a material mask rather than transparency. `--invert-y` appends `-inverty` to the normal line. `--nogpu` / `--gpu <n>` and `--bc <flags>` are forwarded as `-nogpu`, `-gpu <n>`, and `-bc <flags>`.

## Finding texconv

If `--texconv` is not given, the tool looks under `D:\work\DirectXTex` in this order (Release before Debug, x64 first):

    DirectXTex\Texconv\Bin\Desktop_2022\x64\Release\texconv.exe
    DirectXTex\Texconv\Bin\Desktop_2022_Win10\x64\Release\texconv.exe
    DirectXTex\Texconv\Bin\Desktop_2022_Win11\x64\Release\texconv.exe
    DirectXTex\Texconv\Bin\Desktop_2019\x64\Release\texconv.exe
    DirectXTex\Texconv\x64\Release\texconv.exe
    DirectXTex\Texconv\Release\texconv.exe

and the Debug / Win32 / ARM64 equivalents, then `PATH`. It fails with the candidate list if nothing is found.

Build texconv from `D:\work\DirectXTex` if you have not already (the Texconv sample in that repo).

## Assumptions

- DirectXTK is not required. Only DirectXTex's `texconv` is invoked.
- Source TIFFs stay put. "Move" in the original request is treated as "write cooked output into the mirrored tree."
- Colour TIFFs are authored in sRGB and are compressed as `BC7_UNORM_SRGB` with `-srgb`, so mip filtering is done in linear space.
- Normal TIFFs are linear tangent-space XY maps and are compressed as `BC5_UNORM` with no sRGB flags. Y is not inverted unless you pass `--invert-y`.
- The integer index is taken as written (`brick01` and `brick1` are different maps).
