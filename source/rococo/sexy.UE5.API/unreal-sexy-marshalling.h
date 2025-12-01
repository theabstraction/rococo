#pragma once
#include <rococo.types.h>
#include <sexy.types.h>
#include <sexy.script.h>

class UClass;
class UObject;
class UMethod;
typedef int UnknownType;

namespace Rococo::UE5::Marshal
{
	int64 ConstructUObject(Rococo::Script::NativeCallEnvironment& e);
	UMethod* GetNCEUMethod(Rococo::Script::NativeCallEnvironment& e);
	UMethod& GetMethod(UClass& classRef, crwstr methodName);
	UObject* GetNCEUObject(Rococo::Script::NativeCallEnvironment& e, int64 objectHandle);
	void ValidateArgs(UMethod* methodRef, void* args, size_t argSize);
	void ProcessEvent(UObject* object, UMethod* methodRef, void* args);

#pragma pack(push, 4)
	struct FStringImage
	{
		crwstr buffer;
		int32 length;
		int32 alignmentPadding;
	};

	struct FNameImage
	{
		uint32 comparisonIndex;
		uint32 number;
		uint32 displayIndex;
	};

	struct DVector3
	{
		double x;
		double y;
		double z;
	};

	struct DQuat
	{
		double x;
		double y;
		double z;
		double w;
	};

	struct DTransform
	{
		DQuat rotation;
		DVector3 translation;
		DVector3 scale;
	};

	struct DRotator
	{
		double pitch;
		double yaw;
		double roll;
	};
#pragma pack(pop)
}
