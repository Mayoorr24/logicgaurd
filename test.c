int check(int x) {
    if (x > 0) {
        return 1;
        printf("This will never run\n");
    }
    return 0;
}

int main() {
    int i = 0;
    int count = 5;

    while (i < 10) {
        i--;
    }

    while (count > 0) {
        count--;
        if (count == 2) {
            break;
        }
    }

    return 0;
}