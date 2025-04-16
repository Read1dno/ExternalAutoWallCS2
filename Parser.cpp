#include "Parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstring>

std::string Parser::readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::vector<std::string> Parser::extractStructs(const std::string& block) {
    std::vector<std::string> structs;
    int depth = 0;
    size_t start = std::string::npos;
    for (size_t i = 0; i < block.size(); ++i) {
        if (block[i] == '{') {
            if (depth == 0)
                start = i;
            depth++;
        }
        else if (block[i] == '}') {
            depth--;
            if (depth == 0 && start != std::string::npos) {
                structs.push_back(block.substr(start, i - start + 1));
                start = std::string::npos;
            }
        }
    }
    return structs;
}

int Parser::findParam(const std::string& block, const std::string& key) {
    size_t pos = block.find(key);
    if (pos == std::string::npos)
        return -1;
    pos = block.find("=", pos);
    if (pos == std::string::npos)
        return -1;
    pos++;
    while (pos < block.size() && std::isspace(block[pos]))
        pos++;
    std::istringstream iss(block.substr(pos));
    int value;
    iss >> value;
    return value;
}

std::string Parser::extractArrayContent(const std::string& block, const std::string& key) {
    size_t pos = block.find(key);
    if (pos == std::string::npos)
        return "";
    pos = block.find('[', pos);
    if (pos == std::string::npos)
        return "";
    size_t end = block.find(']', pos);
    if (end == std::string::npos)
        return "";
    return block.substr(pos + 1, end - pos - 1);
}

std::string Parser::findMeshesBlock(const std::string& text) {
    size_t pos = text.find("m_meshes");
    if (pos == std::string::npos)
        return "";
    pos = text.find('[', pos);
    if (pos == std::string::npos)
        return "";
    int depth = 1;
    size_t start = pos + 1;
    size_t i = start;
    while (i < text.size() && depth > 0) {
        if (text[i] == '[')
            depth++;
        else if (text[i] == ']')
            depth--;
        i++;
    }
    return text.substr(start, i - start - 1);
}

std::string Parser::extractInnerMesh(const std::string& meshBlock) {
    size_t pos = meshBlock.find("m_Mesh");
    if (pos == std::string::npos)
        return "";
    pos = meshBlock.find('{', pos);
    if (pos == std::string::npos)
        return "";
    int depth = 0;
    size_t start = pos;
    size_t i = pos;
    for (; i < meshBlock.size(); ++i) {
        if (meshBlock[i] == '{')
            depth++;
        else if (meshBlock[i] == '}')
            depth--;
        if (depth == 0)
            break;
    }
    return meshBlock.substr(start, i - start + 1);
}

std::string Parser::extractHexBlob(const std::string& block, const std::string& key) {
    size_t pos = block.find(key);
    if (pos == std::string::npos)
        return "";
    pos = block.find("#[", pos);
    if (pos == std::string::npos)
        return "";
    pos += 2;
    size_t end = block.find(']', pos);
    if (end == std::string::npos)
        return "";
    std::string hexStr = block.substr(pos, end - pos);
    hexStr.erase(std::remove_if(hexStr.begin(), hexStr.end(), ::isspace), hexStr.end());
    return hexStr;
}

std::vector<unsigned char> Parser::hexStringToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    if (hex.size() % 2 != 0)
        return bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        unsigned int byte;
        std::istringstream iss(byteStr);
        iss >> std::hex >> byte;
        bytes.push_back(static_cast<unsigned char>(byte));
    }
    return bytes;
}

void Parser::parseVertices(const std::string& hexBlob, std::vector<Vector3>& vertices) {
    auto bytes = hexStringToBytes(hexBlob);
    size_t numVerts = bytes.size() / (3 * sizeof(float));
    vertices.resize(numVerts);
    for (size_t i = 0; i < numVerts; ++i) {
        float v[3];
        std::memcpy(v, &bytes[i * 12], 12);
        vertices[i] = { v[0], v[1], v[2] };
    }
}

void Parser::parseTriangles(const std::string& hexBlob, std::vector<Triangle>& triangles) {
    auto bytes = hexStringToBytes(hexBlob);
    size_t numTris = bytes.size() / (3 * sizeof(int));
    triangles.resize(numTris);
    for (size_t i = 0; i < numTris; ++i) {
        int idx[3];
        std::memcpy(idx, &bytes[i * 12], 12);
        triangles[i] = { idx[0], idx[1], idx[2] };
    }
}

void Parser::parseMaterials(const std::string& arrayContent, std::vector<int>& materials) {
    materials.clear();
    std::istringstream iss(arrayContent);
    int num;
    while (iss >> num) {
        materials.push_back(num);
        if (iss.peek() == ',')
            iss.ignore();
    }
}

std::vector<int> Parser::parseCollisionAttributes() {
    std::vector<int> defaultIndices;

    std::string fileText = readFile(DataPath);
    if (fileText.empty()) {
        std::cerr << "File is empty or cannot be read." << std::endl;
        return defaultIndices;
    }
    size_t pos = fileText.find("m_collisionAttributes");
    if (pos == std::string::npos) {
        std::cerr << "m_collisionAttributes block not found." << std::endl;
        return defaultIndices;
    }
    pos = fileText.find('[', pos);
    if (pos == std::string::npos)
        return defaultIndices;
    int depth = 1;
    size_t start = pos + 1;
    size_t i = start;
    while (i < fileText.size() && depth > 0) {
        if (fileText[i] == '[')
            depth++;
        else if (fileText[i] == ']')
            depth--;
        i++;
    }
    std::string collisionBlock = fileText.substr(start, i - start - 1);
    auto collisionStructs = extractStructs(collisionBlock);
    for (size_t idx = 0; idx < collisionStructs.size(); idx++) {
        size_t strPos = collisionStructs[idx].find("m_CollisionGroupString");
        if (strPos != std::string::npos) {
            size_t eqPos = collisionStructs[idx].find("=", strPos);
            if (eqPos == std::string::npos)
                continue;
            eqPos++;
            while (eqPos < collisionStructs[idx].size() &&
                (std::isspace(collisionStructs[idx][eqPos]) || collisionStructs[idx][eqPos] == '"'))
                eqPos++;
            size_t endPos = collisionStructs[idx].find_first_of("\"", eqPos);
            if (endPos == std::string::npos)
                continue;
            std::string groupStr = collisionStructs[idx].substr(eqPos, endPos - eqPos);
            std::transform(groupStr.begin(), groupStr.end(), groupStr.begin(), ::tolower);
            if (groupStr == "default") {
                defaultIndices.push_back(static_cast<int>(idx));
            }
        }
    }
    return defaultIndices;
}

void Parser::processMeshes() {
    std::string fileText = readFile(DataPath);
    if (fileText.empty()) {
        std::cerr << "File is empty or cannot be read." << std::endl;
        return;
    }
    std::string meshesBlock = findMeshesBlock(fileText);
    if (meshesBlock.empty()) {
        std::cerr << "m_meshes block not found." << std::endl;
        return;
    }
    auto meshStructs = extractStructs(meshesBlock);
    if (meshStructs.empty()) {
        std::cerr << "No mesh structures found in m_meshes." << std::endl;
        return;
    }

    std::vector<int> defaultCollisionIndices = parseCollisionAttributes();

    std::cout << "=== Parsing m_meshes ===" << std::endl;
    int blockIndex = 1;
    for (const auto& meshStruct : meshStructs) {
        int collisionAttribute = findParam(meshStruct, "m_nCollisionAttributeIndex");
        if (std::find(defaultCollisionIndices.begin(), defaultCollisionIndices.end(), collisionAttribute)
            == defaultCollisionIndices.end()) {
            continue;
        }
        int surfaceProperty = findParam(meshStruct, "m_nSurfacePropertyIndex");

        std::string innerMesh = extractInnerMesh(meshStruct);
        if (innerMesh.empty()) {
            std::cerr << "Inner m_Mesh block not found." << std::endl;
            continue;
        }
        std::string verticesHex = extractHexBlob(innerMesh, "m_Vertices");
        std::string trianglesHex = extractHexBlob(innerMesh, "m_Triangles");
        std::string materialsContent = extractArrayContent(innerMesh, "m_Materials");

        if (verticesHex.empty() || trianglesHex.empty()) {
            std::cerr << "Required block(s) not found in m_meshes." << std::endl;
            continue;
        }

        std::vector<Vector3> vertices;
        std::vector<Triangle> triangles;
        std::vector<int> materials;

        parseVertices(verticesHex, vertices);
        parseTriangles(trianglesHex, triangles);
        bool hasMaterials = !materialsContent.empty();
        if (hasMaterials) {
            parseMaterials(materialsContent, materials);
        }

        std::cout << "Mesh Block " << blockIndex << ":" << std::endl;
        std::cout << "  Collision Attribute = " << collisionAttribute << std::endl;
        if (hasMaterials) {
            std::cout << "  Materials: ";
            for (size_t i = 0; i < materials.size(); ++i) {
                std::cout << materials[i] << (i + 1 < materials.size() ? ", " : "");
            }
            std::cout << std::endl;
        }
        else {
            std::cout << "  Surface Property Index = " << surfaceProperty << std::endl;
        }
        std::cout << "  Number of Geometry (Triangles) = " << triangles.size() << std::endl;
        std::cout << "----------------------------------------------" << std::endl;
        blockIndex++;

        for (size_t i = 0; i < triangles.size(); ++i) {
            TriangleCombined tc;
            const Triangle& t = triangles[i];
            if (t.a >= vertices.size() || t.b >= vertices.size() || t.c >= vertices.size()) {
                std::cerr << "Invalid vertex index in triangle." << std::endl;
                continue;
            }
            tc.v0 = vertices[t.a];
            tc.v1 = vertices[t.b];
            tc.v2 = vertices[t.c];
            if (hasMaterials)
                tc.materialIndex = (i < materials.size()) ? materials[i] : 15;
            else
                tc.materialIndex = surfaceProperty;
            combined.push_back(tc);
        }
    }
}

std::string Parser::findHullsBlock(const std::string& text) {
    size_t pos = text.find("m_hulls");
    if (pos == std::string::npos)
        return "";
    pos = text.find('[', pos);
    if (pos == std::string::npos)
        return "";
    int depth = 1;
    size_t start = pos + 1;
    size_t i = start;
    while (i < text.size() && depth > 0) {
        if (text[i] == '[')
            depth++;
        else if (text[i] == ']')
            depth--;
        i++;
    }
    return text.substr(start, i - start - 1);
}

std::string Parser::extractInnerBlock(const std::string& block, const std::string& key) {
    size_t pos = block.find(key);
    if (pos == std::string::npos)
        return "";
    pos = block.find('{', pos);
    if (pos == std::string::npos)
        return "";
    int depth = 0;
    size_t start = pos;
    size_t i = pos;
    for (; i < block.size(); ++i) {
        if (block[i] == '{')
            depth++;
        else if (block[i] == '}')
            depth--;
        if (depth == 0)
            break;
    }
    return block.substr(start, i - start + 1);
}

std::vector<float> Parser::parseFloatArray(const std::string& arrayContent) {
    std::vector<float> values;
    std::istringstream iss(arrayContent);
    float num;
    while (iss >> num) {
        values.push_back(num);
        if (iss.peek() == ',' || std::isspace(iss.peek()))
            iss.ignore();
    }
    return values;
}

std::vector<TriangleCombined> Parser::generateTrianglesFromAABB(const Vector3& min, const Vector3& max, int materialIndex) {
    std::vector<TriangleCombined> tris;
    Vector3 v0 = { min.x, min.y, min.z };
    Vector3 v1 = { max.x, min.y, min.z };
    Vector3 v2 = { max.x, max.y, min.z };
    Vector3 v3 = { min.x, max.y, min.z };
    Vector3 v4 = { min.x, min.y, max.z };
    Vector3 v5 = { max.x, min.y, max.z };
    Vector3 v6 = { max.x, max.y, max.z };
    Vector3 v7 = { min.x, max.y, max.z };

    auto addFace = [&tris, materialIndex](const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d) {
        TriangleCombined t1, t2;
        t1.v0 = a; t1.v1 = b; t1.v2 = c; t1.materialIndex = materialIndex;
        t2.v0 = a; t2.v1 = c; t2.v2 = d; t2.materialIndex = materialIndex;
        tris.push_back(t1);
        tris.push_back(t2);
        };

    addFace(v0, v1, v2, v3);
    addFace(v4, v5, v6, v7);
    addFace(v0, v1, v5, v4);
    addFace(v3, v2, v6, v7);
    addFace(v0, v3, v7, v4);
    addFace(v1, v2, v6, v5);

    return tris;
}

void Parser::processHulls() {
    std::string fileText = readFile(DataPath);
    if (fileText.empty()) {
        std::cerr << "File is empty or cannot be read." << std::endl;
        return;
    }
    std::string hullsBlock = findHullsBlock(fileText);
    if (hullsBlock.empty()) {
        std::cerr << "m_hulls block not found." << std::endl;
        return;
    }
    auto hullStructs = extractStructs(hullsBlock);
    if (hullStructs.empty()) {
        std::cerr << "No hull structures found in m_hulls." << std::endl;
        return;
    }

    int overallMinSurface = 1000000;
    int overallMaxSurface = -1000000;
    int totalHullTriangles = 0;

    std::cout << "=== Parsing m_hulls ===" << std::endl;
    int hullIndex = 1;
    for (const auto& hullStruct : hullStructs) {
        int surfacePropertyIndex = findParam(hullStruct, "m_nSurfacePropertyIndex");
        std::cout << "Hull Block " << hullIndex << " - m_nSurfacePropertyIndex: " << surfacePropertyIndex << std::endl;
        if (surfacePropertyIndex < overallMinSurface)
            overallMinSurface = surfacePropertyIndex;
        if (surfacePropertyIndex > overallMaxSurface)
            overallMaxSurface = surfacePropertyIndex;

        std::string innerHull = extractInnerBlock(hullStruct, "m_Hull");
        if (innerHull.empty()) {
            std::cerr << "m_Hull block not found in one of the hull structures." << std::endl;
            hullIndex++;
            continue;
        }
        std::string boundsBlock = extractInnerBlock(innerHull, "m_Bounds");
        if (boundsBlock.empty()) {
            std::cerr << "m_Bounds block not found in m_Hull." << std::endl;
            hullIndex++;
            continue;
        }
        std::string minBoundsStr = extractArrayContent(boundsBlock, "m_vMinBounds");
        std::string maxBoundsStr = extractArrayContent(boundsBlock, "m_vMaxBounds");
        if (minBoundsStr.empty() || maxBoundsStr.empty()) {
            std::cerr << "m_vMinBounds or m_vMaxBounds not found in m_Bounds." << std::endl;
            hullIndex++;
            continue;
        }
        std::vector<float> minValues = parseFloatArray(minBoundsStr);
        std::vector<float> maxValues = parseFloatArray(maxBoundsStr);
        if (minValues.size() < 3 || maxValues.size() < 3) {
            std::cerr << "Insufficient values for bounds." << std::endl;
            hullIndex++;
            continue;
        }
        Vector3 vmin = { minValues[0], minValues[1], minValues[2] };
        Vector3 vmax = { maxValues[0], maxValues[1], maxValues[2] };
        auto hullTriangles = generateTrianglesFromAABB(vmin, vmax, surfacePropertyIndex);
        totalHullTriangles += hullTriangles.size();
        combined.insert(combined.end(), hullTriangles.begin(), hullTriangles.end());
        hullIndex++;
    }

    std::cout << "Overall m_nSurfacePropertyIndex Range: " << overallMinSurface
        << " to " << overallMaxSurface << std::endl;
    std::cout << "Total Geometry (Triangles) generated from hulls: " << totalHullTriangles << std::endl;
}

Parser::Parser(const std::string& path) : DataPath(path) {
    processMeshes();
    processHulls();
}
