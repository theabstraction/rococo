#ifndef DYSTOPIA_POST_H
#define  DYSTOPIA_POST_H

#include <rococo.post.h>

namespace Dystopia
{
#pragma pack(push,1)
	struct VerbExamine
	{
		ID_ENTITY entityId;
		uint32 inventoryIndex;
	};

	struct SelectItemOnGround
	{
		ID_ENTITY id;
	};

	struct HintMessage3D
	{
		Vec3 position;
		float duration;
		wchar_t message[15];
	};

	struct VerbDropAtCursor
	{
		ID_ENTITY entityId;
		uint32 inventoryIndex;
		Vec2i cursorPosition;
	};
#pragma pack(pop)
}

namespace Rococo
{
	namespace Post
	{
		using namespace Dystopia;

		enum POST_TYPE : int64
		{
			POST_TYPE_INVALID = 0,
			POST_TYPE_MOUSE_EVENT,
			POST_TYPE_KEYBOARD_EVENT,
			POST_TYPE_TIMESTEP,
			POST_TYPE_ADVANCE_TIMESTEP, // sent when the game/simulation advances by dt
			POST_TYPE_EXAMINE,
			POST_TYPE_SELECT_ITEM_ON_GROUND,
			POST_TYPE_HINT_3D,
			POST_TYPE_DROP_AT_CURSOR
		};

		template<> inline POST_TYPE GetPostType<MouseEvent>() { return POST_TYPE_MOUSE_EVENT; }
		template<> inline POST_TYPE GetPostType<KeyboardEvent>() { return POST_TYPE_KEYBOARD_EVENT; }
		template<> inline POST_TYPE GetPostType<TimestepEvent>() { return POST_TYPE_TIMESTEP; }
		template<> inline POST_TYPE GetPostType<AdvanceTimestepEvent>() { return POST_TYPE_ADVANCE_TIMESTEP; }
		template<> inline POST_TYPE GetPostType<VerbExamine>() { return POST_TYPE_EXAMINE; }
		template<> inline POST_TYPE GetPostType<SelectItemOnGround>() { return POST_TYPE_SELECT_ITEM_ON_GROUND; }
		template<> inline POST_TYPE GetPostType<HintMessage3D>() { return POST_TYPE_HINT_3D; }
		template<> inline POST_TYPE GetPostType<VerbDropAtCursor>() { return POST_TYPE_DROP_AT_CURSOR; }
	}
}

#endif
