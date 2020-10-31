void main() {
    int x;
    if (1 == 1) {
        x = 1;
    }
    else if (1 > 2) {
        x = 2;
    }
    else if (1 <= 2) {
        x = 3;
    }
    else {
        x = 4;
    }

    PrintInt(x == 1);
}