#include "HeaderFile.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "user_console {identificador da consola}");
		
		return -1;
	}
	
	initializeSemaphore();
	
	printCommands();   //São imprimidos os comandos disponíveis na user console.
	
	
	
	return 0;
}
