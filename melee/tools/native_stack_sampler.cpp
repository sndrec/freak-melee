#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

static void json_text(FILE* file, const wchar_t* text)
{
    char utf8[32768];
    WideCharToMultiByte(CP_UTF8,0,text,-1,utf8,sizeof(utf8),NULL,NULL);
    fputc('"',file);
    for (const char* p=utf8; *p; ++p) {
        if (*p=='"' || *p=='\\') fputc('\\',file);
        if ((unsigned char)*p<32) fprintf(file,"\\u%04x",(unsigned char)*p);
        else fputc(*p,file);
    }
    fputc('"',file);
}

int wmain(int argc, wchar_t** argv)
{
    if (argc!=4 && argc!=5) { fprintf(stderr,"stack-sample PID EXE OUTPUT [THREAD_NAME]\n"); return 1; }
    const wchar_t* requested_name=argc==5?argv[4]:NULL;
    if(requested_name && !*requested_name) { fprintf(stderr,"Thread name must not be empty\n"); return 1; }
    DWORD pid=(DWORD)wcstoul(argv[1],NULL,10);
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ|SYNCHRONIZE,FALSE,pid);
    if (!process) { fprintf(stderr,"OpenProcess error=%lu\n",GetLastError()); return 1; }
    wchar_t actual[32768]; DWORD length=_countof(actual);
    if (!QueryFullProcessImageNameW(process,0,actual,&length) || _wcsicmp(actual,argv[2])) {
        fprintf(stderr,"Target executable identity differs\n"); CloseHandle(process); return 1;
    }
    Sleep(2000);
    DWORD tid=0; ULONGLONG earliest=~0ULL; unsigned named_matches=0;
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    THREADENTRY32 entry={sizeof(entry)};
    if (snapshot!=INVALID_HANDLE_VALUE && Thread32First(snapshot,&entry)) do {
        if (entry.th32OwnerProcessID!=pid) continue;
        HANDLE t=OpenThread(THREAD_QUERY_INFORMATION,FALSE,entry.th32ThreadID);
        FILETIME create,exit,kernel,user;
        if (t && requested_name) {
            PWSTR description=NULL;
            if(SUCCEEDED(GetThreadDescription(t,&description)) && description &&
               !_wcsicmp(description,requested_name)) {
                tid=entry.th32ThreadID; ++named_matches;
            }
            if(description)LocalFree(description);
        } else if (t && GetThreadTimes(t,&create,&exit,&kernel,&user)) {
            ULONGLONG when=((ULONGLONG)create.dwHighDateTime<<32)|create.dwLowDateTime;
            if (when<earliest) { earliest=when; tid=entry.th32ThreadID; }
        }
        if (t) CloseHandle(t);
    } while (Thread32Next(snapshot,&entry));
    if (snapshot!=INVALID_HANDLE_VALUE) CloseHandle(snapshot);
    if(requested_name && named_matches!=1) {
        fprintf(stderr,"Expected one named thread; found %u\n",named_matches);CloseHandle(process);return 1;
    }
    HANDLE thread=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION|SYNCHRONIZE,FALSE,tid);
    if (!thread) { fprintf(stderr,"OpenThread error=%lu\n",GetLastError()); CloseHandle(process); return 1; }
    if(GetProcessIdOfThread(thread)!=pid) {
        fprintf(stderr,"Thread owner differs\n");CloseHandle(thread);CloseHandle(process);return 1;
    }
    FILE* file=NULL;
    if (_wfopen_s(&file,argv[3],L"wb") || !file) { CloseHandle(thread); CloseHandle(process); return 1; }
    // Each sampler is a single-threaded process: DbgHelp has no concurrent callers.
    // Only local symbol paths are used; no symbol-server downloads while paused.
    wchar_t directory[32768]; wcscpy_s(directory,actual);
    wchar_t* slash=wcsrchr(directory,L'\\'); if (slash) *slash=0;
    SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_NO_PROMPTS|SYMOPT_FAIL_CRITICAL_ERRORS);
    if (!SymInitializeW(process,directory,FALSE)) {
        fprintf(stderr,"SymInitialize error=%lu\n",GetLastError()); fclose(file); CloseHandle(thread); CloseHandle(process); return 1;
    }
    LARGE_INTEGER frequency,begin; QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&begin);
    fprintf(file,"{\"pid\":%lu,\"tid\":%lu,\"frequency\":%lld,\"exe\":",pid,tid,frequency.QuadPart);
    json_text(file,actual);
    fprintf(file,",\"requested_thread_name\":");json_text(file,requested_name?requested_name:L"");
    PWSTR description=NULL;
    fprintf(file,",\"thread_name\":");
    if(SUCCEEDED(GetThreadDescription(thread,&description)) && description)json_text(file,description);
    else json_text(file,L"");
    if(description)LocalFree(description);
    fprintf(file,",\"modules\":[");
    snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
    MODULEENTRY32W module={sizeof(module)}; unsigned modules=0;
    if (snapshot!=INVALID_HANDLE_VALUE && Module32FirstW(snapshot,&module)) do {
        DWORD64 base=SymLoadModuleExW(process,NULL,module.szExePath,NULL,(DWORD64)module.modBaseAddr,module.modBaseSize,NULL,0);
        fprintf(file,"%s{\"base\":%llu,\"bytes\":%lu,\"loaded\":%llu,\"path\":",modules++?",":"",(DWORD64)module.modBaseAddr,module.modBaseSize,base);
        json_text(file,module.szExePath); fputc('}',file);
    } while (Module32NextW(snapshot,&module));
    if (snapshot!=INVALID_HANDLE_VALUE) CloseHandle(snapshot);
    fprintf(file,"]}\n");
    MMRESULT timer_resolution=timeBeginPeriod(1);
    DWORD failure=0,thread_exit=STILL_ACTIVE; unsigned samples=0;
    LARGE_INTEGER deadline; QueryPerformanceCounter(&deadline); deadline.QuadPart+=40*frequency.QuadPart;
    while (WaitForSingleObject(process,0)==WAIT_TIMEOUT) {
        LARGE_INTEGER started; QueryPerformanceCounter(&started);
        if (started.QuadPart>=deadline.QuadPart) break;
        DWORD previous=SuspendThread(thread);
        if (previous==DWORD(-1)) { failure=GetLastError(); GetExitCodeThread(thread,&thread_exit); break; }
        CONTEXT context={}; context.ContextFlags=CONTEXT_CONTROL|CONTEXT_INTEGER;
        DWORD64 addresses[64]={}; unsigned count=0; DWORD context_error=0;
        if (GetThreadContext(thread,&context)) {
            addresses[count++]=context.Rip;
            STACKFRAME64 frame={};
            frame.AddrPC.Offset=context.Rip; frame.AddrPC.Mode=AddrModeFlat;
            frame.AddrFrame.Offset=context.Rbp; frame.AddrFrame.Mode=AddrModeFlat;
            frame.AddrStack.Offset=context.Rsp; frame.AddrStack.Mode=AddrModeFlat;
            bool first_walk=true;
            while (count<_countof(addresses)) {
                DWORD64 old_pc=frame.AddrPC.Offset,old_sp=frame.AddrStack.Offset;
                if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64,process,thread,&frame,&context,NULL,
                    SymFunctionTableAccess64,SymGetModuleBase64,NULL) || !frame.AddrPC.Offset) break;
                if (first_walk && frame.AddrPC.Offset==addresses[0]) {
                    first_walk=false; continue;
                }
                first_walk=false;
                if (old_pc==frame.AddrPC.Offset && old_sp==frame.AddrStack.Offset) break;
                addresses[count++]=frame.AddrPC.Offset;
            }
        } else context_error=GetLastError();
        // Never leave the target suspended, including partial/failed stack walks.
        DWORD resumed=ResumeThread(thread);
        LARGE_INTEGER ended; QueryPerformanceCounter(&ended);
        if (resumed==DWORD(-1)) { failure=GetLastError(); fprintf(stderr,"ResumeThread failed %lu\n",failure); break; }
        fprintf(file,"{\"qpc\":%lld,\"paused_qpc\":%lld,\"previous\":%lu,\"context_error\":%lu,\"stack\":[",started.QuadPart,ended.QuadPart-started.QuadPart,previous,context_error);
        for (unsigned i=0;i<count;++i) fprintf(file,"%s%llu",i?",":"",addresses[i]);
        fprintf(file,"]}\n"); ++samples;
        Sleep(2);
    }
    fprintf(file,"{\"summary\":true,\"samples\":%u,\"failure\":%lu,\"thread_exit\":%lu}\n",samples,failure,thread_exit);
    if (timer_resolution==TIMERR_NOERROR) timeEndPeriod(1);
    SymCleanup(process); fclose(file); CloseHandle(thread); CloseHandle(process);
    printf("{\"pid\":%lu,\"tid\":%lu,\"samples\":%u,\"failure\":%lu,\"thread_exit\":%lu}\n",pid,tid,samples,failure,thread_exit);
    return 0;
}
