# VGA Debugger Generator

使用 FPGA 板上的 VGA 显示屏显示 Verilog 模块中的一些线的值来辅助调式与展示是一个很好的想法，然而，为了显示模块中的一些本来没有传出模块的线，需要编写大量模板化的代码，包括：

* 在模块的输出部分添加若干行 `output wire dbg_xxx,`
* 在模块内部添加若干行 `assign dbg_xxx = xxx;`
* 在模块的父模块中添加若干行 `wire dbg_xxx;`
* 在实例化模块时添加若干行 `.dbg_xxx(dbg_xxx),`

本身复制修改这些代码就容易出现错误，而且非常浪费时间，在需要显示的模块有多层层级关系时更是如此。

本项目旨在简化该流程，提供一个 VGA 显示模板文件和一个配置文件，程序就会根据其中的内容生成以上四个部分的若干代码到一个 `.vh` 文件中，在 Verilog 代码中只需包含该头文件，在合适的位置添加如下几个宏即可：

* 模块定义的输出部分：``VGA_DBG_ModuleName_Outputs`
* 模块内部的连接部分：``VGA_DBG_ModuleName_Assignments`
* 父模块内的定义部分：``VGA_DBG_ModuleName_Declaration`
* 模块实例化的传参部分：``VGA_DBG_ModuleName_Arguments`

当需要显示的线发生了更改，只需重新生成文件，模块本身的代码也无需修改。

此外，还会生成一个 `VgaDebugger.v`，配合本项目中的 `VgaController.v` 和 `VgaDisplay.v` 使用，适当实例化这些模块就可以进行显示了。以上提供的代码支持的是 640x480、12 位色（RGB 各 4 位）的显示，需要其他 VGA 显示格式的话，需要修改 `VgaController.v` 和 `VgaDisplay.v` 中的一些部分，生成的 `VgaDebugger.v` 应该不受影响。

（文档待补充）

## 模板文件

模板文件是一个纯文本文件，如下是一个例子：

```
RV32I Pipelined CPU

====================================== If ======================================
pc: 00000000
====================================== Id ======================================
pc: 00000000   inst: 00000000
x0: 00000000   ra: 00000000   sp: 00000000   gp: 00000000   tp: 00000000
t0: 00000000   t1: 00000000   t2: 00000000   s0: 00000000   s1: 00000000
a0: 00000000   a1: 00000000   a2: 00000000   a3: 00000000   a4: 00000000
a5: 00000000   a6: 00000000   a7: 00000000   s2: 00000000   s3: 00000000
s4: 00000000   s5: 00000000   s6: 00000000   s7: 00000000   s8: 00000000
s9: 00000000  s10: 00000000  s11: 00000000   t3: 00000000   t4: 00000000
t5: 00000000   t6: 00000000
====================================== Ex ======================================
pc: 00000000   inst: 00000000
rd: 00   rs1: 00   rs2: 00   is_imm: 0   rs1_val: 00000000   rs2_val: 00000000
imm: 00000000   forward_rs1: 00000000   forward_rs2: 00000000
reg_wen: 0   mem_wen: 0   mem_ren: 0   is_branch: 0   is_jal: 0   is_jalr: 0
is_auipc: 0   is_lui: 0   alu_ctrl: 0   cmp_ctrl: 0
====================================== Ma ======================================
pc: 00000000   inst: 00000000
rd: 00   reg_wen: 0   mem_i_data: 00000000
alu_res: 00000000   mem_wen: 0   mem_ren: 0   is_jal: 0   is_jalr: 0
====================================== Wb ======================================
pc: 00000000   inst: 00000000
rd: 00   reg_wen: 0   reg_i_data: 00000000
```

其中 `== block_name ==` 标识了一个块，其下的 `wire: 00000000` 表示 Verilog 代码中一个名为 `wire` 的线，`0` 是显示其十六进制值的占位符。

程序使用占位符 `0` 的个数决定一个线的宽度，一个 `0` 会被认为是 1 个 bit 宽的线，否则认为是 4 倍的 `0` 的数目个 bits 宽。如果实际位宽与之不符，可以在配置文件中设置。

一个块下的线在代码中可以有统一的前后缀，如代码中使用 `pre_name_suf` 的名字来定义这些线，可以在 VGA 上只显示 `name` 的名字，然后在配置文件中指明前后缀。也可以为单一的线指明前后缀。

示例的模板文件中，`Ex` 块中的各个线在代码中的名字就是 `IdEx_xxx`，其中也有一些的前缀是 `Ex_`，如 `forward_rs1`。具体可以参考 `config_example` 中的文件。

## 配置文件

配置文件为一个 `.json` 文件，下面给出了所有支持的配置。具体的使用可以参考 `config_example` 中的文件。

```json
{
    "module_name": "top_module_name", // required
    "template_file": "place of template file", // required
    "output_dir": "output directory",
    "mem_file": "name of outoput .mem file", // required
    "dbg_header": "name of outoput .vh file", // required
    "header_lines": 1, // top 'header_lines' lines will be considered constant
    "template_width": 80, // 640x480 and 8x16 per char, so 80
    "template_height": 30, // 640x480 and 8x16 per char, so 80
    "block_prefix": {
        "block1": "block1_prefix",
        "block2": "block2_prefix"
    },
    "block_suffix": {
        "block1": "block1_suffix",
        "block2": "block2_suffix"
    },
    "len_bits": { // actually length in bits of wires
        "block1": {
            "wire1": 5,
            "wire2": 3
        },
        "block2": "*group1" // "*group_name" refer to a group of wires, this can only be used in "len_bits", "wire_prefix", "wire_suffix" and "submodule"
    },
    "wire_prefix": {
        "*group1": "group1_prefix",
        "block2": {
            "wire1": "block2_wire1_prefix",
            "wire2": "block2_wire2_prefix"
        }
    },
    "wire_suffix": {
        "*group1": "group1_suffix",
        "block2": {
            "wire1": "block2_wire1_suffix",
            "wire2": "block2_wire2_suffix"
        }
    },
    "submodule": [
        {
            "name": "submodule1",
            "wires": "*group1"
        },
        {
            "name": "submodule2",
            "parent": "submodule1", // "module_name" by defauly
            "wires": {
                "block3": [ "wire1", "wire2", "wire3" ]
            }
        }
    ],
    "wire_group": [
        {
        	"name": "group1",
            "wires": {
                "block1": [ "wire4", "wire5", "wire6" ],
                "block2": [ "wire4", "wire5" ],
            }
        }
    ]
}
```


## 示例 - 流水线 CPU

配置文件和模板文件在 `config_example` 中。

在 Sword V4 上的运行结果：

![](./doc/pic/example_pipe_cpu.jpg)

