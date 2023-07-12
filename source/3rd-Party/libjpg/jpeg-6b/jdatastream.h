#pragma once

typedef struct DataStream
{
	const unsigned char* sourceData;
	size_t dataLengthBytes;
	const unsigned char* end;
	const unsigned char* readPosition;
} DataStream;