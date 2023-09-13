// rococo.peroxide.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <rococo.api.h>
#include <rococo.strings.h>
#include <stdio.h>
#include <rococo.libs.inl>
#include <stdlib.h>
#include <vector>

using namespace Rococo;

bool AddBlankFile(const char* deletepath, int fileSize, const char* data)
{
	FILE* fp = nullptr;
	auto err = fopen_s(&fp, deletepath, "wb");
	if (err != 0) return false;

	size_t chunks = fileSize / 32_megabytes;

	for (size_t i = 0; i < chunks; i++)
	{
		size_t written = fwrite(data, 1, 32_megabytes, fp);
		if (written < 32_megabytes)
		{
			fclose(fp);
			return false;
		}
		printf(".");
	}

	fclose(fp);
	return  true;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("Usage: %s <driver-letter>\nApplication will fill drive with blank data.\n", argv[0]);
		return -1;
	}

	char deletepath[256] = { 0 };
	SafeFormat(deletepath, 256, "%s:\\", argv[1]);

	int64 ticks = Rococo::Time::TickCount();
	srand((int32) ticks);

	int len = 3 + rand() % 4;

	for (int i = 0; i < len; ++i)
	{
		deletepath[i + 3] = rand() % 26 + 'a';
	}

	size_t shift = (rand() % 3) + 1;
	int fileSize = (int32) (128_megabytes << shift);

	char wipeValue = rand() % 256;

	std::vector<char> v;
	v.reserve(fileSize);
	v.resize(fileSize);
	memset(v.data(), wipeValue, fileSize);

	int namealgorithm = rand() % 8;
	int extalgorithm = rand() % 8;

	cstr extensions[8] = { "", ".bin", ".dat", ".bak", ".BIN", "", ".BAK", ".txt"};

	cstr underscore[2] = { "_", "" };

	cstr u = underscore[rand() % 2];

	int64 totalSize = 0;

	for (int i = 0; i < 1000000; ++i)
	{
		char fullpath[256];

		switch (namealgorithm)
		{
		case 0:
			SafeFormat(fullpath, 256, "%s%s%d%s", deletepath, u, i, extensions[extalgorithm]);
			break;
		case 1:
			SafeFormat(fullpath, 256, "%s%s%04.4d%s", deletepath, u, i, extensions[extalgorithm]);
			break;
		case 2:
			SafeFormat(fullpath, 256, "%s%s%04.4x%s", deletepath, u, i, extensions[extalgorithm]);
			break;
		case 3:
			SafeFormat(fullpath, 256, "%s%s%X%s", deletepath, u, i, extensions[extalgorithm]);
			break;
		case 4:
			SafeFormat(fullpath, 256, "%s%s%03.3X%s", deletepath, u, i, extensions[extalgorithm]);
			break;
		case 5:
			SafeFormat(fullpath, 256, "%s%s%05.5X%s", deletepath, u, i, extensions[extalgorithm]);
			break;
		case 6:
			SafeFormat(fullpath, 256, "%s%s%05.5d%s", deletepath, u, i, extensions[extalgorithm]);
			break;
		case 7:
			SafeFormat(fullpath, 256, "%s%s%06.6X%s", deletepath, u, i, extensions[extalgorithm]);
			break;
		}

		printf("\nWriting %s - total size %lld MB", fullpath, (totalSize / 1_megabytes));
		if (!AddBlankFile(fullpath, fileSize, v.data())) return 0;
		totalSize += (int64) fileSize;
	}

	return 0;
}

