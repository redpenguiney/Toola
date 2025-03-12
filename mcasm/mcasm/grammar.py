from mcasm import instruction_mcode_generators
from mcasm import directive_handlers

import enum

# tokens

COMMENT_TOKEN = ';'
OPERAND_SPLIT_TOKEN = ' '
BYTE_SPLIT_TOKEN = ','
NULL_TOKEN = "null" # null is an alias for 0 size and no value

VALUE_FIELD_SPLIT_TOKEN = ':'
ARRAY_VALUE_SPLIT_TOKEN = '/'

ENDIANNESS = "big" # mcvm is big endian
STRUCT_PACK_ENDIANNESS = '>' if ENDIANNESS == "big" else '<'

# mcode spec stuff

GLOBAL_SCOPE_SYMBOL = -1

DIRECTIVE_HANDLERS = {
    "label": directive_handlers.handle_label_directive,
    "endfunc": directive_handlers.handle_end_function_directive
}

INSTRUCTION_OPCODES = {
    "dvar": 0, # define variable
    "cvar": 1, # copy variable

    "jmp": 2, # jump
    "sje": 3, # signed integer jump if equal
    "sjne": 4, # signed integer jump if not equal
    "sjg": 5, # signed integer jump if greater than
    "sjge": 6, # signed integer jump if greater than or equal to
    "sjl": 7, # signed integer jump if less than
    "sjle": 8, # signed integer jump if less than or equal to
    "uje": 9, # unsigned integer jump if equal
    "ujne": 10, # unsigned integer jump if not equal
    "ujg": 11, # unsigned integer jump if greater than
    "ujge": 12, # unsigned integer jump if greater than or equal to
    "ujl": 13, # unsigned integer jump if less than
    "ujle": 14, # unsigned integer jump if less than or equal to
    "fje": 15, # float jump if equal
    "fjne": 16, # float jump if not equal
    "fjg": 17, # float jump if greater than
    "fjge": 18, # float jump if greater than or equal to
    "fjl": 19, # float jump if less than
    "fjle": 20, # float jump if less than or equal to
    "dje": 21, # double jump if equal
    "djne": 22, # double jump if not equal
    "djg": 23, # double jump if greater than
    "djge": 24, # double jump if greater than or equal to
    "djl": 25, # double jump if less than
    "djle": 26, # double jump if less than or equal to
    "dfunc": 27, # define function
    "cfunc": 28, # call function
    "cabi": 29, # call ABI function

    "garrl": 30, # get array length
    "garr": 31, # get array value
    "sarr": 32, # set array value
    "aarr": 33, # append array value
    "iarr": 34, # insert array value
    "rarr": 35, # remove array value

    "s2u": 36, # cast signed integer to unsigned integer
    "s2f": 37, # cast signed integer to float
    "s2d": 38, # cast signed integer to double
    "s2str": 39, # cast signed integer to string
    "s2sym": 40, # cast signed integer to symbol
    "u2s": 41, # cast unsigned integer to signed integer
    "u2f": 42, # cast unsigned integer to float
    "u2d": 43, # cast unsigned integer to double
    "u2str": 44, # cast unsigned integer to string
    "u2sym": 45, # cast unsigned integer to symbol
    "f2s": 46, # cast float to signed integer
    "f2u": 47, # cast float to unsigned integer
    "f2d": 48, # cast float to double
    "f2str": 49, # cast float to string
    "d2s": 50, # cast double to signed integer
    "d2u": 51, # cast double to unsigned integer
    "d2f": 52, # cast double to float
    "d2str": 53, # cast double to string
    "sym2s": 54, # cast symbol to signed integer
    "sym2u": 55, # cast symbol to unsigned integer

    "sadd": 56, # signed integer add
    "ssub": 57, # signed integer subtract
    "smul": 58, # signed integer multiply
    "sdiv": 59, # signed integer divide
    "uadd": 60, # unsigned integer add
    "usub": 61, # unsigned integer subtract
    "umul": 62, # unsigned integer multiply
    "udiv": 63, # unsigned integer divide
    "fadd": 64, # float add
    "fsub": 65, # float subtract
    "fmul": 66, # float multiply
    "fdiv": 67, # float divide
    "dadd": 68, # double add
    "dsub": 69, # double subtract
    "dmul": 70, # double multiply
    "ddiv": 71 # double divide
}

INSTRUCTION_MCODE_GENERATORS = { # keys are opcodes (see INSTRUCTION_OPCODES)
    0: instruction_mcode_generators.DefineVariableInstructionMCodeGenerator,
    1: instruction_mcode_generators.CopyVariableInstructionMCodeGenerator,

    2: instruction_mcode_generators.JumpInstructionMCodeGenerator,
    3: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    4: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    5: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    6: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    7: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    8: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    9: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    10: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    11: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    12: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    13: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    14: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    15: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    16: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    17: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    18: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    19: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    20: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    21: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    22: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    23: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    24: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    25: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    26: instruction_mcode_generators.ConditionalJumpInstructionMCodeGenerator,
    27: instruction_mcode_generators.DefineFunctionInstructionMCodeGenerator,
    28: instruction_mcode_generators.CallFunctionInstructionMCodeGenerator,
    29: instruction_mcode_generators.CallABIFunctionInstructionMCodeGenerator,

    30: instruction_mcode_generators.GetArrayLengthInstructionMCodeGenerator,
    31: instruction_mcode_generators.GetArrayValueInstructionMCodeGenerator,
    32: instruction_mcode_generators.SetArrayValueInstructionMCodeGenerator,
    33: instruction_mcode_generators.AppendArrayValueInstructionMCodeGenerator,
    34: instruction_mcode_generators.InsertArrayValueInstructionMCodeGenerator,
    35: instruction_mcode_generators.RemoveArrayValueInstructionMCodeGenerator,

    36: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    37: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    38: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    39: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    40: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    41: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    42: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    43: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    44: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    45: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    46: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    47: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    48: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    49: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    50: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    51: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    52: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    53: instruction_mcode_generators.CastValueToValueInstructionMCodeGenerator,
    54: instruction_mcode_generators.CastSymbolToValueInstructionMCodeGenerator,
    55: instruction_mcode_generators.CastSymbolToValueInstructionMCodeGenerator,
    
    56: instruction_mcode_generators.ArithmeticMCodeGenerator,
    57: instruction_mcode_generators.ArithmeticMCodeGenerator,
    58: instruction_mcode_generators.ArithmeticMCodeGenerator,
    59: instruction_mcode_generators.ArithmeticWithRemainderMCodeGenerator,
    60: instruction_mcode_generators.ArithmeticMCodeGenerator,
    61: instruction_mcode_generators.ArithmeticMCodeGenerator,
    62: instruction_mcode_generators.ArithmeticMCodeGenerator,
    63: instruction_mcode_generators.ArithmeticWithRemainderMCodeGenerator,
    64: instruction_mcode_generators.ArithmeticMCodeGenerator,
    65: instruction_mcode_generators.ArithmeticMCodeGenerator,
    66: instruction_mcode_generators.ArithmeticMCodeGenerator,
    67: instruction_mcode_generators.ArithmeticWithRemainderMCodeGenerator,
    68: instruction_mcode_generators.ArithmeticMCodeGenerator,
    69: instruction_mcode_generators.ArithmeticMCodeGenerator,
    70: instruction_mcode_generators.ArithmeticMCodeGenerator,
    71: instruction_mcode_generators.ArithmeticWithRemainderMCodeGenerator,
}

ABI_SYMBOLS = {
    "logd": 0, # log debug
    "logi": 1, # log info
    "logw": 2, # log warning
    "loge": 3 # log error
}

class ValueType(enum.Enum):
    SIGNED_INTEGER = 0
    UNSIGNED_INTEGER = 1
    FLOAT = 2
    DOUBLE = 3
    STRING = 4
    SYMBOL = 5
    ARRAY = 6

VALUE_TYPE_ALIASES = {
    "sint": ValueType.SIGNED_INTEGER.value, # signed integer
    "uint": ValueType.UNSIGNED_INTEGER.value, # unsigned integer
    "flt": ValueType.FLOAT.value, # float
    "dbl": ValueType.DOUBLE.value, # double
    "str": ValueType.STRING.value, # string
    "sym": ValueType.SYMBOL.value, # symbol
    "arr": ValueType.ARRAY.value # array
}