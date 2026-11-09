//	Third-party service credentials.
//
//	Never commit real keys. To build with real credentials:
//	  1. Copy ApiKeys.local.example.h to ApiKeys.local.h (it is gitignored).
//	  2. Fill in the values.
//	  3. Add JAANGLE_LOCAL_KEYS to the project's Preprocessor Definitions.
//
//	Without JAANGLE_LOCAL_KEYS the build uses empty keys and the
//	corresponding online providers simply fail their requests.
#ifndef _ApiKeys_h_
#define _ApiKeys_h_

#ifdef JAANGLE_LOCAL_KEYS
#include "ApiKeys.local.h"
#endif

#ifndef JAANGLE_AWS_ACCESS_KEY_ID
#define JAANGLE_AWS_ACCESS_KEY_ID ""
#endif

#ifndef JAANGLE_AWS_SECRET_KEY
#define JAANGLE_AWS_SECRET_KEY ""
#endif

#endif
