class Scope:
    def __init__(self,
        variable_symbols={},
        function_symbols={},
        function=None,
        enclosing_scope_symbol=None,
    ):
        # keys are names, values are corresponding symbols
        self.variable_symbols = variable_symbols
        self.function_symbols = function_symbols

        # scope lifetime stuff
        # doesn't apply to global scope
        self.function = function # function that this scope is for
        self.function_size = 0 # used to calculate function size
        self.enclosing_scope_symbol = enclosing_scope_symbol # scope to return to when this one dies

class AssemblerContext:
    def __init__(self):
        self.output_mcode_generators = [] # list of BaseInstructionMCodeGenerator
        self.output_executable = bytearray() # contains the assembled executable

        # symbols
        self.next_variable_symbol = 0
        self.next_function_symbol = 0

        # scope
        # keys are function symbols, values are Scope
        self.scopes = {
            -1: Scope() # global scope
        }
        self.scope_symbol = -1 # current scope

        # labels
        # keys are label names, values are label generators
        self.labels = {}