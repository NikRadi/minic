int main() {
    int x1[3];
    int *y1 = &x1;
    *y1 = 123;
    printf("%d\n", *x1);

    *x1 = 10;
    *(x1 + 1) = 20;
    *(x1 + 2) = 30;
    printf("%d\n", *x1);
    printf("%d\n", *(x1 + 1));
    printf("%d\n", *(x1 + 2));

    x1[0] = 11;
    x1[1] = 22;
    x1[2] = 33;
    printf("%d\n", x1[0]);
    printf("%d\n", x1[1]);
    printf("%d\n", x1[2]);

    int x2[3][4];
    int *y2 = x2;
    *y2 = 100;
    *(y2 + 1) = 200;
    *(y2 + 2) = 300;
    *(y2 + 3) = 400;
    *(y2 + 4) = 500;
    *(y2 + 5) = 600;
    printf("%d\n", **x2);
    printf("%d\n", *(*x2 + 1));
    printf("%d\n", *(*x2 + 2));
    printf("%d\n", *(*x2 + 3));
    printf("%d\n", *(*x2 + 4));
    printf("%d\n", *(*x2 + 5));
}
