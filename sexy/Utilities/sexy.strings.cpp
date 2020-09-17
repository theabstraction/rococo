#include <sexy.types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sexy.strings.h>

namespace Rococo
{
	bool operator == (const sexstring_key& a, const sexstring_key& b)
	{
		return a.Length == b.Length && memcmp(a.Text, b.Text, a.Length) == 0;
	}

	int32 Compare(sexstring a, const char* b) { return Compare(a->Buffer, b); }

	bool AreEqual(sexstring a, sexstring b)
	{
		return a->Length == b->Length && Compare(a->Buffer, b->Buffer) == 0;
	}

	bool operator < (const sexstring_key& a, const sexstring_key& b)
	{
		int64 lengthDelta = a.Length - b.Length;
		if (lengthDelta < 0)
		{
			return true;
		}
		else if (lengthDelta > 0)
		{
			return false;
		}
		else
		{
			return Compare(a.Text, b.Text, a.Length) < 0;
		}
	}
}