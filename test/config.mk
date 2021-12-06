EXEC = tests

CC = gcc
INCLUDE = include
CFLAGS = -std=c99 -O2 -pedantic $(patsubst -I%,-I../%,${A_INCLUDE}) -I${INCLUDE} -Wall
LDFLAGS= -lcunit ${A_LDFLAGS}

SRC_DIR = src
OBJ_DIR = obj


