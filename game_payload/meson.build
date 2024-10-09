fs = import('fs')

include_dir = include_directories('include')

# Input files
sources = [
    'src/main.c',
    'src/pe.c',
    'src/utils.c',
    'src/msg.c',
    'src/ace.c'
]

if fs.exists('src/core.c')
    # Compile the real file first (dirty hack)
    core_fake_exe = executable(
        'core.o',
        'src/core.c',
        link_args: [ '-r' ], # Output an object file
        include_directories: include_dir
    )
    
    # another dirty hack
    copy_core = find_program('copy_core.sh')

    core_target = [custom_target(
        'copy_core',
        output: 'core.o',
        input: core_fake_exe.extract_all_objects(recursive: false),
        command: [ 
            copy_core, 
            '@INPUT0@',
            '@OUTPUT0@', meson.current_source_dir() / 'blob/core.o' 
        ]
    )]
    core_blob = []
else
    message('Using precompiled core blob. Refer to the readme for more details')
    core_target = []
    core_blob = [ 'blob/core.o' ]
endif

lzma_include_dir = include_directories('lzma_sdk')

lzma_sources = [
    'lzma_sdk/LzmaDec.c'
]

conf_data = configuration_data()
conf_data.set('version', meson.project_version())

conf = configure_file(input: 'include/config.h.in', output: 'config.h', configuration: conf_data)

shared_library(
    'game_payload',
    sources,
    core_target,
    lzma_sources,
    conf,
    objects: core_blob,
    include_directories: [ include_dir, lzma_include_dir ],
    name_prefix: ''
)
