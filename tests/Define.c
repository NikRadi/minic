#define MY_VAR1 10
#define MY_VAR2 20
#define MY_VAR3 (1 + 2)
#define PLUS +
#define PRINTF printf("%d\n"

int main() {
    printf("%d %d %d\n", MY_VAR1, MY_VAR2, MY_VAR1 + MY_VAR2);
    printf("%d %d\n", MY_VAR3, MY_VAR3 PLUS MY_VAR3);
    PRINTF, MY_VAR1);
}
