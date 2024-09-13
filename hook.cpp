#include <map>
#include <new>
#include <execinfo.h>
#include <string>
#include <utility>
#include <cxxabi.h>

// https://github.com/archibate/mallocvis

namespace zstl {
    namespace _malloc_hook {
        struct alloc_info {
            std::size_t size;
            void *caller;
        };

        static std::string addr_to_symbol(void* addr) {
            char** strings = backtrace_symbols(&addr, 1);
            if (strings == nullptr) {
                return "";
            }
            
            std::string ret = strings[0];
            free(strings);
            
            std::size_t pos = ret.find('(');
            if (pos == std::string::npos) {
                return ret;
            }
            auto pos2 = ret.find('+', pos);
            if (pos2 == std::string::npos) {
                return ret.substr(pos + 1, ret.size() - pos - 2);
            }

            ret = ret.substr(pos + 1, pos2 - pos - 1);
            char *demangle = abi::__cxa_demangle(ret.data(), nullptr, nullptr, nullptr);
            if (demangle) {
                ret= demangle;
                free(demangle);
            }
            return ret;
        }

        std::map<void*, alloc_info> allocated;
        bool enable;
    
        struct malloc_hook_init {
            malloc_hook_init() {
                // before main
                enable = true;
            }

            ~malloc_hook_init() {
                // after main
                enable = false;

                if (!allocated.empty()) {
                    printf("memory leak!\n");
                    for (auto [ptr, info] : allocated) {
                        printf("at ptr = %p, size = %zd, caller = %s\n", ptr, info.size, addr_to_symbol(info.caller).c_str());
                    }                   
                }
            }
        };
    }
}

void *operator new(std::size_t size) {
    void* caller = __builtin_return_address(0);
    bool was_enable = std::exchange(zstl::_malloc_hook::enable, false);
    void* ptr = malloc(size);
    if (was_enable && ptr) {
        zstl::_malloc_hook::allocated.insert({ptr, {size, caller}});
        printf("new ptr = %p, size = %zd\n", ptr, size);
    }
    zstl::_malloc_hook::enable = was_enable;
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void operator delete(void *ptr) noexcept {
    bool was_enable = std::exchange(zstl::_malloc_hook::enable, false);
    if (was_enable && ptr) {
        printf("delete ptr = %p\n", ptr);
        zstl::_malloc_hook::allocated.erase(ptr);
    }
    free(ptr);
    zstl::_malloc_hook::enable = was_enable;
}

zstl::_malloc_hook::malloc_hook_init __malloc_hook;
