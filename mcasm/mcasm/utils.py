# size utils

# returns number of bytes required to represent any given signed int
def get_size_of_signed_integer(value):
    # if value is 0, return 1
    # bit_length() seems to assume the int is unsigned, so we multiply signed value by 2
    return ((value * 2).bit_length() + 7) // 8 if value != 0 else 1

# returns number of bytes required to represent any given unsigned int
def get_size_of_unsigned_integer(value):
    # if value is 0, return 1
    return (value.bit_length() + 7) // 8 if value != 0 else 1

# resolution utils

def resolve_variable_symbol_name(ctx, symbol_name):
    return ctx.scopes[ctx.scope_symbol].variable_symbols.get(symbol_name)

def resolve_function_symbol_name(ctx, symbol_name):
    return ctx.scopes[ctx.scope_symbol].function_symbols.get(symbol_name)