using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml.Linq;
using Excel = Microsoft.Office.Interop.Excel;
using Office = Microsoft.Office.Core;
using Microsoft.Office.Tools.Excel;

namespace sexcel
{
    public partial class Sexel
    {  
        public static void ShowMessage(string msg)
        {
            System.Windows.Forms.MessageBox.Show(msg, "Sexel - Sexy Excel");
        }

        public static void ShowMessage(Exception ex)
        {
            string msg = "Sexel threw an exception\r\n";

            Exception i = ex;
            while(i != null)
            {
                msg += "\r\nStackTrace:" + i.StackTrace + "\r\nMessage: " + i.Message;
                i = i.InnerException;
            }

            ShowMessage(msg);
        }

        public static bool IsSexyScript()
        {
            Excel.Worksheet sheet = Globals.Sexel.Application.ActiveSheet;
            if (sheet == null)
            {
                return false;
            }

            Excel.Range range = Globals.Sexel.Application.get_Range("A1", "Z20");
            Excel.Range sexyIndicator = range.Find("#sexel");

            return (sexyIndicator != null);
        }

        private static void ParseCommandString(int rowIndex, string text, Excel.Range cell)
        {
            Excel.Worksheet sheet = Globals.Sexel.Application.ActiveSheet;

            switch (text)
            {
                case "string":
                    cell.Font.Bold = true;
                    cell.Font.Color = Excel.XlRgbColor.rgbBlue;
                    break;
                case "#table":
                    try
                    {
                        var table = new TableParser(rowIndex, sheet);
                        cell.Font.Color = Excel.XlRgbColor.rgbGray;
                        Tables.Add(table);
                    }
                    catch(Exception ex)
                    {
                        Sexel.ShowMessage(ex.Message);
                    }
                    break;
            }
        }

        public static void SexUpWorksheet()
        {
            try
            {
                Tables.Clear();
                Excel.Worksheet sheet = Globals.Sexel.Application.ActiveSheet;
                if (sheet == null)
                {
                    return;
                }

                int nRows = sheet.UsedRange.Rows.Count;
                int nColumns = sheet.UsedRange.Rows.Count;

                if (nColumns < 2)
                {
                    return;
                }

                for (int i = 1; i <= nRows; ++i)
                {
                    Excel.Range command = sheet.Cells[i, 2];

                    object value = command.Value;
                    if (value is String)
                    {
                        ParseCommandString(i, value as string, command);
                    }
                }
            }
            catch(Exception ex)
            {
                ShowMessage(ex);
            }
        }

        private void Sexel_Startup(object sender, System.EventArgs e)
        {
        }

        private void Sexel_Shutdown(object sender, System.EventArgs e)
        {
        }

        #region VSTO generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InternalStartup()
        {
            this.Startup += new System.EventHandler(Sexel_Startup);
            this.Shutdown += new System.EventHandler(Sexel_Shutdown);
        }
        
        #endregion
    }
}
