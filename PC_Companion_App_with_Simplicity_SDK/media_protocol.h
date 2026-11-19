#ifndef MEDIA_PROTOCOL_H
#define MEDIA_PROTOCOL_H

// Media playlist request protocol
//
// Request values:
// 0xFFFF: Request full playlist
// 0xFFFE: Request next batch of playlist entries
// 0x0000 - 0xFFFD: Request a specific playlist index
//
#define PLAYLIST_REQUEST_FULL  0xFFFF
#define PLAYLIST_REQUEST_NEXT  0xFFFE

#endif
