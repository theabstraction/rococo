namespace Rococo
{
   namespace OS
   {
      struct UltraClock : public IUltraClock
      {
         ticks hz;
         ticks frameStart;
         ticks start;
         ticks frameDelta;
         Seconds dt;

         UltraClock()
         {
            if (!QueryPerformanceFrequency((LARGE_INTEGER*)&hz))
            {
               Throw(GetLastError(), L"Cannot acquire high performance monitor");
            }

            QueryPerformanceCounter((LARGE_INTEGER*)&start);
         }

         virtual ticks Hz() const { return hz; }
         virtual ticks FrameStart() const { return frameStart; }
         virtual ticks Start() const { return start; }
         virtual ticks FrameDelta() const { return frameDelta; }
         virtual Seconds DT() const { return dt; }
      };
   } // OS
} // Rococo
