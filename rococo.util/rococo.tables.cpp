#include <rococo.api.h>
#include <rococo.io.h>

namespace Rococo::IO
{
	void ValidateHeader(const ColumnHeader& archiveHeader, ColumnType cppType, cstr archiveFile)
	{
		if (archiveHeader.type != cppType)
		{
			Throw(0, "Type mismatch for column %s in %s\nType in archive file was %d\nType in C++ was %d", archiveHeader.name, archiveFile, archiveHeader.type, cppType);
		}
	}

	void ParseTableRows(IBinarySource& source, ITableRowBuilder& builder)
	{

	}
}