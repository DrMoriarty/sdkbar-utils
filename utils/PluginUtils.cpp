#include "PluginUtils.h"

int CallbackFrame::nextId = 0;
std::vector<CallbackFrame*> CallbackFrame::callbackVector;

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
    static void Encode(UTF32 u, UTF8*& dstPos)
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
    static UTF32 Decode(const UTF8*& srcPos, const UTF8* const srcEnd, Status* err)
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
        UTF8Codec::Encode(u, dstPos);
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
        const UTF32 u = UTF8Codec::Decode(srcPos, srcEnd, err);
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
