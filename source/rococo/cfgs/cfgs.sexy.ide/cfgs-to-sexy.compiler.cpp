#include "cfgs.sexy.ide.h"
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::SexyStudio;
using namespace Rococo::Strings;

static void GetTargetFilename(OUT U8FilePath& target)
{
	Format(target, "C:\\work\\rococo\\content\\scripts\\cfgs\\test.cfgs.sxy");
}

static void CompileToStringProtected(StringBuilder& sb, ISexyDatabase& db, ICFGSDatabase& cfgs)
{
	sb << "Hello world!";
}

static bool CompileToString(StringBuilder& sb, ISexyDatabase& db, ICFGSDatabase& cfgs) noexcept
{
	try
	{
		CompileToStringProtected(sb, db, cfgs);
		return true;
	}
	catch (IException& ex)
	{
		char buffer[2048];
		OS::BuildExceptionString(buffer, sizeof buffer, ex, true);
		sb << "Exception thrown compiling CFGS graphs:" << buffer;
	}
	catch (...)
	{
		sb << "Unspecified exception thrown compiling CFGS graphs:";
	}

	return false;
}

namespace Rococo::CFGS
{
	void Compile(ISexyDatabase& db, ICFGSDatabase& cfgs)
	{
		U8FilePath targetPath;
		GetTargetFilename(OUT targetPath);

		AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(32768);
		auto& sb = dsb->Builder();

		bool success = CompileToString(sb, db, cfgs);

		IO::SaveAsciiTextFile(IO::TargetDirectory_Root, targetPath, *sb);

		if (!success)
		{
			Throw(0, "Error compiling %s. Check file for more information", targetPath.buf);
		}
	}
}