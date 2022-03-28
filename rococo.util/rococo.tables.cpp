#include <rococo.api.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <vector>

namespace Rococo::IO
{
	void ValidateHeader(const ColumnHeader& archiveHeader, ColumnType cppType, cstr archiveFile)
	{
		if (archiveHeader.type != cppType)
		{
			Throw(0, "Type mismatch for column %s in %s\nType in archive file was %d\nType in C++ was %d", archiveHeader.name, archiveFile, archiveHeader.type, cppType);
		}
	}

	struct ColumnHeaderImplementation
	{
		HString name;
	};

	// Rather than use alloca, which is not suitable for arbitrarily large tables, we instead use thread-local sliding vectors
	thread_local std::vector<char> scratch;
	thread_local std::vector<ColumnHeader> columnScratch;
	thread_local std::vector<ColumnHeaderImplementation> columnBacking;

	static void ValidateString(IBinarySource& source, const fstring text)
	{
		uint8 len;
		source.Read(1, &len);

		if (len != text.length)
		{
			Throw(0, "Validation failed. Expected %s. Archive string length was %u bytes", text.buffer, len);
		}

		scratch.resize(len);
		source.Read(len, scratch.data());

		if (memcmp(scratch.data(), text.buffer, len) != 0)
		{
			Throw(0, "Validation failed. Expected %s", text.buffer);
		}
	}

	static void ValidateInt32(IBinarySource& source, int32 value)
	{
		int32 archiveValue;
		source.Read(sizeof int32, &archiveValue);

		if (archiveValue != value)
		{
			Throw(0, "Validation failed. Expected 0x%X", value);
		}
	}

	static void ValidateInt64(IBinarySource& source, int64 value)
	{
		int64 archiveValue;
		source.Read(sizeof int64, &archiveValue);

		if (archiveValue != value)
		{
			Throw(0, "Validation failed. Expected 0x%XLL", value);
		}
	}

	static void ValidateFloat32(IBinarySource& source, float value)
	{
		float archiveValue;
		source.Read(sizeof(float), &archiveValue);

		if (archiveValue != value)
		{
			Throw(0, "Validation failed. Expected %g", value);
		}
	}

	static void ValidateFloat64(IBinarySource& source, double value)
	{
		double archiveValue;
		source.Read(sizeof(double), &archiveValue);

		if (archiveValue != value)
		{
			Throw(0, "Validation failed. Expected %g", value);
		}
	}

	static fstring NextString(IBinarySource& source)
	{
		uint8 len;
		source.Read(1, &len);
		scratch.reserve(len + 1);
		source.Read(len, scratch.data());
		scratch[len] = 0;
		return { scratch.data(), len };
	}

	template<class T> T NextItem(IBinarySource& source)
	{
		T data;
		source.Read(sizeof data, &data);
		return data;
	}

	void ParseTableRows(IBinarySource& source, ITableRowBuilder& builder)
	{
		ValidateString(source, "Rococo.Carpenter.BinaryTable"_fstring);
		ValidateString(source, "1.0.0.0"_fstring);
		ValidateString(source, "[BeginMagic"_fstring);
		ValidateInt32(source, 0x12345678);
		ValidateInt64(source, 0x12345678ABCDEF42);
		ValidateFloat32(source, 3.1415926535897931f);
		ValidateFloat64(source, 3.1415926535897931);
		ValidateString(source, "EndMagic]"_fstring);

		fstring sourceLine = NextString(source);
		fstring sourcePrefix = "Source: "_fstring;
		if (!StartsWith(sourceLine, sourcePrefix))
		{
			Throw(0, "Expecting Source line. Found %s", sourceLine.buffer);
		}

		U8FilePath sourceXL;
		Format(sourceXL, "%s", sourceLine.buffer + sourcePrefix.length);

		fstring tableLine = NextString(source);
		fstring tablePrefix = "Table: "_fstring;
		if (!StartsWith(tableLine, tablePrefix))
		{
			Throw(0, "Expecting Table line. Found %s", tableLine.buffer);
		}

		U8FilePath table;
		Format(table, "%s", tableLine.buffer + tablePrefix.length);

		TableRowHeaders headers;
		headers.NumberOfRows = NextItem<int32>(source);
		headers.ExcelFile = sourceXL;
		headers.TableName = table;

		if (headers.NumberOfRows < 0 || headers.NumberOfRows > 1048576)
		{
			Throw(0, "Validation failed. Expected number of rows is [1,1048576]. Archive value was %d", headers.NumberOfRows);
		}

		builder.OnHeaders(headers);

		ValidateString(source, "[BeginColumns:"_fstring);

		int numberOfColumns = NextItem<int>(source);
		if (numberOfColumns < 1 || numberOfColumns > 4096)
		{
			Throw(0, "Validation failed. Expected number of columns is [1,4096]. Archive value was %d", numberOfColumns);
		}

		ValidateString(source, "Types"_fstring);

		columnBacking.resize(numberOfColumns);
		columnScratch.resize(numberOfColumns);

		for (int i = 0; i < numberOfColumns; i++)
		{
			ColumnType type = ColumnType::UnderlyingTypeBool;

			fstring typeString = NextString(source);
			if (Eq(typeString, "Int32"))
			{
				type = ColumnType::UnderlyingTypeInt32;
			}
			else if (Eq(typeString, "Int64"))
			{
				type = ColumnType::UnderlyingTypeInt64;
			}
			else if (Eq(typeString, "Float32"))
			{
				type = ColumnType::UnderlyingTypeFloat32;
			}
			else if (Eq(typeString, "Float64"))
			{
				type = ColumnType::UnderlyingTypeFloat64;
			}
			else if (Eq(typeString, "Bool"))
			{
				type = ColumnType::UnderlyingTypeBool;
			}
			else
			{
				Throw(0, "Unhandled type in column #%d", i);
			}

			columnScratch[i].type = type;
		}

		ValidateString(source, "C++Types"_fstring);

		for (int i = 0; i < numberOfColumns; i++)
		{
			cstr cppType = NextString(source);
		}

		ValidateString(source, "Titles"_fstring);

		for (int i = 0; i < numberOfColumns; i++)
		{
			cstr columnTitle = NextString(source);
			columnBacking[i].name = columnTitle;
			columnScratch[i].name = columnBacking[i].name;
		}

		ValidateString(source, "EndColumns]"_fstring);
		ValidateString(source, "[BeginRows"_fstring);

		builder.OnColumns(numberOfColumns, columnScratch.data());

		for (int i = 0; i < headers.NumberOfRows; ++i)
		{
			int sanityCheck = NextItem<int>(source);
			if (sanityCheck != i)
			{
				Throw(0, "Sanity check failed. Expecting row index %d", i);
			}

			struct ANON : ITableRowData
			{
				IBinarySource* source = nullptr;

				int32 NextInt32() override
				{
					return NextItem<int32>(*source);
				}

				int64 NextInt64() override
				{
					return NextItem<int64>(*source);
				}

				float NextFloat32() override
				{
					return NextItem<float>(*source);
				}

				double NextFloat64() override
				{
					return NextItem<double>(*source);
				}

				bool NextBool() override
				{
					uint8 boolValue = NextItem<uint8>(*source);
					return boolValue ? true : false;
				}

				fstring NextTempString() override
				{
					return NextString(*source);
				}

			} row;
			
			row.source = &source;

			builder.OnRow(row);
		}

		ValidateString(source, "EndRows]"_fstring);

		scratch.reserve(0);
		columnScratch.reserve(0);
		columnBacking.reserve(0);
	}

	void ParseTableRows(cstr sourcePath, ITableRowBuilder& builder)
	{
		try
		{
			WideFilePath sysPath;
			Assign(sysPath, sourcePath);
			AutoFree<IBinarySource> binarySource = Rococo::IO::ReadBinarySource(sysPath);
			ParseTableRows(*binarySource, builder);
		}
		catch (IException& ex)
		{
			Throw(ex.ErrorCode(), "%s. Error parsering %s", ex.Message(), sourcePath);
		}
	}

	void ParseTableRows(const IInstallation& installation, cstr pingPath, ITableRowBuilder& builder)
	{
		try
		{
			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(pingPath, sysPath);
			AutoFree<IBinarySource> binarySource = Rococo::IO::ReadBinarySource(sysPath);
			ParseTableRows(*binarySource, builder);
		}
		catch (IException& ex)
		{
			Throw(ex.ErrorCode(), "%s. Error parsering %s", ex.Message(), pingPath);
		}
	}
}