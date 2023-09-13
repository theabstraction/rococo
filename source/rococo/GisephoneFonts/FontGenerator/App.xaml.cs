using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Windows;
using Microsoft.Win32;

namespace Gisephone.Fonts.Generator
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        private Generator generator = null;
        private MainWindow mainWindow = null;


        private void Application_Startup(object startSender, StartupEventArgs startArgs)
        {
            generator = new Generator();
            mainWindow = new MainWindow();
            mainWindow.Export += () =>
                {
                    var sfd = new SaveFileDialog
                    {
                        CheckPathExists = true,
                        DefaultExt = "csv",
                        Title = "Gisephone.Fonts: Export to CSV...",
                        AddExtension = true,
                        Filter = "Gisephone Fonts File) | *.csv"
                    };

                    if (sfd.ShowDialog().GetValueOrDefault(false))
                    {
                        generator.Save(sfd.FileName);
                    }
                };
            mainWindow.Render = generator.Render;
            mainWindow.FontListChanged += (sender, args) =>
                {
                    generator.Fonts = mainWindow.AllFonts;            
                };
            mainWindow.Show();
        }
    }
}
