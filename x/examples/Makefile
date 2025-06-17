# Build libpc98 first so that the examples can use it
.build:
	for %n in (libpc98 basic bgm gfx fire bench pcm mouse) do make -C %n

all: .build
	mkdir exes
	for %n in (basic bgm gfx fire bench pcm mouse) do copy %n\\%n.exe exes

clean:
	for %n in (libpc98 basic bgm gfx fire bench pcm mouse) do make -C %n clean
	del /Q exes
