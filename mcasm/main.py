#!/usr/bin/env python3

from mcasm import context
from mcasm import grammar
from mcasm import instruction_mcode_generators
from mcasm import operand_mcode_generators

import sys

if __name__ == "__main__":
    program_source_lines = open(sys.argv[1], "r").readlines()
    ctx = context.AssemblerContext()

    # assemble
    for line_index, program_line in enumerate(program_source_lines):
        program_line_parts = program_line.strip().split(grammar.OPERAND_SPLIT_TOKEN)
        instruction_name = program_line_parts[0]
        # handle empty lines
        if len(instruction_name) == 0 or instruction_name.isspace():
            continue

        instruction_operands = program_line_parts[1:]

        # handle start-of-line comments
        if instruction_name.startswith(grammar.COMMENT_TOKEN):
            continue

        # handle in-line comments
        parsed_instruction_operands = []
        for instruction_operand in instruction_operands:
            # stop all operands at and after comment token
            if instruction_operand.startswith(grammar.COMMENT_TOKEN):
                break

            parsed_instruction_operands.append(instruction_operand)

        # handle directives before func size increment and instructions
        directive_handler = grammar.DIRECTIVE_HANDLERS.get(instruction_name)
        if directive_handler is not None:
            directive_handler(ctx, parsed_instruction_operands)
            continue # don't handle anything else if this line was a directive

        # if not in global scope, increase known size of current func as we go through it
        if ctx.scope_symbol != grammar.GLOBAL_SCOPE_SYMBOL:
            ctx.scopes[ctx.scope_symbol].function_size += 1
            
        # create instruction code generator
        instruction_opcode = grammar.INSTRUCTION_OPCODES[instruction_name]
        #print(f"  --> (line {line_index + 1}) assembling {instruction_name} (opcode: {instruction_opcode} scope: {ctx.scope_symbol})")
        new_instruction_mcode_generator = grammar.INSTRUCTION_MCODE_GENERATORS[instruction_opcode](
            ctx,
            instruction_opcode,
            instruction_operands
        )

        ctx.output_mcode_generators.append(new_instruction_mcode_generator)

    # resolve labels by pre-generating code
    for mcode_generator in ctx.output_mcode_generators:
        generated_mcode = mcode_generator.generate()
        ctx.output_executable.extend(generated_mcode)

    # generate code
    ctx.output_executable = bytearray() # remove pre-generated code
    for mcode_generator in ctx.output_mcode_generators:
        #print(f"  --> generating {type(mcode_generator).__name__}")
        generated_mcode = mcode_generator.generate()
        ctx.output_executable.extend(generated_mcode)

    # write executable
    with open("program.mce", "wb") as f:
        f.write(ctx.output_executable)

    print(f"Assembled {len(ctx.output_mcode_generators)} instruction(s) / directive(s) ({len(ctx.output_executable)} B)")