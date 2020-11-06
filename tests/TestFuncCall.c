int Sum(int x, int y) {
    int var_x = x;
    int var_y = y;
    int var_xy = x + y;
    return var_xy;
}

void main() {
    int var_x = 123;
    int var_y = 321;
    int var_xy = Sum(var_x, var_y);
    PrintInt(var_xy == 444);
}