#include "PluginUtils.h"
#include "base/ccUTF8.h"
#include "external/ConvertUTF/ConvertUTF.h"

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

template <typename From, typename To, typename FromTrait = ConvertTrait<From>, typename ToTrait = ConvertTrait<To>>
bool myUtfConvert(
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
        return true;
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
        return false;

    working.resize(reinterpret_cast<To*>(outbeg) - &working[0]);
    to = std::move(working);

    return true;
};


bool myUTF8ToUTF16(const std::string& utf8, std::u16string& outUtf16)
{
    return myUtfConvert(utf8, outUtf16, ConvertUTF8toUTF16);
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
    bool ok = myUTF8ToUTF16(std::string(v, length), strUTF16);

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
