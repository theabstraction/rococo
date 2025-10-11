// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#ifndef ROCOCO_POST_H
#define ROCOCO_POST_H

#include <rococo.types.h>

namespace Rococo
{
	namespace Post
	{
		enum POST_TYPE : int64; // This should be implemented by the consumer of the postbox
	}

	struct Mail
	{
		Post::POST_TYPE id;
		const void* buffer;
		const uint64 nBytes;
	};

	namespace Post
	{
		enum { LARGEST_MESSAGE_SIZE = 47 };

		template<class T> POST_TYPE GetPostType()
		{
			static_assert(sizeof(T) <= LARGEST_MESSAGE_SIZE, "Increase LARGEST_MESSAGE_SIZE");
			return T::GetPostType();
		}

		struct IRecipient
		{
			virtual void OnPost(const Mail& mail) = 0;
		};

		template<class T> inline const T* InterpretAs(const Mail& mail)
		{
			POST_TYPE id = GetPostType<T>();
			if (mail.id != id || sizeof(T) != mail.nBytes) return nullptr;
			return reinterpret_cast<const T*>(mail.buffer);
		}

		ROCOCO_INTERFACE IPostbox
		{
			virtual void PostForLater(const Mail& mail, bool isLossy) = 0;
			virtual void SendDirect(const Mail& mail) = 0;
			virtual void Subscribe(POST_TYPE id, IRecipient* recipient) = 0;
			virtual void Unsubscribe(POST_TYPE id, IRecipient* recipient) = 0;

			template<class T> void SendDirect(const T& t)
			{
				SendDirect(Mail{ GetPostType<T>(), (const void*)&t, sizeof(T) });
			}

			template<class T> void PostForLater(const T& t, bool isLossy)
			{
				PostForLater(Mail{ GetPostType<T>(), (const void*)&t, sizeof(T) }, isLossy);
			}

			template<class T> void Subscribe(IRecipient* recipient)
			{
				Subscribe(GetPostType<T>(), recipient);
			}

			template<class T> void Unsubscribe(IRecipient* recipient)
			{
				Unsubscribe(GetPostType<T>(), recipient);
			}
		};

		template<class T> class Subscribtion
		{
		private:
			IPostbox* postbox;
			IRecipient* recipient;
		public:
			Subscribtion() : postbox(nullptr), recipient(nullptr)
			{

			}

			~Subscribtion()
			{
				End();
			}

			void Start(IPostbox& postbox, IRecipient* recipient)
			{
				if (!this->postbox)
				{
					this->postbox = &postbox;
					this->recipient = recipient;
					postbox.Subscribe<T>(recipient);
				}
			}

			void End()
			{
				if (postbox)
				{
					postbox->Unsubscribe<T>(recipient);
					postbox = nullptr;
				}
			}
		};

		ROCOCO_INTERFACE IPostboxSupervisor : public IPostbox
		{
			virtual void Deliver() = 0;
			virtual void Free() = 0;
		};

		IPostboxSupervisor* CreatePostbox();
	}
}

#endif