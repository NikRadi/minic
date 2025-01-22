int main() {
    int x1 = 3;
    printf("%d\n", *&x1);

    int x2, *x3, **x4;
    x2 = 123;
    x3 = &x2;
    x4 = &x3;
    printf("%d\n", **x4);

    int x5 = 321;
    int *x6 = &x5;
    int **x7 = &x6;
    printf("%d\n", **x7);

    int x9 = 5;
    int *x10 = &x9;
    *x10 = 123;
    printf("%d\n", x9);
    printf("%d\n", (&x9 + 2) - &x9 + 3);
    printf("%d\n", (&x9 + 2) - &x9 + 5);
}
