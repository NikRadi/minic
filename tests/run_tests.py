import glob
import os
import subprocess


COLOR_RED = "\033[91m"
COLOR_GREEN = "\033[92m"
COLOR_END = "\033[0m"


def run_test(c_file, verbose):
    # Compile with minic
    compile_cmd = [".\\bin\\minic", c_file]
    try:
        compile_result = subprocess.run(compile_cmd, capture_output=True)
    except subprocess.CalledProcessError:
        if verbose:
            print("Failed to compile")
        return False

    if compile_result.returncode != 0:
        test_passed = False
        status = f"{COLOR_GREEN}PASS{COLOR_END}" if test_passed else f"{COLOR_RED}FAIL{COLOR_END}"
        print(f"{c_file + ' ':.<40} {status}")

        if verbose:
            print("Failed to compile")
            print(compile_result.stderr.decode())

        return False

    # Run the compiled executable
    try:
        program_result = subprocess.run("tmp.exe", capture_output=True, text=True)
    except subprocess.CalledProcessError:
        if verbose:
            print("Failed to run")
        return False

    actual_result = program_result.stdout.strip()

    # Load expected result
    base_name = os.path.splitext(c_file)[0]
    out_file = f"{base_name}.out"
    try:
        with open(out_file, 'r') as f:
            expected_output = f.read().strip()
    except FileNotFoundError:
        return False

    test_passed = actual_result == expected_output
    status = f"{COLOR_GREEN}PASS{COLOR_END}" if test_passed else f"{COLOR_RED}FAIL{COLOR_END}"
    print(f"{c_file + ' ':.<40} {status}")

    if not test_passed and verbose:
        print("Differences (expected vs actual)")

        from itertools import zip_longest
        actual_lines = actual_result.splitlines()
        expected_lines = expected_output.splitlines()
        for expected, actual in zip_longest(expected_lines, actual_lines, fillvalue="?"):
            if expected != actual:
                print(COLOR_RED, end="")

            print(f"{expected:<5} {actual}")
            if expected != actual:
                print(COLOR_END, end="")

    return test_passed


def main():
    c_files = glob.glob(os.path.join(".\\tests", "*.c"))
    num_failed = 0
    for c_file in c_files:
        test_passed = run_test(c_file, verbose=True)
        if not test_passed:
            num_failed += 1

    minic_size_bytes = os.path.getsize("bin\minic.exe")
    minic_size_kbytes = minic_size_bytes / 1024

    print()
    print('Tests succeeded' if num_failed == 0 else f'Tests failed: {num_failed}')
    print(f"Size of minic.exe: {minic_size_kbytes}kb")

    cloc_result = subprocess.run(["cloc", "--csv", "--quiet", "src"], capture_output=True, text=True)
    stdout_lines = cloc_result.stdout.splitlines()
    minic_lines = stdout_lines[-1].split(',')[-1]
    print(f"Lines of code: {minic_lines}")
    print()


if __name__ == "__main__":
    main()
