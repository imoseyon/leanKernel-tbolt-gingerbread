#define MY_API_VERSION          "2.0"
#define PROCFS_NAME             "nsio"

#define IOBUF_SIZE          512

#define MAX_LOG_RECORDS         128
#define MAX_FILTER_RECORDS      64

#define MAX_DATA_LEN          	1024


typedef struct {
    struct timespec ctime;
    __be32  saddr;
    __be32  daddr;
    __be16  sport;
    __be16  dport;
    __u8    protocol;
    unsigned char  urg:1;
    unsigned short ack:1;
    unsigned short psh:1;
    unsigned short rst:1;
    unsigned short syn:1;
    unsigned short fin:1;
    char	*data;
    u32	    action;
    u32	    conn_id;
    u32	    matched;
    u16	    datalen;
} log_rec_s;

typedef struct {
    u32 	id;
    u32	    action;
    u32	    hitcount;
    u16	    port;
    char	*data;
    char	*match_host;
    char	*match_req;
    char	*match_all;
    char	*match_new;
} filter_rec_s;

enum {
    API_GET_VERSION = 0,
    API_GET_FILTERS,
    API_RESET_FILTER_COUNTERS,
    API_CLEAR_LOG,
    API_SET_FILTER,
    API_GET_LOG,
    API_GET_PACKET,
    API_CLEAR_FILTERS,
    API_MASTER_TOGGLE,
    API_LAST_COMMAND
};

#define ACT_LOG     0x100
#define ACT_IGNORE  0x00
#define ACT_DROP    0x01
#define ACT_ALTER   0x02 

