#ifndef DYSTOPIA_POST_H
#define  DYSTOPIA_POST_H

#include <rococo.post.h>

namespace Dystopia
{
	struct VerbExamine
	{
		ID_ENTITY entityId;
		int inventoryIndex;
	};

	struct SelectItemOnGround
	{
		ID_ENTITY id;
	};
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
			POST_TYPE_SELECT_ITEM_ON_GROUND
		};

		template<> inline POST_TYPE GetPostType<MouseEvent>() { return POST_TYPE_MOUSE_EVENT; }
		template<> inline POST_TYPE GetPostType<KeyboardEvent>() { return POST_TYPE_KEYBOARD_EVENT; }
		template<> inline POST_TYPE GetPostType<TimestepEvent>() { return POST_TYPE_TIMESTEP; }
		template<> inline POST_TYPE GetPostType<AdvanceTimestepEvent>() { return POST_TYPE_ADVANCE_TIMESTEP; }
		template<> inline POST_TYPE GetPostType<VerbExamine>() { return POST_TYPE_EXAMINE; }
		template<> inline POST_TYPE GetPostType<SelectItemOnGround>() { return POST_TYPE_SELECT_ITEM_ON_GROUND; }
	}
}

#endif
