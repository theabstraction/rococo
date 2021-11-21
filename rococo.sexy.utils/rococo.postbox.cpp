#include <rococo.api.h>
#include <rococo.post.h>
#include <deque>
#include <unordered_map>
#include <algorithm>
#include <vector>

namespace
{
	using namespace Rococo;
	using namespace Rococo::Post;

#pragma pack(push,1)
	struct alignas(8) PostItem
	{
		POST_TYPE id;
		uint64 nBytes;
		char opaquedata[LARGEST_MESSAGE_SIZE];
		bool isLossy;
	} TIGHTLY_PACKED;
#pragma pack(pop)

   struct HashPostType
   {
      size_t operator() (POST_TYPE id) const
      {
         return (size_t)id;
      }
   };

	class Postbox : public IPostboxSupervisor
	{
	private:
		std::deque<PostItem> items;
		typedef std::unordered_map<POST_TYPE, std::vector<IRecipient*>, HashPostType> submap;
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
				SendDirectUnchecked(Mail{ item.id, item.opaquedata, item.nBytes });
				items.pop_front();

				if (i++ == MAX_ITEMS_PER_DELIVERY) Throw(0, "Postbox delivery failed. MAX_ITEMS_PER_DELIVERY exceeded");
			}
		}

		virtual void PostForLater(const Mail& mail, bool isLossy)
		{
			if (mail.nBytes > LARGEST_MESSAGE_SIZE) Throw(0, "Postbox message too long. Message was %d bytes. Limit is %d bytes.", mail.nBytes, LARGEST_MESSAGE_SIZE);
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
					Throw(0, "Postbox buffer exhausted. Failed to deliver #%d", mail.id);
				}
			}

			PostItem item;
			item.id = mail.id;
			item.nBytes = mail.nBytes;
			item.isLossy = isLossy;
			memcpy_s(item.opaquedata, LARGEST_MESSAGE_SIZE, mail.buffer, mail.nBytes);
			items.push_back(item);
		}

		void SendDirectUnchecked(const Mail& mail)
		{
			auto i = subscribers.find(mail.id);
			if (i != subscribers.end())
			{
				auto recipients = i->second;
				for (auto r : recipients)
				{
					r->OnPost(mail);
				}
			}
		}

		virtual void SendDirect(const Mail& mail)
		{
			if (mail.nBytes > LARGEST_MESSAGE_SIZE) Throw(0, "Postbox message too long");
			SendDirectUnchecked(mail);
		}

		virtual void Subscribe(POST_TYPE id, IRecipient* recipient)
		{
			auto s = subscribers.find(id);
			if (s == subscribers.end())
			{
				std::vector<IRecipient*> x;
				s = subscribers.insert(std::make_pair(id, x)).first;
			}

			s->second.push_back(recipient);
		}

		virtual void Unsubscribe(POST_TYPE id, IRecipient* recipient)
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