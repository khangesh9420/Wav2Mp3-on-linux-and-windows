STATIC_LIB = 1
all:
ifeq ($(OS),Windows_NT)
ifeq ($(STATIC_LIB),1)
	#TODO: lmp3lame.a for win is not statically compiled, so it call dll.
			g++ -v -static main.cpp -lpthread -L./static_lib/win/ -lmp3lame -o exe/All_wave_static_win
			objdump -D exe/All_wave_static_win > obj/obj_static_win
else
			g++ -v main.cpp -lpthread -lmp3lame -o exe/All_wave_dynamic_win
			objdump -D exe/All_wave_dynamic_win > obj/obj_dynamic_win
endif
else
ifeq ($(STATIC_LIB),1)
			g++ -v main.cpp -lpthread -L./static_lib/linux/ -Wl,-Bstatic -lmp3lame -Wl,-Bdynamic  -o exe/All_wave_static_linux
			objdump -D exe/All_wave_static_linux > obj/obj_static_linux
else
			g++ -v main.cpp -lpthread  -lmp3lame -o exe/All_wave_dynamic_linux
			objdump -D exe/All_wave_dynamic_linux > obj/obj_dynamic_linux
endif
endif
