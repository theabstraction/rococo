/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	http://www.sexiestengine.com

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	and 'Society of Demons.'

	1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.
	
	1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
	The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging. 

	1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
	later versions of Sexy without compromising Sexy's copyright license.
	
	2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application. 

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

namespace
{
	struct ContainerLock
	{
		uint8* ContainerPtr;
		int32 locMemberOffset;
	};

	struct ExceptionHandler
	{
		ID_BYTECODE FunctionId;
		size_t Start;
		size_t End;
		size_t HandlerOffset;
	};

	struct NativeException
	{
		ObjectStub Header;
		int32 errorCode;
		cstr messageHandle;
	};

	typedef std::vector<ExceptionHandler> TExceptionHandlers;

	int GetNullSize(const IStructure& type)
	{
		int size = type.SizeOfStruct();
		for(int i = 0; i < type.InterfaceCount(); ++i)
		{
			size = std::max(type.GetInterface(i).NullObjectType().SizeOfStruct(), size);
		}

		return size;
	}

	void InitObjectStubAsNullLength(ObjectStub& stub, const IStructure& type, int allocSize)
	{
		stub.AllocSize = allocSize;
		stub.Desc = (ObjectDesc*) type.GetVirtualTable(0);
		stub.pVTables[0] = (VirtualTable*) type.GetVirtualTable(1);
	}

	IFunctionBuilder* GetFunctionForAddressFromModule(const uint8* pc, IModuleBuilder& module, IProgramObject& programObject)
	{
		for(int j = 0; j < module.FunctionCount(); ++j)
		{
			IFunctionBuilder& f = module.GetFunction(j);
			CodeSection section;
			f.Code().GetCodeSection(OUT section);

			size_t functionIndex = programObject.ProgramMemory().GetFunctionAddress(section.Id);

			const uint8* functionStart = programObject.ProgramMemory().StartOfMemory() + functionIndex;
			const uint8* functionEnd =  programObject.ProgramMemory().StartOfMemory() + functionIndex + section.End;

			if (pc >= functionStart && pc <= functionEnd)
			{
				return &f;
			}
		}

		return NULL;
	}

	IFunctionBuilder* GetFunctionForAddress(const uint8* pc, IProgramObject& programObject)
	{
		for(int i = 0; i < programObject.ModuleCount(); i++)
		{
			IModuleBuilder& m = programObject.GetModule(i);

			IFunctionBuilder* f = GetFunctionForAddressFromModule(pc, m, programObject);
			if (f != NULL)
			{
				return f;
			}
		}

		IFunctionBuilder* f = GetFunctionForAddressFromModule(pc, programObject.IntrinsicModule(), programObject);
		return f;
	}

	void DeconstructCallArguments(const IFunction& f, const uint8* pc, const uint8* sf, IProgramObject& programObject, REF int& totalStackCorrection)
	{
		for(int i = 0; i < ArgCount(f); ++i)
		{
			const IArgument& arg = f.Arg(i);
			cstr name = arg.Name();

			const IStructure& s = *arg.ResolvedType();
			int sizeOfArg = (s.VarType() == VARTYPE_Derivative) ? sizeof(size_t) : s.SizeOfStruct();
			totalStackCorrection += sizeOfArg;
		}
	}

	void DeconstructLocalObjects(IScriptSystem& ss, size_t functionOffset, const uint8* sf, IFunctionBuilder& f, REF int& totalStackCorrection)
	{
		const StackRecoveryData& src = f.Builder().GetRequiredStackCorrection(functionOffset);

		/* Uncomment to debug local deconstruction
		sexstringstream stream;
		stream << ("DeconstructLocalObjects(ss,") << functionOffset << ", " << sf << ", " << f.Name()  << ", " << totalStackCorrection << ")" << std::endl;
		stream << ("src: start ") << src.InstancePosStart << ", count " << src.InstancePosCount << ", disp " << src.TotalDisplacement << std::endl;
		stream;
		ss.ProgramObject().Log().Write(stream.str().c_str());
		*/
		for(int i = 0; i < src.InstancePosCount; ++i)
		{
			int sfOffset = f.Builder().GetDestructorFromInstancePos(i + src.InstancePosStart);
			const uint8* instance = sf + sfOffset;

			const IStructure& type = f.Builder().GetTypeFromInstancePos(i + src.InstancePosStart);

			if (type.Prototype().IsClass)
			{
				CClassHeader* header = (CClassHeader*) instance;

				const ID_BYTECODE** vTablePtr = (const ID_BYTECODE**) instance;
				const ID_BYTECODE* intrinsicVTable = vTablePtr[0]; 

				ICodeBuilder& builder = (ICodeBuilder&) f.Builder();

				VM::IVirtualMachine& vm = builder.Module().Object().VirtualMachine();

				vm.Push((void*)instance);
				EXECUTERESULT status = vm.ExecuteFunction(intrinsicVTable[1]);
				vm.PopPointer();

				if (status == EXECUTERESULT_RETURNED) vm.SetStatus(EXECUTERESULT_RUNNING);
			}
			else if (AreEqual(type.Name(), ("_Lock")))
			{
				ContainerLock* locker = (ContainerLock*) instance;
				int32* pLock = (int32*) (locker->ContainerPtr + locker->locMemberOffset);
				*pLock = 0;
			}
			else if (AreEqual(type.Name(), ("_Array")))
			{
				ArrayImage* a = (ArrayImage*) instance;
				if (RequiresDestruction(*a->ElementType))
				{
					DestroyElements(*a, ss);
				}
				ArrayDelete(a, ss);
			}
			else if (AreEqual(type.Name(), ("_Node")))
			{
				NodeRef* nr = (NodeRef*) instance;
				if (nr->NodePtr != NULL) ReleaseNode(nr->NodePtr, ss);
			}
			else if (AreEqual(type.Name(), ("_List")))
			{
				ListImage* l = (ListImage*) instance;
				ListClear(*l, ss);
			}
			else if (AreEqual(type.Name(), ("_Map")))
			{
				MapImage* m = (MapImage*) instance;
				MapClear(m, ss);
			}
			else if (AreEqual(type.Name(), ("_MapNode")))
			{
				MapNodeRef* nr = (MapNodeRef*) instance;
				ReleaseNode(nr->NodePtr, ss);
			}
		}

		totalStackCorrection += src.TotalDisplacement;
	}

	void DeconstructLocalObjects(IFunctionBuilder& f, const uint8* pc, const uint8* sf, IScriptSystem& ss, REF int& totalStackCorrection)
	{
		CodeSection section;
		f.Builder().GetCodeSection(OUT section);

		size_t codeStart = ss.ProgramObject().ProgramMemory().GetFunctionAddress(section.Id);
		const uint8* functionStart = ss.ProgramObject().VirtualMachine().Cpu().ProgramStart + codeStart;
		size_t functionOffset = (size_t) (pc - functionStart);

		DeconstructLocalObjects(ss, functionOffset, sf, f, REF totalStackCorrection);
	}

	const uint8* GetHandlerFor(const TExceptionHandlers& handlers, Compiler::IProgramObject& po, const uint8* pc)
	{
		const uint8* startOfMemory = po.VirtualMachine().Cpu().ProgramStart;

		for(auto i = handlers.begin(); i != handlers.end(); ++i)
		{
			const ExceptionHandler& handler = *i;			

			size_t startOfFunctionOffset = po.ProgramMemory().GetFunctionAddress(handler.FunctionId);

			const uint8* startOfProtectedRegion = startOfMemory + startOfFunctionOffset + handler.Start;
			if (pc < startOfProtectedRegion) continue;

			const uint8* endOfProtectedRegion = startOfMemory + startOfFunctionOffset + handler.End;
			if (pc > endOfProtectedRegion) continue;

			return startOfMemory + startOfFunctionOffset + handler.HandlerOffset;
		}
		return NULL;
	}

	const uint8* GetCallerSF(OUT const uint8*& returnAddress, const uint8* SF)
	{
		// Assume SF != NULL

		const uint8* pOldSF = SF - 2 * sizeof(size_t);
		const uint8* pRetAddress = SF - sizeof(size_t);

		returnAddress = *(const uint8**) pRetAddress;
		return *(const uint8**) pOldSF;
	}

	void WriteUnhandledException(IScriptSystem& ss, CClassHeader* ex)
	{
		ss.ProgramObject().Log().Write(("Unhandled exception. Could not find try-catch block."));

		IStructure* exType = ex->_typeInfo->structDef;

		int errorCode = -1;
		cstr exceptionType = exType->Name();
		cstr exceptionMessage = ("Unspecified error");

		const IModule& module = ss.ProgramObject().GetModule(0);
		const IStructure& nativeExType = *module.FindStructure(("NativeException"));

		if (exType == &nativeExType)
		{
			NativeException* nex = (NativeException*) ex;
			ss.ProgramObject().Log().OnUnhandledException(nex->errorCode, nativeExType.Name(), nex->messageHandle, nex);
			return;
		}

		int offset;
		const IMember* member = FindMember(*exType, ("message"), OUT offset);
		if (member != NULL)
		{
			const IStructure& memberType = *member->UnderlyingType();
			if (memberType.InterfaceCount() > 0)
			{
				if (memberType.GetInterface(0) == ss.ProgramObject().Common().SysTypeIString())
				{
					CClassHeader* msgInstance = (CClassHeader*) (((uint8*) ex) + offset);
					const IStructure& concreteType = *msgInstance->_typeInfo->structDef;
					int msgOffset ;
					const IMember* msgMember = FindMember(concreteType, ("buffer"), OUT msgOffset);
					if (msgMember != NULL && msgMember->UnderlyingType()->VarType() == VARTYPE_Pointer)
					{
						uint8* pItem = ((uint8*) ex) + offset + msgOffset;
						char** pBuffer = (char**) pItem;
						if (*pBuffer != NULL)
						{
							exceptionMessage = *pBuffer;
						}
					}
				}
			}			
		}

		member = FindMember(*exType, ("errorCode"), OUT offset);
		if (member != NULL && member->UnderlyingType()->VarType() == VARTYPE_Int32)
		{
			uint8* pItem = ((uint8*) ex) + offset;
			int32* pErrorCode = (int32*) pItem;
			errorCode = *pErrorCode;
		}

		ss.ProgramObject().Log().OnUnhandledException(errorCode, exceptionType, exceptionMessage, ex);
	}

	void LogCpu(ILog& logger, const VM::CPU& cpu)
	{
		sexstringstream<1024> stream;
		stream.sb.AppendFormat("SF: %p\n", cpu.SF());
      stream.sb.AppendFormat("PC: %p\n", cpu.SF());
      stream.sb.AppendFormat("Stack: %p to %p\n", cpu.StackStart, cpu.StackEnd);
		logger.Write(stream);
	}

	bool CatchException(const TExceptionHandlers& handlers, IScriptSystem& ss)
	{
		VM::CPU& cpu = ss.ProgramObject().VirtualMachine().Cpu();

		// LogCpu(ss.ProgramObject().Log(), cpu);

		const uint8* sf = cpu.SF();
		const uint8* returnAddress = cpu.PC();

		int totalStackCorrection = 0;

		while(true)
		{				
			const uint8* pc = returnAddress;

			IFunctionBuilder* f = GetFunctionForAddress(pc, ss.ProgramObject());
			if (f == NULL)
			{
				return false;
			}

			cstr fname = f->Name();

			DeconstructLocalObjects(*f, pc, sf, ss, REF totalStackCorrection);

			const uint8* catchAddress = GetHandlerFor(handlers, ss.ProgramObject(), pc);
			if (catchAddress != NULL)
			{
				cpu.SetSF(sf);
				cpu.SetPC(catchAddress);
				cpu.D[VM::REGISTER_SP].uint8PtrValue -= totalStackCorrection;
				return true;
			}

			totalStackCorrection += 2 * sizeof(size_t); // the return address and the old stack frame are popped off the stack

			const uint8* oldSf = GetCallerSF(OUT returnAddress, sf);

			if (oldSf >= sf) return false; // If sf is NULL, the execution stub, then oldSf will also be null
			sf = oldSf;
		}
	}

	const IStructure& GetType(const void* instance)
	{
		const CClassHeader* header = (const CClassHeader*) instance;
		return *header->_typeInfo->structDef;
	}

	const uint8* GetPtr(const CClassHeader* header, ptrdiff_t offset) 
	{
		return ((const uint8*) header) + offset;
	}

	bool IsPtrInsideInstance(const uint8* ptr, const CClassHeader* instance)
	{
		return ptr >= (const uint8*) instance && ptr < GetPtr(instance, + instance->_allocSize);
	}

	bool IsPtrInsideStack(const uint8* ptr, Compiler::IProgramObject& po)
	{
		VM::CPU& cpu = po.VirtualMachine().Cpu();
		return ptr >= cpu.StackStart && ptr < cpu.StackEnd;
	}

	void UpdateRelativePointer(uint8* newInstance, const CClassHeader* oldInstance, ptrdiff_t offset, const void* oldPtr)
	{
		uint8* newInstancePtr = newInstance + offset;
		uint8** ptrValue = (uint8**) newInstancePtr;

		ptrdiff_t internalOffset =  (const uint8*) oldPtr - (const uint8*) oldInstance;

		*ptrValue = newInstance + internalOffset;
	}

	void UpdateInternalExceptionPointers(Compiler::IProgramObject& po, uint8* newInstance, const CClassHeader* oldInstance, const IStructure& memberType, cstr name, ptrdiff_t offset)
	{
		VARTYPE type = memberType.VarType();
		if (type == VARTYPE_Derivative)
		{
			for(int i = 0; i < memberType.MemberCount(); ++i)
			{
				const IMember& m = memberType.GetMember(i);
				UpdateInternalExceptionPointers(po, newInstance, oldInstance, *m.UnderlyingType(), m.Name(), offset);
				offset += m.SizeOfMember();
			}
		}
		else if (type == VARTYPE_Pointer)
		{
			const uint8** pPointer = (const uint8**) GetPtr(oldInstance, offset);
			const uint8* ptr = *pPointer;

			if (IsPtrInsideInstance(ptr, oldInstance))
			{
				UpdateRelativePointer(newInstance, oldInstance, offset, ptr);
				return;
			}

			if (IsPtrInsideStack(ptr, po))
			{				
				sexstringstream<1024> streamer;
				streamer.sb << ("A pointer inside an exception object '") << GetType(oldInstance).Name() << (".") << name << ("' referred to another object on the stack");
				po.Log().Write(*streamer.sb);
				po.VirtualMachine().Throw();
				return;
			}
		}					
	}

	void CopyException(Compiler::IProgramObject& po, void* newInstance, const CClassHeader* oldInstance)
	{
		AlignedMemcpy(newInstance, oldInstance, oldInstance->_allocSize);

		const IStructure& s = GetType(oldInstance);
		UpdateInternalExceptionPointers(po, (uint8*) newInstance, oldInstance, s, ("ex"), 0);
	}

	CClassHeader* ReadExceptionFromInput(int inputNumber, IPublicProgramObject& po, const IFunction& f)
	{
		void* ex;
		ReadInput(0, ex, po, f);

		const uint8* pExInstance = (const uint8*) ex;
		const ID_BYTECODE** ppVTable = (const ID_BYTECODE**) ex;
		const ID_BYTECODE* vTableInterface = *ppVTable;
		ptrdiff_t offset = (ptrdiff_t) vTableInterface[0];
		CClassHeader* header = (CClassHeader*)  pExInstance + offset;	
		return header;
	}

	class CDefaultExceptionLogic
	{
	private:
		TExceptionHandlers exceptionHandlers;
		bool isWithinException;
		IScriptSystem& ss;

		void Throw(const Compiler::IFunction& function)
		{
			IProgramObject& po = ss.ProgramObject();

			if (isWithinException)
			{
				po.Log().Write(("An exception was thrown inside the processing of a throw. The most likely cause is that a destructor threw an exception."));
				po.VirtualMachine().Throw();
				return;
			}

			isWithinException = true;

			CClassHeader* header = ReadExceptionFromInput(0, po, function);
			const IStructure& underlyingType = GetType(header);
			const IInterface& iexc = po.Common().SysTypeIException();
			
			int nullSizeInBytes = iexc.NullObjectType().SizeOfStruct();
			if (header->_allocSize > nullSizeInBytes)
			{
				po.Log().Write(("An exception was thrown and the null object for Sys.Type.IException was too small to hold the exception reference"));
				po.VirtualMachine().Throw();
				return;
			}

			uint8* newEx = (uint8*)alloca(nullSizeInBytes);
			CopyException(po, newEx, header);

			CClassHeader* tempEx = (CClassHeader*) newEx;

			if (!CatchException(exceptionHandlers, ss))
			{
				WriteUnhandledException(ss, tempEx);
				po.VirtualMachine().Throw();
			}
			else
			{				
				po.VirtualMachine().PushBlob(newEx, nullSizeInBytes);
				// At this point the header block may have been overwritten, so use the temp version to correct the new exception on the stack
				CopyException(po, (uint8*) po.VirtualMachine().Cpu().SP() - nullSizeInBytes, tempEx);
			}

			isWithinException = false;
		}
	public:
		CDefaultExceptionLogic(IScriptSystem& _ss): ss(_ss)
		{
			isWithinException = false;
		}

		void AddCatchHandler(ID_BYTECODE id, size_t start, size_t end, size_t handlerOffset)
		{
			ExceptionHandler handler;
			handler.FunctionId = id;
			handler.Start = start;
			handler.End = end;
			handler.HandlerOffset = handlerOffset;
			exceptionHandlers.push_back(handler);
		}

		void Clear()
		{
			isWithinException = false;
			exceptionHandlers.clear();
		}

		void InstallThrowHandler()
		{
			INamespaceBuilder& ns = ss.ProgramObject().GetRootNamespace().AddNamespace(("Sys.Type"), ADDNAMESPACEFLAGS_NORMAL);

			struct ANON
			{
				static void CALLTYPE_C Throw(NativeCallEnvironment& e)
				{
					CDefaultExceptionLogic* logic = (CDefaultExceptionLogic*) e.context;
					logic->Throw(e.function);
				}

				static void CALLTYPE_C GetSysMessage(NativeCallEnvironment& e)
				{
					IScriptSystem& ss = (IScriptSystem&) e.ss;

					cstr msgHandle;
					ReadInput(0, (void*&) msgHandle, e);
					CStringConstant* sc = ss.GetStringReflection(msgHandle);
					WriteOutput(0, &sc->header._vTables[0], e);
				}
			};

			ss.AddNativeCall(ns, ANON::Throw, this, ("_throw (Sys.Type.Pointer ex)->"), false);		
			ss.AddNativeCall(ns, ANON::GetSysMessage, &ss, ("GetSysMessage (Sys.Type.Pointer msgHandle) -> (Sys.Type.IString message)"));
		}

		void ThrowFromNativeCode(int32 errorCode, cstr staticRefMessage)
		{
			const IModule& module = ss.ProgramObject().GetModule(0);
			const IStructure& nativeExType = *module.FindStructure(("NativeException"));
			
			int size = GetNullSize(nativeExType);
			NativeException* ex = (NativeException*) alloca(size);

			InitObjectStubAsNullLength(ex->Header, nativeExType, size);
			ex->errorCode = errorCode;
			ex->messageHandle = staticRefMessage;

			isWithinException = true;
			if (!CatchException(exceptionHandlers, ss))
			{
				WriteUnhandledException(ss, (CClassHeader*) ex);
				ss.ProgramObject().VirtualMachine().Throw();
			}
			else
			{				
				ss.ProgramObject().VirtualMachine().PushBlob(ex, size);
			}

			isWithinException = false;
		}
	};
}