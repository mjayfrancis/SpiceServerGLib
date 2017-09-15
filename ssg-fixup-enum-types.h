#if 0

// For use by glib-mkenums for types that are not fully defined as enums
// Don't include this directly

typedef enum {
    SPICE_ADDR_FLAG_IPV4_ONLY = (1 << 0),
    SPICE_ADDR_FLAG_IPV6_ONLY = (1 << 1),
    SPICE_ADDR_FLAG_UNIX_ONLY = (1 << 2),
} spice_addr_flags_t;

typedef enum {
    SPICE_STREAM_VIDEO_INVALID,
    SPICE_STREAM_VIDEO_OFF,
    SPICE_STREAM_VIDEO_ALL,
    SPICE_STREAM_VIDEO_FILTER
} spice_stream_video_t;

// spice-server.h defines these with bitshifts but this isn't a bitfield
// -> use plain ints to avoid glib-mkenums making this into flags rather than an enum
typedef enum {
    SPICE_CHANNEL_SECURITY_NONE = 1,
    SPICE_CHANNEL_SECURITY_SSL = 2
} spice_channel_security_t;

#endif
