cc_library(
    name = "goo",
    srcs = [
        "xpdf-3.04/goo/FixedPoint.cc",
        "xpdf-3.04/goo/GHash.cc",
        "xpdf-3.04/goo/GList.cc",
        "xpdf-3.04/goo/GString.cc",
        "xpdf-3.04/goo/gfile.cc",
        "xpdf-3.04/goo/gmem.cc",
        "xpdf-3.04/goo/gmempp.cc",
        "xpdf-3.04/goo/parseargs.c",
    ],
    hdrs = [
        "xpdf-3.04/aconf.h",
        "xpdf-3.04/aconf2.h",
        "xpdf-3.04/goo/GHash.h",
        "xpdf-3.04/goo/GList.h",
        "xpdf-3.04/goo/GString.h",
        "xpdf-3.04/goo/gfile.h",
        "xpdf-3.04/goo/gmem.h",
        "xpdf-3.04/goo/gtypes.h",
        "xpdf-3.04/goo/parseargs.h",
    ],
    defines = ["HAVE_CONFIG_H"],
    includes = [
        "xpdf-3.04",
        "xpdf-3.04/goo",
    ],
)

cc_library(
    name = "fofi",
    srcs = [
        "xpdf-3.04/fofi/FoFiBase.cc",
        "xpdf-3.04/fofi/FoFiEncodings.cc",
        "xpdf-3.04/fofi/FoFiIdentifier.cc",
        "xpdf-3.04/fofi/FoFiTrueType.cc",
        "xpdf-3.04/fofi/FoFiType1.cc",
        "xpdf-3.04/fofi/FoFiType1C.cc",
    ],
    hdrs = [
        "xpdf-3.04/aconf.h",
        "xpdf-3.04/aconf2.h",
        "xpdf-3.04/fofi/FoFiBase.h",
        "xpdf-3.04/fofi/FoFiEncodings.h",
        "xpdf-3.04/fofi/FoFiIdentifier.h",
        "xpdf-3.04/fofi/FoFiTrueType.h",
        "xpdf-3.04/fofi/FoFiType1.h",
        "xpdf-3.04/fofi/FoFiType1C.h",
    ],
    defines = ["HAVE_CONFIG_H"],
    includes = [
        "xpdf-3.04",
        "xpdf-3.04/goo",
    ],
    deps = [
        ":goo",
    ],
)

cc_library(
    name = "splash",
    srcs = [
        "xpdf-3.04/splash/SplashBitmap.cc",
        "xpdf-3.04/splash/Splash.cc",
        "xpdf-3.04/splash/SplashClip.cc",
        "xpdf-3.04/splash/SplashFont.cc",
        "xpdf-3.04/splash/SplashFontEngine.cc",
        "xpdf-3.04/splash/SplashFontFile.cc",
        "xpdf-3.04/splash/SplashFontFileID.cc",
        "xpdf-3.04/splash/SplashFTFont.cc",
        "xpdf-3.04/splash/SplashFTFontEngine.cc",
        "xpdf-3.04/splash/SplashFTFontFile.cc",
        "xpdf-3.04/splash/SplashPath.cc",
        "xpdf-3.04/splash/SplashPattern.cc",
        "xpdf-3.04/splash/SplashScreen.cc",
        "xpdf-3.04/splash/SplashState.cc",
        "xpdf-3.04/splash/SplashXPath.cc",
        "xpdf-3.04/splash/SplashXPathScanner.cc",
    ],
    hdrs = [
        "xpdf-3.04/aconf.h",
        "xpdf-3.04/aconf2.h",
        "xpdf-3.04/splash/SplashBitmap.h",
        "xpdf-3.04/splash/SplashClip.h",
        "xpdf-3.04/splash/SplashErrorCodes.h",
        "xpdf-3.04/splash/SplashFontEngine.h",
        "xpdf-3.04/splash/SplashFontFile.h",
        "xpdf-3.04/splash/SplashFontFileID.h",
        "xpdf-3.04/splash/SplashFont.h",
        "xpdf-3.04/splash/SplashFTFontEngine.h",
        "xpdf-3.04/splash/SplashFTFontFile.h",
        "xpdf-3.04/splash/SplashFTFont.h",
        "xpdf-3.04/splash/SplashGlyphBitmap.h",
        "xpdf-3.04/splash/Splash.h",
        "xpdf-3.04/splash/SplashMath.h",
        "xpdf-3.04/splash/SplashPath.h",
        "xpdf-3.04/splash/SplashPattern.h",
        "xpdf-3.04/splash/SplashScreen.h",
        "xpdf-3.04/splash/SplashState.h",
        "xpdf-3.04/splash/SplashTypes.h",
        "xpdf-3.04/splash/SplashXPath.h",
        "xpdf-3.04/splash/SplashXPathScanner.h",
    ],
    defines = ["HAVE_CONFIG_H"],
    includes = [
        "xpdf-3.04",
        "xpdf-3.04/goo",
    ],
    deps = [
        ":goo",
    ],
)

cc_library(
    name = "xpdf",
    srcs = [
        "xpdf-3.04/xpdf/AcroForm.cc",
        "xpdf-3.04/xpdf/Annot.cc",
        "xpdf-3.04/xpdf/Array.cc",
        "xpdf-3.04/xpdf/BuiltinFont.cc",
        "xpdf-3.04/xpdf/BuiltinFontTables.cc",
        "xpdf-3.04/xpdf/CMap.cc",
        "xpdf-3.04/xpdf/Catalog.cc",
        "xpdf-3.04/xpdf/CharCodeToUnicode.cc",
        "xpdf-3.04/xpdf/CoreOutputDev.cc",
        "xpdf-3.04/xpdf/Decrypt.cc",
        "xpdf-3.04/xpdf/Dict.cc",
        "xpdf-3.04/xpdf/Error.cc",
        "xpdf-3.04/xpdf/Form.cc",
        "xpdf-3.04/xpdf/FontEncodingTables.cc",
        "xpdf-3.04/xpdf/Function.cc",
        "xpdf-3.04/xpdf/Gfx.cc",
        "xpdf-3.04/xpdf/GfxFont.cc",
        "xpdf-3.04/xpdf/GfxState.cc",
        "xpdf-3.04/xpdf/GlobalParams.cc",
        "xpdf-3.04/xpdf/ImageOutputDev.cc",
        "xpdf-3.04/xpdf/JArithmeticDecoder.cc",
        "xpdf-3.04/xpdf/JBIG2Stream.cc",
        "xpdf-3.04/xpdf/JPXStream.cc",
        "xpdf-3.04/xpdf/Lexer.cc",
        "xpdf-3.04/xpdf/Link.cc",
        "xpdf-3.04/xpdf/NameToCharCode.cc",
        "xpdf-3.04/xpdf/Object.cc",
        "xpdf-3.04/xpdf/OptionalContent.cc",
        "xpdf-3.04/xpdf/Outline.cc",
        "xpdf-3.04/xpdf/OutputDev.cc",
        "xpdf-3.04/xpdf/PDFCore.cc",
        "xpdf-3.04/xpdf/PDFDoc.cc",
        "xpdf-3.04/xpdf/PDFDocEncoding.cc",
        "xpdf-3.04/xpdf/PSTokenizer.cc",
        "xpdf-3.04/xpdf/Page.cc",
        "xpdf-3.04/xpdf/Parser.cc",
        "xpdf-3.04/xpdf/SecurityHandler.cc",
        "xpdf-3.04/xpdf/SplashOutputDev.cc",
        "xpdf-3.04/xpdf/Stream.cc",
        "xpdf-3.04/xpdf/TextOutputDev.cc",
        "xpdf-3.04/xpdf/TextString.cc",
        "xpdf-3.04/xpdf/UnicodeMap.cc",
        "xpdf-3.04/xpdf/UnicodeTypeTable.cc",
        "xpdf-3.04/xpdf/XRef.cc",
        "xpdf-3.04/xpdf/XFAForm.cc",
        "xpdf-3.04/xpdf/XpdfPluginAPI.cc",
        "xpdf-3.04/xpdf/Zoox.cc",
    ],
    hdrs = [
        "xpdf-3.04/aconf.h",
        "xpdf-3.04/aconf2.h",
        "xpdf-3.04/xpdf/AcroForm.h",
        "xpdf-3.04/xpdf/Annot.h",
        "xpdf-3.04/xpdf/Array.h",
        "xpdf-3.04/xpdf/BuiltinFont.h",
        "xpdf-3.04/xpdf/BuiltinFontTables.h",
        "xpdf-3.04/xpdf/CMap.h",
        "xpdf-3.04/xpdf/Catalog.h",
        "xpdf-3.04/xpdf/CharCodeToUnicode.h",
        "xpdf-3.04/xpdf/CharTypes.h",
        "xpdf-3.04/xpdf/CoreOutputDev.h",
        "xpdf-3.04/xpdf/Decrypt.h",
        "xpdf-3.04/xpdf/Dict.h",
        "xpdf-3.04/xpdf/Error.h",
        "xpdf-3.04/xpdf/ErrorCodes.h",
        "xpdf-3.04/xpdf/Form.h",
        "xpdf-3.04/xpdf/FontEncodingTables.h",
        "xpdf-3.04/xpdf/Function.h",
        "xpdf-3.04/xpdf/Gfx.h",
        "xpdf-3.04/xpdf/GfxFont.h",
        "xpdf-3.04/xpdf/GfxState.h",
        "xpdf-3.04/xpdf/GlobalParams.h",
        "xpdf-3.04/xpdf/ImageOutputDev.h",
        "xpdf-3.04/xpdf/JArithmeticDecoder.h",
        "xpdf-3.04/xpdf/JBIG2Stream.h",
        "xpdf-3.04/xpdf/JPXStream.h",
        "xpdf-3.04/xpdf/Lexer.h",
        "xpdf-3.04/xpdf/Link.h",
        "xpdf-3.04/xpdf/NameToCharCode.h",
        "xpdf-3.04/xpdf/NameToUnicodeTable.h",
        "xpdf-3.04/xpdf/Object.h",
        "xpdf-3.04/xpdf/OptionalContent.h",
        "xpdf-3.04/xpdf/Outline.h",
        "xpdf-3.04/xpdf/OutputDev.h",
        "xpdf-3.04/xpdf/PDFCore.h",
        "xpdf-3.04/xpdf/PDFDoc.h",
        "xpdf-3.04/xpdf/PDFDocEncoding.h",
        "xpdf-3.04/xpdf/PSTokenizer.h",
        "xpdf-3.04/xpdf/Page.h",
        "xpdf-3.04/xpdf/Parser.h",
        "xpdf-3.04/xpdf/PreScanOutputDev.h",
        "xpdf-3.04/xpdf/SecurityHandler.h",
        "xpdf-3.04/xpdf/SplashOutputDev.h",
        "xpdf-3.04/xpdf/Stream-CCITT.h",
        "xpdf-3.04/xpdf/Stream.h",
        "xpdf-3.04/xpdf/TextOutputDev.h",
        "xpdf-3.04/xpdf/TextString.h",
        "xpdf-3.04/xpdf/UTF8.h",
        "xpdf-3.04/xpdf/UnicodeMap.h",
        "xpdf-3.04/xpdf/UnicodeMapTables.h",
        "xpdf-3.04/xpdf/UnicodeTypeTable.h",
        "xpdf-3.04/xpdf/XRef.h",
        "xpdf-3.04/xpdf/XFAForm.h",
        "xpdf-3.04/xpdf/XpdfPluginAPI.h",
        "xpdf-3.04/xpdf/Zoox.h",
        "xpdf-3.04/xpdf/config.h",
    ],
    defines = ["HAVE_CONFIG_H"],
    includes = [
        "xpdf-3.04",
        "xpdf-3.04/fofi",
        "xpdf-3.04/goo",
        "xpdf-3.04/splash",
        "xpdf-3.04/xpdf",
    ],
    deps = [
        ":goo",
        ":fofi",
        ":splash",
    ],
    copts = [
      "-Wno-unused-but-set-variable",
    ],
    visibility = ["//visibility:public"],
)

# Use the default config.
genrule(
    name = "generate_config",
    srcs = ["xpdf-3.04/aconf.h.in"],
    outs = ["xpdf-3.04/aconf.h"],
    cmd = "sed -e 's/#undef \\(HAVE_DIRENT_H\\)$$/#define \\1 1/'" +
          " $< > $@",
)
