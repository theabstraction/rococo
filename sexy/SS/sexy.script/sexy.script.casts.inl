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
	void SetD4(VM::CPU& cpu, uint8* value)
	{
		cpu.D[VM::REGISTER_D4].uint8PtrValue = value;
	}

	ObjectStub* InterfaceToInstance(void* interf)
	{
		VirtualTable** pvTable = (VirtualTable**) interf;
		VirtualTable* vTable = *pvTable;
		ObjectStub* instance = (ObjectStub*)(vTable->OffsetToInstance + (uint8*) interf);
		return instance;
	}

	// (DynamicCast to-interface from-instance ->) The result is held in D4, which indicates 
	void DynamicCast(NativeCallEnvironment& e)
	{
		void* vCastToInterface;
		ReadInput(0, vCastToInterface, e);
		const IInterface& castToInterf = *(const IInterface*) vCastToInterface;

		void* vSrcInterface;
		ReadInput(1, vSrcInterface, e);

		ObjectStub* obj = InterfaceToInstance(vSrcInterface);							

		const IStructure& typeInfo = *obj->Desc->TypeInfo;

		int interfCount = typeInfo.InterfaceCount();

		int offset = sizeof(ObjectStub) - sizeof(void*); // Gives us the firstvTable

		for(int i = 0; i < interfCount; ++i)
		{
			const IInterface& I = typeInfo.GetInterface(i);
			if (&I == &castToInterf || (I.Base() != NULL && I.Base() == &castToInterf))
			{
				uint8* pCastedInterface =  (i * sizeof(void*) + GetInterfacePtr(*obj));
				SetD4(e.cpu, pCastedInterface);
				return;
			}
		}

		ObjectStub* nullObj = castToInterf.UniversalNullInstance();		
		SetD4(e.cpu, GetInterfacePtr(*nullObj));
	}
} // Anon