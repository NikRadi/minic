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
    if minic_result.returncode != 0:
        print_colored(f'failed compile {arg}', COLOR_RED)
        return

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
    minic_test("{ int a; a=3; return a; }", 3)
    minic_test("{ int z=9; return z; }", 9)
    minic_test("{ int a=3, z=5; return a+z; }", 8)
    minic_test("{ int a=4, z=6, b=1; return a-b+z; }", 9)
    minic_test("{ int a; int b; a=b=3; return a+b; }", 6)
    minic_test("{ int foo=3; return foo; }", 3)
    minic_test("{ int foo123=3; int bar=5; return foo123+bar; }", 8)
    minic_test("{ { 1; { 2; } return 3; } }", 3)
    minic_test("{ ;;; return 5; }", 5)
    minic_test("{ if (0) return 2; return 3; }", 3)
    minic_test("{ if (1-1) return 2; return 3; }", 3)
    minic_test("{ if (1) return 2; return 3; }", 2)
    minic_test("{ if (2-1) return 2; return 3; }", 2)
    minic_test("{ if (0) { 1; 2; return 3; } else { return 4; } }", 4)
    minic_test("{ if (1) { 1; 2; return 3; } else { return 4; } }", 3)
    minic_test("{ int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }", 55)
    minic_test("{ for (;;) {return 3;} return 5; }", 3)
    minic_test("{ int i=0; while(i<10) { i=i+1; } return i; }", 10)
    minic_test("{ int x=3; return *&x; }", 3)
    minic_test("{ int x, *y, **z; x=123; y=&x; z=&y; return **z; }", 123)
    minic_test("{ int x=3; int *y=&x; int **z=&y; return **z; }", 3)
    minic_test("{ int x=3; int y=5; return *(&x+1); }", 5)
    minic_test("{ int x=3; int y=5; return *(&y-1); }", 3)
    minic_test("{ int x=3; int y=5; return *(&x-(-1)); }", 5)
    minic_test("{ int x=3; int *y=&x; *y=6; return x; }", 6)
    minic_test("{ int x=3; int y=5; *(&x+1)=7; return y; }", 7)
    minic_test("{ int x=3; int y=5; *(&y-2+1)=6; return x; }", 6)
    minic_test("{ int x=3; return (&x+2)-&x+3; }", 5)
    minic_test("{ int x=3; return (&x+2)-&x+5; }", 7)
    minic_test("{ return ret7(); }", 7)
    minic_test("{ return ret14(); }", 14)
    minic_test("{ return add(3, 5); }", 8)
    minic_test("{ return add(1 + 1 + 1, 2 + 2 - 2); }", 5)
    minic_test("{ return add(add(1, 2), 3); }", 6)
    minic_test("{ return sub(5, 3); }", 2)
    minic_test("{ return sub(10, 1 + 2 + 3); }", 4)
    minic_test("{ return sub(10, add(1, add(1, 1))); }", 7)
    minic_test("{ return add6(1, 2, 3, 4, 5, 6); }", 21)

    minic_size_bytes = os.path.getsize("bin\minic.exe")
    minic_size_kbytes = minic_size_bytes / 1024
    print()
    print(f"Size of minic.exe: {minic_size_kbytes}kb")

    cloc_result = subprocess.run(["cloc", "--csv", "--quiet", "src"], capture_output=True, text=True)
    stdout_lines = cloc_result.stdout.splitlines()
    minic_lines = stdout_lines[-1].split(',')[-1]
    print(f"Lines of code: {minic_lines}")
    print()


if __name__ == "__main__":
    main()
