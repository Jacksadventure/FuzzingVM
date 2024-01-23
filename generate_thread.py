def generateIndirectCode(input_string):
    items = input_string.split(',')
    items = [s.strip() for s in items]
    dic = {}
    li = []
    for index, value in enumerate(items):
        if value[0].isupper():
            li.append(index)
            dic[index] = len(li) - 1
    print("Thread: {", ", ".join(map(str, li)), "}")
    new_instruments = []
    for i in range(len(items)):
        if i >= 1 and items[i - 1] in ["DT_JMP", "DT_JZ", "DT_JMP_IF","DT_CALL"]:
            new_instruments.append(dic[int(items[i])])
        elif (i >= 1 and items[i - 1] == "DT_IF_ELSE") or (i >= 2 and items[i - 2] == "DT_IF_ELSE"):
            new_instruments.append(dic[int(items[i])])
        else:
            new_instruments.append(items[i])
    print("Instruments: {", ", ".join(map(str, new_instruments)), "}")



input_string = "DT_IMMI,0,DT_STO_IMMI,0,1,DT_LOD,0,DT_ADD,DT_LOD,0,DT_INC,DT_STO,0,DT_LOD,0,DT_IMMI,100,DT_GT,DT_JZ,5,DT_SEEK,DT_END"
result = generateIndirectCode(input_string)
