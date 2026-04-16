#ifndef _GULL_PAGE_H
#define _GULL_PAGE_H 1

/* 如果不是 windows 系统，需要导入 mman 操作内存 */
#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <signal.h>
#include <string.h>

#include "gull_atom.h"
#include "gull_lock.h"
#include "gull_signature.h"
#include "gull_type.h"

/*
Gull 的内存使用页面进行管理
页面分为主页面和拓展 (extend) 页面
主页面的 atom_info 为 _ATOM_INFO_MISC
拓展页面的 atom_info 为 _ATOM_INFO_RECYCLE
每个线程只有一个主页面，主线程的主页面同时充当进程主页面
*/
typedef struct _gull_page page;

typedef size_t (* _container_traversal) (void *, atom *, atom *);
typedef size_t (* _container_destroy) (page *, atom *);

struct _gull_page {
    /* 为了与原子兼容，第一、二个指针预留给 info reference 和 type */
    size_t _page_info: 3;
    size_t _page_reference: sizeof(size_t) * 8 - 3;
    size_t _page_type;
    page * _page_previous; /* 这里与 atom_pair 有相同结构 */
    page * _page_next;

    /* 扩展页面用这个指针记录线程主页面，线程主页面用这个指针记录进程主页面 */
    page * _page_main; /* 进程主页面这里为自身 */
    size_t _page_used; /* 页面使用量 */
    size_t _page_remain; /* 页面剩余量 */
    size_t _page_size; /* 页面尺寸 */

    /* 扩展页面头部只使用前八个指针，后续内容给主页面使用 */

    atom * _page_single; /* 空闲单原子回收区域 */
    size_t _page_single_size;
    atom * _page_double; /* 空闲双原子回收区域 */
    size_t _page_double_size;

    /*
    一个线程只有一个主页面，主页面内存使用尽时，会申请新的内存页面
    新的内存页面叫做 extend 扩展内存；
    页面会分配给原子和原子外内存使用，分配给原子的叫做 page_atom
    分配给原子外内存的叫做 page_extra
    主页面一定是给原子分配使用的，所以 _page_atom 永不为空
    */
    page * _page_atom; /* 原子页面储存区域 */
    size_t _page_atom_size;
    page * _page_extra; /* 额外页面储存区域 */
    size_t _page_extra_size;
    page * _page_extra_merge; /* 准备合并的扩展页面 */
    page * _page_recycle; /* 回收页面储存区域 */
    size_t _page_recycle_size;
    size_t _page_recycle_limit;

    pthread_mutex_t _message_lock;
    pthread_cond_t _message_cond;

    char _thread_signature[_SIGNATURE_SIZE];
    lock _thread_lock; /* 自身锁定锁，进程页面或者共享页面使用 */
    atom _thread_member; /* 线程成员，或者共享页面成员 */
    atomic_size_t _thread_interrupt; /* 打断检测，循环时检测防止锁死 */
    size_t _thread_interrupt_quick; /* 快速检测 */
    size_t _thread_interrupt_check; /* 快速检测最大值 */
    size_t _thread_interrupt_count; /* 快速检测次数达到最大时，进行打断检测 */

    /* 下面属于进程全局的数据 */

    signature _signature_generator; /* 进程签名生成器 */

    _container_traversal _container_traversals[_TYPE_CONTAINER_SIZE];
    _container_destroy _container_destroys[_TYPE_CONTAINER_SIZE];

    page * _threads_shared;
    atom * _threads_by_signature; /* 线程签名储存 */
    atom * _threads_by_name; /* 线程名称储存 */

    size_t _uniform_base[2]; /* 由两个指针组成的，充当原子的地址 */
    page * _uniform_builtins; /* 内嵌容器，静态，所以可以全局访问*/
    atom * _uniform_keywords;
};

#define _PAGE_EXTEND_HEAD_SIZE (sizeof(size_t) * 8)

/*
线程锁以及打断使用了原子变量，原子变量不一定是标准长度
也就是页面头部长度是无法保证的，需要与偶数指针对齐，以保证第一个原子对齐偶数指针
*/
#define _PAGE_THREAD_HEAD_SIZE \
    ((offsetof(page, _signature_generator) + \
    (sizeof(size_t) * 2) - 1) & (sizeof(size_t) * -2))

#define _PAGE_PROCESS_HEAD_SIZE \
    ((sizeof(page) + (sizeof(size_t) * 2) - 1) & (sizeof(size_t) * -2))

static inline void _page_initial_extend(page * _page, page * _thread) {
    _page->_page_info = _TYPE_INFO_PAGE;
    _page->_page_reference = 0;
    _page->_page_type = _TYPE_PAGE;
    /* _page->_page_previous = 0; // 插入链表时会自动处理 */
    /* _page->_page_next = 0; // 插入链表时会自动处理 */
    _page->_page_main = _thread;
    _page->_page_used = 0;
    _page->_page_remain = _thread->_page_size - _PAGE_EXTEND_HEAD_SIZE;
    _page->_page_size = 0;
}

static inline void _page_initial(page * _page, size_t _size, size_t _limit) {
    _page->_page_info = _TYPE_INFO_PAGE;
    _page->_page_reference = 0;
    _page->_page_type = _TYPE_PAGE;
    _page->_page_previous = 0;
    _page->_page_next = 0;
    /* _page->_page_main */
    _page->_page_used = 0;
    /* _page->_page_remain */
    /* _page->_page_size */

    _page->_page_single = 0;
    _page->_page_single_size = 0;
    _page->_page_double = 0;
    _page->_page_double_size = 0;
    _page->_page_atom = _page;
    _page->_page_atom_size = 1;
    _page->_page_extra = 0;
    _page->_page_extra_size = 0;
    _page->_page_extra_merge = 0;
    _page->_page_recycle = 0;
    _page->_page_recycle_size = 0;
    /* _page->_page_recycle_limit */

    if (_size == 0) {
        page * _process = (page *) _limit;
        _page->_page_main = _process;
        _page->_page_remain = _process->_page_size - _PAGE_THREAD_HEAD_SIZE;
        _page->_page_size = _process->_page_size;
        _page->_page_recycle_limit = _process->_page_recycle_limit;
    } else {
        _page->_page_main = _page;
        _page->_page_remain = _size - _PAGE_PROCESS_HEAD_SIZE;
        _page->_page_size = _size;
        _page->_page_recycle_limit = _limit;
    }
}

static inline page * _page_push(page ** _root, page * _page) {
    return (page *) _atom_pair_push((atom **) _root, (atom *) _page);
}

static inline page * _page_pop(page ** _root) {
    return (page *) _atom_pair_pop((atom **) _root);
}

static inline page * _page_append(page ** _root, page * _page) {
    return (page *) _atom_pair_append((atom **) _root, (atom *) _page);
}

static inline page * _page_remove(page ** _root, page * _page) {
    return (page *) _atom_pair_remove((atom **) _root, (atom *) _page);
}

/*
如果原子变量长度和普通变量相同，那么页面的头部尺寸应为 64 400 640（720）
这几个数值都符合 x + 6 * n = 4 ^ m 的模型，比如：
64  + 6 * 160 = 4 ^ 5
400 + 6 * 104 = 4 ^ 5
640 + 6 * 64  = 4 ^ 5
64  + 6 * 672 = 4 ^ 6
640 + 6 * 567 = 4 ^ 6

所以，如果每段内存长度都是 4 的幂，那么页面可以被完整的划分给原子使用
*/
#define _PAGE_SIZE_4K (1 << (2 * 6))
#define _PAGE_SIZE_16K (1 << (2 * 7))
#define _PAGE_SIZE_64K (1 << (2 * 8))
#define _PAGE_SIZE_256K (1 << (2 * 9))
#define _PAGE_SIZE_1M (1 << (2 * 10))

/* 操作系统基础页面一般为 4k / 16k 最大应不超过 1Gb(32-bits) / 4Gb(64-bits) */
#define _PAGE_SIZE_MAX (1L << (2 * (14 + sizeof(size_t) / 4)))

/* 内存分割过大会导致缓存降速，尽量采用小页面，但是也不应小于系统最小分割 */
static size_t _page_size(size_t _desired) {
    /* 获取操作系统内存页面的大小，一般是 4kb / 1Mb / 1Gb 的大小 */
    #if defined(_WIN32) || defined(_WIN64)
        SYSTEM_INFO _info;
        GetSystemInfo(& _info);
        size_t _size = _info.dwPageSize;
    #else
        size_t _size = getpagesize();
    #endif

    /* 操作系统的 _size 是 2 的幂, 按照指数生成的 _desired 一定是 _size 的倍数 */
    size_t _exponent = 0;
    while (_desired > 0) {
        _exponent = _exponent + 1;
        _desired = _desired >> 1;
    }
    _exponent = _exponent / 2;
    _desired = 1 << (_exponent * 2);

    while (_desired < _size) {
        _exponent = _exponent + 1;
        _desired = 1 << (_exponent * 2);
    }

    return _desired > _PAGE_SIZE_MAX ? _PAGE_SIZE_MAX : _desired;
}

static inline page * _page_head(page * _thread, void * _memory) {
    return (page *) (((size_t) _memory) & -_thread->_page_size);
}

/*
申请对齐内存的方法：
先申请预留一段双倍内存，找到其内部对齐的位置
在 win32 下，撤销预留后，按照对齐位置再次申请单倍内存
如果申请失败，允许申请双倍内存而只使用其中一半
预留不会影响真实内存，因为是虚拟内存，但总归有索引上的影响
在 linux/mac 下，申请双倍内存，删掉不对齐内存即可
*/
#if defined(_WIN32) || defined(_WIN64)
#define _page_allocate_align() {                                            \
    size_t _double = _size << 1;                                            \
    _reserve = VirtualAlloc(0, _double, MEM_RESERVE, PAGE_NOACCESS);        \
    if (_reserve == 0) return 0;                                            \
    _align = (void *)(((size_t)_reserve + _size - 1) & -_size);             \
    VirtualFree(_reserve, 0, MEM_RELEASE);                                  \
    _align = VirtualAlloc(                                                  \
        _align, _size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);           \
    if (_align == 0) {                                                      \
        if (GetLastError() != ERROR_INVALID_ADDRESS) return 0;              \
        _reserve = VirtualAlloc(0, _double, MEM_RESERVE, PAGE_NOACCESS);    \
        if (_reserve == 0) return 0;                                        \
        _align = (void *)(((size_t)_reserve + _size - 1) & -_size);         \
        _align = VirtualAlloc(_align, _size, MEM_COMMIT, PAGE_READWRITE);   \
    }                                                                       \
}

#else
#define _page_allocate_align() {                                            \
    _reserve = mmap(0, _size << 1,                                          \
        PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);        \
    if (_reserve == MAP_FAILED) return 0;                                   \
    _align = (void *)(((size_t)_reserve + _size - 1) & -_size);             \
}

/* 这是不对齐的情况下删除，如果对齐可以仅删除一次 */
#define _page_allocate_align_unmap() {                                      \
    size_t _offset = _align - _reserve;                                     \
    munmap(_reserve, _offset);                                              \
    munmap(_align + _size, _size - _offset);                                \
}
#endif

/* 彻底释放系统内存 */
static inline int _page_free(void * _memory, size_t _size) {
    #if defined(_WIN32) || defined(_WIN64)
        MEMORY_BASIC_INFORMATION _mb_info = {};
        VirtualQuery(_memory, &_mb_info, sizeof(MEMORY_BASIC_INFORMATION));
        return VirtualFree(_mb_info.AllocationBase, 0, MEM_RELEASE);
    #else
        /* Linux 会合并 VMA 导致内存释放错误，失败返回 -1 */
        return munmap(_memory, _size) == 0;
    #endif
}

/* 释放内存前，尝试是否可以放入回收区域，仅扩展页面使用 */
static inline void _page_recycle_extend(page * _extend) {
    page * _thread = _extend->_page_main;
    if (_thread->_page_recycle_size >= _thread->_page_recycle_limit) {
        if (_page_free(_extend, _thread->_page_size)) {
            return;
        } else if (_thread->_page_recycle_limit == -1) {
            // printf("系统内存异常溢出\n");
            raise(SIGABRT);
        } else {
            _thread->_page_recycle_limit += 1;
        }
    }
    _page_push(&_thread->_page_recycle, _extend);
    _thread->_page_recycle_size += 1;
}

static page * _page_allocate_extend(page * _thread) {
    if (_thread->_page_recycle_size) {
        _thread->_page_recycle_size -= 1;
        return _page_pop(&_thread->_page_recycle);
    }
    size_t _size = _thread->_page_size;
    void * _reserve, * _align;
    _page_allocate_align();
    _page_initial_extend(_align, _thread);
    #if !defined(_WIN32) && !defined(_WIN64)
        if (_align == _reserve) {
            _reserve = _align + _size;
            _page_initial_extend(_reserve, _thread);
            _page_recycle_extend(_reserve);
        } else {
            _page_allocate_align_unmap();
        }
    #endif
    return _align;
}

static page * _page_allocate_thread(page * _process) {
    size_t _size = _process->_page_size;
    void * _reserve, * _align;
    _page_allocate_align();
    _page_initial(_align, 0, (size_t) _process);
    #if !defined(_WIN32) && !defined(_WIN64)
        if (_align == _reserve) {
            _reserve = _align + _size;
            _page_initial_extend(_reserve, _align);
            _page_recycle_extend(_reserve);
        } else {
            _page_allocate_align_unmap();
        }
    #endif
    return _align;
}

static page * _page_allocate_process(size_t _size, size_t _limit) {
    void * _reserve, * _align;
    _page_allocate_align();
    _page_initial(_align, _size, _limit);
    #if !defined(_WIN32) && !defined(_WIN64)
        if (_align == _reserve) {
            _reserve = _align + _size;
            _page_initial_extend(_reserve, _align);
            _page_recycle_extend(_reserve);
        } else {
            _page_allocate_align_unmap();
        }
    #endif
    return _align;
}

static atom * _page_allocate_double(page * _thread) {
    /* 优先提取空闲的双原子 */
    if (_thread->_page_double_size) {
        _thread->_page_double_size -= 1;
        _page_head(
            _thread, _thread->_page_double)->_page_used += sizeof(atom);
        return _atom_recycle_pop(&_thread->_page_double);
    }

    /* 其次提取页面内未使用的原子 */
    void * _result;
    page * _current = _thread->_page_atom;

    if (_current->_page_remain >= sizeof(atom)) goto _remain;

    /* 页面不足则切换为下一个页面 */
    for (int _i = _thread->_page_atom_size; _i > 1; _i --) {
        _page_append(
            &_thread->_page_atom, _page_pop(&_thread->_page_atom));
        _current = _thread->_page_atom;

        if (_current->_page_remain >= sizeof(atom)) goto _remain;
    }

    /* 所有页面都没有空闲，创建新页面 */
    _current = _page_allocate_extend(_thread);

    _page_push(&_thread->_page_atom, _current);
    _thread->_page_atom_size += 1;

    _remain:
    _result =
        (void *) _current + (_thread->_page_size - _current->_page_remain);
    _current->_page_remain -= sizeof(atom);
    _current->_page_used += sizeof(atom);
    return _result;
}

static atom * _page_allocate_single(page * _thread) {
    /* 优先提取空闲的单原子 */
    if (_thread->_page_single_size) {
        _thread->_page_single_size -= 1;
        _page_head(
            _thread, _thread->_page_single)->_page_used += sizeof(atom) >> 1;
        return _atom_recycle_pop(&_thread->_page_single);
    }

    void * _result;

    /* 其次提取空闲的双原子 */
    if (_thread->_page_double_size) {
        _thread->_page_double_size -= 1;
        _page_head(
            _thread, _thread->_page_double)->_page_used += sizeof(atom) >> 1;
        _result = _atom_recycle_pop(&_thread->_page_double);
        goto _split;
    }

    /* 然后提取页面内未使用的原子 */
    page * _current = _thread->_page_atom;

    if (_current->_page_remain >= sizeof(atom)) goto _remain;

    /* 页面不足则切换为下一个页面 */
    for (int _i = _thread->_page_atom_size; _i > 1; _i --) {
        _page_append(
            &_thread->_page_atom, _page_pop(&_thread->_page_atom));
        _current = _thread->_page_atom;

        if (_current->_page_remain >= sizeof(atom)) goto _remain;
    }

    /* 所有页面都没有空闲，创建新页面 */
    _current = _page_allocate_extend(_thread);

    _page_push(&_thread->_page_atom, _current);
    _thread->_page_atom_size += 1;

    _remain:
    _result =
        (void *) _current + (_thread->_page_size - _current->_page_remain);
    _current->_page_remain -= sizeof(atom);
    _current->_page_used += sizeof(atom) >> 1;

    _split:
    /* 这里需要部分初始化，确保是回收状态 */
    _thread->_page_single_size += 1;
    _atom_set_info(
        _atom_recycle_push(&_thread->_page_single, _atom_pair(_result)),
        _TYPE_INFO_RECYCLE);
    return _result;
}

#define _page_extra_used(memory) (*memory & 1)

#define _page_extra_atom(memory) (atom *) (*memory & -2)

#define _page_extra_bind(memory, atom) *memory = ((size_t) atom) | 1

/* 重新实现 memcpy 确保拷贝方向 */
static void _page_extra_copy(void * _to, void * _from, size_t _size) {
    if (_from > _to)
        for (int _i = 0; _i < _size; _i ++)
            ((char *) _to)[_i] = ((char *) _from)[_i];
}

/* 对当前提供给额外内存使用对页面进行整理 */
static void _page_extra_sort(page * _page, size_t _size) {
    size_t * _to = ((void *) _page) + _PAGE_EXTEND_HEAD_SIZE;
    size_t * _from = _to;
    void * _limit = ((void *) _page) + _size - _page->_page_remain;

    while ((void *) _from < _limit) {
        if (_page_extra_used(_from)) {
            atom * _extra = _page_extra_atom(_from);
            _atom_set_extra_address(_extra, _to + 1);
            size_t _full = _atom_get_extra_size(_extra) + sizeof(size_t);
            _page_extra_copy(_to, _from, _full);
            _to = ((void *) _to) + _full;
            _from = ((void *) _from) + _full;
        } else {
            _from = ((void *) _from) + *_from;
        }
    }

    _page->_page_remain = _size - _PAGE_EXTEND_HEAD_SIZE - _page->_page_used;
}

/*
合并两个拓展页面
因为这里不进行检测，所以需要保证两个页面合并不会溢出
返回被合并页面，用户可以决定是删除它，还是当作新页面使用
*/
static size_t _page_extra_merge_two(page * _thread, page * _page) {
    page * _merge = _thread->_page_extra_merge;
    size_t _size = _thread->_page_size;
    size_t * _to = ((void *) _merge) + _PAGE_EXTEND_HEAD_SIZE;
    size_t * _from = _to;
    void * _limit = ((void *) _merge) + _size - _merge->_page_remain;

    while ((void *) _from < _limit) {
        if (_page_extra_used(_from)) {
            atom * _extra = _page_extra_atom(_from);
            _atom_set_extra_address(_extra, _to + 1);
            size_t _full = _atom_get_extra_size(_extra) + sizeof(size_t);
            _page_extra_copy(_to, _from, _full);
            _to = ((void *) _to) + _full;
            _from = ((void *) _from) + _full;
        } else {
            _from = ((void *) _from) + *_from;
        }
    }

    _from = (void *) _page + _PAGE_EXTEND_HEAD_SIZE;
    _limit = ((void *) _page) + _size - _page->_page_remain;

    while ((void *) _from < _limit) {
        if (_page_extra_used(_from)) {
            atom * _extra = _page_extra_atom(_from);
            _atom_set_extra_address(_extra, _to + 1);
            size_t _full = _atom_get_extra_size(_extra) + sizeof(size_t);
            memcpy(_to, _from, _full); /* 注意这里用 memcpy */
            _to = ((void *) _to) + _full;
            _from = ((void *) _from) + _full;
        } else {
            _from = ((void *) _from) + *_from;
        }
    }

    _merge->_page_used = _merge->_page_used + _page->_page_used;
    _merge->_page_remain =
        _size - _PAGE_EXTEND_HEAD_SIZE - _merge->_page_used;

    _page->_page_used = 0;
    _page->_page_remain = _size - _PAGE_EXTEND_HEAD_SIZE;

    return _merge->_page_used;
}

static size_t * _page_allocate_extra(page * _thread, size_t _length) {
    void * _extra;
    page * _current;

    /* 额外内存必然存放在扩展页面里，所以这里的限制是相同的 */
    size_t _limit = _thread->_page_size - _PAGE_EXTEND_HEAD_SIZE;

    _length = (_length + sizeof(size_t) * 2 - 1) & -sizeof(size_t);

    /* 如果页面的最大容量不支持分配，那么申请系统内存，然后绑定 */
    if (_limit < _length) {
        #if defined(_WIN32) || defined(_WIN64)
            _extra = VirtualAlloc(0, _length, MEM_RESERVE, PAGE_NOACCESS);
        #else
            _extra = mmap(0, _length,
                PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        #endif
        goto _return;
    }

    if (_thread->_page_extra_size) {
        _current = _thread->_page_extra;

        /* 如果页面容量足够，直接分配 */
        if (_current->_page_remain >= _length) goto _remain;

        /* 如果页面容量经过整理后足够，整理后分配 */
        if (_limit - _current->_page_used >= _length) {
            _page_extra_sort(_current, _thread->_page_size);
            goto _remain;
        }

        for (int _i = _thread->_page_extra_size; _i > 1; _i --) {
            _page_append(
                &_thread->_page_extra, _page_pop(&_thread->_page_extra));
            _current = _thread->_page_extra;

            if (_current->_page_remain >= _length) goto _remain;

            if (_limit - _current->_page_used >= _length) {
                _page_extra_sort(_current, _thread->_page_size);
                goto _remain;
            }
        }
    }

    /* 所有页面都不足够的时候，申请新的页面 */
    _current = _page_allocate_extend(_thread);
    _page_push(&_thread->_page_extra, _current);
    _thread->_page_extra_size += 1;
    if (_thread->_page_extra_merge == 0) {
        _thread->_page_extra_merge = _current;
    }

    /* 在当前页面进行分配 */
    _remain:
    _extra =
        ((void *) _current) + _thread->_page_size - _current->_page_remain;
    _current->_page_remain -= _length;
    _current->_page_used += _length;

    if (_thread->_page_extra_merge == _current &&
            _current->_page_used >= (_limit >> 1)) {
        _thread->_page_extra_merge = 0;
    }

    /* 将分配的内存的长度记录后返回 */
    _return:
    *((size_t *) _extra) = _length;
    return _extra;
}

static inline atom * _page_allocate_bind(
        page * _thread, size_t _type, size_t _size) {
    atom * _atom = _page_allocate_double(_thread);
    size_t * _extra = _page_allocate_extra(_thread, _size);
    _atom_set_info(_atom, _TYPE_INFO_FULL);
    _atom_set_reference(_atom, 0);
    _atom_set_type(_atom, _type);
    // _atom_set_s3(_atom, 0);
    // _atom_set_s4(_atom, 0);
    _atom_set_extra_address(_atom, _extra + 1);
    _atom_set_extra_size(_atom, *_extra - sizeof(size_t));
    _page_extra_bind(_extra, _atom); // *_extra = ((size_t) _atom) | 1;
    return _atom;
}

static void _page_release_double(page * _thread, atom * _atom) {
    _atom_set_info(_atom, _TYPE_INFO_RECYCLE);

    page * _page = _page_head(_thread, _atom);
    _page->_page_used -= sizeof(atom);

    if (_page == _thread) {
        _atom_recycle_push(&_page->_page_double, _atom);
        _page->_page_double_size += 1;

    } else {
        page * _thread = _page->_page_main;
        _atom_recycle_push(&_thread->_page_double, _atom);
        _thread->_page_double_size += 1;

        if (_page->_page_used) return;

        /* 只有普通页面会尝试回收 */
        void * _a = ((void *) _page) + _PAGE_EXTEND_HEAD_SIZE, * _b =
            ((void *) _page) + _thread->_page_size - _page->_page_remain;

        _page->_page_remain = _thread->_page_size - _PAGE_EXTEND_HEAD_SIZE;
        _thread->_page_double_size -= (_b - _a) / sizeof(atom);

        for (; _a < _b; _a += sizeof(atom)) {
            _atom_recycle_remove(&_thread->_page_double, _a);
        }

        _thread->_page_atom_size -= 1;
        _page_remove(&_thread->_page_atom, _page);
        _page_recycle_extend(_page);
    }
}

static void _page_release_single(page * _thread, atom * _atom) {
    _atom_set_info(_atom, _TYPE_INFO_RECYCLE);

    atom * _pair = _atom_pair(_atom);
    page * _page = _page_head(_thread, _atom);
    _page->_page_used -= sizeof(atom) >> 1;

    if (_page == _thread) {
        /* 成对原子正在使用 */
        if (_atom_get_info(_pair) != _TYPE_INFO_RECYCLE) {
            _atom_recycle_push(&_page->_page_single, _atom);
            _page->_page_single_size += 1;
        } else {
            _atom_recycle_remove(&_page->_page_single, _pair);
            _atom_recycle_push(
                &_page->_page_double, _atom < _pair ? _atom : _pair);
            _page->_page_single_size -= 1;
            _page->_page_double_size += 1;
        }
    } else {
        page * _thread = _page->_page_main;
        if (_atom_get_info(_pair) != _TYPE_INFO_RECYCLE) {
            _atom_recycle_push(&_thread->_page_single, _atom);
            _thread->_page_single_size += 1;
        } else {
            _atom_recycle_remove(&_thread->_page_single, _pair);
            _atom_recycle_push(
                &_thread->_page_double, _atom < _pair ? _atom : _pair);
            _thread->_page_single_size -= 1;
            _thread->_page_double_size += 1;
        }

        if (_page->_page_used) return;

        void * _a = ((void *) _page) + _PAGE_EXTEND_HEAD_SIZE, * _b =
            ((void *) _page) + _thread->_page_size - _page->_page_remain;

        _page->_page_remain = _thread->_page_size - _PAGE_EXTEND_HEAD_SIZE;
        _thread->_page_double_size -= (_b - _a) / sizeof(atom);

        for (; _a < _b; _a += sizeof(atom)) {
            _atom_recycle_remove(&_thread->_page_double, _a);
        }

        _thread->_page_atom_size -= 1;
        _page_remove(&_thread->_page_atom, _page);
        _page_recycle_extend(_page);
    }
}

/*
对当前额外页面进行合并测试
如果当前页面为空，直接回收
如果当前页面正在使用，且使用量不足一半，进行合并尝试
*/
static void _page_extra_test(page * _thread, page * _page) {
    if (_page->_page_used) {
        if (_thread->_page_extra_merge == _page) {
            return;
        }
        size_t _limit = (_thread->_page_size - _PAGE_EXTEND_HEAD_SIZE) >> 1;
        if (_page->_page_used < _limit) {
            if (_thread->_page_extra_merge) {
                if (_page_extra_merge_two(_thread, _page) >= _limit) {
                    _thread->_page_extra_merge = 0;
                }
                goto _recycle;
            } else {
                _thread->_page_extra_merge = _page;
            }
        }
        return;
    } else {
        if (_thread->_page_extra_merge == _page) {
            _thread->_page_extra_merge = 0;
        }
    }

    _recycle:
    _page_remove(&_thread->_page_extra, _page);
    _thread->_page_extra_size -= 1;
    _page->_page_remain = _thread->_page_size - _PAGE_EXTEND_HEAD_SIZE;
    _page_recycle_extend(_page);
}

static void _page_release_extra(page * _thread, atom * _atom) {
    size_t * _target = _atom_get_extra_address(_atom) - sizeof(size_t);
    *_target = _atom_get_extra_size(_atom) + sizeof(size_t);
    if (*_target > _thread->_page_size - _PAGE_EXTEND_HEAD_SIZE) {
        _page_free(_target, *_target);
    } else {
        page * _page = _page_head(_thread, _target);
        _page->_page_used -= *_target;
        _page_extra_test(_thread, _page);
    }
}

static size_t _multiples3(size_t n) {
    int odd = 0;
    int even = 0;
    if (n < 2) return n;
    while (n) {
        if (n & 1) odd += 1;
        if (n & 2) even += 1;
        n = n >> 2;
    }
    if (odd > even) {
        return _multiples3(odd - even);
    } else {
        return _multiples3(even - odd);
    }
}

static page * _page_is_legal(page * _thread, page * _page) {
    page * _target = _thread->_page_atom;
    while (_target) {
        if (_target == _page) {
            return _page;
        }
        _target = _target->_page_next;
    }
    return 0;
}

/* 判断一个内存是否是线程内的合法原子 */
static atom * _atom_is_legal(page * _thread, void * _memory) {
    /* 当前内存是线程自身 */
    if (_memory == _thread) return _memory;

    /* 找到地址所在页面 */
    page * _head = _page_is_legal(_thread, _page_head(_thread, _memory));

    if (_head == 0) return 0;

    size_t _start;
    if (_thread == _head) { /* 主页面 */
        if (_memory == &_head->_thread_member) {
            return (atom *) _head; /* 是线程自身 */
        }
        if (_head->_page_main == _head) { /* 进程主页面 */
            if (_memory == &_head->_uniform_base) {
                return _memory; /* 是 uniform_base */
            }
            _start = (size_t) _head + _PAGE_PROCESS_HEAD_SIZE;
        } else {
            _start = (size_t) _head + _PAGE_THREAD_HEAD_SIZE;
        }
    } else { /* 扩展页面 */
        _start = (size_t) _head + _PAGE_EXTEND_HEAD_SIZE;
    }

    size_t _end = (size_t) _head +
        _thread->_page_size - _head->_page_remain;

    if ((size_t) _memory < _start || (size_t) _memory >= _end) return 0;

    size_t _offset = (size_t) _memory - _start;

    /* 原子地址偏移量需为 12（32-bit）或者 24（64-bit）的倍数 */
    #if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
        /* 24 的倍数就是 8 和 3 的倍数，确定最后三位为零 */
        if (_offset & 0b111 || _multiples3(_offset)) return 0;
    #else
        /* 12 的倍数就是 4 和 3 的倍数，确定最后两位为零 */
        if (_offset & 0b11 || _multiples3(_offset)) return 0;
    #endif

    /* 范围合法但是要计算原子内容是否合法 */
    size_t _info = _atom_get_info(_memory);
    if (_info == _TYPE_INFO_RECYCLE || _info == _TYPE_INFO_MISC) {
        return 0;
    }
    atom * _pair = _atom_pair(_memory);
    if (_pair < (atom *) _memory) { /* 必须是单原子 */
        if (_info == _TYPE_INFO_FULL) {
            return 0;
        }
        _info = _atom_get_info(_pair);
        if (_info == _TYPE_INFO_FULL || _info == _TYPE_INFO_MISC) {
            return 0;
        }
    }
    return _memory;
}

#endif
