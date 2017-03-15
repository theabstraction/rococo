#include "hv.h"
#include <string.h>

namespace HV
{
   namespace Strings
   {
      void ValidateFQNameIdentifier(const wchar_t* fqName)
      {
         if (fqName == nullptr)
         {
            Throw(0, L"Error validating fully qualified name - null");
         }

         if (*fqName == 0)
         {
            Throw(0, L"Error validating fully qualified name - blank");
         }

         if (wcslen(fqName) > MAX_FQ_NAME_LEN)
         {
            Throw(0, L"Error validating fully qualified name - exceeded maximum of %d chars", MAX_FQ_NAME_LEN);
         }

         enum State
         {
            State_ExpectingSubspace,
            State_InSupspace,
         } state = State_ExpectingSubspace;

         for (auto* p = fqName; *p != 0; p++)
         {
            if (state == State_ExpectingSubspace)
            {
               if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9'))
               {
                  // Dandy
                  state = State_InSupspace;
               }
               else
               {
                  Throw(0, L"Error validating fully qualified name - Characters must be in range 0-9 or a-z");
               }
            }
            else // Insubspace
            {
               if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9'))
               {
                  // Dandy
               }
               else if (*p == '.')
               {
                  state = State_ExpectingSubspace;
               }
               else
               {
                  Throw(0, L"Error validating fully qualified name - Characters must be in range 0-9 or a-z. Use '.' to separate subspaces");
               }
            }
         }

         if (state == State_ExpectingSubspace)
         {
            Throw(0, L"Error validating fully qualified name - name must not terminate on a period '.'");
         }
      }
   }
}