using Microsoft.Office.Tools.Ribbon;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Office.Tools;
using Microsoft.Office.Tools.Excel;
using Microsoft.Office.Interop;
using Microsoft.Win32;

using Excel = Microsoft.Office.Interop.Excel;

namespace sexcel
{
    public partial class Ribbon
    {
        private readonly string REGKEY_SEXEL_SAVEPATH = @"SOFTWARE\Rococo\Sexel\savepath";
        private readonly string REGKEY_SEXEL_CONTENTPATH = @"SOFTWARE\Rococo\Sexel\contentpath";
        private void Ribbon_Load(object sender, RibbonUIEventArgs e)
        {
            string value = Registry.CurrentUser.GetValue(REGKEY_SEXEL_SAVEPATH) as string;
            if (value == null)
            {
                value = @"C:\work\rococo\content\scripts\default-export.sxl";
            }

            string content = Registry.CurrentUser.GetValue(REGKEY_SEXEL_CONTENTPATH) as string;
            if (content == null)
            {
                content = @"C:\work\rococo\content\";
            }

            this.edit_savepath.Text = value;
            this.edit_Content.Text = content;
            this.exportSXY.Enabled = false;

            Globals.Sexel.ContentFolder = content;
        }

        private void format_Click(object sender, RibbonControlEventArgs e)
        {
            try
            {
                Sexel.ValidateSexelFile();
            }
            catch(Exception ex)
            {
                Sexel.ShowMessage(ex.Message);
            }

            Sexel.SexUpWorksheets();

            this.exportSXY.Enabled = Tables.Items.Count > 0;
        }

        private void exportSXY_Click(object sender, RibbonControlEventArgs e)
        {
            Registry.CurrentUser.SetValue(REGKEY_SEXEL_SAVEPATH, this.edit_savepath.Text);
            Sexel.ExportToSXY(this.edit_savepath.Text);
            this.exportSXY.Enabled = false;
        }

        private void importSXY_Click(object sender, RibbonControlEventArgs e)
        {
            
        }

        private void button_open_export_Click(object sender, RibbonControlEventArgs e)
        {
            string name = Sexel.GetSaveNameFromSXLFile(this.edit_savepath.Text);
            if (name != null) { this.edit_savepath.Text = name; }
        }

        private void button_open_content_Click(object sender, RibbonControlEventArgs e)
        {
            string content = Sexel.GetContentFolderFromDialog(this.edit_Content.Text);
            if (content != null)
            { 
                this.edit_Content.Text = content;
                Globals.Sexel.ContentFolder = content;
            }
        }
    }
}
