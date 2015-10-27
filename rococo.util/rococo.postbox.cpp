#include <rococo.types.h>
#include <deque>
#include <unordered_map>
#include <algorithm>
namespace
{
	using namespace Rococo;
	using namespace Rococo::Post;

	struct PostItem
	{
		POST_TYPE id;
		uint64 nBytes;
		bool isLossy;
		char opaquedata[LARGEST_MESSAGE_SIZE];
	};

	class Postbox : public IPostboxSupervisor
	{
	private:
		std::deque<PostItem> items;
		typedef std::unordered_map<POST_TYPE, std::vector<IPostRecipient*>> submap;
		submap subscribers;
		enum { CAPACITY = 64 };
	public:
		virtual void Deliver()
		{
			enum { MAX_ITEMS_PER_DELIVERY = CAPACITY * 2};

			int i = 0;
			while (!items.empty())
			{
				auto item = items.front();
				SendDirectUnchecked(item.id, item.opaquedata, item.nBytes);
				items.pop_front();

				if (i++ == MAX_ITEMS_PER_DELIVERY) Throw(0, L"Postbox delivery failed. MAX_ITEMS_PER_DELIVERY exceeded");
			}
		}

		virtual void PostForLater(POST_TYPE id, const void* buffer, uint64 nBytes, bool isLossy)
		{
			if (nBytes > LARGEST_MESSAGE_SIZE) Throw(0, L"Postbox message too long");
			if (items.size() > CAPACITY)
			{
				auto i = items.begin();
				while (i != items.end())
				{
					if (i->isLossy)
					{
						i = items.erase(i);
					}
					else
					{
						++i;
					}
				}

				if (items.size() > CAPACITY)
				{
					Throw(0, L"Postbox buffer exhausted. Failed to deliver #%d", id);
				}
			}

			PostItem item;
			item.id = id;
			item.nBytes = nBytes;
			item.isLossy = isLossy;
			memcpy_s(item.opaquedata, LARGEST_MESSAGE_SIZE, buffer, nBytes);
			items.push_back(item);
		}

		void SendDirectUnchecked(POST_TYPE id, const void* buffer, uint64 nBytes)
		{
			auto i = subscribers.find(id);
			if (i != subscribers.end())
			{
				auto recipients = i->second;
				for (auto r : recipients)
				{
					r->OnPost(id, buffer, nBytes);
				}
			}
		}

		virtual void SendDirect(POST_TYPE id, const void* buffer, uint64 nBytes)
		{
			if (nBytes > LARGEST_MESSAGE_SIZE) Throw(0, L"Postbox message too long");
			SendDirectUnchecked(id, buffer, nBytes);
		}

		virtual void Subscribe(POST_TYPE id, IPostRecipient* recipient)
		{
			auto s = subscribers.find(id);
			if (s == subscribers.end())
			{
				std::vector<IPostRecipient*> x;
				s = subscribers.insert(std::make_pair(id, x)).first;
			}

			s->second.push_back(recipient);
		}

		virtual void Unsubscribe(POST_TYPE id, IPostRecipient* recipient)
		{
			auto s = subscribers.find(id);
			if (s != subscribers.end())
			{
				auto& recipients = s->second;
				auto erasepoint = std::remove(recipients.begin(), recipients.end(), recipient);
				recipients.erase(erasepoint, recipients.end());
			}
		}

		virtual void Free()
		{
			delete this;
		}
	};
}

namespace Rococo 
{
	namespace Post
	{
		IPostboxSupervisor* CreatePostbox()
		{
			return new Postbox();
		}
	}
}