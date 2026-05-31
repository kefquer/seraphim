#include "spio.h"
#include <stdio.h>
#include <string.h>

void __seraphim_sproute(const char* str, const char* modifier) {
    if (modifier == NULL || strlen(modifier) == 0) {
        printf("%s", str);
    } else if (strcmp(modifier, "end") == 0) {
        printf("%s\n", str);
    } else if (strcmp(modifier, "space") == 0) {
        printf("%s ", str);
    } else {
        printf("%s%s", str, modifier);
    }
}
