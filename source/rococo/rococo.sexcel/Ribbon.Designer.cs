
namespace sexcel
{
    partial class Ribbon : Microsoft.Office.Tools.Ribbon.RibbonBase
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        public Ribbon()
            : base(Globals.Factory.GetRibbonFactory())
        {
            InitializeComponent();
        }

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.tab1 = this.Factory.CreateRibbonTab();
            this.group1 = this.Factory.CreateRibbonGroup();
            this.exportSXY = this.Factory.CreateRibbonButton();
            this.button_open_export = this.Factory.CreateRibbonButton();
            this.edit_Content = this.Factory.CreateRibbonEditBox();
            this.edit_savepath = this.Factory.CreateRibbonEditBox();
            this.format = this.Factory.CreateRibbonButton();
            this.box1 = this.Factory.CreateRibbonBox();
            this.button_open_content = this.Factory.CreateRibbonButton();
            this.box2 = this.Factory.CreateRibbonBox();
            this.tab1.SuspendLayout();
            this.group1.SuspendLayout();
            this.box1.SuspendLayout();
            this.box2.SuspendLayout();
            this.SuspendLayout();
            // 
            // tab1
            // 
            this.tab1.ControlId.ControlIdType = Microsoft.Office.Tools.Ribbon.RibbonControlIdType.Office;
            this.tab1.Groups.Add(this.group1);
            this.tab1.Label = "TabAddIns";
            this.tab1.Name = "tab1";
            // 
            // group1
            // 
            this.group1.Items.Add(this.box1);
            this.group1.Items.Add(this.box2);
            this.group1.Label = "SexyScript";
            this.group1.Name = "group1";
            // 
            // exportSXY
            // 
            this.exportSXY.Label = "to SXL";
            this.exportSXY.Name = "exportSXY";
            this.exportSXY.Click += new Microsoft.Office.Tools.Ribbon.RibbonControlEventHandler(this.exportSXY_Click);
            // 
            // button_open_export
            // 
            this.button_open_export.Label = "...";
            this.button_open_export.Name = "button_open_export";
            this.button_open_export.Click += new Microsoft.Office.Tools.Ribbon.RibbonControlEventHandler(this.button_open_export_Click);
            // 
            // edit_Content
            // 
            this.edit_Content.Label = "content";
            this.edit_Content.MaxLength = 256;
            this.edit_Content.Name = "edit_Content";
            this.edit_Content.SizeString = "C:\\work\\rococo\\content\\ding_dong_bell_six_bongs";
            // 
            // edit_savepath
            // 
            this.edit_savepath.Label = "savepath:";
            this.edit_savepath.MaxLength = 256;
            this.edit_savepath.Name = "edit_savepath";
            this.edit_savepath.ShowLabel = false;
            this.edit_savepath.SizeString = "C:\\work\\rocococ\\content\\scripts\\rpg\\levels\\level0.sxl";
            this.edit_savepath.Text = null;
            // 
            // format
            // 
            this.format.Label = "Sex-up";
            this.format.Name = "format";
            this.format.ScreenTip = "Improves the appearance of SexyScript compliant data blocks";
            this.format.Click += new Microsoft.Office.Tools.Ribbon.RibbonControlEventHandler(this.format_Click);
            // 
            // box1
            // 
            this.box1.Items.Add(this.format);
            this.box1.Items.Add(this.exportSXY);
            this.box1.Items.Add(this.edit_savepath);
            this.box1.Items.Add(this.button_open_export);
            this.box1.Name = "box1";
            // 
            // button_open_content
            // 
            this.button_open_content.Label = "...";
            this.button_open_content.Name = "button_open_content";
            this.button_open_content.Click += new Microsoft.Office.Tools.Ribbon.RibbonControlEventHandler(this.button_open_content_Click);
            // 
            // box2
            // 
            this.box2.Items.Add(this.edit_Content);
            this.box2.Items.Add(this.button_open_content);
            this.box2.Name = "box2";
            // 
            // Ribbon
            // 
            this.Name = "Ribbon";
            this.RibbonType = "Microsoft.Excel.Workbook";
            this.Tabs.Add(this.tab1);
            this.Load += new Microsoft.Office.Tools.Ribbon.RibbonUIEventHandler(this.Ribbon_Load);
            this.tab1.ResumeLayout(false);
            this.tab1.PerformLayout();
            this.group1.ResumeLayout(false);
            this.group1.PerformLayout();
            this.box1.ResumeLayout(false);
            this.box1.PerformLayout();
            this.box2.ResumeLayout(false);
            this.box2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        internal Microsoft.Office.Tools.Ribbon.RibbonTab tab1;
        internal Microsoft.Office.Tools.Ribbon.RibbonGroup group1;
        internal Microsoft.Office.Tools.Ribbon.RibbonEditBox edit_Content;
        internal Microsoft.Office.Tools.Ribbon.RibbonButton button_open_export;
        internal Microsoft.Office.Tools.Ribbon.RibbonBox box1;
        internal Microsoft.Office.Tools.Ribbon.RibbonButton format;
        internal Microsoft.Office.Tools.Ribbon.RibbonButton exportSXY;
        internal Microsoft.Office.Tools.Ribbon.RibbonEditBox edit_savepath;
        internal Microsoft.Office.Tools.Ribbon.RibbonBox box2;
        internal Microsoft.Office.Tools.Ribbon.RibbonButton button_open_content;
    }

    partial class ThisRibbonCollection
    {
        internal Ribbon Ribbon
        {
            get { return this.GetRibbon<Ribbon>(); }
        }
    }
}
