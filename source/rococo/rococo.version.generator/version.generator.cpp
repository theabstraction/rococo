#include <stdio.h>
#include <string.h>

struct Version
{
    int A;
    int B;
    int C;
    int D;
};

enum { MAX_VERSION_COUNT = 1000 };

bool TryReadBuildNumber(Version& version, const char* exeName, const char* build_file)
{
    FILE* fp = fopen(build_file, "rt");
    if (fp == NULL)
    {
        fprintf(stderr, "Could not open %s for reading.\n", build_file);
        return false;
    }

    char versionString[256];
    size_t nBytesRead = fread(versionString, 1, sizeof versionString, fp);
    fclose(fp);

    if (nBytesRead >= sizeof versionString)
    {
        fprintf(stderr, "Expecting fewer than 256 characters in %s\n", build_file);
        return false;
    }

    versionString[nBytesRead] = 0;

    Version v;
    int count = sscanf(versionString, "%u %u %u %u", &v.A, &v.B, &v.C, &v.D);
    if (count != 4)
    {
        fprintf(stderr, "%s -f:%s, Expecting 4 unsigned integers separated by spaces in file\n", exeName, build_file);
        return false;
    }

    if (v.B >= MAX_VERSION_COUNT || v.C >= MAX_VERSION_COUNT || v.D >= MAX_VERSION_COUNT)
    {
        fprintf(stderr, "%s -f:%s, Expecting 4 unsigned integers < %u separated by spaces in file\n", exeName, build_file, MAX_VERSION_COUNT);
        return false;
    }

    version = v;
    return true;
}

void IncrementVersion(Version& v)
{
    v.D += 1;
    if (v.D == MAX_VERSION_COUNT)
    {
        v.D = 0;
        v.C++;

        if (v.C == MAX_VERSION_COUNT)
        {
            v.C = 0;
            v.B++;
            if (v.B == MAX_VERSION_COUNT)
            {
                v.A++;
            }
        }
    }
}

bool TrySaveBuildFile(const Version& v, const char* build_file)
{
    FILE* fp = fopen(build_file, "wt");
    if (fp == NULL)
    {
        fprintf(stderr, "Could not open %s for writing.\n", build_file);
        return false;
    }

    fprintf(fp, "%u %u %u %u", v.A, v.B, v.C, v.D);
    fclose(fp);

    return true;
}

bool HasSwitch(const char* switchStr, int argc, char* argv[])
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], switchStr) == 0)
        {
            return true;
        }
    }

    return false;
}

#include <vector>

void PushInteger(int x, std::vector<char>& output)
{
    char buf[16];
    int len = sprintf(buf, "%u", x);
    for (int i = 0; i < len; i++)
    {
        output.push_back(buf[i]);
    }
}

void Transform(const char* template_file, const char* template_target, const Version& v)
{
    FILE* input = fopen(template_file, "rb");
    if (input == nullptr)
    {
        fprintf(stderr, "Could not open template file %s for reading", template_file);
        return;
    }

    std::vector<char> inputBuffer;
    inputBuffer.resize(1024 * 1024);

    size_t nChars = fread(inputBuffer.data(), 1, inputBuffer.size(), input);
    fclose(input);

    if (nChars <= 12)
    {
        fprintf(stderr, "Insufficient characters for template file %s. Need at least 12 characters", template_file);
        return;
    }

    if (nChars >= inputBuffer.size())
    {
        fprintf(stderr, "Could not parse template file %s. It was too long. Max 1MB", template_file);
        return;
    }

    inputBuffer[nChars] = 0;

    const char* inputTemplate = inputBuffer.data();

    const char* readPoint = inputTemplate;
    const char* endPoint = inputTemplate + nChars;

    std::vector<char> outputData;
    outputData.reserve(nChars);

    while (readPoint < endPoint)
    {
        char c = *readPoint;
        if (c == '$')
        {
            // Maybe our macro
            if (strncmp(readPoint, "$<A>$", 5) == 0)
            {
                PushInteger(v.A, outputData);
                readPoint += 5;
            }
            else if (strncmp(readPoint, "$<B>$", 5) == 0)
            {
                PushInteger(v.B, outputData);
                readPoint += 5;
            }
            else if (strncmp(readPoint, "$<C>$", 5) == 0)
            {
                PushInteger(v.C, outputData);
                readPoint += 5;
            }
            else if (strncmp(readPoint, "$<D>$", 5) == 0)
            {
                PushInteger(v.D, outputData);
                readPoint += 5;
            }
            else
            {
                outputData.push_back('$');
                readPoint++;
            }
        }
        else
        {
            readPoint++;
            outputData.push_back(c);
        }
    }

    outputData.push_back(0);

    FILE* output = fopen(template_target, "wb");
    if (input == nullptr)
    {
        fprintf(stderr, "Could not open template target file %s for writing", template_target);
        fclose(input);
        return;
    }

    fprintf(output, "%s", outputData.data());
    fclose(output);
}

void print_usage(int, char* argv[])
{
    printf("Usage: %s -f:<build_file>.\nWhere <build-file> specifies your text file with versioning info.\nVersioning has the format A B C D. Use spaces to separate 4 unsigned integers\n", argv[0]);
    printf("Switches\n\t-u: updates the build file by incrementing the version number and overwriting the <build-file>\n");
    printf("\n\t-ti:<template-file> -to:<target-file> used together these load a <template-file> and saves the transform to the <target-file>. $<A>$, $<B>$, $<C>$ ,$<D>$ in the template transform to the appropriate version index\n");
}

const char* GetArg(int argc, char* argv[], const char* prefix)
{
    auto len = strlen(prefix);

    for (int i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], prefix, len) == 0)
        {
            return argv[i] + len;
        }
    }

    return nullptr;
}

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        print_usage(argc, argv);
        return 0;
    }

    const char* build_file = GetArg(argc, argv, "-f:");
    const char* template_file = GetArg(argc, argv, "-ti:");
    const char* template_target = GetArg(argc, argv, "-to:");

    if (build_file == nullptr || strlen(build_file) < 2)
    {
        fprintf(stderr, "Expecting -f:<build_file> on the command line\n");
        return -1;
    }

    if (template_file && !template_target)
    {
        fprintf(stderr, "Missing -to:<template target file>\n");
        return -1;
    }

    if (!template_file && template_target)
    {
        fprintf(stderr, "Missing -ti:<template input file>\n");
        return -1;
    }

    Version v;
    if (!TryReadBuildNumber(v, argv[0], build_file))
    {
        return -1;
    }

    bool update = HasSwitch("-u", argc, argv);

    if (update)
    {
        IncrementVersion(v);
        TrySaveBuildFile(v, build_file);
    }

    if (template_file != nullptr && template_target != nullptr)
    {
        Transform(template_file, template_target, v);
    }

    return 0;
}

