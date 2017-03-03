#pragma once

#include <rococo.types.h>
#include <sexy.types.h>
#include <stdio.h>

namespace Rococo
{
   using namespace Sexy;

   class FileAppender
   {
   private:
      FILE* hFile;
      csexstr filename;
   public:
      FileAppender(csexstr _filename);
      ~FileAppender();
      void Append(csexstr format, ...);
      void Append(char c);
      void AppendSequence(int count, char c);
   };

   void FileDelete(csexstr name);
   void TripDebugger();
   void WriteStandardErrorCode(int errorCode);
   int64 GetLastModifiedDate(const char* path);
}