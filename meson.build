project('harmonic', 'c', version: '0.3.0')

nasm = find_program('nasm')
gen_res = find_program('gen_resources.sh')

# Generator for compiling asm files
asm_gen = generator(
    nasm,
    output: '@BASENAME@.bin',
    arguments: [
        '@EXTRA_ARGS@',
        '-f', 'bin',
        '@INPUT@',
        '-o', '@OUTPUT@'
    ]
)

# Payload that gets injected into the game
subdir('game_payload')

# The injector exes and dlls
subdir('injector')
