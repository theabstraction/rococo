#include <rococo.api.h>
#include <rococo.io.h>
#include <stdio.h>
#include <wchar.h>
#include <rococo.strings.h>
#include <rococo.os.h>

void PrintUsage()
{
    auto usage = 
R"(Usage:
    rococo.packager.exe <source_directory> <target_filename>
)";

    puts(usage);
}

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Strings;

struct PackageArgs
{
    const char* source_directory;
    const char* target_filename;
};

struct Pack
{
    AutoFree<IBinaryArchive> f;

    Pack(const wchar_t* archiveFilename, uint64 totalFileLength)
    {
        try
        {
            f = IO::CreateNewBinaryFile(archiveFilename);
        }
        catch (IException& ex)
        {
            Throw(ex.ErrorCode(), "Error creating new binary file: %ls\n", archiveFilename);
        }

        f->Reserve(totalFileLength);
    }

    ~Pack()
    {
        f->Truncate();
    }

    void AppendFile(const wchar_t* fullname)
    {
        AutoFree<IBinarySource> srcData(ReadBinarySource(fullname));
        char buf[8_kilobytes];
        uint32 readLength = 0;
        do
        {
            readLength = srcData->Read(sizeof(buf), buf);
            if (readLength > 0)
            {
                f->Write(1, readLength, buf);
            }
        } while (readLength > 0);

        char nullchar = 0; // Allow text files to be null-terminated
        f->Write(nullchar);
        char newLine = '\n';
        f->Write(newLine);
    }

    void Write(const fstring& s)
    {
        f->Write(1, s.length, s.buffer);
    }

    void WriteHeader(const wchar_t* directory, uint32 fileCount)
    {
        Write("Rococo.Package:v1.0.0.0\n"_fstring);
        Write("HeaderLength:"_fstring);
        auto headerLengthWritePos = f->Position();
        Write("00000000\n"_fstring); // 8 chars
        Write("StringFormat:UTF8\n"_fstring);
        Write("ByteOrder:"_fstring);

        union byteOrderBuffer
        {
            char mega[4];
            uint32 value;
        } u;

        u.value = 0x4D454741; // Encodes MEGA from high end to low

        f->Write(u);

        char dirInfo[128];
        int diCount = SecureFormat(dirInfo, "\nFiles:%u\n\n", fileCount);

        UNUSED(diCount);

        Write(to_fstring(dirInfo));

        struct : IEventCallback<IO::FileItemData>
        {
            IBinaryArchive* f = nullptr;
            uint64 len = 0;
            size_t prefixLen = 0;
            void OnEvent(IO::FileItemData& file)
            {
                auto* srcFile = file.fullPath;

                if (!file.isDirectory)
                {
                    IO::FileAttributes a;
                    if (IO::TryGetFileAttributes(srcFile, a))
                    {
                        char buf[1024];
                        int nBytes = SecureFormat(buf, "%ls\t%llu\n", file.fullPath + prefixLen + 1, a.fileLength);
                        IO::ToUnixPath(buf);
                        f->Write(1, nBytes, buf);
                    }
                }
                else
                {
                    if (file.itemRelContainer[0] > 255 || !IsCapital((char)file.itemRelContainer[0]) && file.itemRelContainer[0] != L'_')
                    {
                        Throw(0, "Sub-directory [%ws] of [%ws] did not begin with a capital letter.", file.itemRelContainer, file.fullPath);
                    }
                }
            }
        } writeDirectory;
        writeDirectory.f = f;
        writeDirectory.prefixLen = StringLength(directory);
        IO::ForEachFileInDirectory(directory, writeDirectory, true);

        Write("\n"_fstring);

        auto headerLength = f->Position();

        f->SeekAbsolute(headerLengthWritePos);

        char buf[9];
        SecureFormat(buf, "%8lu", headerLength);
        f->Write(1, 8, buf);

        f->SeekAbsolute(headerLength);
    }
};

uint64 EvaluateLengthOfFilesWithin(const wchar_t* directory, uint32& fileCount)
{
    struct : IEventCallback<IO::FileItemData>
    {
        uint32 fileCount = 0;
        uint64 len = 0;
        void OnEvent(IO::FileItemData& file)
        {
            auto* srcFile = file.fullPath;

            if (!file.isDirectory)
            {
                IO::FileAttributes a;
                if (IO::TryGetFileAttributes(srcFile, a))
                {
                    len += a.fileLength;
                    fileCount++;
                }
            }

            len += wcslen(file.containerRelRoot);
            len += wcslen(file.itemRelContainer);
            len += 4; // some extra padding
        }
    } sumLen;

    IO::ForEachFileInDirectory(directory, sumLen, true);

    fileCount = sumLen.fileCount;
    return sumLen.len;
}

void Package(const PackageArgs& args)
{
    WideFilePath wSrc;
    Assign(wSrc, args.source_directory);

    WideFilePath wTrg;
    Assign(wTrg, args.target_filename);

    uint32 fileCount;
    uint64 totalLen = EvaluateLengthOfFilesWithin(wSrc, OUT fileCount);
    if (totalLen > 1024_megabytes)
    {
        Throw(0, "Sanity check failed: the archive would expand beyond the maximum size of 1GB");
    }

    auto reserveBytes = totalLen + 128; // Some space for the header

    Pack pack(wTrg, reserveBytes);

    pack.WriteHeader(wSrc, fileCount);

    struct : IEventCallback<IO::FileItemData>
    {
        Pack* pack;
        void OnEvent(IO::FileItemData& file)
        {
            auto* srcFile = file.fullPath;

            try
            {
                if (!file.isDirectory)
                {
                    pack->AppendFile(srcFile);
                }
            }
            catch (IException& ex)
            {
                Throw(ex.ErrorCode(), "Error appending %ls: %s", srcFile, ex.Message());
            }
        }
    } addFileToPack;

    addFileToPack.pack = &pack;

    IO::ForEachFileInDirectory(wSrc, addFileToPack, true);

    uint32 checkFileCount;
    uint64 checkLen = EvaluateLengthOfFilesWithin(wSrc, checkFileCount);

    if (checkLen != totalLen || checkFileCount != fileCount)
    {
        Throw(0, "The source directory appears to have been modified during the generation of the archive");
    }
}

#include <rococo.allocators.h>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        PrintUsage();
        return 0;
    }

    using namespace Rococo::Memory;

    AllocatorLogFlags flags;
    flags.LogDetailedMetrics = false;
    flags.LogLeaks = false;
    flags.LogOnModuleExit = false;
    Rococo::Memory::SetAllocatorLogFlags(flags);

    PackageArgs args;
    args.source_directory = argv[1];
    args.target_filename = argv[2];

    try
    {
        Package(args);
        return 0;
    }
    catch (IException& ex)
    {
        if (ex.ErrorCode() != 0)
        {
            char msg[128];
            Rococo::OS::FormatErrorMessage(msg, sizeof(msg), ex.ErrorCode());
            fprintf(stderr, "Exception:\n%sCode %d: %s\n", ex.Message(), ex.ErrorCode(), msg);
        }
        else
        {
            fprintf(stderr, "Exception:\n%s\n", ex.Message());
        }
        return ex.ErrorCode() ? ex.ErrorCode(): -1;
    }
    catch (...)
    {
        fprintf(stderr, "Unhandled exception packaging \n%s\n", args.source_directory);
        return -1;
    }
}
