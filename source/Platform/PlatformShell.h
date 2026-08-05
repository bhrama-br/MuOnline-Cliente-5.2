#ifndef PLATFORM_SHELL_H
#define PLATFORM_SHELL_H

#if defined(__ANDROID__)
#  include <jni.h>
#endif

// Abertura de recurso externo (URL, arquivo, executavel) fora do processo.
//
// O legado chamava ShellExecute direto em tres pontos (iexplorer.h,
// LuaInterface.cpp, ZzzOpenglUtil.cpp). ShellExecute nao existe fora do Windows
// e cada plataforma tem um mecanismo proprio, entao a chamada passa por aqui.
namespace Platform
{
    // Abre `target` no manipulador padrao da plataforma.
    //
    //   Windows: ShellExecute(NULL, "open", target, parameters, ...)
    //   Web:     window.open(target, "_blank") — o navegador pode bloquear se a
    //            chamada nao vier de um gesto do usuario; nesse caso devolve false.
    //   Android: Intent.ACTION_VIEW via JNI.
    //
    // `parameters` so tem significado no Windows (argumentos de linha de comando
    // quando `target` e um executavel). Nas demais plataformas e ignorado, pois
    // nao ha equivalente: passar NULL e o uso normal.
    //
    // Devolve true quando a plataforma aceitou a solicitacao. Nao significa que o
    // recurso abriu de fato — nenhuma das plataformas informa isso de volta.
    bool OpenExternalUrl(const char* target, const char* parameters = 0);

#if defined(__ANDROID__)
    // Guarda a VM e a Activity usadas para montar o Intent. `activityGlobalRef`
    // precisa ser uma referencia global (NewGlobalRef): a chamada acontece muito
    // depois do JNI que a forneceu, quando uma referencia local ja teria morrido.
    void SetAndroidShellContext(JavaVM* vm, jobject activityGlobalRef);
#endif
}

#endif // PLATFORM_SHELL_H
