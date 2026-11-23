#ifndef MEDIA_PROTOCOL_H
#define MEDIA_PROTOCOL_H

// Request values:
// 0xFFFF: Request first batch entries of the playlist
// 0xFFFE: Request next batch of playlist entries
// 0x0000 - 0xFFFD: Request a specific playlist index
//
// The PC also sends the first batch on its own when the app starts and
// whenever the queue changes. An entry with index 0 starts a new list:
// the device should clear its list when it receives one.
// An entry with total == 0 means the queue is empty.

#define PLAYLIST_REQUEST_FIRST  0xFFFF
#define PLAYLIST_REQUEST_NEXT  0xFFFE

#define PLAYLIST_FIRST_BATCH_SIZE 6
#define PLAYLIST_BATCH_SIZE 3

#define MEDIA_MSG_PLAYLIST_ENTRY 0x01
#define MEDIA_MSG_PLAYLIST_NOW_PLAYLING 0x02

#define MEDIA_TITLE_LENGTH 64
#define MEDIA_ARTIST_LENGTH 64

typedef struct __attribute__((packed)){
    uint8_t type;
    uint16_t index;
    uint16_t total;
    char title[MEDIA_TITLE_LENGTH];
} media_playlist_entry_t;

typedef struct __attribute__((packed)){
    uint8_t type;
    uint8_t is_playing;
    uint16_t progress_s;
    uint16_t duration_s;

    char title[MEDIA_TITLE_LENGTH];
    char artist[MEDIA_ARTIST_LENGTH];
} media_now_playing_t;

#endif
