instruction_dict = {
    'DT_ADD': 0,
    'DT_SUB': 1,
    'DT_MUL': 2,
    'DT_DIV': 3,
    'DT_SHL': 4,
    'DT_SHR': 5,
    'DT_FP_ADD': 6,
    'DT_FP_SUB': 7,
    'DT_FP_MUL': 8,
    'DT_FP_DIV': 9,
    'DT_END': 10,
    'DT_LOD': 11,
    'DT_STO': 12,
    'DT_IMMI': 13,
    'DT_INC': 14,
    'DT_DEC': 15,
    'DT_STO_IMMI': 16,
    'DT_MEMCPY': 17,
    'DT_MEMSET': 18,
    'DT_JMP': 19,
    'DT_JZ': 20,
    'DT_IF_ELSE': 21,
    'DT_JUMP_IF': 22,
    'DT_GT': 23,
    'DT_LT': 24,
    'DT_EQ': 25,
    'DT_GT_EQ': 26,
    'DT_LT_EQ': 27,
    'DT_CALL': 28,
    'DT_RET': 29,
    'DT_SEEK': 30,
    'DT_PRINT': 31,
    'DT_READ_INT': 32,
    'DT_FP_PRINT': 33,
    'DT_FP_READ': 34,
    'DT_SYSCALL': 35,
}

def direct_to_subroutine(input_string):
    items = input_string.split(',')
    items = [s.strip() for s in items]
    for index, value in enumerate(items):
        if value[0].isupper():
            items[index] = str(instruction_dict[value])