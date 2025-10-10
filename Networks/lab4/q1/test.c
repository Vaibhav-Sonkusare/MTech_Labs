#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

double evaluate_expression(const char *expr) {
    double result = 0;
    double current = 0;
    char op = '+';
    const char *p = expr;

    while (*p) {
        // Skip whitespace
        while (isspace(*p)) p++;

        // Read number
        if (sscanf(p, "%lf", &current) != 1) {
            return 0; // Invalid input
        }

        // Apply previous operator
        switch (op) {
            case '+': result += current; break;
            case '-': result -= current; break;
            case '*': result *= current; break;
            case '/': 
                if (current == 0) return 0; // divide by zero
                result /= current; 
                break;
        }

        // Move pointer past the number
        while (*p && (isdigit(*p) || *p == '.' || *p == '-')) p++;

        // Skip whitespace
        while (isspace(*p)) p++;

        // Read next operator
        if (*p) {
            op = *p;
            p++;
        }
    }

    return result;
}

int main() {
    const char *expr = "100 + 200 - 200  * 9";
    printf("Result: %lf\n", evaluate_expression(expr));
    return 0;
}

