#pragma once
#include <ren/ModelData.hpp>
#include <ren/FileHandle.hpp>
#include <cstdint>
#include <ren/ModelDataDefinitions.h>

/// @file
/// @brief Utilities to convert GTA collision files.

namespace ren
{
    CollisionData* gtacollToCollisionData(const FileInputHandle* _singleCollfile, ren_process_flags _processFlags);
}