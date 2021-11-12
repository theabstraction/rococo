#pragma once

namespace Rococo
{
	namespace AutoComplete
	{
		class EditorLine
		{
		public:
			enum { MAX_LINE_LENGTH = 1024 };

		private:    
			char line[MAX_LINE_LENGTH];
			size_t lineLength;

		public:
			EditorLine() : lineLength(0)
			{
				line[0] = 0;
			}

			char* Data()
			{
				return line;
			}

			void SetLength(size_t length)
			{
				lineLength = length;
				line[length] = 0;
			}

			operator substring_ref() const
			{
				return { line, line + lineLength };
			}

			cstr begin() const
			{
				return line;
			}

			cstr end() const
			{
				return line + lineLength;
			}
		};

		struct EditorCursor
		{
			int64 caretPos = 0;
			size_t lineNumber = 0;
			int64 lineStartPosition = 0;
			int64 column;

			ptrdiff_t CaretPos() const
			{
				return caretPos;
			}

			size_t LineNumber() const
			{
				return lineNumber;
			}

			ptrdiff_t LineStartPosition() const
			{
				return lineStartPosition;
			}

			ptrdiff_t ColumnNumber() const
			{
				return column;
			}
		};

		ROCOCOAPI IAutoCompleteBuilder
		{
			virtual void AddItem(cstr item) = 0;
			virtual void ShowAndClearItems() = 0;
		};

		ROCOCOAPI ISexyEditor
		{
			// Return the length of the document
			virtual int64 GetDocLength() const = 0;

			// Retrieve the first len-1 chars in the document and null terminate it 
			virtual int64 GetText(int64 len, char* buffer) = 0;

			// Get the caret position of the editor
			virtual int64 GetCaretPos() const = 0;

			// Show call tip at caret position
			virtual void ShowCallTipAtCaretPos(cstr tip) const = 0;

			// Make autocomplete disappear when the caret moved
			virtual void SetAutoCompleteCancelWhenCaretMoved() = 0;

			// Show the autocomplete list
			virtual void ShowAutoCompleteList(cstr spaceSeparatedItems) = 0;

			// Get cursor metrics
			virtual void GetCursor(EditorCursor& cursor) const = 0;

			// Get the current line being edited
			virtual bool TryGetCurrentLine(EditorLine& line) const = 0;

			// Remove text and startPos,endPos and replace with the null terminated item text
			virtual void ReplaceText(int64 startPos, int64 endPos, cstr item) const = 0;

			// Get the builder
			virtual IAutoCompleteBuilder& AutoCompleteBuilder() = 0;
		};
	}
}