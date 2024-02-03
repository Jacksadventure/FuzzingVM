from compiler import binary
import sys 

def read_code_from_file(file_path):
    with open(file_path, 'r') as file:
        return file.read()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python CIL.py <file_path>")
        sys.exit(1)
    
    file_path = sys.argv[1]
    code = read_code_from_file(file_path)
    binary(code)