import time
import subprocess
import matplotlib.pyplot as plt
import psutil
import compiler as cp

vm_programs = ["./build/thd_vm_routine", "./build/thd_vm_indirect", "./build/thd_vm_direct"]

# Function to run a VM and capture its output
def run_virtual_machine(vm_command):
    # Using a pipe to capture the subprocess's stdout
    return subprocess.Popen(vm_command, stdout=subprocess.PIPE, shell=True, text=True)

# Function to monitor VM execution
def monitor_execution(proc, interval=600,period = 600 , duration=5400):
    start_time = time.time()
    tik_counts = []
    total_tiks = 0

    while time.time() - start_time < duration:
        line = proc.stdout.readline()  # Read a line of output
        if line:
            if "tik" in line:
                total_tiks += 1
        current_time = time.time()
        if current_time - start_time >= interval:
            tik_counts.append(total_tiks)
            interval += period  # Update the time for the next 10-minute interval

    proc.terminate()
    return tik_counts

# Recording the execution data for each VM program
vm_data = {}
vm_code_template = "DT_TIK,DT_JMP,0"
cp.binary(vm_code_template)
for vm_program in vm_programs:
    print(f"Running {vm_program}...")
    vm_proc = subprocess.Popen([vm_program,"program.bin"], 
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    tiks_over_time = monitor_execution(vm_proc)
    vm_data[vm_program] = tiks_over_time
    print(f"{vm_program} tik counts: {tiks_over_time}")

# Plotting the data
for vm_program, tiks in vm_data.items():
    plt.plot([i * 10 for i in range(len(tiks))], tiks, label=vm_program)

print(vm_data)

plt.xlabel('Time (minutes)')
plt.ylabel('Loops Count')
plt.title('Loops Counts over Time for VM Programs(DT_TIK,DT_JMP,0)')
plt.legend()
plt.savefig('tik_counts.png')