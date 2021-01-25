void main() {
    int x0=1, *x1=&x0, **x2=&x1;
    PrintInt(x0 == 1 && *x1 == 1 && **x2 == 1);
}