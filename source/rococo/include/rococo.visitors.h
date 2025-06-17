#ifndef Rococo_VISITORS_H
#define  Rococo_VISITORS_H

#ifndef Rococo_TYPES_H
# error include <rococo.types.h> before including this file
#endif

// Define some visitor ROCOCO_INTERFACEs, makes component development easier to share the same ROCOCO_INTERFACEs

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

			struct Hasher
			{
				size_t operator()(const TREE_NODE_ID& id) const noexcept
				{
					return id.value;
				}
			};

			operator bool() const
			{
				return value != 0;
			}

			static TREE_NODE_ID Root()
			{
				return TREE_NODE_ID{ 0 };
			}
		};

		inline bool operator == (TREE_NODE_ID a, TREE_NODE_ID b)
		{
			return a.value == b.value;
		}

		inline bool operator != (TREE_NODE_ID a, TREE_NODE_ID b)
		{
			return !(a == b);
		}

		ROCOCO_INTERFACE IUITree
		{
			virtual TREE_NODE_ID AddChild(TREE_NODE_ID parentId, cstr text, CheckState state) = 0;

			// Find the first child of the parent that matches the text exactly if the text is not null, else it returns the first child of the parent
			[[nodiscard]] virtual TREE_NODE_ID FindFirstChild(TREE_NODE_ID parentId, cstr withText) = 0;

			// Find the next sibling that matches the text exactly if the text is not null, else it returns the ntext sibling
			[[nodiscard]] virtual TREE_NODE_ID FindNextChild(TREE_NODE_ID siblingId, cstr withText) = 0;
			[[nodiscard]] virtual TREE_NODE_ID GetParent(TREE_NODE_ID childId) = 0;
			[[nodiscard]] virtual TREE_NODE_ID AddRootItem(cstr text, CheckState state) = 0;
			virtual void Delete(TREE_NODE_ID id) = 0;
			virtual void ResetContent() = 0;
			virtual bool Select(TREE_NODE_ID id) = 0;
			virtual void SetId(TREE_NODE_ID nodeId, int64 id) = 0;
			virtual void SetText(TREE_NODE_ID nodeId, cstr text) = 0;
			[[nodiscard]] virtual bool TryGetText(char* buffer, size_t sizeofBuffer, TREE_NODE_ID id) = 0;
			virtual void RefreshGUI() = 0;
			virtual void ScrollTo(TREE_NODE_ID nodeId) = 0;
		};

		ROCOCO_INTERFACE IUIList
		{
			virtual void AddRow(cstr values[]) = 0; // values is null terminated array
			virtual void ClearRows() = 0;
			virtual void SetColumns(cstr columnNames[], int widths[]) = 0; // columnNames is null terminated array, and every non-null member is matched in widths
			virtual int NumberOfRows() const = 0;
			virtual void DeleteRow(int rowIndex) = 0;
		};

		ROCOCO_INTERFACE ITreePopulator
		{
		   virtual void Populate(IUITree & tree) = 0;
		};

		ROCOCO_INTERFACE IListPopulator
		{
		   virtual void Populate(IUIList & list) = 0;
		};

		ROCOCO_INTERFACE ITreeControlHandler
		{
			virtual void OnItemSelected(TREE_NODE_ID id, IUITree& origin) = 0;
			virtual void OnItemRightClicked(TREE_NODE_ID id, IUITree& origin) = 0;
		};
	}

	typedef cstr VisitorName;

	ROCOCO_INTERFACE IMathsVisitor
	{
		virtual void Clear() = 0;
		virtual void Show(VisitorName name, const Matrix4x4& m) = 0;
		virtual void ShowRow(VisitorName name, const float* vector, const size_t nComponents) = 0;
		virtual void ShowColumn(VisitorName name, const float* vector, const size_t nComponents) = 0;
		virtual void ShowDecimal(VisitorName name, const int32 value) = 0;
		virtual void Show(VisitorName name, const float value) = 0;
		virtual void ShowHex(VisitorName name, const int32 value) = 0;
		virtual void ShowBool(VisitorName name, const bool value) = 0;
		virtual void ShowDecimal(VisitorName name, const int64 value) = 0;
		virtual void ShowDecimal(VisitorName name, const uint64 value) = 0;
		virtual void ShowHex(VisitorName name, const int64 value) = 0;
		virtual void ShowPointer(VisitorName name, const void* ptr) = 0;
		virtual void ShowString(VisitorName name, cstr format, ...) = 0;
		virtual void ShowSelectableString(cstr eventName, VisitorName name, cstr format, ...) = 0;
	};

	ROCOCO_INTERFACE IMathsVenue
	{
		virtual void ShowVenue(IMathsVisitor& visitor) = 0;
	};

	namespace Windows
	{
		ROCOCO_INTERFACE IMenuBuilder
		{
			virtual IMenuBuilder & AddPopup(cstr name) = 0; // Returns the popup with the given name, or creates a new one if it was not found
			virtual void AddString(cstr name, uint64 id, cstr keyCmd = nullptr) = 0;
		};
	}
}

#endif // Rococo_VISITORS_H