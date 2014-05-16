//lint -save -e685 -e568 -e668
#include "ToolFun.h"


// dmp buffer to file
BOOL DumpBuf2File(char* pszFileName, BYTE* pbyBuf, DWORD dwBufSize)
{
    BOOL bRet = FALSE;
    HANDLE hOutFile = INVALID_HANDLE_VALUE;
    DWORD dwByteWritten = 0;
    
    do {
        
    hOutFile = ::CreateFileA(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOutFile == INVALID_HANDLE_VALUE)
    {
        DBGOUT(("DumpBuf2File:CreateFile %s failed!\n", pszFileName));
        break;
    }
    
    if (!::WriteFile(hOutFile, pbyBuf, dwBufSize, &dwByteWritten, NULL))
    {
        DBGOUT(("DumpBuf2File:WriteFile failed!\n"));
        break;
    }
    
    if (dwBufSize != dwByteWritten)
    {
        DBGOUT(("DumpBuf2File:WriteFile failed! %u != %u\n", dwBufSize, dwByteWritten));
        break;
    }    
    
    bRet = TRUE;
        
    } while (0);
    
    SAFE_CLOSE_HANDLE(hOutFile);
    hOutFile = INVALID_HANDLE_VALUE;
    
    return bRet;
}

// load file to buf
BOOL LoadFile2Buf(char* pszFilePah, BYTE* &pbyFileBuf, DWORD &dwBufSize)
{
    BOOL bRet = FALSE;
    HANDLE hFile = INVALID_HANDLE_VALUE;    
    DWORD dwBytesRead = 0;
    
    do {
        
    hFile = ::CreateFileA(pszFilePah, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DBGOUT(("[ERROR]:Failed to open file:%s", pszFilePah));
        break;
    }
    
    dwBufSize = ::GetFileSize(hFile, NULL);
    pbyFileBuf = new BYTE[dwBufSize];
    memset(pbyFileBuf, 0, dwBufSize);
    
    if (!::ReadFile(hFile, pbyFileBuf, dwBufSize, &dwBytesRead, NULL))
    {
        DBGOUT(("[ERROR]:Failed to read file:%s:%u", pszFilePah, dwBufSize));
        break;
    }
    
    if (dwBytesRead != dwBufSize)
    {
        DBGOUT(("[ERROR]:Failed to read file:%s:%u", pszFilePah, dwBufSize));
        break;
    }
        
    bRet = TRUE;

    } while (0);
    
    if (hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(hFile);
    }

    return bRet;
}

BYTE IsLeapYear(WORD wYear)
{    
    if ((wYear % 400 == 0) || ((wYear % 4 == 0) && (wYear % 100 != 0)))
    {
        return 1;
    }

    return 0;
}

WORD GetDaysByYear(WORD wYear)
{
    if ((wYear % 400 == 0) || ((wYear % 4 == 0) && (wYear % 100 != 0)))
    {
        return 366;
    }
    
    return 365;
}

#ifdef USE_DBG

void SprintfHex(char *pszValue, BYTE byValue)
{
    BYTE byHigh = byValue / 16;
    BYTE byLow = byValue % 16;

#define SYSBYTE2HEX(a,b)            \
    if (a >= 0 && a <= 9)            \
    {                                \
        b = a + '0';                \
    }                                \
    else if (a >= 10 && a <= 15)    \
    {                                \
        b = a + 'a' - 10;            \
    }                                \
    else                            \
    {                                \
        b = '0';                    \
    }

    SYSBYTE2HEX(byHigh, pszValue[0]); 
    SYSBYTE2HEX(byLow, pszValue[1]);
}

void SprintfHexWithSep(char *pszValue, BYTE byValue)
{
    SprintfHex(pszValue, byValue);
    pszValue[2] = ' ';
}

BYTE SprintfDWORD(char *pszValue, DWORD dwValue)
{
    BYTE by1 = (BYTE)(dwValue & 0x000000ff);
    BYTE by2 = (BYTE)((dwValue >> 8) & 0x000000ff);
    BYTE by3 = (BYTE)((dwValue >> 16) & 0x000000ff);
    BYTE by4 = (BYTE)((dwValue >> 24) & 0x000000ff);

    SprintfHex(pszValue, by4);
    SprintfHex(pszValue + 2, by3);
    SprintfHex(pszValue + 4, by2);
    SprintfHex(pszValue + 6, by1);

    return 8;
}

BYTE SprintfDicimal(char *pszValue, DWORD dwValue)
{
#define DECIMAL_1    1000000000
#define DECIMAL_2    100000000
#define DECIMAL_3    10000000
#define DECIMAL_4    1000000
#define DECIMAL_5    100000
#define DECIMAL_6    10000
#define DECIMAL_7    1000
#define DECIMAL_8    100
#define DECIMAL_9    10

    DWORD dwValuePos = 0;
    BYTE byDecimal[10] = {0};

    byDecimal[0] = (BYTE)((dwValue - dwValuePos) / DECIMAL_1);
    dwValuePos += (byDecimal[0] * DECIMAL_1);

    byDecimal[1] = (BYTE)((dwValue - dwValuePos) / DECIMAL_2);
    dwValuePos += (byDecimal[1] * DECIMAL_2);

    byDecimal[2] = (BYTE)((dwValue - dwValuePos) / DECIMAL_3);
    dwValuePos += (byDecimal[2] * DECIMAL_3);

    byDecimal[3] = (BYTE)((dwValue - dwValuePos) / DECIMAL_4);
    dwValuePos += (byDecimal[3] * DECIMAL_4);

    byDecimal[4] = (BYTE)((dwValue - dwValuePos) / DECIMAL_5);
    dwValuePos += (byDecimal[4] * DECIMAL_5);

    byDecimal[5] = (BYTE)((dwValue - dwValuePos) / DECIMAL_6);
    dwValuePos += (byDecimal[5] * DECIMAL_6);

    byDecimal[6] = (BYTE)((dwValue - dwValuePos) / DECIMAL_7);
    dwValuePos += (byDecimal[6] * DECIMAL_7);

    byDecimal[7] = (BYTE)((dwValue - dwValuePos) / DECIMAL_8);
    dwValuePos += (byDecimal[7] * DECIMAL_8);

    byDecimal[8] = (BYTE)((dwValue - dwValuePos) / DECIMAL_9);
    dwValuePos += (byDecimal[8] * DECIMAL_9);

    byDecimal[9] = (BYTE)((dwValue - dwValuePos) % DECIMAL_9);
    dwValuePos += (byDecimal[9]);

    BYTE byPos = 0;
    for (byPos = 0; byPos < 10; byPos++)
    {
        if (byDecimal[byPos] != 0)
        {
            break;
        }
    }
    BYTE k = 0;
    for (; byPos < 10; byPos++)
    {
        if (byDecimal[byPos] >= 0 && byDecimal[byPos] <= 9)
        {
            *(char*)(pszValue + k++) = '0' + byDecimal[byPos];
        }
        else
        {
            *(char*)(pszValue + k++) = '0';
        }
    }

    return k;
}

void DebugOut(char *pszFormat, ...)
{
    if (!pszFormat)
    {
        return;
    }

    char *pszMsg = (char*)malloc(4096);

    va_list ap;
    va_start(ap, pszFormat);
    _vsnprintf(pszMsg, 4096 - 1, pszFormat, ap);
    va_end(ap);

    OutputDebugStringA(pszMsg);

    free(pszMsg);
}

void DebugOutL(wchar_t *pszFormat, ...)
{
    if (!pszFormat)
    {
        return;
    }

    wchar_t *pszMsg = (wchar_t*)malloc(4096);

    va_list	ap;    
    va_start(ap, pszFormat);
    _vsnwprintf(pszMsg, 4096 - 2, pszFormat, ap);
    va_end(ap);

    OutputDebugStringW(pszMsg);
    free(pszMsg);
}

void DebugOutW(wchar_t *pszFormat, wchar_t *pszContext)
{
    if ((pszFormat == NULL) || (pszContext == NULL))
    {
        return;
    }

    wchar_t *pszMsg = (wchar_t *)malloc(4096);
    size_t nLen1 = wcslen(pszFormat);
    size_t nLen2 = wcslen(pszContext);
    wcsncpy(pszMsg, pszFormat, nLen1 * sizeof(wchar_t));
    wcsncat(pszMsg, pszContext, nLen2 * sizeof(wchar_t));
    OutputDebugStringW(pszMsg);

    free(pszMsg);
}

// void DebugOutHex(BYTE *pbyData, DWORD dwLen)
// {
//     if (!pbyData || dwLen == 0)
//     {
//         return;
//     }
// 
//     char *pszLineBuf = (char*)malloc(3 * dwLen + 1);
//     char *pchTemp = pszLineBuf;
// 
//     for (DWORD i = 0; i < dwLen; i++)
//     {
//         SprintfHexWithSep(pchTemp, pbyData[i]);
//         pchTemp += sizeof(BYTE) * 3;
//     }
// 
//     *pchTemp = '\0';
//     DebugOut("%s", pszLineBuf);
// 
//     free(pszLineBuf);
// }

void DumpHex(BYTE *pbyData, DWORD dwLen)
{
    DWORD dwTimes = dwLen / 16 + (dwLen % 16 > 0 ? 1 : 0);

    for (DWORD dwTmpIdx = 0; dwTmpIdx < dwTimes; dwTmpIdx++)
    {
        if (dwTmpIdx != dwTimes - 1)
        {
            DebugOutHex(pbyData + dwTmpIdx * 16, 16);
        }
        else
        {
            DebugOutHex(pbyData + dwTmpIdx * 16, dwLen - dwTmpIdx * 16);
        }
    }
}

#define DUMPMEMORY_BYTESPERROW 16

BOOL DumpMemory(LPBYTE lpBytes, int nCount)
{
    BOOL bReadException = ::IsBadReadPtr(lpBytes, nCount);
    char szBuffer[DUMPMEMORY_BYTESPERROW * 3 + 1] = {0};
    int nIndex = 0;
    
    if (!bReadException)
    {
        for(nIndex = 0; nIndex < nCount; nIndex++)
        {
            _snprintf(szBuffer, DUMPMEMORY_BYTESPERROW * 3, "%s %02x", szBuffer, lpBytes[nIndex]);
        }
        
        DebugOut(szBuffer);
    }
    
    return bReadException;
}

void DumpMemoryInfo(LPBYTE lpBytes, DWORD dwMemorySize)
{
    int nLoopCount = 0, nLeftBytes = 0, nIndex = 0;
    BOOL bReadException = FALSE;
    
    nLoopCount = dwMemorySize / DUMPMEMORY_BYTESPERROW;
    nLeftBytes = dwMemorySize % DUMPMEMORY_BYTESPERROW;
    
    for(nIndex = 0; nIndex < nLoopCount; nIndex++)
    {
        bReadException = DumpMemory(lpBytes + nIndex * DUMPMEMORY_BYTESPERROW, DUMPMEMORY_BYTESPERROW);
        if (bReadException)
        {
            break;
        }
    }
    
    if (!bReadException && nLeftBytes > 0)
    {
        DumpMemory(lpBytes + nLoopCount * DUMPMEMORY_BYTESPERROW, nLeftBytes);
    }
}

void DebugOutHex(BYTE *pbyHex, DWORD dwLen)
{
    DumpMemoryInfo(pbyHex, dwLen);
}

void DumpHexWithTag(BYTE *pbyData, DWORD dwLen, const char *pszTag)
{
    DWORD dwTimes = dwLen / 16 + (dwLen % 16 > 0 ? 1 : 0);
    
    for (DWORD dwTmpIdx = 0; dwTmpIdx < dwTimes; dwTmpIdx++)
    {
        if (dwTmpIdx != dwTimes - 1)
        {
            DebugOutHex(pbyData + dwTmpIdx * 16, 16, pszTag);
        }
        else
        {
            DebugOutHex(pbyData + dwTmpIdx * 16, dwLen - dwTmpIdx * 16, pszTag);
        }
    }
}

void DebugOutHex(BYTE *pbyData, DWORD dwLen, const char *pszTag)
{
    if (!pbyData || dwLen == 0)
    {
        return;
    }
    
    char *pszLineBuf = (char*)malloc(3 * dwLen + 1);
    if (pszLineBuf == NULL)
    {
        return;
    }
    
    char *pchTemp = pszLineBuf;    
    for (DWORD i = 0; i < dwLen; i++)
    {
        SprintfHexWithSep(pchTemp, pbyData[i]);
        pchTemp += sizeof(BYTE) * 3;
    }
    
    *pchTemp = '\0';
    
    if (pszTag != NULL)
    {
        DebugOut("%s: %s", pszTag, pszLineBuf);
    }
    else
    {
        DebugOut("%s", pszLineBuf);
    }
    
    free(pszLineBuf);
}

void HexToString(BYTE *pbyHex, DWORD dwLen)
{
    if (pbyHex == NULL)
    {
        return;
    }

    char *pszHex = new char[dwLen * 2 + 1];
    if (pszHex == NULL)
    {
        return;
    }
    
    memset(pszHex, 0, 2 * dwLen + 1);
    
    char szTmp[3] = {0};
    for (DWORD i = 0; i < dwLen; i++)
    {
        _snprintf(szTmp, sizeof szTmp - 1, "%02X", pbyHex[i]);
        strncat(pszHex, szTmp, __min(sizeof szTmp - 1, 2 * dwLen - strlen(pszHex)));
    }

    DebugOut("HexToString: %s", pszHex);

    delete []pszHex;
}

// lint -restore
#endif // USE_DBG