void main() {
    int i = 5;
    for (i = 0; i < 100; i = i + 2) {
        i = i - 1;
    }

    PrintInt(i == 100);
}