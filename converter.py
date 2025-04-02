import random
import struct # For converting numbers to bytes
import binascii # For printing hex output

# --- VM Instruction Set Opcodes ---
# Define opcodes directly mapping names to byte values
OPCODES = {
    "DT_ADD": 0x01, "DT_SUB": 0x02, "DT_MUL": 0x03, "DT_DIV": 0x04,
    "DT_INC": 0x05, "DT_DEC": 0x06, "DT_SHL": 0x07, "DT_SHR": 0x08,
    "DT_FP_ADD": 0x09, "DT_FP_SUB": 0x0A, "DT_FP_MUL": 0x0B, "DT_FP_DIV": 0x0C,
    "DT_END": 0x10, "DT_LOD": 0x11, "DT_STO": 0x12, "DT_IMMI": 0x13,
    "DT_STO_IMMI": 0x14, "DT_MEMCPY": 0x15, "DT_MEMSET": 0x16,
    "DT_JMP": 0x20, "DT_JZ": 0x21, "DT_IF_ELSE": 0x22, "DT_JUMP_IF": 0x23,
    "DT_GT": 0x24, "DT_LT": 0x25, "DT_EQ": 0x26, "DT_GT_EQ": 0x27, "DT_LT_EQ": 0x28,
    "DT_CALL": 0x29, "DT_RET": 0x2A,
    "DT_SEEK": 0x30, "DT_PRINT": 0x31, "DT_FP_PRINT": 0x32,
    "DT_READ_INT": 0x33, "DT_READ_FP": 0x34, "DT_TIK": 0x35, "DT_RND": 0x36,
    # Add any missing opcodes if necessary
}

# --- Generator Configuration ---
# Assuming 32-bit architecture for addresses and immediates
ADDRESS_SIZE = 4
IMMEDIATE_SIZE = 4
BYTE_ORDER = 'little' # Or 'big' depending on your VM

# Define memory addresses used by the generated code
BUFFER_TOP_ADDR = 0
MAX_DEPTH_ADDR = 1
TEMP_CHAR_ADDR = 2      # Used by _extend
TEMP_DEPTH_ADDR = 3     # Used by functions for depth+1 or temporary R value
BUFFER_START_ADDR = 10  # Start of the actual output buffer in VM memory

class BytecodeGenerator:
    def __init__(self, grammar, nonterminals, start_symbol, fallbacks, max_depth):
        self.grammar = grammar
        self.nonterminals = set(nonterminals)
        self.start_symbol = start_symbol
        self.fallbacks = fallbacks
        self.max_depth = max_depth

        self.bytecode = bytearray()
        self.current_byte_offset = 0
        self.label_addresses = {} # label_name -> byte_offset
        self.fixups = [] # List of (patch_offset, label_name) to resolve later
        self._label_count = 0

    def _get_unique_label(self, base_name):
        # Labels are now internal symbols, not directly in bytecode,
        # but unique names are still needed for branching logic.
        self._label_count += 1
        return f"{base_name}_{self._label_count}"

    def _emit_label(self, label_name):
        """Records the current offset for a label."""
        if label_name in self.label_addresses:
            print(f"Warning: Duplicate label definition '{label_name}' ignored.")
            return
        self.label_addresses[label_name] = self.current_byte_offset
        # print(f"Label {label_name} defined at offset {self.current_byte_offset}") # Debugging

    def _emit_bytes(self, byte_values):
        """Appends raw bytes and updates offset."""
        self.bytecode.extend(byte_values)
        self.current_byte_offset += len(byte_values)

    def _emit(self, instruction_name, *args):
        """Emits an instruction opcode and handles arguments."""
        if instruction_name not in OPCODES:
            raise ValueError(f"Unknown instruction: {instruction_name}")

        opcode = OPCODES[instruction_name]
        self._emit_bytes([opcode]) # Emit the 1-byte opcode

        # --- Handle Arguments based on Instruction ---

        # Instructions taking ONE IMMEDIATE integer (value or address)
        # Includes DT_LOD when used with an immediate address argument
        if instruction_name in ["DT_IMMI", "DT_READ_INT", "DT_LOD"]:
            if args: # Only process if arguments are provided
                if len(args) != 1 or not isinstance(args[0], int):
                    raise ValueError(f"{instruction_name} expects 1 integer immediate argument when args provided.")
                imm_bytes = args[0].to_bytes(IMMEDIATE_SIZE, byteorder=BYTE_ORDER)
                self._emit_bytes(imm_bytes)
            # If no args are given for DT_LOD, assume stack operation - do nothing more here.

        # DT_STO: Takes an immediate address arg *only if provided* in the call.
        # Otherwise, it's the stack-based version.
        elif instruction_name == "DT_STO":
            if args: # If an arg is given, assume it's the immediate address variant
                if len(args) != 1 or not isinstance(args[0], int):
                    raise ValueError(f"{instruction_name} expects 1 integer memory address argument when args provided.")
                addr_bytes = args[0].to_bytes(ADDRESS_SIZE, byteorder=BYTE_ORDER)
                self._emit_bytes(addr_bytes)
            # If no args are given, assume stack operation - do nothing more here.


        # Instructions taking TWO IMMEDIATE integers (value, then address)
        elif instruction_name == "DT_STO_IMMI":
            if len(args) != 2 or not isinstance(args[0], int) or not isinstance(args[1], int):
                 raise ValueError(f"{instruction_name} expects 2 integer arguments (value, address).")
            val_bytes = args[0].to_bytes(IMMEDIATE_SIZE, byteorder=BYTE_ORDER)
            addr_bytes = args[1].to_bytes(ADDRESS_SIZE, byteorder=BYTE_ORDER)
            self._emit_bytes(val_bytes) # Emit value first
            self._emit_bytes(addr_bytes) # Then address


        # Instructions taking ONE LABEL NAME (target address)
        elif instruction_name in ["DT_JMP", "DT_JZ", "DT_JUMP_IF", "DT_CALL", "DT_IF_ELSE"]: # Adapt DT_IF_ELSE if it needs 2 labels
            if len(args) != 1 or not isinstance(args[0], str):
                raise ValueError(f"{instruction_name} expects 1 label name argument.")
            label_name = args[0]
            if label_name in self.label_addresses:
                # Backward reference or already defined
                target_addr = self.label_addresses[label_name]
                addr_bytes = target_addr.to_bytes(ADDRESS_SIZE, byteorder=BYTE_ORDER)
                self._emit_bytes(addr_bytes)
            else:
                # Forward reference: Record fixup and emit placeholder
                patch_offset = self.current_byte_offset
                placeholder = bytes([0] * ADDRESS_SIZE)
                self._emit_bytes(placeholder) # Reserve space
                self.fixups.append((patch_offset, label_name))

        # Other instructions (DT_ADD, DT_SUB, DT_RET, DT_END, stack-based DT_STO/DT_LOD etc.)
        # require no immediate arguments encoded after the opcode.
        else:
            if args:
                 # If args were provided for an instruction that doesn't take them
                 print(f"Warning: Arguments {args} provided for instruction {instruction_name} which takes none.")
                 pass # Or raise error if strict

    def _apply_fixups(self):
        """Patches the bytecode with addresses for forward references."""
        # print("\nApplying Fixups...") # Debugging
        for patch_offset, label_name in self.fixups:
            if label_name not in self.label_addresses:
                raise ValueError(f"Undefined label referenced in jump/call: {label_name}")

            target_addr = self.label_addresses[label_name]
            addr_bytes = target_addr.to_bytes(ADDRESS_SIZE, byteorder=BYTE_ORDER)

            # Patch the bytecode
            for i in range(ADDRESS_SIZE):
                self.bytecode[patch_offset + i] = addr_bytes[i]
            # print(f"  Patched {label_name} at offset {patch_offset} with address {target_addr} ({addr_bytes.hex()})") # Debugging
        # print("Fixups applied.") # Debugging


    # --- Core Generation Logic (Generates bytecode using _emit and _emit_label) ---

    def _generate_extend_subroutine(self):
        """Generates bytecode for the _extend helper function."""
        self._emit_label("_extend")
        # Store value(stack_top) at immediate address TEMP_CHAR_ADDR
        self._emit("DT_STO", TEMP_CHAR_ADDR)
        # Calculate target address (BUFFER_START_ADDR + value_at(BUFFER_TOP_ADDR)) on stack
        self._emit("DT_IMMI", BUFFER_START_ADDR)
        self._emit("DT_LOD", BUFFER_TOP_ADDR) # Load value from immediate address
        self._emit("DT_ADD") # Stack: [..., address]
        # Load character back onto stack from immediate address TEMP_CHAR_ADDR
        self._emit("DT_LOD", TEMP_CHAR_ADDR) # Load value from immediate address
        # Stack: [..., address, value]
        # Store value at address (both from stack). NO immediate argument here.
        self._emit("DT_STO")
        # Increment value at immediate address BUFFER_TOP_ADDR
        self._emit("DT_LOD", BUFFER_TOP_ADDR) # Load value from immediate address
        self._emit("DT_INC")
        self._emit("DT_STO", BUFFER_TOP_ADDR) # Store value at immediate address
        self._emit("DT_RET")

    def _generate_main(self):
        """Generates the main entry point bytecode."""
        self._emit_label("main")
        # Initialize buffer_top = 0
        self._emit("DT_IMMI", 0)
        self._emit("DT_STO", BUFFER_TOP_ADDR) # Store immediate 0 at address BUFFER_TOP_ADDR
        # Initialize max_depth
        self._emit("DT_IMMI", self.max_depth)
        self._emit("DT_STO", MAX_DEPTH_ADDR) # Store immediate max_depth at address MAX_DEPTH_ADDR
        # Call the start symbol function with depth=1
        self._emit("DT_IMMI", 1) # Push initial depth argument
        self._emit("DT_CALL", f"func_{self.start_symbol}") # Call function by label name
        # Optional: Add buffer printing code here if needed
        self._emit("DT_END") # End program execution

    def _generate_nonterminal_function(self, nt):
        """Generates bytecode for a function corresponding to a non-terminal."""
        func_label = f"func_{nt}"
        continue_label = self._get_unique_label(f"F_{nt}_continue") # Use unique labels
        endswitch_label = self._get_unique_label(f"F_{nt}_endswitch")

        self._emit_label(func_label) # Define function label

        # 1. Depth Check
        # Stack: [..., depth]
        self._emit("DT_LOD", MAX_DEPTH_ADDR) # Load value from MAX_DEPTH_ADDR
        # Stack: [..., depth, MAX_DEPTH]
        self._emit("DT_GT")                  # depth > MAX_DEPTH ? Pushes 1 or 0
        # Stack: [..., boolean]
        # Jump if false (0) to continue_label
        self._emit("DT_JZ", continue_label) # Jump if depth <= MAX_DEPTH

        # 2. Fallback Code (if depth > MAX_DEPTH)
        # Pop the boolean result (1)
        self._emit("DT_IMMI", 0) # Push 0
        self._emit("DT_ADD")     # Add (effectively pop the boolean 1)
        fallback_sequence = self.fallbacks.get(nt, "")
        for char_code in [ord(c) for c in fallback_sequence]:
             self._emit("DT_IMMI", char_code)
             self._emit("DT_CALL", "_extend") # Call subroutine by label name
        # Assume RET handles stack cleanup (argument popping)
        self._emit("DT_RET")

        # 3. Main Logic (depth <= MAX_DEPTH)
        self._emit_label(continue_label) # Define label for continue case
        # Pop the boolean result (0)
        self._emit("DT_IMMI", 0) # Push 0
        self._emit("DT_ADD")     # Add (effectively pop the boolean 0)
        # Stack: [..., depth]

        productions = self.grammar.get(nt, []) # Get productions, default to empty list
        num_productions = len(productions)

        if num_productions == 0:
             print(f"Warning: Non-terminal {nt} has no productions. Emitting RET.")
             self._emit("DT_RET") # Nothing to do, just return

        elif num_productions == 1:
             # Directly execute the single production
             self._generate_production_code(nt, productions[0])
             # Falls through to the final RET of this function

        else:
             # Multiple productions: Use DT_RND and branch
             self._emit("DT_IMMI", num_productions)
             self._emit("DT_RND") # Pops N, pushes R in [0, N-1]
             # Stack: [..., depth, R]

             case_labels = [self._get_unique_label(f"F_{nt}_case") for _ in range(num_productions)]

             # Generate branching logic (Check R == 0, R == 1, ...)
             for i in range(num_productions - 1):
                 # Duplicate R (assuming no DUP, store/load using a temp addr)
                 self._emit("DT_STO", TEMP_DEPTH_ADDR) # Store R temporarily
                 self._emit("DT_LOD", TEMP_DEPTH_ADDR) # Load R back onto stack
                 # Check if R == i
                 self._emit("DT_IMMI", i)
                 self._emit("DT_EQ") # Pops R and i, pushes boolean
                 # If true (1), jump to case i
                 self._emit("DT_JUMP_IF", case_labels[i]) # Jump by label name
                 # If false (0), boolean is consumed (assuming JUMP_IF consumes).
                 # If not, would need an explicit pop here.

             # If we haven't jumped, R must be num_productions - 1
             # The last loaded R might still be on stack if last EQ was false
             # or if num_productions was 2 (only one check).
             # Pop it if necessary (DT_EQ should have consumed the last one checked).
             # If branching logic guarantees R isn't left, this pop isn't needed.
             # For safety, let's assume R from last LOD might linger if i=0 was the only check.
             # A safer approach might be to always pop R after storing it.
             # Let's refine the loop:
             # Stack: [..., depth, R]
             # self._emit("DT_STO", TEMP_DEPTH_ADDR) # Store R
             # for i in range(num_productions - 1):
             #     self._emit("DT_LOD", TEMP_DEPTH_ADDR) # Load R
             #     self._emit("DT_IMMI", i)
             #     self._emit("DT_EQ") # Pops R, i. Pushes bool.
             #     self._emit("DT_JUMP_IF", case_labels[i]) # Consumes bool if jumps. Need pop otherwise? Assume consumes.
             # self._emit("DT_JMP", case_labels[-1]) # If no jumps taken

             # Assuming the simple compare/jump logic suffices:
             self._emit("DT_JMP", case_labels[-1]) # Jump to the last case unconditionally if others fail


             # Generate code for each case
             for i, production in enumerate(productions):
                 self._emit_label(case_labels[i]) # Define label for this case
                 # Stack should be: [..., depth] (R/boolean consumed by branching)
                 self._generate_production_code(nt, production)
                 # After executing production, jump to end (avoid falling through)
                 # Don't jump after the *last* case's code.
                 if i < num_productions - 1:
                     self._emit("DT_JMP", endswitch_label) # Jump by label name

             self._emit_label(endswitch_label) # Define label for end of switch
             # Fallthrough after the last case, or jump target.

        # Common return point for the function
        self._emit("DT_RET")


    def _generate_production_code(self, current_nt, production):
        """Generates bytecode for a sequence of symbols in a production."""
        # Assumes stack upon entry: [..., depth]
        needs_depth_plus_1 = any(symbol in self.nonterminals for symbol in production)

        if needs_depth_plus_1:
            # Calculate depth+1 (original depth is on stack)
            self._emit("DT_INC")
            self._emit("DT_STO", TEMP_DEPTH_ADDR) # Store depth+1 at immediate address
            # Stack is now [...] (depth+1 stored in memory)
        else:
            # If only terminals, original depth remains on stack.
            # It needs to be popped before RET if RET doesn't handle args.
            # For simplicity, assume RET handles it or stack is managed by caller/callee convention.
            pass


        for symbol in production:
            if symbol in self.nonterminals:
                # Non-terminal: Load depth+1 and call
                self._emit("DT_LOD", TEMP_DEPTH_ADDR) # Load value from immediate address
                self._emit("DT_CALL", f"func_{symbol}") # Call function by label name
            else:
                # Terminal: Emit extend call
                try:
                    char_code = ord(symbol)
                    self._emit("DT_IMMI", char_code)
                    self._emit("DT_CALL", "_extend") # Call subroutine by label name
                except TypeError:
                    print(f"Warning: Symbol '{symbol}' in production for {current_nt} not single char terminal.")

        # The final DT_RET for the function is emitted *after* this code runs
        # in the calling context (_generate_nonterminal_function)


    def generate(self):
        """Generates the full bytecode program."""
        self.bytecode = bytearray()
        self.current_byte_offset = 0
        self.label_addresses = {}
        self.fixups = []
        self._label_count = 0

        # --- Generate code sections ---
        # Order matters for forward/backward references, but fixups handle it.
        # Placing main first makes its address 0 (usually desired entry point).
        self._generate_main()
        self._generate_extend_subroutine()

        # Generate function for each non-terminal
        processed_nts = set()
        # Use list from grammar keys to process all defined NTs
        # Sort to ensure deterministic output order if grammar dict order changes
        all_nts = sorted(list(self.grammar.keys()))

        for nt in all_nts:
             if nt not in processed_nts:
                 self._generate_nonterminal_function(nt)
                 processed_nts.add(nt)
                 # Could add dependency tracking here if needed, but processing
                 # all defined NTs should be sufficient.

        # --- Resolve forward references ---
        self._apply_fixups()

        return self.bytecode

# --- Example Usage (using the grammar derived from C code) ---

grammar_c = {
    # Non-Terminal : List of Productions (each production is a list of symbols)
    'NT_1680': [['NT_1552']], # Start Symbol Rule
    'NT_1360': [['NT_1808'], ['NT_1936'], ['NT_2128']],
    'NT_1552': [['NT_2256'], ['NT_2704'], ['NT_3088']],
    'NT_1808': ['0', 'd'], # Terminal Production
    'NT_1936': ['1'],       # Terminal Production
    'NT_2128': ['2'],       # Terminal Production
    'NT_2256': [['NT_1360', 'NT_2384', 'NT_2512', 'NT_2576']],
    'NT_2384': ['3'],       # Terminal Production
    'NT_2512': ['4'],       # Terminal Production
    'NT_2576': ['5'],       # Terminal Production
    'NT_2704': [['NT_1360', 'NT_2832', 'NT_2960']],
    'NT_2832': ['*'],       # Terminal Production
    'NT_2960': ['t'],       # Terminal Production
    'NT_3088': [['NT_1360', 'NT_3216', 'NT_1552']],
    'NT_3216': ['+']        # Terminal Production
}
# Automatically derive nonterminals from grammar keys
nonterminals_c = list(grammar_c.keys())
start_symbol_c = 'NT_1680' # Must be a key in grammar_c

# Fallback sequences for when MAX_DEPTH is reached
fallbacks_c = {
    'NT_1360': '2',           # From C: extend(50)
    'NT_1552': '2*t',         # From C: extend(50); extend(42); extend(116);
    'NT_1680': '2*t',         # From C (same as 1552)
    'NT_2256': '2345',        # From C: extend(50); extend(51); extend(52); extend(53);
    'NT_2704': '2*t',         # From C (same as 1552)
    'NT_3088': '2+2*t',       # From C: extend(50); extend(43); extend(50); extend(42); extend(116);
    # Terminals don't recurse, fallback not strictly needed but harmless if defined
    'NT_1808': '',
    'NT_1936': '',
    'NT_2128': '',
    'NT_2384': '',
    'NT_2512': '',
    'NT_2576': '',
    'NT_2832': '',
    'NT_2960': '',
    'NT_3216': ''
}

MAX_DEPTH_C = 5 # Max recursion depth allowed

# --- Generate the Bytecode ---
generator = BytecodeGenerator(grammar_c, nonterminals_c, start_symbol_c, fallbacks_c, MAX_DEPTH_C)
bytecode_output = generator.generate()

# --- Output ---
print(f"Generated Bytecode Length: {len(bytecode_output)} bytes")

# Optional: Print bytecode as hex
print("\nBytecode (hex):")
# Use 'sep' and 'bytes_per_sep' for readable output if desired
# print(binascii.hexlify(bytecode_output).decode()) # Raw hex string
print(binascii.hexlify(bytecode_output, sep=' ', bytes_per_sep=1).decode()) # Spaced hex bytes

# Optional: Save to file
try:
    with open("grammar_compiled.bin", "wb") as f:
        f.write(bytecode_output)
    print("\nBytecode saved to grammar_compiled.bin")
except IOError as e:
    print(f"\nError saving bytecode to file: {e}")