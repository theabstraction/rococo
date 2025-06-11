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
	   OneReaderOneWriterCircleBuffer(size_t _elementCount) : elementCount((int32)_elementCount+1), readPos(0), writePos(0)
	   {
		   if (_elementCount >= 0x7FFFFFFELL) Throw(0, "%s: element count too high", __ROCOCO_FUNCTION__);
		   elements = new ELEMENT_TYPE[_elementCount+1];
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

	   bool TryPopFront(ELEMENT_TYPE& front)
	   {
		  if (readPos == writePos)
		  {
			 return false;
		  }

		  ELEMENT_TYPE* p = elements + readPos;
		  front = *p;
		  readPos = (readPos + 1) % elementCount;
		  return true;
	   }
	};
}

#endif
