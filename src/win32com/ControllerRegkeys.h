#ifndef CONTROLLERREGKEYS_H
#define CONTROLLERREGKEYS_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define REG_BASEKEY		"Software\\Freeware\\Visual PinMame"
#define REG_DEFAULT		"default"
#define REG_GLOBALS		"globals"

// this one is the only which can't set by a property
#define REG_DWALLOWWRITEACCESS			"AllowWriteAccess"
#define REG_DWALLOWWRITEACCESSDEF		1

#endif // CONTROLLERREGKEYS_H
