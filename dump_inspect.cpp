#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <vector>

#pragma comment(lib, "dbghelp.lib")

struct DumpRange { DWORD64 address; DWORD size; const BYTE* data; };
static std::vector<DumpRange> g_ranges;

static BOOL CALLBACK ReadDumpMemory(HANDLE, DWORD64 address, PVOID buffer, DWORD size, LPDWORD bytesRead)
{
    for (const DumpRange& range : g_ranges)
    {
        if (address >= range.address && address + size <= range.address + range.size)
        {
            memcpy(buffer, range.data + (address - range.address), size);
            *bytesRead = size;
            return TRUE;
        }
    }
    *bytesRead = 0;
    return FALSE;
}

int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    HANDLE file = CreateFileA(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE mapping = file == INVALID_HANDLE_VALUE ? NULL : CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
    void* base = mapping ? MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0) : NULL;
    PMINIDUMP_EXCEPTION_STREAM exceptionStream = NULL;
    ULONG streamSize = 0;
    if (!base || !MiniDumpReadDumpStream(base, ExceptionStream, NULL, reinterpret_cast<PVOID*>(&exceptionStream), &streamSize)) return 3;

    DWORD64 exceptionAddress = (DWORD64)(ULONG_PTR)exceptionStream->ExceptionRecord.ExceptionAddress;
    printf("exception=0x%08lX address=0x%08llX thread=%lu\n", exceptionStream->ExceptionRecord.ExceptionCode, exceptionAddress, exceptionStream->ThreadId);

    PMINIDUMP_MODULE_LIST modules = NULL;
    if (MiniDumpReadDumpStream(base, ModuleListStream, NULL, reinterpret_cast<PVOID*>(&modules), &streamSize))
    {
        for (ULONG i = 0; i < modules->NumberOfModules; ++i)
        {
            const MINIDUMP_MODULE& module = modules->Modules[i];
            DWORD64 start = module.BaseOfImage;
            DWORD64 end = start + module.SizeOfImage;
            if (exceptionAddress >= start && exceptionAddress < end)
            {
                printf("moduleBase=0x%08llX moduleSize=0x%lX offset=0x%llX\n", start, module.SizeOfImage, exceptionAddress - start);
                break;
            }
        }
    }

    PMINIDUMP_MEMORY_LIST memories = NULL;
    if (MiniDumpReadDumpStream(base, MemoryListStream, NULL, reinterpret_cast<PVOID*>(&memories), &streamSize))
    {
        for (ULONG i = 0; i < memories->NumberOfMemoryRanges; ++i)
        {
            const MINIDUMP_MEMORY_DESCRIPTOR& memory = memories->MemoryRanges[i];
            g_ranges.push_back({ memory.StartOfMemoryRange, memory.Memory.DataSize, (const BYTE*)base + memory.Memory.Rva });
        }
    }

    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    if (SymInitialize(process, argv[2], FALSE))
    {
        char imagePath[MAX_PATH] = {};
        char pdbPath[MAX_PATH] = {};
        snprintf(imagePath, sizeof(imagePath), "%s\\Main.exe", argv[2]);
        snprintf(pdbPath, sizeof(pdbPath), "%s\\Main.pdb", argv[2]);
        SymSetSearchPath(process, argv[2]);
        DWORD64 moduleBase = SymLoadModuleEx(process, NULL, imagePath, pdbPath, 0x00400000, 0, NULL, 0);
        printf("symbolModule=0x%08llX error=%lu\n", moduleBase, GetLastError());
        SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(1, sizeof(SYMBOL_INFO) + 1024);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 1023;
        DWORD64 displacement = 0;
        if (moduleBase && SymFromAddr(process, exceptionAddress, &displacement, symbol))
            printf("symbol=%s+0x%llX\n", symbol->Name, displacement);
        else
            printf("symbol lookup failed error=%lu\n", GetLastError());
        IMAGEHLP_LINE64 line = {}; line.SizeOfStruct = sizeof(line);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, exceptionAddress, &lineDisplacement, &line))
            printf("source=%s:%lu+%lu\n", line.FileName, line.LineNumber, lineDisplacement);

        PMINIDUMP_THREAD_LIST threads = NULL;
        if (MiniDumpReadDumpStream(base, ThreadListStream, NULL, reinterpret_cast<PVOID*>(&threads), &streamSize))
        {
            for (ULONG i = 0; i < threads->NumberOfThreads; ++i)
            {
                const MINIDUMP_THREAD& thread = threads->Threads[i];
                if (thread.ThreadId != exceptionStream->ThreadId) continue;
                g_ranges.push_back({ thread.Stack.StartOfMemoryRange, thread.Stack.Memory.DataSize, (const BYTE*)base + thread.Stack.Memory.Rva });
                CONTEXT context = {};
                memcpy(&context, (const BYTE*)base + thread.ThreadContext.Rva, min(sizeof(context), (size_t)thread.ThreadContext.DataSize));
                STACKFRAME64 frame = {};
                frame.AddrPC.Offset = context.Eip; frame.AddrPC.Mode = AddrModeFlat;
                frame.AddrStack.Offset = context.Esp; frame.AddrStack.Mode = AddrModeFlat;
                frame.AddrFrame.Offset = context.Ebp; frame.AddrFrame.Mode = AddrModeFlat;
                printf("stack:\n");
                for (int level = 0; level < 16 && StackWalk64(IMAGE_FILE_MACHINE_I386, process, NULL, &frame, &context, ReadDumpMemory, SymFunctionTableAccess64, SymGetModuleBase64, NULL); ++level)
                {
                    DWORD64 displacement2 = 0;
                    if (SymFromAddr(process, frame.AddrPC.Offset, &displacement2, symbol))
                        printf("  %d: %s+0x%llX (0x%08llX)\n", level, symbol->Name, displacement2, frame.AddrPC.Offset);
                    else
                        printf("  %d: 0x%08llX\n", level, frame.AddrPC.Offset);
                }
                printf("ebp-chain (eip=0x%08lX esp=0x%08lX ebp=0x%08lX):\n", context.Eip, context.Esp, context.Ebp);
                DWORD ebp = context.Ebp;
                for (int level = 0; level < 16; ++level)
                {
                    DWORD pair[2] = {};
                    DWORD read = 0;
                    if (!ReadDumpMemory(NULL, ebp, pair, sizeof(pair), &read) || pair[1] == 0) break;
                    DWORD64 displacement2 = 0;
                    if (SymFromAddr(process, pair[1], &displacement2, symbol))
                        printf("  %d: %s+0x%llX (0x%08lX)\n", level, symbol->Name, displacement2, pair[1]);
                    else
                        printf("  %d: 0x%08lX\n", level, pair[1]);
                    if (pair[0] <= ebp) break;
                    ebp = pair[0];
                }
                break;
            }
        }
        free(symbol);
        SymCleanup(process);
    }
    if (base) UnmapViewOfFile(base);
    if (mapping) CloseHandle(mapping);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    return 0;
}
