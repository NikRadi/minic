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
}
