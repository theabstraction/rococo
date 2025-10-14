using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Security.Cryptography;
using System.Text;

namespace Rococo
{
    // Copyright Epic Games, Inc. All Rights Reserved.

    internal class RococoGUIUltraBuilder: RococoBuilder
    {
        public RococoGUIUltraBuilder()
        {
            AppendFileByLink = false;
        }

        public void Build()
        {
            Console.WriteLine("\nCreating RococoGuiUltra");
            CreateGuiUltraBundles();

            Console.WriteLine("\nCompleted");
        }

        private void CreateGuiUltraBundles()
        {
            string ultraDirectory = Path.Join(rococoHomeDirectory, @"UE5\Plugins\RococoGuiUltra\Source\RococoGuiUltra\Private".Replace('\\', Path.DirectorySeparatorChar));

            if (!Directory.Exists(ultraDirectory))
            {
                throw new Exception("Expecting directory to exist: " + ultraDirectory);
            }

            WrapHeaders(ultraDirectory, null, new List<string>()
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
                    "rococo.game.options.h",
                    "rococo.gui.retained.ex.h",
                    "rococo.gui.retained.h",
                    "rococo.game.options.ex.h",
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
                    "sexy.vector.h",
                    "rococo.imaging.h",
                    "rococo.great.sex.h",
                    "rococo.formatting.h",
                    "rococo.maths.h",
                    "rococo.maths.i32.h",
                    "rococo.sexml.h",
                    "rococo.ui.h",
                    "rococo.vkeys.h",
                    "rococo.vkeys.win32.h",
                    "rococo.vector.ex.h"
                }
          );

            WrapHeaders(ultraDirectory, rococoSexyIncludeDirectory, new List<string>()
                {
                    "sexy.types.h",
                    "sexy.strings.h",
                    "Sexy.S-Parser.h",
                    "sexy.compiler.public.h",
                    "sexy.debug.types.h",
                    "sexy.vm.h",
                    "sexy.vm.cpu.h",
                    "sexy.script.h",
                    "sexy.util.exports.h",
                    "sexy.stdstrings.h",
                }
            );

            CreateBundleDirect(ultraDirectory, "rococo-bundle.rococo_util.cpp", "rococo.os.UE5.h", "rococo.os.UE5.prelude.h", "rococo.os.UE5.postlude.h", "rococo/rococo.util",
                new List<string>()
                {
                    "rococo.strings.cpp",
                    "rococo.base.cpp",
                    "rococo.heap.string.cpp",
                    "rococo.allocators.cpp",
                    "rococo.throw.cr_sex.cpp"
                }
            );

            CreateBundleDirect(ultraDirectory, "rococo-bundle.s-parser.cpp", "rococo.UE5.cpp.h", "rococo.UE5.prelude.h", "rococo.UE5.postlude.h", "rococo/sexy/SP/sexy.s-parser",
                new List<string>()
                {
                      "sexy.s-parser.cpp",
                      "sexy.s-builder.cpp",
                      "sexy.s-parser.s-block.cpp"
                }
            );

            CreateBundleDirect(ultraDirectory, "rococo-bundle.s-utils.cpp", "rococo.UE5.cpp.h", null, null, "rococo/sexy/Utilities",
                new List<string>()
                {
                    "sexy.util.cpp"
                }
            );

            CreateBundleDirect(ultraDirectory, "rococo-bundle.sexml.cpp", "rococo.UE5.cpp.h", null, null, "rococo/rococo.sexml",
                new List<string>()
                {
                    "rococo.sexml.builder.cpp",
                    "rococo.sexml.parser.cpp",
                    "rococo.sexml.user.cpp"
                }
            );

            CreateBundleByMatch(ultraDirectory, "rococo-bundle.gui-retained.cpp", "rococo.UE5.prelude.h", "rococo.UE5.postlude.h", "rococo/rococo.gui.retained",
                new List<string>()
                {
                    "rococo.gr.*.cpp",
                }
            );

            CreateBundleDirect(ultraDirectory, "rococo-bundle.maths.cpp", "rococo.UE5.cpp.h", null, null, "rococo/rococo.maths",
                new List<string>()
                {
                    "rococo.integer.formatting.cpp",
                    "rococo.maths.cpp",
                    "rococo.collisions.cpp",
                }
            );

            CreateBundleDirect(ultraDirectory, "rococo-bundle.greatsex.cpp", "rococo.UE5.cpp.h", null, null, "rococo/rococo.great.sex",
                new List<string>()
                {
                    "great.sex.colour.cpp",
                    "great.sex.scheme.cpp",
                    "great.sex.main.cpp",
                    "great.sex.test-data.cpp"
                }
            );

            CreateSeparateFilesDirect(ultraDirectory, "zlib.", "rococo.zlib.UE5.h", "rococo.zlib.prelude.h", "rococo.zlib.postlude.h", Path.Join("3rd-Party", "zlib"),
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

            CreateSeparateFilesDirect(ultraDirectory, "tiff-lib.", "rococo.tiff.UE5.h", "rococo.tiff.prelude.h", "rococo.tiff.postlude.h", Path.Join("3rd-Party", "libtiff", "libtiff"),
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
                    "tif_zstd.c",
                    "tif_fax3.c",
                    "tif_fax3sm.c"
                }
            );

            CreateSeparateFilesDirect(ultraDirectory, "tiff-lib.", "rococo.tiff.UE5.h", "rococo.tiff.prelude.decl.cpp.h", "rococo.tiff.postlude.h", Path.Join("3rd-Party", "libtiff"),
                new List<string>()
                {
                    "bloke.tiff.cpp"
                }
            );

            WrapHeaders(ultraDirectory, Path.Join(rococoHomeDirectory, "source", "3rd-Party", "libjpg", "jpeg-6b"), new List<string>()
                {
                    "jinclude.h",
                    "jpeglib.h",
                    "jerror.h",
                    "jdatastream.h"
                }
            );

            CreateSeparateFilesDirect(ultraDirectory, "jpeg-lib.", "rococo.jpg.UE5.h", "rococo.jpg.prelude.dll.h", "rococo.jpg.postlude.dll.h", "3rd-Party/libjpg/jpeg-6b",
                new List<string>()
                {
                    "jcparam.c",
                    "jdapistd.c",
                    "transupp.c"
                }
            );

            CreateSeparateFilesDirect(ultraDirectory, "jpeg-lib.", "rococo.jpg.UE5.h", "rococo.jpg.prelude.h", "rococo.jpg.postlude.h", "3rd-Party/libjpg/jpeg-6b",
                new List<string> {
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
            }
            );

            CreateSeparateFilesDirect(ultraDirectory, "jpeg-lib.", "rococo.jpg.UE5.h", "rococo.jpg.prelude.decl.cpp.h", "rococo.jpg.postlude.h", "3rd-Party/libjpg/",
                new List<string>()
                {
                    "readimage.cpp",
                    "writeimage.cpp"
                }
            );

            string ultraHeaderDirectory = Path.Join(ultraDirectory, "..", "public");

            CopyOtherPluginFileToSource(ultraDirectory, "RococoOS", "rococo.OS.UE5.cpp");
            CopyOtherPluginFileToSource(ultraDirectory, "RococoUtil", "RococoBP_Util_Lib.cpp");
            CopyOtherPluginFileToSource(ultraDirectory, "RococoGui", "GameOptionBuilder.cpp");
            CopyOtherPluginFileToSource(ultraDirectory, "RococoGui", "ReflectedGameOptionsBuilder.cpp");
            CopyOtherPluginFileToSource(ultraDirectory, "RococoGui", "RococoFontSet.cpp");
            CopyOtherPluginFileToSource(ultraDirectory, "RococoGui", "RococoGRHostWidget.cpp");
            CopyOtherPluginFileToSource(ultraDirectory, "RococoGui", "RococoGui_BPEvents.cpp");
            CopyOtherPluginFileToSource(ultraDirectory, "RococoGui", "SRococoGRHostWidget.cpp");
            CopyOtherPluginFileToSource(ultraDirectory, "RococoGui", "UE5.GR.EventMarshalling.cpp");
            CopyOtherPluginFileToSource(ultraDirectory, "RococoGui", "UE5.GR.EventMarshalling.h");
            CopyOtherPluginFileToSource(ultraDirectory, "RococoGui", "UE5.GR-Custodian.cpp");

            CopyFileToSource(ultraDirectory, Path.Join(rococoSourceDirectory, "rococo/rococo.gui.retained"), "rococo.gr.image-loading.inl");

            CopyOtherPluginFileToHeader(ultraHeaderDirectory, "RococoUtil", "RococoBP_Util_Lib.h");
            CopyOtherPluginFileToHeader(ultraHeaderDirectory, "RococoGui", "GameOptionBuilder.h");
            CopyOtherPluginFileToHeader(ultraHeaderDirectory, "RococoGui", "ReflectedGameOptionsBuilder.h");
            CopyOtherPluginFileToHeader(ultraHeaderDirectory, "RococoGui", "RococoFontSet.h");
            CopyOtherPluginFileToHeader(ultraHeaderDirectory, "RococoGui", "RococoGRHostWidget.h");
            CopyOtherPluginFileToHeader(ultraHeaderDirectory, "RococoGui", "RococoGui_BPEvents.h");
            CopyOtherPluginFileToHeader(ultraHeaderDirectory, "RococoGui", "RococoGuiAPI.h");
            CopyOtherPluginFileToHeader(ultraHeaderDirectory, "RococoGui", "SlateRenderContext.h");
            CopyOtherPluginFileToHeader(ultraHeaderDirectory, "RococoGui", "SRococoGRHostWidget.h");

            CopyFilesToSource(ultraDirectory, Path.Join(thirdPartyPath, "zlib"), new List<string> 
                {
                    "zutil.h",
                    "deflate.h",
                    "inflate.h",
                    "zlib.h",
                    "zconf.h",
                    "crc32.h",
                    "inftrees.h",
                    "inffast.h",
                    "inffixed.h",
                    "trees.h",
                    "gzguts.h"
                }
            );

            CopyFilesToSource(ultraDirectory, Path.Join(thirdPartyPath, "libjpg\\jpeg-6b"), new List<string>
                {
                    "jconfig.h",
                    "jmorecfg.h",
                    "jpegint.h",
                    "jdct.h",
                    "jchuff.h",
                    "jdhuff.h",
                    "jversion.h",
                    "jmemsys.h",
                    "cdjpeg.h",
                    "cderror.h",
                    "transupp.h"
                }
            );

            CopyFilesToSource(ultraDirectory, Path.Join(rococoSourceDirectory, "rococo\\sexy\\SP\\sexy.s-parser"), new List<string>
                {
                     "sexy.s-parser.stdafx.h",
                     "sexy.s-parser.source.inl",
                     "sexy.s-parser.symbols.inl"
                }
            );

            CopyFilesToSource(ultraDirectory, Path.Join(rococoSourceDirectory, "rococo\\rococo.great.sex"), new List<string>
                {
                    "sexml.widgets.simple.inl"
                }
            );

            CopyFilesToSource(ultraDirectory, Path.Join(rococoSourceDirectory, "rococo\\rococo.util"), new List<string>
                {
                    "rococo.char16.inl",
                    "xxhash.hpp"
                }
             );
        } // RococoGuiUltra 
    }
}
