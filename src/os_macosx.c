/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * os_macosx.c -- Mac specific things for Mac OS/X.
 */

#ifdef MACOS_X_UNIX
# ifdef HAVE_CONFIG_H	    /* Using Makefile. */
#  include "vim.h"
# else
#  include "os_unix.c"	    /* Using Project Builder */
# endif
#else
    Error: MACOS 9 is no longer supported in Vim 7
#endif

#ifdef _DEBUG
    void
Trace(char* fmt, ...)
{
    char buf[2048];
    va_list args;

    va_start(args, fmt);
    /* vsnprintf(buf, sizeof(buf), fmt, args);*/
    fprintf(stderr, "%s", buf);
    va_end(args);
}
#endif

#ifdef MACOS_X_ICONVEMU
/*
 * Libiconv emulation layer
 */

struct _iconv_t
{
    TECObjectRef tec;
    TECObjectRef tecReverse;
    TECSnifferObjectRef sniff;
    TextEncoding from;
    TextEncoding to;
};
/* typedef struct _iconv_t *iconv_t; */


static int last_errno = 0;

/*
 * Get TextEncoding from iconv's encoding name
 */
    static TextEncoding
get_textencoding(const char* encodename)
{
    static struct {
	const char* name;
	TextEncoding encode;
    } encodetable[] = {
	/* ISO-8859 encodings family */
	{"latin1", kTextEncodingISOLatin1},
	{"latin2", kTextEncodingISOLatin2},
	{"latin3", kTextEncodingISOLatin3},
	{"latin4", kTextEncodingISOLatin4},
	{"latin5", kTextEncodingISOLatin5},
	{"latin6", kTextEncodingISOLatin6},
	{"latin7", kTextEncodingISOLatin7},
	{"latin8", kTextEncodingISOLatin8},
	{"latin9", kTextEncodingISOLatin9},
	{"iso-8859-1", kTextEncodingISOLatin1},
	{"iso-8859-2", kTextEncodingISOLatin2},
	{"iso-8859-3", kTextEncodingISOLatin3},
	{"iso-8859-4", kTextEncodingISOLatin4},
	{"iso-8859-5", kTextEncodingISOLatinCyrillic},
	{"iso-8859-6", kTextEncodingISOLatinArabic},
	{"iso-8859-7", kTextEncodingISOLatinGreek},
	{"iso-8859-8", kTextEncodingISOLatinHebrew},
	{"iso-8859-9", kTextEncodingISOLatin5},
	{"iso-8859-10", kTextEncodingISOLatin6},
	{"iso-8859-15", kTextEncodingISOLatin9},

	/* Unicode encodings. */
	/* TODO: Add other type of unicode */
	{"ucs-2", kTextEncodingMacUnicode},

	/* Japanese encoding aliases */
	{"cp932", kTextEncodingShiftJIS},
	{"shift-jis", kTextEncodingShiftJIS},
	{"euc-jp", kTextEncodingEUC_JP},
	{"iso-2022-jp", kTextEncodingISO_2022_JP},
	{"iso-2022-jp-1", kTextEncodingISO_2022_JP_1},
	{"iso-2022-jp-2", kTextEncodingISO_2022_JP_2},
	{"iso-2022-jp-3", kTextEncodingISO_2022_JP_3},

	/* Other aliases. These aliases in this block are just guessed. */
	/* TODO: Must be verified. */
	{"gb2312", kTextEncodingGB_2312_80},
	{"cp936", kTextEncodingMacChineseSimp},
	{"euc-cn", kTextEncodingEUC_CN},
	{"cp950", kTextEncodingMacChineseTrad},
	{"euc-tw", kTextEncodingEUC_TW},
	{"cp949", kTextEncodingMacKorean},
	{"euc-kr", kTextEncodingEUC_KR},

	/*
	 * All encodings supported by Macintosh.  You can find these values
	 * in a file:
	 *  /System/Library/Frameworks/CoreServices.framework/Versions/A/
	 *    Frameworks/CarbonCore.framework/Versions/A/Headers/TextCommon.h
	 */
	{"MacRoman", kTextEncodingMacRoman},
	{"MacJapanese", kTextEncodingMacJapanese},
	{"MacChineseTrad", kTextEncodingMacChineseTrad},
	{"MacKorean", kTextEncodingMacKorean},
	{"MacArabic", kTextEncodingMacArabic},
	{"MacHebrew", kTextEncodingMacHebrew},
	{"MacGreek", kTextEncodingMacGreek},
	{"MacCyrillic", kTextEncodingMacCyrillic},
	{"MacDevanagari", kTextEncodingMacDevanagari},
	{"MacGurmukhi", kTextEncodingMacGurmukhi},
	{"MacGujarati", kTextEncodingMacGujarati},
	{"MacOriya", kTextEncodingMacOriya},
	{"MacBengali", kTextEncodingMacBengali},
	{"MacTamil", kTextEncodingMacTamil},
	{"MacTelugu", kTextEncodingMacTelugu},
	{"MacKannada", kTextEncodingMacKannada},
	{"MacMalayalam", kTextEncodingMacMalayalam},
	{"MacSinhalese", kTextEncodingMacSinhalese},
	{"MacBurmese", kTextEncodingMacBurmese},
	{"MacKhmer", kTextEncodingMacKhmer},
	{"MacThai", kTextEncodingMacThai},
	{"MacLaotian", kTextEncodingMacLaotian},
	{"MacGeorgian", kTextEncodingMacGeorgian},
	{"MacArmenian", kTextEncodingMacArmenian},
	{"MacChineseSimp", kTextEncodingMacChineseSimp},
	{"MacTibetan", kTextEncodingMacTibetan},
	{"MacMongolian", kTextEncodingMacMongolian},
	{"MacEthiopic", kTextEncodingMacEthiopic},
	{"MacCentralEurRoman", kTextEncodingMacCentralEurRoman},
	{"MacVietnamese", kTextEncodingMacVietnamese},
	{"MacExtArabic", kTextEncodingMacExtArabic},
	{"MacSymbol", kTextEncodingMacSymbol},
	{"MacDingbats", kTextEncodingMacDingbats},
	{"MacTurkish", kTextEncodingMacTurkish},
	{"MacCroatian", kTextEncodingMacCroatian},
	{"MacIcelandic", kTextEncodingMacIcelandic},
	{"MacRomanian", kTextEncodingMacRomanian},
	{"MacCeltic", kTextEncodingMacCeltic},
	{"MacGaelic", kTextEncodingMacGaelic},
	{"MacKeyboardGlyphs", kTextEncodingMacKeyboardGlyphs},
	{"MacTradChinese", kTextEncodingMacTradChinese},
	{"MacRSymbol", kTextEncodingMacRSymbol},
	{"MacSimpChinese", kTextEncodingMacSimpChinese},
	{"MacGeez", kTextEncodingMacGeez},
	{"MacEastEurRoman", kTextEncodingMacEastEurRoman},
	{"MacUninterp", kTextEncodingMacUninterp},
	{"MacUnicode", kTextEncodingMacUnicode},
	{"MacFarsi", kTextEncodingMacFarsi},
	{"MacUkrainian", kTextEncodingMacUkrainian},
	{"MacInuit", kTextEncodingMacInuit},
	{"MacVT100", kTextEncodingMacVT100},
	{"MacHFS", kTextEncodingMacHFS},
	{"UnicodeDefault", kTextEncodingUnicodeDefault},
	{"UnicodeV1_1", kTextEncodingUnicodeV1_1},
	{"ISO10646_1993", kTextEncodingISO10646_1993},
	{"UnicodeV2_0", kTextEncodingUnicodeV2_0},
	{"UnicodeV2_1", kTextEncodingUnicodeV2_1},
	{"UnicodeV3_0", kTextEncodingUnicodeV3_0},
	{"UnicodeV3_1", kTextEncodingUnicodeV3_1},
	{"UnicodeV3_2", kTextEncodingUnicodeV3_2},
	{"ISOLatin1", kTextEncodingISOLatin1},
	{"ISOLatin2", kTextEncodingISOLatin2},
	{"ISOLatin3", kTextEncodingISOLatin3},
	{"ISOLatin4", kTextEncodingISOLatin4},
	{"ISOLatinCyrillic", kTextEncodingISOLatinCyrillic},
	{"ISOLatinArabic", kTextEncodingISOLatinArabic},
	{"ISOLatinGreek", kTextEncodingISOLatinGreek},
	{"ISOLatinHebrew", kTextEncodingISOLatinHebrew},
	{"ISOLatin5", kTextEncodingISOLatin5},
	{"ISOLatin6", kTextEncodingISOLatin6},
	{"ISOLatin7", kTextEncodingISOLatin7},
	{"ISOLatin8", kTextEncodingISOLatin8},
	{"ISOLatin9", kTextEncodingISOLatin9},
	{"DOSLatinUS", kTextEncodingDOSLatinUS},
	{"DOSGreek", kTextEncodingDOSGreek},
	{"DOSBalticRim", kTextEncodingDOSBalticRim},
	{"DOSLatin1", kTextEncodingDOSLatin1},
	{"DOSGreek1", kTextEncodingDOSGreek1},
	{"DOSLatin2", kTextEncodingDOSLatin2},
	{"DOSCyrillic", kTextEncodingDOSCyrillic},
	{"DOSTurkish", kTextEncodingDOSTurkish},
	{"DOSPortuguese", kTextEncodingDOSPortuguese},
	{"DOSIcelandic", kTextEncodingDOSIcelandic},
	{"DOSHebrew", kTextEncodingDOSHebrew},
	{"DOSCanadianFrench", kTextEncodingDOSCanadianFrench},
	{"DOSArabic", kTextEncodingDOSArabic},
	{"DOSNordic", kTextEncodingDOSNordic},
	{"DOSRussian", kTextEncodingDOSRussian},
	{"DOSGreek2", kTextEncodingDOSGreek2},
	{"DOSThai", kTextEncodingDOSThai},
	{"DOSJapanese", kTextEncodingDOSJapanese},
	{"DOSChineseSimplif", kTextEncodingDOSChineseSimplif},
	{"DOSKorean", kTextEncodingDOSKorean},
	{"DOSChineseTrad", kTextEncodingDOSChineseTrad},
	{"WindowsLatin1", kTextEncodingWindowsLatin1},
	{"WindowsANSI", kTextEncodingWindowsANSI},
	{"WindowsLatin2", kTextEncodingWindowsLatin2},
	{"WindowsCyrillic", kTextEncodingWindowsCyrillic},
	{"WindowsGreek", kTextEncodingWindowsGreek},
	{"WindowsLatin5", kTextEncodingWindowsLatin5},
	{"WindowsHebrew", kTextEncodingWindowsHebrew},
	{"WindowsArabic", kTextEncodingWindowsArabic},
	{"WindowsBalticRim", kTextEncodingWindowsBalticRim},
	{"WindowsVietnamese", kTextEncodingWindowsVietnamese},
	{"WindowsKoreanJohab", kTextEncodingWindowsKoreanJohab},
	{"US_ASCII", kTextEncodingUS_ASCII},
	{"JIS_X0201_76", kTextEncodingJIS_X0201_76},
	{"JIS_X0208_83", kTextEncodingJIS_X0208_83},
	{"JIS_X0208_90", kTextEncodingJIS_X0208_90},
	{"JIS_X0212_90", kTextEncodingJIS_X0212_90},
	{"JIS_C6226_78", kTextEncodingJIS_C6226_78},
	{"ShiftJIS_X0213_00", kTextEncodingShiftJIS_X0213_00},
	{"GB_2312_80", kTextEncodingGB_2312_80},
	{"GBK_95", kTextEncodingGBK_95},
	{"GB_18030_2000", kTextEncodingGB_18030_2000},
	{"KSC_5601_87", kTextEncodingKSC_5601_87},
	{"KSC_5601_92_Johab", kTextEncodingKSC_5601_92_Johab},
	{"CNS_11643_92_P1", kTextEncodingCNS_11643_92_P1},
	{"CNS_11643_92_P2", kTextEncodingCNS_11643_92_P2},
	{"CNS_11643_92_P3", kTextEncodingCNS_11643_92_P3},
	{"ISO_2022_JP", kTextEncodingISO_2022_JP},
	{"ISO_2022_JP_2", kTextEncodingISO_2022_JP_2},
	{"ISO_2022_JP_1", kTextEncodingISO_2022_JP_1},
	{"ISO_2022_JP_3", kTextEncodingISO_2022_JP_3},
	{"ISO_2022_CN", kTextEncodingISO_2022_CN},
	{"ISO_2022_CN_EXT", kTextEncodingISO_2022_CN_EXT},
	{"ISO_2022_KR", kTextEncodingISO_2022_KR},
	{"EUC_JP", kTextEncodingEUC_JP},
	{"EUC_CN", kTextEncodingEUC_CN},
	{"EUC_TW", kTextEncodingEUC_TW},
	{"EUC_KR", kTextEncodingEUC_KR},
	{"ShiftJIS", kTextEncodingShiftJIS},
	{"KOI8_R", kTextEncodingKOI8_R},
	{"Big5", kTextEncodingBig5},
	{"MacRomanLatin1", kTextEncodingMacRomanLatin1},
	{"HZ_GB_2312", kTextEncodingHZ_GB_2312},
	{"Big5_HKSCS_1999", kTextEncodingBig5_HKSCS_1999},
	{"NextStepLatin", kTextEncodingNextStepLatin},
	{"EBCDIC_US", kTextEncodingEBCDIC_US},
	{"EBCDIC_CP037", kTextEncodingEBCDIC_CP037},
	{"MultiRun", kTextEncodingMultiRun},

	/* Terminator */
	{NULL, -1},
    };
    int i;

    i = 0;
    for (i = 0; encodetable[i].name != NULL; ++i)
    {
	if (STRICMP(encodename, encodetable[i].name) == 0)
	    break;
    }
    return encodetable[i].encode;
}

/*
 * iconv interfaces
 */

    iconv_t
iconv_open(const char* tocode, const char* fromcode)
{
    TextEncoding toEnc, fromEnc;
    iconv_t cd = NULL;
    OSStatus st;

    /* Verify to/from encoding name */
    toEnc = get_textencoding(tocode);
    fromEnc = get_textencoding(fromcode);
    if (toEnc < 0 || fromEnc < 0)
	goto ICONV_OPEN_ERR;

    /* Allocate memory to object */
    cd = (iconv_t)alloc(sizeof(struct _iconv_t));
    if (!cd)
	goto ICONV_OPEN_ERR;
    memset(cd, 0, sizeof(struct _iconv_t));

    /* Create converter */
    if (fromEnc != toEnc)
    {
	TRACE("*** fromEnc=%d toEnc=%d\n", (int)fromEnc, (int)toEnc);
	st = TECCreateConverter(&cd->tec, fromEnc, toEnc);
	if (st != 0)
	{
	    TRACE("*** TECCreateConverter()=%d\n", (int)st);
	    goto ICONV_OPEN_ERR;
	}
	/* Create reverse converter */
	st = TECCreateConverter(&cd->tecReverse, toEnc, fromEnc);
	if (st != 0)
	{
	    TRACE("*** TECCreateConverter()=%d (reverse)\n", (int)st);
	    goto ICONV_OPEN_ERR;
	}
	/* Create Sniffer */
	st = TECCreateSniffer(&cd->sniff, &fromEnc, 1);
	if (st != 0)
	{
	    TRACE("*** TECCreateSniffer()=%d\n", (int)st);
	    goto ICONV_OPEN_ERR;
	}
    }

    cd->from = fromEnc;
    cd->to = toEnc;
    last_errno = 0;
    return cd;

ICONV_OPEN_ERR:
    if (cd)
	iconv_close(cd);
    last_errno = EINVAL;
    return (iconv_t)-1;
}

/*
 * Used when there are same value in 'from encoding' and 'to encoding'.
 * TEC doesn't support conversion between same encodings, and
 * TECCreateConverter() failed.
 */
    static size_t
null_conv(iconv_t cd, const char **inbuf, size_t *inbytesleft,
	char **outbuf, size_t *outbytesleft)
{
    const char* buf_in = inbuf && *inbuf ? *inbuf : NULL;
    char* buf_out = outbuf && *outbuf ? *outbuf : NULL;

    if (buf_in)
    {
	int in_len = inbytesleft ? *inbytesleft : 0;
	int out_len = outbytesleft ? *outbytesleft : 0;

	if (!buf_out || out_len <= 0)
	{
	    last_errno = E2BIG;
	    return -1;
	}
	else if (in_len > 0)
	{
	    int len = in_len < out_len ? in_len : out_len;

	    memcpy (buf_out, buf_in, len);
	    *inbuf += len;
	    *outbuf += len;
	    *inbytesleft -= len;
	    *outbytesleft -= len;
	    if (*outbytesleft <= 0)
	    {
		last_errno = E2BIG;
		return -1;
	    }
	}
    }
    last_errno = 0;
    return 0;
}

    size_t
iconv(iconv_t cd, const char **inbuf, size_t *inbytesleft,
	char **outbuf, size_t *outbytesleft)
{
    ConstTextPtr    buf_in;
    TextPtr	    buf_out;
    ByteCount	    out_len, out_true;
    ByteCount	    in_len, in_true;
    OSStatus	    st;

    if (!cd)
    {
	last_errno = ENOENT; /* TODO: Another error code should be set */
	return -1;
    }
    if (cd->from == cd->to)
	return null_conv(cd, inbuf, inbytesleft, outbuf, outbytesleft) ;

    buf_in = (TextPtr) inbuf ;
    buf_out = (TextPtr) outbuf ;
    out_len = out_true = -1;
    in_len = in_true = -1;

    if (buf_in && buf_out)
    {
	ItemCount error, feature;

	/* Normal convert mode */
	if (!inbytesleft || !outbytesleft)
	{
	    last_errno = EFAULT;
	    return -1;
	}
	in_len = *inbytesleft;
	out_len = *outbytesleft;

	/* Check stream is form in expected encoding or not */
	st = TECSniffTextEncoding(cd->sniff, (TextPtr)buf_in, in_len,
		&cd->from, 1, &error, 1, &feature, 1);
	TRACE("TECSniffTextEncoding()=%d error=%d feature=%d\n",
		(int)st, (int)error, (int)feature);
	if ((error != 0 || feature == 0)
		&& !(error == 0xffffffff && feature == 0xffffffff))
	    /* Not expected encoding */
	    st = kTECUnmappableElementErr;
	else
	{
	    /* Do convert */
	    st = TECConvertText(cd->tec,
		    buf_in, in_len, &in_true,
		    buf_out, out_len, &out_true);
	    /* Verify converted text.  Compare original text with reverse
	     * converted text.  If not match, there is some problem on
	     * converting. */
	    if (st == 0 && in_true > 0)
	    {
		ByteCount rev_in, rev_out;
		TextPtr buf_rev = (TextPtr)alloc(in_true);

		if (buf_rev)
		{
		    st = TECConvertText(cd->tecReverse,
			    buf_out, out_true, &rev_in,
			    buf_rev, in_true, &rev_out);
		    if (st != 0 || rev_in != out_true || rev_out != in_true
			    || memcmp(buf_rev, buf_in, rev_out) != 0)
		    {
#ifdef ICONVOSX_DEBUG
			fprintf(stderr, "  reverse conversion failed.\n");
#endif
			st = kTECUnmappableElementErr;
		    }
		    vim_free(buf_rev);
		}
		else
		    st = kTECUnmappableElementErr;
	    }
	}
    }
    else if (!buf_in && buf_out)
    {
	/* Flush all buffered strings to buffer, and reset status */
	if (!outbytesleft)
	{
	    last_errno = EFAULT;
	    return -1;
	}
	out_len = *outbytesleft;
	st = TECFlushText(cd->tec,
		buf_out, out_len, &out_true);
    }
    else if (!buf_in && !buf_out)
    {
	/* Reset cd's status and cancel buffered strings */
	unsigned char tmp_out[256];

	buf_out = tmp_out;
	out_len = sizeof(tmp_out);
	st = TECFlushText(cd->tec,
		buf_out, out_len, &out_true);
    }
    else
    {
	last_errno = EFAULT;
	return -1;
    }
    TRACE("st=%d, buf_in=%p, in_len=%d, in_true=%d\n"
	    "  buf_out=%p, out_len=%d, out_true=%d\n", (int)st,
	    buf_in, (int)in_len, (int)in_true,
	    buf_out, (int)out_len, (int)out_true);

    switch (st)
    {
	case 0:
	    /* No error */
	    if (inbytesleft)
		*inbytesleft -= in_true;
	    if (outbytesleft)
		*outbytesleft -= out_true;
	    if (inbuf && *inbuf)
		*inbuf += in_true;
	    if (outbuf && *outbuf)
		*outbuf += out_true;
	    last_errno = 0;
	    return 0; /* No error */
	case kTECUnmappableElementErr:
	    last_errno = EILSEQ;
	case kTECIncompleteElementErr:
	    last_errno = EINVAL;
	case kTECOutputBufferFullStatus:
	    last_errno = E2BIG;
	    return -1;
	default:
	    TRACE("iconv(%p, %p, %p, %p, %p) failed. (%d)\n",
		    cd, inbuf, inbytesleft, outbuf, outbytesleft, (int)st);
	    last_errno = EFAULT;
	    return -1;
    }
}

    int
iconv_close(iconv_t cd)
{
    if (cd)
    {
	/* Free all elements of iconv_t */
	if (cd->tec)
	    TECDisposeConverter(cd->tec);
	if (cd->tecReverse)
	    TECDisposeConverter(cd->tecReverse);
	if (cd->sniff)
	    TECDisposeSniffer(cd->sniff);
	vim_free(cd);
	last_errno = 0;
	return 0;
    }
    else
    {
	last_errno = EINVAL;
	return -1;
    }
}

    int *
iconv_errno()
{
    return &last_errno;
}
#endif /* MACOS_X_ICONVEMU */

#ifdef USE_MCH_GETTEXT

#define GETTEXT_BUFNUM		64
#define GETTEXT_BUFSIZE		256

    char *
mch_gettext(const char *msgid)
{
    static char		buf[GETTEXT_BUFNUM][GETTEXT_BUFSIZE];
    static int		bufnum = 0;
    const char		*msg = NULL;
    CFStringRef		strkey = NULL, strmsg = NULL;
    CFStringEncoding	enc;

    if (!msgid)
	goto MCH_GETTEXT_FINISH;
    enc = CFStringGetSystemEncoding();
    TRACE("mch_gettext(%s)\n", msgid);

    strkey = CFStringCreateWithCString(NULL, msgid, enc);
    if (!strkey)
    {
	TRACE("  Can't create a CFString for msgid.\n");
	goto MCH_GETTEXT_FINISH;
    }

    strmsg = CFCopyLocalizedString(strkey, NULL);
    if (!strmsg)
    {
	TRACE("  No localized strings for msgid.\n");
	goto MCH_GETTEXT_FINISH;
    }

    msg = CFStringGetCStringPtr(strmsg, enc);
    if (!msg)
    {
	/* This is as backup when CFStringGetCStringPtr was failed */
	CFStringGetCString(strmsg, buf[bufnum], GETTEXT_BUFSIZE, enc);
	msg = buf[bufnum];
	if (++bufnum >= GETTEXT_BUFNUM)
	    bufnum = 0;
    }
    TRACE("  Localized to: %s\n", msg);

MCH_GETTEXT_FINISH:
    if (strkey)
	CFRelease(strkey);
    if (strmsg)
	CFRelease(strmsg);
    return (char *)(msg ? msg : msgid);
}

    char *
mch_bindtextdomain(const char *domain, const char *dirname)
{
    TRACE("mch_bindtextdomain(%s, %s)\n", domain, dirname);
    return (char*)dirname;
}

    char *
mch_textdomain(const char *domain)
{
    TRACE("mch_textdomain(%s)\n", domain);
    return (char*)domain;
}
#endif
