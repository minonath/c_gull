#ifndef _GULL_TREE_H
#define _GULL_TREE_H 1

#include "gull_page.h"

static inline atom * _tree_allocate(page * _thread, size_t _type) {
    atom * _tree = _page_allocate_double(_thread);
    _atom_set_info(_tree, _TYPE_INFO_FULL);
    _atom_set_reference(_tree, 0);
    _atom_set_type(_tree, _type);
    _atom_set_tree_base(_tree, 0);
    _atom_set_s4(_tree, 0);
    _atom_set_tree_root(_tree, 0);
    _atom_set_tree_pair(_tree, 0);
    return _tree;
}

static inline atom * _tree_node(
        page * _thread, size_t _key, atom * _parent, size_t _color) {
    atom * _node = _page_allocate_double(_thread);
    _atom_set_info(_node, _TYPE_INFO_MISC);
    _atom_set_node_parent(_node, _parent);
    _atom_set_node_child(_node, 0);
    _atom_set_node_key(_node, _key);
    _atom_set_node_color(_node, _color);
    _atom_set_node_value_s(_node, 0);
    _atom_set_node_left(_node, 0);
    _atom_set_node_right(_node, 0);
    return _node;
}

/* 红黑树节点左旋转 */
/*     parent               parent */
/*       |                    |    */
/*    current               right  */
/*    /    \                /   \  */
/* left  right    ->   current  rr */
/*      /    \          /   \      */
/*    rl     rr      left   rl     */
static inline void _tree_left_rotate(atom * _tree, atom * _current) {
    atom * _right = _atom_get_node_right(_current);
    atom * _parent = _atom_get_node_parent(_current);

    /* 右节点的左节点变为父节点的右节点 */
    if (_atom_set_node_right(_current, _atom_get_node_left(_right))) {
        /* 如果可以，把右节点的左节点的父节点设为当前父节点 */
        _atom_set_node_parent(_atom_get_node_left(_right), _current);
    }

    /* 右节点尝试替换当前父节点 */
    _atom_copy_node_parent(_right, _current);
    if (_parent) {
        if (_atom_get_node_left(_parent) == _current) {
            _atom_set_node_left(_parent, _right);
        } else {
            _atom_set_node_right(_parent, _right);
        }
    } else { /* 父节点不存在，那就是在根节点 */
        _atom_set_tree_root(_tree, _right);
    }

    _atom_set_node_left(_right, _current);
    _atom_set_node_parent(_current, _right);
}

static inline void _tree_right_rotate(atom * _tree, atom * _current) {
    atom * _left = _atom_get_node_left(_current);
    atom * _parent = _atom_get_node_parent(_current);

    if (_atom_set_node_left(_current, _atom_get_node_right(_left))) {
        _atom_set_node_parent(_atom_get_node_right(_left), _current);
    }

    _atom_copy_node_parent(_left, _current);
    if (_parent) {
        if (_atom_get_node_left(_parent) == _current) {
            _atom_set_node_left(_parent, _left);
        } else {
            _atom_set_node_right(_parent, _left);
        }
    } else {
        _atom_set_tree_root(_tree, _left);
    }

    _atom_set_node_right(_left, _current);
    _atom_set_node_parent(_current, _left);
}

/* 插入平衡 */
static void _tree_insert_balance(
        atom * _tree, atom * _current, atom * _parent) {
    atom * _grandparent, * _uncle;

    _loop:
    _grandparent = _atom_get_node_parent(_parent);

    if (_parent == _atom_get_node_left(_grandparent)) {
        _uncle = _atom_get_node_right(_grandparent);
        if (_uncle && _atom_get_node_color(_uncle)) {

            /*  grandparent(black)      ->       grandparent(red)      */
            /*      /         \                   /         \          */
            /* parent(red) uncle(red)   ->  parent(black) uncle(black) */
            /*        \                              \                 */
            /*    current(red)          ->       current(red)          */
            _atom_set_node_black(_parent);
            _atom_set_node_black(_uncle);
            _atom_set_node_red(_grandparent);

            /* 之后以 grandparent 为新目标 */
            _current = _grandparent;
            _parent = _atom_get_node_parent(_current);
            if (_parent && _atom_get_node_color(_parent)) {
                goto _loop;
            } else {
                /* 因为循环可能将根节点变红，所以跳出循环后要将根节点变黑 */
                _atom_set_node_black(_atom_get_tree_root(_tree));
            }
        } else {

            /*  grandparent(black)      ->       grandparent(black) */
            /*      /         \                     /         \     */
            /* parent(red)   None       ->     current(red)   None  */
            /*        \                         /                   */
            /*     current(red)         ->  parent(red)             */
            if (_current == _atom_get_node_right(_parent)) {
                _tree_left_rotate(_tree, _parent);
                _current = _parent;
                _parent = _atom_get_node_parent(_current);
            }

            /*     grandparent(black)   ->         parent(black)          */
            /*         /         \                 /        \             */
            /*   parent(red)    None    ->  current(red) grandparent(red) */
            /*      /                                                     */
            /* current(red)                                               */
            _atom_set_node_black(_parent);
            _atom_set_node_red(_grandparent);
            _tree_right_rotate(_tree, _grandparent);
        }
    } else {
        _uncle = _atom_get_node_left(_grandparent);
        if (_uncle && _atom_get_node_color(_uncle)) {
            _atom_set_node_black(_parent);
            _atom_set_node_black(_uncle);
            _atom_set_node_red(_grandparent);
            _current = _grandparent;
            _parent = _atom_get_node_parent(_current);
            if (_parent && _atom_get_node_color(_parent)) {
                goto _loop;
            } else {
                _atom_set_node_black(_atom_get_tree_root(_tree));
            }
        } else {
            if (_current == _atom_get_node_left(_parent)) {
                _tree_right_rotate(_tree, _parent);
                _current = _parent;
                _parent = _atom_get_node_parent(_current);
            }
            _atom_set_node_black(_parent);
            _atom_set_node_red(_grandparent);
            _tree_left_rotate(_tree, _grandparent);
        }
    }
}

/* 移除平衡，只有删除节点是黑色，且非 _root 时才需要平衡 */
static void _tree_remove_balance(atom * _tree, atom * _current) {
    atom * _parent = _atom_get_node_parent(_current), * _brother;

    _loop:

    /* 当前移除点是父节点的左边 */
    if (_current == _atom_get_node_left(_parent)) {

        /* replace 是黑的，所以兄弟节点必然存在 */
        _brother = _atom_get_node_right(_parent);

        /*        parent(black)         ->             brother(black)     */
        /*            /    \                              /    \          */
        /* replace(black) brother(red)  ->       parent(red) right(black) */
        /*               /    \                       /   \               */
        /*     left(black) right(black) -> replace(black) left(black)     */
        if (_atom_get_node_color(_brother)) {
            _atom_set_node_black(_brother);
            _atom_set_node_red(_parent);

            /* 这一步会改变兄弟节点，所以重新定位，新的兄弟节点必然是黑色 */
            _tree_left_rotate(_tree, _parent);
            _brother = _atom_get_node_right(_parent);
        }

        /*        parent(no care)        ->         brother(inherit)     */
        /*            /    \                               /    \        */
        /* replace(black) brother(black) ->   parent(black) right(black) */
        /*               /    \                        /   \             */
        /*     left(no care) right(red)  -> replace(red) left(no care)   */
        if (_atom_get_node_right(_brother) &&
                _atom_get_node_color(_atom_get_node_right(_brother))) {
            _atom_copy_node_color(_brother, _parent); /* inherit */
            _atom_set_node_black(_parent);
            _atom_set_node_black(_atom_get_node_right(_brother));
            _tree_left_rotate(_tree, _parent);

        /*        parent(no care)        ->          parent(no care)   */
        /*            /    \                            /    \         */
        /* replace(black) brother(black) -> replace(black) left(black) */
        /*               /    \                               \        */
        /*        left(red) right(black) ->               brother(red) */
        /*                                                     \       */
        /*                               ->               right(black) */
        } else if (_atom_get_node_left(_brother) &&
                _atom_get_node_color(_atom_get_node_left(_brother))) {
            _atom_set_node_red(_brother);
            _atom_set_node_black(_atom_get_node_left(_brother));
            _tree_right_rotate(_tree, _brother);
            _brother = _atom_get_node_right(_parent);
            /* 然后进行前面一步相同的操作 */
            _atom_copy_node_color(_brother, _parent);
            _atom_set_node_black(_parent);
            _atom_set_node_black(_atom_get_node_right(_brother));
            _tree_left_rotate(_tree, _parent);

        /*          parent(red)          ->        parent(black)         */
        /*            /    \                          /    \             */
        /* replace(black) brother(black) -> replace(red) brother(red)    */
        /*               /    \                           /   \          */
        /*      left(black) right(black) ->     left(black) right(black) */
        } else if (_atom_get_node_color(_parent)) {
            _atom_set_node_red(_brother);
            _atom_set_node_black(_parent);

        /*        parent(black)          ->       parent(black)           */
        /*            /    \                         /    \               */
        /* replace(black) brother(black) -> replace(red) brother(red)     */
        /*               /    \                           /   \           */
        /*      left(black) right(black) ->      left(black) right(black) */
        } else {
            _atom_set_node_red(_brother);
            _current = _parent;
            _parent = _atom_get_node_parent(_current);

            /* 这一操作与前面都不相同，会导致该分支的黑色节点少一个 */
            /* 因此需要向上进行调整，将该分支的兄弟分支也减少一个黑色节点 */
            if (_parent) goto _loop;
        }

    /* 镜像情况 */
    } else {
        _brother = _atom_get_node_left(_parent);
        if (_atom_get_node_color(_brother)) {
            _atom_set_node_black(_brother);
            _atom_set_node_red(_parent);
            _tree_right_rotate(_tree, _parent);
            _brother = _atom_get_node_left(_parent);
        }
        if (_atom_get_node_left(_brother) &&
                _atom_get_node_color(_atom_get_node_left(_brother))) {
            _atom_copy_node_color(_brother, _parent);
            _atom_set_node_black(_parent);
            _atom_set_node_black(_atom_get_node_left(_brother));
            _tree_right_rotate(_tree, _parent);
        } else if (_atom_get_node_right(_brother) &&
                _atom_get_node_color(_atom_get_node_right(_brother))) {
            _atom_set_node_red(_brother);
            _atom_set_node_black(_atom_get_node_right(_brother));
            _tree_left_rotate(_tree, _brother);
            _brother = _atom_get_node_left(_parent);
            _atom_copy_node_color(_brother, _parent);
            _atom_set_node_black(_parent);
            _atom_set_node_black(_atom_get_node_left(_brother));
            _tree_right_rotate(_tree, _parent);
        } else if (_atom_get_node_color(_parent)) {
            _atom_set_node_red(_brother);
            _atom_set_node_black(_parent);
        } else {
            _atom_set_node_red(_brother);
            _current = _parent;
            _parent = _atom_get_node_parent(_current);
            if (_parent) goto _loop;
        }
    }
}

/* 对红黑树进行插入，返回插入后的键值对，可以改为查询模式 */
static atom * _tree_insert(
        page * _thread, atom * _tree, size_t _key, size_t _force) {

    atom * _node = _atom_get_tree_root(_tree);
    if (_node) {
        _inner:
        if (_atom_get_node_key(_node) == _key) {
            return _node;
        } else if (_atom_get_node_key(_node) > _key) {
            if (_atom_get_node_left(_node)) {
                _node = _atom_get_node_left(_node);
                goto _inner;
            } else if (_force) {
                atom * _create = _tree_node(_thread, _key, _node, 1);
                _atom_set_node_left(_node, _create);
                if (_atom_get_node_color(_node))
                    _tree_insert_balance(_tree, _create, _node);
                return _create;
            } else {
                return 0;
            }
        } else {
            if (_atom_get_node_right(_node)) {
                _node = _atom_get_node_right(_node);
                goto _inner;
            } else if (_force) {
                atom * _create = _tree_node(_thread, _key, _node, 1);
                _atom_set_node_right(_node, _create);
                if (_atom_get_node_color(_node))
                    _tree_insert_balance(_tree, _create, _node);
                return _create;
            } else {
                return 0;
            }
        }
    } else if (_force) {
        return _atom_set_tree_root(_tree, _tree_node(_thread, _key, 0, 0));
    } else {
        return 0;
    }
}

/* 将节点移出树 */
static void _tree_remove(
        page * _thread, atom * _tree, atom * _node) {
    /* 一共七种情况 */
    if (_atom_get_node_left(_node)) {
        if (_atom_get_node_right(_node)) {
            /* 目标节点有两个子节点，找到右子节点的最左子节点进行后续处理 */
            atom * _substitute = _atom_get_node_right(_node);
            while (_atom_get_node_left(_substitute))
                _substitute = _atom_get_node_left(_substitute);

            if (_atom_get_node_right(_substitute)) {
                /* substitute 必然没有左子节点，如果有右子节点，可以简易删除 */
                /* 因为，红黑树左右分支黑色数量一致 */
                /* 如果右子节点是黑的，必然有左子节点，而实际上不存在 */
                /* 所以，右子节点必然是红的 */
                /* 再根据红黑树，红色不能连续，substitute 必然是黑色 */
                /* 同样依据，左右分支黑色数量一致，得知右子节点没有子节点 */
                /* 这时，将右子节点和变黑，可以直接移除 substitute */
                /* target(no care)        -> substitute(inherit) */
                /*   /    \                      /    \          */
                /* left substitute(black) ->  left right(black)  */
                /*            \                                  */
                /*        right(red)      ->                     */
                if (_substitute == _atom_get_node_right(_node)) {
                    atom * _right = _atom_get_node_right(_substitute);
                    _atom_set_node_right(_node, _right);
                    _atom_set_node_parent(_right, _node);
                    _atom_set_node_black(_right);

                /* target(no care)   ->  substitute(inherit) */
                /*   /    \                /    \            */
                /* left right        ->  left right          */
                /*       /                      /            */
                /*      ...          ->        ...           */
                /*       /                      /            */
                /*     parent        ->       parent         */
                /*       /                      /            */
                /* substitute(black) ->      sr(black)       */
                /*       \                                   */
                /*     sr(red)       ->                      */
                } else {
                    atom * _parent = _atom_get_node_parent(_substitute);
                    atom * _right = _atom_get_node_right(_substitute);
                    _atom_set_node_left(_parent, _right);
                    _atom_set_node_parent(_right, _parent);
                    _atom_set_node_black(_right);
                }
            } else {
                /* 此时 substitute 可以为红或者黑，黑色进行再平衡 */
                /* 再平衡不会改变 substitute 和 parent 的联系 */
                if (_atom_get_node_color(_substitute) == 0)
                    _tree_remove_balance(_tree, _substitute);

                /*   target(black)           -> substitute(black) */
                /*      /    \                       /            */
                /* left(red) substitute(red) ->  left(red)        */
                if (_substitute == _atom_get_node_right(_node)) {
                    _atom_set_node_right(_node, 0);

                /* target(no care)   ->  substitute(inherit) */
                /*   /    \                /    \            */
                /* left right        ->  left right          */
                /*       /                      /            */
                /*      ...          ->        ...           */
                /*       /                      /            */
                /*     parent        ->       parent         */
                /*       /                                   */
                /* substitute(red)   ->                      */
                } else {
                    _atom_set_node_left(
                        _atom_get_node_parent(_substitute), 0);
                }
            }

            /* 将目标和替换进行数据交换后删除 */
            _atom_set_node_key(_node, _atom_get_node_key(_substitute));
            _atom_set_node_value_s(_node, _atom_get_node_value_s(_substitute));
            _atom_set_node_child(_node, _atom_get_node_child(_substitute));
            _page_release_double(_thread, _substitute);
        } else {

            /* 目标节点只有左子节点，按前述，子节点必然是红色 */
            /* parent(no care)  -> parent(no care) */
            /*       |          ->       |         */
            /*    target(black) ->    left(black)  */
            /*      /           ->                 */
            /*  left(red)       ->                 */
            atom * _substitute = _atom_get_node_left(_node);
            _atom_set_node_left(_node, 0);

            _atom_set_node_key(_node, _atom_get_node_key(_substitute));
            _atom_set_node_value_s(_node, _atom_get_node_value_s(_substitute));
            _atom_set_node_child(_node, _atom_get_node_child(_substitute));
            _page_release_double(_thread, _substitute);
        }
    } else {
        /* 目标节点只有右子节点，为前一种情形的反转 */
        if (_atom_get_node_right(_node)) {
            atom * _substitute = _atom_get_node_right(_node);
            _atom_set_node_right(_node, 0);

            _atom_set_node_key(_node, _atom_get_node_key(_substitute));
            _atom_set_node_value_s(_node, _atom_get_node_value_s(_substitute));
            _atom_set_node_child(_node, _atom_get_node_child(_substitute));
            _page_release_double(_thread, _substitute);
        } else {
            /* 目标节点没有子节点，无需交换 */
            if (_atom_get_tree_root(_tree) == _node) { /* 或者 _node->_node_parent == 0 */
                _atom_set_tree_root(_tree, 0);
                _page_release_double(_thread, _node);
            } else {
                if (_atom_get_node_color(_node) == 0)
                    _tree_remove_balance(_tree, _node);

                atom * _parent = _atom_get_node_parent(_node);
                if (_node == _atom_get_node_left(_parent)) {
                    _atom_set_node_left(_parent, 0);
                } else {
                    _atom_set_node_right(_parent, 0);
                }
                _page_release_double(_thread, _node);
            }
        }
    }
}

static inline atom * _tree_pair(page * _thread, atom * _key) {
    atom * _pair = _page_allocate_double(_thread);
    _atom_set_info(_pair, _TYPE_INFO_FULL);
    _atom_set_reference(_pair, 0);
    _atom_set_type(_pair, _TYPE_PAIR);
    _atom_set_pair_previous(_pair, 0);
    _atom_set_pair_next(_pair, 0);
    _atom_set_pair_key(_pair, 0);
    _atom_set_pair_value(_pair, 0);
    return _pair;
}

static inline atom * _tree_pair_push(atom * _tree, atom * _atom) {
    return _atom_pair_push((atom **) &_tree->_atom_6, _atom);
    // _atom_set_pair_previous(_atom, 0); /* 插入时初始化 */
    // _atom_set_pair_next(_atom, _atom_get_tree_root(_tree));
    // if (_atom_get_tree_root(_tree)) {
    //     _atom_set_pair_previous(_atom_get_tree_root(_tree), _atom);
    // }
    // return _atom_set_tree_root(_tree, _atom); /* 插入链表头 */
}

static atom * _tree_pair_pop(atom * _tree) {
    return _atom_pair_pop((atom **) &_tree->_atom_6);
    // atom * _atom = _atom_get_tree_root(_tree);
    // if (_atom_set_tree_root(_tree, _atom_get_pair_next(_atom))) {
    //     _atom_set_pair_previous(_atom_get_tree_root(_tree), 0);
    //     _atom_set_pair_next(_atom, 0); /* 清除链表特征 */
    // }
    // return _atom;
}

static inline atom * _tree_pair_append(atom * _tree, atom * _atom) {
    return _atom_pair_append((atom **) &_tree->_atom_6, _atom); /*
    if (_atom_get_tree_root(_tree)) {
        atom * _target = _atom_get_tree_root(_tree);
        while (_atom_get_pair_next(_target)) {
            _target = _atom_get_pair_next(_target);
        }
        _atom_set_pair_next(_target, _atom);
        _atom_set_pair_previous(_atom, _target);
    } else {
        _atom_set_tree_root(_tree, _atom);
        _atom_set_pair_previous(_atom, 0);
    }
    _atom_set_pair_next(_atom, 0);
    return _atom; */
}

static inline atom * _tree_pair_remove(atom * _tree, atom * _atom) {
    return _atom_pair_remove((atom **) &_tree->_atom_6, _atom); /*
    if (_atom_get_tree_root(_tree) == _atom) {
        if (_atom_set_tree_root(_tree, _atom_get_pair_next(_atom))) {
            _atom_set_pair_previous(_atom_get_tree_root(_tree), 0);
            _atom_set_pair_next(_atom, 0);
        }
    } else {
        if (_atom_set_pair_next(
                _atom_get_pair_previous(_atom), _atom_get_pair_next(_atom))) {
            _atom_set_pair_previous(
                _atom_get_pair_next(_atom), _atom_get_pair_previous(_atom));
            _atom_set_pair_next(_atom, 0);
        }
        _atom_set_pair_previous(_atom, 0);
    }
    return _atom; */
}

/* 单链表只会插入链表尾 */
static atom * _tree_base_append(atom * _tree, atom * _atom) {
    if (_atom_get_tree_base(_tree)) {
        atom * _target = _atom_get_tree_base(_tree);
        while (_atom_get_base_next(_target)) {
            _target = _atom_get_base_next(_target);
        }
        _atom_set_base_next(_target, _atom);
        /* _atom_set_base_previous(_atom, _target); // 此行被禁止 */
    } else {
        _atom_set_tree_base(_tree, _atom);
        /* _atom_set_base_previous(_atom, 0); // 此行被禁止 */
    }
    _atom_set_base_next(_atom, 0);
    return _atom;
}

/* tree 可以充当 set 使用 */
static inline void _tree_set_insert(page * _thread, atom * _tree, atom * _value, size_t _count) {
    atom * _node = _tree_insert(_thread, _tree, (size_t) _value, 1);
    _atom_set_node_value_s(_node, _atom_get_node_value_s(_node) + _count);
}

/* 如果不使用 direct 需要调用次数和插入次数相同才会删除 */
static inline size_t _tree_set_remove(page * _thread, atom * _tree, atom * _value, size_t _direct) {
    atom * _node = _tree_insert(_thread, _tree, (size_t) _value, 0);
    if (_node) {
        if (_direct) {
            _tree_remove(_thread, _tree, _node);
            return 0;
        } else {
            if (_atom_get_node_value_s(_node) == 1) {
                _tree_remove(_thread, _tree, _node);
                return 0;
            } else {
                return _atom_set_node_value_s(_node, _atom_get_node_value_s(_node) - 1);
            }
        }
    }
}

static void _tree_node_clean(page * _thread, atom * _node) {
    if (_atom_get_node_left(_node)) {
        _tree_node_clean(_thread, _atom_get_node_left(_node));
    }
    if (_atom_get_node_right(_node)) {
        _tree_node_clean(_thread, _atom_get_node_right(_node));
    }
    if (_atom_get_node_child(_node)) {
        _tree_node_clean(_thread, _atom_get_node_child(_node));
        // _atom_set_node_child(_node, 0);
    }
    _page_release_double(_thread, _node);
} 

static inline void _tree_set_clean(page * _thread, atom * _tree) {
    if (_atom_get_tree_root(_tree)) {
        _tree_node_clean(_thread, _atom_get_tree_root(_tree));
        _atom_set_tree_root(_tree, 0);
    }
}

#endif
