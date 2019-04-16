/* 
 * SafeJNI is licensed under MIT licensed. See LICENSE.md file for more information.
 * Copyright (c) 2014 MortimerGoro
*/


#include "safejni.h"

#include <jni.h>
#include <cstdlib>
#include <android/log.h>


#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , "[CYAP:SafeJNI]",__VA_ARGS__)

using std::string;
using std::vector;

namespace safejni {


JavaVM *Tools::javaVM = 0;


void init(JavaVM *vm, JNIEnv *env) {
  Tools::init(vm);
}

JNIMethodInfo::JNIMethodInfo(jclass classId, jmethodID methodId) : classId(classId),
                                                                   methodId(methodId),
                                                                   weak_(false){

}

JNIMethodInfo::JNIMethodInfo(jclass classId, jmethodID methodId,bool weak): classId(classId),
                                                                            methodId(methodId),
                                                                            weak_(weak){

}

JNIMethodInfo::~JNIMethodInfo() {
  if(weak_){
    return;
  }
  if (classId) {
    Tools::attachJniEnv()->DeleteLocalRef(classId);
  }
}


JNIException::JNIException(const std::string &message) : message(message) {
  LOGE("JNI Exception: %s", message.c_str());
}

const char *JNIException::what() const throw() {
  return message.c_str();
}

void Tools::init(JavaVM *vm) {
  javaVM = vm;
}

JNIEnv *Tools::attachJniEnv() {
  JNIEnv *jniEnv;
  if (javaVM) {
    int status = javaVM->AttachCurrentThread(&jniEnv, NULL);
    if (status < 0) {
      throw JNIException("Could not attach the JNI environment to the current thread.");
    }
  }

  return jniEnv;
}

void Tools::detachJniEnv() {
  if (javaVM) {
    int status = javaVM->DetachCurrentThread();
    if (status < 0) {
      throw JNIException("Could not detach the JNI environment to the current thread.");
    }
  }
}


jobjectArray Tools::toJObjectArray(JNIEnv *env, const std::vector<std::string> &data) {
  jclass classId = env->FindClass("java/lang/String");
  jint size = data.size();
  jobjectArray joa = env->NewObjectArray(size, classId, 0);

  for (int i = 0; i < size; i++) {
    jstring jstr = toJString(env, data[i]);
    env->SetObjectArrayElement(joa, i, jstr);

  }
  env->DeleteLocalRef(classId);

  checkException(env);
  return joa;
}

jbyteArray Tools::toJObjectArray(JNIEnv *env, const std::vector<uint8_t> &data) {
  jbyteArray jba = env->NewByteArray(data.size());
  env->SetByteArrayRegion(jba, 0, data.size(), (const jbyte *) &data[0]);
  checkException(env);
  return jba;
}

jobject Tools::toHashMap(JNIEnv *env, const std::map<std::string, std::string> &data) {
  jclass classId = env->FindClass("java/util/HashMap");
  jmethodID methodId = env->GetMethodID(classId, "<init>", "()V");
  jobject hashmap = env->NewObject(classId, methodId);

  methodId = env->GetMethodID(classId, "put",
                              "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
  for (auto &item : data) {
    jstring key = env->NewStringUTF(item.first.c_str());
    jstring value = env->NewStringUTF(item.second.c_str());
    env->CallObjectMethod(hashmap, methodId, key, value);

    env->DeleteLocalRef(key);
    env->DeleteLocalRef(value);
  }
  env->DeleteLocalRef(classId);
  checkException(env);
  return hashmap;
}

std::string Tools::toString(JNIEnv *env, jstring str) {
  if (!str) {
    return std::string();
  }
  jboolean isCopy;
  const char *chars = env->GetStringUTFChars(str, &isCopy);
  std::string s;
  if (chars) {
    s = chars;
    env->ReleaseStringUTFChars(str, chars);
  }
  checkException(env);
  return s;
}

std::vector<std::string> Tools::toVectorString(JNIEnv *env, jobjectArray array) {
  std::vector<std::string> result;
  if (array) {
    jint length = env->GetArrayLength(array);

    for (int i = 0; i < length; i++) {
      jobject valueJObject = env->GetObjectArrayElement(array, i);
      result.push_back(toString(env, (jstring) valueJObject));
      env->DeleteLocalRef(valueJObject);
    }
  }
  checkException(env);
  return result;
}

std::vector<uint8_t> Tools::toVectorByte(JNIEnv *env, jbyteArray array) {
  if (!array) {
    return std::vector<uint8_t>();
  }
  jsize size = env->GetArrayLength(array);
  std::vector<uint8_t> result(size);
  env->GetByteArrayRegion(array, 0, size, (jbyte *) &result[0]);
  checkException(env);
  return result;
}

std::vector<float> Tools::toVectorFloat(JNIEnv *env, jfloatArray array) {
  if (!array) {
    return std::vector<float>();
  }
  jsize size = env->GetArrayLength(array);
  std::vector<float> result(size);
  env->GetFloatArrayRegion(array, 0, size, &result[0]);
  checkException(env);
  return result;
}

std::vector<jobject> Tools::toVectorJObject(JNIEnv *env, jobjectArray array) {
  std::vector<jobject> result;
  if (array) {
    jint length = env->GetArrayLength(array);

    for (int i = 0; i < length; i++) {
      jobject valueJObject = env->GetObjectArrayElement(array, i);
      result.push_back(valueJObject);
    }
  }
  return result;
}

SPJNIMethodInfo
Tools::getStaticMethodInfo(JNIEnv *env, const string &className, const string &methodName,
                           const char *signature) {
  jclass classId = 0;
  jmethodID methodId = 0;
  classId = env->FindClass(className.c_str());
  checkException(env);

  if (!classId) {
    throw JNIException(string("Could not find the given class: ") + className);
  }

  methodId = env->GetStaticMethodID(classId, methodName.c_str(), signature);
  checkException(env);

  if (!methodId) {
    throw JNIException(string("Could not find the given '") + methodName +
                       string("' static method in the given '") + className +
                       string("' class using the '") + signature + string("' signature."));
  }

  return SPJNIMethodInfo(new JNIMethodInfo(classId, methodId));
}

SPJNIMethodInfo
Tools::getMethodInfo(JNIEnv *env, const string &className, const string &methodName,
                     const char *signature) {
  jclass classId = env->FindClass(className.c_str());
  checkException(env);
  if (!classId) {
    throw JNIException(string("Could not find the given class: ") + className);
  }

  jmethodID methodId = 0;
  checkException(env);

  methodId = env->GetMethodID(classId, methodName.c_str(), signature);
  checkException(env);

  if (!methodId) {
    throw JNIException(string("Could not find the given '") + methodName +
                       string("' static method in the given classId'")  +
                       string("' class using the '") + signature + string("' signature."));
  }

  return SPJNIMethodInfo(new JNIMethodInfo(classId, methodId));
}

SPJNIMethodInfo
Tools::getMethodInfo(JNIEnv *env, jclass classId, const string &methodName,
                     const char *signature) {
  jmethodID methodId = 0;
  checkException(env);

  methodId = env->GetMethodID(classId, methodName.c_str(), signature);
  checkException(env);

  if (!methodId) {
    throw JNIException(string("Could not find the given '") + methodName +
                       string("' static method in the given classId'")  +
                       string("' class using the '") + signature + string("' signature."));
  }

  return SPJNIMethodInfo(new JNIMethodInfo(classId, methodId, true));
}

void Tools::checkException(JNIEnv *env) {
  if (env->ExceptionCheck()) {
    jthrowable jthrowable = env->ExceptionOccurred();
    env->ExceptionDescribe();
    env->ExceptionClear();
    SPJNIMethodInfo methodInfo = getMethodInfo(env, "java/lang/Throwable", "getMessage",
                                               "()Ljava/lang/String;");
    string exceptionMessage = toString(env,
                                       reinterpret_cast<jstring>(env->CallObjectMethod(jthrowable,
                                                                                       methodInfo->methodId)));
    throw JNIException(exceptionMessage);
  }
}

void Tools::clearException(JNIEnv *env) {
  if (env->ExceptionCheck()) {
    jthrowable jthrowable = env->ExceptionOccurred();
    env->ExceptionDescribe();
    env->ExceptionClear();
    SPJNIMethodInfo methodInfo = getMethodInfo(env, "java/lang/Throwable", "getMessage",
                                               "()Ljava/lang/String;");
    string exceptionMessage = toString(env,
                                       reinterpret_cast<jstring>(env->CallObjectMethod(jthrowable,
                                                                                       methodInfo->methodId)));
    LOGE("%s", exceptionMessage.c_str());
  }
}
// JNIObject
JNIObject::~JNIObject() {
  if(weak){
    return;
  }
  if (instance) {
    JNIEnv *jniEnv = Tools::attachJniEnv();
    if (globalRef) {
      jniEnv->DeleteGlobalRef(instance);
    } else {
      jniEnv->DeleteLocalRef(instance);
    }
  }
}

void JNIObject::makeGlobalRef() {
  if (!globalRef) {
    this->instance = Tools::attachJniEnv()->NewGlobalRef(this->instance);
    globalRef = true;
  }
}

std::shared_ptr<JNIObject> JNIObject::CreateGlobal(jobject obj) {
  JNIObject *result = new JNIObject();
  JNIEnv *jniEnv = Tools::attachJniEnv();
  result->instance = jniEnv->NewGlobalRef(obj);
  result->makeGlobalRef();
  return std::shared_ptr<JNIObject>(result);
}

std::shared_ptr<JNIObject> JNIObject::CreateShared(jobject obj) {
  JNIObject *result = new JNIObject();
  result->instance = obj;
  return std::shared_ptr<JNIObject>(result);
}

std::shared_ptr<JNIObject> JNIObject::CreateWeak(jobject obj)
{
  JNIObject *result = new JNIObject();
  result->instance = obj;
  result->weak = true;
  return std::shared_ptr<JNIObject>(result);
}

std::shared_ptr<JNIObject> JNIObject::CreateLocal(jobject obj) {
  JNIEnv *jniEnv = Tools::attachJniEnv();
  JNIObject *result = new JNIObject();
  result->instance = jniEnv->NewLocalRef(obj);
  return std::shared_ptr<JNIObject>(result);
}

JNIObject *JNIObject::SetClassName(const std::string& className)
{
  className_ = className;
  return this;
}

JNIObject *JNIObject::SetMemberSignature(const std::string& callSignature)
{
  memberSignature_ = callSignature;
  return this;
}


}
