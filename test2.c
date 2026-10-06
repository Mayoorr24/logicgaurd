int check(int x) {
    if (x > 0) {
        return 1;
        printf("This will never run\n");
    }
    return 0;
}

int process(int y) {
    while (y > 0) {
        y--;
        break;
        printf("This is dead too\n");
    }
    return y;
}