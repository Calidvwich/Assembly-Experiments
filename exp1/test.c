int a = 1;
int b = 2;

int f(int x, int y) {
    int z = x + y * (a - b) / 2 % 3;
    if (z < 10 && z > 0 || z <= 20 && z >= 5 || z == 7 || z != 8) {
        z = z + 1;
    } else {
        z = z - 1;
    }
    while (z < 100) {
        z = z + 1;
        if (z == 50) break;
        continue;
    }
    return z;
}

void g() {
    int i = 0;
    i = f(i, a);
}

int main() {
    int result = 0;
    result = f(1, 2);
    g();
    putint(result);
    return 0;
}
