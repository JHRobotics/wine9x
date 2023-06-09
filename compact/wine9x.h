#ifndef __WINE9X_VERSION_H__INCLUDED__

#define WINE9X_STR_(x) #x
#define WINE9X_STR(x) WINE9X_STR_(x)

#define WINE9X_MAJOR 1
#define WINE9X_MINOR 17
#define WINE9X_PATCH 55

#ifndef WINE9X_BUILD
#define WINE9X_BUILD 0
#endif

#define WINE9X_VERSION_STR_BUILD(_ma, _mi, _pa, _bl) \
	_ma "." _mi "." _pa "." _bl

#define WINE9X_VERSION_STR WINE9X_VERSION_STR_BUILD( \
	WINE9X_STR(WINE9X_MAJOR), \
	WINE9X_STR(WINE9X_MINOR), \
	WINE9X_STR(WINE9X_PATCH), \
	WINE9X_STR(WINE9X_BUILD))

#define WINE9X_VERSION_NUM \
 	WINE9X_MAJOR,WINE9X_MINOR,WINE9X_PATCH,WINE9X_BUILD \

#endif /* __WINE9X_VERSION_H__INCLUDED__ */
