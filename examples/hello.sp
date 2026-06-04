!include [spio]

freed:main() {
    int:x = 10;
    int:y = 20;
    int:c = x + y;
    sproute(c);
    done;
};
