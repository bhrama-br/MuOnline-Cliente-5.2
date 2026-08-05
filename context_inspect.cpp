#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <vector>

#pragma comment(lib, "dbghelp.lib")

static DWORD g_stackBase = 0;
static BYTE* g_stackData = NULL;
static DWORD g_stackSize = 0;
static BOOL CALLBACK ReadCapturedStack(HANDLE, DWORD64 address, PVOID buffer, DWORD size, LPDWORD bytesRead)
{
    if (address >= g_stackBase && address + size <= g_stackBase + g_stackSize)
    {
        memcpy(buffer, g_stackData + (address - g_stackBase), size);
        *bytesRead = size;
        return TRUE;
    }
    *bytesRead = 0;
    return FALSE;
}

int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    FILE* file = fopen(argv[1], "rb");
    EXCEPTION_RECORD record = {};
    CONTEXT context = {};
    DWORD savedEsp = 0;
    std::vector<BYTE> stack(8192);
    if (!file || fread(&record, sizeof(record), 1, file) != 1 || fread(&context, sizeof(context), 1, file) != 1 || fread(&savedEsp, sizeof(savedEsp), 1, file) != 1 || fread(stack.data(), stack.size(), 1, file) != 1) return 3;
    fclose(file);

    g_stackBase = savedEsp;
    g_stackData = stack.data();
    g_stackSize = (DWORD)stack.size();

    printf("exception=0x%08lX fault=0x%08lX eip=0x%08lX esp=0x%08lX ebp=0x%08lX savedEsp=0x%08lX\n", record.ExceptionCode, (DWORD)(ULONG_PTR)record.ExceptionAddress, context.Eip, context.Esp, context.Ebp, savedEsp);
    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    SymInitialize(process, argv[2], FALSE);
    char imagePath[MAX_PATH] = {}, pdbPath[MAX_PATH] = {};
    snprintf(imagePath, sizeof(imagePath), "%s\\Main.exe", argv[2]);
    snprintf(pdbPath, sizeof(pdbPath), "%s\\Main.pdb", argv[2]);
    SymLoadModuleEx(process, NULL, imagePath, pdbPath, 0x00400000, 0, NULL, 0);
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(1, sizeof(SYMBOL_INFO) + 1024);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = 1023;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = context.Eip; frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp; frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp; frame.AddrFrame.Mode = AddrModeFlat;
    printf("stackwalk:\n");
    for (int level = 0; level < 12 && StackWalk64(IMAGE_FILE_MACHINE_I386, process, NULL, &frame, &context, ReadCapturedStack, SymFunctionTableAccess64, SymGetModuleBase64, NULL); ++level)
    {
        DWORD64 displacement = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
            printf("  %d: %s+0x%llX (0x%08llX)\n", level, symbol->Name, displacement, frame.AddrPC.Offset);
        else
            printf("  %d: 0x%08llX\n", level, frame.AddrPC.Offset);
    }
    for (size_t i = 0; i + sizeof(DWORD) <= stack.size(); i += sizeof(DWORD))
    {
        DWORD address = *(DWORD*)(stack.data() + i);
        if (address < 0x00400000 || address >= 0x09400000) continue;
        DWORD64 displacement = 0;
        if (SymFromAddr(process, address, &displacement, symbol))
            printf("stack+0x%03zX: %s+0x%llX (0x%08lX)\n", i, symbol->Name, displacement, address);
    }
    printf("ebp-chain:\n");
    DWORD ebp = context.Ebp;
    for (int level = 0; level < 16 && ebp >= savedEsp && ebp + 8 <= savedEsp + stack.size(); ++level)
    {
        size_t offset = ebp - savedEsp;
        DWORD nextEbp = *(DWORD*)(stack.data() + offset);
        DWORD returnAddress = *(DWORD*)(stack.data() + offset + 4);
        DWORD64 displacement = 0;
        if (SymFromAddr(process, returnAddress, &displacement, symbol))
            printf("  %d: %s+0x%llX (0x%08lX)\n", level, symbol->Name, displacement, returnAddress);
        else
            printf("  %d: 0x%08lX nextEbp=0x%08lX\n", level, returnAddress, nextEbp);
        if (nextEbp <= ebp) break;
        ebp = nextEbp;
    }
    free(symbol);
    SymCleanup(process);
    return 0;
}
