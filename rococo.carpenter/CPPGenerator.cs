using System;
using System.Collections.Generic;
using System.Text;

namespace Rococo.Carpenter
{
    public static class CPPCore
    {

    }

    public class CPPGenerator
    {
        public ITypes Types
        {
            get; 
            private set;
        }

        public IMetaData MetaData
        {
            get;
            private set;
        }

        public ITable Table
        {
            get;
            private set;
        }

        public IRules Rules
        {
            get;
            private set;
        }
        
        public CPPGenerator(ITypes types, IMetaData metaData, ITable table, IRules rules)
        {
            Types = types;
            MetaData = metaData;
            Table = table;
            Rules = rules;
        }

        public void Go()
        {
            Console.WriteLine("Beginning export");
        }
    }
}
