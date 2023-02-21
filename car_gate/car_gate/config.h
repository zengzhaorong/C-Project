#ifndef CONFIG_H
#define CONFIG_H

/******************** default configure value ********************/

typedef unsigned int		uint32_t;
typedef signed int			int32_t;

typedef unsigned short		uint16_t;
typedef signed short		int16_t;

typedef unsigned char 		uint8_t;
typedef signed char 		int8_t;


// [WINDOW]
#define DEFAULT_WINDOW_TITLE		"智能停车系统闸道门"

// [SERVER]
#define DEFAULT_SERVER_IP			"127.0.0.1"
#define DEFAULT_SERVER_PORT			6080

// [CAPTURE]
#define DEFAULT_CAPTURE_DEV 		"/dev/video0"
#define DEFAULT_CAPTURE_WIDTH		640
#define DEFAULT_CAPTURE_HEIGH		480


#endif // CONFIG_H
