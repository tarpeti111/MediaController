#include "spotify.h"
#include "model.h"
#include "input_config.h"

#include <curl/curl.h>
#include "cJSON.h"

#include "openssl/rand.h"
#include "openssl/sha.h"
#include "openssl/bio.h"
#include "openssl/buffer.h"
#include "openssl/evp.h"

#include "winsock2.h"
#include <process.h>

#define URI_REDIRECT "http://127.0.0.1:3000/callback"
#define CLIENT_ID "83b1ec911c044b0fb10307816951f3a8"

#define MAX_TRACK_NAME_LENGHT 64
#define MAX_TRACK_ID_LENGHT 64
#define MAX_ARTIST_NAME_LENGHT 64
#define MAX_DEVICE_NAME_LENGHT 64
#define MAX_DEVICE_ID_LENGHT 64

//https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
struct memory {
  char *response;
  size_t size;
};

typedef struct {
    char name[MAX_TRACK_NAME_LENGHT];
    char id[MAX_TRACK_ID_LENGHT];
    bool is_playing;
    char artist_name[MAX_ARTIST_NAME_LENGHT];
} track_info_t;

typedef struct {
    char name[MAX_DEVICE_NAME_LENGHT];
    char id[MAX_DEVICE_ID_LENGHT];
    uint8_t volume;
} spotify_device_t;

typedef enum {
    SPOTIFY_IDLE,
    SPOTIFY_WAIT_AUTH,
    SPOTIFY_HAVE_CODE,
    SPOTIFY_REFRESHING,
    SPOTIFY_READY
} spotify_state_t;

typedef CURLcode(*spotify_action_t)(void);

static spotify_state_t state = SPOTIFY_IDLE;
static spotify_device_t current_device = {0};
static track_info_t current_track = {0};

static const char spotify_image[] = "11111111111110000001111111111111\
                                    11111111110000000000001111111111\
                                    11111111000000000000000011111111\
                                    11111100000000000000000000111111\
                                    11111000000000000000000000011111\
                                    11110000000000000000000000001111\
                                    11100000000000000000000000000111\
                                    11100000000000000000000000000111\
                                    11000000001111111100000000000011\
                                    11000011111111111111110000000011\
                                    10000111111111111111111110000001\
                                    10000111111111111111111111100001\
                                    10000010000000000000011111110001\
                                    00000000000000000000000011110000\
                                    00000000111111111110000000100000\
                                    00000011111111111111110000000000\
                                    00000011111111111111111100000000\
                                    00000000000000000001111110000000\
                                    00000000000000000000001110000000\
                                    10000000011111111100000000000001\
                                    10000001111111111111100000000001\
                                    10000001111000001111111000000001\
                                    11000000000000000000111100000011\
                                    11000000000000000000001000000011\
                                    11100000000000000000000000000111\
                                    11100000000000000000000000000111\
                                    11110000000000000000000000001111\
                                    11111000000000000000000000011111\
                                    11111100000000000000000000111111\
                                    11111111000000000000000011111111\
                                    11111111110000000000001111111111\
                                    11111111111110000001111111111111";

static CURL* curl = NULL;

static char verifier[65];
static char access_token[512] = {'\0'};
static char refresh_token[512] = {'\0'};
static char auth_code[512] = {'\0'};

static void setup_callback_server(){
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2),&wsa) != 0){
        printf("Failed WSAStartup, error: %d", WSAGetLastError());
        return;
    }

    SOCKET sServer = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;

    server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(3000);
    server.sin_family = AF_INET;

    bind(sServer, (struct sockaddr*)&server, sizeof(server));
    listen(sServer, 1);

    SOCKET sClient = accept(sServer, NULL, NULL);
    char buf[1024];
    int recvCount = recv(sClient, buf, sizeof(buf) - 1, 0);
    if (recvCount > 0) buf[recvCount] = '\0';

    char *cp = strstr(buf, "code=");

    if(cp != NULL){
        cp += 5;

        size_t i;
        for(i = 0; i < sizeof(auth_code) - 1 && cp[i] != '\0' && cp[i] != '&' && cp[i] != ' '; i++){
            auth_code[i] = cp[i];
        }
        auth_code[i] = '\0';
    }

    const char *response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<html><body>You can close this window.</body><script>window.close();</script></html>";

    send(sClient, response, strlen(response), 0);

    shutdown(sClient, SD_SEND);

    closesocket(sClient);
    closesocket(sServer);
    WSACleanup();

    state = SPOTIFY_HAVE_CODE;

    _endthread();
}

static void generate_verifier(char* dest, size_t len){
    const char possible[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    unsigned char values[len];
    
    if(RAND_bytes(values, (int)len) == 1){
        for (size_t i = 0; i < len; i++){
            dest[i] = possible[values[i] % (sizeof(possible) - 1)]; 
        }
    }

    dest[len] = '\0';
}

//Remember to free this later
static char* base64url_encode(const unsigned char* buffer, size_t length) {
    //base64
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); 
    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    char* b64text = (char*)malloc(bufferPtr->length + 1);
    memcpy(b64text, bufferPtr->data, bufferPtr->length);
    b64text[bufferPtr->length] = '\0';

    BIO_free_all(bio);

    //url
    for (int i = 0; b64text[i] != '\0'; i++) {
        if (b64text[i] == '+') b64text[i] = '-';
        else if (b64text[i] == '/') b64text[i] = '_';
    }
    
    char* padding = strchr(b64text, '=');
    if (padding) *padding = '\0';
    
    //Remember to free this later
    return b64text;
}

static void get_auth_code() {
    generate_verifier(verifier, 64);

    unsigned char raw_hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)verifier, strlen(verifier), raw_hash);

    char* code_challenge = base64url_encode(raw_hash, SHA256_DIGEST_LENGTH);

    char cmd[1024];

    snprintf(cmd, sizeof(cmd),
        "start \"\" \"https://accounts.spotify.com/authorize?client_id=%s&response_type=code"
        "&redirect_uri=%s&code_challenge_method=S256"
        "&code_challenge=%s"
        "&scope=user-read-playback-state%%20user-modify-playback-state"
        "&prompt=consent\"",
        CLIENT_ID, URI_REDIRECT, code_challenge
    );

    system(cmd);

    free(code_challenge);
}

//https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
static size_t write_cb(char *data, size_t size, size_t nmemb, void *clientp)
{
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)clientp;
 
  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if(!ptr)
    return 0;  /* out of memory */
 
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
 
  return realsize;
}

//Dummy function needed for curl to not output to the terminal
static size_t discard_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}

static void exchange_code(){
    curl_easy_reset(curl);
    if(strlen(auth_code) == 0) return;
    
    char post[1024];
    snprintf(post, sizeof(post), "grant_type=authorization_code&code=%s&redirect_uri=%s&client_id=%s&code_verifier=%s", auth_code, URI_REDIRECT, CLIENT_ID, verifier);
    
    //Remember to free .response later
    struct memory mem = {0};
    
    //Remember to free this later
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.spotify.com/api/token");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK){
        printf("curl error: %s\n", curl_easy_strerror(res));
    }

    //Remember to use delete on this later
    cJSON* json = cJSON_Parse(mem.response);

    if(json == NULL){
        printf("Error parsing json");
        return;
    }

    cJSON* acctoken = cJSON_GetObjectItemCaseSensitive(json, "access_token");
    cJSON* reftoken = cJSON_GetObjectItemCaseSensitive(json, "refresh_token");
    if(cJSON_IsString(acctoken) && acctoken->valuestring != NULL){
        snprintf(access_token, sizeof(access_token), "%s", acctoken->valuestring);
    }

    if(cJSON_IsString(reftoken) && reftoken->valuestring != NULL){
        snprintf(refresh_token, sizeof(refresh_token), "%s", reftoken->valuestring);
    }

    cJSON_Delete(json);
    free(mem.response);
    curl_slist_free_all(headers);
}

static void refresh_access(){
    curl_easy_reset(curl);
    
    char post[1024];
    snprintf(post, sizeof(post), "grant_type=refresh_token&refresh_token=%s&client_id=%s", refresh_token, CLIENT_ID);

    //Remember to free .response later
    struct memory mem = {0};
    
    //Remember to free this later
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.spotify.com/api/token");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK){
        printf("curl error: %s\n", curl_easy_strerror(res));
    }

    cJSON* json = cJSON_Parse(mem.response);

    if(json == NULL){
        printf("Error parsing json");
        return;
    }

    cJSON* acctoken = cJSON_GetObjectItemCaseSensitive(json, "access_token");
    if(cJSON_IsString(acctoken) && acctoken->valuestring != NULL){
        snprintf(access_token, sizeof(access_token), "%s", acctoken->valuestring);
    }

    cJSON_Delete(json);
    free(mem.response);
    curl_slist_free_all(headers);
}

static void spotify_on_launch(){
    if(state != SPOTIFY_IDLE) return;

    if(strlen(refresh_token) > 0){
        state = SPOTIFY_REFRESHING;
    }
    else{
        _beginthread(setup_callback_server, 2048, NULL);
        get_auth_code();
        state = SPOTIFY_WAIT_AUTH;
    }
}

static CURLcode update_current_track_and_device(){
    curl_easy_reset(curl);

    //Remember to free later
    struct curl_slist *headers = NULL;

    char token_str[600];
    snprintf(token_str, sizeof(token_str), "Authorization: Bearer %s", access_token);

    headers = curl_slist_append(headers, token_str);

    //Remember to free .response later
    struct memory mem = {0};

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.spotify.com/v1/me/player/currently-playing");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // Handle No Content
    if(http_code == 204){
        current_track.artist_name[0] = '\0';
        current_track.id[0] = '\0';
        current_track.is_playing = false;
        current_track.name[0] = '\0';

        curl_slist_free_all(headers);
        free(mem.response);
        return res;
    }

    if(!mem.response){
        curl_slist_free_all(headers);
        free(mem.response);
        return res;
    }

    //Remember to use delete on this later
    cJSON* json = cJSON_Parse(mem.response);

    if(!json){
        printf("Error parsing json");
        return res;
    }

    cJSON* device = cJSON_GetObjectItemCaseSensitive(json, "device");
    cJSON* track = cJSON_GetObjectItemCaseSensitive(json, "item");
    cJSON* is_playing = cJSON_GetObjectItemCaseSensitive(json, "is_playing");
    if(device){
        cJSON* device_name = cJSON_GetObjectItemCaseSensitive(device, "name");
        cJSON* device_id = cJSON_GetObjectItemCaseSensitive(device, "id");
        cJSON* device_volume = cJSON_GetObjectItemCaseSensitive(device, "volume_percent");
        if(cJSON_IsString(device_name) && device_name->valuestring != NULL){
            snprintf(current_device.name, MAX_DEVICE_NAME_LENGHT, "%s", device_name->valuestring);
        }
        if(cJSON_IsString(device_id) && device_id->valuestring != NULL){
            snprintf(current_device.id, MAX_DEVICE_ID_LENGHT, "%s", device_id->valuestring);
        }
        if(cJSON_IsNumber(device_volume)){
            current_device.volume = device_volume->valueint;
        }
    }
    if(track){
        cJSON* track_name = cJSON_GetObjectItemCaseSensitive(track, "name");
        cJSON* track_id = cJSON_GetObjectItemCaseSensitive(track, "id");
        cJSON* artists = cJSON_GetObjectItemCaseSensitive(track, "artists");
        if(cJSON_IsString(track_name) && track_name->valuestring != NULL){
            snprintf(current_track.name, MAX_TRACK_NAME_LENGHT, "%s", track_name->valuestring);
        }
        if(cJSON_IsString(track_id) && track_id->valuestring != NULL){
            snprintf(current_track.id, MAX_TRACK_ID_LENGHT, "%s", track_id->valuestring);
        }
        if(artists){
            cJSON* main_artist = cJSON_GetArrayItem(artists, 0);
            if(main_artist){
                cJSON* main_artist_name = cJSON_GetObjectItemCaseSensitive(main_artist, "name");
                if(cJSON_IsString(main_artist_name) && main_artist_name->valuestring != NULL){
                    snprintf(current_track.artist_name, MAX_ARTIST_NAME_LENGHT, "%s", main_artist_name->valuestring);
                }
            }
        }
    }
    if(cJSON_IsBool(is_playing)){
        current_track.is_playing = cJSON_IsTrue(is_playing);
    }
    
    curl_slist_free_all(headers);
    cJSON_Delete(json);
    free(mem.response);
    
    return res;
}

static CURLcode spotify_request(const char* url, const char* method){
    curl_easy_reset(curl);

    struct curl_slist *headers = NULL;

    char token_str[600];
    snprintf(token_str, sizeof(token_str), "Authorization: Bearer %s", access_token);

    headers = curl_slist_append(headers, token_str);
    headers = curl_slist_append(headers, "Content-Type: application/json");

    char url_full[512];
    if(current_device.id[0] != '\0'){
        snprintf(url_full, sizeof(url_full), "%s?device_id=%s", url, current_device.id);
    }
    else{
        snprintf(url_full, sizeof(url_full), "%s", url);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url_full);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_cb);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    return res;
}

static CURLcode pause_playback(){
    return spotify_request("https://api.spotify.com/v1/me/player/pause", "PUT");
}

static CURLcode resume_playback(){
    return spotify_request("https://api.spotify.com/v1/me/player/play", "PUT");
}

static CURLcode skip_forward(){
    return spotify_request("https://api.spotify.com/v1/me/player/next", "POST");
}

static CURLcode skip_backward(){
    return spotify_request("https://api.spotify.com/v1/me/player/previous", "POST");
}

//Perform an action such as resume_playback, if it fails due to an expired token, get a new one via the refresh token and try again
static void perform_action(spotify_action_t action){
    CURLcode res = action();
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // Handle expired token
    if(http_code == 401){
        refresh_access();

        if(strlen(access_token) > 0){
            res = action();
        }
    }
    if(res != CURLE_OK){
        printf("curl error: %s\n", curl_easy_strerror(res));
    }
}

static void spotify_on_input(uint16_t inputs){
    if(state != SPOTIFY_READY) return;
    curl_easy_reset(curl);

    if(inputs & BTN_SELECT){
        
    }
    if(inputs & BTN_START){
        
    }
    if(inputs & JOY_C){
        update_current_track_and_device();
        if(!current_track.is_playing){
            perform_action(resume_playback);
        }
        else{
            perform_action(pause_playback);
        }
    }
    if(inputs & JOY_N){

    }
    if(inputs & JOY_E){
        perform_action(skip_forward);
        update_current_track_and_device();
    }
    if(inputs & JOY_S){

    }
    if(inputs & JOY_W){
        perform_action(skip_backward);
        update_current_track_and_device();
    }
}

static void spotify_process_action(){
    switch(state){
        case SPOTIFY_HAVE_CODE:
            exchange_code();
            if(strlen(access_token) > 0){
                state = SPOTIFY_READY;
            }
            break;

        case SPOTIFY_REFRESHING:
            access_token[0] = '\0';
            refresh_access();

            if(strlen(access_token) > 0){
                state = SPOTIFY_READY;
            } else {
                // fallback to full auth
                _beginthread(setup_callback_server, 2048, NULL);
                get_auth_code();
                state = SPOTIFY_WAIT_AUTH;
            }
            break;

        default:
            break;
    }
}

void spotify_init(){
    curl = curl_easy_init();
    model_add_app("Spotify", spotify_image, spotify_on_launch, spotify_on_input, spotify_process_action);
}
