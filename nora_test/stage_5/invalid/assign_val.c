int main() {
    int a;
    int b = a = 0; // assign exprs are illegal
    return b;
}
