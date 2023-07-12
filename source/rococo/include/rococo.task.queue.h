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