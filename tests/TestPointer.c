void main() {
    int x = 2;
    int *xp = &x;
    int **xpp = &xp;
    int ***xppp = &xpp;
    int ****xpppp = &xppp;
    ****xpppp = 20;
    PrintInt(x == 20);
}