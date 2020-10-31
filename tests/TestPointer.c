void main() {
    int x = 0;
    int *xptr1 = &x;
    int *xptr2 = xptr1;
    *xptr1 = 20;
    x = *xptr2 + 3;
    PrintInt(x == 23);
}