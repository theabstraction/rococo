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
			POST_TYPE_EXAMINE
		};

		inline POST_TYPE GetPostType(const MouseEvent& t) { return POST_TYPE_MOUSE_EVENT; }
		inline POST_TYPE GetPostType(const KeyboardEvent& t) { return POST_TYPE_KEYBOARD_EVENT; }
		inline POST_TYPE GetPostType(const TimestepEvent& t) { return POST_TYPE_TIMESTEP; }
		inline POST_TYPE GetPostType(const AdvanceTimestepEvent& t) { return POST_TYPE_ADVANCE_TIMESTEP; }
		inline POST_TYPE GetPostType(const VerbExamine& e) { return POST_TYPE_EXAMINE; }
	}
}

#endif
