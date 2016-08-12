#include "JniHelper.h"

#include <android/log.h>

#define  LOG_TAG  "JniHelper"

#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace std;
namespace AndroidExtract7z {

extern "C"{
    static bool _getEnv(JNIEnv **env) {

        bool ret = false;

        do {
            if(!JniHelper::getJavaVM()) {
                LOGE("JavaVM is NULL");
                break;
            }
            
            if (JniHelper::getJavaVM()->GetEnv((void**)env, JNI_VERSION_1_6) != JNI_OK) {
                LOGE("Failed to get the environment using GetEnv()");
                break;
            }

            if (JniHelper::getJavaVM()->AttachCurrentThread(env, 0) < 0) {
                LOGE("Failed to get the environment using AttachCurrentThread()");
                break;
            }
            ret = true;
        } while (false);

        return ret;
    }

    static jclass _getClassID(const char *className, JNIEnv *env) {
        JNIEnv *pEnv = env;
        jclass ret = 0;

        do {
            if (! pEnv) {
                pEnv = JniHelper::getEnv();
                if (! pEnv) {
                    break;
                }
            }

            ret = pEnv->FindClass(className);
            if (! ret) {
                LOGE("Failed to find class of %s", className);
                break;
            }
        } while (0);

        return ret;
    }
}

JavaVM* JniHelper::_psJavaVM = NULL;

JavaVM* JniHelper::getJavaVM() {
    return _psJavaVM;
}

void JniHelper::setJavaVM(JavaVM *javaVM) {
    _psJavaVM = javaVM;
}

JNIEnv* JniHelper::getEnv()
{
    JNIEnv* env = NULL;

    if (! _getEnv(&env))
        env = NULL;

    return env;
}

bool JniHelper::getStaticMethodInfo(JniMethodInfo &methodinfo, const char *className, const char *methodName, const char *paramCode) {
    if ((NULL == className) ||
        (NULL == methodName) ||
        (NULL == paramCode)) {
        return false;
    }

    JNIEnv *pEnv = JniHelper::getEnv();
    if (!pEnv) {
        LOGE("Failed to get JNIEnv");
        return false;
    }

    jclass classID = _getClassID(className, pEnv);
    if (! classID) {
        LOGE("Failed to find class %s", className);
        return false;
    }

    jmethodID methodID = pEnv->GetStaticMethodID(classID, methodName, paramCode);
    if (! methodID) {
        if(pEnv->ExceptionCheck()) {
            pEnv->ExceptionClear();
        }
        LOGE("Failed to find static method id of %s", methodName);
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = pEnv;
    methodinfo.methodID = methodID;
    return true;
}

string JniHelper::jstring2string(jstring jstr) {
    if (jstr == NULL) {
        return "";
    }

    JNIEnv *env = JniHelper::getEnv();
    if (NULL == env) {
        return "";
    }

    jclass classID = env->FindClass("java/lang/String");
    jstring strEncode = env->NewStringUTF( "utf-8");
    jmethodID methodID = env->GetMethodID(classID, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray bArr= (jbyteArray)env->CallObjectMethod(jstr, methodID, strEncode);
    jsize arrLen = env->GetArrayLength(bArr);

    if (arrLen > 0) {
        jbyte* arrBuf = env->GetByteArrayElements(bArr, NULL);
        std::string ret((const char*)arrBuf, (size_t)arrLen);
        env->ReleaseByteArrayElements(bArr, arrBuf, 0);

        env->DeleteLocalRef(classID);
        env->DeleteLocalRef(strEncode);
        env->DeleteLocalRef(bArr);
        return ret;
    }

    env->DeleteLocalRef(classID);
    env->DeleteLocalRef(strEncode);
    env->DeleteLocalRef(bArr);
    return "";
}

jstring JniHelper::newStringUTF(JNIEnv* env, const std::string& utf8Str) {
    if (NULL == env) {
        return NULL;
    }

    jclass clazz = env->FindClass("java/lang/String");
    jmethodID id = env->GetMethodID(clazz, "<init>", "([BLjava/lang/String;)V");

    jstring encoding = env->NewStringUTF( "utf-8");
    size_t utf8StrLen = utf8Str.length();
    jbyteArray bytes = env->NewByteArray(utf8StrLen);
    env->SetByteArrayRegion(bytes, 0, utf8StrLen, (jbyte*)utf8Str.c_str());

    jstring ret = (jstring)env->NewObject(clazz, id , bytes, encoding);

    env->DeleteLocalRef(bytes);
    env->DeleteLocalRef(clazz);
    env->DeleteLocalRef(encoding);

    return ret;
}

} // AndroidExtract7z::
