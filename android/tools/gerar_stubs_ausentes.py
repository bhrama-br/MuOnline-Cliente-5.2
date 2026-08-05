# Gera o arquivo de stubs para os simbolos que os subsistemas ainda nao portados
# deixam indefinidos no build Android.
#
# Emite C++ com rotulos asm() em vez de assembly: `void f() asm("<mangled>")`
# define um simbolo com nome ja manglado sem depender de sintaxe de assembler
# por arquitetura. A tentativa anterior com `.set nome, alvo` falhou em silencio
# porque o alvo era externo ao arquivo e o assembler descartou os aliases.
import subprocess, sys

# Uso: gerar_stubs_ausentes.py und.txt llvm-cxxfilt saida.cpp [dir_libs_sistema] [llvm-readelf]
und_path, cxxfilt, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
system_lib_dir = sys.argv[4] if len(sys.argv) > 4 else ""
readelf        = sys.argv[5] if len(sys.argv) > 5 else "llvm-readelf"

# Simbolos que as bibliotecas do sistema Android exportam.
#
# Eles aparecem como indefinidos no .so porque sao resolvidos no carregamento, a
# partir de libandroid/libEGL/libGLESv3/libOpenSLES/liblog. Gerar stub para eles
# e destrutivo: viram um array zerado de 64 KB e a chamada salta para dentro do
# .bss — foi o que aconteceu com ANativeWindow_fromSurface (SIGSEGV no primeiro
# surfaceCreated) e depois com slCreateEngine (SIGSEGV ao iniciar o audio).
#
# A primeira versao usava uma lista de PREFIXOS, que errou nas duas vezes: e um
# palpite sobre como as APIs se chamam. Agora as proprias bibliotecas do NDK sao
# consultadas, entao o conjunto e exato — e cresce sozinho se mais bibliotecas
# forem linkadas.
def collect_system_symbols(sysroot_lib_dir, readelf):
    import glob, os
    exported = set()
    if not sysroot_lib_dir or not os.path.isdir(sysroot_lib_dir):
        print("AVISO: diretorio de bibliotecas do sistema nao encontrado:", sysroot_lib_dir)
        return exported
    for library in glob.glob(os.path.join(sysroot_lib_dir, "*.so")):
        try:
            output = subprocess.run([readelf, "--dyn-syms", library],
                                    capture_output=True, text=True).stdout
        except OSError:
            continue
        for line in output.split("\n"):
            parts = line.split()
            # Formato do llvm-readelf: Num Value Size Type Bind Vis Ndx Name
            if len(parts) >= 8 and parts[6] != "UND":
                exported.add(parts[7].split("@")[0])
    return exported


SYSTEM_SYMBOLS = collect_system_symbols(system_lib_dir, readelf)
print("simbolos do sistema conhecidos:", len(SYSTEM_SYMBOLS))

candidates = []
for line in open(und_path, encoding="utf-8"):
    parts = line.split()
    if len(parts) != 2:
        continue
    kind, name = parts
    if "@" in name or name.startswith("__") or name in SYSTEM_SYMBOLS:
        continue
    candidates.append(name)

candidates = sorted(set(candidates))
demangled_all = subprocess.run([cxxfilt], input="\n".join(candidates),
                               capture_output=True, text=True).stdout.split("\n")

# Filtra pelo nome DESMANGLADO, nao pelo prefixo manglado. Filtrar "_ZT*" inteiro
# descartava tambem as vtables de classes do jogo (_ZTVN8SEASON3B25CFenrir...),
# e o dlopen falhava justamente numa delas.
def is_runtime(dem):
    # operator new/delete vem da libc++ e desmangla sem prefixo "std::"
    # (_Znwm -> "operator new(unsigned long)"). Stubar isso fazia o app abortar
    # na PRIMEIRA alocacao, logo depois de o dlopen finalmente passar.
    if dem.startswith("operator new") or dem.startswith("operator delete"):
        return True
    for prefix in ("std::", "__cxxabiv1::", "vtable for std::", "typeinfo for std::",
                   "vtable for __cxxabiv1::", "typeinfo for __cxxabiv1::",
                   "typeinfo for unsigned", "typeinfo for int", "typeinfo for char",
                   "typeinfo for float", "typeinfo for double", "typeinfo for bool",
                   "typeinfo for long", "typeinfo for short", "typeinfo for void"):
        if dem.startswith(prefix):
            return True
    return False

funcs, data = [], []
for name, dem in zip(candidates, demangled_all):
    if is_runtime(dem):
        continue
    (funcs if "(" in dem else data).append((name, dem))

with open(out_path, "w", encoding="utf-8") as f:
    f.write('''// GERADO AUTOMATICAMENTE - nao editar a mao. Ver genstubs2.py.
//
// Os subsistemas que ainda nao foram portados (rede, audio, protecao, globais de
// Winmain) deixam simbolos indefinidos. No Web isso e tolerado por
// -sERROR_ON_UNDEFINED_SYMBOLS=0, que cria um stub por funcao e so falha quando a
// chamada acontece. O linker dinamico do Android nao tem equivalente: com
// BIND_NOW ele resolve tudo no dlopen e a biblioteca sequer carrega, como
// aconteceu com "cannot locate symbol g_hFixFont".
//
// Este arquivo reproduz a semantica do Web: cada funcao ausente vira um stub que
// registra o proprio nome no logcat e aborta; cada dado ausente ganha
// armazenamento zerado. A lista encolhe conforme os subsistemas forem portados.
//
// Para regerar depois de mudar o conjunto de modulos:
//   1. tire este arquivo do CMakeLists e linke com -Wl,--unresolved-symbols=ignore-all
//   2. llvm-readelf --dyn-syms libmu_legacy_platform.so \\\\
//        | awk '$7=="UND" && $8 !~ /@/ {print $4" "$8}' | sort -u > und.txt
//   3. python genstubs2.py und.txt llvm-cxxfilt AndroidMissingSymbols.cpp

#include <android/log.h>
#include <stdlib.h>

namespace
{
    // Registra uma vez por simbolo, em nivel ERROR, e devolve nulo.
    //
    // A primeira versao abortava, por preferir falha ruidosa a resultado errado
    // em silencio. Nao da: construtores estaticos dos modulos ligados rodam no
    // dlopen, e um deles chama RegisterLuaReg — o app morria antes de desenhar
    // qualquer coisa. O log por simbolo mantem o aviso visivel sem impedir que a
    // cena suba; `adb logcat -s MuLegacy` lista tudo que foi alcancado.
    //
    // O retorno e void*, nao void: muitas destas funcoes devolvem ponteiro ou
    // inteiro, e zerar o registrador de retorno e bem mais seguro do que deixar
    // lixo do quadro anterior virar um ponteiro.
    void* Unimplemented(const char* symbol, bool* reported)
    {
        if (!*reported)
        {
            *reported = true;
            __android_log_print(ANDROID_LOG_ERROR, "MuLegacy",
                                "subsistema nao portado alcancado: %s", symbol);
        }
        return 0;
    }
}

''')
    f.write("// funcoes ausentes: %d\n" % len(funcs))
    for index, (name, dem) in enumerate(funcs):
        safe = dem.replace("*/", "* /")
        f.write('extern "C" void* mu_missing_fn_%d() asm("%s");\n' % (index, name))
        f.write('extern "C" void* mu_missing_fn_%d() '
                '{ static bool reported = false; return Unimplemented("%s", &reported); }  // %s\n'
                % (index, name, safe))
    f.write("\n// dados ausentes: %d\n" % len(data))
    f.write("// 64 KB por simbolo. O tamanho real e desconhecido aqui: o simbolo e\n"
            "// so um nome, sem informacao de tipo. Com 256 bytes o app quebrava com\n"
            "// SIGSEGV numa thread do JIT — escrita passando do fim de um destes\n"
            "// blocos corrompia memoria vizinha. A folga custa BSS (zerada, sem\n"
            "// impacto no tamanho do APK) e evita esse modo de falha.\n")
    # O "= {0}" e obrigatorio: em C++, uma declaracao dentro de linkage-specification
    # ("extern \"C\" char x[N];") conta como se tivesse `extern`, ou seja, e
    # DECLARACAO e nao definicao — nada era emitido e os simbolos continuavam
    # indefinidos. Array todo-zero vai para .bss, entao nao pesa no APK.
    for index, (name, dem) in enumerate(data):
        f.write('extern "C" char mu_missing_data_%d[65536] asm("%s");\n' % (index, name))
        f.write('extern "C" char mu_missing_data_%d[65536] = {0};  // %s\n'
                % (index, dem.replace("*/", "* /")))

print("funcoes: %d  dados: %d  -> %s" % (len(funcs), len(data), out_path))
