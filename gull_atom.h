#ifndef _GULL_ATOM_H
#define _GULL_ATOM_H 1

#include <stddef.h>
#include <stdint.h>

/**
Gull 使用六个或者三个指针长度的结构储存数据

下面是原子内存的使用方式

TYPE    | FIRST ADDRESS | SECOND | THIRD  | FOURTH         | FIFTH | SIXTH
recycle  0     reference previous next
code     1     reference type     first    previous         current next
tree     1     reference type     base     scanned/cycle    root    pair
base     3     reference value    next
node     3     parent    child    key      color/value      left    right
pair     1     reference type     previous next             key     value
text     1     reference type     length   protect          address size
cache    1     reference type     length   ---              address size
keyword  1     reference type     length   next             address size
list     1     reference type     length   scanned/cycle    address size
state    3     code      cursor   scope    end/branch/index outer   inner
num32    2     reference type     value
num64_32 4/5/6 reference value    ---
num64_64 2     reference type     value
num_big  1     reference type/off first    second           address size
clock_32 1     reference type     second   ---              nano    ---
clock_64 4     reference second   nano
brief_32 7     reference protect/brief
brief_64 2     reference type     protect/brief

原子最小占用 12 个字节（32 位系统一个指针 4 个字节，原子最少三个指针）
如果系统的内存字节大小为 u (u > 0)，那么原子最多只有 (u / 12) 个
(u / 12) < (u / 8)，所以原子的数量不会超过 (u / 8) 个
而原子只能被原子引用，所以任何原子的引用数量不会超过 (u / 8)，即 (u << 3)
如果用一个指针储存原子引用的数据，则不会占满全部的空间，还会遗留三个位
因此这里将原子的第一个指针分成位域 (bit-field)
前三位是原子信息 info 后面的用以储存原子引用数量
*/

typedef struct {
    size_t _atom_1_1: 3;
    size_t _atom_1_2: sizeof(size_t) * 8 - 3;
    size_t _atom_2;
    size_t _atom_3;
    union {
        size_t _atom_4;
        struct {
            size_t _atom_4_1: 1;
            size_t _atom_4_2: 1;
            size_t _atom_4_3: sizeof(size_t) * 8 - 2;
        };
    };
    size_t _atom_5;
    size_t _atom_6;
} atom;

#define _ATOM_MEMBER_ACCESS(index)                                           \
    static inline size_t _atom_get_s##index(atom * _atom) {                  \
        return _atom->_atom_##index;                                         \
    }                                                                        \
    static inline size_t _atom_set_s##index(atom * _atom, size_t _value) {   \
        return _atom->_atom_##index = _value;                                \
    }

#define _ATOM_MEMBER_ACCESS_CAST(index) _ATOM_MEMBER_ACCESS(index)           \
    static inline atom * _atom_get_a##index(atom * _atom) {                  \
        return (atom *) _atom->_atom_##index;                                \
    }                                                                        \
    static inline atom * _atom_set_a##index(atom * _atom, atom * _value) {   \
        return (atom *) (_atom->_atom_##index = (size_t) _value);            \
    }

#define _ATOM_MEMBER_ACCESS_OFFSET(index, off) _ATOM_MEMBER_ACCESS(index)    \
    static inline atom * _atom_get_a##index(atom * _atom) {                  \
        return (atom *) (size_t) (_atom->_atom_##index << off);              \
    }                                                                        \
    static inline atom * _atom_set_a##index(atom * _atom, atom * _value) {   \
        return (atom *) (size_t)                                             \
            (_atom->_atom_##index = (size_t) _value >> off);                 \
    }

_ATOM_MEMBER_ACCESS(1_1)
_ATOM_MEMBER_ACCESS_OFFSET(1_2, 3)
_ATOM_MEMBER_ACCESS_CAST(2)
_ATOM_MEMBER_ACCESS_CAST(3)
_ATOM_MEMBER_ACCESS_CAST(4)
_ATOM_MEMBER_ACCESS(4_1)
_ATOM_MEMBER_ACCESS(4_2)
_ATOM_MEMBER_ACCESS_OFFSET(4_3, 2)
_ATOM_MEMBER_ACCESS_CAST(5)
_ATOM_MEMBER_ACCESS_CAST(6)

static inline void * _atom_get_v5(atom * _atom) {
    return (void *) _atom->_atom_5;
}
static inline void * _atom_set_v5(atom * _atom, void * _value) {
    return (void *) (_atom->_atom_5 = (size_t) _value);
}

#define _atom_get_atom_info _atom_get_s1_1
#define _atom_set_atom_info _atom_set_s1_1
#define _atom_get_atom_reference _atom_get_s1_2
#define _atom_set_atom_reference _atom_set_s1_2
#define _atom_get_atom_type _atom_get_s2
#define _atom_set_atom_type _atom_set_s2

#define _atom_get_recycle_previous _atom_get_a2
#define _atom_set_recycle_previous _atom_set_a2
#define _atom_get_recycle_next _atom_get_a3
#define _atom_set_recycle_next _atom_set_a3

#define _atom_get_code_first _atom_get_a3
#define _atom_set_code_first _atom_set_a3
#define _atom_get_code_previous _atom_get_a4
#define _atom_set_code_previous _atom_set_a4
#define _atom_get_code_current _atom_get_a5
#define _atom_set_code_current _atom_set_a5
#define _atom_get_code_next _atom_get_a6
#define _atom_set_code_next _atom_set_a6

#define _atom_get_container_scanned _atom_get_s4_1
#define _atom_set_container_scanned _atom_set_s4_1
#define _atom_get_container_cycle _atom_get_s4_3
#define _atom_set_container_cycle _atom_set_s4_3
/* tree 和 list 使用 container 数据 */

#define _atom_get_tree_base _atom_get_a3
#define _atom_set_tree_base _atom_set_a3
#define _atom_get_tree_root _atom_get_a5
#define _atom_set_tree_root _atom_set_a5
#define _atom_get_tree_pair _atom_get_a6
#define _atom_set_tree_pair _atom_set_a6

#define _atom_get_base_value _atom_get_a2
#define _atom_set_base_value _atom_set_a2
#define _atom_get_base_next _atom_get_a3
#define _atom_set_base_next _atom_set_a3

#define _atom_get_node_parent _atom_get_a1_2
#define _atom_set_node_parent _atom_set_a1_2
#define _atom_get_node_child _atom_get_a2
#define _atom_set_node_child _atom_set_a2
#define _atom_get_node_key _atom_get_s3
#define _atom_set_node_key _atom_set_s3
#define _atom_get_node_color _atom_get_s4_1
#define _atom_set_node_color _atom_set_s4_1
#define _atom_set_node_black(atom) _atom_set_s4_1(atom, 0)
#define _atom_set_node_red(atom) _atom_set_s4_1(atom, 1)
#define _atom_get_node_value _atom_get_a4_3
#define _atom_set_node_value _atom_set_a4_3
#define _atom_get_node_left _atom_get_a5
#define _atom_set_node_left _atom_set_a5
#define _atom_get_node_right _atom_get_a6
#define _atom_set_node_right _atom_set_a6

#define _atom_get_pair_previous _atom_get_a3
#define _atom_set_pair_previous _atom_set_a3
#define _atom_get_pair_next _atom_get_a4
#define _atom_set_pair_next _atom_set_a4
#define _atom_get_pair_key _atom_get_a5
#define _atom_set_pair_key _atom_set_a5
#define _atom_get_pair_value _atom_get_a6
#define _atom_set_pair_value _atom_set_a6

#define _atom_get_extra_used _atom_get_s3
#define _atom_set_extra_used _atom_set_s3
#define _atom_get_extra_address _atom_get_v5
#define _atom_set_extra_address _atom_set_v5
#define _atom_get_extra_size _atom_get_s6
#define _atom_set_extra_size _atom_set_s6
/* text cache keyword list 使用 extra 区域 */

#define _atom_get_text_protect _atom_get_s4
#define _atom_set_text_protect _atom_set_s4
#define _atom_get_keyword_next _atom_get_a4
#define _atom_set_keyword_next _atom_set_a4

#define _atom_get_state_code _atom_get_a1_2
#define _atom_set_state_code _atom_set_a1_2
#define _atom_get_state_cursor _atom_get_a2
#define _atom_set_state_cursor _atom_set_a2
#define _atom_get_state_scope _atom_get_a3
#define _atom_set_state_scope _atom_set_a3
#define _atom_get_state_end _atom_get_s4_1
#define _atom_set_state_end _atom_set_s4_1
#define _atom_get_state_branch _atom_get_s4_2
#define _atom_set_state_branch _atom_set_s4_2
#define _atom_get_state_index _atom_get_s4_3
#define _atom_set_state_index _atom_set_s4_3
#define _atom_get_state_outer _atom_get_a5
#define _atom_set_state_outer _atom_set_a5
#define _atom_get_state_inner _atom_get_a6
#define _atom_set_state_inner _atom_set_a6

#define _ATOM_INTEGER_ACCESS(m, n)                                           \
    static inline int##m##_t _atom_get_int##m(atom * _atom) {                \
        return * (int##m##_t *) (&_atom->_atom_##n);                         \
    }                                                                        \
    static inline int##m##_t                                                 \
            _atom_set_int##m(atom * _atom, int##m##_t _value) {              \
        return * (int##m##_t *) (&_atom->_atom_##n) = _value;                \
    }                                                                        \
    static inline uint##m##_t _atom_get_uint##m(atom * _atom) {              \
        return * (uint##m##_t *) (&_atom->_atom_##n);                        \
    }                                                                        \
    static inline uint##m##_t                                                \
            _atom_set_uint##m(atom * _atom, uint##m##_t _value) {            \
        return * (uint##m##_t *) (&_atom->_atom_##n) = _value;               \
    }

_ATOM_INTEGER_ACCESS(8, 3)
_ATOM_INTEGER_ACCESS(16, 3)
_ATOM_INTEGER_ACCESS(32, 3)

static inline float _atom_get_float(atom * _atom) {
    return * (float *) (&_atom->_atom_3);
}
static inline float _atom_set_float(atom * _atom, float _value) {
    return * (float *) (&_atom->_atom_3) = _value;
}

#define _ATOM_DOUBLE_ACCESS(b)                                               \
    static inline double _atom_get_double(atom * _atom) {                    \
        return * (double *) (&_atom->_atom_##b);                             \
    }                                                                        \
    static inline double _atom_set_double(atom * _atom, double _value) {     \
        return * (double *) (&_atom->_atom_##b) = _value;                    \
    }

#if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
    _ATOM_INTEGER_ACCESS(64, 3)
    _ATOM_DOUBLE_ACCESS(3)
#else
    _ATOM_INTEGER_ACCESS(64, 2)
    _ATOM_DOUBLE_ACCESS(2)
#endif

#define _atom_get_big_first _atom_get_s3
#define _atom_set_big_first _atom_set_s3
#define _atom_get_big_second _atom_get_s4
#define _atom_set_big_second _atom_set_s4
/* big_first 为整数或者分子的长度，big_second 为小数或者分母的长度 */

static inline size_t _atom_get_big_type(atom * _atom) {
    return _atom->_atom_2 & 0x7f;
}
static inline size_t _atom_get_big_offset(atom * _atom) {
    return _atom->_atom_2 >> 7; /* 如果是大数，会使用 offset 判断数值的阶 */
}
static inline size_t
        _atom_set_big_type_offset(atom * _atom, size_t _type, size_t _off) {
    return _atom->_atom_2 = (_off << 7) | _type;
}

#if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
    #define _atom_get_clock_second (uint64_t) _atom_get_s2
    #define _atom_set_clock_second (uint64_t) _atom_set_s2
    #define _atom_get_clock_nano (uint64_t) _atom_get_s3
    #define _atom_set_clock_nano (uint64_t) _atom_set_s3
#else
    #define _ATOM_CLOCK_ACCESS(name, pointer)                                \
        static inline uint64_t _atom_get_##name(atom * _atom) {              \
            return * (uint64_t *) (&_atom->_atom_##pointer);                 \
        }                                                                    \
        static inline uint64_t                                               \
                _atom_set_##name(atom * _atom, uint64_t _value) {            \
            return * (uint64_t *) (&_atom->_atom_##pointer) = _value;        \
        }
    _ATOM_CLOCK_ACCESS(clock_second, 3)
    _ATOM_CLOCK_ACCESS(clock_nano, 5)
    #undef _ATOM_CLOCK_ACCESS
#endif

#define _ATOM_BRIEF_ACCESS(index)                                            \
    static inline uint8_t _atom_get_brief_protect(atom * _atom) {            \
        return ((uint8_t *) (&_atom->_atom_##index))[0];                     \
    }                                                                        \
    static inline uint8_t                                                    \
            _atom_set_brief_protect(atom * _atom, uint8_t _value) {          \
        return ((uint8_t *) (&_atom->_atom_##index))[0] = _value;            \
    }                                                                        \
    static inline char * _atom_get_brief_address(atom * _atom) {             \
        return ((char *) (&_atom->_atom_##index)) + 1;                       \
    }

#if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
    _ATOM_BRIEF_ACCESS(3)
#else
    _ATOM_BRIEF_ACCESS(2)
#endif

#undef _ATOM_MEMBER_ACCESS
#undef _ATOM_MEMBER_ACCESS_CAST
#undef _ATOM_MEMBER_ACCESS_OFFSET
#undef _ATOM_INTEGER_ACCESS
#undef _ATOM_DOUBLE_ACCESS
#undef _ATOM_BRIEF_ACCESS

#endif
