#include "ogg/os_types.h"
#include <stdlib.h>

FN_OGG_MALLOC g_oggMalloc = malloc;
FN_OGG_CALLOC g_oggCalloc = calloc;
FN_OGG_REALLOC g_oggRealloc = realloc;
FN_OGG_FREE g_oggFreeMemory = free;

__declspec(dllexport) void OggSetAllocators(FN_OGG_MALLOC oggMalloc, FN_OGG_CALLOC oggCalloc, FN_OGG_REALLOC oggRealloc, FN_OGG_FREE oggFreeMemory)
{
	g_oggMalloc = oggMalloc;
	g_oggCalloc = oggCalloc;
	g_oggRealloc = oggRealloc;
	g_oggFreeMemory = oggFreeMemory;
}

void* OggMalloc(size_t nBytes)
{
	return g_oggMalloc(nBytes);
}

void* OggCalloc(size_t nItems, size_t size)
{
	return g_oggCalloc(nItems, size);
}

void* OggRealloc(void* existingBuffer, size_t nBytes)
{
	return g_oggRealloc(existingBuffer, nBytes);
}

void OggFree(void* buffer)
{
	g_oggFreeMemory(buffer);
}

