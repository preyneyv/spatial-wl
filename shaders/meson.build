# Use xxd to create .h files from .glsl files
xxd_exe = find_program('xxd')

shaders_src = [
    files('shaders.c')
]

foreach name : [
    'hello_world_fragment',
    'hello_world_vertex',
]
    shaders_src += custom_target(
        command: [xxd_exe, '-i', '-n', 'shader_@BASENAME@', '@INPUT@', '@OUTPUT@'],
        input: name + '.glsl',
        output: name + '.h'
    )
endforeach
