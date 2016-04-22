#include "misc.h"
#include "../gs/ToolFun.h"

using namespace std;

std::wstring GetFullModuleFileName(HMODULE hm)
{
    int pathLen = 256;
    wchar_t* dllPath = new wchar_t[pathLen];
    int retlen = GetModuleFileName(hm, dllPath, pathLen);
    while (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        delete[] dllPath;
        pathLen *= 2;
        dllPath = new wchar_t[pathLen];
        retlen = GetModuleFileName(0, dllPath, pathLen);
    };
    
    wstring str(dllPath);
    delete[] dllPath;
    return str;
}

wstring GetDllPath(HMODULE hm)
{
    wstring path = L"";
    auto dllFileName = GetFullModuleFileName(hm);
    if (dllFileName.length() != 0)
    {
        auto it = dllFileName.rfind(L'\\');
        if (it != wstring::npos)
        {
            path = dllFileName.substr(0, it + 1);
        }
    }
    return path;
}

void WcharDeleter(wchar_t* p)
{
    delete[] p;
}

void CharDeleter(char* p)
{
    delete[] p;
}

bool ReadUtf8Text(const wchar_t* fileName, char** strPointer, int* textLen)
{
    if (!fileName || !strPointer || !textLen)
    {
        return false;
    }
    FILE* file = _wfopen(fileName, L"rb");
    if (file == NULL) return false;

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);

    char* chars = new char[size + 2];
    chars[size] = '\0';
    chars[size + 1] = '\0';
    for (int i = 0; i < size;) {
        int read = static_cast<int>(fread(&chars[i], 1, size - i, file));
        i += read;
    }
    fclose(file);

    bool ret = true;
    do
    {
        if (size >= 3 && !memcmp(chars, "\xef\xbb\xbf", 3))
        {
            *strPointer = new char[size - 2];
            if (!*strPointer)
            {
                ret = false;
                break;
            }
            memcpy(*strPointer, chars + 3, size - 3);
            (*strPointer)[size - 3] = '\0';
            *textLen = size - 3;
        }
        else if (size >= 2 && !memcmp(chars, "\xff\xfe", 2))
        {
            *textLen = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)(chars + 2), (size - 2) / 2, 0, 0, 0, 0);
            if (*textLen == 0)
            {
                ret = false;
                break;
            }
            *strPointer = new char[*textLen + 1];
            if (!*strPointer)
            {
                ret = false;
                break;
            }
            *textLen = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)(chars + 2), (size - 2) / 2, *strPointer, *textLen, 0, 0);
        }
        else
        {
            *strPointer = chars;
            *textLen = size;
            chars = nullptr;
        }
    } while (false);
    delete[] chars;
    return ret;
}
