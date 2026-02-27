#ifndef MODELDATADEFINITIONS_H
#define MODELDATADEFINITIONS_H

#include <stdint.h>

#ifdef __cplusplus
#define ENUM_OF_TYPE_UINT32 : uint32_t
#else
#define ENUM_OF_TYPE_UINT32
#endif

/// Flags to modify conversion behaviour. 
enum ren_process_flags ENUM_OF_TYPE_UINT32
{
    REN_NOFLAG =                            0x0,

    // 3D Model Processing Flags
    REN_GEOMETRY_SWAP_YZ =                  0x1,
    REN_GEOMETRY_FLIP_TRIANGLES =           0x2,
    REN_GEOMETRY_FLIP_NORMALS =             0x4,
    REN_GEOMETRY_PRUNE_TRANSFORMS =         0x8,

    // Collision Model Processing Flags
    REN_COLLISION_SWAP_YZ =                 0x1,
    REN_COLLISION_FLIP_TRIANGLES =          0x2
};

#endif