#pragma once

namespace Rococo
{
	namespace AutoComplete
	{
		class EditorLine
		{
		public:
			size_t MAX_LINE_LENGTH;

		private:    
			char* line;
			size_t lineLength;

		public:
			EditorLine(char* _line, size_t sizeofLine) : line(_line), lineLength(0), MAX_LINE_LENGTH(sizeofLine)
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

			operator Strings::cr_substring() const
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
			int64 caretPos = 0; // zero based index of the next write position in the text editor
			size_t lineNumber = 0; // zero based index of the line of the next write position
			int64 lineStartPosition = 0; // zero based index of the number of characters from the beginning of the file to the start of the line
			int64 caretColumn; // zero based index of the write position relative to the start of the line in the text editor

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

			ptrdiff_t CaretColumnNumber() const
			{
				return caretColumn;
			}
		};

		ROCOCO_INTERFACE IAutoCompleteBuilder
		{
			virtual void AddHint(Strings::cr_substring item) = 0;
			virtual void AddItem(cstr item) = 0;
			virtual void ShowAndClearItems() = 0;
		};

		// Every editor that works with SexyStudio will implement this interface.
		// An example is in the SexyStudio for Notepad++ module.
		ROCOCO_INTERFACE ISexyEditor
		{
			// Return the length of the document
			virtual int64 GetDocLength() const = 0;

			// Retrieve the first len-1 chars in the document and null terminate it 
			virtual int64 GetText(int64 len, char* buffer) = 0;

			// Get the caret position of the editor
			virtual int64 GetCaretPos() const = 0;

			// Tells the editor to hilight the file path and line number. The interpretation of 'hilight' is up to the implementation on the editor
			virtual void GotoDefinition(const char* searchToken, const char* path, int lineNumber) = 0;

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