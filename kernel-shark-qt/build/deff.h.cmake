// !-- Do Not Hand Edit - This File is generated by CMAKE --!

#ifndef _KS_CONFIG_H
#define _KS_CONFIG_H

#cmakedefine LSB_DISTRIB "@LSB_DISTRIB@"

#cmakedefine DESKTOP_SESSION "@DESKTOP_SESSION@"

#cmakedefine DO_AS_ROOT "@DO_AS_ROOT@"

#cmakedefine KS_VERSION_STRING "@KS_VERSION_STRING@"

#cmakedefine KS_DIR "@KS_DIR@"

#cmakedefine TRACECMD_BIN_DIR "@TRACECMD_BIN_DIR@"

#cmakedefine N_CPUS @N_CPUS@

#ifdef __cplusplus

	#include <QString>
	const QString plugins = "@PLUGINS@";

#endif /* __cplusplus */

#endif // _KS_CONFIG_H
