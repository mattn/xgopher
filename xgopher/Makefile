SRCS = \
	xgopher.c

OBJS = $(subst .c,.o,$(SRCS))

CFLAGS = -Wall
LIBS = -lX11 -lXext -lXpm -ljansson
TARGET = xgopher

all : $(TARGET)

$(TARGET) : $(OBJS)
	gcc -g -o $@ $(OBJS) $(LIBS)

.c.o :
	gcc -g -c $(CFLAGS) -I. $< -o $@

clean :
	rm -f *.o $(TARGET)
