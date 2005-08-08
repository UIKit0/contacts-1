
default: contacts

clean:
	rm -f ./contacts

contacts: main.c ../contacts.glade
	gcc -g -o contacts main.c `pkg-config --libs --cflags glib-2.0 gtk+-2.0 libglade-2.0 libebook-1.2`
