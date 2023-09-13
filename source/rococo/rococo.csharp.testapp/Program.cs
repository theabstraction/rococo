using System;
using Rococo;

namespace Rococo
{
    class Program
    {
        static void Validate(bool value)
        {
            if (!value)
            {
                throw new Exception("Validation failed");
            }
        }
        static void TestAtomic()
        {
            var parser = new SexParser();
            var tree = parser.ParseAsciiTree("TestAtomic", "\n Dogmatic");
            Validate(tree.Root != null);
            Validate(tree.Root.Type == SexType.Compound);
            Validate(tree.Root.Text.Length == 0);
            Validate(tree.Root.NumberOfChildren == 1);
            Validate(tree.Root.Parent == null);
            var child = tree.Root[0];
            Validate(child != null);
            Validate(child.Root == tree.Root);
            Validate(child.Parent == tree.Root);
            Validate(child.Text == "Dogmatic");
            Validate(child.Type == SexType.Atomic);
            Validate(child.NumberOfChildren == 0);
            Validate(child.Start.Row == 2);
            Validate(child.Start.Column == 2);
            Validate(child.End.Row == 2);
            Validate(child.End.Column == 9);
        }
        static void TestCstyleComment()
        {
            var parser = new SexParser();
            var tree = parser.ParseAsciiTree("TestCstyleComment", "/* not a cat */ Dog // phonecall");
            Validate(tree.Root != null);
            Validate(tree.Root.Type == SexType.Compound);
            Validate(tree.Root.Text.Length == 0);
            Validate(tree.Root.NumberOfChildren == 1);
            Validate(tree.Root.Parent == null);
            var child = tree.Root[0];
            Validate(child != null);
            Validate(child.Root == tree.Root);
            Validate(child.Parent == tree.Root);
            Validate(child.Text == "Dog");
            Validate(child.Type == SexType.Atomic);
            Validate(child.NumberOfChildren == 0);
            Validate(child.Start.Row == 1);
            Validate(child.Start.Column == 17);
            Validate(child.End.Row == 1);
            Validate(child.End.Column == 19);
        }
        static void TestCPPLineComment()
        {
            var parser = new SexParser();
            var tree = parser.ParseAsciiTree("TestCPPLineComment", "// panda \nbone kind");
            Validate(tree.Root != null);
            Validate(tree.Root.Type == SexType.Compound);
            Validate(tree.Root.Text.Length == 0);
            Validate(tree.Root.NumberOfChildren == 2);
            Validate(tree.Root.Parent == null);
            var child = tree.Root[0];
            Validate(child != null);
            Validate(child.Root == tree.Root);
            Validate(child.Parent == tree.Root);
            Validate(child.Text == "bone");
            Validate(child.Type == SexType.Atomic);
            Validate(child.NumberOfChildren == 0);
            Validate(child.Start.Row == 2);
            Validate(child.Start.Column == 1);
            Validate(child.End.Row == 2);
            Validate(child.End.Column == 4);

            var child2 = tree.Root[1];
            Validate(child2 != null);
            Validate(child2.Root == tree.Root);
            Validate(child2.Parent == tree.Root);
            Validate(child2.Text == "kind");
            Validate(child2.Type == SexType.Atomic);
            Validate(child2.NumberOfChildren == 0);
            Validate(child2.Start.Row == 2);
            Validate(child2.Start.Column == 6);
            Validate(child2.End.Row == 2);
            Validate(child2.End.Column == 9);
        }
        static void TestNull()
        {
            var parser = new SexParser();
            var tree = parser.ParseAsciiTree("TestNull", @"");
            Validate(tree.Root != null);
            Validate(tree.Root.Type == SexType.Null);
            Validate(tree.Root.Text.Length == 0);
            Validate(tree.Root.NumberOfChildren == 0);
        }
        static void TestNestedNull()
        {
            var parser = new SexParser();
            var tree = parser.ParseAsciiTree("TestNestedNull", @"()");
            Validate(tree.Root != null);
            Validate(tree.Root.Type == SexType.Compound);
            Validate(tree.Root.Text.Length == 0);
            Validate(tree.Root.NumberOfChildren == 1);

            var child = tree.Root[0];
            Validate(child != null);
            Validate(child.Root == tree.Root);
            Validate(child.Parent == tree.Root);
            Validate(child.Text.Length == 0);
            Validate(child.Type == SexType.Null);
            Validate(child.NumberOfChildren == 0);
            Validate(child.Start.Row == 1);
            Validate(child.Start.Column == 1);
            Validate(child.End.Row == 1);
            Validate(child.End.Column == 2);
        }
        static void TestStringLiteral()
        {
            var parser = new SexParser();
            var tree = parser.ParseAsciiTree("TestStringLiteral", @"(""job"")");
            Validate(tree.Root != null);
            Validate(tree.Root.Type == SexType.Compound);
            Validate(tree.Root.Text.Length == 0);
            Validate(tree.Root.NumberOfChildren == 1);

            var child = tree.Root[0];
            Validate(child != null);
            Validate(child.Root == tree.Root);
            Validate(child.Parent == tree.Root);
            Validate(child.Text.Length == 0);
            Validate(child.Type == SexType.Compound);
            Validate(child.NumberOfChildren == 1);
            Validate(child.Start.Row == 1);
            Validate(child.Start.Column == 1);
            Validate(child.End.Row == 1);
            Validate(child.End.Column == 7);

            var literal = child[0];
            Validate(literal != null);
            Validate(literal.Root == tree.Root);
            Validate(literal.Parent == child);
            Validate(literal.Text == "job");
            Validate(literal.Type == SexType.StringLiteral);
            Validate(literal.NumberOfChildren == 0);
            Validate(literal.Start.Row == 1);
            Validate(literal.Start.Column == 2);
            Validate(literal.End.Row == 1);
            Validate(literal.End.Column == 6);

        }

        static void TestStringLiteralEscaped()
        {
            var parser = new SexParser();
            var tree = parser.ParseAsciiTree("TestStringLiteralEscaped", @"(""&n&x31"")");
            Validate(tree.Root != null);
            Validate(tree.Root.Type == SexType.Compound);
            Validate(tree.Root.Text.Length == 0);
            Validate(tree.Root.NumberOfChildren == 1);

            var child = tree.Root[0];
            Validate(child != null);
            Validate(child.Root == tree.Root);
            Validate(child.Parent == tree.Root);
            Validate(child.Text.Length == 0);
            Validate(child.Type == SexType.Compound);
            Validate(child.NumberOfChildren == 1);
            Validate(child.Start.Row == 1);
            Validate(child.Start.Column == 1);
            Validate(child.End.Row == 1);
            Validate(child.End.Column == 10);

            var literal = child[0];
            Validate(literal != null);
            Validate(literal.Root == tree.Root);
            Validate(literal.Parent == child);
            Validate(literal.Text == "\n1");
            Validate(literal.Type == SexType.StringLiteral);
            Validate(literal.NumberOfChildren == 0);
            Validate(literal.Start.Row == 1);
            Validate(literal.Start.Column == 2);
            Validate(literal.End.Row == 1);
            Validate(literal.End.Column == 9);
        }

        static void MainProtected()
        {
            TestNull();
            TestAtomic();
            TestCstyleComment();
            TestCPPLineComment();
            TestNestedNull();
            TestStringLiteral();
            TestStringLiteralEscaped();
        }
        static void Main(string[] args)
        {
            try
            {
                MainProtected();
            }
            catch(Exception ex)
            {
                System.Console.WriteLine(ex.Message);
                System.Console.WriteLine(ex.StackTrace);
                System.Console.WriteLine();
            }
        }
    }
}
