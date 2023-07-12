using System;
using System.Runtime.InteropServices;

namespace SexyDotNet
{
    [StructLayout(LayoutKind.Explicit)]
    public struct RegisterUnion
    {
        public RegisterUnion(long value)
        {
            this.int32Value = 0;
            this.ptrValue = IntPtr.Zero;
            this.int64Value = value;
        }

        [FieldOffset(0)] public long int64Value;
        [FieldOffset(0)] public IntPtr ptrValue;
        [FieldOffset(0)] public int int32Value;
    }

    public class CPUImage
    {
        private RegisterUnion[] lastKnownRegisters = new RegisterUnion[256];
        private string[] registerNames = new string[256];

        public IntPtr PC { get { return lastKnownRegisters[0].ptrValue; } } // Program Counter
        public IntPtr SP { get { return lastKnownRegisters[1].ptrValue; } } // Stack Pointer
        public IntPtr SF { get { return lastKnownRegisters[2].ptrValue; } } // Stack Frame
        public IntPtr SR { get { return lastKnownRegisters[3].ptrValue; } } // Status Register
        public RegisterUnion Register(int index) { return lastKnownRegisters[index];  }
        public Action<int> EvRegisterChanged;

        public CPUImage()
        {
            registerNames[0] = "PC";
            registerNames[1] = "SP";
            registerNames[2] = "SF";
            registerNames[3] = "SR";

            for (int i = 4; i < 256; ++i)
            {
                registerNames[i] = "D" + i;
            }
        }

        public void UpdateRegister(int index, long value)
        {
            if (lastKnownRegisters[index].int64Value != value)
            {
                lastKnownRegisters[index] = new RegisterUnion(value);
                if (EvRegisterChanged != null)
                {
                    EvRegisterChanged(index);
                }                
            }
        }

        public string GetRegisterName(int index)
        {
            return registerNames[index];
        }
    }
}
