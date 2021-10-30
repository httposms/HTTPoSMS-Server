# Compiler / package config

NAME = httposms-server
VERSION = v0.0.0-prerelease

CC = gcc
CFLAGS = -std=c99 -O2 -pedantic -Iinclude -Wall -DNAME="${NAME}" -DVERSION="${VERSION}"
LDFLAGS= 

SRC_DIR = src
OBJ_DIR = obj

# Install params
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man


