using System;
using System.Collections.Generic;
using System.Text;

namespace Rococo
{
    public class SexParser
    {
        public SExpressionTree LoadAndParseAsciiTree(string filename)
        {
            byte[] buffer = System.IO.File.ReadAllBytes(filename);
            return ParseAsciiTree(filename, buffer);
        }

        public SExpressionTree ParseAsciiTree(string name, byte[] buffer)
        {
            SExpressionBuilder builder = new SExpressionBuilder(buffer, name);
            return builder.MakeTree();
        }
        public SExpressionTree ParseAsciiTree(string filename, string s)
        {
            byte[] buffer = new byte[s.Length];
            for(int i = 0; i < s.Length; ++i)
            {
                char c = s[i];
                if (c >= 128)
                {
                    throw new Exception(string.Format("Character in file at position {0} was not ascii", i));
                }
                buffer[i] = (byte) c;
            }

            return ParseAsciiTree(filename, buffer);
        }
    }

    enum SexReadState
    {
        Whitespace,
        Atomic,
        StringLiteral,
        StringLiteralEscape,
        StringLiteralEscapedHexLow,
        StringLiteralEscapedHexHigh,
        OpenSlash,
        CStyleComment,
        CStyleCommentEnd,
        CPPStyleComment
    }
    public enum SexType
    {
        Null,
        Atomic,
        Compound,
        StringLiteral
    }
    public struct CodePosition
    {
        private int row;
        private int column;

        public CodePosition(int row, int column)
        {
            this.row = row;
            this.column = column;
        }

        public int Row
        {
            get
            {
                return row;
            }
        }

        public int Column
        {
            get
            {
                return column;
            }
        }
    }

    public interface ISExpression
    {
        public ISExpression Root { get; }
        public ISExpression Parent { get; }
        public SexType Type { get; }
        public CodePosition Start { get; }
        public CodePosition End { get; }
        public string Text { get; }
        public int NumberOfChildren { get; }
        public ISExpression this [int index] { get;}
    }

    class SExpression: ISExpression
    {
        public SExpression parent;
        public SExpressionTree tree;
        public int xStartPos;
        public int yStartPos;
        public int xEndPos;
        public int yEndPos;
        public List<SExpression> children;
        public string text;
        public SexType type;

        public ISExpression Root
        {
            get
            {
                return tree.Root;
            }
        }
        public ISExpression Parent
        {
            get
            {
                return parent;
            }
        }
        public SexType Type
        { 
            get
            {
                return type;
            }
        }

        public CodePosition Start
        {
            get
            {
                return new CodePosition(yStartPos, xStartPos);
            }
        }
        public CodePosition End
        {
            get
            {
                return new CodePosition(yEndPos, xEndPos);
            }
        }
        public string Text 
        { 
            get
            {
                return text == null ? string.Empty : text;
            }
        }
        public int NumberOfChildren
        {
            get
            {
                return children == null ? 0 : children.Count;
            }
        }
        public ISExpression this[int index]
        { 
            get
            {
                return children[index];
            }
        }
    }

    public class SExpressionBuilder
    {
        StringBuilder sb = new StringBuilder(64);
        byte[] buffer;
        string name;
        SExpression root = new SExpression() { parent = null, xStartPos = 1, yStartPos = 1 };
        SExpression current;
        int xPos = 1;
        int yPos = 1;
        SexReadState state;
        int xStringPos;
        int yStringPos;
        SExpressionTree tree;
        int hexHigh;
        int hexLow;

        public SExpressionBuilder(byte[] buffer, string name)
        {
            this.buffer = buffer;
            this.current = root;
            this.name = name;
            tree = new SExpressionTree(root);
            root.tree = tree;
        }
        private void ReadAtomic(byte c)
        {
            switch (c)
            {
                case (byte)'\r':
                    CompleteAtomic(xPos - 1, yPos);
                    xPos++;
                    state = SexReadState.Whitespace;
                    return;
                case (byte)'\n':
                    xPos = 1;
                    yPos++;
                    CompleteAtomic(xPos, yPos);
                    state = SexReadState.Whitespace;
                    return;
                case (byte)' ':
                    CompleteAtomic(xPos - 1, yPos);
                    xPos++;
                    state = SexReadState.Whitespace;
                    return;
                case (byte)'(':
                    CompleteAtomic(xPos-1, yPos);
                    xPos++;
                    OpenExpression(xPos, yPos);
                    state = SexReadState.Whitespace;
                    return;
                case (byte)')':
                    CompleteAtomic(xPos - 1, yPos);
                    xPos++;
                    CloseExpression(xPos, yPos);
                    state = SexReadState.Whitespace;
                    return;
                default:
                    xPos++;
                    sb.Append((char)c);
                    return;
            }
        }
        private void ReadCStyleComment(byte c)
        {
            switch (c)
            {
                case (byte)'\n':
                    xPos = 1;
                    yPos++;
                    return;
                case (byte)'*':
                    xPos++;
                    state = SexReadState.CStyleCommentEnd;
                    return;
                default:
                    xPos++;
                    return;
            }
        }
        private void ReadCStyleCommentEnd(byte c)
        {
            switch (c)
            {
                case (byte)'\n':
                    xPos = 1;
                    yPos++;
                    state = SexReadState.CStyleComment;
                    return;
                case (byte)'/':
                    xPos++;
                    state = SexReadState.Whitespace;
                    return;
                default:
                    xPos++;
                    return;
            }
        }
        private void ReadCPPStyleComment(byte c)
        {
            switch (c)
            {
                case (byte)'\n':
                    xPos = 1;
                    yPos++;
                    state = SexReadState.Whitespace;
                    return;
                default:
                    xPos++;
                    return;
            }
        }
        private void ReadOpenSlash(byte c)
        {
            switch(c)
            {
                case (byte)'\r':
                    sb.Append('/');
                    CompleteAtomic(xPos - 1, yPos);
                    xPos++;
                    state = SexReadState.Whitespace;
                    return;
                case (byte)'\n':
                    sb.Append('/');
                    CompleteAtomic(xPos - 1, yPos);
                    state = SexReadState.Whitespace;
                    xPos = 1;
                    yPos++;
                    return;
                case (byte)' ':
                    sb.Append('/');
                    CompleteAtomic(xPos - 1, yPos);
                    state = SexReadState.Whitespace;
                    xPos++;
                    return;
                case (byte)'(':
                    sb.Append('/');
                    CompleteAtomic(xPos - 1, yPos);
                    state = SexReadState.Whitespace;
                    xPos++;
                    OpenExpression(xPos++, yPos);
                    return;
                case (byte)')':
                    sb.Append('/');
                    CompleteAtomic(xPos - 1, yPos);
                    state = SexReadState.Whitespace;
                    xPos++;
                    CloseExpression(xPos++, yPos);
                    return;
                case (byte) '/':
                    xPos++;
                    state = SexReadState.CPPStyleComment;
                    return;
                case (byte)'*':
                    xPos++;
                    state = SexReadState.CStyleComment;
                    return;
                default:
                    xPos++;
                    xStringPos = xPos-2;
                    yStringPos = yPos;
                    state = SexReadState.Atomic;
                    sb.Append('/');
                    sb.Append((char)c);
                    return;
            }
        }
        private void ReadString(byte c)
        {
            switch (c)
            {
                case (byte)'\n':
                    xPos = 1;
                    yPos++;
                    sb.Append((char)c);
                    return;
                case (byte)'"':
                    CompleteStringLiteral(xPos++, yPos);
                    state = SexReadState.Whitespace;
                    return;
                case (byte)'&':
                    xPos++;
                    state = SexReadState.StringLiteralEscape;
                    return;
                default:
                    xPos++;
                    sb.Append((char)c);
                    return;
            }
        }
        private void ReadStringEscapeSequence(byte c)
        {
            switch (c)
            {
                case (byte)'r':
                    xPos++;
                    sb.Append('\r');
                    state = SexReadState.StringLiteral;
                    return;
                case (byte)'n':
                    xPos++;
                    sb.Append('\n');
                    state = SexReadState.StringLiteral;
                    return;
                case (byte)'t':
                    xPos++;
                    sb.Append('\t');
                    state = SexReadState.StringLiteral;
                    return;
                case (byte)'&':
                    xPos++;
                    sb.Append('&');
                    state = SexReadState.StringLiteral;
                    return;
                case (byte)'f':
                    xPos++;
                    sb.Append('\f');
                    state = SexReadState.StringLiteral;
                    return;
                case (byte)'v':
                    xPos++;
                    sb.Append('\v');
                    state = SexReadState.StringLiteral;
                    return;
                case (byte)'a':
                    xPos++;
                    sb.Append('\a');
                    state = SexReadState.StringLiteral;
                    return;
                case (byte)'b':
                    xPos++;
                    sb.Append('\b');
                    state = SexReadState.StringLiteral;
                    return;
                case (byte)'x':
                    xPos++;
                    state = SexReadState.StringLiteralEscapedHexHigh;
                    return;
                default:
                    xPos++;
                    sb.Append((char)c);
                    return;
            }
        }
        int FromHex(char c)
        {
            if (c >= '0' && c <= '9')
            {
                return c - '0';
            }
            else if (c >= 'A' && c <= 'F')
            {
                return 10 + c - 'A';
            }
            else if (c >= 'a' && c <= 'f')
            {
                return 10 + c - 'a';
            }
            return -1;
        }

        private void ReadStringEscapedHexHigh(byte c)
        {
            xPos++;
            hexHigh = FromHex((char)c);
            if (hexHigh == -1)
            {
                throw new Exception("Expecting hexademical character in string literal");
            }

            state = SexReadState.StringLiteralEscapedHexLow;
        }
        private void ReadStringEscapedHexLow(byte c)
        {
            xPos++;
            hexLow = FromHex((char)c);
            if (hexLow == -1)
            {
                throw new Exception("Expecting hexademical character in string literal");
            }

            sb.Append((char)(hexHigh * 16 + hexLow));
            state = SexReadState.StringLiteral;
        }
        private void ReadWhitespace(byte c)
        {
            switch(c)
            {
                case (byte)'\r':
                    xPos ++;
                    return;
                case (byte)'\n':
                    xPos = 1;
                    yPos++;
                    return;
                case (byte)' ':
                    xPos++;
                    return;
                case (byte)'(':
                    OpenExpression(xPos++, yPos);
                    return;
                case (byte)')':
                    CloseExpression(xPos++, yPos);
                    return;
                case (byte)'"':
                    xStringPos = xPos++;
                    yStringPos = yPos;
                    state = SexReadState.StringLiteral;
                    return;
                case (byte)'/':
                    xPos++;
                    state = SexReadState.OpenSlash;
                    return;
                default:
                    xStringPos = xPos++;
                    yStringPos = yPos;
                    state = SexReadState.Atomic;
                    sb.Append((char)c);
                    return;
            }
        }
        private void OpenExpression(int x, int y)
        {
            var s = new SExpression()
            {
                parent = current,
                xStartPos = x,
                yStartPos = y,
            };

            s.tree = tree;

            if (current.children == null)
            {
                current.type = SexType.Compound;
                current.children = new List<SExpression>();
            }

            current.children.Add(s);
            current = s;
        }

        private void CloseExpression(int x, int y)
        {
            if (current == root)
            {
                throw new Exception(string.Format("Too many close parenthesis characters ')' at line {0} column {1} ", y, x));
            }

            current.xEndPos = x;
            current.yEndPos = y;
            current.type = current.NumberOfChildren > 0 ? SexType.Compound : SexType.Null;
            current = current.parent;
        }
        private void CompleteAtomic(int x, int y)
        {
            var atomic = new SExpression()
            {
                parent = current,
                xStartPos = xStringPos,
                yStartPos = yStringPos,
                xEndPos = x,
                yEndPos = y,
                text = sb.ToString(),
                type = SexType.Atomic
            };

            atomic.tree = tree;

            if (current.children == null)
            {
                current.type = SexType.Compound;
                current.children = new List<SExpression>();
            }

            current.children.Add(atomic);

            sb.Clear();
        }
        private void CompleteStringLiteral(int x, int y)
        {
            var literal = new SExpression()
            {
                parent = current,
                xStartPos = xStringPos,
                yStartPos = yStringPos,
                xEndPos = x,
                yEndPos = y,
                text = sb.ToString(),
                type = SexType.StringLiteral
            };

            literal.tree = tree;

            if (current.children == null)
            {
                current.type = SexType.Compound;
                current.children = new List<SExpression>();
            }

            current.children.Add(literal);

            sb.Clear();
        }
        public SExpressionTree MakeTree()
        {
            state = SexReadState.Whitespace;

            foreach (byte c in buffer)
            {
                switch(state)
                {
                    case SexReadState.Whitespace:
                        ReadWhitespace(c);
                        break;
                    case SexReadState.Atomic:
                        ReadAtomic(c);
                        break;
                    case SexReadState.StringLiteral:
                        ReadString(c);
                        break;
                    case SexReadState.OpenSlash:
                        ReadOpenSlash(c);
                        break;
                    case SexReadState.CStyleComment:
                        ReadCStyleComment(c);
                        break;
                    case SexReadState.CStyleCommentEnd:
                        ReadCStyleCommentEnd(c);
                        break;
                    case SexReadState.CPPStyleComment:
                        ReadCPPStyleComment(c);
                        break;
                    case SexReadState.StringLiteralEscape:
                        ReadStringEscapeSequence(c);
                        break;
                    case SexReadState.StringLiteralEscapedHexHigh:
                        ReadStringEscapedHexHigh(c);
                        break;
                    case SexReadState.StringLiteralEscapedHexLow:
                        ReadStringEscapedHexLow(c);
                        break;
                }
            }

            switch (state)
            {
                case SexReadState.Atomic:
                    CompleteAtomic(xPos-1, yPos);
                    break;
                case SexReadState.Whitespace:
                case SexReadState.StringLiteral:
                case SexReadState.CPPStyleComment:
                    break;
                    throw new Exception(string.Format("String literal was not properly terminated at row {0} column {1}", yPos, xPos));
                default:
                    throw new Exception("Improperly termination to root expression definition");
            }

            if (current != root)
            {
                throw new Exception(string.Format("Missing close parenthesis characters at row {0} column {1}", yPos, xPos));
            }

            root.xStartPos = 1;
            root.yStartPos = 1;
            root.xEndPos = xPos;
            root.yEndPos = yPos;

            return new SExpressionTree(root); 
        }
    }

    public class SExpressionTree
    {
        private SExpression root;

        public ISExpression Root
        {
            get { return root; }
        }
        internal SExpressionTree(SExpression root)
        {
            this.root = root;
        }
    }
}
