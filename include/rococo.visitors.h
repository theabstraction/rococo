#ifndef Rococo_VISITORS_H
#define  Rococo_VISITORS_H

#ifndef Rococo_TYPES_H
# error include <rococo.types.h> before including this file
#endif

// Define some visitor interfaces, makes component development easier to share the same interfaces

namespace Rococo
{
	namespace Visitors
	{
		enum CheckState : int32
		{
			CheckState_NoCheckBox,
			CheckState_Clear,
			CheckState_Ticked,
		};

		struct TREE_NODE_ID
		{
			size_t value;
		};

		struct NO_VTABLE IUITree
		{
			virtual TREE_NODE_ID AddChild(TREE_NODE_ID parentId, const wchar_t* text, CheckState state) = 0;
			virtual TREE_NODE_ID AddRootItem(const wchar_t* text, CheckState state) = 0;
			virtual void ResetContent() = 0;
		};

		struct NO_VTABLE IUIList
		{
			virtual void AddRow(const wchar_t* values[]) = 0; // values is null terminated array
			virtual void ClearRows() = 0;
			virtual void SetColumns(const wchar_t* columnNames[], int widths[]) = 0; // columnNames is null terminated array, and every non-null member is matched in widths
			virtual int NumberOfRows() const = 0;
			virtual void DeleteRow(int rowIndex) = 0;
		};
	}
}

#endif // Rococo_VISITORS_H