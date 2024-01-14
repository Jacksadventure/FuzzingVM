def d2s(input_string):
    elements = input_string.split(',')
    elements = [e.strip() for e in elements]
    formatted_instructions = []
    jump_instructions = ["DT_JMP", "DT_JZ", "DT_JMP_IF", "DT_CALL"]
    special_jump_instruction = "DT_IF_ELSE"
    address_map = {}
    current_index = 0
    for i, element in enumerate(elements):
        if not element.isdigit(): 
            address_map[i] = current_index
            current_index += 1
    i = 0
    while i < len(elements):
        element = elements[i]
        if element in jump_instructions:
            target_index = int(elements[i + 1])
            new_target_index = address_map.get(target_index, target_index)
            formatted_instructions.append(f"{{{element}, {new_target_index}}}")
            i += 2 
        elif element == special_jump_instruction:
            first_target = int(elements[i + 1])
            second_target = int(elements[i + 2])
            new_first_target = address_map.get(first_target, first_target)
            new_second_target = address_map.get(second_target, second_target)
            formatted_instructions.append(f"{{{element}, {new_first_target}, {new_second_target}}}")
            i += 3  
        elif element.isdigit():
            i += 1
        else:
            formatted_instruction = f"{{{element}"
            while i < len(elements) - 1 and elements[i + 1].isdigit():
                formatted_instruction += f", {elements[i + 1]}"
                i += 1
            formatted_instruction += "}"
            formatted_instructions.append(formatted_instruction)
            i += 1

    return ','.join(formatted_instructions)

input_string = "DT_IMMI,0,DT_STO_IMMI,0,1,DT_LOD,0,DT_ADD,DT_LOD,0,DT_INC,DT_STO,0,DT_LOD,0,DT_IMMI,100,DT_GT,DT_JZ,5,DT_SEEK,DT_END"
print(d2s(input_string))


