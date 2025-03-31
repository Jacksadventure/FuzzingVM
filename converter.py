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
    # Define a NOP if available, otherwise use 0x00 for padding
    "NOP": 0x00, # Assuming 0x00 is a valid NOP or harmless padding byte
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
        self._label_count += 1
        return f"{base_name}_{self._label_count}"

    def _emit_label(self, label_name):
        if label_name in self.label_addresses:
            print(f"Warning: Duplicate label definition '{label_name}' ignored.")
            return
        self.label_addresses[label_name] = self.current_byte_offset

    def _emit_bytes(self, byte_values):
        self.bytecode.extend(byte_values)
        self.current_byte_offset += len(byte_values)

    def _emit(self, instruction_name, *args):
        if instruction_name not in OPCODES:
             # Allow emitting raw bytes if needed (e.g. for padding)
             if isinstance(instruction_name, int):
                 self._emit_bytes([instruction_name])
                 return
             raise ValueError(f"Unknown instruction: {instruction_name}")

        opcode = OPCODES[instruction_name]
        self._emit_bytes([opcode])

        # --- Handle Arguments based on Instruction ---
        if instruction_name in ["DT_IMMI", "DT_READ_INT", "DT_LOD"]:
            if args:
                if len(args) != 1 or not isinstance(args[0], int):
                    raise ValueError(f"{instruction_name} expects 1 integer immediate argument when args provided.")
                imm_bytes = args[0].to_bytes(IMMEDIATE_SIZE, byteorder=BYTE_ORDER, signed=False) # Assume unsigned
                self._emit_bytes(imm_bytes)
        elif instruction_name == "DT_STO":
            if args:
                if len(args) != 1 or not isinstance(args[0], int):
                    raise ValueError(f"{instruction_name} expects 1 integer memory address argument when args provided.")
                addr_bytes = args[0].to_bytes(ADDRESS_SIZE, byteorder=BYTE_ORDER, signed=False) # Assume unsigned address
                self._emit_bytes(addr_bytes)
        elif instruction_name == "DT_STO_IMMI":
            if len(args) != 2 or not isinstance(args[0], int) or not isinstance(args[1], int):
                 raise ValueError(f"{instruction_name} expects 2 integer arguments (value, address).")
            val_bytes = args[0].to_bytes(IMMEDIATE_SIZE, byteorder=BYTE_ORDER, signed=False) # Assume unsigned
            addr_bytes = args[1].to_bytes(ADDRESS_SIZE, byteorder=BYTE_ORDER, signed=False) # Assume unsigned address
            self._emit_bytes(val_bytes)
            self._emit_bytes(addr_bytes)
        elif instruction_name in ["DT_JMP", "DT_JZ", "DT_JUMP_IF", "DT_CALL", "DT_IF_ELSE"]:
            if len(args) != 1 or not isinstance(args[0], str):
                raise ValueError(f"{instruction_name} expects 1 label name argument.")
            label_name = args[0]
            if label_name in self.label_addresses:
                target_addr = self.label_addresses[label_name]
                addr_bytes = target_addr.to_bytes(ADDRESS_SIZE, byteorder=BYTE_ORDER, signed=False) # Assume unsigned address
                self._emit_bytes(addr_bytes)
            else:
                patch_offset = self.current_byte_offset
                placeholder = bytes([0] * ADDRESS_SIZE)
                self._emit_bytes(placeholder)
                self.fixups.append((patch_offset, label_name))
        else:
            if args:
                 print(f"Warning: Arguments {args} provided for instruction {instruction_name} which takes none.")
                 pass

    def _apply_fixups(self):
        for patch_offset, label_name in self.fixups:
            if label_name not in self.label_addresses:
                raise ValueError(f"Undefined label referenced in jump/call: {label_name}")
            target_addr = self.label_addresses[label_name]
            # Ensure address fits in ADDRESS_SIZE bytes (unsigned)
            if target_addr < 0 or target_addr >= (1 << (ADDRESS_SIZE * 8)):
                 raise ValueError(f"Label {label_name} address {target_addr} out of range for {ADDRESS_SIZE}-byte unsigned address.")

            addr_bytes = target_addr.to_bytes(ADDRESS_SIZE, byteorder=BYTE_ORDER, signed=False)
            for i in range(ADDRESS_SIZE):
                self.bytecode[patch_offset + i] = addr_bytes[i]

    # --- Core Generation Logic (No changes needed inside these) ---
    def _generate_extend_subroutine(self):
        self._emit_label("_extend")
        self._emit("DT_STO", TEMP_CHAR_ADDR)
        self._emit("DT_IMMI", BUFFER_START_ADDR)
        self._emit("DT_LOD", BUFFER_TOP_ADDR)
        self._emit("DT_ADD")
        self._emit("DT_LOD", TEMP_CHAR_ADDR)
        self._emit("DT_STO")
        self._emit("DT_LOD", BUFFER_TOP_ADDR)
        self._emit("DT_INC")
        self._emit("DT_STO", BUFFER_TOP_ADDR)
        self._emit("DT_RET")

    def _generate_main(self):
        self._emit_label("main")
        self._emit("DT_IMMI", 0)
        self._emit("DT_STO", BUFFER_TOP_ADDR)
        self._emit("DT_IMMI", self.max_depth)
        self._emit("DT_STO", MAX_DEPTH_ADDR)
        self._emit("DT_IMMI", 1)
        self._emit("DT_CALL", f"func_{self.start_symbol}")
        self._emit("DT_END")

    def _generate_nonterminal_function(self, nt):
        func_label = f"func_{nt}"
        continue_label = self._get_unique_label(f"F_{nt}_continue")
        endswitch_label = self._get_unique_label(f"F_{nt}_endswitch")
        self._emit_label(func_label)
        self._emit("DT_LOD", MAX_DEPTH_ADDR)
        self._emit("DT_GT")
        self._emit("DT_JZ", continue_label)
        self._emit("DT_IMMI", 0)
        self._emit("DT_ADD")
        fallback_sequence = self.fallbacks.get(nt, "")
        for char_code in [ord(c) for c in fallback_sequence]:
             self._emit("DT_IMMI", char_code)
             self._emit("DT_CALL", "_extend")
        self._emit("DT_RET")
        self._emit_label(continue_label)
        self._emit("DT_IMMI", 0)
        self._emit("DT_ADD")
        productions = self.grammar.get(nt, [])
        num_productions = len(productions)
        if num_productions == 0:
             print(f"Warning: Non-terminal {nt} has no productions. Emitting RET.")
             self._emit("DT_RET")
        elif num_productions == 1:
             self._generate_production_code(nt, productions[0])
        else:
             self._emit("DT_IMMI", num_productions)
             self._emit("DT_RND")
             case_labels = [self._get_unique_label(f"F_{nt}_case") for _ in range(num_productions)]
             for i in range(num_productions - 1):
                 self._emit("DT_STO", TEMP_DEPTH_ADDR)
                 self._emit("DT_LOD", TEMP_DEPTH_ADDR)
                 self._emit("DT_IMMI", i)
                 self._emit("DT_EQ")
                 self._emit("DT_JUMP_IF", case_labels[i])
             self._emit("DT_JMP", case_labels[-1])
             for i, production in enumerate(productions):
                 self._emit_label(case_labels[i])
                 self._generate_production_code(nt, production)
                 if i < num_productions - 1:
                     self._emit("DT_JMP", endswitch_label)
             self._emit_label(endswitch_label)
        self._emit("DT_RET")

    def _generate_production_code(self, current_nt, production):
        needs_depth_plus_1 = any(symbol in self.nonterminals for symbol in production)
        if needs_depth_plus_1:
            self._emit("DT_INC")
            self._emit("DT_STO", TEMP_DEPTH_ADDR)
        for symbol in production:
            if symbol in self.nonterminals:
                self._emit("DT_LOD", TEMP_DEPTH_ADDR)
                self._emit("DT_CALL", f"func_{symbol}")
            else:
                try:
                    char_code = ord(symbol)
                    self._emit("DT_IMMI", char_code)
                    self._emit("DT_CALL", "_extend")
                except TypeError:
                    print(f"Warning: Symbol '{symbol}' in production for {current_nt} not single char terminal.")

    def generate(self):
        """Generates the full bytecode program and pads it to be a multiple of 4 bytes."""
        self.bytecode = bytearray()
        self.current_byte_offset = 0
        self.label_addresses = {}
        self.fixups = []
        self._label_count = 0

        # --- Generate code sections ---
        self._generate_main()
        self._generate_extend_subroutine()
        all_nts = sorted(list(self.grammar.keys()))
        processed_nts = set()
        for nt in all_nts:
             if nt not in processed_nts:
                 self._generate_nonterminal_function(nt)
                 processed_nts.add(nt)

        # --- Resolve forward references ---
        self._apply_fixups()

        # --- Add Padding ---
        current_length = len(self.bytecode)
        remainder = current_length % ADDRESS_SIZE # Use ADDRESS_SIZE (typically 4) for alignment
        if remainder != 0:
            padding_needed = ADDRESS_SIZE - remainder
            padding_byte = OPCODES.get("NOP", 0x00) # Use NOP if defined, else 0x00
            self._emit_bytes([padding_byte] * padding_needed)
            print(f"Added {padding_needed} byte(s) of padding (using 0x{padding_byte:02x}) to align file size to {ADDRESS_SIZE} bytes.")

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
nonterminals_c = list(grammar_c.keys())
start_symbol_c = 'NT_1680'
fallbacks_c = {
    'NT_1360': '2', 'NT_1552': '2*t', 'NT_1680': '2*t', 'NT_2256': '2345',
    'NT_2704': '2*t', 'NT_3088': '2+2*t', 'NT_1808': '', 'NT_1936': '',
    'NT_2128': '', 'NT_2384': '', 'NT_2512': '', 'NT_2576': '', 'NT_2832': '',
    'NT_2960': '', 'NT_3216': ''
}
MAX_DEPTH_C = 5

# --- Generate the Bytecode ---
generator = BytecodeGenerator(grammar_c, nonterminals_c, start_symbol_c, fallbacks_c, MAX_DEPTH_C)
bytecode_output = generator.generate()

# --- Output ---
print(f"Generated Bytecode Length: {len(bytecode_output)} bytes (Padded to be multiple of {ADDRESS_SIZE})")

# Optional: Print bytecode as hex
print("\nBytecode (hex):")
print(binascii.hexlify(bytecode_output, sep=' ', bytes_per_sep=1).decode())

# Optional: Save to file
try:
    with open("grammar_compiled.bin", "wb") as f:
        f.write(bytecode_output)
    print("\nBytecode saved to grammar_compiled.bin")
except IOError as e:
    print(f"\nError saving bytecode to file: {e}")

