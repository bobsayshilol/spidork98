# Build libpc98 first so that the examples can use it
.build:
	for %n in (libpc98 basic bgm gfx fire bench pcm mouse ayylmao demo) do make -C %n
	make -C libpc98 tools

all: .build
	mkdir exes
	for %n in (basic bgm gfx fire bench pcm mouse ayylmao demo) do copy %n\\%n.exe exes
	copy libpc98\\snd_conv.exe exes
	copy libpc98\\img_conv.exe exes

clean:
	for %n in (libpc98 basic bgm gfx fire bench pcm mouse ayylmao demo) do make -C %n clean
	del /Q exes
