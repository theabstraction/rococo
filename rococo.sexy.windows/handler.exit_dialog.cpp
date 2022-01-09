#include "sexy.windows.internal.h"

namespace ANON
{
	using namespace Rococo::SexyWindows;

	class ExitHandler : public IMessageHandler
	{
		INT_PTR id;
	public:
		ExitHandler(INT_PTR _id) : id(_id)
		{

		}

		void Handle(Message& message) override
		{
			EndDialog(message.hDlg, id);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	namespace SexyWindows
	{
		namespace Handler
		{
			IMessageHandler* NewEndDialog(INT_PTR id)
			{
				return new ANON::ExitHandler(id);
			}
		}
	}
}