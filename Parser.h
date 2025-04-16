#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include "Math.hpp"

class Parser {
private:
    std::string DataPath;
    std::vector<TriangleCombined> combined;

    std::string readFile(const std::string& filename);
    std::vector<std::string> extractStructs(const std::string& block);
    int findParam(const std::string& block, const std::string& key);
    std::string extractArrayContent(const std::string& block, const std::string& key);
    std::string findMeshesBlock(const std::string& text);
    std::string extractInnerMesh(const std::string& meshBlock);
    std::string extractHexBlob(const std::string& block, const std::string& key);
    std::vector<unsigned char> hexStringToBytes(const std::string& hex);
    void parseVertices(const std::string& hexBlob, std::vector<Vector3>& vertices);
    void parseTriangles(const std::string& hexBlob, std::vector<Triangle>& triangles);
    void parseMaterials(const std::string& arrayContent, std::vector<int>& materials);
    void processMeshes();
    std::string findHullsBlock(const std::string& text);
    std::string extractInnerBlock(const std::string& block, const std::string& key);
    std::vector<float> parseFloatArray(const std::string& arrayContent);
    std::vector<TriangleCombined> generateTrianglesFromAABB(const Vector3& min, const Vector3& max, int materialIndex);
    void processHulls();
    std::vector<int> parseCollisionAttributes();

public:
    Parser(const std::string& path);
    const std::vector<TriangleCombined>& GetCombinedList() const { return combined; }
};

#endif
