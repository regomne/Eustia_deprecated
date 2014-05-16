#ifndef _COMMON_DEF_H_
#define _COMMON_DEF_H_

#include <windows.h>

//
#define MAX_BYTE  (0xFF)
#define MAX_WORD  (0xFFFF)
#define MIN_SHORT (-0x7FFF)
#define MAX_SHORT (0x7FFF)
#define MAX_DWORD (0xFFFFFFFF)
#define MIN_INT   (-0x7FFFFFFF)
#define MAX_INT   (0x7FFFFFFF)

//
#define MEMSET(a,b,size) memset((void*)(a),b,size)
#define MEMCPY(a,b,size) memmove((void*)(a),(void*)(b),size)

#define SAFE_NEW(a,b,size) if (!a) {a = (b*)new unsigned char[size]; memset((void*)a,0,size);}
#define SAFE_DELETE(a) if (a) {delete (a); (a) = 0;}
#define SAFE_ARR_DELETE(a) if (a) {delete[] (a); (a) = 0;}
#define SAFE_DELETE_ARRAY(a) SAFE_ARR_DELETE(a)
#define SAFE_CLOSE_HANDLE(a) if (a) {::CloseHandle(a); (a) = 0;}

// 用于异或加密的固定key和版本相关key
#define CONST_XOR_KEY (0x15238958)

// la数据包最大长度
#define MAX_LA_BUF_LEN     (160)
// #define MAX_LA_PKG_LEN     (200)

#ifndef _MAX_LA_PKG_LEN_
#define _MAX_LA_PKG_LEN_
#define MAX_LA_PKG_LEN           (200) 
#endif

// 定义通用错误码
#define ERROR_CODE_SUCCESS         (0)                               // 0
#define ERROR_CODE_0xFFFFFFFF      (MAX_DWORD)                       // 0xFFFFFFFF
#define ERROR_CODE_0xFFFFFFFE      (ERROR_CODE_0xFFFFFFFF - 1)       // 0xFFFFFFFE
#define ERROR_CODE_0xFFFFFFFD      (ERROR_CODE_0xFFFFFFFE - 1)       // 0xFFFFFFFD
#define ERROR_CODE_0xFFFFFFFC      (ERROR_CODE_0xFFFFFFFD - 1)       // 0xFFFFFFFC
#define ERROR_CODE_0xFFFFFFFB      (ERROR_CODE_0xFFFFFFFC - 1)       // 0xFFFFFFFB
#define ERROR_CODE_0xFFFFFFFA      (ERROR_CODE_0xFFFFFFFB - 1)       // 0xFFFFFFFA
#define ERROR_CODE_0xFFFFFFF9      (ERROR_CODE_0xFFFFFFFA - 1)       // 0xFFFFFFF9
#define ERROR_CODE_0xFFFFFFF8      (ERROR_CODE_0xFFFFFFF9 - 1)       // 0xFFFFFFF8
#define ERROR_CODE_0xFFFFFFF7      (ERROR_CODE_0xFFFFFFF8 - 1)       // 0xFFFFFFF7
#define ERROR_CODE_0xFFFFFFF6      (ERROR_CODE_0xFFFFFFF7 - 1)       // 0xFFFFFFF6
#define ERROR_CODE_0xFFFFFFF5      (ERROR_CODE_0xFFFFFFF6 - 1)       // 0xFFFFFFF5
#define ERROR_CODE_0xFFFFFFF4      (ERROR_CODE_0xFFFFFFF5 - 1)       // 0xFFFFFFF4
#define ERROR_CODE_0xFFFFFFF3      (ERROR_CODE_0xFFFFFFF4 - 1)       // 0xFFFFFFF3
#define ERROR_CODE_0xFFFFFFF2      (ERROR_CODE_0xFFFFFFF3 - 1)       // 0xFFFFFFF2
#define ERROR_CODE_0xFFFFFFF1      (ERROR_CODE_0xFFFFFFF2 - 1)       // 0xFFFFFFF1
#define ERROR_CODE_0xFFFFFFF0      (ERROR_CODE_0xFFFFFFF1 - 1)       // 0xFFFFFFF0
#define ERROR_CODE_EXCEPTION       (ERROR_CODE_0xFFFFFFF0 - 1)       // 0xFFFFFFEF

#endif // _COMMON_DEF_H_