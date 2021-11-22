# Compiler / package config

NAME = httposms-server
VERSION = v0.0.0-prerelease

CC = gcc
INCLUDE = -Iinclude $(shell xml2-config --cflags)
CFLAGS = -std=c99 -O2 -pedantic ${INCLUDE} -Wall -DNAME="${NAME}" -DVERSION="${VERSION}"
LDFLAGS= -lcurl -pthread $(shell xml2-config --libs)

SRC_DIR = src
OBJ_DIR = obj

# Install params
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

ifeq (${CI},true)
LDFLAGS += -lz
endif

