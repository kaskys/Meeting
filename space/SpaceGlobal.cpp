//
// Created by abc on 19-12-13.
//
#include "manager/MemoryReader.h"

attr_tuple attribute_bits = std::make_tuple(TypeBit(), ControlBit(), LargeBit(), SizeBit());    /* NOLINT */

HeadAttribute memory_attr = HeadAttribute(4, 1, 1, 10 + 10 + 6, 3);     /* NOLINT */

SpaceHead memory_buffer_head = convertBufSize({SPACE_TYPE_INSIDE, 0, 1, sizeof(MemoryBuffer) + sizeof(MemoryBufferNote) + sizeof(MemoryBufferUseInfo)});        /* NOLINT */
/*
 * 两个MemoryBufferUseInfo（一个是DynamicMemory、另一个是FixedMemory）
 */
SpaceHead memory_buffer_correlate_use = convertBufSize({SPACE_TYPE_INSIDE, 0, 1, sizeof(MemoryCorrelate) + sizeof(MemoryBufferUseInfo) + sizeof(MemoryBufferUseInfo)});     /* NOLINT */

