#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define CityName_Size  256

struct MemoryStruct {
  char *memory;
  size_t size;
};

void GetCityName(int argc, char* argv[], char CityNameArray[CityName_Size]);
void KeepOpenWindow();
void PrintWeatherForecast(struct MemoryStruct* chunk);
char* GetURL(char CityNameArray[CityName_Size]);
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
size_t strlcpy(char *dst, const char *src, size_t dsize);

int main(int argc, char* argv[0]) {

    char CityNameArray[CityName_Size];
    CURL *curl_handle;
    CURLcode res;
    struct MemoryStruct chunk;

    GetCityName(argc, argv, CityNameArray);
    if(strlen(CityNameArray) == 0){
        printf("\nUnknown city.Execution aborted.\n");
        KeepOpenWindow();
        return 1;
    }

    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl_handle = curl_easy_init();

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, GetURL(CityNameArray));

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    /* some servers do not like requests that are made without a user-agent
     field, so we provide one */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* get it! */
    res = curl_easy_perform(curl_handle);

    /* check for errors */
    if(res == CURLE_OK)
        PrintWeatherForecast(&chunk);
    else
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    free(chunk.memory);

    /* we are done with libcurl, so clean it up */
    curl_global_cleanup();

    return 0;

}

void PrintWeatherForecast(struct MemoryStruct* chunk){

    cJSON* FullMsg = cJSON_ParseWithLength(chunk->memory, chunk->size);
    cJSON *Tmp1, *Tmp2, *Tmp3;

    // Описание погоды
    Tmp1 = cJSON_GetArrayItem(FullMsg->child->child, 16);
    printf("\nWeather: %s;\n", Tmp1->child->child->valuestring);

    // Направление и скорость ветра
    Tmp1 = cJSON_GetArrayItem(FullMsg->child->child, 18);
    Tmp2 = cJSON_GetArrayItem(FullMsg->child->child, 20);
    printf("Wind direction: %s; speed (km/h): %s\n", Tmp1->valuestring, Tmp2->valuestring);

    // Дневной диапазон температуры
    Tmp3 = FullMsg->child->next->next->next;
    Tmp2 = cJSON_GetArrayItem(Tmp3->child, 5);
    Tmp1 = cJSON_GetArrayItem(Tmp3->child, 7);
    printf("Temperature (C): %s - %s\n", Tmp1->valuestring, Tmp2->valuestring);

    KeepOpenWindow();

}

char* GetURL(char CityNameArray[CityName_Size]){

    size_t URLlen = strlen(CityNameArray) + 27;
    char* URL = calloc(1, URLlen * sizeof(char));

    strcat(URL, "https://wttr.in/");
    strcat(URL,  CityNameArray);
    strcat(URL, "?format=j1");
    *(URL + 33) = '\0';

    return URL;
}

// Получает имя города.
void GetCityName(int argc, char* argv[], char CityNameArray[CityName_Size]){

    // Обрабатываем аргументы командной строки (если они есть в "достаточном" количестве)
    if(argc == 2){
        strlcpy(CityNameArray, argv[1], CityName_Size - 1);
    }
    else{

        // Если аргументы отсутсвует, работаем через интерфейс
        printf("        Welcome to the weather forecast program.\n");

        printf("Enter city's name (max len - 255ch): ");
        scanf("%255s", CityNameArray);
    }
}

// Просто держит окно терминала открытым.
void KeepOpenWindow(){

    char EmptyEnter[1];
    scanf("%1s", EmptyEnter);
}

size_t strlcpy(char *dst, const char *src, size_t dsize) {

	const char *osrc = src;
	size_t nleft = dsize;

	/* Copy as many bytes as will fit. */
	if (nleft != 0) {
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src. */
	if (nleft == 0) {
		if (dsize != 0)
			*dst = '\0';		/* NUL-terminate dst */
		while (*src++)
			;
	}

	return(src - osrc - 1);	/* count does not include NUL */
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {

  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(!ptr) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;

}
