# ExternalAutoWallCS2
![aw](https://github.com/user-attachments/assets/2575ad3e-533c-4abd-ac0d-74ef8f098dad)
This project implements an **external AutoWall** system for Counter‑Strike 2. It extracts up‑to‑date collision geometry and material data from Source 2 `.vpk` files, builds a BVH for fast ray‑tracing, and computes bullet penetration damage using real material modifiers.

> **Warning:**  
> - Not a polished release—some maps exhibit malformed geometry and inverted triangles.  
> - Development was time‑consuming and is currently paused. Use this as a reference or starting point.

## 🏗️ Features
- **Physics Extraction**:  
  ‑ Parses `.vphys`/`.phys` via `vmdll` to pull vertex, triangle, and material indices.  
- **Parser Module**:  
  ‑ Reads `m_collisionAttributes`, `m_meshes`, and `m_hulls` blocks.  
  ‑ Converts hex blobs into `std::vector<TriangleCombined>`.  
- **BVH Acceleration**:  
  ‑ Builds or loads a cached `.bvh` file.  
  ‑ Splits along the longest AABB axis until ≤ 4 triangles per node.  
- **Ray‑Triangle Intersection**:  
  ‑ Möller–Trumbore algorithm with BVH speed‑up.  
- **Bullet Penetration Logic**:  
  ‑ Computes damage reduction per material segment and distance falloff.  
- **Python Binding**:  
  ‑ Exposes `handle_bullet_penetration()` via **pybind11**.  
- **Material Parser**:  
  ‑ Reads `surfaceproperties.txt` and `surfaceproperties_game.txt` for penetration modifiers.

### Prerequisites
- Visual Studio 2022 (x64, Release)  
- Python 3.10.6 with `pybind11`

## 🛠️ Build (Visual Studio)
Configure project properties:

- **C/C++ → General → Additional Include Directories**  
  ```
  C:\Users\USERNAME\AppData\Local\Programs\Python\PythonXX\include
  C:\Users\USERNAME\AppData\Local\Programs\Python\PythonXX\Lib\site-packages\pybind11\include
  ```

- **Linker → General → Additional Library Directories**  
  ```
  C:\Users\USERNAME\AppData\Local\Programs\Python\PythonXX\libs
  ```

- **Linker → Input → Additional Dependencies**  
  ```
  pythonXX.lib
  ```

Build configuration:
```
Release x64
```

> ⚠️ Replace `USERNAME` and `PythonXX` with your actual Windows username and Python version.  
> For example: `Python310`, `USERNAME = John`.

## 📦 Example Usage
see [example](https://github.com/Read1dno/ExternalAutoWallCS2/tree/main/example)

## 🔹 Credits & Links
-  [Discord](https://discord.gg/n89PDURbTg)
[Telegram](https://t.me/bloomofficialyt)
- [UC | [Help] External visibility check](https://www.unknowncheats.me/forum/counter-strike-2-a/591548-external-visibility-check.html)

## 📄 License
This project is licensed under the MIT License.  
See the [LICENSE](https://github.com/Read1dno/ExternalAutoWallCS2/blob/main/LICENSE) file for details.
