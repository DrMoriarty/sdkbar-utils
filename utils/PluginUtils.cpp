#include "PluginUtils.h"
#include "base/ccUTF8.h"

int CallbackFrame::nextId = 0;
std::vector<CallbackFrame*> CallbackFrame::callbackVector;

template <typename T>
struct ConvertTrait {
    typedef T ArgType;
};
template <>
struct ConvertTrait<char> {
    typedef UTF8 ArgType;
};
template <>
struct ConvertTrait<char16_t> {
    typedef UTF16 ArgType;
};
template <>
struct ConvertTrait<char32_t> {
    typedef UTF32 ArgType;
};

static const int halfShift  = 10; /* used for shifting by 10 bits */

static const UTF32 halfBase = 0x0010000UL;
static const UTF32 halfMask = 0x3FFUL;

#define UNI_SUR_HIGH_START  (UTF32)0xD800
#define UNI_SUR_HIGH_END    (UTF32)0xDBFF
#define UNI_SUR_LOW_START   (UTF32)0xDC00
#define UNI_SUR_LOW_END     (UTF32)0xDFFF

static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const UTF32 offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 
                     0x03C82080UL, 0xFA082080UL, 0x82082080UL };

static Boolean isLegalUTF8(const UTF8 *source, int length) {
    UTF8 a;
    const UTF8 *srcptr = source+length;
    switch (length) {
    default: return false;
        /* Everything else falls through when "true"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;

        switch (*source) {
            /* no fall-through in this inner switch */
            case 0xE0: if (a < 0xA0) return false; break;
            case 0xED: if (a > 0x9F) return false; break;
            case 0xF0: if (a < 0x90) return false; break;
            case 0xF4: if (a > 0x8F) return false; break;
            default:   if (a < 0x80) return false;
        }

    case 1: if (*source >= 0x80 && *source < 0xC2) return false;
    }
    if (*source > 0xF4) return false;
    return true;
}

int checkUTF8LegalSymbols(const UTF8* sourceStart, const UTF8* sourceEnd) {
    const UTF8* source = sourceStart;
    int counter = 0;
    while (source < sourceEnd) {
        unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
        if (extraBytesToRead >= sourceEnd - source) {
            return counter;
        }
        if (!isLegalUTF8(source, extraBytesToRead+1)) {
            return counter;
        }
        counter += extraBytesToRead + 1;
        source += extraBytesToRead + 1;
    }
    return -1;
}

// ED A0 BC
std::string printUTF8Code(const UTF8* source) {
    unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
    std::stringstream ss;
    for(int i=0; i<=extraBytesToRead; i++) {
        ss << (int)(*source) << " ";
        source++;
    }
    return ss.str();
}

ConversionResult myConvertUTF8toUTF16 (
        const UTF8** sourceStart, const UTF8* sourceEnd, 
        UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags) {
    ConversionResult result = conversionOK;
    const UTF8* source = *sourceStart;
    UTF16* target = *targetStart;
    while (source < sourceEnd) {
        UTF32 ch = 0;
        unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
        if (extraBytesToRead >= sourceEnd - source) {
            result = sourceExhausted; break;
        }
        /* Do this check whether lenient or strict */
        if (!isLegalUTF8(source, extraBytesToRead+1)) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                break;
            } else {
                //*target++ = UNI_REPLACEMENT_CHAR;
                source += extraBytesToRead+1;
                continue;
            }
        }
        /*
         * The cases all fall through. See "Note A" below.
         */
        switch (extraBytesToRead) {
            case 5: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
            case 4: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
            case 3: ch += *source++; ch <<= 6;
            case 2: ch += *source++; ch <<= 6;
            case 1: ch += *source++; ch <<= 6;
            case 0: ch += *source++;
        }
        ch -= offsetsFromUTF8[extraBytesToRead];

        if (target >= targetEnd) {
            source -= (extraBytesToRead+1); /* Back up source pointer! */
            result = targetExhausted; break;
        }
        if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
            /* UTF-16 surrogate values are illegal in UTF-32 */
            if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
                if (flags == strictConversion) {
                    source -= (extraBytesToRead+1); /* return to the illegal value itself */
                    result = sourceIllegal;
                    break;
                } else {
                    //*target++ = UNI_REPLACEMENT_CHAR;
                }
            } else {
                *target++ = (UTF16)ch; /* normal case */
            }
        } else if (ch > UNI_MAX_UTF16) {
            if (flags == strictConversion) {
                result = sourceIllegal;
                source -= (extraBytesToRead+1); /* return to the start */
                break; /* Bail out; shouldn't continue */
            } else {
                //*target++ = UNI_REPLACEMENT_CHAR;
            }
        } else {
            /* target is a character in range 0xFFFF - 0x10FFFF. */
            if (target + 1 >= targetEnd) {
                source -= (extraBytesToRead+1); /* Back up source pointer! */
                result = targetExhausted; break;
            }
            ch -= halfBase;
            *target++ = (UTF16)((ch >> halfShift) + UNI_SUR_HIGH_START);
            *target++ = (UTF16)((ch & halfMask) + UNI_SUR_LOW_START);
        }
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

template <typename From, typename To, typename FromTrait = ConvertTrait<From>, typename ToTrait = ConvertTrait<To>>
ConversionResult myUtfConvert(
    const std::basic_string<From>& from, std::basic_string<To>& to,
    ConversionResult(*cvtfunc)(const typename FromTrait::ArgType**, const typename FromTrait::ArgType*,
        typename ToTrait::ArgType**, typename ToTrait::ArgType*,
        ConversionFlags)
    )
{
    static_assert(sizeof(From) == sizeof(typename FromTrait::ArgType), "Error size mismatched");
    static_assert(sizeof(To) == sizeof(typename ToTrait::ArgType), "Error size mismatched");

    if (from.empty())
    {
        to.clear();
        return conversionOK;
    }

    // See: http://unicode.org/faq/utf_bom.html#gen6
    static const int most_bytes_per_character = 4;

    const size_t maxNumberOfChars = from.length(); // all UTFs at most one element represents one character.
    const size_t numberOfOut = maxNumberOfChars * most_bytes_per_character / sizeof(To);

    std::basic_string<To> working(numberOfOut, 0);

    auto inbeg = reinterpret_cast<const typename FromTrait::ArgType*>(&from[0]);
    auto inend = inbeg + from.length();


    auto outbeg = reinterpret_cast<typename ToTrait::ArgType*>(&working[0]);
    auto outend = outbeg + working.length();
    auto r = cvtfunc(&inbeg, inend, &outbeg, outend, lenientConversion);
    if (r != conversionOK)
        return r;

    working.resize(reinterpret_cast<To*>(outbeg) - &working[0]);
    to = std::move(working);

    return conversionOK;
};


ConversionResult myUTF8ToUTF16(const std::string& utf8, std::u16string& outUtf16)
{
    return myUtfConvert(utf8, outUtf16, myConvertUTF8toUTF16);
}

jsval my_c_string_to_jsval(JSContext* cx, const char* v, size_t length /* = -1 */)
{
    if (v == NULL)
    {
        return JSVAL_NULL;
    }
    if (length == -1)
    {
        length = strlen(v);
    }

    JSB_AUTOCOMPARTMENT_WITH_GLOBAL_OBJCET

    if (0 == length)
    {
        auto emptyStr = JS_NewStringCopyZ(cx, "");
        return STRING_TO_JSVAL(emptyStr);
    }

    jsval ret = JSVAL_NULL;

    std::u16string strUTF16;
    bool ok = myUTF8ToUTF16(std::string(v, length), strUTF16) == conversionOK;

    if (ok && !strUTF16.empty()) {
        JSString* str = JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar*>(strUTF16.data()), strUTF16.size());
        if (str) {
            ret = STRING_TO_JSVAL(str);
        }
    }

    return ret;
}

jsval my_std_string_to_jsval(JSContext* cx, const std::string& v)
{
    return my_c_string_to_jsval(cx, v.c_str(), v.size());
}



static UTF32 ReplaceIfInvalid(UTF32 u, Status* err)
{
    // disallow surrogates
    if(0xD800ul <= u && u <= 0xDFFFul)
        return 0;//RaiseError(ERR::UTF8_SURROGATE, err);
    // outside BMP (UTF-16 representation would require surrogates)
    if(u > 0xFFFFul)
        return 0;//RaiseError(ERR::UTF8_OUTSIDE_BMP, err);
    // noncharacter (note: WEOF (0xFFFF) causes VC's swprintf to fail)
    if(u == 0xFFFEul || u == 0xFFFFul || (0xFDD0ul <= u && u <= 0xFDEFul))
        return 0;//RaiseError(ERR::UTF8_NONCHARACTER, err);
    return u;
}

class UTF8Codec
{
public:
    static void EncodeFrom32(UTF32 u, UTF8*& dstPos)
    {
        switch (Size(u))
        {
        case 1:
            *dstPos++ = UTF8(u);
            break;
        case 2:
            *dstPos++ = UTF8((u >> 6) | 0xC0);
            *dstPos++ = UTF8((u | 0x80u) & 0xBFu);
            break;
        case 3:
            *dstPos++ = UTF8((u >> 12) | 0xE0);
            *dstPos++ = UTF8(((u >> 6) | 0x80u) & 0xBFu);
            *dstPos++ = UTF8((u | 0x80u) & 0xBFu);
            break;
        }
    }

    // @return decoded scalar, or replacementCharacter on error                                                                                                        
    static UTF32 DecodeTo32(const UTF8*& srcPos, const UTF8* const srcEnd, Status* err)
    {
        const size_t size = SizeFromFirstByte(*srcPos);
        if(!IsValid(srcPos, size, srcEnd))
        {
            srcPos += 1;    // only skip the offending byte (increases chances of resynchronization)                                                                   
            return 0;//RaiseError(ERR::UTF8_INVALID_UTF8, err);
        }

        UTF32 u = 0;
        for(size_t i = 0; i < size-1; i++)
        {
            u += UTF32(*srcPos++);
            u <<= 6;
        }
        u += UTF32(*srcPos++);

        static const UTF32 offsets[1+4] = { 0, 0x00000000ul, 0x00003080ul, 0x000E2080ul, 0x03C82080UL };
        u -= offsets[size];
        return u;
    }
private:                                                                                                                                                               
    static inline size_t Size(UTF32 u)
    {
        if(u < 0x80)
            return 1;
        if(u < 0x800)
            return 2;
        // ReplaceIfInvalid ensures > 3 byte encodings are never used.                                                                                                 
        return 3;
    }

    static inline size_t SizeFromFirstByte(UTF8 firstByte)
    {
        if(firstByte < 0xC0)
            return 1;
        if(firstByte < 0xE0)
            return 2;
        if(firstByte < 0xF0)
            return 3;
        // IsValid rejects firstByte values that would cause > 4 byte encodings.                                                                                       
        return 4;
    }

    // c.f. Unicode 3.1 Table 3-7                                                                                                                                      
    // @param size obtained via SizeFromFirstByte (our caller also uses it)                                                                                            
    static bool IsValid(const UTF8* const src, size_t size, const UTF8* const srcEnd)
    {
        if(src+size > srcEnd)   // not enough data                                                                                                                     
            return false;

        if(src[0] < 0x80)
            return true;
        if(!(0xC2 <= src[0] && src[0] <= 0xF4))
            return false;

        // special cases (stricter than the loop)                                                                                                                      
        if(src[0] == 0xE0 && src[1] < 0xA0)
            return false;
        if(src[0] == 0xED && src[1] > 0x9F)
            return false;
        if(src[0] == 0xF0 && src[1] < 0x90)
            return false;
        if(src[0] == 0xF4 && src[1] > 0x8F)
            return false;

        for(size_t i = 1; i < size; i++)
        {
            if(!(0x80 <= src[i] && src[i] <= 0xBF))
                return false;
        }

        return true;
    }
};

std::string utf8_from_wstring(const std::wstring& src, Status* err)
{
    std::string dst(src.size()*3+1, ' ');   // see UTF8Codec::Size; +1 ensures &dst[0] is valid
    UTF8* dstPos = (UTF8*)&dst[0];
    for(size_t i = 0; i < src.size(); i++)
    {
        const UTF32 u = ReplaceIfInvalid(UTF32(src[i]), err);
        UTF8Codec::EncodeFrom32(u, dstPos);
    }
    dst.resize(dstPos - (UTF8*)&dst[0]);
    return dst;
}

std::wstring wstring_from_utf8(const std::string& src, Status* err)
{
    std::wstring dst;
    dst.reserve(src.size());
    const UTF8* srcPos = (const UTF8*)src.data();
    const UTF8* const srcEnd = srcPos + src.size();
    while(srcPos < srcEnd)
    {
        const UTF32 u = UTF8Codec::DecodeTo32(srcPos, srcEnd, err);
        dst.push_back((wchar_t)ReplaceIfInvalid(u, err));
    }
    return dst;
}

struct Stringifier
{
	static bool callback(const char16_t* buf, uint32_t len, void* data)
	{
		utf16string str(buf, buf+len);
		std::wstring strw(str.begin(), str.end());

		Status err; // ignore Unicode errors
		static_cast<Stringifier*>(data)->stream << utf8_from_wstring(strw, &err);
		return true;
	}

	std::stringstream stream;
};

std::string Stringify(JSContext *cx, JS::RootedValue &arg0Val) {
    Stringifier str;
    JS::RootedValue undef(cx, JS::UndefinedValue());
    if(!JS_Stringify(cx, &arg0Val, JS::NullPtr(), undef, &Stringifier::callback, &str)) {
        // error
        JS_ReportError(cx, "Invalid argument");
        return "";
    }
    std::string res = str.stream.str();
    return res;
}


///////////////////////////////////////
//
//  Plugin Utils API
//
///////////////////////////////////////

static void printLog(const char* str) {
    CCLOG("%s", str);
}

static bool jsb_utils_deviceid(JSContext *cx, uint32_t argc, jsval *vp)
{
    printLog("jsb_utils_deviceid");
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
    JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
    if(argc == 0) {
        std::string result = cocos2d::JniHelper::callStaticStringMethod("org/cocos2dx/javascript/Util", "GetId");
        rec.rval().set(std_string_to_jsval(cx, result));
        return true;
    } else {
        JS_ReportError(cx, "Invalid number of arguments");
        return false;
    }
}

static bool jsb_utils_deviceidmd5(JSContext *cx, uint32_t argc, jsval *vp)
{
    printLog("jsb_utils_deviceidmd5");
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
    JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
    if(argc == 0) {
        std::string result = cocos2d::JniHelper::callStaticStringMethod("org/cocos2dx/javascript/Util", "GetIdMd5");
        rec.rval().set(std_string_to_jsval(cx, result));
        return true;
    } else {
        JS_ReportError(cx, "Invalid number of arguments");
        return false;
    }
}

void register_all_utils_framework(JSContext* cx, JS::HandleObject obj) {
    printLog("[Utils] register js interface");
    JS::RootedObject ns(cx);
    get_or_create_js_obj(cx, obj, "utils", &ns);

    JS_DefineFunction(cx, ns, "DeviceId", jsb_utils_deviceid, 0, JSPROP_ENUMERATE | JSPROP_PERMANENT);
    JS_DefineFunction(cx, ns, "DeviceIdMd5", jsb_utils_deviceidmd5, 0, JSPROP_ENUMERATE | JSPROP_PERMANENT);

}
