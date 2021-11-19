# windows/windows.mk: Windows-specific build rules.
#
# The windows build includes an extra object file, which supplies the
# application's icon (assuming that the windres utility is available).

ifdef WINDRES

OBJ += windows/appres.o
RES += $(wildcard windows/icon*.png) sdlui/gfx/icon.png
GENRES += windows/app.ico

windows/appres.o: windows/app.ico
	echo "1 ICON $^" | $(WINDRES) -O coff -o $@

windows/app.ico: $(wildcard windows/icon*.png)
	icotool -c -o $@ $^

# Dummy dependency file, created to avoid generating warnings.
windows/appres.d:
	touch $@

endif
