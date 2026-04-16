#include "Redirector.hpp"

#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <unistd.h>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <string>
#include <optional>
#include <functional>

#if defined(__arm__)
#define ELF_R_GENERIC_JUMP_SLOT R_ARM_JUMP_SLOT
#define ELF_R_GENERIC_GLOB_DAT  R_ARM_GLOB_DAT
#define ELF_R_GENERIC_ABS       R_ARM_ABS32
#elif defined(__aarch64__)
#define ELF_R_GENERIC_JUMP_SLOT R_AARCH64_JUMP_SLOT
#define ELF_R_GENERIC_GLOB_DAT  R_AARCH64_GLOB_DAT
#define ELF_R_GENERIC_ABS       R_AARCH64_ABS64
#elif defined(__i386__)
#define ELF_R_GENERIC_JUMP_SLOT R_386_JMP_SLOT
#define ELF_R_GENERIC_GLOB_DAT  R_386_GLOB_DAT
#define ELF_R_GENERIC_ABS       R_386_32
#elif defined(__x86_64__)
#define ELF_R_GENERIC_JUMP_SLOT R_X86_64_JUMP_SLOT
#define ELF_R_GENERIC_GLOB_DAT  R_X86_64_GLOB_DAT
#define ELF_R_GENERIC_ABS       R_X86_64_64
#elif defined(__riscv)
#define ELF_R_GENERIC_JUMP_SLOT R_RISCV_JUMP_SLOT
#define ELF_R_GENERIC_GLOB_DAT  R_RISCV_64
#define ELF_R_GENERIC_ABS       R_RISCV_64
#endif

#ifdef __LP64__
#define ELF_R_SYM(info)        ELF64_R_SYM(info)
#define ELF_R_INFO(sym, type)  ELF64_R_INFO(sym, type)
#define ELF_R_TYPE(info)       ELF64_R_TYPE(info)
#else
#define ELF_R_SYM(info)        ELF32_R_SYM(info)
#define ELF_R_INFO(sym, type)  ELF32_R_INFO(sym, type)
#define ELF_R_TYPE(info)       ELF32_R_TYPE(info)
#endif

static std::mutex                       g_mutex;
static ErStealthLevel                   g_stealth_level = ErStealthLevel::DIRECT_PATCH;
static std::atomic<bool>                g_zygisk_mode{false};
static ErCleanupCallback                g_cleanup_cb;

struct SymbolCacheKey {
    std::string name;
    uintptr_t   base;
    bool operator==(const SymbolCacheKey& o) const { return base == o.base && name == o.name; }
};

struct SymbolCacheKeyHash {
    size_t operator()(const SymbolCacheKey& k) const {
        size_t h1 = std::hash<std::string>{}(k.name);
        size_t h2 = std::hash<uintptr_t>{}(k.base);
        return h1 ^ (h2 << 32) ^ (h2 >> 32);
    }
};

static std::unordered_map<SymbolCacheKey, uint32_t, SymbolCacheKeyHash> g_sym_cache;

struct Sleb128Decoder {
    const uint8_t* current;
    const uint8_t* end;
};

static void sleb128_init(Sleb128Decoder* d, const uint8_t* buf, size_t count) {
    d->current = buf;
    d->end     = buf + count;
}

static int64_t sleb128_decode(Sleb128Decoder* d) {
    int64_t value = 0;
    size_t  shift = 0;
    uint8_t byte;
    do {
        if (d->current >= d->end) LOGF("SLEB128 buffer overrun");
        byte = *d->current++;
        value |= ((int64_t)(byte & 0x7F)) << shift;
        shift += 7;
    } while (byte & 0x80);
    if (shift < sizeof(int64_t) * CHAR_BIT && (byte & 0x40))
        value |= -((int64_t)1 << shift);
    return value;
}

static void* elf_offset(ElfW(Ehdr)* head, ElfW(Off) off) {
    return (void*)((uintptr_t)head + off);
}

static bool elf_set_ptr(ElfW(Addr)* ptr, ElfW(Addr) base, ElfW(Addr) bias, ElfW(Addr) off) {
    ElfW(Addr) val = bias + off;
    if (val >= base) { *ptr = val; return true; }
    LOGE("elf_set_ptr underflow: base=0x%" PRIxPTR " bias=0x%" PRIxPTR " off=0x%" PRIxPTR,
         (uintptr_t)base, (uintptr_t)bias, (uintptr_t)off);
    *ptr = 0;
    return false;
}

void elfutil_init(ElfInfo* elf, uintptr_t base_addr) {
    memset(elf, 0, sizeof(*elf));
    elf->header_    = (ElfW(Ehdr)*)base_addr;
    elf->base_addr_ = base_addr;

    if (memcmp(elf->header_->e_ident, ELFMAG, SELFMAG) != 0) return;
#ifdef __LP64__
    if (elf->header_->e_ident[EI_CLASS] != ELFCLASS64) return;
#else
    if (elf->header_->e_ident[EI_CLASS] != ELFCLASS32) return;
#endif
    if (elf->header_->e_ident[EI_DATA]    != ELFDATA2LSB) return;
    if (elf->header_->e_ident[EI_VERSION] != EV_CURRENT)  return;
    if (elf->header_->e_type != ET_EXEC && elf->header_->e_type != ET_DYN) return;
#if defined(__arm__)
    if (elf->header_->e_machine != EM_ARM)    return;
#elif defined(__aarch64__)
    if (elf->header_->e_machine != EM_AARCH64) return;
#elif defined(__i386__)
    if (elf->header_->e_machine != EM_386)    return;
#elif defined(__x86_64__)
    if (elf->header_->e_machine != EM_X86_64) return;
#elif defined(__riscv)
    if (elf->header_->e_machine != EM_RISCV)  return;
#endif
    if (elf->header_->e_version != EV_CURRENT) return;

    elf->program_header_ = (ElfW(Phdr)*)elf_offset(elf->header_, elf->header_->e_phoff);
    uintptr_t ph_off = (uintptr_t)elf->program_header_;
    bool has_relro = false;
    for (int i = 0; i < elf->header_->e_phnum; i++, ph_off += elf->header_->e_phentsize) {
        ElfW(Phdr)* ph = (ElfW(Phdr)*)ph_off;
        if (ph->p_type == PT_LOAD && ph->p_offset == 0) {
            if (elf->base_addr_ >= ph->p_vaddr)
                elf->bias_addr_ = elf->base_addr_ - ph->p_vaddr;
        } else if (ph->p_type == PT_DYNAMIC) {
            elf->dynamic_      = (ElfW(Dyn)*)ph->p_vaddr;
            elf->dynamic_size_ = ph->p_memsz;
        } else if (ph->p_type == PT_GNU_RELRO) {
            has_relro = true;
        }
    }
    elf->relro_active_ = has_relro;
    if (!elf->dynamic_ || !elf->bias_addr_) return;
    elf->dynamic_ = (ElfW(Dyn)*)(elf->bias_addr_ + (uintptr_t)elf->dynamic_);

    ElfW(Dyn)* dyn_end = elf->dynamic_ + elf->dynamic_size_ / sizeof(ElfW(Dyn));
    for (ElfW(Dyn)* d = elf->dynamic_; d < dyn_end; ++d) {
        switch (d->d_tag) {
            case DT_NULL:  d = dyn_end; break;
            case DT_STRTAB: if (!elf_set_ptr((ElfW(Addr)*)&elf->dyn_str_, elf->base_addr_, elf->bias_addr_, d->d_un.d_ptr)) return; break;
            case DT_SYMTAB: if (!elf_set_ptr((ElfW(Addr)*)&elf->dyn_sym_, elf->base_addr_, elf->bias_addr_, d->d_un.d_ptr)) return; break;
            case DT_PLTREL: elf->rel_plt_is_rela_ = d->d_un.d_val == DT_RELA; break;
            case DT_JMPREL: if (!elf_set_ptr(&elf->rel_plt_, elf->base_addr_, elf->bias_addr_, d->d_un.d_ptr)) return; break;
            case DT_PLTRELSZ: elf->rel_plt_size_ = d->d_un.d_val; break;
            case DT_REL:  if (!elf_set_ptr(&elf->rel_dyn_, elf->base_addr_, elf->bias_addr_, d->d_un.d_ptr)) return; elf->rel_dyn_is_rela_ = false; break;
            case DT_RELA: if (!elf_set_ptr(&elf->rel_dyn_, elf->base_addr_, elf->bias_addr_, d->d_un.d_ptr)) return; elf->rel_dyn_is_rela_ = true;  break;
            case DT_RELSZ: case DT_RELASZ: elf->rel_dyn_size_ = d->d_un.d_val; break;
            case DT_ANDROID_REL:  if (!elf_set_ptr(&elf->rel_android_, elf->base_addr_, elf->bias_addr_, d->d_un.d_ptr)) return; elf->rel_android_is_rela_ = false; break;
            case DT_ANDROID_RELA: if (!elf_set_ptr(&elf->rel_android_, elf->base_addr_, elf->bias_addr_, d->d_un.d_ptr)) return; elf->rel_android_is_rela_ = true;  break;
            case DT_ANDROID_RELSZ: case DT_ANDROID_RELASZ: elf->rel_android_size_ = d->d_un.d_val; break;
            case DT_RELR: case 0x6fffe000: {
                ElfW(Addr) relr_addr = 0;
                if (elf_set_ptr(&relr_addr, elf->base_addr_, elf->bias_addr_, d->d_un.d_ptr)) {
                    elf->relr_ = relr_addr;
                }
                break;
            }
            case DT_RELRSZ: case 0x6fffe001: elf->relr_size_ = d->d_un.d_val; break;
            case DT_RELRENT: case 0x6fffe003: elf->relr_entry_size_ = d->d_un.d_val; break;
            case DT_HASH: {
                if (elf->bloom_) continue;
                ElfW(Word)* raw    = (ElfW(Word)*)(elf->bias_addr_ + d->d_un.d_ptr);
                elf->bucket_count_ = raw[0];
                elf->bucket_       = raw + 2;
                elf->chain_        = elf->bucket_ + elf->bucket_count_;
                break;
            }
            case DT_GNU_HASH: {
                ElfW(Word)* raw    = (ElfW(Word)*)(elf->bias_addr_ + d->d_un.d_ptr);
                elf->bucket_count_ = raw[0];
                elf->sym_offset_   = raw[1];
                elf->bloom_size_   = raw[2];
                elf->bloom_shift_  = raw[3];
                elf->bloom_        = (ElfW(Addr)*)(raw + 4);
                elf->bucket_       = (uint32_t*)(elf->bloom_ + elf->bloom_size_);
                elf->chain_        = elf->bucket_ + elf->bucket_count_ - elf->sym_offset_;
                break;
            }
            default: break;
        }
    }
    if (elf->relr_ && elf->relr_size_ && elf->relr_entry_size_) {
        size_t count = elf->relr_size_ / elf->relr_entry_size_;
        elf->relr_entries = std::span<const ElfW(Addr)>(
            reinterpret_cast<const ElfW(Addr)*>(elf->relr_),
            count
        );
    }
    if (elf->rel_android_) {
        const char* rel = (const char*)elf->rel_android_;
        if (elf->rel_android_size_ < 4 || rel[0] != 'A' || rel[1] != 'P' || rel[2] != 'S' || rel[3] != '2') return;
        elf->rel_android_      += 4;
        elf->rel_android_size_ -= 4;
    }
    elf->valid_ = true;
}

struct AndroidRelocBuffer { void* data; ElfW(Word) size; };

static bool elfutil_unpack_android_relocs(const ElfInfo* elf, AndroidRelocBuffer* buf) {
    if (!elf->rel_android_ || elf->rel_android_size_ == 0) return false;
    Sleb128Decoder dec;
    sleb128_init(&dec, (const uint8_t*)elf->rel_android_, elf->rel_android_size_);
    int64_t num_relocs = sleb128_decode(&dec);
    if (num_relocs <= 0) return false;
    size_t entry_sz = elf->rel_android_is_rela_ ? sizeof(ElfW(Rela)) : sizeof(ElfW(Rel));
    void* entries = calloc((size_t)num_relocs, entry_sz);
    if (!entries) { LOGE("OOM android relocs"); return false; }
    size_t       out_idx = 0;
    ElfW(Addr)   cur_off = (ElfW(Addr))sleb128_decode(&dec);
    const size_t GROUPED_BY_INFO         = 1;
    const size_t GROUPED_BY_OFFSET_DELTA = 2;
    const size_t GROUPED_BY_ADDEND       = 4;
    const size_t GROUP_HAS_ADDEND        = 8;
    for (int64_t i = 0; i < num_relocs; ) {
        uint64_t grp_size  = (uint64_t)sleb128_decode(&dec);
        uint64_t grp_flags = (uint64_t)sleb128_decode(&dec);
        size_t   off_delta = 0;
        if (grp_flags & GROUPED_BY_OFFSET_DELTA) off_delta = (size_t)sleb128_decode(&dec);
        uint32_t sym_idx = 0, type = 0, r_addend = 0;
        if (grp_flags & GROUPED_BY_INFO) {
            ElfW(Addr) r_info = (ElfW(Addr))sleb128_decode(&dec);
            sym_idx = ELF_R_SYM(r_info);
            type    = ELF_R_TYPE(r_info);
        }
        size_t grp_flags_reloc = 0;
        if (elf->rel_android_is_rela_) {
            grp_flags_reloc = grp_flags & (GROUP_HAS_ADDEND | GROUPED_BY_ADDEND);
            if (grp_flags_reloc == (GROUP_HAS_ADDEND | GROUPED_BY_ADDEND))
                r_addend += (uint32_t)sleb128_decode(&dec);
            else if (!(grp_flags_reloc & GROUP_HAS_ADDEND))
                r_addend = 0;
        }
        for (size_t j = 0; j < grp_size; ++j) {
            if (grp_flags & GROUPED_BY_OFFSET_DELTA) cur_off += off_delta;
            else cur_off += (ElfW(Addr))sleb128_decode(&dec);
            if (!(grp_flags & GROUPED_BY_INFO)) {
                ElfW(Addr) r_info = (ElfW(Addr))sleb128_decode(&dec);
                sym_idx = ELF_R_SYM(r_info);
                type    = ELF_R_TYPE(r_info);
            }
            if (elf->rel_android_is_rela_ && grp_flags_reloc == GROUP_HAS_ADDEND)
                r_addend += (uint32_t)sleb128_decode(&dec);
            if (elf->rel_android_is_rela_) {
                ElfW(Rela)* rela = (ElfW(Rela)*)entries;
                rela[out_idx] = { cur_off, ELF_R_INFO(sym_idx, type), static_cast<decltype(rela[0].r_addend)>(r_addend) };
            } else {
                ElfW(Rel)* rel = (ElfW(Rel)*)entries;
                rel[out_idx] = { cur_off, ELF_R_INFO(sym_idx, type) };
            }
            out_idx++;
        }
        i += (int64_t)grp_size;
    }
    buf->data = entries;
    buf->size = (ElfW(Word))(out_idx * entry_sz);
    return true;
}

static void er_relr_looper(const ElfInfo* elf, uintptr_t** out, size_t* cnt) {
    if (elf->relr_entries.empty()) return;
    ElfW(Addr) base = 0;
    bool first = true;
    for (size_t i = 0; i < elf->relr_entries.size(); ++i) {
        ElfW(Addr) entry = elf->relr_entries[i];
        if ((entry & 1) == 0) {
            if (first) {
                base  = elf->bias_addr_ + entry;
                first = false;
                uintptr_t addr = base;
                if (addr > elf->base_addr_) {
                    uintptr_t* tmp = (uintptr_t*)realloc(*out, (*cnt + 1) * sizeof(uintptr_t));
                    if (tmp) { *out = tmp; (*out)[(*cnt)++] = addr; }
                }
            } else {
                base += entry;
            }
            continue;
        }
        for (int bit = 1; bit < (int)(sizeof(ElfW(Addr)) * 8); ++bit) {
            if (entry & ((ElfW(Addr))1 << bit)) {
                uintptr_t addr = base + (uintptr_t)(bit - 1) * sizeof(ElfW(Addr));
                if (addr > elf->base_addr_) {
                    uintptr_t* tmp = (uintptr_t*)realloc(*out, (*cnt + 1) * sizeof(uintptr_t));
                    if (tmp) { *out = tmp; (*out)[(*cnt)++] = addr; }
                }
            }
        }
        base += (sizeof(ElfW(Addr)) * 8 - 1) * sizeof(ElfW(Addr));
    }
}

static uint32_t gnu_lookup_cached(const ElfInfo* elf, std::string_view name) {
    SymbolCacheKey key{ std::string(name), elf->base_addr_ };
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        auto it = g_sym_cache.find(key);
        if (it != g_sym_cache.end()) return it->second;
    }
    static const uint32_t kBits  = sizeof(ElfW(Addr)) * 8;
    static const uint32_t kInit  = 5381;
    static const uint32_t kShift = 5;
    if (!elf->bucket_ || !elf->bucket_count_ || !elf->bloom_ || !elf->bloom_size_) return 0;
    uint32_t hash = kInit;
    for (char c : name) hash += (hash << kShift) + (uint8_t)c;
    uint32_t    bi       = (hash / kBits) % elf->bloom_size_;
    ElfW(Addr)  bword    = elf->bloom_[bi];
    uintptr_t   bit_lo   = (uintptr_t)1 << (hash % kBits);
    uintptr_t   bit_hi   = (uintptr_t)1 << ((hash >> elf->bloom_shift_) % kBits);
    if ((bword & (bit_lo | bit_hi)) != (bit_lo | bit_hi)) return 0;
    uint32_t idx = elf->bucket_[hash % elf->bucket_count_];
    if (idx < elf->sym_offset_) return 0;
    for (;; idx++) {
        ElfW(Sym)* sym = elf->dyn_sym_ + idx;
        if (((elf->chain_[idx] ^ hash) >> 1) == 0 && name == std::string_view(elf->dyn_str_ + sym->st_name)) {
            std::lock_guard<std::mutex> lk(g_mutex);
            g_sym_cache[key] = idx;
            return idx;
        }
        if (elf->chain_[idx] & 1) break;
    }
    return 0;
}

static uint32_t gnu_lookup(const ElfInfo* elf, const char* name) {
    return gnu_lookup_cached(elf, name);
}

static uint32_t elf_lookup(const ElfInfo* elf, const char* name) {
    static const uint32_t kMask  = 0xf0000000;
    static const uint32_t kShift = 24;
    if (!elf->bucket_ || elf->bloom_) return 0;
    uint32_t hash = 0, tmp;
    for (int i = 0; name[i]; i++) {
        hash = (hash << 4) + name[i];
        tmp  = hash & kMask;
        hash ^= tmp;
        hash ^= tmp >> kShift;
    }
    for (int idx = (int)elf->bucket_[hash % elf->bucket_count_]; idx; idx = (int)elf->chain_[idx]) {
        if (strcmp(name, elf->dyn_str_ + elf->dyn_sym_[idx].st_name) == 0) return (uint32_t)idx;
    }
    return 0;
}

static uint32_t linear_lookup(const ElfInfo* elf, const char* name) {
    if (!elf->dyn_sym_ || !elf->sym_offset_) return 0;
    for (uint32_t i = 0; i < elf->sym_offset_; i++) {
        if (strcmp(name, elf->dyn_str_ + elf->dyn_sym_[i].st_name) == 0) return i;
    }
    return 0;
}

static void looper(const ElfInfo* elf, uint32_t idx, const void* rel_ptr, ElfW(Word) rel_size,
                   bool is_rela, bool is_plt, uintptr_t** res, size_t* cnt) {
    size_t entry_sz = is_rela ? sizeof(ElfW(Rela)) : sizeof(ElfW(Rel));
    for (const char* p = (const char*)rel_ptr; p < (const char*)rel_ptr + rel_size; p += entry_sz) {
        ElfW(Xword) r_info   = is_rela ? ((ElfW(Rela)*)p)->r_info   : ((ElfW(Rel)*)p)->r_info;
        ElfW(Addr)  r_offset = is_rela ? ((ElfW(Rela)*)p)->r_offset : ((ElfW(Rel)*)p)->r_offset;
        uint32_t r_sym  = ELF_R_SYM(r_info);
        uint32_t r_type = ELF_R_TYPE(r_info);
        if (r_sym != idx) continue;
        if ( is_plt && r_type != ELF_R_GENERIC_JUMP_SLOT) continue;
        if (!is_plt && r_type != ELF_R_GENERIC_ABS && r_type != ELF_R_GENERIC_GLOB_DAT) continue;
        uintptr_t addr = elf->bias_addr_ + r_offset;
        if (addr <= elf->base_addr_) continue;
        uintptr_t* tmp = (uintptr_t*)realloc(*res, (*cnt + 1) * sizeof(uintptr_t));
        if (!tmp) { LOGE("OOM looper"); free(*res); *res = nullptr; *cnt = 0; return; }
        *res = tmp;
        (*res)[(*cnt)++] = addr;
        if (is_plt) break;
    }
}

size_t elfutil_find_plt_addr(const ElfInfo* elf, const char* name, uintptr_t** out) {
    if (!elf->valid_ || !out) return 0;
    uint32_t idx = gnu_lookup(elf, name);
    if (!idx) idx = elf_lookup(elf, name);
    if (!idx) idx = linear_lookup(elf, name);
    if (!idx) return 0;
    *out = nullptr;
    size_t cnt = 0;
    looper(elf, idx, (void*)elf->rel_plt_, elf->rel_plt_size_, elf->rel_plt_is_rela_, true,  out, &cnt);
    looper(elf, idx, (void*)elf->rel_dyn_, elf->rel_dyn_size_, elf->rel_dyn_is_rela_, false, out, &cnt);
    AndroidRelocBuffer abuf{};
    if (elfutil_unpack_android_relocs(elf, &abuf)) {
        looper(elf, idx, abuf.data, abuf.size, elf->rel_android_is_rela_, false, out, &cnt);
        free(abuf.data);
    }
    er_relr_looper(elf, out, &cnt);
    return cnt;
}

static void looper_by_prefix(const ElfInfo* elf, const void* rel_ptr, ElfW(Word) rel_size,
                              bool is_rela, bool is_plt, const char* prefix, size_t pfx_len,
                              uintptr_t** res, size_t* cnt) {
    if (!rel_ptr || !rel_size || !elf->dyn_sym_ || !elf->dyn_str_) return;
    size_t entry_sz = is_rela ? sizeof(ElfW(Rela)) : sizeof(ElfW(Rel));
    for (const char* p = (const char*)rel_ptr; p < (const char*)rel_ptr + rel_size; p += entry_sz) {
        ElfW(Xword) r_info   = is_rela ? ((ElfW(Rela)*)p)->r_info   : ((ElfW(Rel)*)p)->r_info;
        ElfW(Addr)  r_offset = is_rela ? ((ElfW(Rela)*)p)->r_offset : ((ElfW(Rel)*)p)->r_offset;
        uint32_t r_sym  = ELF_R_SYM(r_info);
        uint32_t r_type = ELF_R_TYPE(r_info);
        if ( is_plt && r_type != ELF_R_GENERIC_JUMP_SLOT) continue;
        if (!is_plt && r_type != ELF_R_GENERIC_ABS && r_type != ELF_R_GENERIC_GLOB_DAT) continue;
        ElfW(Sym)* sym = elf->dyn_sym_ + r_sym;
        if (!sym || !sym->st_name) continue;
        const char* sname = elf->dyn_str_ + sym->st_name;
        if (!sname || strncmp(sname, prefix, pfx_len) != 0) continue;
        uintptr_t addr = elf->bias_addr_ + r_offset;
        if (addr <= elf->base_addr_) continue;
        uintptr_t* tmp = (uintptr_t*)realloc(*res, (*cnt + 1) * sizeof(uintptr_t));
        if (!tmp) { LOGE("OOM prefix looper"); free(*res); *res = nullptr; *cnt = 0; return; }
        *res = tmp;
        (*res)[(*cnt)++] = addr;
    }
}

size_t elfutil_find_plt_addr_by_prefix(const ElfInfo* elf, const char* prefix, uintptr_t** out) {
    if (!elf->valid_ || !out) return 0;
    size_t pfx_len = strlen(prefix);
    size_t cnt = 0;
    uintptr_t* res = nullptr;
    looper_by_prefix(elf, (void*)elf->rel_plt_, elf->rel_plt_size_, elf->rel_plt_is_rela_, true,  prefix, pfx_len, &res, &cnt);
    looper_by_prefix(elf, (void*)elf->rel_dyn_, elf->rel_dyn_size_, elf->rel_dyn_is_rela_, false, prefix, pfx_len, &res, &cnt);
    AndroidRelocBuffer abuf{};
    if (elfutil_unpack_android_relocs(elf, &abuf)) {
        looper_by_prefix(elf, abuf.data, abuf.size, elf->rel_android_is_rela_, false, prefix, pfx_len, &res, &cnt);
        free(abuf.data);
    }
    *out = res;
    return cnt;
}

struct RegisterInfo {
    dev_t     dev;
    ino_t     inode;
    uintptr_t offset_start;
    uintptr_t offset_end;
    char*     symbol;
    bool      is_prefix;
    bool      is_got;
    void*     callback;
    void**    backup;
};

struct HookEntry {
    uintptr_t addr;
    uintptr_t backup;
};

struct HookInfo {
    MapEntry   map;
    HookEntry* entries;
    size_t     entries_len;
    uintptr_t  backup_region;
    ElfInfo    elf;
    bool       self;
};

static RegisterInfo*    g_regs         = nullptr;
static size_t           g_regs_len     = 0;
static HookInfo*        g_hooks        = nullptr;
static size_t           g_hooks_len    = 0;

static inline uintptr_t page_start_addr(uintptr_t addr) {
    if (!k_page_size) k_page_size = (uintptr_t)getpagesize();
    return addr & ~(k_page_size - 1);
}

static inline char* page_start(uintptr_t addr) {
    return (char*)page_start_addr(addr);
}

static inline char* page_end(uintptr_t addr) {
    if (!k_page_size) k_page_size = (uintptr_t)getpagesize();
    return (char*)(page_start_addr(addr) + k_page_size);
}

static inline void* redirector_memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

#ifdef __LP64__
static inline uintptr_t align_up(uintptr_t v, uintptr_t a) { return (v + a - 1) & ~(a - 1); }

static void* find_backup_hint(size_t needed) {
    MapInfo* maps = redirector_scan_maps("self");
    if (!maps) return nullptr;
    if (!k_page_size) k_page_size = (uintptr_t)getpagesize();
    needed = (size_t)align_up((uintptr_t)needed, k_page_size);
    const uintptr_t min_addr  = 0x100000000ULL;
    const uintptr_t self_addr = (uintptr_t)find_backup_hint;
    uintptr_t self_end = 0;
    size_t    self_idx = maps->length;
    for (size_t i = 0; i < maps->length; i++) {
        MapEntry* m = &maps->maps[i];
        if (self_addr < m->start || self_addr >= m->end) continue;
        self_end = m->end; self_idx = i; break;
    }
    if (self_idx == maps->length) { LOGE("self mapping not found"); redirector_free_maps(maps); return nullptr; }
    for (size_t i = self_idx + 1; i < maps->length; i++) {
        MapEntry* m = &maps->maps[i];
        if (m->start > align_up(self_end, k_page_size)) break;
        self_end = m->end;
    }
    uintptr_t prev_end = 0, hint = 0;
    for (size_t i = 0; i < maps->length; i++) {
        MapEntry*  m         = &maps->maps[i];
        uintptr_t  gap_start = align_up(prev_end ? prev_end : min_addr, k_page_size);
        if (gap_start < self_end && self_end <= m->start) { prev_end = m->end; continue; }
        if (m->start > gap_start && needed <= m->start - gap_start) hint = gap_start;
        prev_end = m->end;
    }
    redirector_free_maps(maps);
    return (void*)hint;
}
#endif

static bool hook_info_matches(const HookInfo* info, const RegisterInfo* reg) {
    return reg->dev == info->map.dev && reg->inode == info->map.inode &&
           info->map.offset >= reg->offset_start && info->map.offset < reg->offset_end;
}

static void free_hook_info(HookInfo* h) {
    free(h->map.path);
    free(h->entries);
    memset(h, 0, sizeof(*h));
}

static void free_hooks(HookInfo* arr, size_t len) {
    for (size_t i = 0; i < len; i++) free_hook_info(&arr[i]);
    free(arr);
}

static int cmp_hooks_desc(const void* a, const void* b) {
    const HookInfo* ha = (const HookInfo*)a;
    const HookInfo* hb = (const HookInfo*)b;
    if (ha->map.start < hb->map.start) return  1;
    if (ha->map.start > hb->map.start) return -1;
    return 0;
}

static ssize_t send_fd(int sock, int sendfd) {
    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    char buf[1] = {0};
    struct iovec iov = { buf, 1 };
    struct msghdr msg = { nullptr, 0, &iov, 1, cmsgbuf, sizeof(cmsgbuf), 0 };
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len   = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    memcpy(CMSG_DATA(cmsg), &sendfd, sizeof(int));
    ssize_t r = sendmsg(sock, &msg, 0);
    if (r == -1) LOGE("sendmsg: %s", strerror(errno));
    return r;
}

static int recv_fd(int sock) {
    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    int  cnt = 1;
    struct iovec iov = { &cnt, sizeof(cnt) };
    struct msghdr msg = { nullptr, 0, &iov, 1, cmsgbuf, sizeof(cmsgbuf), 0 };
    if (recvmsg(sock, &msg, MSG_WAITALL) == -1) { LOGE("recvmsg: %s", strerror(errno)); return -1; }
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg) { LOGE("CMSG_FIRSTHDR: %s", strerror(errno)); return -1; }
    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
}

static bool er_is_linker_namespace_map(const char* path) {
    if (!path) return false;
    if (strstr(path, "/apex/com.android.runtime")) return true;
    if (strstr(path, "/apex/com.android.art"))     return true;
    if (strncmp(path, "[anon:linker_alloc", 18) == 0) return true;
    if (strncmp(path, "[anon:scudo",       11) == 0) return true;
    return false;
}

MapInfo* redirector_scan_maps(const char* pid) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) { LOGE("socketpair fail"); return nullptr; }
    long child = er_syscall<SYS_clone>(SIGCHLD, 0);
    if (child == -1) { LOGE("clone fail"); close(sv[0]); close(sv[1]); return nullptr; }
    if (child == 0) {
        close(sv[0]);
        char path[64];
        snprintf(path, sizeof(path), "/proc/%s/maps", pid);
        int fd = open(path, O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            uint8_t z = 0;
            TEMP_FAILURE_RETRY(write(sv[1], &z, 1));
            close(sv[1]); _exit(EXIT_FAILURE);
        }
        if (send_fd(sv[1], fd) < 0) { close(fd); close(sv[1]); _exit(EXIT_FAILURE); }
        uint8_t ok = 1;
        TEMP_FAILURE_RETRY(read(sv[1], &ok, 1));
        close(fd); close(sv[1]); _exit(EXIT_SUCCESS);
    }
    close(sv[1]);
    int fd = recv_fd(sv[0]);
    if (fd < 0) { close(sv[0]); waitpid((pid_t)child, nullptr, 0); return nullptr; }
    FILE* fp = fdopen(fd, "r");
    if (!fp) { close(fd); close(sv[0]); waitpid((pid_t)child, nullptr, 0); return nullptr; }

    MapInfo* info = (MapInfo*)calloc(1, sizeof(MapInfo));
    if (!info) { fclose(fp); close(sv[0]); waitpid((pid_t)child, nullptr, 0); return nullptr; }
    size_t cap = 2;
    info->maps = (MapEntry*)malloc(cap * sizeof(MapEntry));
    if (!info->maps) { free(info); fclose(fp); close(sv[0]); waitpid((pid_t)child, nullptr, 0); return nullptr; }

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        size_t llen = strlen(line);
        if (llen && line[llen - 1] == '\n') line[llen - 1] = '\0';
        uintptr_t start, end, offset;
        unsigned  dev_maj, dev_min;
        ino_t     inode;
        char      perms[5] = {};
        int       path_off;
        if (sscanf(line, "%" PRIxPTR "-%" PRIxPTR " %4s %" PRIxPTR " %x:%x %lu %n",
                   &start, &end, perms, &offset, &dev_maj, &dev_min, &inode, &path_off) != 7) continue;
        uint8_t pb = 0;
        if (perms[0] == 'r') pb |= PROT_READ;
        if (perms[1] == 'w') pb |= PROT_WRITE;
        if (perms[2] == 'x') pb |= PROT_EXEC;
        while (isspace((unsigned char)line[path_off])) path_off++;
        const char* raw_path = line + path_off;
        if (er_is_linker_namespace_map(raw_path)) continue;
        char* pstr = strdup(raw_path);
        if (!pstr) goto cleanup;
        if (info->length >= cap) {
            cap *= 2;
            MapEntry* tmp = (MapEntry*)realloc(info->maps, cap * sizeof(MapEntry));
            if (!tmp) { free(pstr); goto cleanup; }
            info->maps = tmp;
        }
        info->maps[info->length++] = { start, end, pb, perms[3] == 'p', offset, static_cast<dev_t>(makedev(dev_maj, dev_min)), inode, pstr };
        continue;
    cleanup:
        for (size_t i = 0; i < info->length; i++) free(info->maps[i].path);
        free(info->maps); free(info); fclose(fp); close(sv[0]); waitpid((pid_t)child, nullptr, 0);
        return nullptr;
    }
    fclose(fp);
    uint8_t done = 1;
    TEMP_FAILURE_RETRY(write(sv[0], &done, 1));
    close(sv[0]);
    MapEntry* tmp = (MapEntry*)realloc(info->maps, info->length * sizeof(MapEntry));
    if (tmp) info->maps = tmp;
    waitpid((pid_t)child, nullptr, 0);
    return info;
}

void redirector_free_maps(MapInfo* maps) {
    if (!maps) return;
    for (size_t i = 0; i < maps->length; i++) free(maps->maps[i].path);
    free(maps->maps); free(maps);
}

static bool do_hook_addr(HookInfo* hooks, size_t hooks_len, uintptr_t addr, uintptr_t cb, uintptr_t* bk);

struct SymAddrBatch {
    void** addrs;
    size_t len;
    void*  callback;
    void** backup;
};

static HookInfo* build_hook_infos(MapInfo* maps, size_t* out_len) {
    static ino_t self_inode = 0;
    static dev_t self_dev   = 0;
    if (!self_inode) {
        uintptr_t sa = (uintptr_t)__builtin_return_address(0);
        for (size_t i = 0; i < maps->length; i++) {
            MapEntry* m = &maps->maps[i];
            if (sa < m->start || sa >= m->end) continue;
            self_inode = m->inode; self_dev = m->dev; break;
        }
    }
    size_t cnt = 0;
    for (size_t i = 0; i < maps->length; i++) {
        MapEntry* m = &maps->maps[i];
        if (!m->is_private || !(m->perms & PROT_READ) || !m->path || m->path[0] == '[') continue;
        cnt++;
    }
    HookInfo* arr = (HookInfo*)calloc(cnt, sizeof(HookInfo));
    if (!arr) { LOGE("OOM hook infos"); return nullptr; }
    size_t idx = 0;
    for (size_t i = 0; i < maps->length; i++) {
        MapEntry* m = &maps->maps[i];
        if (!m->is_private || !(m->perms & PROT_READ) || !m->path || m->path[0] == '[') continue;
        arr[idx].map      = *m;
        arr[idx].map.path = strdup(m->path);
        if (!arr[idx].map.path) { free_hooks(arr, idx); return nullptr; }
        arr[idx].self = (m->inode == self_inode && m->dev == self_dev);
        idx++;
    }
    qsort(arr, idx, sizeof(HookInfo), cmp_hooks_desc);
    *out_len = idx;
    return arr;
}

static bool filter_hook_infos(HookInfo* arr, size_t* len) {
    size_t w = 0;
    for (size_t r = 0; r < *len; r++) {
        HookInfo* h = &arr[r];
        bool matched = false;
        for (size_t i = 0; i < g_regs_len; i++) {
            RegisterInfo* reg = &g_regs[i];
            if (!reg->symbol || !hook_info_matches(h, reg)) continue;
            matched = true; break;
        }
        if (matched) {
            if (w != r) { arr[w] = arr[r]; memset(&arr[r], 0, sizeof(HookInfo)); }
            w++;
        } else {
            free_hook_info(h);
        }
    }
    *len = w;
    return w > 0;
}

static bool copy_hook_info(HookInfo* dst, const HookInfo* src) {
    *dst = *src;
    dst->map.path = strdup(src->map.path);
    if (!dst->map.path) return false;
    if (src->entries_len > 0) {
        dst->entries = (HookEntry*)malloc(src->entries_len * sizeof(HookEntry));
        if (!dst->entries) { free(dst->map.path); return false; }
        memcpy(dst->entries, src->entries, src->entries_len * sizeof(HookEntry));
        dst->entries_len = src->entries_len;
    }
    return true;
}

static bool merge_hook_infos(HookInfo** new_arr, size_t* new_len, HookInfo* old_arr, size_t old_len) {
    for (size_t i = 0; i < old_len; i++) {
        HookInfo* oi    = &old_arr[i];
        bool      found = false;
        for (size_t j = 0; j < *new_len; j++) {
            if ((*new_arr)[j].map.start != oi->map.start) continue;
            free_hook_info(&(*new_arr)[j]);
            if (!copy_hook_info(&(*new_arr)[j], oi)) return false;
            found = true; break;
        }
        if (!found) {
            HookInfo* tmp = (HookInfo*)realloc(*new_arr, (*new_len + 1) * sizeof(HookInfo));
            if (!tmp) { PLOGE("realloc merge"); return false; }
            *new_arr = tmp;
            if (!copy_hook_info(&(*new_arr)[*new_len], oi)) return false;
            (*new_len)++;
        }
    }
    qsort(*new_arr, *new_len, sizeof(HookInfo), cmp_hooks_desc);
    return true;
}

static void er_stealth_direct_patch(uintptr_t addr, uintptr_t cb, uintptr_t* out_orig) {
    if (!k_page_size) k_page_size = (uintptr_t)getpagesize();
    uintptr_t page = addr & ~(k_page_size - 1);
    int cur_prot = PROT_READ;
    {
        MapInfo* maps = redirector_scan_maps("self");
        if (maps) {
            for (size_t i = 0; i < maps->length; i++) {
                if (maps->maps[i].start <= addr && addr < maps->maps[i].end) {
                    cur_prot = maps->maps[i].perms;
                    break;
                }
            }
            redirector_free_maps(maps);
        }
    }
    mprotect((void*)page, k_page_size, PROT_READ | PROT_WRITE);
    uintptr_t orig = *(uintptr_t*)addr;
    if (out_orig) *out_orig = orig;
    *(uintptr_t*)addr = cb;
#if defined(__aarch64__)
    __asm__ volatile(
        "dc cvau, %0\n\t"
        "dsb ish\n\t"
        "ic ivau, %0\n\t"
        "dsb ish\n\t"
        "isb"
        :: "r"(addr) : "memory"
    );
#elif defined(__arm__)
    __builtin___clear_cache((void*)page, (void*)(page + k_page_size));
#elif defined(__x86_64__) || defined(__i386__)
    __asm__ volatile("" ::: "memory");
#elif defined(__riscv)
    __asm__ volatile("fence.i" ::: "memory");
#else
    __builtin___clear_cache((void*)page, (void*)(page + k_page_size));
#endif
    int restore_prot = cur_prot & ~PROT_WRITE;
    if (restore_prot == 0) restore_prot = PROT_READ;
    mprotect((void*)page, k_page_size, restore_prot);
}

static void er_stealth_trampoline_patch(uintptr_t addr, uintptr_t cb, uintptr_t* out_orig) {
#if defined(__aarch64__)
    if (!k_page_size) k_page_size = (uintptr_t)getpagesize();
    uintptr_t page = addr & ~(k_page_size - 1);
    mprotect((void*)page, k_page_size, PROT_READ | PROT_WRITE);
    uintptr_t orig = *(uintptr_t*)addr;
    if (out_orig) *out_orig = orig;
    *(uintptr_t*)addr = cb;
    __asm__ volatile(
        "dc cvau, %0\n\t"
        "dsb ish\n\t"
        "ic ivau, %0\n\t"
        "dsb ish\n\t"
        "isb"
        :: "r"(addr) : "memory"
    );
    mprotect((void*)page, k_page_size, PROT_READ);
#else
    er_stealth_direct_patch(addr, cb, out_orig);
#endif
}

static bool do_hook_addr(HookInfo* hooks, size_t hooks_len, uintptr_t addr, uintptr_t cb, uintptr_t* bk) {
    HookInfo* info = nullptr;
    for (size_t i = 0; i < hooks_len; i++) {
        if (addr < hooks[i].map.start || addr >= hooks[i].map.end) continue;
        info = &hooks[i]; break;
    }
    if (!info) { LOGE("no hook info for %p", (void*)addr); return false; }
    size_t len = info->map.end - info->map.start;

    if (g_stealth_level == ErStealthLevel::DIRECT_PATCH) {
        uintptr_t orig = 0;
        er_stealth_direct_patch(addr, cb, &orig);
        if (bk) *bk = orig;
        ssize_t idx = -1;
        for (size_t i = 0; i < info->entries_len; i++) {
            if (info->entries[i].addr == addr) { idx = (ssize_t)i; break; }
        }
        if (idx != -1) {
            if (cb == info->entries[idx].backup) {
                info->entries_len--;
                if ((size_t)idx < info->entries_len)
                    memmove(&info->entries[idx], &info->entries[idx + 1], (info->entries_len - (size_t)idx) * sizeof(HookEntry));
            }
        } else {
            HookEntry* tmp = (HookEntry*)realloc(info->entries, (info->entries_len + 1) * sizeof(HookEntry));
            if (!tmp) { LOGE("OOM hook entries"); return false; }
            info->entries = tmp;
            info->entries[info->entries_len++] = { addr, orig };
        }
        return true;
    }

    if (g_stealth_level == ErStealthLevel::TRAMPOLINE) {
        uintptr_t orig = 0;
        er_stealth_trampoline_patch(addr, cb, &orig);
        if (bk) *bk = orig;
        HookEntry* tmp = (HookEntry*)realloc(info->entries, (info->entries_len + 1) * sizeof(HookEntry));
        if (!tmp) { LOGE("OOM hook entries trampoline"); return false; }
        info->entries = tmp;
        info->entries[info->entries_len++] = { addr, orig };
        return true;
    }

    if (!info->backup_region && !info->self) {
#ifdef __LP64__
        void* hint = find_backup_hint(len);
        if (!hint) { LOGE("no backup hint for %p", (void*)info->map.start); return false; }
        void* baddr = sys_mmap(hint, len, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
#else
        void* baddr = sys_mmap(nullptr, len, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
        if (baddr == MAP_FAILED) { LOGE("mmap backup fail for %p: %s", (void*)info->map.start, strerror(errno)); return false; }
        void* na = sys_mremap((void*)info->map.start, len, len, MREMAP_FIXED | MREMAP_MAYMOVE, baddr);
        if (na == MAP_FAILED || na != baddr) {
            LOGE("mremap to backup fail");
            sys_munmap(baddr, len); return false;
        }
        na = sys_mmap((void*)info->map.start, len, PROT_READ | PROT_WRITE | info->map.perms, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
        if (na == MAP_FAILED) {
            void* restore = sys_mremap(baddr, len, len, MREMAP_FIXED | MREMAP_MAYMOVE, (void*)info->map.start);
            if (restore == MAP_FAILED || restore != (void*)info->map.start) return false;
            LOGE("mmap original fail"); return false;
        }
        redirector_memcpy((void*)info->map.start, baddr, len);
        info->backup_region = (uintptr_t)baddr;
    }

    if (info->self && !(info->map.perms & PROT_WRITE)) {
        mprotect((void*)info->map.start, len, info->map.perms | PROT_WRITE);
        info->map.perms |= PROT_WRITE;
    }

    uintptr_t* target  = (uintptr_t*)addr;
    uintptr_t  orig    = *target;
    if (orig != cb) {
        *target = cb;
        if (bk) *bk = orig;
        __builtin___clear_cache(page_start(addr), page_end(addr));
    }

    ssize_t idx = -1;
    for (size_t i = 0; i < info->entries_len; i++) {
        if (info->entries[i].addr == addr) { idx = (ssize_t)i; break; }
    }
    if (idx != -1) {
        if (cb == info->entries[idx].backup) {
            info->entries_len--;
            if ((size_t)idx < info->entries_len)
                memmove(&info->entries[idx], &info->entries[idx + 1], (info->entries_len - (size_t)idx) * sizeof(HookEntry));
        }
    } else {
        HookEntry* tmp = (HookEntry*)realloc(info->entries, (info->entries_len + 1) * sizeof(HookEntry));
        if (!tmp) { LOGE("OOM hook entries"); return false; }
        info->entries = tmp;
        info->entries[info->entries_len++] = { addr, orig };
    }

    if (info->entries_len == 0 && info->backup_region && !info->self) {
        void* na = sys_mremap((void*)info->backup_region, len, len, MREMAP_FIXED | MREMAP_MAYMOVE, (void*)info->map.start);
        if (na == MAP_FAILED || (uintptr_t)na != info->map.start) {
            LOGF("mremap restore fail: %p -> %p", (void*)info->backup_region, (void*)info->map.start);
            return false;
        }
        info->backup_region = 0;
    }
    return true;
}

static bool register_hook_internal(dev_t dev, ino_t inode, uintptr_t offset, size_t size,
                                   const char* symbol, bool is_prefix, bool is_got,
                                   void* callback, void** backup) {
    if (!dev || !inode || !symbol || !symbol[0] || !callback) {
        LOGE("register_hook invalid params: dev=%lu inode=%lu sym=%s cb=%p",
             (unsigned long)dev, (unsigned long)inode, symbol ? symbol : "NULL", callback);
        return false;
    }
    std::lock_guard<std::mutex> lk(g_mutex);
    RegisterInfo* tmp = (RegisterInfo*)realloc(g_regs, (g_regs_len + 1) * sizeof(RegisterInfo));
    if (!tmp) { PLOGE("realloc regs"); return false; }
    g_regs = tmp;
    RegisterInfo* n = &g_regs[g_regs_len];
    n->symbol = strdup(symbol);
    if (!n->symbol) { LOGE("strdup symbol fail"); return false; }
    n->dev = dev; n->inode = inode;
    n->offset_start = offset; n->offset_end = offset + size;
    n->is_prefix = is_prefix;
    n->is_got    = is_got;
    n->callback  = callback; n->backup = backup;
    g_regs_len++;
    LOGV("RegisterHook inode=%lu sym=%s%s%s", (unsigned long)inode, n->symbol,
         is_prefix ? " (prefix)" : "", is_got ? " (GOT)" : "");
    return true;
}

bool redirector_register_hook(dev_t dev, ino_t inode, const char* symbol, void* callback, void** backup) {
    return register_hook_internal(dev, inode, 0, UINTPTR_MAX, symbol, false, false, callback, backup);
}

bool redirector_register_hook_by_prefix(dev_t dev, ino_t inode, const char* prefix, void* callback, void** backup) {
    return register_hook_internal(dev, inode, 0, UINTPTR_MAX, prefix, true, false, callback, backup);
}

bool redirector_register_hook_with_offset(dev_t dev, ino_t inode, uintptr_t offset, size_t size,
                                          const char* symbol, void* callback, void** backup) {
    return register_hook_internal(dev, inode, offset, size, symbol, false, false, callback, backup);
}

bool redirector_register_got_hook(dev_t dev, ino_t inode, const char* symbol, void* callback, void** backup) {
    return register_hook_internal(dev, inode, 0, UINTPTR_MAX, symbol, false, true, callback, backup);
}

bool er_set_stealth_level(ErStealthLevel level) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_stealth_level = level;
    return true;
}

static bool do_hooks_for_all(HookInfo* hooks, size_t hooks_len) {
    bool         ok       = true;
    SymAddrBatch* batches = nullptr;
    size_t        batches_len = 0;

    for (size_t i = 0; i < hooks_len; i++) {
        HookInfo* h = &hooks[i];
        for (size_t j = 0; j < g_regs_len; j++) {
            RegisterInfo* reg = &g_regs[j];
            if (!reg->symbol) continue;
            if (h->map.offset != reg->offset_start || !hook_info_matches(h, reg)) continue;

            bool restore = false;
            if (g_hooks) for (size_t k = 0; k < g_hooks_len; k++) {
                for (size_t l = 0; l < g_hooks[k].entries_len; l++) {
                    if (g_hooks[k].entries[l].backup == (uintptr_t)reg->callback) {
                        restore = true;
                        ok = do_hook_addr(hooks, hooks_len, g_hooks[k].entries[l].addr, g_hooks[k].entries[l].backup, nullptr) && ok;
                        break;
                    }
                }
            }

            if (!h->elf.base_addr_ && !restore) elfutil_init(&h->elf, h->map.start);
            if (h->elf.valid_ && !restore) {
                if (h->elf.relro_active_) {
                    uintptr_t page = h->map.start & ~(k_page_size - 1);
                    mprotect((void*)page, h->map.end - page, PROT_READ | PROT_WRITE);
                }
                uintptr_t* addrs = nullptr;
                size_t     addrs_len;
                if (!reg->is_prefix) addrs_len = elfutil_find_plt_addr(&h->elf, reg->symbol, &addrs);
                else                 addrs_len = elfutil_find_plt_addr_by_prefix(&h->elf, reg->symbol, &addrs);
                if (!addrs_len) {
                    LOGE("PLT addr not found: %s in %s", reg->symbol, h->map.path);
                    ok = false; free(addrs); free(reg->symbol); reg->symbol = nullptr;
                    goto next_reg;
                }
                SymAddrBatch* tmp = (SymAddrBatch*)realloc(batches, (batches_len + 1) * sizeof(SymAddrBatch));
                if (!tmp) { PLOGE("realloc batches"); ok = false; free(addrs); free(reg->symbol); reg->symbol = nullptr; goto next_reg; }
                batches = tmp;
                batches[batches_len++] = { (void**)addrs, addrs_len, reg->callback, reg->backup };
            }
            free(reg->symbol); reg->symbol = nullptr;
            continue;
        next_reg:;
        }
    }

    if (ok) for (size_t i = 0; i < batches_len; i++) {
        for (size_t j = 0; j < batches[i].len; j++) {
            ok = do_hook_addr(hooks, hooks_len, (uintptr_t)batches[i].addrs[j],
                              (uintptr_t)batches[i].callback, (uintptr_t*)batches[i].backup) && ok;
        }
    }
    for (size_t i = 0; i < batches_len; i++) free(batches[i].addrs);
    free(batches);
    return ok;
}

bool redirector_commit_hook_manual(MapInfo* maps) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_regs_len) return true;

    size_t    new_len = 0;
    HookInfo* new_arr = build_hook_infos(maps, &new_len);
    if (!new_arr) { LOGE("build_hook_infos fail"); return false; }
    if (!filter_hook_infos(new_arr, &new_len)) {
        LOGE("no hooks matched");
        free_hooks(new_arr, new_len); return false;
    }
    if (g_hooks && !merge_hook_infos(&new_arr, &new_len, g_hooks, g_hooks_len)) {
        LOGE("merge fail");
        free_hooks(new_arr, new_len); return false;
    }
    bool result = do_hooks_for_all(new_arr, new_len);
    if (g_hooks) free_hooks(g_hooks, g_hooks_len);
    g_hooks = new_arr; g_hooks_len = new_len;
    return result;
}

bool redirector_commit_hook() {
    MapInfo* maps = redirector_scan_maps("self");
    if (!maps) { LOGE("scan_maps fail"); return false; }
    bool r = redirector_commit_hook_manual(maps);
    redirector_free_maps(maps);
    return r;
}

bool redirector_unhook(dev_t dev, ino_t inode, const char* symbol) {
    if (!symbol || !symbol[0]) return false;
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_hooks) return false;
    bool restored = false;
    for (size_t i = 0; i < g_hooks_len; i++) {
        HookInfo* h = &g_hooks[i];
        if (h->map.dev != dev || h->map.inode != inode) continue;
        for (size_t j = 0; j < h->entries_len; j++) {
            uintptr_t addr = h->entries[j].addr;
            uintptr_t bk   = h->entries[j].backup;
            if (g_stealth_level == ErStealthLevel::DIRECT_PATCH ||
                g_stealth_level == ErStealthLevel::TRAMPOLINE) {
                er_stealth_direct_patch(addr, bk, nullptr);
            } else {
                uintptr_t page = addr & ~(k_page_size - 1);
                mprotect((void*)page, k_page_size, PROT_READ | PROT_WRITE);
                *(uintptr_t*)addr = bk;
                __builtin___clear_cache(page_start(addr), page_end(addr));
                mprotect((void*)page, k_page_size, PROT_READ);
            }
            restored = true;
        }
        free(h->entries);
        h->entries     = nullptr;
        h->entries_len = 0;
    }
    return restored;
}

bool invalidate_backups() {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_hooks) return true;
    bool res = true;
    for (size_t i = 0; i < g_hooks_len; i++) {
        HookInfo* h = &g_hooks[i];
        if (!h->backup_region) continue;
        for (size_t j = 0; j < h->entries_len; j++)
            h->entries[j].backup = *(uintptr_t*)h->entries[j].addr;
        size_t len = h->map.end - h->map.start;
        void* na = sys_mremap((void*)h->backup_region, len, len, MREMAP_FIXED | MREMAP_MAYMOVE, (void*)h->map.start);
        if (na == MAP_FAILED || (uintptr_t)na != h->map.start) {
            LOGF("mremap invalidate fail: %p -> %p", (void*)h->backup_region, (void*)h->map.start);
            res = false; continue;
        }
        if (mprotect(page_start(h->map.start), len, h->map.perms | PROT_WRITE) == 0) {
            for (size_t j = 0; j < h->entries_len; j++)
                *(uintptr_t*)h->entries[j].addr = h->entries[j].backup;
            mprotect(page_start(h->map.start), len, h->map.perms);
        }
        h->backup_region = 0;
    }
    return res;
}

void redirector_free_resources() {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_hooks) { free_hooks(g_hooks, g_hooks_len); g_hooks = nullptr; g_hooks_len = 0; }
    if (g_regs) {
        for (size_t i = 0; i < g_regs_len; i++) if (g_regs[i].symbol) free(g_regs[i].symbol);
        free(g_regs); g_regs = nullptr; g_regs_len = 0;
    }
    g_sym_cache.clear();
    if (g_cleanup_cb) {
        g_cleanup_cb();
        g_cleanup_cb = nullptr;
    }
    LOGV("redirector freed");
}

void er_set_cleanup_callback(ErCleanupCallback cb) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_cleanup_cb = std::move(cb);
}

bool er_init_for_zygisk() {
    g_zygisk_mode.store(true, std::memory_order_release);
    er_set_stealth_level(ErStealthLevel::DIRECT_PATCH);
    if (!k_page_size) k_page_size = (uintptr_t)getpagesize();
    LOGV("er_init_for_zygisk: stealth=DIRECT_PATCH pagesize=%zu", (size_t)k_page_size);
    return true;
}

static void er_atexit_handler() {
    redirector_free_resources();
}

__attribute__((constructor))
static void er_lib_init() {
    atexit(er_atexit_handler);
    if (!k_page_size) k_page_size = (uintptr_t)getpagesize();
}
