#include <..\sexy\Common\sexy.types.h>
#include <..\sexy\Common\sexy.s-parser.h>
#include <vector>

#include <sexy.windows.h>
#include <rococo.strings.h>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::SexyWindows;

	class RollingLog : public IRollingLog, public ILog
	{
		std::vector<char> logBuffer;
		std::vector<cstr> rows;
		const int lineLength;
		const int rowCount;

		int linePosition = 0;
		int rowPosition = 0;

	public:
		RollingLog(int _lineLength, int _rowCount) :
			lineLength(_lineLength), rowCount(_rowCount)
		{
			logBuffer.resize(lineLength * rowCount);
			std::fill(logBuffer.begin(), logBuffer.end(), 0);
			rows.resize(rowCount + 1);
			std::fill(rows.begin(), rows.end(), nullptr);
		}

		ILog& SexyLog()
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}

		operator StringPtrArray()
		{
			for (size_t i = 0; i < rowPosition; i++)
			{
				rows[i] = logBuffer.data() + lineLength * i;
			}

			for (size_t i = rowPosition; i < rowCount; i++)
			{
				rows[i] = nullptr;
			}

			return StringPtrArray{ rows.data() };
		}

		void MoveToNextRow()
		{
			rowPosition++;
			linePosition = 0;

			if (rowPosition > rowCount - 1)
			{
				rowPosition = rowCount - 1;
				memcpy(logBuffer.data(), logBuffer.data() + lineLength, logBuffer.size() - lineLength);
				logBuffer[lineLength * rowPosition] = 0;
			}
		}

		void AppendRaw(char c)
		{
			logBuffer[lineLength * rowPosition + linePosition] = c;
			linePosition++;
			if (linePosition == lineLength - 1)
			{
				logBuffer[lineLength * rowPosition + lineLength - 1] = 0;
				MoveToNextRow();
			}
		}

		void Append(char c)
		{
			switch (c)
			{
			case '\r':
				break;
			case '\n':
				MoveToNextRow();
				break;
			default:
				AppendRaw(c);
				break;
			}
		}

		void Write(csexstr text) override
		{
			WriteRaw(text);
			MoveToNextRow();
		}

		void WriteRaw(cstr text)
		{
			for (cstr p = text; *p != 0; p++)
			{
				Append(*p);
			}
		}

		void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance) override
		{
			WriteRaw("\n        Unhandled Exception Begin     ");
			WriteRaw("\n---------------------------------------");
			WriteRaw("\n\tType: ");
			WriteRaw(exceptionType);
			WriteRaw("\n\tMessage: ");
			WriteRaw(message);

			char errorNumber[256];
			SafeFormat(errorNumber, 256, "%d (0x%08.8X)", errorCode, errorCode);

			WriteRaw("\n\tErrorCode: ");
			WriteRaw(errorNumber);

			WriteRaw("\n--------------------------------------");
			WriteRaw("\n        Unhandled Exception End       ");
			WriteRaw("\n");
		}

		void OnJITCompileException(Sex::ParseException& ex) override
		{
			WriteRaw("\n       OnJITCompileException Begin     ");
			WriteRaw("\n---------------------------------------");
			WriteRaw("\n\tName: ");
			WriteRaw(ex.Name());
			WriteRaw("\n\t");
			WriteRaw("\n\tMessage: ");
			WriteRaw(ex.Message());
			WriteRaw("\n\tSpecimen: ");
			WriteRaw(ex.Specimen());
			WriteRaw("\n\tSpecimen: ");

			char pos[64];
			SafeFormat(pos, sizeof(pos), "[%d,%d] to [%d,%d]", ex.Start().x, ex.Start().y, ex.End().x, ex.End().y);

			WriteRaw("\n\tSection: ");
			WriteRaw(pos);
			WriteRaw("\n--------------------------------------");
			WriteRaw("\n        OnJITCompileException End     ");
			WriteRaw("\n");
		}
	};
}

namespace Rococo
{
	namespace SexyWindows
	{
		IRollingLog* CreateRollingLogger(int _lineLength, int _rowCount)
		{
			return new ANON::RollingLog(_lineLength, _rowCount);
		}
	}
}