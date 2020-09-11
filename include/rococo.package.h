#pragma once

namespace Rococo
{
	/*
		Package management class.
		The consumer is expected to call CountFiles and CountDirectories with null arguments
		This gives the top level set of items. The top level directories are resource paths
		that can be used for recursive iteration. Resource paths consist of tokens separated
		by the unix path separator /. Tokens obey the Sexy namespace conventions, begining with
		A-Z and followed by a sequence of alpha numericals. File names are more or less
		arbitrary, but if they link to a SEXY file they should consist of a namespace token
		with extension sxy.

		Example Sys/Maths/F32.sxy

		It is expected that a consumer of package will map the resource path to Sexy namespace
		format. So Sys/Maths/I64.sxy will map to Sys.Maths.I64. Any paths that do not obey
		the Sexy namespace protocol will not map to a Sexy namespace. Any SXY files that
		do not obey the Sexy namespace protocol should cause an exception in the consumer
		and complain to the application that the IPackage implementation is non-conformant.
	*/
	ROCOCOAPI IPackage
	{
		/*
			Every package has a unique id, such as a guid, or a URL + version
			This means any consumer can cache its contents without redundant copying
		*/
		virtual cstr UniqueName() const = 0;

		virtual void LoadFileImageForCopying(const char* resourcePath, IFileHandler& handler) = 0;
		virtual void LoadFileImageIntoBuffer(const char* resourcePath, void* buffer, int64 capacity) = 0;

		virtual void GetFileInfo(const char* resourcePath, int index, SubPackageData& pkg) const = 0;
		virtual void GetDirectoryInfo(const char* resourcePath, int index, SubPackageData& pkg) const = 0;

		virtual int CountDirectories(const char* resourcePath) const = 0;
		virtual int CountFiles(const char* resourcePath) const = 0;
	};

	ROCOCOAPI IPackageSupervisor : IPackage
	{
		virtual void Free() = 0;
	};
}