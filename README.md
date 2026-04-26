# Renewal

一个面向 C++ Modules 的包管理与构建系统。  
A package management and build orchestration system for C++ Modules.

Renewal 的目标是为 C++ 模块化开发提供一套统一的源码组织、依赖声明和构建编排模型，让模块包像 Cargo/Go Modules 一样可以被描述、校验、编排和复用。  
Renewal aims to provide a unified model for source layout, dependency declaration, and build orchestration for C++ modular development, so module packages can be described, validated, orchestrated, and reused in a Cargo/Go Modules-like way.

## 文档 | Documents

- [规范草案 | Specification Draft](./docs/spec-draft.md)
- [构建系统介绍 | Build System Overview](./docs/build-system.md)

## 项目定位 | Project Scope

Renewal 当前聚焦一个最小可行的、以 CMake 为后端的 C++ 模块包管理前端。  
Renewal currently focuses on a minimal viable C++ module package-manager frontend backed by CMake.

当前版本的核心能力：  
Current capabilities:

- 包模型：使用 `pkg.toml` 作为包身份、依赖和构建输入的单一真相源  
  Package model: uses `pkg.toml` as the single source of truth for package identity, dependencies, and build inputs.
- 静态校验：校验包结构、统一导出模块、跨包导入、依赖声明和 workspace 级规则  
  Static validation: validates package layout, unified export modules, cross-package imports, dependency declarations, and workspace-level rules.
- CMake 生成：为单包或 workspace 生成完整的 CMake 工程，而不是仅输出片段  
  CMake generation: generates a complete CMake project for a package or workspace instead of only emitting fragments.
- 构建驱动：通过 `renewal build` 串联 `validate -> generate -> cmake configure -> cmake build`  
  Build driver: `renewal build` runs `validate -> generate -> cmake configure -> cmake build`.
- Toolchain 适配：当前优先适配 Clang，并为 GCC/MSVC 保留统一适配层  
  Toolchain adaptation: currently prioritizes Clang, with a shared adapter layer for GCC/MSVC.

职责边界：  
Responsibility boundary:

- Renewal：包模型、依赖声明、静态校验、workspace 发现、CMake 工程生成、构建驱动  
  Renewal: package model, dependency declaration, static validation, workspace discovery, CMake project generation, and build driving.
- CMake 与编译器：模块扫描、编译顺序推导、实际编译  
  CMake and the compiler: module scanning, compilation order derivation, and actual compilation.

## 命令 | Commands

当前 CLI 提供以下命令：  
The current CLI provides the following commands:

- `renewal new <path>`  
  创建一个新的 Renewal 包骨架；省略 `path` 时默认在当前工作目录初始化。  
  Create a new Renewal package scaffold; when `path` is omitted, the current working directory is initialized.
- `renewal validate [path]`  
  校验单包或 workspace；省略 `path` 时默认使用当前工作目录。  
  Validate a package or workspace; when `path` is omitted, the current working directory is used.
- `renewal generate [path]`  
  在目标目录下的 `.renewal/cmake-src/` 生成完整的 `CMakeLists.txt`；省略 `path` 时默认使用当前工作目录。  
  Generate a complete `CMakeLists.txt` under `.renewal/cmake-src/`; when `path` is omitted, the current working directory is used.
- `renewal generate [path] --stdout`  
  把生成结果输出到标准输出。  
  Print the generated CMake project to stdout.
- `renewal build [path]`  
  自动执行校验、生成、CMake configure 和构建；省略 `path` 时默认使用当前工作目录。  
  Run validation, generation, CMake configure, and build automatically; when `path` is omitted, the current working directory is used.

## 当前限制 | Current Limitations

- v1 仅完整支持本地 `path` 依赖  
  v1 fully supports local `path` dependencies only.
- 版本依赖目前只保留 manifest 语义；`generate/build` 会明确报未实现  
  Version dependencies are kept in the manifest model, but `generate/build` fail with a clear not-implemented message.
- 第一版禁止 header units  
  Header units are forbidden in v1.
- 第一版禁止 `import std`  
  `import std` is forbidden in v1.
- 实际模块扫描能力取决于 CMake 版本、generator 和编译器支持  
  Actual module scanning depends on the active CMake version, generator, and compiler support.

## 示例 | Example

```bash
renewal new hello
cd hello
renewal validate
renewal generate
renewal build
```

生成目录默认位于：  
Generated artifacts are placed under:

```text
<current-package-or-workspace>/
  .renewal/
    cmake-src/
      CMakeLists.txt
    build/
```
