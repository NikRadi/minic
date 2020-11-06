int Sum(int x, int y) {
    int var_x = x;
    int var_y = y;
    int var_xy = x + y;
    return var_xy;
}

void Add6(int *x) {
    *x = *x + 6;
}

void main() {
    int var_x = 123;
    int var_y = 321;
    int var_xy = Sum(var_x, var_y);
    Add6(&var_xy);
    PrintInt(var_xy == 450);
}