int check(int x) {
    if (x > 0) {
        return 1;
        printf("This will never run\n");
    }
    return 0;
}

int main() {
    int i = 0;
    int safe = 0;
    int flag = 1;

   
    while (i < 10) {
        i--;
    }

    while (safe < 5) {
        safe++;
        if (safe == 3) {
            break;
        }
    }

    while (i < 10 && flag == 1) {
        i--;
    }

    return 0;
}