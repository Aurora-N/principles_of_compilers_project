# principles_of_compilers_project

这是一个基于 Qt/C++ 的《编译原理》课程项目仓库，包含两个核心工具：

- **正则表达式转换器**：正则表达式 → NFA → DFA → 最小化 DFA，并可生成词法分析代码。
- **LR(1) 语法分析器**：文法处理、First/Follow 集计算、LR(0)/LR(1) 自动机构造、LR(1) 分析表与语法分析。

## 仓库结构

```text
principles_of_compilers_project/
├── compiler_tools/               # 主程序集成工程（推荐入口）
│   ├── main.cpp                  # 工具选择入口（Regex / LR1）
│   ├── regexmainwindow.*         # 正则转换器界面
│   ├── lr1mainwindow.*           # LR(1) 分析器界面
│   ├── regex_converter/          # 正则与自动机核心逻辑
│   │   ├── regex.*
│   │   ├── nfa.*
│   │   ├── classifier.*
│   │   └── codegenerate.*
│   └── lr1_analyser/             # LR(1) 文法分析核心逻辑
│       └── grammar.*
├── regex_to_dfa_converter/       # 正则转换器独立工程
├── LR1_analyser/                 # LR(1) 分析器独立工程
├── generated_code_example.cpp    # 生成词法代码示例
└── .gitignore
```

## 各模块说明

### 1) `compiler_tools`（主工程）
- Qt Widgets 应用，统一入口。
- 启动后可在对话框中选择：
  - 正则表达式转换器
  - LR(1) 语法分析器

### 2) `compiler_tools/regex_converter`
负责词法前端相关算法：
- 正则表达式预处理与结构化（`regex.*`）
- NFA / DFA / 最小化 DFA 构造（`nfa.*`, `classifier.*`）
- 词法分析器代码生成（`codegenerate.*`）

### 3) `compiler_tools/lr1_analyser`
负责语法前端相关算法：
- 文法解析与合法性检查
- First / Follow 集计算
- LR(0)/LR(1) 项集族与 DFA 构造
- LR(1) ACTION/GOTO 表构造与冲突信息输出
- 基于分析表的源代码语法分析与语法树构建

### 4) `regex_to_dfa_converter` 与 `LR1_analyser`
这两个目录分别是独立版本的子工具工程，与 `compiler_tools` 中对应模块能力相近，可单独构建运行。

## 构建说明（Qt / qmake）

建议使用 Qt Creator 直接打开以下任一 `.pro` 文件：

- `compiler_tools/compiler_tools.pro`（推荐）
- `regex_to_dfa_converter/regex_to_dfa_converter.pro`
- `LR1_analyser/LR1_analyser.pro`

或使用命令行（以主工程为例）：

```bash
cd compiler_tools
qmake compiler_tools.pro
make
```

## 适用场景

- 编译原理课程实验与演示
- 词法/语法分析算法学习
- 从正则到可执行词法代码的快速验证
