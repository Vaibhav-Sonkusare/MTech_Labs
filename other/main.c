#include <stdio.h>
#include <unistd.h>

void displayPrompt();
void read_cdir();

char cdir[1024] = {0};

int main() {
	
	while (1) {
		displayPrompt();

	}
	return 0;
}

void displayPrompt() {
	fprintf(stdout, "\n%s $", cdir);
}
