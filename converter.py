import json

MAX_DEPTH = 5

def generate_function_code(func_name, branches=None):
    """
    Generates code for a function.
    
    Every function begins with a depth check:
       DT_DUP isn’t allowed in the final assembly,
       so instead we assume that the current depth is already on the stack.
       The depth check is:
         DT_IMMI MAX_DEPTH, DT_GT, DT_IF_ELSE, <RET_addr>, <BODY_addr>
       (Here we assume that if depth > MAX_DEPTH, we jump to return.)
       
    For a simple function (branches is None), after the depth check we simply emit DT_RET.
    
    For a branching function:
      1. A comparison block is generated.
         For each branch i (1 ≤ i ≤ k):
            Push immediate i,
            then DT_EQ,
            then DT_IF_ELSE with:
              • true target: label for branch i body,
              • false target: label for next comparison (or the function return for the last branch).
      2. For each branch, generate its body:
         For each call in the branch, to pass (depth+1) we simply do:
           DT_INC, DT_CALL, <target_function_start>, 1
         (Thus each call in a branch will increment the current depth.)
         At the end of the branch, an unconditional DT_JMP jumps to the function’s return.
      3. Finally, the function return label is set and DT_RET is emitted.
      
    The function returns a tuple (code, labels) where code is a list of tokens and labels is
    a dict mapping label names to offsets.
    """
    code = []
    labels = {}
    # Mark the function start.
    labels[func_name + "_start"] = 0

    # --- Depth check sequence ---
    # We assume that the depth value is already on the stack.
    # Push MAX_DEPTH and perform a greater-than test.
    # Then use DT_IF_ELSE to jump either to return (if depth > MAX_DEPTH) or to the body.
    code.extend(["DT_IMMI", MAX_DEPTH, "DT_GT", "DT_IF_ELSE", ("PATCH", func_name + "_ret"), ("PATCH", func_name + "_body")])
    labels[func_name + "_body"] = len(code)

    if branches is None:
        # Simple function: mark return label and emit DT_RET.
        labels[func_name + "_ret"] = len(code)
        code.append("DT_RET")
    else:
        k = len(branches)
        # --- Generate comparison block for branch selection ---
        labels[func_name + "_cmp_1"] = len(code)
        for i in range(1, k + 1):
            if i < k:
                next_cmp = f"{func_name}_cmp_{i+1}"
            else:
                next_cmp = f"{func_name}_ret"
            # For branch i: push immediate i, compare (DT_EQ), then DT_IF_ELSE.
            code.extend(["DT_IMMI", i, "DT_EQ", "DT_IF_ELSE", ("PATCH", f"{func_name}_branch_{i}"), ("PATCH", next_cmp)])
            if next_cmp not in labels:
                labels[next_cmp] = len(code)
        # --- Generate branch bodies ---
        for i, branch in enumerate(branches, start=1):
            labels[f"{func_name}_branch_{i}"] = len(code)
            for call in branch:
                target_func = call.strip("<>")
                # Instead of duplicating the depth, we simply increment it.
                # (This means that if multiple calls occur in the same branch, the depth increases with each call.)
                code.extend(["DT_INC", "DT_CALL", ("PATCH", f"{target_func}_start"), 1])
            # After processing a branch, jump unconditionally to the function return.
            code.extend(["DT_JMP", ("PATCH", f"{func_name}_ret")])
        # Mark the function return label and emit DT_RET.
        labels[f"{func_name}_ret"] = len(code)
        code.append("DT_RET")
    return code, labels

def generate_all_code(json_data):
    """
    Given a JSON specification of functions (keys like "<a>": branches list),
    generate the full assembly program as a comma-separated string.
    
    Functions referenced but not defined become simple functions.
    Also, a main function is generated that calls function 'a' with depth 0.
    """
    # Build branching function definitions (strip angle brackets).
    branching_funcs = {} 
    for key, branches in json_data.items():
        func_name = key.strip("<>")
        branching_funcs[func_name] = branches

    # Gather all function calls referenced in branches.
    simple_funcs = {}
    for branches in json_data.values():
        for branch in branches:
            for call in branch:
                if call.startswith("<") and call.endswith(">"):
                    fname = call.strip("<>")
                    if fname not in branching_funcs:
                        branching_funcs[fname] = None  # treat as simple function
                else:
                    if call not in branching_funcs:
                        simple_funcs[call] = None

    # Any function in branching_funcs with a None definition is a simple function.
    for fname, branches in branching_funcs.items():
        if branches is None:
            simple_funcs[fname] = None

    # Generate code for each function.
    func_codes = {}
    func_labels = {}
    func_order = []

    # First, generate simple functions (alphabetically).
    for fname in sorted(simple_funcs.keys()):
        code, labels = generate_function_code(fname, branches=None)
        func_codes[fname] = code
        func_labels[fname] = labels
        func_order.append(fname)

    # Then, generate branching functions.
    for fname in sorted(branching_funcs.keys()):
        if branching_funcs[fname] is not None:
            code, labels = generate_function_code(fname, branches=branching_funcs[fname])
            func_codes[fname] = code
            func_labels[fname] = labels
            func_order.append(fname)

    # Generate main function: call function 'a' with depth 0.
    main_code = ["DT_IMMI", 0, "DT_CALL", ("PATCH", "a_start"), 1, "DT_RET"]
    func_codes["main"] = main_code
    func_labels["main"] = {"main_start": 0}
    func_order.append("main")

    # Concatenate all functions to compute absolute addresses.
    abs_code = []
    abs_labels = {}  # Global mapping: label -> absolute address.
    for fname in func_order:
        func_start = len(abs_code)
        for label, offset in func_labels[fname].items():
            abs_labels[label] = func_start + offset
        abs_code.extend(func_codes[fname])

    # Patch the placeholders.
    for i, token in enumerate(abs_code):
        if isinstance(token, tuple) and token[0] == "PATCH":
            label = token[1]
            if label in abs_labels:
                abs_code[i] = abs_labels[label]
            else:
                raise ValueError("Undefined label: " + label)

    return ",".join(str(x) for x in abs_code)

if __name__ == "__main__":
    # Example JSON input:
    # {
    #     "<a>": [["a"], ["d"], ["g"]]
    # }
    json_str = r'''
    {
        "<a>": [["a"], ["d"], ["g"]]
    }
    '''
    json_data = json.loads(json_str)
    assembly = generate_all_code(json_data)
    print(assembly)