/* 
 * SafeJNI is licensed under MIT licensed. See LICENSE.md file for more information.
 * Copyright (c) 2014 MortimerGoro
 * Copyright (c) 2019 xqyphp
*/

#pragma once

#include <jni.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <exception>
#include <cstdint>


namespace safejni {


#pragma mark Compile Time Strings

//Compile Time String
template<char... Cs>
struct CompileTimeString {
  static const char *value() {
    static const char a[sizeof...(Cs)] = {Cs...};
    return a;
  }
};

//Concatenate 2 Strings
template<class L, class R>
struct Concatenate2;

template<char... LC, char... RC>
struct Concatenate2<CompileTimeString<LC...>, CompileTimeString<RC...>> {
  using Result = CompileTimeString<LC..., RC...>;
};

//Concatenate N strings
template<typename ...C>
struct Concatenate;

template<typename C1>
struct Concatenate<C1> {
  using Result = C1;
};

template<typename C1, typename C2>
struct Concatenate<C1, C2> {
  using Result = typename Concatenate2<C1, C2>::Result;
};

template<typename C1, typename ...C>
struct Concatenate<C1, C...> {
  using Result = typename Concatenate2<C1, typename Concatenate<C...>::Result>::Result;
};

#pragma mark Utility functions

class JNIException : public std::exception {
public:
  std::string message;

  explicit JNIException(const std::string &message);

  virtual const char *what() const throw();
};

class JNIMethodInfo {
public:
  jclass classId;
  jmethodID methodId;
  bool weak_;
  JNIMethodInfo(jclass classId, jmethodID methodId);
  JNIMethodInfo(jclass classId, jmethodID methodId,bool weak);
  ~JNIMethodInfo();
};

typedef std::shared_ptr<JNIMethodInfo> SPJNIMethodInfo;

class Tools {
private:
  static JavaVM *javaVM;
public:

  static void init(JavaVM *vm);

  static JNIEnv *attachJniEnv();

  static void detachJniEnv();

  static jstring toJString(JNIEnv *env, const char *str) {
    return env->NewStringUTF(str);
  }

  inline static jstring toJString(JNIEnv *env, const std::string &str) {
    return toJString(env, str.c_str());
  }

  static jobjectArray toJObjectArray(JNIEnv *env, const std::vector<std::string> &data);

  static jbyteArray toJObjectArray(JNIEnv *env, const std::vector<uint8_t> &data);

  static jobject toHashMap(JNIEnv *env, const std::map<std::string, std::string> &data);

  static std::string toString(JNIEnv *env, jstring str);

  static std::vector<std::string> toVectorString(JNIEnv *env, jobjectArray array);

  static std::vector<uint8_t> toVectorByte(JNIEnv *env, jbyteArray);

  static std::vector<float> toVectorFloat(JNIEnv *env, jfloatArray);

  static std::vector<jobject> toVectorJObject(JNIEnv *env, jobjectArray);

  static SPJNIMethodInfo
  getStaticMethodInfo(JNIEnv *env, const std::string &className, const std::string &methodName,
                      const char *signature);

  static SPJNIMethodInfo
  getMethodInfo(JNIEnv *env, const std::string &className, const std::string &methodName,
                const char *signature);

  static SPJNIMethodInfo
  getMethodInfo(JNIEnv *env, jclass classId, const std::string &methodName,
                const char *signature);

  static void checkException(JNIEnv *env);

  static void clearException(JNIEnv *env);

};

void init(JavaVM *javaVM, JNIEnv *env);

class JNIObject {
public:
  virtual ~JNIObject();

  void makeGlobalRef();

  static std::shared_ptr<JNIObject> CreateGlobal(jobject obj);

  static std::shared_ptr<JNIObject> CreateShared(jobject obj);

  static std::shared_ptr<JNIObject> CreateWeak(jobject obj);

  static std::shared_ptr<JNIObject> CreateLocal(jobject obj);

  template<typename... Args>
  static std::shared_ptr<JNIObject> NewObject(const std::string &className,
                                              const std::string &signature, Args ...v);

  template<typename... Args>
  static std::shared_ptr<JNIObject> NewObject(jclass classId,
                                              const std::string &signature, Args ...v);

  JNIObject *SetClassName(const std::string &className);

  JNIObject *SetMemberSignature(const std::string &memberSignature);

  template<typename T = void, typename... Args>
  inline T Call(const std::string &methodName, Args... v);

  template<typename T>
  inline T Get(const std::string &propertyName);

  template<typename T>
  inline void Set(const std::string &propertyName, T value);

  jobject instance = nullptr;
  std::string className_;
  std::string memberSignature_;
public:
  bool globalRef = false;
  bool weak = false;
};

class JNIObject;

typedef std::shared_ptr<JNIObject> JNIObjectPtr;


class NameAndSignatureClear {
public:
  explicit NameAndSignatureClear(JNIObject *instance) : instance_(instance) {

  }

  ~NameAndSignatureClear() {
    instance_->className_.clear();
    instance_->memberSignature_.clear();
  }

  JNIObject *instance_;
};

#pragma mark C++ To JNI conversion templates

template<typename T>
struct JNIParamsCheck {
  inline static bool needDeleteLocal(T obj) { return true; }
};

template<>
struct JNIParamsCheck<JNIObjectPtr> {
  inline static bool needDeleteLocal(JNIObjectPtr obj) { return false; }
};

//default template
template<typename T>
struct CPPToJNIConversor {
  inline static void convert(JNIEnv *env, T obj) = delete;
};

//generic pointer implementation 禁止转换，需要传递的时候手工转换
template<typename T>
struct CPPToJNIConversor<T *> {
  using JNIType = CompileTimeString<'J'>;

  inline static jlong convert(JNIEnv *env, T *obj) = delete;
};

//object implementations
template<>
struct CPPToJNIConversor<std::string> {
  using JNIType = CompileTimeString<'L', 'j', 'a', 'v', 'a', '/', 'l', 'a', 'n', 'g', '/', 'S', 't', 'r', 'i', 'n', 'g', ';'>;

  inline static jstring convert(JNIEnv *env, const std::string &obj) {
    return Tools::toJString(env, obj);
  }
};

template<>
struct CPPToJNIConversor<const char *> {
  using JNIType = CompileTimeString<'L', 'j', 'a', 'v', 'a', '/', 'l', 'a', 'n', 'g', '/', 'S', 't', 'r', 'i', 'n', 'g', ';'>;

  inline static jstring convert(JNIEnv *env, const char *obj) { return Tools::toJString(env, obj); }
};

template<>
struct CPPToJNIConversor<std::vector<std::string>> {
  using JNIType = CompileTimeString<'[', 'L', 'j', 'a', 'v', 'a', '/', 'l', 'a', 'n', 'g', '/', 'S', 't', 'r', 'i', 'n', 'g', ';'>;

  inline static jobjectArray
  convert(JNIEnv *env, const std::vector<std::string> &obj) {
    return Tools::toJObjectArray(env, obj);
  }
};

template<>
struct CPPToJNIConversor<std::vector<uint8_t>> {
  using JNIType = CompileTimeString<'[', 'B'>;

  inline static jbyteArray
  convert(JNIEnv *env, const std::vector<uint8_t> &obj) { return Tools::toJObjectArray(env, obj); }
};

template<>
struct CPPToJNIConversor<JNIObjectPtr> {
  using JNIType = CompileTimeString<'L', 'j', 'a', 'v', 'a', '/', 'l', 'a', 'n', 'g', '/', 'O', 'b', 'j', 'e', 'c', 't', ';'>;

  inline static jobject convert(JNIEnv *env, const JNIObjectPtr &obj) { return obj->instance; }
};

template<>
struct CPPToJNIConversor<std::vector<jobject>> {
  using JNIType = CompileTimeString<'[', 'L', 'j', 'a', 'v', 'a', '/', 'l', 'a', 'n', 'g', '/', 'O', 'b', 'j', 'e', 'c', 't', ';'>;

  inline static jbyteArray
  convert(JNIEnv *env, const std::vector<uint8_t> &obj) { return Tools::toJObjectArray(env, obj); }
};

template<>
struct CPPToJNIConversor<std::map<std::string, std::string>> {
  using JNIType = CompileTimeString<'L', 'j', 'a', 'v', 'a', '/', 'u', 't', 'i', 'l', '/', 'H', 'a', 's', 'h', 'M', 'a', 'p', ';'>;

  inline static jobject
  convert(JNIEnv *env, const std::map<std::string, std::string> &obj) {
    return Tools::toHashMap(env, obj);
  }
};

//Add more types here

//void conversor

template<>
struct CPPToJNIConversor<void> {
  using JNIType = CompileTimeString<'V'>;
};

//primivite objects implementation

template<>
struct CPPToJNIConversor<int32_t> {
  using JNIType = CompileTimeString<'I'>;

  inline static jint convert(JNIEnv *env, int value) { return static_cast<jint>(value); }
};

template<>
struct CPPToJNIConversor<int64_t> {
  using JNIType = CompileTimeString<'J'>;

  inline static jlong convert(JNIEnv *env, int64_t value) { return static_cast<jlong>(value); }
};

template<>
struct CPPToJNIConversor<float> {
  using JNIType = CompileTimeString<'F'>;

  inline static jfloat convert(JNIEnv *env, float value) { return static_cast<jfloat>(value); }
};

template<>
struct CPPToJNIConversor<double> {
  using JNIType = CompileTimeString<'D'>;

  inline static jdouble convert(JNIEnv *env, double value) { return static_cast<jdouble>(value); }
};

template<>
struct CPPToJNIConversor<bool> {
  using JNIType = CompileTimeString<'Z'>;

  inline static jboolean convert(JNIEnv *env, bool value) { return static_cast<jboolean>(value); }
};

template<>
struct CPPToJNIConversor<int8_t> {
  using JNIType = CompileTimeString<'B'>;

  inline static jbyte convert(JNIEnv *env, int8_t value) { return static_cast<jbyte>(value); }
};

template<>
struct CPPToJNIConversor<uint8_t> {
  using JNIType = CompileTimeString<'C'>;

  inline static jchar convert(JNIEnv *env, uint8_t value) { return static_cast<uint8_t>(value); }
};

template<>
struct CPPToJNIConversor<int16_t> {
  using JNIType = CompileTimeString<'S'>;

  inline static jshort convert(JNIEnv *env, int16_t value) { return static_cast<jshort>(value); }
};


#pragma mark JNI To C++ conversion templates

template<typename T>
struct JNIToCPPConversor {
  inline static void convert(JNIEnv *env, T);

  inline static const char *jniTypeName();
};

template<>
struct JNIToCPPConversor<std::string> {
  inline static std::string convert(JNIEnv *env, jobject obj) {
    return Tools::toString(env, (jstring) obj);
  }
};

template<>
struct JNIToCPPConversor<std::vector<std::string>> {
  inline static std::vector<std::string> convert(JNIEnv *env, jobject obj) {
    return Tools::toVectorString(env, (jobjectArray) obj);
  }
};

template<>
struct JNIToCPPConversor<std::vector<uint8_t>> {
  inline static std::vector<uint8_t> convert(JNIEnv *env, jobject obj) {
    return Tools::toVectorByte(env, (jbyteArray) obj);
  }
};

template<>
struct JNIToCPPConversor<std::vector<jobject>> {
  inline static std::vector<jobject> convert(JNIEnv *env, jobject obj) {
    return Tools::toVectorJObject(env, (jobjectArray) obj);
  }
};


#pragma mark JNI Call Template Specializations

//default implementation (for jobject types)
template<typename T, typename... Args>
struct JNICaller {
  static T callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    auto obj = env->CallStaticObjectMethod(cls, method, v...);
    T result = JNIToCPPConversor<T>::convert(env, obj);
    if (obj)
      env->DeleteLocalRef(obj);
    return result;
  }

  static T callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    auto obj = env->CallObjectMethod(instance, method, v...);
    T result = JNIToCPPConversor<T>::convert(env, obj);
    if (obj)
      env->DeleteLocalRef(obj);
    return result;
  }

  static T getField(JNIEnv *env, jobject instance, jfieldID fid) {
    auto obj = env->GetObjectField(instance, fid);
    T result = JNIToCPPConversor<T>::convert(env, obj);
    if (obj)
      env->DeleteLocalRef(obj);
    return result;
  }

  static T getStaticField(JNIEnv *env, jclass cls, jfieldID fid) {
    auto obj = env->GetStaticObjectField(cls, fid);
    T result = JNIToCPPConversor<T>::convert(env, obj);
    if (obj)
      env->DeleteLocalRef(obj);
    return result;
  }

  static void setField(JNIEnv *env, jobject object, jfieldID fid, T value) {
    auto java_value = CPPToJNIConversor<T>::convert(env, value);
    env->SetObjectField(object, fid, java_value);
  }

};

// Raw jobject implementation (When the user wants one instead of auto conversion)
template<typename... Args>
struct JNICaller<JNIObjectPtr, Args...> {
  static JNIObjectPtr callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    return JNIObject::CreateShared(env->CallStaticObjectMethod(cls, method, v...));
  }

  static JNIObjectPtr
  callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    return JNIObject::CreateShared(env->CallObjectMethod(instance, method, v...));
  }

  static JNIObjectPtr getField(JNIEnv *env, jobject instance, jfieldID fid) {
    return JNIObject::CreateShared(env->GetObjectField(instance, fid));
  }

  static JNIObjectPtr getStaticField(JNIEnv *env, jclass cls, jfieldID fid) {
    return JNIObject::CreateShared(env->GetStaticObjectField(cls, fid));
  }

  static void setField(JNIEnv *pEnv, jobject object, jfieldID fid, const JNIObjectPtr &value) {
    pEnv->SetObjectField(object, fid, value->instance);
  }
};

//generic pointer implementation (using jlong types)
template<typename T, typename... Args>
struct JNICaller<T *, Args...> {
  static T *callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) = delete;

  static T *callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) = delete;

  static T *getField(JNIEnv *env, jobject instance, jfieldID fid) = delete;

  static T *getStaticField(JNIEnv *env, jclass cls, jfieldID fid) = delete;

  static void setField(JNIEnv *pEnv, jobject object, jfieldID fid, T *value) = delete;

};

//void implementation
template<typename... Args>
struct JNICaller<void, Args...> {
  static void callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    env->CallStaticVoidMethod(cls, method, v...);
  }

  static void callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    env->CallVoidMethod(instance, method, v...);
  }

  static void
  callNonVirtual(JNIEnv *env, jobject instance, jclass cls, jmethodID method, Args... v) {
    env->CallNonvirtualVoidMethod(instance, cls, method, v...);
  }

};


//primitive types implementations
template<typename... Args>
struct JNICaller<bool, Args...> {
  static bool callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    return env->CallStaticBooleanMethod(cls, method, v...);
  }

  static bool callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    return env->CallBooleanMethod(instance, method, v...);
  }

  static bool getField(JNIEnv *env, jobject instance, jfieldID fid) {
    return env->GetBooleanField(instance, fid);
  }

  static bool getStaticField(JNIEnv *env, jclass cls, jfieldID fid) {
    return env->GetStaticBooleanField(cls, fid);
  }

  static void setField(JNIEnv *pEnv, jobject object, jfieldID fid, bool value) {
    pEnv->SetBooleanField(object, fid, static_cast<jboolean>(value));
  }

};

template<typename... Args>
struct JNICaller<int8_t, Args...> {
  static int8_t callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    return env->CallStaticByteMethod(cls, method, v...);
  }

  static int8_t callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    return env->CallByteMethod(instance, method, v...);
  }

  static int8_t getField(JNIEnv *env, jobject instance, jfieldID fid) {
    return env->GetByteField(instance, fid);
  }

  static int8_t getStaticField(JNIEnv *env, jclass cls, jfieldID fid) {
    return env->GetStaticByteField(cls, fid);
  }
};

template<typename... Args>
struct JNICaller<uint8_t, Args...> {
  static uint8_t callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    return env->CallStaticCharMethod(cls, method, v...);
  }

  static uint8_t callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    return env->CallCharMethod(instance, method, v...);
  }

  static uint8_t getField(JNIEnv *env, jobject instance, jfieldID fid) {
    return env->GetCharField(instance, fid);
  }

  static uint8_t getStaticField(JNIEnv *env, jclass cls, jfieldID fid) {
    return env->GetStaticCharField(cls, fid);
  }
};

template<typename... Args>
struct JNICaller<int16_t, Args...> {
  static int16_t callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    return env->CallStaticShortMethod(cls, method, v...);
  }

  static int16_t callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    return env->CallShortMethod(instance, method, v...);
  }

  static int16_t getField(JNIEnv *env, jobject instance, jfieldID fid) {
    return env->GetShortField(instance, fid);
  }

  static int16_t getStaticField(JNIEnv *env, jclass cls, jfieldID fid) {
    return env->GetStaticShortField(cls, fid);
  }
};

template<typename... Args>
struct JNICaller<int32_t, Args...> {
  static int32_t callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    return env->CallStaticIntMethod(cls, method, v...);
  }

  static int32_t callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    return env->CallIntMethod(instance, method, v...);
  }

  static int32_t getField(JNIEnv *env, jobject instance, jfieldID fid) {
    return env->GetIntField(instance, fid);
  }

  static int32_t getStaticField(JNIEnv *env, jclass cls, jfieldID fid) {
    return env->GetStaticIntField(cls, fid);
  }
};

template<typename... Args>
struct JNICaller<int64_t, Args...> {
  static int64_t callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    return env->CallStaticLongMethod(cls, method, v...);
  }

  static int64_t callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    return env->CallLongMethod(instance, method, v...);
  }

  static int64_t getField(JNIEnv *env, jobject instance, jfieldID fid) {
    return env->GetLongField(instance, fid);
  }

  static int64_t getStaticField(JNIEnv *env, jclass cls, jfieldID fid) {
    return env->GetStaticLongField(cls, fid);
  }
};

template<typename... Args>
struct JNICaller<float, Args...> {
  static float callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    return env->CallStaticFloatMethod(cls, method, v...);
  }

  static float callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    return env->CallFloatMethod(instance, method, v...);
  }

  static float getField(JNIEnv *env, jobject instance, jfieldID fid) {
    return env->GetFloatField(instance, fid);
  }

  static float getStaticField(JNIEnv *env, jclass cls, jfieldID fid) {
    return env->GetStaticFloatField(cls, fid);
  }
};

template<typename... Args>
struct JNICaller<double, Args...> {
  static double callStatic(JNIEnv *env, jclass cls, jmethodID method, Args... v) {
    return env->CallStaticDoubleMethod(cls, method, v...);
  }

  static double callInstance(JNIEnv *env, jobject instance, jmethodID method, Args... v) {
    return env->CallDoubleMethod(instance, method, v...);
  }

  static double getField(JNIEnv *env, jobject instance, jfieldID fid) {
    return env->GetDoubleField(instance, fid);
  }

  static double getStaticField(JNIEnv *env, jclass cls, jfieldID fid) {
    return env->GetStaticDoubleField(cls, fid);
  }
};

#pragma mark JNI Signature Utilities

//helper method to append a JNI parameter signature to a buffer
template<typename T>
char appendJNIParamSignature(std::string &buffer) {
  buffer += CPPToJNIConversor<T>::jniTypeName();
}

//deduces the signature of a JNI method according to the variadic params and the return type
template<typename T, typename... Args>
inline const char *getJNISignature(Args...) {
  return Concatenate<CompileTimeString<'('>, //left parenthesis
          typename CPPToJNIConversor<Args>::JNIType..., //params signature
          CompileTimeString<')'>, //right parenthesis
          typename CPPToJNIConversor<T>::JNIType,
          CompileTimeString<'\0'>> //return type signature
  ::Result::value();
}

#pragma mark JNI Param Destructor Templates

//Helper object to destroy parameters converter to JNI
template<uint8_t NUM_PARAMS>
struct JNIParamDestructor {
  JNIEnv *jniEnv;
  jobject jniParams[NUM_PARAMS] = {0};
  int currentIndex;

  explicit JNIParamDestructor(JNIEnv *env) : jniEnv(env), currentIndex(0) {

  }

  void add(jobject jniObject) {
    jniParams[currentIndex++] = jniObject;
  }

  ~JNIParamDestructor() {
    for (int i = 0; i < NUM_PARAMS; ++i) {
      if (jniParams[i])
        jniEnv->DeleteLocalRef(jniParams[i]);
    }
    Tools::clearException(jniEnv);
  }
};

//optimized base case for the destructor
template<>
struct JNIParamDestructor<0> {
  JNIEnv *jniEnv;

  explicit JNIParamDestructor(JNIEnv *env) : jniEnv(env) {}

  ~JNIParamDestructor() {
    Tools::clearException(jniEnv);
  }
};


//helper template to decide when a jni type must be destroyed (by adding the ref to the JNIParamDestructor struct);
template<typename T, typename D>
struct JNIDestructorDecider {
  //by default the template ignores destruction (for primivitve types)
  inline static void decide(T jniParam, D &destructor) {}
};

template<typename D>
struct JNIDestructorDecider<jobject, D> {
  inline static void decide(jobject obj, D &destructor) { destructor.add(obj); }
};

template<typename D>
struct JNIDestructorDecider<jbyteArray, D> {
  inline static void decide(jbyteArray obj, D &destructor) { destructor.add((jobject) obj); }
};

template<typename D>
struct JNIDestructorDecider<jobjectArray, D> {
  inline static void decide(jobjectArray obj, D &destructor) {
    destructor.add((jobject) obj);
  }
};

template<typename D>
struct JNIDestructorDecider<jstring, D> {
  inline static void decide(jstring obj, D &destructor) { destructor.add((jobject) obj); }
};

#pragma mark JNI Param Conversor Utility Template

//JNI param conversor helper: Converts the parameter to JNI and adds it to the destructor if needed
template<typename T, typename D>
auto
JNIParamConversor(JNIEnv *env, const T &arg,
                  D &destructor) -> decltype(CPPToJNIConversor<T>::convert(env, arg)) {
  auto result = CPPToJNIConversor<T>::convert(env, arg);
  if (JNIParamsCheck<T>::needDeleteLocal(arg)) {
    JNIDestructorDecider<decltype(CPPToJNIConversor<T>::convert(env, arg)), D>::decide(result,
                                                                                       destructor);
  }

  return result;
}

#pragma mark Public API

//generic call to static method
template<typename T = void, typename... Args>
T CallStatic(const std::string &className,
                const std::string &methodName, const std::string &signature, Args... v) {
  static constexpr uint8_t nargs = sizeof...(Args);
  JNIEnv *jniEnv = Tools::attachJniEnv();

  const char *sig;
  if(signature.empty()){
    sig = getJNISignature<T, Args...>(v...);
  } else{
    sig = signature.c_str();
  }

  SPJNIMethodInfo methodInfo = Tools::getStaticMethodInfo(jniEnv, className, methodName,
                                                          signature.c_str());
  JNIParamDestructor<nargs> paramDestructor(jniEnv);
  return JNICaller<T, decltype(CPPToJNIConversor<Args>::convert(jniEnv, v))...>::callStatic(jniEnv,
                                                                                            methodInfo->classId,
                                                                                            methodInfo->methodId,
                                                                                            JNIParamConversor<Args>(
                                                                                                    jniEnv,
                                                                                                    v,
                                                                                                    paramDestructor)...);
}

//generic call to instance method
template<typename T = void, typename... Args>
T Call(jobject instance,
       jclass classId, const std::string &methodName,
       const std::string &signature, Args... v) {
  static constexpr uint8_t nargs = sizeof...(Args);
  JNIEnv *jniEnv = Tools::attachJniEnv();
  const char *sig;
  if (signature.empty()) {
    sig = getJNISignature<T, Args...>(v...);
  }else{
    sig = signature.c_str();
  }

  SPJNIMethodInfo methodInfo = Tools::getMethodInfo(jniEnv, classId, methodName,
                                                    sig);
  JNIParamDestructor<nargs> paramDestructor(jniEnv);
  return JNICaller<T, decltype(CPPToJNIConversor<Args>::convert(jniEnv, v))...>::callInstance(
          jniEnv,
          instance,
          methodInfo->methodId,
          JNIParamConversor<Args>(
                  jniEnv,
                  v,
                  paramDestructor)...);
}


//generic call to instance method
template<typename T = void, typename... Args>
T callRawNonVirtual(jobject instance,
                    const std::string &className, const std::string &methodName,
                    const std::string &signature, Args... v) {

  static constexpr uint8_t nargs = sizeof...(Args);
  JNIEnv *jniEnv = Tools::attachJniEnv();
  SPJNIMethodInfo methodInfo = Tools::getMethodInfo(jniEnv, className, methodName,
                                                    signature.c_str());
  JNIParamDestructor<nargs> paramDestructor(jniEnv);
  return JNICaller<T, decltype(CPPToJNIConversor<Args>::convert(jniEnv, v))...>::callNonVirtual(
          jniEnv,
          instance,
          methodInfo->classId,
          methodInfo->methodId,
          JNIParamConversor<Args>(
                  jniEnv,
                  v,
                  paramDestructor)...);
}

template<typename T>
T Get(jobject instance,
      const std::string &propertyName, const std::string &signature) {
  const char *sig;
  if (signature.empty()) {
    sig = Concatenate<typename CPPToJNIConversor<T>::JNIType,
            CompileTimeString<'\0'>> //return type signature
    ::Result::value();
  } else {
    sig = signature.c_str();
  }

  JNIEnv *jniEnv = Tools::attachJniEnv();
  jclass clazz = jniEnv->GetObjectClass(instance);
  jfieldID fid = jniEnv->GetFieldID(clazz, propertyName.c_str(), sig);
  if (clazz) {
    jniEnv->DeleteLocalRef(clazz);
  }
  return JNICaller<T>::getField(jniEnv, instance, fid);

}

template<typename T>
void Set(jobject instance, const std::string &propertyName, const std::string &signature, T value) {

  const char *sig;
  if (signature.empty()) {
    sig = Concatenate<typename CPPToJNIConversor<T>::JNIType,
            CompileTimeString<'\0'>> //return type signature
    ::Result::value();
  } else {
    sig = signature.c_str();
  }


  JNIEnv *jniEnv = Tools::attachJniEnv();
  jclass clazz = jniEnv->GetObjectClass(instance);
  jfieldID fid = jniEnv->GetFieldID(clazz, propertyName.c_str(), sig);
  if (clazz) {
    jniEnv->DeleteLocalRef(clazz);
  }
  JNICaller<T>::setField(jniEnv, instance, fid, value);
}

template<typename T>
T GetStatic(const std::string &className, const std::string &propertyName,const std::string &signature) {
  const char *sig = signature.c_str();
  if(signature.empty()){
    sig = Concatenate<typename CPPToJNIConversor<T>::JNIType,
            CompileTimeString<'\0'>> //return type signature
    ::Result::value();
  }

  JNIEnv *jniEnv = Tools::attachJniEnv();
  jclass clazz = jniEnv->FindClass(className.c_str());
  jfieldID fid = jniEnv->GetStaticFieldID(clazz, propertyName.c_str(), sig);
  T ret = JNICaller<T>::getStaticField(jniEnv, clazz, fid);
  if (clazz) {
    jniEnv->DeleteLocalRef(clazz);
  }
  return ret;
}


inline void RegisterNatives(const std::string &clsName, JNINativeMethod *mtdList, int mtdCount) {

  JNIEnv *jniEnv = Tools::attachJniEnv();
  jclass clazz = jniEnv->FindClass(clsName.c_str());
  if (clazz == nullptr) {
    throw JNIException("class not find");
  }
  if (jniEnv->RegisterNatives(clazz, mtdList, mtdCount) < 0) {
    throw JNIException("register failed");
  }
  Tools::checkException(jniEnv);
}

// JNIObject templates

template<typename... Args>
std::shared_ptr<JNIObject> JNIObject::NewObject(const std::string &className,
                                                const std::string &signature, Args ...v) {

  constexpr uint8_t nargs = sizeof...(Args);

  auto result = std::make_shared<JNIObject>();

  JNIEnv *jniEnv = Tools::attachJniEnv();
  std::string signature_ = signature;
  if (signature.empty()) {
    signature_ = getJNISignature<void, Args...>(v...);
  }

  SPJNIMethodInfo methodInfo = Tools::getMethodInfo(jniEnv, className, "<init>",
                                                    signature_.c_str());
  JNIParamDestructor<nargs> paramDestructor(jniEnv);
  result->instance = jniEnv->NewObject(methodInfo->classId, methodInfo->methodId,
                                       JNIParamConversor<Args>(jniEnv, v, paramDestructor)...);
  result->makeGlobalRef();
  return result;
}

template<typename... Args>
std::shared_ptr<JNIObject> JNIObject::NewObject(jclass classId,
                                            const std::string &signature, Args ...v)
{
  constexpr uint8_t nargs = sizeof...(Args);

  auto result = std::make_shared<JNIObject>();

  JNIEnv *jniEnv = Tools::attachJniEnv();
  std::string signature_ = signature;
  if (signature.empty()) {
    signature_ = getJNISignature<void, Args...>(v...);
  }

  SPJNIMethodInfo methodInfo = Tools::getMethodInfo(jniEnv, classId, "<init>",
                                                    signature_.c_str());
  JNIParamDestructor<nargs> paramDestructor(jniEnv);
  result->instance = jniEnv->NewObject(methodInfo->classId, methodInfo->methodId,
                                       JNIParamConversor<Args>(jniEnv, v, paramDestructor)...);
  result->makeGlobalRef();
  return result;
}

template<typename T, typename... Args>
inline T JNIObject::Call(const std::string &methodName, Args... v) {
  JNIEnv *jniEnv = Tools::attachJniEnv();
  NameAndSignatureClear clear(this);

  jclass classId;
  if (className_.empty()) {
    classId = jniEnv->GetObjectClass(instance);
  } else {
    classId = jniEnv->FindClass(className_.c_str());
  }

  return safejni::Call<T, Args...>(instance, classId, methodName, memberSignature_, v...);
}


template<typename T>
inline T JNIObject::Get(const std::string &propertyName) {
  NameAndSignatureClear clear(this);
  return safejni::Get<T>(instance, propertyName, memberSignature_);


}

template<typename T>
inline void JNIObject::Set(const std::string &propertyName, T value) {
  NameAndSignatureClear clear(this);
  safejni::Set(instance, propertyName, memberSignature_, value);
}

}
