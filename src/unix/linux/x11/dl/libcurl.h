// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#pragma once

#include "sym.h"


// Interface

#define CURLOPT(na, t, nu) na = t + nu

#define CURLOPTTYPE_LONG          0
#define CURLOPTTYPE_OBJECTPOINT   10000
#define CURLOPTTYPE_FUNCTIONPOINT 20000
#define CURLOPTTYPE_STRINGPOINT   CURLOPTTYPE_OBJECTPOINT
#define CURLOPTTYPE_SLISTPOINT    CURLOPTTYPE_OBJECTPOINT
#define CURLOPTTYPE_CBPOINT       CURLOPTTYPE_OBJECTPOINT
#define CURLOPTTYPE_VALUES        CURLOPTTYPE_LONG

#define CURLINFO_LONG   0x200000
#define CURLINFO_SOCKET 0x500000

#define CURL_GLOBAL_SSL   0x1
#define CURL_GLOBAL_WIN32 0x2
#define CURL_GLOBAL_ALL   (CURL_GLOBAL_SSL | CURL_GLOBAL_WIN32)

typedef enum {
	CURLE_OK    = 0,
	CURLE_AGAIN = 81,
} CURLcode;

typedef enum {
	CURLOPT(CURLOPT_WRITEDATA, CURLOPTTYPE_CBPOINT, 1),
	CURLOPT(CURLOPT_URL, CURLOPTTYPE_STRINGPOINT, 2),
	CURLOPT(CURLOPT_PROXY, CURLOPTTYPE_STRINGPOINT, 4),
	CURLOPT(CURLOPT_WRITEFUNCTION, CURLOPTTYPE_FUNCTIONPOINT, 11),
	CURLOPT(CURLOPT_POSTFIELDS, CURLOPTTYPE_OBJECTPOINT, 15),
	CURLOPT(CURLOPT_LOW_SPEED_LIMIT, CURLOPTTYPE_LONG, 19),
	CURLOPT(CURLOPT_LOW_SPEED_TIME, CURLOPTTYPE_LONG, 20),
	CURLOPT(CURLOPT_HTTPHEADER, CURLOPTTYPE_SLISTPOINT, 23),
	CURLOPT(CURLOPT_CUSTOMREQUEST, CURLOPTTYPE_STRINGPOINT, 36),
	CURLOPT(CURLOPT_POSTFIELDSIZE, CURLOPTTYPE_LONG, 60),
	CURLOPT(CURLOPT_HTTP_VERSION, CURLOPTTYPE_VALUES, 84),
	CURLOPT(CURLOPT_NOSIGNAL, CURLOPTTYPE_LONG, 99),
	CURLOPT(CURLOPT_CONNECT_ONLY, CURLOPTTYPE_LONG, 141),
	CURLOPT(CURLOPT_CONNECTTIMEOUT_MS, CURLOPTTYPE_LONG, 156),
} CURLoption;

typedef enum {
	CURLINFO_RESPONSE_CODE = CURLINFO_LONG + 2,
	CURLINFO_ACTIVESOCKET  = CURLINFO_SOCKET + 44,
} CURLINFO;

enum {
	CURL_HTTP_VERSION_NONE,
	CURL_HTTP_VERSION_1_0,
	CURL_HTTP_VERSION_1_1,
	CURL_HTTP_VERSION_2_0,
	CURL_HTTP_VERSION_2TLS,
};

struct curl_slist {
	char *data;
  	struct curl_slist *next;
};

typedef struct Curl_easy CURL;
typedef int curl_socket_t;

static CURLcode (*curl_global_init)(long flags);
static CURL *(*curl_easy_init)(void);
static void (*curl_easy_cleanup)(CURL *curl);
static CURLcode (*curl_easy_setopt)(CURL *curl, CURLoption option, ...);
static CURLcode (*curl_easy_perform)(CURL *curl);
static CURLcode (*curl_easy_send)(CURL *curl, const void *buffer, size_t buflen, size_t *n);
static CURLcode (*curl_easy_recv)(CURL *curl, void *buffer, size_t buflen, size_t *n);
static CURLcode (*curl_easy_getinfo)(CURL *curl, CURLINFO info, ...);
static struct curl_slist *(*curl_slist_append)(struct curl_slist *list, const char *data);
static void (*curl_slist_free_all)(struct curl_slist *list);


// Runtime open

static MTY_Atomic32 LIBCURL_LOCK;
static MTY_SO *LIBCURL_SO;
static bool LIBCURL_INIT;

static void __attribute__((destructor)) libcurl_global_destroy(void)
{
	MTY_GlobalLock(&LIBCURL_LOCK);

	MTY_SOUnload(&LIBCURL_SO);
	LIBCURL_INIT = false;

	MTY_GlobalUnlock(&LIBCURL_LOCK);
}

static bool libcurl_global_init(void)
{
	MTY_GlobalLock(&LIBCURL_LOCK);

	if (!LIBCURL_INIT) {
		bool r = true;
		LIBCURL_SO = MTY_SOLoad("libcurl.so.4");

		if (!LIBCURL_SO) {
			r = false;
			goto except;
		}

		LOAD_SYM(LIBCURL_SO, curl_global_init);
		LOAD_SYM(LIBCURL_SO, curl_easy_init);
		LOAD_SYM(LIBCURL_SO, curl_easy_cleanup);
		LOAD_SYM(LIBCURL_SO, curl_easy_setopt);
		LOAD_SYM(LIBCURL_SO, curl_easy_perform);
		LOAD_SYM(LIBCURL_SO, curl_easy_send);
		LOAD_SYM(LIBCURL_SO, curl_easy_recv);
		LOAD_SYM(LIBCURL_SO, curl_easy_getinfo);
		LOAD_SYM(LIBCURL_SO, curl_slist_append);
		LOAD_SYM(LIBCURL_SO, curl_slist_free_all);

		CURLcode e = curl_global_init(CURL_GLOBAL_ALL);
		if (e != CURLE_OK) {
			MTY_Log("'curl_global_init' failed with error %d", e);
			r = false;
			goto except;
		}

		except:

		if (!r)
			libcurl_global_destroy();

		LIBCURL_INIT = r;
	}

	MTY_GlobalUnlock(&LIBCURL_LOCK);

	return LIBCURL_INIT;
}