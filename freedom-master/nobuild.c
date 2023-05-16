#define NOBUILD_IMPLEMENTATION
#include "nobuild.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "imgui/stb_sprintf.h"

#define DLL_DIRS "freedom/", "imgui/", "imgui/backends/"

#define MSVC_COMMON_FLAGS "/nologo", "/DWIN32_LEAN_AND_MEAN", "/DUNICODE", "/DIMGUI_USE_STB_SPRINTF", "/DIMGUI_DEFINE_MATH_OPERATORS", "/std:c++17", "/EHsc", "/Iinclude", "/Iimgui", "/Iimgui/backends"
#define MSVC_RELEASE_FLAGS MSVC_COMMON_FLAGS, "/DNDEBUG", "/O2", "/MT"
#define MSVC_DEBUG_FLAGS MSVC_COMMON_FLAGS, "/Od", "/Z7", "/MDd", "/FS"

#define MSVC_LINK_RELEASE_FLAGS "/MACHINE:x86"
#define MSVC_LINK_DEBUG_FLAGS "/DEBUG", "/MACHINE:x86"

#define PROCESSES_CAPACITY 256

#define CALL_LINK(object_file_extenstion, ...)                                    \
    do                                                                            \
    {                                                                             \
        Cstr_Array line = cstr_array_make(__VA_ARGS__, NULL);                     \
        FOREACH_FILE_IN_DIR(file, ".",                                            \
                            {                                                     \
                                if (ENDS_WITH(file, object_file_extenstion))      \
                                    line = cstr_array_append(line, strdup(file)); \
                            });                                                   \
        Cmd cmd = {.line = line};                                                 \
        INFO("CMD: %s", cmd_show(cmd));                                           \
        cmd_run_sync(cmd);                                                        \
    } while (0)

int debug_flag = 0;
int rebuild_flag = 0;
char define_commit_hash[32] = {0};

void async_obj_foreach_file_in_dir(Pid *proc, size_t *proc_count, Cstr directory)
{
    FOREACH_FILE_IN_DIR(file, directory,
                        {
                            if (ENDS_WITH(file, ".cpp"))
                            {
                                Cstr src = CONCAT(directory, strdup(file));
                                Cstr obj = CONCAT(NOEXT(file), ".obj");
                                if (!PATH_EXISTS(obj) ||
                                    (rebuild_flag || is_path1_modified_after_path2(src, obj)))
                                {
                                    if (define_commit_hash[0] != '\0')
                                    {
                                        Cstr_Array line = debug_flag ? cstr_array_make("cl", define_commit_hash, MSVC_DEBUG_FLAGS, src, "/c", NULL) : cstr_array_make("cl", define_commit_hash, MSVC_RELEASE_FLAGS, src, "/c", NULL);
                                        Cmd cmd = {.line = line};
                                        INFO("CMD: %s", cmd_show(cmd));
                                        proc[(*proc_count)++] = cmd_run_async(cmd, NULL, NULL);
                                    }
                                    else
                                    {
                                        Cstr_Array line = debug_flag ? cstr_array_make("cl", MSVC_DEBUG_FLAGS, src, "/c", NULL) : cstr_array_make("cl", MSVC_RELEASE_FLAGS, src, "/c", NULL);
                                        Cmd cmd = {.line = line};
                                        INFO("CMD: %s", cmd_show(cmd));
                                        proc[(*proc_count)++] = cmd_run_async(cmd, NULL, NULL);
                                    }
                                }
                            }
                        });
}

void async_obj_foreach_file_in_dirs(Cstr first, ...)
{
    Pid proc[PROCESSES_CAPACITY];
    size_t proc_count = 0;
    async_obj_foreach_file_in_dir(proc, &proc_count, first);
    va_list args;
    va_start(args, first);
    for (Cstr directory = va_arg(args, Cstr);
         directory != NULL;
         directory = va_arg(args, Cstr))
    {
        async_obj_foreach_file_in_dir(proc, &proc_count, directory);
    }
    va_end(args);
    for (size_t i = 0; i < proc_count; ++i)
        pid_wait(proc[i]);
}

void build()
{
    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0);
    SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    TCHAR szCmdline[] = TEXT("git rev-parse --short HEAD");
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.hStdInput = NULL;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    CreateProcess(NULL, szCmdline, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);

    WaitForSingleObject(piProcInfo.hProcess, INFINITE);

    DWORD exit_status;
    GetExitCodeProcess(piProcInfo.hProcess, &exit_status);

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
    CloseHandle(hChildStd_OUT_Wr);

    if (exit_status == 0)
    {
        DWORD dwRead;
        char git_commit_hash[16] = {0};
        ReadFile(hChildStd_OUT_Rd, git_commit_hash, 7, &dwRead, NULL);
        if (dwRead > 0)
            stbsp_snprintf(define_commit_hash, sizeof(define_commit_hash), "/DGIT_COMMIT_HASH=%s", git_commit_hash);
    }

    async_obj_foreach_file_in_dirs(DLL_DIRS, NULL);
    if (debug_flag)
        CALL_LINK(".obj", "LINK", "/DLL", "/OUT:freedom.dll", MSVC_LINK_DEBUG_FLAGS);
    else
        CALL_LINK(".obj", "LINK", "/DLL", "/OUT:freedom.dll", MSVC_LINK_RELEASE_FLAGS);
    CMD("cl", "/DWIN32_LEAN_AND_MEAN", "/DNDEBUG", "/DUNICODE", "/std:c++17", "/O2", "/MT", "/EHsc", "/nologo", "/Fe:freedom.exe", "injector.cpp", "/link", "/MACHINE:x86");
    CMD("csc", "/nologo", "/optimize", "/target:library", "/out:prejit.dll", "freedom/prejit.cs");
}

void process_args(int argc, char **argv)
{
    for (int i = 0; i < argc; ++i)
    {
        if (!debug_flag && strcmp(argv[i], "debug") == 0)
            debug_flag = 1;
        if (!rebuild_flag && strcmp(argv[i], "rebuild") == 0)
            rebuild_flag = 1;
    }
    if (rebuild_flag)
    {
        FOREACH_FILE_IN_DIR(file, ".", {
            if (ENDS_WITH(file, ".obj"))
                RM(file);
        });
    }
    build();
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);

#ifdef _WIN32
    if (PATH_EXISTS(CONCAT(NOEXT(argv[0]), ".obj")))
        RM(CONCAT(NOEXT(argv[0]), ".obj"));
#endif

    if (argc > 1)
        process_args(argc, argv);
    else
        build();
    return 0;
}
