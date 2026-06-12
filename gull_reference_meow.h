#ifndef _GULL_REFERENCE_MEOW_H
#define _GULL_REFERENCE_MEOW_H 1

#include "gull_page.h"

#include <stdio.h>

static inline void _debug_list(char * _string, atom * _link) {
    printf("%s ", _string);
    // 遍历链表，将其中每个原子的循环引用计数修改
    size_t _s = _atom_get_extra_used(_link) / sizeof(size_t);
    atom ** _a = _atom_get_extra_address(_link);
    for (size_t _i = 0; _i < _s; _i++) {
        printf("%p ", _a[_i]);
    }
    printf("\n");
}

static inline _container_traversal _reference_get_traversal_function(
        page * _thread, atom * _atom) {
    if (_atom_get_info(_atom) == _TYPE_INFO_FULL) {
        size_t _index = _atom_get_type(_atom) - _TYPE_CONTAINER_0;

        if (_index < _TYPE_CONTAINER_SIZE) {
            return _thread->_page_main->_container_traversals[_index];
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

static inline _container_count _reference_get_count_function(
        page * _thread, atom * _atom) {
    if (_atom_get_info(_atom) == _TYPE_INFO_FULL) {
        size_t _index = _atom_get_type(_atom) - _TYPE_CONTAINER_0;

        if (_index < _TYPE_CONTAINER_SIZE) {
            return _thread->_page_main->_container_counts[_index];
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

static inline _container_destroy _reference_get_destroy_function(
        page * _thread, atom * _atom) {
    if (_atom_get_info(_atom) == _TYPE_INFO_FULL) {
        size_t _index = _atom_get_type(_atom) - _TYPE_CONTAINER_0;

        if (_index < _TYPE_CONTAINER_SIZE) {
            return _thread->_page_main->_container_destroys[_index];
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

// 此处返回的统计值会多一次
static inline void _reference_count(
        page * _thread, atom * _scan, atom * _atom) {

    _container_count _function =
            _reference_get_count_function(_thread, _atom);

    if (_function) {
        if (_atom_get_container_scan(_atom)) {
            if (_atom_get_container_cache(_atom)) {
                _atom_set_container_count(
                    _atom, _atom_get_container_count(_atom) + 1);
            }
            return;
        }

        _atom_set_container_scan(_atom, 1); // 已遍历标记
        _atom_set_container_count(_atom, 1); // 计数器清空
        _function(_thread, _scan, _atom);
    }
}

// 统计处在正在遍历状态的某个标记的数量
static inline size_t _reference_count_mark(atom * _scan, size_t _mark) {
    size_t _s = _atom_get_extra_used(_scan) / sizeof(size_t);
    atom ** _a = _atom_get_extra_address(_scan);
    size_t _c = 0;
    for (size_t _i = 0; _i < _s; _i ++) {
        if (_atom_get_container_count(_a[_i]) == _mark) {
            if (_atom_get_container_cache(_a[_i])) {
                _c += 1;
            }
        }
    }
    return _c;
}

// 将之前的标记切换为后续标记
static inline size_t _reference_convert_mark(
        atom * _scan, size_t _old, size_t _new) {
    size_t _s = _atom_get_extra_used(_scan) / sizeof(size_t);
    atom ** _a = _atom_get_extra_address(_scan);
    for (size_t _i = 0; _i < _s; _i ++) {
        if (_atom_get_container_count(_a[_i]) == _old) {
            _atom_set_container_count(_a[_i], _new);
        }
    }
    return _new;
}

// 合并同一个节点下的前后标记
static inline size_t _reference_merge_mark(
        atom * _scan, size_t _previous, size_t _current, size_t _target) {
    if (_previous == 0) return _current;
    _target = _target >> 3;
    if (_previous == _target) return _previous;
    if (_current == 0) return _previous;
    if (_current == _target) return _current;
    return _reference_convert_mark(_scan, _previous, _current);
}

static size_t _reference_traversal(
        page * _thread, atom * _scan, atom * _atom, atom * _target) {
    if (_target == _atom) {
        return (size_t) _target >> 3;
    }

    _container_traversal _function =
            _reference_get_traversal_function(_thread, _atom);

    if (_function) {
        if (_atom_get_container_scan(_atom)) {
            if (_atom_get_container_cache(_atom)) {
                return _atom_set_container_count(
                    _atom, (size_t) _atom >> 3);
            }
            return _atom_get_container_count(_atom);
        }

        _cache_push(_thread, _scan, atom *, _atom);
        _atom_set_container_scan(_atom, 1); // 已遍历标记
        _atom_set_container_cache(_atom, 1); // 正在遍历标记
        _atom_set_container_count(_atom, 0); // 计数器清空
        size_t _mark = _function(_thread, _scan, _atom, _target);
        _atom_set_container_cache(_atom, 0);

        if (_mark == 0) return 0; // _mark 为零时 _current 必然为零
        size_t _current = _atom_get_container_count(_atom);

        if (_current) {
            if (_current == _mark) { // 此时分两种情况
                if (_current != (size_t) _target >> 3) {
                    // 都不是目标的情况下，是死循环，死循环返回零
                    if (_reference_count_mark(_scan, _current) == 0) {
                        return _reference_convert_mark(_scan, _current, 0);
                    }
                }
                return _mark;
            } else { // _current 和 _mark 不同，替换所有 _current
                return _reference_convert_mark(_scan, _current, _mark);
            }
        } else { // _current 为零，用 _mark 替换它
            return _atom_set_container_count(_atom, _mark);
        }
    }
    return 0;
}

static atom * _reference_collect( /* 参数里用一个函数指针节省一次获取 */
        page * _thread, _container_traversal _function,
        atom * _atom, size_t _count) {
    atom * _scan = _cache_allocate(_thread, 60000 * sizeof(size_t));

    _cache_push(_thread, _scan, atom *, _atom);
    _atom_set_container_scan(_atom, 1); // 已遍历标记
    _atom_set_container_cache(_atom, 1); // 正在遍历标记
    _atom_set_container_count(_atom, 0); // 计数器清空
    size_t _mark = _function(_thread, _scan, _atom, _atom);
    _atom_set_container_cache(_atom, 0);

    if (_mark) {
        size_t _current = _atom_get_container_count(_atom);
        if (_current) {
            if (_current == _mark) {
                if (_current != (size_t) _atom >> 3) {
                    if (_reference_count_mark(_scan, _current) == 0) {
                        _reference_convert_mark(_scan, _current, 0);
                    }
                }
            } else {
                _reference_convert_mark(_scan, _current, _mark);
            }
        } else {
            _atom_set_container_count(_atom, _mark);
        }
    }

    size_t _s = _atom_get_extra_used(_scan) / sizeof(atom *);
    // if (_s > 1) {
        atom ** _a = _atom_get_extra_address(_scan);
        _mark = (size_t) _atom >> 3;
        if (_count) {
            for (size_t _i = 0; _i < _s; _i++) {
                atom * _t = _a[_i];
                _atom_set_container_scan(_t, 0);
                if (_atom_get_container_count(_t) == _mark) {
                    // _atom_set_container_loop(_t, 1);
                    _atom_set_container_cache(_t, 1);
                }
            }
            _reference_count(_thread, _scan, _atom);
            for (size_t _i = 0; _i < _s; _i ++) {
                _atom_set_container_scan(_a[_i], 0);
            }
        } else {
            for (size_t _i = 0; _i < _s; _i++) {
                atom * _t = _a[_i];
                _atom_set_container_scan(_t, 0);
                if (_atom_get_container_count(_t) == _mark) {
                    _atom_set_container_loop(_t, 1);
                    // _atom_set_container_cache(_t, 1);
                }
            }
        }
    // }
    // _cache_release_direct(_thread, _scan);
    printf("scan %ld\n", _atom_get_extra_used(_scan));
    return _scan;
}

static inline void _reference_increase(page * _thread, atom * _atom) {
    _atom_increase_reference(_atom);

    // 读取对应遍历函数，顺带确定容器是否需要遍历
    _container_traversal _function =
        _reference_get_traversal_function(_thread, _atom);

    if (_function) {
        _cache_release_direct(_thread,
            _reference_collect(_thread, _function, _atom, 0));
    }
}

// 将循环引用全部删除，其它保留后续处理
static inline atom * _reference_clean_loop(page * _thread, atom * _loop) {
    atom * _result = _cache_allocate(_thread, 20 * sizeof(size_t));
    size_t _size = _atom_get_extra_used(_loop) / sizeof(size_t);
    atom ** _list = _atom_get_extra_address_block(_thread, _loop);
    for (size_t _i = 0; _i < _size; _i ++) {
        atom * _temp = _list[_i];
        if (_atom_get_container_cache(_temp))
            _reference_get_destroy_function(
                _thread, _temp)(_thread, _temp, _result);
    }
    for (size_t _i = 0; _i < _size; _i ++) {
        atom * _temp = _list[_i];
        if (_atom_get_container_cache(_temp))
            _reference_get_destroy_function(
                _thread, _temp)(_thread, _temp, 0);
    }
    _atom_set_extra_address_unblock(_thread, _loop);
    return _result;
}

static inline void _reference_recycle(page * _thread, atom * _atom);

static inline void _reference_decrease_simple(page * _thread, atom * _atom) {
    _atom_decrease_reference(_atom);
    if (_atom_get_reference(_atom) == 0) {
        _reference_recycle(_thread, _atom);
    }
}

static inline void _reference_decrease(page * _thread, atom * _atom) {
    _atom_decrease_reference(_atom);

    if (_atom_get_reference(_atom)) {
        _container_traversal _function =
            _reference_get_traversal_function(_thread, _atom);

        if (_function && _atom_get_container_loop(_atom)) {
            atom * _scan = _reference_collect(_thread, _function, _atom, 1);

            size_t _size = _atom_get_extra_used(_scan) / sizeof(size_t);
            atom ** _list = _atom_get_extra_address(_scan);

            size_t _r = 0, _c = -1;
            for (size_t _i = 0; _i < _size; _i ++) {
                atom * _temp = _list[_i];
                if (_atom_get_container_cache(_temp)) {
                    _r += _atom_get_reference(_temp);
                    _c += _atom_get_container_count(_temp);
                }
            }
            printf("%ld %ld \n", _r, _c);
            if (_r == 0) { // 说明不存在循环
                _atom_set_container_loop(_atom, 0);
            } else if (_r == _c) {
                atom * _later = _reference_clean_loop(_thread, _scan);
                size_t _ls = _atom_get_extra_used(_later) / sizeof(size_t);
                if (_ls) {
                    atom ** _ll =
                        _atom_get_extra_address_block(_thread, _later);
                    for (size_t _i = 0; _i < _ls; _i ++) {
                        _reference_decrease(_thread, _ll[_i]);
                    }
                    _atom_set_extra_address_unblock(_thread, _later);
                }
                _cache_release_direct(_thread, _later);
            }
            _cache_release_direct(_thread, _scan);
        }
    } else {
        // printf("release 2 %p \n", _atom);
        _reference_recycle(_thread, _atom);
    }
}

static inline void _reference_destroy(
        page * _thread, atom * _atom, atom * _later) {
    if (_atom_get_info(_atom) == _TYPE_INFO_FULL) {
        size_t _index = _atom_get_type(_atom) - _TYPE_CONTAINER_0;

        if (_index < _TYPE_CONTAINER_SIZE) {
            if (_atom_get_container_cache(_atom) == 0) {
                _cache_push(_thread, _later, atom *, _atom);
            }
        } else {
            _reference_decrease_simple(_thread, _atom);
        }
    } else {
        _reference_decrease_simple(_thread, _atom);
    }
}

static inline void _reference_recycle(page * _thread, atom * _atom) {
    switch(_atom_get_info(_atom)) {
        case _TYPE_INFO_RECYCLE:
            printf("已回收原子再次回收\n");
            raise(SIGABRT);

        case _TYPE_INFO_FULL:
            switch (_atom_get_type(_atom)) {
                // 代码删除
                case _TYPE_LINE:
                case _TYPE_TAIL:
                case _TYPE_EXECUTE:
                case _TYPE_SEQUENCE:
                case _TYPE_SYNCHRONIC:
                case _TYPE_STATIC:
                case _TYPE_QUICK:
                case _TYPE_COMMENT:
                case _TYPE_EXPLAIN:
                case _TYPE_INTERPRET:
                case _TYPE_EXTEND:
                case _TYPE_EXTEND_EXPLAIN:
                case _TYPE_EXTEND_INTERPRET:
                case _TYPE_MARK:
                case _TYPE_JUMP:
                case _TYPE_RETURN:
                case _TYPE_PHRASE:
                case _TYPE_ACCESS:
                case _TYPE_GOTO:
                    if (_atom_get_code_current(_atom)) {
                        _reference_decrease(
                            _thread, _atom_get_code_current(_atom));
                    }
                    if (_atom_get_code_next(_atom)) {
                        _reference_decrease(
                            _thread, _atom_get_code_next(_atom));
                    }
                    _page_release_double(_thread, _atom);
                    break;
                // 短文本删除
                case _TYPE_BRIEF_32:
                    _page_release_double(_thread, _atom);
                    break;
                case _TYPE_TEXT:
                case _TYPE_BYTES:
                case _TYPE_CACHE:
                    _page_release_extra(_thread, _atom);
                    _page_release_double(_thread, _atom);
                    break;
                case _TYPE_CLOCK_32:
                    _page_release_double(_thread, _atom);
                    break;
                case _TYPE_PAIR:
                case _TYPE_STRUCTURE:
                case _TYPE_SHARED:
                    printf("未实现\n");
                    raise(SIGABRT);

                case _TYPE_DECIMAL:
                case _TYPE_DECIMAL_NEGATIVE:
                case _TYPE_FRACTION:
                case _TYPE_FRACTION_NEGATIVE:
                    _page_release_extra(_thread, _atom);
                    _page_release_double(_thread, _atom);
                    break;

                default:
                    {
                        size_t _index =
                            _atom_get_type(_atom) - _TYPE_CONTAINER_0;

                        if (_index < _TYPE_CONTAINER_SIZE) {
                            _thread->_page_main->_container_destroys[_index](
                                _thread, _atom, 0);
                        }
                    }
            }
            break;

        case _TYPE_INFO_HALF:
            _page_release_single(_thread, _atom);
            break;

        case _TYPE_INFO_MISC:
            printf("未知原子回收，请使用专用回收\n");
            raise(SIGABRT);

        #if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
            case _TYPE_INFO_BRIEF_64:
            case _TYPE_INFO_CLOCK_64:
        #else
            case _TYPE_INFO_LONG_32:
            case _TYPE_INFO_UNSIGNED_LONG_32:
            case _TYPE_INFO_DOUBLE_32:
        #endif
            _page_release_single(_thread, _atom);
            break;

        case _TYPE_INFO_PAGE:
            printf("线程应使用专用回收\n");
            raise(SIGABRT);
    }
}

/*
// 数字类
case _TYPE_BIT:
case _TYPE_CHAR:
case _TYPE_SHORT:
case _TYPE_INTEGER:
case _TYPE_LONG_64:
case _TYPE_UNSGINED_CHAR:
case _TYPE_UNSGINED_SHORT:
case _TYPE_UNSGINED_INTEGER:
case _TYPE_UNSGINED_LONG_64:
case _TYPE_FLOAT:
case _TYPE_DOUBLE_64:
*/


#endif
