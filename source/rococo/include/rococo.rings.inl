// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

namespace Rococo
{
   template<class T>
   class Ring: public IRing<T>
   {
   private:
      const T* ring;
      size_t nElements;
   public:
      Ring(const T* _ring, size_t _nElements) : ring(_ring), nElements(_nElements) {}
      size_t ElementCount() const
      {
         return nElements;
      }

      T operator[](size_t index) const
      {
         return ring[index % nElements];
      }

      bool IsEmpty() const
      {
         return nElements == 0;
      }

      const T* Array() const
      {
         return ring;
      }
   };

   template<class T>
   class RingManipulator: public IRingManipulator<T>
   {
   private:
      T* ring;
      size_t nElements;
   public:
      RingManipulator(T* _ring, size_t _nElements) :
         ring(_ring), nElements(_nElements)
      {

      }

      size_t ElementCount() const override
      {
         return nElements;
      }

      T operator[](size_t index) const override
      {
         return ring[index % nElements];
      }

      bool IsEmpty() const override
      {
         return nElements == 0;
      }

      void Erase(size_t index) override
      {
         if (nElements == 0)
         {
            Throw(0, "Ring is empty in call to RingManipulator::Erase(...)");
         }

         size_t normalizedIndex = index % nElements;

         for (size_t i = normalizedIndex + 1; i < nElements; ++i)
         {
            ring[i - 1] = ring[i];
         }

         nElements--;
      }

      T* Array() override
      {
         return ring;
      }
   };
}