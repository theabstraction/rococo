using System;
using SexyDotNet.Properties;
using System.Diagnostics;

namespace SexyDotNet
{
    public static class ThirdParty
    {
        static ThirdParty() { }

        public static void OpenEditor(string filename, int startX, int startY)
        {
            string editor = Settings.Default.ThirdPartyEditor;
            string editorFormat = Settings.Default.ThirdPartyEditorArgs;

            string editInvoke;
            string editArgs;

            if (editor == null || editor.Length == 0)
            {
                editInvoke = "notepad.exe";
                editArgs = filename;
            }
            else
            {
                editInvoke = editor;
                editArgs = string.Format(editorFormat, filename, startX, startY);
            }

            try
            {
                Process.Start(editInvoke, editArgs);
            }
            catch
            {
                Process.Start("notepad.exe", filename);
            }
        }
    }
}
