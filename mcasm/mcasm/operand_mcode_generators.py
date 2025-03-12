from mcasm import grammar
from mcasm import packing
from mcasm import utils

import sys

# operands

class ValueOperandMCodeGenerator:
    def __init__(self, ctx, unparsed_operand):
        # parse
        value_fields = unparsed_operand.split(grammar.VALUE_FIELD_SPLIT_TOKEN)
        type_field = value_fields[0]
        value_field = grammar.VALUE_FIELD_SPLIT_TOKEN.join(value_fields[1:])

        self.type = grammar.VALUE_TYPE_ALIASES[type_field]

        # handle symbol values
        # this also resolves symbol values for non-symbol types
        non_symbol_type_value_is_symbol = ctx.scopes[ctx.scope_symbol].variable_symbols.get(value_field) is not None
        if self.type == grammar.ValueType.SYMBOL.value or non_symbol_type_value_is_symbol:
            # if non-symbol value is symbol, then make it a symbol but inform the user
            # that this is bad
            if self.type != grammar.ValueType.SYMBOL.value:
                self.type = grammar.ValueType.SYMBOL.value
                print(f"WARNING: symbol value \"{value_field}\" passed with non-symbol type, casting it to symbol...")

            self.value_bytes = packing.pack_signed_integer_value_data(utils.resolve_variable_symbol_name(ctx, value_field))

        else: # handle non-symbol value
            self.value_bytes = []
 
            # add value if not null
            if value_field != grammar.NULL_TOKEN:
                values = value_field.split(grammar.BYTE_SPLIT_TOKEN)
                for value in values:
                    # pack value data
                    # this breaks down int values into however many bytes they need to be stored
                    # also makes us big endian
                    if self.type == grammar.ValueType.SIGNED_INTEGER.value or self.type == grammar.ValueType.SYMBOL.value:
                        self.value_bytes.extend(packing.pack_signed_integer_value_data(int(value)))

                    elif self.type == grammar.ValueType.UNSIGNED_INTEGER.value:
                        self.value_bytes.extend(packing.pack_unsigned_integer_value_data(int(value)))

                    elif self.type == grammar.ValueType.FLOAT.value:
                        self.value_bytes.extend(packing.pack_float_value_data(float(value)))

                    elif self.type == grammar.ValueType.DOUBLE.value:
                        self.value_bytes.extend(packing.pack_double_value_data(float(value)))

                    elif self.type == grammar.ValueType.STRING.value: # ASCII currently
                        self.value_bytes.extend(packing.pack_string_value_data(int(value)))

                    elif self.type == grammar.ValueType.ARRAY.value:
                        # generate array immediately
                        self.value_bytes.extend(ValueArrayOperandMCodeGenerator(ctx, value).generate())
                    
                    else:
                        self.value_bytes.append(int(value))

            else: # handle null value
                self.value_bytes = [0]

    def generate(self):
        value_size = len(self.value_bytes)
        return [
            *packing.pack_1_byte_unsigned_integer(self.type),
            *packing.pack_1_byte_unsigned_integer(utils.get_size_of_unsigned_integer(value_size)),
            *packing.pack_n_byte_unsigned_integer(value_size),
            *self.value_bytes
        ]

class ValueArrayOperandMCodeGenerator:
    def __init__(self, ctx, unparsed_operand):
        # parse
        # handle null array
        if unparsed_operand == grammar.NULL_TOKEN:
            self.values = []

        else: # non-null array
            array_values = unparsed_operand.split(grammar.ARRAY_VALUE_SPLIT_TOKEN)
            self.values = [ # store values as generators
                ValueOperandMCodeGenerator(ctx, array_value) for array_value in array_values
            ]

    def generate(self):
        array_size = len(self.values)
        array_values_mcode = []
        for value in self.values: # generate values
            array_values_mcode.extend(value.generate())

        return [
            *packing.pack_1_byte_unsigned_integer(utils.get_size_of_unsigned_integer(array_size)),
            *packing.pack_n_byte_unsigned_integer(array_size),
            *array_values_mcode
        ]

class FunctionRequirementOperandMCodeGenerator:
    def __init__(self, ctx, unparsed_operand, create=False):
        # parse
        function_requirement_fields = unparsed_operand.split(grammar.VALUE_FIELD_SPLIT_TOKEN)
        self.name = function_requirement_fields[0]
        type_field = function_requirement_fields[1]
        
        if create:
            self.symbol = ctx.next_variable_symbol
            ctx.next_variable_symbol += 1
        else:
            self.symbol = utils.resolve_variable_symbol_name(ctx, self.name)

        self.type = grammar.VALUE_TYPE_ALIASES[type_field]

    def generate(self):
        return [
            *packing.pack_1_byte_unsigned_integer(utils.get_size_of_signed_integer(self.symbol)),
            *packing.pack_n_byte_signed_integer(self.symbol),
            *packing.pack_1_byte_unsigned_integer(self.type)
        ]

class FunctionRequirementArrayOperandMCodeGenerator:
    def __init__(self, ctx, unparsed_operand, create=False):
        # parse
        # handle null array
        if unparsed_operand == grammar.NULL_TOKEN:
            self.values = []

        else:
            array_values = unparsed_operand.split(grammar.ARRAY_VALUE_SPLIT_TOKEN)
            self.values = [ # store values as generators
                FunctionRequirementOperandMCodeGenerator(ctx, array_value, create) for array_value in array_values
            ]

    def generate(self):
        array_size = len(self.values)
        array_values_mcode = []
        for value in self.values: # generate values
            array_values_mcode.extend(value.generate())

        return [
            *packing.pack_1_byte_unsigned_integer(utils.get_size_of_unsigned_integer(array_size)),
            *packing.pack_n_byte_unsigned_integer(array_size),
            *array_values_mcode
        ]

class VariableSymbolOperandMCodeGenerator:
    def __init__(self, ctx, unparsed_operand, create=False):
        # parse
        self.name = unparsed_operand

        resolved_variable_symbol_name = utils.resolve_variable_symbol_name(ctx, self.name)
        # only create new variable if we aren't redefining one
        if create and resolved_variable_symbol_name is None: # create new symbol
            self.value = ctx.next_variable_symbol
            ctx.next_variable_symbol += 1

        else: # find existing symbol
            self.value = resolved_variable_symbol_name

    def generate(self):
        return [
            *packing.pack_1_byte_unsigned_integer(utils.get_size_of_signed_integer(self.value)),
            *packing.pack_n_byte_signed_integer(self.value)
        ]

class FunctionSymbolOperandMCodeGenerator:
    def __init__(self, ctx, unparsed_operand, create=False):
        # parse
        self.name = unparsed_operand

        resolved_function_symbol_name = utils.resolve_function_symbol_name(ctx, self.name)
        # only create new func if we aren't redefining one
        if create and resolved_function_symbol_name is None: # create new symbol
            self.value = ctx.next_function_symbol
            ctx.next_function_symbol += 1

        else: # find existing symbol
            self.value = resolved_function_symbol_name

    def generate(self):
        return [
            *packing.pack_1_byte_unsigned_integer(utils.get_size_of_signed_integer(self.value)),
            *packing.pack_n_byte_signed_integer(self.value)
        ]

# generic symbol generator that supports both variables and functions
# NOTE: doesn't support creating symbols if they don't exist
class SymbolOperandMCodeGenerator:
    def __init__(self, ctx, unparsed_operand):
        # parse
        self.name = unparsed_operand
        variable_symbol_value = utils.resolve_variable_symbol_name(ctx, self.name)
        # try variable
        if variable_symbol_value is None:
            # if not variable, try function
            function_symbol_value = utils.resolve_function_symbol_name(ctx, self.name)
            if function_symbol_value is None:
                # all is lost
                sys.exit("generic symbol handler couldn't resolve symbol, world explosion imminent")

            else: # function success
                self.value = function_symbol_value

        else: # variable success
            self.value = variable_symbol_value

    def generate(self):
        return [
            *packing.pack_1_byte_unsigned_integer(utils.get_size_of_signed_integer(self.value)),
            *packing.pack_n_byte_signed_integer(self.value)
        ]

class ABISymbolOperandMCodeGenerator:
    def __init__(self, ctx, unparsed_operand):
        # parse
        name = unparsed_operand
        self.value = grammar.ABI_SYMBOLS[name]

    def generate(self):
        return [
            *packing.pack_1_byte_unsigned_integer(utils.get_size_of_signed_integer(self.value)),
            *packing.pack_n_byte_signed_integer(self.value)
        ]

# wraps ValueOperandMCodeGenerator
class JumpByteIndexOperandMCodeGenerator:
    def __init__(self, ctx, unparsed_operand):
        self.ctx = ctx
        self.unparsed_operand = unparsed_operand
        self.has_pre_generated = False

    def generate(self):
        jump_byte_index = self.ctx.labels.get(self.unparsed_operand)
        if jump_byte_index is None:
            if self.has_pre_generated:
                sys.exit(f"Label \"{self.unparsed_operand}\" not defined")

            self.has_pre_generated = True
            jump_byte_index = 0

        underlying_value_operand = ValueOperandMCodeGenerator(
            self.ctx, f"uint{grammar.VALUE_FIELD_SPLIT_TOKEN}{jump_byte_index}" # format value
        )

        return underlying_value_operand.generate()