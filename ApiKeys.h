//	Third-party service credentials.
//
//	Never commit real keys. To build with real credentials:
//	  1. Copy ApiKeys.local.example.h to ApiKeys.local.h (it is gitignored).
//	  2. Fill in the values.
//	  3. Add JAANGLE_LOCAL_KEYS to the project's Preprocessor Definitions.
//
//	Without JAANGLE_LOCAL_KEYS the build uses empty keys; the Last.fm API key
//	can also be entered at runtime (Options > information providers > Last.fm).
#ifndef _ApiKeys_h_
#define _ApiKeys_h_

#ifdef JAANGLE_LOCAL_KEYS
#include "ApiKeys.local.h"
#endif

//Last.fm API 2.0 key (biographies). Free: https://www.last.fm/api/account/create
#ifndef JAANGLE_LASTFM_API_KEY
#define JAANGLE_LASTFM_API_KEY ""
#endif

#endif
