﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml.Linq;
using Excel = Microsoft.Office.Interop.Excel;
using Office = Microsoft.Office.Core;
using Microsoft.Office.Tools.Excel;
using System.Windows.Forms;

namespace sexcel
{
    public partial class Sexel
    {
        public string ContentFolder { get; set; }
        public static void ShowMessage(string msg)
        {
            System.Windows.Forms.MessageBox.Show(msg, "Sexel - Sexy Excel");
        }

        public static string GetSaveNameFromSXLFile(string initialFilename)
        {
            string title = string.Format("Microsoft Excel: Choose a filename to export the Sexel class");
            dynamic result = Globals.Sexel.Application.GetSaveAsFilename(initialFilename, "Sexel File (*.sxl), *.sxl", 1, title);
            if (result is string)
            {
                return result as string;
            }
            else
            {
                return null;
            }
        }

        public static string GetContentFolderFromDialog(string initialPath)
        {
            var dlg = new System.Windows.Forms.FolderBrowserDialog();
            dlg.Description = "Microsoft Excel: Select the content path for the Rococo application";
            dlg.RootFolder = Environment.SpecialFolder.MyComputer;
            dlg.SelectedPath = initialPath;
            dlg.ShowNewFolderButton = true;
            var topLevelWindow = NativeWindow.FromHandle(new IntPtr(Globals.Sexel.Application.Hwnd));
            if (dlg.ShowDialog(topLevelWindow) == DialogResult.OK)
            {
                return dlg.SelectedPath;
            }
            else
            {
                return null;
            }
        }

        public static void ShowMessage(Exception ex)
        {
            string msg = "Sexel threw an exception\r\n";

            Exception i = ex;
            while (i != null)
            {
                msg += "\r\nStackTrace:" + i.StackTrace + "\r\nMessage: " + i.Message;
                i = i.InnerException;
            }

            ShowMessage(msg);
        }

        public static readonly string WorksheetName_MetaDataPrefix = "Sexel Metadata";
        public static readonly string WorksheetName_TablePrefix = "Table";

        public static void ValidateSexelFile()
        {
            foreach(Excel.Worksheet sheet in Globals.Sexel.Application.Sheets)
            {
                if (sheet.Name.StartsWith(WorksheetName_MetaDataPrefix))
                {
                    return;
                }
            }

            throw new Exception("No worksheet with a name prefixed with 'Sexel Metadata' found");
        }

        public static void SexUpWorksheets()
        {
            Tables.Clear();
            MetaData.Clear();

            int index = 1;
            foreach (Excel.Worksheet sheet in Globals.Sexel.Application.Worksheets)
            {
                try
                {
                    if (sheet.Name.StartsWith(WorksheetName_MetaDataPrefix))
                    {
                        MetaData.BuildFrom(sheet);
                    }
                    else if (sheet.Name.StartsWith(WorksheetName_TablePrefix))
                    {
                        string suffix = sheet.Name.Substring(WorksheetName_TablePrefix.Length).Trim();
                        if (suffix.Length == 0)
                        {
                            throw new Exception("Missing name. Sheet name has format 'Table <name>' where <name> is an alphanumeric string optionally padded with spaces");
                        }

                        var table = new TableParser(suffix, sheet);
                        Tables.Add(table);
                    }
                    index++;
                }
                catch(Exception ex)
                {
                    Sexel.ShowMessage(string.Format("Error in worksheet #{0} '{1}':\n{2}", index, sheet.Name, ex.Message));
                }
            }
        }

        private void Sexel_Startup(object sender, System.EventArgs e)
        {
        }

        private void Sexel_Shutdown(object sender, System.EventArgs e)
        {
        }

        public static void ExportToSXY(string filename)
        {
            if (Tables.Items.Count == 0)
            {
                Sexel.ShowMessage("There were no tables to export. Sex-up the worksheet and try again.");
            }

            try
            {
                var sexport = new Sexport(filename);
                sexport.AppendHeaders();

                sexport.AppendClass();

                System.IO.File.WriteAllText(filename, sexport.ToString(), Encoding.UTF8);
            }
            catch(Exception ex)
            {
                Sexel.ShowMessage(ex.Message);
            }
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
