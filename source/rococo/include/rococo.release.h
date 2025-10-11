// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

namespace Rococo
{
	template<class T>
	struct AutoRelease
	{
		T* item;

		AutoRelease(T* refItem) : item(refItem)
		{
		}

		~AutoRelease()
		{
			if (item)
				item->Release();
		}

		AutoRelease() : item(nullptr)
		{
		}

		void operator = (T* refItem)
		{
			item = refItem;
		}

		T** operator& ()
		{
			return &item;
		}

		operator T* ()
		{
			return item;
		}

		T* operator->()
		{
			return item;
		}

		T& operator*()
		{
			return *item;
		}
	};
}
