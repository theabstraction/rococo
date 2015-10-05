namespace Bloke
{
	namespace Fonts
	{
		int FindFirstFont(IFont& font, const char* specToken, bool throwOnError)
		{
			for (int i = 0; i < font.NumberOfGlyphSets(); ++i)
			{
				if (strstr(font[i].Name(), specToken) != nullptr)
				{
					return i;
				}
			}

			if (throwOnError)
			{
				Throw(0, L"Cannot find any font with token %S", specToken);
			}

			return -1;
		}

		int GetFontMatchingHeight(IFont& font, const char* specToken, float height, bool throwOnError)
		{
			int firstIndex = FindFirstFont(font, specToken, throwOnError);

			if (firstIndex < 0) return -1;

			for (int i = firstIndex; i < font.NumberOfGlyphSets(); ++i)
			{
				float fontHeight = font[i].FontHeight();
				if (fontHeight == height)
				{
					return i;
				}
			}

			if (throwOnError)
			{
				Throw(0, L"Cannot find %S font with height matching %f", specToken, height);
			}

			return -1;
		}

		int GetFontNearestHeight(IFont& font, const char* specToken, float height, bool throwOnError)
		{
			int firstIndex = FindFirstFont(font, specToken, throwOnError);

			if (firstIndex < 0) return -1;

			float bestDelta = 1000000.0f;
			int bestIndex = firstIndex;

			for (int i = firstIndex; i < font.NumberOfGlyphSets(); ++i)
			{
				if (strstr(font[i].Name(), specToken) == nullptr) continue;
				float fontHeight = font[i].FontHeight();
				float delta = fabsf(fontHeight - height);
				if (bestDelta > delta)
				{
					bestDelta = delta;
					bestIndex = i;
				}
			}

			return bestIndex;
		}
	}
}