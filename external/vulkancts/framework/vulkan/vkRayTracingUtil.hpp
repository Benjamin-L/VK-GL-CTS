#ifndef _VKRAYTRACINGUTIL_HPP
#define _VKRAYTRACINGUTIL_HPP
/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2020 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Vulkan ray tracing utility.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkMemUtil.hpp"
#include "vkBufferWithMemory.hpp"

#include "deFloat16.h"

#include "tcuVector.hpp"
#include "tcuVectorType.hpp"

#include <vector>
#include <limits>

namespace vk
{

#ifndef CTS_USES_VULKANSC

constexpr VkShaderStageFlags SHADER_STAGE_ALL_RAY_TRACING =
    VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
    VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_KHR;

const VkTransformMatrixKHR identityMatrix3x4 = {
    {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}}};

template <typename T>
inline de::SharedPtr<Move<T>> makeVkSharedPtr(Move<T> move)
{
    return de::SharedPtr<Move<T>>(new Move<T>(move));
}

template <typename T>
inline de::SharedPtr<de::MovePtr<T>> makeVkSharedPtr(de::MovePtr<T> movePtr)
{
    return de::SharedPtr<de::MovePtr<T>>(new de::MovePtr<T>(movePtr));
}

template <typename T>
inline const T *dataOrNullPtr(const std::vector<T> &v)
{
    return (v.empty() ? DE_NULL : v.data());
}

template <typename T>
inline T *dataOrNullPtr(std::vector<T> &v)
{
    return (v.empty() ? DE_NULL : v.data());
}

inline std::string updateRayTracingGLSL(const std::string &str)
{
    return str;
}

std::string getCommonRayGenerationShader(void);

// Get lowercase version of the format name with no VK_FORMAT_ prefix.
std::string getFormatSimpleName(vk::VkFormat format);

// Checks the given vertex buffer format is valid for acceleration structures.
// Note: VK_KHR_get_physical_device_properties2 and VK_KHR_acceleration_structure are supposed to be supported.
void checkAccelerationStructureVertexBufferFormat(const vk::InstanceInterface &vki, vk::VkPhysicalDevice physicalDevice,
                                                  vk::VkFormat format);

class RaytracedGeometryBase
{
public:
    RaytracedGeometryBase()                                      = delete;
    RaytracedGeometryBase(const RaytracedGeometryBase &geometry) = delete;
    RaytracedGeometryBase(VkGeometryTypeKHR geometryType, VkFormat vertexFormat, VkIndexType indexType);
    virtual ~RaytracedGeometryBase();

    inline VkGeometryTypeKHR getGeometryType(void) const
    {
        return m_geometryType;
    }
    inline bool isTrianglesType(void) const
    {
        return m_geometryType == VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    }
    inline VkFormat getVertexFormat(void) const
    {
        return m_vertexFormat;
    }
    inline VkIndexType getIndexType(void) const
    {
        return m_indexType;
    }
    inline bool usesIndices(void) const
    {
        return m_indexType != VK_INDEX_TYPE_NONE_KHR;
    }
    inline VkGeometryFlagsKHR getGeometryFlags(void) const
    {
        return m_geometryFlags;
    }
    inline void setGeometryFlags(const VkGeometryFlagsKHR geometryFlags)
    {
        m_geometryFlags = geometryFlags;
    }
    virtual uint32_t getVertexCount(void) const         = 0;
    virtual const uint8_t *getVertexPointer(void) const = 0;
    virtual VkDeviceSize getVertexStride(void) const    = 0;
    virtual VkDeviceSize getAABBStride(void) const      = 0;
    virtual size_t getVertexByteSize(void) const        = 0;
    virtual uint32_t getIndexCount(void) const          = 0;
    virtual const uint8_t *getIndexPointer(void) const  = 0;
    virtual VkDeviceSize getIndexStride(void) const     = 0;
    virtual size_t getIndexByteSize(void) const         = 0;
    virtual uint32_t getPrimitiveCount(void) const      = 0;
    virtual void addVertex(const tcu::Vec3 &vertex)     = 0;
    virtual void addIndex(const uint32_t &index)        = 0;

private:
    VkGeometryTypeKHR m_geometryType;
    VkFormat m_vertexFormat;
    VkIndexType m_indexType;
    VkGeometryFlagsKHR m_geometryFlags;
};

template <typename T>
inline T convertSatRte(float f)
{
    // \note Doesn't work for 64-bit types
    DE_STATIC_ASSERT(sizeof(T) < sizeof(uint64_t));
    DE_STATIC_ASSERT((-3 % 2 != 0) && (-4 % 2 == 0));

    int64_t minVal = std::numeric_limits<T>::min();
    int64_t maxVal = std::numeric_limits<T>::max();
    float q        = deFloatFrac(f);
    int64_t intVal = (int64_t)(f - q);

    // Rounding.
    if (q == 0.5f)
    {
        if (intVal % 2 != 0)
            intVal++;
    }
    else if (q > 0.5f)
        intVal++;
    // else Don't add anything

    // Saturate.
    intVal = de::max(minVal, de::min(maxVal, intVal));

    return (T)intVal;
}

// Converts float to signed integer with variable width.
// Source float is assumed to be in the [-1, 1] range.
template <typename T>
inline T deFloat32ToSNorm(float src)
{
    DE_STATIC_ASSERT(std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_signed);
    const T range  = std::numeric_limits<T>::max();
    const T intVal = convertSatRte<T>(src * static_cast<float>(range));
    return de::clamp<T>(intVal, -range, range);
}

typedef tcu::Vector<deFloat16, 2> Vec2_16;
typedef tcu::Vector<deFloat16, 3> Vec3_16;
typedef tcu::Vector<deFloat16, 4> Vec4_16;
typedef tcu::Vector<int16_t, 2> Vec2_16SNorm;
typedef tcu::Vector<int16_t, 3> Vec3_16SNorm;
typedef tcu::Vector<int16_t, 4> Vec4_16SNorm;
typedef tcu::Vector<int8_t, 2> Vec2_8SNorm;
typedef tcu::Vector<int8_t, 3> Vec3_8SNorm;
typedef tcu::Vector<int8_t, 4> Vec4_8SNorm;

template <typename V>
VkFormat vertexFormatFromType();
template <>
inline VkFormat vertexFormatFromType<tcu::Vec2>()
{
    return VK_FORMAT_R32G32_SFLOAT;
}
template <>
inline VkFormat vertexFormatFromType<tcu::Vec3>()
{
    return VK_FORMAT_R32G32B32_SFLOAT;
}
template <>
inline VkFormat vertexFormatFromType<tcu::Vec4>()
{
    return VK_FORMAT_R32G32B32A32_SFLOAT;
}
template <>
inline VkFormat vertexFormatFromType<Vec2_16>()
{
    return VK_FORMAT_R16G16_SFLOAT;
}
template <>
inline VkFormat vertexFormatFromType<Vec3_16>()
{
    return VK_FORMAT_R16G16B16_SFLOAT;
}
template <>
inline VkFormat vertexFormatFromType<Vec4_16>()
{
    return VK_FORMAT_R16G16B16A16_SFLOAT;
}
template <>
inline VkFormat vertexFormatFromType<Vec2_16SNorm>()
{
    return VK_FORMAT_R16G16_SNORM;
}
template <>
inline VkFormat vertexFormatFromType<Vec3_16SNorm>()
{
    return VK_FORMAT_R16G16B16_SNORM;
}
template <>
inline VkFormat vertexFormatFromType<Vec4_16SNorm>()
{
    return VK_FORMAT_R16G16B16A16_SNORM;
}
template <>
inline VkFormat vertexFormatFromType<tcu::DVec2>()
{
    return VK_FORMAT_R64G64_SFLOAT;
}
template <>
inline VkFormat vertexFormatFromType<tcu::DVec3>()
{
    return VK_FORMAT_R64G64B64_SFLOAT;
}
template <>
inline VkFormat vertexFormatFromType<tcu::DVec4>()
{
    return VK_FORMAT_R64G64B64A64_SFLOAT;
}
template <>
inline VkFormat vertexFormatFromType<Vec2_8SNorm>()
{
    return VK_FORMAT_R8G8_SNORM;
}
template <>
inline VkFormat vertexFormatFromType<Vec3_8SNorm>()
{
    return VK_FORMAT_R8G8B8_SNORM;
}
template <>
inline VkFormat vertexFormatFromType<Vec4_8SNorm>()
{
    return VK_FORMAT_R8G8B8A8_SNORM;
}

struct EmptyIndex
{
};
template <typename I>
VkIndexType indexTypeFromType();
template <>
inline VkIndexType indexTypeFromType<uint16_t>()
{
    return VK_INDEX_TYPE_UINT16;
}
template <>
inline VkIndexType indexTypeFromType<uint32_t>()
{
    return VK_INDEX_TYPE_UINT32;
}
template <>
inline VkIndexType indexTypeFromType<EmptyIndex>()
{
    return VK_INDEX_TYPE_NONE_KHR;
}

template <typename V>
V convertFloatTo(const tcu::Vec3 &vertex);
template <>
inline tcu::Vec2 convertFloatTo<tcu::Vec2>(const tcu::Vec3 &vertex)
{
    return tcu::Vec2(vertex.x(), vertex.y());
}
template <>
inline tcu::Vec3 convertFloatTo<tcu::Vec3>(const tcu::Vec3 &vertex)
{
    return vertex;
}
template <>
inline tcu::Vec4 convertFloatTo<tcu::Vec4>(const tcu::Vec3 &vertex)
{
    return tcu::Vec4(vertex.x(), vertex.y(), vertex.z(), 0.0f);
}
template <>
inline Vec2_16 convertFloatTo<Vec2_16>(const tcu::Vec3 &vertex)
{
    return Vec2_16(deFloat32To16(vertex.x()), deFloat32To16(vertex.y()));
}
template <>
inline Vec3_16 convertFloatTo<Vec3_16>(const tcu::Vec3 &vertex)
{
    return Vec3_16(deFloat32To16(vertex.x()), deFloat32To16(vertex.y()), deFloat32To16(vertex.z()));
}
template <>
inline Vec4_16 convertFloatTo<Vec4_16>(const tcu::Vec3 &vertex)
{
    return Vec4_16(deFloat32To16(vertex.x()), deFloat32To16(vertex.y()), deFloat32To16(vertex.z()),
                   deFloat32To16(0.0f));
}
template <>
inline Vec2_16SNorm convertFloatTo<Vec2_16SNorm>(const tcu::Vec3 &vertex)
{
    return Vec2_16SNorm(deFloat32ToSNorm<int16_t>(vertex.x()), deFloat32ToSNorm<int16_t>(vertex.y()));
}
template <>
inline Vec3_16SNorm convertFloatTo<Vec3_16SNorm>(const tcu::Vec3 &vertex)
{
    return Vec3_16SNorm(deFloat32ToSNorm<int16_t>(vertex.x()), deFloat32ToSNorm<int16_t>(vertex.y()),
                        deFloat32ToSNorm<int16_t>(vertex.z()));
}
template <>
inline Vec4_16SNorm convertFloatTo<Vec4_16SNorm>(const tcu::Vec3 &vertex)
{
    return Vec4_16SNorm(deFloat32ToSNorm<int16_t>(vertex.x()), deFloat32ToSNorm<int16_t>(vertex.y()),
                        deFloat32ToSNorm<int16_t>(vertex.z()), deFloat32ToSNorm<int16_t>(0.0f));
}
template <>
inline tcu::DVec2 convertFloatTo<tcu::DVec2>(const tcu::Vec3 &vertex)
{
    return tcu::DVec2(static_cast<double>(vertex.x()), static_cast<double>(vertex.y()));
}
template <>
inline tcu::DVec3 convertFloatTo<tcu::DVec3>(const tcu::Vec3 &vertex)
{
    return tcu::DVec3(static_cast<double>(vertex.x()), static_cast<double>(vertex.y()),
                      static_cast<double>(vertex.z()));
}
template <>
inline tcu::DVec4 convertFloatTo<tcu::DVec4>(const tcu::Vec3 &vertex)
{
    return tcu::DVec4(static_cast<double>(vertex.x()), static_cast<double>(vertex.y()), static_cast<double>(vertex.z()),
                      0.0);
}
template <>
inline Vec2_8SNorm convertFloatTo<Vec2_8SNorm>(const tcu::Vec3 &vertex)
{
    return Vec2_8SNorm(deFloat32ToSNorm<int8_t>(vertex.x()), deFloat32ToSNorm<int8_t>(vertex.y()));
}
template <>
inline Vec3_8SNorm convertFloatTo<Vec3_8SNorm>(const tcu::Vec3 &vertex)
{
    return Vec3_8SNorm(deFloat32ToSNorm<int8_t>(vertex.x()), deFloat32ToSNorm<int8_t>(vertex.y()),
                       deFloat32ToSNorm<int8_t>(vertex.z()));
}
template <>
inline Vec4_8SNorm convertFloatTo<Vec4_8SNorm>(const tcu::Vec3 &vertex)
{
    return Vec4_8SNorm(deFloat32ToSNorm<int8_t>(vertex.x()), deFloat32ToSNorm<int8_t>(vertex.y()),
                       deFloat32ToSNorm<int8_t>(vertex.z()), deFloat32ToSNorm<int8_t>(0.0f));
}

template <typename V>
V convertIndexTo(uint32_t index);
template <>
inline EmptyIndex convertIndexTo<EmptyIndex>(uint32_t index)
{
    DE_UNREF(index);
    TCU_THROW(TestError, "Cannot add empty index");
}
template <>
inline uint16_t convertIndexTo<uint16_t>(uint32_t index)
{
    return static_cast<uint16_t>(index);
}
template <>
inline uint32_t convertIndexTo<uint32_t>(uint32_t index)
{
    return index;
}

template <typename V, typename I>
class RaytracedGeometry : public RaytracedGeometryBase
{
public:
    RaytracedGeometry()                                  = delete;
    RaytracedGeometry(const RaytracedGeometry &geometry) = delete;
    RaytracedGeometry(VkGeometryTypeKHR geometryType, uint32_t paddingBlocks = 0u);
    RaytracedGeometry(VkGeometryTypeKHR geometryType, const std::vector<V> &vertices,
                      const std::vector<I> &indices = std::vector<I>(), uint32_t paddingBlocks = 0u);

    uint32_t getVertexCount(void) const override;
    const uint8_t *getVertexPointer(void) const override;
    VkDeviceSize getVertexStride(void) const override;
    VkDeviceSize getAABBStride(void) const override;
    size_t getVertexByteSize(void) const override;
    uint32_t getIndexCount(void) const override;
    const uint8_t *getIndexPointer(void) const override;
    VkDeviceSize getIndexStride(void) const override;
    size_t getIndexByteSize(void) const override;
    uint32_t getPrimitiveCount(void) const override;

    void addVertex(const tcu::Vec3 &vertex) override;
    void addIndex(const uint32_t &index) override;

private:
    void init();                           // To be run in constructors.
    void checkGeometryType() const;        // Checks geometry type is valid.
    void calcBlockSize();                  // Calculates and saves vertex buffer block size.
    size_t getBlockSize() const;           // Return stored vertex buffer block size.
    void addNativeVertex(const V &vertex); // Adds new vertex in native format.

    // The implementation below stores vertices as byte blocks to take the requested padding into account. m_vertices is the array
    // of bytes containing vertex data.
    //
    // For triangles, the padding block has a size that is a multiple of the vertex size and each vertex is stored in a byte block
    // equivalent to:
    //
    //    struct Vertex
    //    {
    // V vertex;
    // uint8_t padding[m_paddingBlocks * sizeof(V)];
    // };
    //
    // For AABBs, the padding block has a size that is a multiple of kAABBPadBaseSize (see below) and vertices are stored in pairs
    // before the padding block. This is equivalent to:
    //
    //        struct VertexPair
    //        {
    // V vertices[2];
    // uint8_t padding[m_paddingBlocks * kAABBPadBaseSize];
    // };
    //
    // The size of each pseudo-structure above is saved to one of the correspoding union members below.
    union BlockSize
    {
        size_t trianglesBlockSize;
        size_t aabbsBlockSize;
    };

    const uint32_t m_paddingBlocks;
    size_t m_vertexCount;
    std::vector<uint8_t> m_vertices; // Vertices are stored as byte blocks.
    std::vector<I> m_indices;        // Indices are stored natively.
    BlockSize m_blockSize;           // For m_vertices.

    // Data sizes.
    static constexpr size_t kVertexSize      = sizeof(V);
    static constexpr size_t kIndexSize       = sizeof(I);
    static constexpr size_t kAABBPadBaseSize = 8; // As required by the spec.
};

template <typename V, typename I>
RaytracedGeometry<V, I>::RaytracedGeometry(VkGeometryTypeKHR geometryType, uint32_t paddingBlocks)
    : RaytracedGeometryBase(geometryType, vertexFormatFromType<V>(), indexTypeFromType<I>())
    , m_paddingBlocks(paddingBlocks)
    , m_vertexCount(0)
{
    init();
}

template <typename V, typename I>
RaytracedGeometry<V, I>::RaytracedGeometry(VkGeometryTypeKHR geometryType, const std::vector<V> &vertices,
                                           const std::vector<I> &indices, uint32_t paddingBlocks)
    : RaytracedGeometryBase(geometryType, vertexFormatFromType<V>(), indexTypeFromType<I>())
    , m_paddingBlocks(paddingBlocks)
    , m_vertexCount(0)
    , m_vertices()
    , m_indices(indices)
{
    init();
    for (const auto &vertex : vertices)
        addNativeVertex(vertex);
}

template <typename V, typename I>
uint32_t RaytracedGeometry<V, I>::getVertexCount(void) const
{
    return (isTrianglesType() ? static_cast<uint32_t>(m_vertexCount) : 0u);
}

template <typename V, typename I>
const uint8_t *RaytracedGeometry<V, I>::getVertexPointer(void) const
{
    DE_ASSERT(!m_vertices.empty());
    return reinterpret_cast<const uint8_t *>(m_vertices.data());
}

template <typename V, typename I>
VkDeviceSize RaytracedGeometry<V, I>::getVertexStride(void) const
{
    return ((!isTrianglesType()) ? 0ull : static_cast<VkDeviceSize>(getBlockSize()));
}

template <typename V, typename I>
VkDeviceSize RaytracedGeometry<V, I>::getAABBStride(void) const
{
    return (isTrianglesType() ? 0ull : static_cast<VkDeviceSize>(getBlockSize()));
}

template <typename V, typename I>
size_t RaytracedGeometry<V, I>::getVertexByteSize(void) const
{
    return m_vertices.size();
}

template <typename V, typename I>
uint32_t RaytracedGeometry<V, I>::getIndexCount(void) const
{
    return static_cast<uint32_t>(isTrianglesType() ? m_indices.size() : 0);
}

template <typename V, typename I>
const uint8_t *RaytracedGeometry<V, I>::getIndexPointer(void) const
{
    const auto indexCount = getIndexCount();
    DE_UNREF(indexCount); // For release builds.
    DE_ASSERT(indexCount > 0u);

    return reinterpret_cast<const uint8_t *>(m_indices.data());
}

template <typename V, typename I>
VkDeviceSize RaytracedGeometry<V, I>::getIndexStride(void) const
{
    return static_cast<VkDeviceSize>(kIndexSize);
}

template <typename V, typename I>
size_t RaytracedGeometry<V, I>::getIndexByteSize(void) const
{
    const auto indexCount = getIndexCount();
    DE_ASSERT(indexCount > 0u);

    return (indexCount * kIndexSize);
}

template <typename V, typename I>
uint32_t RaytracedGeometry<V, I>::getPrimitiveCount(void) const
{
    return static_cast<uint32_t>(isTrianglesType() ? (usesIndices() ? m_indices.size() / 3 : m_vertexCount / 3) :
                                                     (m_vertexCount / 2));
}

template <typename V, typename I>
void RaytracedGeometry<V, I>::addVertex(const tcu::Vec3 &vertex)
{
    addNativeVertex(convertFloatTo<V>(vertex));
}

template <typename V, typename I>
void RaytracedGeometry<V, I>::addNativeVertex(const V &vertex)
{
    const auto oldSize   = m_vertices.size();
    const auto blockSize = getBlockSize();

    if (isTrianglesType())
    {
        // Reserve new block, copy vertex at the beginning of the new block.
        m_vertices.resize(oldSize + blockSize, uint8_t{0});
        deMemcpy(&m_vertices[oldSize], &vertex, kVertexSize);
    }
    else // AABB
    {
        if (m_vertexCount % 2 == 0)
        {
            // New block needed.
            m_vertices.resize(oldSize + blockSize, uint8_t{0});
            deMemcpy(&m_vertices[oldSize], &vertex, kVertexSize);
        }
        else
        {
            // Insert in the second position of last existing block.
            //
            //                                                Vertex Size
            //                                                +-------+
            //    +-------------+------------+----------------------------------------+
            //    |             |            |      ...       | vertex vertex padding |
            //    +-------------+------------+----------------+-----------------------+
            //                                                +-----------------------+
            //                                                        Block Size
            //    +-------------------------------------------------------------------+
            //                            Old Size
            //
            deMemcpy(&m_vertices[oldSize - blockSize + kVertexSize], &vertex, kVertexSize);
        }
    }

    ++m_vertexCount;
}

template <typename V, typename I>
void RaytracedGeometry<V, I>::addIndex(const uint32_t &index)
{
    m_indices.push_back(convertIndexTo<I>(index));
}

template <typename V, typename I>
void RaytracedGeometry<V, I>::init()
{
    checkGeometryType();
    calcBlockSize();
}

template <typename V, typename I>
void RaytracedGeometry<V, I>::checkGeometryType() const
{
    const auto geometryType = getGeometryType();
    DE_UNREF(geometryType); // For release builds.
    DE_ASSERT(geometryType == VK_GEOMETRY_TYPE_TRIANGLES_KHR || geometryType == VK_GEOMETRY_TYPE_AABBS_KHR);
}

template <typename V, typename I>
void RaytracedGeometry<V, I>::calcBlockSize()
{
    if (isTrianglesType())
        m_blockSize.trianglesBlockSize = kVertexSize * static_cast<size_t>(1u + m_paddingBlocks);
    else
        m_blockSize.aabbsBlockSize = 2 * kVertexSize + m_paddingBlocks * kAABBPadBaseSize;
}

template <typename V, typename I>
size_t RaytracedGeometry<V, I>::getBlockSize() const
{
    return (isTrianglesType() ? m_blockSize.trianglesBlockSize : m_blockSize.aabbsBlockSize);
}

de::SharedPtr<RaytracedGeometryBase> makeRaytracedGeometry(VkGeometryTypeKHR geometryType, VkFormat vertexFormat,
                                                           VkIndexType indexType, bool padVertices = false);

VkDeviceAddress getBufferDeviceAddress(const DeviceInterface &vkd, const VkDevice device, const VkBuffer buffer,
                                       VkDeviceSize offset);

// type used for creating a deep serialization/deserialization of top-level acceleration structures
class SerialInfo
{
    std::vector<uint64_t> m_addresses;
    std::vector<VkDeviceSize> m_sizes;

public:
    SerialInfo() = default;

    // addresses: { (owner-top-level AS address) [, (first bottom_level AS address), (second bottom_level AS address), ...] }
    // sizes:     { (owner-top-level AS serial size) [, (first bottom_level AS serial size), (second bottom_level AS serial size), ...] }
    SerialInfo(const std::vector<uint64_t> &addresses, const std::vector<VkDeviceSize> &sizes)
        : m_addresses(addresses)
        , m_sizes(sizes)
    {
        DE_ASSERT(!addresses.empty() && addresses.size() == sizes.size());
    }

    const std::vector<uint64_t> &addresses() const
    {
        return m_addresses;
    }
    const std::vector<VkDeviceSize> &sizes() const
    {
        return m_sizes;
    }
};

class SerialStorage
{
public:
    enum
    {
        DE_SERIALIZED_FIELD(
            DRIVER_UUID, VK_UUID_SIZE), // VK_UUID_SIZE bytes of data matching VkPhysicalDeviceIDProperties::driverUUID
        DE_SERIALIZED_FIELD(
            COMPAT_UUID,
            VK_UUID_SIZE), // VK_UUID_SIZE bytes of data identifying the compatibility for comparison using vkGetDeviceAccelerationStructureCompatibilityKHR
        DE_SERIALIZED_FIELD(
            SERIALIZED_SIZE,
            sizeof(
                uint64_t)), // A 64-bit integer of the total size matching the value queried using VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR
        DE_SERIALIZED_FIELD(
            DESERIALIZED_SIZE,
            sizeof(
                uint64_t)), // A 64-bit integer of the deserialized size to be passed in to VkAccelerationStructureCreateInfoKHR::size
        DE_SERIALIZED_FIELD(
            HANDLES_COUNT,
            sizeof(
                uint64_t)), // A 64-bit integer of the count of the number of acceleration structure handles following. This will be zero for a bottom-level acceleration structure.
        SERIAL_STORAGE_SIZE_MIN
    };

    // An old fashion C-style structure that simplifies an access to the AS header
    struct alignas(16) AccelerationStructureHeader
    {
        union
        {
            struct
            {
                uint8_t driverUUID[VK_UUID_SIZE];
                uint8_t compactUUID[VK_UUID_SIZE];
            };
            uint8_t uuids[VK_UUID_SIZE * 2];
        };
        uint64_t serializedSize;
        uint64_t deserializedSize;
        uint64_t handleCount;
        VkDeviceAddress handleArray[1];
    };

    SerialStorage() = delete;
    SerialStorage(const DeviceInterface &vk, const VkDevice device, Allocator &allocator,
                  const VkAccelerationStructureBuildTypeKHR buildType, const VkDeviceSize storageSize);
    // An additional constructor for creating a deep copy of top-level AS's.
    SerialStorage(const DeviceInterface &vk, const VkDevice device, Allocator &allocator,
                  const VkAccelerationStructureBuildTypeKHR buildType, const SerialInfo &SerialInfo);

    // below methods will return host addres if AS was build on cpu and device addres when it was build on gpu
    VkDeviceOrHostAddressKHR getAddress(const DeviceInterface &vk, const VkDevice device,
                                        const VkAccelerationStructureBuildTypeKHR buildType);
    VkDeviceOrHostAddressConstKHR getAddressConst(const DeviceInterface &vk, const VkDevice device,
                                                  const VkAccelerationStructureBuildTypeKHR buildType);

    // this methods retun host address regardless of where AS was built
    VkDeviceOrHostAddressKHR getHostAddress(VkDeviceSize offset = 0);
    VkDeviceOrHostAddressConstKHR getHostAddressConst(VkDeviceSize offset = 0);

    // works the similar way as getHostAddressConst() but returns more readable/intuitive object
    AccelerationStructureHeader *getASHeader();
    bool hasDeepFormat() const;
    de::SharedPtr<SerialStorage> getBottomStorage(uint32_t index) const;

    VkDeviceSize getStorageSize() const;
    const SerialInfo &getSerialInfo() const;
    uint64_t getDeserializedSize();

protected:
    const VkAccelerationStructureBuildTypeKHR m_buildType;
    const VkDeviceSize m_storageSize;
    const SerialInfo m_serialInfo;
    de::MovePtr<BufferWithMemory> m_buffer;
    std::vector<de::SharedPtr<SerialStorage>> m_bottoms;
};

class BottomLevelAccelerationStructure
{
public:
    static uint32_t getRequiredAllocationCount(void);

    BottomLevelAccelerationStructure();
    BottomLevelAccelerationStructure(const BottomLevelAccelerationStructure &other) = delete;
    virtual ~BottomLevelAccelerationStructure();

    virtual void setGeometryData(const std::vector<tcu::Vec3> &geometryData, const bool triangles,
                                 const VkGeometryFlagsKHR geometryFlags = 0u);
    virtual void setDefaultGeometryData(const VkShaderStageFlagBits testStage,
                                        const VkGeometryFlagsKHR geometryFlags = 0u);
    virtual void setGeometryCount(const size_t geometryCount);
    virtual void addGeometry(de::SharedPtr<RaytracedGeometryBase> &raytracedGeometry);
    virtual void addGeometry(const std::vector<tcu::Vec3> &geometryData, const bool triangles,
                             const VkGeometryFlagsKHR geometryFlags = 0u);

    virtual void setBuildType(const VkAccelerationStructureBuildTypeKHR buildType)                         = DE_NULL;
    virtual void setCreateFlags(const VkAccelerationStructureCreateFlagsKHR createFlags)                   = DE_NULL;
    virtual void setCreateGeneric(bool createGeneric)                                                      = 0;
    virtual void setBuildFlags(const VkBuildAccelerationStructureFlagsKHR buildFlags)                      = DE_NULL;
    virtual void setBuildWithoutGeometries(bool buildWithoutGeometries)                                    = 0;
    virtual void setBuildWithoutPrimitives(bool buildWithoutPrimitives)                                    = 0;
    virtual void setDeferredOperation(const bool deferredOperation, const uint32_t workerThreadCount = 0u) = DE_NULL;
    virtual void setUseArrayOfPointers(const bool useArrayOfPointers)                                      = DE_NULL;
    virtual void setIndirectBuildParameters(const VkBuffer indirectBuffer, const VkDeviceSize indirectBufferOffset,
                                            const uint32_t indirectBufferStride)                           = DE_NULL;
    virtual VkBuildAccelerationStructureFlagsKHR getBuildFlags() const                                     = DE_NULL;
    VkDeviceSize getStructureSize() const;

    // methods specific for each acceleration structure
    virtual void create(const DeviceInterface &vk, const VkDevice device, Allocator &allocator,
                        VkDeviceSize structureSize, VkDeviceAddress deviceAddress = 0u)                   = DE_NULL;
    virtual void build(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer) = DE_NULL;
    virtual void copyFrom(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                          BottomLevelAccelerationStructure *accelerationStructure, bool compactCopy)      = DE_NULL;

    virtual void serialize(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                           SerialStorage *storage)   = DE_NULL;
    virtual void deserialize(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                             SerialStorage *storage) = DE_NULL;

    // helper methods for typical acceleration structure creation tasks
    void createAndBuild(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                        Allocator &allocator, VkDeviceAddress deviceAddress = 0u);
    void createAndCopyFrom(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                           Allocator &allocator, BottomLevelAccelerationStructure *accelerationStructure,
                           VkDeviceSize compactCopySize = 0u, VkDeviceAddress deviceAddress = 0u);
    void createAndDeserializeFrom(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                                  Allocator &allocator, SerialStorage *storage, VkDeviceAddress deviceAddress = 0u);

    virtual const VkAccelerationStructureKHR *getPtr(void) const = DE_NULL;

protected:
    std::vector<de::SharedPtr<RaytracedGeometryBase>> m_geometriesData;
    VkDeviceSize m_structureSize;
    VkDeviceSize m_updateScratchSize;
    VkDeviceSize m_buildScratchSize;
};

de::MovePtr<BottomLevelAccelerationStructure> makeBottomLevelAccelerationStructure();

struct InstanceData
{
    InstanceData(VkTransformMatrixKHR matrix_, uint32_t instanceCustomIndex_, uint32_t mask_,
                 uint32_t instanceShaderBindingTableRecordOffset_, VkGeometryInstanceFlagsKHR flags_)
        : matrix(matrix_)
        , instanceCustomIndex(instanceCustomIndex_)
        , mask(mask_)
        , instanceShaderBindingTableRecordOffset(instanceShaderBindingTableRecordOffset_)
        , flags(flags_)
    {
    }
    VkTransformMatrixKHR matrix;
    uint32_t instanceCustomIndex;
    uint32_t mask;
    uint32_t instanceShaderBindingTableRecordOffset;
    VkGeometryInstanceFlagsKHR flags;
};

class TopLevelAccelerationStructure
{
public:
    static uint32_t getRequiredAllocationCount(void);

    TopLevelAccelerationStructure();
    TopLevelAccelerationStructure(const TopLevelAccelerationStructure &other) = delete;
    virtual ~TopLevelAccelerationStructure();

    virtual void setInstanceCount(const size_t instanceCount);
    virtual void addInstance(de::SharedPtr<BottomLevelAccelerationStructure> bottomLevelStructure,
                             const VkTransformMatrixKHR &matrix = identityMatrix3x4, uint32_t instanceCustomIndex = 0,
                             uint32_t mask = 0xFF, uint32_t instanceShaderBindingTableRecordOffset = 0,
                             VkGeometryInstanceFlagsKHR flags = VkGeometryInstanceFlagBitsKHR(0u));

    virtual void setBuildType(const VkAccelerationStructureBuildTypeKHR buildType)                         = DE_NULL;
    virtual void setCreateFlags(const VkAccelerationStructureCreateFlagsKHR createFlags)                   = DE_NULL;
    virtual void setCreateGeneric(bool createGeneric)                                                      = 0;
    virtual void setBuildFlags(const VkBuildAccelerationStructureFlagsKHR buildFlags)                      = DE_NULL;
    virtual void setBuildWithoutPrimitives(bool buildWithoutPrimitives)                                    = 0;
    virtual void setInactiveInstances(bool inactiveInstances)                                              = 0;
    virtual void setDeferredOperation(const bool deferredOperation, const uint32_t workerThreadCount = 0u) = DE_NULL;
    virtual void setUseArrayOfPointers(const bool useArrayOfPointers)                                      = DE_NULL;
    virtual void setIndirectBuildParameters(const VkBuffer indirectBuffer, const VkDeviceSize indirectBufferOffset,
                                            const uint32_t indirectBufferStride)                           = DE_NULL;
    virtual void setUsePPGeometries(const bool usePPGeometries)                                            = 0;
    virtual VkBuildAccelerationStructureFlagsKHR getBuildFlags() const                                     = DE_NULL;
    VkDeviceSize getStructureSize() const;

    // methods specific for each acceleration structure
    virtual void create(const DeviceInterface &vk, const VkDevice device, Allocator &allocator,
                        VkDeviceSize structureSize = 0u, VkDeviceAddress deviceAddress = 0u)              = DE_NULL;
    virtual void build(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer) = DE_NULL;
    virtual void copyFrom(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                          TopLevelAccelerationStructure *accelerationStructure, bool compactCopy)         = DE_NULL;

    virtual void serialize(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                           SerialStorage *storage)   = DE_NULL;
    virtual void deserialize(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                             SerialStorage *storage) = DE_NULL;

    virtual std::vector<VkDeviceSize> getSerializingSizes(const DeviceInterface &vk, const VkDevice device,
                                                          const VkQueue queue,
                                                          const uint32_t queueFamilyIndex) = DE_NULL;

    virtual std::vector<uint64_t> getSerializingAddresses(const DeviceInterface &vk,
                                                          const VkDevice device) const = DE_NULL;

    // helper methods for typical acceleration structure creation tasks
    void createAndBuild(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                        Allocator &allocator, VkDeviceAddress deviceAddress = 0u);
    void createAndCopyFrom(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                           Allocator &allocator, TopLevelAccelerationStructure *accelerationStructure,
                           VkDeviceSize compactCopySize = 0u, VkDeviceAddress deviceAddress = 0u);
    void createAndDeserializeFrom(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                                  Allocator &allocator, SerialStorage *storage, VkDeviceAddress deviceAddress = 0u);

    virtual const VkAccelerationStructureKHR *getPtr(void) const = DE_NULL;

    virtual void updateInstanceMatrix(const DeviceInterface &vk, const VkDevice device, size_t instanceIndex,
                                      const VkTransformMatrixKHR &matrix) = 0;

protected:
    std::vector<de::SharedPtr<BottomLevelAccelerationStructure>> m_bottomLevelInstances;
    std::vector<InstanceData> m_instanceData;
    VkDeviceSize m_structureSize;
    VkDeviceSize m_updateScratchSize;
    VkDeviceSize m_buildScratchSize;

    virtual void createAndDeserializeBottoms(const DeviceInterface &vk, const VkDevice device,
                                             const VkCommandBuffer cmdBuffer, Allocator &allocator,
                                             SerialStorage *storage) = DE_NULL;
};

de::MovePtr<TopLevelAccelerationStructure> makeTopLevelAccelerationStructure();

template <class ASType>
de::MovePtr<ASType> makeAccelerationStructure();
template <>
inline de::MovePtr<BottomLevelAccelerationStructure> makeAccelerationStructure()
{
    return makeBottomLevelAccelerationStructure();
}
template <>
inline de::MovePtr<TopLevelAccelerationStructure> makeAccelerationStructure()
{
    return makeTopLevelAccelerationStructure();
}

bool queryAccelerationStructureSize(const DeviceInterface &vk, const VkDevice device, const VkCommandBuffer cmdBuffer,
                                    const std::vector<VkAccelerationStructureKHR> &accelerationStructureHandles,
                                    VkAccelerationStructureBuildTypeKHR buildType, const VkQueryPool queryPool,
                                    VkQueryType queryType, uint32_t firstQuery, std::vector<VkDeviceSize> &results);

class RayTracingPipeline
{
public:
    RayTracingPipeline();
    ~RayTracingPipeline();

    void addShader(VkShaderStageFlagBits shaderStage, Move<VkShaderModule> shaderModule, uint32_t group,
                   const VkSpecializationInfo *specializationInfo = nullptr,
                   const VkPipelineShaderStageCreateFlags pipelineShaderStageCreateFlags =
                       static_cast<VkPipelineShaderStageCreateFlags>(0),
                   const void *pipelineShaderStageCreateInfopNext = nullptr);
    void addShader(VkShaderStageFlagBits shaderStage, de::SharedPtr<Move<VkShaderModule>> shaderModule, uint32_t group,
                   const VkSpecializationInfo *specializationInfoPtr = nullptr,
                   const VkPipelineShaderStageCreateFlags pipelineShaderStageCreateFlags =
                       static_cast<VkPipelineShaderStageCreateFlags>(0),
                   const void *pipelineShaderStageCreateInfopNext = nullptr);
    void addShader(VkShaderStageFlagBits shaderStage, VkShaderModule shaderModule, uint32_t group,
                   const VkSpecializationInfo *specializationInfo = nullptr,
                   const VkPipelineShaderStageCreateFlags pipelineShaderStageCreateFlags =
                       static_cast<VkPipelineShaderStageCreateFlags>(0),
                   const void *pipelineShaderStageCreateInfopNext = nullptr);
    void addLibrary(de::SharedPtr<de::MovePtr<RayTracingPipeline>> pipelineLibrary);
    Move<VkPipeline> createPipeline(const DeviceInterface &vk, const VkDevice device,
                                    const VkPipelineLayout pipelineLayout,
                                    const std::vector<de::SharedPtr<Move<VkPipeline>>> &pipelineLibraries =
                                        std::vector<de::SharedPtr<Move<VkPipeline>>>());
    std::vector<de::SharedPtr<Move<VkPipeline>>> createPipelineWithLibraries(const DeviceInterface &vk,
                                                                             const VkDevice device,
                                                                             const VkPipelineLayout pipelineLayout);
    de::MovePtr<BufferWithMemory> createShaderBindingTable(
        const DeviceInterface &vk, const VkDevice device, const VkPipeline pipeline, Allocator &allocator,
        const uint32_t &shaderGroupHandleSize, const uint32_t shaderGroupBaseAlignment, const uint32_t &firstGroup,
        const uint32_t &groupCount, const VkBufferCreateFlags &additionalBufferCreateFlags = VkBufferCreateFlags(0u),
        const VkBufferUsageFlags &additionalBufferUsageFlags = VkBufferUsageFlags(0u),
        const MemoryRequirement &additionalMemoryRequirement = MemoryRequirement::Any,
        const VkDeviceAddress &opaqueCaptureAddress = 0u, const uint32_t shaderBindingTableOffset = 0u,
        const uint32_t shaderRecordSize = 0u, const void **shaderGroupDataPtrPerGroup = nullptr);
    void setCreateFlags(const VkPipelineCreateFlags &pipelineCreateFlags);
    void setMaxRecursionDepth(const uint32_t &maxRecursionDepth);
    void setMaxPayloadSize(const uint32_t &maxPayloadSize);
    void setMaxAttributeSize(const uint32_t &maxAttributeSize);
    void setDeferredOperation(const bool deferredOperation, const uint32_t workerThreadCount = 0);
    void addDynamicState(const VkDynamicState &dynamicState);

protected:
    Move<VkPipeline> createPipelineKHR(const DeviceInterface &vk, const VkDevice device,
                                       const VkPipelineLayout pipelineLayout,
                                       const std::vector<de::SharedPtr<Move<VkPipeline>>> &pipelineLibraries);

    std::vector<de::SharedPtr<Move<VkShaderModule>>> m_shadersModules;
    std::vector<de::SharedPtr<de::MovePtr<RayTracingPipeline>>> m_pipelineLibraries;
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderCreateInfos;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shadersGroupCreateInfos;
    VkPipelineCreateFlags m_pipelineCreateFlags;
    uint32_t m_maxRecursionDepth;
    uint32_t m_maxPayloadSize;
    uint32_t m_maxAttributeSize;
    bool m_deferredOperation;
    uint32_t m_workerThreadCount;
    std::vector<VkDynamicState> m_dynamicStates;
};

class RayTracingProperties
{
protected:
    RayTracingProperties()
    {
    }

public:
    RayTracingProperties(const InstanceInterface &vki, const VkPhysicalDevice physicalDevice)
    {
        DE_UNREF(vki);
        DE_UNREF(physicalDevice);
    }
    virtual ~RayTracingProperties()
    {
    }

    virtual uint32_t getShaderGroupHandleSize(void)                  = DE_NULL;
    virtual uint32_t getMaxRecursionDepth(void)                      = DE_NULL;
    virtual uint32_t getMaxShaderGroupStride(void)                   = DE_NULL;
    virtual uint32_t getShaderGroupBaseAlignment(void)               = DE_NULL;
    virtual uint64_t getMaxGeometryCount(void)                       = DE_NULL;
    virtual uint64_t getMaxInstanceCount(void)                       = DE_NULL;
    virtual uint64_t getMaxPrimitiveCount(void)                      = DE_NULL;
    virtual uint32_t getMaxDescriptorSetAccelerationStructures(void) = DE_NULL;
    virtual uint32_t getMaxRayDispatchInvocationCount(void)          = DE_NULL;
    virtual uint32_t getMaxRayHitAttributeSize(void)                 = DE_NULL;
};

de::MovePtr<RayTracingProperties> makeRayTracingProperties(const InstanceInterface &vki,
                                                           const VkPhysicalDevice physicalDevice);

void cmdTraceRays(const DeviceInterface &vk, VkCommandBuffer commandBuffer,
                  const VkStridedDeviceAddressRegionKHR *raygenShaderBindingTableRegion,
                  const VkStridedDeviceAddressRegionKHR *missShaderBindingTableRegion,
                  const VkStridedDeviceAddressRegionKHR *hitShaderBindingTableRegion,
                  const VkStridedDeviceAddressRegionKHR *callableShaderBindingTableRegion, uint32_t width,
                  uint32_t height, uint32_t depth);

void cmdTraceRaysIndirect(const DeviceInterface &vk, VkCommandBuffer commandBuffer,
                          const VkStridedDeviceAddressRegionKHR *raygenShaderBindingTableRegion,
                          const VkStridedDeviceAddressRegionKHR *missShaderBindingTableRegion,
                          const VkStridedDeviceAddressRegionKHR *hitShaderBindingTableRegion,
                          const VkStridedDeviceAddressRegionKHR *callableShaderBindingTableRegion,
                          VkDeviceAddress indirectDeviceAddress);

#else

uint32_t rayTracingDefineAnything();

#endif // CTS_USES_VULKANSC

} // namespace vk

#endif // _VKRAYTRACINGUTIL_HPP
