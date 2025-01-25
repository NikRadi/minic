int main() {
    int x1 = 123;
    printf("%d %d %d\n", sizeof(x1), sizeof x1, sizeof(x1 + 123));
    sizeof(x1 = 3);
    printf("%d\n", x1);

    int *x2;
    char x3;
    printf("%d %d\n", sizeof(x2), sizeof(x3));

}
