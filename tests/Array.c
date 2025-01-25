/* Testing multi line comments */
int main() {
    int x1[3];
    int *y1 = &x1;
    *y1 = 123;
    printf("%d\n", *x1);

    *x1 = 10;
    *(x1 + 1) = 20;
    *(x1 + 2) = 30;
    printf("%d %d %d\n", *x1, *(x1 + 1), *(x1 + 2));

    x1[0] = 11;
    x1[1] = 22;
    x1[2] = 33;
    printf("%d %d %d\n", x1[0], x1[1], x1[2]);

    char x2[3];
    char *y2 = &x2;
    *y2 = 321;
    printf("%d\n", *x2);

    *x2 = 100;
    *(x2 + 1) = 200;
    *(x2 + 2) = 300;
    printf("%d %d %d\n", *x2, *(x2 + 1), *(x2 + 2));

    x2[0] = 110;
    x2[1] = 210;
    x2[2] = 310;
    printf("%d %d %d\n", x2[0], x2[1], x2[2]);
}
