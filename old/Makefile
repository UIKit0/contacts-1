
SOURCES = contacts-main.c contacts-utils.c contacts-callbacks-ui.c contacts-callbacks-ebook.c contacts-edit-pane.c
INCLUDES = contacts-main.h contacts-utils.h contacts-callbacks-ui.h contacts-callbacks-ebook.h contacts-edit-pane.h

default: contacts

clean:
	rm -f ./contacts

contacts: $(SOURCES) $(INCLUDES) ../data/contacts.glade
	gcc -Wall -g -o contacts -DPKGDATADIR='"../data"' $(SOURCES) `pkg-config --libs --cflags glib-2.0 gtk+-2.0 libglade-2.0 libebook-1.2`
