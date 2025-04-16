#ifndef VISCHECK_H
#define VISCHECK_H

#include <memory>
#include <vector>
#include <tuple>
#include <string>
#include "Parser.h"
#include "Math.hpp"

struct BVHNode {
    AABB bounds;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;
    std::vector<TriangleCombined> triangles;

    bool IsLeaf() const {
        return left == nullptr && right == nullptr;
    }
};

class VisCheck {
public:
    explicit VisCheck(const std::string& physPath);
    std::vector<std::tuple<float, int, float, int>> IsPointVisible(const Vector3& point1, const Vector3& point2);

    bool RayIntersectsTriangle(const Vector3& rayOrigin, const Vector3& rayDir,
        const TriangleCombined& triangle, float& t);
private:
    std::vector<TriangleCombined> combined;
    std::unique_ptr<BVHNode> bvhRoot;
    std::string bvhFilename;

    std::unique_ptr<BVHNode> BuildBVH(const std::vector<TriangleCombined>& tris);
    bool IntersectBVH(const BVHNode* node, const Vector3& rayOrigin, const Vector3& rayDir,
        float maxDistance, float& hitDistance, int& hitMaterial);
    void CollectIntersections(const Vector3& rayOrigin, const Vector3& rayDir,
        float maxDistance, std::vector<std::pair<float, int>>& intersections);
    bool SaveBVH(const BVHNode* node, std::ofstream& out);
    std::unique_ptr<BVHNode> LoadBVH(std::ifstream& in);
};

#endif