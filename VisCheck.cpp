#include "VisCheck.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <functional>
#include <fstream>
#include <iostream>

const size_t LEAF_THRESHOLD = 4;

static void SerializeVector3(std::ofstream& out, const Vector3& v) {
    out.write(reinterpret_cast<const char*>(&v.x), sizeof(float));
    out.write(reinterpret_cast<const char*>(&v.y), sizeof(float));
    out.write(reinterpret_cast<const char*>(&v.z), sizeof(float));
}

static Vector3 DeserializeVector3(std::ifstream& in) {
    float x, y, z;
    in.read(reinterpret_cast<char*>(&x), sizeof(float));
    in.read(reinterpret_cast<char*>(&y), sizeof(float));
    in.read(reinterpret_cast<char*>(&z), sizeof(float));
    return Vector3(x, y, z);
}

static void SerializeAABB(std::ofstream& out, const AABB& box) {
    SerializeVector3(out, box.min);
    SerializeVector3(out, box.max);
}

static AABB DeserializeAABB(std::ifstream& in) {
    AABB box;
    box.min = DeserializeVector3(in);
    box.max = DeserializeVector3(in);
    return box;
}

static void SerializeTriangle(std::ofstream& out, const TriangleCombined& tri) {
    SerializeVector3(out, tri.v0);
    SerializeVector3(out, tri.v1);
    SerializeVector3(out, tri.v2);
    out.write(reinterpret_cast<const char*>(&tri.materialIndex), sizeof(int));
}

static TriangleCombined DeserializeTriangle(std::ifstream& in) {
    TriangleCombined tri;
    tri.v0 = DeserializeVector3(in);
    tri.v1 = DeserializeVector3(in);
    tri.v2 = DeserializeVector3(in);
    in.read(reinterpret_cast<char*>(&tri.materialIndex), sizeof(int));
    return tri;
}

bool VisCheck::SaveBVH(const BVHNode* node, std::ofstream& out) {
    if (!node)
        return false;
    SerializeAABB(out, node->bounds);
    bool isLeaf = node->IsLeaf();
    out.write(reinterpret_cast<const char*>(&isLeaf), sizeof(bool));
    if (isLeaf) {
        size_t triCount = node->triangles.size();
        out.write(reinterpret_cast<const char*>(&triCount), sizeof(size_t));
        for (const auto& tri : node->triangles) {
            SerializeTriangle(out, tri);
        }
    }
    else {
        SaveBVH(node->left.get(), out);
        SaveBVH(node->right.get(), out);
    }
    return true;
}

std::unique_ptr<BVHNode> VisCheck::LoadBVH(std::ifstream& in) {
    if (in.eof())
        return nullptr;
    auto node = std::make_unique<BVHNode>();
    node->bounds = DeserializeAABB(in);
    bool isLeaf = false;
    in.read(reinterpret_cast<char*>(&isLeaf), sizeof(bool));
    if (isLeaf) {
        size_t triCount = 0;
        in.read(reinterpret_cast<char*>(&triCount), sizeof(size_t));
        node->triangles.resize(triCount);
        for (size_t i = 0; i < triCount; ++i) {
            node->triangles[i] = DeserializeTriangle(in);
        }
    }
    else {
        node->left = LoadBVH(in);
        node->right = LoadBVH(in);
    }
    return node;
}

VisCheck::VisCheck(const std::string& physPath) {
    bvhFilename = physPath + ".bvh";
    std::ifstream infile(bvhFilename, std::ios::binary);
    if (infile.good()) {
        bvhRoot = LoadBVH(infile);
        infile.close();
        std::cout << "BVH tree loaded from file: " << bvhFilename << std::endl;
    }
    else {
        Parser parser(physPath);
        const std::vector<TriangleCombined>& allCombined = parser.GetCombinedList();
        if (allCombined.empty()) {
            std::cerr << "Geometry not found in phys file: " << physPath << std::endl;
        }
        else {
            combined = allCombined;
        }
        bvhRoot = BuildBVH(combined);
        std::ofstream outfile(bvhFilename, std::ios::binary);
        if (outfile.good()) {
            SaveBVH(bvhRoot.get(), outfile);
            outfile.close();
            std::cout << "BVH tree built and saved to file: " << bvhFilename << std::endl;
        }
        else {
            std::cerr << "Failed to open file for writing: " << bvhFilename << std::endl;
        }
    }
}

std::unique_ptr<BVHNode> VisCheck::BuildBVH(const std::vector<TriangleCombined>& tris) {
    auto node = std::make_unique<BVHNode>();
    if (tris.empty())
        return node;
    AABB bounds = tris[0].ComputeAABB();
    for (size_t i = 1; i < tris.size(); ++i) {
        AABB tAABB = tris[i].ComputeAABB();
        bounds.min.x = std::min(bounds.min.x, tAABB.min.x);
        bounds.min.y = std::min(bounds.min.y, tAABB.min.y);
        bounds.min.z = std::min(bounds.min.z, tAABB.min.z);
        bounds.max.x = std::max(bounds.max.x, tAABB.max.x);
        bounds.max.y = std::max(bounds.max.y, tAABB.max.y);
        bounds.max.z = std::max(bounds.max.z, tAABB.max.z);
    }
    node->bounds = bounds;
    if (tris.size() <= LEAF_THRESHOLD) {
        node->triangles = tris;
        return node;
    }
    Vector3 diff = bounds.max - bounds.min;
    int axis = (diff.x > diff.y && diff.x > diff.z) ? 0 : ((diff.y > diff.z) ? 1 : 2);
    std::vector<TriangleCombined> sortedTris = tris;
    std::sort(sortedTris.begin(), sortedTris.end(), [axis](const TriangleCombined& a, const TriangleCombined& b) {
        AABB aAABB = a.ComputeAABB();
        AABB bAABB = b.ComputeAABB();
        float centerA, centerB;
        if (axis == 0) {
            centerA = (aAABB.min.x + aAABB.max.x) / 2.0f;
            centerB = (bAABB.min.x + bAABB.max.x) / 2.0f;
        }
        else if (axis == 1) {
            centerA = (aAABB.min.y + aAABB.max.y) / 2.0f;
            centerB = (bAABB.min.y + bAABB.max.y) / 2.0f;
        }
        else {
            centerA = (aAABB.min.z + aAABB.max.z) / 2.0f;
            centerB = (bAABB.min.z + bAABB.max.z) / 2.0f;
        }
        return centerA < centerB;
        });
    size_t mid = sortedTris.size() / 2;
    std::vector<TriangleCombined> leftTris(sortedTris.begin(), sortedTris.begin() + mid);
    std::vector<TriangleCombined> rightTris(sortedTris.begin() + mid, sortedTris.end());
    node->left = BuildBVH(leftTris);
    node->right = BuildBVH(rightTris);
    return node;
}

bool VisCheck::IntersectBVH(const BVHNode* node, const Vector3& rayOrigin, const Vector3& rayDir,
    float maxDistance, float& hitDistance, int& hitMaterial) {
    if (!node->bounds.RayIntersects(rayOrigin, rayDir))
        return false;

    bool hit = false;
    if (node->IsLeaf()) {
        for (const auto& tri : node->triangles) {
            float t;
            if (RayIntersectsTriangle(rayOrigin, rayDir, tri, t)) {
                if (t < maxDistance && t < hitDistance) {
                    hitDistance = t;
                    hitMaterial = tri.materialIndex;
                    hit = true;
                }
            }
        }
    }
    else {
        if (node->left)
            hit |= IntersectBVH(node->left.get(), rayOrigin, rayDir, maxDistance, hitDistance, hitMaterial);
        if (node->right)
            hit |= IntersectBVH(node->right.get(), rayOrigin, rayDir, maxDistance, hitDistance, hitMaterial);
    }
    return hit;
}

void VisCheck::CollectIntersections(const Vector3& rayOrigin, const Vector3& rayDir,
    float maxDistance, std::vector<std::pair<float, int>>& intersections) {
    std::function<void(const BVHNode*)> traverse = [&](const BVHNode* node) {
        if (!node || !node->bounds.RayIntersects(rayOrigin, rayDir))
            return;
        if (node->IsLeaf()) {
            for (const auto& tri : node->triangles) {
                float t;
                if (RayIntersectsTriangle(rayOrigin, rayDir, tri, t)) {
                    if (t > 1e-7f && t < maxDistance) {
                        intersections.push_back(std::make_pair(t, tri.materialIndex));
                    }
                }
            }
        }
        else {
            if (node->left)
                traverse(node->left.get());
            if (node->right)
                traverse(node->right.get());
        }
        };
    traverse(bvhRoot.get());
}

std::vector<std::tuple<float, int, float, int>> VisCheck::IsPointVisible(const Vector3& point1, const Vector3& point2) {
    Vector3 rayDir = { point2.x - point1.x, point2.y - point1.y, point2.z - point1.z };
    float totalDistance = std::sqrt(rayDir.dot(rayDir));
    rayDir = { rayDir.x / totalDistance, rayDir.y / totalDistance, rayDir.z / totalDistance };
    std::vector<std::pair<float, int>> hits;
    CollectIntersections(point1, rayDir, totalDistance, hits);
    std::sort(hits.begin(), hits.end(), [](const std::pair<float, int>& a, const std::pair<float, int>& b) {
        return a.first < b.first;
        });

    std::vector<std::tuple<float, int, float, int>> result;
    float prevExitT = 0.0f;
    for (size_t i = 0; i + 1 < hits.size(); i += 2) {
        float entryT = hits[i].first;
        int entryMat = hits[i].second;
        float exitT = hits[i + 1].first;
        int exitMat = hits[i + 1].second;
        float gap = entryT - prevExitT;
        float thickness = exitT - entryT;
        result.push_back(std::make_tuple(gap, entryMat, thickness, exitMat));
        prevExitT = exitT;
    }
    return result;
}

bool VisCheck::RayIntersectsTriangle(const Vector3& rayOrigin, const Vector3& rayDir,
    const TriangleCombined& triangle, float& t) {
    const float EPSILON = 1e-7f;
    Vector3 edge1 = triangle.v1 - triangle.v0;
    Vector3 edge2 = triangle.v2 - triangle.v0;
    Vector3 h = rayDir.cross(edge2);
    float a = edge1.dot(h);
    if (a > -EPSILON && a < EPSILON)
        return false;
    float f = 1.0f / a;
    Vector3 s = rayOrigin - triangle.v0;
    float u = f * s.dot(h);
    if (u < 0.0f || u > 1.0f)
        return false;
    Vector3 q = s.cross(edge1);
    float v = f * rayDir.dot(q);
    if (v < 0.0f || u + v > 1.0f)
        return false;
    t = f * edge2.dot(q);
    return (t > EPSILON);
}
