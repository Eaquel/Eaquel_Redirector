#pragma once

#define EAQUEL_REDIRECTOR_VERSION "1.0.0-2026"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "Eaquel_Redirector"

#include <android/log.h>
#include <errno.h>
#include <link.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <span>
#include <mutex>
#include <optional>
#include <string_view>
#include <atomic>
#include <functional>
#include <unordered_map>

#ifdef ER_LOG_DISABLED
#define LOGD(...) ((void)0)
#define LOGV(...) ((void)0)
#define LOGI(...) ((void)0)
#define LOGW(...) ((void)0)
#define LOGE(...) ((void)0)
#define LOGF(...) ((void)0)
#define PLOGE(fmt, ...) ((void)0)
#else
#ifndef NDEBUG
#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s:%d#%s: " fmt, __FILE_NAME__, __LINE__, __PRETTY_FUNCTION__ __VA_OPT__(,) __VA_ARGS__)
#define LOGV(fmt, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "%s:%d#%s: " fmt, __FILE_NAME__, __LINE__, __PRETTY_FUNCTION__ __VA_OPT__(,) __VA_ARGS__)
#else
#define LOGD(...) ((void)0)
#define LOGV(...) ((void)0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)
#define PLOGE(fmt, ...) LOGE(fmt " failed with %d: %s" __VA_OPT__(,) __VA_ARGS__, errno, strerror(errno))
#endif

#define CHECK_SYSCALL_ERROR(res)                            \
    if ((unsigned long)(res) >= (unsigned long)-4095) {     \
        errno = -(long)(res);                               \
        return MAP_FAILED;                                  \
    }

#define CHECK_SYSCALL_ERROR_INTEGER(res)                    \
    if ((unsigned long)(res) >= (unsigned long)-4095) {     \
        errno = -(long)(res);                               \
        return -1;                                          \
    }

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

static uintptr_t k_page_size = 0;

enum class ErStealthLevel : uint8_t {
    DIRECT_PATCH = 1,
    TRAMPOLINE   = 2,
    BACKUP_FULL  = 3
};

[[gnu::always_inline]] static inline void* sys_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    if (k_page_size == 0) k_page_size = static_cast<uintptr_t>(getpagesize());
    if (offset % static_cast<off_t>(k_page_size) != 0) { errno = EINVAL; return MAP_FAILED; }
    void* result;
#if defined(__arm__)
    register long r0 __asm__("r0") = (long)addr;
    register long r1 __asm__("r1") = (long)length;
    register long r2 __asm__("r2") = (long)prot;
    register long r3 __asm__("r3") = (long)flags;
    register long r4 __asm__("r4") = (long)fd;
    register long r5 __asm__("r5") = (long)(offset / k_page_size);
    long scno = __NR_mmap2;
    __asm__ volatile("mov r12, r7\n\tmov r7, %[sc]\n\tsvc #0\n\tmov r7, r12"
        : "+r"(r0) : [sc]"r"(scno), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5) : "r12", "cc", "memory");
    result = (void*)r0;
#elif defined(__aarch64__)
    register long x0 __asm__("x0") = (long)addr;
    register long x1 __asm__("x1") = (long)length;
    register long x2 __asm__("x2") = (long)prot;
    register long x3 __asm__("x3") = (long)flags;
    register long x4 __asm__("x4") = (long)fd;
    register long x5 __asm__("x5") = (long)offset;
    register long x8 __asm__("x8") = SYS_mmap;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8) : "cc", "memory");
    result = (void*)x0;
#elif defined(__i386__)
    __asm__ volatile("pushl %%ebp\n\tmovl %[ofs], %%ebp\n\tint $0x80\n\tpopl %%ebp"
        : "=a"(result) : "a"(SYS_mmap2), "b"(addr), "c"(length), "d"(prot),
        "S"(flags), "D"(fd), [ofs]"g"(offset / k_page_size) : "memory", "cc");
#elif defined(__x86_64__)
    register long rdi __asm__("rdi") = (long)addr;
    register long rsi __asm__("rsi") = (long)length;
    register long rdx __asm__("rdx") = (long)prot;
    register long r10 __asm__("r10") = (long)flags;
    register long  r8 __asm__("r8")  = (long)fd;
    register long  r9 __asm__("r9")  = (long)offset;
    register long rax __asm__("rax") = SYS_mmap;
    __asm__ volatile("syscall" : "+r"(rax) : "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "cc", "memory");
    result = (void*)rax;
#elif defined(__riscv)
    register long a0 __asm__("a0") = (long)addr;
    register long a1 __asm__("a1") = (long)length;
    register long a2 __asm__("a2") = (long)prot;
    register long a3 __asm__("a3") = (long)flags;
    register long a4 __asm__("a4") = (long)fd;
    register long a5 __asm__("a5") = (long)offset;
    register long a7 __asm__("a7") = SYS_mmap;
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a7) : "memory");
    result = (void*)a0;
#endif
    CHECK_SYSCALL_ERROR(result);
    return result;
}

[[gnu::always_inline]] static inline void* sys_mremap(void* old_address, size_t old_size, size_t new_size, int flags, void* new_address) {
    void* result;
#if defined(__arm__)
    register long r0 __asm__("r0") = (long)old_address;
    register long r1 __asm__("r1") = (long)old_size;
    register long r2 __asm__("r2") = (long)new_size;
    register long r3 __asm__("r3") = (long)flags;
    register long r4 __asm__("r4") = (long)new_address;
    long scno = SYS_mremap;
    __asm__ volatile("mov r12, r7\n\tmov r7, %[sc]\n\tsvc #0\n\tmov r7, r12"
        : "+r"(r0) : [sc]"r"(scno), "r"(r1), "r"(r2), "r"(r3), "r"(r4) : "r12", "cc", "memory");
    result = (void*)r0;
#elif defined(__aarch64__)
    register long x0 __asm__("x0") = (long)old_address;
    register long x1 __asm__("x1") = (long)old_size;
    register long x2 __asm__("x2") = (long)new_size;
    register long x3 __asm__("x3") = (long)flags;
    register long x4 __asm__("x4") = (long)new_address;
    register long x8 __asm__("x8") = SYS_mremap;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x8) : "cc", "memory");
    result = (void*)x0;
#elif defined(__i386__)
    __asm__ volatile("int $0x80" : "=a"(result)
        : "a"(SYS_mremap), "b"(old_address), "c"(old_size), "d"(new_size), "S"(flags), "D"(new_address) : "memory", "cc");
#elif defined(__x86_64__)
    register long rdi __asm__("rdi") = (long)old_address;
    register long rsi __asm__("rsi") = (long)old_size;
    register long rdx __asm__("rdx") = (long)new_size;
    register long r10 __asm__("r10") = (long)flags;
    register long  r8 __asm__("r8")  = (long)new_address;
    register long rax __asm__("rax") = SYS_mremap;
    __asm__ volatile("syscall" : "+r"(rax) : "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8) : "rcx", "r11", "cc", "memory");
    result = (void*)rax;
#elif defined(__riscv)
    register long a0 __asm__("a0") = (long)old_address;
    register long a1 __asm__("a1") = (long)old_size;
    register long a2 __asm__("a2") = (long)new_size;
    register long a3 __asm__("a3") = (long)flags;
    register long a4 __asm__("a4") = (long)new_address;
    register long a7 __asm__("a7") = SYS_mremap;
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a7) : "memory");
    result = (void*)a0;
#endif
    CHECK_SYSCALL_ERROR(result);
    return result;
}

[[gnu::always_inline]] static inline int sys_munmap(void* addr, size_t length) {
    long result;
#if defined(__arm__)
    register long r0 __asm__("r0") = (long)addr;
    register long r1 __asm__("r1") = (long)length;
    long scno = SYS_munmap;
    __asm__ volatile("mov r12, r7\n\tmov r7, %[sc]\n\tsvc #0\n\tmov r7, r12"
        : "+r"(r0) : [sc]"r"(scno), "r"(r1) : "r12", "cc", "memory");
    result = r0;
#elif defined(__aarch64__)
    register long x0 __asm__("x0") = (long)addr;
    register long x1 __asm__("x1") = (long)length;
    register long x8 __asm__("x8") = SYS_munmap;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "cc", "memory");
    result = x0;
#elif defined(__i386__)
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(SYS_munmap), "b"(addr), "c"(length) : "memory", "cc");
#elif defined(__x86_64__)
    register long rdi __asm__("rdi") = (long)addr;
    register long rsi __asm__("rsi") = (long)length;
    register long rax __asm__("rax") = SYS_munmap;
    __asm__ volatile("syscall" : "+r"(rax) : "r"(rdi), "r"(rsi) : "rcx", "r11", "cc", "memory");
    result = rax;
#elif defined(__riscv)
    register long a0 __asm__("a0") = (long)addr;
    register long a1 __asm__("a1") = (long)length;
    register long a7 __asm__("a7") = SYS_munmap;
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");
    result = a0;
#endif
    CHECK_SYSCALL_ERROR_INTEGER(result);
    return (int)result;
}

template<long NR, typename... Args>
[[gnu::always_inline]] static inline long er_syscall(Args... args) {
    long result;
#if defined(__aarch64__)
    static_assert(sizeof...(args) <= 6, "er_syscall: too many arguments (max 6)");
    long a[6] = {};
    { long tmp[] = { (long)args... }; for (size_t i = 0; i < sizeof...(args); i++) a[i] = tmp[i]; }
    register long x0 __asm__("x0") = a[0];
    register long x1 __asm__("x1") = a[1];
    register long x2 __asm__("x2") = a[2];
    register long x3 __asm__("x3") = a[3];
    register long x4 __asm__("x4") = a[4];
    register long x5 __asm__("x5") = a[5];
    register long x8 __asm__("x8") = NR;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8) : "cc", "memory");
    result = x0;
#elif defined(__x86_64__)
    result = syscall(NR, args...);
#elif defined(__arm__)
    result = syscall(NR, args...);
#elif defined(__riscv)
    result = syscall(NR, args...);
#else
    result = syscall(NR, args...);
#endif
    return result;
}

struct ElfInfo {
    ElfW(Addr)              base_addr_;
    ElfW(Addr)              bias_addr_;
    ElfW(Ehdr)*             header_;
    ElfW(Phdr)*             program_header_;
    ElfW(Dyn)*              dynamic_;
    ElfW(Word)              dynamic_size_;
    const char*             dyn_str_;
    ElfW(Sym)*              dyn_sym_;
    ElfW(Word)              dyn_str_size_;
    ElfW(Addr)              relr_;
    ElfW(Word)              relr_size_;
    ElfW(Word)              relr_entry_size_;
    std::span<const ElfW(Addr)> relr_entries;
    ElfW(Addr)              rel_plt_;
    ElfW(Word)              rel_plt_size_;
    ElfW(Addr)              rel_dyn_;
    ElfW(Word)              rel_dyn_size_;
    ElfW(Addr)              rel_android_;
    ElfW(Word)              rel_android_size_;
    uint32_t*               bucket_;
    uint32_t                bucket_count_;
    uint32_t*               chain_;
    uint32_t                sym_offset_;
    uint32_t                sym_count_;
    ElfW(Addr)*             bloom_;
    uint32_t                bloom_size_;
    uint32_t                bloom_shift_;
    bool                    rel_plt_is_rela_;
    bool                    rel_dyn_is_rela_;
    bool                    rel_android_is_rela_;
    bool                    valid_;
    bool                    relro_active_;
};

void   elfutil_init(ElfInfo* elf, uintptr_t base_addr);
size_t elfutil_find_plt_addr(const ElfInfo* elf, const char* name, uintptr_t** out_addrs);
size_t elfutil_find_plt_addr_by_prefix(const ElfInfo* elf, const char* name_prefix, uintptr_t** out_addrs);

struct MapEntry {
    uintptr_t start;
    uintptr_t end;
    int       perms;
    bool      is_private;
    uintptr_t offset;
    dev_t     dev;
    ino_t     inode;
    char*     path;
};

struct MapInfo {
    MapEntry* maps;
    size_t    length;
};

using ErCleanupCallback = std::function<void()>;

[[nodiscard]] MapInfo* redirector_scan_maps(const char* pid);
void                   redirector_free_maps(MapInfo* maps);
[[nodiscard]] bool     redirector_register_hook(dev_t dev, ino_t inode, const char* symbol, void* callback, void** backup);
[[nodiscard]] bool     redirector_register_hook_by_prefix(dev_t dev, ino_t inode, const char* symbol_prefix, void* callback, void** backup);
[[nodiscard]] bool     redirector_register_hook_with_offset(dev_t dev, ino_t inode, uintptr_t offset, size_t size, const char* symbol, void* callback, void** backup);
[[nodiscard]] bool     redirector_register_got_hook(dev_t dev, ino_t inode, const char* symbol, void* callback, void** backup);
[[nodiscard]] bool     redirector_commit_hook_manual(MapInfo* maps);
[[nodiscard]] bool     redirector_commit_hook();
[[nodiscard]] bool     redirector_unhook(dev_t dev, ino_t inode, const char* symbol);
[[nodiscard]] bool     er_set_stealth_level(ErStealthLevel level);
[[nodiscard]] bool     invalidate_backups();
void                   redirector_free_resources();
void                   er_set_cleanup_callback(ErCleanupCallback cb);
[[nodiscard]] bool     er_init_for_zygisk();

// ==================== RUST SUBMODULE İÇİN C LINKAGE ====================
#ifdef __cplusplus
extern "C" {
#endif

// Mevcut public API'lerini buraya yaz (hepsini kopyala-yapıştır yap)
MapInfo* redirector_scan_maps(const char* pid);
void     redirector_free_maps(MapInfo* maps);

bool redirector_register_hook(dev_t dev, ino_t inode, const char* symbol, void* callback, void** backup);
bool redirector_register_hook_by_prefix(dev_t dev, ino_t inode, const char* symbol_prefix, void* callback, void** backup);
bool redirector_register_hook_with_offset(dev_t dev, ino_t inode, uintptr_t offset, size_t size, const char* symbol, void* callback, void** backup);
bool redirector_register_got_hook(dev_t dev, ino_t inode, const char* symbol, void* callback, void** backup);

bool redirector_commit_hook_manual(MapInfo* maps);
bool redirector_commit_hook();
bool redirector_unhook(dev_t dev, ino_t inode, const char* symbol);

bool er_set_stealth_level(ErStealthLevel level);
bool invalidate_backups();
void redirector_free_resources();
void er_set_cleanup_callback(ErCleanupCallback cb);
bool er_init_for_zygisk();

#ifdef __cplusplus
}
#endif
