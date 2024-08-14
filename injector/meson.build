include_dir = include_directories('include')
str_include_dir = join_paths(meson.current_source_dir(), 'include')

# Assemble the payloads
launcher_payload_bin = asm_gen.process(
    'src/launcher_p.asm', 
    extra_args: [ '-i', str_include_dir ]
)

game_payload_bin = asm_gen.process(
    'src/game_p.asm',
    extra_args: [ '-i', str_include_dir ]
)

# Embed them into .o files
launcher_p_res_files = custom_target(
    'launcher_p.[oh]',
    output: [ 'launcher_p.o', 'launcher_p.h' ],
    input: [ launcher_payload_bin ],
    command: [ gen_res, '--header', '--object', './injector', '@OUTPUT0@', '@OUTPUT1@', '@INPUT@' ]
)

game_p_res_files = custom_target(
    'game_p.[oh]',
    output: [ 'game_p.o', 'game_p.h' ],
    input: [ game_payload_bin ],
    command: [ gen_res, '--header', '--object', './injector', '@OUTPUT0@', '@OUTPUT1@', '@INPUT@' ]
)

# Dumper injector exe
executable(
    'harmonic_dumper',
    'src/dumper_exe.c',
    'src/inject.c',
    game_p_res_files,
    include_directories: include_dir,
    name_prefix: '',
    link_args: '-municode'
)

# Loader injector exe
executable(
    'harmonic_loader',
    'src/loader_exe.c',
    'src/inject.c',
    launcher_p_res_files,
    include_directories: include_dir,
    name_prefix: '',
    link_args: '-municode'
)

# Dll that will be injected into the launcher
shared_library(
    'launcher_payload',
    'src/dll.c',
    'src/inject.c',
    game_p_res_files,
    include_directories: include_dir,
    name_prefix: '',
    link_args: '-municode'
)
