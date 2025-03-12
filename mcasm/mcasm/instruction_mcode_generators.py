from mcasm import operand_mcode_generators
from mcasm import grammar
from mcasm import context
from mcasm import packing
from mcasm import utils

# base that other generators inherit from
class BaseInstructionMCodeGenerator:
    def __init__(self, ctx, instruction_opcode, unparsed_instruction_operands):
        self.ctx = ctx
        self.instruction_opcode = instruction_opcode
        self.unparsed_instruction_operands = unparsed_instruction_operands

        # set by derived instructions
        self.instruction_operand_mcode_generators = []

    def generate(self):
        # generate instruction operands
        instruction_operands_mcode = []
        for instruction_operand_mcode_generator in self.instruction_operand_mcode_generators:
            instruction_operands_mcode.extend(instruction_operand_mcode_generator.generate())

        instruction_operands_mcode_size = len(instruction_operands_mcode)
        instruction_mcode = [
            # instruction header
            *packing.pack_1_byte_unsigned_integer(self.instruction_opcode), # opcode
            # size
            *packing.pack_1_byte_unsigned_integer(utils.get_size_of_unsigned_integer(instruction_operands_mcode_size)),
            *packing.pack_n_byte_unsigned_integer(instruction_operands_mcode_size),
            # operands
            *instruction_operands_mcode
        ]
        
        return instruction_mcode

# variable instructions

class DefineVariableInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # get symbol
        new_variable_symbol = operand_mcode_generators.VariableSymbolOperandMCodeGenerator(
            self.ctx, self.unparsed_instruction_operands[0], create=True # create symbol
        )

        # define variable in scope
        self.ctx.scopes[self.ctx.scope_symbol].variable_symbols[new_variable_symbol.name] = new_variable_symbol.value

        self.instruction_operand_mcode_generators = [
            new_variable_symbol, # symbol
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ) # value
        ]

class CopyVariableInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # to
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ) # from
        ]

# control flow instructions

class JumpInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.JumpByteIndexOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ) # index
        ]

class ConditionalJumpInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.JumpByteIndexOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # index
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ), # value A
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[2]
            ) # value B
        ]

class DefineFunctionInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # get symbol and size
        new_function_symbol = operand_mcode_generators.FunctionSymbolOperandMCodeGenerator(
            self.ctx, self.unparsed_instruction_operands[0], create=True # create symbol
        )

        # define function in scope
        current_scope = self.ctx.scopes[self.ctx.scope_symbol]
        current_scope.function_symbols[new_function_symbol.name] = new_function_symbol.value

        current_scope = self.ctx.scopes[self.ctx.scope_symbol]
        # create new scope for duration of function execution
        new_scope = context.Scope( # pass vars and funcs
            # shallow copy to keep references
            current_scope.variable_symbols.copy(),
            current_scope.function_symbols.copy(),
            function=self, # it's our scope
            enclosing_scope_symbol=self.ctx.scope_symbol, # scope to return to when this one dies
        )
        # add new scope to scopes and set it as current one
        self.ctx.scopes[new_function_symbol.value] = new_scope
        self.ctx.scope_symbol = new_function_symbol.value

        # create function requirements
        new_function_requirements = operand_mcode_generators.FunctionRequirementArrayOperandMCodeGenerator(
            self.ctx, self.unparsed_instruction_operands[1], create=True
        )
        new_function_requirement_values = new_function_requirements.values
        for new_function_requirement_value in new_function_requirement_values:
            new_scope.variable_symbols[new_function_requirement_value.name] = new_function_requirement_value.symbol

        # define function requirements in scope

        self.instruction_operand_mcode_generators = [
            new_function_symbol, # symbol
            None, # size is determined later
            new_function_requirements # requirements
        ]

class CallFunctionInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.FunctionSymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # symbol
            operand_mcode_generators.ValueArrayOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ) # arguments
        ]

class CallABIFunctionInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.ABISymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # ABI symbol
            operand_mcode_generators.ValueArrayOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ) # arguments
        ]

# array instructions

class GetArrayLengthInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # symbol
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ) # result
        ]

class GetArrayValueInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # symbol
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ), # index
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[2]
            ) # result
        ]

class SetArrayValueInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # symbol
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ), # index
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[2]
            ) # value
        ]

class AppendArrayValueInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # symbol
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ) # value
        ]

class InsertArrayValueInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # symbol
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ), # index
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[2]
            ) # value
        ]

class RemoveArrayValueInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # symbol
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ) # index
        ]

# casting instructions

class CastValueToValueInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # value
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ) # result
        ]

class CastSymbolToValueInstructionMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # generic symbol
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ) # result
        ]

# arithmetic instructions

class ArithmeticMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # value A
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ), # value B
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[2]
            ) # result
        ]

class ArithmeticWithRemainderMCodeGenerator(BaseInstructionMCodeGenerator):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.instruction_operand_mcode_generators = [
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[0]
            ), # value A
            operand_mcode_generators.ValueOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[1]
            ), # value B
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[2]
            ), # result
            operand_mcode_generators.SymbolOperandMCodeGenerator(
                self.ctx, self.unparsed_instruction_operands[3]
            ) # remainder
        ]