CC = gcc
CFLAGS = -gdwarf-3 -Og -Wall
CFLAGS += -D BUILD_VERSION="\"$(shell git describe --dirty --always)\""	\
		-D BUILD_DATE="\"$(shell date '+%Y-%m-%d %H:%M:%S')\""

LIBPREDICT_DIR = libpredict
LIBPREDICT_SRCS = $(LIBPREDICT_DIR)/julian_date.c \
		$(LIBPREDICT_DIR)/observer.c \
		$(LIBPREDICT_DIR)/orbit.c \
		$(LIBPREDICT_DIR)/refraction.c \
		$(LIBPREDICT_DIR)/sdp4.c \
		$(LIBPREDICT_DIR)/sgp4.c \
		$(LIBPREDICT_DIR)/sun.c \
		$(LIBPREDICT_DIR)/unsorted.c

BIN = gss
SRCS = main.c \
	config.c \
	minIni.c \
        tle.c \
	track.c \
	schedule.c \
	time.c \
	$(LIBPREDICT_SRCS)

LIBS = -lpq -lcurl -lm
INCLUDES = -I/usr/include/postgresql

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(BIN) $(LIBS) $(INCLUDES)

clean:
	rm -fv *.o $(BIN)

