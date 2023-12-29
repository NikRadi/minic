import os
import subprocess


COLOR_RED = "\033[91m"
COLOR_GREEN = "\033[92m"
COLOR_YELLOW = "\033[93m"


def print_colored(text, color):
    print(color, end='')
    print(text, end='')
    print('\033[0m')


def minic_test(arg, expected):
    args = ["bin\minic", arg]
    minic_result = subprocess.run(args, capture_output=True, text=True)

    args = ["tmp.exe"]
    program_result = subprocess.run(args, capture_output=True, text=True)
    color = COLOR_GREEN if program_result.returncode == expected else COLOR_RED
    print_colored(f"Expected {expected}, got {program_result.returncode} ({arg})", color)


def main():
    minic_test("{ return 0; }", 0)
    minic_test("{ return 42; }", 42)
    minic_test("{ return 5+20-4; }", 21)
    minic_test("{ return  12 + 34 - 5 ; }", 41)
    minic_test("{ return 5+6*7; }", 47)
    minic_test("{ return 5*(9-6); }", 15)
    minic_test("{ return (3+5)/2; }", 4)
    minic_test("{ return -10+20; }", 10)
    minic_test("{ return - -10; }", 10)
    minic_test("{ return - - +10; }", 10)
    minic_test("{ return 0==1; }", 0)
    minic_test("{ return 42==42; }", 1)
    minic_test("{ return 0!=1; }", 1)
    minic_test("{ return 42!=42; }", 0)
    minic_test("{ return 0<1; }", 1)
    minic_test("{ return 1<1; }", 0)
    minic_test("{ return 2<1; }", 0)
    minic_test("{ return 0<=1; }", 1)
    minic_test("{ return 1<=1; }", 1)
    minic_test("{ return 2<=1; }", 0)
    minic_test("{ return 1>0; }", 1)
    minic_test("{ return 1>1; }", 0)
    minic_test("{ return 1>2; }", 0)
    minic_test("{ return 1>=0; }", 1)
    minic_test("{ return 1>=1; }", 1)
    minic_test("{ return 1>=2; }", 0)
    minic_test("{ return 1; 2; 3; }", 1)
    minic_test("{ a=3; return a; }", 3)
    minic_test("{ z=9; return z; }", 9)
    minic_test("{ a=3; z=5; return a+z; }", 8)
    minic_test("{ a=4; z=6; b=1; return a-b+z; }", 9)
    minic_test("{ a=b=3; return a+b; }", 6)
    minic_test("{ foo=3; return foo; }", 3)
    minic_test("{ foo123=3; bar=5; return foo123+bar; }", 8)
    minic_test("{ { 1; { 2; } return 3; } }", 3)
    minic_test("{ ;;; return 5; }", 5)
    minic_test("{ if (0) return 2; return 3; }", 3)
    minic_test("{ if (1-1) return 2; return 3; }", 3)
    minic_test("{ if (1) return 2; return 3; }", 2)
    minic_test("{ if (2-1) return 2; return 3; }", 2)
    minic_test("{ if (0) { 1; 2; return 3; } else { return 4; } }", 4)
    minic_test("{ if (1) { 1; 2; return 3; } else { return 4; } }", 3)
    minic_test("{ i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }", 55)
    minic_test("{ for (;;) {return 3;} return 5; }", 3)
    minic_test("{ i=0; while(i<10) { i=i+1; } return i; }", 10)
    minic_test("{ x=3; return *&x; }", 3)
    minic_test("{ x=3; y=&x; z=&y; return **z; }", 3)
    minic_test("{ x=3; y=5; return *(&x+8); }", 5)
    minic_test("{ x=3; y=5; return *(&y-8); }", 3)
    minic_test("{ x=3; y=&x; *y=5; return x; }", 5)
    minic_test("{ x=3; y=5; *(&x+8)=7; return y; }", 7)
    minic_test("{ x=3; y=5; *(&y-8)=6; return x; }", 6)


if __name__ == "__main__":
    main()

    minic_size_bytes = os.path.getsize("bin\minic.exe")
    minic_size_kbytes = minic_size_bytes / 1024
    print()
    print(f"Size of minic.exe: {minic_size_kbytes}kb")

    cloc_result = subprocess.run(["cloc", "--csv", "--quiet", "src"], capture_output=True, text=True)
    stdout_lines = cloc_result.stdout.splitlines()
    minic_lines = stdout_lines[-1].split(',')[-1]
    print(f"Lines of code: {minic_lines}")
    print()
