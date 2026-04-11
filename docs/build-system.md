# Renewal 构建系统介绍 | Build System Overview

## 1. 定位 | Positioning

Renewal 不是一个自己实现完整扫描和编译流程的构建系统，而是一个位于上层的包管理与构建编排工具。  
Renewal is not a build system that implements the full scanning and compilation pipeline itself. It is a higher-level package management and build orchestration tool.

它负责：  
It is responsible for:

- 读取 `pkg.toml`  
  Reading `pkg.toml`
- 确认包身份和依赖关系  
  Determining package identity and dependencies
- 校验统一导出模块 `modules/<package.name>/mod.cppm`  
  Validating the unified export module `modules/<package.name>/mod.cppm`
- 生成 CMake target 与源码映射  
  Generating CMake targets and source mappings

它不负责：  
It is not responsible for:

- 自行实现模块扫描  
  Implementing module scanning itself
- 自行计算最终编译顺序  
  Computing the final compilation order itself
- 直接替代编译器完成 C++ Modules 构建  
  Replacing the compiler in performing C++ Modules compilation

## 2. 职责边界 | Responsibility Boundary

职责划分如下：  
Responsibilities are split as follows:

- Renewal：包模型、依赖声明、静态校验、CMake 映射生成  
  Renewal: package model, dependency declaration, static validation, and CMake mapping generation
- CMake：扫描模块单元、推导编译顺序、组织 target 依赖、执行增量构建  
  CMake: scans module units, derives compilation order, organizes target dependencies, and performs incremental builds
- 编译器：完成 C++ Modules 语义分析、BMI 生成和目标文件编译  
  Compiler: performs C++ Modules semantic analysis, BMI generation, and object file compilation

这意味着 Renewal 把“包管理层”的问题和“编译执行层”的问题分开处理。  
This means Renewal separates package-management concerns from compilation-execution concerns.

## 3. 为什么采用 CMake | Why CMake

选择 CMake 的原因主要有三点：  
There are three main reasons for choosing CMake:

- CMake 已经具备 C++ Modules 的构建表达能力  
  CMake already provides a build model for C++ Modules
- CMake 能把模块接口单元映射到 `FILE_SET ... TYPE CXX_MODULES`  
  CMake can map module interface units to `FILE_SET ... TYPE CXX_MODULES`
- 实际模块扫描、编译顺序推导和工具链兼容性问题，更适合交给 CMake 与编译器处理  
  Actual module scanning, compilation-order derivation, and toolchain compatibility are better handled by CMake and the compiler

因此 Renewal 不重复实现已有能力，而是专注于：  
Therefore, Renewal does not reimplement existing capabilities and instead focuses on:

- 包结构约束  
  Package layout constraints
- manifest 规范  
  Manifest specification
- 依赖声明与校验  
  Dependency declaration and validation
- 构建图到 CMake 的翻译  
  Translating the build graph into CMake

## 4. 一个包如何进入构建 | How a Package Enters the Build

对一个包，Renewal 的处理流程可以抽象为：  
For a package, Renewal's processing flow can be summarized as:

1. 读取包根目录下的 `pkg.toml`  
   Read `pkg.toml` from the package root
2. 确认 `package.name`  
   Determine `package.name`
3. 推导统一导出模块路径 `modules/<package.name>/mod.cppm`  
   Derive the unified export module path `modules/<package.name>/mod.cppm`
4. 校验该文件中的 `export module <package.name>;`  
   Validate the `export module <package.name>;` declaration in that file
5. 检查跨包导入是否都在 `[deps]` 中声明  
   Check whether all cross-package imports are declared in `[deps]`
6. 生成 CMake target 名和源码映射  
   Generate the CMake target name and source mapping
7. 把后续模块扫描与实际编译交给 CMake  
   Hand off subsequent module scanning and actual compilation to CMake

## 5. 目录与构建映射 | Layout and Build Mapping

一个包的典型结构如下：  
A typical package layout looks like this:

```text
json/
  pkg.toml
  modules/
    json/
      mod.cppm
      reader.cppm
      writer.cppm
      parse.cpp
      write.cpp
```

其中：  
Where:

- `mod.cppm` 是统一导出模块  
  `mod.cppm` is the unified export module
- 其他 `.cppm` 是其他模块接口单元、子模块或分区  
  Other `.cppm` files are other module interface units, submodules, or partitions
- `.cpp` 是普通实现文件  
  `.cpp` files are regular implementation files

构建时可以映射为：  
During the build, this can be mapped as:

```cmake
add_library(pkg_json)

target_sources(pkg_json
  PUBLIC
    FILE_SET cxxmods TYPE CXX_MODULES
    FILES
      modules/json/mod.cppm
      modules/json/reader.cppm
      modules/json/writer.cppm
  PRIVATE
      modules/json/parse.cpp
      modules/json/write.cpp
)
```

## 6. 设计收益 | Benefits

这套架构的收益是：  
This architecture provides the following benefits:

- Renewal 的规则简单，manifest 字段少  
  Renewal keeps the rules simple and the manifest compact
- 包入口固定，校验成本低  
  Package entry points are fixed, making validation cheap
- CMake 继续负责它最擅长的模块扫描和编译组织  
  CMake continues to handle module scanning and build orchestration, which it is good at
- 工具职责边界清晰，后续扩展空间更大  
  Tool responsibilities remain clear, leaving more room for future extension
