int Get5() {
    return 5;
}

void main() {
    int x = Get5() + 2 + Get5() - 1;
    PrintInt(x == 11);
}