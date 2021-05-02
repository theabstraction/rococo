using Microsoft.Office.Tools.Ribbon;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Office.Tools;
using Microsoft.Office.Tools.Excel;
using Microsoft.Office.Interop;

using Excel = Microsoft.Office.Interop.Excel;

namespace sexcel
{
    public partial class Ribbon
    {
        private void Ribbon_Load(object sender, RibbonUIEventArgs e)
        {

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
        }

        private void exportSXY_Click(object sender, RibbonControlEventArgs e)
        {
            Sexel.ExportToSXY();
        }
    }
}
