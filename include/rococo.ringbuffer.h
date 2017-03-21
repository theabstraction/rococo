#ifndef ROCOCO_RINGBUFFER_H
#define ROCOCO_RINGBUFFER_H

namespace Rococo
{
	template<class ELEMENT_TYPE> class OneReaderOneWriterCircleBuffer
	{
	   typedef ELEMENT_TYPE Type;

	private:
	   ELEMENT_TYPE* elements;
	   int32 elementCount;
	   int32 readPos;
	   int32 writePos;
	public:
	   OneReaderOneWriterCircleBuffer(int32 _elementCount) : elementCount(_elementCount), readPos(0), writePos(0)
	   {
		  elements = new ELEMENT_TYPE[_elementCount];
	   }

	   ~OneReaderOneWriterCircleBuffer()
	   {
		  delete[] elements;
	   }

	   bool IsEmpty() const
	   {
		  return readPos == writePos;
	   }

	   ELEMENT_TYPE* GetBackSlot()
	   {
		  if ((writePos + 1) % elementCount == readPos) return nullptr;
		  else return elements + writePos;
	   }

	   void WriteBack()
	   {
		  writePos = (writePos + 1) % elementCount;
	   }

	   ELEMENT_TYPE* TryPopFront()
	   {
		  if (readPos == writePos)
		  {
			 return nullptr;
		  }

		  ELEMENT_TYPE* front = elements + readPos;
		  readPos = (readPos + 1) % elementCount;
		  return front;
	   }
	};
}

#endif
