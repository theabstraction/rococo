using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media;
using System.Windows;
using System.Drawing;
using System.Windows.Media.Imaging;
using System.Xml;
using System.IO;

namespace Gisephone.Fonts.Generator
{
    public class Generator
    {
        public const int Width = 1024;
        public const int Height = 1024;
        private byte[] pixels = new byte[Width * Height];
        private Font[] fonts = null;
        private BitmapSource bitmap = null;
        private FontDescriptors[] decriptors = null;
        private BitmapSource distanceMap = null;

        public IEnumerable<Font> Fonts
        {
            set
            {
                IEnumerable<Font> v = value;
                int count = 0;
                foreach (var f in v) count++;
                fonts = new Font[count];
                int i = 0;
                foreach (var f in v)
                {
                    fonts[i++] = f;
                }

                bitmap = null;
            }
        }

        public static void SetAttribute(XmlNode node, string name, string value)
        {
            var a = node.OwnerDocument.CreateAttribute(name);
            a.Value = value;
            node.Attributes.Append(a);
        }

        public static void SetAttribute(XmlNode node, string name, int value)
        {
            SetAttribute(node, name, value.ToString());
        }

        public void Save(string filename)
        {
            var sb = new StringBuilder();

            sb.AppendFormat("NumberOfSpecs,{0},1024,1024\n", decriptors.Length);

            int specNumber = 0;
            foreach (var fd in decriptors)
            { 
                int chars = 0;
                foreach(char c in fd.GlyphCharacters)
                {
                    chars++;
                }

                sb.AppendFormat("Spec,{0},{1},{2},{3},{4},{5}\n", specNumber++, fd.Name, fd.Height, fd.Ascent, chars, (int) '.');

                foreach(char c in fd.GlyphCharacters)
                {
                    Glyph g = fd[c];
                    sb.AppendFormat("{0},ABC,{1},{2},{3},{4},{5}\n", (int)c, g.A, g.B, g.C, g.X + g.A, g.Y);
                }
            }

            try
            {
                File.WriteAllText(filename, sb.ToString(), Encoding.ASCII);
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.Message, "Error saving: " + filename, MessageBoxButton.OK, MessageBoxImage.Error);
            }
            
            if (bitmap != null)
            {
                string target = Path.GetFileNameWithoutExtension(filename) + ".tif";
                string dir = Path.GetDirectoryName(filename);
                string fullfilename = dir + Path.DirectorySeparatorChar + target;
                using (var fs = File.Create(fullfilename))
                {
                    var encoder = new TiffBitmapEncoder();
                    encoder.Frames.Add(BitmapFrame.Create(bitmap));
                    encoder.Save(fs);
                }
            }

            if (distanceMap != null)
            {
                string target = Path.GetFileNameWithoutExtension(filename) + ".dist.tif";
                string dir = Path.GetDirectoryName(filename);
                string fullfilename = dir + Path.DirectorySeparatorChar + target;
                using (var fs = File.Create(fullfilename))
                {
                    var encoder = new TiffBitmapEncoder();
                    encoder.Frames.Add(BitmapFrame.Create(distanceMap));
                    encoder.Save(fs);
                }
            }
        }
        
        public void Render(DrawingContext dc, Rect rect)
        {
            LazyGenerate();
            dc.DrawImage(bitmap, rect);
        }

        private void LazyGenerate()
        {
            if (bitmap != null) return;

            decriptors = GDIFontGenerator.Generate(Width, Height, pixels, fonts);

            PixelFormat pf = PixelFormats.Gray8;
            int dpi = 96;
            bitmap = BitmapSource.Create(Width, Height, dpi, dpi, pf, null, pixels, Width);

            var distanceCells = GDIFontGenerator.GenerateDistanceMap(Width, Height, pixels, fonts, decriptors);
            distanceMap = BitmapSource.Create(Width, Height, dpi, dpi, pf, null, distanceCells, Width);
        }       
    }
}
