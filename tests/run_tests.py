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
        return False

    args = ["tmp.exe"]
    program_result = subprocess.run(args, capture_output=True, text=True)
    color = COLOR_GREEN if program_result.returncode == expected else COLOR_RED
    print_colored(f"Expected {expected}, got {program_result.returncode} ({arg})", color)
    return program_result.returncode == expected


def main():
    test_cases = [
        ("int main() { return 0; }", 0),
        ("int main() { return 42; }", 42),
        ("int main() { return 5+20-4; }", 21),
        ("int main() { return  12 + 34 - 5 ; }", 41),
        ("int main() { return 5+6*7; }", 47),
        ("int main() { return 5*(9-6); }", 15),
        ("int main() { return (3+5)/2; }", 4),
        ("int main() { return -10+20; }", 10),
        ("int main() { return - -10; }", 10),
        ("int main() { return - - +10; }", 10),
        ("int main() { return 0==1; }", 0),
        ("int main() { return 42==42; }", 1),
        ("int main() { return 0!=1; }", 1),
        ("int main() { return 42!=42; }", 0),
        ("int main() { return 0<1; }", 1),
        ("int main() { return 1<1; }", 0),
        ("int main() { return 2<1; }", 0),
        ("int main() { return 0<=1; }", 1),
        ("int main() { return 1<=1; }", 1),
        ("int main() { return 2<=1; }", 0),
        ("int main() { return 1>0; }", 1),
        ("int main() { return 1>1; }", 0),
        ("int main() { return 1>2; }", 0),
        ("int main() { return 1>=0; }", 1),
        ("int main() { return 1>=1; }", 1),
        ("int main() { return 1>=2; }", 0),
        ("int main() { return 1; 2; 3; }", 1),
        ("int main() { int a; a=3; return a; }", 3),
        ("int main() { int z=9; return z; }", 9),
        ("int main() { int a=3, z=5; return a+z; }", 8),
        ("int main() { int a=4, z=6, b=1; return a-b+z; }", 9),
        ("int main() { int a; int b; a=b=3; return a+b; }", 6),
        ("int main() { int foo=3; return foo; }", 3),
        ("int main() { int foo123=3; int bar=5; return foo123+bar; }", 8),
        ("int main() { { 1; { 2; } return 3; } }", 3),
        ("int main() { ;;; return 5; }", 5),
        ("int main() { if (0) return 2; return 3; }", 3),
        ("int main() { if (1-1) return 2; return 3; }", 3),
        ("int main() { if (1) return 2; return 3; }", 2),
        ("int main() { if (2-1) return 2; return 3; }", 2),
        ("int main() { if (0) { 1; 2; return 3; } else { return 4; } }", 4),
        ("int main() { if (1) { 1; 2; return 3; } else { return 4; } }", 3),
        ("int main() { int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }", 55),
        ("int main() { for (;;) {return 3;} return 5; }", 3),
        ("int main() { int i=0; while(i<10) { i=i+1; } return i; }", 10),
        ("int main() { int x=3; return *&x; }", 3),
        ("int main() { int x, *y, **z; x=123; y=&x; z=&y; return **z; }", 123),
        ("int main() { int x=3; int *y=&x; int **z=&y; return **z; }", 3),
        ("int main() { int x=3; int y=5; return *(&x+1); }", 5),
        ("int main() { int x=3; int y=5; return *(&y-1); }", 3),
        ("int main() { int x=3; int y=5; return *(&x-(-1)); }", 5),
        ("int main() { int x=3; int *y=&x; *y=6; return x; }", 6),
        ("int main() { int x=3; int y=5; *(&x+1)=7; return y; }", 7),
        ("int main() { int x=3; int y=5; *(&y-2+1)=6; return x; }", 6),
        ("int main() { int x=3; return (&x+2)-&x+3; }", 5),
        ("int main() { int x=3; return (&x+2)-&x+5; }", 7),
        ("int ret7() { return 7; } int main() { return ret7(); }", 7),
        ("int ret14() { return 14; } int main() { return ret14(); }", 14),
        ("int add(int x, int y) { return x+y; } int main() { return add(3, 5); }", 8),
        ("int add(int x, int y) { return x+y; } int main() { return add(1 + 1 + 1, 2 + 2 - 2); }", 5),
        ("int add(int x, int y) { return x+y; } int main() { return add(add(1, 2), 3); }", 6),
        ("int sub(int x, int y) { return x-y; } int main() { return sub(5, 3); }", 2),
        ("int sub(int x, int y) { return x-y; } int main() { return sub(10, 1 + 2 + 3); }", 4),
        ("int add(int x, int y) { return x+y; } int sub(int x, int y) { return x-y; } int main() { return sub(10, add(1, add(1, 1))); }", 7),
        ("int add6(int x1, int x2, int x3, int x4, int x5, int x6) { return x1+x2+x3+x4+x5+x6; } int main() { return add6(1, 2, 3, 4, 5, 6); }", 21),
        ("int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }", 55),
        ("int main() { int x[2]; int *y=&x; *y=3; return *x; }", 3),
        ("int main() { int x[3]; *x=3; *(x+1)=5; *(x+2)=8; return *x; }", 3),
        ("int main() { int x[3]; *x=3; *(x+1)=5; *(x+2)=8; return *(x+1); }", 5),
        ("int main() { int x[3]; *x=3; *(x+1)=5; *(x+2)=8; return *(x+2); }", 8),
        ("int main() { int x[3][4]; int *y=x; *y=123; return **x; }", 123),
        ("int main() { int x[3][4]; int *y=x; *(y+1) = 0; return **x; }", 1),
        ("int main() { int x[3][4]; int *y=x; *(y+2) = 1; return *(*x+1); }", 2),
        ("int main() { int x[3][4]; int *y=x; *(y+3) = 2; return *(*x+2); }", 3),
        ("int main() { int x[3][4]; int *y=x; *(y+4) = 3; return **x; }", 4),
        ("int main() { int x[3][4]; int *y=x; *(y+5) = 4; return **x; }", 5),
    ]

    num_failed = 0
    for test_case, expected in test_cases:
        if not minic_test(test_case, expected):
            num_failed += 1

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
