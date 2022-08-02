#ifndef  _MISC_H__
#define  _MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "http.h"
#include "wrstring.h"

void DBG(const char *fmt, ...);
void WEB_DBG(const char *fmt, ...);
void errorLog(const char *mess);
// int url_decode(const char *src ,char *dst ,int is_form_url_encoded);
int httpd_url_decode(const char *src, char *dst, int is_form_url_encoded);
int UTF8toANSI(char *src);


static const char *methods[3] = {"HEAD", "GET", "POST"};


/*copy form moogoose*/
static const struct {
    const char	*ext;
    const char	*type;
} mmt[] = {
    {"html",	"text/html"			},
    {"htm",		"text/html"			},
    {"shtm",	"text/html"			},
    {"shtml",	"text/html"			},
    {"css",		"text/css"			},
    {"js",		"application/x-javascript"	},
    {"ico",		"image/x-icon"			},
    {"gif",		"image/gif"			},
    {"jpg",		"image/jpeg"			},
    {"jpeg",	"image/jpeg"			},
    {"png",		"image/png"			},
    {"svg",		"image/svg+xml"			},
    {"torrent",	"application/x-bittorrent"	},
    {"wav",		"audio/x-wav"			},
    {"mp3",		"audio/x-mp3"			},
    {"mid",		"audio/mid"			},
    {"m3u",		"audio/x-mpegurl"		},
    {"ram",		"audio/x-pn-realaudio"		},
    {"ra",		"audio/x-pn-realaudio"		},
    {"doc",		"application/msword",		},
    {"exe",		"application/octet-stream"	},
    {"zip",		"application/x-zip-compressed"	},
    {"xls",		"application/excel"		},
    {"tgz",		"application/x-tar-gz"		},
    {"tar.gz",	"application/x-tar-gz"		},
    {"tar",		"application/x-tar"		},
    {"gz",		"application/x-gunzip"		},
    {"arj",		"application/x-arj-compressed"	},
    {"rar",		"application/x-arj-compressed"	},
    {"rtf",		"application/rtf"		},
    {"pdf",		"application/pdf"		},
    {"swf",		"application/x-shockwave-flash"	},
    {"mpg",		"video/mpeg"			},
    {"mpeg",	"video/mpeg"			},
    {"asf",		"video/x-ms-asf"		},
    {"avi",		"video/x-msvideo"		},
    {"bmp",		"image/bmp"			},
    {NULL,		NULL}
};

#ifdef __cplusplus
}
#endif
#endif
