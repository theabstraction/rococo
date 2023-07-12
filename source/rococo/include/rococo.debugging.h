#pragma once

#include <rococo.types.h>

namespace Rococo
{
	namespace Debugging
	{
		struct StackFrame
		{
			int depth;
			int lineNumber;
			char moduleName[256];
			char functionName[256];
			char sourceFile[256];

			struct Address
			{
				uint16 segment;
				uint64 offset;
			} address;
		};

		ROCOCO_INTERFACE IStackFrameFormatter
		{
			virtual void Format(const StackFrame& sf) = 0;
		};

		ROCOCO_API void FormatStackFrames(IStackFrameFormatter& formatter);

		ROCOCO_INTERFACE IStackFrameEnumerator
		{
			virtual void FormatEachStackFrame(IStackFrameFormatter& formatter) = 0;
		};
	}
}