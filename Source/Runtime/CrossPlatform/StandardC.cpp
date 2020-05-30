#include "CrossPlatform.h"
#include "StandardC.h"


__BEGIN_NAMESPACE

static ErrorCode g_PlatformLastErrorCode;
void SetErrorCode(ErrorCode err)
{
	g_PlatformLastErrorCode = err;
}

ErrorCode GetLastErrorCode()
{
	ErrorCode err = g_PlatformLastErrorCode;
	g_PlatformLastErrorCode = ErrorCode::OK;
	return err;
}

__END_NAMESPACE
