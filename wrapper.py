import subprocess
import itertools
import os
import sys

DEFAULT_EXE = "VMCacheSim.exe" if os.name == "nt" else "./VMCacheSim"

VALID_CACHE_KB = [8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192]
VALID_BLOCK_B = [4, 8, 16, 32, 64]
VALID_ASSOC = [1, 2, 4, 8, 16]
VALID_POLICIES = ["rr", "rnd"]
VALID_PHYS_MB = [128, 256, 512, 1024, 2048, 4096]

def find_executable():
    for candidate in [DEFAULT_EXE, "./VMCacheSim", "VMCacheSim.exe", "./VMCacheSim.exe"]:
        if os.path.isfile(candidate):
            return candidate
    return DEFAULT_EXE

def build_cmd(s, b, a, r, p, n, u, files, milestone=None):
    cmd = [
        find_executable(),
        "-s", str(s),
        "-b", str(b),
        "-a", str(a),
        "-r", str(r),
        "-p", str(p),
        "-n", str(n),
        "-u", str(u)
    ]
    for f in files:
        cmd.extend(["-f", f])
    if milestone is not None:
        cmd.extend(["-m", str(milestone)])
    return cmd

def run_command(cmd, save_to_file=None):
    print("\n" + "=" * 70)
    print("COMMAND: " + " ".join(cmd))
    print("=" * 70 + "\n")
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        print(result.stdout, end="")
        if save_to_file:
            with open(save_to_file, "w", encoding="utf-8") as f:
                f.write(result.stdout)
            print(f"\n[Saved output to {save_to_file}]")
        return result.stdout
    except FileNotFoundError:
        print(f"Error: Executable '{cmd[0]}' not found. Compile main.c first.")
    except subprocess.CalledProcessError as e:
        print(f"Simulator exited with error code {e.returncode}:")
        if e.stderr:
            print(e.stderr)
    return None

def prompt_choice(prompt_text, valid_options, default):
    while True:
        print(f"\n{prompt_text}")
        print(f"Valid options: {valid_options}")
        raw = input(f"Enter choice [{default}]: ").strip()
        if not raw:
            return default
        if isinstance(valid_options[0], int):
            try:
                val = int(raw)
            except ValueError:
                print("Invalid input. Please enter an integer.")
                continue
        else:
            val = raw.lower()
        if val in valid_options:
            return val
        print(f"Error: '{raw}' is not in {valid_options}.")

def prompt_range(prompt_text, min_val, max_val, default, is_float=False):
    while True:
        print(f"\n{prompt_text}")
        raw = input(f"Enter value ({min_val} to {max_val}, enter -1 for MAX) [{default}]: ").strip()
        if not raw:
            return default
        try:
            val = float(raw) if is_float else int(raw)
            if val == -1 or min_val <= val <= max_val:
                return int(val) if not is_float and val == int(val) else val
            print(f"Error: Value must be between {min_val} and {max_val} (or -1).")
        except ValueError:
            print("Invalid number.")

def prompt_trace_files(default_files=None):
    if default_files is None:
        default_files = ["Trace1.trc", "Trace2_4Evaluation.trc", "Corruption1.trc"]
    files = []
    print("\nSpecify 1 to 3 trace files (-f). Press Enter on an empty prompt when done.")
    for i in range(1, 4):
        df = default_files[i - 1] if i <= len(default_files) else ""
        prompt = f"Trace file #{i}" + (f" [{df}]" if df else " (optional)") + ": "
        val = input(prompt).strip()
        if not val:
            if df and len(files) == 0:
                files.extend(default_files)
            break
        files.append(val)
    return files

def wizard_custom_parameters():
    s = prompt_choice("1. Cache Size in KB (-s):", VALID_CACHE_KB, 512)
    b = prompt_choice("2. Block Size in bytes (-b):", VALID_BLOCK_B, 16)
    a = prompt_choice("3. Associativity (-a):", VALID_ASSOC, 4)
    r = prompt_choice("4. Replacement Policy (-r):", VALID_POLICIES, "rr")
    p = prompt_choice("5. Physical Memory in MB (-p):", VALID_PHYS_MB, 1024)
    n = prompt_range("6. Instructions / Time Slice (-n) (-1 for MAX):", -1, 100000000, 100, is_float=False)
    u = prompt_range("7. Percent Physical Memory Used by System (-u):", 0, 100, 75, is_float=True)
    files = prompt_trace_files()
    return s, b, a, r, p, n, u, files

def interactive_menu():
    while True:
        print("\n" + "=" * 60)
        print("CS 3853 VM & Cache Simulator — Interactive Control Wrapper (Team 12)")
        print("=" * 60)
        print("1: Guided Custom Run")
        print("2: Run Section 4.2 Benchmark Example")
        #print("3: Generate 3 Milestone Deliverable Runs (Team_12_Sim_n_M#Z.txt)")
        #print("4: Run Experiment Matrix Sweep")
        print("0: Exit")

        choice = input("\nSelect an option [1]: ").strip() or "1"

        if choice == "1":
            s, b, a, r, p, n, u, files = wizard_custom_parameters()
            cmd = build_cmd(s, b, a, r, p, n, u, files)
            save = input("\nSave output to a file as well? (Enter filename or leave blank): ").strip()
            run_command(cmd, save_to_file=save if save else None)

        elif choice == "2":
            cmd = build_cmd(
                s=512, b=16, a=4, r="rr", p=1024, n=100, u=75,
                files=["Trace1.trc", "Trace2_4Evaluation.trc", "Corruption1.trc"]
            )
            run_command(cmd)

        elif choice == "3":
            m_num = prompt_choice("Which Milestone are these 3 runs for?", [1, 2, 3], 1)
            default_traces = ["A-9_new_trunk.trc"] if m_num == 1 else ["Trace1.trc", "Trace2_4Evaluation.trc", "Corruption1.trc"]
            files = prompt_trace_files(default_traces)

            mode = prompt_choice("Use 3 preset parameter sets (1) or customize each run (2)?", [1, 2], 1)
            presets = [
                (512, 16, 4, "rr", 1024, 100, 75),
                (1024, 32, 8, "rnd", 2048, 500, 50),
                (256, 64, 2, "rr", 512, -1, 25),
            ]
            for run_idx in range(1, 4):
                print(f"\n--- Configuring Run #{run_idx} of 3 ---")
                if mode == 1:
                    s, b, a, r, p, n, u = presets[run_idx - 1]
                else:
                    s, b, a, r, p, n, u, _ = wizard_custom_parameters()
                out_file = f"Team_12_Sim_{run_idx}_M#{m_num}.txt"
                cmd = build_cmd(s, b, a, r, p, n, u, files, milestone=m_num)
                run_command(cmd, save_to_file=out_file)

        elif choice == "4":
            m_num = prompt_choice("Stop at which Milestone output stage?", [1, 2, 3], 1)
            a = prompt_choice("Associativity (-a):", VALID_ASSOC, 4)
            p = prompt_choice("Physical Memory in MB (-p):", VALID_PHYS_MB, 1024)
            u = prompt_range("OS Memory Percent (-u):", 0, 100, 75, is_float=True)
            n = prompt_range("Instructions / Time Slice (-n):", -1, 100000000, -1, is_float=False)
            files = prompt_trace_files(["Trace1.trc"])

            combos = list(itertools.product([8, 64, 256, 1024], [4, 16, 64], ["rr", "rnd"]))
            for idx, (c_size, b_size, pol) in enumerate(combos, start=1):
                print(f"\n[Run {idx}/{len(combos)}] Cache={c_size}KB, Block={b_size}B, Policy={pol.upper()}")
                cmd = build_cmd(c_size, b_size, a, pol, p, n, u, files, milestone=m_num)
                run_command(cmd)

        elif choice == "0":
            sys.exit(0)
        else:
            print("Invalid selection.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        normalized_args = [arg.replace("–", "-").replace("—", "-") for arg in sys.argv[1:]]
        cmd = [find_executable()] + normalized_args
        run_command(cmd)
    else:
        interactive_menu()