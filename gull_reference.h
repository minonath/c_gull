#ifndef _GULL_REFERENCE_H
#define _GULL_REFERENCE_H

// #include "gull_reference_meow.h"
                                                                                //
static inline atom * _loop_cache_allocate(page * _thread) {
    atom * _atom = _page_allocate_double(_thread);
    _atom_set_info(_atom, _TYPE_INFO_FULL);
    _atom_set_reference(_atom, 0); // 记录引用数量
    _atom_set_type(_atom, _TYPE_TREE);
    _atom_set_container_cycle(_atom, 0); // 记录循环引用数量
    _atom_set_s4(_atom, 0); // 容器信息
    _atom_set_s5(_atom, 0); // 树结构
    _atom_set_a6(_atom, _cache_allocate(_thread, sizeof(size_t) * 2));
    return _atom;
}

static void _loop_cache_remove(atom * _loop, atom * _target) {
    atom * _cache = _atom_get_a6(_loop);
    size_t _size = _atom_get_extra_used(_cache);
    atom ** _list = _atom_get_extra_address(_cache);
    size_t _i = 0;
    for (; _i < _size; _i ++) {
        if (_list[_i] == _target) {
            _size -= 1;
            break;
        }
    }
    for (size_t _j; _i < _size; _i = _j) {
        _j = _i + 1;
        _list[_i] = _list[_j];
    }
    _atom_set_extra_used(_cache, _size);
}

static inline void _reference_recycle_info(page * _thread, atom * _atom) {
    switch(_atom_get_info(_atom)) {
        case _TYPE_INFO_RECYCLE:
            printf("已回收原子再次回收\n");
            raise(SIGABRT);
            
        case _TYPE_INFO_PAGE:
            printf("线程应使用专用回收\n");
            raise(SIGABRT);

        case _TYPE_INFO_MISC:
            printf("未知原子使用专用回收\n");
            raise(SIGABRT);

        case _TYPE_INFO_HALF:
            _page_release_single(_thread, _atom);
            break;

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

        // default:
        //     _page_release_single(_thread, _atom);
        //     break;
    }
}

static inline void _reference_recycle_type(page * _thread, atom * _atom);

static inline void _reference_recycle_container(
        page * _thread, atom * _atom, size_t _index) {
    // 标准模式删除
    _thread->_container_releases[_index](_thread, _atom, 0);
}

static void _loop_cache_release(page * _thread, atom * _loop, size_t _special);

static void _loop_cache_release_inner(page * _thread, atom * _node, atom * _loop) {
    if (_atom_get_node_left(_node)) {
        _loop_cache_release_inner(_thread, _atom_get_node_left(_node), _loop);
    }
    if (_atom_get_node_right(_node)) {
        _loop_cache_release_inner(_thread, _atom_get_node_right(_node), _loop);
    }

    // 处理当前节点
    atom * _atom = (atom *) _atom_get_node_key(_node);

    _atom_dec_refer(_atom, _atom_get_node_value_s(_node));

    // 判断原子是否为容器类型，不是容器直接测试引用回收
    if (_atom_get_info(_atom) != _TYPE_INFO_FULL) {
        // 节点删除次数
        if (_atom_get_reference(_atom) == 0) {
            _reference_recycle_info(_thread, _atom);
        }
        return;
    }

    // 同上，进一步判断是否为容器类型
    size_t _index = _atom_get_type(_atom) - _TYPE_CONTAINER_0;
    if (_index >= _TYPE_CONTAINER_SIZE) {
        if (_atom_get_reference(_atom) == 0) {
            _reference_recycle_type(_thread, _atom);
        }
        return;
    }

    atom * _loop_a = _atom_get_container_cache(_atom);
    if (_loop_a) {
        if (_loop_a == _loop) {
            if (_atom_get_reference(_atom) != 0) {
                printf("理论上这里应该是零\n");
                raise(SIGABRT);
            }
            _thread->_container_releases[_index](_thread, _atom, 1); // 按照特殊模式删除
            return;
        }

        // 如果原子在循环内，循环引用次数需要同时减少
        _atom_dec_refer(_loop_a, _atom_get_node_value_s(_node));

        // 循环引用数量等于循环数量，说明没有外部引用，应当删除
        if (_atom_get_reference(_loop_a) ==
                _atom_get_container_cycle(_loop_a)) {
            _loop_cache_release(_thread, _loop_a, 1); // 特殊模式
        }
        return;
    }

    if (_atom_get_reference(_atom) == 0) { // 标准删除
        _reference_recycle_container(_thread, _atom, _index);
    }
}

static void _loop_cache_release(page * _thread, atom * _loop, size_t _special) {
    if (_special) {
        _loop_cache_release_inner(_thread, _atom_get_tree_root(_loop), _loop);
        /*atom * _cache = _atom_get_a6(_loop);
        size_t _size = _atom_get_extra_used(_cache) / sizeof(atom *);
        atom ** _list = _atom_get_extra_address_block(_thread, _cache);
        for (size_t _i = 0; _i < _size; _i ++) {
            printf("%p %p\n", _list[_i], _atom_get_container_cache(_list[_i]));
            // _thread->_container_releases[_atom_get_type(_list[_i]) -
            //     _TYPE_CONTAINER_0](_thread, _list[_i], 1); // 按照特殊模式删除
        }
        _atom_set_extra_address_unblock(_thread, _cache);
        */
    }
    _tree_set_clean(_thread, _loop);
    _cache_release_direct(_thread, _atom_get_a6(_loop));
    _page_release_double(_thread, _loop);
}

static void _loop_cache_record_inner(
        page * _thread, atom * _loop, atom * _container, atom * _node) {
    if (_atom_get_node_left(_node)) {
        _loop_cache_record_inner(
            _thread, _loop, _container, _atom_get_node_left(_node));
    }
    if (_atom_get_node_right(_node)) {
        _loop_cache_record_inner(
            _thread, _loop, _container, _atom_get_node_right(_node));
    }
    // if (_atom_get_node_child(_node)) {
    //     _loop_cache_record_inner(
    //         _thread, _loop, _container, _atom_get_node_child(_node));
    // }
    atom * _target = (atom *) _atom_get_node_key(_node);
    _tree_set_insert(_thread, _loop, _target,
        _atom_get_node_value_s(_node));

    // 其实这步可以在 _cache_concat 那里进行的，不过模块化了不好插入
    if (_atom_get_container_cache(_target) == _container) {
        _atom_set_container_cache(_target, _loop);
    }
}

// 将一个循环缓存的内容加入另一个循环缓存
static void _loop_cache_record(
        page * _thread, atom * _loop, atom * _container) {
    // 加入循环
    _cache_concat(_thread, _atom_get_a6(_loop), _atom_get_a6(_container));
    // 添加子项
    _loop_cache_record_inner(
        _thread, _loop, _container, _atom_get_tree_root(_container));
    _loop_cache_release(_thread, _container, 0);
}

// 记录模板，其它容器记录时，可以调用此函数进行简化
static void _tree_set_insert_container(
        page * _thread, atom * _loop, atom * _atom, size_t _size) {

    // 判断原子是否为容器类型
    if (_atom_get_info(_atom) != _TYPE_INFO_FULL) return;
    size_t _index = _atom_get_type(_atom) - _TYPE_CONTAINER_0;
    if (_index >= _TYPE_CONTAINER_SIZE) return;

    _tree_set_insert(_thread, _loop, _atom, _size);
}

// 将一个容器，或者循环缓存，加入循环缓存
static inline atom * _loop_cache_append(
        page * _thread, atom * _loop, atom * _container) {

    _atom_set_container_loop(_container, 0);
    _atom_set_container_cache(_container, _loop);

    // 这里需要调用每个容器类型独特的记录方式
    _thread->_container_records[_atom_get_type(_container) -
        _TYPE_CONTAINER_0](_thread, _loop, _container);
}

static void _loop_cache_analyze(
        page * _thread, atom * _loop, atom * _node) {
    if (_atom_get_node_left(_node)) {
        _loop_cache_analyze(_thread, _loop, _atom_get_node_left(_node));
    }
    if (_atom_get_node_right(_node)) {
        _loop_cache_analyze(_thread, _loop, _atom_get_node_right(_node));
    }

    atom * _current = (atom *) _atom_get_node_key(_node);
    if (_atom_get_container_cache(_current) == _loop) {
        _atom_set_reference(_loop, // 记录引用次数
            _atom_get_reference(_loop) + _atom_get_reference(_current));
        _atom_set_container_cycle(_loop, // 记录循环次数
            _atom_get_container_cycle(_loop) + _atom_get_node_value_s(_node));
    }
}

// 将记录中标记为循环的容器，或者循环缓存，合并为一个循环缓存
static void _reference_combine(page * _thread, atom * _scan) {
    size_t _size = _atom_get_extra_used(_scan) / sizeof(size_t);
    atom ** _list = _atom_get_extra_address_block(_thread, _scan);
    atom * _loop;

    // 先找到已经存在的循环缓存，避免重复创建
    for (size_t _i = 0; _i < _size; _i ++) {
        _loop = _list[_i];
        if (_atom_get_container_loop(_loop) &&
                _atom_get_type(_loop) == _TYPE_TREE) {
            // 记录不需要在这里清除，后续统一清理
            // _atom_set_container_scan(_loop, 0);
            _atom_set_container_loop(_loop, 0);

            for (size_t _n = 0; _n < _i; _n ++) {
                if (_atom_get_container_loop(_list[_n]))
                    _loop_cache_append(_thread, _loop, _list[_n]);
            }
            // 跳过
            for (size_t _n = _i + 1; _n < _size; _n ++) {
                if (_atom_get_container_loop(_list[_n]))
                    _loop_cache_append(_thread, _loop, _list[_n]);
            }
            goto _end;
        }
    }

    // 如果没找到就新建缓存
    _loop = _loop_cache_allocate(_thread);

    for (size_t _n = 0; _n < _size; _n ++) {
        if (_atom_get_container_loop(_list[_n]))
            _loop_cache_append(_thread, _loop, _list[_n]);
    }

    _end: // 缓存分析，解除堵塞
    _atom_set_reference(_loop, 0);
    _atom_set_container_cycle(_loop, 0);
    _loop_cache_analyze(_thread, _loop, _atom_get_tree_root(_loop));
    _atom_set_extra_address_unblock(_thread, _scan);
}

// 遍历模板，其它容器遍历时，可以调用此函数进行简化
static size_t _reference_traversal(
        page * _thread, atom * _scan, atom * _atom, atom * _target) {

    // 判断原子是否为容器类型
    if (_atom_get_info(_atom) != _TYPE_INFO_FULL) return 0;
    size_t _index = _atom_get_type(_atom) - _TYPE_CONTAINER_0;
    if (_index >= _TYPE_CONTAINER_SIZE) return 0;

    // 如果原子原本处在一个循环内，切换遍历对象
    if (_atom_get_container_cache(_atom)) {
        _atom = _atom_get_container_cache(_atom);
        _index = 0;
    }

    // 如果原子已经被遍历，返回之前的结果
    if (_atom_get_container_scan(_atom)) {
        return _atom_get_container_loop(_atom);
    }

    // 如果原子没有遍历，标记为已记录
    _atom_set_container_scan(_atom, 1);
    _cache_push(_thread, _scan, atom *, _atom);

    // 命中目标
    if (_atom == _target) {
        return _atom_set_container_loop(_atom, 1);
    } else { // 继续遍历
        return _thread->_container_traversals[_index](
            _thread, _scan, _atom, _target);
    }
}

static inline void _reference_increase(
        page * _thread, atom * _container, atom * _atom) {
    // 增加原子的引用计数
    _atom_increase_reference(_atom);

    // 判断原子是否为容器类型
    if (_atom_get_info(_atom) != _TYPE_INFO_FULL) return;
    size_t _index = _atom_get_type(_atom) - _TYPE_CONTAINER_0;
    if (_index >= _TYPE_CONTAINER_SIZE) return;

    // 判断引用原子的容器是否在循环内
    atom * _target = _atom_get_container_cache(_container);
    if (_target == 0) {
        _target = _container;
    } else { // 如果在循环内，循环也要添加原子记录
        _tree_set_insert(_thread, _target, _atom, 1);
    }

    // 如果原子原本处在一个循环内，切换遍历对象
    if (_atom_get_container_cache(_atom)) {
        _atom = _atom_get_container_cache(_atom);
        _atom_increase_reference(_atom);
        if (_atom == _target) { // 两个循环相同，自己引用自己
            _atom_set_container_cycle(_atom, _atom_get_container_cycle(_atom) + 1);
            return; // 无需处理其它容器
        }
        _index = 0; // 循环使用第一个索引位置
    }

    // 创建一个遍历记录，将原子记录为已遍历
    atom * _scan = _cache_allocate(_thread, sizeof(size_t) * 2);
    _atom_set_container_scan(_atom, 1);
    _cache_push(_thread, _scan, atom *, _atom);

    // 进行遍历，不同原子类型遍历方法不同
    if (_thread->_container_traversals[_index](
            _thread, _scan, _atom, _target)) {
        // 返回真表示新建循环，需要重新构建循环缓存
        _reference_combine(_thread, _scan);
    }

    // 删除记录
    size_t _size = _atom_get_extra_used(_scan) / sizeof(size_t);
    atom ** _list = _atom_get_extra_address(_scan);
    for (size_t _i = 0; _i < _size; _i ++) {
        _atom_set_container_scan(_list[_i], 0);
    }
    _cache_release_direct(_thread, _scan);
}

static size_t _loop_cache_traversal_inner(page * _thread, atom * _scan,
        atom * _loop, atom * _node, atom * _target) {
    size_t _result = 0;
    if (_atom_get_node_left(_node)) {
        _result |= _loop_cache_traversal_inner(
            _thread, _scan, _loop, _atom_get_node_left(_node), _target);
    }
    if (_atom_get_node_right(_node)) {
        _result |= _loop_cache_traversal_inner(
            _thread, _scan, _loop, _atom_get_node_right(_node), _target);
    }

    atom * _current = (atom *) _atom_get_node_key(_node);

    if (_atom_get_container_cache(_current) == _loop) return _result;

    return _result | _reference_traversal(_thread, _scan, _current, _target);
}

static size_t _loop_cache_traversal(
        page * _thread, atom * _scan, atom * _loop, atom * _target) {
    if (_loop_cache_traversal_inner(
            _thread, _scan, _loop, _atom_get_tree_root(_loop), _target)) {
        return _atom_set_container_loop(_loop, 1);
    }
    return 0;
}

static inline void _reference_decrease(
        page * _thread, atom * _container, atom * _atom) {
    // 记录外部容器所在的循环，原子所在的循环
    atom * _loop_c, * _loop_a;

    // 如果外部容器有循环，记录循环减少一次原子记录后的残留量
    size_t _temp;

    // 减少原子引用
    _atom_decrease_reference(_atom);

    if (_container) {
        _loop_c = _atom_get_container_cache(_container);
        if (_loop_c) { // 外部容器的循环会同步删除一次记录
            _temp = _tree_set_remove(_thread, _loop_c, _atom, 0);
        }
        // 外部容器删除原子的操作，应由外部容器自己操作
    } else {
        _loop_c = 0;
        // _temp = 0;
    }

    // 判断原子是否为容器类型，不是容器直接测试引用回收
    if (_atom_get_info(_atom) != _TYPE_INFO_FULL) {
        if (_atom_get_reference(_atom) == 0) {
            _reference_recycle_info(_thread, _atom);
        }
        return;
    }

    // 同上，进一步判断是否为容器类型
    size_t _index = _atom_get_type(_atom) - _TYPE_CONTAINER_0;
    if (_index >= _TYPE_CONTAINER_SIZE) {
        if (_atom_get_reference(_atom) == 0) {
            _reference_recycle_type(_thread, _atom);
        }
        return;
    }

    // 判断原子是否也在一个循环里
    if ((_loop_a = _atom_get_container_cache(_atom))) {
        // 如果原子在循环内，循环引用次数需要同时减少
        _atom_decrease_reference(_loop_a);

        if (/*_loop_c && */_loop_a == _loop_c) {
            // 两个循环相同，自己引用自己，修改循环的引用记录
            _atom_set_container_cycle(_loop_c,
                _atom_get_container_cycle(_loop_c) - 1);

            if (_temp == 0) {
                // 循环内不再有原子时，删除数组里的记录
                _loop_cache_remove(_loop_a, _atom);

                // 原子没有引用会被彻底删除
                if (_atom_get_reference(_atom) == 0) {
                    _reference_recycle_container(_thread, _atom, _index);
                }
            }
            return;
        }

        // 循环引用数量等于循环数量，说明没有外部引用，应当删除
        if (_atom_get_reference(_loop_a) ==
                _atom_get_container_cycle(_loop_a)) {
            _loop_cache_release(_thread, _loop_a, 1); // 特殊模式
        }
        return;
    }

    if (_atom_get_reference(_atom) == 0) { // 标准删除
        _reference_recycle_container(_thread, _atom, _index);
    }
}

static inline void _reference_recycle_type(page * _thread, atom * _atom) {
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
                    _thread, 0, _atom_get_code_current(_atom));
            }
            if (_atom_get_code_next(_atom)) {
                _reference_decrease(
                    _thread, 0, _atom_get_code_next(_atom));
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
    }
}

#endif
