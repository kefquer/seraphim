!include [lib]

class NoName {
   invisible:
    int:x::absolute = 2;
    absolute::string:y = "autizm";
    :freedom = "svobodnaya peremennaya s auto tipom";
   visible:
    bool:editing = false;

    absolute::freed:function() {
        sprinp(int:input::absolute, <size>, <arrow_moving>);
        sproute("Hello World"L:end);
    };

    int:function1() {
        function:func; /// записывает возврощаемые данные функцией в переменную
        //function():func; / запускает функцию
        sproute(input);

        return input;
    };
};

freed:main() {
    NoName:nn;
    nn.function1();

    done;
};
