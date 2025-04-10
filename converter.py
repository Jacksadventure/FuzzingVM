import json

MAX_DEPTH = 5

def generate_function_code(func_name, branches=None):
    """
    Generates code for a function.
    
    Every function begins with a depth–check sequence:
      DT_DUP, DT_IMMI MAX_DEPTH, DT_GT, DT_IF_ELSE, <RET_addr>, <BODY_addr>
    
    For a simple (terminal) function (branches is None) the function simply emits DT_RET
    after the depth–check.
    
    For a branching (nonterminal) function:
      1. A comparison block is generated. For each branch i (1 ≤ i ≤ k) it:
         - pushes immediate i,
         - executes DT_EQ,
         - then DT_IF_ELSE with:
               true target: label for branch i’s body,
               false target: for branches 1..(k–1) the next comparison label,
                              and for the last branch the function return label.
         (Here the patch tokens for jumps are in relative mode.)
      2. For each branch, the body is generated. For every call in the branch,
         the code emits DT_DUP and DT_INC (to pass depth+1) and then DT_CALL using an
         absolute patch token. At the end of each branch a DT_JMP (relative patch)
         jumps to the function return.
      3. Finally, the function’s return label is set and DT_RET is emitted.
    
    Jump patch tokens are represented as tuples ("PATCH", label, mode),
    where mode is "rel" (relative; final value = target_absolute – patch_token_index)
    or "abs" (absolute).
    
    Nonterminal functions (defined in JSON with keys like "<a>") keep their angle brackets,
    so that a call "a" (terminal) is distinct from a call "<a>" (nonterminal).
    
    Returns (code, labels) where code is a list of tokens and labels is a dict mapping
    local label names to offsets (relative to the function’s start).
    """
    code = []
    labels = {}
    # Keep the name as given (e.g. "<a>") so nonterminals remain distinct.
    labels[func_name + "_start"] = 0

    # Depth-check sequence (relative jumps):
    # DT_DUP, DT_IMMI MAX_DEPTH, DT_GT, DT_IF_ELSE, <RET_addr>, <BODY_addr>
    code.extend([
        "DT_DUP",
        "DT_IMMI", MAX_DEPTH,
        "DT_GT",
        "DT_IF_ELSE", ("PATCH", func_name + "_ret", "rel"), ("PATCH", func_name + "_body", "rel")
    ])
    # Mark the beginning of the function body.
    # (The depth-check occupies indices 0..6, so the body ideally starts at 7.)
    labels[func_name + "_body"] = len(code)

    if branches is None:
        # Terminal function: simply mark return label and emit DT_RET.
        labels[func_name + "_ret"] = len(code)
        code.append("DT_RET")
    else:
        k = len(branches)
        # --- Generate comparison block ---
        labels[func_name + "_cmp_1"] = len(code)
        for i in range(1, k + 1):
            # For branch i: if not last, false target is next comparison label; if last, false target is function return.
            next_cmp = f"{func_name}_cmp_{i+1}" if i < k else f"{func_name}_ret"
            code.extend([
                "DT_IMMI", i,
                "DT_EQ",
                "DT_IF_ELSE", ("PATCH", f"{func_name}_branch_{i}", "rel"), ("PATCH", next_cmp, "rel")
            ])
            if next_cmp not in labels:
                labels[next_cmp] = len(code)
        # --- Generate branch bodies ---
        for i, branch in enumerate(branches, start=1):
            labels[f"{func_name}_branch_{i}"] = len(code)
            for call in branch:
                # For each call: do not strip angle brackets.
                target_func = call
                code.extend([
                    "DT_DUP",
                    "DT_INC",
                    "DT_CALL", ("PATCH", f"{target_func}_start", "abs"), 1
                ])
            # End branch with unconditional jump to function return.
            code.extend(["DT_JMP", ("PATCH", f"{func_name}_ret", "rel")])
        # --- Mark function return label and emit DT_RET.
        labels[f"{func_name}_ret"] = len(code)
        code.append("DT_RET")
    return code, labels

def generate_all_code(json_data):
    """
    Given a JSON specification of functions (with keys like "<a>": branches list),
    generate the full assembly program as a comma-separated string.
    
    Functions referenced in branch calls but not defined are generated as terminal functions.
    The main function is generated to push 0 and call the entry function.
    
    All jump instructions (DT_IF_ELSE and DT_JMP) use relative offsets;
    DT_CALL uses an absolute address.
    
    The entry function is chosen from nonterminals (if any exist) and is placed immediately after main.
    """
    # Build nonterminal functions from JSON (keys with angle brackets remain unchanged).
    nonterminals = {}
    for key, branches in json_data.items():
        nonterminals[key] = branches

    # Gather terminal function names from branch calls.
    terminals = {}
    for branches in json_data.values():
        for branch in branches:
            for call in branch:
                # Calls not enclosed in angle brackets are terminal.
                if not (call.startswith("<") and call.endswith(">")):
                    terminals[call] = None

    # For nonterminals, assume all are defined in JSON.
    # Terminal functions that are referenced but not defined become terminal.
    
    # Generate code for each function.
    func_codes = {}
    func_labels = {}
    # Choose an entry function from nonterminals if available; otherwise, default to "a".
    entry_point = next(iter(nonterminals)) if nonterminals else "a"
    
    # Build function order:
    # main first, then the entry function, then remaining nonterminals (alphabetically, excluding entry), then terminals.
    func_order = ["main", entry_point]
    for key in sorted(nonterminals.keys()):
        if key != entry_point:
            func_order.append(key)
    for key in sorted(terminals.keys()):
        func_order.append(key)
    
    # Generate nonterminal function code.
    for func in nonterminals:
        # If the nonterminal has a branches list, generate branching code; otherwise, terminal.
        if nonterminals[func] is not None:
            code, labels = generate_function_code(func, branches=nonterminals[func])
        else:
            code, labels = generate_function_code(func, branches=None)
        func_codes[func] = code
        func_labels[func] = labels

    # Generate terminal function code.
    for func in terminals:
        code, labels = generate_function_code(func, branches=None)
        func_codes[func] = code
        func_labels[func] = labels

    # Generate main function: push 0 and call the entry function.
    main_code = ["DT_IMMI", 0, "DT_CALL", ("PATCH", f"{entry_point}_start", "abs"), 1, "DT_RET"]
    func_codes["main"] = main_code
    func_labels["main"] = {"main_start": 0}

    # Concatenate all functions in the order of func_order.
    abs_code = []
    abs_labels = {}  # Global mapping: label -> absolute address.
    for fname in func_order:
        if fname not in func_codes:
            continue
        func_start = len(abs_code)
        for label, offset in func_labels[fname].items():
            abs_labels[label] = func_start + offset
        abs_code.extend(func_codes[fname])

    # Patch all placeholders.
    for i, token in enumerate(abs_code):
        if isinstance(token, tuple) and token[0] == "PATCH":
            label = token[1]
            mode = token[2]
            if label not in abs_labels:
                raise ValueError("Undefined label: " + label)
            target = abs_labels[label]
            if mode == "abs":
                patched = target
            elif mode == "rel":
                # Relative offset computed as: target_absolute - patch_token_index.
                patched = target - i
            else:
                raise ValueError("Unknown patch mode: " + mode)
            abs_code[i] = patched

    return ",".join(str(x) for x in abs_code)

if __name__ == "__main__":
    # Sample JSON: nonterminal <a> has one branch that calls terminal a.
    json_str = r'''
    {
        "<a>": [["a"]]
    }
    '''
    json_data = json.loads(json_str)
    assembly = generate_all_code(json_data)
    print(assembly)