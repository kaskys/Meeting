//
// Created by abc on 19-12-13.
//

#ifndef UNTITLED5_GLOBAL_H
#define UNTITLED5_GLOBAL_H

struct HeadAttribute;
struct SpaceHead;
struct TypeBit;
struct ControlBit;
struct LargeBit;
struct SizeBit;

#define space_size_round_up(size, n)     (((size > 0) && (n > 0)) ? (((size) + (1 << n) - 1) & ~((1 << n) - 1)) : 0)

#define set_flag(value, flag) (value | flag)
#define get_flag(value, flag) (value & flag)

using attr_tuple = std::tuple<TypeBit, ControlBit, LargeBit, SizeBit>;
/**
 * 声明有依赖关系的对象 ==> 按依赖关系依次声明
 */
extern attr_tuple attribute_bits;
extern HeadAttribute memory_attr;

extern SpaceHead memory_buffer_head;
extern SpaceHead memory_buffer_correlate_use;

#endif //UNTITLED5_GLOBAL_H
