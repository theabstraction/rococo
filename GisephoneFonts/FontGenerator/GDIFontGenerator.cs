using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace Gisephone.Fonts.Generator
{
    public struct Glyph
    {
        public int A { get; private set; }
        public int B { get; private set; }
        public int C { get; private set; }
        public int X { get; private set; }
        public int Y { get; private set; }

        public Glyph(int a, int b, int c, int x, int y): this()
        {
            A = a;
            B = b;
            C = c;
            X = x;
            Y = y;
        }
    }

    public class FontDescriptors
    {        
        private Dictionary<char, Glyph> glyphs = new Dictionary<char, Glyph>();

        public FontDescriptors(Font f, FontMetrics metrics)
        {
            Name = f.Name;
            Height = f.Height;
            Ascent = metrics.Ascent;
        }

        public string Name { get; private set; }
        public int Height { get; private set; }
        public int Ascent { get; private set; }

        public void Add(char c, int A, int B, int C, int X, int Y)
        {
            var g = new Glyph( A, B, C, X, Y );
            glyphs.Add(c, g);
        }

        public Glyph this[char index]
        {
            get
            {
                Glyph g;
                if (glyphs.TryGetValue(index, out g))
                {
                    return g;
                }
                else
                {
                    return new Glyph( 0, 0, 0, -1, -1 );
                }
            }
        }

        public IEnumerable<char> GlyphCharacters
        {
            get
            {
                return glyphs.Keys;
            }
        }
    }

    public class GDIFontGenerator
    {
        public static FontDescriptors[] Generate(int width, int height, byte[] pixels, Font[] fonts)
        {
            FontDescriptors[] descriptors = null;

            if (fonts != null && fonts.Length > 0)
            {
                using (Bitmap bmp = new Bitmap(width, height, PixelFormat.Format32bppArgb))
                {
                    using (Graphics g = Graphics.FromImage(bmp))
                    {
                        descriptors = DrawFonts(g, width, height, fonts);
                    }

                    CopyBmpDataToArray(pixels, width, height, bmp);
                }
            }

            return descriptors;
        }

        private static float ComputeNearestDistanceToBlack(int targetX, int targetY, int width, int height, byte[] pixels, int x0, int x1, int y0, int y1)
        {
            double minRadiusSq = float.MaxValue;

            for (int j = y0; j < y1; ++j)
            {
                for (int i = x0; i < x1; ++i)
                {
                    byte intensity = pixels[i + j * width];
                    if (intensity > 0)
                    {
                        double dX = (double)(i - targetX);
                        double dY = (double)(j - targetY);

                        double radiusSq = dX * dX + dY * dY + (255 - intensity) / 255.0;

                        if (radiusSq < minRadiusSq) minRadiusSq = radiusSq;
                    }
                }
            }

            return (float) (8.0f * System.Math.Sqrt(minRadiusSq));
        }

        private static void ComputeDistanceGlyphs(Glyph glyph, int fontHeight, byte[] distanceMap, int width, int height, byte[] pixels)
        {
            int x0 = glyph.X + glyph.A;
            int x1 = x0 + glyph.B;
            int y0 = glyph.Y;
            int y1 = y0 + fontHeight;

            if (y1 > height) return;

            for(int j = y0; j < y1; ++j)
            {
                for (int i = x0; i < x1; ++i)
                {
                    float dist = ComputeNearestDistanceToBlack(i, j, width, height, pixels, x0, x1, y0, y1);
                    distanceMap[i + j * width] = (dist > 254) ? (byte)255 : (byte)(int)dist;
                }
            }
        }

        public static byte[] GenerateDistanceMap(int width, int height, byte[] pixels, Font[] fonts, FontDescriptors[] fd)
        {
            byte maxValue = 255;

            byte[] cells = new byte[pixels.Length];
            for(int i = 0; i < cells.Length; ++i)
            {
                cells[i] = maxValue;
            }

            if (fonts != null)
            {
                for (int i = 0; i < fonts.Length; ++i)
                {
                    var desc = fd[i];
                    var font = fonts[i];
                    foreach (var c in desc.GlyphCharacters)
                    {
                        var glyph = desc[c];
                        ComputeDistanceGlyphs(glyph, font.Height, cells, width, height, pixels);
                    }
                }
            }
           
            return cells;
        }

        unsafe private static void CopyBmpDataToArray(byte[] pixels, int width, int height, Bitmap bmp)
        {
            BitmapData bmpData = bmp.LockBits(new Rectangle(0, 0, width, height), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
            int* scan0 = (int*)bmpData.Scan0.ToPointer();

            int stride;
            int* pBits;
            if (bmpData.Stride > 0)
            {
                pBits = scan0;
                stride = bmpData.Stride >> 2;
            }
            else
            {
                pBits = scan0 + bmpData.Stride * (height - 1);
                stride = -bmpData.Stride >> 2;
            }

            fixed (byte* target = pixels)
            {
                byte* targetLine = target;
                int* srcLine = pBits;

                for (int j = 0; j < height; j++)
                {
                    byte* targetPt = targetLine;
                    int* srcPt = srcLine;

                    for (int i = 0; i < width; i++)
                    {
                        *targetPt++ = (byte) (0x000000FF & *srcPt);
                        srcPt++;
                    }

                    targetLine += width;
                    srcLine += stride;
                }                
            }

            bmp.UnlockBits(bmpData);
        }

        static IEnumerable<char> AnsiGlyphs
        {
            get
            {
                for (int c = 32; c < 255; ++c)
                {
                    yield return (char) c;
                }
            }
        }

        private static FontDescriptors[] DrawFonts(Graphics g, int width, int height, Font[] fonts)
        {
            float hborder = 4.0f;
            float safetyMargin = 2.0f * hborder;
            float y = 0;
            float LHS = safetyMargin;
            float x = LHS;

            var descriptors = new FontDescriptors[fonts.Length];

            float lastHeight = -1.0f;
            float nextRowHeight = -1.0f;

            for (int i = 0; i < fonts.Length; i++)
            {
                Font f = fonts[i];
                
                var metrics = FontMetrics.From(g, f);

                descriptors[i] = new FontDescriptors(f, metrics);

                IEnumerable<char> glyphs;

                glyphs = AnsiGlyphs;

                if (nextRowHeight < 0)
                {
                    nextRowHeight = metrics.Height;
                }

                if (lastHeight >= metrics.Height)
                {
                    
                }
                else
                {
                    x = LHS;
                    if (lastHeight > 0)
                    {
                        y += nextRowHeight + 1;
                        nextRowHeight = metrics.Height;
                    }
                }

                lastHeight = metrics.Height;

                var sf = StringFormat.GenericTypographic;
                
                foreach (char c in glyphs)
                {
                    if (!metrics.IsGlyphAvailable(c)) continue;

                    string s = new string(c, 1);
                       
                    int A = metrics.GetA(c) - (int) hborder;
                    int B = metrics.GetB(c) + (int) safetyMargin;
                    int C = metrics.GetC(c) - (int)hborder;

                    float dx = (float)(B);
                    if (x + dx >= width)
                    {
                        x = LHS;
                        y += nextRowHeight + 1;
                        nextRowHeight = metrics.Height;
                    }

                    float Tx = x - (float)A;

                    descriptors[i].Add(c, A, B, C, (int) Tx, (int) y);

                    if (metrics.Height > 22)
                    {
                        g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.ClearTypeGridFit;
                    }
                    else
                    {
                        g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.SingleBitPerPixelGridFit;
                    }

                    TextRenderer.DrawText(g, s, f, new Point((int) Tx, (int) y), Color.White, TextFormatFlags.NoPadding);

                //    g.DrawString(s, f, Brushes.White, new PointF(Tx, y), sf);

                    x += dx + safetyMargin;
                }
            }

            return descriptors;
        }        
    }
}
