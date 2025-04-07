import struct
import re
import sys
instruction_dict = {
    'DT_ADD': 0,
    'DT_SUB': 1,
    'DT_MUL': 2,
    'DT_DIV': 3,
    'DT_MOD': 4,
    'DT_SHL': 5,
    'DT_SHR': 6,
    'DT_FP_ADD': 7,
    'DT_FP_SUB': 8,
    'DT_FP_MUL': 9,
    'DT_FP_DIV': 10,
    'DT_DUP': 11,
    'DT_END': 12,
    'DT_LOD': 13,
    'DT_STO': 14,
    'DT_IMMI': 15,
    'DT_INC': 16,
    'DT_DEC': 17,
    'DT_STO_IMMI': 18,
    'DT_MEMCPY': 19,
    'DT_MEMSET': 20,
    'DT_JMP': 21,
    'DT_JZ': 22,
    'DT_IF_ELSE': 23,
    'DT_JUMP_IF': 24,
    'DT_GT': 25,
    'DT_LT': 26,
    'DT_EQ': 27,
    'DT_GT_EQ': 28,
    'DT_LT_EQ': 29,
    'DT_CALL': 30,
    'DT_RET': 31,
    'DT_SEEK': 32,
    'DT_PRINT': 33,
    'DT_READ_INT': 34,
    'DT_FP_PRINT': 35,
    'DT_FP_READ': 36,
    'DT_Tik': 37,
    'DT_SYSCALL': 38,
    'DT_RND': 39
}

def binary(input_file, output_file):
    with open(input_file, 'r') as file:
        code_str = file.read()
    items = code_str.split(',')
    items = [s.strip() for s in items]
    byte_stream = bytearray()
    
    for item in items:
        if item in instruction_dict:
            byte_stream.extend(struct.pack('I', instruction_dict[item]))
        elif "float_to_uint32" in item:
            match = re.search(r"\((\d+\.\d+)\)", item)
            byte_stream.extend(struct.pack('f', float(match.group(1))))
        else:
            byte_stream.extend(struct.pack('I', int(item)))
    with open(output_file, 'wb') as file:
        file.write(byte_stream)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compiler.py <input_file> <output_file>")
        sys.exit(1)
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    binary(input_file, output_file)
    print(f"Compiled {input_file} to {output_file}")