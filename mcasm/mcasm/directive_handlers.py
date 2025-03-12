from mcasm import operand_mcode_generators
from mcasm import grammar

import sys

# this is a sneaky generator that, when generated,
# doesn't generate any bytecode but instead creates the label
# at the current byte position of the executable
class LabelDirectivePseudoMCodeGenerator:
    def __init__(self, ctx, label_name):
        self.ctx = ctx
        self.label_name = label_name

    def generate(self):
        output_executable_size = len(self.ctx.output_executable)
        # use executable's size to get index of the end of what's been generated so far
        label_byte_index = output_executable_size
        # define label
        self.ctx.labels[self.label_name] = label_byte_index
        return [] # doesn't generate any actual bytecode

def handle_label_directive(ctx, directive_operands):
    label_name = directive_operands[0]
    # 0 until label resolving pre-generation, when we figure out
    # the byte index and then use it when generating again
    ctx.labels[label_name] = 0
    # when pre-generation happens, this pseudo-generator will define the label and
    # its bytecode index in the executable
    ctx.output_mcode_generators.append(LabelDirectivePseudoMCodeGenerator(ctx, label_name))

def handle_end_function_directive(ctx, directive_operands):
    # handle function lifetime for non-global scopes
    if ctx.scope_symbol == -1:
        sys.exit("Can't use end function directive in global scope")

    current_scope = ctx.scopes[ctx.scope_symbol]
    final_function_size = current_scope.function_size # how big the func grew to before ending
    # retroactively inject size operand into function now that we know it
    current_scope.function.instruction_operand_mcode_generators[1] = operand_mcode_generators.ValueOperandMCodeGenerator(
        ctx, f"uint{grammar.VALUE_FIELD_SPLIT_TOKEN}{final_function_size}"
    )

    del ctx.scopes[ctx.scope_symbol] # kill it!!
    ctx.scope_symbol = current_scope.enclosing_scope_symbol

    # if the enclosing scope is another function, grow that function by our size
    if ctx.scope_symbol != -1:
        new_current_scope = ctx.scopes[ctx.scope_symbol]
        new_current_scope.function_size += final_function_size