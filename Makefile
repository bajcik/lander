
# project: lander
#    file: Makefile
# license: GNU General Public Licence
#  author: Krzysztof Garus <kgarus@bigfoot.com>
#          http://www.bigfoot.com/~kgarus

NAME = lander
VERSION = 0.1
FULL = $(NAME)-$(VERSION)

CC = gcc
#CFLAGS = -s -O2
CFLAGS = -g

CFL = $(CFLAGS) $(shell sdl-config --cflags)
LIBS = $(shell sdl-config --libs) -lSDL_image -lSDL_gfx

$(NAME): $(NAME).o
	gcc -o $@ $(NAME).o $(LIBS)

$(NAME).o: $(NAME).c
	gcc -o $@ -c $(CFL) $<

dist:
	rm -rf $(FULL)
	mkdir $(FULL)
	cp $(NAME).c Makefile COPYING README ChangeLog *.png *.xcf rec $(FULL)
	tar czvf $(FULL).tar.gz $(FULL)
	rm -r $(FULL)

clean:
	rm -f *.o $(NAME)


