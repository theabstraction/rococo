// xc2sexml: converts BennyHill config files (*.xc, *.config) to SEXML config files (<name>.sexml, e.g. config.xc.sexml)
//
// Usage: dotnet run --project build/tools/xc2sexml -- [--check] <file-or-directory>...
//   A directory is searched recursively for *.xc files. Other config files, such as api.config, must be named explicitly.
//   --check converts in memory and fails if the existing .sexml file is missing or differs from the conversion. Nothing is written.
//
// Every conversion is verified before it is written: a model of the input is built from the xc parse tree, and a second model is built
// independently by parsing the generated SEXML text. The two models must be identical. The models hold every config item in order, with
// its directive, its values and the token tree of every defstruct field.
//
// The s-expression parser below mirrors the Sexy parser (sexy.s-parser.s-block.cpp) including the way comments are attached: every comment is
// attached to the next compound expression that opens after it, and comments after the final compound are dropped. Config comments are kept
// as // comments in the SEXML output, on the lines before the directive they precede.
//
// The schema of the output is documented in source/rococo/sexy/sexy.bennyhill/README.sexml.txt
//
// BennyHill generator files (*.sxh) are not converted; they remain in the sxh format.

using System.Globalization;
using System.Text;

namespace Rococo.Tools.Xc2Sexml
{
	public enum NodeType { Atomic, StringLiteral, Compound }

	public sealed class Comment
	{
		public bool IsBlock;
		public string Text = "";
		public int Start; // Offset of the leading '/'
		public int End;   // Offset one past the end of the comment text, excluding the terminating newline or */

		public string ToSource() => IsBlock ? "/*" + Text + "*/" : "//" + Text;
	}

	public sealed class Node
	{
		public NodeType Type;
		public int Start; // Offset of the first character: '(' for compounds, '"' for string literals
		public int End;   // Offset one past the last character: past ')' for compounds, past '"' for string literals
		public string Text = ""; // The atomic token, or the string literal with its escape sequences resolved
		public readonly List<Node> Children = new();
		public List<Comment> Comments = new(); // Comments attached to this compound
		public Node? Parent;

		public int Count => Children.Count;
		public Node this[int index] => Children[index];
		public bool IsAtomic => Type == NodeType.Atomic;
		public bool IsString => Type == NodeType.StringLiteral;
		public bool IsCompound => Type == NodeType.Compound;
		public bool Is(string atomic) => IsAtomic && Text == atomic;
	}

	public sealed class ConversionException : Exception
	{
		public ConversionException(string message) : base(message) { }
	}

	// Mirrors SParser in sexy.s-parser.s-block.cpp, with the string escapes of ParseSexyString<'&'>
	public sealed class SexyParser
	{
		private readonly string src;
		private readonly int end;
		private List<Comment> pending = new();

		public readonly Node Root;
		public readonly List<Comment> AllComments = new();
		public List<Comment> Orphans => pending; // Comments after the final compound, which the Sexy parser drops

		public SexyParser(string source)
		{
			src = source;
			end = source.Length;
			Root = new Node { Type = NodeType.Compound, Start = 0, End = end };
			int next = ParseCompound(0, 0, Root);
			if (next != end) throw Error(next, "Too many close parenthesis characters");
		}

		public string Position(int offset) => Util.Position(src, offset);

		private ConversionException Error(int offset, string message) => new ConversionException($"({Position(offset)}) {message}");

		private static bool IsBlank(char c) => c <= ' ';

		private int ParseCompound(int next, int depth, Node parent)
		{
			while (true)
			{
				if (next == end) break;

				while (next != end && IsBlank(src[next])) next++;

				if (next == end) break;

				char c = src[next++];

				switch (c)
				{
					case '(':
						{
							var child = new Node { Type = NodeType.Compound, Start = next - 1, Parent = parent, Comments = pending };
							pending = new List<Comment>();
							parent.Children.Add(child);
							next = ParseCompound(next, depth + 1, child);
						}
						break;
					case ')':
						if (depth == 0) throw Error(next - 1, "Unexpected close parenthesis at the root expression");
						parent.End = next;
						return next;
					case '"':
						next = ParseString(next, parent);
						break;
					case '/':
						if (next < end && src[next] == '/')
						{
							next = ParseLineComment(next + 1);
						}
						else if (next < end && src[next] == '*')
						{
							next = ParseBlockComment(next + 1);
						}
						else
						{
							next = ParseAtomic(next - 1, parent);
						}
						break;
					default:
						next = ParseAtomic(next - 1, parent);
						break;
				}
			}

			if (depth != 0) throw Error(next, "Missing close parenthesis character before end of s-expression");

			return next;
		}

		private int ParseAtomic(int start, Node parent)
		{
			int next = start;

			while (true)
			{
				if (next == end)
				{
					AddAtomic(start, next, parent);
					return end;
				}

				char c = src[next];

				if (IsBlank(c))
				{
					AddAtomic(start, next, parent);
					return next + 1;
				}

				if (c == '(' || c == ')')
				{
					AddAtomic(start, next, parent);
					return next;
				}

				next++;
			}
		}

		private void AddAtomic(int start, int finish, Node parent)
		{
			parent.Children.Add(new Node { Type = NodeType.Atomic, Start = start, End = finish, Text = src.Substring(start, finish - start), Parent = parent });
		}

		private int ParseString(int start, Node parent)
		{
			var sb = new StringBuilder();
			int p = start;

			while (true)
			{
				while (p != end)
				{
					char c = src[p++];

					if (c == '&')
					{
						break;
					}
					else if (c == '"')
					{
						parent.Children.Add(new Node { Type = NodeType.StringLiteral, Start = start - 1, End = p, Text = sb.ToString(), Parent = parent });
						return p;
					}
					else
					{
						sb.Append(c);
					}
				}

				if (p >= end) throw Error(start - 1, "Unexpected end of string literal");

				switch (src[p])
				{
					case '&':
					case 'e': sb.Append('&'); p++; break;
					case 'a': sb.Append('\a'); p++; break;
					case 'b': sb.Append('\b'); p++; break;
					case 'f': sb.Append('\f'); p++; break;
					case 'r': sb.Append('\r'); p++; break;
					case 'n': sb.Append('\n'); p++; break;
					case 't': sb.Append('\t'); p++; break;
					case 'v': sb.Append('\v'); p++; break;
					case '\'': sb.Append('\''); p++; break;
					case '"': sb.Append('"'); p++; break;
					case 'x':
						{
							if (p + 3 > end) throw Error(p, "Unexpected end of escape sequence");
							int high = HexValue(src[p + 1], p + 1);
							int low = HexValue(src[p + 2], p + 2);
							sb.Append((char)((high << 4) + low));
							p += 3;
							if (p >= end) throw Error(p, "Unexpected end of escape sequence");
						}
						break;
					default:
						// The Sexy parser ignores unknown escapes. The character after & is then read as an ordinary character
						break;
				}
			}
		}

		private int HexValue(char c, int offset)
		{
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			throw Error(offset, "Bad hex value");
		}

		private int ParseLineComment(int start)
		{
			int s = start;
			while (s != end)
			{
				if (src[s] == '\r' || src[s] == '\n')
				{
					AddComment(false, start, s);
					return s + 1;
				}
				s++;
			}

			// The Sexy parser does not record a line comment that is terminated by the end of the file
			return s;
		}

		private int ParseBlockComment(int start)
		{
			int s = start;
			while (s != end)
			{
				if (src[s] == '*' && s + 1 < end && src[s + 1] == '/')
				{
					AddComment(true, start, s);
					return s + 2;
				}
				s++;
			}

			// The Sexy parser does not record an unterminated block comment
			return s;
		}

		private void AddComment(bool isBlock, int start, int finish)
		{
			var c = new Comment { IsBlock = isBlock, Text = src.Substring(start, finish - start), Start = start - 2, End = finish };
			pending.Add(c);
			AllComments.Add(c);
		}
	}

	public static class Util
	{
		public static string Position(string src, int offset)
		{
			int line = 1, column = 1;
			for (int i = 0; i < offset && i < src.Length; i++)
			{
				if (src[i] == '\n') { line++; column = 1; }
				else if (src[i] != '\r') column++;
			}
			return $"line {line} col {column}";
		}

		// Quotes a string for the models, so that every model line is unambiguous
		public static string Q(string s)
		{
			var sb = new StringBuilder("\"");
			foreach (char c in s)
			{
				switch (c)
				{
					case '\\': sb.Append("\\\\"); break;
					case '"': sb.Append("\\\""); break;
					case '\r': sb.Append("\\r"); break;
					case '\n': sb.Append("\\n"); break;
					default:
						if (c < ' ') sb.Append("\\x").Append(((int)c).ToString("X2", CultureInfo.InvariantCulture));
						else sb.Append(c);
						break;
				}
			}
			return sb.Append('"').ToString();
		}

		// The canonical form of an expression: atomics verbatim, string literals quoted by value, compounds as parenthesised lists
		public static string C(Node n)
		{
			switch (n.Type)
			{
				case NodeType.Atomic: return n.Text;
				case NodeType.StringLiteral: return Q(n.Text);
				default: return "(" + string.Join(" ", n.Children.Select(C)) + ")";
			}
		}

		public static string Docs(IEnumerable<string> items) => "[" + string.Join(",", items.Select(Q)) + "]";

		// Escapes text as a Sexy string literal body, using the & escape character of the Sexy parser
		public static string EscapeSexy(string s)
		{
			var sb = new StringBuilder();
			foreach (char c in s)
			{
				switch (c)
				{
					case '&': sb.Append("&&"); break;
					case '"': sb.Append("&\""); break;
					case '\r': sb.Append("&r"); break;
					case '\n': sb.Append("&n"); break;
					case '\t': sb.Append('\t'); break;
					default:
						if (c < ' ' || c == '\x7F') sb.Append("&x").Append(((int)c).ToString("X2", CultureInfo.InvariantCulture));
						else sb.Append(c);
						break;
				}
			}
			return sb.ToString();
		}

		public static string DetectNewline(string src) => src.Contains("\r\n") ? "\r\n" : "\n";
	}

	// Converts a config file from xc to sexml. Comments and layout outside the config items are kept, with the syntax examples in the comments updated
	public static class ConfigConverter
	{
		public const string Header = "// BennyHill config file in SEXML format. The schema is described in source/rococo/sexy/sexy.bennyhill/README.sexml.txt";

		// Comment text that describes the xc syntax, and its SEXML equivalent
		private static readonly (string from, string to)[] CommentRewrites =
		{
			("one and only one cpp.root entry", "one and only one (CppRoot (Path <cpp-root>)) entry"),
			("replaced with the cpp.root as specified above", "replaced with the CppRoot path as specified above"),
			("The format of the command is (primitive", "The format of the directive is (primitive"),
			("(primitive <name> <sexy-name> <cpp-name>)", "(Primitive (Name <name>) (SexyType <sexy-name>) (CppType <cpp-name>))"),
			("(struct <name> <sexy-name> <cpp-name>)", "(Struct (Name <name>) (SexyType <sexy-name>) (CppType <cpp-name>))"),
			("(defstruct <name> <sexy-name> <cpp-name> (fields) )", "(DefStruct (Name <name>) (SexyType <sexy-name>) (CppType <cpp-name>) (' Fields (fields)))"),
			("the parent defstruct", "the parent DefStruct"),
			("in the target specified in (cpp.types ...)", "in the target specified in (TypeFiles ...)"),
			("// sxy.types gives the location", "// (TypeFiles (SexyTypes <sexy-types-path>) (CppTypes <cpp-types-path>)) gives the location"),
			("// type.files gives the location", "// (TypeFiles (SexyTypes <sexy-types-path>) (CppTypes <cpp-types-path>)) gives the location"),
			("The first argument gives the sexy definition file, the second argument gives the types that are written into the cpp header", "SexyTypes gives the sexy definition file, CppTypes gives the types that are written into the cpp header"),
		};

		public static string Convert(string src)
		{
			string nl = Util.DetectNewline(src);
			var parser = new SexyParser(src);
			var o = new StringBuilder();

			o.Append(Header).Append(nl);

			int pos = 0;
			foreach (var item in parser.Root.Children)
			{
				ConversionException Error(string message) => new ConversionException($"({parser.Position(item.Start)}) {message}");

				if (!item.IsCompound || item.Count == 0 || !item[0].IsAtomic) throw Error("Expecting compound item in config definition");

				o.Append(RewriteComments(src.Substring(pos, item.Start - pos)));
				pos = item.End;

				string Tok(Node n) => src.Substring(n.Start, n.End - n.Start);

				void ExpectAtoms(int count, string form)
				{
					if (item.Count != count) throw Error($"Expecting {form}");
					for (int i = 1; i < count; i++) if (item[i].IsCompound) throw Error($"Expecting {form} with no sub-expressions");
				}

				int innerEnd = item.End;
				string text;

				switch (item[0].Text)
				{
					case "cpp.root":
						ExpectAtoms(2, "(cpp.root <cpp-root-directory>)");
						text = $"(CppRoot (Path {Tok(item[1])}))";
						break;
					case "cpp.exception":
						ExpectAtoms(2, "(cpp.exception <cpp-exception-name>)");
						text = $"(CppException (Type {Tok(item[1])}))";
						break;
					case "type.files":
						ExpectAtoms(3, "(type.files <sexy-types> <cpp-types>)");
						text = $"(TypeFiles (SexyTypes {Tok(item[1])}) (CppTypes {Tok(item[2])}))";
						break;
					case "primitive":
					case "struct":
					case "interface":
						{
							ExpectAtoms(4, $"({item[0].Text} <sxh-type> <sxy-type> <cpp-type>)");
							string name = item[0].Text switch { "primitive" => "Primitive", "struct" => "Struct", _ => "Interface" };
							text = $"({name} (Name {Tok(item[1])}) (SexyType {Tok(item[2])}) (CppType {Tok(item[3])}))";
						}
						break;
					case "defstruct":
						{
							if (item.Count < 5 || !item[1].IsAtomic || !item[2].IsAtomic || !item[3].IsAtomic) throw Error("Expecting (defstruct <sxh-type> <sxy-type> <cpp-type> (field1)...(fieldN))");
							for (int i = 4; i < item.Count; i++) if (!item[i].IsCompound) throw Error("Expecting compound fields (<type> <name>)");

							// The fields keep their layout and comments. The raw attribute value is the compound of fields
							int fieldsStart = item[3].End;
							int fieldsEnd = item.End - 1;
							text = $"(DefStruct (Name {Tok(item[1])}) (SexyType {Tok(item[2])}) (CppType {Tok(item[3])}) (' Fields ({src.Substring(fieldsStart, fieldsEnd - fieldsStart)})))";
							innerEnd = item[3].End;
						}
						break;
					default:
						throw Error($"Unknown config item '{item[0].Text}'. Expecting one of cpp.root, cpp.exception, type.files, primitive, struct, interface or defstruct");
				}

				// Comments inside rewritten items would be lost
				foreach (var c in parser.AllComments)
				{
					if (c.Start > item.Start && c.End <= innerEnd) throw new ConversionException($"({parser.Position(c.Start)}) The converter has no place for the comment {c.ToSource()} inside a config item");
				}

				o.Append(text);
			}

			o.Append(RewriteComments(src.Substring(pos)));

			return o.ToString();
		}

		private static string RewriteComments(string gap)
		{
			foreach (var (from, to) in CommentRewrites)
			{
				gap = gap.Replace(from, to, StringComparison.Ordinal);
			}
			return gap;
		}
	}

	// Builds the models that are compared to verify a conversion. The xc model is built from the input parse tree.
	// The sexml models are built from the generated text, interpreting the SEXML directive syntax independently of the converters.
	public static class Models
	{
		private static string F(Node? n) => n == null ? "-" : Util.C(n);

		public static List<string> FromXc(string src)
		{
			var parser = new SexyParser(src);
			var model = new List<string>();

			foreach (var item in parser.Root.Children)
			{
				switch (item[0].Text)
				{
					case "cpp.root": model.Add("CppRoot " + F(item[1])); break;
					case "cpp.exception": model.Add("CppException " + F(item[1])); break;
					case "type.files": model.Add($"TypeFiles {F(item[1])} {F(item[2])}"); break;
					case "primitive": model.Add($"Primitive {F(item[1])} {F(item[2])} {F(item[3])}"); break;
					case "struct": model.Add($"Struct {F(item[1])} {F(item[2])} {F(item[3])}"); break;
					case "interface": model.Add($"Interface {F(item[1])} {F(item[2])} {F(item[3])}"); break;
					case "defstruct": model.Add($"DefStruct {F(item[1])} {F(item[2])} {F(item[3])} fields={string.Join(" ", item.Children.Skip(4).Select(Util.C))}"); break;
				}
			}

			return model;
		}

		// A SEXML directive: (<FQName> <attribute>... [: <sub-directive>...])
		private sealed class Directive
		{
			public string Name = "";
			public Node S = null!;
			public readonly List<(string name, Node s)> Attributes = new();
			public readonly List<Directive> Children = new();

			public Node? Find(string name) => Attributes.FirstOrDefault(a => a.name == name).s;

			// The value of (<name> <value>)
			public Node? Value(string name)
			{
				var a = Find(name);
				if (a == null) return null;
				if (a.Count != 2 || a[1].IsCompound) throw new ConversionException($"Expecting ({name} <atomic-or-string>) in ({Name} ...)");
				return a[1];
			}

			// The raw compound of (' <name> <raw>)
			public Node Raw(string name)
			{
				var a = Find(name) ?? throw new ConversionException($"Missing (' {name} ...) in ({Name} ...)");
				if (!a[0].Is("'") || a.Count != 3) throw new ConversionException($"Expecting (' {name} <raw>) in ({Name} ...)");
				return a[2];
			}

			// The string items of ([] <name> "item"...)
			public List<string> Doc()
			{
				var a = Find("Doc");
				if (a == null) return new List<string>();
				if (!a[0].Is("[]")) throw new ConversionException($"Expecting ([] Doc ...) in ({Name} ...)");
				return a.Children.Skip(2).Select(item => item.IsString ? item.Text : throw new ConversionException("Doc items must be string literals")).ToList();
			}

			public void Allow(params string[] names)
			{
				foreach (var a in Attributes)
				{
					if (!names.Contains(a.name)) throw new ConversionException($"Unexpected attribute {a.name} in ({Name} ...)");
				}
			}

			public bool Has(string name) => Find(name) != null;
		}

		private static Directive ParseDirective(Node s)
		{
			if (!s.IsCompound || s.Count == 0 || !s[0].IsAtomic) throw new ConversionException("Expecting a directive (<FQName> ...)");

			var d = new Directive { Name = s[0].Text, S = s };

			foreach (var segment in d.Name.Split('.'))
			{
				if (segment.Length == 0 || !char.IsAsciiLetterUpper(segment[0]) || !segment.All(char.IsAsciiLetterOrDigit)) throw new ConversionException($"Bad directive name {d.Name}");
			}

			int i = 1;
			for (; i < s.Count; i++)
			{
				var a = s[i];
				if (a.Is(":")) { i++; break; }
				if (!a.IsCompound || a.Count == 0 || !a[0].IsAtomic) throw new ConversionException($"Bad attribute in ({d.Name} ...)");

				int namePos = (a[0].Is("'") || a[0].Is("[]")) ? 1 : 0;
				if (namePos >= a.Count || !a[namePos].IsAtomic || !char.IsAsciiLetter(a[namePos].Text[0])) throw new ConversionException($"Bad attribute name in ({d.Name} ...)");

				string name = a[namePos].Text;
				if (d.Has(name)) throw new ConversionException($"Duplicate attribute {name} in ({d.Name} ...)");
				d.Attributes.Add((name, a));
			}

			for (; i < s.Count; i++)
			{
				d.Children.Add(ParseDirective(s[i]));
			}

			return d;
		}

		private static void NoChildren(Directive d)
		{
			if (d.Children.Count > 0) throw new ConversionException($"({d.Name} ...) does not take sub-directives");
		}

		public static List<string> FromConfigSexml(string sexml)
		{
			var parser = new SexyParser(sexml);
			var model = new List<string>();

			foreach (var s in parser.Root.Children)
			{
				var d = ParseDirective(s);
				NoChildren(d);
				switch (d.Name)
				{
					case "CppRoot": d.Allow("Path"); model.Add("CppRoot " + F(d.Value("Path"))); break;
					case "CppException": d.Allow("Type"); model.Add("CppException " + F(d.Value("Type"))); break;
					case "TypeFiles": d.Allow("SexyTypes", "CppTypes"); model.Add($"TypeFiles {F(d.Value("SexyTypes"))} {F(d.Value("CppTypes"))}"); break;
					case "Primitive":
					case "Struct":
					case "Interface":
						d.Allow("Name", "SexyType", "CppType");
						model.Add($"{d.Name} {F(d.Value("Name"))} {F(d.Value("SexyType"))} {F(d.Value("CppType"))}");
						break;
					case "DefStruct":
						d.Allow("Name", "SexyType", "CppType", "Fields");
						model.Add($"DefStruct {F(d.Value("Name"))} {F(d.Value("SexyType"))} {F(d.Value("CppType"))} fields={string.Join(" ", d.Raw("Fields").Children.Select(Util.C))}");
						break;
					default:
						throw new ConversionException($"Unexpected directive ({d.Name} ...)");
				}
			}

			return model;
		}
	}

	public static class Program
	{
		private static void PrintUsage()
		{
			Console.WriteLine("Usage: xc2sexml [--check] <file-or-directory>...");
			Console.WriteLine("  <name>.xc and <name>.config are converted to <name>.xc.sexml and <name>.config.sexml");
			Console.WriteLine("  Directories are searched recursively for *.xc files");
			Console.WriteLine("  --check  verify that the existing .sexml files match the conversion, writing nothing");
		}

		public static int Main(string[] args)
		{
			bool check = false;
			var inputs = new List<string>();

			foreach (var arg in args)
			{
				if (arg == "--check") check = true;
				else if (arg == "-h" || arg == "--help") { PrintUsage(); return 0; }
				else if (arg.StartsWith("--", StringComparison.Ordinal)) { Console.Error.WriteLine($"Unknown option {arg}"); PrintUsage(); return 2; }
				else inputs.Add(arg);
			}

			if (inputs.Count == 0)
			{
				PrintUsage();
				return 2;
			}

			var files = new List<string>();
			foreach (var input in inputs)
			{
				if (Directory.Exists(input))
				{
					files.AddRange(Directory.EnumerateFiles(input, "*.xc", SearchOption.AllDirectories).OrderBy(f => f, StringComparer.Ordinal));
				}
				else
				{
					files.Add(input);
				}
			}

			int failures = 0;

			foreach (var file in files)
			{
				try
				{
					Console.WriteLine(ConvertFile(file, check));
				}
				catch (Exception ex) when (ex is ConversionException || ex is IOException || ex is UnauthorizedAccessException)
				{
					Console.Error.WriteLine($"{file}: error: {ex.Message}");
					failures++;
				}
			}

			Console.WriteLine($"{files.Count - failures} of {files.Count} file(s) {(check ? "up to date" : "converted")}");
			return failures == 0 ? 0 : 1;
		}

		private static string ConvertFile(string file, bool check)
		{
			string extension = Path.GetExtension(file).ToLowerInvariant();
			if (extension != ".xc" && extension != ".config")
			{
				throw new ConversionException("Expecting a .xc or .config file");
			}

			string output = file + ".sexml";

			// Latin1 maps every byte to one char and back, so the bytes of the input are preserved exactly
			string src = Encoding.Latin1.GetString(File.ReadAllBytes(file));

			string sexml = ConfigConverter.Convert(src);

			var expected = Models.FromXc(src);
			List<string> actual;
			try
			{
				actual = Models.FromConfigSexml(sexml);
			}
			catch (ConversionException ex)
			{
				throw new ConversionException("The generated SEXML could not be read back: " + ex.Message);
			}

			for (int i = 0; i < Math.Max(expected.Count, actual.Count); i++)
			{
				string e = i < expected.Count ? expected[i] : "<none>";
				string a = i < actual.Count ? actual[i] : "<none>";
				if (e != a) throw new ConversionException($"Round trip check failed at model line {i + 1}:\n  input:  {e}\n  sexml:  {a}");
			}

			if (check)
			{
				if (!File.Exists(output)) throw new ConversionException($"{output} does not exist");
				if (Encoding.Latin1.GetString(File.ReadAllBytes(output)) != sexml) throw new ConversionException($"{output} differs from the conversion of {file}");
				return $"{output}: up to date ({expected.Count} model lines verified)";
			}

			File.WriteAllBytes(output, Encoding.Latin1.GetBytes(sexml));
			return $"{output}: written ({expected.Count} model lines verified)";
		}
	}
}
