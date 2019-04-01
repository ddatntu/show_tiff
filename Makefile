# https://www3.ntu.edu.sg/home/ehchua/programming/cpp/gcc_make.html
# https://www.cs.swarthmore.edu/~newhall/unixhelp/howto_makefiles.html

# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#  add x11 and tiff libraries
CFLAGS  = -g -Wall -lX11 -ltiff

# the build target executable:
TARGET = show_tiff

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) $(TARGET)
