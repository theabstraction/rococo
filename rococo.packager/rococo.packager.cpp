#include <rococo.api.h>
#include <rococo.io.h>
#include <stdio.h>
#include <wchar.h>
#include <rococo.strings.h>

void PrintUsage()
{
    auto usage = 
R"(
"Usage:
    rococo.packager.exe <source_directory> <target_filename>
")";

    puts(usage);
}

using namespace Rococo;
using namespace Rococo::IO;

struct PackageArgs
{
    const char* source_directory;
    const char* target_filename;
};

struct Pack
{
    AutoFree<IBinaryArchive> f;

    Pack(const wchar_t* archiveFilename, uint64 totalFileLength):
        f(IO::CreateNewBinaryFile(archiveFilename))
    {
        f->Reserve(totalFileLength);
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
    }

    void Write(const fstring& s)
    {
        f->Write(1, s.length, s.buffer);
    }

    void WriteHeader(const wchar_t* directory)
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

        Write("\n\n"_fstring);

        struct : IEventCallback<IO::FileItemData>
        {
            IBinaryArchive* f;
            uint64 len = 0;
            void OnEvent(IO::FileItemData& file)
            {
                auto* srcFile = file.fullPath;

                if (!file.isDirectory)
                {
                    IO::FileAttributes a;
                    if (IO::TryGetFileAttributes(srcFile, a))
                    {
                        char buf[1024];
                        int nBytes = SecureFormat(buf, "%ls/%ls\n%llu\n", file.containerRelRoot, file.itemRelContainer, a.fileLength);
                        OS::ToUnixPath(buf);
                        f->Write(1, nBytes, buf);
                    }
                }
            }
        } writeEntry;
        writeEntry.f = f;
        IO::ForEachFileInDirectory(directory, writeEntry, true);

        auto headerLength = f->Position();

        f->SeekAbsolute(headerLengthWritePos);

        char buf[9];
        SecureFormat(buf, "%8.8lu", headerLength);
        f->Write(1, 8, buf);

        f->SeekAbsolute(headerLength);
    }
};

uint64 EvaluateLengthOfFilesWithin(const wchar_t* directory)
{
    struct : IEventCallback<IO::FileItemData>
    {
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
                }
            }

            len += wcslen(file.containerRelRoot);
            len += wcslen(file.itemRelContainer);
            len += 4; // some extra padding
        }
    } sumLen;

    IO::ForEachFileInDirectory(directory, sumLen, true);

    return sumLen.len;
}

void Package(const PackageArgs& args)
{
    WideFilePath wSrc;
    Assign(wSrc, args.source_directory);

    WideFilePath wTrg;
    Assign(wTrg, args.target_filename);

    uint64 totalLen = EvaluateLengthOfFilesWithin(wSrc);
    if (totalLen > 1024_megabytes)
    {
        Throw(0, "Sanity check failed: the archive would expand beyond the maximum size of 1GB");
    }

    Pack pack(wTrg, totalLen);

    pack.WriteHeader(wSrc);

    struct : IEventCallback<IO::FileItemData>
    {
        Pack* pack;
        void OnEvent(IO::FileItemData& file)
        {
            auto* srcFile = file.fullPath;

            try
            {
                pack->AppendFile(srcFile);
            }
            catch (IException& ex)
            {
                Throw(ex.ErrorCode(), "Error appending %ls: %s", srcFile, ex.Message());
            }
        }
    } addFileToPack;

    addFileToPack.pack = &pack;

    IO::ForEachFileInDirectory(wSrc, addFileToPack, true);
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        PrintUsage();
        return 0;
    }

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
            fprintf(stderr, "Exception:\n%s\%d: %s\n", ex.Message(), ex.ErrorCode(), msg);
        }
        else
        {
            fprintf(stderr, "Exception:\n%s\n", ex.Message());
        }
        return ex.ErrorCode();
    }
    catch (...)
    {
        fprintf(stderr, "Unhandled exception packaging \n%s\n", args.source_directory);
        return 0;
    }
}
