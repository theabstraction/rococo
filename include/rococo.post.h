#ifndef ROCOCO_POST_H
#define ROCOCO_POST_H

#ifndef Rococo_TYPES_H
#error "include <rococo.types.h> before including this file"
#endif

namespace Rococo
{
	namespace Post
	{
		enum POST_TYPE : int64; // This should be implemented by the consumer of the postbox

		enum { LARGEST_MESSAGE_SIZE = 47 };

		template<class T> POST_TYPE GetPostType(const T& t)
		{
			static_assert(sizeof(T) <= LARGEST_MESSAGE_SIZE, "Increase LARGEST_MESSAGE_SIZE");
			return T::GetPostType();
		}

		struct IRecipient
		{
			virtual void OnPost(POST_TYPE id, const void* buffer, uint64 nBytes) = 0;
		};

		template<class T> inline const T& InterpretAs(POST_TYPE id, const void* buffer, uint64 nBytes)
		{
			if (sizeof(T) != nBytes) Throw(0, L"Bad message conversion");
			return *reinterpret_cast<const T*>(buffer);
		}

		ROCOCOAPI IPostbox
		{
			virtual void PostForLater(POST_TYPE id, const void* buffer, uint64 nBytes, bool isLossy) = 0;
			virtual void SendDirect(POST_TYPE id, const void* buffer, uint64 nBytes) = 0;
			virtual void Subscribe(POST_TYPE id, IRecipient* recipient) = 0;
			virtual void Unsubscribe(POST_TYPE id, IRecipient* recipient) = 0;

			template<class T> void SendDirect(const T& t)
			{
				SendDirect(GetPostType(t), (const void*)&t, sizeof(T));
			}

			template<class T> void PostForLater(const T& t, bool isLossy)
			{
				PostForLater(GetPostType(t), (const void*)&t, sizeof(T), isLossy);
			}
		};

		ROCOCOAPI IPostboxSupervisor : public IPostbox
		{
			virtual void Deliver() = 0;
			virtual void Free() = 0;
		};

		IPostboxSupervisor* CreatePostbox();
	}
}

#endif