using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Security.Cryptography;
using System.Text;

namespace Rococo
{
    internal class Program
    {
        static bool HasArg(string argTarget, string[] args)
        {
            foreach (string arg in args)
            {
                if (arg == argTarget)
                {
                    return true;
                }
            }

            return false;
        }
        static void Main(string[] args)
        {
            // bool isUltra = HasArg("-ultra", args);
            // if (isUltra)
            RococoGUIUltraBuilder builderUltra = new RococoGUIUltraBuilder();
            builderUltra.Build();
           
            RococoGUIBuilder builder = new RococoGUIBuilder();
            builder.Build();
        }
    }

    // Copyright Epic Games, Inc. All Rights Reserved.

    internal class RococoGUIBuilder: RococoBuilder
    {
        public RococoGUIBuilder()
        {
            AppendFileByLink = true;
        }


        public void Build()
        {
            Console.WriteLine("\nCreating RococoOS");
            CreatePluginOSBundles();

            Console.WriteLine("\nCreating RococoUtil");
            CreateUtilBundles();

            Console.WriteLine("\nCreating RococoZLIB");
            CreateZLIBBundles();

            Console.WriteLine("\nCreating RococoTiff");
            CreateTiffBundles();

            Console.WriteLine("\nCreating RococoJPEG");
            CreateJPEGBundles();

            Console.WriteLine("\nCreating RococoGui");
            CreateGuiBundles();

            Console.WriteLine("\nCompleted");
        }

        private void CreatePluginOSBundles()
        {
            string osSourceDirectory = MakePluginSourceFolder("RococoOS");

            WrapHeaders(osSourceDirectory, null, new List<string>()
                {
                    "rococo.types.h",
                    "rococo.io.h",
                    "rococo.os.h",
                    "rococo.compiler.options.h",
                    "rococo.functional.h",
                    "rococo.task.queue.h",
                    "rococo.debugging.h",
                    "rococo.time.h",
                    "rococo.strings.h",
                    "rococo.hashtable.h",
                    "rococo.reflector.h",
                    "rococo.strings.reflection.h",
                    "rococo.ids.h",
                    Path.Join("allocators", "rococo.allocator.malloc.h"),
                    Path.Join("allocators", "rococo.allocator.via.interface.h"),
                    "rococo.map.h",
                    "rococo.allocators.h",
                    "rococo.os.win32.h",
                    "rococo.win32.target.win7.h",
                    "rococo.parse.h",
                    "rococo.impressario.inl",
                    "rococo.ringbuffer.h",
                    "rococo.SI.h",
                    "rococo.API.h",
                    "rococo.parse.h",
                    "rococo.sexy.allocators.h",
                    "sexy.unordered_map.h",
                    "sexy.vector.h"
                }
            );

            WrapHeaders(osSourceDirectory, rococoSexyIncludeDirectory, new List<string>()
                {
                    "sexy.types.h",
                    "sexy.strings.h",
                    "Sexy.S-Parser.h",
                    "sexy.compiler.public.h",
                    "sexy.debug.types.h",
                    "sexy.vm.h",
                    "sexy.vm.cpu.h",
                    "sexy.script.h"
    }
            );

            CreateBundleDirect(osSourceDirectory, "wrap.rococo_util.cpp", "rococo.os.UE5.h", "rococo.os.UE5.prelude.h", "rococo.os.UE5.postlude.h", "rococo/rococo.util",
                new List<string>()
                {
                "rococo.strings.cpp",
                "rococo.base.cpp",
                "rococo.heap.string.cpp",
                "rococo.allocators.cpp",
                "rococo.throw.cr_sex.cpp"
                }
            );
        }

        private void CreateUtilBundles()
        {
            string utilSourceDirectory = MakePluginSourceFolder("RococoUtil");

            WrapHeaders(utilSourceDirectory, null, new List<string>()
                {
                    "rococo.types.h",
                    "rococo.io.h",
                    "rococo.os.h",
                    "rococo.ui.h",
                    "rococo.compiler.options.h",
                    "rococo.functional.h",
                    "rococo.task.queue.h",
                    "rococo.debugging.h",
                    "rococo.time.h",
                    "rococo.strings.h",
                    "rococo.hashtable.h",
                    "rococo.reflector.h",
                    "rococo.strings.reflection.h",
                    "rococo.ids.h",
                    "rococo.map.h",
                    "rococo.allocators.h",
                    "rococo.great.sex.h",
                    "rococo.sexml.h",
                    "rococo.formatting.h",
                    "rococo.game.options.h",
                    "rococo.gui.retained.ex.h",
                    "rococo.gui.retained.h",
                    "rococo.game.options.ex.h",
                    "rococo.maths.h",
                    "rococo.maths.i32.h",
                    "rococo.vkeys.h",
                    "rococo.vkeys.win32.h",
                    "rococo.impressario.inl",
                    "rococo.ringbuffer.h",
                    "rococo.SI.h",
                    "rococo.API.h",
                    "rococo.parse.h",
                    "rococo.sexy.allocators.h",
                    "sexy.unordered_map.h",
                    "sexy.vector.h"
                }
            );

            WrapHeaders(utilSourceDirectory, rococoSexyIncludeDirectory, new List<string>()
                {
                    "sexy.types.h",
                    "Sexy.S-Parser.h",
                    "sexy.util.exports.h",
                    "sexy.strings.h",
                    "sexy.stdstrings.h"
                }
);

            CreateBundleDirect(utilSourceDirectory, "wrap.s-parser.cpp", "rococo.UE5.h", "rococo.UE5.prelude.h", "rococo.UE5.postlude.h", "rococo/sexy/SP/sexy.s-parser",
                new List<string>()
                {
                "sexy.s-parser.cpp",
                "sexy.s-builder.cpp",
                "sexy.s-parser.s-block.cpp"
                }
            );

            CreateBundleDirect(utilSourceDirectory, "wrap.s-utils.cpp", "rococo.UE5.h", null, null, "rococo/sexy/Utilities",
                new List<string>()
                {
                "sexy.util.cpp"
                }
            );

            CreateBundleDirect(utilSourceDirectory, "wrap.sexml.cpp", "rococo.UE5.h", null, null, "rococo/rococo.sexml",
              new List<string>()
              {
                "rococo.sexml.builder.cpp",
                "rococo.sexml.parser.cpp",
                "rococo.sexml.user.cpp"
              }
            );

            CreateBundleByMatch(utilSourceDirectory, "wrap.gui-retained.cpp", "rococo.UE5.prelude.h", "rococo.UE5.postlude.h", "rococo/rococo.gui.retained",
              new List<string>()
              {
                "rococo.gr.*.cpp"
              }
            );

            CreateBundleDirect(utilSourceDirectory, "wrap.maths.cpp", "rococo.UE5.h", null, null, "rococo/rococo.maths",
              new List<string>()
              {
                "rococo.integer.formatting.cpp",
                "rococo.maths.cpp",
                "rococo.collisions.cpp",
              }
            );

            CreateBundleDirect(utilSourceDirectory, "wrap.greatsex.cpp", "rococo.UE5.h", null, null, "rococo/rococo.great.sex",
                new List<string>()
                {
                "great.sex.colour.cpp",
                "great.sex.scheme.cpp",
                "great.sex.main.cpp",
                "great.sex.test-data.cpp"
                }
            );
        }

        private void CreateZLIBBundles()
        {
            string zlibSourceDirectory = MakePluginSourceFolder("RococoZLib");

            CreateSeparateFilesDirect(zlibSourceDirectory, "wrap.", "rococo.zlib.UE5.h", "rococo.zlib.prelude.h", "rococo.zlib.postlude.h", "3rd-Party/zlib",
                new List<string>()
                {
                "adler32.c",
                "crc32.c",
                "deflate.c",
                "infback.c",
                "inffast.c",
                "inflate.c",
                "inftrees.c",
                "trees.c",
                "uncompr.c",
                "zutil.c"
                }
            );
        }

        private void CreateGuiBundles()
        {
            string guiSourceDirectory = MakePluginSourceFolder("RococoGui");

            WrapHeaders(guiSourceDirectory, null, new List<string>()
                {
                    "rococo.types.h",
                    "rococo.compiler.options.h",
                    "rococo.gui.retained.ex.h",
                    "rococo.gui.retained.h",
                    "rococo.great.sex.h",
                    "rococo.types.h",
                    "rococo.io.h",
                    "rococo.os.h",
                    "rococo.compiler.options.h",
                    "rococo.functional.h",
                    "rococo.task.queue.h",
                    "rococo.debugging.h",
                    "rococo.time.h",
                    "rococo.strings.h",
                    "rococo.hashtable.h",
                    "rococo.reflector.h",
                    "rococo.strings.reflection.h",
                    "rococo.ids.h",
                    "rococo.map.h",
                    "rococo.allocators.h",
                    "rococo.ui.h",
                    "rococo.maths.h",
                    "rococo.maths.i32.h",
                    "rococo.vkeys.h",
                    "rococo.vkeys.win32.h",
                    "rococo.vector.ex.h",
                    "rococo.imaging.h",
                    "rococo.game.options.h"
                }
            );

            WrapHeaders(guiSourceDirectory, Path.Join(rococoHomeDirectory, "source", "rococo", "rococo.gui.retained"), new List<string>()
                {
                   "rococo.gr.image-loading.inl"
                }
            );
        }

        private void CreateTiffBundles()
        {
            string tiffSourceDirectory = MakePluginSourceFolder("RococoTiffLib");

            WrapHeaders(tiffSourceDirectory, null, new List<string>()
                {
                    "rococo.api.h",
                    "rococo.types.h",
                    "rococo.compiler.options.h",
                    "rococo.imaging.h",
                    "rococo.os.h",
                    "rococo.strings.h",
                    "rococo.os.win32.h",
                    "rococo.win32.target.win7.h"
                }
            );

            CreateSeparateFilesDirect(tiffSourceDirectory, "wrap.", "rococo.tiff.UE5.h", "rococo.tiff.prelude.h", "rococo.tiff.postlude.h", "3rd-Party/libtiff/libtiff",
                new List<string>()
                {
                "tif_aux.c",
                "tif_close.c",
                "tif_codec.c",
                "tif_color.c",
                "tif_compress.c",
                "tif_dir.c",
                "tif_dirinfo.c",
                "tif_dirread.c",
                "tif_dirwrite.c",
                "tif_dumpmode.c",
                "tif_error.c",
                "tif_extension.c",
                "tif_fax3.c",
                "tif_fax3sm.c",
                "tif_flush.c",
                "tif_getimage.c",
                "tif_hash_set.c",
                "tif_jbig.c",
                "tif_jpeg.c",
                "tif_jpeg_12.c",
                "tif_lerc.c",
                "tif_luv.c",
                "tif_lzma.c",
                "tif_lzw.c",
                "tif_next.c",
                "tif_ojpeg.c",
                "tif_open.c",
                "tif_packbits.c",
                "tif_pixarlog.c",
                "tif_predict.c",
                "tif_print.c",
                "tif_read.c",
                "tif_strip.c",
                "tif_swab.c",
                "tif_thunder.c",
                "tif_tile.c",
                "tif_version.c",
                "tif_warning.c",
                "tif_webp.c",
                "tif_write.c",
                "tif_zip.c",
                "tif_zstd.c"
                }
            );

            CreateSeparateFilesDirect(tiffSourceDirectory, "wrap.", "rococo.tiff.UE5.h", "rococo.tiff.prelude.decl.h", "rococo.tiff.postlude.h", "3rd-Party/libtiff/",
                new List<string>()
                {
                "bloke.tiff.cpp"
                }
            );
        }

        private void CreateJPEGBundles()
        {
            string jpegSourceDirectory = MakePluginSourceFolder("RococoJPEGLib");

            WrapHeaders(jpegSourceDirectory, null, new List<string>()
                {
                    "rococo.types.h",
                    "rococo.compiler.options.h",
                    "rococo.imaging.h"
                }
            );

            WrapHeaders(jpegSourceDirectory, Path.Join(rococoHomeDirectory, "source", "3rd-Party", "libjpg", "jpeg-6b"), new List<string>()
                {
                    "jinclude.h",
                    "jpeglib.h",
                    "jerror.h",
                    "jdatastream.h"
                }
            );

            CreateSeparateFilesDirect(jpegSourceDirectory, "wrap.", "rococo.jpg.UE5.h", "rococo.jpg.prelude.dll.h", "rococo.jpg.postlude.dll.h", "3rd-Party/libjpg/jpeg-6b",
                   new List<string>()
                   {
                    "jcparam.c",
                    "jdapistd.c",
                    "transupp.c"
                   }
            );

            CreateSeparateFilesDirect(jpegSourceDirectory, "wrap.", "rococo.jpg.UE5.h", "rococo.jpg.prelude.h", "rococo.jpg.postlude.h", "3rd-Party/libjpg/jpeg-6b", new List<string>()
            {
                "jcapimin.c",
                "jcapistd.c",
                "jccoefct.c",
                "jccolor.c",
                "jcdctmgr.c",
                "jchuff.c",
                "jcinit.c",
                "jcmainct.c",
                "jcmarker.c",
                "jcmaster.c",
                "jcomapi.c",
                "jcphuff.c",
                "jcprepct.c",
                "jcsample.c",
                "jctrans.c",
                "jdapimin.c",
                "jdatafromem.c",
                "jdatasrc.c",
                "jdatadst.c",
                "jdatastream.h",
                "jdcoefct.c",
                "jdcolor.c",
                "jddctmgr.c",
                "jdhuff.c",
                "jdinput.c",
                "jdmainct.c",
                "jdmarker.c",
                "jdmaster.c",
                "jdmerge.c",
                "jdphuff.c",
                "jdpostct.c",
                "jdsample.c",
                "jdtrans.c",
                "jerror.c",
                "jfdctflt.c",
                "jfdctfst.c",
                "jfdctint.c",
                "jidctflt.c",
                "jidctint.c",
                "jidctfst.c",
                "jidctred.c",
                "jmemmgr.c",
                "jmemnobs.c",
                "jquant1.c",
                "jquant2.c",
                "jutils.c",
                "rdbmp.c",
                "rdcolmap.c",
                "rdgif.c",
                "rdppm.c",
                "rdrle.c",
                "rdswitch.c",
                "rdtarga.c",
                "wrbmp.c",
                "wrgif.c",
                "wrppm.c",
                "wrrle.c",
                "wrtarga.c"
            });

            CreateSeparateFilesDirect(jpegSourceDirectory, "wrap.", "rococo.jpg.UE5.h", "rococo.jpg.prelude.decl.h", "rococo.jpg.postlude.h", "3rd-Party/libjpg/",
                new List<string>()
                {
                "readimage.cpp",
                "writeimage.cpp"
                }
            );
        }
    }
}
