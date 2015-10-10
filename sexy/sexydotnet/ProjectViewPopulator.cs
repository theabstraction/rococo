using System;
using System.Collections.Generic;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows;

using SexyDotNet.Host;

namespace SexyDotNet
{
    public static class ProjectViewPopulator
    {
        static ProjectViewPopulator() { }

        private static void OnContextMenu_Open(object sender, RoutedEventArgs e)
        {
            var mi = sender as MenuItem;
            if (mi != null)
            {
                var pf = mi.Tag as ProjectFile;
                if (pf != null)
                {
                    ThirdParty.OpenEditor(pf.Filename, 1, 1);
                }
            }
        }

        public static void Populate(Project project, TreeView tree, ContextMenu contextMenu, CompileError emphasis)
        {
            tree.Items.Clear();

            List<string> items = new List<string>();
            foreach (string key in project.Files.Keys)
            {
                items.Add(key);
            }

            items.Sort();

            foreach (string key in items)
            {
                var i = new TreeViewItem();
                i.Header = key;
                i.Tag = project.Files[key];
                i.ContextMenu = contextMenu;

                if (emphasis != null && emphasis.SourceFile == key)
                {
                    i.Foreground = Brushes.Red;
                    i.ToolTip = emphasis.Message;
                }
                else
                {
                    i.Foreground = Brushes.Black;
                }

                i.ContextMenuOpening += (sender, args) =>
                {
                    var ti = sender as TreeViewItem;
                    if (ti != null)
                    {
                        var pf = ti.Tag as ProjectFile;

                        if (pf != null)
                        {
                            contextMenu.Items.Clear();

                            var menuItem = new MenuItem { Header = "Open " + pf.Filename, Tag = pf };
                            menuItem.Click += OnContextMenu_Open;
                            contextMenu.Items.Add(menuItem);
                        }
                    }
                };

                tree.Items.Add(i);
            }
        }
    }
}
