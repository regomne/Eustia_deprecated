#ifndef _TOOL_FUN_H_
#define _TOOL_FUN_H_

#include "CommonDef.h"

#pragma warning(disable: 4505) 


#ifdef USE_DBG
#include <stdio.h>
#endif // USE_DBG

// 4 字节对齐
#define PACK_DWORD(x) ((DWORD)(x) & 3 ? ((DWORD)(x) & ~3) + 4 : (DWORD)(x))

//
#define BITES_PER_BYTE (8)

#define BYTE_SHIFT_NUM (sizeof(BYTE) * BITES_PER_BYTE)
#define BYTE_ROL(Val, ShiftNum) ((Val << (ShiftNum)) | (Val >> (BYTE_SHIFT_NUM - ShiftNum)))
#define BYTE_ROR(Val, ShiftNum) ((Val >> (ShiftNum)) | (Val << (BYTE_SHIFT_NUM - ShiftNum)))

#define WORD_SHIFT_NUM (sizeof(WORD) * BITES_PER_BYTE)
#define WORD_ROL(Val, ShiftNum) ((Val << (ShiftNum)) | (Val >> (WORD_SHIFT_NUM - ShiftNum)))
#define WORD_ROR(Val, ShiftNum) ((Val >> (ShiftNum)) | (Val << (WORD_SHIFT_NUM - ShiftNum)))

#define DWORD_SHIFT_NUM (sizeof(DWORD) * BITES_PER_BYTE)
#define DWORD_ROL(Val, ShiftNum) ((Val << (ShiftNum)) | (Val >> (DWORD_SHIFT_NUM - ShiftNum)))
#define DWORD_ROR(Val, ShiftNum) ((Val >> (ShiftNum)) | (Val << (DWORD_SHIFT_NUM - ShiftNum)))

// network <-> host, DWORD/WORD
DWORD __inline N2HL(DWORD dwValue)
{
    BYTE c;
    DWORD dwResult = dwValue;
    BYTE *cb = (BYTE*)&dwResult;

    c = cb[0]; cb[0] = cb[3]; cb[3] = c;
    c = cb[1]; cb[1] = cb[2]; cb[2] = c;

    return dwResult;
}

DWORD __inline H2NL(DWORD dwValue)
{
    return N2HL(dwValue);
}

WORD __inline N2HS(WORD wValue)
{
    unsigned char  c;
    WORD wResult = wValue;
    BYTE *cb = (BYTE*)&wResult;

    c = cb[0]; cb[0] = cb[1]; cb[1] = c;

    return wResult;
}

WORD __inline H2NS(WORD wValue)
{
    return N2HS(wValue);
}

// get value
WORD __inline GetMinWord(DWORD dwValue, WORD wMaxValue)
{
    WORD wRet = 0;

    if (dwValue > wMaxValue)
    {
        wRet = wMaxValue;
    }
    else
    {
        wRet = (WORD)dwValue;
    }

    return wRet;
}

//lint -save -e18
BYTE __inline GetMinByte(WORD wValue, BYTE byMaxValue)
{
    BYTE byRet = 0;

    if (wValue > byMaxValue)
    {
        byRet = byMaxValue;
    }
    else
    {
        byRet = (BYTE)wValue;
    }

    return byRet;
}

/*
BYTE __inline GetMinByte(DWORD dwValue, BYTE byMaxValue)
{
    BYTE byRet = 0;

    if (dwValue > byMaxValue)
    {
        byRet = byMaxValue;
    }
    else
    {
        byRet = (BYTE)dwValue;
    }

    return byRet;
}
*/

short __inline Int2Short(int nValue)
{
    short sRet = 0;
    
    if (nValue > MAX_SHORT)
    {
        sRet = MAX_SHORT;
    }
    else if (nValue < MIN_SHORT)
    {
        sRet = MIN_SHORT;
    }
    else
    {
        sRet = (short)nValue;
    }
    
    return sRet;
}

DWORD __inline GetDeltaDword(DWORD dwVal1, DWORD dwVal2)
{
    DWORD dwDelta = 0;
    
    if (dwVal1 > dwVal2)
    {
        dwDelta = dwVal1 - dwVal2;
    }
    else
    {
        dwDelta = dwVal2 - dwVal1;
    }
    
    return dwDelta;
}

float __inline GetDeltaFloat(float fVal1, float fVal2)
{
    float fDelta = 0.0;

    if (fVal1 > fVal2)
    {
        fDelta = fVal1 - fVal2;
    }
    else
    {
        fDelta = fVal2 - fVal1;
    }

    return fDelta;
}

//lint -restore
// read buf to data, return data, modify buf position
BYTE __inline ReadBufByte(BYTE* pbyBuf, DWORD &dwPos)
{
    BYTE byData = *(BYTE*)(pbyBuf + dwPos);
    dwPos += sizeof(BYTE);
    return byData;
}

WORD __inline ReadBufWord(BYTE* pbyBuf, DWORD &dwPos)
{
    WORD wData = *(WORD*)(pbyBuf + dwPos);
    dwPos += sizeof(WORD);
    return wData;
}

DWORD __inline ReadBufDword(BYTE* pbyBuf, DWORD &dwPos)
{
    DWORD dwData = *(DWORD*)(pbyBuf + dwPos);
    dwPos += sizeof(DWORD);
    return dwData;
}

// read buf to data, return data, modify buf position
BYTE __inline ReadBufByteByXlat(BYTE* pbyBuf, DWORD &dwPos)
{
    BYTE byData = 0;
    DWORD dwAddr = (DWORD)pbyBuf + dwPos;

    __asm
    {
        mov ebx, dwAddr; //lint !e529
        mov al, 0;
        xlat;
        mov byData, al;
    }

    dwPos += sizeof(BYTE);
    return byData;
}

static BYTE ReadBufByte(BYTE* pbyBuf)
{
    BYTE byData = *(BYTE*)(pbyBuf);
    return byData;
}

static BYTE ReadBufByteByXlat(BYTE* pbyBuf)
{
    BYTE byData = 0;
    DWORD dwAddr = (DWORD)pbyBuf;

    __asm
    {
        mov ebx, dwAddr
        mov al, 0
        xlat
        mov byData, al
    }

    return byData;
}

// write data to buf, modify and return buf position
DWORD __inline WriteByte2Buf(BYTE* pbyBuf, DWORD &dwPos, BYTE byByte2Write)
{
    *(BYTE*)(pbyBuf + dwPos) = byByte2Write;
    dwPos += sizeof(byByte2Write);
    return dwPos;
}

DWORD __inline WriteWord2Buf(BYTE* pbyBuf, DWORD &dwPos, WORD wWord2Write)
{
    *(WORD*)(pbyBuf + dwPos) = wWord2Write;
    dwPos += sizeof(wWord2Write);
    return dwPos;
}

DWORD __inline WriteShort2Buf(BYTE* pbyBuf, DWORD &dwPos, short sShort2Write)
{
    *(short*)(pbyBuf + dwPos) = sShort2Write;
    dwPos += sizeof(sShort2Write);
    return dwPos;
}

DWORD __inline WriteDword2Buf(BYTE* pbyBuf, DWORD &dwPos, DWORD dwDword2Write)
{
    *(DWORD*)(pbyBuf + dwPos) = dwDword2Write;
    dwPos += sizeof(dwDword2Write);
    return dwPos;
}

//////////////////////////////////////////////////////////////////////////
// Rand functions
//////////////////////////////////////////////////////////////////////////
#define DWORD_VAL_SHIFT_NUM (sizeof(DWORD) * 8)

DWORD __forceinline FastGenRandDword(BYTE byShiftHigh = 5, BYTE byShiftLow = 7)
{
    DWORD dwHigh = 0, dwLow = 0, dwRet = 0;
    __asm rdtsc;
    __asm mov dwHigh, eax;
    __asm mov dwLow, edx;

    if (byShiftHigh == 0)
    {
        byShiftHigh = (BYTE)dwHigh;
    }

    if (byShiftHigh == 0)
    {
        byShiftHigh = 5;
    }

    if (byShiftLow == 0)
    {
        byShiftLow = (BYTE)(8 - (BYTE)dwHigh);
    }

    if (byShiftLow == 0)
    {
        byShiftLow = 7;
    }

    dwRet = ((dwHigh >> byShiftHigh) ^ (dwHigh << (DWORD_VAL_SHIFT_NUM - byShiftLow)))
          ^ ((dwLow  << byShiftHigh) ^ (dwLow  >> (DWORD_VAL_SHIFT_NUM - byShiftLow)));
    return dwRet;
}

WORD __inline FastGenRandWord(BYTE byShiftHigh = 5, BYTE byShiftLow = 7)
{
    WORD wRet = (WORD)FastGenRandDword(byShiftHigh, byShiftLow);
    return wRet;
}

BYTE __inline FastGenRandByte(BYTE byShiftHigh = 5, BYTE byShiftLow = 7)
{
    BYTE byRet = (BYTE)FastGenRandWord(byShiftHigh, byShiftLow);
    return byRet;
}

// dwRandSeed通常通过GetTickCount初始化
static DWORD GenRandDword(DWORD dwRandSeed, DWORD dwLimitStart, DWORD dwLimitLen)
{
    //    DWORD dwRandResult = ::GetTickCount();
    DWORD dwRandResult = dwRandSeed;

    DWORD dwHigh = 0, dwLow = 0;

    __asm rdtsc;
    __asm mov dwHigh, eax;
    __asm mov dwLow, edx;

    DWORD dwKey = dwRandResult ^ dwHigh ^ dwLow;

    for (BYTE i = 0; i < 32; ++i)
    {
        __asm mov ebx, dwKey;
        __asm shl ebx, 1;
        __asm mov eax, dwKey;
        __asm mov edx, 0xe7bd2160;
        __asm and eax, edx;
        __asm mov edx, eax;
        __asm shr eax, 16;
        __asm xor ax, dx;
        __asm xor ah, al;
        __asm jp _next;
        __asm inc ebx;
_next:
        __asm mov dwKey, ebx;
    }

    if (dwKey >= dwLimitStart && dwKey <= dwLimitStart + dwLimitLen)
    {
        dwRandResult = dwKey;
    }
    else
    {
        dwRandResult = dwKey % (dwLimitLen + 1) + dwLimitStart;
    }

    return dwRandResult;
}

// DWORD dwRandResult = dwRandSeed;
//
// DWORD dwHigh = 0, dwLow = 0;
//
// __asm rdtsc;
// __asm mov dwHigh, eax;
// __asm mov dwLow, edx;
//
// DWORD dwKey = dwRandResult ^ dwHigh ^ dwLow;

#define GENRANDVAL(dwRandResult, dwRandSeed, dwLimitStart, dwLimitLen) \
{\
    __asm \
    { \
        mov edi, dwRandSeed \
        rdtsc \
        xor edi, eax \
        xor edi, edx \
        mov dwRandResult, edi \
    } \
    for (BYTE i = 0; i < 32; ++i) \
    { \
        __asm mov ebx, dwRandResult; \
        __asm shl ebx, 1; \
        __asm mov eax, dwRandResult; \
        __asm mov edx, 0xe7bd2160; \
        __asm and eax, edx; \
        __asm mov edx, eax; \
        __asm shr eax, 16; \
        __asm xor ax, dx; \
        __asm xor ah, al; \
        __asm jp _next; \
        __asm inc ebx; \
_next: \
        __asm mov dwRandResult, ebx; \
    } \
    if (dwRandResult >= dwLimitStart && dwRandResult <= dwLimitStart + dwLimitLen) {} \
    else \
    { \
        dwRandResult = dwRandResult % (dwLimitLen + 1) + dwLimitStart; \
    } \
}

// 使用这个宏之后的代码不能加代码混淆，因为里面有硬编码的跳转地址
#define GENRANDVALBETWEEN(dwRandResult, dwRandSeed, dwLimitStart, dwLimitEnd) \
{\
    DWORD dwLimitLen = 0;\
    \
    if (dwLimitEnd > dwLimitStart)\
    {\
        dwLimitLen = dwLimitEnd - dwLimitStart;\
    }\
    else\
    {\
        dwLimitLen = dwLimitStart - dwLimitEnd;\
    }\
    __asm \
    { \
        __asm mov edi, dwRandSeed \
        __asm rdtsc \
        __asm xor edi, eax \
        __asm xor edi, edx \
        __asm mov dwRandResult, edi \
    } \
    for (BYTE i = 0; i < 32; ++i) \
    { \
        __asm \
        { \
        __asm mov ebx, dwRandResult \
        __asm shl ebx, 1 \
        __asm mov eax, dwRandResult \
        __asm mov edx, 0xe7bd2160 \
        __asm and eax, edx \
        __asm mov edx, eax \
        __asm shr eax, 16 \
        __asm xor ax, dx \
        __asm xor ah, al \
        __asm _emit 0x7A \
        __asm _emit 0x01 \
        __asm inc ebx \
        __asm mov dwRandResult, ebx \
        }\
    } \
    if (dwRandResult >= dwLimitStart && dwRandResult <= dwLimitStart + dwLimitLen) {} \
    else \
    { \
        dwRandResult = dwRandResult % (dwLimitLen + 1) + dwLimitStart; \
    } \
}


DWORD __inline GenRandValInline(DWORD dwRandSeed, DWORD dwLimitStart, DWORD dwLimitLen)
{
    //    DWORD dwRandResult = ::GetTickCount();
    DWORD dwRandResult = dwRandSeed;

    DWORD dwHigh = 0, dwLow = 0;

    __asm rdtsc;
    __asm mov dwHigh, eax;
    __asm mov dwLow, edx;

    DWORD dwKey = dwRandResult ^ dwHigh ^ dwLow;

    for (BYTE i = 0; i < 32; ++i)
    {
        __asm mov ebx, dwKey;
        __asm shl ebx, 1;
        __asm mov eax, dwKey;
        __asm mov edx, 0xe7bd2160;
        __asm and eax, edx;
        __asm mov edx, eax;
        __asm shr eax, 16;
        __asm xor ax, dx;
        __asm xor ah, al;
        __asm jp _next;
        __asm inc ebx;
_next:
        __asm mov dwKey, ebx;
    }

    if (dwKey >= dwLimitStart && dwKey <= dwLimitStart + dwLimitLen)
    {
        dwRandResult = dwKey;
    }
    else
    {
        dwRandResult = dwKey % (dwLimitLen + 1) + dwLimitStart;
    }

    return dwRandResult;
}

DWORD __inline GenRandValBetweenInline(DWORD dwRandSeed, DWORD dwLimitStart, DWORD dwLimitEnd)
{
    DWORD dwLimitLen = 0;

    if (dwLimitEnd > dwLimitStart)
    {
        dwLimitLen = dwLimitEnd - dwLimitStart;
    }
    else
    {
        dwLimitLen = dwLimitStart - dwLimitEnd;
    }

    return GenRandValInline(dwRandSeed, dwLimitStart, dwLimitLen);
}

//
void __inline RandomizeBuf(BYTE* pbyBuf, DWORD dwBufSize)
{
    DWORD dwDwordNum = dwBufSize / (sizeof(DWORD));
    BYTE  byByteNum  = (BYTE)(dwBufSize - dwDwordNum * sizeof(DWORD));
    BYTE  byShiftHigh = 0, byShiftLow = 0;
    DWORD dwRandVal = 0;
    BYTE  byRandVal = 0;
    DWORD dwPos = 0;
    DWORD i = 0;
    for (i = 0; i < dwDwordNum; i++)
    {
        byShiftHigh = (BYTE)((i + dwDwordNum) % DWORD_VAL_SHIFT_NUM);
        byShiftLow  = (BYTE)((i + byByteNum ) % DWORD_VAL_SHIFT_NUM);
        dwRandVal = FastGenRandDword(byShiftHigh, byShiftLow);
        WriteDword2Buf(pbyBuf, dwPos, dwRandVal); //lint !e1058
    }

    for (i = 0; i < byByteNum; i++)
    {
        byShiftHigh = (BYTE)((i + dwDwordNum) % DWORD_VAL_SHIFT_NUM);
        byShiftLow  = (BYTE)((i + byByteNum ) % DWORD_VAL_SHIFT_NUM);
        byRandVal = FastGenRandByte(byShiftHigh, byShiftLow);
        WriteByte2Buf(pbyBuf, dwPos, byRandVal); //lint !e1058
    }
}

// ap hash function
DWORD __inline ApHash(BYTE *pbyOffset, DWORD dwLen)
{
    DWORD dwHash = 0;    
    BYTE *pbyHashOffset = pbyOffset;
    
    for (DWORD dwIndex = 0; dwIndex < dwLen; pbyHashOffset++, dwIndex++)
    {
        dwHash ^= ( ((dwIndex & 1) == 0) ? 
            ((dwHash <<  7) ^ (*(BYTE *)pbyHashOffset) ^ (dwHash >> 3)) : 
        (~((dwHash << 11) ^ (*(BYTE *)pbyHashOffset) ^ (dwHash >> 5))) );
    }
    
    dwHash = (dwHash & 0x7fffffff);
    return dwHash;
}

DWORD __inline GetTrueNum()
{
    DWORD dwSeed = GetTickCount();
    DWORD dwRand = GenRandValInline(dwSeed, 0, MAX_DWORD);
    if ((dwRand & 0x1C000) == 0)
    {
        dwRand = dwRand | 0x1C000;      // 中间3位不为零
    }
    
    return dwRand;
}

DWORD __inline GetFalseNum()
{
    DWORD dwSeed = GetTickCount();
    DWORD dwRand = GenRandValInline(dwSeed, 0, MAX_DWORD);
    dwRand = dwRand & 0xFFFE3FFF;       // 中间3位为零
    
    return dwRand;
}

BOOL __inline IsTrueNum(DWORD dwValue)
{
    return ((dwValue & (0x1C000)) != 0);
}

BYTE IsLeapYear(WORD wYear);
WORD GetDaysByYear(WORD wYear);

__forceinline PVOID LoadGetFuncAddr(char* pszLibName, char* pszFunName)
{
    PVOID pvFuncAddr = NULL;
    
    HMODULE hDll = ::GetModuleHandleA(pszLibName);
    if (hDll == NULL)
    {
        hDll = ::LoadLibraryA(pszLibName);              
    }

    if (hDll != NULL)
    {
        pvFuncAddr = ::GetProcAddress(hDll, pszFunName);
    } 
    
    return pvFuncAddr;
}

//////////////////////////////////////////////////////////////////////////
// DebugOut functions
//////////////////////////////////////////////////////////////////////////
#ifdef USE_DBG
void DebugOut(char *pszFormat, ...);
void DebugOutL(wchar_t *pszFormat, ...);
void DebugOutW(wchar_t *pszFormat, wchar_t *pszContext);
void DumpHex(BYTE *pbyData, DWORD dwLen);
BOOL DumpMemory(LPBYTE lpBytes, int nCount);
void DumpMemoryInfo(LPBYTE lpBytes, DWORD dwMemorySize);
void DebugOutHex(BYTE *pbyHex, DWORD dwLen);
void DebugOutHex(BYTE *pbyData, DWORD dwLen, const char *pszTag);
void DumpHexWithTag(BYTE *pbyData, DWORD dwLen, const char *pszTag);

void HexToString(BYTE *pbyHex, DWORD dwLen);
#endif // USE_DBG

#ifdef USE_DBG
#define DBGOUT(a)        (DebugOut) a
#define DBGOUTW(a, b)    (DebugOutW)(a, b)
#define DBGOUTHEX(a)    (DumpHex) a
#define HEX2STRING(a, b)      (HexToString)(a, b)
#define DBGOUTL(a)       (DebugOutL)a
//#define DBGOUTHEXWITHTAG(a)    (DumpHexWithTag)a
#else
#define DBGOUT(a)        // do{}while(0);
#define DBGOUTW(a, b)    // do{}while(0);
#define DBGOUTHEX(a)
#define HEX2STRING(a, b)
#define DBTOUTL(a)
//#define DBGOUTHEXWITHTAG(a)
#endif // USE_DBG

BOOL DumpBuf2File(char* pszFileName, BYTE* pbyBuf, DWORD dwBufSize);
BOOL LoadFile2Buf(char* pszFilePah, BYTE* &pbyFileBuf, DWORD &dwBufSize);

#endif // _TOOL_FUN_H_