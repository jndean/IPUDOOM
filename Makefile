

CPU_OBJ = $(addprefix build/cpu_obj/, ipu_host.o ipu_transfer.o i_system.o d_mode.o i_main.o m_argv.o m_misc.o net_common.o net_dedicated.o net_io.o net_packet.o net_query.o net_sdl.o net_server.o net_structrw.o aes_prng.o d_event.o d_iwad.o d_loop.o gusconf.o i_cdmus.o i_input.o i_joystick.o i_sdlmusic.o i_sdlsound.o i_sound.o i_timer.o i_video.o i_videohr.o midifile.o mus2mid.o m_bbox.o m_cheat.o m_config.o m_controls.o m_fixed.o net_client.o sha1.o memio.o tables.o v_video.o w_checksum.o w_main.o w_wad.o w_file.o w_file_stdc.o w_file_posix.o w_merge.o z_zone.o net_loop.o am_map.o d_items.o d_main.o d_net.o doomdef.o doomstat.o dstrings.o f_wipe.o g_game.o hu_lib.o hu_stuff.o info.o m_menu.o m_random.o p_ceilng.o p_doors.o p_enemy.o p_floor.o p_inter.o p_lights.o p_map.o p_maputl.o p_mobj.o p_plats.o p_pspr.o p_saveg.o p_setup.o p_sight.o p_spec.o p_switch.o p_telept.o p_tick.o p_user.o r_bsp.o r_data.o r_draw.o r_main.o r_plane.o r_segs.o r_sky.o r_things.o s_sound.o sounds.o st_lib.o st_stuff.o statdump.o wi_stuff.o)
IPU_OBJ = $(addprefix build/ipu_obj/, \
	ipu_vertices.gp \
	ipu_print.gp \
	ipu_transfer.gp \
	ipu_malloc.gp \
	g_game.gp \
	g_game_codelets.gp \
	am_map.gp \
	m_fixed.gp \
	m_controls.gp \
	p_setup.gp \
	p_setup_codelets.gp \
	p_tick.gp \
	tables.gp \
)



CPU_FLAGS = -I src -I /usr/local/include/SDL2 -I/usr/include/libpng16 -I/opt/poplar/include \
      		-D_REENTRANT -lSDL2 -lSDL2_mixer -lSDL2_net -lpng16 -lz -lpoplar \
     		-Wall -Werror \
      		-O2

IPU_FLAGS = -I src/ipu \
			-Wall -Werror -Wno-unused-variable \
			-O2


all: build build/doom build/ipu_rt.gp

build/doom: $(CPU_OBJ)
	g++ $^ $(CPU_FLAGS) -o build/doom

build/cpu_obj/%.o: src/%.c #src/*.h
	gcc src/$*.c $(CPU_FLAGS) -c -o $@

build/cpu_obj/%.o: src/%.cpp #src/*.h
	gcc src/$*.cpp $(CPU_FLAGS) -c -o $@

build/ipu_rt.gp: $(IPU_OBJ)
	popc $^ $(IPU_FLAGS) -o $@

build/ipu_obj/%.gp: src/ipu/%.c src/ipu/*.h
	popc $< $(IPU_FLAGS) -o $@

build/ipu_obj/%.gp: src/ipu/%.cpp src/*.h
	popc $< $(IPU_FLAGS) -o $@

 	
build:
	mkdir -p build/cpu_obj build/ipu_obj build/pop_obj

clean:
	rm -r build/