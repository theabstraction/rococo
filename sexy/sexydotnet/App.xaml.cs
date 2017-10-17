using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Windows;
using System.Windows.Threading;
using System.Diagnostics;
using System.Security.Permissions;

namespace SexyDotNet
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {

        
    }

    public static class Logger
    {
        unsafe public static void Log(byte* text)
        {
            String s = new string((sbyte*)text);
            Program.Log(s);
        }
    }

    public class Program
    {
        static private MainWindow mainWindow = null;
        static private OutputWindow outputWindow = null;
        static private App app = null;

        [SecurityPermissionAttribute(SecurityAction.Demand, Flags = SecurityPermissionFlag.UnmanagedCode)]
        public static void DoEvents()
        {
            DispatcherFrame frame = new DispatcherFrame();
            Dispatcher.CurrentDispatcher.BeginInvoke(DispatcherPriority.Background,  new DispatcherOperationCallback(ExitFrame), frame);
            Dispatcher.PushFrame(frame);
        }

        public static object ExitFrame(object f)
        {
            ((DispatcherFrame)f).Continue = false;
            return null;
        }

        static private bool RouteSysMessages()
        {
            DoEvents();
            return !Dispatcher.CurrentDispatcher.HasShutdownStarted;
        }

        [STAThread]
        static void Main()
        {
            app = new App();
            
            Controller controller = null;

            try
            {
                mainWindow = new MainWindow();

                if (Debugger.IsAttached)
                {
                    mainWindow.Title = mainWindow.Title + " (debug mode)";
                }
               
                mainWindow.Show();

                outputWindow = new OutputWindow { Owner = mainWindow };
                outputWindow.WindowState = WindowState.Minimized;
                outputWindow.Show();

                controller = new Controller(mainWindow.Events, mainWindow.Views, RouteSysMessages);

                app.DispatcherUnhandledException += (sender, args) =>
                    {
                        ShowException(args.Exception, controller);
                        args.Handled = true;
                    };

                app.Run(mainWindow);
            }
            catch (Exception ex)
            {
                ShowException(ex, null);                
            }
            finally
            {
                mainWindow = null;
                outputWindow = null;
            }
        }

        public static void ShowException(Exception ex, Controller controller)
        {
            if (ex is SexyDotNet.Host.CompileError)
            {
                var ce = ex as SexyDotNet.Host.CompileError;
                var compileBox = new CompileWindow();
                compileBox.SetSource(ce.SourceFile);
                compileBox.SetText(ex.Message);
                compileBox.SetSourceLocation(ce.Start, ce.End);
                compileBox.ShowDialog();

                if (controller != null)
                {
                    controller.SetEmphasis(ce);
                }
            }
            else
            {
                var errBox = new ExceptionWindow();
                errBox.SetException(ex);
                errBox.ShowDialog();
            }
        }

        public static void Log(string text)
        {
            if (outputWindow != null)
            {
                outputWindow.AppendText(text);
            }
        }

    }
}
