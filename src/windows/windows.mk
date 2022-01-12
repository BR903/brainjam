# windows/windows.mk: Windows-specific build rules.
#
# The windows build includes an extra object file, which supplies the
# icon and a bit of XML that sets the character set to UTF-8.

ifdef WINDRES

OBJ += windows/appres.o
RES += $(wildcard windows/icon*.png) sdlui/gfx/icon.png
GENRES += windows/app.ico

%.o: %.rc
	$(WINDRES) -O coff -o $@ $<

windows/app.ico: $(wildcard windows/icon*.png)
	icotool -c -o $@ $^

# Sadly, the dependencies for the resource object must be created manually.
windows/appres.d:
	echo "windows/appres.d windows/appres.o: windows/appres.rc \\" > $@
	echo " windows/app.ico windows/utf8.xml" >> $@

endif
