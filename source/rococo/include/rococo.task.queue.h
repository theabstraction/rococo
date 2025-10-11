// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#include <rococo.types.h>
#include <rococo.functional.h>

namespace Rococo::Tasks
{
	ROCOCO_INTERFACE ITaskQueue
	{
		virtual void AddTask(Rococo::Function<void()> lambda) = 0;
		virtual bool ExecuteNext() = 0;
	};
};