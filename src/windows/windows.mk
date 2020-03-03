# windows/windows.mk: Windows-specific build rules.
#
# The windows build requires an extra object file. This object file
# supplies the application's icon.

OBJ += windows/appres.o
GENRES += windows/app.ico
SRCREC += $(wildcard windows/icon*.png)

windows/appres.o: windows/app.ico
	echo "1 ICON $^" | $(WINDRES) -O coff -o $@

windows/app.ico: $(wildcard windows/icon*.png)
	icotool -c -o $@ $^

# Dummy dependency file, created to avoid generating warnings.
windows/appres.d:
	touch $@
