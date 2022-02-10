all: sp


sp: sp.c
	gcc sp.c -o sp -Wall -Wextra -Wpedantic -g

