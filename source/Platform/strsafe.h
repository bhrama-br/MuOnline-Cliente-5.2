#pragma once

// Substituto de <strsafe.h> para os alvos nao-Windows.
//
// Seis arquivos do subsistema de loja fazem `#include <strsafe.h>`. Em vez de
// editar cada um, este arquivo satisfaz o include: as funcoes StringCch* estao
// em WindowsCompat.h.
//
// Ele NAO sombreia o strsafe.h de verdade no build do PC: o Main.vcxproj tem
// `source\` na lista de includes, mas nao `source\Platform\`, entao no Windows
// quem responde continua sendo o cabecalho do SDK. O guarda abaixo torna isso
// explicito em vez de depender so da configuracao do projeto.

#if defined(_WIN32)
#error "Platform/strsafe.h nao deve ser usado no build do Windows: use o do SDK."
#endif

#include "WindowsCompat.h"
