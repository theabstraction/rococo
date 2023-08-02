#pragma once
#include <rococo.types.h>
#include <rococo.maths.h>

namespace Rococo
{
	ROCOCO_ID(ID_SPRITE, uint64, 0);

	struct BloodyNotifyArgs;
	struct IBloodyPropertySetEditor;
	struct IGUIStack;
	struct IKeyboardSupervisor;
	struct IStringVector;
	struct IUltraClock;
	struct Platform;

	struct QuadColours
	{
		RGBAb a;
		RGBAb b;
		RGBAb c;
		RGBAb d;
	};

	struct QuadVertices
	{
		Quad positions;
		GuiRectf uv;
		Quad normals;
		QuadColours colours;
	};

	ROCOCO_INTERFACE IStringVector
	{
		virtual int32 Count() const = 0;
		virtual void GetItem(int32 item, char* text, size_t capacity) const = 0;
	};
}