#include "PlatformShell.h"

#include <stddef.h>

#if defined(_WIN32)
#  include <windows.h>
#  include <shellapi.h>
#elif defined(__EMSCRIPTEN__)
#  include <emscripten.h>
#elif defined(__ANDROID__)
#  include <jni.h>
#endif

#if defined(__ANDROID__)
namespace
{
    // Preenchidos por Platform::SetAndroidShellContext, chamado do JNI_OnLoad /
    // da Activity. Sem eles nao ha como montar um Intent.
    JavaVM* g_javaVm = 0;
    jobject g_activity = 0;
}

namespace Platform
{
    void SetAndroidShellContext(JavaVM* vm, jobject activityGlobalRef)
    {
        g_javaVm = vm;
        g_activity = activityGlobalRef;
    }
}
#endif

namespace Platform
{
    bool OpenExternalUrl(const char* target, const char* parameters)
    {
        if (target == 0 || target[0] == '\0')
            return false;

#if defined(_WIN32)
        // Comportamento identico ao legado: o valor de retorno de ShellExecute e um
        // codigo de erro quando <= 32, e so acima disso significa sucesso.
        HINSTANCE result = ShellExecuteA(NULL, "open", target, parameters, "", SW_SHOW);
        return 32 < (UINT_PTR)result;

#elif defined(__EMSCRIPTEN__)
        (void)parameters; // nao ha equivalente a argumentos de processo no navegador.
        // window.open devolve null quando o bloqueador de pop-up barra a chamada —
        // o que acontece se ela nao partir de um gesto do usuario. Propagamos isso
        // em vez de mentir que abriu.
        //
        // Sem 'noopener' na string de features de proposito: com ela o retorno e
        // SEMPRE null por especificacao, mesmo quando a aba abre, e nao restaria
        // como distinguir sucesso de bloqueio. Anular handle.opener depois tem o
        // mesmo efeito de seguranca (a pagina aberta nao alcanca a nossa) e ainda
        // deixa o resultado observavel.
        int opened = MAIN_THREAD_EM_ASM_INT({
            try {
                var handle = window.open(UTF8ToString($0), '_blank');
                if (!handle) return 0;
                try { handle.opener = null; } catch (e) {}
                return 1;
            } catch (e) {
                return 0;
            }
        }, target);
        return opened != 0;

#elif defined(__ANDROID__)
        (void)parameters; // Intent.ACTION_VIEW nao recebe argumentos de processo.
        if (g_javaVm == 0 || g_activity == 0)
            return false;

        JNIEnv* environment = 0;
        bool attached = false;
        if (g_javaVm->GetEnv((void**)&environment, JNI_VERSION_1_6) != JNI_OK)
        {
            if (g_javaVm->AttachCurrentThread(&environment, 0) != JNI_OK)
                return false;
            attached = true;
        }

        bool launched = false;
        jclass uriClass = environment->FindClass("android/net/Uri");
        jclass intentClass = environment->FindClass("android/content/Intent");
        jclass activityClass = environment->GetObjectClass(g_activity);
        if (uriClass != 0 && intentClass != 0 && activityClass != 0)
        {
            jmethodID parse = environment->GetStaticMethodID(uriClass, "parse",
                "(Ljava/lang/String;)Landroid/net/Uri;");
            jmethodID construct = environment->GetMethodID(intentClass, "<init>",
                "(Ljava/lang/String;Landroid/net/Uri;)V");
            jmethodID addFlags = environment->GetMethodID(intentClass, "addFlags",
                "(I)Landroid/content/Intent;");
            jmethodID startActivity = environment->GetMethodID(activityClass, "startActivity",
                "(Landroid/content/Intent;)V");
            if (parse != 0 && construct != 0 && addFlags != 0 && startActivity != 0)
            {
                jstring targetString = environment->NewStringUTF(target);
                jstring actionString = environment->NewStringUTF("android.intent.action.VIEW");
                jobject uri = environment->CallStaticObjectMethod(uriClass, parse, targetString);
                jobject intent = environment->NewObject(intentClass, construct, actionString, uri);
                if (intent != 0)
                {
                    // FLAG_ACTIVITY_NEW_TASK: obrigatorio quando o Intent parte de
                    // um contexto que nao e uma Activity em primeiro plano.
                    environment->CallObjectMethod(intent, addFlags, 0x10000000);
                    environment->CallVoidMethod(g_activity, startActivity, intent);
                    // Sem manipulador para o esquema da URL, startActivity lanca
                    // ActivityNotFoundException. Limpar a excecao evita derrubar a
                    // proxima chamada JNI com um erro que nao e dela.
                    if (environment->ExceptionCheck())
                        environment->ExceptionClear();
                    else
                        launched = true;
                    environment->DeleteLocalRef(intent);
                }
                if (uri != 0) environment->DeleteLocalRef(uri);
                environment->DeleteLocalRef(actionString);
                environment->DeleteLocalRef(targetString);
            }
        }
        if (environment->ExceptionCheck())
            environment->ExceptionClear();
        if (uriClass != 0) environment->DeleteLocalRef(uriClass);
        if (intentClass != 0) environment->DeleteLocalRef(intentClass);
        if (activityClass != 0) environment->DeleteLocalRef(activityClass);
        if (attached)
            g_javaVm->DetachCurrentThread();
        return launched;

#else
        (void)parameters;
        return false;
#endif
    }
}
