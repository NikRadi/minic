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
        return -1

    args = ["tmp.exe"]
    program_result = subprocess.run(args, capture_output=True, text=True)
    color = COLOR_GREEN if program_result.returncode == expected else COLOR_RED
    print_colored(f"Expected {expected}, got {program_result.returncode} ({arg})", color)
    return -1 if color == COLOR_RED else 0


def main():
    num_failed = 0
    num_failed += minic_test("int main() { return 0; }", 0)
    num_failed += minic_test("int main() { return 42; }", 42)
    num_failed += minic_test("int main() { return 5+20-4; }", 21)
    num_failed += minic_test("int main() { return  12 + 34 - 5 ; }", 41)
    num_failed += minic_test("int main() { return 5+6*7; }", 47)
    num_failed += minic_test("int main() { return 5*(9-6); }", 15)
    num_failed += minic_test("int main() { return (3+5)/2; }", 4)
    num_failed += minic_test("int main() { return -10+20; }", 10)
    num_failed += minic_test("int main() { return - -10; }", 10)
    num_failed += minic_test("int main() { return - - +10; }", 10)
    num_failed += minic_test("int main() { return 0==1; }", 0)
    num_failed += minic_test("int main() { return 42==42; }", 1)
    num_failed += minic_test("int main() { return 0!=1; }", 1)
    num_failed += minic_test("int main() { return 42!=42; }", 0)
    num_failed += minic_test("int main() { return 0<1; }", 1)
    num_failed += minic_test("int main() { return 1<1; }", 0)
    num_failed += minic_test("int main() { return 2<1; }", 0)
    num_failed += minic_test("int main() { return 0<=1; }", 1)
    num_failed += minic_test("int main() { return 1<=1; }", 1)
    num_failed += minic_test("int main() { return 2<=1; }", 0)
    num_failed += minic_test("int main() { return 1>0; }", 1)
    num_failed += minic_test("int main() { return 1>1; }", 0)
    num_failed += minic_test("int main() { return 1>2; }", 0)
    num_failed += minic_test("int main() { return 1>=0; }", 1)
    num_failed += minic_test("int main() { return 1>=1; }", 1)
    num_failed += minic_test("int main() { return 1>=2; }", 0)
    num_failed += minic_test("int main() { return 1; 2; 3; }", 1)
    num_failed += minic_test("int main() { int a; a=3; return a; }", 3)
    num_failed += minic_test("int main() { int z=9; return z; }", 9)
    num_failed += minic_test("int main() { int a=3, z=5; return a+z; }", 8)
    num_failed += minic_test("int main() { int a=4, z=6, b=1; return a-b+z; }", 9)
    num_failed += minic_test("int main() { int a; int b; a=b=3; return a+b; }", 6)
    num_failed += minic_test("int main() { int foo=3; return foo; }", 3)
    num_failed += minic_test("int main() { int foo123=3; int bar=5; return foo123+bar; }", 8)
    num_failed += minic_test("int main() { { 1; { 2; } return 3; } }", 3)
    num_failed += minic_test("int main() { ;;; return 5; }", 5)
    num_failed += minic_test("int main() { if (0) return 2; return 3; }", 3)
    num_failed += minic_test("int main() { if (1-1) return 2; return 3; }", 3)
    num_failed += minic_test("int main() { if (1) return 2; return 3; }", 2)
    num_failed += minic_test("int main() { if (2-1) return 2; return 3; }", 2)
    num_failed += minic_test("int main() { if (0) { 1; 2; return 3; } else { return 4; } }", 4)
    num_failed += minic_test("int main() { if (1) { 1; 2; return 3; } else { return 4; } }", 3)
    num_failed += minic_test("int main() { int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }", 55)
    num_failed += minic_test("int main() { for (;;) {return 3;} return 5; }", 3)
    num_failed += minic_test("int main() { int i=0; while(i<10) { i=i+1; } return i; }", 10)
    num_failed += minic_test("int main() { int x=3; return *&x; }", 3)
    num_failed += minic_test("int main() { int x, *y, **z; x=123; y=&x; z=&y; return **z; }", 123)
    num_failed += minic_test("int main() { int x=3; int *y=&x; int **z=&y; return **z; }", 3)
    num_failed += minic_test("int main() { int x=3; int y=5; return *(&x+1); }", 5)
    num_failed += minic_test("int main() { int x=3; int y=5; return *(&y-1); }", 3)
    num_failed += minic_test("int main() { int x=3; int y=5; return *(&x-(-1)); }", 5)
    num_failed += minic_test("int main() { int x=3; int *y=&x; *y=6; return x; }", 6)
    num_failed += minic_test("int main() { int x=3; int y=5; *(&x+1)=7; return y; }", 7)
    num_failed += minic_test("int main() { int x=3; int y=5; *(&y-2+1)=6; return x; }", 6)
    num_failed += minic_test("int main() { int x=3; return (&x+2)-&x+3; }", 5)
    num_failed += minic_test("int main() { int x=3; return (&x+2)-&x+5; }", 7)
    num_failed += minic_test("int ret7() { return 7; } int main() { return ret7(); }", 7)
    num_failed += minic_test("int ret14() { return 14; } int main() { return ret14(); }", 14)
    num_failed += minic_test("int add(int x, int y) { return x+y; } int main() { return add(3, 5); }", 8)
    num_failed += minic_test("int add(int x, int y) { return x+y; } int main() { return add(1 + 1 + 1, 2 + 2 - 2); }", 5)
    num_failed += minic_test("int add(int x, int y) { return x+y; } int main() { return add(add(1, 2), 3); }", 6)
    num_failed += minic_test("int sub(int x, int y) { return x-y; } int main() { return sub(5, 3); }", 2)
    num_failed += minic_test("int sub(int x, int y) { return x-y; } int main() { return sub(10, 1 + 2 + 3); }", 4)
    num_failed += minic_test("int add(int x, int y) { return x+y; } int sub(int x, int y) { return x-y; } int main() { return sub(10, add(1, add(1, 1))); }", 7)
    num_failed += minic_test("int add6(int x1, int x2, int x3, int x4, int x5, int x6) { return x1+x2+x3+x4+x5+x6; } int main() { return add6(1, 2, 3, 4, 5, 6); }", 21)
    num_failed += minic_test("int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }", 55)
    num_failed += minic_test("int main() { int x[2]; int *y=&x; *y=3; return *x; }", 3)
    num_failed += minic_test("int main() { int x[3]; *x=3; *(x+1)=5; *(x+2)=8; return *x; }", 3)
    num_failed += minic_test("int main() { int x[3]; *x=3; *(x+1)=5; *(x+2)=8; return *(x+1); }", 5)
    num_failed += minic_test("int main() { int x[3]; *x=3; *(x+1)=5; *(x+2)=8; return *(x+2); }", 8)

    minic_size_bytes = os.path.getsize("bin\minic.exe")
    minic_size_kbytes = minic_size_bytes / 1024

    print()
    print('PASS' if num_failed == 0 else f'FAILED: {-num_failed}')
    print(f"Size of minic.exe: {minic_size_kbytes}kb")

    cloc_result = subprocess.run(["cloc", "--csv", "--quiet", "src"], capture_output=True, text=True)
    stdout_lines = cloc_result.stdout.splitlines()
    minic_lines = stdout_lines[-1].split(',')[-1]
    print(f"Lines of code: {minic_lines}")
    print()


if __name__ == "__main__":
    main()
