/* 7zMain.c - Test application for 7z Decoder
 2016-05-16 : Igor Pavlov : Public domain */

#include "Precomp.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../../7z.h"
#include "../../7zAlloc.h"
#include "../../7zBuf.h"
#include "../../7zCrc.h"
#include "../../7zFile.h"
#include "../../7zVersion.h"

#ifndef USE_WINDOWS_FILE
/* for mkdir */
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <errno.h>
#endif
#endif

#if defined(ANDROID)
#include <android/log.h>
#define  LOG_TAG "extract7z"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define  LOGD printf
#define  LOGE printf
#endif

static ISzAlloc g_Alloc = { SzAlloc, SzFree };

static int Buf_EnsureSize(CBuf *dest, size_t size)
{
    if (dest->size >= size)
        return 1;
    Buf_Free(dest, &g_Alloc);
    return Buf_Create(dest, size, &g_Alloc);
}

#ifndef _WIN32
#define _USE_UTF8
#endif

#ifdef _USE_UTF8

#define _UTF8_START(n) (0x100 - (1 << (7 - (n))))

#define _UTF8_RANGE(n) (((UInt32)1) << ((n) * 5 + 6))

#define _UTF8_HEAD(n, val) ((Byte)(_UTF8_START(n) + (val >> (6 * (n)))))
#define _UTF8_CHAR(n, val) ((Byte)(0x80 + (((val) >> (6 * (n))) & 0x3F)))

static size_t Utf16_To_Utf8_Calc(const UInt16 *src, const UInt16 *srcLim)
{
    size_t size = 0;
    for (;;)
    {
        UInt32 val;
        if (src == srcLim)
            return size;
        
        size++;
        val = *src++;
        
        if (val < 0x80)
            continue;
        
        if (val < _UTF8_RANGE(1))
        {
            size++;
            continue;
        }
        
        if (val >= 0xD800 && val < 0xDC00 && src != srcLim)
        {
            UInt32 c2 = *src;
            if (c2 >= 0xDC00 && c2 < 0xE000)
            {
                src++;
                size += 3;
                continue;
            }
        }
        
        size += 2;
    }
}

static Byte *Utf16_To_Utf8(Byte *dest, const UInt16 *src, const UInt16 *srcLim)
{
    for (;;)
    {
        UInt32 val;
        if (src == srcLim)
            return dest;
        
        val = *src++;
        
        if (val < 0x80)
        {
            *dest++ = (char)val;
            continue;
        }
        
        if (val < _UTF8_RANGE(1))
        {
            dest[0] = _UTF8_HEAD(1, val);
            dest[1] = _UTF8_CHAR(0, val);
            dest += 2;
            continue;
        }
        
        if (val >= 0xD800 && val < 0xDC00 && src != srcLim)
        {
            UInt32 c2 = *src;
            if (c2 >= 0xDC00 && c2 < 0xE000)
            {
                src++;
                val = (((val - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
                dest[0] = _UTF8_HEAD(3, val);
                dest[1] = _UTF8_CHAR(2, val);
                dest[2] = _UTF8_CHAR(1, val);
                dest[3] = _UTF8_CHAR(0, val);
                dest += 4;
                continue;
            }
        }
        
        dest[0] = _UTF8_HEAD(2, val);
        dest[1] = _UTF8_CHAR(1, val);
        dest[2] = _UTF8_CHAR(0, val);
        dest += 3;
    }
}

static SRes Utf16_To_Utf8Buf(CBuf *dest, const UInt16 *src, size_t srcLen)
{
    size_t destLen = Utf16_To_Utf8_Calc(src, src + srcLen);
    destLen += 1;
    if (!Buf_EnsureSize(dest, destLen))
        return SZ_ERROR_MEM;
    *Utf16_To_Utf8(dest->data, src, src + srcLen) = 0;
    return SZ_OK;
}

#endif

static SRes Utf16_To_Char(CBuf *buf, const UInt16 *s)
{
    unsigned len = 0;
    for (len = 0; s[len] != 0; len++);
    return Utf16_To_Utf8Buf(buf, s, len);
}

#define ZIP_FULLPATH_MAX 512
static char g7z_error_buf[100];

static WRes MyCreateDir(char* fullPathBuff, char* targetDirEnd, const UInt16 *name)
{
    CBuf buf;
    WRes res;
    
    Buf_Init(&buf);
    RINOK(Utf16_To_Char(&buf, name));
    
    strcpy(targetDirEnd, (const char *) buf.data);
    
    if (access(fullPathBuff, 0) == -1)
    {
        res = mkdir(fullPathBuff, S_IRWXU | S_IRWXG | S_IRWXO) == 0 ? 0 : errno;
        if (res != 0)
        {
            strerror_r(errno, g7z_error_buf, 99);
            LOGE("create directory failed:%s  error:%s", fullPathBuff, g7z_error_buf);
        }
    }
    else
        res = 0;
    
    Buf_Free(&buf, &g_Alloc);
    return res;
}

static WRes OutFile_OpenUtf16(CSzFile *p,char* fullPathBuff, char* targetDirEnd, const UInt16 *name)
{
    CBuf buf;
    WRes res;
    
    Buf_Init(&buf);
    RINOK(Utf16_To_Char(&buf, name));
    
    strcpy(targetDirEnd, (const char *) buf.data);
    
    res = OutFile_Open(p, fullPathBuff);
    if (res != 0)
    {
        strerror_r(errno, g7z_error_buf, 99);
        LOGE("OutFile_OpenUtf16 failed:%s  error:%s", fullPathBuff, g7z_error_buf);
    }
    
    Buf_Free(&buf, &g_Alloc);
    return res;
}

/*static SRes PrintString(const UInt16 *s)
{
    CBuf buf;
    SRes res;
    Buf_Init(&buf);
    res = Utf16_To_Char(&buf, s);
    if (res == SZ_OK)
        LOGD("file name:%s", (const char *)buf.data);
    Buf_Free(&buf, &g_Alloc);
    return res;
}*/

int extract7z(const char* inFile, const char* outPath)
{
    CFileInStream archiveStream;
    CLookToRead lookStream;
    CSzArEx db;
    SRes res;
    ISzAlloc allocImp;
    ISzAlloc allocTempImp;
    UInt16 *temp = NULL;
    size_t tempSize = 200;
    
    LOGD("\n7z ANSI-C Decoder " MY_VERSION_COPYRIGHT_DATE "\n");
    
    temp = (UInt16 *)SzAlloc(NULL, tempSize * sizeof(temp[0]));
    if (!temp)
    {
        LOGE("SzAlloc failed");
        return 1;
    }
    
    allocImp.Alloc = SzAlloc;
    allocImp.Free = SzFree;
    
    allocTempImp.Alloc = SzAllocTemp;
    allocTempImp.Free = SzFreeTemp;
    
    if (InFile_Open(&archiveStream.file, inFile))
    {
        LOGE("can not open input file");
        return 1;
    }
    
    FileInStream_CreateVTable(&archiveStream);
    LookToRead_CreateVTable(&lookStream, False);
    
    lookStream.realStream = &archiveStream.s;
    LookToRead_Init(&lookStream);
    
    CrcGenerateTable();
    
    SzArEx_Init(&db);
    
    res = SzArEx_Open(&db, &lookStream.s, &allocImp, &allocTempImp);
    
    if (res == SZ_OK)
    {
        UInt32 i;
        
        /*
         if you need cache, use these 3 variables.
         if you use external function, you can make these variable as static.
         */
        UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
        Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
        size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */
        
        size_t offset = 0;
        size_t outSizeProcessed = 0;
        CSzFile outFile;
        
        char targetDirPath[ZIP_FULLPATH_MAX] = { 0 };
        strcpy(targetDirPath, outPath);
        size_t targetDirPathLen = strlen(targetDirPath);
        if (targetDirPath[targetDirPathLen - 1] != '/')
        {
            strcat(targetDirPath, STRING_PATH_SEPARATOR);
            targetDirPathLen += 1;
        }
        char* targetDirEnd = &targetDirPath[targetDirPathLen];
        
        for (i = 0; i < db.NumFiles; i++)
        {
            offset = 0;
            outSizeProcessed = 0;
            
            unsigned isDir = SzArEx_IsDir(&db, i);
            SzArEx_GetFileNameUtf16(&db, i, temp);
            
            UInt16 *name = (UInt16 *)temp;
            const UInt16 *destPath = (const UInt16 *)name;
            
            if (isDir)
            {
                MyCreateDir(targetDirPath, targetDirEnd, destPath);
                continue;
            }
            else
            {
                res = SzArEx_Extract(&db, &lookStream.s, i,
                                     &blockIndex, &outBuffer, &outBufferSize,
                                     &offset, &outSizeProcessed,
                                     &allocImp, &allocTempImp);
                if (res != SZ_OK)
                    break;
            }
            
            /*for (size_t j = 0; name[j] != 0; j++)
             {
             if (name[j] == '/')
             {
             name[j] = 0;
             MyCreateDir(outPath, name);
             name[j] = CHAR_PATH_SEPARATOR;
             }
             }*/
            
            if (OutFile_OpenUtf16(&outFile, targetDirPath, targetDirEnd, destPath))
            {
                LOGE("can not open output file");
                res = SZ_ERROR_FAIL;
                break;
            }
            
            size_t processedSize = outSizeProcessed;
            
            if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0 || processedSize != outSizeProcessed)
            {
                LOGE("can not write output file");
                res = SZ_ERROR_FAIL;
                break;
            }
            
            if (File_Close(&outFile))
            {
                LOGE("can not close output file");
                res = SZ_ERROR_FAIL;
                break;
            }
        }
        IAlloc_Free(&allocImp, outBuffer);
    }
    
    SzArEx_Free(&db, &allocImp);
    SzFree(NULL, temp);
    
    File_Close(&archiveStream.file);
    
    if (res == SZ_OK)
    {
        LOGD("\nEverything is Ok\n");
        return 0;
    }
    
    if (res == SZ_ERROR_UNSUPPORTED)
        LOGE("decoder doesn't support this archive");
    else if (res == SZ_ERROR_MEM)
        LOGE("can not allocate memory");
    else if (res == SZ_ERROR_CRC)
        LOGE("CRC error");
    else
        LOGD("\nERROR #%d\n", res);
    
    return 1;
}

#if defined(ANDROID)
#include <jni.h>
extern "C"
{
    JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
    {
        return JNI_VERSION_1_6;
    }
    
    jint Java_Utils_SevenZipHelper_extract7z(JNIEnv *env, jclass thiz, jstring archiveFilePath, jstring outPath)
    {
        const char* cfilePath = (const char*)env->GetStringUTFChars(archiveFilePath, NULL);
        const char* coutPath = (const char*)env->GetStringUTFChars(outPath, NULL);
        jint ret = extract7z(cfilePath, coutPath);
        env->ReleaseStringUTFChars(archiveFilePath, cfilePath);
        env->ReleaseStringUTFChars(outPath, coutPath);
        return ret;
    }
}
#endif

