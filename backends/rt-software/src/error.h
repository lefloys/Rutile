#ifndef RTSW_ERROR_H
#define RTSW_ERROR_H

#include "config.h"
#include "rutile.h"

#include <stdarg.h>

struct rtsw_context;

RTSW_EXTERN_C_ENTER

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

RTSW_API void rtSetOutput(rt_output output, void* user_data);
RTSW_API enum rt_error rtError(void);
RTSW_API const char* rtErrorMessage(void);
RTSW_API void rtClearError(void);

/*===============================================================================================*/
/*                                                                                               */
/*===============================================================================================*/

void rtsw_printf(const char* format, ...);
void rtsw_vprintf(const char* format, va_list args);
void rtsw_throwf(enum rt_error error, const char* format, ...);
enum rt_error rtsw_error(void);
void rtsw_clear_error(void);
void rtsw_error_attach_context(struct rtsw_context* context);

RTSW_EXTERN_C_EXIT

#endif
