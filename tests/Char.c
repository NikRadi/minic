int add3(char x, char y, char z) {
    return x + y + z;
}

int main() {
    char x1 = 11;
    char x2 = 22;
    printf("%d\n", x1);
    printf("%d\n", x2);
    printf("%d\n", sizeof(x1));

    char x3 = add3(1, 2, 3);
    printf("%d\n", x3);
}
