/* Copyright 1998 Invasion Games.  All Rights Reserved.  Do NOT distribute. */

/* dxdecode.h for client */

/* Main header file for dxdecode.cpp */

#ifndef DXDECODE_H
#define DXDECODE_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

extern const char* DirectXDecodeError(HRESULT errorval);

#endif /* DXDECODE_H */
