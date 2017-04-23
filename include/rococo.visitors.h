#ifndef Rococo_VISITORS_H
#define  Rococo_VISITORS_H

#ifndef Rococo_TYPES_H
# error include <rococo.types.h> before including this file
#endif

// Define some visitor ROCOCOAPIs, makes component development easier to share the same ROCOCOAPIs

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

		ROCOCOAPI IUITree
		{
			virtual TREE_NODE_ID AddChild(TREE_NODE_ID parentId, cstr text, CheckState state) = 0;
			virtual TREE_NODE_ID AddRootItem(cstr text, CheckState state) = 0;
			virtual void ResetContent() = 0;
         virtual void SetId(TREE_NODE_ID nodeId, int64 id) = 0;
		};

		ROCOCOAPI IUIList
		{
			virtual void AddRow(cstr values[]) = 0; // values is null terminated array
			virtual void ClearRows() = 0;
			virtual void SetColumns(cstr columnNames[], int widths[]) = 0; // columnNames is null terminated array, and every non-null member is matched in widths
			virtual int NumberOfRows() const = 0;
			virtual void DeleteRow(int rowIndex) = 0;
		};

      ROCOCOAPI ITreePopulator
      {
         virtual void Populate(IUITree& tree) = 0;
      };

      ROCOCOAPI IListPopulator
      {
         virtual void Populate(IUIList& list) = 0;
      };
	}
}

#endif // Rococo_VISITORS_H