# Renewal

一个面向 C++ Modules 的包管理与构建系统。  
A package management and build orchestration system for C++ Modules.

Renewal 的目标是为 C++ 模块化开发提供一套统一的源码组织、依赖声明、构建映射和源码拉取机制，让模块包像 Cargo/Go Modules 一样可以被描述、校验、编排和复用。  
Renewal aims to provide a unified model for source layout, dependency declaration, build mapping, and source retrieval for C++ modular development, so module packages can be described, validated, orchestrated, and reused in a Cargo/Go Modules-like way.

## 文档 | Documents

- [规范草案 | Specification Draft](./docs/spec-draft.md)
- [构建系统介绍 | Build System Overview](./docs/build-system.md)

## 项目定位 | Project Scope

Renewal 当前聚焦一个最小可行的 C++ 模块包管理器，主要负责两件事：  
Renewal currently focuses on a minimal viable C++ module package manager, with two main responsibilities:

- 构建编排：识别包入口、校验依赖、生成 CMake 映射  
  Build orchestration: identify package entry points, validate dependencies, and generate CMake mappings.
- 源码管理：管理包身份、本地路径依赖、版本依赖，以及后续的源码拉取入口  
  Source management: manage package identity, local path dependencies, version dependencies, and future source retrieval entry points.

职责边界：  
Responsibility boundary:

- Renewal：包模型、依赖声明、静态校验、CMake 映射生成  
  Renewal: package model, dependency declaration, static validation, and CMake mapping generation.
- CMake 与编译器：模块扫描、编译顺序推导、实际编译  
  CMake and the compiler: module scanning, compilation order derivation, and actual compilation.
