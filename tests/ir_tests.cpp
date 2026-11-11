#include "compiler/ir/ir.hpp"
#include "compiler/ir/ir_printer.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

//done
TEST(IRPrinterTest, ConstantReturn) {

    compiler::ir::BasicBlock block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::ConstI64{
                compiler::ir::ValueId{0},
                42
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{0}
        }
    };

    compiler::ir::IRFunction function{
        .id = compiler::ir::FunctionId{0},
        .name = "main",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {},
        .parameter_locals = {},
        .locals = {},
        .blocks = {block}
    };

    compiler::ir::IRProgram program{
        .functions = {function}
    };

    const std::string expected = R"(fn @main() -> i64 {
bb0:
    %v0 = const.i64 42
    ret %v0
}
)";

    EXPECT_EQ(compiler::ir::dump_ir(program), expected);
}

//done
TEST(IRPrinterTest, BooleanConstant) {

    compiler::ir::BasicBlock block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::ConstBool{
                compiler::ir::ValueId{0},
                true
            },
            compiler::ir::ConstBool{
                compiler::ir::ValueId{1},
                false
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{0}
        }
    };

    compiler::ir::IRFunction function{
        .id = compiler::ir::FunctionId{0},
        .name = "main",
        .return_type = compiler::ir::IRType::Bool,
        .parameter_types = {},
        .parameter_locals = {},
        .locals = {},
        .blocks = {block}
    };

    compiler::ir::IRProgram program{
        .functions = {function}
    };


    const std::string expected = R"(fn @main() -> bool {
bb0:
    %v0 = const.bool true
    %v1 = const.bool false
    ret %v0
}
)";

    EXPECT_EQ(compiler::ir::dump_ir(program), expected);

}
//done
TEST(IRPrinterTest, BinaryArithmetic) {

    compiler::ir::BasicBlock block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::ConstI64{
                compiler::ir::ValueId{0},
                10
            },
            compiler::ir::ConstI64{
                compiler::ir::ValueId{1},
                5
            },
            compiler::ir::Binary{
                compiler::ir::ValueId{2},
                compiler::ir::BinaryOp::Add, 
                compiler::ir::ValueId{0}, 
                compiler::ir::ValueId{1}
            },
            compiler::ir::ConstI64{
                compiler::ir::ValueId{3},
                2
            },
            compiler::ir::Binary{
                compiler::ir::ValueId{4},
                compiler::ir::BinaryOp::Multiply, 
                compiler::ir::ValueId{2}, 
                compiler::ir::ValueId{3}
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{4}
        }
    };
    
    compiler::ir::IRFunction function{
        .id = compiler::ir::FunctionId{0},
        .name = "main",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {},
        .parameter_locals = {},
        .locals = {},
        .blocks = {block}
    };

    compiler::ir::IRProgram program{
        .functions = {function}
    };

    const std::string expected = R"(fn @main() -> i64 {
bb0:
    %v0 = const.i64 10
    %v1 = const.i64 5
    %v2 = add %v0, %v1
    %v3 = const.i64 2
    %v4 = mul %v2, %v3
    ret %v4
}
)";

    EXPECT_EQ(compiler::ir::dump_ir(program), expected);
}
//done
TEST(IRPrinterTest, LocalLoadStore) {

    compiler::ir::BasicBlock block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::ConstI64{
                compiler::ir::ValueId{0},
                10
            },
            compiler::ir::StoreLocal{
                compiler::ir::LocalId{0},
                compiler::ir::ValueId{0}
            },
            compiler::ir::LoadLocal{
                compiler::ir::ValueId{1},
                compiler::ir::LocalId{0}
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{1}
        }
    };
    
    compiler::ir::IRFunction function{
        .id = compiler::ir::FunctionId{0},
        .name = "main",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {},
        .parameter_locals = {},
        .locals = {
            compiler::ir::Local{
                compiler::ir::LocalId{0},
                compiler::ir::IRType::I64,
                "x"
            }
        },
        .blocks = {block}
    };

    compiler::ir::IRProgram program{
        .functions = {function}
    };


    const std::string expected = R"(fn @main() -> i64 {
bb0:
    %v0 = const.i64 10
    store $l0, %v0
    %v1 = load $l0
    ret %v1
}
)";

    EXPECT_EQ(compiler::ir::dump_ir(program), expected);
}
//
TEST(IRPrinterTest, FunctionParameters) {

    compiler::ir::BasicBlock block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::LoadLocal{
                compiler::ir::ValueId{0},
                compiler::ir::LocalId{0}
            },
            compiler::ir::LoadLocal{
                compiler::ir::ValueId{1},
                compiler::ir::LocalId{1}
            },
            compiler::ir::Binary{
                compiler::ir::ValueId{2},
                compiler::ir::BinaryOp::Add, 
                compiler::ir::ValueId{0}, 
                compiler::ir::ValueId{1}
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{2}
        }
    };

    compiler::ir::IRFunction function{
        .id = compiler::ir::FunctionId{0},
        .name = "add",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {compiler::ir::IRType::I64, compiler::ir::IRType::I64},
        .parameter_locals = {
            compiler::ir::LocalId{0},
            compiler::ir::LocalId{1}
        },
        .locals = {
            compiler::ir::Local{
                compiler::ir::LocalId{0},
                compiler::ir::IRType::I64,
                "x"
            },
            compiler::ir::Local{
                compiler::ir::LocalId{1},
                compiler::ir::IRType::I64,
                "y"
            }
        },
        .blocks = {block}
    };

    compiler::ir::IRProgram program{
        .functions = {function}
    };

    const std::string expected = R"(fn @add($l0: i64, $l1: i64) -> i64 {
bb0:
    %v0 = load $l0
    %v1 = load $l1
    %v2 = add %v0, %v1
    ret %v2
}
)";

    EXPECT_EQ(compiler::ir::dump_ir(program), expected);
}


TEST(IRPrinterTest, ConditionalBranch) {
    compiler::ir::BasicBlock entry{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::LoadLocal{
                compiler::ir::ValueId{0},
                compiler::ir::LocalId{0}
            }
        },
        .terminator = compiler::ir::CondBranchTerminator{
            compiler::ir::ValueId{0},
            compiler::ir::BlockId{1},
            compiler::ir::BlockId{2}
        }
    };

    compiler::ir::BasicBlock true_block{
        .id = compiler::ir::BlockId{1},
        .debug_name = "true",
        .instructions = {
            compiler::ir::ConstI64{
                compiler::ir::ValueId{1},
                10
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{1}
        }
    };

    compiler::ir::BasicBlock false_block{
        .id = compiler::ir::BlockId{2},
        .debug_name = "false",
        .instructions = {
            compiler::ir::ConstI64{
                compiler::ir::ValueId{2},
                20
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{2}
        }
    };

    compiler::ir::IRFunction function{
        .id = compiler::ir::FunctionId{0},
        .name = "choose",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {compiler::ir::IRType::Bool},
        .parameter_locals = {compiler::ir::LocalId{0}},
        .locals = {
            compiler::ir::Local{
                compiler::ir::LocalId{0},
                compiler::ir::IRType::Bool,
                "flag"
            }
        },
        .blocks = {entry, true_block, false_block}
    };

    compiler::ir::IRProgram program{
        .functions = {function}
    };

    const std::string expected = R"(fn @choose($l0: bool) -> i64 {
bb0:
    %v0 = load $l0
    br_if %v0, bb1, bb2
bb1:
    %v1 = const.i64 10
    ret %v1
bb2:
    %v2 = const.i64 20
    ret %v2
}
)";

    EXPECT_EQ(compiler::ir::dump_ir(program), expected);
}

TEST(IRPrinterTest, FunctionCalls) {
    compiler::ir::BasicBlock identity_block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::LoadLocal{
                compiler::ir::ValueId{0},
                compiler::ir::LocalId{0}
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{0}
        }
    };

    compiler::ir::IRFunction identity{
        .id = compiler::ir::FunctionId{0},
        .name = "identity",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {compiler::ir::IRType::I64},
        .parameter_locals = {compiler::ir::LocalId{0}},
        .locals = {
            compiler::ir::Local{
                compiler::ir::LocalId{0},
                compiler::ir::IRType::I64,
                "x"
            }
        },
        .blocks = {identity_block}
    };

    compiler::ir::BasicBlock log_block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {},
        .terminator = compiler::ir::ReturnTerminator{
            std::nullopt
        }
    };

    compiler::ir::IRFunction log_value{
        .id = compiler::ir::FunctionId{1},
        .name = "log_value",
        .return_type = compiler::ir::IRType::Void,
        .parameter_types = {compiler::ir::IRType::I64},
        .parameter_locals = {compiler::ir::LocalId{0}},
        .locals = {
            compiler::ir::Local{
                compiler::ir::LocalId{0},
                compiler::ir::IRType::I64,
                "x"
            }
        },
        .blocks = {log_block}
    };

    compiler::ir::BasicBlock main_block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::ConstI64{
                compiler::ir::ValueId{0},
                7
            },
            compiler::ir::Call{
                compiler::ir::ValueId{1},
                compiler::ir::FunctionId{0},
                {compiler::ir::ValueId{0}}
            },
            compiler::ir::Call{
                std::nullopt,
                compiler::ir::FunctionId{1},
                {compiler::ir::ValueId{1}}
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{1}
        }
    };

    compiler::ir::IRFunction main_function{
        .id = compiler::ir::FunctionId{2},
        .name = "main",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {},
        .parameter_locals = {},
        .locals = {},
        .blocks = {main_block}
    };

    compiler::ir::IRProgram program{
        .functions = {identity, log_value, main_function}
    };

    const std::string expected = R"(fn @identity($l0: i64) -> i64 {
bb0:
    %v0 = load $l0
    ret %v0
}

fn @log_value($l0: i64) -> void {
bb0:
    ret
}

fn @main() -> i64 {
bb0:
    %v0 = const.i64 7
    %v1 = call @f0(%v0)
    call @f1(%v1)
    ret %v1
}
)";

    EXPECT_EQ(compiler::ir::dump_ir(program), expected);
}

TEST(IRPrinterTest, UnterminatedBlock) {
    compiler::ir::BasicBlock block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::ConstI64{
                compiler::ir::ValueId{0},
                42
            }
        },
        .terminator = std::nullopt
    };

    compiler::ir::IRFunction function{
        .id = compiler::ir::FunctionId{0},
        .name = "main",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {},
        .parameter_locals = {},
        .locals = {},
        .blocks = {block}
    };

    compiler::ir::IRProgram program{
        .functions = {function}
    };

    const std::string expected = R"(fn @main() -> i64 {
bb0:
    %v0 = const.i64 42
    <unterminated>
}
)";

    EXPECT_EQ(compiler::ir::dump_ir(program), expected);
}

TEST(IRPrinterTest, MultipleFunctions) {
    compiler::ir::BasicBlock first_block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::ConstI64{
                compiler::ir::ValueId{0},
                10
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{0}
        }
    };

    compiler::ir::BasicBlock second_block{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {
            compiler::ir::ConstI64{
                compiler::ir::ValueId{0},
                20
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{0}
        }
    };

    compiler::ir::IRFunction first{
        .id = compiler::ir::FunctionId{0},
        .name = "first",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {},
        .parameter_locals = {},
        .locals = {},
        .blocks = {first_block}
    };

    compiler::ir::IRFunction second{
        .id = compiler::ir::FunctionId{1},
        .name = "second",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {},
        .parameter_locals = {},
        .locals = {},
        .blocks = {second_block}
    };

    compiler::ir::IRProgram program{
        .functions = {first, second}
    };

    const std::string expected = R"(fn @first() -> i64 {
bb0:
    %v0 = const.i64 10
    ret %v0
}

fn @second() -> i64 {
bb0:
    %v0 = const.i64 20
    ret %v0
}
)";

    EXPECT_EQ(compiler::ir::dump_ir(program), expected);
}

TEST(IRPrinterTest, UnconditionalBranch) {
    compiler::ir::BasicBlock entry{
        .id = compiler::ir::BlockId{0},
        .debug_name = "entry",
        .instructions = {},
        .terminator = compiler::ir::BranchTerminator{
            compiler::ir::BlockId{1}
        }
    };

    compiler::ir::BasicBlock exit{
        .id = compiler::ir::BlockId{1},
        .debug_name = "exit",
        .instructions = {
            compiler::ir::ConstI64{
                compiler::ir::ValueId{0},
                42
            }
        },
        .terminator = compiler::ir::ReturnTerminator{
            compiler::ir::ValueId{0}
        }
    };

    compiler::ir::IRFunction function{
        .id = compiler::ir::FunctionId{0},
        .name = "main",
        .return_type = compiler::ir::IRType::I64,
        .parameter_types = {},
        .parameter_locals = {},
        .locals = {},
        .blocks = {entry, exit}
    };

    compiler::ir::IRProgram program{
        .functions = {function}
    };

    const std::string expected = R"(fn @main() -> i64 {
bb0:
    br bb1
bb1:
    %v0 = const.i64 42
    ret %v0
}
)";

    EXPECT_EQ(compiler::ir::dump_ir(program), expected);
}

