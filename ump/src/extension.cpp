// Extension lib defines
#define EXTENSION_NAME ump
#define LIB_NAME "ump"
#define MODULE_NAME LIB_NAME

// Defold SDK
#define DLIB_LOG_DOMAIN LIB_NAME
#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)


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
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;

    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "requestConsentInfoUpdate", "(Landroid/app/Activity;ZLjava/lang/String;)V");

    jobject activity = dmGraphics::GetNativeAndroidActivity();
    bool testDevice = lua_toboolean(L, 1);
    const char* testDeviceId = luaL_checkstring(L, 2);

    jstring jtestDeviceId = env->NewStringUTF(testDeviceId);
    env->CallStaticVoidMethod(cls, method, activity, testDevice, jtestDeviceId);
    env->DeleteLocalRef(jtestDeviceId);

    return 0;
}

// Show privacy options form
static int ShowPrivacyOptionsForm(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    AttachScope attachscope;
    JNIEnv* env = attachscope.m_Env;

    jclass cls = GetClass(env, "com.defold.umpext.UMPExtension");
    jmethodID method = env->GetStaticMethodID(cls, "showPrivacyOptionsForm", "(Landroid/app/Activity;)V");

    jobject activity = dmGraphics::GetNativeAndroidActivity();
    env->CallStaticVoidMethod(cls, method, activity);

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



// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"request_consent_info_update", RequestConsentInfoUpdate},
    {"show_privacy_options_form", ShowPrivacyOptionsForm},
    {"is_privacy_options_required", IsPrivacyOptionsRequired},
    {"can_request_ads", CanRequestAds},
    {"initialize_mobile_ads_sdk", InitializeMobileAdsSdk},
    {"get_consent_status", GetConsentStatus},  // <-- New function
    {"reset_consent_information", ResetConsentInformation},
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result AppInitializeExtension(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeExtension(dmExtension::Params* params)
{
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

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, AppInitializeExtension, AppFinalizeExtension, InitializeExtension, 0, 0, FinalizeExtension)
