#pragma once

#include <rococo.post.h>

namespace Rococo
{
	namespace Cute
	{
		struct ITreeNode;
	}

	namespace Post
	{
		enum POST_TYPE: int64
		{
			PostType_PopulateTree
		};

		struct PopulateTree
		{
			cstr id;
			Cute::ITreeNode* Root;
		};

		template<> inline POST_TYPE GetPostType<PopulateTree>() { return PostType_PopulateTree; }
	}
}