int add2(int x, int y) {
    return x + y;
}

int add6(int x1, int x2, int x3, int x4, int x5, int x6) {
    return x1 + x2 + x3 + x4 + x5 + x6;
}

int sub2(int x, int y) {
    return x - y;
}

int fib(int x) {
    if (x <= 1) {
        return 1;
    }

    return fib(x - 1) + fib(x - 2);
}

int ret7() {
    return 7;
}

int ret14() {
    return ret7() + ret7();
}

int main() {
    int x1 = ret7();
    printf("%d\n", x1);

    x1 = ret14();
    printf("%d\n", x1);

    x1 = add2(3, 5);
    printf("%d\n", x1);

    x1 = add2(1 + 1 + 1, 2 + 2 - 2);
    printf("%d\n", x1);

    x1 = add2(add2(1, 2), 3);
    printf("%d\n", x1);

    x1 = sub2(5, 3);
    printf("%d\n", x1);

    x1 = sub2(10, 1 + 2 + 3);
    printf("%d\n", x1);

    x1 = sub2(10, add2(1, add2(1, 1)));
    printf("%d\n", x1);

    x1 = add6(1, 2, 3, 4, 5 6);
    printf("%d\n", x1);

    x1 = fib(9);
    printf("%d\n", x1);
}
