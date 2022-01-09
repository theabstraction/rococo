#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

typedef unsigned __int32 uint32;

int main(int argc, char* argv[])
{
	printf("Build Maker\n");
	if (argc != 2)
	{
		printf("Expecting two arguments: build.maker.exe <build-file>\nOnly %d argument(s) supplied.\n", argc-1);
		return EINVAL;
	}

	FILE* fp;
	errno_t errCode = fopen_s(&fp, argv[1], "rt");
	if (errCode != 0)
	{
		int err = errno;
		perror("Could not open file for reading.\n");
		printf("Could not open %s for reading\n", argv[1]);
		return errCode;
	}

	uint32 major;
	uint32 minor;

	char versionLine[256];
	if (!fgets(versionLine, 256, fp))
	{
		major = 0;
		minor = 0;

		printf("Expecting first line version file: <major_version> <minor_version>, where numbers are uint32\nVersion will be set to 0 0\n");
	}

	int nArgs = sscanf_s(versionLine, "%u%u", &major, &minor);

	if (nArgs != 2)
	{
		printf("Expecting first line version file: <major_version> <minor_version>, where numbers are uint32\nVersion will be set to 0 0\n");
	}

	fclose(fp);

	errCode = fopen_s(&fp, argv[1], "wt");
	if (errCode != 0)
	{
		int err = errno;
		perror("Could not open file for writing.\n");
		printf("Could not open %s for writing\n", argv[1]);
		return errCode;
	}

	time_t now = time(nullptr);

	tm t;
	gmtime_s(&t, &now);

	minor++;

	fprintf(fp, "%u %u\n", major, minor);
	fprintf(fp, "Built at %.2d:%.2d:%.2d on %d/%d/%d\n", t.tm_hour, t.tm_min, t.tm_sec, t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
	fclose(fp);

	char cppFile[_MAX_PATH];
	sprintf_s(cppFile, "%s.cpp", argv[1]);
	errCode = fopen_s(&fp, cppFile, "wt");
	if (errCode != 0)
	{
		int err = errno;
		perror("Could not open file for writing.\n");
		printf("Could not open %s for writing\n", cppFile);
		return errCode;
	}

	fprintf_s(fp, "// build.maker generated this file at %.2d:%.2d:%.2d on %d/%d/%d\n", t.tm_hour, t.tm_min, t.tm_sec, t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
	fprintf_s(fp, "void GetBuildVersion(unsigned& major, unsigned& minor) { major = %u; minor = %u; }\n", major, minor);

	fclose(fp);

	return 0;
}