#include <rococo.allocators.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.os.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::Sex;

void ParseFile(crwstr filename, ISParser& parser)
{
	Auto<ISourceCode> src = parser.LoadSource(filename, { 1,1 });
}

int mainProtected(int, char*[])
{
	AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(64, 4096, "main");
	Auto<ISParser> sParser = CreateSexParser_2_0(*allocator, SEXY_STANDARD_MAX_ATOMIC_STRING_LENGTH);

	crwstr filename = L"D:\\work\\Rococo.Reflect\\S-API\\Jibberish.sxy";
	ParseFile(filename, *sParser);
	
	return 0;
}

int main(int argc, char* argv[])
{
	try
	{
		return mainProtected(argc, argv);
	}
	catch (IException& ex)
	{
		std::vector<char> err;
		err.resize(64_kilobytes);
		Rococo::OS::BuildExceptionString(err.data(), err.size(), ex, true);
		printf("%s", err.data());
		return ex.ErrorCode();
	}
}