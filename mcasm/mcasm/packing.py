from mcasm import grammar
from mcasm import utils

import struct

# these functions assist with turning values into bytes (packing them)

def pack_n_byte_signed_integer(value):
    return list(value.to_bytes(utils.get_size_of_signed_integer(value), grammar.ENDIANNESS, signed=True))

def pack_n_byte_unsigned_integer(value):
    return list(value.to_bytes(utils.get_size_of_unsigned_integer(value), grammar.ENDIANNESS, signed=False))

def pack_1_byte_signed_integer(value):
    return list(struct.pack(f"{grammar.STRUCT_PACK_ENDIANNESS}b", value))

def pack_1_byte_unsigned_integer(value):
    return list(struct.pack(f"{grammar.STRUCT_PACK_ENDIANNESS}B", value))

# value stuff

def pack_signed_integer_value_data(value):
    return list(struct.pack(f"{grammar.STRUCT_PACK_ENDIANNESS}i", value))

def pack_unsigned_integer_value_data(value):
    return list(struct.pack(f"{grammar.STRUCT_PACK_ENDIANNESS}I", value))

def pack_float_value_data(value):
    return list(struct.pack(f"{grammar.STRUCT_PACK_ENDIANNESS}f", value))

def pack_double_value_data(value):
    return list(struct.pack(f"{grammar.STRUCT_PACK_ENDIANNESS}d", value))

def pack_string_value_data(value):
    # one char of a string
    return list(struct.pack(f"{grammar.STRUCT_PACK_ENDIANNESS}B", value))