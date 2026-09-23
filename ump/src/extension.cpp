// Extension lib defines
#define EXTENSION_NAME ump
#define LIB_NAME "ump"
#define MODULE_NAME LIB_NAME

// Defold SDK
#define DLIB_LOG_DOMAIN LIB_NAME
#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

enum Operation { REQUEST_CONSENT = 1, PRIVACY_OPTIONS = 2 };

struct Completion {
    int m_Operation;
    int m_Id;
    bool m_Success;
};

static dmMutex::HMutex g_Mutex;
static dmArray<Completion> g_Completions;
static bool g_AcceptCompletions = false;
static int g_NextId = 0;
static int g_RequestId = 0;
static int g_PrivacyId = 0;
static dmScript::LuaCallbackInfo* g_RequestCallback = 0;
static dmScript::LuaCallbackInfo* g_PrivacyCallback = 0;

extern "C" JNIEXPORT void JNICALL Java_com_defold_umpext_UMPExtension_onNativeCompletion(
        JNIEnv* env, jclass cls, jint operation, jint id, jboolean success)
{
    DM_MUTEX_SCOPED_LOCK(g_Mutex);
    if (!g_AcceptCompletions) return;
    if (g_Completions.Full()) g_Completions.OffsetCapacity(4);
    Completion completion = {(int)operation, (int)id, success == JNI_TRUE};
    g_Completions.Push(completion);
}

static void InvokeCompletion(dmScript::LuaCallbackInfo* callback, bool success)
{
    if (!callback) return;
    if (dmScript::IsCallbackValid(callback) && dmScript::SetupCallback(callback)) {
        lua_State* L = dmScript::GetCallbackLuaContext(callback);
        lua_pushboolean(L, success);
        dmScript::PCall(L, 2, 0);
        dmScript::TeardownCallback(callback);
    }
    dmScript::DestroyCallback(callback);
}

static dmExtension::Result UpdateExtension(dmExtension::Params* params)
{
    dmArray<Completion> completions;
    {
        DM_MUTEX_SCOPED_LOCK(g_Mutex);
        completions.Swap(g_Completions);
    }
    for (uint32_t i = 0; i < completions.Size(); ++i) {
        const Completion& completion = completions[i];
        dmScript::LuaCallbackInfo** slot = 0;
        if (completion.m_Operation == REQUEST_CONSENT && completion.m_Id == g_RequestId) {
            slot = &g_RequestCallback;
        } else if (completion.m_Operation == PRIVACY_OPTIONS && completion.m_Id == g_PrivacyId) {
            slot = &g_PrivacyCallback;
        }
        if (slot) {
            dmScript::LuaCallbackInfo* callback = *slot;
            *slot = 0;
            InvokeCompletion(callback, completion.m_Success);
        }
    }
    return dmExtension::RESULT_OK;
}

// Keep these values in sync with UMPExtension.java.
enum UmpConstant {
    REQUEST_STATE_UPDATING = 0,
    REQUEST_STATE_COMPLETE = 1,
    REQUEST_STATE_FAILED = 2,
    REQUEST_STATE_FORM_PENDING = 3,
    PRIVACY_OPTIONS_STATE_NOT_SHOWN = 0,
    PRIVACY_OPTIONS_STATE_SHOWING = 1,
    PRIVACY_OPTIONS_STATE_DISMISSED = 2,
    PRIVACY_OPTIONS_STATE_ERROR = 3,
    GDPR_APPLIES_UNKNOWN = -1,
    GDPR_APPLIES_NO = 0,
    GDPR_APPLIES_YES = 1,
};

static JNIEnv* Attach()
{
    JNIEnv* env;
    JavaVM* vm = dmGraphics::GetNativeAndroidJavaVM();
    vm->AttachCurrentThread(&env, NULL);
    return env;
}

static bool Detach(JNIEnv* env)
{
    bool exception = (bool) env->ExceptionCheck();
    env->ExceptionClear();
    JavaVM* vm = dmGraphics::GetNativeAndroidJavaVM();
    vm->DetachCurrentThread();
    return !exception;
}

namespace {
    struct AttachScope
    {
        JNIEnv* m_Env;
        AttachScope() : m_Env(Attach())
        {
        }
        ~AttachScope()
        {
            Detach(m_Env);
        }
    };
}

static jclass GetClass(JNIEnv* env, const char* classname)
{
    jclass activity_class = env->FindClass("android/app/NativeActivity");
    jmethodID get_class_loader = env->GetMethodID(activity_class, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject cls = env->CallObjectMethod(dmGraphics::GetNativeAndroidActivity(), get_class_loader);
    jclass class_loader = env->FindClass("java/lang/ClassLoader");
    jmethodID find_class = env->GetMethodID(class_loader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    jstring str_class_name = env->NewStringUTF(classname);
    jclass outcls = (jclass)env->CallObjectMethod(cls, find_class, str_class_name);
    env->DeleteLocalRef(str_class_name);
    return outcls;
}

// Request consent info update
static int RequestConsentInfoUpdate(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* testDeviceId = luaL_checkstring(L, 2);
    if (!lua_isnoneornil(L, 3)) luaL_checktype(L, 3, LUA_TFUNCTION);
    if (g_RequestCallback) dmScript::DestroyCallback(g_RequestCallback);
    g_RequestCallback = lua_isnoneornil(L, 3) ? 0 : dmScript::CreateCallback(L, 3);
    g_RequestId = ++g_NextId;
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;

    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "requestConsentInfoUpdate", "(Landroid/app/Activity;ZLjava/lang/String;I)V");

    jobject activity = dmGraphics::GetNativeAndroidActivity();
    bool testDevice = lua_toboolean(L, 1);
    jstring jtestDeviceId = env->NewStringUTF(testDeviceId);
    env->CallStaticVoidMethod(cls, method, activity, testDevice, jtestDeviceId, g_RequestId);
    env->DeleteLocalRef(jtestDeviceId);

    return 0;
}

// Show privacy options form
static int ShowPrivacyOptionsForm(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (!lua_isnoneornil(L, 1)) luaL_checktype(L, 1, LUA_TFUNCTION);
    if (g_PrivacyCallback) dmScript::DestroyCallback(g_PrivacyCallback);
    g_PrivacyCallback = lua_isnoneornil(L, 1) ? 0 : dmScript::CreateCallback(L, 1);
    g_PrivacyId = ++g_NextId;
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;

    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "showPrivacyOptionsForm", "(Landroid/app/Activity;I)V");

    jobject activity = dmGraphics::GetNativeAndroidActivity();
    env->CallStaticVoidMethod(cls, method, activity, g_PrivacyId);

    return 0;
}

// Check if privacy options are required
static int IsPrivacyOptionsRequired(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;

    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "isPrivacyOptionsRequired", "()Z");

    jboolean isRequired = env->CallStaticBooleanMethod(cls, method);
    lua_pushboolean(L, isRequired);

    return 1;
}

// Can request ads
static int CanRequestAds(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;

    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "canRequestAds", "()Z");

    jboolean canRequestAds = env->CallStaticBooleanMethod(cls, method);
    lua_pushboolean(L, canRequestAds);

    return 1;
}

// Initialize Mobile Ads SDK
static int InitializeMobileAdsSdk(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;

    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "initializeMobileAdsSdk", "(Landroid/content/Context;)V");

    jobject context = dmGraphics::GetNativeAndroidActivity();
    env->CallStaticVoidMethod(cls, method, context);

    return 0;
}

// Reset consent information
static int ResetConsentInformation(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;

    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "resetConsentInformation", "()V");

    env->CallStaticVoidMethod(cls, method);

    return 0;
}

// Lua function to get consent status
static int GetConsentStatus(lua_State* L) {
    DM_LUA_STACK_CHECK(L, 1);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;

    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "getConsentStatus", "()I");

    jint consentStatus = env->CallStaticIntMethod(cls, method);
    lua_pushinteger(L, consentStatus);

    return 1;
}


static int GetRequestState(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;
    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "getRequestState", "()I");
    lua_pushinteger(L, env->CallStaticIntMethod(cls, method));
    return 1;
}

static int WasConsentFormRequired(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;
    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "wasConsentFormRequired", "()Z");
    lua_pushboolean(L, env->CallStaticBooleanMethod(cls, method));
    return 1;
}

static int GetPrivacyOptionsState(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;
    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "getPrivacyOptionsState", "()I");
    lua_pushinteger(L, env->CallStaticIntMethod(cls, method));
    return 1;
}

static int GetGdprApplies(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;
    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jobject activity = dmGraphics::GetNativeAndroidActivity();
    jmethodID method = env->GetStaticMethodID(cls, "getGdprApplies", "(Landroid/app/Activity;)I");
    lua_pushinteger(L, env->CallStaticIntMethod(cls, method, activity));
    return 1;
}

static int GetPurposeConsents(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;
    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jobject activity = dmGraphics::GetNativeAndroidActivity();
    jmethodID method = env->GetStaticMethodID(cls, "getPurposeConsents", "(Landroid/app/Activity;)Ljava/lang/String;");
    jstring value = (jstring)env->CallStaticObjectMethod(cls, method, activity);
    if (value) {
        const char* chars = env->GetStringUTFChars(value, NULL);
        lua_pushstring(L, chars);
        env->ReleaseStringUTFChars(value, chars);
        env->DeleteLocalRef(value);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"request_consent_info_update", RequestConsentInfoUpdate},
    {"show_privacy_options_form", ShowPrivacyOptionsForm},
    {"is_privacy_options_required", IsPrivacyOptionsRequired},
    {"can_request_ads", CanRequestAds},
    {"initialize_mobile_ads_sdk", InitializeMobileAdsSdk},
    {"get_consent_status", GetConsentStatus},  // <-- New function
    {"get_request_state", GetRequestState},
    {"was_consent_form_required", WasConsentFormRequired},
    {"get_privacy_options_state", GetPrivacyOptionsState},
    {"get_gdpr_applies", GetGdprApplies},
    {"get_purpose_consents", GetPurposeConsents},
    {"reset_consent_information", ResetConsentInformation},
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);

    #define SETCONSTANT(name) lua_pushinteger(L, name); lua_setfield(L, -2, #name);
    SETCONSTANT(REQUEST_STATE_UPDATING)
    SETCONSTANT(REQUEST_STATE_COMPLETE)
    SETCONSTANT(REQUEST_STATE_FAILED)
    SETCONSTANT(REQUEST_STATE_FORM_PENDING)
    SETCONSTANT(PRIVACY_OPTIONS_STATE_NOT_SHOWN)
    SETCONSTANT(PRIVACY_OPTIONS_STATE_SHOWING)
    SETCONSTANT(PRIVACY_OPTIONS_STATE_DISMISSED)
    SETCONSTANT(PRIVACY_OPTIONS_STATE_ERROR)
    SETCONSTANT(GDPR_APPLIES_UNKNOWN)
    SETCONSTANT(GDPR_APPLIES_NO)
    SETCONSTANT(GDPR_APPLIES_YES)
    #undef SETCONSTANT

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result AppInitializeExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeExtension(dmExtension::Params* params)
{
    if (!g_Mutex) g_Mutex = dmMutex::New();
    {
        DM_MUTEX_SCOPED_LOCK(g_Mutex);
        g_AcceptCompletions = true;
    }
    // Init Lua
    LuaInit(params->m_L);
    printf("Registered %s Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeExtension(dmExtension::Params* params)
{
    {
        DM_MUTEX_SCOPED_LOCK(g_Mutex);
        g_AcceptCompletions = false;
        g_Completions.SetSize(0);
    }
    if (g_RequestCallback) dmScript::DestroyCallback(g_RequestCallback);
    if (g_PrivacyCallback) dmScript::DestroyCallback(g_PrivacyCallback);
    g_RequestCallback = 0;
    g_PrivacyCallback = 0;
    return dmExtension::RESULT_OK;
}

#else

static dmExtension::Result AppInitializeExtension(dmExtension::AppParams* params)
{
    dmLogWarning("Registered %s (null) Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeExtension(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeExtension(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

#endif

#if defined(DM_PLATFORM_ANDROID)
DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, AppInitializeExtension, AppFinalizeExtension, InitializeExtension, UpdateExtension, 0, FinalizeExtension)
#else
DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, AppInitializeExtension, AppFinalizeExtension, InitializeExtension, 0, 0, FinalizeExtension)
#endif
