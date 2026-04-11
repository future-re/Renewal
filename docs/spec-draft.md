# Renewal 规范草案 | Specification Draft

## 1. 包结构 | Package Layout

最小包结构：  
Minimal package layout:

```text
json/
  pkg.toml
  modules/
    json/
      mod.cppm
      parse.cpp
      write.cpp
  tests/
```

含多个模块单元的包可以组织成这样：  
A package containing multiple module units can be organized like this:

```text
json/
  pkg.toml
  modules/
    json/
      mod.cppm             # 统一导出模块：export module json;
      reader.cppm          # 其他模块接口单元：export module json.reader;
      writer.cppm          # 其他模块接口单元：export module json.writer;
      detail/
        lexer.cppm         # 内部分区或内部模块单元
        parser.cppm
      parse.cpp            # 普通实现文件
      write.cpp            # 普通实现文件
      format.cpp           # 普通实现文件
  tests/
```

- `modules/<package.name>/mod.cppm` 是固定的统一导出模块  
  `modules/<package.name>/mod.cppm` is the fixed unified export module.
- 包内可以存在其他 `.cppm` 文件，它们可以是子模块、分区，或者其他模块接口单元  
  Additional `.cppm` files may exist inside the package as submodules, partitions, or other module interface units.
- 普通 `.cpp` 文件继续作为实现文件存在  
  Regular `.cpp` files remain normal implementation files.
- 包内对外 API 必须通过模块导出，不使用头文件接口  
  Public APIs must be exposed through modules, not header-based interfaces.
- 包级默认入口固定为 `import <package.name>;`  
  The default package-level entry is fixed as `import <package.name>;`.

workspace 示例：  
Workspace example:

```text
workspace/
  modules/
    core/
      pkg.toml
      modules/core/mod.cppm
    http/
      pkg.toml
      modules/http/mod.cppm
  external/
    json/
      pkg.toml
      modules/json/mod.cppm
```

## 2. `pkg.toml` 规范 v0.1 | `pkg.toml` Spec v0.1

`pkg.toml` 是 Renewal 的核心清单文件，用来描述一个 C++ 模块包。  
`pkg.toml` is the core manifest file of Renewal and describes a C++ module package.

设计借鉴：  
Design references:

- **Cargo**：一个 `Cargo.toml` 描述编译包需要的元数据和依赖  
  **Cargo**: a `Cargo.toml` describes the metadata and dependencies needed to build a package.
- **Go Modules**：一个 `go.mod` 定义模块身份和依赖  
  **Go Modules**: a `go.mod` defines module identity and dependencies.
- **CMake**：模块接口源通过 `target_sources(... FILE_SET ... TYPE CXX_MODULES ...)` 进入构建  
  **CMake**: module interface sources enter the build via `target_sources(... FILE_SET ... TYPE CXX_MODULES ...)`.

## 3. 顶层结构 | Top-Level Structure

`pkg.toml` 只允许以下顶层表：  
`pkg.toml` only allows the following top-level tables:

```toml
[package]
[deps]
[build]
[metadata]
```

| 表 | 说明 |
|---|---|
| `[package]` | 包身份 |
| `[deps]` | 包级依赖 |
| `[build]` | 构建映射 |
| `[metadata]` | 可选扩展，不参与核心构建语义 |

| Table | Description |
|---|---|
| `[package]` | Package identity |
| `[deps]` | Package-level dependencies |
| `[build]` | Build mapping |
| `[metadata]` | Optional extension fields outside core build semantics |

## 4. `[package]` 表 | `[package]` Table

### 必填字段 | Required Fields

```toml
[package]
name = "json"
version = "0.1.0"
edition = "c++23"
```

### 字段定义 | Field Definitions

#### `name`

- **类型**: `string`
- **必填**: 是
- **规则**:
  - 只能包含：`[A-Za-z0-9_-]`
  - 不能为空
  - 在整个 workspace 内唯一

- **Type**: `string`
- **Required**: yes
- **Rules**:
  - May only contain `[A-Za-z0-9_-]`
  - Must not be empty
  - Must be unique within the workspace

#### `version`

- **类型**: `string`
- **必填**: 是
- **规则**:
  - 第一版要求使用 semver 形态：`MAJOR.MINOR.PATCH`
  - 例如：`0.1.0`、`1.2.3`

- **Type**: `string`
- **Required**: yes
- **Rules**:
  - v0.1 requires semantic version form: `MAJOR.MINOR.PATCH`
  - Example: `0.1.0`, `1.2.3`

#### `edition`

- **类型**: `string`
- **必填**: 是
- **允许值**:
  - `"c++20"`
  - `"c++23"`
  - `"c++26"`

- **Type**: `string`
- **Required**: yes
- **Allowed values**:
  - `"c++20"`
  - `"c++23"`
  - `"c++26"`

### 可选字段 | Optional Fields

```toml
description = "JSON parser module"
license = "MIT"
repository = "https://example.com/json"
```

| 字段 | 类型 |
|---|---|
| `description` | `string` |
| `license` | `string` |
| `repository` | `string` |

| Field | Type |
|---|---|
| `description` | `string` |
| `license` | `string` |
| `repository` | `string` |

## 5. `[deps]` 表 | `[deps]` Table

定义这个包依赖哪些别的包。第一版只支持两种依赖形式。  
Defines which other packages this package depends on. v0.1 supports only two dependency forms.

### 形式 A：本地路径依赖 | Form A: Local Path Dependency

```toml
[deps]
core = { path = "../../modules/core" }
json = { path = "../../external/json" }
```

规则：  
Rules:

- `path` 必须是相对路径  
  `path` must be a relative path.
- 指向另一个包根目录  
  It must point to another package root directory.
- 目标目录中必须存在 `pkg.toml`  
  The target directory must contain a `pkg.toml`.

### 形式 B：版本依赖 | Form B: Version Dependency

```toml
[deps]
json = "0.1.0"
core = "1.2.0"
```

规则：  
Rules:

- 先只接受精确版本  
  Only exact versions are accepted in v0.1.
- 具体如何解析版本由后续的源码拉取器负责  
  Concrete version resolution is handled by a future source retriever.

### 强制一致性规则 | Mandatory Consistency Rule

如果源码中出现跨包导入：  
If cross-package imports appear in source code:

```cpp
import core;
import json;
```

则 `[deps]` 必须包含：  
Then `[deps]` must include:

```toml
[deps]
core = ...
json = ...
```

否则校验失败。  
Otherwise validation fails.

## 6. `[build]` 表 | `[build]` Table

把包映射到 CMake target，并承载 `mod.cppm` 之外的构建输入。  
Maps the package to a CMake target and carries build inputs other than `mod.cppm`.

### 必填字段 | Required Fields

```toml
[build]
target = "pkg_json"
type = "module_library"
```

### 字段定义 | Field Definitions

#### `target`

- **类型**: `string`
- **必填**: 是
- **规则**:
  - 全 workspace 唯一
  - 推荐格式：`pkg_<package.name>`

- **Type**: `string`
- **Required**: yes
- **Rules**:
  - Must be unique within the workspace
  - Recommended format: `pkg_<package.name>`

#### `type`

- **类型**: `string`
- **必填**: 是
- **第一版允许值**:
  - `"module_library"`

- **Type**: `string`
- **Required**: yes
- **Allowed values in v0.1**:
  - `"module_library"`

### 可选字段 | Optional Fields

```toml
sources = [
  "modules/json/parse.cpp",
  "modules/json/write.cpp"
]
```

#### `sources`

- **类型**: `array[string]`
- **必填**: 否
- **用途**:
  - 作为普通实现源加入 target
  - 只列出 `mod.cppm` 之外需要参与构建的源码

- **Type**: `array[string]`
- **Required**: no
- **Purpose**:
  - Added to the target as regular implementation sources
  - Lists only sources other than `mod.cppm` that participate in the build

统一导出模块文件 `modules/<package.name>/mod.cppm` 不需要在 `pkg.toml` 中重复声明，而是由包名直接推导。  
The unified export module file `modules/<package.name>/mod.cppm` does not need to be repeated in `pkg.toml`; it is derived directly from the package name.

## 7. `[metadata]` 表 | `[metadata]` Table

预留扩展，不参与核心构建语义。  
Reserved for extensions and not part of core build semantics.

```toml
[metadata]
authors = ["runtime-team"]
tags = ["json", "parser"]
```

第一版可以完全忽略这一节。  
This section can be completely ignored in v0.1.

## 8. 强制校验规则 | Mandatory Validation Rules

### 包级规则 | Package Rules

| 规则 | 说明 |
|---|---|
| **R1** | 同一个 workspace 内，`package.name` 不可重复 |
| **R2** | 同一个 workspace 内，`build.target` 不可重复 |
| **R3** | 一个包只有一个统一导出模块文件 `modules/<package.name>/mod.cppm` |

| Rule | Description |
|---|---|
| **R1** | `package.name` must be unique within the workspace |
| **R2** | `build.target` must be unique within the workspace |
| **R3** | A package has exactly one unified export module file at `modules/<package.name>/mod.cppm` |

### 模块级规则 | Module Rules

| 规则 | 说明 |
|---|---|
| **R4** | 包的统一导出模块名由 `package.name` 唯一确定 |
| **R5** | `modules/<package.name>/mod.cppm` 必须存在且是 `.cppm` |
| **R6** | `modules/<package.name>/mod.cppm` 中的 `export module` 必须与 `package.name` 一致 |
| **R7** | `mod.cppm` 是包的统一导出模块，外部默认通过 `import <package.name>;` 使用该包 |

| Rule | Description |
|---|---|
| **R4** | The unified export module name is uniquely determined by `package.name` |
| **R5** | `modules/<package.name>/mod.cppm` must exist and must be a `.cppm` file |
| **R6** | The `export module` declaration in `modules/<package.name>/mod.cppm` must match `package.name` |
| **R7** | `mod.cppm` is the unified export module, and external users consume the package via `import <package.name>;` |

### 依赖级规则 | Dependency Rules

| 规则 | 说明 |
|---|---|
| **R8** | 所有跨包 `import` 必须在 `[deps]` 中声明 |
| **R9** | 不允许导入未声明包 |
| **R10** | 不允许循环包依赖 |

| Rule | Description |
|---|---|
| **R8** | All cross-package `import`s must be declared in `[deps]` |
| **R9** | Importing undeclared packages is not allowed |
| **R10** | Cyclic package dependencies are not allowed |

### 语言/工具规则 | Language / Tooling Rules

| 规则 | 说明 |
|---|---|
| **R11** | 第一版禁止 header units |
| **R12** | 第一版禁止 `import std` |
| **R13** | 模块包内禁止业务头文件接口作为跨包 API 面 |

| Rule | Description |
|---|---|
| **R11** | Header units are forbidden in v0.1 |
| **R12** | `import std` is forbidden in v0.1 |
| **R13** | Business header interfaces must not be used as cross-package APIs |

## 9. 完整示例 | Full Examples

### `external/json/pkg.toml`

```toml
[package]
name = "json"
version = "0.1.0"
edition = "c++23"
description = "JSON parser module"
license = "MIT"

[deps]

[build]
target = "pkg_json"
type = "module_library"
sources = [
  "modules/json/parse.cpp",
  "modules/json/write.cpp"
]

[metadata]
authors = ["runtime-team"]
```

### `modules/http/pkg.toml`

```toml
[package]
name = "http"
version = "0.2.0"
edition = "c++23"

[deps]
core = { path = "../../modules/core" }
json = { path = "../../external/json" }

[build]
target = "pkg_http"
type = "module_library"
sources = [
  "modules/http/request.cpp",
  "modules/http/response.cpp"
]
```

## 参考链接 | References

- [The Manifest Format - The Cargo Book](https://doc.rust-lang.org/cargo/reference/manifest.html)
- [Go Modules Reference](https://go.dev/ref/mod)
- [target_sources — CMake Documentation](https://cmake.org/cmake/help/latest/command/target_sources.html)
- [Specifying Dependencies - The Cargo Book](https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html)
- [cmake-cxxmodules(7) — CMake Documentation](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html)
